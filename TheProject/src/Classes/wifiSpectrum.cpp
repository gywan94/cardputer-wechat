#include "wifiSpectrum.h"
#include "MyOS.h"
#include "ExtraMenu.h"



void wifiSpectrum::Begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    for (int i = 0; i < 15; i++)
    {
        channelHeights[i] = 0;
        memset(ssidNames[i], 0, 33);
    }
   // mainOS->ShowOnScreenMessege("Scanning", 10);

   performScan();

}

void wifiSpectrum::performScan()
{
    WiFi.scanNetworks(true);
   // mainOS->ShowOnScreenMessege("Scanning...", 1);
}

void wifiSpectrum::Loop()
{
    int n = WiFi.scanComplete();

    if (n >= 0)
    {
        for (int i = 0; i < 15; i++)
            channelHeights[i] = 0;

        for (int i = 0; i < n; ++i)
        {
            int chan = WiFi.channel(i);
            int rssi = WiFi.RSSI(i);
            int val = map(rssi, -100, -30, 0, 80);
            val = constrain(val, 0, 80);

            if (chan < 15 && val > channelHeights[chan])
            {
                channelHeights[chan] = val;
                String name = WiFi.SSID(i);
                name.substring(0, 10).toCharArray(ssidNames[chan], 10); 
            }
        }
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
    }

    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new Extra(mainOS));
    }
          Draw();

}
void wifiSpectrum::Draw()
{
    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillSprite(TFT_DARKGREY);

    mainOS->sprite.setTextColor(WHITE);
    mainOS->sprite.setTextSize(1);
    mainOS->sprite.drawString("2.4 WiFi Spectrum Visualizer", 10, 5);

    uint16_t channelColors[] = {
        0, 
        RED, BLUE, GREEN, YELLOW, MAGENTA, CYAN,
        ORANGE, WHITE, PURPLE, DARKGREEN, MAROON, NAVY, PINK};

    int baselineY = 105;
    mainOS->sprite.drawLine(10, baselineY, 230, baselineY, BLACK);

    for (int i = 1; i <= 13; i++)
    {
        int x = 20 + (i * 15);
        int h = channelHeights[i];

        if (h > 0)
        {
            uint16_t color = channelColors[i]; 

            mainOS->sprite.fillTriangle(
                x - 20, baselineY, 
                x + 20, baselineY,
                x, baselineY - h,  // 
                color);

            mainOS->sprite.drawTriangle(x - 20, baselineY, x + 20, baselineY, x, baselineY - h, BLACK);

            if (ssidNames[i][0] != '\0')
            { 
                mainOS->sprite.setTextColor(WHITE);
                int textY = baselineY - h - 15;
                if (textY < 15)
                    textY = 15; 

                mainOS->sprite.setCursor(x - 10, textY);
                mainOS->sprite.print(ssidNames[i]);
            }
        }

        mainOS->sprite.setTextColor(WHITE);
        mainOS->sprite.setCursor(x - 4, baselineY + 5);
        mainOS->sprite.printf("%d", i);
    }

    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}

void wifiSpectrum::OnExit()
{
            WiFi.mode(WIFI_OFF);
                mainOS->WifiConnected=false;

}