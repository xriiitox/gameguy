#include <cstdint>
#include <array>

#pragma once

// internal class, yay!
class Bus {
public:
    uint8_t bank0[0x4000];
    std::array<std::array<uint8_t, 0x4000>, 129> bankx; // switchable via mapper
    uint8_t vram[0x2000]; // switchable if cgb
    uint8_t eram[0x2000]; // from game cart, switchable
    uint8_t wram1[0x1000];
    uint8_t wram2[0x1000]; // switchable if cgb
    uint8_t echoram[0xFDFF - 0xE000+1]; // mirror of $C000-$DDFF
    uint8_t oam[0xFE9F - 0xFE00+1]; // object attribute memory
    uint8_t joyp = 0xCF;
    uint8_t* div;
    uint8_t IF = 0xE1;
    uint8_t lcdc = 0x91;
    uint8_t stat = 0x80;
    uint8_t scy = 0;
    uint8_t scx = 0;
    uint8_t ly = 0;
    uint8_t lyc = 0;
    uint8_t dma = 0xFF;
    uint8_t bgp = 0xFC;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t wy = 0;
    uint8_t wx = 0;
    uint8_t hram[0xFFFE - 0xFF80+1]; // high ram
    uint8_t ie = 0;

    // timer state machine
    struct Timer {
        uint8_t tima = 0;
        uint8_t tma = 0;
        uint8_t tac = 0xF8;
        int reload_delay = 0;
        bool tima_just_reloaded;
    } timers;

    // audio regs
    uint8_t nr10 = 0x80;
    uint8_t nr11 = 0xBF;
    uint8_t nr12 = 0xF3;
    uint8_t nr13 = 0xFF;
    uint8_t nr14 = 0xBF;
    uint8_t nr21 = 0x3F;
    uint8_t nr22 = 0;
    uint8_t nr23 = 0xFF;
    uint8_t nr24 = 0xBF;
    uint8_t nr30 = 0x7F;
    uint8_t nr31 = 0xFF;
    uint8_t nr32 = 0x9F;
    uint8_t nr33 = 0xFF;
    uint8_t nr34 = 0xBF;
    uint8_t nr41 = 0xFF;
    uint8_t nr42 = 0;
    uint8_t nr43 = 0;
    uint8_t nr44 = 0xBF;
    uint8_t nr50 = 0x77;
    uint8_t nr51 = 0xF3;
    uint8_t nr52 = 0xF1;
    uint8_t wav_ram[16];

    // better serial implementation
    struct SerialPort {
        uint8_t sb = 0x00;
        uint8_t sc = 0x7E;

        bool transfer_active = false;
        bool int_clock = false;

        int bit_count = 0;
    } serial_port;

    // MBC registers
    int banks = 0;
    uint8_t ram_en = 0;
    int sel_bank = 1;

    uint8_t openBus = 0xFF;

    void write(uint16_t addr, uint8_t val, bool tk = true);
    uint8_t read(uint16_t addr, bool tk = true, bool bypass = false);
    void tick_serial();
    bool get_timer_bit();
    bool get_timer_bit_at(uint16_t clk, uint8_t tac);

    void* gb;

    Bus(void* gb);
private:
    void write_helper(uint16_t addr, uint8_t val);
    void write_div();
    void write_tima(uint8_t val);
    void write_tma(uint8_t val);
    void write_tac(uint8_t val);
    void write_dma(uint8_t val);
    void write_sc(uint8_t val);
    uint8_t read_vram(uint16_t addr);
    void write_vram(uint16_t addr, uint8_t val);
};
