#include <cstdint>
#include <string>
#include "bus.h"

void Load_Rom(std::string filename, Bus* bus);

void switch_rom_bank(uint8_t val, Bus* bus);

void switch_ram_bank();
