/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_doom.hpp"
#include "ui.hpp"

namespace ui::external_app::doom {

//clang-format off
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define RENDER_HEIGHT 280
#define HALF_WIDTH (SCREEN_WIDTH / 2)
#define HALF_HEIGHT (RENDER_HEIGHT / 2)
#define LEVEL_WIDTH_BASE 6
#define LEVEL_WIDTH (1 << LEVEL_WIDTH_BASE)
#define LEVEL_HEIGHT 57
#define LEVEL_SIZE (LEVEL_WIDTH / 2 * LEVEL_HEIGHT)
#define MAX_RENDER_DEPTH 12
#define MIN_ENTITIES 15
#define MAX_ENTITIES 25
#define RES_DIVIDER 2
#define DISTANCE_MULTIPLIER 20
#define ROT_SPEED 0.25
#define MOV_SPEED 1.0
#define JOGGING_SPEED 1.2
#define MAX_ENTITY_DISTANCE 200
#define ITEM_COLLIDER_DIST 6
#define PI 3.14159265358979323846
#define GUN_WIDTH 30
#define GUN_HEIGHT 40
#define GUN_TARGET_POS 24
#define GUN_SHOT_POS (GUN_TARGET_POS + 6)
#define ENEMY_SIZE 16
#define ENEMY_SPEED 0.03
#define ENEMY_MELEE_DIST 20
#define ENEMY_MELEE_DAMAGE 2
#define GUN_MAX_DAMAGE 15

#define ACTIVATION_RADIUS 120
#define STATE_INACTIVE 6

#define COLOR_BACKGROUND Black
#define COLOR_WALL_LIGHT Green
#define COLOR_WALL_DARK Maroon
#define COLOR_FLOOR_DARK Black
//clang-format on

// Stole this level map from Flipper Zero Doom, but he stole it from a guy who stole it and that guy probably stole it too. Argh!
static const uint8_t level[LEVEL_SIZE] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x02, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x20, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x90, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xF2, 0x00, 0x00, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x4F, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0x40, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x20, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0x05, 0x00, 0x00, 0x90, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0xFF,
    0xF0, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x00, 0x02, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0x00, 0x08, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF2, 0x02, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0x40, 0x80, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x40, 0x00, 0x02, 0x00, 0x90, 0xFF, 0xFF,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xF0, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x50, 0x00, 0x20, 0x00, 0x7F, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x0F, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x40, 0x00, 0x40, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct Coords {
    double x, y;
};

struct Player {
    Coords pos;
    Coords dir;
    Coords plane;
    double velocity;
    uint8_t health;
    uint8_t keys;
    uint8_t ammo;
};

struct Entity {
    uint16_t uid;
    Coords pos;
    Coords death_pos;
    uint8_t state;
    uint8_t health;
    uint8_t distance;
    uint8_t timer;
};

static Ticker game_timer;
static Player player;
static Entity entities[MAX_ENTITIES];
static uint8_t num_entities = 0;
static uint16_t kills = 0;
static uint8_t scene = 0;
static bool up, down, left, right, fired;
static double jogging, view_height;
static bool needs_redraw = false;
static bool needs_gun_redraw = false;
static uint8_t gun_pos = GUN_TARGET_POS;
static bool gun_fired = false;
static int prev_gun_x = 0;
static int prev_gun_y = 0;

Coords create_coords(double x, double y) {
    Coords c;
    c.x = x;
    c.y = y;
    return c;
}

Player create_player(double x, double y) {
    Player p;
    p.pos = create_coords(x + 0.5, y + 0.5);
    p.dir = create_coords(1, 0);
    p.plane = create_coords(0, -0.66);
    p.velocity = 0;
    p.health = 100;
    p.keys = 0;
    p.ammo = 99;
    return p;
}

Entity create_entity(uint8_t type, uint8_t x, uint8_t y, uint8_t state, uint8_t health) {
    Entity e;
    e.uid = ((y << 6) | x) << 4 | type;
    e.pos = create_coords(x + 0.5, y + 0.5);
    e.death_pos = create_coords(x + 0.5, y + 0.5);
    e.state = state;
    e.health = health;
    e.distance = 0;
    e.timer = 0;
    return e;
}

uint8_t get_block_at(uint8_t x, uint8_t y) {
    if (x >= LEVEL_WIDTH || y >= LEVEL_HEIGHT) return 0xF;
    return level[((LEVEL_HEIGHT - 1 - y) * LEVEL_WIDTH + x) / 2] >> (!(x % 2) * 4) & 0b1111;
}

Coords translate_into_view(Coords pos) {
    double sprite_x = pos.x - player.pos.x;
    double sprite_y = pos.y - player.pos.y;
    double inv_det = 1.0 / (player.plane.x * player.dir.y - player.dir.x * player.plane.y);
    double transform_x = inv_det * (player.dir.y * sprite_x - player.dir.x * sprite_y);
    double transform_y = inv_det * (-player.plane.y * sprite_x + player.plane.x * sprite_y);
    return create_coords(transform_x, transform_y);
}

void spawn_entity(uint8_t type, uint8_t x, uint8_t y) {
    if (num_entities >= MAX_ENTITIES) return;
    entities[num_entities] = create_entity(type, x, y, STATE_INACTIVE, 50);
    num_entities++;
}

void spawn_random_entity(uint8_t type) {
    if (num_entities >= MAX_ENTITIES) return;
    std::srand(LPC_RTC->CTIME0);
    uint8_t spawn_x, spawn_y;
    do {
        spawn_x = std::rand() % LEVEL_WIDTH;
        spawn_y = std::rand() % LEVEL_HEIGHT;
    } while (get_block_at(spawn_x, spawn_y) == 0xF ||
             (spawn_x == (uint8_t)player.pos.x && spawn_y == (uint8_t)player.pos.y));

    entities[num_entities] = create_entity(type, spawn_x, spawn_y, STATE_INACTIVE, 50);
    num_entities++;
}

void remove_entity(uint8_t index) {
    for (uint8_t i = index; i < num_entities - 1; i++) {
        entities[i] = entities[i + 1];
    }
    num_entities--;

    if (num_entities < MIN_ENTITIES) {
        uint8_t attempts = 0;
        bool spawned = false;

        while (!spawned && attempts < 50) {
            std::srand(LPC_RTC->CTIME0 + attempts);
            uint8_t spawn_x = std::rand() % LEVEL_WIDTH;
            uint8_t spawn_y = std::rand() % LEVEL_HEIGHT;

            int16_t dx = (int16_t)spawn_x - (int16_t)player.pos.x;
            int16_t dy = (int16_t)spawn_y - (int16_t)player.pos.y;
            uint16_t dist_squared = dx * dx + dy * dy;

            if (dist_squared >= 64 &&
                get_block_at(spawn_x, spawn_y) != 0xF &&
                !(spawn_x == (uint8_t)player.pos.x && spawn_y == (uint8_t)player.pos.y)) {
                entities[num_entities] = create_entity(0x2, spawn_x, spawn_y, STATE_INACTIVE, 50);
                entities[num_entities].pos.x = (double)spawn_x + 0.5;
                entities[num_entities].pos.y = (double)spawn_y + 0.5;
                entities[num_entities].death_pos.x = (double)spawn_x + 0.5;
                entities[num_entities].death_pos.y = (double)spawn_y + 0.5;

                if (get_block_at((uint8_t)entities[num_entities].pos.x, (uint8_t)entities[num_entities].pos.y) != 0xF) {
                    num_entities++;
                    spawned = true;
                }
            }
            attempts++;
        }
    }
}

void initialize_level() {
    for (uint8_t y = LEVEL_HEIGHT - 1; y > 0; y--) {
        for (uint8_t x = 0; x < LEVEL_WIDTH; x++) {
            uint8_t block = get_block_at(x, y);
            if (block == 0x1) {
                player = create_player(x, y);
                break;
            }
        }
    }
    num_entities = 0;

    uint8_t initial_enemies = 0;
    uint8_t attempts = 0;

    while (initial_enemies < 2 && num_entities < MAX_ENTITIES && attempts < 50) {
        std::srand(LPC_RTC->CTIME0 + attempts);
        int8_t offset_x = (std::rand() % 11) - 5;
        int8_t offset_y = (std::rand() % 11) - 5;

        if (abs(offset_x) < 3 && abs(offset_y) < 3) {
            attempts++;
            continue;
        }

        int16_t temp_x = (int16_t)player.pos.x + offset_x;
        int16_t temp_y = (int16_t)player.pos.y + offset_y;
        uint8_t spawn_x = (temp_x < 0) ? 0 : (temp_x >= LEVEL_WIDTH) ? LEVEL_WIDTH - 1
                                                                     : temp_x;
        uint8_t spawn_y = (temp_y < 0) ? 0 : (temp_y >= LEVEL_HEIGHT) ? LEVEL_HEIGHT - 1
                                                                      : temp_y;

        if (get_block_at(spawn_x, spawn_y) != 0xF &&
            !(spawn_x == (uint8_t)player.pos.x && spawn_y == (uint8_t)player.pos.y)) {
            entities[num_entities] = create_entity(0x2, spawn_x, spawn_y, STATE_INACTIVE, 50);
            entities[num_entities].pos.x = (double)spawn_x + 0.5;
            entities[num_entities].pos.y = (double)spawn_y + 0.5;
            entities[num_entities].death_pos.x = (double)spawn_x + 0.5;
            entities[num_entities].death_pos.y = (double)spawn_y + 0.5;

            if (get_block_at((uint8_t)entities[num_entities].pos.x, (uint8_t)entities[num_entities].pos.y) == 0xF) {
                attempts++;
                continue;
            }

            num_entities++;
            initial_enemies++;
        }
        attempts++;
    }

    attempts = 0;
    while (num_entities < MIN_ENTITIES && attempts < 100) {
        std::srand(LPC_RTC->CTIME0 + attempts + 100);

        uint8_t spawn_x = std::rand() % LEVEL_WIDTH;
        uint8_t spawn_y = std::rand() % LEVEL_HEIGHT;

        int16_t dx = (int16_t)spawn_x - (int16_t)player.pos.x;
        int16_t dy = (int16_t)spawn_y - (int16_t)player.pos.y;
        uint16_t dist_squared = dx * dx + dy * dy;

        if (dist_squared >= 25 &&
            get_block_at(spawn_x, spawn_y) != 0xF &&
            !(spawn_x == (uint8_t)player.pos.x && spawn_y == (uint8_t)player.pos.y)) {
            entities[num_entities] = create_entity(0x2, spawn_x, spawn_y, STATE_INACTIVE, 50);
            entities[num_entities].pos.x = (double)spawn_x + 0.5;
            entities[num_entities].pos.y = (double)spawn_y + 0.5;
            entities[num_entities].death_pos.x = (double)spawn_x + 0.5;
            entities[num_entities].death_pos.y = (double)spawn_y + 0.5;

            if (get_block_at((uint8_t)entities[num_entities].pos.x, (uint8_t)entities[num_entities].pos.y) == 0xF) {
                attempts++;
                continue;
            }

            num_entities++;
        }
        attempts++;
    }
}

void fire() {
    if (player.ammo > 0) {
        uint8_t closest_i = 255;
        double min_dist = 1000.0;
        for (uint8_t i = 0; i < num_entities; i++) {
            if (entities[i].state == 5) continue;
            double dx = entities[i].pos.x - player.pos.x;
            double dy = entities[i].pos.y - player.pos.y;
            double dist = sqrt(dx * dx + dy * dy) * DISTANCE_MULTIPLIER;
            double dot = dx * player.dir.x + dy * player.dir.y;
            double angle = acos(dot / (dist / DISTANCE_MULTIPLIER));
            if (dist < 100 && angle < 0.2 && dot > 0 && dist < min_dist) {
                closest_i = i;
                min_dist = dist;
            }
        }
        if (closest_i != 255) {
            uint8_t damage = fmin(GUN_MAX_DAMAGE, GUN_MAX_DAMAGE * (100 - min_dist) / 100);
            if (damage > 0) {
                entities[closest_i].health = fmax(0, entities[closest_i].health - damage);
                entities[closest_i].state = 4;
                entities[closest_i].timer = 4;
                needs_redraw = true;
            }
        }
        player.ammo--;
    }
}

void update_entities() {
    uint8_t i = 0;
    while (i < num_entities) {
        entities[i].distance = sqrt(pow(player.pos.x - entities[i].pos.x, 2) + pow(player.pos.y - entities[i].pos.y, 2)) * DISTANCE_MULTIPLIER;

        if (entities[i].state != 5) {
            if (entities[i].distance > ACTIVATION_RADIUS && entities[i].state != STATE_INACTIVE) {
                entities[i].state = STATE_INACTIVE;
                needs_redraw = true;
            } else if (entities[i].distance <= ACTIVATION_RADIUS && entities[i].state == STATE_INACTIVE) {
                entities[i].state = 0;
                needs_redraw = true;
            }
        }

        if (entities[i].timer > 0) entities[i].timer--;

        if (entities[i].health == 0 && entities[i].state == 5 && entities[i].timer == 0) {
            kills++;
            remove_entity(i);
            continue;
        }
        if (entities[i].health == 0 && entities[i].state != 5) {
            entities[i].state = 5;
            entities[i].timer = 6;
            entities[i].death_pos = entities[i].pos;
            needs_redraw = true;
        }
        if (entities[i].state == 4 && entities[i].timer == 0) {
            entities[i].state = 0;
            needs_redraw = true;
        }

        if (entities[i].state != STATE_INACTIVE && entities[i].state != 4 && entities[i].state != 5) {
            double dx = player.pos.x - entities[i].pos.x;
            double dy = player.pos.y - entities[i].pos.y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist <= (ENEMY_MELEE_DIST / DISTANCE_MULTIPLIER) && entities[i].timer == 0) {
                entities[i].state = 3;
                player.health = fmax(0, player.health - ENEMY_MELEE_DAMAGE);
                entities[i].timer = 30;
                needs_redraw = true;
            }

            double move_dist = fmin(ENEMY_SPEED, dist - 0.2);

            bool moved = false;

            double new_x = entities[i].pos.x + (dx / dist) * move_dist;
            double new_y = entities[i].pos.y + (dy / dist) * move_dist;
            if (get_block_at((uint8_t)new_x, (uint8_t)entities[i].pos.y) != 0xF) {
                entities[i].pos.x = new_x;
                moved = true;
            }
            if (get_block_at((uint8_t)entities[i].pos.x, (uint8_t)new_y) != 0xF) {
                entities[i].pos.y = new_y;
                moved = true;
            }

            if (!moved) {
                bool wall_x_pos = get_block_at((uint8_t)entities[i].pos.x + 1, (uint8_t)entities[i].pos.y) == 0xF;
                bool wall_x_neg = get_block_at((uint8_t)entities[i].pos.x - 1, (uint8_t)entities[i].pos.y) == 0xF;
                bool wall_y_pos = get_block_at((uint8_t)entities[i].pos.x, (uint8_t)entities[i].pos.y + 1) == 0xF;
                bool wall_y_neg = get_block_at((uint8_t)entities[i].pos.x, (uint8_t)entities[i].pos.y - 1) == 0xF;

                uint8_t dir_choice = (entities[i].uid + LPC_RTC->CTIME0) % 4;

                if (wall_x_pos || wall_x_neg) {
                    double try_y = entities[i].pos.y + (dir_choice < 2 ? 1 : -1) * move_dist;
                    if (get_block_at((uint8_t)entities[i].pos.x, (uint8_t)try_y) != 0xF) {
                        entities[i].pos.y = try_y;
                        moved = true;
                    }
                } else if (wall_y_pos || wall_y_neg) {
                    double try_x = entities[i].pos.x + (dir_choice < 2 ? 1 : -1) * move_dist;
                    if (get_block_at((uint8_t)try_x, (uint8_t)entities[i].pos.y) != 0xF) {
                        entities[i].pos.x = try_x;
                        moved = true;
                    }
                }

                if (!moved) {
                    double angle = (entities[i].uid * 17 + LPC_RTC->CTIME0) % 628 / 100.0;
                    double try_x = entities[i].pos.x + cos(angle) * move_dist;
                    double try_y = entities[i].pos.y + sin(angle) * move_dist;

                    if (get_block_at((uint8_t)try_x, (uint8_t)entities[i].pos.y) != 0xF) {
                        entities[i].pos.x = try_x;
                        moved = true;
                    }
                    if (get_block_at((uint8_t)entities[i].pos.x, (uint8_t)try_y) != 0xF) {
                        entities[i].pos.y = try_y;
                        moved = true;
                    }
                }
            }

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    if (get_block_at((uint8_t)(entities[i].pos.x + dx * 0.3), (uint8_t)(entities[i].pos.y + dy * 0.3)) == 0xF) {
                        entities[i].pos.x -= dx * 0.05;
                        entities[i].pos.y -= dy * 0.05;
                    }
                }
            }

