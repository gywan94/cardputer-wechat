/*
 * 仅微信 iLink 的精简固件（无 AdvanceOS 其它功能）。
 * 中文显示与 M5Claw chat.cpp 一致：fonts::efontCN_12 + UTF-8 按像素换行 (fitBytes)。
 */
#include <M5Cardputer.h>
#include <M5GFX.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>
#include <cstring>

#include "wx_link.h"

static constexpr int SD_SPI_SCK_PIN = 40;
static constexpr int SD_SPI_MISO_PIN = 39;
static constexpr int SD_SPI_MOSI_PIN = 14;
static constexpr int SD_SPI_CS_PIN = 12;
static const char *kSdWifiPath = "/wx_store/wifi.txt";
static const char *kSdIlinkPath = "/wx_store/ilink.txt";
static const char *kSdStoreDir = "/wx_store";

static bool g_sd_ok = false;
static bool g_spi_sd_inited = false;

static constexpr int SW = 240;
static constexpr int SH = 135;

// ---- M5Claw chat.cpp：UTF-8 安全按宽度折行 ----
template <typename T>
static int fitBytes(T &canvas, const char *start, int len, int maxW, char *buf, int bufSize)
{
    if (len == 0)
        return 0;
    int tryLen = (len < bufSize - 1) ? len : bufSize - 1;
    memcpy(buf, start, tryLen);
    buf[tryLen] = '\0';
    if (canvas.textWidth(buf) <= maxW)
        return tryLen;
    int lo = 1, hi = tryLen;
    int best = 1;
    while (lo <= hi)
    {
        int mid = (lo + hi) / 2;
        memcpy(buf, start, mid);
        buf[mid] = '\0';
        if (canvas.textWidth(buf) <= maxW)
        {
            best = mid;
            lo = mid + 1;
        }
        else
            hi = mid - 1;
    }
    while (best > 0 && (start[best] & 0xC0) == 0x80)
        best--;
    if (best == 0)
        best = 1;
    return best;
}

template <typename T>
static void applyCnFont(T &d)
{
    d.unloadFont();
    d.setFont(&fonts::efontCN_12);
    d.setTextSize(1);
    d.setAttribute(lgfx::UTF8_SWITCH, 1u);
    d.setTextWrap(false);
}

static void pushWrappedLines(std::vector<String> &lines, const String &text, int maxW)
{
    M5Canvas measure(&M5Cardputer.Display);
    measure.createSprite(240, 32);
    applyCnFont(measure);
    char buf[256];
    const int maxTotal = 120;
    int segStart = 0;
    const int n = text.length();
    for (int e = 0; e <= n; e++)
    {
        if (e < n && text.charAt(e) != '\n')
            continue;
        String seg = text.substring(segStart, e);
        segStart = e + 1;
        int pos = 0;
        while (pos < seg.length())
        {
            int fit = fitBytes(measure, seg.c_str() + pos, seg.length() - pos, maxW, buf, sizeof(buf));
            if (fit <= 0)
                fit = 1;
            lines.push_back(seg.substring(pos, pos + fit));
            pos += fit;
            while ((int)lines.size() > maxTotal)
                lines.erase(lines.begin());
        }
    }
    measure.deleteSprite();
}

enum UiMode : uint8_t
{
    UI_WIFI,
    UI_PAIR,
    UI_CHAT
};

static UiMode mode = UI_WIFI;
static std::vector<String> chatLines;
static String inputLine;
static int chatScroll = 0;
static bool pairStarted = false;
static const char *qrPayload = nullptr;
static unsigned long lastPairPollMs = 0;

// 仅在有新事件时 markDraw() 后全屏重绘（无定时刷新、无光标闪烁）
static bool g_needDraw = true;
static bool g_msgBeepEnabled = true;

static inline void markDraw()
{
    g_needDraw = true;
}

static void loadMsgBeepPref()
{
    Preferences p;
    if (!p.begin("wxst", true))
        return;
    g_msgBeepEnabled = p.getUChar("msgb", 1) != 0;
    p.end();
}

static void saveMsgBeepPref()
{
    Preferences p;
    if (!p.begin("wxst", false))
        return;
    p.putUChar("msgb", g_msgBeepEnabled ? 1 : 0);
    p.end();
}

static void ensureWxStoreDir()
{
    if (!g_sd_ok)
        return;
    if (!SD.exists(kSdStoreDir))
        SD.mkdir(kSdStoreDir);
}

