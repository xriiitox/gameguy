#include "mbc1.h"
#include "mapper.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <array>
#include <algorithm>

MBC1::MBC1(std::string filename, bool ram, bool battery) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

    mbc1mm = detect_multicart(buffer);
    std::cout << mbc1mm << std::endl;

    Mapper::rom_size = length;
    Mapper::ram = ram;
    Mapper::battery = battery;
    Mapper::bank_count = length / 0x4000;

    Mapper::ram_bank_count = 0;
    switch ((uint8_t)buffer[0x0149]) {
        case 0x0:
            Mapper::ram = false;
            Mapper::ram_bank_count = 0;
            break;
        case 0x2:
            Mapper::ram_bank_count = 1;
            break;
        case 0x3:
            Mapper::ram_bank_count = 4;
            break;
        case 0x4:
            Mapper::ram_bank_count = 16;
            break;
        case 0x5:
            Mapper::ram_bank_count = 8;
            break;
    }

    if (Mapper::ram) {
        ram_banks.resize(Mapper::ram_bank_count * 0x2000);
    }

    std::string ext = p.extension().string();
    int extnum;
    if (ext == "gb") {
        extnum = 2;
    } else {
        extnum = 3; // gbc filename
    }
    std::string savename = filename.substr(0, filename.size()-extnum)+".sav";
    if (std::filesystem::exists(savename) && Mapper::battery && Mapper::ram) {
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
        std::filesystem::path savepath{savename};
        auto savelength = std::filesystem::file_size(savepath);
        std::ifstream save(savename, std::ios_base::binary);
        std::vector<std::byte> rambuf(savelength);
        save.read(reinterpret_cast<char*>(rambuf.data()), savelength);
        ram_banks.resize(savelength);
        std::memcpy(ram_banks.data(), rambuf.data(), savelength);
        save.close();

    } else if (Mapper::battery && Mapper::ram) {
        // create file
        std::ofstream file;
        file.open(savename, std::ios::binary | std::ios::trunc);
        std::vector<char> emptyRam(ram_bank_count*0x2000, 0xFF);
        file.write(emptyRam.data(), ram_bank_count*0x2000);
        file.close();
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
    }
    // load rom
    rom_banks.resize(length);
    std::memcpy(rom_banks.data(), buffer.data(), length);
}

MBC1::~MBC1() {
    if (Mapper::battery) {
        for (int i = 0; i < ram_banks.size(); i++) {
            mmap[i] = ram_banks[i];
        }
        mmap.sync(error);
    }
}

bool MBC1::detect_multicart(const std::vector<std::byte>& rom) {
    constexpr std::array<uint8_t, 8> logo = { 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B };
    if (rom.size() >= 0x80108) {
        bool logo_at_1mb = (std::memcmp(rom.data() + 0x40104, logo.data(), 8) == 0);
        bool logo_at_2mb = (std::memcmp(rom.data() + 0x80104, logo.data(), 8) == 0);
        return logo_at_1mb || logo_at_2mb;
    }
    return false;
}

uint8_t MBC1::read(uint16_t addr) {
    if (addr >= 0x0000 && addr <= 0x3FFF) {
        size_t bank = 0;
        if (mode_select & 0x01) {
            size_t shift = mbc1mm ? 4 : 5;
            bank = (two_bit & 0x03) << shift;
        }

        int max_banks = mbc1mm ? 64 : 128;
        size_t effective_banks = std::min(Mapper::bank_count, max_banks);
        bank &= (effective_banks - 1);

        return rom_helper(bank, addr);
    } else if (addr >= 0x4000 && addr <= 0x7FFF) { // rom banks
        size_t low_mask = mbc1mm ? 0x0F : 0x1F;
        size_t shift = mbc1mm ? 4 : 5;

        size_t low_bits = rom_select & low_mask;
        if (low_bits == 0) low_bits = 1;

        size_t raw_bank = low_bits;;
        if (mbc1mm || (mode_select & 0x01) || Mapper::bank_count >= 64) {
            raw_bank |= ((two_bit & 0x03) << shift);
        }
        int max_banks = mbc1mm ? 64 : 128;
        size_t effective_banks = std::min(Mapper::bank_count, max_banks);
        size_t bank = raw_bank & (effective_banks - 1);

        return rom_helper(bank, addr - 0x4000);
    } else if (addr >= 0xA000 && addr <= 0xBFFF) { // ram banks
        if (Mapper::ram && (ram_enable & 0x0F) == 0x0A) {
            if ((mode_select & 0x01)) {
                int sel_bank = (two_bit & 0x03);
                return ram_banks[(sel_bank % Mapper::ram_bank_count)*0x2000 + addr - 0xA000];
            } else return ram_banks[addr - 0xA000];
        }
    }
    return 0xFF;
}

uint8_t MBC1::rom_helper(size_t bank, uint16_t offset) {
    size_t real_addr = (bank * 0x4000) + offset;
    return rom_banks[real_addr % Mapper::rom_size];
}

void MBC1::write(uint16_t addr, uint8_t val) {
    if (addr >= 0 && addr <= 0x1FFF) {
        ram_enable = val & 0x0F;
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        rom_select = val & (mbc1mm ? 0x0F : 0x1F);
    } else if (addr >= 0x4000 && addr <= 0x5FFF) {
        two_bit = val & 0x03;
    } else if (addr >= 0x6000 && addr <= 0x7FFF) {
        mode_select = val & 0x01;
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (Mapper::ram) {
            if ((ram_enable & 0x0F) == 0x0A) {
                int sel_bank = (mode_select & 1) ? (two_bit & 0x03) : 0;
                ram_banks[(sel_bank % Mapper::ram_bank_count)*0x2000 + addr - 0xA000] = val;
            }
        }
    }
    return;
}
