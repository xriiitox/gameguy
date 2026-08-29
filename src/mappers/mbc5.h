#pragma once
#include "mapper.h"
#include <string>
#include <system_error>
#include <vector>
#include "../mio.hpp"

class MBC5 : public Mapper {
public:
    MBC5(std::string filename, bool ram, bool battery);
    ~MBC5();
    virtual uint8_t read(uint16_t addr);
    virtual void write(uint16_t addr, uint8_t val);
    virtual void tick_rtc() {};
private:
    // cart regs
    uint8_t ram_enable = 0; // addrs 0x0000-0x1FFF
    uint8_t rom_select = 1; // addrs 0x2000-0x2FFF
    uint8_t bit9 = 0; // addrs 0x3000-0x3FFF
    uint8_t ram_select = 0; // addrs 0x4000-0x5FFF

    mio::mmap_sink mmap;
    std::error_code error;

    uint8_t rom_helper(size_t bank, uint16_t offset);
    std::vector<uint8_t> rom_banks;

    std::vector<uint8_t> ram_banks;
};