static bool mountSd()
{
    if (g_sd_ok)
        return true;
    if (!g_spi_sd_inited)
    {
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        g_spi_sd_inited = true;
    }
    g_sd_ok = SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
    return g_sd_ok;
}

static void sdRemoveIfExists(const char *path)
{
    if (g_sd_ok && SD.exists(path))
        SD.remove(path);
}

static bool loadWifiFromStore(String &ssid, String &pass)
{
    ssid = "";
    pass = "";
    if (g_sd_ok && SD.exists(kSdWifiPath))
    {
        File f = SD.open(kSdWifiPath, FILE_READ);
        if (f)
        {
            while (f.available())
            {
                String line = f.readStringUntil('\n');
                line.trim();
                if (line.startsWith("SSID="))
                    ssid = line.substring(5);
                else if (line.startsWith("PASS="))
                    pass = line.substring(5);
            }
            f.close();
        }
    }
    if (ssid.length() == 0)
    {
        Preferences p;
        if (p.begin("wxst", true))
        {
            ssid = p.getString("ssid", "");
            pass = p.getString("pass", "");
            p.end();
        }
    }
    return ssid.length() > 0;
}

static void persistWifiCreds(const String &ssid, const String &pass)
{
    {
        Preferences p;
        if (p.begin("wxst", false))
        {
            p.putString("ssid", ssid);
            p.putString("pass", pass);
            p.end();
        }
    }
    if (!g_sd_ok)
        return;
    ensureWxStoreDir();
    File f = SD.open(kSdWifiPath, FILE_WRITE);
    if (!f)
        return;
    f.println(String("SSID=") + ssid);
    f.println(String("PASS=") + pass);
    f.close();
}

static void loadIlinkFromSdOverlay()
{
    if (!g_sd_ok || !SD.exists(kSdIlinkPath))
        return;
    File f = SD.open(kSdIlinkPath, FILE_READ);
    if (!f)
        return;
    String tok, host, lastUid;
    while (f.available())
    {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.startsWith("TOKEN="))
            tok = line.substring(6);
        else if (line.startsWith("HOST="))
            host = line.substring(5);
        else if (line.startsWith("LASTUID="))
            lastUid = line.substring(8);
    }
    f.close();
    tok.trim();
    host.trim();
    lastUid.trim();
    if (tok.length())
        wx::applyCredsFromStrings(tok.c_str(), host.length() ? host.c_str() : nullptr, true);
    if (lastUid.length() && wx::isPaired())
        wx::setLastUserId(lastUid.c_str());
}

static void persistIlinkToSd()
{
    if (!g_sd_ok || !wx::isPaired())
        return;
    ensureWxStoreDir();
    File f = SD.open(kSdIlinkPath, FILE_WRITE);
    if (!f)
        return;
    f.println(String("TOKEN=") + String(wx::tokenStr()));
    f.println(String("HOST=") + String(wx::apiHost()));
    if (wx::lastUserId()[0])
        f.println(String("LASTUID=") + String(wx::lastUserId()));
    f.close();
}

static void drawBootSplash(const char *subtitle)
{
    M5GFX &d = M5Cardputer.Display;
    for (int y = 0; y < SH; y++)
    {
        uint8_t r = (uint8_t)(18 + (y * 28) / SH);
        uint8_t g = (uint8_t)(8 + (y * 50) / SH);
        uint8_t b = (uint8_t)(42 + (y * 70) / SH);
        d.drawFastHLine(0, y, SW, d.color565(r, g, b));
    }
    d.fillCircle(198, 28, 26, d.color565(8, 20, 60));
    d.fillCircle(42, 102, 20, d.color565(40, 10, 50));
    d.drawCircle(120, 68, 48, TFT_LIGHTGREY);
    applyCnFont(d);
    d.setTextColor(TFT_WHITE);
    d.setTextDatum(textdatum_t::middle_center);
    d.drawString("WeChat iLink", SW / 2, SH / 2 - 16);
    d.setTextColor(TFT_CYAN);
    d.drawString(subtitle && subtitle[0] ? subtitle : "精简版", SW / 2, SH / 2 + 6);
    d.setTextDatum(textdatum_t::top_left);
}

static void pumpConnectOrScanUi(const char *title)
{
    M5Cardputer.update();
    M5GFX &d = M5Cardputer.Display;
    d.fillScreen(TFT_BLACK);
    applyCnFont(d);
    d.setTextColor(TFT_CYAN, TFT_BLACK);
    d.setCursor(6, 10);
    d.println(title);
    d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    d.setCursor(6, 36);
    int n = (int)(millis() / 200) % 6;
    for (int i = 0; i <= n; i++)
        d.print(".");
    d.setCursor(6, 58);
    d.print("请稍候，设备工作中");
}

