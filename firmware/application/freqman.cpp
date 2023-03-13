/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "freqman.hpp"
#include <algorithm>

using option_t = std::pair<std::string, int32_t>;
using options_t = std::vector<option_t>;

options_t freqman_entry_modulations = {
    { "AM", 0 },
    { "NFM", 1 },
    { "WFM", 2 }
};

options_t freqman_entry_bandwidths[ 4 ] = {
    { //AM
        { "DSB 9k" , 0 },
        { "DSB 6k" , 1 },
        { "USB+3k" , 2 },
        { "LSB-3k" , 3 },
        { "CW"     , 4 }
    },
    { //NFM
        { "8k5" , 0 },
        { "11k" , 1 },
        { "16k" , 2 }
    },
    { //WFM
        { "200k" , 0 },
    }
};

options_t freqman_entry_steps = {
    { "0.1KHz      " , 100 },
    { "1KHz        " , 1000 },
    { "5KHz (SA AM)" , 5000 },
    { "6.25KHz(NFM)" , 6250 },
    { "8.33KHz(AIR)" , 8330 },
    { "9KHz (EU AM)" , 9000 },
    { "10KHz(US AM)" , 10000 },
    { "12.5KHz(NFM)" , 12500 },
    { "15KHz  (HFM)" , 15000 },
    { "25KHz   (N1)" , 25000 },
    { "30KHz (OIRT)" , 30000 },
    { "50KHz  (FM1)" , 50000 },
    { "100KHz (FM2)" , 100000 },
    { "250KHz  (N2)" , 250000 },
    { "500KHz (WFM)" , 500000 },
    { "1MHz        " , 1000000 }
};

options_t freqman_entry_steps_short = {
    { "0.1KHz"  , 100 },
    { "1KHz"    , 1000 },
    { "5KHz"    , 5000 },
    { "6.25KHz" , 6250 },
    { "8.33KHz" , 8330 },
    { "9KHz"    , 9000 },
    { "10KHz"   , 10000 },
    { "12.5KHz" , 12500 },
    { "15KHz"   , 15000 },
    { "25KHz"   , 25000 },
    { "30KHz"   , 30000 },
    { "50KHz"   , 50000 },
    { "100KHz"  , 100000 },
    { "250KHz"  , 250000 },
    { "500KHz"  , 500000 },
    { "1MHz"    , 1000000 }
};

std::vector<std::string> get_freqman_files() {
    std::vector<std::string> file_list;

    auto files = scan_root_files(u"FREQMAN", u"*.TXT");

    for (auto file : files) {
        std::string file_name = file.stem().string();
        // don't propose tmp / hidden files in freqman's list
        if (file_name.length() && file_name[0] != '.') {
            file_list.emplace_back(file_name);
        }
    }

    return file_list;
};

bool load_freqman_file(std::string& file_stem, freqman_db &db) {
    return load_freqman_file_ex( file_stem , db , true , true , true );
}

