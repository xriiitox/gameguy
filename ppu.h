#include "bus.h"

#pragma once

class PPU {
public:
    PPU(Bus* bus);

    void tick();

    int framebuffer[160*144];
private:
    Bus* bus;

    uint8_t mode = 2;
    uint16_t mode_cycles = 0;
    uint8_t lx = 0;
    uint8_t fetch_x = 0;

    uint8_t bg_fifo_color[8];
    uint8_t fifo_size = 0;
    void render_single_dot();

};