/** 收到微信新消息时短提示音（M5Unified Speaker / Cardputer 蜂鸣器） */
static void playIncomingMessageBeep()
{
    if (!g_msgBeepEnabled)
        return;
    auto &spk = M5Cardputer.Speaker;
    if (!spk.isEnabled())
        return;
    if (spk.getVolume() < 48)
        spk.setVolume(120);
    spk.tone(1760.0f, 85u, -1, true);
}

/** 断开正在关联的 STA，避免残留状态导致后续 scanNetworks 异常或一直 0 个热点 */
static void wifiStopConnectForScan()
{
    WiFi.disconnect(true, false);
    delay(120);
    WiFi.mode(WIFI_STA);
    delay(80);
}

static bool attemptConnectSavedWifi()
{
    String ssid, pass;
    if (!loadWifiFromStore(ssid, pass))
        return false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, false);
    delay(80);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000)
    {
        pumpConnectOrScanUi("正在连接已保存的 WiFi");
        delay(120);
    }
    if (WiFi.status() == WL_CONNECTED)
        return true;
    wifiStopConnectForScan();
    return false;
}

// ---- 极简 Wi-Fi ----
static int scanCount = 0;
static int wifiSel = 0;
static int wifiListFirst = 0;
static bool scanDone = false;
static bool scanning = false;
static bool wifiPassMode = false;
static String wifiPassword;

static void wifiEnsureSelVisible(int rowH)
{
    const int topY = 18;
    const int bottomReserve = rowH * 2 + 22;
    int maxRows = (SH - topY - bottomReserve) / rowH;
    if (maxRows < 1)
        maxRows = 1;
    if (scanCount <= maxRows)
    {
        wifiListFirst = 0;
        return;
    }
    if (wifiSel < wifiListFirst)
        wifiListFirst = wifiSel;
    if (wifiSel >= wifiListFirst + maxRows)
        wifiListFirst = wifiSel - maxRows + 1;
    if (wifiListFirst < 0)
        wifiListFirst = 0;
    if (wifiListFirst > scanCount - maxRows)
        wifiListFirst = scanCount - maxRows;
}

static void startWifiScan()
{
    scanning = true;
    scanDone = false;
    wifiListFirst = 0;
    wifiSel = 0;
    markDraw();
    wifiStopConnectForScan();
    WiFi.scanDelete();
    pumpConnectOrScanUi("正在扫描 WiFi");
    // 同步扫描：在已清理 STA 连接状态后最稳定；异步轮询在部分状态下易误判或 0 结果
    int n = WiFi.scanNetworks(false, true, false, 300);
    scanCount = (n < 0) ? 0 : n;
    scanning = false;
    scanDone = true;
    wifiSel = 0;
    wifiListFirst = 0;
    markDraw();
}

static void tryWifiConnect()
{
    if (scanCount <= 0 || wifiSel < 0 || wifiSel >= scanCount)
        return;
    String ssid = WiFi.SSID(wifiSel);
    String pass = wifiPassword;
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 12000)
    {
        pumpConnectOrScanUi("正在连接 WiFi");
        delay(120);
    }
    wifiPassMode = false;
    wifiPassword = "";
    if (WiFi.status() == WL_CONNECTED)
    {
        persistWifiCreds(ssid, pass);
        wx::loadPrefs();
        loadIlinkFromSdOverlay();
        if (wx::isPaired())
        {
            mode = UI_CHAT;
            wx::startPollTask();
            chatLines.clear();
            chatScroll = 0;
            pushWrappedLines(chatLines, "已绑定，可聊天。", 220);
            persistIlinkToSd();
        }
        else
        {
            mode = UI_PAIR;
            pairStarted = false;
            qrPayload = nullptr;
            chatLines.clear();
        }
    }
    else
    {
        pushWrappedLines(chatLines, "Wi-Fi 密码错误或超时。", 220);
        wifiStopConnectForScan();
    }
    markDraw();
}

