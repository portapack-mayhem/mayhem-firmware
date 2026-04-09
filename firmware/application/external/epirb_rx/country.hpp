/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __COUNTRY_RX_H__
#define __COUNTRY_RX_H__

#include <cstdint>
#include <string>
#include "string_format.hpp"

namespace ui::external_app::epirb_rx {

class Country {
   public:
    int code{0};
    const char* alphaCode{};
    const char* longName{};
    const char* shortName{};

    inline void setValues(int code, const char* alphaCode, const char* longName, const char* shortName) {
        this->code = code;
        this->alphaCode = alphaCode;
        this->longName = longName;
        this->shortName = shortName;
    }

    inline std::string toString() {
        std::string result = alphaCode;
        result = result + "(" + to_string_dec_int(code) + " - " + longName + ")";
        return result;
    }

    static inline Country getCountry(int code) {
        Country result;
        switch (code) {
            /*case 201:
                result.setValues(201, "ALB", "Albania", "Albania");
                break;
            case 202:
                result.setValues(202, "AND", "Andorra", "Andorra");
                break;
            case 203:
                result.setValues(203, "AUT", "Austria", "Austria");
                break;
            case 204:
                result.setValues(204, "AZR", "Azores", "Azores");
                break;*/
            case 205:
                result.setValues(205, "BEL", "Belgium", "Belgium");
                break;
            /*case 206:
                result.setValues(206, "BLR", "Belarus", "Belarus");
                break;
            case 207:
                result.setValues(207, "BUL", "Bulgaria", "Bulgaria");
                break;
            case 208:
                result.setValues(208, "VAT", "Vatican City State", "Vatican");
                break;
            case 209:
                result.setValues(209, "CYP", "Cyprus", "Cyprus");
                break;
            case 210:
                result.setValues(210, "CYP", "Cyprus", "Cyprus");
                break;*/
            case 211:
                result.setValues(211, "GER", "Germany (Federal Republic of)", "Germany");
                break;
            /*case 212:
                result.setValues(212, "DEN", "Denmark", "Denmark");
                break;
            case 213:
                result.setValues(213, "GEO", "Georgia", "Georgia");
                break;
            case 214:
                result.setValues(214, "MDA", "Moldova (Republic of)", "Moldova");
                break;
            case 215:
                result.setValues(215, "MLT", "Malta", "Malta");
                break;
            case 216:
                result.setValues(216, "ARM", "Armenia", "Armenia");
                break;*/
            case 218:
                result.setValues(218, "GER", "Germany (Federal Republic of)", "Germany");
                break;
            /*case 219:
                result.setValues(219, "DEN", "Denmark", "Denmark");
                break;
            case 220:
                result.setValues(220, "DEN", "Denmark", "Denmark");
                break;*/
            case 224:
                result.setValues(224, "ESP", "Spain", "Spain");
                break;
            case 225:
                result.setValues(225, "ESP", "Spain", "Spain");
                break;
            case 226:
                result.setValues(226, "FRA", "France", "France");
                break;
            case 227:
                result.setValues(227, "FRA", "France", "France");
                break;
            case 228:
                result.setValues(228, "FRA", "France", "France");
                break;
            /*case 229:
                result.setValues(229, "MLT", "Malta", "Malta");
                break;
            case 230:
                result.setValues(230, "FIN", "Finland", "Finland");
                break;
            case 231:
                result.setValues(231, "FRO", "Faroe Islands", "Faroe Is");
                break;*/
            case 232:
                result.setValues(232, "GBR", "United Kingdom", "UK");
                break;
            case 233:
                result.setValues(233, "GBR", "United Kingdom", "UK");
                break;
            case 234:
                result.setValues(234, "GBR", "United Kingdom", "UK");
                break;
            case 235:
                result.setValues(235, "GBR", "United Kingdom", "UK");
                break;
            /*case 236:
                result.setValues(236, "GIB", "Gibraltar", "Gibraltar");
                break;
            case 237:
                result.setValues(237, "GRE", "Greece", "Greece");
                break;
            case 238:
                result.setValues(238, "CRO", "Croatia (Republic of)", "Croatia");
                break;
            case 239:
                result.setValues(239, "GRE", "Greece", "Greece");
                break;
            case 240:
                result.setValues(240, "GRE", "Greece", "Greece");
                break;
            case 241:
                result.setValues(241, "GRE", "Greece", "Greece");
                break;
            case 242:
                result.setValues(242, "MAR", "Morocco", "Morocco");
                break;
            case 243:
                result.setValues(243, "HUN", "Hungary (Republic of)", "Hungary");
                break;
            case 244:
                result.setValues(244, "NET", "Netherlands (Kingdom of the)", "Netherl.");
                break;
            case 245:
                result.setValues(245, "NET", "Netherlands (Kingdom of the)", "Netherl.");
                break;
            case 246:
                result.setValues(246, "NET", "Netherlands (Kingdom of the)", "Netherl.");
                break;
            case 247:
                result.setValues(247, "ITA", "Italy", "Italy");
                break;
            case 248:
                result.setValues(248, "ITA", "Italy", "Italy");
                break;
            case 249:
                result.setValues(249, "ITA", "Italy", "Italy");
                break;
            case 250:
                result.setValues(250, "IRL", "Ireland", "Ireland");
                break;
            case 251:
                result.setValues(251, "ISL", "Iceland", "Iceland");
                break;
            case 252:
                result.setValues(252, "LIE", "Liechtenstein (Principality of)", "Liechten.");
                break;
            case 253:
                result.setValues(253, "LUX", "Luxembourg", "Luxemb.");
                break;
            case 254:
                result.setValues(254, "MON", "Monaco (Principality of)", "Monaco");
                break;
            case 255:
                result.setValues(255, "MAD", "Madeira", "Madeira");
                break;
            case 256:
                result.setValues(256, "NOR", "Norway", "Norway");
                break;
            case 257:
                result.setValues(257, "NOR", "Norway", "Norway");
                break;
            case 258:
                result.setValues(258, "NOR", "Norway", "Norway");
                break;*/
            case 259:
                result.setValues(259, "NOR", "Norway", "Norway");
                break;
            case 261:
                result.setValues(261, "POL", "Poland (Republic of)", "Poland");
                break;
            /*case 262:
                result.setValues(262, "MNE", "Montenegro (Republic of)", "Monteneg.");
                break;
            case 263:
                result.setValues(263, "POR", "Portugal", "Portugal");
                break;
            case 264:
                result.setValues(264, "ROM", "Romania", "Romania");
                break;
            case 265:
                result.setValues(265, "SWE", "Sweden", "Sweden");
                break;
            case 266:
                result.setValues(266, "SWE", "Sweden", "Sweden");
                break;
            case 267:
                result.setValues(267, "SVK", "Slovakia", "Slovakia");
                break;
            case 268:
                result.setValues(268, "SMR", "San Marino (Republic of)", "San Marino");
                break;*/
            case 269:
                result.setValues(269, "SUI", "Switzerland (Confederation of)", "Switzerl.");
                break;
            /*case 270:
                result.setValues(270, "CZE", "Czech Republic", "Czech Rep");
                break;
            case 271:*/
                result.setValues(271, "TUR", "Turkey", "Turkey");
                break;
            case 272:
                result.setValues(272, "UKR", "Ukraine", "Ukraine");
                break;
            case 273:
                result.setValues(273, "RUS", "Russian Federation", "Russia");
                break;
            /*case 274:
                result.setValues(274, "MKD", "Macedonia (Former Yugoslav Rep)", "Macedonia");
                break;
            case 275:
                result.setValues(275, "LAT", "Latvia (Republic of)", "Latvia");
                break;
            case 276:
                result.setValues(276, "EST", "Estonia (Republic of)", "Estonia");
                break;
            case 277:
                result.setValues(277, "LIT", "Lithuania (Republic of)", "Lithuania");
                break;
            case 278:
                result.setValues(278, "SLO", "Slovenia (Republic of)", "Slovenia");
                break;
            case 279:
                result.setValues(279, "SRB", "Serbia (Republic of)", "Serbia");
                break;
            case 301:
                result.setValues(301, "ANG", "Anguilla", "Anguilla");
                break;
            case 303:
                result.setValues(303, "USA", "Alaska (State of)", "Alaska");
                break;
            case 304:
                result.setValues(304, "ATG", "Antigua and Barbuda", "Antigua");
                break;
            case 305:
                result.setValues(305, "ATG", "Antigua and Barbuda", "Antigua");
                break;
            case 306:
                result.setValues(306, "ANT", "Netherlands Antilles", "Antilles");
                break;
            case 307:
                result.setValues(307, "ARU", "Aruba", "Aruba");
                break;
            case 308:
                result.setValues(308, "BAH", "Bahamas (Commonwealth of the)", "Bahamas");
                break;
            case 309:
                result.setValues(309, "BAH", "Bahamas (Commonwealth of the)", "Bahamas");
                break;
            case 310:
                result.setValues(310, "BER", "Bermuda", "Bermuda");
                break;
            case 311:
                result.setValues(311, "BLZ", "Belize", "Belize");
                break;
            case 312:
                result.setValues(312, "BRB", "Barbados", "Barbados");
                break;
            case 314:
                result.setValues(314, "BRV", "British Virgin Islands", "Virg. IsB");
                break;*/
            case 316:
                result.setValues(316, "CAN", "Canada", "Canada");
                break;
            /*case 319:
                result.setValues(319, "CAY", "Cayman Islands", "Cayman Is");
                break;
            case 321:
                result.setValues(321, "CRC", "Costa Rica", "Costa Rica");
                break;
            case 323:
                result.setValues(323, "CUB", "Cuba", "Cuba");
                break;
            case 325:
                result.setValues(325, "DOM", "Dominica (Commonwealth of)", "Dominica");
                break;
            case 327:
                result.setValues(327, "DOM", "Dominican Republic", "Dominic.R");
                break;
            case 329:
                result.setValues(329, "GRL", "Greenland", "Greenland");
                break;
            case 330:
                result.setValues(330, "GRN", "Grenada", "Grenada");
                break;
            case 331:
                result.setValues(331, "GLP", "Guadeloupe (French Dept. of)", "Guadel.");
                break;
            case 332:
                result.setValues(332, "GUA", "Guatemala (Republic of)", "Guatemala");
                break;
            case 334:
                result.setValues(334, "HON", "Honduras (Republic of)", "Honduras");
                break;
            case 336:
                result.setValues(336, "HAI", "Haiti (Republic of)", "Haiti");
                break;*/
            case 338:
                result.setValues(338, "USA", "United States of America", "USA");
                break;
            /*case 339:
                result.setValues(339, "JAM", "Jamaica", "Jamaica");
                break;
            case 341:
                result.setValues(341, "SKN", "Saint Kitts and Nevis", "St Kitts");
                break;
            case 343:
                result.setValues(343, "LCA", "Saint Lucia", "St Lucia");
                break;*/
            case 345:
                result.setValues(345, "MEX", "Mexico", "Mexico");
                break;
            /*case 347:
                result.setValues(347, "MTQ", "Martinique (French Dept. of)", "Martin.");
                break;
            case 348:
                result.setValues(348, "MSV", "Montserrat", "Montserrat");
                break;
            case 350:
                result.setValues(350, "NIC", "Nicaragua", "Nicaragua");
                break;
            case 351:
                result.setValues(351, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 352:
                result.setValues(352, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 353:
                result.setValues(353, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 354:
                result.setValues(354, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 355:
                result.setValues(355, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 356:
                result.setValues(356, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 357:
                result.setValues(357, "PAN", "Panama (Republic of)", "Panama");
                break;
            case 358:
                result.setValues(358, "PUR", "Puerto Rico", "Puerto R.");
                break;
            case 359:
                result.setValues(359, "SAL", "El Salvador (Republic of)", "Salvador");
                break;
            case 361:
                result.setValues(361, "SPM", "Saint Pierre and Miquelon", "St Pierre");
                break;
            case 362:
                result.setValues(362, "TRI", "Trinidad and Tobago", "Trinidad");
                break;
            case 364:
                result.setValues(364, "TCA", "Turks and Caicos Islands", "Turks Is");
                break;*/
            case 366:
                result.setValues(366, "USA", "United States of America", "USA");
                break;
            case 367:
                result.setValues(367, "USA", "United States of America", "USA");
                break;
            case 368:
                result.setValues(368, "USA", "United States of America", "USA");
                break;
            case 369:
                result.setValues(369, "USA", "United States of America", "USA");
                break;
            /*case 375:
                result.setValues(375, "VIN", "Saint Vincent and Grenadines", "St Vincent");
                break;
            case 376:
                result.setValues(376, "VIN", "Saint Vincent and Grenadines", "St Vincent");
                break;
            case 377:
                result.setValues(377, "VIN", "Saint Vincent and Grenadines", "St Vincent");
                break;
            case 378:
                result.setValues(378, "IVB", "British Virgin Islands", "Virg. IsI");
                break;*/
            case 379:
                result.setValues(379, "USA", "United States Virgin Islands", "Virg. IsU");
                break;
            /*case 401:
                result.setValues(401, "AFG", "Afghanistan", "Afghan.");
                break;
            case 403:
                result.setValues(403, "SAU", "Saudi Arabia (Kingdom of)", "Saudi Ar.");
                break;
            case 405:
                result.setValues(405, "BAN", "Bangladesh (People's Rep of)", "Banglad.");
                break;
            case 408:
                result.setValues(408, "BAH", "Bahrain (Kingdom of)", "Bahrain");
                break;
            case 410:
                result.setValues(410, "BHU", "Bhutan (Kingdom of)", "Bhutan");
                break;
            case 412:
                result.setValues(412, "MYA", "Myanmar (Union of)", "Myanmar");
                break;
            case 413:
                result.setValues(413, "SRI", "Sri Lanka (Democratic Socialist Rep of)", "Sri Lanka");
                break;
            case 414:
                result.setValues(414, "SRI", "Sri Lanka (Democratic Socialist Rep of)", "Sri Lanka");
                break;
            case 416:
                result.setValues(416, "TAI", "Taiwan", "Taiwan");
                break;
            case 417:
                result.setValues(417, "SRI", "Sri Lanka (Democratic Socialist Rep of)", "Sri Lanka");
                break;*/
            case 419:
                result.setValues(419, "IND", "India (Republic of)", "India");
                break;
            /*case 422:
                result.setValues(422, "INS", "Indonesia (Republic of)", "Indonesia");
                break;
            case 423:
                result.setValues(423, "INS", "Indonesia (Republic of)", "Indonesia");
                break;
            case 425:
                result.setValues(425, "IRA", "Iran (Islamic Republic of)", "Iran");
                break;
            case 428:
                result.setValues(428, "IRQ", "Iraq (Republic of)", "Iraq");
                break;*/
            case 431:
                result.setValues(431, "ISR", "Israel (State of)", "Israel");
                break;
            /*case 432:
                result.setValues(432, "JAP", "Japan", "Japan");
                break;
            case 434:
                result.setValues(434, "TKM", "Turkmenistan", "Turkmen.");
                break;
            case 436:
                result.setValues(436, "JOR", "Jordan (Hashemite Kingdom of)", "Jordan");
                break;
            case 438:
                result.setValues(438, "KOR", "Korea (Republic of)", "Korea S");
                break;
            case 440:
                result.setValues(440, "KOR", "Korea (Republic of)", "Korea S");
                break;
            case 441:
                result.setValues(441, "KOR", "Korea (Republic of)", "Korea S");
                break;
            case 443:
                result.setValues(443, "KOW", "Kuwait (State of)", "Kuwait");
                break;
            case 445:
                result.setValues(445, "LAO", "Lao People's Democratic Rep", "Lao");
                break;
            case 447:
                result.setValues(447, "LEB", "Lebanon", "Lebanon");
                break;
            case 450:
                result.setValues(450, "MAL", "Malaysia", "Malaysia");
                break;
            case 451:
                result.setValues(451, "MAL", "Malaysia", "Malaysia");
                break;
            case 453:
                result.setValues(453, "MLD", "Maldives (Republic of)", "Maldives");
                break;
            case 455:
                result.setValues(455, "MON", "Mongolia", "Mongolia");
                break;
            case 457:
                result.setValues(457, "NEP", "Nepal", "Nepal");
                break;
            case 459:
                result.setValues(459, "OMA", "Oman (Sultanate of)", "Oman");
                break;
            case 461:
                result.setValues(461, "PAK", "Pakistan (Islamic Republic of)", "Pakistan");
                break;
            case 463:
                result.setValues(463, "PHI", "Philippines (Republic of the)", "Philip.");
                break;
            case 466:
                result.setValues(466, "PHI", "Philippines (Republic of the)", "Philip.");
                break;*/
            case 468:
                result.setValues(468, "QAT", "Qatar (State of)", "Qatar");
                break;
            /*case 470:
                result.setValues(470, "UAE", "United Arab Emirates", "UAE");
                break;
            case 472:
                result.setValues(472, "TJK", "Tajikistan", "Tajik.");
                break;
            case 473:
                result.setValues(473, "SAU", "Saudi Arabia (Kingdom of)", "Saudi Ar.");
                break;
            case 475:
                result.setValues(475, "SAU", "Saudi Arabia (Kingdom of)", "Saudi Ar.");
                break;
            case 477:
                result.setValues(477, "HKG", "Hong Kong", "Hong Kong");
                break;
            case 478:
                result.setValues(478, "BIH", "Bosnia and Herzegovina", "Bosnia");
                break;
            case 501:
                result.setValues(501, "ADE", "Adélie Land", "Adelie L");
                break;*/
            case 503:
                result.setValues(503, "AUS", "Australia", "Australia");
                break;
            /*case 506:
                result.setValues(506, "FIJ", "Fiji (Republic of)", "Fiji");
                break;
            case 508:
                result.setValues(508, "PYF", "French Polynesia", "Polynesia");
                break;
            case 510:
                result.setValues(510, "KIR", "Kiribati (Republic of)", "Kiribati");
                break;
            case 512:
                result.setValues(512, "NZL", "New Zealand", "New Zeal.");
                break;
            case 514:
                result.setValues(514, "NZL", "New Zealand", "New Zeal.");
                break;
            case 515:
                result.setValues(515, "NZL", "New Zealand", "New Zeal.");
                break;
            case 516:
                result.setValues(516, "NCL", "New Caledonia", "New Cal.");
                break;
            case 518:
                result.setValues(518, "NZL", "New Zealand", "New Zeal.");
                break;
            case 520:
                result.setValues(520, "FSM", "Micronesia (Federated States of)", "Micrones.");
                break;
            case 523:
                result.setValues(523, "NAU", "Nauru (Republic of)", "Nauru");
                break;
            case 525:
                result.setValues(525, "NZL", "New Zealand", "New Zeal.");
                break;
            case 529:
                result.setValues(529, "KIR", "Kiribati (Republic of)", "Kiribati");
                break;
            case 533:
                result.setValues(533, "MAU", "Mauritius (Republic of)", "Mauritius");
                break;
            case 536:
                result.setValues(536, "NZL", "New Zealand", "New Zeal.");
                break;
            case 538:
                result.setValues(538, "MHL", "Marshall Islands (Republic of)", "Marshall");
                break;
            case 540:
                result.setValues(540, "NZL", "New Zealand", "New Zeal.");
                break;
            case 542:
                result.setValues(542, "NIU", "Niue", "Niue");
                break;
            case 544:
                result.setValues(544, "NRU", "Nauru (Republic of)", "Nauru");
                break;
            case 546:
                result.setValues(546, "PYF", "French Polynesia", "Polynesia");
                break;
            case 548:
                result.setValues(548, "PHI", "Philippines (Republic of the)", "Philip.");
                break;
            case 553:
                result.setValues(553, "PLW", "Palau (Republic of)", "Palau");
                break;
            case 555:
                result.setValues(555, "PNG", "Papua New Guinea", "Papua NG");
                break;
            case 557:
                result.setValues(557, "SLM", "Solomon Islands", "Solomon");
                break;
            case 559:
                result.setValues(559, "SOL", "Solomon Islands", "Solomon");
                break;
            case 561:
                result.setValues(561, "SAM", "Samoa (Independent State of)", "Samoa");
                break;
            case 563:
                result.setValues(563, "SAM", "Samoa (Independent State of)", "Samoa");
                break;
            case 564:
                result.setValues(564, "SAM", "Samoa (Independent State of)", "Samoa");
                break;
            case 565:
                result.setValues(565, "SAM", "Samoa (Independent State of)", "Samoa");
                break;
            case 566:
                result.setValues(566, "SAM", "Samoa (Independent State of)", "Samoa");
                break;
            case 567:
                result.setValues(567, "TON", "Tonga (Kingdom of)", "Tonga");
                break;
            case 570:
                result.setValues(570, "TUV", "Tuvalu", "Tuvalu");
                break;
            case 572:
                result.setValues(572, "VAN", "Vanuatu (Republic of)", "Vanuatu");
                break;
            case 574:
                result.setValues(574, "VAN", "Vanuatu (Republic of)", "Vanuatu");
                break;
            case 576:
                result.setValues(576, "VAN", "Vanuatu (Republic of)", "Vanuatu");
                break;
            case 577:
                result.setValues(577, "VAN", "Vanuatu (Republic of)", "Vanuatu");
                break;
            case 578:
                result.setValues(578, "WLF", "Wallis and Futuna Islands", "Wallis");
                break;
            case 601:
                result.setValues(601, "RSA", "South Africa (Republic of)", "South Afr.");
                break;
            case 603:
                result.setValues(603, "AGO", "Angola (Republic of)", "Angola");
                break;
            case 605:
                result.setValues(605, "ALG", "Algeria (People's Democratic Rep of)", "Algeria");
                break;
            case 607:
                result.setValues(607, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 608:
                result.setValues(608, "ASC", "Ascension Island", "Ascension");
                break;
            case 609:
                result.setValues(609, "BDI", "Burundi (Republic of)", "Burundi");
                break;
            case 610:
                result.setValues(610, "BEN", "Benin (Republic of)", "Benin");
                break;
            case 611:
                result.setValues(611, "BOT", "Botswana (Republic of)", "Botswana");
                break;
            case 612:
                result.setValues(612, "CAF", "Central African Republic", "Cen.Afr.R");
                break;
            case 613:
                result.setValues(613, "CAM", "Cameroon (Republic of)", "Cameroon");
                break;
            case 615:
                result.setValues(615, "CON", "Congo (Republic of the)", "Congo");
                break;
            case 616:
                result.setValues(616, "COM", "Comoros (Union of)", "Comoros");
                break;
            case 617:
                result.setValues(617, "CPV", "Cape Verde (Republic of)", "Cape Verde");
                break;
            case 618:
                result.setValues(618, "CRO", "Crozet Archipelago", "Crozet");
                break;
            case 619:
                result.setValues(619, "CIV", "Côte d'Ivoire (Republic of)", "Cote d'Iv.");
                break;
            case 621:
                result.setValues(621, "DJI", "Djibouti (Republic of)", "Djibouti");
                break;
            case 622:
                result.setValues(622, "EGY", "Egypt (Arab Republic of)", "Egypt");
                break;
            case 624:
                result.setValues(624, "ETH", "Ethiopia (Federal Democratic Rep of)", "Ethiopia");
                break;
            case 625:
                result.setValues(625, "ERI", "Eritrea", "Eritrea");
                break;
            case 626:
                result.setValues(626, "GAB", "Gabonese Republic", "Gabon");
                break;
            case 627:
                result.setValues(627, "GHA", "Ghana", "Ghana");
                break;
            case 629:
                result.setValues(629, "GAM", "Gambia (Republic of the)", "Gambia");
                break;
            case 630:
                result.setValues(630, "GNB", "Guinea-Bissau (Republic of)", "Guinea-B.");
                break;
            case 631:
                result.setValues(631, "GUI", "Guinea (Republic of)", "Guinea");
                break;
            case 632:
                result.setValues(632, "EQG", "Equatorial Guinea (Republic of)", "Eq. Guinea");
                break;
            case 633:
                result.setValues(633, "KER", "Kerguelen Islands", "Kerguelen");
                break;
            case 634:
                result.setValues(634, "KEN", "Kenya (Republic of)", "Kenya");
                break;
            case 635:
                result.setValues(635, "LSO", "Lesotho (Kingdom of)", "Lesotho");
                break;
            case 636:
                result.setValues(636, "LBR", "Liberia (Republic of)", "Liberia");
                break;
            case 637:
                result.setValues(637, "LBY", "Libya", "Libya");
                break;
            case 642:
                result.setValues(642, "MAD", "Madagascar (Republic of)", "Madagas.");
                break;
            case 644:
                result.setValues(644, "MLI", "Mali (Republic of)", "Mali");
                break;
            case 645:
                result.setValues(645, "MAU", "Mauritania (Islamic Republic of)", "Mauritan.");
                break;
            case 647:
                result.setValues(647, "MAU", "Mauritius (Republic of)", "Mauritius");
                break;
            case 649:
                result.setValues(649, "MAR", "Morocco", "Morocco");
                break;
            case 651:
                result.setValues(651, "MOZ", "Mozambique (Republic of)", "Mozamb.");
                break;
            case 654:
                result.setValues(654, "NAM", "Namibia (Republic of)", "Namibia");
                break;
            case 655:
                result.setValues(655, "NIG", "Niger (Republic of the)", "Niger");
                break;
            case 656:
                result.setValues(656, "NGR", "Nigeria (Federal Republic of)", "Nigeria");
                break;
            case 657:
                result.setValues(657, "NGR", "Nigeria (Federal Republic of)", "Nigeria");
                break;
            case 659:
                result.setValues(659, "REU", "Reunion (French Dept. of)", "Reunion");
                break;
            case 660:
                result.setValues(660, "RWA", "Rwanda (Republic of)", "Rwanda");
                break;
            case 661:
                result.setValues(661, "STH", "Saint Helena", "St Helena");
                break;
            case 662:
                result.setValues(662, "SEN", "Senegal (Republic of)", "Senegal");
                break;
            case 663:
                result.setValues(663, "SEN", "Senegal (Republic of)", "Senegal");
                break;
            case 664:
                result.setValues(664, "SEY", "Seychelles (Republic of)", "Seychel.");
                break;
            case 665:
                result.setValues(665, "SEY", "Seychelles (Republic of)", "Seychel.");
                break;
            case 666:
                result.setValues(666, "SLE", "Sierra Leone", "S. Leone");
                break;
            case 667:
                result.setValues(667, "SLE", "Sierra Leone", "S. Leone");
                break;
            case 668:
                result.setValues(668, "SLE", "Sierra Leone", "S. Leone");
                break;
            case 669:
                result.setValues(669, "SOM", "Somalia (Democratic Republic of)", "Somalia");
                break;
            case 670:
                result.setValues(670, "SOM", "Somalia (Democratic Republic of)", "Somalia");
                break;
            case 671:
                result.setValues(671, "SOM", "Somalia (Democratic Republic of)", "Somalia");
                break;
            case 672:
                result.setValues(672, "SOM", "Somalia (Democratic Republic of)", "Somalia");
                break;
            case 674:
                result.setValues(674, "TAN", "Tanzania (United Republic of)", "Tanzania");
                break;
            case 675:
                result.setValues(675, "TAN", "Tanzania (United Republic of)", "Tanzania");
                break;
            case 676:
                result.setValues(676, "ETH", "Ethiopia (Federal Democratic Rep of)", "Ethiopia");
                break;
            case 677:
                result.setValues(677, "CHA", "Chad (Republic of)", "Chad");
                break;
            case 678:
                result.setValues(678, "TOG", "Togolese Republic", "Togo");
                break;
            case 679:
                result.setValues(679, "TUN", "Tunisia", "Tunisia");
                break;
            case 680:
                result.setValues(680, "UGA", "Uganda (Republic of)", "Uganda");
                break;
            case 681:
                result.setValues(681, "COD", "Congo (Democratic Republic of the)", "DR Congo");
                break;
            case 682:
                result.setValues(682, "ZAM", "Zambia (Republic of)", "Zambia");
                break;
            case 683:
                result.setValues(683, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 684:
                result.setValues(684, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 685:
                result.setValues(685, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 686:
                result.setValues(686, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 687:
                result.setValues(687, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 688:
                result.setValues(688, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 689:
                result.setValues(689, "SAR", "Saint Paul and Amsterdam Is", "St Paul");
                break;
            case 691:
                result.setValues(691, "ZIM", "Zimbabwe (Republic of)", "Zimbabwe");
                break;
            case 701:
                result.setValues(701, "ARG", "Argentine Republic", "Argentina");
                break;
            case 710:
                result.setValues(710, "BRA", "Brazil", "Brazil");
                break;
            case 720:
                result.setValues(720, "BOL", "Bolivia", "Bolivia");
                break;
            case 725:
                result.setValues(725, "CHI", "Chile", "Chile");
                break;
            case 730:
                result.setValues(730, "COL", "Colombia", "Colombia");
                break;
            case 735:
                result.setValues(735, "ECU", "Ecuador", "Ecuador");
                break;
            case 740:
                result.setValues(740, "FAL", "Falkland Islands (Malvinas)", "Falkland I");
                break;
            case 745:
                result.setValues(745, "GUI", "Guiana (French Dept. Of)", "Guiana");
                break;
            case 750:
                result.setValues(750, "GUY", "Guyana", "Guyana");
                break;
            case 755:
                result.setValues(755, "PAR", "Paraguay", "Paraguay");
                break;
            case 760:
                result.setValues(760, "PER", "Peru", "Peru");
                break;
            case 765:
                result.setValues(765, "SUR", "Suriname", "Suriname");
                break;
            case 770:
                result.setValues(770, "URU", "Uruguay (Eastern Republic of)", "Uruguay");
                break;
            case 775:
                result.setValues(775, "VEN", "Venezuela (Bolivarian Republic of)", "Venezuela");
                break;*/
            default:
                result.setValues(code, "UNK", "Unknown", "Unknown");
                break;
        }
        return result;
    }
};

}  // namespace ui::external_app::epirb_rx

#endif  // __COUNTRY_RX_H__