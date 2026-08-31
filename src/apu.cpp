#include "apu.h"
#include <SDL3/SDL_audio.h>

APU::APU() {
    SDL_AudioSpec src_spec;
    src_spec.format = SDL_AUDIO_S8;
    src_spec.channels = 2;
    src_spec.freq = 48000;
    SDL_AudioSpec dst_spec;
    dst_spec.format = SDL_AUDIO_S16;
    dst_spec.channels = 2;
    src_spec.freq = 48000;
    sq1 = SDL_CreateAudioStream(&src_spec, &dst_spec);
    sq2 = SDL_CreateAudioStream(&src_spec, &dst_spec);
    wav = SDL_CreateAudioStream(&src_spec, &dst_spec);
    noise = SDL_CreateAudioStream(&src_spec, &dst_spec);
}
