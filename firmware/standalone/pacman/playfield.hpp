
#include <cstring>

/******************************************************************************/
/*   MAIN GAME VARIABLES                                                      */
/******************************************************************************/

#define BONUS_INACTIVE_TIME 600
#define BONUS_ACTIVE_TIME 300

#define START_LIFES 2
#define START_LEVEL 1

typedef uint8_t byte;
typedef uint32_t word;

byte SPEED = 2;
byte MAXLIFES = 20;
size_t LIFES = START_LIFES;
byte GAMEWIN = 0;
byte GAMEOVER = 0;
byte DEMO = 1;
byte LEVEL = START_LEVEL;
byte ACTUALBONUS = 0;  // actual bonus icon
byte ACTIVEBONUS = 0;  // status of bonus
byte GAMEPAUSED = 0;

byte PACMANFALLBACK = 0;

bool cheat_level = false;
bool cheat_lifes = false;

#include "DrawIndexedMap.h"

/******************************************************************************/
/*   Controll KEYPAD LOOP                                                     */
/******************************************************************************/

typedef bool boolean;

boolean but_A = false;      // 38
boolean but_B = false;      // 40
boolean but_UP = false;     // 52
boolean but_DOWN = false;   // 50
boolean but_LEFT = false;   // 48
boolean but_RIGHT = false;  // 46

enum cheat_code_keymap {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

enum cheat_code_keymap cheat_code[] = {UP, DOWN, UP, DOWN, RIGHT, LEFT, RIGHT, LEFT};
byte cheat_code_index = 0;

/******************************************************************************/
/*   GAME VARIABLES AND DEFINITIONS                                           */
/******************************************************************************/

#include "PacmanTiles.h"

enum GameState {
    ReadyState,
    PlayState,
    DeadGhostState,  // Player got a ghost, show score sprite and only move eyes
    DeadPlayerState,
    EndLevelState
};

enum SpriteState {
    PenState,
    RunState,
    FrightenedState,
    DeadNumberState,
    DeadEyesState,
    AteDotState,  // pacman
    DeadPacmanState
};

enum {
    MStopped = 0,
    MRight = 1,
    MDown = 2,
    MLeft = 3,
    MUp = 4,
};

#define ushort uint16_t

#define BINKY 0
#define PINKY 1
#define INKY 2
#define CLYDE 3
#define PACMAN 4
#define BONUS 5

const byte _initSprites[] =
    {
        BINKY,
        14,
        17 - 3,
        31,
        MLeft,
        PINKY,
        14 - 2,
        17,
        79,
        MLeft,
        INKY,
        14,
        17,
        137,
        MLeft,
        CLYDE,
        14 + 2,
        17,
        203,
        MRight,
        PACMAN,
        14,
        17 + 9,
        0,
        MLeft,
        BONUS,
        14,
        17 + 3,
        0,
        MLeft,
};

// Ghost colors
const byte _palette2[] =
    {
        0, 11, 1, 15,   // BINKY red
        0, 11, 3, 15,   // PINKY pink
        0, 11, 5, 15,   // INKY cyan
        0, 11, 7, 15,   // CLYDE brown
        0, 11, 9, 9,    // PACMAN yellow
        0, 11, 15, 15,  // FRIGHTENED
        0, 11, 0, 15,   // DEADEYES

        0, 1, 15, 2,   // cherry
        0, 1, 15, 12,  // strawberry
        0, 7, 2, 12,   // peach
        0, 9, 15, 0,   // bell

        0, 15, 1, 2,   // apple
        0, 12, 15, 5,  // grape
        0, 11, 9, 1,   // galaxian
        0, 5, 15, 15,  // key

};

const byte _paletteIcon2[] =
    {
        0, 9, 9, 9,  // PACMAN

        0, 2, 15, 1,   // cherry
        0, 12, 15, 1,  // strawberry
        0, 12, 2, 7,   // peach
        0, 0, 15, 9,   // bell

        0, 2, 15, 1,   // apple
        0, 12, 15, 5,  // grape
        0, 1, 9, 11,   // galaxian
        0, 5, 15, 15,  // key
};

#define PACMANICON 1
#define BONUSICON 2

#define FRIGHTENEDPALETTE 5
#define DEADEYESPALETTE 6

#define BONUSPALETTE 7

#define FPS 60
#define CHASE 0
#define SCATTER 1

#define DOT 7
#define PILL 14
#define PENGATE 0x1B

const byte _opposite[] = {MStopped, MLeft, MUp, MRight, MDown};
#define OppositeDirection(_x) pgm_read_byte(_opposite + _x)

const byte _scatterChase[] = {7, 20, 7, 20, 5, 20, 5, 0};
const byte _scatterTargets[] = {2, 0, 25, 0, 0, 35, 27, 35};               // inky/clyde scatter targets are backwards
const char _pinkyTargetOffset[] = {4, 0, 0, 4, (char)-4, 0, (char)-4, 4};  // Includes pinky target bug

#define FRIGHTENEDGHOSTSPRITE 0
#define GHOSTSPRITE 2
#define NUMBERSPRITE 10
#define PACMANSPRITE 14

const byte _pacLeftAnim[] = {5, 6, 5, 4};
const byte _pacRightAnim[] = {2, 0, 2, 4};
const byte _pacVAnim[] = {4, 3, 1, 3};

word _BonusInactiveTimmer = BONUS_INACTIVE_TIME;
word _BonusActiveTimmer = 0;

/******************************************************************************/
/*   GAME - Sprite Class                                                      */
/******************************************************************************/

class Sprite {
   public:
    int16_t _x, _y;
    int16_t lastx, lasty;
    byte cx, cy;  // cell x and y
    byte tx, ty;  // target x and y

