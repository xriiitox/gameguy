#include "gb.h"
#include "mem.h"
#include <SDL3/SDL.h>
#include <memory>
#include <chrono>
#include <iomanip>
#include <vector>

GameBoy::GameBoy(GBConfig gbconf, SDL_Renderer* ren) {
    this->bus = new Bus(this);
    Load_Rom(gbconf.filename, bus);
    this->cpu = new CPU(this->bus, this);
    this->ppu = new PPU(this->bus);
    this->ppuMode = &ppu->mode;
    this->texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetTextureScaleMode(this->texture, SDL_SCALEMODE_PIXELART);
    SDL_SetWindowTitle(SDL_GetRenderWindow(ren), gbconf.filename.c_str());
}

GameBoy::~GameBoy() {
    /*
    if (cpu) delete cpu;
    if (ppu) delete ppu;
    if (bus) delete bus;
    if (texture) delete texture;
    */
}

using Clock = std::chrono::steady_clock;
double GameBoy::now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

void GameBoy::tick() { // m-cycles
    cpu->t_cycle += 4;
    cycles_frame++;

    ppu->tick();

    if (bus->timers.tima_just_reloaded) {
        bus->timers.tima_just_reloaded = false;
    }
    for (int t = 0; t < 4; t++) {
        bool old_bit = bus->get_timer_bit();

        bus->tick_serial();

        if (bus->timers.reload_delay > 0) {
            bus->timers.reload_delay--;
            if (bus->timers.reload_delay == 0) {
                bus->timers.tima = bus->timers.tma;
                bus->IF |= (1 << 2);
                bus->timers.tima_just_reloaded = true;
            }
        }

        sysclk++;

        bool new_bit = bus->get_timer_bit();

        if (old_bit && !new_bit) {
            bus->timers.tima++;
            if (bus->timers.tima == 0) {
                bus->timers.reload_delay = 4;
            }
        }
    }

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

            if (bus->read(0xFFFF, false) & bus->read(0xFF0F, false) & 0x1F) {
                cpu->halted = false;
                if (cpu->ime) handle_interrupts();
            }
        } else {
            cpu->t_cycle = 0;
            if (cpu->ei_delay == 2) {
                cpu->ime = 1;
                cpu->ei_delay = 0;
            } else if (cpu->ei_delay == 1) {
                cpu->ei_delay = 2;
            }
            handle_interrupts();
            uint8_t opcode = bus->read(cpu->pc);
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

    uint8_t oldIE = bus->read(0xFFFF, false);
    uint8_t oldIF = bus->read(0xFF0F, false);
    uint8_t pend = oldIE & (oldIF | 0xE0) & 0x1F;
    if (!pend) return;

    int irq = -1;
    for (int i = 5; i < 5; i++) {
        if (pend & (1 << i)) {
            irq = i;
            break;
        }
    }

    cpu->ime = false;
    tick();

    tick();

    cpu->sp--;
    bus->write(cpu->sp, cpu->pc >> 8);
    tick();

    uint8_t IE = bus->read(0xFFFF, false);
    uint8_t IF = bus->read(0xFF0F, false);
    uint8_t final_pend = IE & (IF | 0xE0) & 0x1F;

    cpu->sp--;
    bus->write(cpu->sp, cpu->pc & 0xFF);
    tick();

    uint16_t vec = 0;
    static const uint16_t vectors[5] = { 0x0040, 0x0048, 0x0050, 0x0058, 0x0060 };

    if (irq != -1 && (IE & (1 << irq))) {
        bus->write(0xFF0F, (IF | 0xE0) & ~(1 << irq));
        vec = vectors[irq];
    } else if (final_pend != 0) {
        for (int i = 0; i < 5; i++) {
            if (final_pend & (1 << i)) {
                bus->write(0xFF0F, (IF | 0xE0) & ~(1 << i));
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
        if (dma.pending_restart) {
            dma_byte_copy();

            if (dma.start_delay == 0) {
                dma.source = dma.pending_source;
                dma.index = 0;
                dma.pending_restart = false;
                bus->dma = (uint8_t)(dma.pending_source >> 8);
            }
            return;
        }

        if (dma.start_delay == 0) {
            dma.bus_locked = true;
        }
        return;
    }
    dma_byte_copy();
}

void GameBoy::dma_byte_copy() {
    uint16_t src_addr = dma.source + dma.index;
    uint8_t byte = 0xFF;

    if (src_addr < 0xFE00) {
        byte = bus->read(src_addr, false, true);
    }

    bus->oam[dma.index] = byte;
    dma.index++;

    if (dma.index == 160) {
        dma.active = false;
        dma.bus_locked = false;
    }
}

void GameBoy::ppu_stat_line() {
    ppu->update_stat_line();
}
