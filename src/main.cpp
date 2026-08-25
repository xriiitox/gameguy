#include "gb.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "config.h"
#include <iostream>
#include <nfd.hpp>
#include <nfd_sdl2.h>

NFD::UniquePath outPath;

using namespace std::literals::string_literals;

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

    SDL_SetWindowAspectRatio(win, 160.0f/144.0f, 160.0f/144.0f);
    SDL_SetWindowMinimumSize(win, 160, 144);
    NFD_SetDisplayPropertiesFromSDLWindow(win);

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
    gbconf.palette = "dmg";

    bool quit = false;
    bool configBar = true;

    GameBoy* gb = nullptr;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer3_Init(ren);

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)) {
                quit = true;
            } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_F1 && e.key.repeat == false) {
                configBar = !configBar;
            } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == false) {
                if (gb != nullptr) gb->handle_input(e, true);
            } else if (e.type == SDL_EVENT_KEY_UP && e.key.repeat == false) {
                if (gb != nullptr) gb->handle_input(e, false);
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        if (configBar) {
            MakeConfigBar(win, gbconf , filterItem);
        }
        ImGui::Render();
        SDL_SetRenderDrawColor(ren, 0x9B, 0xBC, 0x0F, 255);
        SDL_RenderClear(ren);
        // window resizing bs
        int width, height;
        SDL_GetWindowSizeInPixels(win, &width, &height);
        const float base_width = 480.0f;
        float scale_factor = static_cast<float>(width) / base_width;
        if (scale_factor < 0.5f) scale_factor = 0.5f;
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = scale_factor;
        ImGuiStyle& style = ImGui::GetStyle();
        style = ImGuiStyle();
        style.ScaleAllSizes(scale_factor);
        float target_aspect = 160.0f / 144.0f;
        float d_width = static_cast<float>(width);
        float d_height = d_width / target_aspect;
        if (d_height > static_cast<float>(height)) {
            d_height = static_cast<float>(height);
            d_width = d_height * target_aspect;
        }
        SDL_FRect dst_rect;
        dst_rect.w = d_width;
        dst_rect.h = d_height;
        dst_rect.x = (static_cast<float>(width) - d_width) / 2.0f;
        dst_rect.y = (static_cast<float>(height) - d_height) / 2.0f;

        if (gbconf.run) {
            if (gbconf.firstrun) {
                if (gb != nullptr) delete gb;
                gb = new GameBoy(gbconf, ren);
                gbconf.firstrun = false;
            }
            gb->cycle();
            SDL_RenderTexture(ren, gb->texture, nullptr, &dst_rect);
        }
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
