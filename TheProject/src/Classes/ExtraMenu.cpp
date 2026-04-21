#include "ExtraMenu.h"
#include "./MyOS.h"
#include "StepCounter.h"
#include "Timer.h"
#include "GPIOcontrol.h"
#include "Fonts/DryBrush15.h"
#include "tools/Calculator.h"
#include "RecordV2.h"
// #include "tools/ResistorCalc.h"
#include "tools/ColorCode.h"
// #include "FUN/Boty.h"
#include "partition_maneger.h"
#include "GamesV2.h"
#include "MusicPlayerV2.h"
#include "tools/ESPnowChat.h"
// #include "RecTest2.h"
#if defined(COMPILE_WIFI_THINGS)
#include "EspProjectRC/EspProjectRemoteControl.h"
#include "WIFI_HANDLE.h"
#include "wifiSpectrum.h"
#include "WeChatILink.h"
#endif
// #include "Arduboy/ArduboyGamesController.h"
#include "PasswordVault.h"
#if defined(COMPILE_EMULATOR)
#include "Emulator/Emulator.h"
#endif
#include "Theme_Maneger.h"
#include "MainMenu.h"
#include "painter.h"
#include "Pic/ExtraMenuPic.c"
#include "FUN/Snake.h"
#include "FUN/Piano.h"
#include "RECPlayer.h"
// #include "SD_To_DiskOnKey.h"
void Extra::Begin()
{
    SlectedString = &menuItems;
}

