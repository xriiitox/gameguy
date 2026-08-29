#pragma once
#include "mapper.h"
#include <string>
#include <system_error>
#include <vector>
#include "../mio.hpp"

class MBC1 : public Mapper {
public:
    MBC1(std::string filename, bool ram, bool battery);
    ~MBC1();
    virtual uint8_t read(uint16_t addr);
    virtual void write(uint16_t addr, uint8_t val);
    virtual void tick_rtc() {};
private:
    // cart regs
    uint8_t ram_enable = 0; // addrs 0x0000-0x1FFF
    uint8_t rom_select = 0; // addrs 0x2000-0x3FFF
    uint8_t two_bit = 0; // addrs 0x4000-0x5FFF
    uint8_t mode_select = 0; // addrs 0x6000-0x7FFF

    bool detect_multicart(const std::vector<std::byte>& rom);
    bool mbc1mm = false;

    mio::mmap_sink mmap;
    std::error_code error;

    uint8_t rom_helper(size_t bank, uint16_t offset);
    std::vector<uint8_t> rom_banks;

    std::vector<uint8_t> ram_banks;
};