static void drawAll()
{
    M5GFX &d = M5Cardputer.Display;
    d.fillScreen(TFT_BLACK);
    applyCnFont(d);

    if (mode == UI_WIFI)
    {
        d.setTextColor(TFT_WHITE, TFT_BLACK);
        d.setCursor(4, 2);
        d.println("微信精简版 - WiFi");
        if (scanning)
        {
            d.setCursor(4, 22);
            d.print("扫描中...");
        }
        else if (!scanDone)
        {
            d.setCursor(4, 22);
            d.print("按 Enter 扫描");
        }
        else
        {
            const int rowH = d.fontHeight() + 1;
            wifiEnsureSelVisible(rowH);
            const int topY = 18;
            const int bottomReserve = rowH * 2 + 22;
            int maxRows = (SH - topY - bottomReserve) / rowH;
            if (maxRows < 1)
                maxRows = 1;
            int y = topY;
            for (int k = 0; k < maxRows; k++)
            {
                int i = wifiListFirst + k;
                if (i >= scanCount)
                    break;
                d.setCursor(4, y);
                if (i == wifiSel)
                    d.setTextColor(TFT_YELLOW, TFT_BLACK);
                else
                    d.setTextColor(TFT_CYAN, TFT_BLACK);
                d.printf("%s %s", (i == wifiSel) ? ">" : " ", WiFi.SSID(i).c_str());
                y += rowH;
            }
            d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            d.setCursor(4, SH - d.fontHeight() * 3);
            d.printf("热点 %d/%d", wifiSel + 1, scanCount);
            d.setCursor(4, SH - d.fontHeight() * 2);
            d.println("Ctrl上 Tab下 Enter密码");
            d.setCursor(4, SH - d.fontHeight());
            if (wifiPassMode)
                d.printf("密码:%s", wifiPassword.c_str());
            else
                d.print("Fn+R重扫");
        }
    }
    else if (mode == UI_PAIR)
    {
        d.fillRect(0, 0, SW, 16, TFT_DARKGREY);
        d.setTextColor(TFT_WHITE, TFT_DARKGREY);
        d.setCursor(4, 2);
        d.print("扫码绑定 Enter重试");
        if (qrPayload && qrPayload[0])
        {
            int qrSz = SH - 22;
            if (qrSz > 105)
                qrSz = 105;
            d.qrcode(qrPayload, 2, 18, qrSz, 6);
            d.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
            d.setCursor(qrSz + 6, 24);
            d.println("微信扫一扫");
        }
        else
        {
            d.setTextColor(TFT_RED, TFT_BLACK);
            d.setCursor(6, 40);
            d.println("无二维码");
        }
    }
    else
    {
        d.fillRect(0, 0, SW, 14, TFT_DARKGREEN);
        d.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        d.setCursor(4, 2);
        String h = wx::apiHost();
        if (h.length() > 16)
            h = h.substring(0, 14) + "..";
        d.print("已连接 ");
        d.print(h);

        const int lineH = d.fontHeight() + 2;
        const int top = 16;
        const int hintH = d.fontHeight() + 2;
        const int inH = 22;
        const int bot = SH - inH - hintH;
        int maxVis = (bot - top) / lineH;
        if (maxVis < 2)
            maxVis = 2;
        int total = (int)chatLines.size();
        int maxTop = (total > maxVis) ? (total - maxVis) : 0;
        if (chatScroll > maxTop)
            chatScroll = maxTop;
        if (chatScroll < 0)
            chatScroll = 0;
        int start = (total > maxVis + chatScroll) ? (total - maxVis - chatScroll) : 0;
        int y = top;
        for (int i = start; i < total && y + lineH <= bot; i++)
        {
            d.setTextColor(chatLines[i].startsWith(">>") ? TFT_GREENYELLOW : TFT_CYAN, TFT_BLACK);
            d.setCursor(4, y);
            d.print(chatLines[i]);
            y += lineH;
        }
        d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        d.setCursor(4, SH - inH - hintH);
        d.print("Tab上 Ctrl下 Fn+R重绑 Opt铃声");
        d.fillRect(0, SH - inH, SW, inH, TFT_DARKGREY);
        d.setTextColor(TFT_WHITE, TFT_DARKGREY);
        d.setCursor(4, SH - inH + 4);
        d.print(">");
        {
            String show = inputLine;
            while (show.length() && d.textWidth(show.c_str()) > 210)
            {
                uint8_t b = (uint8_t)show.charAt(0);
                int cut = 1;
                if ((b & 0xF0u) == 0xE0u)
                    cut = 3;
                else if ((b & 0xE0u) == 0xC0u)
                    cut = 2;
                else if ((b & 0xF8u) == 0xF0u)
                    cut = 4;
                show = show.substring(cut);
            }
            d.print(show);
            d.print("|");
        }
    }
}