void Extra::Loop()
{
    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new MainMenu(mainOS));
        return;
    }
    if (mainOS->screenOff)
    {
        return;
    }
    // if (M5Cardputer.Keyboard.isChange())
    //{

    if (mainOS->NewKey.ifKeyJustPress('.'))

    // if (M5Cardputer.Keyboard.isKeyPressed('.'))
    {
        MenuIdFocus++;
        if (MenuIdFocus > SlectedString->size() - 1)
            MenuIdFocus = 0;
        targetCameraY = MenuIdFocus * DistanceBetweenTXT;
    }
    if (mainOS->NewKey.ifKeyJustPress(';'))

    // if (M5Cardputer.Keyboard.isKeyPressed(';'))
    {
        MenuIdFocus--;
        if (MenuIdFocus < 0)
            MenuIdFocus = SlectedString->size() - 1;
        targetCameraY = MenuIdFocus * DistanceBetweenTXT;
    }
    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))

    //  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER))
    {
        if ((*SlectedString)[MenuIdFocus] == "Step's Counter")
        {
            mainOS->ChangeMenu(new StepCounter(mainOS));
            // MenuIdFocus = 0;
        }
        else if ((*SlectedString)[MenuIdFocus] == "Timer")
        {
            mainOS->ChangeMenu(new Timer(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "GPIO Control")
        {
            mainOS->ChangeMenu(new GPIOcontrol(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Calculator")
        {
            mainOS->ChangeMenu(new Calculator(mainOS));
        }

        else if ((*SlectedString)[MenuIdFocus] == "MIC Record to SD Card")
        {
            mainOS->ChangeMenu(new RecordV2(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "WIFI")
        {
            mainOS->ChangeMenu(new Wifi_handle(mainOS));
        }
        /*         else if ((*SlectedString)[MenuIdFocus] == "Volos Resistor Calculator")
                {
                    mainOS->ChangeMenu(new ResistorCalc(mainOS));
                } */
        else if ((*SlectedString)[MenuIdFocus] == "Password Vault")
        {

            mainOS->ChangeMenu(new PasswordVault(mainOS));
        }

        else if ((*SlectedString)[MenuIdFocus] == "Esp Projects Remote Control")
        {
            /*        mainOS->SetFastBootVar(3);
                   ESP.restart(); */
#if defined(COMPILE_WIFI_THINGS)

            mainOS->ChangeMenu(new EspProjectRemoteControl(mainOS));
#endif
        }
        else if ((*SlectedString)[MenuIdFocus] == "emulator")
        {
            mainOS->SetFastBootVar(5);
            ESP.restart();

#if defined(COMPILE_EMULATOR)
            mainOS->ChangeMenu(new Emulator(mainOS));
#endif
        }
        else if ((*SlectedString)[MenuIdFocus] == "Color Picker")
        {
            mainOS->ChangeMenu(new ColorPicker(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "ESPnow Chat")
        {
            mainOS->ChangeMenu(new ESPnowChat(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Wifi Spectrum")
        {
            mainOS->ChangeMenu(new wifiSpectrum(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Theme Manager")
        {
            mainOS->ChangeMenu(new Theme_Maneger(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Cardputer To Disk On Key")
        {
            // mainOS->ChangeMenu(new SD_To_DiskOnKey(mainOS));
        }

        else if ((*SlectedString)[MenuIdFocus] == "Painter")
        {
            mainOS->ChangeMenu(new Painter(mainOS));
            return;
        }

        else if ((*SlectedString)[MenuIdFocus] == "(NOT) AI Chatbot (not Work Well)")
        {
            //   mainOS->ChangeMenu(new Boty(mainOS));

            mainOS->SetFastBootVar(3);
            // mainOS->saveSettings();
            ESP.restart();
            /*      esp_sleep_enable_timer_wakeup(10 * 1000); // 10ms = 10,000µs
             esp_light_sleep_start(); */
        }

        else if ((*SlectedString)[MenuIdFocus] == "Partition Viewer")
        {
            mainOS->ChangeMenu(new partition_maneger(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "GameV2")
        {
            mainOS->ChangeMenu(new GamesV2(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Snake")
        {
            mainOS->ChangeMenu(new Snake(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Piano")
        {
            mainOS->ChangeMenu(new Piano(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "REC Player")
        {
            mainOS->ChangeMenu(new RECPlayer(mainOS));
            return;
        }
#if defined(COMPILE_WIFI_THINGS)
        else if ((*SlectedString)[MenuIdFocus] == "WeChat iLink")
        {
            mainOS->ChangeMenu(new WeChatILink(mainOS));
            return;
        }
#endif
    }
    // }

    float finalCameraYPos = MenuIdFocus * DistanceBetweenTXT;
    cameraY = cameraY + (targetCameraY - cameraY) * 0.12f;
    if (!mainOS->screenOff)

    {
        Draw();
    }
}

void Extra::Draw()
{
    const int HEADER_H = 18;
    const int MENU_X = 35;
    const int PIC_X = 11;
    const int PIC_Y_OFFSET = -1;

    int y = HEADER_H + 6 - cameraY;

    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillScreen(BLACK);
    mainOS->sprite.unloadFont();
    mainOS->sprite.setTextSize(1);
    mainOS->sprite.setTextWrap(false);

    /* ---------- MENU ITEMS ---------- */
    for (int i = 0; i < SlectedString->size(); i++)
    {
        if (MenuIdFocus == i)
        {
            mainOS->sprite.fillRect(0, y - 2, 240, DistanceBetweenTXT, TFT_DARKGREEN);
            mainOS->sprite.setTextColor(WHITE);

            mainOS->sprite.setCursor(4, y);
            mainOS->sprite.print(">");
        }
        else
        {
            mainOS->sprite.setTextColor(YELLOW);
        }
        switch (i)
        {
        case 0:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, remote);
            break;
        case 1:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, vault);
            break;
        case 2:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, Steps);
            break;
        case 3:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, colorPallete);
            break;
        case 4:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, timer);
            break;
        case 5:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, calculator);
            break;
        case 6:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, theme);
            break;
        case 7:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, gpio);
            break;
        case 8:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, RecordMic);
            break;
        case 9:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, partition);
            break;
        case 10:
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, WifiSpectrum);
            break;
        default:
            break;
        }
#if defined(COMPILE_WIFI_THINGS)
        if ((*SlectedString)[i] == "WeChat iLink")
        {
            mainOS->sprite.pushImage(PIC_X, y + PIC_Y_OFFSET, 24, 24, WifiSpectrum);
        }
#endif
        mainOS->sprite.setCursor(MENU_X, y);
        mainOS->sprite.print((*SlectedString)[i]);

        y += DistanceBetweenTXT;
    }
    /* ---------- HEADER ---------- */
    mainOS->sprite.fillRect(0, 0, 240, HEADER_H, DARKGREY);
    mainOS->sprite.setTextColor(WHITE);
    mainOS->sprite.setCursor(6, 5);
    mainOS->sprite.print("EXTRA MENU");

    mainOS->sprite.drawFastHLine(0, HEADER_H, 240, DARKGREY);
    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}