            if (dist > (ENEMY_MELEE_DIST / DISTANCE_MULTIPLIER)) entities[i].state = 0;
        }
        i++;
    }
}

void update_game() {
    bool state_changed = false;
    bool gun_only_change = false;

    if (scene == 1) {
        if (player.health > 0) {
            if (up) {
                player.velocity += (MOV_SPEED - player.velocity) * 0.4;
                jogging = fabs(player.velocity) * 5;
                up = false;
                state_changed = true;
            } else if (down) {
                player.velocity += (-MOV_SPEED - player.velocity) * 0.4;
                jogging = fabs(player.velocity) * 5;
                down = false;
                state_changed = true;
            } else {
                double old_velocity = player.velocity;
                player.velocity *= 0.5;
                jogging = fabs(player.velocity) * 5;
                if (fabs(player.velocity) < 0.001) player.velocity = 0;
                if (old_velocity != player.velocity) state_changed = true;
            }

            if (right) {
                double old_dir_x = player.dir.x;
                player.dir.x = player.dir.x * cos(-ROT_SPEED) - player.dir.y * sin(-ROT_SPEED);
                player.dir.y = old_dir_x * sin(-ROT_SPEED) + player.dir.y * cos(-ROT_SPEED);
                double old_plane_x = player.plane.x;
                player.plane.x = player.plane.x * cos(-ROT_SPEED) - player.plane.y * sin(-ROT_SPEED);
                player.plane.y = old_plane_x * sin(-ROT_SPEED) + player.plane.y * cos(-ROT_SPEED);
                right = false;
                state_changed = true;
            } else if (left) {
                double old_dir_x = player.dir.x;
                player.dir.x = player.dir.x * cos(ROT_SPEED) - player.dir.y * sin(ROT_SPEED);
                player.dir.y = old_dir_x * sin(ROT_SPEED) + player.dir.y * cos(ROT_SPEED);
                double old_plane_x = player.plane.x;
                player.plane.x = player.plane.x * cos(ROT_SPEED) - player.plane.y * sin(ROT_SPEED);
                player.plane.y = old_plane_x * sin(ROT_SPEED) + player.plane.y * cos(ROT_SPEED);
                left = false;
                state_changed = true;
            }

            if (player.velocity != 0) {
                view_height = fabs(sin(LPC_RTC->CTIME0 * JOGGING_SPEED)) * 6 * jogging;
            } else {
                view_height = 0;
            }

            if (fabs(player.velocity) > 0.003) {
                double new_x = player.pos.x + player.dir.x * player.velocity;
                double new_y = player.pos.y + player.dir.y * player.velocity;
                bool can_move_x = get_block_at((uint8_t)new_x, (uint8_t)player.pos.y) != 0xF;
                bool can_move_y = get_block_at((uint8_t)player.pos.x, (uint8_t)new_y) != 0xF;
                for (uint8_t i = 0; i < num_entities; i++) {
                    if (entities[i].state != 5 && entities[i].state != STATE_INACTIVE) {
                        double dx = new_x - entities[i].pos.x;
                        double dy = player.pos.y - entities[i].pos.y;
                        if (sqrt(dx * dx + dy * dy) < 0.5) {
                            can_move_x = false;
                        }
                        dx = player.pos.x - entities[i].pos.x;
                        dy = new_y - entities[i].pos.y;
                        if (sqrt(dx * dx + dy * dy) < 0.5) {
                            can_move_y = false;
                        }
                    }
                }
                if (can_move_x) {
                    player.pos.x = new_x;
                    state_changed = true;
                }
                if (can_move_y) {
                    player.pos.y = new_y;
                    state_changed = true;
                }
            } else if (player.velocity != 0) {
                player.velocity = 0;
                state_changed = true;
            }

            if (gun_pos > GUN_TARGET_POS) {
                gun_pos -= 1;
                gun_only_change = true;
            } else if (gun_pos < GUN_TARGET_POS) {
                gun_pos += 2;
                gun_only_change = true;
            } else if (fired) {
                gun_pos = GUN_SHOT_POS;
                fire();
                needs_gun_redraw = true;
                gun_only_change = true;
                fired = false;
            }
        } else {
            if (view_height > -10) {
                view_height--;
                state_changed = true;
            }
            if (gun_pos > 1) {
                gun_pos -= 2;
                gun_only_change = true;
            }
        }
        update_entities();
    }

    if (state_changed && !gun_only_change)
        needs_redraw = true;
    else if (gun_only_change)
        needs_gun_redraw = true;
}

