#pragma once

#include <cstdint>

// interface class, do not instantiate
class Mapper {
public:
    virtual ~Mapper() {};

    virtual uint8_t read(uint16_t addr) = 0;

    virtual void write(uint16_t addr, uint8_t value) = 0;
    // rom size in bytes
    int rom_size;
    // Bank count including b0
    int bank_count;

    // Cart has ram banks?
    bool ram;
    // if so, how many
    uint8_t ram_bank_count = 0;

    bool rtc;
    // is eRAM battery backed?
    bool battery;
};
