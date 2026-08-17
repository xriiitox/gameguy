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
        uint16_t sysclk = 0;

        double cycle_dt = 1.0 / 4194304;
        double now_seconds();
        double t = now_seconds();
        double next_inst = t;

    public:
        GameBoy(GBConfig gbconf);
        void cycle();
        void tick();
        int evilCounter = 0;
};
