#include "mbc2.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>

MBC2::MBC2(std::string filename, bool battery) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

    Mapper::rom_size = length;
    Mapper::ram = true;
    Mapper::battery = battery;
    Mapper::bank_count = length / 0x4000;

    std::string ext = p.extension().string();
    int extnum;
    if (ext == "gb") {
        extnum = 2;
    } else {
        extnum = 3; // gbc filename
    }
    std::string savename = filename.substr(0, filename.size()-extnum)+".sav";
    if (std::filesystem::exists(savename) && Mapper::battery) {
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
        std::filesystem::path savepath{savename};
        auto savelength = std::filesystem::file_size(savepath);
        std::ifstream save(savename, std::ios_base::binary);
        std::vector<std::byte> rambuf(savelength);
        save.read(reinterpret_cast<char*>(rambuf.data()), savelength);
        std::memcpy(ram_banks.data(), rambuf.data(), savelength);
        save.close();
    } else {
        // create file
        std::ofstream file;
        file.open(savename, std::ios::binary | std::ios::trunc);
        std::vector<char> emptyRam(512, 0xFF);
        file.write(emptyRam.data(), 512);
        file.close();
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
    }
    // load rom
    rom_banks.resize(length);
    std::memcpy(rom_banks.data(), buffer.data(), length);
}

MBC2::~MBC2() {
    if (Mapper::battery) {
        for (int i = 0; i < ram_banks.size(); i++) {
            mmap[i] = ram_banks[i];
        }
        mmap.sync(error);
    }
}

uint8_t MBC2::read(uint16_t addr) {
    if (addr >= 0 && addr <= 0x3FFF) {
        return rom_banks[addr];
    }
    if (addr >= 0x4000 && addr <= 0x7FFF) {
        int bank = rom_sel & 0x0F;
        if (bank == 0) bank = 1;
        return rom_helper(bank, addr - 0x4000);
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if ((ram_en & 0x0F) == 0xA)
            return ram_banks[addr & 0x01FF];
        return 0xFF;
    }
    return 0xFF;
}

uint8_t MBC2::rom_helper(size_t bank, uint16_t offset) {
    size_t real_addr = (bank * 0x4000) + offset;
    return rom_banks[real_addr % Mapper::rom_size];
}

void MBC2::write(uint16_t addr, uint8_t val) {
    bool bit8 = (addr & 0x0100) != 0;
    if (addr >= 0 && addr <= 0x3FFF) {
        if (bit8) {
            rom_sel = val & 0x0F;
        } else {
            ram_en = val & 0x0F;
        }
        return;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if ((ram_en & 0x0F) == 0xA)
            ram_banks[addr & 0x01FF] = (val & 0x0F) | 0xF0;
        return;
    }

    return;
}
