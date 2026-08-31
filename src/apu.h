#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

class APU {
    SDL_AudioStream* sq1;
    SDL_AudioStream* sq2;
    SDL_AudioStream* wav;
    SDL_AudioStream* noise;
public:
    APU();
};
