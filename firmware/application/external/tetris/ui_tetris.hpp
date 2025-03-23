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

#ifndef __UI_TETRIS_H__
#define __UI_TETRIS_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "limits.h"
#include "ui_widget.hpp"

namespace ui::external_app::tetris {

enum {
    White,
    Blue,
    Yellow,
    Purple,
    Green,
    Red,
    Maroon,
    Orange,
    Black,
};

extern const Color pp_colors[];
extern Painter painter;
extern bool but_RIGHT;
extern bool but_LEFT;
extern bool but_UP;
extern bool but_DOWN;
extern bool but_SELECT;

void cls();
void background(int color);
void foreground(int color);
void locate(int x, int y);
void fillrect(int x1, int y1, int x2, int y2, int color);
void rect(int x1, int y1, int x2, int y2, int color);
void printf(std::string str);
void printf(std::string str, int v);

#define wait(x) chThdSleepMilliseconds(x * 1000)

using Callback = void (*)(void);

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

extern Ticker game;
extern Ticker joystick;

extern unsigned char level;
extern const float delays[4];
extern unsigned int score;
extern bool firstTime;
extern bool gameStarted;
extern unsigned char nextFigure;
extern short board[20][10];
extern const int colors[8];
extern const short DIMENSION;
extern const short DIMENSION_NEXT;
extern short figuresX[7][4];
extern short figuresY[7][4];

unsigned int GenerateRandomSeed();
void Init();
void ShowScore();
void ShowNextFigure();
void DrawCursor(int color, unsigned char lev);
void ShowLevelMenu();
void ReadJoystickForLevel();
void EndPlay();
void StartGame();
void copyCoordinates(short X[], short Y[], unsigned char index);
bool BottomEdge(int x);
bool LeftEdge(int y);
bool RightEdge(int y);
bool OutOfBounds(int y, int x);
void PutBorders(short x, short y);
void Tetromino(unsigned char c);
void Initialize(unsigned char c);
void Rotate();
void DrawFigure();
void DeleteFigure();
void OnAttached();
bool MoveDown(char delta);
void MoveLeft();
void MoveRight();
void SoftDrop();
bool InCollisionDown(char delta);
bool InCollisionLeft();
bool InCollisionRight();
void ReadJoystickForFigure();
void CheckLines(short& firstLine, short& numberOfLines);
unsigned int UpdateScore(short numOfLines);
void UpdateBoard();
bool IsOver();
void ShowGameOverScreen();
void InitGame();
void PlayGame();
void OnTasterPressed();
void pause_game();

class TetrisView : public View {
   public:
    TetrisView(NavigationView& nav);
    void on_show() override;

    std::string title() const override { return "Tetris"; }

    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

   private:
    bool initialized = false;
    NavigationView& nav_;

    Button dummy{
        {240, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::tetris

#endif /* __UI_TETRIS_H__ */