bool load_freqman_file_ex(std::string& file_stem, freqman_db& db, bool load_freqs , bool load_ranges , bool load_hamradios ) {
    File freqman_file;
    size_t length, n = 0, file_position = 0;
    char * pos;
    char * line_start;
    char * line_end;
    std::string description;
    rf::Frequency frequency_a, frequency_b;
    char file_data[257];
    freqman_entry_type type;
    freqman_index_t modulation = 0 ;
    freqman_index_t bandwidth = 0 ;
    freqman_index_t step = 0 ;
    freqman_index_t tone = 0 ;


    db.clear();

    auto result = freqman_file.open("FREQMAN/" + file_stem + ".TXT");
    if (result.is_valid())
        return false;

    while (1) {
        // Read a 256 bytes block from file
        freqman_file.seek(file_position);

        memset(file_data, 0, 257);
        auto read_size = freqman_file.read(file_data, 256);
        if (read_size.is_error())
            return false;	// Read error

        file_position += 256;

        // Reset line_start to beginning of buffer
        line_start = file_data;

        if (!strstr(file_data, "f=") && !strstr(file_data, "a=") && !strstr(file_data, "r=") )
            break;

        // Look for complete lines in buffer
        while ((line_end = strstr(line_start, "\x0A"))) {

            modulation = -1 ;
            bandwidth = -1 ;
            step = -1 ;
            tone = -1 ;
            type=SINGLE ;

            frequency_a = frequency_b = 0;
            // Read frequency
            pos = strstr(line_start, "f=");
            if(pos) {
                pos += 2;
                frequency_a = strtoll(pos, nullptr, 10);
            } else {
                // ...or range
                pos = strstr(line_start, "a=");
                if (pos) {
                    pos += 2;
                    frequency_a = strtoll(pos, nullptr, 10);
                    type = RANGE;
                    pos = strstr(line_start, "b=");
                    if (pos) {
                        pos += 2;
                        frequency_b = strtoll(pos, nullptr, 10);
                    } else
                        frequency_b = 0;
                }else {
                    // ... or hamradio
                    pos = strstr(line_start, "r=");
                    if (pos) {
                        pos += 2;
                        frequency_a = strtoll(pos, nullptr, 10);
                        type = HAMRADIO;
                        pos = strstr(line_start, "t=");
                        if (pos) {
                            pos += 2;
                            frequency_b = strtoll(pos, nullptr, 10);
                        } else
                            frequency_b = frequency_a ;
                    } else
                        frequency_a = 0;
                }
            }
            // modulation if any
            pos = strstr(line_start, "m=");
            if (pos) {
                pos += 2;
                modulation = freqman_entry_get_modulation_from_str( pos );
            } 
            // bandwidth if any
            pos = strstr(line_start, "bw=");
            if (pos) {
                pos += 3;
                bandwidth = freqman_entry_get_bandwidth_from_str( modulation , pos );
            } 
            // step if any
            pos = strstr(line_start, "s=");
            if (pos) {
                pos += 2;
                step = freqman_entry_get_step_from_str_short( pos );
            } 
            // ctcss tone if any
            /* disabled until better form
               pos = strstr(line_start, "c=");
               if (pos) {
               pos += 2;
               tone = tone_key_index_by_value( strtoll( pos , nullptr , 10 ) );
               } */
            // Read description until , or LF
            pos = strstr(line_start, "d=");
            if (pos) {
                pos += 2;
                length = std::min(strcspn(pos, ",\x0A"), (size_t)FREQMAN_DESC_MAX_LEN);
                description = string(pos, length);
            } else
                description = "-";
            if( (type == SINGLE && load_freqs) || (type == RANGE && load_ranges) || (type == HAMRADIO && load_hamradios) )
            {
                db.push_back({ frequency_a, frequency_b, description, type , modulation , bandwidth , step , tone });
                n++;
                if (n >= FREQMAN_MAX_PER_FILE) return true;
            }

            line_start = line_end + 1;
            if (line_start - file_data >= 256) break;
        }

        if (read_size.value() != 256)
            break;	// End of file

        // Restart at beginning of last incomplete line
        file_position -= (file_data + 256 - line_start);
    }

    /* populate implicitly specified modulation / bandwidth */
    if( db.size() > 2 )
    {
        modulation = db[ 0 ] . modulation; 
        bandwidth = db[ 0 ] . bandwidth; 

        for( unsigned int it = 1 ; it < db.size() ; it ++ )
        {
            if( db[ it ] . modulation < 0 )
            {
                db[ it ] . modulation = modulation ;
            }
            else
            {
                modulation = db[ it ] . modulation ;
            }
            if( db[ it ] . bandwidth < 0 )
            {
                db[ it ] . bandwidth = bandwidth ;
            }
            else
            {
                modulation = db[ it ] . bandwidth ;
            }
        }
    }
    return true;
}

