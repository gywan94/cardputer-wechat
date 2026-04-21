#pragma once

#include "./GlobalParentClass.h"
#include <Arduino.h>
#include <vector>

class RECPlayer : public GlobalParentClass
{
public:
    RECPlayer(MyOS *os) : GlobalParentClass(os) {}
    void Begin() override;
    void Loop() override;
    void Draw() override;

private:
    std::vector<String> records;
    int selectedIndex = 0;
    float cameraY = 0.0f;
    float targetCameraY = 0.0f;
    const int lineH = 20;

    void refreshRecordList();
    void playSelected();
};
