set(EXTCPPSRC
	#afsk_rx   16 byte
	external/afsk_rx/main.cpp
	external/afsk_rx/ui_afsk_rx.cpp

	#calculator  632 bytes
	external/calculator/main.cpp
	external/calculator/ui_calculator.cpp

	#font_viewer 8 byte?!
	external/font_viewer/main.cpp
	external/font_viewer/ui_font_viewer.cpp

	#blespam 336 bytes - array initializers?
	external/blespam/main.cpp
	external/blespam/ui_blespam.cpp

	#analogtv 552 bytes 
	external/analogtv/main.cpp
	external/analogtv/analog_tv_app.cpp
	external/analogtv/ui_tv.cpp

	#nrf_rx  40 byte
	external/nrf_rx/main.cpp
	external/nrf_rx/ui_nrf_rx.cpp

	#coasterp  0 byte
	external/coasterp/main.cpp
	external/coasterp/ui_coasterp.cpp

	#lge  120 byte
	external/lge/main.cpp
	external/lge/lge_app.cpp
	external/lge/rfm69.cpp

	#lcr - 460 byte flash
	external/lcr/main.cpp
	external/lcr/ui_lcr.cpp

	#jammer 144 byte
	external/jammer/main.cpp
	external/jammer/ui_jammer.cpp

	#gpssim  160 byte
	external/gpssim/main.cpp
	external/gpssim/gps_sim_app.cpp

	#spainter   464 byte
	external/spainter/main.cpp
	external/spainter/ui_spectrum_painter.cpp
	external/spainter/ui_spectrum_painter_text.cpp
	external/spainter/ui_spectrum_painter_image.cpp

	#keyfob 216 byte
	external/keyfob/main.cpp
	external/keyfob/ui_keyfob.cpp
	external/keyfob/ui_keyfob.hpp

	#tetris 88 byte
	external/tetris/main.cpp
	external/tetris/ui_tetris.cpp


	#extsensors 192 byte
	external/extsensors/main.cpp
	external/extsensors/ui_extsensors.cpp
	external/extsensors/ui_extsensors.hpp

	#foxhunt 0
	external/foxhunt/main.cpp
	external/foxhunt/ui_foxhunt_rx.cpp
	external/foxhunt/ui_foxhunt_rx.hpp

	#audio_test 192 byte
	external/audio_test/main.cpp
	external/audio_test/ui_audio_test.cpp

	#wardrivemap 64 byte
	external/wardrivemap/main.cpp
	external/wardrivemap/ui_wardrivemap.cpp

	#tpmsrx 920 byte- possible shared part with baseband
	external/tpmsrx/main.cpp
	external/tpmsrx/tpms_app.cpp

	#tpmstx 800 bytes - TPMS transmit with editable fields
	external/tpmstx/main.cpp
	external/tpmstx/tpms_tx_app.cpp

	#protoview 8 byte
	external/protoview/main.cpp
	external/protoview/ui_protoview.cpp

	#adsbtx  3544 byte - adsb shared part
	external/adsbtx/main.cpp
	external/adsbtx/ui_adsb_tx.cpp

	#morse_tx 768 bytes -- disabled because of the new morse tx app with more functions
	#external/morse_tx/main.cpp
	#external/morse_tx/ui_morse.cpp

	#sstvtx 456 bytes
	external/sstvtx/main.cpp
	external/sstvtx/ui_sstvtx.cpp

	#same_tx
	external/same_tx/main.cpp
	external/same_tx/ui_same_tx.cpp

	#mdc_tx
	external/mdc_tx/main.cpp
	external/mdc_tx/ui_mdc_tx.cpp

	#sstvrx
	external/sstvrx/main.cpp
	external/sstvrx/ui_sstvrx.cpp

	#random 464  bytes.
	external/random_password/main.cpp
	external/random_password/ui_random_password.cpp
	external/random_password/sha512.cpp

	#acars
	external/acars_rx/main.cpp
	external/acars_rx/acars_app.cpp

	#wefax_rx 192 bytes
	external/wefax_rx/main.cpp
	external/wefax_rx/ui_wefax_rx.cpp


	#noaaapt_rx  72 bytes
	external/noaaapt_rx/main.cpp
	external/noaaapt_rx/ui_noaaapt_rx.cpp

	#shoppingcart_lock 272 bytes
	external/shoppingcart_lock/main.cpp
	external/shoppingcart_lock/shoppingcart_lock.cpp


	#ookbrute  80 byte
	external/ookbrute/main.cpp
	external/ookbrute/ui_ookbrute.cpp

	#ook_editor  1808 bytes
	external/ook_editor/main.cpp
	external/ook_editor/ui_ook_editor.cpp

	#cvs_spam 24 byte
	external/cvs_spam/main.cpp
	external/cvs_spam/cvs_spam.cpp

	#flippertx  712 bytes
	external/flippertx/main.cpp
	external/flippertx/ui_flippertx.cpp

	#remote 1664 bytes
	external/remote/main.cpp
	external/remote/ui_remote.cpp

	#mcu_temperature    112
	external/mcu_temperature/main.cpp
	external/mcu_temperature/mcu_temperature.cpp

	#fmradio  640
	external/fmradio/main.cpp
	external/fmradio/ui_fmradio.cpp

	#tuner 384
	external/tuner/main.cpp
	external/tuner/ui_tuner.cpp

	#metronome 696 bytes
	external/metronome/main.cpp
	external/metronome/ui_metronome.cpp

	#app_manager 40 bytes
	external/app_manager/main.cpp
	external/app_manager/ui_app_manager.cpp

	#hopper 472 bytes
	external/hopper/main.cpp
	external/hopper/ui_hopper.cpp

	# whip calculator  48 bytes
	external/antenna_length/main.cpp
	external/antenna_length/ui_whipcalc.cpp

	# wav viewer 1232 bytes
	external/wav_view/main.cpp
	external/wav_view/ui_view_wav.cpp


	# wipe sdcard 16 byte
	external/sd_wipe/main.cpp
	external/sd_wipe/ui_sd_wipe.cpp

	# playlist editor 232 bytes
	external/playlist_editor/main.cpp
	external/playlist_editor/ui_playlist_editor.cpp

	#snake 240 bytes
	external/snake/main.cpp
	external/snake/ui_snake.cpp


	#stopwatch 0
	external/stopwatch/main.cpp
	external/stopwatch/ui_stopwatch.cpp

	#breakout 1144 bytes
	external/breakout/main.cpp
	external/breakout/ui_breakout.cpp

	#dinogame 0 
	external/dinogame/main.cpp
	external/dinogame/ui_dinogame.cpp

	#doom 224
	external/doom/main.cpp
	external/doom/ui_doom.cpp

	#debug_pmem  944 byte
	external/debug_pmem/main.cpp
	external/debug_pmem/ui_debug_pmem.cpp

	#scanner 520 byte
	external/scanner/main.cpp
	external/scanner/ui_scanner.cpp

	#level  24 byte
	external/level/main.cpp
	external/level/ui_level.cpp

	#gfxEQ 80 byte
	external/gfxeq/main.cpp
	external/gfxeq/ui_gfxeq.cpp	

	#waterfall designer
	external/waterfall_designer/main.cpp
	external/waterfall_designer/ui_waterfall_designer.cpp

	#detector_rx  168 byte
	external/detector_rx/main.cpp
	external/detector_rx/ui_detector_rx.cpp

	#space_invaders  0 byte
	external/spaceinv/main.cpp
	external/spaceinv/ui_spaceinv.cpp

	#blackjack 24 byte
	external/blackjack/main.cpp
	external/blackjack/ui_blackjack.cpp

	#battleship  256 byte
	external/battleship/main.cpp
	external/battleship/ui_battleship.cpp

	#ert 3040 bytes - has common with baseband, could be renamed the namespace, so both could have it, but not kept in fw.
	external/ert/main.cpp
	external/ert/ert_app.cpp

	#epirb_rx 168 byte flash 
	external/epirb_rx/main.cpp
	external/epirb_rx/ui_epirb_rx.cpp
	external/epirb_rx/ui_beaconlist.cpp
	external/epirb_rx/beacon_db.cpp
	external/epirb_rx/beacon.cpp
	external/epirb_rx/location.cpp

	#epirb_tx
	external/epirb_tx/main.cpp
	external/epirb_tx/ui_epirb_tx.cpp

	#soundboard  272byte  - 1236 bytes
	external/soundboard/main.cpp
	external/soundboard/soundboard_app.cpp

	#game2048   - 168 byte flash
	external/game2048/main.cpp
	external/game2048/ui_game2048.cpp

	#bht_tx - 3920 byte flash, unknown
	external/bht_tx/main.cpp
	external/bht_tx/ui_bht_tx.cpp
	external/bht_tx/bht.cpp

	#morse_practice - 80 byte flash - bc of array initializers
	external/morse_practice/main.cpp
	external/morse_practice/ui_morse_practice.cpp

	#adult_toys_controller  144 bytes 
	external/adult_toys_controller/main.cpp
	external/adult_toys_controller/ui_adult_toys_controller.cpp

	#flex_rx
	external/flex_rx/main.cpp
	external/flex_rx/ui_flex_rx.cpp	

	#subcarrx
	external/subcarrx/main.cpp
	external/subcarrx/ui_subcar.cpp

	#siggen
	external/siggen/main.cpp
	external/siggen/ui_siggen.cpp

	#morse_radio
	external/morse_radio/main.cpp
	external/morse_radio/ui_morse_radio.cpp

	#morseradiotx
	external/morseradiotx/main.cpp
	external/morseradiotx/ui_morse_radiotx.cpp

	#keeloqtx
  	external/keeloqtx/main.cpp
  	external/keeloqtx/ui_keeloqtx.cpp

	#rtty_rx
	external/rtty_rx/main.cpp
	external/rtty_rx/ui_rtty_rx.cpp
	external/rtty_rx/baudot.cpp

	#rtty_tx
	external/rtty_tx/main.cpp
	external/rtty_tx/ui_rtty_tx.cpp
	external/rtty_tx/baudot.cpp

	#pocsag_tx
	external/pocsag_tx/main.cpp
	external/pocsag_tx/ui_pocsag_tx.cpp

	#flex_tx
	external/flex_tx/main.cpp
	external/flex_tx/ui_flex_tx.cpp

	#time_sink
	external/time_sink/main.cpp
	external/time_sink/ui_time_sink.cpp

	#kiss_tnc
	external/kiss_tnc/main.cpp
	external/kiss_tnc/ui_kiss_tnc.cpp

	#fpv_detect
	external/fpv_detect/main.cpp
	external/fpv_detect/ui_fpv_detect.cpp

	#p25_tx
	external/p25_tx/main.cpp
	external/p25_tx/ui_p25_tx.cpp

	#two_tone_pager
	external/two_tone_pager/main.cpp
	external/two_tone_pager/ui_two_tone_pager.cpp

	#two_tone_rx
	external/two_tone_rx/main.cpp
	external/two_tone_rx/ui_two_tone_rx.cpp

	#hard_reset
	external/hard_reset/main.cpp
	external/hard_reset/ui_hard_reset.cpp

	#secplustx
	external/secplustx/main.cpp
	external/secplustx/ui_secplustx.cpp
	external/secplustx/secplustx.cpp
)

