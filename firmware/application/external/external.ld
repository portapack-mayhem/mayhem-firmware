/*
    Copyright (C) 2023 Bernd Herzog
    Copyright (C) 2024 Mark Thompson

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

MEMORY
{
    /*
     * External apps: regions can't overlap so addresses are corrected after build.
     * Picking uncommon address values for search & replace in binaries (no false positives) - 0xADB00000-0xADEF0000 seems to be good.
     * Also need to consider processor memory map - reading 0xADxxxxxx generates a fault which may be better than unexpected behavior.
     * External app address ranges below must match those in python file "external_app_info.py".
     */
    ram_external_app_afsk_rx    (rwx) : org = 0xADB10000, len = 32k
    ram_external_app_calculator (rwx) : org = 0xADB20000, len = 32k
    ram_external_app_font_viewer(rwx) : org = 0xADB30000, len = 32k
    ram_external_app_blespam    (rwx) : org = 0xADB40000, len = 32k
    ram_external_app_analogtv   (rwx) : org = 0xADB50000, len = 32k
    ram_external_app_nrf_rx     (rwx) : org = 0xADB60000, len = 32k
    ram_external_app_coasterp   (rwx) : org = 0xADB70000, len = 32k
    ram_external_app_lge        (rwx) : org = 0xADB80000, len = 32k
    ram_external_app_lcr        (rwx) : org = 0xADB90000, len = 32k
    ram_external_app_jammer     (rwx) : org = 0xADBA0000, len = 32k
    ram_external_app_gpssim     (rwx) : org = 0xADBB0000, len = 32k
    ram_external_app_spainter   (rwx) : org = 0xADBC0000, len = 32k
    ram_external_app_keyfob     (rwx) : org = 0xADBD0000, len = 32k
    ram_external_app_tetris     (rwx) : org = 0xADBE0000, len = 32k
    ram_external_app_extsensors (rwx) : org = 0xADBF0000, len = 32k
    ram_external_app_foxhunt_rx (rwx) : org = 0xADC00000, len = 32k
    ram_external_app_audio_test (rwx) : org = 0xADC10000, len = 32k
    ram_external_app_wardrivemap(rwx) : org = 0xADC20000, len = 32k
    ram_external_app_tpmsrx     (rwx) : org = 0xADC30000, len = 32k
    ram_external_app_protoview  (rwx) : org = 0xADC40000, len = 32k
    ram_external_app_adsbtx     (rwx) : org = 0xADC50000, len = 32k
    ram_external_app_morse_tx   (rwx) : org = 0xADC60000, len = 32k
    ram_external_app_sstvtx     (rwx) : org = 0xADC70000, len = 32k
    ram_external_app_random_password(rwx): org = 0xADC80000, len = 32k
    ram_external_app_acars_rx   (rwx) : org = 0xADC90000, len = 32k
    ram_external_app_shoppingcart_lock(rwx): org = 0xADCA0000, len = 32k
    ram_external_app_cvs_spam   (rwx) : org = 0xADCB0000, len = 32k
    ram_external_app_ookbrute   (rwx) : org = 0xADCC0000, len = 32k
    ram_external_app_flippertx  (rwx) : org = 0xADCD0000, len = 32k
    ram_external_app_ook_editor (rwx) : org = 0xADCE0000, len = 32k
    ram_external_app_remote     (rwx) : org = 0xADCF0000, len = 32k
    ram_external_app_mcu_temperature(rwx): org = 0xADD00000, len = 32k
    ram_external_app_fmradio    (rwx) : org = 0xADD10000, len = 32k
    ram_external_app_tuner      (rwx) : org = 0xADD20000, len = 32k
    ram_external_app_metronome  (rwx) : org = 0xADD30000, len = 32k
    ram_external_app_app_manager(rwx) : org = 0xADD40000, len = 32k
    ram_external_app_hopper     (rwx) : org = 0xADD50000, len = 32k
    ram_external_app_antenna_length(rwx): org = 0xADD60000, len = 32k
    ram_external_app_view_wav   (rwx) : org = 0xADD70000, len = 32k
    ram_external_app_sd_wipe    (rwx) : org = 0xADD80000, len = 32k
    ram_external_app_playlist_editor(rwx): org = 0xADD90000, len = 32k
    ram_external_app_snake      (rwx) : org = 0xADDA0000, len = 32k
    ram_external_app_stopwatch  (rwx) : org = 0xADDB0000, len = 32k
    ram_external_app_wefax_rx   (rwx) : org = 0xADDC0000, len = 32k
    ram_external_app_breakout   (rwx) : org = 0xADDD0000, len = 32k
    ram_external_app_doom       (rwx) : org = 0xADDE0000, len = 32k
    ram_external_app_debug_pmem (rwx) : org = 0xADDF0000, len = 32k
    ram_external_app_scanner    (rwx) : org = 0xADE00000, len = 32k
    ram_external_app_level      (rwx) : org = 0xADE10000, len = 32k
    ram_external_app_gfxeq      (rwx) : org = 0xADE20000, len = 32k
    ram_external_app_noaaapt_rx   (rwx) : org = 0xADE30000, len = 32k
    ram_external_app_detector_rx   (rwx) : org = 0xADE40000, len = 32k
    ram_external_app_dinogame   (rwx) : org = 0xADE50000, len = 32k
    ram_external_app_spaceinv   (rwx) : org = 0xADE60000, len = 32k
    ram_external_app_blackjack  (rwx) : org = 0xADE70000, len = 32k
    ram_external_app_battleship   (rwx) : org = 0xADE80000, len = 32k
    ram_external_app_ert        (rwx) : org = 0xADE90000, len = 32k
    ram_external_app_epirb_rx   (rwx) : org = 0xADEA0000, len = 32k
    ram_external_app_soundboard   (rwx) : org = 0xADEB0000, len = 32k
    ram_external_app_game2048     (rwx) : org = 0xADEC0000, len = 32k
    ram_external_app_bht_tx          (rwx) : org = 0xADED0000, len = 32k
    ram_external_app_morse_practice  (rwx) : org = 0xADEE0000, len = 32k


}

