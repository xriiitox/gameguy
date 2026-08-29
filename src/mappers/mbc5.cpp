#include "mbc5.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>

MBC5::MBC5(std::string filename, bool ram, bool battery) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

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

MBC5::~MBC5() {
    if (Mapper::battery) {
        for (int i = 0; i < ram_banks.size(); i++) {
            mmap[i] = ram_banks[i];
        }
        mmap.sync(error);
    }
}

uint8_t MBC5::read(uint16_t addr) {
    if (addr >= 0 && addr <= 0x3FFF) {
        return rom_banks[addr];
    }
    if (addr >= 0x4000 && addr <= 0x7FFF) {
        int bank = ((bit9 & 1) << 8) | rom_select;
        return rom_helper(bank, addr - 0x4000);
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if ((ram_enable & 0x0F) == 0x0A) {
            return ram_banks[(ram_select % Mapper::ram_bank_count)*0x2000 + addr - 0xA000];
        }
    }
    return 0xFF;
}

uint8_t MBC5::rom_helper(size_t bank, uint16_t offset) {
    size_t real_addr = (bank * 0x4000) + offset;
    return rom_banks[real_addr % Mapper::rom_size];
}

void MBC5::write(uint16_t addr, uint8_t val) {
    if (addr >= 0 && addr <= 0x1FFF) {
        ram_enable = val & 0x0F;
        return;
    }
    if (addr >= 0x2000 && addr <= 0x2FFF) {
        rom_select = val;
        return;
    }
    if (addr >= 0x3000 && addr <= 0x3FFF) {
        bit9 = val & 1;
        return;
    }
    if (addr >= 0x4000 && addr <= 0x5FFF) {
        ram_select = val;
        return;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (Mapper::ram && (ram_enable & 0x0F) == 0x0A)
            ram_banks[(ram_select % Mapper::ram_bank_count)*0x2000 + addr - 0xA000] = val;
        return;
    }
}
