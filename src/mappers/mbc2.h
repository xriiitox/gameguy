#pragma once
#include "mapper.h"
#include <string>
#include <system_error>
#include <vector>
#include <array>
#include "../mio.hpp"

class MBC2 : public Mapper {
public:
    MBC2(std::string filename, bool battery);
    ~MBC2();
    virtual uint8_t read(uint16_t addr);
    virtual void write(uint16_t addr, uint8_t val);
    virtual void tick_rtc() {};
private:
    // cart reg: rom/ram select/enable
    uint8_t rom_sel = 1;
    uint8_t ram_en = 0;
    uint8_t rom_helper(size_t bank, uint16_t offset);

    mio::mmap_sink mmap;
    std::error_code error;

    std::vector<uint8_t> rom_banks;

    std::array<uint8_t, 512> ram_banks;
};