    SpriteState state;
    byte pentimer;  // could be the same

    byte who;
    byte _speed;
    byte dir;
    byte lastDir;
    byte phase;

    // Sprite bits
    byte palette2;  // 4->16 color map index
    byte bits;      // index of sprite bits

    signed char sy;

    void Init(const byte* s) {
        who = pgm_read_byte(s++);
        cx = pgm_read_byte(s++);
        cy = pgm_read_byte(s++);
        pentimer = pgm_read_byte(s++);
        dir = pgm_read_byte(s);
        _x = lastx = (int16_t)cx * 8 - 4;
        _y = lasty = (int16_t)cy * 8;
        state = PenState;
        _speed = 0;
        Target(rand() % 20, rand() % 20);
    }

    void Target(byte x, byte y) {
        tx = x;
        ty = y;
    }

    int16_t Distance(byte x, byte y) {
        int16_t dx = cx - x;
        int16_t dy = cy - y;
        return dx * dx + dy * dy;  // Distance to target
    }

    // once per sprite, not 9 times
    void SetupDraw(GameState gameState, byte deadGhostIndex) {
        sy = 1;
        palette2 = who;
        byte p = phase >> 3;

        if (who == BONUS) {
            // BONUS ICONS
            bits = 21 + ACTUALBONUS;
            palette2 = BONUSPALETTE + ACTUALBONUS;
            return;
        }

        if (who != PACMAN) {
            bits = GHOSTSPRITE + ((dir - 1) << 1) + (p & 1);  // Ghosts
            switch (state) {
                case FrightenedState:
                    bits = FRIGHTENEDGHOSTSPRITE + (p & 1);  // frightened
                    palette2 = FRIGHTENEDPALETTE;
                    break;
                case DeadNumberState:
                    palette2 = FRIGHTENEDPALETTE;
                    bits = NUMBERSPRITE + deadGhostIndex;
                    break;
                case DeadEyesState:
                    palette2 = DEADEYESPALETTE;
                    break;
                default:;
            }
            return;
        }

        // PACMAN animation
        byte f = (phase >> 1) & 3;
        if (dir == MLeft)
            f = pgm_read_byte(_pacLeftAnim + f);
        else if (dir == MRight)
            f = pgm_read_byte(_pacRightAnim + f);
        else if (dir == MDown || dir == MUp)
            f = pgm_read_byte(_pacVAnim + f);
        else if (dir == MStopped) {
            if (lastDir == MLeft)
                f = pgm_read_byte(_pacLeftAnim + f);
            else if (lastDir == MRight)
                f = pgm_read_byte(_pacRightAnim + f);
            else if (lastDir == MDown || lastDir == MUp)
                f = pgm_read_byte(_pacVAnim + f);
        }
        if ((dir == MUp) || (dir == MStopped && lastDir == MUp))
            sy = -1;
        bits = f + PACMANSPRITE;
    }

    // Draw this sprite into the tile at x,y
    void Draw8(int16_t x, int16_t y, byte* tile) {
        int16_t px = x - (_x - 4);
        if (px <= -8 || px >= 16) return;
        int16_t py = y - (_y - 4);
        if (py <= -8 || py >= 16) return;

        // Clip y
        int16_t lines = py + 8;
        if (lines > 16)
            lines = 16;
        if (py < 0) {
            tile -= py * 8;
            py = 0;
        }
        lines -= py;

        // Clip in X
        byte right = 16 - px;
        if (right > 8)
            right = 8;
        byte left = 0;
        if (px < 0) {
            left = -px;
            px = 0;
        }

        // Get bitmap
        signed char dy = sy;
        if (dy < 0)
            py = 15 - py;  // VFlip
        byte* data = (byte*)(pacman16x16 + bits * 64);
        data += py << 2;
        dy <<= 2;
        data += px >> 2;
        px &= 3;

        const byte* palette = _palette2 + (palette2 << 2);
        while (lines) {
            const byte* src = data;
            byte d = pgm_read_byte(src++);
            d >>= px << 1;
            byte sx = 4 - px;
            byte x = left;
            do {
                byte p = d & 3;
                if (p) {
                    p = pgm_read_byte(palette + p);
                    if (p)
                        tile[x] = p;
                }
                d >>= 2;  // Next pixel
                if (!--sx) {
                    d = pgm_read_byte(src++);
                    sx = 4;
                }
            } while (++x < right);

            tile += 8;
            data += dy;
            lines--;
        }
    }
};

/******************************************************************************/
/*   GAME - Playfield Class                                                   */
/******************************************************************************/

class Playfield {
    Sprite _sprites[5];

    Sprite _BonusSprite;  // Bonus

    byte _dotMap[(32 / 4) * (36 - 6)];

    GameState _state;
    long _score;    // 7 digits of score
    long _hiscore;  // 7 digits of score
    long _lifescore;
    signed char _scoreStr[8];
    signed char _hiscoreStr[8];
    byte _icons[14];  // Along bottom of screen

