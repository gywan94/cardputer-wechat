/*
 * WeChat iLink 客户端：扫码绑定 + 长轮询收消息 + 发文字。
 * HTTPS / JSON / iLink 路径与请求头参考 M5Claw（GPL-3.0）
 * https://github.com/fwz233-RE/M5Claw — wechat_bot.cpp
 */

#include "WeChatILink.h"
#include "../MyOS.h"
#include "ExtraMenu.h"
#include "WIFI_HANDLE.h"

#include <M5GFX.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mbedtls/base64.h>

namespace {

constexpr const char *kPrefNs = "wxilk";
constexpr const char *kKeyTok = "token";
constexpr const char *kKeyHost = "host";
constexpr const char *kDefaultHost = "ilinkai.weixin.qq.com";

constexpr const char *kApiGetUpdates = "/ilink/bot/getupdates";
constexpr const char *kApiSendMsg = "/ilink/bot/sendmessage";
constexpr const char *kQrPath = "/ilink/bot/get_bot_qrcode?bot_type=3";
constexpr const char *kQrStatus = "/ilink/bot/get_qrcode_status?qrcode=";

constexpr size_t kTokMax = 512;
constexpr size_t kHostMax = 128;
constexpr size_t kUpdateBuf = 512;
constexpr size_t kUinB64 = 24;
constexpr size_t kDedup = 32;
constexpr size_t kCtxEntries = 6;
constexpr int kDispQueue = 6;
constexpr size_t kMaxChunk = 2048;
constexpr int kTaskStack = 10 * 1024;
constexpr int kTaskPrio = 5;

char s_token[kTokMax];
char s_api_host[kHostMax];
char s_uin_b64[kUinB64];
char s_update_buf[kUpdateBuf];
char s_last_uid[128];
uint32_t s_msg_seq = 0;

struct CtxEntry
{
    char userId[128];
    char contextToken[256];
};
CtxEntry s_ctx[kCtxEntries];

char *s_dispQ[kDispQueue] = {};
volatile int s_dispH = 0;
volatile int s_dispT = 0;

uint64_t s_seen[kDedup];
size_t s_seenIdx = 0;

TaskHandle_t s_pollTask = nullptr;
volatile bool s_pollRun = false;

void genUin()
{
    uint32_t r = esp_random();
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)s_uin_b64, sizeof(s_uin_b64), &olen,
                          (const unsigned char *)&r, sizeof(r));
    s_uin_b64[olen] = '\0';
}

