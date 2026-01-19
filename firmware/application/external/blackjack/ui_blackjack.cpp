/*
 * Blackjack Game for Portapack Mayhem
 * Ported / Enhanced / Graphically made awesome by RocketGod (https://betaskynet.com)
 * Based on BlackJack 83 for TI Calculator by Harper Maddox (was written in Assembly)
 */

#include "ui_blackjack.hpp"

namespace ui::external_app::blackjack {

// Global variables
static BlackjackView* current_instance = nullptr;
static Callback game_update_callback = nullptr;
static uint32_t game_update_timeout = 0;
static uint32_t game_update_counter = 0;
static Painter painter;

void check_game_timer() {
    if (game_update_callback) {
        if (++game_update_counter >= game_update_timeout) {
            game_update_counter = 0;
            game_update_callback();
        }
    }
}

void Ticker::attach(Callback func, double delay_sec) {
    game_update_callback = func;
    game_update_timeout = delay_sec * 60;
}

void Ticker::detach() {
    game_update_callback = nullptr;
}

void game_timer_check() {
    if (current_instance) {
        current_instance->update_game();
    }
}

BlackjackView::BlackjackView(NavigationView& nav)
    : nav_{nav}, game_timer{} {
    // Initialize screen dimensions dynamically
    screen_w = ui::screen_width;
    screen_h = ui::screen_height;

    // Scale card size based on screen
    if (screen_w <= 240) {
        card_width = 50;
        card_height = 65;
    } else if (screen_w >= 400) {
        card_width = 70;
        card_height = 90;
    }

    add_children({&dummy});
    current_instance = this;
    game_timer.attach(&game_timer_check, 1.0 / 60.0);
}

BlackjackView::~BlackjackView() {
    current_instance = nullptr;
}

void BlackjackView::on_show() {
    draw_menu_static();
}

void BlackjackView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        init_deck();
    }
}

void BlackjackView::frame_sync() {
    check_game_timer();
    set_dirty();
}

void BlackjackView::clear_screen() {
    painter.fill_rectangle({0, 0, screen_w, screen_h}, Color::black());
}

void BlackjackView::init_deck() {
    // Initialize deck with card values 0-51
    // 0-12 = Spades (A-K), 13-25 = Hearts, 26-38 = Diamonds, 39-51 = Clubs
    for (int i = 0; i < 52; i++) {
        deck[i] = i;
    }
    shuffle_deck();
}

