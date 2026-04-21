#pragma once

#include "./GlobalParentClass.h"
#include <Arduino.h>
#include <M5Cardputer.h>
#include <vector>

/**
 * 微信 iLink 扫码绑定与文字收发（逻辑参考 GPL-3.0 项目 M5Claw 的 wechat_bot）。
 * 需已连接 Wi-Fi；配对信息保存在 Preferences。
 */
class WeChatILink : public GlobalParentClass
{
public:
    explicit WeChatILink(MyOS *os) : GlobalParentClass(os) {}

    void Begin() override;
    void Loop() override;
    void Draw() override;
    void OnExit() override;

private:
    enum UiMode : uint8_t
    {
        UI_NEED_WIFI,
        UI_PAIR_QR,
        UI_CHAT
    };

    void drawFullSprite();
    void handleChatKeys();
    void appendChatLine(const char *prefix, const String &text);
    void pushUtf8Wrapped(const String &text);
    void tryPollPairing();
    void startPairFlow();

    UiMode mode = UI_NEED_WIFI;
    String inputLine;
    std::vector<String> chatLines;
    int chatScroll = 0;
    unsigned long lastPairPollMs = 0;
    bool pairStarted = false;
    const char *qrPayload = nullptr;
    unsigned long cursorBlinkMs = 0;
    bool cursorOn = true;
};
