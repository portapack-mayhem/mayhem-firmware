#ifndef _INCLUDE_TEXT_H_
#define _INCLUDE_TEXT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Utility functions for characters and strings

const char* trim_front(const char* str, char to_trim);
void trim_back(char* str, char to_trim);

/// In-place whitespace trim from front and back:
/// 1) trims the back by changing whitespaces to '\0'
/// 2) trims the front by skipping whitespaces
/// @return trimmed string (pointer to first non-whitespace character)
char* trim(char* str);

/// Trim whitespace from start and end of string
void trim_copy(char* trimmed, const char* str);

/// In-place trim of <> characters from front and back:
/// 1) trims the back by changing > to '\0'
/// 2) trims the front by skipping <
/// @return trimmed string (pointer to first non-whitespace character)
char* trim_brackets(char* str);

char to_upper(char c);
bool is_digit(char c);
bool is_letter(char c);
bool is_space(char c);
bool in_range(char c, char min, char max);
bool starts_with(const char* string, const char* prefix);
bool ends_with(const char* string, const char* suffix);
bool equals(const char* string1, const char* string2);

// Text message formatting:
//   - replaces lowercase letters with uppercase
//   - merges consecutive spaces into single space
void fmtmsg(char* msg_out, const char* msg_in);

/// Extract and copy a space-delimited token from a string.
/// When the last token has been extracted, the return value points to the terminating zero character.
/// @param[out] token Buffer to receive the extracted token
/// @param[in] length Length of the token buffer (number of characters)
/// @param[in] string Pointer to the string
/// @return Pointer to the next token (can be passed to copy_token to extract the next token)
const char* copy_token(char* token, int length, const char* string);

char* append_string(char* string, const char* token);

// Parse a 2 digit integer from string
int dd_to_int(const char* str, int length);

// Convert a 2 digit integer to string
void int_to_dd(char* str, int value, int width, bool full_sign);

typedef enum
{
    FT8_CHAR_TABLE_FULL,                 // table[42] " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-./?"
    FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH, // table[38] " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/"
    FT8_CHAR_TABLE_ALPHANUM_SPACE,       // table[37] " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    FT8_CHAR_TABLE_LETTERS_SPACE,        // table[27] " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    FT8_CHAR_TABLE_ALPHANUM,             // table[36] "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    FT8_CHAR_TABLE_NUMERIC,              // table[10] "0123456789"
} ft8_char_table_e;

/// Convert integer index to ASCII character according to one of character tables
char charn(int c, ft8_char_table_e table);

/// Look up the index of an ASCII character in one of character tables
int nchar(char c, ft8_char_table_e table);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_TEXT_H_