bool get_freq_string( freqman_entry &entry , std::string &item_string )
{
    rf::Frequency frequency_a, frequency_b;

    frequency_a = entry.frequency_a;
    if (entry.type == SINGLE) {
        // Single
        item_string = "f=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
    } else if( entry.type == RANGE ) {
        // Range
        frequency_b = entry.frequency_b;
        item_string = "a=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
        item_string += ",b=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
        if( entry.step >= 0 )
        {
            item_string += ",s=" + freqman_entry_get_step_string_short( entry.step );
        }
    } else if( entry.type == HAMRADIO ) {
        frequency_b = entry.frequency_b;
        item_string = "r=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
        item_string += ",t=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
        if( entry.tone >= 0 )
        {
            item_string += ",c=" + tone_key_string( entry.tone );
        }
    }
    if( entry.modulation >= 0 && (unsigned)entry.modulation < freqman_entry_modulations . size() )
    {
        item_string += ",m=" + freqman_entry_get_modulation_string( entry.modulation );
        if( entry.bandwidth >= 0 && (unsigned)entry.bandwidth < freqman_entry_bandwidths[ entry.modulation ] . size()  )
        {
            item_string += ",bw=" + freqman_entry_get_bandwidth_string( entry.modulation , entry.bandwidth );
        }
    }
    if (entry.description.size())
        item_string += ",d=" + entry.description;

    return true ;
}

bool save_freqman_file(std::string &file_stem, freqman_db &db) {

    File freqman_file;

    std::string freq_file_path = "FREQMAN/" + file_stem + ".TXT";
    std::string tmp_freq_file_path = "FREQMAN/" + file_stem + ".TXT.TMP";

    if( !db.size() )
    {
        delete_file( "FREQMAN/"+file_stem+".TXT" );
        return true ;
    }

    delete_file( tmp_freq_file_path );
    auto result = freqman_file.open( tmp_freq_file_path );
    if ( !result.is_valid() ) {
        for (size_t n = 0; n < db.size(); n++) {
            std::string item_string;
            auto& entry = db[n];
            get_freq_string( entry , item_string );
            freqman_file.write_line( item_string );
            delete &item_string; 
        }
        delete_file( freq_file_path );
        rename_file( tmp_freq_file_path , freq_file_path );
        return true;
    }
    return false ;
}

bool create_freqman_file(std::string& file_stem, File& freqman_file) {

    auto result = freqman_file.create( "FREQMAN/" + file_stem + ".TXT" );

    if (result.is_valid())
        return false;

    return true;
}

std::string freqman_item_string(freqman_entry &entry, size_t max_length) {
    std::string item_string;

    switch( entry.type ){
        case SINGLE:
            item_string = to_string_short_freq(entry.frequency_a) + "M: " + entry.description;
            break;
        case RANGE:
            item_string = "R: " + entry.description;
            break;
        case HAMRADIO:
            item_string = "H: " + entry.description;
            break;
        default:
            item_string = "!UNKNOW TYPE " + entry.description;
            break;
    }

    if (item_string.size() > max_length)
        return item_string.substr(0, max_length - 3) + "...";

    return item_string;
}

void freqman_set_modulation_option( OptionsField &option )
{
    option.set_options( freqman_entry_modulations );
}

void freqman_set_bandwidth_option( freqman_index_t modulation , OptionsField &option )
{
    option.set_options( freqman_entry_bandwidths[ modulation ] );
}

void freqman_set_step_option( OptionsField &option )
{
    option.set_options( freqman_entry_steps );
}

void freqman_set_step_option_short( OptionsField &option )
{
    option.set_options( freqman_entry_steps_short );
}

std::string freqman_entry_get_modulation_string( freqman_index_t modulation )
{
    if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
    {
        return std::string( "" ); // unknown modulation
    }
    return freqman_entry_modulations[ modulation ] . first ;
}

