#include <cstdint>

#pragma once

// internal class, yay!
class Bus {
public:
    uint8_t bank0[0x4000];
    uint8_t bank1[0x4000]; // switchable via mapper
    uint8_t vram[0x2000]; // switchable if cgb
    uint8_t eram[0x2000]; // from game cart, switchable
    uint8_t wram1[0x1000];
    uint8_t wram2[0x1000]; // switchable if cgb
    uint8_t echoram[0xFDFF - 0xE000+1]; // mirror of $C000-$DDFF
    uint8_t oam[0xFE9F - 0xFE00+1]; // object attribute memory
    uint8_t joyp = 0xCF;
    uint8_t sb = 0;
    uint8_t sc = 0x7E;
    uint8_t* div;
    uint8_t tima = 0;
    uint8_t tma = 0;
    uint8_t tac = 0xF8;
    uint8_t IF = 0xE1;
    uint8_t lcdc = 0x91;
    uint8_t stat = 0x85;
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

    uint8_t openBus = 0xFF;

    void write(uint16_t addr, uint8_t val, bool tk = true);
    uint8_t* read(uint16_t addr, bool tk = true, bool bypass = false);

    void* gb;

    Bus(void* gb);
private:
    void write_timer(uint8_t val, bool div);
    void write_tima(uint8_t val);
    void write_tma(uint8_t val);
    void write_dma(uint8_t val);

};