uint64_t fnv1a64(const char *s)
{
    uint64_t h = 1469598103934665603ULL;
    if (!s)
        return h;
    while (*s)
    {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

bool dedup(const char *key)
{
    uint64_t k = fnv1a64(key);
    for (size_t i = 0; i < kDedup; i++)
    {
        if (s_seen[i] == k)
            return true;
    }
    s_seen[s_seenIdx] = k;
    s_seenIdx = (s_seenIdx + 1) % kDedup;
    return false;
}

void rememberCtx(const char *userId, const char *contextToken)
{
    if (!userId || !userId[0] || !contextToken || !contextToken[0])
        return;
    int freeIdx = -1;
    for (int i = 0; i < (int)(sizeof(s_ctx) / sizeof(s_ctx[0])); i++)
    {
        if (strcmp(s_ctx[i].userId, userId) == 0)
        {
            strlcpy(s_ctx[i].contextToken, contextToken, sizeof(s_ctx[i].contextToken));
            return;
        }
        if (freeIdx < 0 && s_ctx[i].userId[0] == '\0')
            freeIdx = i;
    }
    int idx = (freeIdx >= 0) ? freeIdx : (int)(esp_random() % (sizeof(s_ctx) / sizeof(s_ctx[0])));
    strlcpy(s_ctx[idx].userId, userId, sizeof(s_ctx[idx].userId));
    strlcpy(s_ctx[idx].contextToken, contextToken, sizeof(s_ctx[idx].contextToken));
}

const char *findCtx(const char *userId)
{
    if (!userId || !userId[0])
        return "";
    for (const auto &e : s_ctx)
    {
        if (strcmp(e.userId, userId) == 0)
            return e.contextToken;
    }
    return "";
}

bool resolveHost(const char *host, IPAddress &ip)
{
    if (WiFi.status() != WL_CONNECTED)
        return false;
    for (int i = 0; i < 2; i++)
    {
        if (WiFi.hostByName(host, ip))
            return true;
        delay(100);
    }
    return false;
}

int findJsonStart(const String &resp)
{
    int i = resp.indexOf('{');
    return i >= 0 ? i : -1;
}

void enqueueDisp(const char *text)
{
    if (!text || !text[0])
        return;
    int next = (s_dispH + 1) % kDispQueue;
    if (next == s_dispT)
    {
        free(s_dispQ[s_dispT]);
        s_dispQ[s_dispT] = nullptr;
        s_dispT = (s_dispT + 1) % kDispQueue;
    }
    s_dispQ[s_dispH] = strdup(text);
    s_dispH = next;
}

bool hasDisp()
{
    return s_dispH != s_dispT;
}

char *takeDisp()
{
    if (s_dispH == s_dispT)
        return nullptr;
    char *t = s_dispQ[s_dispT];
    s_dispQ[s_dispT] = nullptr;
    s_dispT = (s_dispT + 1) % kDispQueue;
    return t;
}

String httpsPostRaw(const char *host, const char *path, const char *body, const char *authTok, int timeoutMs)
{
    if (WiFi.status() != WL_CONNECTED)
        return "";

    WiFiClientSecure *client = new WiFiClientSecure();
    if (!client)
        return "";
    client->setInsecure();
    client->setTimeout(timeoutMs / 1000);

    IPAddress ip;
    if (!resolveHost(host, ip))
    {
        delete client;
        return "";
    }
    if (!client->connect(host, 443))
    {
        delete client;
        return "";
    }

    client->printf("POST %s HTTP/1.0\r\n", path);
    client->printf("Host: %s\r\n", host);
    client->println("Content-Type: application/json");
    if (authTok && authTok[0])
    {
        client->println("AuthorizationType: ilink_bot_token");
        client->printf("Authorization: Bearer %s\r\n", authTok);
        client->printf("X-WECHAT-UIN: %s\r\n", s_uin_b64);
    }
    client->printf("Content-Length: %d\r\n", (int)strlen(body));
    client->println();
    client->print(body);

    unsigned long deadline = millis() + (unsigned)timeoutMs;
    int state = 0;
    bool headersDone = false;
    while (client->connected() && millis() < deadline)
    {
        if (!client->available())
        {
            delay(1);
            continue;
        }
        char c = client->read();
        switch (state)
        {
        case 0:
            state = (c == '\r') ? 1 : (c == '\n') ? 2 : 0;
            break;
        case 1:
            state = (c == '\n') ? 2 : (c == '\r') ? 1 : 0;
            break;
        case 2:
            if (c == '\n')
                headersDone = true;
            state = (c == '\r') ? 3 : 0;
            break;
        case 3:
            if (c == '\n')
                headersDone = true;
            state = 0;
            break;
        }
        if (headersDone)
            break;
    }

    String resp;
    resp.reserve(4096);
    while (millis() < deadline)
    {
        if (client->available())
        {
            char c = client->read();
            if (resp.length() < 8192)
                resp += c;
        }
        else if (!client->connected())
            break;
        else
            delay(1);
    }
    client->stop();
    delete client;
    return resp;
}

String httpsGet(const char *host, const char *path, int timeoutMs = 15000)
{
    if (WiFi.status() != WL_CONNECTED)
        return "";

    WiFiClientSecure *client = new WiFiClientSecure();
    if (!client)
        return "";
    client->setInsecure();
    client->setTimeout(timeoutMs / 1000);

    IPAddress ip;
    if (!resolveHost(host, ip))
    {
        delete client;
        return "";
    }
    if (!client->connect(host, 443))
    {
        delete client;
        return "";
    }

    client->printf("GET %s HTTP/1.0\r\n", path);
    client->printf("Host: %s\r\n", host);
    client->println("iLink-App-ClientVersion: 1");
    client->println();

    unsigned long deadline = millis() + (unsigned)timeoutMs;
    int st = 0;
    bool headersDone = false;
    while (client->connected() && millis() < deadline)
    {
        if (!client->available())
        {
            delay(1);
            continue;
        }
        char c = client->read();
        switch (st)
        {
        case 0:
            st = (c == '\r') ? 1 : (c == '\n') ? 2 : 0;
            break;
        case 1:
            st = (c == '\n') ? 2 : (c == '\r') ? 1 : 0;
            break;
        case 2:
            if (c == '\n')
                headersDone = true;
            st = (c == '\r') ? 3 : 0;
            break;
        case 3:
            if (c == '\n')
                headersDone = true;
            st = 0;
            break;
        }
        if (headersDone)
            break;
    }

    String resp;
    resp.reserve(2048);
    while (millis() < deadline)
    {
        if (client->available())
        {
            char c = client->read();
            if (resp.length() < 4096)
                resp += c;
        }
        else if (!client->connected())
            break;
        else
            delay(1);
    }
    client->stop();
    delete client;
    return resp;
}

void loadPrefs()
{
    Preferences p;
    if (!p.begin(kPrefNs, true))
        return;
    String t = p.getString(kKeyTok, "");
    String h = p.getString(kKeyHost, kDefaultHost);
    p.end();
    s_token[0] = 0;
    s_api_host[0] = 0;
    if (t.length())
        strlcpy(s_token, t.c_str(), sizeof(s_token));
    if (h.length())
        strlcpy(s_api_host, h.c_str(), sizeof(s_api_host));
    else
        strlcpy(s_api_host, kDefaultHost, sizeof(s_api_host));
    genUin();
}

void saveCreds()
{
    Preferences p;
    if (!p.begin(kPrefNs, false))
        return;
    p.putString(kKeyTok, s_token);
    p.putString(kKeyHost, s_api_host);
    p.end();
}

void clearCreds()
{
    Preferences p;
    if (p.begin(kPrefNs, false))
    {
        p.clear();
        p.end();
    }
    s_token[0] = 0;
    strlcpy(s_api_host, kDefaultHost, sizeof(s_api_host));
    memset(s_ctx, 0, sizeof(s_ctx));
    s_last_uid[0] = 0;
    s_update_buf[0] = 0;
}

bool isPaired()
{
    return s_token[0] && s_api_host[0];
}

void handleIncoming(JsonObject msg)
{
    int msgType = msg["message_type"] | 0;
    if (msgType != 1)
        return;

    const char *fromUserId = msg["from_user_id"] | "";
    const char *contextToken = msg["context_token"] | "";
    int messageId = msg["message_id"] | 0;
    if (!fromUserId[0])
        return;

    rememberCtx(fromUserId, contextToken);
    strlcpy(s_last_uid, fromUserId, sizeof(s_last_uid));

    char dedupKey[64];
    snprintf(dedupKey, sizeof(dedupKey), "%s_%d", fromUserId, messageId);
    if (messageId && dedup(dedupKey))
        return;

    JsonArray items = msg["item_list"];
    if (items.isNull())
        return;

    String messageText;
    for (JsonVariant itemVar : items)
    {
        JsonObject item = itemVar.as<JsonObject>();
        int type = item["type"] | 0;
        if (type != 1)
            continue;
        JsonObject textItem = item["text_item"];
        if (textItem.isNull())
            continue;
        const char *text = textItem["text"] | "";
        if (!text[0])
            continue;
        if (messageText.length())
            messageText += "\n";
        messageText += text;
    }
    if (messageText.length() == 0)
        return;

    String line = String("[微信] ") + messageText;
    enqueueDisp(line.c_str());
}

void pollTask(void *)
{
    int failCount = 0;
    while (s_pollRun)
    {
        if (WiFi.status() != WL_CONNECTED || !s_token[0] || !s_api_host[0])
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        JsonDocument reqDoc;
        reqDoc["get_updates_buf"] = s_update_buf;
        reqDoc["base_info"]["channel_version"] = "1.0.2";
        String body;
        serializeJson(reqDoc, body);

        String resp = httpsPostRaw(s_api_host, kApiGetUpdates, body.c_str(), s_token, 45000);
        if (resp.length() == 0)
        {
            int backoffSec = (failCount < 3) ? 1 : (failCount <= 8 ? 15 : 60);
            vTaskDelay(pdMS_TO_TICKS(backoffSec * 1000));
            failCount++;
            continue;
        }

        int jsonStart = findJsonStart(resp);
        if (jsonStart < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        JsonDocument respDoc;
        if (deserializeJson(respDoc, resp.c_str() + jsonStart))
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        int errcode = respDoc["errcode"] | 0;
        if (errcode == -14)
        {
            vTaskDelay(pdMS_TO_TICKS(30000));
            failCount++;
            continue;
        }

        int ret = respDoc["ret"] | 0;
        if (ret != 0)
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
            failCount++;
            continue;
        }

        failCount = 0;
        const char *newBuf = respDoc["get_updates_buf"] | "";
        if (newBuf[0])
            strlcpy(s_update_buf, newBuf, sizeof(s_update_buf));

        JsonArray msgs = respDoc["msgs"];
        if (!msgs.isNull())
        {
            for (JsonVariant mv : msgs)
            {
                JsonObject msg = mv.as<JsonObject>();
                handleIncoming(msg);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    s_pollTask = nullptr;
    vTaskDelete(nullptr);
}

void stopPollTask()
{
    s_pollRun = false;
    unsigned long t = millis();
    while (s_pollTask && millis() - t < 6000)
        delay(50);
}

void startPollTask()
{
    if (s_pollTask || !isPaired())
        return;
    s_pollRun = true;
    BaseType_t ok = xTaskCreatePinnedToCore(pollTask, "wx_ilink", kTaskStack, nullptr, kTaskPrio, &s_pollTask, 0);
    if (ok != pdPASS)
    {
        s_pollRun = false;
        s_pollTask = nullptr;
    }
}

bool sendWxMessage(const char *userId, const char *text)
{
    if (!isPaired() || !userId || !userId[0] || !text)
        return false;
    size_t textLen = strlen(text);
    size_t offset = 0;
    bool allOk = true;
    while (offset < textLen)
    {
        size_t chunk = textLen - offset;
        if (chunk > kMaxChunk)
            chunk = kMaxChunk;

        char clientId[40];
        snprintf(clientId, sizeof(clientId), "aos-%lu-%u", millis(), ++s_msg_seq);

        JsonDocument doc;
        JsonObject msg = doc["msg"].to<JsonObject>();
        msg["from_user_id"] = "";
        msg["to_user_id"] = userId;
        msg["client_id"] = clientId;
        msg["message_type"] = 2;
        msg["message_state"] = 2;
        const char *ctx = findCtx(userId);
        if (ctx[0])
            msg["context_token"] = ctx;
        JsonArray itemList = msg["item_list"].to<JsonArray>();
        JsonObject item = itemList.add<JsonObject>();
        item["type"] = 1;
        JsonObject textItem = item["text_item"].to<JsonObject>();
        textItem["text"] = String(text + offset, chunk);
        doc["base_info"]["channel_version"] = "1.0.2";

        String body;
        serializeJson(doc, body);
        String resp = httpsPostRaw(s_api_host, kApiSendMsg, body.c_str(), s_token, 45000);
        if (resp.length() == 0)
        {
            allOk = false;
            break;
        }
        int js = findJsonStart(resp);
        if (js < 0)
        {
            allOk = false;
            break;
        }
        JsonDocument rdoc;
        if (deserializeJson(rdoc, resp.c_str() + js))
        {
            allOk = false;
            break;
        }
        int rret = rdoc["ret"] | 0;
        if (rret != 0)
            allOk = false;
        offset += chunk;
    }
    return allOk;
}

// --- 扫码配对（与 M5Claw 相同 API）---
enum PairState
{
    PAIR_IDLE,
    PAIR_WAITING,
    PAIR_OK,
    PAIR_FAIL
};
PairState s_pairState = PAIR_IDLE;
char s_pairQrUrl[512];
char s_pairQrCode[256];
char s_pairHost[128];

const char *startPairing()
{
    s_pairQrUrl[0] = 0;
    s_pairQrCode[0] = 0;
    const char *host = s_api_host[0] ? s_api_host : kDefaultHost;
    strlcpy(s_pairHost, host, sizeof(s_pairHost));

    String resp = httpsGet(host, kQrPath, 15000);
    if (resp.length() == 0)
    {
        s_pairState = PAIR_FAIL;
        return nullptr;
    }
    int js = findJsonStart(resp);
    if (js < 0)
    {
        s_pairState = PAIR_FAIL;
        return nullptr;
    }
    JsonDocument rdoc;
    if (deserializeJson(rdoc, resp.c_str() + js))
    {
        s_pairState = PAIR_FAIL;
        return nullptr;
    }
    const char *qrImgContent = rdoc["qrcode_img_content"] | (const char *)nullptr;
    const char *qrCode = rdoc["qrcode"] | "";
    if (!qrImgContent || !qrImgContent[0])
    {
        s_pairState = PAIR_FAIL;
        return nullptr;
    }
    strlcpy(s_pairQrUrl, qrImgContent, sizeof(s_pairQrUrl));
    strlcpy(s_pairQrCode, qrCode, sizeof(s_pairQrCode));
    s_pairState = PAIR_WAITING;
    return s_pairQrUrl;
}

bool pollPairing()
{
    if (s_pairState != PAIR_WAITING || !s_pairQrCode[0])
        return false;

    char path[384];
    snprintf(path, sizeof(path), "%s%s", kQrStatus, s_pairQrCode);

    String resp = httpsGet(s_pairHost, path, 40000);
    if (resp.length() == 0)
        return false;

    int js = findJsonStart(resp);
    if (js < 0)
        return false;

    JsonDocument rdoc;
    if (deserializeJson(rdoc, resp.c_str() + js))
        return false;

    const char *status = rdoc["status"] | "wait";
    if (strcmp(status, "expired") == 0)
    {
        s_pairState = PAIR_FAIL;
        return false;
    }
    if (strcmp(status, "confirmed") != 0)
        return false;

    const char *botToken = rdoc["bot_token"] | "";
    const char *baseUrl = rdoc["baseurl"] | "";
    if (!botToken[0])
    {
        s_pairState = PAIR_FAIL;
        return false;
    }

    strlcpy(s_token, botToken, sizeof(s_token));
    if (baseUrl[0])
    {
        String host = baseUrl;
        if (host.startsWith("https://"))
            host = host.substring(8);
        if (host.startsWith("http://"))
            host = host.substring(7);
        int slash = host.indexOf('/');
        if (slash > 0)
            host = host.substring(0, slash);
        strlcpy(s_api_host, host.c_str(), sizeof(s_api_host));
    }
    else
        strlcpy(s_api_host, s_pairHost, sizeof(s_api_host));

    saveCreds();
    genUin();
    s_pairState = PAIR_OK;
    return true;
}

void cancelPairing()
{
    s_pairState = PAIR_IDLE;
    s_pairQrUrl[0] = 0;
    s_pairQrCode[0] = 0;
}

} // namespace

// --- WeChatILink UI ---

void WeChatILink::Begin()
{
    TopOffset = 0;
    showTopBar = false;
    chatLines.clear();
    chatScroll = 0;
    inputLine = "";
    pairStarted = false;
    qrPayload = nullptr;
    lastPairPollMs = 0;
    loadPrefs();

    if (WiFi.status() != WL_CONNECTED)
    {
        mode = UI_NEED_WIFI;
        appendChatLine("", "未连接 Wi-Fi。请先联网（例如主菜单 Extra 里可打开 WIFI 工具）。");
    }
    else if (isPaired())
    {
        mode = UI_CHAT;
        startPollTask();
        appendChatLine("", "已绑定。用微信发消息到机器人，在此回复。");
    }
    else
    {
        mode = UI_PAIR_QR;
    }
    drawFullSprite();
}

void WeChatILink::OnExit()
{
    stopPollTask();
    cancelPairing();
    while (char *p = takeDisp())
        free(p);
    mainOS->sprite.unloadFont();
}

void WeChatILink::pushUtf8Wrapped(const String &text)
{
    const int maxLinesTotal = 100;
    const int maxCpPerLine = 18;

    auto pushOneLine = [&](const String &line) {
        if (line.length() == 0)
            return;
        while ((int)chatLines.size() >= maxLinesTotal)
            chatLines.erase(chatLines.begin());
        chatLines.push_back(line);
    };

    int segStart = 0;
    const int n = text.length();
    for (int e = 0; e <= n; e++)
    {
        if (e < n && text.charAt(e) != '\n')
            continue;

        String seg = text.substring(segStart, e);
        segStart = e + 1;

        int i = 0;
        while (i < seg.length())
        {
            int lineStart = i;
            int cps = 0;
            while (i < seg.length() && cps < maxCpPerLine)
            {
                uint8_t b = (uint8_t)seg.charAt(i);
                int nb = 1;
                if (b < 0x80u)
                    nb = 1;
                else if ((b & 0xE0u) == 0xC0u)
                    nb = 2;
                else if ((b & 0xF0u) == 0xE0u)
                    nb = 3;
                else if ((b & 0xF8u) == 0xF0u)
                    nb = 4;
                else
                {
                    i++;
                    continue;
                }
                if (i + nb > seg.length())
                    break;
                i += nb;
                cps++;
            }
            pushOneLine(seg.substring(lineStart, i));
        }
    }
}

void WeChatILink::appendChatLine(const char *prefix, const String &text)
{
    String combined = text;
    if (prefix && prefix[0])
        combined = String(prefix) + text;
    pushUtf8Wrapped(combined);
}

void WeChatILink::startPairFlow()
{
    cancelPairing();
    pairStarted = true;
    qrPayload = startPairing();
    lastPairPollMs = millis();
    if (!qrPayload)
        appendChatLine("", "获取二维码失败，按 Enter 重试。");
}

void WeChatILink::tryPollPairing()
{
    if (millis() - lastPairPollMs < 2500)
        return;
    lastPairPollMs = millis();
    if (pollPairing())
    {
        pairStarted = false;
        qrPayload = nullptr;
        mode = UI_CHAT;
        chatLines.clear();
        chatScroll = 0;
        appendChatLine("", "绑定成功。");
        startPollTask();
    }
    else if (s_pairState == PAIR_FAIL)
    {
        pairStarted = true;
        qrPayload = nullptr;
        appendChatLine("", "二维码失效或未确认，按 Enter 重新获取。");
    }
}

void WeChatILink::drawFullSprite()
{
    auto &d = mainOS->sprite;
    const int H = SCREEN_H - TopOffset;
    d.createSprite(SCREEN_W, H);
    d.fillSprite(TFT_BLACK);
    d.unloadFont();
    d.setFont(&fonts::lgfxJapanGothic_12);
    d.setTextSize(1);
    d.setAttribute(lgfx::UTF8_SWITCH, 1u);
    d.setTextWrap(false);
    d.setTextColor(TFT_WHITE, TFT_BLACK);

    if (mode == UI_NEED_WIFI)
    {
        d.setCursor(4, 4);
        d.println("WeChat iLink");
        d.setTextColor(TFT_CYAN, TFT_BLACK);
        int ly = 20;
        for (size_t i = 0; i < chatLines.size(); i++)
        {
            d.setCursor(4, ly);
            d.print(chatLines[i]);
            ly += d.fontHeight() + 2;
            if (ly > H - 28)
                break;
        }
        d.setTextColor(TFT_YELLOW, TFT_BLACK);
        d.setCursor(4, H - d.fontHeight() * 2 - 4);
        d.println("` 返回");
        d.setCursor(4, H - d.fontHeight() - 2);
        d.println("Enter: WiFi");
    }
    else if (mode == UI_PAIR_QR)
    {
        d.fillRect(0, 0, SCREEN_W, 16, TFT_DARKGREY);
        d.setTextColor(TFT_WHITE, TFT_DARKGREY);
        d.setCursor(4, 3);
        d.print("`返回 Enter重扫");

        if (qrPayload && qrPayload[0])
        {
            int qrSz = H - 24;
            if (qrSz > 108)
                qrSz = 108;
            d.qrcode(qrPayload, 2, 18, qrSz, 6);
            d.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
            d.setCursor(qrSz + 8, 28);
            d.println("微信");
            d.setCursor(qrSz + 8, 28 + d.fontHeight() + 2);
            d.println("扫一扫");
            d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            d.setCursor(qrSz + 8, 28 + (d.fontHeight() + 2) * 3);
            d.println("等待绑定");
        }
        else
        {
            d.setCursor(6, 36);
            d.setTextColor(TFT_RED, TFT_BLACK);
            d.println("无二维码");
            int row = 0;
            for (int j = (int)chatLines.size() - 1; j >= 0 && row < 4; j--, row++)
            {
                d.setTextColor(TFT_SILVER, TFT_BLACK);
                d.setCursor(6, 52 + row * (d.fontHeight() + 2));
                d.print(chatLines[(size_t)j]);
            }
        }
    }
    else
    {
        d.fillRect(0, 0, SCREEN_W, 15, TFT_DARKGREEN);
        d.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        d.setCursor(4, 2);
        d.print("已连接 ");
        String hostShort = String(s_api_host);
        if (hostShort.length() > 18)
            hostShort = hostShort.substring(0, 16) + "..";
        d.print(hostShort);

        const int lineH = d.fontHeight() + 2;
        const int areaTop = 18;
        const int hintH = d.fontHeight() + 4;
        const int inputBarH = 24;
        const int chatBottom = H - inputBarH - hintH;
        int maxVis = (chatBottom - areaTop) / lineH;
        if (maxVis < 2)
            maxVis = 2;

        int total = (int)chatLines.size();
        int maxScrollTop = total > maxVis ? total - maxVis : 0;
        if (chatScroll > maxScrollTop)
            chatScroll = maxScrollTop;
        if (chatScroll < 0)
            chatScroll = 0;

        int start = total > maxVis + chatScroll ? total - maxVis - chatScroll : 0;
        int y = areaTop;
        for (int idx = start; idx < total && y + lineH <= chatBottom; idx++)
        {
            bool mine = chatLines[idx].startsWith(">>");
            d.setTextColor(mine ? TFT_GREENYELLOW : TFT_CYAN, TFT_BLACK);
            d.setCursor(4, y);
            d.print(chatLines[idx]);
            y += lineH;
        }

        d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        d.setCursor(4, H - inputBarH - hintH + 1);
        d.print("Tab上翻 Ctrl下翻 Fn+R解绑");

        d.fillRect(0, H - inputBarH, SCREEN_W, inputBarH, TFT_DARKGREY);
        d.setTextColor(TFT_WHITE, TFT_DARKGREY);
        d.setCursor(4, H - inputBarH + 5);
        d.print(">");
        {
            String showIn = inputLine;
            while (showIn.length() && d.textWidth(showIn.c_str()) > 210)
            {
                int cut = 1;
                uint8_t b = (uint8_t)showIn.charAt(0);
                if ((b & 0xF0u) == 0xE0u)
                    cut = 3;
                else if ((b & 0xE0u) == 0xC0u)
                    cut = 2;
                else if ((b & 0xF8u) == 0xF0u)
                    cut = 4;
                showIn = showIn.substring(cut);
            }
            d.print(showIn);
            d.print(cursorOn ? "|" : " ");
        }
    }

    d.pushSprite(0, TopOffset);
    d.deleteSprite();
}

void WeChatILink::handleChatKeys()
{
    if (!M5Cardputer.Keyboard.isChange())
        return;
    if (!M5Cardputer.Keyboard.isPressed())
        return;

    Keyboard_Class::KeysState st = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && !st.word.empty())
    {
        for (char c : st.word)
        {
            if (c == 'r' || c == 'R')
            {
                stopPollTask();
                clearCreds();
                cancelPairing();
                pairStarted = false;
                qrPayload = nullptr;
                chatLines.clear();
                chatScroll = 0;
                mode = UI_PAIR_QR;
                mainOS->ShowOnScreenMessege("已清除绑定", 800);
                return;
            }
        }
    }

    if (st.tab || st.ctrl || st.alt)
        return;

    if (st.enter)
    {
        if (inputLine.length())
        {
            if (!s_last_uid[0])
            {
                appendChatLine("", "[系统] 请让对方先发来一条微信");
            }
            else
            {
                bool ok = sendWxMessage(s_last_uid, inputLine.c_str());
                appendChatLine("", String(">> ") + inputLine);
                if (!ok)
                    appendChatLine("", "[系统] 发送失败");
            }
            inputLine = "";
        }
        return;
    }

    if (st.del && inputLine.length())
        inputLine.remove(inputLine.length() - 1);

    if (st.space && inputLine.length() < 500)
        inputLine += ' ';

    for (char c : st.word)
    {
        if ((uint8_t)c < 32u)
            continue;
        if (inputLine.length() >= 500)
            break;
        inputLine += c;
    }
}

void WeChatILink::Loop()
{
    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new Extra(mainOS));
        return;
    }

    if (WiFi.status() != WL_CONNECTED && mode != UI_NEED_WIFI)
    {
        stopPollTask();
        mode = UI_NEED_WIFI;
        chatLines.clear();
        chatScroll = 0;
        appendChatLine("", "Wi-Fi 已断开。");
        pairStarted = false;
        qrPayload = nullptr;
    }

    if (mode == UI_NEED_WIFI)
    {
        if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
        {
            mainOS->ChangeMenu(new Wifi_handle(mainOS));
            return;
        }
        drawFullSprite();
        return;
    }

    if (mode == UI_PAIR_QR)
    {
        if (!pairStarted)
            startPairFlow();

        if (pairStarted && s_pairState == PAIR_WAITING)
            tryPollPairing();

        if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
        {
            chatLines.clear();
            pairStarted = false;
            startPairFlow();
        }
        drawFullSprite();
        return;
    }

    // CHAT
    while (hasDisp())
    {
        char *t = takeDisp();
        if (t)
        {
            appendChatLine("", String(t));
            free(t);
            chatScroll = 0;
        }
    }

    if (mainOS->NewKey.ifKeyJustPress((char)KEY_TAB))
    {
        int lineH = 14;
        int areaH = SCREEN_H - 18 - 12 - 24;
        int maxVis = areaH / lineH;
        if (maxVis < 2)
            maxVis = 2;
        int total = (int)chatLines.size();
        int maxTop = total > maxVis ? total - maxVis : 0;
        if (chatScroll < maxTop)
            chatScroll++;
    }
    if (mainOS->NewKey.ifKeyJustPress((char)KEY_LEFT_CTRL))
    {
        if (chatScroll > 0)
            chatScroll--;
    }

    if (millis() - cursorBlinkMs > 450)
    {
        cursorBlinkMs = millis();
        cursorOn = !cursorOn;
    }

    handleChatKeys();
    drawFullSprite();
}

void WeChatILink::Draw() {}
