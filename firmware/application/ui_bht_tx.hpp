/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "bmp_bulb_on.hpp"
#include "bmp_bulb_off.hpp"
#include "bmp_bulb_ignore.hpp"

#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
//#include "receiver_model.hpp"
#include "portapack.hpp"

using namespace encoders;

#define CCIR_TONE_LENGTH (153600-1)		// 1536000*0.1
#define CCIR_DELTA_COEF (43.691)		// (65536*1024)/1536000
#define CCIR_SILENCE (614400-1)			// 400ms

namespace ui {

class BHTView : public View {
public:
	BHTView(NavigationView& nav);
	~BHTView();
	
	void focus() override;
	
	std::string title() const override { return "BHT transmit"; };

private:
	const uint32_t ccir_deltas[16] = {
		(uint32_t)(1981 * CCIR_DELTA_COEF),
		(uint32_t)(1124 * CCIR_DELTA_COEF),
		(uint32_t)(1197 * CCIR_DELTA_COEF),
		(uint32_t)(1275 * CCIR_DELTA_COEF),
		(uint32_t)(1358 * CCIR_DELTA_COEF),
		(uint32_t)(1446 * CCIR_DELTA_COEF),
		(uint32_t)(1540 * CCIR_DELTA_COEF),
		(uint32_t)(1640 * CCIR_DELTA_COEF),
		(uint32_t)(1747 * CCIR_DELTA_COEF),
		(uint32_t)(1860 * CCIR_DELTA_COEF),
		(uint32_t)(2400 * CCIR_DELTA_COEF),
		(uint32_t)(930  * CCIR_DELTA_COEF),
		(uint32_t)(2247 * CCIR_DELTA_COEF),
		(uint32_t)(991  * CCIR_DELTA_COEF),
		(uint32_t)(2110 * CCIR_DELTA_COEF),
		(uint32_t)(1055 * CCIR_DELTA_COEF)
	};

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;
	
	struct bht_city {
		std::string name;
		uint8_t freq_index;
		bool recent;
	};
	
	const bht_city bht_cities[122] = {
		{ "Aizenay",			0, false },
		{ "Albertville",		3, false },
		{ "Ales",				3, false },
		{ "Artannes/Indre",		5, false },
		{ "Avignon",			3, true },
		{ "Azay-le-Rideau",		5, false },
		{ "Baux Ste. Croix",	0, false },
		{ "Beaugency",			4, false },
		{ "Beaune",				4, false },
		{ "Betton",				2, false },
		{ "Bihorel",			0, true },
		{ "Blanquefort",		4, true },
		{ "Bobigny",			5, false },
		{ "Bouffere",			4, true },
		{ "Boulogne/Mer",		0, true },
		{ "Bourg-en-Bresse",	3, false },
		{ "Bourges",			0, false },
		{ "Bouscat",			0, false },
		{ "Carquefou",			5, false },
		{ "St. Cast",			0, false },
		{ "Caudebec/Caux",		3, true },
		{ "Cercy-la-Tour",		5, false },
		{ "Chamalieres",		5, false },
		{ "St. Chamond",		5, false },
		{ "Chapelle/Fgrtz",		2, false },
		{ "Charite/Loire",		3, false },
		{ "Charleville-Mzr",	1, false },
		{ "Chilly Mazarin",		5, false },
		{ "Clermont Frrd.",		5, false },
		{ "Cluses",				2, false },
		{ "Compiegne",			4, false },
		{ "Coulanges/Nevers",	5, false },
		{ "Cour Cheverny",		5, false },
		{ "Cournon Auvergne",	5, false },
		{ "Crolles",			5, true },
		{ "Cublize",			4, true },
		{ "Donges",				5, false },
		{ "Emalleville",		0, false },
		{ "Etrepagny",			0, false },
		{ "Fecamp",				0, false },
		{ "Ferriere",			0, false },
		{ "Ferte Imbault",		5, false },
		{ "Fontaine",			5, true },
		{ "Forbach",			3, false },
		{ "Fourchambault",		5, false },
		{ "Fresnay/Sarthe",		3, false },
		{ "St Fulgent",			5, true },
		{ "Gaillac",			3, true },
		{ "St. Georges/Grs",	0, false },
		{ "St. Gervais/Frt",	5, false },
		{ "Givors",				5, false },
		{ "Guichen",			2, false },
		{ "Guildo",				0, false },
		{ "Guipry",				2, false },
		{ "St Hilaire/Riez",	0, false },
		{ "Hossegor/Capbrtn",	0, true },
		{ "Houlbec-Cocherel",	0, false },
		{ "Huisseau/Cosson",	5, false },
		{ "Huningue",			5, false },
		{ "Iffendic",			2, false },
		{ "La Croix St. Ouen",	4, false },
		{ "Langrune/Mer",		0, false },
		{ "Le Neubourg",		2, true },
		{ "St Leger/Vignes",	5, false },
		{ "Levallois-Perret",	5, false },
		{ "Lille",				5, true },
		{ "Limoges",			5, false },
		{ "Longueil-Anel",		4, false },
		{ "Lormont",			5, true },
		{ "Mantes-la-Jolie",	5, false },
		{ "Martigues",			0, true },
		{ "Marzy",				5, false },
		{ "Ste. Memmie",		3, false },
		{ "Menton",				0, true },
		{ "Metz",				3, false },
		{ "Mezidon Canon",		1, false },
		{ "Millau",				5, false },
		{ "Miniac-Morvan",		2, false },
		{ "Mt. Pres Chambord",	5, false },
		{ "Montesson",			5, false },
		{ "Monts",				5, false },
		{ "Noisy-le-Grand",		4, true },
		{ "St Ouen",			5, false },
		{ "Ozoir/Ferriere",		5, false },
		{ "Pace",				2, false },
		{ "Pelussin",			5, false },
		{ "Petite Foret",		1, false },
		{ "Plestin/Greves",		0, false },
		{ "Pleumeur Bodou",		5, true },
		{ "Pont Audemer",		0, false },
		{ "Pontcharra",			5, true },
		{ "Pontchateau",		5, false },
		{ "Pressagny L'Org.",	0, false },
		{ "Remiremont",			4, true },
		{ "Ribeauville",		5, false },
		{ "La Roche sur Yon",	0, false },
		{ "Romorantin-Lant.",	5, false },
		{ "Rueil Malmaison",	5, false },
		{ "Sault-les-Rethel",	3, false },
		{ "Selles-St-Denis",	5, false },
		{ "Selles/Cher",		5, false },
		{ "Sens",				4, false },
		{ "Sezanne",			3, false },
		{ "Sommesous",			3, false },
		{ "Ste. Suzanne",		2, true },
		{ "Talence",			3, true },
		{ "Thionville",			3, false },
		{ "Thonon-les-Bains",	2, false },
		{ "Tours en Sologne",	5, true },
		{ "Trelaze",			5, true },
		{ "Trouville/Mer",		0, false },
		{ "Tulle",				2, false },
		{ "Ussel",				2, false },
		{ "Valberg",			5, true },
		{ "Valence",			5, false },
		{ "Velizy",				5, false },
		{ "Vesoul",				5, false },
		{ "Ville S. la Ferte",	0, false },
		{ "Villefrance/Saone",	5, false },
		{ "Villers Cotterets",	3, false },
		{ "Vitre",				2, false },
		{ "Vitry-le-Francois",	4, true }
	};
	
