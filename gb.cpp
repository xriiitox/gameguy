#include "gb.h"
#include "mem.h"
#include <memory>
#include <chrono>
#include <iomanip>

GameBoy::GameBoy(GBConfig gbconf) {
    this->bus = std::make_shared<Bus>(this);
    Load_Rom(gbconf.filename, bus.get());
    this->cpu = std::make_shared<CPU>(this->bus.get(), this);
    std::cout << "done with constructor" << std::endl;
}

using Clock = std::chrono::steady_clock;
double GameBoy::now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

void GameBoy::tick() { // m-cycles
    cpu->t_cycle += 4;
    sysclk += 4;
    // timer tick TODO
    // apu tick TODO
    // ppu tick TODO
}

// runs instructions
void GameBoy::cycle() {
    t = now_seconds();
    // inst cycle
    while (t >= next_inst) {

        // if (cpu->pc == 0x0206) cpu->debugPrint();
        cpu->opcode(*bus->read(cpu->pc++));
        cpu->t_cycle -= 4; // compensate for opcode/debug read?
        next_inst += (cpu->t_cycle) * cycle_dt;

        // serial out
        if (*bus->read(0xFF02) == 0x81) {
            std::cout << (char)*bus->read(0xFF01) << std::flush;
            bus->write(0xFF02, 0);
        }
    }
    // ppu/timers cycle
}
