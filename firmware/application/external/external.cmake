set(EXTCPPSRC

	#tetris
	external/tetris/main.cpp
	external/tetris/ui_tetris.cpp

	#afsk_rx
	external/afsk_rx/main.cpp
	external/afsk_rx/ui_afsk_rx.cpp

	#calculator
	external/calculator/main.cpp
	external/calculator/ui_calculator.cpp

	#font_viewer
	external/font_viewer/main.cpp
	external/font_viewer/ui_font_viewer.cpp

	#blespam
	external/blespam/main.cpp
	external/blespam/ui_blespam.cpp

	#analogtv
	external/analogtv/main.cpp
	external/analogtv/analog_tv_app.cpp

	#nrf_rx
	external/nrf_rx/main.cpp
	external/nrf_rx/ui_nrf_rx.cpp

	#coasterp
	external/coasterp/main.cpp
	external/coasterp/ui_coasterp.cpp

	#lge
	external/lge/main.cpp
	external/lge/lge_app.cpp

	#lcr
	external/lcr/main.cpp
	external/lcr/ui_lcr.cpp

	#jammer
	external/jammer/main.cpp
	external/jammer/ui_jammer.cpp

	#gpssim
	external/gpssim/main.cpp
	external/gpssim/gps_sim_app.cpp

	#spainter
	external/spainter/main.cpp
	external/spainter/ui_spectrum_painter.cpp
	external/spainter/ui_spectrum_painter_text.cpp
	external/spainter/ui_spectrum_painter_image.cpp

	#keyfob
	external/keyfob/main.cpp
	external/keyfob/ui_keyfob.cpp
	external/keyfob/ui_keyfob.hpp

	#extsensors
	external/extsensors/main.cpp
	external/extsensors/ui_extsensors.cpp
	external/extsensors/ui_extsensors.hpp

	#foxhunt
	external/foxhunt/main.cpp
	external/foxhunt/ui_foxhunt_rx.cpp
	external/foxhunt/ui_foxhunt_rx.hpp

	#audio_test
	external/audio_test/main.cpp
	external/audio_test/ui_audio_test.cpp

	#wardrivemap
	external/wardrivemap/main.cpp
	external/wardrivemap/ui_wardrivemap.cpp

	#tpmsrx
	external/tpmsrx/main.cpp
	external/tpmsrx/tpms_app.cpp

	#protoview
	external/protoview/main.cpp
	external/protoview/ui_protoview.cpp

	#adsbtx
	external/adsbtx/main.cpp
	external/adsbtx/ui_adsb_tx.cpp

	#morse_tx
	external/morse_tx/main.cpp
	external/morse_tx/ui_morse.cpp

	#sstvtx
	external/sstvtx/main.cpp
	external/sstvtx/ui_sstvtx.cpp

	#random
	external/random_password/main.cpp
	external/random_password/ui_random_password.cpp
	external/random_password/sha512.cpp
	external/random_password/sha512.h

	#acars
	external/acars_rx/main.cpp
	external/acars_rx/acars_app.cpp

	#shoppingcart_lock
	external/shoppingcart_lock/main.cpp
	external/shoppingcart_lock/shoppingcart_lock.cpp	
)

set(EXTAPPLIST
	afsk_rx
	calculator
	font_viewer
	blespam
	nrf_rx
	analogtv
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
	#acars_rx
	shoppingcart_lock
)