    ushort _stateTimer;
    ushort _frightenedTimer;
    byte _frightenedCount;
    byte _scIndex;    //
    ushort _scTimer;  // next change of sc status

    bool _inited;
    byte* _dirty;

   public:
    Playfield()
        : _inited(false) {
        // Swizzle palette TODO just fix in place
        // byte * p = (byte*)_paletteW;
        // for (int16_t i = 0; i < 16; i++)
        // {
        //   ushort w = _paletteW[i];    // Swizzle
        //   *p++ = w >> 8;
        //   *p++ = w;
        // }
    }

    // Draw 2 bit BG into 8 bit icon tiles at bottom
    void DrawBG2(byte cx, byte cy, byte* tile) {
        byte index = 0;
        signed char b = 0;

        index = _icons[cx >> 1];  // 13 icons across bottom
        if (index == 0) {
            memset(tile, 0, 64);
            return;
        }
        index--;
        index <<= 2;  // 4 tiles per icon

        b = (1 - (cx & 1)) + ((cy & 1) << 1);  // Index of tile

        const byte* bg = pacman8x8x2 + ((b + index) << 4);
        const byte* palette = _paletteIcon2 + index;

        byte x = 16;
        while (x--) {
            byte bits = (signed char)pgm_read_byte(bg++);
            byte i = 4;
            while (i--) {
                tile[i] = pgm_read_byte(palette + (bits & 3));
                bits >>= 2;
            }
            tile += 4;
        }
    }

    byte GetTile(int16_t cx, int16_t ty) {
        if (_state != ReadyState && ty == 20 && cx > 10 && cx < 17) return (0);  // READY TEXT ZONE

        if (LEVEL % 5 == 1) return pgm_read_byte(playMap1 + ty * 28 + cx);
        if (LEVEL % 5 == 2) return pgm_read_byte(playMap2 + ty * 28 + cx);
        if (LEVEL % 5 == 3) return pgm_read_byte(playMap3 + ty * 28 + cx);
        if (LEVEL % 5 == 4) return pgm_read_byte(playMap4 + ty * 28 + cx);
        if (LEVEL % 5 == 0) return pgm_read_byte(playMap5 + ty * 28 + cx);
    }

    // Draw 1 bit BG into 8 bit tile
    void DrawBG(byte cx, byte cy, byte* tile) {
        if (cy >= 34)  // DRAW ICONS BELLOW MAZE
        {
            DrawBG2(cx, cy, tile);
            return;
        }

        byte c = 11;
        if (LEVEL % 8 == 1) c = 11;  // Blue
        if (LEVEL % 8 == 2) c = 12;  // Green
        if (LEVEL % 8 == 3) c = 1;   // Red
        if (LEVEL % 8 == 4) c = 9;   // Yellow
        if (LEVEL % 8 == 5) c = 2;   // Brown
        if (LEVEL % 8 == 6) c = 5;   // Cyan
        if (LEVEL % 8 == 7) c = 3;   // Pink
        if (LEVEL % 8 == 0) c = 15;  // White

        byte b = GetTile(cx, cy);
        const byte* bg;

        // This is a little messy
        memset(tile, 0, 64);
        if (cy == 20 && cx >= 11 && cx < 17) {
            if (DEMO == 1 && ACTIVEBONUS == 1) return;

            if ((_state != ReadyState && GAMEPAUSED != 1 && DEMO != 1) || ACTIVEBONUS == 1)
                b = 0;  // hide 'READY!'
            else if (DEMO == 1 && cx == 11)
                b = 0;
            else if (DEMO == 1 && cx == 12)
                b = 'D';
            else if (DEMO == 1 && cx == 13)
                b = 'E';
            else if (DEMO == 1 && cx == 14)
                b = 'M';
            else if (DEMO == 1 && cx == 15)
                b = 'O';
            else if (DEMO == 1 && cx == 16)
                b = 0;
            else if (GAMEPAUSED == 1 && cx == 11)
                b = 'P';
            else if (GAMEPAUSED == 1 && cx == 12)
                b = 'A';
            else if (GAMEPAUSED == 1 && cx == 13)
                b = 'U';
            else if (GAMEPAUSED == 1 && cx == 14)
                b = 'S';
            else if (GAMEPAUSED == 1 && cx == 15)
                b = 'E';
            else if (GAMEPAUSED == 1 && cx == 16)
                b = 'D';
        } else if (cy == 1) {
            if (cx < 7)
                b = _scoreStr[cx];
            else if (cx >= 10 && cx < 17)
                b = _hiscoreStr[cx - 10];  // HiScore
        } else {
            if (b == DOT || b == PILL)  // DOT==7 or PILL==16
            {
                if (!GetDot(cx, cy))
                    return;
                c = 14;
            }
            if (b == PENGATE)
                c = 14;
        }

        bg = playTiles + (b << 3);
        if (b >= '0')
            c = 15;  // text is white

        for (byte y = 0; y < 8; y++) {
            signed char bits = (signed char)pgm_read_byte(bg++);  /// WARNING CHAR MUST BE signed !!!
            byte x = 0;
            while (bits) {
                if (bits < 0)
                    tile[x] = c;
                bits <<= 1;
                x++;
            }
            tile += 8;
        }
    }

