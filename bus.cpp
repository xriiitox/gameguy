#include "bus.h"
#include "gb.h"
#include "mem.h"

Bus::Bus(void* gb) {
    this->gb = gb;
    this->div = (uint8_t*)&((GameBoy*)this->gb)->sysclk + 1;
    *this->div = 0xAB;
}

void Bus::write(uint16_t addr, uint8_t val, bool tk) {
    if (tk) ((GameBoy*)this->gb)->tick();
    if (((GameBoy*)this->gb)->dma.bus_locked) {
        if (addr < 0xFF80 || addr > 0xFFFE) return;
    }
    if (addr <= 0x1FFF) return; // MBC RAM enable
    if (addr >= 0x2000 && addr <= 0x3FFF) { switch_rom_bank(val); return; }
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
    if (addr == 0xFF44) { return; } // readonly
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
    if (tk && (!isdma || (addr >= 0xFF80 && addr <= 0xFFFE))) ((GameBoy*)this->gb)->tick(); // tick if read from hram during dma
    // only allow reads from hram or source addr during dma
    if (!bypass && isdma) {
        if (addr < 0xFF80 || addr > 0xFFFE) return &pholder;
    }
    if (addr <= 0x3FFF) return &bank0[addr];
    if (addr <= 0x7FFF) return &bank1[addr - 0x4000];
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
    std::cout << "how did you get here" << std::endl;
    return nullptr;
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
    gub->dma.source = val << 8;
    gub->dma.index = 0;
    gub->dma.start_delay = 2;
    gub->dma.active = true;
    gub->dma.bus_locked = false;
    dma = val;
}
