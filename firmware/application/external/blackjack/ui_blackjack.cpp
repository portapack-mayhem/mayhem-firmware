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
    painter.fill_rectangle({0, 0, 240, 320}, Color::black());
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

    painter.draw_string({84, 20}, style_title, "BLACKJACK");

    // Draw rules
    painter.draw_string({70, 55}, style_rules, "-- RULES --");
    painter.draw_string({61, 75}, style_text, "Get close to 21");
    painter.draw_string({61, 90}, style_text, "without going over");
    painter.draw_string({61, 110}, style_text, "Dealer hits on 16");
    painter.draw_string({61, 125}, style_text, "Dealer stays on 17");
    painter.draw_string({61, 145}, style_text, "Blackjack pays 1:1");

    // Controls
    painter.draw_string({65, 175}, style_rules, "-- CONTROLS --");
    painter.draw_string({61, 195}, style_text, "SELECT: Start/Hit");
    painter.draw_string({61, 210}, style_text, "LEFT: Stats");
    painter.draw_string({61, 225}, style_text, "RIGHT: Exit/Stay");

    // Draw high score
    painter.draw_string({61, 250}, style_text, "High Score: $" + std::to_string(high_score));
}

void BlackjackView::draw_menu() {
    // Only handle the flashing text animation
    if (++blink_counter >= 30) {
        blink_counter = 0;
        blink_state = !blink_state;

        if (blink_state) {
            auto style = *ui::Theme::getInstance()->fg_yellow;
            painter.draw_string({55, 280}, style, "* PRESS SELECT *");
        } else {
            // Clear just the text area
            painter.fill_rectangle({55, 278, 130, 20}, Color::black());
        }
    }
}

void BlackjackView::draw_stats() {
    clear_screen();

    auto style_title = *ui::Theme::getInstance()->fg_green;
    auto style_text = *ui::Theme::getInstance()->fg_light;
    auto style_value = *ui::Theme::getInstance()->fg_yellow;

    painter.draw_string({75, 30}, style_title, "STATISTICS");

    painter.draw_string({30, 80}, style_text, "Wins:");
    painter.draw_string({150, 80}, style_value, std::to_string(wins));

    painter.draw_string({30, 100}, style_text, "Losses:");
    painter.draw_string({150, 100}, style_value, std::to_string(losses));

    // Win percentage
    uint32_t total = wins + losses;
    if (total > 0) {
        uint32_t win_pct = (wins * 100) / total;
        painter.draw_string({30, 120}, style_text, "Win %:");
        painter.draw_string({150, 120}, style_value, std::to_string(win_pct) + "%");
    }

    painter.draw_string({30, 160}, style_text, "High Score:");
    painter.draw_string({150, 160}, style_value, "$" + std::to_string(high_score));

    painter.draw_string({30, 180}, style_text, "Cash:");
    painter.draw_string({150, 180}, style_value, "$" + std::to_string(cash));

    painter.draw_string({40, 250}, style_text, "SELECT: Back");
}

void BlackjackView::draw_betting() {
    static uint32_t last_bet = 0;
    static bool betting_drawn = false;

    if (!betting_drawn) {
        clear_screen();

        auto style_title = *ui::Theme::getInstance()->fg_green;
        auto style_text = *ui::Theme::getInstance()->fg_light;

        painter.draw_string({70, 40}, style_title, "PLACE BET");
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
        painter.draw_string({140, 10}, style, "Bet: $" + std::to_string(bet));

        // Draw dealer hand with value next to label
        auto style_value = *ui::Theme::getInstance()->fg_yellow;
        painter.draw_string({10, 45}, *ui::Theme::getInstance()->fg_light, "Dealer:");
        if (!dealer_hidden || game_state == GameState::GAME_OVER) {
            int dealer_value = calculate_hand_value(dealer_cards, dealer_card_count);
            painter.draw_string({70, 45}, style_value, "(" + std::to_string(dealer_value) + ")");
        }
        draw_hand(10, 65, dealer_cards, dealer_card_count, true);

        // Draw player hand with value next to label
        painter.draw_string({10, 165}, *ui::Theme::getInstance()->fg_light, "You:");
        int player_value = calculate_hand_value(player_cards, player_card_count);
        painter.draw_string({50, 165}, style_value, "(" + std::to_string(player_value) + ")");
        draw_hand(10, 185, player_cards, player_card_count, false);

        // Draw controls or result
        if (game_state == GameState::PLAYING) {
            auto style_text = *ui::Theme::getInstance()->fg_light;
            painter.draw_string({30, 290}, style_text, "SELECT: Hit");
            painter.draw_string({130, 290}, style_text, "RIGHT: Stay");
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
            painter.draw_string({60, 270}, *style_result, result);

            // Draw compact bet selector in top right area
            auto style_bet = *ui::Theme::getInstance()->fg_cyan;
            painter.draw_string({140, 25}, style_bet, "Next: $" + std::to_string(bet));

            // Show controls
            painter.draw_string({10, 290}, *ui::Theme::getInstance()->fg_light, "SELECT: Deal  ENC: +/-");
        }

        last_player_count = player_card_count;
        last_dealer_count = dealer_card_count;
        last_game_state = game_state;
        last_bet = bet;
    } else if (game_state == GameState::GAME_OVER && bet != last_bet) {
        // Only update the bet display when it changes
        painter.fill_rectangle({140, 25, 90, 16}, Color::black());
        painter.draw_string({140, 25}, *ui::Theme::getInstance()->fg_cyan, "Next: $" + std::to_string(bet));
        last_bet = bet;
    }
}

