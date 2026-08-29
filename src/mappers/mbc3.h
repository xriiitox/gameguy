#pragma once
#include "mapper.h"
#include <string>
#include <system_error>
#include <vector>
#include <array>
#include "../mio.hpp"

class MBC3 : public Mapper {
public:
    MBC3(std::string filename, bool timer, bool ram, bool battery);
    ~MBC3();
    virtual uint8_t read(uint16_t addr);
    virtual void write(uint16_t addr, uint8_t val);
    void tick_rtc();
private:
    // cart reg: rom/ram select/enable
    uint8_t ram_timer_en = 0;
    uint8_t rom_select = 1;
    uint8_t ram_rtc_sel = 0;

    uint8_t rom_helper(size_t bank, uint16_t offset);

    mio::mmap_sink mmap;
    std::error_code error;

    std::vector<uint8_t> rom_banks;

    std::vector<uint8_t> ram_banks;

    std::array<uint32_t, 5> rtc_regs;
    int latch = 0;
    std::array<uint32_t, 5> latched_regs;
};
