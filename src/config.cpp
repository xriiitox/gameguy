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
        ImGui::EndMainMenuBar();
    }
    if (gbconf.filename != "") {
        gbconf.run = true;
    }
}
