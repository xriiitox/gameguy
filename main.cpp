#include "gb.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <vector>
#include <string>
#include <filesystem>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "config.h"
#include <iostream>
#include <memory>
#include <nfd.hpp>
#include <nfd_sdl2.h>

NFD::UniquePath outPath;

int main (int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    NFD::Guard nfdGuard;

    nfdfilteritem_t filterItem = {"Game Boy ROM", "gb"};

    SDL_Window* win = SDL_CreateWindow("game guy", 480, 432, 0);
    if (win == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    NFD_SetDisplayPropertiesFromSDLWindow(win);

    SDL_SetWindowResizable(win, false);

    SDL_Renderer* ren = SDL_CreateRenderer(win, nullptr);
    if (ren == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_Event e;

    GBConfig gbconf;
    gbconf.filename = "";
    gbconf.run = false;
    gbconf.firstrun = true;

    bool quit = false;
    bool configBar = true;

    GameBoy* gb = nullptr;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer3_Init(ren);

    std::vector<std::string> roms;
    for (const auto& entry : std::filesystem::directory_iterator(".")) {
        if (entry.path().extension() == ".gb") roms.push_back(entry.path());
    }

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)) {
                quit = true;
            } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_F1 && e.key.repeat == false) {
                configBar = !configBar;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        if (configBar) {
            MakeConfigBar(win, gbconf, roms, filterItem);
        }
        SDL_RenderClear(ren);
        if (gbconf.run) {
            if (gbconf.firstrun) {
                if (gb != nullptr) delete gb;
                gb = new GameBoy(gbconf, ren);
                gbconf.firstrun = false;
            }
            gb->cycle();
            SDL_RenderTexture(ren, gb->texture, nullptr, nullptr);
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren);
        SDL_RenderPresent(ren);
    }

    delete gb;
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
