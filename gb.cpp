#include "gb.h"
#include "mem.h"
#include <memory>
#include <chrono>

GameBoy::GameBoy(GBConfig gbconf) {
    Load_Rom(gbconf.filename, bus);
    this->cpu = std::make_unique<CPU>(this->bus);
    std::cout << "done with constructor" << std::endl;
}

using Clock = std::chrono::steady_clock;
double GameBoy::now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

// runs instructions
void GameBoy::cycle() {
    t = now_seconds();
    // inst cycle
    while (t >= next_inst) {
        // std::cout << std::hex << (int)bus[cpu->pc] << " ";
        cpu->opcode(bus[cpu->pc++]);
        next_inst += (cpu->t_cycle) * cycle_dt;
    }

    if (bus[0xFF02] == 0x81) {
        std::cout << (char)bus[0xFF01] << std::flush;
        bus[0xFF02] = 0;
    }
    // ppu/timers cycle
}
