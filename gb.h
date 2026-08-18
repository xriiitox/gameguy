#include <string>
#include <cstdint>
#include <memory>
#include "cpu.h"
#include "bus.h"

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

        double cycle_dt = 1.0 / 4194304;
        double now_seconds();
        double t = now_seconds();
        double next_inst = t;



        void handle_interrupts();

    public:
        GameBoy(GBConfig gbconf);
        void cycle();
        void tick();
        bool tima_reload_pending = false;
        uint8_t curr_inst;
        uint16_t sysclk = 0;
        int timer_count = 0;
        int evilCounter = 0;
};
