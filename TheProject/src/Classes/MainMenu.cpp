#define MAIM_COLOR rgb565(color1R, color1G, color1B)
#define SECONDARY_COLOR rgb565(color2R, color2G, color2B)

#include "MainMenu.h"

#include "MyOS.h"
#include "FileBrowser.h"
#include "Pic/MainMenuPic.c"
#include "Function.h"
#include "Fonts/DryBrush15.h"
#include "Setting.h"
#include "TextEditor.h"
#include "Notes.h"
#include "ExtraMenu.h"
#include "FUN/BotyV2.1.h"
#if defined(NEW_VERSION_GAMES)
#include "GamesV2.h"
#else
#include "Arduboy/ArduboyGamesController.h"
#endif
#include "Painter.h"
// currentPath = FromFilePathToFolderPath(FileSelectedInFS);

void MainMenu::Begin()
{
    SetRandomStarPos(); // star pose for EXTRA in menu
    if (mainOS->CurrentThemeSelectedPath != "")
    {
        themeLoaded = true;
    }
    CheackIfHaveThemePictureForThisMenu();
}

void MainMenu::Loop()
{
    if (M5Cardputer.Keyboard.isKeyPressed('i'))
    {
        RenderINFO = true;
    }
    else
    {
        RenderINFO = false;
    }
    if (mainOS->screenOff)
    {
        return;
    }
    if (mainOS->NewKey.ifKeyJustPress('`')) // if press make  black screen
    {

        mainOS->PlayCuteEvilTone();
        M5Cardputer.Display.setBrightness(0);
        mainOS->screenOff = true;
        delay(400);
    }

    if (mainOS->NewKey.ifKeyJustPress('/')) // right arrow next in menu
    {
        opositAnim = true;
        imageAnim = -0.0f;
        imageAnimStart = millis();
        lastMenuIndex = mainOS->currentMenuIndex;

        mainOS->PlayCuteEvilTone();

        mainOS->currentMenuIndex++;
        if (mainOS->currentMenuIndex >= menuCount)
            mainOS->currentMenuIndex = 0;
        CheackIfHaveThemePictureForThisMenu();
    }
    if (mainOS->NewKey.ifKeyJustPress(',')) // left arrow
    {
        opositAnim = true;

        imageAnim = 0.0f;
        imageAnimStart = millis();
        lastMenuIndex = mainOS->currentMenuIndex;
        mainOS->PlayCuteEvilTone();

        mainOS->currentMenuIndex--;
        if (mainOS->currentMenuIndex < 0)
            mainOS->currentMenuIndex = menuCount - 1;
        CheackIfHaveThemePictureForThisMenu();
    }
    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER)) // enter to menu
    // if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER))
    {
        mainOS->PlayCuteEvilTone();

        switch (mainOS->currentMenuIndex)
        {
        case 0:
            mainOS->EnterMusicPlayer();
            break;
        case 1:
            mainOS->ChangeMenu(new FileBrowser(mainOS));
            break;
        case 2:
            mainOS->ChangeMenu(new BotyV2(mainOS));
            break;
        case 3:
            mainOS->ChangeMenu(new Notes(mainOS));
            break;
        case 4:
            mainOS->ChangeMenu(new Painter(mainOS));
            return;
            break;
        case 5:
            mainOS->ChangeMenu(new Extra(mainOS));
            break;
        case 6:
#if defined(NEW_VERSION_GAMES)
            mainOS->ChangeMenu(new GamesV2(mainOS));
            return;
#else
            mainOS->ChangeMenu(new ArduboyGamesController(mainOS));
#endif
            break;
        case 7:
            mainOS->ChangeMenu(new Setting(mainOS));

            break;
        default:
            break;
        }
    }
    // 2 colors lerp for smooth color transition
    color1R = lerp(color1R, TOcolor1R, 0.15);
    color1G = lerp(color1G, TOcolor1G, 0.15);
    color1B = lerp(color1B, TOcolor1B, 0.15);
    // Second color rgb lerp
    color2R = lerp(color2R, TOcolor2R, 0.15);
    color2G = lerp(color2G, TOcolor2G, 0.15);
    color2B = lerp(color2B, TOcolor2B, 0.15);
    // text camera lerp for smoothj text
    cameraX = lerp(cameraX, lastMenuIndex * DistanceBetweenTXT, 0.15);

    Draw();
}