std::string freqman_entry_get_bandwidth_string( freqman_index_t modulation , freqman_index_t bandwidth )
{
    if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
    {
        return std::string( "" ); // unknown modulation
    }
    if( bandwidth < 0 || (unsigned)bandwidth > freqman_entry_bandwidths[ modulation ] . size() )
    {
        return std::string( "" ); // unknown modulation
    }
    return freqman_entry_bandwidths[ modulation ][ bandwidth ] . first ;
}

std::string freqman_entry_get_step_string( freqman_index_t step )
{
    if( step < 0 || (unsigned)step >= freqman_entry_steps . size() )
    {
        return std::string( "" ); // unknown modulation
    }
    return freqman_entry_steps[ step ] . first ;
}

std::string freqman_entry_get_step_string_short( freqman_index_t step )
{
    if( step < 0 || (unsigned)step >= freqman_entry_steps_short . size() )
    {
        return std::string( "" ); // unknown modulation
    }
    return freqman_entry_steps_short[ step ] . first ;
}

int32_t freqman_entry_get_modulation_value( freqman_index_t modulation )
{
    if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
    {
        return -1 ; // unknown modulation
    }
    return freqman_entry_modulations[ modulation ] . second ;
}

int32_t freqman_entry_get_bandwidth_value( freqman_index_t modulation , freqman_index_t bandwidth )
{
    if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
    {
        return -1 ; // unknown modulation
    }
    if( bandwidth < 0 || (unsigned)bandwidth > freqman_entry_bandwidths[ modulation ] . size() )
    {
        return -1 ; // unknown bandwidth for modulation
    }
    return freqman_entry_bandwidths[ modulation ][ bandwidth ] . second ;
}

int32_t freqman_entry_get_step_value( freqman_index_t step )
{
    if( step < 0 || (unsigned)step >= freqman_entry_steps . size() )
    {
        return -1 ; // unknown modulation
    }
    return freqman_entry_steps[ step ] . second ;
}

freqman_index_t freqman_entry_get_modulation_from_str( char *str )
{
    if( !str )
        return -1 ;
    for( freqman_index_t index = 0 ; (unsigned)index < freqman_entry_modulations . size() ; index ++ )
    {
        if( strncmp( freqman_entry_modulations[ index ] . first . c_str() , str , freqman_entry_modulations[ index ] . first . size() ) == 0 )
            return index ;
    }
    return -1 ;	
}

freqman_index_t freqman_entry_get_bandwidth_from_str( freqman_index_t modulation , char *str )
{
    if( !str )
        return -1 ;
    if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
        return -1 ;
    for( freqman_index_t index = 0 ; (unsigned)index < freqman_entry_bandwidths[ modulation ] . size() ; index ++ )
    {
        if( strncmp( freqman_entry_bandwidths[ modulation ][ index ] . first . c_str() , str , freqman_entry_bandwidths[ modulation ][ index ] . first . size() ) == 0 )
            return index ;
    }
    return -1 ;	
}

freqman_index_t freqman_entry_get_step_from_str( char *str )
{
    if( !str )
        return -1 ;
    for( freqman_index_t index = 0 ; (unsigned)index < freqman_entry_steps . size() ; index ++ )
    {
        if( strncmp( freqman_entry_steps[ index ] . first . c_str() , str , freqman_entry_steps[ index ] . first . size() ) == 0 )
            return index ;
    }
    return -1 ;	
}

freqman_index_t freqman_entry_get_step_from_str_short( char *str )
{
    if( !str )
        return -1 ;
    for( freqman_index_t index = 0 ; (unsigned)index < freqman_entry_steps_short . size() ; index ++ )
    {
        if( strncmp( freqman_entry_steps_short[ index ] . first . c_str() , str , freqman_entry_steps_short[ index ] . first . size() ) == 0 )
            return index ;
    }
    return -1 ;	
}