static void handleChatKeys()
{
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed())
        return;
    auto st = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && !st.word.empty())
    {
        for (char c : st.word)
        {
            if (c == 'r' || c == 'R')
            {
                wx::stopPollTask();
                wx::clearCreds();
                sdRemoveIfExists(kSdIlinkPath);
                wx::cancelPairing();
                pairStarted = false;
                qrPayload = nullptr;
                chatLines.clear();
                chatScroll = 0;
                mode = UI_PAIR;
                markDraw();
                return;
            }
        }
    }
    if (st.tab || st.ctrl || st.alt || st.opt)
        return;
    if (st.enter && inputLine.length())
    {
        if (!wx::lastUserId()[0])
            pushWrappedLines(chatLines, "[系统] 请对方先发一条消息", 220);
        else
        {
            bool ok = wx::sendWxMessage(wx::lastUserId(), inputLine.c_str());
            pushWrappedLines(chatLines, String(">> ") + inputLine, 220);
            if (!ok)
                pushWrappedLines(chatLines, "[系统] 发送失败", 220);
        }
        inputLine = "";
        markDraw();
        return;
    }
    if (st.del && inputLine.length())
    {
        inputLine.remove(inputLine.length() - 1);
        markDraw();
    }
    if (st.space && inputLine.length() < 500)
    {
        inputLine += ' ';
        markDraw();
    }
    for (char c : st.word)
    {
        if ((uint8_t)c < 32u)
            continue;
        if (inputLine.length() < 500)
        {
            inputLine += c;
            markDraw();
        }
    }
}

// 物理键边沿（不受 Caps 把 ; 变成 : 等影响）
static bool edgeKeyTabWifi()
{
    static bool prev;
    bool now = M5Cardputer.Keyboard.isKeyPressed((char)KEY_TAB);
    bool e = now && !prev;
    prev = now;
    return e;
}

static bool edgeKeyCtrlWifi()
{
    static bool prev;
    bool now = M5Cardputer.Keyboard.isKeyPressed((char)KEY_LEFT_CTRL);
    bool e = now && !prev;
    prev = now;
    return e;
}

static void wifiKeys()
{
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed())
        return;
    auto st = M5Cardputer.Keyboard.keysState();
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN))
    {
        for (char c : st.word)
            if (c == 'r' || c == 'R')
                startWifiScan();
    }
    if (wifiPassMode)
    {
        if (st.enter)
        {
            tryWifiConnect();
            markDraw();
            return;
        }
        if (st.del && wifiPassword.length())
        {
            wifiPassword.remove(wifiPassword.length() - 1);
            markDraw();
        }
        if (st.space)
        {
            wifiPassword += ' ';
            markDraw();
        }
        for (char c : st.word)
        {
            if ((uint8_t)c >= 32u && wifiPassword.length() < 64)
            {
                wifiPassword += c;
                markDraw();
            }
        }
        return;
    }
    if (scanDone && scanCount > 0)
    {
        if (st.enter)
        {
            wifiPassMode = true;
            wifiPassword = "";
            markDraw();
            return;
        }
    }
    if (scanDone && scanCount > 0 && !wifiPassMode)
    {
        bool moved = false;
        for (char c : st.word)
        {
            if (c == ';' || c == ':' || c == ',' || c == '<')
            {
                wifiSel--;
                if (wifiSel < 0)
                    wifiSel = scanCount - 1;
                moved = true;
            }
            if (c == '.' || c == '>')
            {
                wifiSel++;
                if (wifiSel >= scanCount)
                    wifiSel = 0;
                moved = true;
            }
        }
        if (moved)
            markDraw();
    }
    if (st.enter && !scanning && !scanDone)
    {
        startWifiScan();
        markDraw();
    }
}

static bool keyTabOnce()
{
    static bool prev;
    bool now = M5Cardputer.Keyboard.isKeyPressed((char)KEY_TAB);
    bool edge = now && !prev;
    prev = now;
    return edge;
}

static bool keyCtrlOnce()
{
    static bool prev;
    bool now = M5Cardputer.Keyboard.isKeyPressed((char)KEY_LEFT_CTRL);
    bool edge = now && !prev;
    prev = now;
    return edge;
}

/** Opt（Go）键按下边沿：开关新消息提示音 */
static bool edgeKeyOptToggleBeep()
{
    static bool prev;
    bool now = M5Cardputer.Keyboard.keysState().opt;
    bool edge = now && !prev;
    prev = now;
    return edge;
}

