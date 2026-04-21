#pragma once
#include <Arduino.h>



// PNG stuff

#include <PNGdec.h>
inline int cropX = 0, cropY = 0; 
#define MAX_IMAGE_WIDTH 400
inline int pic_zoom_out = 0; // 0 = normal, 1 = half, 2 = quarter...
inline int16_t IMG_x_POS = 0, IMG_y_POS = 0;


void DrawPNGPartial(String path, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY);
void DrawPNG(String path);
int pngDrawCropped(PNGDRAW *pDraw);





#ifndef DRAW_IMAGE_TRANSPARENT_H
#define DRAW_IMAGE_TRANSPARENT_H
template <typename DrawFunc>
void drawImageTransparent(
    int16_t x, int16_t y,
    int imgW, int imgH,
    const uint16_t *img,
    DrawFunc drawer,
    uint16_t TransparentColor,
    bool useOverrideColor = false,
    uint16_t OverrideColor = 0
)
{
    for (int j = 0; j < imgH; j++)
    {
        for (int i = 0; i < imgW; i++)
        {
            uint16_t c = img[j * imgW + i];
            if (c == TransparentColor) continue;

            drawer(x + i, y + j,
                   useOverrideColor ? OverrideColor : c);
        }
    }
}

#endif






float lerp(float StartPos, float EndPos, float Presentage_0_to_1);

int findStringInArrayStringReturnIndex(const String &value, String arr[], int size);

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);


