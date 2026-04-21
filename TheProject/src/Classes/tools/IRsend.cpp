#include "IRsend.h"
// #include <ArduinoJson.h>
#include "../../MyOS.h"
#include "../FileBrowser.h"
#include "../../Function.h"
void IRSenderApp::Begin()
{
    ir.begin();

    CommandFromFile.clear();
    CommandFromFile = getAllCommandNamesFromFile(mainOS->FileSelectedInFS);
    DeviceID = mainOS->getFileNameFromPath(mainOS->FileSelectedInFS);
}

void IRSenderApp::Loop()
{
    if (mainOS->NewKey.ifKeyJustPress('`')) // esc
    {

        mainOS->ChangeMenu(new FileBrowser(mainOS));
    }
    if (mainOS->NewKey.ifKeyJustPress(';')) // up
    {
        InCorrentCommand--;
    }
    if (mainOS->NewKey.ifKeyJustPress('.')) // down
    {
        InCorrentCommand++;
    }
    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER)) // down
    {
        loadAndSendFromFile(CommandFromFile[InCorrentCommand], mainOS->FileSelectedInFS);
        mainOS->ShowOnScreenMessege("\"" + CommandFromFile[InCorrentCommand] + "\" Emitted!", 1000);

        //  String msg=DeviceID+":"+CommandFromFile[InCorrentCommand];
    }
    if (InCorrentCommand < 0)
        InCorrentCommand = CommandFromFile.size() - 1;

    if (InCorrentCommand >= CommandFromFile.size())
        InCorrentCommand = 0;
   // CameraY = InCorrentCommand * distancebetweenMenuLines;
CameraY = lerp(CameraY, InCorrentCommand * distancebetweenMenuLines, 0.05f);
    Draw();
}
#define BG_COLOR        0x3000      
#define CARD_COLOR      0x4208     
#define CARD_SELECTED   0xF800    
#define CARD_BORDER     0xFFFF
#define TEXT_NORMAL     0xFFFF
#define TEXT_SELECTED   0xFFFF

void IRSenderApp::Draw()
{
    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillSprite(BG_COLOR);
    mainOS->sprite.setTextWrap(false);
    mainOS->sprite.unloadFont();

    int cardH = 22;
    //int startY = 30 - CameraY;


    mainOS->sprite.setTextSize(1);

    for (int i = 0; i < CommandFromFile.size(); i++)
    {
        int y = i*distancebetweenMenuLines+60-CameraY;// * (cardH + distancebetweenMenuLines);

        bool selected = (i == InCorrentCommand);

        uint16_t bg = selected ? CARD_SELECTED : CARD_COLOR;

        mainOS->sprite.fillRoundRect(14, y + 2, 210, cardH, 6, TFT_BLACK);

        mainOS->sprite.fillRoundRect(12, y, 210, cardH, 6, bg);

        if (selected)
            mainOS->sprite.drawRoundRect(12, y, 210, cardH, 6, CARD_BORDER);

        if (selected)
            mainOS->sprite.fillRect(8, y, 4, cardH, TFT_YELLOW);

        mainOS->sprite.setTextColor(TEXT_SELECTED);
        mainOS->sprite.setCursor(20, y + 6);
        mainOS->sprite.print(CommandFromFile[i]);
    }
    mainOS->sprite.fillRect(0, 0, 240, 24, 0x1800);
    mainOS->sprite.setTextColor(WHITE);
    mainOS->sprite.setTextSize(1);
    mainOS->sprite.setCursor(6, 8);
    mainOS->sprite.print("IR Device: " + DeviceID);
    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}

void IRSenderApp::OnExit()
{
}

bool IRSenderApp::SendCommand(const String &commandName, const String &filePathh)
{
    return loadAndSendFromFile(commandName, filePathh);
}

std::vector<String> IRSenderApp::getAllCommandNamesFromFile(const String &filePath)
{

    std::vector<String> names;

    File f = SD.open(filePath, "r");
    if (!f)
    {
        Serial.println("Failed to open file: " + filePath);
        return names;
    }

    while (f.available())
    {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.startsWith("name: "))
        {
            names.push_back(line.substring(6));
        }
    }

    f.close();
    return names;
}
bool IRSenderApp::loadAndSendFromFile(const String &name, const String &filePath)
{
    File f = SD.open(filePath);
    if (!f)
        return false;

    bool found = false;
    String line;

    while (f.available())
    {
        line = f.readStringUntil('\n');
        line.trim();

        if (line.startsWith("name: "))
        {
            found = (line.substring(6) == name);
        }
        else if (found && line.startsWith("data: "))
        {
            line.remove(0, 6);

            uint16_t raw[300];
            int count = 0;

            char buf[line.length() + 1];
            line.toCharArray(buf, sizeof(buf));

            char *tok = strtok(buf, " ");
            while (tok && count < 300)
            {
                raw[count++] = atoi(tok);
                tok = strtok(nullptr, " ");
            }

            ir.sendRaw(raw, count, 38000);
            f.close();
            return true;
        }
    }

    f.close();
    return false;
}