// Auto: iLink protocol (from WeChatILink / M5Claw)
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace wx {
  void loadPrefs();
  void saveCreds();
  /** 用 SD/手动读出的字符串覆盖内存中的 token/host；persistNvs 为 true 时同步写入 Preferences */
  void applyCredsFromStrings(const char *tok, const char *host, bool persistNvs);
  const char *tokenStr();
  void clearCreds();
  bool isPaired();
  void enqueueDisp(const char* text);
  bool hasDisp();
  char* takeDisp();
  bool sendWxMessage(const char* userId, const char* text);
  void startPollTask();
  void stopPollTask();
  const char* startPairing();
  bool pollPairing();
  void cancelPairing();
  int pairState();
  const char* apiHost();
  const char* lastUserId();
  /** 设置上次对话的微信 user_id，并写入 NVS（开机后可继续给对方发消息） */
  void setLastUserId(const char* uid);
}
