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
#include "ui_recon_settings.hpp"
#include "file.hpp"

// Id's for messages between ReconThread and ReconView
#define MSG_RECON_PAUSE 9999                    // for handle_retune to know that recon thread triggered a pause. f is not important with that message
#define MSG_RECON_SET_MODULATION 10000          // for handle_retune to know that recon thread triggered a modulation change. f is the index of the modulation
#define MSG_RECON_SET_BANDWIDTH 20000           // for handle_retune to know that recon thread triggered a bandwidth change. f is the new bandwidth value index for current modulation
#define MSG_RECON_SET_STEP 30000                // for handle_retune to know that recon thread triggered a bandwidth change. f is the new bandwidth value index for current modulation
#define MSG_RECON_SET_RECEIVER_BANDWIDTH 40000 	// for handle_retune to know that recon thread triggered a receiver bandwidth change. f is the new bandwidth in hz
#define MSG_RECON_SET_RECEIVER_SAMPLERATE 50000 // for handle_retune to know that recon thread triggered a receiver samplerate change. f is the new samplerate in hz/s


using namespace portapack;
using portapack::memory::map::backup_ram;

namespace ui {

    ReconThread::ReconThread( freqman_db *database ) : frequency_list_ { *database } {
        thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ReconThread::static_fn, this );
    }

    ReconThread::~ReconThread() {
        stop();
    }

    void ReconThread::stop() {
        if( thread ) {
            chThdTerminate(thread);
            chThdWait(thread);
            thread = nullptr;
        }
    }


    void ReconThread::set_recon(const bool v) {
        _recon = v;
    }

    void ReconThread::set_freq_delete(const bool v) {
        _freq_delete = v;
    }

    void ReconThread::set_stepper( const int64_t v){
        _stepper = v;
    }

    void ReconThread::set_lock_duration( const uint32_t v ){
        _lock_duration = v;
    }

    uint32_t ReconThread::get_lock_duration() {
        return _lock_duration ;
    }

    void ReconThread::set_lock_nb_match( const uint32_t v ){
        _lock_nb_match = v;
    }

    uint32_t ReconThread::get_lock_nb_match() {
        return _lock_nb_match ;
    }

    bool ReconThread::is_recon() {
        return _recon;
    }

    void ReconThread::set_freq_lock(const uint32_t v) {
        _freq_lock = v;
    }
    uint32_t ReconThread::is_freq_lock() {
        return _freq_lock;
    }
    int64_t ReconThread::get_current_freq() {
        return freq ;
    }

    void ReconThread::change_recon_direction() {
        _fwd = !_fwd;
        //        chThdSleepMilliseconds(300);	//Give some pause after reversing recon direction
    }

    bool ReconThread::get_recon_direction() {
        return _fwd ;
    }
    void ReconThread::set_recon_direction( const bool v) {
        _fwd = v ;
    }

    void ReconThread::set_continuous(const bool v) {
        _continuous = v;
    }

    freqman_index_t ReconThread::get_current_modulation() {
        return last_entry.modulation ;
    }

    freqman_index_t ReconThread::get_current_bandwidth() {
        return last_entry.bandwidth ;
    }


    void ReconThread::set_default_modulation( freqman_index_t index ) {
        def_modulation = index ;
    }

    void ReconThread::set_default_bandwidth( freqman_index_t index ) {
        def_bandwidth = index ;
    }
    void ReconThread::set_default_step( freqman_index_t index ) {
        def_step = index ;
    }
    void ReconThread::set_freq_index( int16_t index ) {
        frequency_index = index ;
    }
    int16_t ReconThread::get_freq_index() {
        return frequency_index ;
    }

    msg_t ReconThread::static_fn( void* arg ) {
        auto obj = static_cast<ReconThread*>(arg);
        obj->run();
        return 0;
    }

    void ReconThread::run() {

        if (frequency_list_.size() > 0 )	{			//IF THERE IS A FREQUENCY LIST ...
            int64_t minfreq = 0 ;
            int64_t maxfreq = 0 ;
            bool has_looped = false ;
            bool entry_has_changed = false ;
            RetuneMessage message { };

            if( frequency_list_[ 0 ] . step >= 0 )
                step = freqman_entry_get_step_value( frequency_list_[ 0 ] . step );
            else
                step = freqman_entry_get_step_value( def_step );

            switch( frequency_list_[ 0 ] . type ){
                case SINGLE:
                    freq = frequency_list_[ 0 ] . frequency_a ;
                    break;
                case RANGE:
                    minfreq = frequency_list_[ 0 ] . frequency_a ;
                    maxfreq = frequency_list_[ 0 ] . frequency_b ;
                    if( _fwd )
                    {
                        freq = minfreq ;
                    }
                    else
                    {
                        freq = maxfreq ;
                    }
                    if( frequency_list_[ 0 ] . step >= 0 )
                        step = freqman_entry_get_step_value( frequency_list_[ 0 ] . step );
                    break;
                case HAMRADIO:
                    minfreq = frequency_list_[ 0 ] . frequency_a ;
                    maxfreq = frequency_list_[ 0 ] . frequency_b ;
                    if( _fwd )
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
            bool restart_recon = false;					//Flag whenever scanning is restarting after a pause

            while( !chThdShouldTerminate() && frequency_list_.size() > 0 ) {
                if( !_freq_delete )
                {
                    if( _recon || _stepper != 0 )
                    {
                        if( _freq_lock == 0 || _stepper != 0 )  //normal recon (not performing freq_lock)
                        {
                            if( !restart_recon || _stepper != 0 )
                            {
                                has_looped = false ;
                                entry_has_changed = false ;

                                if( last_entry . frequency_a != freq )
                                {
                                    last_entry . frequency_a = freq ;
                                    receiver_model.set_tuning_frequency( freq );	// Retune
                                    message.freq = freq ;
                                    message.range = frequency_index ;
                                    EventDispatcher::send_message(message);
                                }

                                // Set modulation if any
                                if( last_entry . modulation != frequency_list_[ frequency_index ] . modulation && frequency_list_[ frequency_index ] . modulation >= 0 )
                                {
                                    last_entry . modulation = frequency_list_[ frequency_index ]. modulation;
                                    message.freq  = last_entry . modulation  ;
                                    message.range = MSG_RECON_SET_MODULATION ;
                                    EventDispatcher::send_message(message);
                                }
                                // Set bandwidth if any
                                if( last_entry . bandwidth != frequency_list_[ frequency_index ] . bandwidth && frequency_list_[ frequency_index ] . bandwidth >= 0 )
                                {
                                    last_entry . bandwidth = frequency_list_[ frequency_index ]. bandwidth;
                                    message.freq  = last_entry . bandwidth  ;
                                    message.range = MSG_RECON_SET_BANDWIDTH ;
                                    EventDispatcher::send_message(message);
                                }
                                if( last_entry . step != frequency_list_[ frequency_index ] . step && frequency_list_[ frequency_index ] . step >= 0 )
                                {
                                    last_entry . step = frequency_list_[ frequency_index ]. step ;
                                    message.freq  = last_entry . step ;
                                    message.range = MSG_RECON_SET_STEP ;
                                    EventDispatcher::send_message(message);
                                    step = freqman_entry_get_step_value( last_entry . step );
                                }

                                /* we are doing a range */
                                if( frequency_list_[ frequency_index ] . type == RANGE ) {

                                    if ( ( _fwd && _stepper == 0 ) || _stepper > 0 ) {
                                        //forward
                                        freq += step ;
                                        // if bigger than range max
                                        if (freq > maxfreq ) {
                                            // when going forward we already know that we can skip a whole range => two values in the list
                                            frequency_index ++ ;
                                            entry_has_changed = true ;
                                            // looping
                                            if( (uint32_t)frequency_index >= frequency_list_.size() )
                                            {
                                                has_looped = true ;
                                                frequency_index = 0  ;
                                            }
                                        }
                                    }
                                    else  if( (!_fwd && _stepper == 0 ) || _stepper < 0 ) {
                                        //reverse
                                        freq -= step ;
                                        // if lower than range min
                                        if (freq < minfreq ) {
                                            // when back we have to check one step at a time
                                            frequency_index -- ;
                                            entry_has_changed = true ;
                                            // looping
                                            if( frequency_index < 0 )
                                            {
                                                has_looped = true ;
                                                frequency_index = frequency_list_.size() - 1 ;
                                            }
                                        }
                                    }
                                }
                                else if( frequency_list_[ frequency_index ] . type == SINGLE ) {
                                    if ( (_fwd && _stepper == 0 ) || _stepper > 0 ) {					//forward
                                        frequency_index++;
                                        entry_has_changed = true ;
                                        // looping
                                        if( (uint32_t)frequency_index >= frequency_list_.size() )
                                        {
                                            has_looped = true ;
                                            frequency_index = 0 ;
                                        }
                                    }
                                    else if( (!_fwd && _stepper == 0 ) || _stepper < 0 ) {
                                        //reverse
                                        frequency_index--;
                                        entry_has_changed = true ;
                                        // if previous if under the list => go back from end
                                        if( frequency_index < 0 )
                                        {
                                            has_looped = true ;
                                            frequency_index =  frequency_list_.size() - 1 ;
                                        }
                                    }
                                }
                                else if( frequency_list_[ frequency_index ] . type == HAMRADIO )
                                {
                                    if ( (_fwd && _stepper == 0 ) || _stepper > 0 ) {					//forward
                                        if( ( minfreq != maxfreq ) && freq == minfreq )
                                        {
                                            freq = maxfreq ;
                                        }
                                        else
                                        {
                                            frequency_index++;
                                            entry_has_changed = true ;
                                            // looping
                                            if( (uint32_t)frequency_index >= frequency_list_.size() )
                                            {
                                                has_looped = true ;
                                                frequency_index = 0 ;
                                            }
                                        }
                                    }
                                    else if( (!_fwd  && _stepper == 0 ) || _stepper < 0 ) {
                                        //reverse
                                        if( ( minfreq != maxfreq ) && freq == maxfreq )
                                        {
                                            freq = minfreq ;
                                        }
                                        else
                                        {
                                            frequency_index--;
                                            entry_has_changed = true ;
                                            // if previous if under the list => go back from end
                                            if( frequency_index < 0 )
                                            {
                                                has_looped = true ;
                                                frequency_index =  frequency_list_.size() - 1 ;
                                            }
                                        }
                                    }
                                }
                                // set index to boundary if !continuous
                                if( has_looped && !_continuous )
                                {
                                    entry_has_changed = true ;
                                    /* prepare values for the next run, when user will resume */
                                    if( ( _fwd && _stepper == 0 ) || _stepper > 0 )
                                    {
                                        frequency_index = 0 ;
                                    }
                                    else if( ( !_fwd && _stepper == 0 ) || _stepper < 0 )
                                    {
                                        frequency_index = frequency_list_.size() - 1 ;
                                    }
                                }

                            }
                            else
                            {
                                restart_recon = false ;
                            }
                        }
                        // reload entry if changed
                        if( entry_has_changed ){
                            switch( frequency_list_[ frequency_index ] . type ){
                                case SINGLE:
                                    freq = frequency_list_[ frequency_index ] . frequency_a ;
                                    break;
                                case RANGE:
                                    minfreq = frequency_list_[ frequency_index ] . frequency_a ;
                                    maxfreq = frequency_list_[ frequency_index ] . frequency_b ;
                                    if( ( _fwd && _stepper == 0 ) || _stepper > 0 )
                                    {
                                        freq = minfreq ;
                                    }
                                    else if( ( !_fwd && _stepper == 0 ) || _stepper < 0 )
                                    {
                                        freq = maxfreq ;
                                    }
                                    break;
                                case HAMRADIO:
                                    minfreq = frequency_list_[ frequency_index ] . frequency_a ;
                                    maxfreq = frequency_list_[ frequency_index ] . frequency_b ;
                                    if(  ( _fwd && _stepper == 0 ) || _stepper > 0 )
                                    {
                                        freq = minfreq ;
                                    }
                                    else if( ( !_fwd && _stepper == 0 ) || _stepper < 0 )
                                    {
                                        freq = maxfreq ;
                                    }
                                    break;
                                default:
                                    break;
                            }

                        } 

                        // send a pause message with the right freq
                        if( has_looped && !_continuous )
                        {
                            // signal pause to handle_retune
                            receiver_model.set_tuning_frequency( freq );	// Retune to actual freq
                            message.freq = freq ;
                            message.range = MSG_RECON_PAUSE ;
                            EventDispatcher::send_message(message);
                        }
                        if( _stepper < 0 ) _stepper ++ ;
                        if( _stepper > 0 ) _stepper -- ;
                    }
                    else
                    {
                        restart_recon = true ;
                    }
                }
                chThdSleepMilliseconds( _lock_duration );	//Needed to (eventually) stabilize the receiver into new freq
            }
        }
    }

    bool ReconView::check_sd_card()
    {
        return (sd_card::status() == sd_card::Status::Mounted) ? true : false;
    }

    void ReconView::set_display_freq( int64_t freq )
    {
        int64_t freqMHz = ( freq / 1000000 );
        int64_t freqMhzFloatingPart = ( freq - ( 1000000 * freqMHz ) ) / 100 ;
        big_display.set( "FREQ: "+to_string_dec_int( freqMHz )+"."+to_string_dec_int( freqMhzFloatingPart )+" MHz" );
    }

    void ReconView::handle_retune( int64_t freq , uint32_t index ) {

        static int64_t last_freq = 0 ;
        static uint32_t last_index = 999999 ;

        switch( index ) {
            case MSG_RECON_PAUSE:
                user_pause();
                if( update_ranges && current_index < frequency_list.size() )
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
                return ;
                break ;
            case MSG_RECON_SET_MODULATION :
                receiver_model.disable();
                baseband::shutdown();
                change_mode( freq );
                if ( !recon_thread->is_recon() ) 						//for some motive, audio output gets stopped.
                    audio::output::start();								    //So if recon was stopped we resume audio
                receiver_model.enable();
                field_mode.set_selected_index( freq );
                return ;
                break ;
            case MSG_RECON_SET_BANDWIDTH:
                switch( recon_thread->get_current_modulation() )
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
                return ;
                break ;
            case MSG_RECON_SET_STEP:
                step_mode.set_selected_index( freq );
                return ;
                break ;
        }

        if( last_index != index )
        {
            last_index = index ;
            current_index = index ;
            if(  frequency_list.size() && index <  frequency_list.size() && frequency_list[ index ] . type == RANGE )
            {
                if( update_ranges )
                {
                    button_manual_start.set_text( to_string_short_freq( frequency_list[ index ] . frequency_a ) );
                    frequency_range.min = frequency_list[ index ] . frequency_a ;
                    if( frequency_list[ index ] . frequency_b != 0 )
                    {
                        button_manual_end.set_text( to_string_short_freq( frequency_list[ index ] . frequency_b ) );
                        frequency_range.max = frequency_list[ index ] . frequency_b ;
                    }
                    else
                    {
                        button_manual_end.set_text( to_string_short_freq( frequency_list[ index ] . frequency_a ) );
                        frequency_range.max = frequency_list[ index ] . frequency_a ;
                    }
                }
            }
        }

        uint32_t freq_lock = recon_thread->is_freq_lock();

        if( freq_lock == 0 ) {
            //NO FREQ LOCK, ONGOING STANDARD SCANNING
            if( index < 1000 && index < frequency_list.size() )
            {
                text_cycle.set( to_string_dec_uint( index + 1 , 3 ) );
                if(frequency_list[index].description.size() > 0) desc_cycle.set( frequency_list[index].description );	//Show new description
            }
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
        else if( index < 1000 && freq_lock >= recon_thread -> get_lock_nb_match() )
        {
            big_display.set_style( &style_green);
            button_pause.set_text("<UNLOCK>");	

            //FREQ IS STRONG: GREEN and recon will pause when on_statistics_update()
            if( (!scanner_mode) && autosave && last_freq != freq ) {
                File recon_file;
                std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                std::string frequency_to_add ;

                freqman_entry entry = frequency_list[ current_index ] ;
                entry . frequency_a =  recon_thread->get_current_freq();
                entry . frequency_b =  recon_thread->get_current_freq();
                entry . modulation =  recon_thread->get_current_modulation();
                entry . bandwidth =  recon_thread->get_current_bandwidth();
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
            last_freq = freq ;
        }
        set_display_freq( freq );	//UPDATE the big Freq after 0, 1 or recon_lock_nb_match (at least, for color synching)
    }


    void ReconView::focus() {
        button_pause.focus();
    }

    ReconView::~ReconView() {

        ReconSetupSaveStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , recon_lock_duration , field_volume.value() );

        // save app settings
        settings.save("recon", &app_settings);

        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

    void ReconView::show_max( bool refresh_display ) {		//show total number of freqs to recon
        static int32_t last_db = 999999 ;
        static int32_t last_nb_match = 999999 ;
        static int32_t last_timer = -1 ;
        if( recon_thread && frequency_list.size() > 0 )
        {
            int32_t nb_match = recon_thread->is_freq_lock();
            if( last_db != db )
            {
                last_db = db ;
                refresh_display = true ;
            }
            if( last_nb_match != nb_match )
            {
                last_nb_match = nb_match ;
                refresh_display = true ;
            }
            if( last_timer != timer )
            {
                last_timer = timer ;
                refresh_display = true ;
            }
            if( refresh_display )
            {
                text_max.set( "/" + to_string_dec_uint( frequency_list.size() ) + " " + to_string_dec_int( db ) + " db " + to_string_dec_uint( nb_match ) + "/" + to_string_dec_uint( recon_thread->get_lock_nb_match() ) );
                freq_stats.set( "RSSI: "+to_string_dec_int( rssi.get_min() )+"/"+to_string_dec_int( rssi.get_avg() )+"/"+to_string_dec_int( rssi.get_max() )+" db" );
                text_timer.set( "TIMER: " + to_string_dec_int( timer ) );
            }
        }
        else
        {
            if( refresh_display )
            {
                text_max.set( " " );
                freq_stats.set( "RSSI: " +to_string_dec_int( rssi.get_min() )+"/"+to_string_dec_int( rssi.get_avg() )+"/"+to_string_dec_int( rssi.get_max() )+" db" );
                text_timer.set( "TIMER: " + to_string_dec_int( timer ) );
            }
        }
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
        if( check_sd_card() ) {                     // Check to see if SD Card is mounted
            make_new_directory( u"/RECON" );
            sd_card_mounted = true ;
        }

        def_step = 0 ;
        change_mode(AM_MODULATION);	//Start on AM
        field_mode.set_by_value(AM_MODULATION);	//Reflect the mode into the manual selector

        //HELPER: Pre-setting a manual range, based on stored frequency
        rf::Frequency stored_freq = persistent_memory::tuned_frequency();
        receiver_model.set_tuning_frequency( stored_freq );
        frequency_range.min = stored_freq - 1000000;
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        frequency_range.max = stored_freq + 1000000;
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

        //Loading input and output file from settings
        ReconSetupLoadStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , recon_lock_duration , volume );

        field_volume.set_value( volume );

        // load auto common app settings
        if( sd_card_mounted )
        {
            auto rc = settings.load("recon", &app_settings);
            if(rc == SETTINGS_OK) {
                field_lna.set_value(app_settings.lna);
                field_vga.set_value(app_settings.vga);
                field_rf_amp.set_value(app_settings.rx_amp);
                receiver_model.set_rf_amp(app_settings.rx_amp);
            }
        }
        button_scanner_mode.set_style( &style_blue );
        button_scanner_mode.set_text( "RECON" );
        file_name.set( "USE:" );

        field_squelch.set_value( squelch );

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

        button_manual_start.on_change = [this]() {
            frequency_range.min = frequency_range.min + button_manual_start.get_encoder_delta() * freqman_entry_get_step_value( def_step );
            if( frequency_range.min < 1 )
            {
                frequency_range.min = 1 ;
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



        button_pause.on_select = [this](ButtonWithEncoder&) {
            if( recon_thread && frequency_list.size() > 0 )
            {
                if( continuous_lock )
                {
                    if( fwd )
                    {
                        recon_thread-> set_stepper( 1 );
                    }
                    else
                    {
                        recon_thread-> set_stepper( -1 );
                    }
                    timer = 0 ;
                    recon_resume();
                    button_pause.set_text("<PAUSE>");		//Show button for non continuous stop
                    continuous_lock = false ;
                }
                else
                {
                    if( userpause )
                    {
                        user_resume();
                    }
                    else
                    {
                        user_pause();

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
            }
            button_manual_start.set_encoder_delta( 0 );
        };

        button_pause.on_change = [this]() {
            if( recon_thread && frequency_list.size() > 0 )
            {
                if( button_pause.get_encoder_delta() > 0 )
                {
                    fwd = true ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "FW>" );
                    recon_thread->set_freq_lock( 0 );
                    recon_thread-> set_stepper( 1 );
                }
                else if( button_pause.get_encoder_delta() < 0 )
                {
                    fwd = false ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "<RW" );
                    recon_thread->set_freq_lock( 0 );
                    recon_thread-> set_stepper( -1 );
                }
            }
            button_pause.set_encoder_delta( 0 );
        };

        button_audio_app.on_select = [this](Button&) {
            recon_thread->stop();
            nav_.pop();
            nav_.push<AnalogAudioView>();
        };

        button_looking_glass.on_select = [this](Button&) {
            recon_thread->stop();
            nav_.pop();
            nav_.push<GlassView>();
        };

        
        rssi.set_focusable(true);
        rssi.set_peak( true , 500 );
        rssi.on_select = [this](RSSI&) {
            recon_thread->stop();
            nav_.pop();
            nav_.push<LevelView>();
        };

        button_mic_app.on_select = [this](Button&) {
            recon_thread->stop();
            nav_.pop();
            nav_.push<MicTXView>();
        };

        button_remove.on_select = [this](ButtonWithEncoder&) {
            bool previous_is_recon = false ;
            bool previous_userpause = userpause ;
            if( recon_thread )
            {
                previous_is_recon = recon_thread->is_recon();
                recon_thread->set_recon(false);
                recon_thread->set_freq_delete(true);
                chThdSleepMilliseconds( recon_lock_duration ); // give some time to Thread::Run to pause
            }
            if( scanner_mode )
            {
                if(frequency_list.size() > 0 )
                {
                    if( current_index >= frequency_list.size() )
                    {
                        current_index = frequency_list.size() - 1 ;
                    }
                    frequency_list.erase( frequency_list.begin()+ current_index );
                    if( current_index >= frequency_list.size() )
                    {
                        current_index = frequency_list.size() - 1 ;
                    }
                    if( frequency_list.size() > 0 )
                    {
                        recon_thread->set_freq_index( current_index );
                        if(frequency_list[current_index].description.size() > 0)
                        {
                            desc_cycle.set( frequency_list[current_index].description ); //Show new description
                        }
                        else
                        {
                            desc_cycle.set( "no description" ); //Show new description
                            show_max( true );					//UPDATE new list size on screen
                        }
                        text_cycle.set( to_string_dec_uint( current_index + 1 , 3 ) );

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
            }
            else // RECON MODE / MANUAL, only remove matching from output
            {
                File recon_file;
                File tmp_recon_file;
                std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                std::string tmp_freq_file_path = "/FREQMAN/"+output_file+"TMP.TXT" ;
                std::string frequency_to_add ;

                freqman_entry entry = frequency_list[ current_index ] ;
                entry . frequency_a =  recon_thread->get_current_freq();
                entry . frequency_b =  recon_thread->get_current_freq();
                entry . modulation =  recon_thread->get_current_modulation();
                entry . bandwidth =  recon_thread->get_current_bandwidth();
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
            if( frequency_list.size() == 0 )
            {
                text_cycle.set( " " );
                desc_cycle.set( "no entries in list" ); //Show new description
                show_max( true );								//UPDATE new list size on screen
                delete_file( "FREQMAN/"+output_file+".TXT" );
            }

	    if( recon_thread )
	    {
		timer = 0 ;
		
		recon_thread->set_freq_index( current_index );
		
		RetuneMessage message { };
		receiver_model.set_tuning_frequency( frequency_list[ current_index ] . frequency_a );	// Retune
		message.freq = frequency_list[ current_index ] . frequency_a ;
		message.range = current_index ;
		EventDispatcher::send_message(message);
		
		chThdSleepMilliseconds( recon_lock_duration ); // give some time to Thread::Run to pause
							       
		if( previous_userpause )
		{
		    user_pause();
		}
		else
		{
		    user_resume();
		}

		recon_thread->set_freq_delete(false);
		recon_thread->set_recon( previous_is_recon );
	    }
        };

        button_remove.on_change = [this]() {
            if( recon_thread && frequency_list.size() > 0 )
            {
                timer = 0 ;
                if( button_remove.get_encoder_delta() > 0 )
                {
                    fwd = true ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "FW>" );
                    recon_thread->set_freq_lock( 0 );
                    recon_thread-> set_stepper( 1 );
                }
                else if( button_remove.get_encoder_delta() < 0 )
                {
                    fwd = false ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "<RW" );
                    recon_thread->set_freq_lock( 0 );
                    recon_thread-> set_stepper( -1 );
                }
            }
            button_remove.set_encoder_delta( 0 );
        };

        button_manual_recon.on_select = [this](Button&) {
            scanner_mode = false ;
            manual_mode = true ;
            if (!frequency_range.min || !frequency_range.max) {
                nav_.display_modal("Error", "Both START and END freqs\nneed a value");
            } else if (frequency_range.min > frequency_range.max) {
                nav_.display_modal("Error", "END freq\nis lower than START");
            } else {
                audio::output::stop();
                recon_thread->stop();	//STOP SCANNER THREAD

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
                set_display_freq( frequency_range.min );

                freq_stats.set_style(&style_white);
                freq_stats.set( "0/0/0" );

                show_max(); /* display step information */
                text_cycle.set( "MANUAL SEARCH" );
                button_scanner_mode.set_style( &style_white );
                button_scanner_mode.set_text( "MSEARCH" );
                file_name.set_style( &style_white );
                file_name.set( "USE: MANUAL RANGE" );

                start_recon_thread();
                user_resume();
            }
        };

        field_mode.on_change = [this](size_t, OptionsField::value_t v) {
            if( v != -1 )
            {
                receiver_model.disable();
                baseband::shutdown();
                change_mode(v);
                if ( !recon_thread->is_recon() ) 						//for some motive, audio output gets stopped.
                    audio::output::start();								//So if recon was stopped we resume audio
                receiver_model.enable();
            }
        };

        button_dir.on_select = [this](Button&) {
            recon_thread->change_recon_direction();
            fwd = recon_thread->get_recon_direction();
            if( fwd )
            {
                button_dir.set_text( "FW>" );
            }
            else
            {
                button_dir.set_text( "<RW" );
            }
            timer = 0 ;
            if ( userpause )				//If user-paused, resume
                user_resume();
        };

        button_restart.on_select = [this](Button&) {
            if( frequency_list.size() > 0 )
            {
                def_step = step_mode.selected_index();     //Use def_step from manual selector
                frequency_file_load( true );
                if( recon_thread )
                {
                    recon_thread->set_lock_duration( recon_lock_duration );
                    recon_thread->set_lock_nb_match( recon_lock_nb_match );
                }
                if( fwd )
                {
                    button_dir.set_text( "FW>" );
                }
                else
                {
                    button_dir.set_text( "<RW" );
                }
                timer = 0 ; 	 		//Will trigger a recon_resume() on_statistics_update, also advancing to next freq.
                show_max();
                user_resume();
            }
        };


        button_add.on_select = [this](ButtonWithEncoder&) {  //frequency_list[current_index]
            if( !scanner_mode)
            {
                if( recon_thread && frequency_list.size() && frequency_list.size() && current_index < frequency_list.size() )
                {
                    int64_t freq = recon_thread->get_current_freq();
                    bool found=false;
                    File recon_file;
                    std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
                    std::string frequency_to_add ;

                    freqman_entry entry = frequency_list[ current_index ] ;
                    entry . frequency_a =  recon_thread->get_current_freq();
                    entry . frequency_b =  recon_thread->get_current_freq();
                    entry . modulation =  recon_thread->get_current_modulation();
                    entry . bandwidth =  recon_thread->get_current_bandwidth();
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
                            set_display_freq( freq );
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
            if( recon_thread && frequency_list.size() > 0 )
            {
                timer = 0 ;
                if( button_add.get_encoder_delta() > 0 )
                {
                    fwd = true ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "FW>" );
                    recon_thread-> set_stepper( 1 );
                    recon_thread->set_freq_lock( 0 );
                }
                else if( button_add.get_encoder_delta() < 0 )
                {
                    fwd = false ;
                    recon_thread -> set_recon_direction( fwd );
                    button_dir.set_text( "<RW" );
                    recon_thread->set_freq_lock( 0 );
                    recon_thread-> set_stepper( -1 );
                }
            }
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
            if( recon_thread )
            {
                recon_thread->set_lock_duration( recon_lock_duration );
                recon_thread->set_lock_nb_match( recon_lock_nb_match );
                recon_thread->set_continuous( continuous );
                recon_thread->set_recon_direction( fwd );
            }
            if( autostart )
            {
                user_resume();
            }
            else
            {
                user_pause();
            }
        };

        button_recon_setup.on_select = [this,&nav](Button&) {

            ReconSetupSaveStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , recon_lock_duration , field_volume.value() );

            user_pause();

            auto open_view = nav.push<ReconSetupView>(input_file,output_file,recon_lock_duration,recon_lock_nb_match,recon_match_mode);
            open_view -> on_changed = [this](std::vector<std::string> result) {
                input_file = result[0];
                output_file = result[1];
                recon_lock_duration = strtol( result[2].c_str() , nullptr , 10 );
                recon_lock_nb_match = strtol( result[3].c_str() , nullptr , 10 );
                recon_match_mode    = strtol( result[4].c_str() , nullptr , 10 );

                ReconSetupSaveStrings( "RECON/RECON.CFG" , input_file , output_file , recon_lock_duration , recon_lock_nb_match , squelch , recon_match_mode , wait , recon_lock_duration , field_volume.value() );

                autosave = persistent_memory::recon_autosave_freqs();
                autostart = persistent_memory::recon_autostart_recon();
                continuous = persistent_memory::recon_continuous();
                filedelete = persistent_memory::recon_clear_output();
                load_freqs = persistent_memory::recon_load_freqs();
                load_ranges = persistent_memory::recon_load_ranges();
                load_hamradios = persistent_memory::recon_load_hamradios();

                update_ranges = persistent_memory::recon_update_ranges_when_recon();

                frequency_file_load( true );
                if( recon_thread )
                {
                    recon_thread->set_lock_duration( recon_lock_duration );
                    recon_thread->set_lock_nb_match( recon_lock_nb_match );
                    recon_thread->set_continuous( continuous );
                    recon_thread->set_recon_direction( fwd );
                }
                if( autostart )
                {
                    user_resume();
                }
                else
                {
                    user_pause();
                }

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

                lock_wait = ( 4 * ( recon_lock_duration * recon_lock_nb_match ) ) / 100 ;
                lock_wait = lock_wait * 100 ; // poor man's rounding
                if( lock_wait < 400 )
                    lock_wait = 400 ;
                field_lock_wait.set_value( lock_wait );
                show_max();
                return ;
            };

            if( userpause != true )
            {
                timer = 0 ;
                user_resume();
            }
            else
            {
                RetuneMessage message { };
                message.freq = recon_thread->get_current_freq();
                message.range = current_index ;
                EventDispatcher::send_message(message);
            }
        };

        //PRE-CONFIGURATION:
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
        field_lock_wait.on_change = [this](int32_t v)
        {
            lock_wait = v ;

            uint32_t lock_white = ( 4 * ( recon_lock_duration * recon_lock_nb_match ) ) / 100 ;
            lock_white = lock_white * 100 ;
            if( lock_white < 400 )
                lock_white = 400 ;

            uint32_t lock_yellow = 300 ;

            if( lock_wait >= lock_white )
            {
                field_lock_wait.set_style( &style_white );
            }
            else if( lock_wait >= lock_yellow )
            {
                field_lock_wait.set_style( &style_yellow);
            }
            else
            {
                field_lock_wait.set_style(&style_red);
            }
        };

        field_wait.set_value(wait);
        lock_wait = ( 4 * ( recon_lock_duration * recon_lock_nb_match ) );
        lock_wait = lock_wait / 100 ; lock_wait = lock_wait * 100 ; // poor man's rounding
        if( lock_wait < 400 )
            lock_wait = 400 ;
        field_lock_wait.set_value(lock_wait);

        field_squelch.on_change = [this](int32_t v) {
            squelch = v ;
        };
        field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v);	};
        field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);

        //FILL STEP OPTIONS
        freqman_set_modulation_option( field_mode );
        freqman_set_step_option( step_mode );

        if( filedelete )
        {
            delete_file( "FREQMAN/"+output_file+".TXT" );
        }

        frequency_file_load( false ); /* do not stop all at start */
        if( recon_thread && frequency_list.size() > 0 )
        {
            recon_thread->set_lock_duration( recon_lock_duration );
            recon_thread->set_lock_nb_match( recon_lock_nb_match );
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

            if( autostart )
            {
                timer = 0 ; 	 		//Will trigger a recon_resume() on_statistics_update, also advancing to next freq.
                user_resume();
            }
            else
            {
                user_pause();
            }
            show_max();
        }
    }


    void ReconView::frequency_file_load( bool stop_all_before) {

        // stop everything running now if required
        if (stop_all_before) {
            recon_thread->stop();
        }
        audio::output::stop();

        if( !scanner_mode)
        {
            def_step = step_mode.selected_index();		//Use def_step from manual selector
            frequency_list.clear();                         // clear the existing frequency list (expected behavior)
            if( !load_freqman_file_ex( input_file , frequency_list, load_freqs, load_ranges, load_hamradios ) )
            {
                desc_cycle.set(" NO " + input_file + ".TXT FILE ..." );
                file_name.set_style( &style_white );
                file_name.set( "USE: NO DATA" );
            }
            else
            {
                file_name.set_style( &style_blue );
                file_name.set( "USE: "+input_file );
            }
            step_mode.set_selected_index(def_step); //Impose the default step into the manual step selector
        }
        else
        {
            def_step = step_mode.selected_index();		//Use def_step from manual selector
            frequency_list.clear();                         // clear the existing frequency list (expected behavior)
            if( !load_freqman_file_ex( output_file , frequency_list, load_freqs, load_ranges, load_hamradios ) )
            {
                desc_cycle.set(" NO " + output_file + ".TXT FILE ..." );
                file_name.set_style( &style_white );
                file_name.set( "USE:" );
            }
            else
            {
                file_name.set( "USE: "+output_file );
                file_name.set_style( &style_red );
            }
            step_mode.set_selected_index(def_step); //Impose the default step into the manual step selector
        }

        start_recon_thread();
    }

    void ReconView::on_statistics_update(const ChannelStatistics& statistics) {

        int32_t actual_db = statistics.max_db ;
        bool update_stats = false ;
        int32_t freq_lock = recon_thread->is_freq_lock();
        int32_t max_lock = recon_thread -> get_lock_nb_match();

        // 0 recon , 1 locking , 2 locked
        static int32_t status = -1 ;

        if( actual_db != 0 && db != actual_db )
        {
            db = actual_db ;
            update_stats = true ;
        }

        if( !userpause )
        {
            if( !timer )
            {
                if( status != 0 )
                {
                    status = 0 ;
                    update_stats = true ;
                    continuous_lock = false ;
                    recon_thread->set_freq_lock( 0 ); //in lock period, still analyzing the signal
                    recon_resume();  // RESUME!
                    big_display.set_style(&style_white);
                }
            }
            if( freq_lock >= max_lock ) // LOCKED
            {
                if( status != 2 )
                {
                    status = 2 ;
                    update_stats = true ;

                    if( wait != 0 )
                    {
                        audio::output::start();
                        this->on_headphone_volume_changed( (receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99 );
                    }
                    if( wait >= 0 )
                    {
                        timer = wait ;
                    }
                    //Inform freq (for coloring purposes also!)
                    RetuneMessage message { };
                    message.freq = recon_thread->get_current_freq() ;
                    message.range = current_index ;
                    EventDispatcher::send_message(message);
                }
                if( wait < 0 )
                {
                    if( actual_db > squelch ) //MATCHING LEVEL IN STAY X AFTER LAST ACTIVITY
                    {
                        timer = abs( wait );
                    }
                }
            }
            else // freq_lock < max_lock , LOCKING
            {
                if( actual_db > squelch ) //MATCHING LEVEL
                {
                    if( status != 1 )
                    {
                        status = 1 ;
                        continuous_lock = true ;
                        if( wait != 0 )
                        {
                            audio::output::stop();
                        }
                        timer = lock_wait ;
                        //Inform freq (for coloring purposes also!)
                        RetuneMessage message { };
                        message.freq = recon_thread->get_current_freq() ;
                        current_index = recon_thread->get_freq_index() ;
                        message.range = current_index ;
                        EventDispatcher::send_message(message);
                    }
                    freq_lock ++ ;
                    recon_thread->set_freq_lock( freq_lock ); //in lock period, still analyzing the signal
                    update_stats = true ;
                }
                else
                {
                    // continuous, direct cut it if not consecutive match
                    if( recon_match_mode == 0 )
                    {
                        timer = 0 ;
                        update_stats = true ;
                    }
                }
            }
        }
        else
        {
            if( actual_db > squelch )
            {
                if( status != 2 ) //MATCHING LEVEL
                {
                    status = 2 ;
                    big_display.set_style(&style_yellow);
                    set_display_freq( recon_thread->get_current_freq() );
                }
            }
            else
            {
                if( status != 0 )
                {
                    status = 0 ;
                    big_display.set_style(&style_white);
                    set_display_freq( recon_thread->get_current_freq() );
                }

            }
        }

        if( update_stats )
        {
            show_max();
        }

        timer -= 50 ;
        if( timer < 0 )
        {
            timer = 0 ;
        }
    } /* on_statistic_updates */

    void ReconView::recon_pause() {
        if (recon_thread->is_recon()) 
        {
            audio::output::start();
            this->on_headphone_volume_changed( (receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99 );
            recon_thread->set_freq_lock( 0 ); //in lock period, still analyzing the signal
            recon_thread ->set_recon(false); // WE STOP SCANNING
        }
    }


    void ReconView::recon_resume() {
        audio::output::stop();
        if( !recon_thread->is_recon() )
            recon_thread->set_recon(true);       // RESUME!
        big_display.set_style(&style_white); //Back to grey color
    }

    void ReconView::user_pause() {
        timer = 0 ; 	 		        // Will trigger a recon_resume() on_statistics_update, also advancing to next freq.
        //button_pause.set_text("<RESUME>");	//PAUSED, show resume
        userpause=true;
        continuous_lock=false;
        recon_pause();
    }

    void ReconView::user_resume() {
        timer = 0 ; 	 		        // Will trigger a recon_resume() on_statistics_update, also advancing to next freq.
        //button_pause.set_text("<PAUSE>");		//Show button for pause
        userpause=false;			    // Resume recon
        continuous_lock=false;
        recon_resume();
    }

    void ReconView::on_headphone_volume_changed(int32_t v) {
        const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
        receiver_model.set_headphone_volume(new_volume);
    }

    size_t ReconView::change_mode( freqman_index_t new_mod ) { //Before this, do a recon_thread->stop();  After this do a start_recon_thread()

        field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n;	};

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
                //bw 200k (0) only/default
                field_bw.set_selected_index(0);
                baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
                receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
                receiver_model.set_wfm_configuration(field_bw.selected_index());
                field_bw.on_change = [this](size_t n, OptionsField::value_t) {	receiver_model.set_wfm_configuration(n); };
                receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000);
                break;
            default:
                break;
        }
        return freqman_entry_get_step_value( def_step );
    }

    void ReconView::start_recon_thread() {
        receiver_model.enable();
        receiver_model.set_squelch_level(0);
        recon_thread = std::make_unique<ReconThread>(&frequency_list);
        recon_thread->set_continuous( continuous );
        recon_thread->set_lock_duration( recon_lock_duration );
        recon_thread->set_lock_nb_match( recon_lock_nb_match );
        recon_thread->set_recon_direction( fwd );
    }

} /* namespace ui */
