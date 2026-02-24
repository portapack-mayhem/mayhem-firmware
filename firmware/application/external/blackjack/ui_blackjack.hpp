/*
 * Blackjack Game for Portapack Mayhem
 * Ported / Enhanced / Graphically made awesome by RocketGod (https://betaskynet.com)
 * Based on BlackJack 83 for TI Calculator by Harper Maddox (was written in Assembly)
 */

#ifndef __UI_BLACKJACK_H__
#define __UI_BLACKJACK_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "ui_widget.hpp"
#include "app_settings.hpp"
#include <string>
#include <vector>

namespace ui::external_app::blackjack {

// Game states
enum class GameState {
    MENU,
    BETTING,
    PLAYING,
    DEALER_TURN,
    GAME_OVER,
    STATS
};

// Card structure
struct Card {
    uint8_t value;  // 1-13 (Ace=1, Jack=11, Queen=12, King=13)
    uint8_t suit;   // 0=Spades, 1=Hearts, 2=Diamonds, 3=Clubs

    Card()
        : value(0), suit(0) {}
    Card(uint8_t v, uint8_t s)
        : value(v), suit(s) {}
};

// Timer class
using Callback = void (*)(void);

class Ticker {
   public:
    Ticker() = default;
    void attach(Callback func, double delay_sec);
    void detach();
};

// Function declarations
void check_game_timer();
void game_timer_check();

class BlackjackView : public View {
   public:
    BlackjackView(NavigationView& nav);
    ~BlackjackView();

    void on_show() override;
    std::string title() const override { return "Blackjack"; }
    void focus() override { dummy.focus(); }
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

    // Public for timer callback
    GameState game_state = GameState::MENU;
    void update_game();

   private:
    NavigationView& nav_;
    bool initialized = false;

    // Screen dimensions (set dynamically)
    int screen_w = 0;
    int screen_h = 0;
    int card_width = 60;
    int card_height = 80;

    // Game variables
    static constexpr uint8_t MAX_CARDS_IN_HAND = 11;  // Maximum possible cards before bust
    uint8_t deck[52];
    uint8_t deck_position = 0;

    uint8_t player_cards[MAX_CARDS_IN_HAND];
    uint8_t player_card_count = 0;

    uint8_t dealer_cards[MAX_CARDS_IN_HAND];
    uint8_t dealer_card_count = 0;

    // Game state variables
    uint32_t cash = 100;
    uint32_t bet = 10;
    uint32_t wins = 0;
    uint32_t losses = 0;
    uint32_t high_score = 100;
    bool dealer_hidden = true;

    // UI state
    uint8_t menu_selection = 0;
    bool blink_state = true;
    uint32_t blink_counter = 0;
    uint32_t dealer_timer = 0;

    // Timer
    Ticker game_timer;

    // Methods
    void init_deck();
    void shuffle_deck();
    uint8_t draw_card();
    void deal_cards();
    int calculate_hand_value(uint8_t* cards, uint8_t count);
    int get_card_value(uint8_t card);
    uint8_t get_card_suit(uint8_t card);
    std::string get_card_string(uint8_t card);
    void player_hit();
    void player_stay();
    void dealer_turn();
    void check_game_over();
    void reset_game();

    // Drawing methods
    void draw_menu();
    void draw_menu_static();
    void draw_stats();
    void draw_game();
    void draw_betting();
    void draw_card(int x, int y, uint8_t card, bool hidden = false);
    void draw_hand(int x, int y, uint8_t* cards, uint8_t count, bool is_dealer = false);
    void draw_suit_symbol(int x, int y, uint8_t suit, Color color, bool large);
    void clear_screen();

    // Settings
    app_settings::SettingsManager settings_{
        "blackjack",
        app_settings::Mode::NO_RF,
        {{"cash"sv, &cash},
         {"wins"sv, &wins},
         {"losses"sv, &losses},
         {"highscore"sv, &high_score}}};

    Button dummy{
        {0, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::blackjack

#endif /* __UI_BLACKJACK_H__ */