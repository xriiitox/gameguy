#include <string>
#include <cstdint>
#include <SDL3/SDL.h>
#include "cpu.h"
#include "bus.h"
#include "ppu.h"
#include "mappers/mapper.h"

#pragma once

struct GBConfig {
    bool run;
    bool firstrun;
    std::string filename;
    // gbc mode here?
};

class GameBoy {
    private:
        Bus* bus;

        CPU* cpu;

        PPU* ppu;
        void init_mapper(std::string filename);

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
        ~GameBoy();
        void cycle();
        void tick();
        uint8_t d_pad_state = 0x0F;
        uint8_t buttons_state = 0x0F;
        void handle_input(SDL_Event e, bool set);
        uint8_t curr_inst;
        uint16_t sysclk = 0xABC8;
        int timer_count = 0;
        int evilCounter = 0;

        uint8_t* ppuMode;
        void ppu_stat_line();

        SDL_Texture* texture;
        Mapper* mapper;

        void dma_byte_copy();
        struct DMA {
            bool active = false;
            bool bus_locked = false;
            uint16_t source = 0;
            uint16_t pending_source = 0;
            uint8_t index = 0;
            int start_delay = 0;
            bool pending_restart = false;
        } dma;
};
