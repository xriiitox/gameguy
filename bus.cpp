#include "bus.h"
#include "gb.h"
#include "mem.h"
#include <algorithm>

Bus::Bus(void* gb) {
    this->gb = gb;
    this->div = (uint8_t*)&((GameBoy*)this->gb)->sysclk + 1;
    *this->div = 0xAB;
    for (auto bank : bankx) {
        bank.fill(0);
    }
}

bool is_external_bus(uint16_t addr) {
    return (addr <= 0x7FFF) ||
           (addr >= 0xA000 && addr <= 0xFDFF);
}

bool is_vram_bus(uint16_t addr) {
    return (addr >= 0x8000 && addr <= 0x9FFF);
}

void Bus::write(uint16_t addr, uint8_t val, bool tk) {
    if (tk) ((GameBoy*)this->gb)->tick();
    if (((GameBoy*)this->gb)->dma.bus_locked) {
            uint16_t dma_src = ((GameBoy*)this->gb)->dma.source;

            if (addr >= 0xFE00 && addr <= 0xFE9F) return; // OAM locked
            if (is_external_bus(dma_src) && is_external_bus(addr)) return;
            if (is_vram_bus(dma_src) && is_vram_bus(addr)) return;
        }
    if (addr <= 0x1FFF) { ram_en = val; return; } // MBC RAM enable
    if (addr >= 0x2000 && addr <= 0x3FFF) { switch_rom_bank(val, this); return; }
    if (addr <= 0x7FFF) return; // ROM is obviously read only
    if (addr <= 0x9FFF) { vram[addr - 0x8000] = val; return; } // vram
    if (addr <= 0xBFFF) { eram[addr - 0xA000] = val; return; } // external ram
    if (addr <= 0xCFFF) { wram1[addr - 0xC000] = val; return; }
    if (addr <= 0xDFFF) { wram2[addr - 0xD000] = val; return; }
    if (addr <= 0xEFFF) { wram1[addr - 0xE000] = val; return; } // echo ram section 1
    if (addr <= 0xFDFF) { wram2[addr - 0xF000] = val; return; } // echo ram section 2
    if (addr <= 0xFE9F) { oam[addr - 0xFE00] = val; return; } // oam
    // controller
    if (addr == 0xFF00) { joyp = val & 0xF0; return; }
    // serial
    if (addr == 0xFF01) { sb = val; return; }
    if (addr == 0xFF02) { sc = val; if (val & 0x80) std::cout << (char)sb << std::flush; sc &= ~0x80; IF |= (1 << 3); return; }
    if (addr == 0xFF04) { write_timer(0, true); return; }
    if (addr == 0xFF05) { write_tima(val); return; }
    if (addr == 0xFF06) { write_tma(val); return; }
    if (addr == 0xFF07) { write_timer(val, false); return; }
    if (addr == 0xFF0F) { IF = val; return; }
    if (addr == 0xFF40) { lcdc = val; return; }
    if (addr == 0xFF41) { stat = val & 0xF8; return; } // leave last few bits read only
    if (addr == 0xFF42) { scy = val; return; }
    if (addr == 0xFF43) { scx = val; return; }
    if (addr == 0xFF44) { ly = 0; return; } // reset scanline
    if (addr == 0xFF45) { lyc = val; return; }
    if (addr == 0xFF46) { write_dma(val); return; }
    if (addr == 0xFF47) { bgp = val; return; }
    if (addr == 0xFF48) { obp0 = val; return; }
    if (addr == 0xFF49) { obp1 = val; return; }
    if (addr == 0xFF4A) { wy = val; return; }
    if (addr == 0xFF4B) { wx = val; return;}
    if (addr <= 0xFFFE) { hram[addr - 0xFF80] = val; return; } // high ram
    if (addr == 0xFFFF) { ie = val; return; }
 }

