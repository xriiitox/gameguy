#include "nomap.h"
#include <filesystem>
#include <fstream>

NoMapper::NoMapper(std::string filename) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();
    int i = 0;
    for (auto byte : buffer) {
        if (i <= 0x3FFF) {
            bank0[i] = static_cast<uint8_t>(buffer[i]);
        }
        else if (i >= 0x4000 && i <= 0x7FFF) {
            bank1[i-0x4000] = static_cast<uint8_t>(buffer[i]);
        }
        i++;
    }
}

uint8_t NoMapper::read(uint16_t addr) {
    if (addr >= 0 && addr <= 0x3FFF) {
        return bank0[addr];
    } else if (addr >= 0x4000 && addr <= 0x7FFF) {
        return bank1[addr - 0x4000];
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return eram[addr - 0xA000];
    }
    else return 0xFF;
}

void NoMapper::write(uint16_t addr, uint8_t val) {
    if (addr >= 0 && addr <= 0x7FFF) {
        return; // ROM is not writable
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        eram[addr - 0xA000] = val;
    }
}
