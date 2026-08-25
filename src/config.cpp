#include "gb.h"
#include "imgui.h"
#include "config.h"
#include <nfd.hpp>
#include <nfd_sdl2.h>
#include <SDL3/SDL.h>
#include <string>

extern NFD::UniquePath outPath;

void MakeConfigBar(SDL_Window* win, GBConfig& gbconf, nfdfilteritem_t filter) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open ROM")) {
                gbconf.run = false;
                gbconf.filename = "";
                gbconf.firstrun = true;
                nfdwindowhandle_t window;
                NFD_GetNativeWindowFromSDLWindow(win, &window);
                nfdresult_t result = NFD::OpenDialog(outPath, &filter, 1, nullptr, window);
                if (result == NFD_OKAY) {
                    gbconf.filename = outPath.get();
                    std::cout << gbconf.filename << std::endl;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::BeginCombo("Palette", gbconf.palette.c_str())) {
                for (int n = 0; n < IM_ARRAYSIZE(keys); n++)
                {
                    bool is_selected = (gbconf.palette == keys[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(keys[n].c_str(), is_selected))
                        gbconf.palette = keys[n];
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (gbconf.filename != "") {
        gbconf.run = true;
    }
}
