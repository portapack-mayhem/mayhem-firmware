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

	#protoview 8 byte
	external/protoview/main.cpp
	external/protoview/ui_protoview.cpp

	#adsbtx  3544 byte - adsb shared part
	external/adsbtx/main.cpp
	external/adsbtx/ui_adsb_tx.cpp

	#morse_tx 768 bytes
	external/morse_tx/main.cpp
	external/morse_tx/ui_morse.cpp

	#sstvtx 456 bytes
	external/sstvtx/main.cpp
	external/sstvtx/ui_sstvtx.cpp

	#random 464  bytes.
	external/random_password/main.cpp
	external/random_password/ui_random_password.cpp
	external/random_password/sha512.cpp

	#acars
	#external/acars_rx/main.cpp
	#external/acars_rx/acars_app.cpp

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
	protoview
	adsbtx
	morse_tx
	sstvtx
	random_password
	# acars_rx --not working
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
	detector_rx
	spaceinv
	blackjack
	battleship
	ert
	epirb_rx
	soundboard
	game2048
	bht_tx
	morse_practice
	adult_toys_controller
)