	const rf::Frequency bht_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 88000000 };
	
	char ccir_message[21];
	bool speaker_enabled = false;
	size_t _mode = 0;
	
	void ascii_to_ccir(char * ascii);
	void start_tx();
	void generate_message();
	void on_tx_progress(const int progress, const bool done);
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	const Style style_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	
	OptionsField options_mode {
		{ 10 * 8, 4 },
		10,
		{
			{ "Mode Xy.", 0 },
			{ "Mode EP.", 1 }
		}
	};
	
	Checkbox checkbox_speaker {
		{ 22 * 8, 4 },
		0,
		""
	};
	Image bmp_speaker {
		{ 204, 8, 16, 16 },
		&bitmap_speaker,
		ui::Color::white(),
		ui::Color::black()
	};
	
	Text text_header {
		{ 8 * 8, 3 * 8, 7 * 8, 16 },
		"Header:"
	};
	NumberField header_code_a {
		{ 16 * 8, 3 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	NumberField header_code_b {
		{ 18 * 8, 3 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	
	Text text_city {
		{ 4 * 8, 5 * 8, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code_xy {
		{ 16 * 8, 5 * 8 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	NumberField city_code_ep {
		{ 16 * 8, 5 * 8 },
		3,
		{ 0, 255 },
		1,
		' '
	};
	
	Text text_family {
		{ 7 * 8, 7 * 8, 8 * 8, 16 },
		"Famille:"
	};
	NumberField family_code_xy {
		{ 16 * 8, 7 * 8 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	OptionsField family_code_ep {
		{ 16 * 8, 7 * 8 },
		2,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};
	
	Text text_subfamily {
		{ 2 * 8, 9 * 8 + 2, 13 * 8, 16 },
		"Sous-famille:"
	};
	NumberField subfamily_code {
		{ 16 * 8, 9 * 8 + 2 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 8 * 8 + 6 },
		6,
		"Toutes"
	};
	
	Text text_receiver {
		{ 2 * 8, 13 * 8, 13 * 8, 16 },
		"ID recepteur:"
	};
	NumberField receiver_code {
		{ 16 * 8, 13 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 12 * 8 + 4 },
		4,
		"Tous"
	};
	
	Text text_freq {
		{ 6 * 8, 8 * 16, 10 * 8, 16 },
		"Frequence:"
	};
	OptionsField options_freq {
		{ 17 * 8, 8 * 16},
		7,
		{
			{ "31.3250", 0 },
			{ "31.3875", 1 },
			{ "31.4375", 2 },
			{ "31.4750", 3 },
			{ "31.6875", 4 },
			{ "31.9750", 5 },
			{ "TEST 88", 6 }
		}
	};
	
	Text text_relais {
		{ 8, 19 * 8, 13 * 8, 16 },
		"Etats relais:"
	};
	
	std::array<ImageOptionsField, 4> relay_states;
	
	ImageOptionsField::options_t relay_options = {
		{ &bulb_ignore_bmp[0], 0 },
		{ &bulb_off_bmp[0], 1 },
		{ &bulb_on_bmp[0], 2 }
	};
	
	ProgressBar progressbar {
		{ 5 * 8, 27 * 8, 20 * 8, 16 },
	};
	Text text_message {
		{ 5 * 8, 29 * 8, 20 * 8, 16 },
		""
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"START"
	};
	
	Checkbox checkbox_cligno {
		{ 16 * 8, 16 * 16 + 4},
		3,
		"J/N"
	};
	NumberField tempo_cligno {
		{ 24 * 8, 16 * 16 + 8},
		2,
		{ 1, 99 },
		1,
		' '
	};
	Text text_cligno {
		{ 26 * 8, 16 * 16 + 8, 2 * 8, 16 },
		"s."
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */
