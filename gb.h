#include <string>
#include <cstdint>
#include <memory>
#include "cpu.h"

#pragma once

struct GBConfig {
    bool run;
    std::string filename;
    // gbc mode here?
};

class GameBoy {
    private:
        // full address space
        uint8_t bus[0x10000];

        std::unique_ptr<CPU> cpu;

    public:
        GameBoy(GBConfig gbconf);
        void cycle();
};