void BlackjackView::shuffle_deck() {
    // Simple shuffle using random swaps
    for (int i = 51; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
    deck_position = 0;
}

uint8_t BlackjackView::draw_card() {
    if (deck_position >= 52) {
        shuffle_deck();
    }
    return deck[deck_position++];
}

int BlackjackView::get_card_value(uint8_t card) {
    int value = (card % 13) + 1;  // 1-13
    if (value > 10) value = 10;   // Face cards worth 10
    return value;
}

uint8_t BlackjackView::get_card_suit(uint8_t card) {
    return card / 13;  // 0=Spades, 1=Hearts, 2=Diamonds, 3=Clubs
}

std::string BlackjackView::get_card_string(uint8_t card) {
    int value = (card % 13) + 1;
    std::string result;

    if (value == 1)
        result = "A";
    else if (value == 11)
        result = "J";
    else if (value == 12)
        result = "Q";
    else if (value == 13)
        result = "K";
    else if (value == 10)
        result = "10";  // Special case for 10
    else
        result = std::to_string(value);

    return result;
}

int BlackjackView::calculate_hand_value(uint8_t* cards, uint8_t count) {
    int value = 0;
    int aces = 0;

    for (uint8_t i = 0; i < count; i++) {
        int card_val = get_card_value(cards[i]);
        if (card_val == 1) {
            aces++;
            value += 1;
        } else {
            value += card_val;
        }
    }

    // Convert aces from 1 to 11 if beneficial
    while (aces > 0 && value + 10 <= 21) {
        value += 10;
        aces--;
    }

    return value;
}

void BlackjackView::deal_cards() {
    // Clear hands
    player_card_count = 0;
    dealer_card_count = 0;

    // Deal initial cards
    player_cards[player_card_count++] = draw_card();
    dealer_cards[dealer_card_count++] = draw_card();
    player_cards[player_card_count++] = draw_card();
    dealer_cards[dealer_card_count++] = draw_card();

    dealer_hidden = true;
    game_state = GameState::PLAYING;
}

void BlackjackView::player_hit() {
    if (player_card_count < MAX_CARDS_IN_HAND) {
        player_cards[player_card_count++] = draw_card();

        int value = calculate_hand_value(player_cards, player_card_count);
        if (value > 21) {
            // Bust!
            cash = (cash >= bet) ? cash - bet : 0;
            losses++;
            game_state = GameState::GAME_OVER;
        }
    }
}

void BlackjackView::player_stay() {
    dealer_hidden = false;
    game_state = GameState::DEALER_TURN;
    dealer_timer = 0;
}

void BlackjackView::dealer_turn() {
    int dealer_value = calculate_hand_value(dealer_cards, dealer_card_count);

    if (dealer_value < 17 && dealer_card_count < MAX_CARDS_IN_HAND) {
        dealer_cards[dealer_card_count++] = draw_card();
    } else {
        check_game_over();
    }
}

void BlackjackView::check_game_over() {
    int player_value = calculate_hand_value(player_cards, player_card_count);
    int dealer_value = calculate_hand_value(dealer_cards, dealer_card_count);

    if (player_value > 21) {
        // Player bust (already handled)
    } else if (dealer_value > 21) {
        // Dealer bust - player wins
        cash += bet;
        wins++;
    } else if (player_value > dealer_value) {
        // Player wins
        cash += bet;
        wins++;
    } else if (player_value < dealer_value) {
        // Dealer wins
        cash = (cash >= bet) ? cash - bet : 0;
        losses++;
    }
    // Else it's a tie - no money changes hands

    if (cash > high_score) {
        high_score = cash;
    }

    game_state = GameState::GAME_OVER;
}

void BlackjackView::update_game() {
    switch (game_state) {
        case GameState::MENU:
            draw_menu();
            break;

        case GameState::BETTING:
            draw_betting();
            break;

        case GameState::PLAYING:
            draw_game();
            break;

        case GameState::DEALER_TURN:
            if (++dealer_timer >= 60) {  // 1 second delay
                dealer_timer = 0;
                dealer_turn();
            }
            draw_game();
            break;

        case GameState::GAME_OVER:
            draw_game();
            break;

        case GameState::STATS:
            draw_stats();
            break;
    }
}

void BlackjackView::draw_menu_static() {
    clear_screen();

    auto style_title = *ui::Theme::getInstance()->fg_green;
    auto style_text = *ui::Theme::getInstance()->fg_light;
    auto style_rules = *ui::Theme::getInstance()->fg_cyan;

    painter.draw_string({UI_POS_X_CENTER(10), 20}, style_title, "BLACKJACK");

    // Draw rules - adjust Y positions for smaller screens
    int rules_y = (screen_h > 300) ? 55 : 45;
    painter.draw_string({UI_POS_X_CENTER(12), rules_y}, style_rules, "-- RULES --");
    painter.draw_string({UI_POS_X_CENTER(16), rules_y + 20}, style_text, "Get close to 21");
    painter.draw_string({UI_POS_X_CENTER(19), rules_y + 35}, style_text, "without going over");
    painter.draw_string({UI_POS_X_CENTER(18), rules_y + 55}, style_text, "Dealer hits on 16");
    painter.draw_string({UI_POS_X_CENTER(19), rules_y + 70}, style_text, "Dealer stays on 17");
    painter.draw_string({UI_POS_X_CENTER(19), rules_y + 90}, style_text, "Blackjack pays 1:1");

    // Controls
    int controls_y = (screen_h > 300) ? 175 : 155;
    painter.draw_string({UI_POS_X_CENTER(15), controls_y}, style_rules, "-- CONTROLS --");
    painter.draw_string({UI_POS_X_CENTER(18), controls_y + 20}, style_text, "SELECT: Start/Hit");
    painter.draw_string({UI_POS_X_CENTER(12), controls_y + 35}, style_text, "LEFT: Stats");
    painter.draw_string({UI_POS_X_CENTER(17), controls_y + 50}, style_text, "RIGHT: Exit/Stay");

    // Draw high score at bottom
    painter.draw_string({UI_POS_X_CENTER(17), screen_h - 70}, style_text, "High Score: $" + std::to_string(high_score));
}

void BlackjackView::draw_menu() {
    // Only handle the flashing text animation
    if (++blink_counter >= 30) {
        blink_counter = 0;
        blink_state = !blink_state;

        if (blink_state) {
            auto style = *ui::Theme::getInstance()->fg_yellow;
            painter.draw_string({UI_POS_X_CENTER(17), screen_h - 40}, style, "* PRESS SELECT *");
        } else {
            // Clear just the text area
            painter.fill_rectangle({UI_POS_X_CENTER(17), screen_h - 42, 130, 20}, Color::black());
        }
    }
}

void BlackjackView::draw_stats() {
    clear_screen();

    auto style_title = *ui::Theme::getInstance()->fg_green;
    auto style_text = *ui::Theme::getInstance()->fg_light;
    auto style_value = *ui::Theme::getInstance()->fg_yellow;

    painter.draw_string({UI_POS_X_CENTER(11), 30}, style_title, "STATISTICS");

    int stats_x = (screen_w > 300) ? 50 : 30;

    painter.draw_string({stats_x, 80}, style_text, "Wins:");
    painter.draw_string({UI_POS_X_CENTER(5), 80}, style_value, std::to_string(wins));

    painter.draw_string({stats_x, 100}, style_text, "Losses:");
    painter.draw_string({UI_POS_X_CENTER(5), 100}, style_value, std::to_string(losses));

    // Win percentage
    uint32_t total = wins + losses;
    if (total > 0) {
        uint32_t win_pct = (wins * 100) / total;
        painter.draw_string({stats_x, 120}, style_text, "Win %:");
        painter.draw_string({UI_POS_X_CENTER(5), 120}, style_value, std::to_string(win_pct) + "%");
    }

    painter.draw_string({stats_x, 160}, style_text, "High Score:");
    painter.draw_string({UI_POS_X_CENTER(5), 160}, style_value, "$" + std::to_string(high_score));

    painter.draw_string({stats_x, 180}, style_text, "Cash:");
    painter.draw_string({UI_POS_X_CENTER(5), 180}, style_value, "$" + std::to_string(cash));

    painter.draw_string({UI_POS_X_CENTER(12), screen_h - 70}, style_text, "SELECT: Back");
}

void BlackjackView::draw_betting() {
    static uint32_t last_bet = 0;
    static bool betting_drawn = false;

    if (!betting_drawn) {
        clear_screen();

        auto style_title = *ui::Theme::getInstance()->fg_green;
        auto style_text = *ui::Theme::getInstance()->fg_light;

        painter.draw_string({UI_POS_X_CENTER(10), 40}, style_title, "PLACE BET");
        painter.draw_string({30, 80}, style_text, "Cash: $" + std::to_string(cash));

        painter.draw_string({30, 140}, style_text, "ENCODER: +/- $10");
        painter.draw_string({30, 160}, style_text, "SELECT: Deal");

        betting_drawn = true;
        last_bet = 0;
    }

    // Update bet display
    if (bet != last_bet) {
        painter.fill_rectangle({30, 110, 150, 20}, Color::black());
        painter.draw_string({30, 110}, *ui::Theme::getInstance()->fg_yellow, "Bet: $" + std::to_string(bet));
        last_bet = bet;
    }
}

void BlackjackView::draw_game() {
    static int last_player_count = -1;
    static int last_dealer_count = -1;
    static GameState last_game_state = GameState::MENU;
    static uint32_t last_bet = 0;

    // Clear and redraw if hands changed or game state changed
    if (player_card_count != last_player_count || dealer_card_count != last_dealer_count || game_state != last_game_state) {
        clear_screen();

        auto style = *ui::Theme::getInstance()->fg_green;
        painter.draw_string({10, 10}, style, "Cash: $" + std::to_string(cash));
        painter.draw_string({screen_w - 80, 10}, style, "Bet: $" + std::to_string(bet));

        // Draw dealer hand with value next to label
        auto style_value = *ui::Theme::getInstance()->fg_yellow;
        painter.draw_string({10, 45}, *ui::Theme::getInstance()->fg_light, "Dealer:");
        if (!dealer_hidden || game_state == GameState::GAME_OVER) {
            int dealer_value = calculate_hand_value(dealer_cards, dealer_card_count);
            painter.draw_string({70, 45}, style_value, "(" + std::to_string(dealer_value) + ")");
        }
        draw_hand(10, 65, dealer_cards, dealer_card_count, true);

        // Draw player hand with value next to label
        int player_y = (screen_h > 300) ? 165 : 145;
        painter.draw_string({10, player_y}, *ui::Theme::getInstance()->fg_light, "You:");
        int player_value = calculate_hand_value(player_cards, player_card_count);
        painter.draw_string({50, player_y}, style_value, "(" + std::to_string(player_value) + ")");
        draw_hand(10, player_y + 20, player_cards, player_card_count, false);

        // Draw controls or result
        int controls_y = screen_h - 50;
        if (game_state == GameState::PLAYING) {
            auto style_text = *ui::Theme::getInstance()->fg_light;
            painter.draw_string({30, controls_y}, style_text, "SELECT: Hit");
            painter.draw_string({screen_w - 100, controls_y}, style_text, "RIGHT: Stay");
        } else if (game_state == GameState::GAME_OVER) {
            const Style* style_result = ui::Theme::getInstance()->fg_yellow;
            std::string result;

            if (player_value > 21) {
                result = "BUST! You Lose";
                style_result = ui::Theme::getInstance()->fg_red;
            } else if (calculate_hand_value(dealer_cards, dealer_card_count) > 21) {
                result = "Dealer Bust! Win!";
                style_result = ui::Theme::getInstance()->fg_green;
            } else if (player_value > calculate_hand_value(dealer_cards, dealer_card_count)) {
                result = "You Win!";
                style_result = ui::Theme::getInstance()->fg_green;
            } else if (player_value < calculate_hand_value(dealer_cards, dealer_card_count)) {
                result = "You Lose";
                style_result = ui::Theme::getInstance()->fg_red;
            } else {
                result = "Push (Tie)";
            }

            // Draw result
            painter.draw_string({UI_POS_X_CENTER(result.length()), controls_y - 20}, *style_result, result);

            // Draw compact bet selector
            auto style_bet = *ui::Theme::getInstance()->fg_cyan;
            painter.draw_string({screen_w - 100, 25}, style_bet, "Next: $" + std::to_string(bet));

            // Show controls
            painter.draw_string({10, controls_y}, *ui::Theme::getInstance()->fg_light, "SELECT: Deal  ENC: +/-");
        }

        last_player_count = player_card_count;
        last_dealer_count = dealer_card_count;
        last_game_state = game_state;
        last_bet = bet;
    } else if (game_state == GameState::GAME_OVER && bet != last_bet) {
        // Only update the bet display when it changes
        painter.fill_rectangle({screen_w - 100, 25, 90, 16}, Color::black());
        painter.draw_string({screen_w - 100, 25}, *ui::Theme::getInstance()->fg_cyan, "Next: $" + std::to_string(bet));
        last_bet = bet;
    }
}

void BlackjackView::draw_suit_symbol(int x, int y, uint8_t suit, Color color, bool large) {
    if (large) {
        // Scale large symbols based on card size
        int scale = card_width / 60;

        switch (suit) {
            case 0:  // Spades
                // Draw using scaled rectangles
                painter.fill_rectangle({x + 9 * scale, y + 4 * scale, 2 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 5 * scale, 4 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 6 * scale, 6 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 7 * scale, 8 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 5 * scale, y + 8 * scale, 10 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 4 * scale, y + 9 * scale, 12 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 10 * scale, 14 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 11 * scale, 16 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 12 * scale, 18 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 0 * scale, y + 13 * scale, 20 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 16 * scale, 18 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 17 * scale, 16 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 18 * scale, 14 * scale, 1 * scale}, color);
                // Stem
                painter.fill_rectangle({x + 9 * scale, y + 19 * scale, 2 * scale, 4 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 22 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 23 * scale, 6 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 24 * scale, 8 * scale, 1 * scale}, color);
                break;

            case 1:  // Hearts
                // Draw using scaled rectangles
                painter.fill_rectangle({x + 3 * scale, y + 5 * scale, 4 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 6 * scale, 6 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 7 * scale, 8 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 0 * scale, y + 9 * scale, 9 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 13 * scale, y + 5 * scale, 4 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 12 * scale, y + 6 * scale, 6 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 11 * scale, y + 7 * scale, 8 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 11 * scale, y + 9 * scale, 9 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 11 * scale, 18 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 14 * scale, 16 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 16 * scale, 14 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 4 * scale, y + 18 * scale, 12 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 5 * scale, y + 19 * scale, 10 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 20 * scale, 8 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 21 * scale, 6 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 22 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 9 * scale, y + 23 * scale, 2 * scale, 1 * scale}, color);
                break;

            case 2:  // Diamonds
                // Draw using scaled rectangles
                painter.fill_rectangle({x + 10 * scale, y + 3 * scale, 1 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 9 * scale, y + 4 * scale, 3 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 5 * scale, 5 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 6 * scale, 7 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 7 * scale, 9 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 5 * scale, y + 8 * scale, 11 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 4 * scale, y + 9 * scale, 13 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 10 * scale, 15 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 11 * scale, 17 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 12 * scale, 19 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 0 * scale, y + 13 * scale, 21 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 1 * scale, y + 14 * scale, 19 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 15 * scale, 17 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 16 * scale, 15 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 4 * scale, y + 17 * scale, 13 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 5 * scale, y + 18 * scale, 11 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 19 * scale, 9 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 20 * scale, 7 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 21 * scale, 5 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 9 * scale, y + 22 * scale, 3 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 10 * scale, y + 23 * scale, 1 * scale, 1 * scale}, color);
                break;

            case 3:  // Clubs
                // Draw using scaled rectangles
                painter.fill_rectangle({x + 8 * scale, y + 8 * scale, 5 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 9 * scale, 7 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 12 * scale, 5 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 11 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 2 * scale, y + 12 * scale, 6 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 3 * scale, y + 15 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 14 * scale, y + 11 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 13 * scale, y + 12 * scale, 6 * scale, 3 * scale}, color);
                painter.fill_rectangle({x + 14 * scale, y + 15 * scale, 4 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 13 * scale, 9 * scale, 2 * scale}, color);
                painter.fill_rectangle({x + 9 * scale, y + 16 * scale, 3 * scale, 4 * scale}, color);
                painter.fill_rectangle({x + 8 * scale, y + 19 * scale, 5 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 7 * scale, y + 20 * scale, 7 * scale, 1 * scale}, color);
                painter.fill_rectangle({x + 6 * scale, y + 21 * scale, 9 * scale, 1 * scale}, color);
                break;
        }
    } else {
        // Small suit symbols remain the same
        switch (suit) {
            case 0:  // Spades - small
                painter.fill_rectangle({x + 2, y + 1, 3, 3}, color);
                painter.fill_rectangle({x + 1, y + 3, 5, 2}, color);
                painter.fill_rectangle({x + 3, y + 5, 1, 2}, color);
                break;

            case 1:  // Hearts - small
                painter.fill_rectangle({x + 1, y + 1, 2, 2}, color);
                painter.fill_rectangle({x + 4, y + 1, 2, 2}, color);
                painter.fill_rectangle({x + 1, y + 2, 5, 2}, color);
                painter.fill_rectangle({x + 2, y + 4, 3, 1}, color);
                painter.fill_rectangle({x + 3, y + 5, 1, 1}, color);
                break;

            case 2:  // Diamonds - small
                painter.fill_rectangle({x + 3, y + 1, 1, 1}, color);
                painter.fill_rectangle({x + 2, y + 2, 3, 1}, color);
                painter.fill_rectangle({x + 1, y + 3, 5, 1}, color);
                painter.fill_rectangle({x + 2, y + 4, 3, 1}, color);
                painter.fill_rectangle({x + 3, y + 5, 1, 1}, color);
                break;

            case 3:  // Clubs - small
                painter.fill_rectangle({x + 3, y + 1, 2, 2}, color);
                painter.fill_rectangle({x + 1, y + 3, 2, 2}, color);
                painter.fill_rectangle({x + 4, y + 3, 2, 2}, color);
                painter.fill_rectangle({x + 3, y + 5, 2, 2}, color);
                break;
        }
    }
}

void BlackjackView::draw_card(int x, int y, uint8_t card, bool hidden) {
    // Draw card background
    painter.fill_rectangle({x, y, card_width, card_height}, Color::white());
    painter.draw_rectangle({x, y, card_width, card_height}, Color::black());
    painter.draw_rectangle({x + 1, y + 1, card_width - 2, card_height - 2}, Color::grey());

    if (hidden) {
        // Draw card back pattern - diagonal lines
        for (int i = 4; i < card_width - 4; i += 6) {
            for (int j = 4; j < card_height - 4; j += 6) {
                painter.fill_rectangle({x + i, y + j, 3, 3}, Color::blue());
                painter.fill_rectangle({x + i + 3, y + j + 3, 3, 3}, Color::red());
            }
        }
    } else {
        // Draw card value
        uint8_t suit = get_card_suit(card);
        Color suit_color = (suit == 1 || suit == 2) ? Color::red() : Color::black();

        const auto* base_style = ui::Theme::getInstance()->fg_light;
        Style card_style{
            .font = base_style->font,
            .background = Color::white(),
            .foreground = suit_color};

        std::string value_str = get_card_string(card);

        // Draw value in top-left corner
        painter.draw_string({x + 4, y + 4}, card_style, value_str);

        // Draw small suit symbol next to value
        int suit_x = (value_str == "10") ? x + 20 : x + 12;
        draw_suit_symbol(suit_x, y + 4, suit, suit_color, false);

        // Draw value in bottom-right corner
        int bottom_x = (value_str == "10") ? x + card_width - 24 : x + card_width - 16;
        painter.draw_string({bottom_x, y + card_height - 18}, card_style, value_str);

        // Draw small suit symbol in bottom-right
        draw_suit_symbol(x + card_width - 10, y + card_height - 16, suit, suit_color, false);

        // Draw large suit symbol in center
        int center_x = x + (card_width - 20) / 2;
        int center_y = y + (card_height - 25) / 2;
        draw_suit_symbol(center_x, center_y, suit, suit_color, true);
    }
}

void BlackjackView::draw_hand(int x, int y, uint8_t* cards, uint8_t count, bool is_dealer) {
    // Calculate total width needed
    const int overlap = card_width * 2 / 3;   // Amount of overlap when cards need to fit
    const int max_width = screen_w - 10 - x;  // Available width on screen

    int spacing;
    if (count == 1) {
        spacing = 0;
    } else if (count == 2) {
        spacing = card_width + 5;  // Small gap for 2 cards
    } else {
        // Calculate spacing to fit all cards
        int total_overlap_width = card_width + (count - 1) * overlap;
        if (total_overlap_width <= max_width) {
            spacing = overlap;
        } else {
            // Need more overlap to fit
            spacing = (max_width - card_width) / (count - 1);
        }
    }

    for (uint8_t i = 0; i < count; i++) {
        bool hide = is_dealer && dealer_hidden && i == 1;
        int card_x = x + (i * spacing);
        draw_card(card_x, y, cards[i], hide);
    }
}

bool BlackjackView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        switch (game_state) {
            case GameState::MENU:
                if (cash < 10) {
                    cash = 100;  // Reset if broke
                    wins = 0;
                    losses = 0;
                }
                game_state = GameState::BETTING;
                break;

            case GameState::BETTING:
                deal_cards();
                break;

            case GameState::PLAYING:
                player_hit();
                break;

            case GameState::GAME_OVER:
                // Deal new hand with current bet
                if (cash >= bet) {
                    deal_cards();
                } else if (cash >= 10) {
                    // Not enough for current bet, reduce to what they can afford
                    bet = 10;
                    deal_cards();
                } else {
                    // Broke, reset game
                    cash = 100;
                    wins = 0;
                    losses = 0;
                    game_state = GameState::MENU;
                    draw_menu_static();
                }
                break;

            case GameState::STATS:
                game_state = GameState::MENU;
                draw_menu_static();
                break;

            default:
                break;
        }
        return true;
    } else if (key == KeyEvent::Left) {
        if (game_state == GameState::MENU) {
            game_state = GameState::STATS;
        }
        return true;
    } else if (key == KeyEvent::Right) {
        if (game_state == GameState::MENU) {
            nav_.pop();
            return true;
        } else if (game_state == GameState::PLAYING) {
            player_stay();
            return true;
        }
    }

    return false;
}

bool BlackjackView::on_encoder(const EncoderEvent delta) {
    if (game_state == GameState::BETTING || game_state == GameState::GAME_OVER) {
        if (delta > 0 && bet + 10 <= cash) {
            bet += 10;
        } else if (delta < 0 && bet >= 20) {
            bet -= 10;
        }
        return true;
    }

    return false;
}

}  // namespace ui::external_app::blackjack