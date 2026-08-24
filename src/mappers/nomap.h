#pragma once
#include "mapper.h"
#include <string>

class NoMapper : public Mapper {
public:
    NoMapper(std::string filename);

    virtual uint8_t read(uint16_t addr);

    virtual void write(uint16_t addr, uint8_t val);

private:
    uint8_t bank0[0x4000];
    uint8_t bank1[0x4000];
    uint8_t eram[0x2000]; // optional 8kb cartridge ram
};