    // Draw BG then all sprites in this cell
    void Draw(uint16_t x, uint16_t y, bool sprites) {
        byte tile[8 * 8];

        // Fill with BG
        if (y == 20 && x >= 11 && x < 17 && DEMO == 1 && ACTIVEBONUS == 1) return;
        DrawBG(x, y, tile);

        // Overlay sprites
        x <<= 3;
        y <<= 3;
        if (sprites) {
            for (byte i = 0; i < 5; i++)
                _sprites[i].Draw8(x, y, tile);

            // AND BONUS
            if (ACTIVEBONUS) _BonusSprite.Draw8(x, y, tile);
        }

        // Show sprite block
#if 0
      for (byte i = 0; i < 5; i++)
      {
        Sprite* s = _sprites + i;
        if (s->cx == (x >> 3) && s->cy == (y >> 3))
        {
          memset(tile, 0, 8);
          for (byte j = 1; j < 7; j++)
            tile[j * 8] = tile[j * 8 + 7] = 0;
          memset(tile + 56, 0, 8);
        }
      }
#endif

        x += (240 - 224) / 2;
        y += (320 - 288) / 2 + 8;

        // Should be a direct Graphics call

        byte n = tile[0];
        byte i = 0;
        word color = (word)_paletteW[n];

        drawIndexedmap(tile, x, y);
    }

    boolean updateMap[36][28];

    // Mark tile as dirty (should not need range checking here)
    void Mark(int16_t x, int16_t y, byte* m) {
        x -= 4;
        y -= 4;

        updateMap[(y >> 3)][(x >> 3)] = true;
        updateMap[(y >> 3)][(x >> 3) + 1] = true;
        updateMap[(y >> 3)][(x >> 3) + 2] = true;
        updateMap[(y >> 3) + 1][(x >> 3)] = true;
        updateMap[(y >> 3) + 1][(x >> 3) + 1] = true;
        updateMap[(y >> 3) + 1][(x >> 3) + 2] = true;
        updateMap[(y >> 3) + 2][(x >> 3)] = true;
        updateMap[(y >> 3) + 2][(x >> 3) + 1] = true;
        updateMap[(y >> 3) + 2][(x >> 3) + 2] = true;
    }

    void DrawAllBG() {
        for (byte y = 0; y < 36; y++)
            for (byte x = 0; x < 28; x++) {
                Draw(x, y, false);
            }
    }

    // Draw sprites overlayed on cells
    void DrawAll() {
        byte* m = _dirty;

        // Mark sprite old/new positions as dirty
        for (byte i = 0; i < 5; i++) {
            Sprite* s = _sprites + i;
            Mark(s->lastx, s->lasty, m);
            Mark(s->_x, s->_y, m);
        }

        // Mark BONUS sprite old/new positions as dirty
        Sprite* _s = &_BonusSprite;
        Mark(_s->lastx, _s->lasty, m);
        Mark(_s->_x, _s->_y, m);

        // Animation
        for (byte i = 0; i < 5; i++)
            _sprites[i].SetupDraw(_state, _frightenedCount - 1);

        _BonusSprite.SetupDraw(_state, _frightenedCount - 1);

        for (byte tmpY = 0; tmpY < 36; tmpY++) {
            for (byte tmpX = 0; tmpX < 28; tmpX++) {
                if (updateMap[tmpY][tmpX] == true) Draw(tmpX, tmpY, true);
                updateMap[tmpY][tmpX] = false;
            }
        }
    }

    int16_t Chase(Sprite* s, int16_t cx, int16_t cy) {
        while (cx < 0)  // Tunneling
            cx += 28;
        while (cx >= 28)
            cx -= 28;

        byte t = GetTile(cx, cy);
        if (!(t == 0 || t == DOT || t == PILL || t == PENGATE))
            return 0x7FFF;

        if (t == PENGATE) {
            if (s->who == PACMAN)
                return 0x7FFF;  // Pacman can't cross this to enter pen
            if (!(InPen(s->cx, s->cy) || s->state == DeadEyesState))
                return 0x7FFF;  // Can cross if dead or in pen trying to get out
        }

        int16_t dx = s->tx - cx;
        int16_t dy = s->ty - cy;
        return (dx * dx + dy * dy);  // Distance to target
    }

    void UpdateTimers() {
        // Update scatter/chase selector, low bit of index indicates scatter
        if (_scIndex < 8) {
            if (_scTimer-- == 0) {
                byte duration = pgm_read_byte(_scatterChase + _scIndex++);
                _scTimer = duration * FPS;
            }
        }

        // BONUS timmer
        if (ACTIVEBONUS == 0 && _BonusInactiveTimmer-- == 0) {
            _BonusActiveTimmer = BONUS_ACTIVE_TIME;  // 5*FPS;
            ACTIVEBONUS = 1;
        }
        if (ACTIVEBONUS == 1 && _BonusActiveTimmer-- == 0) {
            _BonusInactiveTimmer = BONUS_INACTIVE_TIME;  // 10*FPS;
            ACTIVEBONUS = 0;
        }

        // Release frightened ghosts
        if (_frightenedTimer && !--_frightenedTimer) {
            for (byte i = 0; i < 4; i++) {
                Sprite* s = _sprites + i;
                if (s->state == FrightenedState) {
                    s->state = RunState;
                    s->dir = OppositeDirection(s->dir);
                }
            }
        }
    }

