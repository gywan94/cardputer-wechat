#include "Function.h"
#include "MyOS.h"
#include "M5Cardputer.h"
PNG png;
File pngfile;


int pngDrawCropped(PNGDRAW *pDraw)
{
    if (pDraw->y < cropY || pDraw->y >= cropY + SCREEN_H)
        return 1; 

    uint16_t fullLine[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, fullLine, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    int startX = cropX;
    int endX = cropX + SCREEN_W;

    if (startX < 0)
        startX = 0;
    if (endX > pDraw->iWidth)
        endX = pDraw->iWidth;

    int outW = endX - startX;

    M5Cardputer.Display.pushImage(
        IMG_x_POS,                     
        IMG_y_POS + (pDraw->y - cropY), 
        outW,
        1,
        &fullLine[startX] 
    );

    return 1;
}



int pngDraw(PNGDRAW *pDraw)
{
    int skip = pic_zoom_out + 1;

    if (pic_zoom_out > 0)
    {
        if (pDraw->y % skip != 0)
            return 1;
    }

    uint16_t fullLine[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, fullLine, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (pic_zoom_out == 0)
    {
        M5Cardputer.Display.pushImage(
            IMG_x_POS,
            IMG_y_POS + pDraw->y,
            pDraw->iWidth,
            1,
            fullLine);
        return 1;
    }

    static uint16_t smallLine[MAX_IMAGE_WIDTH];
    int outIndex = 0;

    for (int x = 0; x < pDraw->iWidth; x += skip)
    {
        smallLine[outIndex++] = fullLine[x];
    }

    M5Cardputer.Display.pushImage(
        IMG_x_POS,
        IMG_y_POS + (pDraw->y / skip), 
        outIndex,                     
        1,
        smallLine);

    return 1;
}

void *pngOpen(const char *filename, int32_t *size)
{
    pngfile = SD.open(filename, "r"); 
    if (!pngfile)
        return NULL;
    *size = pngfile.size();
    return &pngfile;
}

void pngClose(void *handle)
{
    if (pngfile)
        pngfile.close();
}

int32_t pngRead(PNGFILE *f, uint8_t *buffer, int32_t length)
{
    return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *f, int32_t position)
{
    return pngfile.seek(position);
}

void DrawPNG(String path)
{
    if (!SD.exists(path))
    {
        Serial.println("PNG not found!");
        return;
    }

    int16_t rc = png.open(
        path.c_str(),
        pngOpen,
        pngClose,
        pngRead,
        pngSeek,
        pngDraw);

    if (rc != PNG_SUCCESS)
    {
        Serial.printf("PNG open failed: %d\n", rc);
        return;
    }

    M5Cardputer.Display.startWrite();

    if (png.getWidth() <= MAX_IMAGE_WIDTH)
    {
        png.decode(NULL, 0);
    }
    else
    {
        os.ShowOnScreenMessege("Image too big", 1000);
        // Serial.println("Image too wide!");
    }
    if (png.getHeight() <= MAX_IMAGE_WIDTH)
    {
        png.decode(NULL, 0);
    }
    else
    {
        os.ShowOnScreenMessege("Image too big", 1000);
        // Serial.println("Image too wide!");
    }
    png.close();
    M5Cardputer.Display.endWrite();
}

void DrawPNGPartial(String path, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY)
{
    cropX = srcX;
    cropY = srcY;
    /*     cropW = srcW;
        cropH = srcH; */

    IMG_x_POS = dstX;
    IMG_y_POS = dstY;

    int16_t rc = png.open(
        path.c_str(),
        pngOpen,
        pngClose,
        pngRead,
        pngSeek,
        pngDrawCropped);

    if (rc != PNG_SUCCESS)
    {
        Serial.println("PNG open failed");
        return;
    }

    M5Cardputer.Display.startWrite();
    png.decode(NULL, 0); 
    png.close();
    M5Cardputer.Display.endWrite();
}



float lerp(float StartPos, float EndPos, float Presentage_0_to_1)
{
        return StartPos + Presentage_0_to_1 * (EndPos - StartPos);
}
int findStringInArrayStringReturnIndex(const String &value, String arr[], int size)
{

    for (int i = 0; i < size; i++)
        if (arr[i] == value)
            return i;

    return -1; 

}


// i think we have alredy built in function like this but still.... 
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
