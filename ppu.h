#include "bus.h"

#pragma once

struct oam_obj {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_id;
    uint8_t attr;
    uint8_t oam_ind;
};

bool oam_comp(const oam_obj& a, const oam_obj& b);

class PPU {
public:
    PPU(Bus* bus);

    void tick();

    int framebuffer[160*144];
    uint8_t mode = 2;
    void update_stat_line();
private:
    Bus* bus;

    uint16_t mode_cycles = 0;
    uint8_t scx_latch = 0;
    uint8_t mode3_dur = 0;
    uint8_t lx = 0;
    uint8_t fetch_x = 0;
    bool window_render = false;
    int window_counter = 0;


    uint8_t bg_fifo_color[8];
    uint8_t fifo_size = 0;
    void render_single_dot();
    bool stat_line = false;

};