    // Target closes pill, run from ghosts?
    void PacmanAI() {
        Sprite* pacman;
        pacman = _sprites + PACMAN;

        // Chase frightened ghosts
        Sprite* closestGhost = NULL;
        Sprite* frightenedGhost = NULL;
        Sprite* closestAttackingGhost = NULL;
        Sprite* DeadEyesStateGhost = NULL;
        int16_t dist = 0x7FFF;
        int16_t closestfrightenedDist = 0x7FFF;
        int16_t closestAttackingDist = 0x7FFF;
        for (byte i = 0; i < 4; i++) {
            Sprite* s = _sprites + i;
            int16_t d = s->Distance(pacman->cx, pacman->cy);
            if (d < dist) {
                dist = d;
                if (s->state == FrightenedState) {
                    frightenedGhost = s;
                    closestfrightenedDist = d;
                } else {
                    closestAttackingGhost = s;
                    closestAttackingDist = d;
                }
                closestGhost = s;

                if (s->state == DeadEyesState) DeadEyesStateGhost = s;
            }
        }

        PACMANFALLBACK = 0;

        if (DEMO == 1 && !DeadEyesStateGhost && frightenedGhost) {
            pacman->Target(frightenedGhost->cx, frightenedGhost->cy);
            return;
        }

        // Under threat; just avoid closest ghost
        if (DEMO == 1 && !DeadEyesStateGhost && dist <= 32 && closestAttackingDist < closestfrightenedDist) {
            if (dist <= 16) {
                pacman->Target(pacman->cx * 2 - closestAttackingGhost->cx, pacman->cy * 2 - closestAttackingGhost->cy);
                PACMANFALLBACK = 1;
            } else {
                pacman->Target(pacman->cx * 2 - closestAttackingGhost->cx, pacman->cy * 2 - closestAttackingGhost->cy);
            }
            return;
        }

        if (ACTIVEBONUS == 1) {
            pacman->Target(13, 20);
            return;
        }

        // Go for the pill
        if (GetDot(1, 6))
            pacman->Target(1, 6);
        else if (GetDot(26, 6))
            pacman->Target(26, 6);
        else if (GetDot(1, 26))
            pacman->Target(1, 26);
        else if (GetDot(26, 26))
            pacman->Target(26, 26);
        else {
            // closest dot
            int16_t dist = 0x7FFF;
            for (byte y = 4; y < 32; y++) {
                for (byte x = 1; x < 26; x++) {
                    if (GetDot(x, y)) {
                        int16_t d = pacman->Distance(x, y);
                        if (d < dist) {
                            dist = d;
                            pacman->Target(x, y);
                        }
                    }
                }
            }

            if (dist == 0x7FFF) {
                GAMEWIN = 1;  // No dots, GAME WIN!
            }
        }
    }

    void Scatter(Sprite* s) {
        const byte* st = _scatterTargets + (s->who << 1);
        s->Target(pgm_read_byte(st), pgm_read_byte(st + 1));
    }

    void UpdateTargets() {
        if (_state == ReadyState)
            return;
        PacmanAI();
        Sprite* pacman = _sprites + PACMAN;

        // Ghost AI
        bool scatter = _scIndex & 1;
        for (byte i = 0; i < 4; i++) {
            Sprite* s = _sprites + i;

            // Deal with returning ghost to pen
            if (s->state == DeadEyesState) {
                if (s->cx == 14 && s->cy == 17)  // returned to pen
                {
                    s->state = PenState;  // Revived in pen
                    s->pentimer = 80;
                } else
                    s->Target(14, 17);  // target pen
                continue;               //
            }

            // Release ghost from pen when timer expires
            if (s->pentimer) {
                if (--s->pentimer)  // stay in pen for awhile
                    continue;
                s->state = RunState;
            }

            if (InPen(s->cx, s->cy)) {
                s->Target(14, 14 - 2);  // Get out of pen first
            } else {
                if (scatter || s->state == FrightenedState)
                    Scatter(s);
                else {
                    // Chase mode targeting
                    signed char tx = pacman->cx;
                    signed char ty = pacman->cy;
                    switch (s->who) {
                        case PINKY: {
                            const char* pto = _pinkyTargetOffset + ((pacman->dir - 1) << 1);
                            tx += pgm_read_byte(pto);
                            ty += pgm_read_byte(pto + 1);
                        } break;
                        case INKY: {
                            const char* pto = _pinkyTargetOffset + ((pacman->dir - 1) << 1);
                            Sprite* binky = _sprites + BINKY;
                            tx += pgm_read_byte(pto) >> 1;
                            ty += pgm_read_byte(pto + 1) >> 1;
                            tx += tx - binky->cx;
                            ty += ty - binky->cy;
                        } break;
                        case CLYDE: {
                            if (s->Distance(pacman->cx, pacman->cy) < 64) {
                                const byte* st = _scatterTargets + CLYDE * 2;
                                tx = pgm_read_byte(st);
                                ty = pgm_read_byte(st + 1);
                            }
                        } break;
                    }
                    s->Target(tx, ty);
                }
            }
        }
    }

