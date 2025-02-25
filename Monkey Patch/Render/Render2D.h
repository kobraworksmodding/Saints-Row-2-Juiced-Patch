#pragma once
namespace Render2D
{
    typedef float(__cdecl* ChangeTextColorT)(int R, int G, int B, int Alpha);
    extern ChangeTextColorT ChangeTextColor;
    extern int processtextwidth(int width);

    extern void Init();
    extern void InitMenVerNum();
    extern void InGamePrint(const char* Text, int x, int y, int font);

    extern bool BetterChatTest;
}