
#pragma once

#ifndef IMGUI_LOG_H
#define IMGUI_LOG_H

#include <imgui.h>

// 源自imgui_demo.cpp

class ImGuiLog
{
public:

    ImGuiLog();
    static ImGuiLog& Get();
    static bool HasInstance();

    void Clear();
    void AddLog(const char* fmt, ...);

    void Draw(const char* title, bool* p_open = nullptr);

private:
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.
};

#endif