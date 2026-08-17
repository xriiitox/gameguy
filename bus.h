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
    uint8_t io[0x7F];
    uint8_t hram[0xFFFE - 0xFF80+1]; // high ram
    uint8_t ie = 0;

    void write(uint16_t addr, uint8_t val);
    uint8_t* read(uint16_t addr);

    void* gb;

    Bus(void* gb);

};
