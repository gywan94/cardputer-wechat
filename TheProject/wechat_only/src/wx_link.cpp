#include "wx_link.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mbedtls/base64.h>

namespace wx {


constexpr const char *kPrefNs = "wxilk";
constexpr const char *kKeyTok = "token";
constexpr const char *kKeyHost = "host";
constexpr const char *kKeyLuid = "luid";
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

static void persistLastUserId()
{
    Preferences p;
    if (!p.begin(kPrefNs, false))
        return;
    if (s_last_uid[0])
        p.putString(kKeyLuid, s_last_uid);
    else
        p.remove(kKeyLuid);
    p.end();
}

void setLastUserId(const char *uid)
{
    if (!uid || !uid[0])
    {
        s_last_uid[0] = 0;
        persistLastUserId();
        return;
    }
    strlcpy(s_last_uid, uid, sizeof(s_last_uid));
    persistLastUserId();
}

void loadPrefs()
{
    Preferences p;
    if (!p.begin(kPrefNs, true))
        return;
    String t = p.getString(kKeyTok, "");
    String h = p.getString(kKeyHost, kDefaultHost);
    String lu = p.getString(kKeyLuid, "");
    p.end();
    s_token[0] = 0;
    s_api_host[0] = 0;
    s_last_uid[0] = 0;
    if (t.length())
        strlcpy(s_token, t.c_str(), sizeof(s_token));
    if (h.length())
        strlcpy(s_api_host, h.c_str(), sizeof(s_api_host));
    else
        strlcpy(s_api_host, kDefaultHost, sizeof(s_api_host));
    if (t.length() && lu.length())
        strlcpy(s_last_uid, lu.c_str(), sizeof(s_last_uid));
    genUin();
}

void applyCredsFromStrings(const char *tok, const char *host, bool persistNvs)
{
    if (tok && tok[0])
        strlcpy(s_token, tok, sizeof(s_token));
    if (host && host[0])
        strlcpy(s_api_host, host, sizeof(s_api_host));
    else if (!s_api_host[0])
        strlcpy(s_api_host, kDefaultHost, sizeof(s_api_host));
    genUin();
    if (persistNvs && s_token[0])
        saveCreds();
}

const char *tokenStr()
{
    return s_token;
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
    setLastUserId(fromUserId);

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


int pairState() { return (int)s_pairState; }
const char* apiHost() { return s_api_host; }
const char* lastUserId() { return s_last_uid; }
}
