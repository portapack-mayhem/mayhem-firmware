#include "ui_rf3d.hpp"
#include <cmath>
#include <cstdlib>

namespace ui::external_app::rf3d {

RF3DView::RF3DView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
    spectrum_data.resize(240, std::vector<uint8_t>(64, 0));
}

void RF3DView::on_show() {
    if (!initialized) {
        initialized = true;
        baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
        start();
    }
}

void RF3DView::start() {
    if (!running) {
        baseband::spectrum_streaming_start();
        running = true;
    }
}

void RF3DView::stop() {
    if (running) {
        baseband::spectrum_streaming_stop();
        running = false;
    }
}

void RF3DView::update_spectrum() {
    for (int x = 0; x < 240; x++) {
        for (int z = 63; z > 0; z--) {
            spectrum_data[x][z] = spectrum_data[x][z - 1];
        }
        spectrum_data[x][0] = rand() % 256;
    }
}

void RF3DView::render_3d_waterfall(Painter& painter) {
    painter.fill_rectangle({0, 0, 240, 320}, Color::black());
    
    float fov = 60.0f;
    float half_fov = fov * 0.5f * 3.14159f / 180.0f;
    float aspect = 240.0f / 320.0f;
    float near = 1.0f;
    float far = 64.0f;

    angle += 0.01f;
    if (angle > 6.28f) angle -= 6.28f;

    for (int x = 0; x < 240; x++) {
        float ray_angle = (x - 120) * (half_fov / 120.0f) + angle;
        for (int z = 0; z < 64; z++) {
            float depth = z + near;
            float proj_x = (x - 120) * aspect / (depth * tan(half_fov));
            float proj_y = spectrum_data[x][z] / 255.0f * 100.0f / (depth * tan(half_fov));
            int screen_x = (proj_x + 1.0f) * 120;
            int screen_y = (1.0f - proj_y) * 160;

            if (screen_x >= 0 && screen_x < 240 && screen_y >= 0 && screen_y < 320) {
                int height = spectrum_data[x][z] * 200 / (z + 1);
                int top = 160 - height / 2;
                int bottom = 160 + height / 2;
                if (top < 0) top = 0;
                if (bottom > 320) bottom = 320;
                
                uint8_t r = spectrum_data[x][z];
                uint8_t g = 255 - spectrum_data[x][z];
                uint8_t b = 0;
                painter.fill_rectangle({screen_x, top, 1, bottom - top}, Color(r, g, b));
            }
        }
    }
}

void RF3DView::paint(Painter& painter) {
    if (!initialized) {
        initialized = true;
        baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
        start();
    }
    render_3d_waterfall(painter);
}

void RF3DView::frame_sync() {
    if (running) {
        update_spectrum();
        set_dirty();
    }
}

} // namespace ui::external_app::rf3d