#include "gb.h"
#include "mem.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <memory>
#include <chrono>
#include <iomanip>
#include <vector>

GameBoy::GameBoy(GBConfig gbconf, SDL_Renderer* ren) {
    this->bus = std::make_shared<Bus>(this);
    Load_Rom(gbconf.filename, bus.get());
    this->cpu = std::make_shared<CPU>(this->bus.get(), this);
    this->ppu = std::make_shared<PPU>(this->bus.get());
    this->texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetTextureScaleMode(this->texture, SDL_SCALEMODE_PIXELART);
}

using Clock = std::chrono::steady_clock;
double GameBoy::now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

void GameBoy::tick() { // m-cycles
    cpu->t_cycle += 4;
    cycles_frame++;

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

    ppu->tick();

    // oam dma
    tick_dma();

    // apu tick TODO
}

// runs instructions
void GameBoy::cycle() {
    t = now_seconds();
    static const uint32_t MCYCLES_PER_FRAME = 17556;

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
            cpu->t_cycle = 0;
            handle_interrupts();
            uint8_t opcode = *bus->read(cpu->pc);
            if (!cpu->halt_bug) cpu->pc++;
            else cpu->halt_bug = false;
            cpu->opcode(opcode);
            next_inst += (cpu->t_cycle) * cycle_dt;
        }
    }
    if (cycles_frame >= MCYCLES_PER_FRAME) {
        SDL_UpdateTexture(texture, nullptr, ppu->framebuffer, 160*sizeof(uint32_t));
    }

}

void GameBoy::handle_interrupts() {
    if (!cpu->ime) return;

    uint8_t oldIE = *bus->read(0xFFFF, false);
    uint8_t oldIF = *bus->read(0xFF0F, false);
    uint8_t pend = oldIE & oldIF & 0x1F;
    if (!pend) return;

    int irq = -1;
    for (int i = 0; i < 5; i++) {
        if (pend & (1 << i)) {
            irq = i;
            break;
        }
    }

    cpu->ime = false;
    tick();

    tick();

    cpu->sp--;
    bus->write(cpu->sp, cpu->pc >> 8, true);

    uint8_t IE = *bus->read(0xFFFF, false);
    uint8_t IF = *bus->read(0xFF0F, false);
    uint8_t final_pend = IE & IF & 0x1F;

    cpu->sp--;
    bus->write(cpu->sp, cpu->pc & 0xFF, true);

    uint16_t vec = 0;
    static const uint16_t vectors[5] = { 0x0040, 0x0048, 0x0050, 0x0058, 0x0060 };

    if (irq != -1 && (IE & (1 << irq))) {
        bus->write(0xFF0F, IF & ~(1 << irq), false);
        vec = vectors[irq];
    } else if (final_pend != 0) {
        for (int i = 0; i < 5; i++) {
            if (final_pend & (1 << i)) {
                bus->write(0xFF0F, IF & ~(1 << i), false);
                vec = vectors[i];
                break;
            }
        }
    }
    tick();
    cpu->pc = vec;
}

void GameBoy::tick_dma() {
    if (!dma.active) return;
    if (dma.start_delay > 0) {
        dma.start_delay--;
        if (dma.start_delay == 0) {
            dma.bus_locked = true;
        }
        return;
    }

    uint16_t src_addr = dma.source + dma.index;
    uint8_t byte = 0xFF;
    if (src_addr < 0xFE00) {
        byte = *bus->read(src_addr, false, true);
    }

    bus->oam[dma.index] = byte;

    dma.index++;

    if (dma.index == 160) {
        dma.active = false;
        dma.bus_locked = false;
    }
}
