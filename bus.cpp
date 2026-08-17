#include "bus.h"
#include "gb.h"
#include "mem.h"

Bus::Bus(void* gb) {
    this->gb = gb;
}

void Bus::write(uint16_t addr, uint8_t val) {
    ((GameBoy*)this->gb)->tick();
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
    if (addr <= 0xFF7F) { io[addr - 0xFF00] = val; return; } // io regs
    if (addr <= 0xFFFE) { hram[addr - 0xFF80] = val; return; } // high ram
    if (addr == 0xFFFF) { ie = val; return; }
 }

uint8_t* Bus::read(uint16_t addr) {
    ((GameBoy*)this->gb)->tick();
    if (addr <= 0x3FFF) return &bank0[addr];
    if (addr <= 0x7FFF) return &bank1[addr - 0x4000];
    if (addr <= 0x9FFF) return &vram[addr - 0x8000];
    if (addr <= 0xBFFF) return &eram[addr - 0xA000];
    if (addr <= 0xCFFF) return &wram1[addr - 0xC000];
    if (addr <= 0xDFFF) return &wram2[addr - 0xD000];
    if (addr <= 0xEFFF) return &wram1[addr - 0xE000];
    if (addr <= 0xFDFF) return &wram2[addr - 0xF000];
    if (addr <= 0xFE9F) return &oam[addr - 0xFE00];
    if (addr <= 0xFF7F) return &io[addr - 0xFF00];
    if (addr <= 0xFFFE) return &hram[addr - 0xFF80];
    if (addr == 0xFFFF) return &ie;
    std::cout << "how did you get here" << std::endl;
    return nullptr;
}
