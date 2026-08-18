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

    uint16_t sysOld = sysclk;
    sysclk += 4;

    if (tima_reload_pending) {
        bus->tima = bus->tma;
        bus->IF |= (1 << 2);
        tima_reload_pending = false;
    }

    static const uint16_t bit_masks[4] = {
        (1 << 9),
        (1 << 3),
        (1 << 5),
        (1 << 7),
    };

    uint16_t mask = bit_masks[bus->tac & 0x03];
    bool timer_enable = (bus->tac & 0x04) != 0;

    // falling edge detector
    bool prev_signal = (sysOld & mask) && timer_enable;
    bool signal = (sysclk & mask) && timer_enable;

    if (prev_signal && !signal) {
        bus->tima++;
        if (bus->tima == 0) {
            tima_reload_pending = true;
        }
    }

    if (cpu->ei_delay == 2) {
        cpu->ime = 1;
        cpu->ei_delay = 0;
    } else if (cpu->ei_delay == 1) {
        cpu->ei_delay = 2;
    }

    // apu tick TODO
    // ppu tick TODO
}

// runs instructions
void GameBoy::cycle() {
    t = now_seconds();
    // inst cycle
    while (t >= next_inst) {
        if (cpu->halted) {
            tick();
            next_inst += 4 * cycle_dt;

            if (*bus->read(0xFFFF, false) & *bus->read(0xFF0F, false) & 0x1F) {
                cpu->halted = false;
                if (cpu->ime) handle_interrupts();
            }
        } else {
            handle_interrupts();
            cpu->opcode(*bus->read(cpu->pc++));
            next_inst += (cpu->t_cycle) * cycle_dt;
        }
    }
}

void GameBoy::handle_interrupts() {
    if (!cpu->ime) return;

    uint8_t pending = *bus->read(0xFFFF, false) & *bus->read(0xFF0F, false) & 0x1F;
    if (!pending) return;

    static const uint16_t vectors[5] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
    for (int i = 0; i < 5; i++) {
        if (pending & (1 << i)) {
            cpu->ime = false;
            tick();
            bus->write(0xFF0F, *bus->read(0xFF0F) & ~(1 << i));
            bus->write(--cpu->sp, cpu->pc >> 8);
            bus->write(--cpu->sp, cpu->pc & 0xFF);
            cpu->pc = vectors[i];
            break;
        }
    }
}