float normalize(float angle, float MaxNomber)
{
    angle = fmod(angle, MaxNomber);
    if (angle < 0)
        angle += MaxNomber;
    return angle;
}

void MainMenu::Draw()
{
    if (imageAnim < 1.0f)
    {
        float t = (millis() - imageAnimStart) / (float)imageAnimDuration;
        if (t > 1.0f)
            t = 1.0f;

        // easing
        // imageAnim = t * t * (3 - 2 * t); // smoothstep
        imageAnim = 1.0f - pow(1.0f - t, 3); // fast in, soft stop
    }
    else
    {
        if (lastMenuIndex != mainOS->currentMenuIndex)
        {
            opositAnim = false;
            lastMenuIndex = mainOS->currentMenuIndex;
            imageAnim = -0.0f;
            imageAnimStart = millis();
        }
    }
    if (PicAvaliableForThisMenu)
    {
        mainOS->sprite.createSprite(140, 135 - TopOffset); // 100 pixel for the image
    }
    else
    {
        mainOS->sprite.createSprite(240, 135 - TopOffset);
    }
    mainOS->sprite.fillScreen(MAIM_COLOR);
    mainOS->sprite.setTextWrap(false);
    mainOS->sprite.setSwapBytes(true);

    float NormelizedYrotrted = normalize(mainOS->CardputerRotationY, 120);

    const float frequency = 1.5;

    float sineValue = sin(frequency * millis() / 1000.0); // from 1 to -1

    int slideOffset;
    if (opositAnim)
    {
        slideOffset = imageAnim * 170;
    }
    else
    {
        slideOffset = (1.0f - imageAnim) * 170;
    }

    switch (lastMenuIndex)
    {
    case 0: // music player
        // change menu colors

        mainOS->getRGB_from_HEX(mainOS->MENU_1_MUSIC_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_1_MUSIC_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);

        if (!PicAvaliableForThisMenu)
        {
            mainOS->sprite.fillRect(83 + slideOffset, 8, 150, 80, rgb565(247, 227, 129));
            mainOS->sprite.drawRect(83 + slideOffset, 8, 150, 80, BLACK);
            mainOS->SampleGyroscope();
            for (size_t i = 0; i < 5; i++) // draw music lines
            {
                mainOS->sprite.fillRect(90 + slideOffset, 20 + i * 13, 130, 2, BLACK);
            }
            drawImageTransparent(
                110 + NormelizedYrotrted * 1 + slideOffset,
                50 + sineValue * 10,
                16, 23,
                HalfNotePA,
                [&](int x, int y, uint16_t c)
                {
                    mainOS->sprite.drawPixel(x, y, c);
                },
                0xF800,
                true,
                BLACK);
            drawImageTransparent(
                70 + NormelizedYrotrted * 0.8 + slideOffset,
                22 + sineValue * 15,
                16, 23,
                HalfNotePA,
                [&](int x, int y, uint16_t c)
                {
                    mainOS->sprite.drawPixel(x, y, c);
                },
                0xF800,
                true,
                BLACK);
            drawImageTransparent(90 + NormelizedYrotrted * 0.4 + slideOffset, 13 + -sineValue * 7, 67, 74, MusicNotePA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }

        break;
    case 1: // files
        // change menu colors

        mainOS->getRGB_from_HEX(mainOS->MENU_2_FILES_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_2_FILES_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        if (!PicAvaliableForThisMenu)
        {
            drawImageTransparent(110 + NormelizedYrotrted * 0.4 + slideOffset, 13, 79, 72, FileExplorerPA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }
        // mainOS->ChangeMenu(new FileBrowser(mainOS));
        break;
    case 2: // ai chatbot
        if (!PicAvaliableForThisMenu)

        {
            drawLinesInBackground(mainOS->sprite);
        }
        // change menu colors

        mainOS->getRGB_from_HEX(mainOS->MENU_3_BOT_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_3_BOT_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        if (!PicAvaliableForThisMenu)

        {
            drawImageTransparent(165 + slideOffset, 30, 68, 88, RobotPA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }
        // mainOS->ChangeMenu(new FileBrowser(mainOS));
        break;
    case 3: // notes

        // mainOS->ChangeMenu(new Notes(mainOS));

        mainOS->getRGB_from_HEX(mainOS->MENU_4_NOTES_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_4_NOTES_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        /*         TOcolor1R = 0;
                TOcolor1G = 31;
                TOcolor1B = 79; // dark blue
                TOcolor2R = 0;
                TOcolor2G = 131;
                TOcolor2B = 95; // some green */
        if (!PicAvaliableForThisMenu)
        {
            drawImageTransparent(110 + NormelizedYrotrted * 0.4 + slideOffset, 13, 82, 76, NotesPA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }
        break;
    case 4: // paint
        // change menu colors

        mainOS->getRGB_from_HEX(mainOS->MENU_4_PAINT_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_4_PAINT_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        if (!PicAvaliableForThisMenu)

        {
            if (SplashChange > 500)
            {
                SplashChange=0;
                InSplash++;
                if (InSplash > 3)
                {
                    InSplash = 1;
                }
                switch (InSplash)
                {
                case 1:
                    Pos1X = random(160, 200);
                    Pos1Y = random(10, 30);

                    break;
                case 2:
                    Pos2X = random(180, 220);
                    Pos2Y = random(20, 40);
                    break;
                case 3:
                    Pos3X = random(130, 170);
                    Pos3Y = random(10, 30);
                    break;
                default:
                    break;
                }
            }

            drawImageTransparent(Pos1X, Pos1Y, 35, 35, Splash, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800, true, RED);
            drawImageTransparent(Pos2X, Pos2Y, 35, 35, Splash, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800, true, YELLOW);
            drawImageTransparent(Pos3X, Pos3Y, 35, 35, Splash, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800, true, BLUE);

            drawImageTransparent(165, 20, 56, 70, PaintPA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }
        // mainOS->ChangeMenu(new FileBrowser(mainOS));

        break;

    case 5: // extra menu star
        /*         if (RandomStarTimer > 1000)
                {
                    RandomStarTimer = 0;
                    RefrashStarPos();
                } */

        if (!PicAvaliableForThisMenu)
        {
            RefrashStarPos();

            mainOS->SampleGyroscope();
            if (!themeLoaded)
            {
                mainOS->sprite.fillRect(0, 50, 240, 65, BLACK);
            }
            for (size_t i = 0; i < STAR_COUNT; i++)
            {
                mainOS->sprite.drawPixel(StarsPosX[i], StarsPosY[i], YELLOW);
            }

            //    mainOS->SampleGyroscope();
            mainOS->sprite2.createSprite(82, 79);
            mainOS->sprite3.createSprite(78, 75);
            mainOS->sprite2.setSwapBytes(true);
            mainOS->sprite3.setSwapBytes(true);
            // mainOS->sprite3.pushImage(0, 0, 32, 32, Gear);

            drawImageTransparent(0, 0, 78, 75, StarPA, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite3.drawPixel(x, y, c); }, 0xF800);

            mainOS->sprite3.pushRotated(&mainOS->sprite2, 0 /* mainOS->CardputerRotationZ * 2 */, RED);
            mainOS->sprite2.pushSprite(&mainOS->sprite, 110 + NormelizedYrotrted * 0.4 + slideOffset, 10, BLACK);
            mainOS->sprite2.deleteSprite();
            mainOS->sprite3.deleteSprite();
        }

        /*         drawImageTransparent(110 + NormelizedYrotrted * 0.4, 13, 78, 75, StarPA, [&](int x, int y, uint16_t c)
                                     { mainOS->sprite.drawPixel(x, y, c); }, 0xF800); */

        mainOS->getRGB_from_HEX(mainOS->MENU_5_EXTRA_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_5_EXTRA_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        /*        TOcolor1R = 115;
TOcolor1G = 3;
TOcolor1B = 55; // purple
TOcolor2R = 229;
TOcolor2G = 35;
TOcolor2B = 99; // pink */
        // mainOS->ChangeMenu(new Extra(mainOS));
        break;
    case 6: // games
        if (!PicAvaliableForThisMenu)

        {
            drawImageTransparent(110 + slideOffset, 13, 90, 82, GBC, [&](int x, int y, uint16_t c)
                                 { mainOS->sprite.drawPixel(x, y, c); }, 0xF800);
        }

        mainOS->getRGB_from_HEX(mainOS->MENU_6_GAMES_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_6_GAMES_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);

        // mainOS->ChangeMenu(new Extra(mainOS));
        break;
    case 7: // setting

        mainOS->getRGB_from_HEX(mainOS->MENU_7_SETTINGS_COLOR_1, TOcolor1R, TOcolor1G, TOcolor1B);
        mainOS->getRGB_from_HEX(mainOS->MENU_7_SETTINGS_COLOR_2, TOcolor2R, TOcolor2G, TOcolor2B);
        if (!PicAvaliableForThisMenu)
        {
            // draw transperent Gear
            mainOS->SampleGyroscope();
            mainOS->sprite2.createSprite(40, 40);
            mainOS->sprite3.createSprite(32, 32);
            mainOS->sprite2.setSwapBytes(true);
            mainOS->sprite3.setSwapBytes(true);
            // mainOS->sprite3.pushImage(0, 0, 32, 32, Gear);

            drawImageTransparent(
                0,
                0,
                32, 32,
                Gear,
                [&](int x, int y, uint16_t c)
                {
                    mainOS->sprite3.drawPixel(x, y, c);
                },
                0xF800,
                true,
                YELLOW);

            mainOS->sprite3.pushRotated(&mainOS->sprite2, mainOS->CardputerRotationZ * 2, RED);
            mainOS->sprite2.pushSprite(&mainOS->sprite, 160 + slideOffset, 30, BLACK);
            mainOS->sprite2.deleteSprite();
            mainOS->sprite3.deleteSprite();
            // end draw transperent gear
            // draw transperent Gear
            // mainOS->SampleGyroscope();
            mainOS->sprite2.createSprite(80, 80);
            mainOS->sprite3.createSprite(32, 32);
            mainOS->sprite2.setSwapBytes(true);
            mainOS->sprite3.setSwapBytes(true);
            //  mainOS->sprite3.pushImage(0, 0, 32, 32, Gear);

            drawImageTransparent(
                0,
                0,
                32, 32,
                Gear,
                [&](int x, int y, uint16_t c)
                {
                    mainOS->sprite3.drawPixel(x, y, c);
                },
                0xF800,
                true,
                GOLD);

            mainOS->sprite3.pushRotateZoom(&mainOS->sprite2, -mainOS->CardputerRotationZ * 2 * 0.5, 2, 2);
            mainOS->sprite2.pushSprite(&mainOS->sprite, 95 + slideOffset, 0, BLACK);
            mainOS->sprite2.deleteSprite();
            mainOS->sprite3.deleteSprite();
            // end draw transperent gear
            // draw transperent Gear
            // mainOS->SampleGyroscope();
            mainOS->sprite2.createSprite(80, 80);
            mainOS->sprite3.createSprite(32, 32);
            mainOS->sprite2.setSwapBytes(true);
            mainOS->sprite3.setSwapBytes(true);
            //  mainOS->sprite3.pushImage(0, 0, 32, 32, Gear);
            //  mainOS->sprite3.pushImage(0, 0, 32, 32, Gear);

            drawImageTransparent(
                0,
                0,
                32, 32,
                Gear,
                [&](int x, int y, uint16_t c)
                {
                    mainOS->sprite3.drawPixel(x, y, c);
                },
                0xF800,
                true,
                GOLD);

            mainOS->sprite3.pushRotateZoom(&mainOS->sprite2, -mainOS->CardputerRotationZ * 2 * 0.75, 1.5, 1.5);
            mainOS->sprite2.pushSprite(&mainOS->sprite, 180 + slideOffset, 20, BLACK);
            mainOS->sprite2.deleteSprite();
            mainOS->sprite3.deleteSprite();
        }
        // end draw transperent gear

        break;
    default:
        break;
    }

    // under glow text
    if (!PicAvaliableForThisMenu)

    { // glow text from GPT

        int16_t txtPosY = 95;
        int16_t baseColor = DARKGREY;
        int16_t glowColor = TFT_WHITE;

        static int glowIndex = 0;
        static unsigned long lastGlowMillis = 0;
        static bool waiting = false;

        unsigned long now = millis();

        if (!waiting)
        {
            if (now - lastGlowMillis > 10)
            {
                lastGlowMillis = now;
                glowIndex++;
                if (glowIndex >= BottomTextArray[mainOS->currentMenuIndex].length())
                {
                    glowIndex = -1;
                    waiting = true;
                    lastGlowMillis = now;
                }
            }
        }
        else
        {
            if (now - lastGlowMillis > 3000)
            {
                glowIndex = 0;
                waiting = false;
                lastGlowMillis = now;
            }
        }
        mainOS->sprite.unloadFont();
        mainOS->sprite.setTextSize(1);
        int16_t textWidth = mainOS->sprite.textWidth(BottomTextArray[mainOS->currentMenuIndex]); // make text x position in the middle
        int16_t txtPosX = (mainOS->sprite.width() - textWidth) / 2;
        mainOS->sprite.setCursor(txtPosX, txtPosY);

        for (int i = 0; i < BottomTextArray[mainOS->currentMenuIndex].length(); i++)
        {
            char c = BottomTextArray[mainOS->currentMenuIndex][i];
            mainOS->sprite.setTextColor((i == glowIndex) ? glowColor : baseColor);
            mainOS->sprite.print(c);
        }
        // mainOS->sprite.setTextDatum()
    }

    // end of glow text
    // end under explain text

    // sprite.setCursor(50, 0);
    //  sprite.print(mainOS->CardputerRotationZ);
    for (int i = 0; i < menuCount; i++)
    {
        int x = (i * DistanceBetweenTXT) - cameraX + 10; // nomber is offset from left
        int y = 10;
        mainOS->sprite.setTextColor(SECONDARY_COLOR);
        mainOS->sprite.loadFont(DryBrush15); // Must load the font first

        mainOS->sprite.setTextSize(2);
        mainOS->sprite.setCursor(x, y);
        mainOS->sprite.print(menuItems[i]);
        // M5Cardputer.Display.println(menuItems[i]);
    }
    // start compass
    /*     mainOS->sprite3.createSprite(10, 38);
        mainOS->sprite3.setSwapBytes(true);
        drawImageTransparent(0, 0, 10, 38, needle, [&](int x, int y, uint16_t c)
                             { mainOS->sprite3.drawPixel(x, y, c); }, 0xF800);

        mainOS->sprite2.createSprite(64, 64);
        mainOS->sprite2.setSwapBytes(true);

        mainOS->sprite2.fillScreen(RED);

        mainOS->sprite2.pushImage(0, 0, 64, 64, CompassFull);
        mainOS->sprite3.pushRotated(&mainOS->sprite2, mainOS->CardputerRotationZ, BLACK);

        mainOS->sprite3.deleteSprite(); */
    // end of compass
    // mainOS->CardputerRotationZ;
    // mainOS->sprite2.pushSprite(&mainOS->sprite, 10, 50, 0xF800);
    // mainOS->sprite2.deleteSprite();

    // mainOS->sprite.drawRect(1,1-TopOffset,240-2,135-2,BLUE);
    if (!PicAvaliableForThisMenu)

    {
        mainOS->sprite.drawRoundRect(1, 1, 240 - 2, 135 - TopOffset - 2, 5, SECONDARY_COLOR);
        mainOS->sprite.drawRoundRect(3, 3, 240 - 6, 135 - TopOffset - 6, 5, SECONDARY_COLOR);

        if (RenderINFO)
        {
            int W = 200;
            int H = 80;
            mainOS->sprite2.createSprite(W, H);
            mainOS->sprite2.setTextColor(BLACK);
            mainOS->sprite2.setTextSize(1);
            mainOS->sprite2.unloadFont();

            mainOS->sprite2.fillRect(0, 0, W, H, TFT_WHITE);
            mainOS->sprite2.drawRect(0, 0, W, H, BLACK);
            mainOS->sprite2.setCursor(4, 4);
            mainOS->sprite2.print(infoTextArray[mainOS->currentMenuIndex]);
            mainOS->sprite2.pushSprite(&mainOS->sprite, 10, 10);
            mainOS->sprite2.deleteSprite();
        }
    }
    else
    {
        mainOS->sprite.drawRoundRect(1, 1, 140 - 2, 135 - TopOffset - 2, 5, SECONDARY_COLOR); // if show theme pic
        mainOS->sprite.drawRoundRect(3, 3, 140 - 6, 135 - TopOffset - 6, 5, SECONDARY_COLOR);
    }

    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();

    // RotatedSprite.pushSprite(100,50);
}

void MainMenu::CheackIfHaveThemePictureForThisMenu()
{
    // mainOS->ShowOnScreenMessege("1", 500);
    PicAvaliableForThisMenu = false;
    if (!themeLoaded)
    {
        return;
    }
    // mainOS->ShowOnScreenMessege("2",500);

    CurrentThemePicPath = mainOS->FromFilePathToFolderPath(mainOS->CurrentThemeSelectedPath);
    IMG_x_POS = 140, // sprite partial size is 140
        IMG_y_POS = 18;
    // mainOS->ShowOnScreenMessege(CurrentThemePicPath,4000);
    switch (mainOS->currentMenuIndex)
    {
    case 0:
        CurrentThemePicPath += "/MUSIC.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 1:
        CurrentThemePicPath += "/FILES.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 2:
        CurrentThemePicPath += "/BOT.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 3:
        CurrentThemePicPath += "/NOTES.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 4:
        CurrentThemePicPath += "/PAINT.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 5:
        CurrentThemePicPath += "/EXTRA.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            Serial.println("theme pic not found");
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;

    case 6:
        CurrentThemePicPath += "/GAMES.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            Serial.println("theme pic not found");
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    case 7:
        CurrentThemePicPath += "/SETTING.png";

        if (!SD.exists(CurrentThemePicPath.c_str()))
        {
            Serial.println("theme pic not found");
            return;
        }
        PicAvaliableForThisMenu = true;

        DrawPNG(CurrentThemePicPath.c_str());

        //   mainOS->ShowOnScreenMessege("4",500);

        break;
    default:
        break;
    }
}

void MainMenu::SetRandomStarPos()
{
    for (size_t i = 0; i < STAR_COUNT; i++)
    {
        StarsPosX[i] = random(0, 240);
        StarsPosY[i] = random(0, 135);
    }
}
void MainMenu::RefrashStarPos()
{
    for (size_t i = 0; i < STAR_COUNT; i++)
    {

        StarsPosX[i] += 1;
        StarsPosY[i] += 1;
        // Y wrap
        if (StarsPosY[i] < 0)
            StarsPosY[i] = SCREEN_H;
        else if (StarsPosY[i] > SCREEN_H)
            StarsPosY[i] = 0;

        // X wrap
        if (StarsPosX[i] < 0)
            StarsPosX[i] = SCREEN_W;
        else if (StarsPosX[i] > SCREEN_W)
            StarsPosX[i] = 0;
    }
}

void MainMenu::drawLinesInBackground(M5Canvas &sprite)
{
    // --- Configuration ---
    const int startX = 5;
    const int startY = 5;
    const int rowHeight = 10; // Fixed vertical step for each line
    const int maxWidth = sprite.width() - 15;
    const int maxHeight = sprite.height() - 80;

    static int totalRects = 0;
    totalRects++;

    // --- Phase 1: Count total lines created so far ---
    int tempX = startX;
    int totalLines = 0;

    for (int i = 0; i < totalRects; i++)
    {
        srand(i);
        int rW = rand() % 14 + 4;
        int breakChance = rand() % 100;

        // Logical check: same logic as actual drawing phase
        if (tempX + rW > maxWidth || (tempX > maxWidth / 3 && breakChance < 8))
        {
            tempX = startX;
            totalLines++;
        }
        else
        {
            tempX += (rW + (rand() % 6 + 2));
        }
    }

    // --- Phase 2: Scrolling Calculation ---
    // Calculate how many lines can fit in the sprite
    int maxVisibleLines = maxHeight / rowHeight;
    int scrollOffset = 0;

    // If total lines exceed visibility, shift everything UP
    if (totalLines >= maxVisibleLines)
    {
        scrollOffset = (totalLines - maxVisibleLines + 1) * rowHeight;
    }

    // --- Phase 3: Actual Rendering ---
    int currentX = startX;
    int currentY = startY - scrollOffset; // Apply the scroll shift here

    for (int i = 0; i < totalRects; i++)
    {
        srand(i);
        int rectWidth = rand() % 14 + 4;
        int rectHeight = rand() % 3 + 5;
        int spacing = rand() % 6 + 2;
        int breakChance = rand() % 100;

        // Line break logic
        if (currentX + rectWidth > maxWidth || (currentX > maxWidth / 3 && breakChance < 8))
        {
            currentX = startX;
            currentY += rowHeight;
        }

        // Draw only if it's within the sprite's Y boundaries
        if (currentY >= 0 && currentY < sprite.height())
        {
            // Centering the random height rectangle within the fixed rowHeight
            int yPos = currentY + (rowHeight - rectHeight) / 2;
            sprite.fillRect(currentX, yPos, rectWidth, rectHeight, LIGHTGREY);
        }

        currentX += (rectWidth + spacing);
    }

    // Continuous Loop: Reset when it gets too long
    if (totalLines > 40)
    {
        totalRects = 0;
    }
}