set(EXTAPPLIST
	afsk_rx
	calculator
	font_viewer
	blespam
	analogtv
	nrf_rx
	coasterp
	lge
	lcr
	jammer
	gpssim
	spainter
	keyfob
	tetris
	extsensors
	foxhunt_rx
	audio_test
	wardrivemap
	tpmsrx
	tpmstx
	protoview
	adsbtx
	#morse_tx
	sstvtx
	same_tx
	mdc_tx
	sstvrx
	random_password
	acars_rx
	wefax_rx
	noaaapt_rx
	shoppingcart_lock
	ookbrute
	ook_editor
	cvs_spam
	flippertx
	remote
	mcu_temperature
	fmradio
	tuner
	metronome
	app_manager
	hopper
	antenna_length
	view_wav
	sd_wipe
	playlist_editor
	snake
	stopwatch
	breakout
	dinogame
	doom
	debug_pmem
	scanner
	level
	gfxeq
	waterfall_designer
	detector_rx
	fpv_detect
	spaceinv
	blackjack
	battleship
	ert
	epirb_rx
	epirb_tx
	soundboard
	game2048
	bht_tx
	morse_practice
	adult_toys_controller
	flex_rx
	subcarrx
	siggen
	morse_radio
	morseradiotx
	keeloqtx
	rtty_rx
	rtty_tx
	pocsag_tx
	flex_tx
	time_sink
	kiss_tnc
	p25_tx
	two_tone_pager
	two_tone_rx
	hard_reset
	secplustx
)

# sdusb has type conflicts with PRALINE (HackRF Pro) - add only for non-PRALINE builds
if(NOT BOARD STREQUAL "PRALINE")
       list(APPEND EXTCPPSRC
               external/sdusb/main.cpp
               external/sdusb/ui_sd_over_usb.cpp
       )
       list(APPEND EXTAPPLIST sdusb)
endif()