uint8_t* Bus::read(uint16_t addr, bool tk, bool bypass) {
    bool isdma = ((GameBoy*)this->gb)->dma.bus_locked;
    if (tk) ((GameBoy*)gb)->tick();
    // if (tk && (!isdma || (addr >= 0xFF80 && addr <= 0xFFFE))) ((GameBoy*)this->gb)->tick(); // tick if read from hram during dma
    // only allow reads from hram or source addr during dma
    if (!bypass && isdma) {
            uint16_t dma_src = ((GameBoy*)this->gb)->dma.source;

            // OAM is always locked for CPU reads during DMA
            if (addr >= 0xFE00 && addr <= 0xFE9F) {
                static uint8_t open_bus = 0xFF;
                return &open_bus;
            }

            // Check for conflict on External Bus
            if (is_external_bus(dma_src) && is_external_bus(addr)) {
                static uint8_t open_bus = 0xFF;
                return &open_bus;
            }

            // Check for conflict on VRAM Bus
            if (is_vram_bus(dma_src) && is_vram_bus(addr)) {
                static uint8_t open_bus = 0xFF;
                return &open_bus;
            }
        }
    if (addr <= 0x3FFF) return &bank0[addr];
    if (addr <= 0x7FFF) return &bankx[sel_bank][addr - 0x4000];
    if (addr <= 0x9FFF) return &vram[addr - 0x8000];
    if (addr <= 0xBFFF) return &eram[addr - 0xA000];
    if (addr <= 0xCFFF) return &wram1[addr - 0xC000];
    if (addr <= 0xDFFF) return &wram2[addr - 0xD000];
    if (addr <= 0xEFFF) return &wram1[addr - 0xE000];
    if (addr <= 0xFDFF) return &wram2[addr - 0xF000];
    if (addr <= 0xFE9F) return &oam[addr - 0xFE00];
    // read io regs
    if (addr == 0xFF00) return &joyp;
    if (addr == 0xFF01) return &sb;
    if (addr == 0xFF02) return &sc;
    if (addr == 0xFF04) return div; // pointer to div
    if (addr == 0xFF05) return &tima;
    if (addr == 0xFF06) return &tma;
    if (addr == 0xFF07) return &tac;
    if (addr == 0xFF0F) return &IF;
    if (addr == 0xFF40) return &lcdc;
    if (addr == 0xFF41) return &stat;
    if (addr == 0xFF42) return &scy;
    if (addr == 0xFF43) return &scx;
    if (addr == 0xFF44) return &ly;
    if (addr == 0xFF45) return &lyc;
    if (addr == 0xFF46) return &dma;
    if (addr == 0xFF47) return &bgp;
    if (addr == 0xFF48) return &obp0;
    if (addr == 0xFF49) return &obp1;
    if (addr == 0xFF4A) return &wy;
    if (addr == 0xFF4B) return &wx;
    if (addr <= 0xFFFE) return &hram[addr - 0xFF80];
    if (addr == 0xFFFF) return &ie;

    if (addr >= 0xFF00 && addr <= 0xFF7F) {
        static uint8_t io_open_bus = 0xFF;
        io_open_bus = 0xFF;
        return &io_open_bus;
    }

    static uint8_t default_open_bus = 0xFF;
    default_open_bus = 0xFF;
    return &default_open_bus;
}

// stealing code from gb.cpp :thumbsup:
void Bus::write_timer(uint8_t value, bool div) {
    static const uint16_t bit_masks[4] = {
        (1 << 9),
        (1 << 3),
        (1 << 5),
        (1 << 7),
    };

    uint16_t mask = bit_masks[tac & 0x03];
    bool timer_enable = (tac & 0x04) != 0;

    bool prev_signal = (((GameBoy*)gb)->sysclk & mask) && timer_enable;

    if (div)
        ((GameBoy*)gb)->sysclk = 0;
    else {
        tac = value;
        mask = bit_masks[tac & 0x03];
        timer_enable = (tac & 0x04) != 0;
    }

    bool signal = (((GameBoy*)gb)->sysclk & mask) && timer_enable;

    if (prev_signal && !signal) {
        tima++;
        if (tima == 0) {
            ((GameBoy*)gb)->tima_reload_pending = true;
        }
    }
}

void Bus::write_tima(uint8_t val) {
    if (((GameBoy*)gb)->tima_reload_pending) {
        ((GameBoy*)gb)->tima_reload_pending = false;
    }
    tima = val;
}

void Bus::write_tma(uint8_t val) {
    tma = val;
    if (((GameBoy*)gb)->tima_reload_pending) {
        tima = val;
    }
}

void Bus::write_dma(uint8_t val) {
    GameBoy* gub = (GameBoy*)gb;
    uint16_t src = val << 8;
    if (src >= 0xE000) {
        src -= 0x2000; // Map 0xE000-0xFFFF down to 0xC000-0xDF00
    }
    gub->dma.source = src;
    gub->dma.index = 0;
    gub->dma.start_delay = 1;
    gub->dma.active = true;
    gub->dma.bus_locked = false;
    dma = val;
}
