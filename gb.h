#include <string>
#include <cstdint>

#pragma once

using namespace std;

struct GBConfig {
    bool run;
    string filename;
    // gbc mode here?
};

class GameBoy {
    private:
        // full address space
        uint8_t bus[65536];
        // registers
        uint16_t af = 0x01B0; // accumulator and flags
        uint8_t* a = (uint8_t*)&af + 1; // pointer to upper half of AF
        uint16_t bc = 0x0013;
        uint8_t* b = (uint8_t*)&bc + 1;
        uint8_t* c = (uint8_t*)&bc;
        uint16_t de = 0x00D8;
        uint8_t* d = (uint8_t*)&de + 1;
        uint8_t* e = (uint8_t*)&de;
        uint16_t hl = 0x014D;
        uint8_t* h = (uint8_t*)&hl + 1;
        uint8_t* l = (uint8_t*)&hl;
        uint16_t sp = 0xFFFE; // stack pointer
        uint16_t pc = 0x100; // program counter

        void instruction(uint8_t inst);
    public:
        GameBoy(GBConfig gbconf);
        void cycle();
};
