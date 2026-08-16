#include "gb.h"
#include "nfd.hpp"
#include <vector>
#include <string>
#include <SDL3/SDL.h>

void MakeConfigBar(SDL_Window* win, GBConfig& gbconf, std::vector<std::string> roms, nfdfilteritem_t filter);
