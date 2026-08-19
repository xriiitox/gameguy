#include <string>
#include <cstdint>
#include <memory>
#include <SDL3/SDL.h>
#include "cpu.h"
#include "bus.h"
#include "ppu.h"

#pragma once

struct GBConfig {
    bool run;
    bool firstrun;
    std::string filename;
    // gbc mode here?
};

class GameBoy {
    private:
        std::shared_ptr<Bus> bus;

        std::shared_ptr<CPU> cpu;

        std::shared_ptr<PPU> ppu;

        double cycle_dt = 1.0 / 4194304;
        double now_seconds();
        double t = now_seconds();
        double next_inst = t;

        void handle_interrupts();
        void tick_dma();
        static int get_interrupt_mask(int IF, int IE);

        int cycles_frame = 0;

    public:
        GameBoy(GBConfig gbconf, SDL_Renderer* ren);
        void cycle();
        void tick();
        bool tima_reload_pending = false;
        uint8_t curr_inst;
        uint16_t sysclk = 0;
        int timer_count = 0;
        int evilCounter = 0;

        SDL_Texture* texture;

        struct DMA {
            bool active = false;
            bool bus_locked = false;
            uint16_t source = 0;
            uint8_t index = 0;
            int start_delay = 0;
        } dma;
};