    // Default to current direction
    byte ChooseDir(int16_t dir, Sprite* s) {
        int16_t choice[4];
        choice[0] = Chase(s, s->cx, s->cy - 1);  // Up
        choice[1] = Chase(s, s->cx - 1, s->cy);  // Left
        choice[2] = Chase(s, s->cx, s->cy + 1);  // Down
        choice[3] = Chase(s, s->cx + 1, s->cy);  // Right

        if (DEMO == 0 && s->who == PACMAN && choice[0] < 0x7FFF && but_UP)
            dir = MUp;
        else if (DEMO == 0 && s->who == PACMAN && choice[1] < 0x7FFF && but_LEFT)
            dir = MLeft;
        else if (DEMO == 0 && s->who == PACMAN && choice[2] < 0x7FFF && but_DOWN)
            dir = MDown;
        else if (DEMO == 0 && s->who == PACMAN && choice[3] < 0x7FFF && but_RIGHT)
            dir = MRight;

        else if (DEMO == 0 && choice[0] < 0x7FFF && s->who == PACMAN && dir == MUp) {
            dir = MUp;
            s->lastDir = MUp;
        } else if (DEMO == 0 && choice[1] < 0x7FFF && s->who == PACMAN && dir == MLeft) {
            dir = MLeft;
            s->lastDir = MLeft;
        } else if (DEMO == 0 && choice[2] < 0x7FFF && s->who == PACMAN && dir == MDown) {
            dir = MDown;
            s->lastDir = MDown;
        } else if (DEMO == 0 && choice[3] < 0x7FFF && s->who == PACMAN && dir == MRight) {
            dir = MRight;
            s->lastDir = MRight;
        } else if ((DEMO == 0 && s->who != PACMAN) || DEMO == 1) {
            // Don't choose opposite of current direction?

            int16_t dist = choice[4 - dir];  // favor current direction
            byte opposite = OppositeDirection(dir);
            for (byte i = 0; i < 4; i++) {
                byte d = 4 - i;
                if ((d != opposite && choice[i] < dist) || (s->who == PACMAN && PACMANFALLBACK && choice[i] < dist)) {
                    if (s->who == PACMAN && PACMANFALLBACK) PACMANFALLBACK = 0;
                    dist = choice[i];
                    dir = d;
                }
            }

        } else {
            dir = MStopped;
        }

        return dir;
    }

    bool InPen(byte cx, byte cy) {
        if (cx <= 10 || cx >= 18) return false;
        if (cy <= 14 || cy >= 18) return false;
        return true;
    }

    byte GetSpeed(Sprite* s) {
        if (s->who == PACMAN)
            return _frightenedTimer ? 90 : 80;
        if (s->state == FrightenedState)
            return 40;
        if (s->state == DeadEyesState)
            return 100;
        if (s->cy == 17 && (s->cx <= 5 || s->cx > 20))
            return 40;  // tunnel
        return 75;
    }

    void PackmanDied() {  // Noooo... PACMAN DIED :(

        if (LIFES <= 0 && !cheat_lifes) {
            GAMEOVER = 1;
            LEVEL = START_LEVEL;
            LIFES = START_LIFES;
            DEMO = 1;
            Init();
        } else {
            if (!cheat_lifes)
                LIFES--;

            _inited = true;
            _state = ReadyState;
            _stateTimer = FPS / 2;
            _frightenedCount = 0;
            _frightenedTimer = 0;

            const byte* s = _initSprites;
            for (int16_t i = 0; i < 5; i++)
                _sprites[i].Init(s + i * 5);

            _scIndex = 0;
            _scTimer = 1;

            memset(_icons, 0, sizeof(_icons));

            // AND BONUS
            _BonusSprite.Init(s + 5 * 5);
            _BonusInactiveTimmer = BONUS_INACTIVE_TIME;
            _BonusActiveTimmer = 0;

            for (byte i = 0; i < ACTUALBONUS; i++) {
                _icons[13 - i] = BONUSICON + i;
            }

            if (!cheat_lifes) {
                for (byte i = 0; i < LIFES; i++) {
                    _icons[0 + i] = PACMANICON;
                }
            } else {
                for (byte i = 0; i < 14; i++) {
                    _icons[0 + i] = PACMANICON;
                }
            }

            // Draw LIFE and BONUS Icons
            for (byte y = 34; y < 36; y++)
                for (byte x = 0; x < 28; x++) {
                    Draw(x, y, false);
                }

            DrawAllBG();
        }
    }

