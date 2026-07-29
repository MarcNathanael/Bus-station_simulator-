#pragma once
#include "imgui.h"

namespace Theme {
    inline ImFont* FontDefault = nullptr;
    inline ImFont* FontBold = nullptr;
    inline ImFont* FontMono = nullptr;

    // À appeler APRÈS ImGui::SFML::Init
    inline void LoadFonts() {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Medium.ttf", 18.0f);
        FontBold = io.Fonts->AddFontFromFileTTF("assets/fonts/SpaceGrotesk-Bold.ttf", 20.0f);
        FontMono = io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", 16.0f);
        io.FontDefault = FontDefault;
    }

    inline void ApplyStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.GrabRounding = 4.0f;
        style.FrameRounding = 4.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;

        ImVec4* c = ImGui::GetStyle().Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.07f, 0.10f, 1.00f);
        c[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
        c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
        c[ImGuiCol_Border] = ImVec4(0.20f, 0.25f, 0.30f, 0.50f);
        c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.15f, 0.18f, 1.00f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
        c[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.15f, 0.20f, 1.00f);
        c[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.07f, 0.10f, 1.00f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
        c[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.80f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.80f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.90f, 1.00f, 1.00f);
        c[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.40f, 0.50f, 1.00f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        c[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.50f, 1.00f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        c[ImGuiCol_Text] = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    }
}