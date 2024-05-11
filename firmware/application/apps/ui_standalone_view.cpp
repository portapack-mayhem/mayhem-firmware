#include "ui_standalone_view.hpp"
#include "irq_controls.hpp"

namespace ui {

void create_thread(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority) {
    // TODO: collect memory on terminate
    chThdCreateFromHeap(NULL, stack_size, priority, fn, arg);
}

void fill_rectangle(int x, int y, int width, int height, uint16_t color) {
    ui::Painter painter;
    painter.fill_rectangle({x, y, width, height}, ui::Color(color));
}

void* alloc(size_t size) {
    void* p = chHeapAlloc(0x0, size);
    if (p == nullptr)
        chDbgPanic("Out of Memory");
    return p;
}

standalone_application_api_t api = {
    /* .malloc = */ &alloc,
    /* .calloc = */ &calloc,
    /* .realloc = */ &realloc,
    /* .free = */ &chHeapFree,
    /* .create_thread = */ &create_thread,
    /* .fill_rectangle = */ &fill_rectangle,
};

StandaloneView::StandaloneView(NavigationView& nav, std::unique_ptr<uint8_t[]> app_image)
    : nav_(nav), _app_image(std::move(app_image)) {
    get_application_information()->initialize(api);
    add_children({&dummy});
}

void StandaloneView::focus() {
    dummy.focus();
}

void StandaloneView::paint(Painter& painter) {
    (void)painter;
    // static bool wait_for_button_release{false};

    // auto switches_raw = swizzled_switches() & ((1 << (int)Switch::Right) | (1 << (int)Switch::Left) | (1 << (int)Switch::Down) | (1 << (int)Switch::Up) | (1 << (int)Switch::Sel) | (1 << (int)Switch::Dfu));
    // auto switches_debounced = get_switches_state().to_ulong();

    // // For the Select (Start/Pause) button, wait for release to avoid repeat
    // uint8_t buttons_to_wait_for = (1 << (int)Switch::Sel);
    // if (wait_for_button_release) {
    //     if ((switches_debounced & buttons_to_wait_for) == 0)
    //         wait_for_button_release = false;
    //     switches_debounced &= ~buttons_to_wait_for;
    // } else {
    //     if (switches_debounced & buttons_to_wait_for)
    //         wait_for_button_release = true;
    // }

    // // For the directional buttons, use the raw inputs for fastest response time
    // but_RIGHT = (switches_raw & (1 << (int)Switch::Right)) != 0;
    // but_LEFT = (switches_raw & (1 << (int)Switch::Left)) != 0;
    // but_DOWN = (switches_raw & (1 << (int)Switch::Down)) != 0;
    // but_UP = (switches_raw & (1 << (int)Switch::Up)) != 0;

    // // For the pause button, use the debounced input to avoid glitches, and OR in the value to make sure that we don't clear it before it's seen
    // but_A |= (switches_debounced & (1 << (int)Switch::Sel)) != 0;

    // _game.Step();
}

void StandaloneView::frame_sync() {
    if (initialized) {
        get_application_information()->on_event(1);
    } else {
        initialized = true;
    }
}

}  // namespace ui
