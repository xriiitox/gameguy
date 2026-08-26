#pragma once
#include <nfd.hpp>
#include <SDL3/SDL.h>
#include <string>

struct GBConfig;

void MakeConfigBar(SDL_Window* win, GBConfig& gbconf, nfdfilteritem_t* filter);

const std::string keys[2] = {
    "dmg", "mgb"
};
