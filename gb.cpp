#include "gb.h"
#include "mem.h"
#include <memory>

GameBoy::GameBoy(GBConfig gbconf) {
    Load_Rom(gbconf.filename, bus);
    this->cpu = std::make_unique<CPU>(this->bus);
}

void GameBoy::cycle() {
    // inst cycle

    // ppu/timers cycle
}