void setup()
{
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);
    drawBootSplash("精简版");
    delay(380);
    mountSd();
    loadMsgBeepPref();
    wx::loadPrefs();
    loadIlinkFromSdOverlay();
    if (attemptConnectSavedWifi())
    {
        if (wx::isPaired())
        {
            mode = UI_CHAT;
            wx::startPollTask();
            pushWrappedLines(chatLines, "已自动连接 WiFi。", 220);
            persistIlinkToSd();
        }
        else
        {
            mode = UI_PAIR;
            pairStarted = false;
            qrPayload = nullptr;
            chatLines.clear();
        }
        markDraw();
        return;
    }
    WiFi.mode(WIFI_STA);
    mode = UI_WIFI;
    startWifiScan();
    markDraw();
}

void loop()
{
    M5Cardputer.update();

    if (WiFi.status() != WL_CONNECTED && mode != UI_WIFI)
    {
        wx::stopPollTask();
        mode = UI_WIFI;
        chatLines.clear();
        scanDone = false;
        pairStarted = false;
        qrPayload = nullptr;
        startWifiScan();
        markDraw();
    }

    if (mode == UI_WIFI)
    {
        if (scanDone && scanCount > 0 && !wifiPassMode)
        {
            if (edgeKeyCtrlWifi())
            {
                wifiSel--;
                if (wifiSel < 0)
                    wifiSel = scanCount - 1;
                markDraw();
            }
            if (edgeKeyTabWifi())
            {
                wifiSel++;
                if (wifiSel >= scanCount)
                    wifiSel = 0;
                markDraw();
            }
        }
        wifiKeys();
        if (g_needDraw)
        {
            drawAll();
            g_needDraw = false;
        }
        delay(20);
        return;
    }

    if (mode == UI_PAIR)
    {
        if (!pairStarted)
        {
            wx::cancelPairing();
            pairStarted = true;
            qrPayload = wx::startPairing();
            lastPairPollMs = millis();
            markDraw();
        }
        if (pairStarted && wx::pairState() == 1)
        {
            if (millis() - lastPairPollMs > 2500)
            {
                lastPairPollMs = millis();
                if (wx::pollPairing())
                {
                    pairStarted = false;
                    qrPayload = nullptr;
                    mode = UI_CHAT;
                    chatLines.clear();
                    chatScroll = 0;
                    wx::startPollTask();
                    pushWrappedLines(chatLines, "绑定成功。", 220);
                    persistIlinkToSd();
                    markDraw();
                }
                else if (wx::pairState() == 3)
                {
                    pairStarted = true;
                    qrPayload = nullptr;
                    pushWrappedLines(chatLines, "二维码失效，Enter重试", 220);
                    markDraw();
                }
            }
        }
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
        {
            auto st = M5Cardputer.Keyboard.keysState();
            if (st.enter)
            {
                pairStarted = false;
                chatLines.clear();
                markDraw();
            }
        }
        if (g_needDraw)
        {
            drawAll();
            g_needDraw = false;
        }
        delay(20);
        return;
    }

    bool gotWxDisp = false;
    while (wx::hasDisp())
    {
        char *t = wx::takeDisp();
        if (t)
        {
            playIncomingMessageBeep();
            pushWrappedLines(chatLines, String(t), 220);
            free(t);
            chatScroll = 0;
            gotWxDisp = true;
            markDraw();
        }
    }
    if (gotWxDisp)
        persistIlinkToSd();

    if (edgeKeyOptToggleBeep())
    {
        g_msgBeepEnabled = !g_msgBeepEnabled;
        saveMsgBeepPref();
        pushWrappedLines(chatLines, g_msgBeepEnabled ? "[系统] 提示音：开" : "[系统] 提示音：关", 220);
        if (g_msgBeepEnabled)
            playIncomingMessageBeep();
        markDraw();
    }

    if (keyTabOnce())
    {
        int lineH = 14, maxVis = 6, total = (int)chatLines.size();
        int maxTop = (total > maxVis) ? total - maxVis : 0;
        if (chatScroll < maxTop)
        {
            chatScroll++;
            markDraw();
        }
    }
    if (keyCtrlOnce() && chatScroll > 0)
    {
        chatScroll--;
        markDraw();
    }

    handleChatKeys();
    if (g_needDraw)
    {
        drawAll();
        g_needDraw = false;
    }
    delay(15);
}
