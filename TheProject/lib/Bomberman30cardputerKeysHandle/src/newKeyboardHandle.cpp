#include "newKeyboardHandle.h"
#include <M5Cardputer.h>

unsigned long NewKeyboardHandle::lastPressTime[256] = {0};
bool NewKeyboardHandle::wasPressed[256] = {0};
bool NewKeyboardHandle::durationTriggered[256] = {0};
unsigned long NewKeyboardHandle::lastWorkTime[256] = {0};
unsigned long NewKeyboardHandle::applyKeyTime[256] = {0};
bool NewKeyboardHandle::anyKeyWasPressed = false;
unsigned long NewKeyboardHandle::anyKeyStartTime = 0;
unsigned long NewKeyboardHandle::anyKeyLastRepeatTime = 0;
// To test for Key_Press_1_Click_And_After_Few_MS_RepeatClick
bool wasPressed[256] = {0};
unsigned long applyKeyTime[256] = {0};
unsigned long repeatStartTime[256] = {0};

bool NewKeyboardHandle::ifKeyJustPress(char key)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);
    if (pressed && !wasPressed[idx])
    {
        wasPressed[idx] = true;
        return true;
    }
    if (!pressed)
    {
        wasPressed[idx] = false;
    }
    return false;
}

bool NewKeyboardHandle::ifKeyJustRelease(char key)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);

    // If it was previously pressed and is now released — this is the release frame
    if (!pressed && wasPressed[idx])
    {
        wasPressed[idx] = false;
        return true;
    }

    // Update status
    if (pressed)
    {
        wasPressed[idx] = true;
    }

    return false;
}

// 2) Holds for X milliseconds — triggered once only after the time has passed, 
//    and is not triggered again until the key is released.
bool NewKeyboardHandle::isKeyPressForDuration(char key, int durationInMs)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);
    unsigned long now = millis();

    if (pressed)
    {
        // Start of press — save start time and reset the flag
        if (lastPressTime[idx] == 0)
        {
            lastPressTime[idx] = now;
            durationTriggered[idx] = false;
        }
        // If time has passed and the action hasn't triggered yet — trigger once
        if (!durationTriggered[idx] && (now - lastPressTime[idx] >= (unsigned long)durationInMs))
        {
            durationTriggered[idx] = true;
            return true;
        }
    }
    else
    {
        // Released — reset to allow trigger next time
        lastPressTime[idx] = 0;
        durationTriggered[idx] = false;
    }

    return false;
}

// 3) Triggers every intervalMs as long as the key is pressed (repeated execution)
bool NewKeyboardHandle::KeyPressWorkInMilliSecond(char key, int intervalMs)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);
    unsigned long now = millis();

    if (pressed)
    {
        // If this is the first execution — return immediately
        if (lastWorkTime[idx] == 0)
        {
            lastWorkTime[idx] = now;
            return true;
        }
        // Repeated executions every intervalMs
        if (now - lastWorkTime[idx] >= (unsigned long)intervalMs)
        {
            lastWorkTime[idx] = now;
            return true;
        }
    }
    else
    {
        // Reset when key is released
        lastWorkTime[idx] = 0;
    }
    return false;
}

// New function: ApplyPushedKeyEveryMS
// - Returns true immediately when pressing starts, then every intervalMs as long as the key is held.
// - Resets when the key is released.
bool NewKeyboardHandle::ApplyPushedKeyEveryMS(char key, int intervalMs)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);
    unsigned long now = millis();

    if (pressed)
    {
        if (applyKeyTime[idx] == 0)
        {
            // First trigger immediately
            applyKeyTime[idx] = now;
            return true;
        }
        if (now - applyKeyTime[idx] >= (unsigned long)intervalMs)
        {
            applyKeyTime[idx] = now;
            return true;
        }
    }
    else
    {
        // Reset when the key is released
        applyKeyTime[idx] = 0;
    }

    return false;
}

bool NewKeyboardHandle::Any_Key_Press_1_Click_And_After_Few_MS_RepeatClick(int delayBeforeRepeatMs, int repeatIntervalMs)
{
    bool pressed = M5Cardputer.Keyboard.isPressed();
    unsigned long now = millis();
    if (M5Cardputer.Keyboard.isChange())// Cheack if working
    {
        // release
        anyKeyWasPressed = false;
        anyKeyStartTime = 0;
        anyKeyLastRepeatTime = 0;
    }
    if (pressed) {
        // first press
        if (!anyKeyWasPressed) {
            anyKeyWasPressed = true;
            anyKeyStartTime = now;
            anyKeyLastRepeatTime = 0;
            return true;
        }

        
        if (now - anyKeyStartTime >= (unsigned long)delayBeforeRepeatMs) {
            if (anyKeyLastRepeatTime == 0 ||
                now - anyKeyLastRepeatTime >= (unsigned long)repeatIntervalMs) {
                anyKeyLastRepeatTime = now;
                return true;
            }
        }
    } else {
        // release
        anyKeyWasPressed = false;
        anyKeyStartTime = 0;
        anyKeyLastRepeatTime = 0;
    }

    return false;

}

bool NewKeyboardHandle::Key_Press_1_Click_And_After_Few_MS_RepeatClick(
    char key,
    int delayBeforeRepeatMs, // How much time before repeat starts
    int repeatIntervalMs     // Interval for each subsequent repeat
)
{
    uint8_t idx = (uint8_t)key;
    bool pressed = M5Cardputer.Keyboard.isKeyPressed(key);
    unsigned long now = millis();

    // If the key is not pressed — reset everything
    if (!pressed)
    {
        wasPressed[idx] = false;
        applyKeyTime[idx] = 0;
        repeatStartTime[idx] = 0;
        return false;
    }

    // --- First Press ---
    if (!wasPressed[idx])
    {
        wasPressed[idx] = true;
        applyKeyTime[idx] = now;  // Press start time
        repeatStartTime[idx] = 0; // Repeat hasn't started yet
        return true;              // The first return!
    }

    // --- Start Repeat after holding duration ---
    if (repeatStartTime[idx] == 0)
    {
        if (now - applyKeyTime[idx] >= (unsigned long)delayBeforeRepeatMs)
        {
            repeatStartTime[idx] = now; // This is the time from which repeats start
            return true;                // First repeat
        }
        return false;
    }

    // --- Repeat every repeatIntervalMs ---
    if (now - repeatStartTime[idx] >= (unsigned long)repeatIntervalMs)
    {
        repeatStartTime[idx] = now;
        return true;
    }

    return false;
}