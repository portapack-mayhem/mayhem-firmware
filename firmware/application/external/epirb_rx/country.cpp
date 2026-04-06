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

#include "country.hpp"

namespace ui::external_app::epirb_rx {

using std::string;

string Country::toString() {
    string result = alphaCode;
    result = result + "(" + std::to_string(code) + " - " + longName;
    return result;
}

void Country::setValues(int code, string alphaCode, string longName, string shortName) {
    this->code = code;
    this->alphaCode = alphaCode;
    this->longName = longName;
    this->shortName = shortName;
}

Country Country::getCountry(int code) {
    Country result;
    switch (code) {
  /*      case 501:
            result.setValues(501, "ADE", "Adelie Land", "AdelieLand");
            break;
        case 201:
            result.setValues(201, "ALB", "Albania", "Albania");
            break;
        case 202:
            result.setValues(202, "AND", "Andorra", "Andorra");
            break;
        case 203:
            result.setValues(203, "AUT", "Austria", "Austria");
            break;
        case 204:
            result.setValues(204, "AZC", "Azores", "Azores");
            break;
        case 205:
            result.setValues(205, "BEL", "Belgium", "Belgium");
            break;
        case 206:
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
            break;
        case 211:
            result.setValues(211, "GER", "Germany", "Germany");
            break;
        case 212:
            result.setValues(212, "CYP", "Cyprus", "Cyprus");
            break;
        case 213:
            result.setValues(213, "GOG", "Georgia", "Georgia");
            break;
        case 214:
            result.setValues(214, "MOL", "Moldova", "Moldova");
            break;
        case 215:
            result.setValues(215, "MAL", "Malta", "MALTA");
            break;
        case 216:
            result.setValues(216, "ARM", "Armenia", "Armenia");
            break;
        case 218:
            result.setValues(218, "GER", "Germany", "Germany");
            break;
        case 219:
            result.setValues(219, "DEN", "Denmark", "Denmark");
            break;
        case 220:
            result.setValues(220, "DEN", "Denmark", "Denmark");
            break;
        case 224:
            result.setValues(224, "SPA", "Spain", "Spain");
            break;
        case 225:
            result.setValues(225, "SPA", "Spain", "Spain");
            break;*/
        case 226:
            result.setValues(226, "FRA", "France", "France");
            break;
        case 227:
            result.setValues(227, "FRA", "France", "France");
            break;
        case 228:
            result.setValues(228, "FRA", "France", "France");
            break;
/*        case 229:
            result.setValues(229, "MAL", "Malta", "Malta");
            break;
        case 230:
            result.setValues(230, "FIN", "Finland", "Finland");
            break;
        case 231:
            result.setValues(231, "FAR", "Faroe Islands", "Faro Isle");
            break;
        case 232:
            result.setValues(232, "UKM", "United Kingdom", "G Britain");
            break;
        case 233:
            result.setValues(233, "UKM", "United Kingdom", "G Britain");
            break;
        case 234:
            result.setValues(234, "UKM", "United Kingdom", "G Britain");
            break;
        case 235:
            result.setValues(235, "UKM", "United Kingdom", "G Britain");
            break;
        case 236:
            result.setValues(236, "GIB", "Gibraltar", "Gibraltar");
            break;
        case 237:
            result.setValues(237, "GRE", "Greece", "Greece");
            break;
        case 238:
            result.setValues(238, "CRT", "Croatia", "Croatia");
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
            result.setValues(242, "MOR", "Morocco", "Morocco");
            break;
        case 243:
            result.setValues(243, "HUN", "Hungary", "Hungary");
            break;
        case 244:
            result.setValues(244, "NET", "Netherlands", "Netherland");
            break;
        case 245:
            result.setValues(245, "NET", "Netherlands", "Netherland");
            break;
        case 246:
            result.setValues(246, "NET", "Netherlands", "Netherland");
            break;
        case 247:
            result.setValues(247, "ITA", "Italy", "Italy");
            break;
        case 248:
            result.setValues(248, "MAL", "Malta", "Malta");
            break;
        case 249:
            result.setValues(249, "MAL", "Malta", "Malta");
            break;
        case 250:
            result.setValues(250, "IRE", "Ireland", "Ireland");
            break;
        case 251:
            result.setValues(251, "ICE", "Iceland", "Iceland");
            break;
        case 252:
            result.setValues(252, "LIE", "Liechtenstein", "Liechten");
            break;
        case 253:
            result.setValues(253, "LUX", "Luxembourg", "Luxembourg");
            break;
        case 254:
            result.setValues(254, "MON", "Monaco", "Monaco");
            break;
        case 255:
            result.setValues(255, "MAE", "Madeira", "Madeira");
            break;
        case 256:
            result.setValues(256, "MAL", "Malta", "Malta");
            break;
        case 257:
            result.setValues(257, "NOR", "Norway", "Norway");
            break;
        case 258:
            result.setValues(258, "NOR", "Norway", "Norway");
            break;
        case 259:
            result.setValues(259, "NOR", "Norway", "Norway");
            break;
        case 261:
            result.setValues(261, "POL", "Poland", "Poland");
            break;
        case 262:
            result.setValues(262, "MNT", "Montenegro", "Montenegro");
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
            result.setValues(267, "SLV", "Slovak Republic", "Slovakia");
            break;
        case 268:
            result.setValues(268, "SAN", "San Marino", "San Marino");
            break;
        case 269:
            result.setValues(269, "SWT", "Switzerland", "Swiss");
            break;
        case 270:
            result.setValues(270, "CZH", "Czech Republic", "Czech Rep");
            break;
        case 271:
            result.setValues(271, "TUR", "TÃƒÂ¼rkiye", "TÃƒÂ¼rkiye");
            break;
        case 272:
            result.setValues(272, "UKR", "Ukraine", "Ukraine");
            break;
        case 273:
            result.setValues(273, "RUS", "Russian Federation", "Russia");
            break;
        case 274:
            result.setValues(274, "MKD", "North Macedonia (Republic of)", "North Mac");
            break;
        case 275:
            result.setValues(275, "LAT", "Latvia", "Latvia");
            break;
        case 276:
            result.setValues(276, "EST", "Estonia", "Estonia");
            break;
        case 277:
            result.setValues(277, "LIT", "Lithuania", "Lithuania");
            break;
        case 278:
            result.setValues(278, "SVN", "Slovenia", "Slovenia");
            break;
        case 279:
            result.setValues(279, "SER", "Serbia", "Serbia");
            break;
        case 301:
            result.setValues(301, "ANA", "Anguilla", "Anguilla");
            break;
        case 303:
            result.setValues(303, "ALA", "Alaska (State of)", "Alaska");
            break;
        case 304:
            result.setValues(304, "ANT", "Antigua and Barbuda", "Antigua");
            break;
        case 305:
            result.setValues(305, "ANT", "Antigua and Barbuda", "Antigua");
            break;
        case 306:
            result.setValues(306, "NEA", "Netherlands Antilles", "N Antilles");
            break;
        case 307:
            result.setValues(307, "ARU", "Aruba", "Aruba");
            break;
        case 308:
            result.setValues(308, "BAA", "Bahamas", "Bahamas");
            break;
        case 309:
            result.setValues(309, "BAA", "Bahamas", "Bahamas");
            break;
        case 310:
            result.setValues(310, "BER", "Bermuda", "Bermuda");
            break;
        case 311:
            result.setValues(311, "BAA", "Bahamas", "Bahamas");
            break;
        case 312:
            result.setValues(312, "BEZ", "Belize", "Belize");
            break;
        case 314:
            result.setValues(314, "BAR", "Barbados", "Barbados");
            break;
        case 316:
            result.setValues(316, "CAN", "Canada", "Canada");
            break;
        case 319:
            result.setValues(319, "CAY", "Cayman Islands", "Cayman Is");
            break;
        case 321:
            result.setValues(321, "COS", "Costa Rica", "Costa Rica");
            break;
        case 323:
            result.setValues(323, "CUB", "Cuba", "Cuba");
            break;
        case 325:
            result.setValues(325, "DOM", "Dominica", "Dominica");
            break;
        case 327:
            result.setValues(327, "DOR", "Dominican Republic", "Dominican");
            break;
        case 329:
            result.setValues(329, "GUA", "Guadeloupe", "Guadeloupe");
            break;
        case 330:
            result.setValues(330, "GRA", "Grenada", "Grenada");
            break;
        case 331:
            result.setValues(331, "GRN", "Greenland", "Greenland");
            break;
        case 332:
            result.setValues(332, "GUT", "Guatemala", "Guatemala");
            break;
        case 334:
            result.setValues(334, "HON", "Honduras", "Honduras");
            break;
        case 336:
            result.setValues(336, "HAI", "Haiti", "Haiti");
            break;
        case 338:
            result.setValues(338, "USA", "United States", "USA");
            break;
        case 339:
            result.setValues(339, "JAM", "Jamaica", "Jamaica");
            break;
        case 341:
            result.setValues(341, "SKN", "Saint Kitts and Nevis", "St Kitts");
            break;
        case 343:
            result.setValues(343, "SLU", "Saint Lucia", "St Lucia");
            break;
        case 345:
            result.setValues(345, "MEX", "Mexico", "Mexico");
            break;
        case 347:
            result.setValues(347, "MTQ", "Martinique", "Martinique");
            break;
        case 348:
            result.setValues(348, "MOT", "Montserrat", "Montserrat");
            break;
        case 350:
            result.setValues(350, "NIC", "Nicaragua", "Nicaragua");
            break;
        case 351:
            result.setValues(351, "PAN", "Panama", "Panama");
            break;
        case 352:
            result.setValues(352, "PAN", "Panama", "Panama");
            break;
        case 353:
            result.setValues(353, "PAN", "Panama", "Panama");
            break;
        case 354:
            result.setValues(354, "PAN", "Panama", "Panama");
            break;
        case 355:
            result.setValues(355, "PAN", "Panama", "Panama");
            break;
        case 356:
            result.setValues(356, "PAN", "Panama", "Panama");
            break;
        case 357:
            result.setValues(357, "PAN", "Panama", "Panama");
            break;
        case 358:
            result.setValues(358, "PUE", "Puerto Rico", "PuertoRico");
            break;
        case 359:
            result.setValues(359, "ELS", "El Salvador", "ElSalvador");
            break;
        case 361:
            result.setValues(361, "SPI", "St. Pierre and Miquelon", "St Pierre");
            break;
        case 362:
            result.setValues(362, "TAT", "Trinidad and Tobago", "Trinidad");
            break;
        case 364:
            result.setValues(364, "TUK", "Turks and Caicos Islands", "Caicos Is");
            break;
        case 366:
            result.setValues(366, "USA", "United States", "USA");
            break;
        case 367:
            result.setValues(367, "USA", "United States", "USA");
            break;
        case 368:
            result.setValues(368, "USA", "United States", "USA");
            break;
        case 369:
            result.setValues(369, "USA", "United States", "USA");
            break;
        case 370:
            result.setValues(370, "PAN", "Panama", "Panama");
            break;
        case 371:
            result.setValues(371, "PAN", "Panama", "Panama");
            break;
        case 372:
            result.setValues(372, "PAN", "Panama", "Panama");
            break;
        case 373:
            result.setValues(373, "PAN", "Panama", "Panama");
            break;
        case 374:
            result.setValues(374, "PAN", "Panama", "Panama");
            break;
        case 375:
            result.setValues(375, "SVG", "Saint Vincent and the Grenadines", "St Vincent");
            break;
        case 376:
            result.setValues(376, "SVG", "Saint Vincent and the Grenadines", "St Vincent");
            break;
        case 377:
            result.setValues(377, "SVG", "Saint Vincent and the Grenadines", "St Vincent");
            break;
        case 378:
            result.setValues(378, "BVI", "British Virgin Islands", "Virgin GB");
            break;
        case 379:
            result.setValues(379, "USV", "United States Virgin Islands", "Virgin US");
            break;
        case 401:
            result.setValues(401, "AFG", "Afghanistan", "Afghan");
            break;
        case 403:
            result.setValues(403, "SAU", "Saudi Arabia", "Saudi");
            break;
        case 405:
            result.setValues(405, "BAN", "Bangladesh", "Bangladesh");
            break;
        case 408:
            result.setValues(408, "BAH", "Bahrain", "Bahrain");
            break;
        case 410:
            result.setValues(410, "BHU", "Bhutan", "Bhutan");
            break;
        case 412:
            result.setValues(412, "CHN", "China", "China");
            break;
        case 413:
            result.setValues(413, "CHN", "China", "China");
            break;
        case 414:
            result.setValues(414, "CHN", "China", "CHINA");
            break;
        case 416:
            result.setValues(416, "TAI", "Chinese Taipei", "Taipei");
            break;
        case 417:
            result.setValues(417, "SRI", "Sri Lanka", "Sri Lanka");
            break;
        case 419:
            result.setValues(419, "IND", "India", "India");
            break;
        case 422:
            result.setValues(422, "IRN", "Iran", "Iran");
            break;
        case 423:
            result.setValues(423, "AZR", "Azerbaijan", "Azerbaijan");
            break;
        case 425:
            result.setValues(425, "IRQ", "Iraq", "Iraq");
            break;
        case 428:
            result.setValues(428, "ISR", "Israel", "Israel");
            break;
        case 431:
            result.setValues(431, "JPN", "Japan", "Japan");
            break;
        case 432:
            result.setValues(432, "JPN", "Japan", "Japan");
            break;
        case 434:
            result.setValues(434, "TKM", "Turkmenistan", "Turkmenist");
            break;
        case 436:
            result.setValues(436, "KAZ", "Kazakhstan", "Kazakhstan");
            break;
        case 437:
            result.setValues(437, "UZB", "Uzbekistan", "Uzbekistan");
            break;
        case 438:
            result.setValues(438, "JOR", "Jordan", "Jordan");
            break;
        case 440:
            result.setValues(440, "KOR", "Korea (Republic of)", "Korea Sou");
            break;
        case 441:
            result.setValues(441, "KOR", "Korea (Republic of)", "Korea Sou");
            break;
        case 443:
            result.setValues(443, "PAA", "Palestine", "Palestine");
            break;
        case 445:
            result.setValues(445, "KDR", "Democratic People's Republic of Korea", "Korea Nor");
            break;
        case 447:
            result.setValues(447, "KUW", "Kuwait", "Kuwait");
            break;
        case 450:
            result.setValues(450, "LEB", "Lebanon", "Lebanon");
            break;
        case 451:
            result.setValues(451, "KYR", "Kyrgyz Republic", "Kyrgyzia");
            break;
        case 453:
            result.setValues(453, "MAC", "Macao, China", "Macao");
            break;
        case 455:
            result.setValues(455, "MAV", "Maldives", "Maldives");
            break;
        case 457:
            result.setValues(457, "MNG", "Mongolia", "Mongolia");
            break;
        case 459:
            result.setValues(459, "NEP", "Nepal", "Nepal");
            break;
        case 461:
            result.setValues(461, "OMN", "Oman", "Oman");
            break;
        case 463:
            result.setValues(463, "PAK", "Pakistan", "Pakistan");
            break;
        case 466:
            result.setValues(466, "QAT", "Qatar", "Qatar");
            break;
        case 468:
            result.setValues(468, "SYR", "Syria", "Syria");
            break;
        case 470:
            result.setValues(470, "UAE", "United Arab Emirates", "UAE");
            break;
        case 471:
            result.setValues(471, "UAE", "United Arab Emirates", "UAE");
            break;
        case 472:
            result.setValues(472, "TJK", "Tajikistan", "TAJIKISTAN");
            break;
        case 473:
            result.setValues(473, "YEM", "Yemen", "Yemen");
            break;
        case 475:
            result.setValues(475, "YEM", "Yemen", "Yemen");
            break;
        case 477:
            result.setValues(477, "HKG", "Hong Kong, China", "Hong Kong");
            break;
        case 478:
            result.setValues(478, "BOS", "Bosnia and Herzegovina", "Bosniaherz");
            break;
        case 503:
            result.setValues(503, "AUS", "Australia", "Australia");
            break;
        case 506:
            result.setValues(506, "BUR", "Myanmar", "Burma");
            break;
        case 508:
            result.setValues(508, "BRU", "Brunei Darussalam", "Brunei");
            break;
        case 510:
            result.setValues(510, "MIC", "Micronesia", "Micronesia");
            break;
        case 511:
            result.setValues(511, "PAL", "Palau", "Palau");
            break;
        case 512:
            result.setValues(512, "NZL", "New Zealand", "NewZealand");
            break;
        case 514:
            result.setValues(514, "CMB", "Cambodia", "Cambodia");
            break;
        case 515:
            result.setValues(515, "CMB", "Cambodia", "Cambodia");
            break;
        case 516:
            result.setValues(516, "CHR", "Christmas Island", "Christmas");
            break;
        case 518:
            result.setValues(518, "COO", "Cook Islands", "Cook Isles");
            break;
        case 520:
            result.setValues(520, "FIJ", "Fiji", "Fiji");
            break;
        case 523:
            result.setValues(523, "COC", "Cocos (Keeling) Islands", "Cocos Isle");
            break;
        case 525:
            result.setValues(525, "INO", "Indonesia", "Indonesia");
            break;
        case 529:
            result.setValues(529, "KIR", "Kiribati", "Kiribati");
            break;
        case 531:
            result.setValues(531, "LAO", "Laos", "Lao");
            break;
        case 533:
            result.setValues(533, "MLY", "Malaysia", "Malaysia");
            break;
        case 536:
            result.setValues(536, "MAI", "Northern Mariana Islands", "Mariana Is");
            break;
        case 538:
            result.setValues(538, "MAR", "Marshall Islands", "Marshall I");
            break;
        case 540:
            result.setValues(540, "NCA", "New Caledonia", "Caledonia");
            break;
        case 542:
            result.setValues(542, "NIU", "Niue", "Niue Isle");
            break;
        case 544:
            result.setValues(544, "NAU", "Nauru", "Nauru");
            break;
        case 546:
            result.setValues(546, "PLY", "French Polynesia", "Polynesia");
            break;
        case 548:
            result.setValues(548, "PHI", "Philippines", "Philippine");
            break;
        case 550:
            result.setValues(550, "TIM", "Timor-Leste (Democratic Republic of)", "TimorLeste");
            break;
        case 553:
            result.setValues(553, "PAP", "Papua New Guinea", "Papua NG");
            break;
        case 555:
            result.setValues(555, "PIT", "Pitcairn", "Pitcairn I");
            break;
        case 557:
            result.setValues(557, "SOL", "Solomon Islands", "Solomon Is");
            break;
        case 559:
            result.setValues(559, "ASA", "American Samoa", "Samoa USA");
            break;
        case 561:
            result.setValues(561, "WSA", "Samoa", "West Samoa");
            break;
        case 563:
            result.setValues(563, "SIN", "Singapore", "Singapore");
            break;
        case 564:
            result.setValues(564, "SIN", "Singapore", "Singapore");
            break;
        case 565:
            result.setValues(565, "SIN", "Singapore", "Singapore");
            break;
        case 566:
            result.setValues(566, "SIN", "Singapore", "SINGAPORE");
            break;
        case 567:
            result.setValues(567, "THA", "Thailand", "Thailand");
            break;
        case 570:
            result.setValues(570, "TON", "Tonga", "Tonga");
            break;
        case 572:
            result.setValues(572, "TUV", "Tuvalu", "Tuvalu Is");
            break;
        case 574:
            result.setValues(574, "VIE", "Vietnam", "Vietnam");
            break;
        case 576:
            result.setValues(576, "VAN", "Vanuatu", "Vanuatu");
            break;
        case 577:
            result.setValues(577, "VAN", "Vanuatu", "Vanuatu");
            break;
        case 578:
            result.setValues(578, "WAL", "Wallis and Futuna Islands", "Wallis Is");
            break;
        case 601:
            result.setValues(601, "SAF", "South Africa", "So Africa");
            break;
        case 603:
            result.setValues(603, "ANG", "Angola", "Angola");
            break;
        case 605:
            result.setValues(605, "ALG", "Algeria", "Algeria");
            break;
        case 607:
            result.setValues(607, "SPL", "Saint Paul and Amsterdam Islands", "St Paul");
            break;
        case 608:
            result.setValues(608, "ASC", "Ascension Island", "Ascension");
            break;
        case 609:
            result.setValues(609, "BUI", "Burundi", "Burundi");
            break;
        case 610:
            result.setValues(610, "BEN", "Benin", "Benin");
            break;
        case 611:
            result.setValues(611, "BOT", "Botswana", "Botswana");
            break;
        case 612:
            result.setValues(612, "CAR", "Central African Republic", "CenAfr Rep");
            break;
        case 613:
            result.setValues(613, "CAM", "Cameroon", "Cameroon");
            break;
        case 615:
            result.setValues(615, "CON", "Congo", "Congo");
            break;
        case 616:
            result.setValues(616, "COM", "Comoros", "Comoros");
            break;
        case 617:
            result.setValues(617, "CAP", "Cape Verde", "Cape Verde");
            break;
        case 618:
            result.setValues(618, "CRP", "Crozet Archipelago", "Crozet");
            break;
        case 619:
            result.setValues(619, "IVO", "Ivory Coast", "IvoryCoast");
            break;
        case 620:
            result.setValues(620, "COM", "Comoros", "Comoros");
            break;
        case 621:
            result.setValues(621, "DJI", "Djibouti", "Djibouti");
            break;
        case 622:
            result.setValues(622, "EGY", "Egypt", "Egypt");
            break;
        case 624:
            result.setValues(624, "ETH", "Ethiopia", "Ethiopia");
            break;
        case 625:
            result.setValues(625, "ERT", "Eritrea", "Eritrea");
            break;
        case 626:
            result.setValues(626, "GAB", "Gabon", "Gabon Rep");
            break;
        case 627:
            result.setValues(627, "GHA", "Ghana", "Ghana");
            break;
        case 629:
            result.setValues(629, "GAM", "Gambia", "Gambia");
            break;
        case 630:
            result.setValues(630, "GUB", "Guinea-Bissau", "Guinea Bis");
            break;
        case 631:
            result.setValues(631, "EQG", "Equatorial Guinea", "Eq Guinea");
            break;
        case 632:
            result.setValues(632, "GUN", "Guinea", "Guinea Rep");
            break;
        case 633:
            result.setValues(633, "BUF", "Burkina Faso", "Burkina FS");
            break;
        case 634:
            result.setValues(634, "KEN", "Kenya", "Kenya");
            break;
        case 635:
            result.setValues(635, "KER", "Kerguelen Islands", "Kerguelen");
            break;
        case 636:
            result.setValues(636, "LIB", "Liberia", "Liberia");
            break;
        case 637:
            result.setValues(637, "LIB", "Liberia", "Liberia");
            break;
        case 638:
            result.setValues(638, "SSD", "South Sudan", "Southsudan");
            break;
        case 642:
            result.setValues(642, "LBY", "Libya", "Libya");
            break;
        case 644:
            result.setValues(644, "LES", "Lesotho", "Lesotho");
            break;
        case 645:
            result.setValues(645, "MAU", "Mauritius", "Mauritius");
            break;
        case 647:
            result.setValues(647, "MAD", "Madagascar", "Madagascar");
            break;
        case 649:
            result.setValues(649, "MLI", "Mali", "Mali");
            break;
        case 650:
            result.setValues(650, "MOZ", "Mozambique", "Mozambique");
            break;
        case 654:
            result.setValues(654, "MAA", "Mauritania", "Mauritania");
            break;
        case 655:
            result.setValues(655, "MAW", "Malawi", "Malawi");
            break;
        case 656:
            result.setValues(656, "NIG", "Niger", "Niger");
            break;
        case 657:
            result.setValues(657, "NIA", "Nigeria", "Nigeria");
            break;
        case 659:
            result.setValues(659, "NAM", "Namibia", "Namibia");
            break;
        case 660:
            result.setValues(660, "REU", "Reunion (also same country code for Mayotte)", "Reunion");
            break;
        case 661:
            result.setValues(661, "RWA", "Rwanda", "Rwanda");
            break;
        case 662:
            result.setValues(662, "SUD", "Sudan", "Sudan");
            break;
        case 663:
            result.setValues(663, "SEN", "Senegal", "Senegal");
            break;
        case 664:
            result.setValues(664, "SEY", "Seychelles", "Seychelle");
            break;
        case 665:
            result.setValues(665, "SHE", "St. Helena", "St Helena");
            break;
        case 666:
            result.setValues(666, "SOM", "Somalia", "Somali");
            break;
        case 667:
            result.setValues(667, "SIL", "Sierra Leone", "Sierra Leo");
            break;
        case 668:
            result.setValues(668, "SAO", "Sao Tome and Principe", "Sao Tome");
            break;
        case 669:
            result.setValues(669, "SWZ", "Eswatini", "Eswatini");
            break;
        case 670:
            result.setValues(670, "CHA", "Chad", "Chad");
            break;
        case 671:
            result.setValues(671, "TOG", "Togo", "Togo");
            break;
        case 672:
            result.setValues(672, "TUN", "Tunisia", "Tunisia");
            break;
        case 674:
            result.setValues(674, "TAN", "Tanzania", "Tanzania");
            break;
        case 675:
            result.setValues(675, "UGA", "Uganda", "Uganda");
            break;
        case 676:
            result.setValues(676, "ZAI", "Democratic Republic of the Congo", "Zaire");
            break;
        case 677:
            result.setValues(677, "TAN", "Tanzania", "Tanzania");
            break;
        case 678:
            result.setValues(678, "ZAM", "Zambia", "Zambia");
            break;
        case 679:
            result.setValues(679, "ZIM", "Zimbabwe", "Zimbabwe");
            break;
        case 701:
            result.setValues(701, "ARG", "Argentina", "Argentina");
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
            result.setValues(770, "URU", "Uruguay", "Uruguay");
            break;
        case 775:
            result.setValues(775, "VEN", "Venezuela", "Venezuela");
            break;*/
        default:
            result.setValues(code, "UNK", "Unknown country", "Unkown");
    }
    return result;
}

}  // namespace ui::external_app::epirb_rx