static void game_timer_check() {
    if (scene == 1) update_game();
}

void render_gun(Painter& painter, uint8_t gun_pos, double jogging) {
    int x = HALF_WIDTH - GUN_WIDTH / 2 + sin(LPC_RTC->CTIME0 * JOGGING_SPEED) * 10 * jogging;
    int y = RENDER_HEIGHT - gun_pos - 29 + fabs(cos(LPC_RTC->CTIME0 * JOGGING_SPEED)) * 8 * jogging;

    prev_gun_x = x;
    prev_gun_y = y;

    int recoil = gun_pos > GUN_TARGET_POS ? (gun_pos - GUN_TARGET_POS) / 2 : 0;

    // Gun body - shotgun style with more detail
    painter.fill_rectangle({x + 8, y + 8, 18, 12}, pp_colors[Green]);
    painter.fill_rectangle({x + 6, y + 10, 2, 8}, pp_colors[Black]);
    painter.fill_rectangle({x + 26, y + 10, 2, 8}, pp_colors[Black]);

    // Barrel
    painter.fill_rectangle({x + 12, y - 4, 8, 12}, pp_colors[Green]);
    painter.fill_rectangle({x + 14, y - 6, 4, 2}, pp_colors[Green]);

    // Barrel shading
    painter.fill_rectangle({x + 12, y - 4, 2, 12}, pp_colors[White]);
    painter.fill_rectangle({x + 18, y - 4, 2, 12}, pp_colors[Black]);

    // Handle
    painter.fill_rectangle({x + 12, y + 20, 8, 20}, pp_colors[Maroon]);
    painter.fill_rectangle({x + 10, y + 20, 2, 18}, pp_colors[White]);
    painter.fill_rectangle({x + 20, y + 20, 2, 18}, pp_colors[Black]);

    // Trigger guard
    painter.fill_rectangle({x + 10, y + 20, 12, 2}, pp_colors[Green]);
    painter.fill_rectangle({x + 10, y + 20, 2, 6}, pp_colors[Green]);
    painter.fill_rectangle({x + 20, y + 20, 2, 6}, pp_colors[Green]);
    painter.fill_rectangle({x + 10, y + 26, 12, 2}, pp_colors[Green]);

    // Trigger
    painter.fill_rectangle({x + 14, y + 22 + recoil / 2, 4, 4}, pp_colors[Black]);

    // Pump action
    painter.fill_rectangle({x + 8, y + 4, 16, 4}, pp_colors[Maroon]);
    painter.fill_rectangle({x + 7, y + 4, 1, 4}, pp_colors[White]);
    painter.fill_rectangle({x + 24, y + 4, 1, 4}, pp_colors[Black]);

    // Action slide rails
    painter.fill_rectangle({x + 10, y + 8, 1, 8 + recoil}, pp_colors[White]);
    painter.fill_rectangle({x + 21, y + 8, 1, 8 + recoil}, pp_colors[Black]);

    // Ammo chamber
    painter.fill_rectangle({x + 24, y + 12, 4, 4}, pp_colors[Black]);

    // Muzzle flash
    if (gun_pos > GUN_TARGET_POS) {
        int flash_size = 10 + (gun_pos - GUN_TARGET_POS) * 2;
        int flash_center_x = x + 16;
        int flash_center_y = y - 6;

        // Core of flash
        painter.fill_rectangle({flash_center_x - flash_size / 4, flash_center_y - flash_size / 2,
                                flash_size / 2, flash_size},
                               pp_colors[White]);

        // Outer yellow glow
        painter.fill_rectangle({flash_center_x - flash_size / 2, flash_center_y - flash_size / 4,
                                flash_size, flash_size / 2},
                               pp_colors[Yellow]);

        // Left spike
        painter.fill_rectangle({flash_center_x - flash_size, flash_center_y - 2,
                                flash_size / 2, 4},
                               pp_colors[Yellow]);

        // Right spike
        painter.fill_rectangle({flash_center_x + flash_size / 2, flash_center_y - 2,
                                flash_size / 2, 4},
                               pp_colors[Yellow]);

        // Top spike
        painter.fill_rectangle({flash_center_x - 2, flash_center_y - flash_size,
                                4, flash_size / 2},
                               pp_colors[Yellow]);

        // Small random sparks
        if (LPC_RTC->CTIME0 % 2) {
            painter.fill_rectangle({flash_center_x - flash_size / 2 - 2, flash_center_y - flash_size / 2 - 2,
                                    2, 2},
                                   pp_colors[Yellow]);
            painter.fill_rectangle({flash_center_x + flash_size / 3, flash_center_y - flash_size / 3,
                                    2, 2},
                                   pp_colors[Yellow]);
        } else {
            painter.fill_rectangle({flash_center_x + flash_size / 3, flash_center_y - flash_size / 2,
                                    2, 2},
                                   pp_colors[Yellow]);
            painter.fill_rectangle({flash_center_x - flash_size / 4, flash_center_y + flash_size / 4,
                                    2, 2},
                                   pp_colors[Yellow]);
        }

        // Smoke effect
        if (gun_pos > GUN_TARGET_POS + 2) {
            int smoke_y = flash_center_y - flash_size - 4;
            painter.fill_rectangle({flash_center_x - 6, smoke_y, 12, 4}, pp_colors[White]);
            painter.fill_rectangle({flash_center_x - 8, smoke_y - 4, 16, 4}, pp_colors[White]);
            painter.fill_rectangle({flash_center_x - 6, smoke_y - 8, 12, 4}, pp_colors[White]);
        }
    }

    // Shell ejection
    if (gun_pos > GUN_TARGET_POS && gun_pos < GUN_TARGET_POS + 6) {
        int shell_x = x + 24 + (gun_pos - GUN_TARGET_POS) * 2;
        int shell_y = y + 10 - (gun_pos - GUN_TARGET_POS);

        painter.fill_rectangle({shell_x, shell_y, 4, 2}, pp_colors[Yellow]);
        painter.fill_rectangle({shell_x + 1, shell_y - 1, 2, 4}, pp_colors[Yellow]);
    }
}

