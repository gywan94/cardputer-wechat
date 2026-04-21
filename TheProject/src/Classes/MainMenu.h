#pragma once
#include "./GlobalParentClass.h"
#include <Arduino.h>
#include <M5Cardputer.h>
#include <elapsedMillis.h>
#define STAR_COUNT 20
class MainMenu : public GlobalParentClass
{
public:

    MainMenu(MyOS *os) : GlobalParentClass(os) {}
    ~MainMenu()
    {
    }
    void Begin() override;

    void Loop() override;
    void Draw() override;

private:
// theme thing
bool themeLoaded=false;
bool PicAvaliableForThisMenu=false;
String CurrentThemePicPath;

void CheackIfHaveThemePictureForThisMenu();
// end of theme thing

bool opositAnim=false;
int lastMenuIndex = 0;
float imageAnim = 1.0f;          // 0..1
unsigned long imageAnimStart = 0;
const int imageAnimDuration = 120; // ms

    float StarsPosX[STAR_COUNT];
        float StarsPosY[STAR_COUNT];

void SetRandomStarPos();
void RefrashStarPos();
    int color1R = 0;

    uint8_t color1G = 0;
    uint8_t color1B = 0;
    uint8_t color2R = 0;
    uint8_t color2G = 0;
    uint8_t color2B = 0;
    uint8_t TOcolor1R = 0;
    uint8_t TOcolor1G = 0;
    uint8_t TOcolor1B = 0;
    uint8_t TOcolor2R = 0;
    uint8_t TOcolor2G = 0;
    uint8_t TOcolor2B = 0;
    bool RenderINFO = false;
   // String UnderExplainText = "Play Music From SD Card";

    String menuItems[8] = {"Music", "Files","Mr' BOT", "Notes","Paint", "Extra","Games", "Settings"};
    int menuCount = 8;

    int DistanceBetweenTXT = 250;
        const String BottomTextArray[8] = {"Play Music From SD Card [i] For Info",
             "Explore Files [i] For Info","Talk To (NOT) AI Chatbot","Everyday Notes [i] For Info","Paint Picture [i] For Info","Cool Stuff [i] For Info","Arduboy Games [i] For Info"
             ,"Set The OS Setting [i] For Info"};

    const String infoTextArray[8] = {"Music player\n Press + or - to change Volume\n Press > or < to change song\n press [ or ] to\n fast forward or backwords", "File Explorer\n Move Copy Delete\n Multiple Files\n Text Edit\n Image View","Enjoy To Talk To Our Chatbot","Notes\n Save And Edit\n Multiple Notes","Painter !!\n ESC Open Menu\n See shortcut in Menu\n Hold FN to Move Slower","Extra menu\n Tons Of New Fonction","Games\n Arduboy Games\n Keys: Arrows and Z and X\n Press [ or ] to change volume\n Press 1 to change color palette","Settings\n The OS Setting"};
    float cameraX = 0;
    //float targetCameraX = 0;



elapsedMillis SplashChange;
int InSplash=1;
        int Pos1X, Pos1Y, Pos2X, Pos2Y, Pos3X, Pos3Y;

    void drawLinesInBackground(M5Canvas &sprite);
   // float inTextIndex=0.0;
};
