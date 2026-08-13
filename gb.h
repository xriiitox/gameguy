#include <string>
#include <cstdint>

#pragma once

using namespace std;

struct GBConfig {
    bool run;
    string filename;
    // gbc mode here?
};

struct Registers {
    uint16_t af = 0x0180;
    uint16_t bc = 0x0013;
    uint16_t de = 0x00D8;
    uint16_t hl = 0x014D;

    uint8_t* a = (uint8_t*)&af + 1;
    uint8_t* f = (uint8_t*)&af;
    uint8_t* b = (uint8_t*)&bc + 1;
    uint8_t* c = (uint8_t*)&bc;
    uint8_t* d = (uint8_t*)&de + 1;
    uint8_t* e = (uint8_t*)&de;
    uint8_t* h = (uint8_t*)&hl + 1;
    uint8_t* l = (uint8_t*)&hl;
};

class GameBoy {
    private:
        // full address space
        uint8_t bus[0xFFFF];
        // registers
        Registers reg;
        uint16_t sp = 0xFFFE; // stack pointer
        uint16_t pc = 0x100; // program counter

        int t_cycle = 0;

        void opcode(uint8_t inst);
        void flag(char fl, bool set);
    public:
        GameBoy(GBConfig gbconf);
        void cycle();
};
