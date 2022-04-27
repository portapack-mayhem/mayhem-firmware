/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    SPC564Axx/vectors.s
 * @brief   SPC564Axx vectors table.
 *
 * @addtogroup PPC_CORE
 * @{
 */

#if !defined(__DOXYGEN__)

        /* Software vectors table. The vectors are accessed from the IVOR4
           handler only. In order to declare an interrupt handler just create
           a function withe the same name of a vector, the symbol will
           override the weak symbol declared here.*/
        .section    .vectors, "ax"
        .align		4
        .globl      _vectors
_vectors:
        .long       vector0,    vector1,    vector2,    vector3
        .long       vector4,    vector5,    vector6,    vector7
        .long       vector8,    vector9,    vector10,   vector11
        .long       vector12,   vector13,   vector14,   vector15
        .long       vector16,   vector17,   vector18,   vector19
        .long       vector20,   vector21,   vector22,   vector23
        .long       vector24,   vector25,   vector26,   vector27
        .long       vector28,   vector29,   vector30,   vector31
        .long       vector32,   vector33,   vector34,   vector35
        .long       vector36,   vector37,   vector38,   vector39
        .long       vector40,   vector41,   vector42,   vector43
        .long       vector44,   vector45,   vector46,   vector47
        .long       vector48,   vector49,   vector50,   vector51
        .long       vector52,   vector53,   vector54,   vector55
        .long       vector56,   vector57,   vector58,   vector59
        .long       vector60,   vector61,   vector62,   vector63
        .long       vector64,   vector65,   vector66,   vector67
        .long       vector68,   vector69,   vector70,   vector71
        .long       vector72,   vector73,   vector74,   vector75
        .long       vector76,   vector77,   vector78,   vector79
        .long       vector80,   vector81,   vector82,   vector83
        .long       vector84,   vector85,   vector86,   vector87
        .long       vector88,   vector89,   vector90,   vector91
        .long       vector92,   vector93,   vector94,   vector95
        .long       vector96,   vector97,   vector98,   vector99
        .long       vector100,  vector101,  vector102,  vector103
        .long       vector104,  vector105,  vector106,  vector107
        .long       vector108,  vector109,  vector110,  vector111
        .long       vector112,  vector113,  vector114,  vector115
        .long       vector116,  vector117,  vector118,  vector119
        .long       vector120,  vector121,  vector122,  vector123
        .long       vector124,  vector125,  vector126,  vector127
        .long       vector128,  vector129,  vector130,  vector131
        .long       vector132,  vector133,  vector134,  vector135
        .long       vector136,  vector137,  vector138,  vector139
        .long       vector140,  vector141,  vector142,  vector143
        .long       vector144,  vector145,  vector146,  vector147
        .long       vector148,  vector149,  vector150,  vector151
        .long       vector152,  vector153,  vector154,  vector155
        .long       vector156,  vector157,  vector158,  vector159
        .long       vector160,  vector161,  vector162,  vector163
        .long       vector164,  vector165,  vector166,  vector167
        .long       vector168,  vector169,  vector170,  vector171
        .long       vector172,  vector173,  vector174,  vector175
        .long       vector176,  vector177,  vector178,  vector179
        .long       vector180,  vector181,  vector182,  vector183
        .long       vector184,  vector185,  vector186,  vector187
        .long       vector188,  vector189,  vector190,  vector191
        .long       vector192,  vector193,  vector194,  vector195
        .long       vector196,  vector197,  vector198,  vector199
        .long       vector200,  vector201,  vector202,  vector203
        .long       vector204,  vector205,  vector206,  vector207
        .long       vector208,  vector209,  vector210,  vector211
        .long       vector212,  vector213,  vector214,  vector215
        .long       vector216,  vector217,  vector218,  vector219
        .long       vector220,  vector221,  vector222,  vector223
        .long       vector224,  vector225,  vector226,  vector227
        .long       vector228,  vector229,  vector230,  vector231
        .long       vector232,  vector233,  vector234,  vector235
        .long       vector236,  vector237,  vector238,  vector239
        .long       vector240,  vector241,  vector242,  vector243
        .long       vector244,  vector245,  vector246,  vector247
        .long       vector248,  vector249,  vector250,  vector251
        .long       vector252,  vector253,  vector254,  vector255
        .long       vector256,  vector257,  vector258,  vector259
        .long       vector260,  vector261,  vector262,  vector263
        .long       vector264,  vector265,  vector266,  vector267
        .long       vector268,  vector269,  vector270,  vector271
        .long       vector272,  vector273,  vector274,  vector275
        .long       vector276,  vector277,  vector278,  vector279
        .long       vector280,  vector281,  vector282,  vector283
        .long       vector284,  vector285,  vector286,  vector287
        .long       vector288,  vector289,  vector290,  vector291
        .long       vector292,  vector293,  vector294,  vector295
        .long       vector296,  vector297,  vector298,  vector299
        .long       vector300,  vector301,  vector302,  vector303
        .long       vector304,  vector305,  vector306,  vector307
        .long       vector308,  vector309,  vector310,  vector311
        .long       vector312,  vector313,  vector314,  vector315
        .long       vector316,  vector317,  vector318,  vector319
        .long       vector320,  vector321,  vector322,  vector323
        .long       vector324,  vector325,  vector326,  vector327
        .long       vector328,  vector329,  vector330,  vector331
        .long       vector332,  vector333,  vector334,  vector335
        .long       vector336,  vector337,  vector338,  vector339
        .long       vector340,  vector341,  vector342,  vector343
        .long       vector344,  vector345,  vector346,  vector347
        .long       vector348,  vector349,  vector350,  vector351
        .long       vector352,  vector353,  vector354,  vector355
        .long       vector356,  vector357,  vector358,  vector359
        .long       vector360,  vector361,  vector362,  vector363
        .long       vector364,  vector365,  vector366,  vector367
        .long       vector368,  vector369,  vector370,  vector371
        .long       vector372,  vector373,  vector374,  vector375
        .long       vector376,  vector377,  vector378,  vector379
        .long       vector380,  vector381,  vector382,  vector383
        .long       vector384,  vector385,  vector386,  vector387
        .long       vector388,  vector389,  vector390,  vector391
        .long       vector392,  vector393,  vector394,  vector395
        .long       vector396,  vector397,  vector398,  vector399
        .long       vector400,  vector401,  vector402,  vector403
        .long       vector404,  vector405,  vector406,  vector407
        .long       vector408,  vector409,  vector410,  vector411
        .long       vector412,  vector413,  vector414,  vector415
        .long       vector416,  vector417,  vector418,  vector419
        .long       vector420,  vector421,  vector422,  vector423
        .long       vector424,  vector425,  vector426,  vector427
        .long       vector428,  vector429,  vector430,  vector431
        .long       vector432,  vector433,  vector434,  vector435
        .long       vector436,  vector437,  vector438,  vector439
        .long       vector440,  vector441,  vector442,  vector443
        .long       vector444,  vector445,  vector446,  vector447
        .long       vector448,  vector449,  vector450,  vector451
        .long       vector452,  vector453,  vector454,  vector455
        .long       vector456,  vector457,  vector458,  vector459
        .long       vector460,  vector461,  vector462,  vector463
        .long       vector464,  vector465,  vector466,  vector467
        .long       vector468,  vector469,  vector470,  vector471
        .long       vector472,  vector473,  vector474,  vector475
        .long       vector476,  vector477,  vector478,  vector479
        .long       vector480,  vector481,  vector482,  vector483
        .long       vector484,  vector485

        .text
        .align      2
        
        .weak       vector0,    vector1,    vector2,    vector3
        .weak       vector4,    vector5,    vector6,    vector7
        .weak       vector8,    vector9,    vector10,   vector11
        .weak       vector12,   vector13,   vector14,   vector15
        .weak       vector16,   vector17,   vector18,   vector19
        .weak       vector20,   vector21,   vector22,   vector23
        .weak       vector24,   vector25,   vector26,   vector27
        .weak       vector28,   vector29,   vector30,   vector31
        .weak       vector32,   vector33,   vector34,   vector35
        .weak       vector36,   vector37,   vector38,   vector39
        .weak       vector40,   vector41,   vector42,   vector43
        .weak       vector44,   vector45,   vector46,   vector47
        .weak       vector48,   vector49,   vector50,   vector51
        .weak       vector52,   vector53,   vector54,   vector55
        .weak       vector56,   vector57,   vector58,   vector59
        .weak       vector60,   vector61,   vector62,   vector63
        .weak       vector64,   vector65,   vector66,   vector67
        .weak       vector68,   vector69,   vector70,   vector71
        .weak       vector72,   vector73,   vector74,   vector75
        .weak       vector76,   vector77,   vector78,   vector79
        .weak       vector80,   vector81,   vector82,   vector83
        .weak       vector84,   vector85,   vector86,   vector87
        .weak       vector88,   vector89,   vector90,   vector91
        .weak       vector92,   vector93,   vector94,   vector95
        .weak       vector96,   vector97,   vector98,   vector99
        .weak       vector100,  vector101,  vector102,  vector103
        .weak       vector104,  vector105,  vector106,  vector107
        .weak       vector108,  vector109,  vector110,  vector111
        .weak       vector112,  vector113,  vector114,  vector115
        .weak       vector116,  vector117,  vector118,  vector119
        .weak       vector120,  vector121,  vector122,  vector123
        .weak       vector124,  vector125,  vector126,  vector127
        .weak       vector128,  vector129,  vector130,  vector131
        .weak       vector132,  vector133,  vector134,  vector135
        .weak       vector136,  vector137,  vector138,  vector139
        .weak       vector140,  vector141,  vector142,  vector143
        .weak       vector144,  vector145,  vector146,  vector147
        .weak       vector148,  vector149,  vector150,  vector151
        .weak       vector152,  vector153,  vector154,  vector155
        .weak       vector156,  vector157,  vector158,  vector159
        .weak       vector160,  vector161,  vector162,  vector163
        .weak       vector164,  vector165,  vector166,  vector167
        .weak       vector168,  vector169,  vector170,  vector171
        .weak       vector172,  vector173,  vector174,  vector175
        .weak       vector176,  vector177,  vector178,  vector179
        .weak       vector180,  vector181,  vector182,  vector183
        .weak       vector184,  vector185,  vector186,  vector187
        .weak       vector188,  vector189,  vector190,  vector191
        .weak       vector192,  vector193,  vector194,  vector195
        .weak       vector196,  vector197,  vector198,  vector199
        .weak       vector200,  vector201,  vector202,  vector203
        .weak       vector204,  vector205,  vector206,  vector207
        .weak       vector208,  vector209,  vector210,  vector211
        .weak       vector212,  vector213,  vector214,  vector215
        .weak       vector216,  vector217,  vector218,  vector219
        .weak       vector220,  vector221,  vector222,  vector223
        .weak       vector224,  vector225,  vector226,  vector227
        .weak       vector228,  vector229,  vector230,  vector231
        .weak       vector232,  vector233,  vector234,  vector235
        .weak       vector236,  vector237,  vector238,  vector239
        .weak       vector240,  vector241,  vector242,  vector243
        .weak       vector244,  vector245,  vector246,  vector247
        .weak       vector248,  vector249,  vector250,  vector251
        .weak       vector252,  vector253,  vector254,  vector255
        .weak       vector256,  vector257,  vector258,  vector259
        .weak       vector260,  vector261,  vector262,  vector263
        .weak       vector264,  vector265,  vector266,  vector267
        .weak       vector268,  vector269,  vector270,  vector271
        .weak       vector272,  vector273,  vector274,  vector275
        .weak       vector276,  vector277,  vector278,  vector279
        .weak       vector280,  vector281,  vector282,  vector283
        .weak       vector284,  vector285,  vector286,  vector287
        .weak       vector288,  vector289,  vector290,  vector291
        .weak       vector292,  vector293,  vector294,  vector295
        .weak       vector296,  vector297,  vector298,  vector299
        .weak       vector300,  vector301,  vector302,  vector303
        .weak       vector304,  vector305,  vector306,  vector307
        .weak       vector308,  vector309,  vector310,  vector311
        .weak       vector312,  vector313,  vector314,  vector315
        .weak       vector316,  vector317,  vector318,  vector319
        .weak       vector320,  vector321,  vector322,  vector323
        .weak       vector324,  vector325,  vector326,  vector327
        .weak       vector328,  vector329,  vector330,  vector331
        .weak       vector332,  vector333,  vector334,  vector335
        .weak       vector336,  vector337,  vector338,  vector339
        .weak       vector340,  vector341,  vector342,  vector343
        .weak       vector344,  vector345,  vector346,  vector347
        .weak       vector348,  vector349,  vector350,  vector351
        .weak       vector352,  vector353,  vector354,  vector355
        .weak       vector356,  vector357,  vector358,  vector359
        .weak       vector360,  vector361,  vector362,  vector363
        .weak       vector364,  vector365,  vector366,  vector367
        .weak       vector368,  vector369,  vector370,  vector371
        .weak       vector372,  vector373,  vector374,  vector375
        .weak       vector376,  vector377,  vector378,  vector379
        .weak       vector380,  vector381,  vector382,  vector383
        .weak       vector384,  vector385,  vector386,  vector387
        .weak       vector388,  vector389,  vector390,  vector391
        .weak       vector392,  vector393,  vector394,  vector395
        .weak       vector396,  vector397,  vector398,  vector399
        .weak       vector400,  vector401,  vector402,  vector403
        .weak       vector404,  vector405,  vector406,  vector407
        .weak       vector408,  vector409,  vector410,  vector411
        .weak       vector412,  vector413,  vector414,  vector415
        .weak       vector416,  vector417,  vector418,  vector419
        .weak       vector420,  vector421,  vector422,  vector423
        .weak       vector424,  vector425,  vector426,  vector427
        .weak       vector428,  vector429,  vector430,  vector431
        .weak       vector432,  vector433,  vector434,  vector435
        .weak       vector436,  vector437,  vector438,  vector439
        .weak       vector440,  vector441,  vector442,  vector443
        .weak       vector444,  vector445,  vector446,  vector447
        .weak       vector448,  vector449,  vector450,  vector451
        .weak       vector452,  vector453,  vector454,  vector455
        .weak       vector456,  vector457,  vector458,  vector459
        .weak       vector460,  vector461,  vector462,  vector463
        .weak       vector464,  vector465,  vector466,  vector467
        .weak       vector468,  vector469,  vector470,  vector471
        .weak       vector472,  vector473,  vector474,  vector475
        .weak       vector476,  vector477,  vector478,  vector479
        .weak       vector480,  vector481,  vector482,  vector483
        .weak       vector484,  vector485

vector0:
vector1:
vector2:
vector3:
vector4:
vector5:
vector6:
vector7:
vector8:
vector9:
vector10:
vector11:
vector12:
vector13:
vector14:
vector15:
vector16:
vector17:
vector18:
vector19:
vector20:
vector21:
vector22:
vector23:
vector24:
vector25:
vector26:
vector27:
vector28:
vector29:
vector30:
vector31:
vector32:
vector33:
vector34:
vector35:
vector36:
vector37:
vector38:
vector39:
vector40:
vector41:
vector42:
vector43:
vector44:
vector45:
vector46:
vector47:
vector48:
vector49:
vector50:
vector51:
vector52:
vector53:
vector54:
vector55:
vector56:
vector57:
vector58:
vector59:
vector60:
vector61:
vector62:
vector63:
vector64:
vector65:
vector66:
vector67:
vector68:
vector69:
vector70:
vector71:
vector72:
vector73:
vector74:
vector75:
vector76:
vector77:
vector78:
vector79:
vector80:
vector81:
vector82:
vector83:
vector84:
vector85:
vector86:
vector87:
vector88:
vector89:
vector90:
vector91:
vector92:
vector93:
vector94:
vector95:
vector96:
vector97:
vector98:
vector99:
vector100:
vector101:
vector102:
vector103:
vector104:
vector105:
vector106:
vector107:
vector108:
vector109:
vector110:
vector111:
vector112:
vector113:
vector114:
vector115:
vector116:
vector117:
vector118:
vector119:
vector120:
vector121:
vector122:
vector123:
vector124:
vector125:
vector126:
vector127:
vector128:
vector129:
vector130:
vector131:
vector132:
vector133:
vector134:
vector135:
vector136:
vector137:
vector138:
vector139:
vector140:
vector141:
vector142:
vector143:
vector144:
vector145:
vector146:
vector147:
vector148:
vector149:
vector150:
vector151:
vector152:
vector153:
vector154:
vector155:
vector156:
vector157:
vector158:
vector159:
vector160:
vector161:
vector162:
vector163:
vector164:
vector165:
vector166:
vector167:
vector168:
vector169:
vector170:
vector171:
vector172:
vector173:
vector174:
vector175:
vector176:
vector177:
vector178:
vector179:
vector180:
vector181:
vector182:
vector183:
vector184:
vector185:
vector186:
vector187:
vector188:
vector189:
vector190:
vector191:
vector192:
vector193:
vector194:
vector195:
vector196:
vector197:
vector198:
vector199:
vector200:
vector201:
vector202:
vector203:
vector204:
vector205:
vector206:
vector207:
vector208:
vector209:
vector210:
vector211:
vector212:
vector213:
vector214:
vector215:
vector216:
vector217:
vector218:
vector219:
vector220:
vector221:
vector222:
vector223:
vector224:
vector225:
vector226:
vector227:
vector228:
vector229:
vector230:
vector231:
vector232:
vector233:
vector234:
vector235:
vector236:
vector237:
vector238:
vector239:
vector240:
vector241:
vector242:
vector243:
vector244:
vector245:
vector246:
vector247:
vector248:
vector249:
vector250:
vector251:
vector252:
vector253:
vector254:
vector255:
vector256:
vector257:
vector258:
vector259:
vector260:
vector261:
vector262:
vector263:
vector264:
vector265:
vector266:
vector267:
vector268:
vector269:
vector270:
vector271:
vector272:
vector273:
vector274:
vector275:
vector276:
vector277:
vector278:
vector279:
vector280:
vector281:
vector282:
vector283:
vector284:
vector285:
vector286:
vector287:
vector288:
vector289:
vector290:
vector291:
vector292:
vector293:
vector294:
vector295:
vector296:
vector297:
vector298:
vector299:
vector300:
vector301:
vector302:
vector303:
vector304:
vector305:
vector306:
vector307:
vector308:
vector309:
vector310:
vector311:
vector312:
vector313:
vector314:
vector315:
vector316:
vector317:
vector318:
vector319:
vector320:
vector321:
vector322:
vector323:
vector324:
vector325:
vector326:
vector327:
vector328:
vector329:
vector330:
vector331:
vector332:
vector333:
vector334:
vector335:
vector336:
vector337:
vector338:
vector339:
vector340:
vector341:
vector342:
vector343:
vector344:
vector345:
vector346:
vector347:
vector348:
vector349:
vector350:
vector351:
vector352:
vector353:
vector354:
vector355:
vector356:
vector357:
vector358:
vector359:
vector360:
vector361:
vector362:
vector363:
vector364:
vector365:
vector366:
vector367:
vector368:
vector369:
vector370:
vector371:
vector372:
vector373:
vector374:
vector375:
vector376:
vector377:
vector378:
vector379:
vector380:
vector381:
vector382:
vector383:
vector384:
vector385:
vector386:
vector387:
vector388:
vector389:
vector390:
vector391:
vector392:
vector393:
vector394:
vector395:
vector396:
vector397:
vector398:
vector399:
vector400:
vector401:
vector402:
vector403:
vector404:
vector405:
vector406:
vector407:
vector408:
vector409:
vector410:
vector411:
vector412:
vector413:
vector414:
vector415:
vector416:
vector417:
vector418:
vector419:
vector420:
vector421:
vector422:
vector423:
vector424:
vector425:
vector426:
vector427:
vector428:
vector429:
vector430:
vector431:
vector432:
vector433:
vector434:
vector435:
vector436:
vector437:
vector438:
vector439:
vector440:
vector441:
vector442:
vector443:
vector444:
vector445:
vector446:
vector447:
vector448:
vector449:
vector450:
vector451:
vector452:
vector453:
vector454:
vector455:
vector456:
vector457:
vector458:
vector459:
vector460:
vector461:
vector462:
vector463:
vector464:
vector465:
vector466:
vector467:
vector468:
vector469:
vector470:
vector471:
vector472:
vector473:
vector474:
vector475:
vector476:
vector477:
vector478:
vector479:
vector480:
vector481:
vector482:
vector483:
vector484:
vector485:

        .weak       _unhandled_irq
        .type       _unhandled_irq, @function
_unhandled_irq:
         b          _unhandled_irq

#endif /* !defined(__DOXYGEN__) */

/** @} */
