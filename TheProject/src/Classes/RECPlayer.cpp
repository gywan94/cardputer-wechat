#include "RECPlayer.h"
#include "./MyOS.h"
#include "ExtraMenu.h"

void RECPlayer::Begin()
{
    refreshRecordList();
}

void RECPlayer::refreshRecordList()
{
    records.clear();
    SD.mkdir("/AdvanceOS");
    SD.mkdir("/AdvanceOS/REC");

    File root = SD.open("/AdvanceOS/REC");
    if (!root || !root.isDirectory())
    {
        if (root)
        {
            root.close();
        }
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            String fullPath = String(file.path());
            String ext = mainOS->GetExtensionLower(fullPath.c_str());
            if (ext == "wav" || ext == "mp3")
            {
                records.push_back(fullPath);
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    if (selectedIndex >= (int)records.size())
    {
        selectedIndex = max(0, (int)records.size() - 1);
    }
}

void RECPlayer::playSelected()
{
    if (records.empty())
    {
        mainOS->ShowOnScreenMessege("No record file found");
        return;
    }

    mainOS->ClearAllSong();
    mainOS->PlayWavFile(records[selectedIndex].c_str());
    mainOS->EnterMusicPlayer(true);
}

void RECPlayer::Loop()
{
    if (mainOS->screenOff)
    {
        return;
    }

    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new Extra(mainOS));
        return;
    }

    if (mainOS->NewKey.ifKeyJustPress(';'))
    {
        selectedIndex--;
        if (selectedIndex < 0)
        {
            selectedIndex = max(0, (int)records.size() - 1);
        }
        targetCameraY = selectedIndex * lineH;
    }

    if (mainOS->NewKey.ifKeyJustPress('.'))
    {
        selectedIndex++;
        if (selectedIndex >= (int)records.size())
        {
            selectedIndex = 0;
        }
        targetCameraY = selectedIndex * lineH;
    }

    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
    {
        playSelected();
        return;
    }

    if (mainOS->NewKey.ifKeyJustPress('r') || mainOS->NewKey.ifKeyJustPress('R'))
    {
        refreshRecordList();
    }

    cameraY = cameraY + (targetCameraY - cameraY) * 0.12f;
    Draw();
}

void RECPlayer::Draw()
{
    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillScreen(BLACK);
    mainOS->sprite.setTextWrap(false);
    mainOS->sprite.unloadFont();
    mainOS->sprite.setTextSize(1);

    mainOS->sprite.fillRect(0, 0, 240, 18, DARKGREY);
    mainOS->sprite.setTextColor(WHITE);
    mainOS->sprite.setCursor(6, 5);
    mainOS->sprite.print("REC PLAYER");

    if (records.empty())
    {
        mainOS->sprite.setTextColor(TFT_ORANGE);
        mainOS->sprite.setCursor(8, 28);
        mainOS->sprite.print("No recording in /AdvanceOS/REC");
        mainOS->sprite.setCursor(8, 44);
        mainOS->sprite.setTextColor(LIGHTGREY);
        mainOS->sprite.print("Use MIC Record to SD Card first.");
    }
    else
    {
        int y = 24 - (int)cameraY;
        for (int i = 0; i < (int)records.size(); i++)
        {
            bool selected = (i == selectedIndex);
            if (selected)
            {
                mainOS->sprite.fillRoundRect(2, y - 2, 236, 16, 3, TFT_DARKGREEN);
                mainOS->sprite.setTextColor(WHITE);
                mainOS->sprite.setCursor(6, y);
                mainOS->sprite.print("> ");
            }
            else
            {
                mainOS->sprite.setTextColor(YELLOW);
                mainOS->sprite.setCursor(14, y);
            }

            mainOS->sprite.print(mainOS->getFileNameFromPath(records[i]));
            y += lineH;
        }
    }

    mainOS->sprite.setTextColor(LIGHTGREY);
    mainOS->sprite.setCursor(6, mainOS->sprite.height() - 10);
    mainOS->sprite.print(";/. move  Enter play  R refresh  ` back");

    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}
