#include "ppu.h"
#include <sys/types.h>
#include <algorithm>

PPU::PPU(Bus* bus) {
    this->bus = bus;
    // ensure framebuffer empty
    std::fill(framebuffer, framebuffer + (160*144), 0u);
}

void PPU::tick() {
    mode_cycles += 4;

    switch (mode) {
        case 2: // OAM read (80t/20m)
            if (mode_cycles >= 80) {
                mode = 3;
                lx = 0;
                // Pre-fill or prepare background fetcher
            }
            break;
        case 3: // pixel transfer
            for (int dot = 0; dot < 4; dot++) {
                if (lx < 160) {
                    render_single_dot();
                    lx++;
                }
            }
            if (lx >= 160) { // enter hblank
                mode = 0;
                bus->IF |= 2; // request lcd/stat interrupt
            }
            break;
        case 0: // hblank period
            if (mode_cycles >= 456) {
                mode_cycles -= 456;
                bus->ly++;

                if (bus->ly == 144) {
                    mode = 1; // enter vblank
                    bus->IF |= 1; // req vblank int
                } else mode = 2;
            }
            break;
        case 1: // vblank
            if (mode_cycles >= 456) {
                mode_cycles -= 456;
                bus->ly++;
                if (bus->ly > 153) {
                    bus->ly = 0;
                    mode = 0;
                }
            }
            break;
    }
}

void PPU::render_single_dot() {
    // tile coordinates
    uint8_t scan_x = lx + *bus->read(0xFF43, false); // scx
    uint8_t scan_y = bus->ly + *bus->read(0xFF42, false); // scy

    uint8_t tile_x = scan_x / 8;
    uint8_t tile_y = scan_y / 8;
    uint8_t pixel_x = scan_x % 8;
    uint8_t pixel_y = scan_y % 8;

    // background title id
    uint16_t tilemap = (bus->lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint8_t tile_id = bus->vram[tilemap - 0x8000 + (tile_y * 32) + tile_x];

    // get low+high tile data bytes
    uint16_t tile_addr;
    if (bus->lcdc & 0x10) { // 0x8000 unsigned
        tile_addr = 0x8000 + (tile_id * 16) + (pixel_y * 2);
    } else {
        int8_t signed_id = (int8_t)tile_id;
        tile_addr = 0x9000 + (signed_id * 16) + (pixel_y * 2);
    }

    uint8_t byte1 = bus->vram[tile_addr - 0x8000];
    uint8_t byte2 = bus->vram[tile_addr - 0x8000 + 1];

    // decode color data per pixel
    uint8_t bit = 7 - pixel_x;
    uint8_t color_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

    // apply palette
    uint8_t bgp = *bus->read(0xFF47, false);
    uint8_t final_color = (bgp >> (color_idx * 2)) & 0x03;
    int final_final_color;
    switch (final_color) {
        case 3: // darkest green
            final_final_color = 0xFF0F380F;
            break;
        case 2: // dark green
            final_final_color = 0xFF306230;
            break;
        case 1: // light green
            final_final_color = 0xFF8BAC0F;
            break;
        case 0: // lightest green
            final_final_color = 0xFF9BBC0F;
            break;
    }
    framebuffer[bus->ly*160 + lx] = final_final_color;

}
