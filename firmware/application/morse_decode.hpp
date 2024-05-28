#ifndef MORSE_DECODER_H
#define MORSE_DECODER_H

#include <string>
#include <unordered_map>
#include <vector>

class MorseDecoder {
public:
    MorseDecoder();
    void init();
    void run();

private:
    void init_ui();
    void update_ui();
    void init_signal_processing();
    void process_signal();
    void decode_morse();
    void update_waterfall();

    void init_morse_table();
    char decode_morse_character(const std::string& morse_code);

    std::unordered_map<std::string, char> morse_table;
    int frequency;
    int wpm;
    int audio_frequency;
    std::vector<std::string> decoded_text;
};

#endif // MORSE_DECODER_H