void render_entities(Painter& painter) {
    for (uint8_t i = 0; i < num_entities; i++) {
        if (entities[i].state == STATE_INACTIVE) continue;
        Coords transform = translate_into_view(entities[i].state == 5 ? entities[i].death_pos : entities[i].pos);
        if (transform.y <= 0.1 || transform.y > MAX_RENDER_DEPTH) continue;

        int16_t sprite_x = HALF_WIDTH * (1.0 + transform.x / transform.y);
        int16_t sprite_y = HALF_HEIGHT + view_height / transform.y;
        uint8_t base_size = ENEMY_SIZE / fmax(0.1, transform.y);

        if (sprite_x < -base_size || sprite_x > SCREEN_WIDTH + base_size || sprite_y < 0 || sprite_y > RENDER_HEIGHT) continue;

        uint8_t size = base_size * 2;
        int16_t half_size = size / 2;
        int16_t x_start = sprite_x - half_size;
        int16_t y_start = sprite_y - half_size;

        Color body_color = Color(180, 40, 20);
        Color spine_color = Color(120, 20, 10);
        Color eye_color = Color(255, 165, 0);
        Color pupil_color = Color(255, 0, 0);
        Color claw_color = Color(60, 60, 60);
        Color teeth_color = Color(220, 220, 220);
        Color blood_color = Color(150, 0, 0);
        Color horn_color = Color(90, 30, 10);

        if (entities[i].state == 5) {
            // Death animation - gibbed/flattened demon
            uint8_t death_phase = entities[i].timer / 5;
            if (death_phase > 3) death_phase = 3;

            if (death_phase == 0) {
                // Initial death - falling apart
                painter.fill_rectangle({x_start + size / 5, sprite_y, size * 3 / 5, size / 3}, body_color);
                painter.fill_rectangle({x_start + size * 2 / 5, sprite_y - size / 6, size / 5, size / 4}, horn_color);
                painter.fill_rectangle({x_start + size / 3, sprite_y + size / 8, size / 8, size / 6}, blood_color);
                painter.fill_rectangle({x_start + size / 2, sprite_y + size / 6, size / 6, size / 8}, blood_color);
                painter.fill_rectangle({x_start + size / 4, sprite_y + size / 4, size / 5, size / 10}, claw_color);
                painter.fill_rectangle({x_start + size * 3 / 5, sprite_y + size / 5, size / 5, size / 10}, claw_color);
            } else if (death_phase == 1) {
                // More splattered
                painter.fill_rectangle({x_start + size / 6, sprite_y, size * 2 / 3, size / 4}, body_color);
                painter.fill_rectangle({x_start + size / 4, sprite_y + size / 8, size / 6, size / 6}, blood_color);
                painter.fill_rectangle({x_start + size / 2, sprite_y + size / 10, size / 6, size / 8}, blood_color);
                painter.fill_rectangle({x_start + size * 2 / 3, sprite_y + size / 12, size / 8, size / 8}, blood_color);
            } else {
                // Final gore puddle
                painter.fill_rectangle({x_start + size / 8, sprite_y, size * 3 / 4, size / 5}, body_color);
                painter.fill_rectangle({x_start + size / 6, sprite_y - size / 20, size * 2 / 3, size / 10}, blood_color);
                painter.fill_rectangle({x_start + size / 4, sprite_y + size / 10, size / 5, size / 12}, blood_color);
                painter.fill_rectangle({x_start + size / 2, sprite_y + size / 12, size / 4, size / 10}, blood_color);
            }
        } else {
            // Body
            painter.fill_rectangle({x_start + size / 3, y_start + size / 4, size / 3, size * 3 / 4}, body_color);

            // Spine/backbone
            painter.fill_rectangle({x_start + size * 5 / 12, y_start + size / 6, size / 6, size / 2}, spine_color);

            // Head
            painter.fill_rectangle({x_start + size * 3 / 8, y_start, size * 5 / 16, size / 3}, body_color);

            // Horns
            painter.fill_rectangle({x_start + size / 4, y_start - size / 6, size / 8, size / 5}, horn_color);
            painter.fill_rectangle({x_start + size * 5 / 8, y_start - size / 6, size / 8, size / 5}, horn_color);

            // Eyes
            if (entities[i].state != 4) {
                uint8_t eye_anim = (LPC_RTC->CTIME0 % 8 == 0) ? 1 : 0;

                painter.fill_rectangle({x_start + size * 5 / 12, y_start + size / 12, size / 6, size / 6 - eye_anim}, eye_color);
                painter.fill_rectangle({x_start + size * 7 / 16, y_start + size / 8, size / 20, size / 20}, pupil_color);

                painter.fill_rectangle({x_start + size * 7 / 16, y_start + size / 12, size / 6, size / 6 - eye_anim}, eye_color);
                painter.fill_rectangle({x_start + size * 17 / 32, y_start + size / 8, size / 20, size / 20}, pupil_color);
            } else {
                // Damaged eye
                painter.fill_rectangle({x_start + size * 5 / 12, y_start + size / 12, size / 6, size / 6}, eye_color);
                painter.fill_rectangle({x_start + size * 7 / 16, y_start + size / 8, size / 20, size / 20}, pupil_color);

                // Blood from damaged eye
                painter.fill_rectangle({x_start + size * 7 / 16, y_start + size / 6, size / 10, size / 8}, blood_color);
                painter.fill_rectangle({x_start + size / 2, y_start + size / 3, size / 12, size / 12}, blood_color);
            }

            // Teeth/mouth
            painter.fill_rectangle({x_start + size * 7 / 16, y_start + size * 5 / 16, size / 20, size / 16}, teeth_color);
            painter.fill_rectangle({x_start + size / 2, y_start + size * 5 / 16, size / 20, size / 16}, teeth_color);
            painter.fill_rectangle({x_start + size * 9 / 16, y_start + size * 5 / 16, size / 20, size / 16}, teeth_color);

            // Attacking state - claws extended
            if (entities[i].state == 3) {
                uint8_t claw_anim = (LPC_RTC->CTIME0 % 4) / 2;

                if (claw_anim == 0) {
                    // Claws forward
                    painter.fill_rectangle({x_start - size / 8, y_start + size * 5 / 12, size / 3, size / 6}, claw_color);
                    painter.fill_rectangle({x_start - size / 12, y_start + size / 2, size / 5, size / 4}, claw_color);
                    painter.fill_rectangle({x_start + size * 3 / 4, y_start + size * 5 / 12, size / 3, size / 6}, claw_color);
                    painter.fill_rectangle({x_start + size * 3 / 4, y_start + size / 2, size / 5, size / 4}, claw_color);
                } else {
                    // Claws slashing
                    painter.fill_rectangle({x_start, y_start + size * 4 / 12, size / 4, size / 5}, claw_color);
                    painter.fill_rectangle({x_start + size / 12, y_start + size * 6 / 12, size / 5, size / 3}, claw_color);
                    painter.fill_rectangle({x_start + size * 3 / 4, y_start + size * 4 / 12, size / 4, size / 5}, claw_color);
                    painter.fill_rectangle({x_start + size * 2 / 3, y_start + size * 6 / 12, size / 5, size / 3}, claw_color);
                }
            } else {
                // Normal claws
                painter.fill_rectangle({x_start + size / 6, y_start + size / 2, size / 5, size / 5}, claw_color);
                painter.fill_rectangle({x_start + size * 3 / 5, y_start + size / 2, size / 5, size / 5}, claw_color);
            }

            // Legs animation
            uint8_t leg_anim = (LPC_RTC->CTIME0 % 4) / 2;
            if (entities[i].state == 0 && leg_anim == 0) {
                // Walking animation frame 1
                painter.fill_rectangle({x_start + size / 4, y_start + size * 5 / 6, size / 6, size / 5}, body_color);
                painter.fill_rectangle({x_start + size / 3, y_start + size * 11 / 12, size / 10, size / 10}, spine_color);
                painter.fill_rectangle({x_start + size * 7 / 12, y_start + size * 3 / 4, size / 6, size / 4}, body_color);
                painter.fill_rectangle({x_start + size * 2 / 3, y_start + size * 5 / 6, size / 10, size / 5}, spine_color);
            } else {
                // Walking animation frame 2
                painter.fill_rectangle({x_start + size / 4, y_start + size * 3 / 4, size / 6, size / 4}, body_color);
                painter.fill_rectangle({x_start + size / 3, y_start + size * 5 / 6, size / 10, size / 5}, spine_color);
                painter.fill_rectangle({x_start + size * 7 / 12, y_start + size * 5 / 6, size / 6, size / 5}, body_color);
                painter.fill_rectangle({x_start + size * 2 / 3, y_start + size * 11 / 12, size / 10, size / 10}, spine_color);
            }

            // Injury effects for damaged state
            if (entities[i].state == 4) {
                painter.fill_rectangle({x_start + size / 2, y_start + size / 3, size / 10, size / 8}, blood_color);
                painter.fill_rectangle({x_start + size * 5 / 12, y_start + size * 7 / 12, size / 8, size / 8}, blood_color);
                painter.fill_rectangle({x_start + size * 3 / 5, y_start + size / 2, size / 10, size / 10}, blood_color);
            }
        }
    }
}

