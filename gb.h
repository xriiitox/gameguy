#include <string>
#include <cstdint>
#include <iostream>
#include <map>
#include <functional>

#pragma once

struct GBConfig {
    bool run;
    std::string filename;
    // gbc mode here?
};

class Registers {
public:
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

    bool getFlag(char fl) {
        switch (fl) {
            case 'z':
                return *f & 0x80;
            case 'n':
                return *f & 0x40;
            case 'h':
                return *f & 0x20;
            case 'c':
                return *f & 0x10;
            default:
                std::cout << "invalid flag" << std::endl;
                return false;
        }
    }
    void setFlag(char fl, bool set) {
        if (set) {
            switch (fl) {
                case 'z':
                    *f |= (1 << 7);
                    break;
                case 'n':
                    *f |= (1 << 6);
                    break;
                case 'h':
                    *f |= (1 << 5);
                    break;
                case 'c':
                    *f |= (1 << 4);
                    break;
                default:
                    std::cout << "something is wrong" << std::endl;
            }
        } else {
            switch (fl) {
                case 'z':
                    *f &= ~(1 << 7);
                    break;
                case 'n':
                    *f &= ~(1 << 6);
                    break;
                case 'h':
                    *f &= ~(1 << 5);
                    break;
                case 'c':
                    *f &= ~(1 << 4);
                    break;
                default:
                    std::cout << "something is wrong" << std::endl;
            }
        }
    }

    std::map<int, std::function<bool()>> CC = {
        { 0 , [this]() { return !this->getFlag('z'); } },
        { 1 , [this]() { return this->getFlag('z'); } },
        { 2 , [this]() { return !this->getFlag('c'); } },
        { 3 , [this]() { return this->getFlag('c'); } },
    };
};

class GameBoy {
    private:
        // full address space
        uint8_t bus[0x10000];
        // registers
        Registers reg;
        uint16_t sp = 0xFFFE; // stack pointer
        uint16_t pc = 0x100; // program counter

        bool ime = false;

        int t_cycle = 0;

        void opcode(uint8_t inst);

        // instuction functions
        void jr_cc_e8(std::function<bool()> cc);
        void add_hl_rpp(int p);
        void ld_rpp_nn(int p);
        void inc16(int p);
        void dec16(int p);
        void inc8(int y);
        void dec8(int y);
        void ret_cc(std::function<bool()> cc);
        void add_sp_e();
        void ld_hl_sp_e();
        void jp_cc_n16(std::function<bool()> cc);

        // 8 bit register table
        std::map<int, std::function<uint8_t*()>> r = {
            { 0, [this]{ return this->reg.b; } },
            { 1, [this]{ return this->reg.c; } },
            { 2, [this]{ return this->reg.d; } },
            { 3, [this]{ return this->reg.e; } },
            { 4, [this]{ return this->reg.h; } },
            { 5, [this]{ return this->reg.l; } },
            { 6, [this]{ return &this->bus[this->reg.hl]; } }, // pointer to memory address pointed to by HL
            { 7, [this]{ return this->reg.a; } }
        };

        // register pairs featuring sp
        std::map<int, std::function<uint16_t*()>> rp = {
            { 0, [this]{ return &this->reg.bc; }},
            { 1, [this]{ return &this->reg.de; }},
            { 2, [this]{ return &this->reg.hl; }},
            { 3, [this]{ return &this->sp; }}
        };

        // register pairs featuring af
        std::map<int, std::function<uint16_t*()>> rp2 = {
            { 0, [this]{ return &this->reg.bc; }},
            { 1, [this]{ return &this->reg.de; }},
            { 2, [this]{ return &this->reg.hl; }},
            { 3, [this]{ return &this->reg.af; }}
        };

        // misc accumulator/flag ops
        void rlca();
        void rrca();
        void rla();
        void rra();
        void daa();
        void cpl();
        void scf();
        void ccf();
        std::map<int, std::function<void()>> flag_ops = {
            { 0, [this]{ this->rlca(); } },
            { 1, [this]{ this->rrca(); } },
            { 2, [this]{ this->rla(); } },
            { 3, [this]{ this->rra(); } },
            { 4, [this]{ this->daa(); } },
            { 5, [this]{ this->cpl(); } },
            { 6, [this]{ this->scf(); } },
            { 7, [this]{ this->ccf(); } }
        };

        void add_a(int z);
        void adc_a(int z);
        void sub_a(int z);
        void sbc_a(int z);
        void and_a(int z);
        void xor_a(int z);
        void or_a(int z);
        void cp_a(int z);
        std::map<int, std::function<void(int)>> alu_r = {
            { 0, [this](int z){ this->add_a(z); } },
            { 1, [this](int z){ this->adc_a(z); } },
            { 2, [this](int z){ this->sub_a(z); } },
            { 3, [this](int z){ this->sbc_a(z); } },
            { 4, [this](int z){ this->and_a(z); } },
            { 5, [this](int z){ this->xor_a(z); } },
            { 6, [this](int z){ this->or_a(z); } },
            { 7, [this](int z){ this->cp_a(z); } }
        };

    public:
        GameBoy(GBConfig gbconf);
        void cycle();
};