    void MoveAll() {
        UpdateTimers();
        UpdateTargets();

        // Update game state
        if (_stateTimer) {
            if (--_stateTimer <= 0) {
                switch (_state) {
                    case ReadyState:
                        _state = PlayState;
                        _dirty[20 * 4 + 1] |= 0x1F;  // Clear 'READY!'
                        _dirty[20 * 4 + 2] |= 0x80;

                        for (byte tmpX = 11; tmpX < 17; tmpX++) Draw(tmpX, 20, false);  // ReDraw (clear) 'READY' position

                        break;
                    case DeadGhostState:
                        _state = PlayState;
                        for (byte i = 0; i < 4; i++) {
                            Sprite* s = _sprites + i;
                            if (s->state == DeadNumberState)
                                s->state = DeadEyesState;
                        }
                        break;
                    default:;
                }
            } else {
                if (_state == ReadyState)
                    return;
            }
        }

        for (byte i = 0; i < 5; i++) {
            Sprite* s = _sprites + i;

            // In DeadGhostState, only eyes move
            if (_state == DeadGhostState && s->state != DeadEyesState)
                continue;

            // Calculate speed
            s->_speed += GetSpeed(s);
            if (s->_speed < 100)
                continue;
            s->_speed -= 100;

            s->lastx = s->_x;
            s->lasty = s->_y;
            s->phase++;

            int16_t x = s->_x;
            int16_t y = s->_y;

            if ((x & 0x7) == 0 && (y & 0x7) == 0)  // cell aligned
                s->dir = ChooseDir(s->dir, s);     // time to choose another direction

            switch (s->dir) {
                case MLeft:
                    x -= SPEED;
                    break;
                case MRight:
                    x += SPEED;
                    break;
                case MUp:
                    y -= SPEED;
                    break;
                case MDown:
                    y += SPEED;
                    break;
                case MStopped:
                    break;
            }

            // Wrap x because of tunnels
            while (x < 0)
                x += 224;
            while (x >= 224)
                x -= 224;

            s->_x = x;
            s->_y = y;
            s->cx = (x + 4) >> 3;
            s->cy = (y + 4) >> 3;

            if (s->who == PACMAN)
                EatDot(s->cx, s->cy);
        }

        // Collide
        Sprite* pacman = _sprites + PACMAN;

        // Collide with BONUS
        Sprite* _s = &_BonusSprite;
        if (ACTIVEBONUS == 1 && _s->cx == pacman->cx && _s->cy == pacman->cy) {
            Score(ACTUALBONUS * 50);
            ACTUALBONUS++;
            if (ACTUALBONUS > 7) {
                ACTUALBONUS = 0;
                if (LIFES < MAXLIFES) LIFES++;

                // reset all icons
                memset(_icons, 0, sizeof(_icons));

                for (byte i = 0; i < LIFES; i++) {
                    _icons[0 + i] = PACMANICON;
                }
            }

            for (byte i = 0; i < ACTUALBONUS; i++) {
                _icons[13 - i] = BONUSICON + i;
            }

            // REDRAW LIFE and BONUS icons
            for (byte y = 34; y < 36; y++)
                for (byte x = 0; x < 28; x++) {
                    Draw(x, y, false);
                }

            ACTIVEBONUS = 0;
            _BonusInactiveTimmer = BONUS_INACTIVE_TIME;
        }

        for (byte i = 0; i < 4; i++) {
            Sprite* s = _sprites + i;
            // if (s->cx == pacman->cx && s->cy == pacman->cy)
            if (s->_x + SPEED >= pacman->_x && s->_x - SPEED <= pacman->_x && s->_y + SPEED >= pacman->_y && s->_y - SPEED <= pacman->_y)

            {
                if (s->state == FrightenedState) {
                    s->state = DeadNumberState;  // Killed a ghost
                    _frightenedCount++;
                    _state = DeadGhostState;
                    _stateTimer = 10;
                    Score((1 << _frightenedCount) * 100);
                } else {  // pacman died
                    if (s->state == DeadNumberState || s->state == FrightenedState || s->state == DeadEyesState) {
                    } else {
                        PackmanDied();
                    }
                }
            }
        }
    }

    // Mark a position dirty
    void Mark(int16_t pos) {
        for (byte tmp = 0; tmp < 28; tmp++)
            updateMap[1][tmp] = true;
    }

    void SetScoreChar(byte i, signed char c) {
        if (_scoreStr[i] == c)
            return;
        _scoreStr[i] = c;
        Mark(i + 32);  // Score
        // Mark(i+32+10); //HiScore
    }

    void SetHiScoreChar(byte i, signed char c) {
        if (_hiscoreStr[i] == c)
            return;
        _hiscoreStr[i] = c;
        // Mark(i+32);    //Score
        Mark(i + 32 + 10);  // HiScore
    }

    void Score(int16_t delta) {
        char str[8];
        _score += delta;
        if (DEMO == 0 && _score > _hiscore) _hiscore = _score;

        if (_score > _lifescore && _score % 10000 > 0) {
            _lifescore = (_score / 10000 + 1) * 10000;

            LIFES++;  // EVERY 10000 points = 1UP

            for (byte i = 0; i < LIFES; i++) {
                _icons[0 + i] = PACMANICON;
            }

            // REDRAW LIFE and BONUS icons
            for (byte y = 34; y < 36; y++)
                for (byte x = 0; x < 28; x++) {
                    Draw(x, y, false);
                }
            _score = _score + 100;
        }

        sprintf(str, "%ld", _score);
        byte i = 7 - strlen(str);
        byte j = 0;
        while (i < 7)
            SetScoreChar(i++, str[j++]);
        sprintf(str, "%ld", _hiscore);
        i = 7 - strlen(str);
        j = 0;
        while (i < 7)
            SetHiScoreChar(i++, str[j++]);
    }

    bool GetDot(byte cx, byte cy) {
        return _dotMap[(cy - 3) * 4 + (cx >> 3)] & (0x80 >> (cx & 7));
    }

    void EatDot(byte cx, byte cy) {
        if (!GetDot(cx, cy))
            return;
        byte mask = 0x80 >> (cx & 7);
        _dotMap[(cy - 3) * 4 + (cx >> 3)] &= ~mask;

        byte t = GetTile(cx, cy);
        if (t == PILL) {
            _frightenedTimer = 10 * FPS;
            _frightenedCount = 0;
            for (byte i = 0; i < 4; i++) {
                Sprite* s = _sprites + i;
                if (s->state == RunState) {
                    s->state = FrightenedState;
                    s->dir = OppositeDirection(s->dir);
                }
            }
            Score(50);
        } else
            Score(10);
    }

