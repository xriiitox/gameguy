#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include "bus.h"


// will only work for up to 32kb rom for now
void Load_Rom(std::string filename, Bus* bus) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

    int fakepc = 0x0;
    for (auto byte : buffer) {
        if (fakepc <= 0x3FFF)
            bus->bank0[fakepc++] = static_cast<uint8_t>(byte);
        else
            bus->bank1[(fakepc++) - 0x4000] = static_cast<uint8_t>(byte);

    }
}

void switch_rom_bank(uint8_t val) {
    std::cout << "mbc unimplemented" << std::endl;
}
