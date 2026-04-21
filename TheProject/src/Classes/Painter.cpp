#include "Painter.h"
#include "MyOS.h"
#include "MainMenu.h"
#include "FileBrowser.h"
void Painter::Begin()
{
    showTopBar = false;

    if (mainOS->EditImageFromFile)
    {
        PaintFromFile = true;
        mainOS->EditImageFromFile = false;
        LoadBMP_To_PixelsArray_var(mainOS->FileSelectedInFS);
        cursorX = PicSizeX / 2;
        cursorY = PicSizeY / 2;
        Draw();
        return;
    }
    cursorX = PicSizeX / 2;
    cursorY = PicSizeY / 2;
    SetPixelsArrayWHITE_Blank_canvas();
    Draw();
}
void Painter::Loop()
{
    if (messegeDebounce > 0)
    {
        messegeDebounce -= 1;
        if (messegeDebounce <= 0)
        {
            Draw();
        }
    }
    /*     if (mainOS->NewKey.ifKeyJustPress('`'))
        {
            if (mainOS->EditImageFromFile)
            {
                mainOS->ChangeMenu(new FileBrowser(mainOS));
                return;
            }
            else
            {
                mainOS->ChangeMenu(new MainMenu(mainOS));
                return;
            }
        } */
    if (mainOS->NewKey.ifKeyJustPress('`')) // esc
    {
        if (firstPointSet)
        {
            ResetShape();
            Draw();
            return;
        }
        menuOpen = !menuOpen;
        Draw();
        return;
    }

    if (menuOpen)
    {
        /*        if (mainOS->NewKey.ifKeyJustPress(';'))
               {
                   menuSelection--;
                   Draw();
                   DrawMenu();
               } // Up
               if (mainOS->NewKey.ifKeyJustPress('.'))
               {
                   menuSelection++;
                   Draw();
                   DrawMenu();
               } // Down */
        HandleMenuInput();
        //  menuSelection = (menuSelection + menuItemsCount) % menuItemsCount;

        if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
        {

            switch (menuSelection)
            {
            case 0:
                brushSize = (brushSize + 1) % 10;
                break;
            case 1:
                shapeMode = (ShapeMode)((shapeMode + 1) % 8);
                break;
            case 2:
                shapeFilled = !shapeFilled;
                break;
            case 3:
                menuOpen = false;
                currentColor = mainOS->AskForColor("choose color", currentColor);
                Draw();
                if (!menuOpen)
                    return;
                break;
            case 4:

                // String text_to_draw = mainOS->AskFromUserForString("Enter Text:", false);
                menuOpen = false;
                DrawTextToBuffer(mainOS->AskFromUserForString("Enter Text:", false), cursorX, cursorY, brushSize + 1);
                Draw();
                if (!menuOpen)
                    return;
                break;
            case 5: // only save
                SavePic(false);
                break;
            case 6: // only save
                SavePic(false, true);
                break;

            case 7: // save and quit
                SavePic(true);

                break;
            case 8:
                ReloadPicture();
                return;
            case 9:
                mainOS->ChangeMenu(new MainMenu(mainOS));
                return;
            }
            Draw();
            DrawMenu();
        }
        return;
    }

    if (mainOS->NewKey.ifKeyJustPress('p'))
    {
        currentColor = mainOS->AskForColor("Choose color", currentColor);
        Draw();
    }
    if (mainOS->NewKey.ifKeyJustPress('='))
    {
        brushSize += 1;
        messegeDebounce = 8000;
        ShowQuickMessege(String("Brush Size: " + String(brushSize + 1)));
    }
    if (mainOS->NewKey.ifKeyJustPress('-'))
    {
        brushSize -= 1;
        if (brushSize < 0)
        {
            brushSize = 0;
        }
        messegeDebounce = 8000;

        ShowQuickMessege(String("Brush Size: " + String(brushSize + 1)));
    }

    if (mainOS->NewKey.ifKeyJustPress('b'))
    {
        SetShapeMode(NONE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode BRUSH");
    }
    if (mainOS->NewKey.ifKeyJustPress('l'))
    {
        SetShapeMode(LINE_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode LINE");
    }
    if (mainOS->NewKey.ifKeyJustPress('t'))
    {
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode TRIANGLE");
        SetShapeMode(TRIANGLE_MODE);
    }
    if (mainOS->NewKey.ifKeyJustPress('i'))
    {
        SetShapeMode(EYEDROPPER_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("EYEDROPPER Choose color from PIC");
    }
    if (mainOS->NewKey.ifKeyJustPress('w'))
    {
        String text_to_draw = mainOS->AskFromUserForString("Enter Text:", false);
        if (text_to_draw != "")
        {
            DrawTextToBuffer(text_to_draw, cursorX, cursorY, brushSize + 1);
            Draw();
        }
    }
    if (mainOS->NewKey.ifKeyJustPress('c'))
    {
        SetShapeMode(CIRCLE_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode CIRCLE");
    }
    if (mainOS->NewKey.ifKeyJustPress('g'))
    {
        SetShapeMode(BUCKET_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode FILL BUCKET");
    }
    if (mainOS->NewKey.ifKeyJustPress('r'))
    {
        SetShapeMode(RECT_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode Rectangle");
    }
    if (mainOS->NewKey.ifKeyJustPress('e'))
    {
        SetShapeMode(ELLIPSE_MODE);
        messegeDebounce = 8000;
        ShowQuickMessege("Draw mode ELLIPSE");
    }
    if (mainOS->NewKey.ifKeyJustPress('f'))
    {
        shapeFilled = !shapeFilled;
        messegeDebounce = 8000;
        ShowQuickMessege(shapeFilled ? "Fill: ON" : "Fill: OFF");
    }
    if (mainOS->NewKey.ifKeyJustPress('s'))
    {
        SavePic(false);
    }
    if (mainOS->NewKey.ifKeyJustPress('a'))
    {
        ReloadPicture();
    }
    if (mainOS->NewKey.ifKeyJustPress('m'))
    {
        messegeDebounce = 8000;
        ShowQuickMessege(getHeapInfoKB());
    }

    bool moved = false;
    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
    {
        if (shapeMode == NONE)
        {
            DrawBrushToBuffer();
            Draw();
        }
        else
        {
            HandleShapeLogic();
        }
    }

    int oldX = cursorX;
    int oldY = cursorY;

    if (mainOS->NewKey.KeyPressWorkInMilliSecond(';', GetMoveTickDelay()))
    {
        cursorY--;
        moved = true;
    }
    if (mainOS->NewKey.KeyPressWorkInMilliSecond('.', GetMoveTickDelay()))
    {
        cursorY++;
        moved = true;
    }
    if (mainOS->NewKey.KeyPressWorkInMilliSecond(',', GetMoveTickDelay()))
    {
        cursorX--;
        moved = true;
    }
    if (mainOS->NewKey.KeyPressWorkInMilliSecond('/', GetMoveTickDelay()))
    {
        cursorX++;
        moved = true;
    }
    cursorX = constrain(cursorX, 0, PicSizeX - 1);
    cursorY = constrain(cursorY, 0, PicSizeY - 1);

    if (drawing)
    {
        int index = cursorY * PicSizeX + cursorX;
        PixelsArray[index] = currentColor;
        DrawBrushToBuffer();
    }

    if (moved || drawing)
    {
        Draw();
    }
}

void Painter::Draw()
{
    if (PicSizeX < 240 || PicSizeY < 135)
    {
        M5Cardputer.Display.fillScreen(BLACK);
    }

    Draw_PixelsArray_Var_toScreen();
    DrawCursor();

    if (firstPointSet)
    {
        if (shapeMode == LINE_MODE)
        {
            M5.Lcd.drawLine(x1, y1, cursorX, cursorY, TFT_RED);
        }
        else if (shapeMode == TRIANGLE_MODE)
        {
            if (!secondPointSet)
            {
                M5.Lcd.drawLine(x1, y1, cursorX, cursorY, TFT_RED);
            }
            else
            {
                M5.Lcd.drawLine(x1, y1, x2, y2, TFT_RED);
                M5.Lcd.drawLine(x2, y2, cursorX, cursorY, TFT_RED);
                M5.Lcd.drawLine(cursorX, cursorY, x1, y1, TFT_RED);
            }
        }
        else if (shapeMode == CIRCLE_MODE)
        {
            int dx = cursorX - x1;
            int dy = cursorY - y1;
            int radius = sqrt(dx * dx + dy * dy);

            M5.Lcd.drawCircle(x1, y1, radius, TFT_RED);
        }
        else if (shapeMode == RECT_MODE)
        {
            int xMin = min(x1, cursorX);
            int xMax = max(x1, cursorX);
            int yMin = min(y1, cursorY);
            int yMax = max(y1, cursorY);

            M5.Lcd.drawRect(xMin, yMin, xMax - xMin, yMax - yMin, TFT_RED);
        }
        else if (shapeMode == ELLIPSE_MODE && firstPointSet)
        {
            int rx = abs(cursorX - x1);
            int ry = abs(cursorY - y1);

            M5.Lcd.drawEllipse(x1, y1, rx, ry, TFT_RED);
        }
    }
    if (menuOpen)
    {
        DrawMenu();
    }
}

void Painter::DrawMenu()
{
    int menuW = 180;
    int menuH = 135;
    int menuX = 20;
    int menuY = 2;

    M5.Lcd.fillRect(menuX, menuY, menuW, menuH, TFT_DARKGREY);
    M5.Lcd.drawRect(menuX, menuY, menuW, menuH, TFT_WHITE);
    M5.Lcd.drawRect(menuX + 2, menuY + 2, menuW - 4, menuH - 4, TFT_BLACK);

    // header
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.setTextSize(1);
     M5.Lcd.unloadFont();
    M5.Lcd.setCursor(menuX + 10, menuY + 8);
    M5.Lcd.print("--- PAINTER MENU ---");

    String options[] = {
        "Brush Size(+/-): " + String(brushSize + 1),
        "Shape: " + getShapeName(shapeMode),
        "Fill Mode: " + String(shapeFilled ? "[ON]" : "[OFF]"),
        "Current Color(P): ",
        "Enter Word (W)",
        "Save  Image (S)",
        "Save As New Image",
        "Save And Quit Image",
        "Reload Picture From File (A)",
        "Exit"};

    for (int i = 0; i < visibleItems; i++)
    {
        int itemIndex = scrollOffset + i;
        if (itemIndex >= menuItemsCount)
            break;

        int itemY = menuY + 22 + (i * 14);

        if (itemIndex == menuSelection)
        {
            M5.Lcd.fillRect(menuX + 5, itemY - 2, menuW - 10, 12, TFT_BLUE);
            M5.Lcd.setTextColor(TFT_WHITE);
        }
        else
        {
            M5.Lcd.setTextColor(TFT_LIGHTGREY);
        }

        if (itemIndex == 3)
        {
            int swatchX = menuX + 110;
            M5.Lcd.fillRect(swatchX, itemY - 1, 15, 9, currentColor);
            M5.Lcd.drawRect(swatchX - 1, itemY - 2, 17, 11, TFT_WHITE);
        }

        M5.Lcd.setCursor(menuX + 10, itemY);
        M5.Lcd.print(options[itemIndex]);
    }

    M5.Lcd.setTextColor(TFT_CYAN);
    M5.Lcd.setCursor(menuX + 10, menuY + menuH - 12);
    M5.Lcd.print("Arrows to move");
}

String Painter::getShapeName(ShapeMode mode)
{
    switch (mode)
    {
    case NONE:
        return "Free Hand (B)";
    case LINE_MODE:
        return "Line (L)";
    case TRIANGLE_MODE:
        return "Triangle (T)";
    case CIRCLE_MODE:
        return "Circle (C)";
    case RECT_MODE:
        return "Rectangle (R)";
    case ELLIPSE_MODE:
        return "Ellipse (E)";
    case BUCKET_MODE:
        return "Bucket (G)";
    case EYEDROPPER_MODE:
        return "Eyedropper (I)";
    default:
        return "Unknown";
    }
}

void Painter::ShowQuickMessege(String Messege)
{
    M5Cardputer.Display.fillRect(20, 3, 200, 25, YELLOW);
    M5Cardputer.Display.drawRect(20, 3, 200, 25, BLACK);
    M5Cardputer.Display.setCursor(25, 7);
    M5Cardputer.Display.unloadFont();
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(BLACK);
    M5Cardputer.Display.print(Messege);
}

void Painter::ReloadPicture()
{
    if (!PaintFromFile)
    {
        messegeDebounce = 8000;
        ShowQuickMessege("The paint Not Save In Any File");
    }
    else
    {
        menuOpen = false;
        messegeDebounce = 8000;
        ShowQuickMessege("Reload The Image");
        LoadBMP_To_PixelsArray_var(mainOS->FileSelectedInFS);
        //  cursorX = PicSizeX / 2;
        // cursorY = PicSizeY / 2;
        Draw();
    }
}

int Painter::GetMoveTickDelay()
{
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN))
    {
        return 40;
    }
    return 12;
}

void Painter::LoadBMP_To_PixelsArray_var(String BMP_PATH)
{

    File bmpFile = SD.open(BMP_PATH.c_str(), FILE_READ);
    if (!bmpFile)
    {
        Serial.println("Error: Could not open file");
        return;
    }

    uint8_t header[54];
    bmpFile.read(header, 54);

    int32_t width = *(int32_t *)&header[18];
    int32_t height = *(int32_t *)&header[22];

    PicSizeX = width;
    PicSizeY = abs(height);
    if (PicSizeX > 240)
    {
        mainOS->ShowOnScreenMessege("BMP image width large then 240 pixel , the painter support only 240 PX  or less");
        bmpFile.close();
        return;
    }
    if (PicSizeY > 135)
    {
        mainOS->ShowOnScreenMessege("BMP image hight large then 135 pixel , the painter support only 135 PX  or less");
        bmpFile.close();
        return;
    }

    if (PicSizeX * PicSizeY > 32400)
    {
        Serial.println("Error: Image too large for the array!");
        bmpFile.close();
        return;
    }

    bool flip = true;
    if (height < 0)
    {
        height = -height;
        flip = false;
    }

    int rowSize = (width * 3 + 3) & ~3;

    uint8_t lineBuffer[width * 3];

    for (int y = 0; y < PicSizeY; y++)
    {
        int pos = flip ? (height - 1 - y) : y;

        bmpFile.seek(54 + (pos * rowSize));
        bmpFile.read(lineBuffer, width * 3);

        for (int x = 0; x < PicSizeX; x++)
        {
            uint8_t b = lineBuffer[x * 3];
            uint8_t g = lineBuffer[x * 3 + 1];
            uint8_t r = lineBuffer[x * 3 + 2];

            uint16_t pixelColor = ((r & 0xF8) << 8) |
                                  ((g & 0xFC) << 3) |
                                  ((b & 0xF8) >> 3);

            PixelsArray[y * PicSizeX + x] = pixelColor;
        }
    }

    bmpFile.close();
}

void Painter::SetPixelsArrayWHITE_Blank_canvas()
{
    int totalPixels = PicSizeX * PicSizeY;

    for (int i = 0; i < totalPixels; i++)
    {
        PixelsArray[i] = 0xFFFF;
    }
}

void Painter::Draw_PixelsArray_Var_toScreen()
{
    M5.Lcd.setSwapBytes(true);

    M5.Lcd.startWrite();
    M5.Lcd.pushImage(0, 0, PicSizeX, PicSizeY, PixelsArray);
    M5.Lcd.endWrite();
    M5.Lcd.setSwapBytes(false);
}

void Painter::DrawCursor()
{
    int size = 2;

    for (int i = -size; i <= size; i++)
    {
        M5.Lcd.drawPixel(cursorX + i, cursorY + i, TFT_RED);

        M5.Lcd.drawPixel(cursorX + i, cursorY - i, TFT_RED);
    }
}

void Painter::DrawBrushToBuffer()
{
    for (int dy = -brushSize; dy <= brushSize; dy++)
    {
        for (int dx = -brushSize; dx <= brushSize; dx++)
        {
            if (dx * dx + dy * dy > brushSize * brushSize)
                continue;

            int px = cursorX + dx;
            int py = cursorY + dy;

            if (px < 0 || px >= PicSizeX ||
                py < 0 || py >= PicSizeY)
                continue;

            PixelsArray[py * PicSizeX + px] = currentColor;
        }
    }
}

void Painter::DrawLineToBuffer(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        // PixelsArray[y0 * PicSizeX + x0] = currentColor;
        DrawThickPixel(x0, y0);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void Painter::DrawTriangleToBuffer(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (!shapeFilled)
    {
        DrawLineToBuffer(x1, y1, x2, y2);
        DrawLineToBuffer(x2, y2, x3, y3);
        DrawLineToBuffer(x3, y3, x1, y1);
    }
    else
    {

        FillTriangleHelper(x1, y1, x2, y2, x3, y3);
    }
}

void Painter::ResetShape()
{
    firstPointSet = false;
    secondPointSet = false;
}

void Painter::SetShapeMode(ShapeMode mode)
{
    if (shapeMode == (ShapeMode)EYEDROPPER_MODE)
    {
        if (mode == (ShapeMode)EYEDROPPER_MODE)
        {
            return;
        }
    }// if eyedropper alredy choosen and the user choose again it will failed for after the eydroper pick color it will return ti old shape mode
    oldShapeMode = shapeMode;
    shapeMode = mode;

    firstPointSet = false;
    secondPointSet = false;
}
void Painter::HandleShapeLogic()
{
    if (shapeMode == EYEDROPPER_MODE)
    {
        int index = cursorY * PicSizeX + cursorX;

        if (index >= 0 && index < (PicSizeX * PicSizeY))
        {
            currentColor = PixelsArray[index];
            messegeDebounce = 8000;

            ShowQuickMessege("Color Picked!");

            shapeMode = oldShapeMode;
        }
    }

    else if (!firstPointSet)
    {
        x1 = cursorX;
        y1 = cursorY;
        firstPointSet = true;

        Draw(); // preview
    }
    else if (shapeMode == LINE_MODE)
    {
        DrawLineToBuffer(x1, y1, cursorX, cursorY);
        ResetShape();
        Draw();
    }
    else if (shapeMode == TRIANGLE_MODE)
    {
        if (!secondPointSet)
        {
            x2 = cursorX;
            y2 = cursorY;
            secondPointSet = true;

            Draw();
        }
        else
        {
            DrawTriangleToBuffer(x1, y1, x2, y2, cursorX, cursorY);
            ResetShape();
            Draw();
        }
    }
    else if (shapeMode == CIRCLE_MODE)
    {
        int dx = cursorX - x1;
        int dy = cursorY - y1;
        int radius = sqrt(dx * dx + dy * dy);

        DrawCircleToBuffer(x1, y1, radius);
        ResetShape();
        Draw();
    }
    else if (shapeMode == ELLIPSE_MODE)
    {
        if (!firstPointSet)
        {
            x1 = cursorX;
            y1 = cursorY;
            firstPointSet = true;
            Draw();
        }
        else
        {
            int rx = abs(cursorX - x1);
            int ry = abs(cursorY - y1);

            DrawEllipseToBuffer(x1, y1, rx, ry);
            ResetShape();
            Draw();
        }
    }
    else if (shapeMode == BUCKET_MODE)
    {
        FillBucket();
        // FloodFill(cursorX, cursorY);
        Draw();
    }

    else if (shapeMode == RECT_MODE)
    {
        if (!firstPointSet)
        {
            x1 = cursorX;
            y1 = cursorY;
            firstPointSet = true;
            Draw();
        }
        else
        {
            DrawSquareToBuffer(x1, y1, cursorX, cursorY);
            ResetShape();
            Draw();
        }
    }
}

void Painter::DrawCircleToBuffer(int cx, int cy, int r)
{
    if (r <= 0)
    {
        DrawThickPixel(cx, cy);
        return;
    }

    if (!shapeFilled)
    {
        int x = r, y = 0;
        int err = 1 - x;

        while (x >= y)
        {
            DrawThickPixel(cx + x, cy + y);
            DrawThickPixel(cx + y, cy + x);
            DrawThickPixel(cx - y, cy + x);
            DrawThickPixel(cx - x, cy + y);
            DrawThickPixel(cx - x, cy - y);
            DrawThickPixel(cx - y, cy - x);
            DrawThickPixel(cx + y, cy - x);
            DrawThickPixel(cx + x, cy - y);

            y++;
            if (err < 0)
            {
                err += 2 * y + 1;
            }
            else
            {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
    else
    {

        int originalBrush = brushSize;
        brushSize = 0;

        for (int y = -r; y <= r; y++)
        {
            int x = (int)sqrt(r * r - y * y);
            DrawLineToBuffer(cx - x, cy + y, cx + x, cy + y);
        }

        brushSize = originalBrush;
    }
}

void Painter::PlotCirclePoints(int cx, int cy, int x, int y)
{
    auto plot = [&](int px, int py)
    {
        if (px >= 0 && px < PicSizeX && py >= 0 && py < PicSizeY)
            PixelsArray[py * PicSizeX + px] = currentColor;
    };

    plot(cx + x, cy + y);
    plot(cx - x, cy + y);
    plot(cx + x, cy - y);
    plot(cx - x, cy - y);
    plot(cx + y, cy + x);
    plot(cx - y, cy + x);
    plot(cx + y, cy - x);
    plot(cx - y, cy - x);
}

void Painter::DrawThickPixel(int cx, int cy)
{
    for (int y = -brushSize; y <= brushSize; y++)
    {
        for (int x = -brushSize; x <= brushSize; x++)
        {
            if (x * x + y * y <= brushSize * brushSize)
            {
                int px = cx + x;
                int py = cy + y;

                if (px >= 0 && px < PicSizeX &&
                    py >= 0 && py < PicSizeY)
                {
                    PixelsArray[py * PicSizeX + px] = currentColor;
                }
            }
        }
    }
}

/* void Painter::FloodFill(int startX, int startY)
{
    uint16_t targetColor = PixelsArray[startY * PicSizeX + startX];

    if (targetColor == currentColor)
        return;

    std::vector<std::pair<int, int>> stack;
    stack.push_back({startX, startY});

    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();

        int index = y * PicSizeX + x;

        if (PixelsArray[index] != targetColor)
            continue;

        PixelsArray[index] = currentColor;

        if (x > 0)
            stack.push_back({x - 1, y});
        if (x < PicSizeX - 1)
            stack.push_back({x + 1, y});
        if (y > 0)
            stack.push_back({x, y - 1});
        if (y < PicSizeY - 1)
            stack.push_back({x, y + 1});
    }
} */

void Painter::DrawSquareToBuffer(int ax, int ay, int bx, int by)
{
    int xMin = min(ax, bx);
    int xMax = max(ax, bx);
    int yMin = min(ay, by);
    int yMax = max(ay, by);

    if (!shapeFilled)
    {
        DrawLineToBuffer(xMin, yMin, xMax, yMin);
        DrawLineToBuffer(xMin, yMax, xMax, yMax);
        DrawLineToBuffer(xMin, yMin, xMin, yMax);
        DrawLineToBuffer(xMax, yMin, xMax, yMax);
    }
    else
    {
        for (int y = yMin; y <= yMax; y++)
        {
            DrawLineToBuffer(xMin, y, xMax, y);
        }
    }
}

#include <queue>
#include <algorithm>

void Painter::FillBucket()
{
    uint16_t targetColor = PixelsArray[cursorY * PicSizeX + cursorX];

    if (currentColor == targetColor)
        return;

    std::queue<int> q;
    int startX = cursorX;
    int startY = cursorY;

    if (startX < 0 || startX >= PicSizeX || startY < 0 || startY >= PicSizeY)
    {
        return;
    }

    int startIndex = startY * PicSizeX + startX;

    q.push(startIndex);
    PixelsArray[startIndex] = currentColor;

    while (!q.empty())
    {
        int currentIndex = q.front();
        q.pop();

        int x = currentIndex % PicSizeX;
        int y = currentIndex / PicSizeX;

        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; ++i)
        {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx >= 0 && nx < PicSizeX && ny >= 0 && ny < PicSizeY)
            {
                int nIndex = ny * PicSizeX + nx;

                if (PixelsArray[nIndex] == targetColor)
                {
                    PixelsArray[nIndex] = currentColor;
                    q.push(nIndex);
                }
            }
        }
    }
}

bool Painter::saveBMP(String fileName, uint16_t *pixelsArray, int width, int height)
{

    // 1. Check if the SD card is initialized
    if (!SD.begin())
    {
        os.ShowOnScreenMessege("no sd");
        return false;
    }
    if (!PaintFromFile)
    {
        if (SD.exists(fileName.c_str()))
        {
            if (mainOS->AskSomthing("File alredy Exist Writing over the existing file?"))
            {
            }
            else
            {
                return false;
            }
        }
    }
    // Open file for writing (overwrite existing)
    File file = SD.open(fileName, FILE_WRITE);
    if (!file)
    {
        os.ShowOnScreenMessege("eror open file");

        return false;
    }

    // 2. Calculate Row Padding
    // BMP requires each row to be a multiple of 4 bytes (32-bit alignment).
    // Each pixel in RGB888 is 3 bytes.
    int rowBytes = width * 3;
    int padding = (4 - (rowBytes % 4)) % 4;

    // 3. Calculate Total File Size
    // Header (54 bytes) + Data ((RowSize + Padding) * Height)
    uint32_t dataSize = (rowBytes + padding) * height;
    uint32_t fileSize = 54 + dataSize;

    // 4. Prepare BMP Header
    unsigned char header[54] = {
        'B', 'M',    // Signature
        0, 0, 0, 0,  // File Size (filled below)
        0, 0, 0, 0,  // Reserved
        54, 0, 0, 0, // Offset to Pixel Array
        40, 0, 0, 0, // Info Header Size
        0, 0, 0, 0,  // Width (filled below)
        0, 0, 0, 0,  // Height (filled below)
        1, 0,        // Planes
        24, 0,       // Bits per Pixel (RGB888)
        0, 0, 0, 0,  // Compression
        0, 0, 0, 0,  // Image Size (can be 0 for uncompressed)
        0, 0, 0, 0,  // Horizontal Resolution
        0, 0, 0, 0,  // Vertical Resolution
        0, 0, 0, 0,  // Colors Used
        0, 0, 0, 0   // Important Colors
    };

    // Fill Header Fields (Little Endian)
    header[2] = fileSize & 0xFF;
    header[3] = (fileSize >> 8) & 0xFF;
    header[4] = (fileSize >> 16) & 0xFF;
    header[5] = (fileSize >> 24) & 0xFF;

    header[18] = width & 0xFF;
    header[19] = (width >> 8) & 0xFF;
    header[20] = (width >> 16) & 0xFF;
    header[21] = (width >> 24) & 0xFF;

    // Note: Height is stored as positive for "Bottom-Up" BMP format.
    // We will write the data from bottom to top.
    header[22] = height & 0xFF;
    header[23] = (height >> 8) & 0xFF;
    header[24] = (height >> 16) & 0xFF;
    header[25] = (height >> 24) & 0xFF;

    // Write Header
    file.write(header, 54);

    // Buffer for one row of RGB888 data + padding
    // Max width is usually small enough to fit in stack, but if >1920 it might be tight.
    // Assuming typical screen sizes, this is safe. If needed, allocate on heap.
    int bufferSize = (width * 3) + padding;
    unsigned char *rowBuffer = new unsigned char[bufferSize];

    if (!rowBuffer)
    {
        file.close();
        os.ShowOnScreenMessege("33");

        return false;
    }

    // 5. Convert RGB565 to RGB888 and Write Rows (Bottom-Up)
    for (int y = height - 1; y >= 0; y--)
    {
        int rowOffset = y * width;

        // Clear padding area just in case
        memset(rowBuffer, 0, bufferSize);

        for (int x = 0; x < width; x++)
        {
            uint16_t pixel565 = pixelsArray[rowOffset + x];

            // Extract RGB from 565 format
            // R: bits 15-11 (shift right 11) -> *8 to get 8-bit
            int r = ((pixel565 >> 11) & 0x1F);
            // G: bits 10-5 (mask 0x3F, shift 5) -> *4 to get 8-bit
            int g = ((pixel565 >> 5) & 0x3F);
            // B: bits 4-0 (mask 0x1F) -> *8 to get 8-bit
            int b = (pixel565 & 0x1F);

            // Expand to 8-bit precision (simple scaling by multiplying)
            r = (r << 3) | (r >> 2); // 5 bits -> 8 bits
            g = (g << 2) | (g >> 4); // 6 bits -> 8 bits
            b = (b << 3) | (b >> 2); // 5 bits -> 8 bits

            // BMP stores as BGR, so we write Blue first, then Green, then Red
            rowBuffer[x * 3] = b;
            rowBuffer[x * 3 + 1] = g;
            rowBuffer[x * 3 + 2] = r;
        }

        // Write the row (including padding) to file
        if (!file.write(rowBuffer, bufferSize))
        {
            delete[] rowBuffer;
            file.close();
            os.ShowOnScreenMessege("55");

            return false;
        }
    }

    delete[] rowBuffer;
    file.close();
    return true;
}

void Painter::DrawEllipseToBuffer(int x0, int y0, int rx, int ry)
{
    if (rx <= 0 || ry <= 0)
    {
        DrawThickPixel(x0, y0);
        return;
    }
    if (!shapeFilled)
    {
        long x = -rx, y = 0; /* II. quadrant */
        long e2 = ry, dx = (1 + 2 * x) * e2 * e2;
        long dy = x * x, err = dx + dy;

        do
        {
            DrawThickPixel(x0 - x, y0 + y); /* I. Quadrant */
            DrawThickPixel(x0 + x, y0 + y); /* II. Quadrant */
            DrawThickPixel(x0 + x, y0 - y); /* III. Quadrant */
            DrawThickPixel(x0 - x, y0 - y); /* IV. Quadrant */

            e2 = 2 * err;
            if (e2 >= dx)
            {
                x++;
                err += dx += 2 * (long)ry * ry;
            }
            if (e2 <= dy)
            {
                y++;
                err += dy += 2 * (long)rx * rx;
            }
        } while (x <= 0);

        while (y++ < ry)
        {
            DrawThickPixel(x0, y0 + y);
            DrawThickPixel(x0, y0 - y);
        }
    }
    else
    {
        for (int y = -ry; y <= ry; y++)
        {
            int x = sqrt(1.0 - (double)(y * y) / (ry * ry)) * rx;
            DrawLineToBuffer(x0 - x, y0 + y, x0 + x, y0 + y);
        }
    }
}
void Painter::FillTriangleHelper(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (y1 > y2)
    {
        std::swap(y1, y2);
        std::swap(x1, x2);
    }
    if (y1 > y3)
    {
        std::swap(y1, y3);
        std::swap(x1, x3);
    }
    if (y2 > y3)
    {
        std::swap(y2, y3);
        std::swap(x2, x3);
    }

    if (y1 == y3)
        return;

    int originalBrush = brushSize;
    brushSize = 0;

    int x4 = x1 + (int)((float)(y2 - y1) / (y3 - y1) * (x3 - x1));

    FillFlatTriangle(x1, y1, x2, y2, x4, y2);

    FillFlatTriangle(x3, y3, x2, y2, x4, y2);

    brushSize = originalBrush;
}
void Painter::FillFlatTriangle(int v1x, int v1y, int v2x, int v2y, int v3x, int v3y)
{
    float invslope1 = (float)(v2x - v1x) / (v2y - v1y);
    float invslope2 = (float)(v3x - v1x) / (v3y - v1y);

    float curx1 = v1x;
    float curx2 = v1x;

    if (v1y < v2y)
    {
        for (int scanlineY = v1y; scanlineY <= v2y; scanlineY++)
        {
            DrawLineToBuffer((int)curx1, scanlineY, (int)curx2, scanlineY);
            curx1 += invslope1;
            curx2 += invslope2;
        }
    }
    else
    {
        for (int scanlineY = v1y; scanlineY > v2y; scanlineY--)
        {
            DrawLineToBuffer((int)curx1, scanlineY, (int)curx2, scanlineY);
            curx1 -= invslope1;
            curx2 -= invslope2;
        }
    }
}

void Painter::DrawTextToBuffer(String text, int x, int y, uint8_t size)
{
    if (text.length() == 0)
        return;

    LGFX_Sprite tempSprite(&M5.Lcd);

    tempSprite.setTextSize(size);
    int textW = tempSprite.textWidth(text) + 2;
    int textH = 8 * size;

    if (!tempSprite.createSprite(textW, textH))
        return;

    tempSprite.fillSprite(0x0000);
    tempSprite.setTextColor(0xFFFF);
    tempSprite.setCursor(0, 0);
    tempSprite.print(text);

    for (int iY = 0; iY < textH; iY++)
    {
        for (int iX = 0; iX < textW; iX++)
        {

            uint16_t pixel = tempSprite.readPixel(iX, iY);

            if (pixel != 0x0000)
            {
                int targetX = x + iX;
                int targetY = y + iY;

                if (targetX >= 0 && targetX < PicSizeX && targetY >= 0 && targetY < PicSizeY)
                {
                    PixelsArray[targetY * PicSizeX + targetX] = currentColor;
                }
            }
        }
    }

    tempSprite.deleteSprite();
}

void Painter::HandleMenuInput()
{
    bool changed = false;

    // DOWN
    if (mainOS->NewKey.ifKeyJustPress('.'))
    {
        menuSelection++;
        if (menuSelection >= menuItemsCount)
            menuSelection = 0; // wrap to start

        changed = true;
    }

    // UP
    if (mainOS->NewKey.ifKeyJustPress(';'))
    {
        menuSelection--;
        if (menuSelection < 0)
            menuSelection = menuItemsCount - 1; // wrap
        changed = true;
    }

    if (mainOS->NewKey.ifKeyJustPress('/'))
    {
        switch (menuSelection)
        {
        case 0:
            brushSize = (brushSize + 1) % 10;
            changed = true;

            break;

        case 1:
            shapeMode = (ShapeMode)((shapeMode + 1) % 8);
            changed = true;

            break;
        }
    }
    // LEFT
    if (mainOS->NewKey.ifKeyJustPress(','))
    {
        switch (menuSelection)
        {
        case 0:
            brushSize = (brushSize - 1) % 10;
            if (brushSize < 0)
            {
                brushSize = 0;
            }
            changed = true;

            break;
        case 1:
            shapeMode = (ShapeMode)((shapeMode - 1) % 8);
            changed = true;
            if ((ShapeMode)(shapeMode) < 0)
            {
                shapeMode = (ShapeMode)0;
            }
            break;
        }
    }

    if (!changed)
        return;

    // update scroll
    if (menuSelection < scrollOffset)
        scrollOffset = menuSelection;

    if (menuSelection >= scrollOffset + visibleItems)
        scrollOffset = menuSelection - visibleItems + 1;
    Draw();
    DrawMenu();
}

void Painter::SavePic(bool Exit, bool forceNewFile)
{
    menuOpen = false;
    if (PaintFromFile && forceNewFile == false)
    {
        bool success = saveBMP(mainOS->FileSelectedInFS.c_str(), PixelsArray, PicSizeX, PicSizeY);
        if (success)
        {
            messegeDebounce = 8000;
            ShowQuickMessege(String(mainOS->FileSelectedInFS + " Saved"));
            if (Exit)
            {
                mainOS->ChangeMenu(new MainMenu(mainOS));
                return;
            }
        }
    }
    else
    {
        SD.mkdir("/AdvanceOS/Paint"); // folder for the theme
        String Name = "/AdvanceOS/Paint/";
        String input = mainOS->AskFromUserForString("Enter File Name");
        Name += input;
        Name += ".bmp";
        bool success = saveBMP(Name.c_str(), PixelsArray, PicSizeX, PicSizeY);
        if (success)
        {
            messegeDebounce = 8000;
            ShowQuickMessege("save Compleate in AdvanceOS/Paint");
            //  mainOS->ChangeMenu(new MainMenu(mainOS));
            // return;
            PaintFromFile = true;
            mainOS->FileSelectedInFS = Name;
            if (Exit)
            {
                mainOS->ChangeMenu(new MainMenu(mainOS));
                return;
            }
        }
    }
}
