#include "Calculator.h"
#include "./MyOS.h"
#include <vector>

#include "Classes/ExtraMenu.h"
const char *opButtons[] = {"(", ")", "+", "-", "*", "/", "="};
const int opCount = 7;

int selIndex = 0;
String inputBuffer = "";
String result = "";

void Calculator::Begin()
{
    inputBuffer = "";
    result = "";
}

void Calculator::Loop()
{
    // GlobalParentClass::Loop();
    if (mainOS->NewKey.ifKeyJustPress('`'))
    {
        mainOS->ChangeMenu(new Extra(mainOS));
        //delay(500);
    }
    if (M5Cardputer.Keyboard.isChange())
    {
        auto ks = M5Cardputer.Keyboard.keysState();
        for (auto k : ks.word)
        {
            if (k >= '0' && k <= '9')
                inputBuffer += (char)k;
        }
    }

    if (mainOS->NewKey.ifKeyJustPress(','))
        selIndex = (selIndex - 1 + opCount) % opCount;

    if (mainOS->NewKey.ifKeyJustPress('/'))
        selIndex = (selIndex + 1) % opCount;

    if (mainOS->NewKey.ifKeyJustPress(KEY_ENTER))
    {
        String op = opButtons[selIndex];
        if (op == "=")
        {
            Evaluate();
        }
        else
        {
            inputBuffer += op;
        }
    }

    // BACKSPACE
    if (mainOS->NewKey.ifKeyJustPress(KEY_BACKSPACE))
    {
        if (inputBuffer.length() > 0)
            inputBuffer.remove(inputBuffer.length() - 1);
    }
    if (!mainOS->screenOff)
    {
        Draw();
    }
}
void Calculator::Draw()
{
    mainOS->sprite.createSprite(240, 135 - TopOffset);
    mainOS->sprite.fillScreen(TFT_BLACK);
    mainOS->sprite.unloadFont();
    mainOS->sprite.setTextSize(2);
    mainOS->sprite.setTextColor(TFT_GREEN);
    mainOS->sprite.setCursor(10, 5);
    mainOS->sprite.print(inputBuffer);

    // 
    mainOS->sprite.setTextSize(2);
    mainOS->sprite.setTextColor(TFT_WHITE);
    mainOS->sprite.setCursor(10, 30);
    mainOS->sprite.print(result);

    int startY = 70;
    int btnW = 28;
    int spacing = 5;
    int x = 10;

    for (int i = 0; i < opCount; i++)
    {

        bool sel = (i == selIndex);

        mainOS->sprite.fillRoundRect(
            x, startY, btnW, 28, 4,
            sel ? TFT_BLUE : TFT_DARKGREY);

        mainOS->sprite.setTextColor(TFT_WHITE);
        mainOS->sprite.setTextSize(1);
        mainOS->sprite.setCursor(x + 8, startY + 6);
        mainOS->sprite.print(opButtons[i]);

        x += btnW + spacing;
    }

    mainOS->sprite.pushSprite(0, TopOffset);
    mainOS->sprite.deleteSprite();
}


float eval(const String &expr, bool &ok);

float evalFlat(const String &expr, bool &ok);


void Calculator::Evaluate()
{
    bool ok = true;

    float v = eval(inputBuffer, ok);

    if (!ok || isnan(v))
    {
        result = "ERR";
        inputBuffer = "";
        return;
    }

    if (v == (int)v)
        result = String((int)v);
    else
        result = String(v, 4);

    inputBuffer = ""; 
}


float eval(const String &expr, bool &ok)
{
    String s = "";
    int len = expr.length();

    for (int i = 0; i < len; i++)
    {
        char c = expr[i];

        if (c == '(')
        {
            int depth = 1;
            int j = i + 1;

            while (j < len && depth > 0)
            {
                if (expr[j] == '(')
                    depth++;
                else if (expr[j] == ')')
                    depth--;
                j++;
            }

            if (depth != 0)
            {
                ok = false; 
                return NAN;
            }

            String inside = expr.substring(i + 1, j - 1);
            float insideVal = eval(inside, ok);
            if (!ok)
                return NAN;

            s += String(insideVal);
            i = j - 1;
        }
        else
        {
            s += c;
        }
    }

    return evalFlat(s, ok);
}


float evalFlat(const String &expr, bool &ok)
{
    std::vector<float> nums;
    std::vector<char> ops;

    String cur = "";
    bool negative = false;

    for (int i = 0; i < expr.length(); i++)
    {
        char c = expr[i];

        if (c == '-' && (i == 0 || strchr("+-*/(", expr[i - 1])))
        {
            cur += '-';
        }
        else if ((c >= '0' && c <= '9') || c == '.')
        {
            cur += c;
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/')
        {
            if (cur == "")
            {
                ok = false;
                return NAN;
            }

            nums.push_back(cur.toFloat());
            cur = "";
            ops.push_back(c);
        }
    }

    if (cur != "")
        nums.push_back(cur.toFloat());

    if (nums.size() == 0)
    {
        ok = false;
        return NAN;
    }

    for (int i = 0; i < ops.size(); i++)
    {
        if (ops[i] == '*' || ops[i] == '/')
        {
            float a = nums[i];
            float b = nums[i + 1];

            if (ops[i] == '*')
                nums[i] = a * b;
            else
            {
                if (b == 0)
                {
                    ok = false;
                    return NAN;
                }
                nums[i] = a / b;
            }

            nums.erase(nums.begin() + i + 1);
            ops.erase(ops.begin() + i);
            i--;
        }
    }

    float res = nums[0];
    for (int i = 0; i < ops.size(); i++)
    {
        if (ops[i] == '+')
            res += nums[i + 1];
        else if (ops[i] == '-')
            res -= nums[i + 1];
    }

    return res;
}