#pragma once

#include "./GlobalParentClass.h"
#include <Arduino.h>
#include <M5Cardputer.h>

class Painter : public GlobalParentClass
{
public:
bool PaintFromFile=false;
    Painter(MyOS *os) : GlobalParentClass(os) {}
    int PicSizeX = 240;
    int PicSizeY = 135;
    int cursorY;
    int cursorX;
    uint16_t currentColor = BLACK;
    uint16_t PixelsArray[32400]; // variable thet contain the corrent picture pixels for the cardputer RGB565

   // uint16_t UndoPic1[32400]; // variable thet contain the corrent picture pixels for the cardputer RGB565
   // uint16_t UndoPic2[32400]; // variable thet contain the corrent picture pixels for the cardputer RGB565
    //uint16_t UndoPic3[32400]; // variable thet contain the corrent picture pixels for the cardputer RGB565

    void Begin() override;

    void Loop() override;
    void Draw() override;
void DrawMenu();
private:
    int brushSize = 0;
bool shapeFilled = false;
enum ShapeMode { NONE, LINE_MODE, TRIANGLE_MODE, CIRCLE_MODE, RECT_MODE, BUCKET_MODE , ELLIPSE_MODE ,EYEDROPPER_MODE};
    ShapeMode shapeMode = NONE;
    ShapeMode oldShapeMode=NONE;
    void LoadBMP_To_PixelsArray_var(String BMP_PATH);
    void SetPixelsArrayWHITE_Blank_canvas();
    void Draw_PixelsArray_Var_toScreen();
    void DrawCursor();
    void DrawBrushToBuffer();
    void DrawTriangleToBuffer(int ax, int ay, int bx, int by, int cx, int cy);
    void DrawLineToBuffer(int x0, int y0, int x1, int y1);
    void DrawCircleToBuffer(int cx, int cy, int r);
    void PlotCirclePoints(int cx, int cy, int x, int y);
void DrawThickPixel(int cx, int cy);
void DrawEllipseToBuffer(int cx, int cy, int rx, int ry);
void FillTriangleHelper(int x1, int y1, int x2, int y2, int x3, int y3);
void FillFlatTriangle(int v1x, int v1y, int v2x, int v2y, int v3x, int v3y);

//void FloodFill(int startX, int startY);
void DrawSquareToBuffer(int ax, int ay, int bx, int by);
void FillBucket();
// text
void DrawTextToBuffer(String text, int x, int y, uint8_t size);
//bool getPixelFromFont(char c, int x, int y);


bool saveBMP(String fileName, uint16_t* pixelsArray, int width, int height);





    void ResetShape();
    void SetShapeMode(ShapeMode mode);
    void HandleShapeLogic();
   // int brushMode = 0; // 0=pixel 1=square 2=circle

    bool drawing = false;

    bool firstPointSet = false;
    bool secondPointSet = false;
    int x1, y1;
    int x2, y2;

void HandleMenuInput();
void SavePic(bool Exit,bool forceNewFile=false);
    bool menuOpen = false;
int menuSelection = 0;
const int menuItemsCount = 10;

String getShapeName(ShapeMode mode);
int scrollOffset = 0;
const int visibleItems = 6;  

int messegeDebounce=0;
void ShowQuickMessege(String Messege);

void ReloadPicture();
int GetMoveTickDelay();
};
