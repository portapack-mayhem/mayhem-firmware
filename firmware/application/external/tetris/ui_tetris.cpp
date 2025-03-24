/*
 * Copyright (C) 2024 Mark Thompson
 * 2025 updates by RocketGod (https://betaskynet.com/)
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_tetris.hpp"

namespace ui::external_app::tetris {

Ticker game;
Ticker joystick;

unsigned char level = 0;
const float delays[4] = {1.2, 0.7, 0.4, 0.25};
unsigned int score = 0;
bool firstTime = true;
bool gameStarted = false;
unsigned char nextFigure = 1;
short board[20][10] = {0};
const int colors[8] = {White, Blue, Yellow, Purple, Green, Red, Maroon, Orange};
const short DIMENSION = 16;
const short DIMENSION_NEXT = 12;
short figuresX[7][4] = {{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 0, 0}, {0, 1, 0, 1}, {1, 1, 1, 0}, {1, 1, 1, 0}};
short figuresY[7][4] = {{0, 1, 2, 3}, {1, 0, 0, 1}, {1, 1, 2, 0}, {0, 1, 1, 2}, {0, 1, 1, 2}, {2, 1, 0, 0}, {0, 1, 2, 2}};

const Color pp_colors[] = {
    Color::white(),
    Color::blue(),
    Color::yellow(),
    Color::purple(),
    Color::green(),
    Color::red(),
    Color::magenta(),
    Color::orange(),
    Color::black(),
};

Painter painter;

bool but_RIGHT = false;
bool but_LEFT = false;
bool but_UP = false;
bool but_DOWN = false;
bool but_SELECT = false;

static int x_pos{0};
static int y_pos{0};

static int rotation_state = 0;

static int fg_color;
static int bg_color;

static Callback fall_timer_callback = nullptr;
static uint32_t fall_timer_timeout = 0;
static uint32_t fall_timer_counter = 0;
static Callback dir_button_callback = nullptr;

void cls() {
    painter.fill_rectangle({0, 0, portapack::display.width(), portapack::display.height()}, Color::black());
}

void background(int color) {
    bg_color = color;
}

void foreground(int color) {
    fg_color = color;
}

void locate(int x, int y) {
    x_pos = x;
    y_pos = y;
}

void fillrect(int x1, int y1, int x2, int y2, int color) {
    painter.fill_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void rect(int x1, int y1, int x2, int y2, int color) {
    painter.draw_rectangle({x1, y1, x2 - x1, y2 - y1}, pp_colors[color]);
}

void printf(std::string str) {
    auto style = (fg_color == White) ? *ui::Theme::getInstance()->bg_darkest : *ui::Theme::getInstance()->fg_light;
    painter.draw_string({x_pos, y_pos - 1}, style, str);
}

void printf(std::string str, int v) {
    if (str.find_first_of("%") != std::string::npos) {
        str.resize(str.find_first_of("%"));
    }
    printf(str + to_string_dec_uint(v));
}

void check_fall_timer() {
    if (fall_timer_callback) {
        if (++fall_timer_counter >= fall_timer_timeout) {
            fall_timer_counter = 0;
            fall_timer_callback();
        }
    }
}

void Ticker::attach(Callback func, double delay_sec) {
    if (delay_sec == 0.3) {
        dir_button_callback = func;
    } else {
        fall_timer_callback = func;
        fall_timer_timeout = delay_sec * 60;
    }
}

void Ticker::detach() {
    dir_button_callback = nullptr;
    fall_timer_callback = nullptr;
}

unsigned int GenerateRandomSeed() {
    return LPC_RTC->CTIME0;
}

void Init() {
}

void ShowScore() {
    fillrect(165, 10, 235, 60, Black);
    rect(165, 10, 235, 60, White);
    locate(200, 35);
    printf("%d", score);
}

void ShowNextFigure() {
    fillrect(165, 70, 235, 130, Black);
    rect(165, 70, 235, 130, White);
    int upperLeftX = 176, upperLeftY = 83;
    for (int i = 0; i < 4; i++) {
        int x = upperLeftX + DIMENSION_NEXT * figuresY[nextFigure - 1][i], y = upperLeftY + DIMENSION_NEXT * figuresX[nextFigure - 1][i];
        fillrect(x, y, x + DIMENSION_NEXT, y + DIMENSION_NEXT, colors[nextFigure]);
        rect(x, y, x + DIMENSION_NEXT, y + DIMENSION_NEXT, Black);
    }
}

void DrawCursor(int color, unsigned char lev) {
    fillrect(60, lev * 70 + 50, 72, lev * 70 + 50 + 12, color);
}

void ShowLevelMenu() {
    cls();
    background(Black);
    foreground(White);
    locate(80, 50);
    printf("LEVEL 1");
    locate(80, 120);
    printf("LEVEL 2");
    locate(80, 190);
    printf("LEVEL 3");
    locate(80, 260);
    printf("LEVEL 4");
    DrawCursor(White, level);
}

void ReadJoystickForLevel() {
    unsigned char old = level;
    if (but_UP) {
        (level == 0) ? level = 3 : level--;
    } else if (but_DOWN) {
        level = (level + 1) % 4;
    }
    if (old != level) {
        DrawCursor(Black, old);
        DrawCursor(White, level);
    }
}

void EndPlay() {
    joystick.detach();
    game.detach();
    score = 0;
    firstTime = true;
    gameStarted = false;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            board[i][j] = 0;
        }
    }
    ShowLevelMenu();
    joystick.attach(&ReadJoystickForLevel, 0.3);
}

void StartGame() {
    cls();
    background(Black);
    foreground(White);
    fillrect(0, 0, 162, 320, Black);
    rect(162, 0, 164, 320, White);
    fillrect(164, 0, 240, 320, Black);
    ShowScore();
    ShowNextFigure();
}

void copyCoordinates(short X[], short Y[], unsigned char index) {
    for (int i = 0; i < 4; i++) {
        X[i] = figuresX[index][i];
        Y[i] = figuresY[index][i];
    }
}

bool BottomEdge(int x) {
    return x > 19;
}

bool LeftEdge(int y) {
    return y < 0;
}

bool RightEdge(int y) {
    return y > 9;
}

bool OutOfBounds(int y, int x) {
    return y < 0 || y > 9 || x > 19;
}

void PutBorders(short x, short y) {
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (i < 0 || i > 9 || j < 0 || j > 19 || board[j][i] == 0) continue;
            rect(i * DIMENSION, j * DIMENSION, (i + 1) * DIMENSION, (j + 1) * DIMENSION, Black);
        }
    }
}

short X[4];
short Y[4];
short boardX, boardY;
unsigned char colorIndex;

void Tetromino(unsigned char c) {
    Initialize(c);
}

void Initialize(unsigned char c) {
    colorIndex = c;
    boardX = 0;
    boardY = 4;
    copyCoordinates(X, Y, c - 1);
    rotation_state = 0;
}

void Rotate() {
    short newX[4], newY[4];
    int next_state = (rotation_state + 1) % 4;

    if (colorIndex == 2) {
        return;
    }

    const short kick_tests_I[4][5][2] = {
        {{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
        {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
        {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}}};
    const short kick_tests_other[4][5][2] = {
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}};

    bool is_I_tetromino = (colorIndex == 1);
    const short(*kick_tests)[5][2] = is_I_tetromino ? kick_tests_I : kick_tests_other;

    for (int test = 0; test < 5; test++) {
        short kickX = kick_tests[rotation_state][test][0];
        short kickY = kick_tests[rotation_state][test][1];

        for (int i = 0; i < 4; i++) {
            short tmpX = X[i] - X[1];
            short tmpY = Y[i] - Y[1];
            newX[i] = X[1] - tmpY;
            newY[i] = Y[1] + tmpX;
            int testX = boardX + newX[i] + kickX;
            int testY = boardY + newY[i] + kickY;
            if (OutOfBounds(testY, testX) || (testX >= 0 && board[testX][testY] != 0)) {
                goto next_test;
            }
        }

        DeleteFigure();
        for (int i = 0; i < 4; i++) {
            X[i] = newX[i];
            Y[i] = newY[i];
        }
        boardX += kickX;
        boardY += kickY;
        rotation_state = next_state;
        DrawFigure();
        return;

    next_test:
        continue;
    }
}

void DrawFigure() {
    for (int i = 0; i < 4; i++) {
        int upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
        fillrect(upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, colors[colorIndex]);
        rect(upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, Black);
    }
}

void DeleteFigure() {
    for (int i = 0; i < 4; i++) {
        short upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
        fillrect(upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, Black);
        PutBorders(upperLeftY, upperLeftX);
    }
}

void OnAttached() {
    for (int i = 0; i < 4; i++) {
        board[boardX + X[i]][boardY + Y[i]] = colorIndex;
    }
}

bool MoveDown(char delta) {
    if (!InCollisionDown(delta)) {
        DeleteFigure();
        boardX += delta;
        DrawFigure();
        return true;
    }
    return false;
}

void MoveLeft() {
    if (!InCollisionLeft()) {
        DeleteFigure();
        boardY--;
        DrawFigure();
    }
}

void MoveRight() {
    if (!InCollisionRight()) {
        DeleteFigure();
        boardY++;
        DrawFigure();
    }
}

void SoftDrop() {
    DeleteFigure();
    MoveDown(2);
    DrawFigure();
    score += 2 * (level + 1);
    ShowScore();
}

bool InCollisionDown(char delta) {
    int newX, newY;
    for (int i = 0; i < 4; i++) {
        newX = boardX + X[i] + delta;
        newY = boardY + Y[i];
        if (BottomEdge(newX) || board[newX][newY] != 0) {
            return true;
        }
    }
    return false;
}

bool InCollisionLeft() {
    int newX, newY;
    for (int i = 0; i < 4; i++) {
        newX = boardX + X[i];
        newY = boardY + Y[i] - 1;
        if (LeftEdge(newY) || board[newX][newY] != 0) {
            return true;
        }
    }
    return false;
}

bool InCollisionRight() {
    int newX, newY;
    for (int i = 0; i < 4; i++) {
        newX = boardX + X[i];
        newY = boardY + Y[i] + 1;
        if (RightEdge(newY) || board[newX][newY] != 0) {
            return true;
        }
    }
    return false;
}

void ReadJoystickForFigure() {
    if (but_LEFT) {
        MoveLeft();
    } else if (but_RIGHT) {
        MoveRight();
    } else if (but_UP) {
        pause_game();
    } else if (but_DOWN) {
        SoftDrop();
    } else if (but_SELECT) {
        Rotate();
    }
}

void CheckLines(short& firstLine, short& numberOfLines) {
    firstLine = -1;
    numberOfLines = 0;
    for (int i = 19; i >= 0; i--) {
        short temp = 0;
        for (int j = 0; j < 10; j++) {
            if (board[i][j] == 0) {
                if (numberOfLines > 0) return;
                break;
            }
            temp++;
        }
        if (temp == 10) {
            numberOfLines++;
            if (firstLine == -1) firstLine = i;
        }
    }
}

unsigned int UpdateScore(short numOfLines) {
    unsigned int newIncrement = 0;
    switch (numOfLines) {
        case 1:
            newIncrement = 40;
            break;
        case 2:
            newIncrement = 100;
            break;
        case 3:
            newIncrement = 300;
            break;
        case 4:
            newIncrement = 1200;
            break;
        default:
            newIncrement = 0;
            break;
    }
    return newIncrement * (level + 1);
}

void UpdateBoard() {
    short firstLine, numberOfLines;
    do {
        CheckLines(firstLine, numberOfLines);
        for (int i = firstLine; i >= numberOfLines; i--) {
            for (int j = 0; j < 10; j++) {
                board[i][j] = board[i - numberOfLines][j];
                board[i - numberOfLines][j] = 0;
            }
        }
        fillrect(0, 0, 162, 320, Black);
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 10; j++) {
                if (board[i][j] != 0) {
                    fillrect(j * DIMENSION, i * DIMENSION, (j + 1) * DIMENSION, (i + 1) * DIMENSION, colors[board[i][j]]);
                    rect(j * DIMENSION, i * DIMENSION, (j + 1) * DIMENSION, (i + 1) * DIMENSION, Black);
                }
            }
        }
        score += UpdateScore(numberOfLines);
        DrawFigure();
    } while (numberOfLines != 0);
}

bool IsOver() {
    for (int i = 0; i < 10; i++) {
        if (board[0][i] != 0) return true;
    }
    return false;
}

void ShowGameOverScreen() {
    background(Black);
    foreground(White);
    locate(60, 120);
    printf("GAME OVER");
    locate(40, 150);
    printf("YOUR SCORE IS %d", score);
    wait(5);
}

void InitGame() {
    if (firstTime) {
        Tetromino(rand() % 7 + 1);
        DrawFigure();
        nextFigure = rand() % 7 + 1;
        ShowNextFigure();
        firstTime = false;
    }
}

void PlayGame() {
    InitGame();
    if (!MoveDown(1)) {
        OnAttached();
        UpdateBoard();
        ShowScore();
        Tetromino(nextFigure);
        DrawFigure();
        nextFigure = rand() % 7 + 1;
        ShowNextFigure();
        if (IsOver()) {
            game.detach();
            ShowGameOverScreen();
            EndPlay();
        }
    }
}

void OnTasterPressed() {
    static uint32_t debounce_counter = 0;
    const uint32_t debounce_threshold = 12;

    if (debounce_counter == 0) {
        if (!gameStarted) {
            joystick.detach();
            gameStarted = true;
            StartGame();
            joystick.attach(&ReadJoystickForFigure, 0.3);
            game.attach(&PlayGame, delays[level]);
        }
        debounce_counter = debounce_threshold;
    } else if (debounce_counter > 0) {
        debounce_counter--;
    }
}

void pause_game() {
    game.detach();
    joystick.detach();
    locate(180, 200);
    printf("PAUSED");
    while ((get_switches_state().to_ulong() & 0x10) == 0)
        ;
    printf("      ");
    joystick.attach(&ReadJoystickForFigure, 0.3);
    game.attach(&PlayGame, delays[level]);
}

int main() {
    std::srand(GenerateRandomSeed());
    Init();
    ShowLevelMenu();
    joystick.attach(&ReadJoystickForLevel, 0.3);
    return 0;
}

TetrisView::TetrisView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dummy});
}

void TetrisView::on_show() {
}

void TetrisView::paint(Painter& painter) {
    (void)painter;
    if (!initialized) {
        initialized = true;
        main();
    }
}

void TetrisView::frame_sync() {
    check_fall_timer();
    set_dirty();
}

bool TetrisView::on_encoder(const EncoderEvent delta) {
    if (!gameStarted) {
        unsigned char old = level;
        if (delta > 0) {
            level = (level + 1) % 4;
        } else if (delta < 0) {
            (level == 0) ? level = 3 : level--;
        }
        if (old != level) {
            DrawCursor(Black, old);
            DrawCursor(White, level);
        }
    } else if (gameStarted && delta != 0) {
        Rotate();
    }
    set_dirty();
    return true;
}

bool TetrisView::on_key(const KeyEvent key) {
    auto switches_debounced = get_switches_state().to_ulong();
    but_RIGHT = (switches_debounced & 0x01) != 0;
    but_LEFT = (switches_debounced & 0x02) != 0;
    but_DOWN = (switches_debounced & 0x04) != 0;
    but_UP = (switches_debounced & 0x08) != 0;
    but_SELECT = (switches_debounced & 0x10) != 0;

    if (key == KeyEvent::Select) {
        if (!gameStarted) {
            OnTasterPressed();
        } else {
            Rotate();
        }
    } else if (gameStarted) {
        ReadJoystickForFigure();
    } else {
        ReadJoystickForLevel();
    }
    set_dirty();
    return true;
}

}  // namespace ui::external_app::tetris