#include "Theme_Maneger.h"
#include "MyOS.h"
#include "FileBrowser.h"
#include "ExtraMenu.h"
void Theme_Maneger::Begin()
{
    SlectedString = &menuItems;
    if (mainOS->inEditThemeFileMode)
    {
        mainOS->inEditThemeFileMode = false;
        inEditThemeFileMode = true;
    }

    if (inEditThemeFileMode)
    {
        menuItems.push_back("Update Theme File");
    }
    else
    {
        menuItems.push_back("Open Theme Folder");
        menuItems.push_back("Reset To Default");
        menuItems.push_back("Save Current As Theme");
    }
}

void Theme_Maneger::Loop()
{

    if (mainOS->screenOff)
    {
        return;
    }
        if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new Extra(mainOS));
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
        if ((*SlectedString)[MenuIdFocus] == "Save Current As Theme")
        {
            delay(500);
            if (mainOS->SaveCurrentTheme())
            {
                mainOS->ShowOnScreenMessege("theme saved in AdvanceOS/Theme folder");
            }
            else
            {
                mainOS->ShowOnScreenMessege("save failed");
            }
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Top Bar Color 1")
        {
            delay(500);

            mainOS->TOP_BAR_COLOR_1 = mainOS->AskForColor("Choose color 1 for top bar", mainOS->TOP_BAR_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Top Bar Color 2")
        {
            delay(500);

            mainOS->TOP_BAR_COLOR_2 = mainOS->AskForColor("Choose color 2 for top bar", mainOS->TOP_BAR_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Open Theme Folder")
        {
            delay(500);
            mainOS->currentPath = "/AdvanceOS/Theme";
            mainOS->ChangeMenu(new FileBrowser(mainOS));
        }
        else if ((*SlectedString)[MenuIdFocus] == "Reset To Default")
        {
            delay(500);
            mainOS->ResetToDefaultTheme();
        }

        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 1 MUSIC Color 1")
        {
            delay(500);

            mainOS->MENU_1_MUSIC_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_1_MUSIC_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 1 MUSIC Color 2")
        {
            delay(500);

            mainOS->MENU_1_MUSIC_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_1_MUSIC_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 2 FILES Color 1")
        {

            delay(500);

            mainOS->MENU_2_FILES_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_2_FILES_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 2 FILES Color 2")
        {
            delay(500);

            mainOS->MENU_2_FILES_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_2_FILES_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 3 BOT Color 1")
        {
            delay(500);

            mainOS->MENU_3_BOT_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_3_BOT_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 3 BOT Color 2")

        {

            delay(500);

            mainOS->MENU_3_BOT_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_3_BOT_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 4 NOTES Color 1")
        {
            delay(500);

            mainOS->MENU_4_NOTES_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_4_NOTES_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 4 NOTES Color 2")
        {
            delay(500);

            mainOS->MENU_4_NOTES_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_4_NOTES_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 5 EXTRA Color 1")
        {
            delay(500);

            mainOS->MENU_5_EXTRA_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_5_EXTRA_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 5 EXTRA Color 2")
        {
            delay(500);

            mainOS->MENU_5_EXTRA_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_5_EXTRA_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 6 GAMES Color 1")
        {
            delay(500);

            mainOS->MENU_6_GAMES_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_6_GAMES_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 6 GAMES Color 2")
        {
            delay(500);

            mainOS->MENU_6_GAMES_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_6_GAMES_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 7 SETTING Color 1")
        {
            delay(500);

            mainOS->MENU_7_SETTINGS_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_7_SETTINGS_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 7 SETTING Color 2")
        {
            delay(500);

            mainOS->MENU_7_SETTINGS_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_7_SETTINGS_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 8 PAINT Color 1")
        {
            delay(500);

            mainOS->MENU_4_PAINT_COLOR_1 = mainOS->AskForColor("Choose color", mainOS->MENU_4_PAINT_COLOR_1);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Set Menu 8 PAINT Color 2")
        {
            delay(500);

            mainOS->MENU_4_PAINT_COLOR_2 = mainOS->AskForColor("Choose color", mainOS->MENU_4_PAINT_COLOR_2);
        }
        else if ((*SlectedString)[MenuIdFocus] == "Update Theme File")
        {
            delay(500);
            mainOS->SaveCurrentTheme(mainOS->FileSelectedInFS);
            mainOS->ChangeMenu(new FileBrowser(mainOS));
        }
    }
    // }

    float finalCameraYPos = MenuIdFocus * DistanceBetweenTXT;
    cameraY = cameraY + (targetCameraY - cameraY) * 0.12f;
    if (!mainOS->screenOff)

    {
        Draw();
    }
}

void Theme_Maneger::Draw()
{
    const int HEADER_H = 18;
    const int MENU_X = 14;

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

        mainOS->sprite.setCursor(MENU_X, y);
        mainOS->sprite.print((*SlectedString)[i]);

        y += DistanceBetweenTXT;
    }
    /* ---------- HEADER ---------- */
    mainOS->sprite.fillRect(0, 0, 240, HEADER_H, DARKGREY);
    mainOS->sprite.setTextColor(WHITE);
    mainOS->sprite.setCursor(6, 5);
    mainOS->sprite.print("Theme MENU");

    //seperate line
    mainOS->sprite.drawFastHLine(0, HEADER_H, 240, DARKGREY);
    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}