void BlackjackView::draw_suit_symbol(int x, int y, uint8_t suit, Color color, bool large) {
    if (large) {
        // Large suit symbols for center of card
        switch (suit) {
            case 0:  // Spades
                // Top curve
                painter.fill_rectangle({x + 9, y + 4, 2, 2}, color);
                painter.fill_rectangle({x + 8, y + 5, 4, 2}, color);
                painter.fill_rectangle({x + 7, y + 6, 6, 2}, color);
                painter.fill_rectangle({x + 6, y + 7, 8, 2}, color);
                painter.fill_rectangle({x + 5, y + 8, 10, 2}, color);
                painter.fill_rectangle({x + 4, y + 9, 12, 2}, color);
                painter.fill_rectangle({x + 3, y + 10, 14, 2}, color);
                painter.fill_rectangle({x + 2, y + 11, 16, 2}, color);
                painter.fill_rectangle({x + 1, y + 12, 18, 2}, color);
                painter.fill_rectangle({x + 0, y + 13, 20, 3}, color);
                painter.fill_rectangle({x + 1, y + 16, 18, 2}, color);
                painter.fill_rectangle({x + 2, y + 17, 16, 1}, color);
                painter.fill_rectangle({x + 3, y + 18, 14, 1}, color);
                // Stem
                painter.fill_rectangle({x + 9, y + 19, 2, 4}, color);
                painter.fill_rectangle({x + 8, y + 22, 4, 1}, color);
                painter.fill_rectangle({x + 7, y + 23, 6, 1}, color);
                painter.fill_rectangle({x + 6, y + 24, 8, 1}, color);
                break;

            case 1:  // Hearts
                // Left bump
                painter.fill_rectangle({x + 3, y + 5, 4, 2}, color);
                painter.fill_rectangle({x + 2, y + 6, 6, 2}, color);
                painter.fill_rectangle({x + 1, y + 7, 8, 3}, color);
                painter.fill_rectangle({x + 0, y + 9, 9, 3}, color);
                // Right bump
                painter.fill_rectangle({x + 13, y + 5, 4, 2}, color);
                painter.fill_rectangle({x + 12, y + 6, 6, 2}, color);
                painter.fill_rectangle({x + 11, y + 7, 8, 3}, color);
                painter.fill_rectangle({x + 11, y + 9, 9, 3}, color);
                // Body
                painter.fill_rectangle({x + 1, y + 11, 18, 3}, color);
                painter.fill_rectangle({x + 2, y + 14, 16, 2}, color);
                painter.fill_rectangle({x + 3, y + 16, 14, 2}, color);
                painter.fill_rectangle({x + 4, y + 18, 12, 1}, color);
                painter.fill_rectangle({x + 5, y + 19, 10, 1}, color);
                painter.fill_rectangle({x + 6, y + 20, 8, 1}, color);
                painter.fill_rectangle({x + 7, y + 21, 6, 1}, color);
                painter.fill_rectangle({x + 8, y + 22, 4, 1}, color);
                painter.fill_rectangle({x + 9, y + 23, 2, 1}, color);
                break;

            case 2:  // Diamonds
                painter.fill_rectangle({x + 10, y + 3, 1, 1}, color);
                painter.fill_rectangle({x + 9, y + 4, 3, 1}, color);
                painter.fill_rectangle({x + 8, y + 5, 5, 1}, color);
                painter.fill_rectangle({x + 7, y + 6, 7, 1}, color);
                painter.fill_rectangle({x + 6, y + 7, 9, 1}, color);
                painter.fill_rectangle({x + 5, y + 8, 11, 1}, color);
                painter.fill_rectangle({x + 4, y + 9, 13, 1}, color);
                painter.fill_rectangle({x + 3, y + 10, 15, 1}, color);
                painter.fill_rectangle({x + 2, y + 11, 17, 1}, color);
                painter.fill_rectangle({x + 1, y + 12, 19, 1}, color);
                painter.fill_rectangle({x + 0, y + 13, 21, 1}, color);
                painter.fill_rectangle({x + 1, y + 14, 19, 1}, color);
                painter.fill_rectangle({x + 2, y + 15, 17, 1}, color);
                painter.fill_rectangle({x + 3, y + 16, 15, 1}, color);
                painter.fill_rectangle({x + 4, y + 17, 13, 1}, color);
                painter.fill_rectangle({x + 5, y + 18, 11, 1}, color);
                painter.fill_rectangle({x + 6, y + 19, 9, 1}, color);
                painter.fill_rectangle({x + 7, y + 20, 7, 1}, color);
                painter.fill_rectangle({x + 8, y + 21, 5, 1}, color);
                painter.fill_rectangle({x + 9, y + 22, 3, 1}, color);
                painter.fill_rectangle({x + 10, y + 23, 1, 1}, color);
                break;

            case 3:  // Clubs
                // Center circle
                painter.fill_rectangle({x + 8, y + 8, 5, 1}, color);
                painter.fill_rectangle({x + 7, y + 9, 7, 3}, color);
                painter.fill_rectangle({x + 8, y + 12, 5, 1}, color);
                // Left circle
                painter.fill_rectangle({x + 3, y + 11, 4, 1}, color);
                painter.fill_rectangle({x + 2, y + 12, 6, 3}, color);
                painter.fill_rectangle({x + 3, y + 15, 4, 1}, color);
                // Right circle
                painter.fill_rectangle({x + 14, y + 11, 4, 1}, color);
                painter.fill_rectangle({x + 13, y + 12, 6, 3}, color);
                painter.fill_rectangle({x + 14, y + 15, 4, 1}, color);
                // Connect circles
                painter.fill_rectangle({x + 6, y + 13, 9, 2}, color);
                // Stem
                painter.fill_rectangle({x + 9, y + 16, 3, 4}, color);
                painter.fill_rectangle({x + 8, y + 19, 5, 1}, color);
                painter.fill_rectangle({x + 7, y + 20, 7, 1}, color);
                painter.fill_rectangle({x + 6, y + 21, 9, 1}, color);
                break;
        }
    } else {
        // Small suit symbols
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
    const int width = 60;
    const int height = 80;

    // Draw card background
    painter.fill_rectangle({x, y, width, height}, Color::white());
    painter.draw_rectangle({x, y, width, height}, Color::black());
    painter.draw_rectangle({x + 1, y + 1, width - 2, height - 2}, Color::grey());  // Inner border

    if (hidden) {
        // Draw card back pattern - diagonal lines
        for (int i = 4; i < width - 4; i += 6) {
            for (int j = 4; j < height - 4; j += 6) {
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
        int bottom_x = (value_str == "10") ? x + width - 24 : x + width - 16;
        painter.draw_string({bottom_x, y + height - 18}, card_style, value_str);

        // Draw small suit symbol in bottom-right
        draw_suit_symbol(x + width - 10, y + height - 16, suit, suit_color, false);

        // Draw large suit symbol in center
        draw_suit_symbol(x + 20, y + 28, suit, suit_color, true);
    }
}

void BlackjackView::draw_hand(int x, int y, uint8_t* cards, uint8_t count, bool is_dealer) {
    // Calculate total width needed
    const int card_width = 60;
    const int overlap = 40;         // Amount of overlap when cards need to fit
    const int max_width = 230 - x;  // Available width on screen

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
                    cash = 100;  // Reset if broke - maybe should provide https://gamblersanonymous.org/ link if they also lost their wife and house
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
