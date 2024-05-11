
#include <memory>
#include "pacman.hpp"

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Weffc++"
#include "playfield.hpp"
#pragma GCC diagnostic pop

std::unique_ptr<Playfield> _playfield;

void initialize(const standalone_application_api_t& api) {
    _api = &api;
}

void on_event(const uint32_t& events) {
    if (!_playfield) {
        _playfield = std::make_unique<Playfield>();
        _playfield->Init();
    }

    if (events & 1 && _playfield) {
        _playfield->Step();
    }
}

void shutdown() {
    _playfield.reset();
}
