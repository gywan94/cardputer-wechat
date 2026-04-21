#ifndef NEW_KEYBOARD_HANDLE_H
#define NEW_KEYBOARD_HANDLE_H

#include <Arduino.h>

class NewKeyboardHandle
{
public:
    // Triggers on the first frame of the key press only
    static bool ifKeyJustPress(char key);
    static bool ifKeyJustRelease(char key);

    // Triggers once after the key has been held for X milliseconds
    static bool isKeyPressForDuration(char key, int durationInMs);

    // Triggers repeatedly every X milliseconds while the key is held
    static bool KeyPressWorkInMilliSecond(char key, int intervalMs);

    // New Function: Triggers immediately on press, then repeats every intervalMs while held
    static bool ApplyPushedKeyEveryMS(char key, int intervalMs);
    static bool Any_Key_Press_1_Click_And_After_Few_MS_RepeatClick(
        int delayBeforeRepeatMs,
        int repeatIntervalMs);
    // Triggers once immediately, waits for a delay, then begins repeating at a set interval
    bool Key_Press_1_Click_And_After_Few_MS_RepeatClick(
        char key,
        int delayBeforeRepeatMs, // Time to wait before repetition starts
        int repeatIntervalMs     // Frequency of subsequent repetitions
    );

private:
    // Arrays are declared here (definitions should be in the .cpp file)
    static unsigned long lastPressTime[256];
    static bool wasPressed[256];
    static bool durationTriggered[256];
    static unsigned long lastWorkTime[256];
    static unsigned long applyKeyTime[256];
    static bool anyKeyWasPressed;
    static unsigned long anyKeyStartTime;
    static unsigned long anyKeyLastRepeatTime;



};

#endif // NEW_KEYBOARD_HANDLE_H