#include "Notes.h"
#include <M5Cardputer.h>
#include "./MyOS.h"
#include "./Function.h"
#include "TextEditor.h"
#include "Fonts/DryBrush15.h"
#include "MainMenu.h"

void Notes::Begin()
{
    SD.mkdir("/AdvanceOS");
    SD.mkdir("/AdvanceOS/Notes");
    SetNotesVariable();
}

void Notes::Loop()
{
    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new MainMenu(mainOS));
        return;
    }

    if (mainOS->NewKey.ifKeyJustPress('.'))
    {
        mainOS->PlayCuteEvilTone();

        InNoteIDselect++;
        if (InNoteIDselect >= NodesInSD.size())
        {
            InNoteIDselect = 0;
        }
    }
    if (mainOS->NewKey.ifKeyJustPress(';'))
    {
        mainOS->PlayCuteEvilTone();

        InNoteIDselect--;
        if (InNoteIDselect < 0)
        {
            InNoteIDselect = NodesInSD.size() - 1;
        }
    }
    if (mainOS->NewKey.ifKeyJustPress('1')) // create new note
    {
        mainOS->PlayCuteEvilTone();

        delay(500); // delay for the nomber 1 not be presed inside the text editor
        String fileName = "Note";
        fileName += String(NodesInSD.size());
        fileName += ".txt";
        //   += String(NodesInSD.size()) += ".txt";
        String CreatedFilePath;
        CreatedFilePath = mainOS->createFile(SD, "/AdvanceOS/Notes", fileName);

        mainOS->TextEditorGetTextFromFile = true;

        mainOS->FileSelectedInFS = CreatedFilePath;
        mainOS->ChangeMenu(new TextEditor(mainOS));
        // mainOS->currentApp

        TextEditor *child = static_cast<TextEditor *>(mainOS->currentApp);
        child->EditMode = true;
        child->CameFromNotes = true;
    }
    if (mainOS->NewKey.ifKeyJustPress(KEY_BACKSPACE)) // delete current note
    {
        mainOS->PlayCuteEvilTone();
        if (mainOS->AskSomthing("Are you sure you want to delete this note?"))
        {
            mainOS->deleteFromSd(SD, NodesInSD[InNoteIDselect].NotePath);
            SetNotesVariable();
        }
    }
    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER)) // edit current note

    {
        mainOS->PlayCuteEvilTone();

        delay(500); // delay for the ENTER KEY not be presed inside the text editor

        mainOS->TextEditorGetTextFromFile = true;
        mainOS->FileSelectedInFS = NodesInSD[InNoteIDselect].NotePath;
        mainOS->ChangeMenu(new TextEditor(mainOS));
        // mainOS->currentApp

        TextEditor *child = static_cast<TextEditor *>(mainOS->currentApp);
        child->EditMode = true;
        child->CameFromNotes = true;
    }
    cameraY = lerp(cameraY, InNoteIDselect * destanceBetweenNotes, 0.15);

    Draw();
}

void Notes::Draw()
{

    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillScreen(rgb565(0, 194, 218));
    mainOS->sprite.setTextWrap(false);

    int CurrentPos = 0;
    int downOffset = 10;

    uint16_t colorNormal = rgb565(255, 220, 0);
    uint16_t colorSelected = rgb565(255, 220, 0);
    uint16_t borderColor = BLACK;
    NoteSprite.setTextColor(BLACK);

    for (int i = 0; i < NodesInSD.size(); i++)
    {
        int w = 140;

        NoteSprite.createSprite(w, NoteHight);
        // NoteSprite.loadFont(DryBrush15);
        NoteSprite.fillSprite(RED); // for transperent
        NoteSprite.fillRoundRect(0, 0, w, NoteHight, 4, rgb565(60, 60, 100));

        uint16_t bg = (InNoteIDselect == i ? colorSelected : colorNormal);
        NoteSprite.fillRoundRect(1, 1, w - 2, NoteHight - 2, 6, bg);

        for (size_t i = 0; i < 8; i++)
        {
            NoteSprite.drawRect(3, i * 8 + 11, 130, 1, ORANGE); // lines
        }
        NoteSprite.drawRoundRect(0, 0, w, NoteHight, 6, borderColor);

        NoteSprite.fillCircle(w - 3, 3, 5, MAROON); // pin circle

        NoteSprite.setCursor(6, 3);
        NoteSprite.print(NodesInSD[i].TextFileContain);

        NoteSprite.pushSprite(&mainOS->sprite, 8, CurrentPos - cameraY + downOffset, RED);
        NoteSprite.deleteSprite();

        destanceBetweenNotes = NoteHight + 4;
        CurrentPos += destanceBetweenNotes;
    }

    NoteSprite.createSprite(74, 50);
    NoteSprite.fillRoundRect(0, 0, 74, 50, 2, YELLOW);
    NoteSprite.drawRoundRect(0, 0, 74, 50, 2, BLACK);

    // NoteSprite.setTextColor(WHITE);
    NoteSprite.unloadFont();
    NoteSprite.setCursor(1, 4);
    NoteSprite.setTextSize(1);
    NoteSprite.print("1: New Note\n\n ENTER: Edit\n\n DEL: Delete");

    NoteSprite.pushSprite(&mainOS->sprite, 164, 5);
    NoteSprite.deleteSprite();

    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}

void Notes::SetNotesVariable()
{
    NodesInSD.clear();
    mainOS->GetFilesToList("txt", "/AdvanceOS/Notes");
    for (int i = 0; i < mainOS->FileList.size(); i++)
    {
        NoteData n;
        n.NotePath = mainOS->FileList[i];
        n.TextFileContain = mainOS->GetFileText(mainOS->FileList[i]);
        NodesInSD.push_back(n);

        // mainOS->FileList[i]
    }
}