    void Init() {
        if (GAMEWIN == 1) {
            GAMEWIN = 0;
        } else {
            if (!cheat_level) {
                LEVEL = START_LEVEL;
            }
            if (!cheat_lifes) {
                LIFES = START_LIFES;
            }

            ACTUALBONUS = 0;  // actual bonus icon
            ACTIVEBONUS = 0;  // status of bonus

            _score = 0;
            _lifescore = 10000;

            memset(_scoreStr, 0, sizeof(_scoreStr));
            _scoreStr[5] = _scoreStr[6] = '0';
        }

        _inited = true;
        _state = ReadyState;
        _stateTimer = FPS / 2;
        _frightenedCount = 0;
        _frightenedTimer = 0;

        const byte* s = _initSprites;
        for (int16_t i = 0; i < 5; i++)
            _sprites[i].Init(s + i * 5);

        // AND BONUS
        _BonusSprite.Init(s + 5 * 5);
        _BonusInactiveTimmer = BONUS_INACTIVE_TIME;
        _BonusActiveTimmer = 0;

        _scIndex = 0;
        _scTimer = 1;

        memset(_icons, 0, sizeof(_icons));

        // SET BONUS icons
        for (byte i = 0; i < ACTUALBONUS; i++) {
            _icons[13 - i] = BONUSICON + i;
        }

        // SET Lifes icons
        if (cheat_lifes) {
            for (byte i = 0; i < 14; i++) {  // cuz 14 lives full fills PP's screen
                _icons[0 + i] = PACMANICON;
            }
        } else {
            for (byte i = 0; i < LIFES; i++) {
                _icons[0 + i] = PACMANICON;
            }
        }

        // Draw LIFE and BONUS Icons
        for (byte y = 34; y < 36; y++)
            for (byte x = 0; x < 28; x++) {
                Draw(x, y, false);
            }

        // Init dots from rom
        memset(_dotMap, 0, sizeof(_dotMap));
        byte* map = _dotMap;
        for (byte y = 3; y < 36 - 3; y++)  // 30 interior lines
        {
            for (byte x = 0; x < 28; x++) {
                byte t = GetTile(x, y);
                if (t == 7 || t == 14) {
                    byte s = x & 7;
                    map[x >> 3] |= (0x80 >> s);
                }
            }
            map += 4;
        }
        DrawAllBG();
    }

    void Step() {
        if (GAMEWIN == 1) {
            cheat_level = false;
            cheat_lifes = false;
            LEVEL++;
            Init();
        }

        // Start GAME
        if (but_A && DEMO == 1 && GAMEPAUSED == 0) {  // start
            but_A = false;
            Init();
            DEMO = 0;
        } else if (but_A && DEMO == 0 && GAMEPAUSED == 0) {  // pause
            but_A = false;
            GAMEPAUSED = 1;
        } else if (but_LEFT && DEMO == 1 && GAMEPAUSED == 0) {  // -level

            if ((cheat_code[cheat_code_index] == 2) && (cheat_code_index <= 8)) {
                cheat_code_index++;
            }

            if (cheat_code_index == 8) {
                cheat_level = true;
                if (LEVEL > 1) {
                    LEVEL--;
                }
                Init();
            }

            but_LEFT = false;

        } else if (but_RIGHT && DEMO == 1 && GAMEPAUSED == 0) {  // +level

            if ((cheat_code[cheat_code_index] == 3) && (cheat_code_index <= 8)) {
                cheat_code_index++;
            }

            if (cheat_code_index == 8) {
                cheat_level = true;
                if (LEVEL < 255) {
                    LEVEL++;
                }
                Init();
            }

            but_RIGHT = false;

        } else if (but_UP && DEMO == 1 && GAMEPAUSED == 0) {  // full of lifes

            if ((cheat_code[cheat_code_index] == 0) && (cheat_code_index <= 8)) {
                cheat_code_index++;
            }

            if (cheat_code_index == 8) {
                cheat_lifes = true;
                Init();
            }

            but_UP = false;

        } else if (but_DOWN && DEMO == 1 && GAMEPAUSED == 0) {  // reset

            if ((cheat_code[cheat_code_index] == 1) && (cheat_code_index <= 8)) {
                cheat_code_index++;
            }

            if (cheat_code_index == 8) {
                cheat_level = false;
                cheat_lifes = false;
                but_DOWN = false;
                LIFES = START_LIFES;
                cheat_code_index = 0;
                Init();
            }
        }

        if (GAMEPAUSED && but_A && DEMO == 0) {
            but_A = false;
            GAMEPAUSED = 0;
            for (byte tmpX = 11; tmpX < 17; tmpX++) Draw(tmpX, 20, false);
        }

        // Reset / Start GAME
        if (but_B) {
            DEMO = 0;
            Init();
        } else if (!_inited) {
            DEMO = 1;
            Init();
        }

        // Create a bitmap of dirty tiles
        byte m[(32 / 8) * 36];  // 144 bytes
        memset(m, 0, sizeof(m));
        _dirty = m;

        if (!GAMEPAUSED) MoveAll();  // IF GAME is PAUSED STOP ALL

        if ((ACTIVEBONUS == 0 && DEMO == 1) || GAMEPAUSED == 1)
            for (byte tmpX = 11; tmpX < 17; tmpX++) Draw(tmpX, 20, false);  // Draw 'PAUSED' or 'DEMO' text

        DrawAll();
    }
};