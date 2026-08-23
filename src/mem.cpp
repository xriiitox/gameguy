#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
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

    for (size_t i = 0; i < buffer.size(); i++) {
        bus->banks = i / 0x4000;
        size_t offset = i % 0x4000;

        if (bus->banks == 0) {
            bus->bank0[offset] = static_cast<uint8_t>(buffer[i]);
        } else  {
            if (bus->banks < 129) {
                bus->bankx[bus->banks][offset] = static_cast<uint8_t>(buffer[i]);
            }
        }
    }
}

void switch_rom_bank(uint8_t val, Bus* bus) {
    int banknum = val & 0x1F;
    if (banknum == 0) banknum++;
    if (banknum > bus->banks) {
        banknum &= bus->banks;
    }
    bus->sel_bank = banknum;
}

void switch_ram_bank() {

}
