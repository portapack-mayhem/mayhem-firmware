/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_recon.hpp"
#include "ui_fileman.hpp"
#include "file.hpp"

using namespace portapack;
using portapack::memory::map::backup_ram;

namespace ui {

    void ReconView::audio_output_start()
    {  
        audio::output::start();
        this->on_headphone_volume_changed( (receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99 );
    }

    bool ReconView::check_sd_card()
    {
        return (sd_card::status() == sd_card::Status::Mounted) ? true : false;
    }

    void ReconView::recon_redraw() {

        static int32_t last_db = 999999 ;
        static uint32_t last_nb_match = 999999 ;
        static uint32_t last_freq_lock = 999999 ;
        static size_t last_list_size = 0 ;
        static int8_t last_rssi_min = -127 ;
        static int8_t last_rssi_med = -127 ;
        static int8_t last_rssi_max = -127 ;

        if( last_rssi_min != rssi.get_min() || last_rssi_med != rssi.get_avg() || last_rssi_max != rssi.get_max() ) 
        {
            last_rssi_min = rssi.get_min();
            last_rssi_med = rssi.get_avg();
            last_rssi_max = rssi.get_max();
            freq_stats.set( "RSSI: "+to_string_dec_int( rssi.get_min() )+"/"+to_string_dec_int( rssi.get_avg() )+"/"+to_string_dec_int( rssi.get_max() )+" db" );
        }

        if( last_entry . frequency_a != freq )
        {
            last_entry . frequency_a = freq ;
            big_display.set( "FREQ: "+to_string_short_freq( freq )+" MHz" );
        }

        if( last_db != db  || last_list_size != frequency_list.size() || last_freq_lock != freq_lock || last_nb_match != recon_lock_nb_match ) 
        {
            last_freq_lock = freq ;
            last_list_size = frequency_list.size();
            last_db = db ;
            last_nb_match = recon_lock_nb_match ;
            text_max.set( "/" + to_string_dec_uint( frequency_list.size() ) + " " + to_string_dec_int( db ) + " db " + to_string_dec_uint( freq_lock ) + "/" + to_string_dec_uint( recon_lock_nb_match ) );
            if( freq_lock == 0 ) {
                //NO FREQ LOCK, ONGOING STANDARD SCANNING
                big_display.set_style(&style_white);
                if( !userpause )
                    button_pause.set_text("<PAUSE>");	
                else
                    button_pause.set_text("<RESUME>");	
            }
            else if( freq_lock == 1 && recon_lock_nb_match != 1 )
            {
                //STARTING LOCK FREQ
                big_display.set_style(&style_yellow);
                button_pause.set_text("<SKPLCK>");	
            }
            else if( freq_lock >= recon_lock_nb_match )
            {
                big_display.set_style( &style_green);
                button_pause.set_text("<UNLOCK>");	

                //FREQ IS STRONG: GREEN and recon will pause when on_statistics_update()
                if( (!scanner_mode) && autosave && frequency_list.size() > 0 ) {
                    File recon_file;
                    std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                    std::string frequency_to_add ;

                    freqman_entry entry = frequency_list[ current_index ] ;
                    entry . frequency_a =  freq ;
                    entry . frequency_b =  0 ;
                    entry . modulation =  last_entry.modulation ;
                    entry . bandwidth =  last_entry.bandwidth ;
                    entry . type = SINGLE ;

                    get_freq_string( entry , frequency_to_add );

                    auto result = recon_file.open(freq_file_path);	//First recon if freq is already in txt
                    if (!result.is_valid()) {
                        char one_char[1];		//Read it char by char
                        std::string line;		//and put read line in here
                        bool found=false;
                        for (size_t pointer=0; pointer < recon_file.size();pointer++) {
                            recon_file.seek(pointer);
                            recon_file.read(one_char, 1);
                            if ((int)one_char[0] > 31) {			//ascii space upwards
                                line += one_char[0];				//Add it to the textline
                            }
                            else if (one_char[0] == '\n') {			//New Line
                                if (line.compare(0, frequency_to_add.size(),frequency_to_add) == 0) {
                                    found=true;
                                    break;
                                }
                                line.clear();						//Ready for next textline
                            }
                        }
                        if( !found) {
                            result = recon_file.append(freq_file_path); //Second: append if it is not there
                            if( !result.is_valid() )
                            {
                                recon_file.write_line( frequency_to_add );
                            }
                        }
                    }
                    else {
                        result = recon_file.create( freq_file_path );	//First freq if no output file
                        if( !result.is_valid() )
                        {
                            recon_file.write_line( frequency_to_add );
                        }
                    }
                }
            }
        }
    }

    void ReconView::handle_retune() {
        static int32_t last_index = -1 ;
        static int64_t last_freq = 0 ;
        if( last_freq != freq )
        {
            last_freq = freq ;
            receiver_model.set_tuning_frequency( freq ); // Retune
        }
        if( frequency_list.size() > 0 )
        {
            if( last_entry . modulation != frequency_list[ current_index ] . modulation && frequency_list[ current_index ] . modulation >= 0 )
            {
                last_entry . modulation = frequency_list[ current_index ]. modulation;
                change_mode( frequency_list[ current_index ] . modulation );
            }
            // Set bandwidth if any
            if( last_entry . bandwidth != frequency_list[ current_index ] . bandwidth && frequency_list[ current_index ] . bandwidth >= 0 )
            {
                last_entry . bandwidth = frequency_list[ current_index ]. bandwidth;
                switch( frequency_list[ current_index ].modulation )
                {
                    case AM_MODULATION:
                        receiver_model.set_am_configuration( freq );
                        break ;
                    case NFM_MODULATION:
                        receiver_model.set_nbfm_configuration( freq );
                        break ;
                    case WFM_MODULATION:
                        receiver_model.set_wfm_configuration( freq );
                    default:
                        break ;
                }
                field_bw.set_selected_index( freq );
            }
            if( last_entry . step != frequency_list[ current_index ] . step && frequency_list[ current_index ] . step >= 0 )
            {
                last_entry . step = frequency_list[ current_index ]. step ;
                step = freqman_entry_get_step_value( last_entry . step );
                step_mode.set_selected_index( step );
            }
            if( last_index != current_index )
            {
                last_index = current_index ;
                if(  (int32_t)frequency_list.size() && current_index <  (int32_t)frequency_list.size() && frequency_list[ current_index ] . type == RANGE )
                {
                    if( update_ranges )
                    {
                        button_manual_start.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_a ) );
                        frequency_range.min = frequency_list[ current_index ] . frequency_a ;
                        if( frequency_list[ current_index ] . frequency_b != 0 )
                        {
                            button_manual_end.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_b ) );
                            frequency_range.max = frequency_list[ current_index ] . frequency_b ;
                        }
                        else
                        {
                            button_manual_end.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_a ) );
                            frequency_range.max = frequency_list[ current_index ] . frequency_a ;
                        }
                    }
                }
                text_cycle.set_text( to_string_dec_uint( current_index + 1 , 3 ) );
                if(frequency_list[current_index].description.size() > 0) 
                {
                    switch( frequency_list[current_index].type )
                    {
                        case RANGE:
                            desc_cycle.set( "R: " + frequency_list[current_index].description ); //Show new description
                            break ;
                        case HAMRADIO:
                            desc_cycle.set( "H: " + frequency_list[current_index].description ); //Show new description
                            break ;
                        default:
                        case SINGLE:
                            desc_cycle.set( "S: " + frequency_list[current_index].description ); //Show new description
                            break ;
                    }
                }
                else
                {
                    desc_cycle.set( "...no description..." ); //Show new description
                }
            }
        }
    }


    void ReconView::focus() {
        button_pause.focus();
    }

    ReconView::~ReconView() {

        ReconSetupSaveStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , field_volume.value() );

        // save app settings
        settings.save("recon", &app_settings);

        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }


    ReconView::ReconView( NavigationView& nav) : nav_ { nav } {

        add_children( {
                &labels,
                &field_lna,
                &field_vga,
                &field_rf_amp,
                &field_volume,
                &field_bw,
                &field_squelch,
                &field_wait,
                &field_lock_wait,
                &button_recon_setup,
                &button_scanner_mode,
                &button_looking_glass,
                &file_name,
                &rssi,
                &text_cycle,
                &text_max,
                &desc_cycle,
                &big_display,
                &freq_stats,
                &text_timer,
                &text_ctcss,
                &button_manual_start,
                &button_manual_end,
                &button_manual_recon,
                &field_mode,
                &step_mode,
                &button_pause,
                &button_audio_app,
                &button_add,
                &button_dir,
                &button_restart,
                &button_mic_app,
                &button_remove
        } );

        // Recon directory
        if( check_sd_card() ) {					 // Check to see if SD Card is mounted
            make_new_directory( u"/RECON" );
            sd_card_mounted = true ;
        }
        def_step = 0 ;
        //HELPER: Pre-setting a manual range, based on stored frequency
        rf::Frequency stored_freq = persistent_memory::tuned_frequency();
        receiver_model.set_tuning_frequency( stored_freq );
        if( stored_freq - OneMHz > 0 )
            frequency_range.min = stored_freq - OneMHz ;
        else
            frequency_range.min = 0 ;
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        if( stored_freq + OneMHz < MAX_UFREQ )
            frequency_range.max = stored_freq + OneMHz ;
        else
            frequency_range.max = MAX_UFREQ ;
        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
        // Loading settings
        autostart = persistent_memory::recon_autostart_recon();
        autosave = persistent_memory::recon_autosave_freqs();
        continuous = persistent_memory::recon_continuous();
        filedelete = persistent_memory::recon_clear_output();
        load_freqs = persistent_memory::recon_load_freqs();
        load_ranges = persistent_memory::recon_load_ranges();
        load_hamradios = persistent_memory::recon_load_hamradios();
        update_ranges = persistent_memory::recon_update_ranges_when_recon();

        field_volume.set_value( volume );
        if( sd_card_mounted )
        {
            // load auto common app settings
            auto rc = settings.load("recon", &app_settings);
            if(rc == SETTINGS_OK) {
                field_lna.set_value(app_settings.lna);
                field_vga.set_value(app_settings.vga);
                field_rf_amp.set_value(app_settings.rx_amp);
                receiver_model.set_rf_amp(app_settings.rx_amp);
            }
        }

        button_manual_start.on_select = [this, &nav](ButtonWithEncoder& button) {
            auto new_view = nav_.push<FrequencyKeypadView>(frequency_range.min);
            new_view->on_changed = [this, &button](rf::Frequency f) {
                frequency_range.min = f;
                button_manual_start.set_text(to_string_short_freq(f));
            };
        };

        button_manual_end.on_select = [this, &nav](ButtonWithEncoder& button) {
            auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
            new_view->on_changed = [this, &button](rf::Frequency f) {
                frequency_range.max = f;
                button_manual_end.set_text(to_string_short_freq(f));
            };
        };

        text_cycle.on_select = [this, &nav](ButtonWithEncoder& button) {
            if( frequency_list.size() > 0 )
            {
                auto new_view = nav_.push<FrequencyKeypadView>(current_index);
                new_view->on_changed = [this, &button](rf::Frequency f) {
                    f = f / OneMHz ;
                    if( f >= 1 && f <= frequency_list.size() )
                    {
                        index_stepper = f - 1 - current_index ;
                        freq_lock = 0 ;
                    }
                };
            }
        };

        button_manual_start.on_change = [this]() {
            frequency_range.min = frequency_range.min + button_manual_start.get_encoder_delta() * freqman_entry_get_step_value( def_step );
            if( frequency_range.min < 0 )
            {
                frequency_range.min = 0 ;
            }
            if( frequency_range.min > ( MAX_UFREQ - freqman_entry_get_step_value( def_step ) ) )
            {
                frequency_range.min = MAX_UFREQ - freqman_entry_get_step_value( def_step );
            }
            if( frequency_range.min > (frequency_range.max - freqman_entry_get_step_value( def_step ) ) )
            {
                frequency_range.max = frequency_range.min + freqman_entry_get_step_value( def_step );
                if( frequency_range.max > MAX_UFREQ )
                {
                    frequency_range.min = MAX_UFREQ - freqman_entry_get_step_value( def_step );
                    frequency_range.max = MAX_UFREQ ;
                }
            }
            button_manual_start.set_text( to_string_short_freq(frequency_range.min) );
            button_manual_end.set_text( to_string_short_freq(frequency_range.max) );
            button_manual_start.set_encoder_delta( 0 );
        };

        button_manual_end.on_change = [this]() {
            frequency_range.max = frequency_range.max + button_manual_end.get_encoder_delta() * freqman_entry_get_step_value( def_step );
            if( frequency_range.max < ( freqman_entry_get_step_value( def_step ) + 1 ) )
            {
                frequency_range.max = ( freqman_entry_get_step_value( def_step ) + 1 );
            }
            if( frequency_range.max > MAX_UFREQ )
            {
                frequency_range.max = MAX_UFREQ ;
            }
            if( frequency_range.max < (frequency_range.min + freqman_entry_get_step_value( def_step ) ) )
            {
                frequency_range.min = frequency_range.max - freqman_entry_get_step_value( def_step );
                if( frequency_range.max < ( freqman_entry_get_step_value( def_step ) + 1 ) )
                {
                    frequency_range.min = 1 ;
                    frequency_range.max = ( freqman_entry_get_step_value( def_step ) + 1 ) ;
                }
            }
            button_manual_start.set_text( to_string_short_freq(frequency_range.min) );
            button_manual_end.set_text( to_string_short_freq(frequency_range.max) );
            button_manual_end.set_encoder_delta( 0 );
        };

        text_cycle.on_change = [this]() {
            on_index_delta( text_cycle.get_encoder_delta() );
            text_cycle.set_encoder_delta( 0 );
        };

        button_pause.on_select = [this](ButtonWithEncoder&) {
            if( frequency_list.size() > 0 )
            {
                if( userpause )
                {
                    recon_resume();
                }
                else
                {
                    recon_pause();

                    if( update_ranges )
                    {
                        button_manual_start.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_a ) );
                        frequency_range.min = frequency_list[ current_index ] . frequency_a ;
                        if( frequency_list[ current_index ] . frequency_b != 0 )
                        {
                            button_manual_end.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_b ) );
                            frequency_range.max = frequency_list[ current_index ] . frequency_b ;
                        }
                        else
                        {
                            button_manual_end.set_text( to_string_short_freq( frequency_list[ current_index ] . frequency_a ) );
                            frequency_range.max = frequency_list[ current_index ] . frequency_a ;
                        }
                    }
                }
            }
        };

        button_pause.on_change = [this]() {
            on_stepper_delta( button_pause.get_encoder_delta() );
            button_pause.set_encoder_delta( 0 );
        };

        button_audio_app.on_select = [this](Button&) {
            nav_.pop();
            nav_.push<AnalogAudioView>();
        };

        button_looking_glass.on_select = [this](Button&) {
            nav_.pop();
            nav_.push<GlassView>();
        };


        rssi.set_focusable(true);
        rssi.set_peak( true , 500 );
        rssi.on_select = [this](RSSI&) {
            nav_.pop();
            nav_.push<LevelView>();
        };

        button_mic_app.on_select = [this](Button&) {
            nav_.pop();
            nav_.push<MicTXView>();
        };

        button_remove.on_select = [this](ButtonWithEncoder&) {
            if(frequency_list.size() > 0 )
            {
                if( scanner_mode )
                {
                    if( current_index >= (int32_t)frequency_list.size() )
                    {
                        current_index = frequency_list.size() - 1 ;
                    }
                    frequency_list.erase( frequency_list.begin()+ current_index );
                    if( current_index >= (int32_t)frequency_list.size() )
                    {
                        current_index = frequency_list.size() - 1 ;
                    }
                    if( frequency_list.size() > 0 )
                    {
                        if(frequency_list[current_index].description.size() > 0)
                        {
                            switch( frequency_list[current_index].type )
                            {
                                case RANGE:
                                    desc_cycle.set( "R: " + frequency_list[current_index].description );
                                    break ;
                                case HAMRADIO:
                                    desc_cycle.set( "H: " + frequency_list[current_index].description );
                                    break ;
                                default:
                                case SINGLE:
                                    desc_cycle.set( "S: " + frequency_list[current_index].description );
                                    break ;
                            }
                        }
                        else
                        {
                            desc_cycle.set( "...no description..." );
                        }
                        text_cycle.set_text( to_string_dec_uint( current_index + 1 , 3 ) );

                        File freqman_file;

                        std::string freq_file_path = "FREQMAN/" + output_file + ".TXT";

                        delete_file( freq_file_path );

                        auto result = freqman_file.create( freq_file_path );
                        if ( !result.is_valid() )
                        {
                            for (size_t n = 0; n < frequency_list.size(); n++)
                            {
                                std::string line ;
                                get_freq_string( frequency_list[ n ] , line );
                                freqman_file.write_line( line );
                            }
                        }
                    }
                }
                else // RECON MODE / MANUAL, only remove matching from output
                {
                    File recon_file;
                    File tmp_recon_file;
                    std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                    std::string tmp_freq_file_path = "/FREQMAN/"+output_file+"TMP.TXT" ;
                    std::string frequency_to_add ;

                    freqman_entry entry = frequency_list[ current_index ] ;
                    entry . frequency_a =  freq ;
                    entry . frequency_b =  0 ;
                    entry . modulation =  last_entry.modulation ;
                    entry . bandwidth =  last_entry.bandwidth ;
                    entry . type = SINGLE ;

                    get_freq_string( entry , frequency_to_add );

                    delete_file( tmp_freq_file_path );
                    auto result = tmp_recon_file.create(tmp_freq_file_path);	//First recon if freq is already in txt
                                                                                //
                    if (!result.is_valid()) {
                        bool found=false;
                        result = recon_file.open(freq_file_path);	//First recon if freq is already in txt
                        if (!result.is_valid()) {
                            char one_char[1];		//Read it char by char
                            std::string line;		//and put read line in here
                            for (size_t pointer=0; pointer < recon_file.size();pointer++) {
                                recon_file.seek(pointer);
                                recon_file.read(one_char, 1);
                                if ((int)one_char[0] > 31) {			//ascii space upwards
                                    line += one_char[0];				//Add it to the textline
                                }
                                else if (one_char[0] == '\n') {			//New Line
                                    if (line.compare(0, frequency_to_add.size(),frequency_to_add) == 0) {
                                        found=true;
                                    }
                                    else
                                    {
                                        tmp_recon_file.write_line( frequency_to_add );
                                    }
                                    line.clear();						//Ready for next textline
                                }
                            }
                            if( found)
                            {
                                delete_file( freq_file_path );
                                rename_file( tmp_freq_file_path , freq_file_path );
                            }
                            else
                            {
                                delete_file( tmp_freq_file_path );
                            }
                        }
                    }
                }
                receiver_model.set_tuning_frequency( frequency_list[ current_index ] . frequency_a );	// Retune
            }
            if( frequency_list.size() == 0 )
            {
                text_cycle.set_text( " " );
                desc_cycle.set( "no entries in list" ); //Show new description
                delete_file( "FREQMAN/"+output_file+".TXT" );
            }
            timer = 0 ;
        };

        button_remove.on_change = [this]() {
            on_stepper_delta( button_remove.get_encoder_delta() );
            button_remove.set_encoder_delta( 0 );
        };

        button_manual_recon.on_select = [this](Button&) {
            scanner_mode = false ;
            manual_mode = true ;
            recon_pause();

            if (!frequency_range.min || !frequency_range.max) {
                nav_.display_modal("Error", "Both START and END freqs\nneed a value");
            } else if (frequency_range.min > frequency_range.max) {
                nav_.display_modal("Error", "END freq\nis lower than START");
            } else {
                audio::output::stop();

                frequency_list.clear();
                freqman_entry manual_freq_entry ;

                def_step = step_mode.selected_index(); // max range val

                manual_freq_entry . type = RANGE ;
                manual_freq_entry . description =
                    "R " + to_string_short_freq(frequency_range.min) + ">"
                    + to_string_short_freq(frequency_range.max) + " S" 	// current Manual range
                    + to_string_short_freq(freqman_entry_get_step_value(def_step)).erase(0,1) ; //euquiq: lame kludge to reduce spacing in step freq
                manual_freq_entry . frequency_a = frequency_range.min ; // min range val
                manual_freq_entry . frequency_b = frequency_range.max ; // max range val
                manual_freq_entry . modulation = -1 ;
                manual_freq_entry . bandwidth = -1 ;
                manual_freq_entry . step = def_step ;

                frequency_list . push_back( manual_freq_entry );

                big_display.set_style(&style_white); //Back to white color

                freq_stats.set_style(&style_white);
                freq_stats.set( "0/0/0" );

                text_cycle.set_text( "1" );
                text_max.set( "/1" );
                button_scanner_mode.set_style( &style_white );
                button_scanner_mode.set_text( "MSEARCH" );
                file_name.set_style( &style_white );
                file_name.set( "MANUAL RANGE RECON" );
                desc_cycle.set_style( &style_white );
                desc_cycle.set( "MANUAL RANGE RECON" );

                current_index = 0 ;
                freq = manual_freq_entry . frequency_a ;
                handle_retune();
                recon_redraw();
                recon_resume();
            }
        };

        field_mode.on_change = [this](size_t, OptionsField::value_t v) {
            if( v != -1 )
            {
                change_mode(v);
            }
        };

        button_dir.on_select = [this](Button&) {
            if( fwd )
            {
                fwd = false ;
                button_dir.set_text( "<RW" );
            }
            else
            {
                fwd = true ;
                button_dir.set_text( "FW>" );
            }
            timer = 0 ;
            if ( userpause )				//If user-paused, resume
                recon_resume();
        };

        button_restart.on_select = [this](Button&) {
            if( frequency_list.size() > 0 )
            {
                def_step = step_mode.selected_index();	 //Use def_step from manual selector
                frequency_file_load( true );
                if( fwd )
                {
                    button_dir.set_text( "FW>" );
                }
                else
                {
                    button_dir.set_text( "<RW" );
                }
                recon_resume();
            }
            if( scanner_mode )
            {
                file_name.set_style( &style_red );
                button_scanner_mode.set_style( &style_red );
                button_scanner_mode.set_text( "SCANNER" );
            }
            else
            {
                file_name.set_style( &style_blue );
                button_scanner_mode.set_style( &style_blue );
                button_scanner_mode.set_text( "RECON" );
            }
        };

        button_add.on_select = [this](ButtonWithEncoder&) {  //frequency_list[current_index]
            if( !scanner_mode)
            {
                if( frequency_list.size() && frequency_list.size() && current_index < (int32_t)frequency_list.size() )
                {
                    bool found=false;
                    File recon_file;
                    std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                    std::string frequency_to_add ;

                    freqman_entry entry = frequency_list[ current_index ] ;
                    entry . frequency_a =  freq ;
                    entry . frequency_b =  0 ;
                    entry . modulation =  last_entry.modulation ;
                    entry . bandwidth =  last_entry.bandwidth;
                    entry . type = SINGLE ;

                    get_freq_string( entry , frequency_to_add );

                    auto result = recon_file.open(freq_file_path);	//First recon if freq is already in txt
                    if (!result.is_valid()) {
                        char one_char[1];		//Read it char by char
                        std::string line;		//and put read line in here
                        for (size_t pointer=0; pointer < recon_file.size();pointer++) {
                            recon_file.seek(pointer);
                            recon_file.read(one_char, 1);
                            if ((int)one_char[0] > 31) {			//ascii space upwards
                                line += one_char[0];				//Add it to the textline
                            }
                            else if (one_char[0] == '\n') {			//New Line
                                if (line.compare(0, frequency_to_add.size(),frequency_to_add) == 0) {
                                    found=true;
                                    break;
                                }
                                line.clear();						//Ready for next textline
                            }
                        }
                        if( !found) {
                            result = recon_file.append(freq_file_path); //Second: append if it is not there
                            if( !result.is_valid() )
                            {
                                recon_file.write_line( frequency_to_add );
                            }
                        }
                        if (found) {
                            nav_.display_modal("Error", "Frequency already exists");
                        }
                    }
                    else
                    {
                        auto result = recon_file.create(freq_file_path); //third: create if it is not there
                        if( !result.is_valid() )
                        {
                            recon_file.write_line( frequency_to_add );
                        }
                    }
                }
            }
        };

        button_add.on_change = [this]() {
            on_stepper_delta( button_add.get_encoder_delta() );
            button_add.set_encoder_delta( 0 );
        };

        button_scanner_mode.on_select = [this,&nav](Button&) {
            manual_mode = false ;
            if( scanner_mode )
            {
                scanner_mode = false ;
                button_scanner_mode.set_style( &style_blue );
                button_scanner_mode.set_text( "RECON" );
            }
            else
            {
                scanner_mode = true ;
                button_scanner_mode.set_style( &style_red );
                button_scanner_mode.set_text( "SCANNER" );
            }
            frequency_file_load( true );
            if( autostart )
            {
                recon_resume();
            }
            else
            {
                recon_pause();
            }
        };

        button_recon_setup.on_select = [this,&nav](Button&) {

            audio::output::stop();

            frequency_list.clear();

            auto open_view = nav.push<ReconSetupView>(input_file,output_file,recon_lock_duration,recon_lock_nb_match,recon_match_mode);
            open_view -> on_changed = [this](std::vector<std::string> result) {
                input_file = result[0];
                output_file = result[1];
                recon_lock_duration = strtol( result[2].c_str() , nullptr , 10 );
                recon_lock_nb_match = strtol( result[3].c_str() , nullptr , 10 );
                recon_match_mode	= strtol( result[4].c_str() , nullptr , 10 );

                ReconSetupSaveStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , field_volume.value() );

                autosave = persistent_memory::recon_autosave_freqs();
                autostart = persistent_memory::recon_autostart_recon();
                continuous = persistent_memory::recon_continuous();
                filedelete = persistent_memory::recon_clear_output();
                load_freqs = persistent_memory::recon_load_freqs();
                load_ranges = persistent_memory::recon_load_ranges();
                load_hamradios = persistent_memory::recon_load_hamradios();

                update_ranges = persistent_memory::recon_update_ranges_when_recon();

                field_lock_wait.set_value(recon_lock_duration);

                frequency_file_load( false );
                if( autostart )
                {
                    recon_resume();
                }
                else
                {
                    recon_pause();
                }
                if( userpause != true )
                {
                    recon_resume();
                }
            };
        };

        field_wait.on_change = [this](int32_t v)
        {
            wait = v ;
            if( wait == 0 )
            {
                field_wait.set_style( &style_blue );
            }
            else if( wait >= 500 )
            {
                field_wait.set_style(&style_white);
            }
            else if( wait > -500 && wait < 500 )
            {
                field_wait.set_style( &style_red );
            }
            else if( wait <= -500 )
            {
                field_wait.set_style( &style_green );
            }
        };

        field_lock_wait.on_change = [this](uint32_t v)
        {
            recon_lock_duration = v ;
            if( recon_match_mode == RECON_MATCH_CONTINUOUS )
            {
                if( (v / STATS_UPDATE_INTERVAL ) > recon_lock_nb_match )
                {
                    field_lock_wait.set_style( &style_white );
                }
                else if( (v / STATS_UPDATE_INTERVAL ) == recon_lock_nb_match )
                {
                    field_lock_wait.set_style(&style_yellow);
                }
            }
            else
            {
                field_lock_wait.set_style( &style_white );
            }
        };

        field_squelch.on_change = [this](int32_t v) {
            squelch = v ;
        };

        field_volume.on_change = [this](int32_t v) { 
            this->on_headphone_volume_changed(v);	
        };

        //PRE-CONFIGURATION:

        button_scanner_mode.set_style( &style_blue );
        button_scanner_mode.set_text( "RECON" );
        file_name.set( "=>" );

        //Loading input and output file from settings
        ReconSetupLoadStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , volume );

        field_squelch.set_value( squelch );
        field_wait.set_value(wait);
        field_lock_wait.set_value(recon_lock_duration);
        field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);

        //FILL STEP OPTIONS
        freqman_set_modulation_option( field_mode );
        freqman_set_step_option( step_mode );

        change_mode(AM_MODULATION);	//Start on AM, sets callback field_mode.on_change

        receiver_model.set_tuning_frequency( portapack::persistent_memory::tuned_frequency() );	// Retune

        if( filedelete )
        {
            delete_file( "FREQMAN/"+output_file+".TXT" );
        }

        frequency_file_load( false ); /* do not stop all at start */
        if( autostart )
        {
            recon_resume();
        }
        else
        {
            recon_pause();
        }

        recon_redraw();
    }

    void ReconView::frequency_file_load( bool stop_all_before) {

        (void)(stop_all_before);
        audio::output::stop();

        def_step = step_mode.selected_index();		// use def_step from manual selector
        frequency_list.clear();					 // clear the existing frequency list (expected behavior)
        std::string file_input = input_file ;	   // default recon mode
        if( scanner_mode )
        {
            file_input = output_file ;
            file_name.set_style( &style_red );
            button_scanner_mode.set_style( &style_red );
            button_scanner_mode.set_text( "SCANNER" );
        }
        else
        {
            file_name.set_style( &style_blue );
            button_scanner_mode.set_style( &style_blue );
            button_scanner_mode.set_text( "RECON" );
        }
        file_name.set_style( &style_white );
        desc_cycle.set_style( &style_white );
        if( !load_freqman_file_ex( file_input , frequency_list, load_freqs, load_ranges, load_hamradios ) )
        {	
            file_name.set_style( &style_red );
            desc_cycle.set_style( &style_red );
            desc_cycle.set(" NO " + file_input + ".TXT FILE ..." );
            file_name.set( "=> NO DATA" );
        }
        else
        {
            file_name.set( "=> "+file_input );
            if( frequency_list.size() == 0 )
            {
                file_name.set_style( &style_red );
                desc_cycle.set_style( &style_red );
                desc_cycle.set("/0 no entries in list" );
                file_name.set( "BadOrEmpty "+file_input );
            }
            else
            {
                if( frequency_list.size() > FREQMAN_MAX_PER_FILE )
                {
                    file_name.set_style( &style_yellow );
                    desc_cycle.set_style( &style_yellow );
                }
            }
        }

        if( frequency_list[ 0 ] . step >= 0 )
            step = freqman_entry_get_step_value( frequency_list[ 0 ] . step );
        else
            step = freqman_entry_get_step_value( def_step );

        switch( frequency_list[ 0 ] . type ){
            case SINGLE:
                freq = frequency_list[ 0 ] . frequency_a ;
                break;
            case RANGE:
                minfreq = frequency_list[ 0 ] . frequency_a ;
                maxfreq = frequency_list[ 0 ] . frequency_b ;
                if( fwd )
                {
                    freq = minfreq ;
                }
                else
                {
                    freq = maxfreq ;
                }
                if( frequency_list[ 0 ] . step >= 0 )
                    step = freqman_entry_get_step_value( frequency_list[ 0 ] . step );
                break;
            case HAMRADIO:
                minfreq = frequency_list[ 0 ] . frequency_a ;
                maxfreq = frequency_list[ 0 ] . frequency_b ;
                if( fwd )
                {
                    freq = minfreq ;
                }
                else
                {
                    freq = maxfreq ;
                }
                break;
            default:
                break;
        }
        last_entry . modulation = -1 ;
        last_entry . bandwidth = -1 ;
        last_entry . step = -1 ;

        step_mode.set_selected_index(def_step); //Impose the default step into the manual step selector
        receiver_model.enable();
        receiver_model.set_squelch_level(0);
        std::string description = "...no description..." ;
        if( frequency_list.size() != 0 )
        {
            current_index = 0 ;
            switch( frequency_list[current_index].type )
            {
                case RANGE:
                    description = "R: " + frequency_list[current_index].description ;
                    break ;
                case HAMRADIO:
                    description = "H: " + frequency_list[current_index].description ; 
                    break ;
                default:
                case SINGLE:
                    description = "S: " + frequency_list[current_index].description ;
                    break ;
            }
            text_cycle.set_text( to_string_dec_uint( current_index + 1 , 3 ) );
        }
        else
        {
            text_cycle.set_text( " " );
        }
        desc_cycle.set( description );
        current_index = 0 ;
        handle_retune();
    }

    void ReconView::on_statistics_update(const ChannelStatistics& statistics) {
        // 0 recon , 1 locking , 2 locked
        static int32_t status = -1 ;
        static int32_t last_timer = -1 ;

        db = statistics.max_db ;

        if( !userpause )
        {
            if( !timer )
            {
                status = 0 ;
                continuous_lock = false ;
                freq_lock = 0 ; 
                timer = recon_lock_duration ; 
                big_display.set_style(&style_white);
            }
            if( freq_lock < recon_lock_nb_match ) // LOCKING
            {
                if( status != 1 )
                {
                    status = 1 ;
                    if( wait != 0 )
                    {
                        audio::output::stop();
                    }
                }
                if( db > squelch ) //MATCHING LEVEL
                {
                    continuous_lock = true ;
                    freq_lock ++ ;
                }
                else
                {
                    // continuous, direct cut it if not consecutive match after 1 first match
                    if( recon_match_mode == RECON_MATCH_CONTINUOUS )
                    {
                        if( freq_lock > 0 )
                        {
                            timer = 0 ;
                            continuous_lock = false ;
                        }
                    }
                }
            }
            if( freq_lock >= recon_lock_nb_match ) // LOCKED
            {
                if( status != 2 )
                {
                    continuous_lock = false ;
                    status = 2 ;
                    if( wait != 0 )
                    {
                        audio_output_start();
                    }
                    if( wait >= 0 )
                    {
                        timer = wait ;
                    }
                }
                if( wait < 0 )
                {
                    if( db > squelch ) //MATCHING LEVEL IN STAY X AFTER LAST ACTIVITY
                    {
                        timer = abs( wait );
                    }
                }
            }
        }
        if( last_timer != timer )
        {
            last_timer = timer ;
            text_timer.set( "TIMER: " + to_string_dec_int( timer ) );
        }
        if( timer )
        {
            if( !continuous_lock )
                timer -= STATS_UPDATE_INTERVAL ;
            if( timer < 0 )
            {
                timer = 0 ;
            }
        }
        if( recon || stepper != 0 || index_stepper != 0 ) 
        {
            if( !timer || stepper != 0 || index_stepper != 0 ) 
            {
                //IF THERE IS A FREQUENCY LIST ...
                if (frequency_list.size() > 0 )
                {
                    has_looped = false ;
                    entry_has_changed = false ;

                    if( recon || stepper != 0 || index_stepper != 0 )
                    {
                        if( index_stepper == 0 )
                        {
                            /* we are doing a range */
                            if( frequency_list[ current_index ] . type == RANGE ) {
                                if ( ( fwd && stepper == 0 ) || stepper > 0 ) {
                                    //forward
                                    freq += step ;
                                    // if bigger than range max
                                    if (freq > maxfreq ) {
                                        // when going forward we already know that we can skip a whole range => two values in the list
                                        current_index ++ ;
                                        entry_has_changed = true ;
                                        // looping
                                        if( (uint32_t)current_index >= frequency_list.size() )
                                        {
                                            has_looped = true ;
                                            current_index = 0  ;
                                        }
                                    }
                                }
                                else  if( (!fwd && stepper == 0 ) || stepper < 0 ) {
                                    //reverse
                                    freq -= step ;
                                    // if lower than range min
                                    if (freq < minfreq ) {
                                        // when back we have to check one step at a time
                                        current_index -- ;
                                        entry_has_changed = true ;
                                        // looping
                                        if( current_index < 0 )
                                        {
                                            has_looped = true ;
                                            current_index = frequency_list.size() - 1 ;
                                        }
                                    }
                                }
                            }
                            else if( frequency_list[ current_index ] . type == SINGLE ) {
                                if ( (fwd && stepper == 0 ) || stepper > 0 ) {					//forward
                                    current_index++;
                                    entry_has_changed = true ;
                                    // looping
                                    if( (uint32_t)current_index >= frequency_list.size() )
                                    {
                                        has_looped = true ;
                                        current_index = 0 ;
                                    }
                                }
                                else if( (!fwd && stepper == 0 ) || stepper < 0 ) {
                                    //reverse
                                    current_index--;
                                    entry_has_changed = true ;
                                    // if previous if under the list => go back from end
                                    if( current_index < 0 )
                                    {
                                        has_looped = true ;
                                        current_index =  frequency_list.size() - 1 ;
                                    }
                                }
                            }
                            else if( frequency_list[ current_index ] . type == HAMRADIO )
                            {
                                if ( (fwd && stepper == 0 ) || stepper > 0 ) {					//forward
                                    if( ( minfreq != maxfreq ) && freq == minfreq )
                                    {
                                        freq = maxfreq ;
                                    }
                                    else
                                    {
                                        current_index++;
                                        entry_has_changed = true ;
                                        // looping
                                        if( (uint32_t)current_index >= frequency_list.size() )
                                        {
                                            has_looped = true ;
                                            current_index = 0 ;
                                        }
                                    }
                                }
                                else if( (!fwd  && stepper == 0 ) || stepper < 0 ) {
                                    //reverse
                                    if( ( minfreq != maxfreq ) && freq == maxfreq )
                                    {
                                        freq = minfreq ;
                                    }
                                    else
                                    {
                                        current_index--;
                                        entry_has_changed = true ;
                                        // if previous if under the list => go back from end
                                        if( current_index < 0 )
                                        {
                                            has_looped = true ;
                                            current_index =  frequency_list.size() - 1 ;
                                        }
                                    }
                                }
                            }
                            // set index to boundary if !continuous
                            if( has_looped && !continuous )
                            {
                                entry_has_changed = true ;
                                /* prepare values for the next run, when user will resume */
                                if( ( fwd && stepper == 0 ) || stepper > 0 )
                                {
                                    current_index = 0 ;
                                }
                                else if( ( !fwd && stepper == 0 ) || stepper < 0 )
                                {
                                    current_index = frequency_list.size() - 1 ;
                                }
                            }
                        }
                        else
                        {
                            current_index += index_stepper ;
                            if( current_index < 0 )
                                current_index += frequency_list.size();
                            if( (unsigned)current_index >= frequency_list.size() )
                                current_index -= frequency_list.size() ;

                            entry_has_changed = true ;

                            if( !recon ) // for some motive, audio output gets stopped.
                                audio_output_start();
                        }
                        // reload entry if changed
                        if( entry_has_changed ){
                            timer = 0 ;
                            switch( frequency_list[ current_index ] . type ){
                                case SINGLE:
                                    freq = frequency_list[ current_index ] . frequency_a ;
                                    break;
                                case RANGE:
                                    minfreq = frequency_list[ current_index ] . frequency_a ;
                                    maxfreq = frequency_list[ current_index ] . frequency_b ;
                                    if( ( fwd && !stepper && !index_stepper ) || stepper > 0 || index_stepper > 0 )
                                    {
                                        freq = minfreq ;
                                    }
                                    else if( ( !fwd && !stepper && !index_stepper ) || stepper < 0 || index_stepper < 0 )
                                    {
                                        freq = maxfreq ;
                                    }
                                    break;
                                case HAMRADIO:
                                    minfreq = frequency_list[ current_index ] . frequency_a ;
                                    maxfreq = frequency_list[ current_index ] . frequency_b ;
                                    if( ( fwd && !stepper && !index_stepper ) || stepper > 0 || index_stepper > 0 )
                                    {
                                        freq = minfreq ;
                                    }
                                    else if( ( !fwd && !stepper && !index_stepper ) || stepper < 0 || index_stepper < 0 )
                                    {
                                        freq = maxfreq ;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            handle_retune();
                        } 
                        // send a pause message with the right freq
                        if( has_looped && !continuous )
                        {
                            recon_pause();
                        }
                        index_stepper =  0 ;
                        if( stepper < 0 ) stepper ++ ;
                        if( stepper > 0 ) stepper -- ;
                    } // if( recon || stepper != 0 || index_stepper != 0 )
                }//if (frequency_list.size() > 0 )		
            } /* on_statistic_updates */
        }
        recon_redraw();
    }

    void ReconView::recon_pause() {
        timer = 0 ; 	 				    // Will trigger a recon_resume() on_statistics_update, also advancing to next freq.
        freq_lock = 0 ;
        userpause = true;
        continuous_lock=false;
        recon = false ; 

        audio_output_start();

        big_display.set_style(&style_white); 
        button_pause.set_text("<RESUME>");	//PAUSED, show resume
    }

    void ReconView::recon_resume() {
        timer = 0 ;
        freq_lock = 0 ;
        userpause = false;                 
        continuous_lock = false ;
        recon = true ;	   

        audio::output::stop();

        big_display.set_style(&style_white); 
        button_pause.set_text("<PAUSE>"); 
    }

    void ReconView::on_index_delta(int32_t v)
    {
        if( v > 0 )
        {
            fwd = true ;
            button_dir.set_text( "FW>" );
        }
        else
        {

            fwd = false ;
            button_dir.set_text( "<RW" );
        }
        if( frequency_list.size() > 0 )
            index_stepper = v ;
    }

    void ReconView::on_stepper_delta(int32_t v)
    {
        if( v > 0 )
        {
            fwd = true ;
            button_dir.set_text( "FW>" );
        }
        else
        {

            fwd = false ;
            button_dir.set_text( "<RW" );
        }
        if( frequency_list.size() > 0 )
            stepper = v ;
    }

    void ReconView::on_headphone_volume_changed(int32_t v) {
        const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
        receiver_model.set_headphone_volume(new_volume);
    }

    size_t ReconView::change_mode( freqman_index_t new_mod ) { 

        field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n;	};

        // dirty ui hack so we can use change_mode into field_mode itself
        field_mode.on_change = [this](size_t n, OptionsField::value_t) { (void)n;	};
        field_mode.set_selected_index( new_mod );
        field_mode.on_change = [this](size_t, OptionsField::value_t v) {
            if( v != -1 )
            {
                change_mode(v);
            }
        };

        receiver_model.disable();
        baseband::shutdown();

        switch( new_mod ) {
            case AM_MODULATION:
                freqman_set_bandwidth_option( new_mod , field_bw );
                //bw DSB (0) default
                field_bw.set_selected_index(0);
                baseband::run_image(portapack::spi_flash::image_tag_am_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
                receiver_model.set_am_configuration(field_bw.selected_index());
                field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_am_configuration(n);	};
                receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000);
                text_ctcss.set("             ");
                break;
            case NFM_MODULATION:
                freqman_set_bandwidth_option( new_mod , field_bw );
                //bw 16k (2) default
                field_bw.set_selected_index(2);
                baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
                receiver_model.set_nbfm_configuration(field_bw.selected_index());
                field_bw.on_change = [this](size_t n, OptionsField::value_t) { 	receiver_model.set_nbfm_configuration(n); };
                receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000);
                break;
            case WFM_MODULATION:
                freqman_set_bandwidth_option( new_mod , field_bw );
                //bw 200k (0) default
                field_bw.set_selected_index(0);
                baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
                receiver_model.set_wfm_configuration(field_bw.selected_index());
                field_bw.on_change = [this](size_t n, OptionsField::value_t) {	receiver_model.set_wfm_configuration(n); };
                receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000);
                text_ctcss.set("             ");
                break;
            default:
                break;
        }

        if( !recon ) // for some motive, audio output gets stopped.
            audio_output_start();

        receiver_model.enable();

        return freqman_entry_get_step_value( def_step );
    }

    void ReconView::handle_coded_squelch(const uint32_t value) {
        static int32_t last_idx = -1 ;

        float diff, min_diff = value;
        size_t min_idx { 0 };
        size_t c;

        if( field_mode.selected_index() != NFM_MODULATION )
        {
            text_ctcss.set("             ");
            return ;
        }

        // Find nearest match
        for (c = 0; c < tone_keys.size(); c++) {
            diff = abs(((float)value / 100.0) - tone_keys[c].second);
            if (diff < min_diff) {
                min_idx = c;
                min_diff = diff;
            }
        }

        // Arbitrary confidence threshold
        if( last_idx < 0 || (unsigned)last_idx != min_idx )
        {
            last_idx = min_idx ;
            if (min_diff < 40)
                text_ctcss.set("T: "+tone_keys[min_idx].first);
            else
                text_ctcss.set("             ");
        }
    }
} /* namespace ui */