SECTIONS
{
    .external_app_afsk_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_afsk_rx.application_information));
        *(*ui*external_app*afsk_rx*);
    } > ram_external_app_afsk_rx

    .external_app_calculator : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_calculator.application_information));
        *(*ui*external_app*calculator*);
    } > ram_external_app_calculator

    .external_app_font_viewer : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_font_viewer.application_information));
        *(*ui*external_app*font_viewer*);
    } > ram_external_app_font_viewer

    .external_app_blespam : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_blespam.application_information));
        *(*ui*external_app*blespam*);
    } > ram_external_app_blespam

    .external_app_analogtv : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_analogtv.application_information));
        *(*ui*external_app*analogtv*);
    } > ram_external_app_analogtv

    .external_app_nrf_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_nrf_rx.application_information));
        *(*ui*external_app*nrf_rx*);
    } > ram_external_app_nrf_rx

     .external_app_coasterp : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_coasterp.application_information));
        *(*ui*external_app*coasterp*);
    } > ram_external_app_coasterp

    .external_app_lge : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_lge.application_information));
        *(*ui*external_app*lge*);
    } > ram_external_app_lge

    .external_app_lcr : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_lcr.application_information));
        *(*ui*external_app*lcr*);
    } > ram_external_app_lcr

    .external_app_jammer : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_jammer.application_information));
        *(*ui*external_app*jammer*);
    } > ram_external_app_jammer

    .external_app_gpssim : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_gpssim.application_information));
        *(*ui*external_app*gpssim*);
    } > ram_external_app_gpssim

    .external_app_spainter : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_spainter.application_information));
        *(*ui*external_app*spainter*);
    } > ram_external_app_spainter

    .external_app_keyfob : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_keyfob.application_information));
        *(*ui*external_app*keyfob*);
    } > ram_external_app_keyfob

    .external_app_tetris : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_tetris.application_information));
        *(*ui*external_app*tetris*);
    } > ram_external_app_tetris

   .external_app_extsensors : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_extsensors.application_information));
        *(*ui*external_app*extsensors*);
    } > ram_external_app_extsensors

   .external_app_foxhunt_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_foxhunt_rx.application_information));
        *(*ui*external_app*foxhunt_rx*);
    } > ram_external_app_foxhunt_rx

   .external_app_audio_test : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_audio_test.application_information));
        *(*ui*external_app*audio_test*);
    } > ram_external_app_audio_test

   .external_app_wardrivemap : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_wardrivemap.application_information));
        *(*ui*external_app*wardrivemap*);
    } > ram_external_app_wardrivemap

   .external_app_tpmsrx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_tpmsrx.application_information));
        *(*ui*external_app*tpmsrx*);
    } > ram_external_app_tpmsrx

   .external_app_protoview : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_protoview.application_information));
        *(*ui*external_app*protoview*);
    } > ram_external_app_protoview

   .external_app_adsbtx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_adsbtx.application_information));
        *(*ui*external_app*adsbtx*);
    } > ram_external_app_adsbtx

   .external_app_morse_tx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_morse_tx.application_information));
        *(*ui*external_app*morse_tx*);
    } > ram_external_app_morse_tx

   .external_app_sstvtx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_sstvtx.application_information));
        *(*ui*external_app*sstvtx*);
    } > ram_external_app_sstvtx

    .external_app_random_password : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_random_password.application_information));
        *(*ui*external_app*random_password*);
    } > ram_external_app_random_password

    .external_app_acars_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_acars_rx.application_information));
        *(*ui*external_app*acars_rx*);
    } > ram_external_app_acars_rx

   .external_app_shoppingcart_lock : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_shoppingcart_lock.application_information));
        *(*ui*external_app*shoppingcart_lock*);
    } > ram_external_app_shoppingcart_lock

   .external_app_cvs_spam : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_cvs_spam.application_information));
        *(*ui*external_app*cvs_spam*);
    } > ram_external_app_cvs_spam

   .external_app_ookbrute : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_ookbrute.application_information));
        *(*ui*external_app*ookbrute*);
    } > ram_external_app_ookbrute

    .external_app_ook_editor : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_ook_editor.application_information));
        *(*ui*external_app*ook_editor*);
    } > ram_external_app_ook_editor

   .external_app_flippertx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_flippertx.application_information));
        *(*ui*external_app*flippertx*);
    } > ram_external_app_flippertx

    .external_app_remote : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_remote.application_information));
        *(*ui*external_app*remote*);
    } > ram_external_app_remote

    .external_app_mcu_temperature : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_mcu_temperature.application_information));
        *(*ui*external_app*mcu_temperature*);
    } > ram_external_app_mcu_temperature

    .external_app_fmradio : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_fmradio.application_information));
        *(*ui*external_app*fmradio*);
    } > ram_external_app_fmradio

    .external_app_tuner : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_tuner.application_information));
        *(*ui*external_app*tuner*);
    } > ram_external_app_tuner

    .external_app_metronome : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_metronome.application_information));
        *(*ui*external_app*metronome*);
    } > ram_external_app_metronome

    .external_app_app_manager : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_app_manager.application_information));
        *(*ui*external_app*app_manager*);
    } > ram_external_app_app_manager

    .external_app_hopper : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_hopper.application_information));
        *(*ui*external_app*hopper*);
    } > ram_external_app_hopper

    .external_app_antenna_length : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_antenna_length.application_information));
        *(*ui*external_app*antenna_length*);
    } > ram_external_app_antenna_length

    .external_app_view_wav : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_view_wav.application_information));
        *(*ui*external_app*view_wav*);
    } > ram_external_app_view_wav

    .external_app_sd_wipe : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_sd_wipe.application_information));
        *(*ui*external_app*sd_wipe*);
    } > ram_external_app_sd_wipe

    .external_app_playlist_editor : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_playlist_editor.application_information));
        *(*ui*external_app*playlist_editor*);
    } > ram_external_app_playlist_editor

    .external_app_snake : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_snake.application_information));
        *(*ui*external_app*snake*);
    } > ram_external_app_snake

    .external_app_stopwatch : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_stopwatch.application_information));
        *(*ui*external_app*stopwatch*);
    } > ram_external_app_stopwatch

   .external_app_wefax_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_wefax_rx.application_information));
        *(*ui*external_app*wefax_rx*);
    } > ram_external_app_wefax_rx

   .external_app_noaaapt_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_noaaapt_rx.application_information));
        *(*ui*external_app*noaaapt_rx*);
    } > ram_external_app_noaaapt_rx

    .external_app_breakout : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_breakout.application_information));
        *(*ui*external_app*breakout*);
    } > ram_external_app_breakout

    .external_app_doom : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_doom.application_information));
        *(*ui*external_app*doom*);
    } > ram_external_app_doom

    .external_app_debug_pmem : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_debug_pmem.application_information));
        *(*ui*external_app*debug_pmem*);
    } > ram_external_app_debug_pmem

    .external_app_scanner : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_scanner.application_information));
        *(*ui*external_app*scanner*);
    } > ram_external_app_scanner

    .external_app_level : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_level.application_information));
        *(*ui*external_app*level*);
    } > ram_external_app_level

    .external_app_gfxeq : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_gfxeq.application_information));
        *(*ui*external_app*gfxeq*);
    } > ram_external_app_gfxeq

    .external_app_detector_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_detector_rx.application_information));
        *(*ui*external_app*detector_rx*);
    } > ram_external_app_detector_rx

    .external_app_dinogame : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_dinogame.application_information));
        *(*ui*external_app*dinogame*);
    } > ram_external_app_dinogame

    .external_app_spaceinv : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_spaceinv.application_information));
        *(*ui*external_app*spaceinv*);
    } > ram_external_app_spaceinv

    .external_app_blackjack : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_blackjack.application_information));
        *(*ui*external_app*blackjack*);
    } > ram_external_app_blackjack

    .external_app_battleship : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_battleship.application_information));
        *(*ui*external_app*battleship*);
    } > ram_external_app_battleship

    .external_app_ert : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_ert.application_information));
        *(*ui*external_app*ert*);
    } > ram_external_app_ert

    .external_app_epirb_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_epirb_rx.application_information));
        *(*ui*external_app*epirb_rx*);
    } > ram_external_app_epirb_rx

    .external_app_soundboard : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_soundboard.application_information));
        *(*ui*external_app*soundboard*);
    } > ram_external_app_soundboard

    .external_app_game2048 : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_game2048.application_information));
        *(*ui*external_app*game2048*);
    } > ram_external_app_game2048


    .external_app_bht_tx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_bht_tx.application_information));
        *(*ui*external_app*bht_tx*);
    } > ram_external_app_bht_tx


    .external_app_morse_practice : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_morse_practice.application_information));
        *(*ui*external_app*morse_practice*);
    } > ram_external_app_morse_practice


}

