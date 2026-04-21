#pragma once

#include "./GlobalParentClass.h"
#include <Arduino.h>
#include <M5Cardputer.h>

#include <SD.h>
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <elapsedMillis.h>
#include <AudioOutputI2S.h> 

class MusicPlayerV2 : public GlobalParentClass
{
public:
    enum class FileType
    {
        UNKNOWN,
        MP3,
        WAV,

    };
    FileType CurrentFileType;
    MusicPlayerV2(MyOS *os) : GlobalParentClass(os) {}

    void Begin() override;
    void Loop() override;
    void Draw() override;
            void OnExit() override;

    // std::vector<String> playlist;
    bool PlayWAV = false;
    bool Focused = true;
    bool first_frame = true;
    bool isPlaying = false;
    // String CurrentFileType;
    // elapsedSeconds refrasFirstFrame;
    bool pause = false;
static void loop2(void *pvParameters);
   // TaskHandle_t taskCore2Handle = nullptr;

private:
    void stop();
    void play(const char *fname);

    AudioFileSourceSD AudioFile;
    AudioFileSourceID3 *id3 = nullptr;
    AudioGeneratorMP3 MP3_GENERATOR;
    AudioGeneratorWAV WAV_GENERATOR;

    // AudioGeneratorMIDI WAV_GENERATOR;

    size_t fileIndex = 0;
    void gfxSetup(LGFX_Device *gfx);


   // AudioOutput *output = nullptr;
//AudioOutputI2S *outRes;

   // AudioOutputI2S *outRes;
    int InMusicPlayLoop = 0;

    void WheneSongLoading();
    void WheneSongFinishLoadAndPlay();
    elapsedMillis CheackTimer;
    uint32_t lastCheackPos;
    int TotalSongSeconds = 0;
std::vector<int>SecondArray;
int SecondAverage;
void SetSongTimeByAverage();
    int seconds;
    int minutes;



    /*
                float percent = (AudioFile.getPos() * 100.0f) / AudioFile.getSize();

                mainOS->ShowOnScreenMessege(
                    "in pos: " + String(AudioFile.getPos()) +
                        " size: " + String(AudioFile.getSize()) +
                        " " + String(percent, 1) + "%",
                    2000); */
    // get file pos
};
