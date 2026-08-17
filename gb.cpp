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

    // timers
    timer_count++;
    sysclk += 4; // system counter/div
    if (bus->tac & 0x04) { // TAC/TIMA
        switch (bus->tac & 0x03) {
            case 0:
                if (timer_count % 256 == 0) {
                    bus->tima++;
                    if (bus->tima == 0) bus->IF |= (1 << 2); // request timer interrupt
                    timer_count = 0;
                }
                break;
            case 1:
                if (timer_count % 4 == 0) {
                    bus->tima++;
                    if (bus->tima == 0) bus->IF |= (1 << 2); // request timer interrupt
                    timer_count = 0;
                }
                break;
            case 2:
                if (timer_count % 16 == 0) {
                    bus->tima++;
                    if (bus->tima == 0) bus->IF |= (1 << 2); // request timer interrupt
                    timer_count = 0;
                }
                break;
            case 3:
                if (timer_count % 64 == 0) {
                    bus->tima++;
                    if (bus->tima == 0) bus->IF |= (1 << 2); // request timer interrupt
                    timer_count = 0;
                }
                break;
        }
    }

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
        cpu->t_cycle -= 4; // compensate for opcode read
        next_inst += (cpu->t_cycle) * cycle_dt;
    }

    handle_interrupts();

}

void GameBoy::handle_interrupts() {
    if (cpu->ime) {
        // an interrupt is enabled and allowed
        if (*bus->read(0xFFFF) & *bus->read(0xFF0F)) {
            // vblank
            if ((*bus->read(0xFFFF) & 1) & (*bus->read(0xFF0F) & 1)) {
                bus->write(--cpu->sp, cpu->pc >> 8);
                bus->write(--cpu->sp, cpu->pc & 0xFF);
                cpu->pc = 0x40;
                bus->write(0xFF0F, *bus->read(0xFF0F) & ~1);
            }

            // STAT
            if ((*bus->read(0xFFFF) & 2) & (*bus->read(0xFF0F) & 2)) {
                bus->write(--cpu->sp, cpu->pc >> 8);
                bus->write(--cpu->sp, cpu->pc & 0xFF);
                cpu->pc = 0x48;
                bus->write(0xFF0F, *bus->read(0xFF0F) & ~2);
            }

            // Timer int
            if ((*bus->read(0xFFFF) & 4) & (*bus->read(0xFF0F) & 4)) {
                bus->write(--cpu->sp, cpu->pc >> 8);
                bus->write(--cpu->sp, cpu->pc & 0xFF);
                cpu->pc = 0x50;
                bus->write(0xFF0F, *bus->read(0xFF0F) & ~4);
            }

            // Serial int
            if ((*bus->read(0xFFFF) & 8) & (*bus->read(0xFF0F) & 8)) {
                bus->write(--cpu->sp, cpu->pc >> 8);
                bus->write(--cpu->sp, cpu->pc & 0xFF);
                cpu->pc = 0x58;
                bus->write(0xFF0F, *bus->read(0xFF0F) & ~8);
            }

            // Joypad int
            if ((*bus->read(0xFFFF) & 16) & (*bus->read(0xFF0F) & 16)) {
                bus->write(--cpu->sp, cpu->pc >> 8);
                bus->write(--cpu->sp, cpu->pc & 0xFF);
                cpu->pc = 0x60;
                bus->write(0xFF0F, *bus->read(0xFF0F) & ~16);
            }
        }
    }
}