void render_map(Painter& painter, bool full_clear, int16_t x_start = 0, int16_t x_end = SCREEN_WIDTH) {
    if (full_clear) {
        painter.fill_rectangle({0, 0, SCREEN_WIDTH, RENDER_HEIGHT / 2}, Color(64, 64, 128));
        painter.fill_rectangle({0, RENDER_HEIGHT / 2, SCREEN_WIDTH, RENDER_HEIGHT / 2}, Color(32, 32, 32));
    }

    for (uint8_t x = x_start; x < x_end; x += RES_DIVIDER) {
        double camera_x = 2 * (double)x / SCREEN_WIDTH - 1;
        double ray_x = player.dir.x + player.plane.x * camera_x;
        double ray_y = player.dir.y + player.plane.y * camera_x;

        if (fabs(ray_x) < 0.00001) ray_x = 0.00001;
        if (fabs(ray_y) < 0.00001) ray_y = 0.00001;

        uint8_t map_x = (uint8_t)player.pos.x;
        uint8_t map_y = (uint8_t)player.pos.y;
        double delta_x = fabs(1 / ray_x);
        double delta_y = fabs(1 / ray_y);

        int8_t step_x, step_y;
        double side_x, side_y;

        if (ray_x < 0) {
            step_x = -1;
            side_x = (player.pos.x - map_x) * delta_x;
        } else {
            step_x = 1;
            side_x = (map_x + 1.0 - player.pos.x) * delta_x;
        }
        if (ray_y < 0) {
            step_y = -1;
            side_y = (player.pos.y - map_y) * delta_y;
        } else {
            step_y = 1;
            side_y = (map_y + 1.0 - player.pos.y) * delta_y;
        }

        uint8_t depth = 0;
        bool hit = false;
        bool side = false;
        double perpWallDist = 0.0;

        bool close_wall_handled = false;
        uint8_t current_block = get_block_at(map_x, map_y);
        if (current_block == 0xF) {
            hit = true;
            perpWallDist = 0.1;
            close_wall_handled = true;
        } else {
            uint8_t check_blocks[4][3] = {
                {(uint8_t)(map_x + 1), map_y, 0},
                {(uint8_t)(map_x - 1), map_y, 0},
                {map_x, (uint8_t)(map_y + 1), 1},
                {map_x, (uint8_t)(map_y - 1), 1}};

            for (int i = 0; i < 4; i++) {
                if (get_block_at(check_blocks[i][0], check_blocks[i][1]) == 0xF) {
                    double dist_to_wall;
                    if (check_blocks[i][2] == 0) {
                        dist_to_wall = (check_blocks[i][0] > map_x) ? check_blocks[i][0] - player.pos.x : player.pos.x - check_blocks[i][0];
                    } else {
                        dist_to_wall = (check_blocks[i][1] > map_y) ? check_blocks[i][1] - player.pos.y : player.pos.y - check_blocks[i][1];
                    }

                    if (dist_to_wall < 0.2) {
                        double dot_product;
                        if (check_blocks[i][2] == 0) {
                            double wall_normal_x = (check_blocks[i][0] > map_x) ? -1.0 : 1.0;
                            dot_product = ray_x * wall_normal_x;
                        } else {
                            double wall_normal_y = (check_blocks[i][1] > map_y) ? -1.0 : 1.0;
                            dot_product = ray_y * wall_normal_y;
                        }

                        if (dot_product < 0) {
                            hit = true;
                            side = (check_blocks[i][2] == 1);
                            perpWallDist = fmax(0.1, dist_to_wall);
                            close_wall_handled = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!close_wall_handled) {
            while (!hit && depth < MAX_RENDER_DEPTH) {
                if (side_x < side_y) {
                    side_x += delta_x;
                    map_x += step_x;
                    side = false;
                } else {
                    side_y += delta_y;
                    map_y += step_y;
                    side = true;
                }

                uint8_t block = get_block_at(map_x, map_y);
                if (block == 0xF) {
                    hit = true;
                    if (side == false) {
                        perpWallDist = (map_x - player.pos.x + (1 - step_x) / 2) / ray_x;
                    } else {
                        perpWallDist = (map_y - player.pos.y + (1 - step_y) / 2) / ray_y;
                    }
                }
                depth++;
            }
        }

        if (perpWallDist <= 0) perpWallDist = 0.1;

        int start_y = 0;
        int end_y = RENDER_HEIGHT;

        if (hit) {
            uint8_t line_height = fmin(255, RENDER_HEIGHT / perpWallDist);

            start_y = view_height / perpWallDist - line_height / 2 + RENDER_HEIGHT / 2;
            end_y = view_height / perpWallDist + line_height / 2 + RENDER_HEIGHT / 2;

            if (start_y < 0) start_y = 0;
            if (end_y > RENDER_HEIGHT) end_y = RENDER_HEIGHT;

            uint8_t brightness = fmax(64, 255 - (perpWallDist * 20));
            uint8_t noise = ((map_x * 17 + map_y * 23) & 0x0F);

            Color wall_color;
            if (!side) {
                wall_color = Color(brightness / 4, brightness - noise, brightness / 4);
            } else {
                wall_color = Color(brightness / 5, (brightness - noise) * 0.8, brightness / 5);
            }

            painter.fill_rectangle({x, start_y, RES_DIVIDER, end_y - start_y}, wall_color);
        }

        if (!full_clear && hit) {
            if (start_y > 0) {
                painter.fill_rectangle({x, 0, RES_DIVIDER, start_y}, Color(64, 64, 128));
            }
            if (end_y < RENDER_HEIGHT) {
                painter.fill_rectangle({x, end_y, RES_DIVIDER, RENDER_HEIGHT - end_y}, Color(32, 32, 32));
            }
        }
    }
}

DoomView::DoomView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
    game_timer.attach(&game_timer_check, 1.0 / 60.0);
}

void DoomView::on_show() {
    dummy.focus();
}

void DoomView::paint(Painter& painter) {
    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        scene = 0;
        up = down = left = right = fired = false;
        jogging = view_height = 0;
        gun_pos = GUN_TARGET_POS;
        gun_fired = false;
        num_entities = 0;
        kills = 0;
        prev_gun_x = 0;
        prev_gun_y = 0;
        painter.fill_rectangle({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, pp_colors[COLOR_BACKGROUND]);
        needs_redraw = true;
        needs_gun_redraw = false;
    }

    if (scene == 0) {
        auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
        painter.draw_string({50, 40}, style_yellow, "* * * DOOM * * *");
        auto style_green = *ui::Theme::getInstance()->fg_green;
        painter.draw_string({15, 240}, style_green, "** PRESS SELECT TO START **");
    } else if (scene == 1) {
        if (needs_redraw || (needs_gun_redraw && jogging > 0)) {
            bool full_clear = (player.velocity == 0 || !prev_velocity_moving);
            render_map(painter, full_clear);
            render_entities(painter);
            render_gun(painter, gun_pos, jogging);
            painter.fill_rectangle({0, RENDER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - RENDER_HEIGHT}, pp_colors[COLOR_BACKGROUND]);
            auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
            auto style_red = *ui::Theme::getInstance()->fg_red;
            auto style_blue = *ui::Theme::getInstance()->fg_blue;
            painter.draw_string({5, RENDER_HEIGHT + 5}, style_yellow, "Health: " + std::to_string(player.health));
            painter.draw_string({100, RENDER_HEIGHT + 5}, style_red, "Kills: " + std::to_string(kills));
            painter.draw_string({170, RENDER_HEIGHT + 5}, style_blue, "Ammo: " + std::to_string(player.ammo));
            prev_velocity_moving = (player.velocity != 0);
            needs_redraw = false;
            needs_gun_redraw = false;
        } else if (needs_gun_redraw) {
            int current_x = HALF_WIDTH - GUN_WIDTH / 2 + sin(LPC_RTC->CTIME0 * JOGGING_SPEED) * 10 * jogging;
            int current_y = RENDER_HEIGHT - gun_pos - 29 + fabs(cos(LPC_RTC->CTIME0 * JOGGING_SPEED)) * 8 * jogging;
            int flash_height = (gun_pos > GUN_TARGET_POS) ? (gun_pos - GUN_TARGET_POS) * 2 + 10 : 0;
            int flash_width = (gun_pos > GUN_TARGET_POS) ? GUN_WIDTH + (gun_pos - GUN_TARGET_POS) * 2 : GUN_WIDTH;
            int min_x = fmin(current_x, prev_gun_x) - 10;
            int min_y = fmin(current_y, prev_gun_y) - flash_height - 10;
            int max_x = fmax(current_x, prev_gun_x) + flash_width + 10;
            int max_y = fmax(current_y, prev_gun_y) + GUN_HEIGHT + 10;
            min_x = fmax(0, min_x);
            min_y = fmax(0, min_y);
            max_x = fmin(SCREEN_WIDTH, max_x);
            max_y = fmin(RENDER_HEIGHT, max_y);
            render_map(painter, false, min_x, max_x);
            render_entities(painter);
            render_gun(painter, gun_pos, jogging);
            painter.fill_rectangle({0, RENDER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - RENDER_HEIGHT}, pp_colors[COLOR_BACKGROUND]);
            auto style_yellow = *ui::Theme::getInstance()->fg_yellow;
            auto style_red = *ui::Theme::getInstance()->fg_red;
            auto style_blue = *ui::Theme::getInstance()->fg_blue;
            painter.draw_string({5, RENDER_HEIGHT + 5}, style_yellow, "Health: " + std::to_string(player.health));
            painter.draw_string({100, RENDER_HEIGHT + 5}, style_red, "Kills: " + std::to_string(kills));
            painter.draw_string({170, RENDER_HEIGHT + 5}, style_blue, "Ammo: " + std::to_string(player.ammo));
            needs_gun_redraw = false;
        }
    }
}

void DoomView::frame_sync() {
    if (scene == 1) {
        update_game();
        if (needs_redraw || needs_gun_redraw) set_dirty();
    }
}

bool DoomView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select && scene == 0) {
        scene = 1;
        initialize_level();
        needs_redraw = true;
        set_dirty();
        return true;
    }
    if (scene == 1) {
        if (player.health > 0) {
            if (key == KeyEvent::Up) {
                up = true;
                needs_redraw = true;
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Down) {
                down = true;
                needs_redraw = true;
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Left) {
                left = true;
                needs_redraw = true;
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Right) {
                right = true;
                needs_redraw = true;
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Select) {
                fired = true;
                set_dirty();
                return true;
            }
        } else if (key == KeyEvent::Select) {
            scene = 0;
            initialized = false;
            needs_redraw = true;
            set_dirty();
            return true;
        }
    }
    return false;
}

}  // namespace ui::external_app::doom