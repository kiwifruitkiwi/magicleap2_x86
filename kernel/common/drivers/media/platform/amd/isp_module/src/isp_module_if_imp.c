/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "isp_common.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "buffer_mgr.h"
#include "isp_fw_if.h"
#include "i2c.h"
#include "log.h"
#include "isp_para_capability.h"
#include "isp_soc_adpt.h"

const uint32 sqrtmap[] = {
	0,			/*0 */
	10000,			/*1 */
	14142,			/*2 */
	17320,			/*3 */
	20000,			/*4 */
	22360,			/*5 */
	24494,			/*6 */
	26457,			/*7 */
	28284,			/*8 */
	30000,			/*9 */
	31622,			/*10 */
	33166,			/*11 */
	34641,			/*12 */
	36055,			/*13 */
	37416,			/*14 */
	38729,			/*15 */
	40000,			/*16 */
	41231,			/*17 */
	42426,			/*18 */
	43588,			/*19 */
	44721,			/*20 */
	45825,			/*21 */
	46904,			/*22 */
	47958,			/*23 */
	48989,			/*24 */
	50000,			/*25 */
	50990,			/*26 */
	51961,			/*27 */
	52915,			/*28 */
	53851,			/*29 */
	54772,			/*30 */
	55677,			/*31 */
	56568,			/*32 */
	57445,			/*33 */
	58309,			/*34 */
	59160,			/*35 */
	60000,			/*36 */
	60827,			/*37 */
	61644,			/*38 */
	62449,			/*39 */
	63245,			/*40 */
	64031,			/*41 */
	64807,			/*42 */
	65574,			/*43 */
	66332,			/*44 */
	67082,			/*45 */
	67823,			/*46 */
	68556,			/*47 */
	69282,			/*48 */
	70000,			/*49 */
	70710,			/*50 */
	71414,			/*51 */
	72111,			/*52 */
	72801,			/*53 */
	73484,			/*54 */
	74161,			/*55 */
	74833,			/*56 */
	75498,			/*57 */
	76157,			/*58 */
	76811,			/*59 */
	77459,			/*60 */
	78102,			/*61 */
	78740,			/*62 */
	79372,			/*63 */
	80000,			/*64 */
	80622,			/*65 */
	81240,			/*66 */
	81853,			/*67 */
	82462,			/*68 */
	83066,			/*69 */
	83666,			/*70 */
	84261,			/*71 */
	84852,			/*72 */
	85440,			/*73 */
	86023,			/*74 */
	86602,			/*75 */
	87177,			/*76 */
	87749,			/*77 */
	88317,			/*78 */
	88881,			/*79 */
	89442,			/*80 */
	90000,			/*81 */
	90553,			/*82 */
	91104,			/*83 */
	91651,			/*84 */
	92195,			/*85 */
	92736,			/*86 */
	93273,			/*87 */
	93808,			/*88 */
	94339,			/*89 */
	94868,			/*90 */
	95393,			/*91 */
	95916,			/*92 */
	96436,			/*93 */
	96953,			/*94 */
	97467,			/*95 */
	97979,			/*96 */
	98488,			/*97 */
	98994,			/*98 */
	99498,			/*99 */
	100000,			/*100 */
	100498,			/*101 */
	100995,			/*102 */
	101488,			/*103 */
	101980,			/*104 */
	102469,			/*105 */
	102956,			/*106 */
	103440,			/*107 */
	103923,			/*108 */
	104403,			/*109 */
	104880,			/*110 */
	105356,			/*111 */
	105830,			/*112 */
	106301,			/*113 */
	106770,			/*114 */
	107238,			/*115 */
	107703,			/*116 */
	108166,			/*117 */
	108627,			/*118 */
	109087,			/*119 */
	109544,			/*120 */
	110000,			/*121 */
	110453,			/*122 */
	110905,			/*123 */
	111355,			/*124 */
	111803,			/*125 */
	112249,			/*126 */
	112694,			/*127 */
	113137,			/*128 */
	113578,			/*129 */
	114017,			/*130 */
	114455,			/*131 */
	114891,			/*132 */
	115325,			/*133 */
	115758,			/*134 */
	116189,			/*135 */
	116619,			/*136 */
	117046,			/*137 */
	117473,			/*138 */
	117898,			/*139 */
	118321,			/*140 */
	118743,			/*141 */
	119163,			/*142 */
	119582,			/*143 */
	120000,			/*144 */
	120415,			/*145 */
	120830,			/*146 */
	121243,			/*147 */
	121655,			/*148 */
	122065,			/*149 */
	122474,			/*150 */
	122882,			/*151 */
	123288,			/*152 */
	123693,			/*153 */
	124096,			/*154 */
	124498,			/*155 */
	124899,			/*156 */
	125299,			/*157 */
	125698,			/*158 */
	126095,			/*159 */
	126491,			/*160 */
	126885,			/*161 */
	127279,			/*162 */
	127671,			/*163 */
	128062,			/*164 */
	128452,			/*165 */
	128840,			/*166 */
	129228,			/*167 */
	129614,			/*168 */
	130000,			/*169 */
	130384,			/*170 */
	130766,			/*171 */
	131148,			/*172 */
	131529,			/*173 */
	131909,			/*174 */
	132287,			/*175 */
	132664,			/*176 */
	133041,			/*177 */
	133416,			/*178 */
	133790,			/*179 */
	134164,			/*180 */
	134536,			/*181 */
	134907,			/*182 */
	135277,			/*183 */
	135646,			/*184 */
	136014,			/*185 */
	136381,			/*186 */
	136747,			/*187 */
	137113,			/*188 */
	137477,			/*189 */
	137840,			/*190 */
	138202,			/*191 */
	138564,			/*192 */
	138924,			/*193 */
	139283,			/*194 */
	139642,			/*195 */
	140000,			/*196 */
	140356,			/*197 */
	140712,			/*198 */
	141067,			/*199 */
	141421,			/*200 */
	141774,			/*201 */
	142126,			/*202 */
	142478,			/*203 */
	142828,			/*204 */
	143178,			/*205 */
	143527,			/*206 */
	143874,			/*207 */
	144222,			/*208 */
	144568,			/*209 */
	144913,			/*210 */
	145258,			/*211 */
	145602,			/*212 */
	145945,			/*213 */
	146287,			/*214 */
	146628,			/*215 */
	146969,			/*216 */
	147309,			/*217 */
	147648,			/*218 */
	147986,			/*219 */
	148323,			/*220 */
	148660,			/*221 */
	148996,			/*222 */
	149331,			/*223 */
	149666,			/*224 */
	150000,			/*225 */
	150332,			/*226 */
	150665,			/*227 */
	150996,			/*228 */
	151327,			/*229 */
	151657,			/*230 */
	151986,			/*231 */
	152315,			/*232 */
	152643,			/*233 */
	152970,			/*234 */
	153297,			/*235 */
	153622,			/*236 */
	153948,			/*237 */
	154272,			/*238 */
	154596,			/*239 */
	154919,			/*240 */
	155241,			/*241 */
	155563,			/*242 */
	155884,			/*243 */
	156204,			/*244 */
	156524,			/*245 */
	156843,			/*246 */
	157162,			/*247 */
	157480,			/*248 */
	157797,			/*249 */
	158113,			/*250 */
	158429,			/*251 */
	158745,			/*252 */
	159059,			/*253 */
	159373,			/*254 */
	159687,			/*255 */
	160000,			/*256 */
	160312,			/*257 */
	160623,			/*258 */
	160934,			/*259 */
	161245,			/*260 */
	161554,			/*261 */
	161864,			/*262 */
	162172,			/*263 */
	162480,			/*264 */
	162788,			/*265 */
	163095,			/*266 */
	163401,			/*267 */
	163707,			/*268 */
	164012,			/*269 */
	164316,			/*270 */
	164620,			/*271 */
	164924,			/*272 */
	165227,			/*273 */
	165529,			/*274 */
	165831,			/*275 */
	166132,			/*276 */
	166433,			/*277 */
	166733,			/*278 */
	167032,			/*279 */
	167332,			/*280 */
	167630,			/*281 */
	167928,			/*282 */
	168226,			/*283 */
	168522,			/*284 */
	168819,			/*285 */
	169115,			/*286 */
	169410,			/*287 */
	169705,			/*288 */
	170000,			/*289 */
	170293,			/*290 */
	170587,			/*291 */
	170880,			/*292 */
	171172,			/*293 */
	171464,			/*294 */
	171755,			/*295 */
	172046,			/*296 */
	172336,			/*297 */
	172626,			/*298 */
	172916,			/*299 */
	173205,			/*300 */
	173493,			/*301 */
	173781,			/*302 */
	174068,			/*303 */
	174355,			/*304 */
	174642,			/*305 */
	174928,			/*306 */
	175214,			/*307 */
	175499,			/*308 */
	175783,			/*309 */
	176068,			/*310 */
	176351,			/*311 */
	176635,			/*312 */
	176918,			/*313 */
	177200,			/*314 */
	177482,			/*315 */
	177763,			/*316 */
	178044,			/*317 */
	178325,			/*318 */
	178605,			/*319 */
	178885,			/*320 */
	179164,			/*321 */
	179443,			/*322 */
	179722,			/*323 */
	180000,			/*324 */
	180277,			/*325 */
	180554,			/*326 */
	180831,			/*327 */
	181107,			/*328 */
	181383,			/*329 */
	181659,			/*330 */
	181934,			/*331 */
	182208,			/*332 */
	182482,			/*333 */
	182756,			/*334 */
	183030,			/*335 */
	183303,			/*336 */
	183575,			/*337 */
	183847,			/*338 */
	184119,			/*339 */
	184390,			/*340 */
	184661,			/*341 */
	184932,			/*342 */
	185202,			/*343 */
	185472,			/*344 */
	185741,			/*345 */
	186010,			/*346 */
	186279,			/*347 */
	186547,			/*348 */
	186815,			/*349 */
	187082,			/*350 */
	187349,			/*351 */
	187616,			/*352 */
	187882,			/*353 */
	188148,			/*354 */
	188414,			/*355 */
	188679,			/*356 */
	188944,			/*357 */
	189208,			/*358 */
	189472,			/*359 */
	189736,			/*360 */
	190000,			/*361 */
	190262,			/*362 */
	190525,			/*363 */
	190787,			/*364 */
	191049,			/*365 */
	191311,			/*366 */
	191572,			/*367 */
	191833,			/*368 */
	192093,			/*369 */
	192353,			/*370 */
	192613,			/*371 */
	192873,			/*372 */
	193132,			/*373 */
	193390,			/*374 */
	193649,			/*375 */
	193907,			/*376 */
	194164,			/*377 */
	194422,			/*378 */
	194679,			/*379 */
	194935,			/*380 */
	195192,			/*381 */
	195448,			/*382 */
	195703,			/*383 */
	195959,			/*384 */
	196214,			/*385 */
	196468,			/*386 */
	196723,			/*387 */
	196977,			/*388 */
	197230,			/*389 */
	197484,			/*390 */
	197737,			/*391 */
	197989,			/*392 */
	198242,			/*393 */
	198494,			/*394 */
	198746,			/*395 */
	198997,			/*396 */
	199248,			/*397 */
	199499,			/*398 */
	199749,			/*399 */
	200000,			/*400 */
	200249,			/*401 */
	200499,			/*402 */
	200748,			/*403 */
	200997,			/*404 */
	201246,			/*405 */
	201494,			/*406 */
	201742,			/*407 */
	201990,			/*408 */
	202237,			/*409 */
	202484,			/*410 */
	202731,			/*411 */
	202977,			/*412 */
	203224,			/*413 */
	203469,			/*414 */
	203715,			/*415 */
	203960,			/*416 */
	204205,			/*417 */
	204450,			/*418 */
	204694,			/*419 */
	204939,			/*420 */
	205182,			/*421 */
	205426,			/*422 */
	205669,			/*423 */
	205912,			/*424 */
	206155,			/*425 */
	206397,			/*426 */
	206639,			/*427 */
	206881,			/*428 */
	207123,			/*429 */
	207364,			/*430 */
	207605,			/*431 */
	207846,			/*432 */
	208086,			/*433 */
	208326,			/*434 */
	208566,			/*435 */
	208806,			/*436 */
	209045,			/*437 */
	209284,			/*438 */
	209523,			/*439 */
	209761,			/*440 */
	210000,			/*441 */
	210237,			/*442 */
	210475,			/*443 */
	210713,			/*444 */
	210950,			/*445 */
	211187,			/*446 */
	211423,			/*447 */
	211660,			/*448 */
	211896,			/*449 */
	212132,			/*450 */
	212367,			/*451 */
	212602,			/*452 */
	212837,			/*453 */
	213072,			/*454 */
	213307,			/*455 */
	213541,			/*456 */
	213775,			/*457 */
	214009,			/*458 */
	214242,			/*459 */
	214476,			/*460 */
	214709,			/*461 */
	214941,			/*462 */
	215174,			/*463 */
	215406,			/*464 */
	215638,			/*465 */
	215870,			/*466 */
	216101,			/*467 */
	216333,			/*468 */
	216564,			/*469 */
	216794,			/*470 */
	217025,			/*471 */
	217255,			/*472 */
	217485,			/*473 */
	217715,			/*474 */
	217944,			/*475 */
	218174,			/*476 */
	218403,			/*477 */
	218632,			/*478 */
	218860,			/*479 */
	219089,			/*480 */
	219317,			/*481 */
	219544,			/*482 */
	219772,			/*483 */
	220000,			/*484 */
	220227,			/*485 */
	220454,			/*486 */
	220680,			/*487 */
	220907,			/*488 */
	221133,			/*489 */
	221359,			/*490 */
	221585,			/*491 */
	221810,			/*492 */
	222036,			/*493 */
	222261,			/*494 */
	222485,			/*495 */
	222710,			/*496 */
	222934,			/*497 */
	223159,			/*498 */
	223383,			/*499 */
	223606,			/*500 */
	223830,			/*501 */
	224053,			/*502 */
	224276,			/*503 */
	224499,			/*504 */
	224722,			/*505 */
	224944,			/*506 */
	225166,			/*507 */
	225388,			/*508 */
	225610,			/*509 */
	225831,			/*510 */
	226053,			/*511 */
	226274,			/*512 */
	226495,			/*513 */
	226715,			/*514 */
	226936,			/*515 */
	227156,			/*516 */
	227376,			/*517 */
	227596,			/*518 */
	227815,			/*519 */
	228035,			/*520 */
	228254,			/*521 */
	228473,			/*522 */
	228691,			/*523 */
	228910,			/*524 */
	229128,			/*525 */
	229346,			/*526 */
	229564,			/*527 */
	229782,			/*528 */
	230000,			/*529 */
	230217,			/*530 */
	230434,			/*531 */
	230651,			/*532 */
	230867,			/*533 */
	231084,			/*534 */
	231300,			/*535 */
	231516,			/*536 */
	231732,			/*537 */
	231948,			/*538 */
	232163,			/*539 */
	232379,			/*540 */
	232594,			/*541 */
	232808,			/*542 */
	233023,			/*543 */
	233238,			/*544 */
	233452,			/*545 */
	233666,			/*546 */
	233880,			/*547 */
	234093,			/*548 */
	234307,			/*549 */
	234520,			/*550 */
	234733,			/*551 */
	234946,			/*552 */
	235159,			/*553 */
	235372,			/*554 */
	235584,			/*555 */
	235796,			/*556 */
	236008,			/*557 */
	236220,			/*558 */
	236431,			/*559 */
	236643,			/*560 */
	236854,			/*561 */
	237065,			/*562 */
	237276,			/*563 */
	237486,			/*564 */
	237697,			/*565 */
	237907,			/*566 */
	238117,			/*567 */
	238327,			/*568 */
	238537,			/*569 */
	238746,			/*570 */
	238956,			/*571 */
	239165,			/*572 */
	239374,			/*573 */
	239582,			/*574 */
	239791,			/*575 */
	240000,			/*576 */
	240208,			/*577 */
	240416,			/*578 */
	240624,			/*579 */
	240831,			/*580 */
	241039,			/*581 */
	241246,			/*582 */
	241453,			/*583 */
	241660,			/*584 */
	241867,			/*585 */
	242074,			/*586 */
	242280,			/*587 */
	242487,			/*588 */
	242693,			/*589 */
	242899,			/*590 */
	243104,			/*591 */
	243310,			/*592 */
	243515,			/*593 */
	243721,			/*594 */
	243926,			/*595 */
	244131,			/*596 */
	244335,			/*597 */
	244540,			/*598 */
	244744,			/*599 */
	244948,			/*600 */
	245153,			/*601 */
	245356,			/*602 */
	245560,			/*603 */
	245764,			/*604 */
	245967,			/*605 */
	246170,			/*606 */
	246373,			/*607 */
	246576,			/*608 */
	246779,			/*609 */
	246981,			/*610 */
	247184,			/*611 */
	247386,			/*612 */
	247588,			/*613 */
	247790,			/*614 */
	247991,			/*615 */
	248193,			/*616 */
	248394,			/*617 */
	248596,			/*618 */
	248797,			/*619 */
	248997,			/*620 */
	249198,			/*621 */
	249399,			/*622 */
	249599,			/*623 */
	249799,			/*624 */
	250000,			/*625 */
	250199,			/*626 */
	250399,			/*627 */
	250599,			/*628 */
	250798,			/*629 */
	250998,			/*630 */
	251197,			/*631 */
	251396,			/*632 */
	251594,			/*633 */
	251793,			/*634 */
	251992,			/*635 */
	252190,			/*636 */
	252388,			/*637 */
	252586,			/*638 */
	252784,			/*639 */
	252982,			/*640 */
	253179,			/*641 */
	253377,			/*642 */
	253574,			/*643 */
	253771,			/*644 */
	253968,			/*645 */
	254165,			/*646 */
	254361,			/*647 */
	254558,			/*648 */
	254754,			/*649 */
	254950,			/*650 */
	255147,			/*651 */
	255342,			/*652 */
	255538,			/*653 */
	255734,			/*654 */
	255929,			/*655 */
	256124,			/*656 */
	256320,			/*657 */
	256515,			/*658 */
	256709,			/*659 */
	256904,			/*660 */
	257099,			/*661 */
	257293,			/*662 */
	257487,			/*663 */
	257681,			/*664 */
	257875,			/*665 */
	258069,			/*666 */
	258263,			/*667 */
	258456,			/*668 */
	258650,			/*669 */
	258843,			/*670 */
	259036,			/*671 */
	259229,			/*672 */
	259422,			/*673 */
	259615,			/*674 */
	259807,			/*675 */
	260000,			/*676 */
	260192,			/*677 */
	260384,			/*678 */
	260576,			/*679 */
	260768,			/*680 */
	260959,			/*681 */
	261151,			/*682 */
	261342,			/*683 */
	261533,			/*684 */
	261725,			/*685 */
	261916,			/*686 */
	262106,			/*687 */
	262297,			/*688 */
	262488,			/*689 */
	262678,			/*690 */
	262868,			/*691 */
	263058,			/*692 */
	263248,			/*693 */
	263438,			/*694 */
	263628,			/*695 */
	263818,			/*696 */
	264007,			/*697 */
	264196,			/*698 */
	264386,			/*699 */
	264575,			/*700 */
	264764,			/*701 */
	264952,			/*702 */
	265141,			/*703 */
	265329,			/*704 */
	265518,			/*705 */
	265706,			/*706 */
	265894,			/*707 */
	266082,			/*708 */
	266270,			/*709 */
	266458,			/*710 */
	266645,			/*711 */
	266833,			/*712 */
	267020,			/*713 */
	267207,			/*714 */
	267394,			/*715 */
	267581,			/*716 */
	267768,			/*717 */
	267955,			/*718 */
	268141,			/*719 */
	268328,			/*720 */
	268514,			/*721 */
	268700,			/*722 */
	268886,			/*723 */
	269072,			/*724 */
	269258,			/*725 */
	269443,			/*726 */
	269629,			/*727 */
	269814,			/*728 */
	270000,			/*729 */
	270185,			/*730 */
	270370,			/*731 */
	270554,			/*732 */
	270739,			/*733 */
	270924,			/*734 */
	271108,			/*735 */
	271293,			/*736 */
	271477,			/*737 */
	271661,			/*738 */
	271845,			/*739 */
	272029,			/*740 */
	272213,			/*741 */
	272396,			/*742 */
	272580,			/*743 */
	272763,			/*744 */
	272946,			/*745 */
	273130,			/*746 */
	273313,			/*747 */
	273495,			/*748 */
	273678,			/*749 */
	273861,			/*750 */
	274043,			/*751 */
	274226,			/*752 */
	274408,			/*753 */
	274590,			/*754 */
	274772,			/*755 */
	274954,			/*756 */
	275136,			/*757 */
	275317,			/*758 */
	275499,			/*759 */
	275680,			/*760 */
	275862,			/*761 */
	276043,			/*762 */
	276224,			/*763 */
	276405,			/*764 */
	276586,			/*765 */
	276767,			/*766 */
	276947,			/*767 */
	277128,			/*768 */
	277308,			/*769 */
	277488,			/*770 */
	277668,			/*771 */
	277848,			/*772 */
	278028,			/*773 */
	278208,			/*774 */
	278388,			/*775 */
	278567,			/*776 */
	278747,			/*777 */
	278926,			/*778 */
	279105,			/*779 */
	279284,			/*780 */
	279463,			/*781 */
	279642,			/*782 */
	279821,			/*783 */
	280000,			/*784 */
	280178,			/*785 */
	280356,			/*786 */
	280535,			/*787 */
	280713,			/*788 */
	280891,			/*789 */
	281069,			/*790 */
	281247,			/*791 */
	281424,			/*792 */
	281602,			/*793 */
	281780,			/*794 */
	281957,			/*795 */
	282134,			/*796 */
	282311,			/*797 */
	282488,			/*798 */
	282665,			/*799 */
	282842,			/*800 */
	283019,			/*801 */
	283196,			/*802 */
	283372,			/*803 */
	283548,			/*804 */
	283725,			/*805 */
	283901,			/*806 */
	284077,			/*807 */
	284253,			/*808 */
	284429,			/*809 */
	284604,			/*810 */
	284780,			/*811 */
	284956,			/*812 */
	285131,			/*813 */
	285306,			/*814 */
	285482,			/*815 */
	285657,			/*816 */
	285832,			/*817 */
	286006,			/*818 */
	286181,			/*819 */
	286356,			/*820 */
	286530,			/*821 */
	286705,			/*822 */
	286879,			/*823 */
	287054,			/*824 */
	287228,			/*825 */
	287402,			/*826 */
	287576,			/*827 */
	287749,			/*828 */
	287923,			/*829 */
	288097,			/*830 */
	288270,			/*831 */
	288444,			/*832 */
	288617,			/*833 */
	288790,			/*834 */
	288963,			/*835 */
	289136,			/*836 */
	289309,			/*837 */
	289482,			/*838 */
	289654,			/*839 */
	289827,			/*840 */
	290000,			/*841 */
	290172,			/*842 */
	290344,			/*843 */
	290516,			/*844 */
	290688,			/*845 */
	290860,			/*846 */
	291032,			/*847 */
	291204,			/*848 */
	291376,			/*849 */
	291547,			/*850 */
	291719,			/*851 */
	291890,			/*852 */
	292061,			/*853 */
	292232,			/*854 */
	292403,			/*855 */
	292574,			/*856 */
	292745,			/*857 */
	292916,			/*858 */
	293087,			/*859 */
	293257,			/*860 */
	293428,			/*861 */
	293598,			/*862 */
	293768,			/*863 */
	293938,			/*864 */
	294108,			/*865 */
	294278,			/*866 */
	294448,			/*867 */
	294618,			/*868 */
	294788,			/*869 */
	294957,			/*870 */
	295127,			/*871 */
	295296,			/*872 */
	295465,			/*873 */
	295634,			/*874 */
	295803,			/*875 */
	295972,			/*876 */
	296141,			/*877 */
	296310,			/*878 */
	296479,			/*879 */
	296647,			/*880 */
	296816,			/*881 */
	296984,			/*882 */
	297153,			/*883 */
	297321,			/*884 */
	297489,			/*885 */
	297657,			/*886 */
	297825,			/*887 */
	297993,			/*888 */
	298161,			/*889 */
	298328,			/*890 */
	298496,			/*891 */
	298663,			/*892 */
	298831,			/*893 */
	298998,			/*894 */
	299165,			/*895 */
	299332,			/*896 */
	299499,			/*897 */
	299666,			/*898 */
	299833,			/*899 */
	300000,			/*900 */
	300166,			/*901 */
	300333,			/*902 */
	300499,			/*903 */
	300665,			/*904 */
	300832,			/*905 */
	300998,			/*906 */
	301164,			/*907 */
	301330,			/*908 */
	301496,			/*909 */
	301662,			/*910 */
	301827,			/*911 */
	301993,			/*912 */
	302158,			/*913 */
	302324,			/*914 */
	302489,			/*915 */
	302654,			/*916 */
	302820,			/*917 */
	302985,			/*918 */
	303150,			/*919 */
	303315,			/*920 */
	303479,			/*921 */
	303644,			/*922 */
	303809,			/*923 */
	303973,			/*924 */
	304138,			/*925 */
	304302,			/*926 */
	304466,			/*927 */
	304630,			/*928 */
	304795,			/*929 */
	304959,			/*930 */
	305122,			/*931 */
	305286,			/*932 */
	305450,			/*933 */
	305614,			/*934 */
	305777,			/*935 */
	305941,			/*936 */
	306104,			/*937 */
	306267,			/*938 */
	306431,			/*939 */
	306594,			/*940 */
	306757,			/*941 */
	306920,			/*942 */
	307083,			/*943 */
	307245,			/*944 */
	307408,			/*945 */
	307571,			/*946 */
	307733,			/*947 */
	307896,			/*948 */
	308058,			/*949 */
	308220,			/*950 */
	308382,			/*951 */
	308544,			/*952 */
	308706,			/*953 */
	308868,			/*954 */
	309030,			/*955 */
	309192,			/*956 */
	309354,			/*957 */
	309515,			/*958 */
	309677,			/*959 */
	309838,			/*960 */
	310000,			/*961 */
	310161,			/*962 */
	310322,			/*963 */
	310483,			/*964 */
	310644,			/*965 */
	310805,			/*966 */
	310966,			/*967 */
	311126,			/*968 */
	311287,			/*969 */
	311448,			/*970 */
	311608,			/*971 */
	311769,			/*972 */
	311929,			/*973 */
	312089,			/*974 */
	312249,			/*975 */
	312409,			/*976 */
	312569,			/*977 */
	312729,			/*978 */
	312889,			/*979 */
	313049,			/*980 */
	313209,			/*981 */
	313368,			/*982 */
	313528,			/*983 */
	313687,			/*984 */
	313847,			/*985 */
	314006,			/*986 */
	314165,			/*987 */
	314324,			/*988 */
	314483,			/*989 */
	314642,			/*990 */
	314801,			/*991 */
	314960,			/*992 */
	315119,			/*993 */
	315277,			/*994 */
	315436,			/*995 */
	315594,			/*996 */
	315753,			/*997 */
	315911,			/*998 */
	316069,			/*999 */
};


int32 isp_get_zoom_window(uint32 orig_w, uint32 orig_h, uint32 zoom,
				Window_t *zoom_wnd)
{
	/*
	 *
	 * X*Y = zoom*X'*Y'
	 * X/Y = X'/Y'
	 * ==>  X'=X/sqrt(zoom), Y'=Y/sqrt(zoom)
	 *
	 */
	int32 ret = RET_FAILURE;

	if ((zoom >= 100) && (zoom <= 400) && zoom_wnd) {
		/*calcluate current width and height */
		uint32 w = orig_w * 100000 / sqrtmap[zoom];
		uint32 h = orig_h * 100000 / sqrtmap[zoom];
		/*calcluate current left and top */
		zoom_wnd->h_offset = (orig_w - w) / 2;
		zoom_wnd->v_offset = (orig_h - h) / 2;
		/*calcluate current right and bottom */
		zoom_wnd->h_size = w;
		zoom_wnd->v_size = h;
		/*make offset and size are even */
		zoom_wnd->h_offset &= ~1;
		zoom_wnd->v_offset &= ~1;
		zoom_wnd->h_size &= ~1;
		zoom_wnd->v_size &= ~1;

		if (w && h) {
			ret = RET_SUCCESS;
			isp_dbg_print_info
		("-><- %s suc,%u:%u zoom %u [%u,%u,%u,%u]\n", __func__,
				orig_w, orig_h, zoom, zoom_wnd->h_offset,
				zoom_wnd->v_offset, zoom_wnd->h_size,
				zoom_wnd->v_size);
		} else {
			isp_dbg_print_err("-><- %s fail orig %u:%u zoom %u\n",
				__func__, orig_w, orig_h, zoom);
		}
	} else {
		isp_dbg_print_err("-><- %s fail bad zoom %u or wnd %p\n",
			__func__, zoom, zoom_wnd);
	};

	return ret;
}

void ref_no_op(pvoid context)
{
	unreferenced_parameter(context);
};

void deref_no_op(pvoid context)
{
	unreferenced_parameter(context);
}

void isp_module_if_init_ext(struct isp_isp_module_if *p_interface)
{
	isp_dbg_print_info("-> %s,inf %p\n", __func__, p_interface);
	if (p_interface) {
		p_interface->get_init_para = get_init_para_imp;
		p_interface->get_isp_work_buf = get_isp_work_buf_imp;
	};
	isp_dbg_print_info("<- %s\n", __func__);
}

void isp_module_if_init(struct isp_isp_module_if *p_interface)
{
	isp_dbg_print_info("-> %s\n", __func__);
	if (p_interface) {
	/*rtl_zero_memory(p_interface, sizeof(struct isp_isp_module_if)); */
		//memset(p_interface, 0, sizeof(struct isp_isp_module_if));
		p_interface->size = sizeof(struct isp_isp_module_if);
		p_interface->version = ISP_MODULE_IF_VERSION;
		p_interface->context = &g_isp_context;

		isp_dbg_print_info("interface_context %p\n",
			p_interface->context);
		p_interface->if_reference = ref_no_op;
		p_interface->if_dereference = deref_no_op;
		p_interface->set_fw_bin = set_fw_bin_imp;
		p_interface->set_calib_bin = set_calib_bin_imp;
		p_interface->de_init = de_init;
		p_interface->open_camera = open_camera_imp;
		p_interface->close_camera = close_camera_imp;
		p_interface->start_preview = start_preview_imp;
		p_interface->stop_preview = stop_preview_imp;
		p_interface->set_preview_buf = set_preview_buf_imp;
		p_interface->reg_notify_cb = reg_notify_cb_imp;
		p_interface->unreg_notify_cb = unreg_notify_cb_imp;
		p_interface->i2c_read_mem = i2c_read_mem_imp;
		p_interface->i2c_write_mem = i2c_write_mem_imp;
		p_interface->i2c_read_reg = i2c_read_reg_imp;
		p_interface->i2c_read_reg_fw = i2c_read_reg_fw_imp;
		p_interface->i2c_write_reg = i2c_write_reg_imp;
		p_interface->i2c_write_reg_fw = i2c_write_reg_fw_imp;
		p_interface->reg_snr_op = reg_snr_op_imp;
		p_interface->unreg_snr_op = unreg_snr_op_imp;
		p_interface->reg_vcm_op = reg_vcm_op_imp;
		p_interface->unreg_vcm_op = unreg_vcm_op_imp;
		p_interface->reg_flash_op = reg_flash_op_imp;
		p_interface->unreg_flash_op = unreg_flash_op_imp;
		p_interface->take_one_pic = take_one_pic_imp;
		p_interface->stop_take_one_pic = stop_take_one_pic_imp;
		/*
		 *p_interface->set_one_photo_seq_buf =
		 *				set_one_photo_seq_buf_imp;
		 *p_interface->start_photo_seq = start_photo_seq_imp;
		 *p_interface->stop_photo_seq = stop_photo_seq_imp;
		 */
		p_interface->isp_reg_write32 = write_isp_reg32_imp;
		p_interface->isp_reg_read32 = read_isp_reg32_imp;
		p_interface->set_common_para = set_common_para_imp;
		p_interface->get_common_para = get_common_para_imp;
		p_interface->set_preview_para = set_preview_para_imp;
		p_interface->get_prev_para = get_preview_para_imp;
		p_interface->set_video_para = set_video_para_imp;
		p_interface->get_video_para = get_video_para_imp;
		p_interface->set_rgbir_ir_para = set_rgbir_ir_para_imp;
		p_interface->get_rgbir_ir_para = get_rgbir_ir_para_imp;
		p_interface->set_zsl_para = set_zsl_para_imp;
		p_interface->get_zsl_para = get_zsl_para_imp;
		p_interface->set_rgbir_raw_para = set_rgbir_raw_para_imp;
		p_interface->get_rgbir_raw_para = get_rgbir_raw_para_imp;

		p_interface->set_video_buf = set_video_buf_imp;
		p_interface->start_video = start_video_imp;
		p_interface->start_rgbir_ir = start_rgbir_ir_imp;
		p_interface->stop_video = stop_video_imp;
		p_interface->stop_rgbir_ir = stop_rgbir_ir_imp;
		p_interface->get_capabilities = get_capabilities_imp;
		//p_interface->snr_pwr_set = snr_pwr_set_imp;
		p_interface->snr_clk_set = snr_clk_set_imp;
		p_interface->get_camera_res_fps = get_camera_res_fps_imp;
		//p_interface->enable_tnr = enable_tnr_imp;
		//p_interface->disable_tnr = disable_tnr_imp;
		//p_interface->enable_vs = enable_vs_imp;
		//p_interface->disable_vs = disable_vs_imp;
		p_interface->take_raw_pic = take_raw_pic_imp;
		p_interface->stop_take_raw_pic = stop_take_raw_pic_imp;
		p_interface->set_focus_mode = set_focus_mode_imp;
		p_interface->auto_focus_lock = auto_focus_lock_imp;
		p_interface->auto_focus_unlock = auto_focus_unlock_imp;
		p_interface->auto_exposure_lock = auto_exposure_lock_imp;
		p_interface->auto_exposure_unlock = auto_exposure_unlock_imp;
		p_interface->auto_wb_lock = auto_wb_lock_imp;
		p_interface->auto_wb_unlock = auto_wb_unlock_imp;
		p_interface->set_lens_position = set_lens_position_imp;
		p_interface->get_focus_position = get_focus_position_imp;
		p_interface->set_exposure_mode = set_exposure_mode_imp;
		p_interface->set_wb_mode = set_wb_mode_imp;
		p_interface->set_snr_ana_gain = set_snr_ana_gain_imp;
		p_interface->set_snr_dig_gain = set_snr_dig_gain_imp;
		p_interface->get_snr_gain = get_snr_gain_imp;
		p_interface->get_snr_gain_cap = get_snr_gain_cap_imp;
		p_interface->set_snr_itime = set_snr_itime_imp;
		p_interface->get_snr_itime = get_snr_itime_imp;
		p_interface->get_snr_itime_cap = get_snr_itime_cap_imp;
		p_interface->reg_fw_parser = reg_fw_parser_imp;
		p_interface->unreg_fw_parser = unreg_fw_parser_imp;
		p_interface->set_fw_gv = set_fw_gv_imp;
		p_interface->set_digital_zoom = set_digital_zoom_imp;
		//p_interface->flash_config = flash_config_imp;
		p_interface->get_snr_raw_img_type = get_snr_raw_img_type_imp;
		p_interface->set_iso_value = set_iso_value_imp;
		p_interface->set_ev_value = set_ev_value_imp;
		//p_interface->get_sharpness = get_sharpness_imp;
		//p_interface->set_color_effect = set_color_effect_imp;
		p_interface->set_zsl_buf = set_zsl_buf_imp;
		p_interface->start_zsl = start_zsl_imp;
		p_interface->stop_zsl = stop_zsl_imp;
		//p_interface->take_one_pp_jpeg = take_one_pp_jpeg_imp;
		p_interface->set_digital_zoom_ratio =
			set_digital_zoom_ratio_imp;
		p_interface->set_digital_zoom_pos = set_digital_zoom_pos_imp;
		p_interface->test_gfx_interface = test_gfx_interface_imp;
		p_interface->set_metadata_buf = set_metadata_buf_imp;
		//p_interface->start_metadata = start_metadata_imp;
		//p_interface->stop_metadata = stop_metadata_imp;
		p_interface->get_camera_raw_type = get_camera_raw_type_imp;
		p_interface->get_camera_dev_info = get_camera_dev_info_imp;
		p_interface->set_wb_light = set_wb_light_imp;
		p_interface->disable_color_effect = disable_color_effect_imp;
		p_interface->set_awb_roi = set_awb_roi_imp;
		p_interface->set_ae_roi = set_ae_roi_imp;
		p_interface->set_af_roi = set_af_roi_imp;
		p_interface->start_af = start_af_imp;
		p_interface->stop_af = stop_af_imp;
		p_interface->cancel_af = cancel_af_imp;
		p_interface->start_ae = start_ae_imp;
		p_interface->stop_ae = stop_ae_imp;
		p_interface->start_wb = start_wb_imp;
		p_interface->stop_wb = stop_wb_imp;
		p_interface->set_scene_mode = set_scene_mode_imp;
		p_interface->set_gamma = set_gamma_imp;
		p_interface->set_color_temperature =
				set_color_temperature_imp;
		p_interface->set_snr_calib_data = set_snr_calib_data_imp;

		p_interface->map_handle_to_vram_addr =
		    map_handle_to_vram_addr_imp;
		p_interface->set_per_frame_ctl_info =
		    set_per_frame_ctl_info_imp;
		p_interface->set_drv_settings = set_drv_settings_imp;
		p_interface->enable_dynamic_frame_rate =
		    enable_dynamic_frame_rate_imp;
		p_interface->set_max_frame_rate = set_max_frame_rate_imp;
		p_interface->set_iso_priority = set_iso_priority_imp;
		p_interface->set_flicker = set_flicker_imp;
		p_interface->set_ev_compensation = set_ev_compensation_imp;
		p_interface->set_lens_range = set_lens_range_imp;

		p_interface->set_flash_mode = set_flash_mode_imp;
		p_interface->set_flash_power = set_flash_power_imp;
		p_interface->set_red_eye_mode = set_red_eye_mode_imp;
		p_interface->set_flash_assist_mode = set_flash_assist_mode_imp;
		p_interface->set_flash_assist_power =
						set_flash_assist_power_imp;
		p_interface->set_video_hdr = set_video_hdr_imp;
		p_interface->set_photo_hdr = set_video_hdr_imp;

		p_interface->set_lens_shading_matrix =
						set_lens_shading_matrix_imp;
		p_interface->set_lens_shading_sector =
						set_lens_shading_sector_imp;
		p_interface->set_awb_cc_matrix = set_awb_cc_matrix_imp;
		p_interface->set_awb_cc_offset = set_awb_cc_offset_imp;
		p_interface->gamma_enable = gamma_enable_imp;
		p_interface->wdr_enable = wdr_enable_imp;
		p_interface->tnr_enable = tnr_enable_imp;
		p_interface->snr_enable = snr_enable_imp;
		p_interface->dpf_enable = dpf_enable_imp;
		p_interface->start_rgbir = start_rgbir_imp;
		p_interface->stop_rgbir = stop_rgbir_imp;
		p_interface->set_rgbir_output_buf = set_rgbir_output_buf_imp;
		p_interface->set_rgbir_input_buf = set_rgbir_input_buf_imp;
		p_interface->set_mem_cam_input_buf = set_mem_cam_input_buf_imp;
		p_interface->get_original_wnd_size = get_original_wnd_size_imp;
		p_interface->get_cproc_status = get_cproc_status_imp;
		p_interface->cproc_enable = cproc_enable_imp;
		p_interface->cproc_set_contrast = cproc_set_contrast_imp;
		p_interface->cproc_set_brightness = cproc_set_brightness_imp;
		p_interface->cproc_set_saturation = cproc_set_saturation_imp;
		p_interface->cproc_set_hue = cproc_set_hue_imp;
		p_interface->get_raw_img_info = get_raw_img_info_imp;
		p_interface->fw_cmd_send = fw_cmd_send_imp;
		p_interface->get_tuning_data_info = get_tuning_data_info_imp;
		p_interface->loopback_start = loopback_start_imp;
		p_interface->loopback_stop = loopback_stop_imp;
		p_interface->loopback_set_raw = loopback_set_raw_imp;
		p_interface->loopback_process_raw = loopback_process_raw_imp;
		p_interface->set_face_authtication_mode =
						set_face_authtication_mode_imp;
	}
	isp_dbg_print_info("<- %s\n", __func__);
}

/*corresponding to aidt_ctrl_open_board*/
int32 set_fw_bin_imp(pvoid context, pvoid fw_data, uint32 fw_len)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((isp == NULL)
	|| (fw_data == NULL)
	|| (fw_len == 0)) {
		isp_dbg_print_err
			("-><- %s fail for illegal parameter\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->fw_len && isp->fw_data) {
		isp_dbg_print_info
			("-><- %s suc, do none for already done\n", __func__);
		return IMF_RET_SUCCESS;
	}

	isp->fw_len = fw_len;
	if (isp->fw_data) {
		isp_sys_mem_free(isp->fw_data);
		isp->fw_data = NULL;
	};

	isp->fw_data = isp_sys_mem_alloc(fw_len);
	if (isp->fw_data == NULL) {
		isp_dbg_print_err("<- %s fail for aloc fw buf,len %u\n",
			__func__, isp->fw_len);
		goto quit;
	} else {
		memcpy(isp->fw_data, fw_data, isp->fw_len);
		isp_dbg_print_info("fw %p,len %u\n", isp->fw_data, isp->fw_len);
	};

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
quit:

	return IMF_RET_FAIL;
};

int32 set_calib_bin_imp(pvoid context, enum camera_id cam_id,
			pvoid calib_data, uint32 len)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)
	|| (calib_data == NULL)
	|| (len == 0)) {
		isp_dbg_print_err
			("-><- %s fail for illegal parameter\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->sensor_info[cam_id].status != START_STATUS_NOT_START) {
		isp_dbg_print_err
			("-><- %s fail for bad status %d\n", __func__,
			isp->sensor_info[cam_id].status);
		return IMF_RET_FAIL;
	};

	uninit_cam_calib_item(isp, cam_id);
	init_cam_calib_item(isp, cam_id);

	isp_dbg_print_info("-> %s, cid %d\n", __func__, cam_id);

	if (!parse_calib_data(isp, cam_id, calib_data, len)) {
		isp_dbg_print_err
			("<- %s fail, bad calib, %p, len %u\n", __func__,
			calib_data, len);
		goto quit;
	} else {
		isp_dbg_print_info
			("<- %s suc,good calib, %p, len %u\n", __func__,
			calib_data, len);
	};

	return IMF_RET_SUCCESS;

quit:
	uninit_cam_calib_item(isp, cam_id);
	return IMF_RET_FAIL;
}

void de_init(pvoid context)
{
	unreferenced_parameter(context);
	unit_isp();
}

int32 open_camera_imp(pvoid context, enum camera_id cid,
			uint32 res_fps_id, uint32_t flag)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct camera_dev_info cam_dev_info;
	uint32 pix_size;
	uint32 res_fps;
	uint32 index;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_OC,
			PT_STATUS_SUCC);
	if (!is_para_legal(context, cid)) {
		isp_dbg_print_err("-><- %s, fail for para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->fw_data == NULL) {
		isp_dbg_print_err("-><- %s, fail for no fw\n", __func__);
		return IMF_RET_FAIL;
	}

	if ((cid == CAMERA_ID_MEM)
	&& ((res_fps_id == 0) || (flag == 0))) {
		isp_dbg_print_err
			("-><- %s,cid:%d,fail for bad w %d,h %d\n",
			__func__, cid, res_fps_id, flag);
		return IMF_RET_FAIL;
	}

	/*res_fps_id = isp_get_res_fps_id_for_str(isp,cid, str_id); */
	index = get_index_from_res_fps_id(context, cid, res_fps_id);

	if (ISP_GET_STATUS(isp) == ISP_STATUS_UNINITED) {
		isp_dbg_print_err
		("-><- %s,cid:%d,fail for isp uninit\n", __func__, cid);
		return IMF_RET_FAIL;
	}

	if ((cid < CAMERA_ID_MEM) && (flag & OPEN_CAMERA_FLAG_HDR)
	&& (!isp->sensor_info[cid].res_fps.res_fps[index].hdr_support)) {
		isp_dbg_print_err
		("-><- %s,fail for hdr open not support,cid:%d,fpsid:%d\n",
			__func__, cid, res_fps_id);
		return IMF_RET_FAIL;
	}

	if ((cid < CAMERA_ID_MEM) && (!(flag & OPEN_CAMERA_FLAG_HDR))
	&& (!isp->sensor_info[cid].res_fps.res_fps[index].none_hdr_support)) {
		isp_dbg_print_err
		("-><- %s,fail for normal open not support,cid:%d,fpsid:%d\n",
			__func__, cid, res_fps_id);
		return IMF_RET_FAIL;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (is_camera_started(isp, cid)) {
		isp_mutex_unlock(&isp->ops_mutex);
		if (cid < CAMERA_ID_MEM) {
			if ((isp->sensor_info[cid].cur_res_fps_id !=
			(char)res_fps_id)
			|| (isp->sensor_info[cid].open_flag != flag)) {
				isp_dbg_print_err
		("%s,cid:%d,fail for incompatible, fps id:%u->%u,flag:%u->%u)n",
					__func__, cid,
					isp->sensor_info[cid].cur_res_fps_id,
					res_fps_id,
					isp->sensor_info[cid].open_flag, flag);
				return IMF_RET_FAIL;
			}

			isp_dbg_print_info("-><- %s,cid:%d,suc for already\n",
				__func__, cid);
			return IMF_RET_SUCCESS;

		} else if (cid == CAMERA_ID_MEM) {
			if ((res_fps_id != isp->sensor_info[cid].raw_width)
			|| (flag != isp->sensor_info[cid].raw_height)) {
				isp_dbg_print_err
				("%s,fail for incompatible,w:%u->%u,h:%u->%u)n",
					__func__,
					isp->sensor_info[cid].raw_width,
					res_fps_id,
					isp->sensor_info[cid].raw_height,
					flag);
				return IMF_RET_FAIL;
			}

			isp_dbg_print_info("%s,cid:%d,suc for already\n",
				__func__, cid);
			return IMF_RET_SUCCESS;
		}
	}

	if (cid < CAMERA_ID_MEM) {
		isp_dbg_print_info
			("-> %s,cid:%d,fpsid:%d,flag:0x%x\n",
			__func__, cid, res_fps_id, flag);
	} else {
		isp_dbg_print_info
			("-> %s,cid:%d,fpsid(w):%d,flag(h):%u\n",
			__func__, cid, res_fps_id, flag);
	}
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_IPON,
			PT_STATUS_SUCC);
	if (cid < CAMERA_ID_MEM) {
		isp->sensor_info[cid].cam_type = CAMERA_TYPE_RGB_BAYER;
		if (IMF_RET_SUCCESS ==
		    get_camera_dev_info_imp(isp, cid, &cam_dev_info)) {
			isp->sensor_info[cid].cam_type = cam_dev_info.type;
		} else {
			isp_dbg_print_err
				("in %s, get_camera_dev_info_imp fail\n",
				__func__);
		};
		pix_size = isp->sensor_info[cid].res_fps.res_fps[index].width
			* isp->sensor_info[cid].res_fps.res_fps[index].height;
		res_fps = isp->sensor_info[cid].res_fps.res_fps[index].fps;
		if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_RGBIR)
			g_rgbir_raw_count_in_fw = 0;
	} else {		/*if (cid == CAMERA_ID_MEM) */

		isp->sensor_info[cid].cam_type = CAMERA_TYPE_MEM;
		isp->sensor_info[cid].res_fps.res_fps[index].width =
								res_fps_id;
		isp->sensor_info[cid].res_fps.res_fps[index].height = flag;
		isp->sensor_info[cid].res_fps.res_fps[index].fps = 30;
		//pix_size = res_fps_id * flag;
		//res_fps = 30;
	}

	if (isp_ip_pwr_on(isp, cid, index, flag & OPEN_CAMERA_FLAG_HDR) !=
	RET_SUCCESS) {
		isp_dbg_print_err("in %s, isp_ip_pwr_on fail\n", __func__);
		goto fail;
	};
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_IPON,
			PT_STATUS_SUCC);

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_PHYON,
			PT_STATUS_SUCC);
	if (isp_dphy_pwr_on(isp) != RET_SUCCESS) {
		isp_dbg_print_err("in %s, isp_dphy_pwr_on fail\n", __func__);
		goto fail;
	};
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD,
			PT_CMD_PHYON, PT_STATUS_SUCC);

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_SF,
			PT_STATUS_SUCC);
	if (isp_fw_start(isp) != RET_SUCCESS) {
		isp_dbg_print_err("in %s, isp_fw_start fail\n", __func__);
		goto fail;
	};
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_SF,
			PT_STATUS_SUCC);

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_SNRO,
			PT_STATUS_SUCC);
	if ((cid < CAMERA_ID_MEM)
	&& (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM)) {
		if (isp_snr_pwr_toggle(isp, cid, 1) != RET_SUCCESS) {
			isp_dbg_print_err
				("%s, isp_snr_pwr_toggle fail\n", __func__);
			goto fail;
		};
		if (isp_snr_clk_toggle(isp, cid, 1) != RET_SUCCESS) {
			isp_dbg_print_err
				("%s, isp_snr_clk_toggle fail\n", __func__);
			goto fail;
		};

		if (isp_snr_open(isp, cid, res_fps_id, flag) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s, isp_snr_open fail\n", __func__);
			goto fail;
		} else {
			isp_dbg_print_info
				("in %s,on privacy led\n", __func__);
			isp_hw_reg_write32(mmISP_GPIO_PRIV_LED, 0xff);

		};
	} else if (cid == CAMERA_ID_MEM) {
		struct isp_pwr_unit *pwr_unit;

		pwr_unit = &isp->isp_pu_cam[cid];
		isp_mutex_lock(&pwr_unit->pwr_status_mutex);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
		isp->sensor_info[cid].raw_width = res_fps_id;
		isp->sensor_info[cid].raw_height = flag;
		isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	} else {	//CAMERA_TYPE_MEM == isp->sensor_info[cid].cam_type
		struct isp_pwr_unit *pwr_unit;

		pwr_unit = &isp->isp_pu_cam[cid];
		isp_mutex_lock(&pwr_unit->pwr_status_mutex);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
		isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	};

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_SNRO,
			PT_STATUS_SUCC);

#ifdef SET_P_STAGE_WATERMARK_REGISTER
	// Program watermark registers
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_Y = 0x628c0;
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CB = 0x628c4;
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CR = 0x628c8;
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_Y = 0x62cc0;
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CB = 0x62cc4;
	uint32 MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CR = 0x62cc8;

	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_Y,
			(uint32) 0x10087);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CB,
			(uint32) 0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CR,
			(uint32) 0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_Y,
			(uint32) 0x10087);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CB,
			(uint32) 0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CR,
			(uint32) 0x10080);
#endif

#ifdef SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER
//1.DAGB0_WRCLI12/15/16/17 set as same as DAGB0_WRCLI10. (FE5FE0F8)
//2.MMEA0_DRAM_WR_CLI2GRP_MAP0->CID10_GROUP/CID11_GROUP/CID12_GROUP/CID15_GROUP
//	set to 0.
//3.MMEA0_DRAM_WR_CLI2GRP_MAP1->CID16_GROUP/CID17_GROUP set to 0.
#ifdef SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER_SWRT
	uint32 DAGB0_WRCLI10 = 0x00068128;
	uint32 DAGB0_WRCLI11 = 0x0006812C;
	uint32 DAGB0_WRCLI12 = 0x00068130;
	uint32 DAGB0_WRCLI15 = 0x0006813C;
	uint32 DAGB0_WRCLI16 = 0x00068140;
	uint32 DAGB0_WRCLI17 = 0x00068144;
	uint32 MMEA0_DRAM_WR_CLI2GRP_MAP0 = 0x00068408;
	uint32 MMEA0_DRAM_WR_CLI2GRP_MAP1 = 0x0006840C;
	uint32 reg_value;
//1.DAGB0_WRCLI12/15/16/17 set as same as DAGB0_WRCLI10. (FE5FE0F8)
//2.MMEA0_DRAM_WR_CLI2GRP_MAP0->CID10_GROUP/CID11_GROUP/CID12_GROUP/CID15_GROUP
//	set to 0.
//3.MMEA0_DRAM_WR_CLI2GRP_MAP1->CID16_GROUP/CID17_GROUP set to 0.
	isp_dbg_print_info("SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER\n");
	isp_hw_reg_write32(DAGB0_WRCLI10, 0xFE5FE0F8);
	isp_hw_reg_write32(DAGB0_WRCLI11, 0xFE5FE0F8);
	isp_hw_reg_write32(DAGB0_WRCLI12, 0xFE5FE0F8);
	isp_hw_reg_write32(DAGB0_WRCLI15, 0xFE5FE0F8);
	isp_hw_reg_write32(DAGB0_WRCLI16, 0xFE5FE0F8);
	isp_hw_reg_write32(DAGB0_WRCLI17, 0xFE5FE0F8);
	reg_value = isp_hw_reg_read32(MMEA0_DRAM_WR_CLI2GRP_MAP0);
	reg_value &= (~((3 << 20) | (3 << 22) | (3 << 24) | (3 << 30)));
	isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP0, reg_value);

	reg_value = isp_hw_reg_read32(MMEA0_DRAM_WR_CLI2GRP_MAP1);
	reg_value &= (~((3 << 0) | (3 << 2)));
	isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP1, reg_value);
#endif
#ifdef SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER_HRT
	{
		const uint32 shiftBits = 12;
		//const uint32 VMC_TAP_CONTEXT0_PDE_REQUEST_SNOOP = 8;
		//const uint32 VMC_TAP_CONTEXT0_PTE_REQUEST_SNOOP = 11;
		uint32 reg_value;
		uint64 reg_value2;
		uint32 DAGB0_WRCLI10 = 0x00068128;
		uint32 DAGB0_WRCLI11 = 0x0006812C;
		uint32 DAGB0_WRCLI12 = 0x00068130;
		uint32 DAGB0_WRCLI15 = 0x0006813C;
		uint32 DAGB0_WRCLI16 = 0x00068140;
		uint32 DAGB0_WRCLI17 = 0x00068144;
		uint32 MMEA0_DRAM_WR_CLI2GRP_MAP0 = 0x00068408;
		uint32 MMEA0_DRAM_WR_CLI2GRP_MAP1 = 0x0006840C;
		uint32 VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32 = 0x00069CAC;
		uint32 VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32 = 0x00069CB0;
		uint32 VM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32 = 0x00069D2C;
		uint32 VM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32 = 0x00069D30;
		uint32 VM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32 = 0x00069DAC;
		uint32 VM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32 = 0x00069DB0;

		uint32 VM_L2_SAW_CNTL = 0x00069800;
		uint32 VM_L2_SAW_CNTL2 = 0x00069804;
		uint32 VM_L2_SAW_CNTL3 = 0x00069808;
		uint32 VM_L2_SAW_CNTL4 = 0x0006980C;
		uint32 VM_L2_SAW_CONTEXT0_CNTL = 0x00069810;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32 =
		    0x00069818;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32 =
		    0x0006981C;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_LO32 =
		    0x00069820;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_HI32 =
		    0x00069824;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_LO32 = 0x00069828;
		uint32 VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_HI32 = 0x0006982C;

		isp_hw_reg_write32(VM_L2_SAW_CNTL, 0x0C0B8602);
		isp_hw_reg_write32(VM_L2_SAW_CNTL2, 0x00000003);
		isp_hw_reg_write32(VM_L2_SAW_CNTL3, 0x80100004);
		isp_hw_reg_write32(VM_L2_SAW_CNTL4, 0x00000901);
		isp_hw_reg_write32(VM_L2_SAW_CONTEXT0_CNTL, 0x00FFFED8);

		isp_hw_reg_write32
			(VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32));
		isp_hw_reg_write32
			(VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32));

		isp_hw_reg_write32(VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_LO32,
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32));
		isp_hw_reg_write32(VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_HI32,
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32));

		reg_value =
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32);
		reg_value2 = (uint64)
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32) <<
			32 | (uint64) reg_value;
		isp_hw_reg_write32(
			VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			(uint32) (reg_value2 >> shiftBits));
		isp_hw_reg_write32(
			VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			(uint32) (reg_value2 >> (shiftBits + 32)));

		isp_hw_reg_write32(DAGB0_WRCLI10, 0xFE5FE0FB);
		isp_hw_reg_write32(DAGB0_WRCLI11, 0xFE5FE0FB);
		isp_hw_reg_write32(DAGB0_WRCLI12, 0xFE5FE0FB);
		isp_hw_reg_write32(DAGB0_WRCLI15, 0xFE5FE0FB);
		isp_hw_reg_write32(DAGB0_WRCLI16, 0xFE5FE0FB);
		isp_hw_reg_write32(DAGB0_WRCLI17, 0xFE5FE0FB);

		isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP0, 0xD7F05645);
		isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP1, 0x0050000F);
	}
#endif
#endif

	//Put ISP DS4 bit to busy
	isp_program_mmhub_ds_reg(TRUE);
	if (isp_init_stream(isp, cid) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_init_stream\n", __func__);
		goto fail;
	};
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_info("<- %s, suc\n", __func__);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_OC,
			PT_STATUS_SUCC);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	close_camera_imp(isp, cid);
	isp_dbg_print_err("<- %s, fail\n", __func__);
	return IMF_RET_FAIL;
}

int32 close_camera_imp(pvoid context, enum camera_id cid)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	uint32 cnt;
	uint32 reg_val;
	struct isp_cmd_element *ele = NULL;

	if (!is_para_legal(context, cid)) {
		isp_dbg_print_err("-><- %s, fail for para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("-> %s, cid %d\n", __func__, cid);
	if (isp_uninit_stream(isp, cid) != RET_SUCCESS) {
		isp_mutex_unlock(&isp->ops_mutex);
		isp_dbg_print_info(
		"<- %s, cid %d fail by isp_uninit_stream\n", __func__, cid);
		return IMF_RET_FAIL;
	};
	sif = &isp->sensor_info[cid];
	if (sif->status == START_STATUS_STARTED) {
		isp_dbg_print_err
			("in %s, fail,stream still running\n", __func__);
		goto fail;
	};
	sif->status = START_STATUS_NOT_START;

	if ((cid < CAMERA_ID_MEM)
	&& (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM)) {
		uint32 index;
		struct sensor_info *snr_info;

		isp_snr_close(isp, cid);
		isp_snr_pwr_toggle(isp, cid, 0);
		isp_snr_clk_toggle(isp, cid, 0);	//problematic
		snr_info = &isp->sensor_info[cid];
		index = get_index_from_res_fps_id
				(isp, cid, snr_info->cur_res_fps_id);
		isp_clk_change(isp, cid, index, snr_info->hdr_enable, false);

	} else {//(cid == CAMERA_ID_MEM)
		//|| (CAMERA_TYPE_MEM == isp->sensor_info[cid].cam_type)
		struct isp_pwr_unit *pwr_unit;

		pwr_unit = &isp->isp_pu_cam[cid];
		isp_mutex_lock(&pwr_unit->pwr_status_mutex);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	};
	isp->sensor_info[cid].raw_width = 0;
	isp->sensor_info[cid].raw_height = 0;

	cnt = isp_get_started_stream_count(isp);
	if (cnt > 0) {
		isp_dbg_print_info
			("in %s, no need power off isp\n", __func__);
		goto suc;
	};

	isp_dbg_print_info("in %s,off privacy led\n", __func__);

	isp_hw_reg_write32(mmISP_GPIO_PRIV_LED, 0);

	isp_dbg_print_info("in %s, power off isp\n", __func__);

	/*power off ccpu */
	/*(1) Stall CCPU */
	reg_val = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	reg_val |= ISP_CCPU_CNTL__CCPU_STALL_MASK;
	isp_hw_reg_write32(mmISP_CCPU_CNTL, reg_val);

	/*(2) Delay: Wait */
	usleep_range(1000, 2000);

	/*(3) Hold CCPU Reset */
	reg_val = isp_hw_reg_read32(mmISP_SOFT_RESET);
	reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	isp_hw_reg_write32(mmISP_SOFT_RESET, reg_val);
	//ISP_SET_STATUS(isp, ISP_STATUS_FW_RUNNING);

	isp_dphy_pwr_off(isp);
	isp_ip_pwr_off(isp);

	if (sif->calib_gpu_mem) {
		isp_gpu_mem_free(sif->calib_gpu_mem);
		sif->calib_gpu_mem = NULL;
	};

	do {
		ele = isp_rm_cmd_from_cmdq_by_stream(isp,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				false);
		if (ele == NULL)
			break;
		if (ele->mc_addr)
			isp_fw_ret_pl(isp->fw_work_buf_hdl, ele->mc_addr);

		if (ele->gpu_pkg)
			isp_gpu_mem_free(ele->gpu_pkg);

		isp_sys_mem_free(ele);
	} while (ele != NULL);

	ISP_SET_STATUS(isp, ISP_STATUS_PWR_OFF);

	//Put ISP DS4 bit to idle
	isp_program_mmhub_ds_reg(FALSE);

suc:
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_err("<- %s, fail\n", __func__);
	return IMF_RET_FAIL;
};

int32 start_stream_imp(pvoid context, enum camera_id cam_id,
			enum stream_id str_id, bool is_perframe_ctl)
{
	struct isp_context *isp = context;
	/*enum camera_id cam_id; */
	result_t ret;
	enum pvt_img_fmt fmt;

	/*struct sensor_info *sensor_info; */
	int32 ret_val = IMF_RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d,str:%d\n",
			__func__, context, cam_id, str_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (str_id < STREAM_ID_RGBIR) {
		fmt = isp->sensor_info[cam_id].str_info[str_id].format;
		if ((fmt == PVT_IMG_FMT_INVALID) || (fmt >= PVT_IMG_FMT_MAX)) {
			isp_dbg_print_err
				("-><- %s fail,cid:%d,str:%d,fmt not set\n",
				__func__, cam_id, str_id);
			return IMF_RET_FAIL;
		};
	} else if (!isp_is_host_processing_raw(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail bad para,none RGBIR sensor\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_mutex_unlock(&isp->ops_mutex);
		isp_dbg_print_err
			("-><- %s(cid:%d,str:%d) fail, bad fsm %d\n",
			__func__, cam_id, str_id, ISP_GET_STATUS(isp));
		return IMF_RET_FAIL;
	};

	isp_dbg_print_info("-> %s,cid:%d,sid:%d\n", __func__, cam_id, str_id);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_SS,
			PT_STATUS_SUCC);
	ret = isp_start_stream(isp, cam_id, str_id, is_perframe_ctl);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_SS,
			PT_STATUS_SUCC);

	if (ret != RET_SUCCESS)
		ret_val = IMF_RET_FAIL;
	else
		ret_val = IMF_RET_SUCCESS;

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret_val != IMF_RET_SUCCESS) {
		stop_stream_imp(isp, cam_id, str_id, false);
		isp_dbg_print_err("<- %s fail\n", __func__);
	} else
		isp_dbg_print_info("<- %s suc\n", __func__);

	return ret_val;
};

int32 start_preview_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;
	enum stream_id sid = STREAM_ID_PREVIEW;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SP,
			PT_STATUS_SUCC);
#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	ret = start_stream_imp(context, cam_id, sid, false);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_SP,
			PT_STATUS_SUCC);
	return ret;
}


int32 stop_stream_imp(pvoid context, enum camera_id cid, enum stream_id sid,
	bool pause)
{
	struct isp_context *isp = context;
	struct isp_stream_info *sif = NULL;
	int32 ret_val = IMF_RET_SUCCESS;
	uint32 cmd;
	uint32 out_cnt;
	uint32 timeout;
//	CmdStopStream_t stop_stream_para;
	enum fw_cmd_resp_stream_id stream_id;
	struct isp_mapped_buf_info *cur = NULL;
//	isp_ret_status_t result;
	char *pt_cmd_str = "";

	if (sid == STREAM_ID_PREVIEW)
		pt_cmd_str = PT_CMD_EP;
	else if (sid == STREAM_ID_VIDEO)
		pt_cmd_str = PT_CMD_EV;
	else if (sid == STREAM_ID_ZSL)
		pt_cmd_str = PT_CMD_EZ;
	else if (sid == STREAM_ID_RGBIR)
		pt_cmd_str = PT_CMD_ER;

	if (!is_para_legal(context, cid) || (sid > STREAM_ID_NUM)) {
		isp_dbg_print_err
			("-><- %s fail,bad para,isp:%p,cid:%d,sid:%d\n",
			__func__, context, cid, sid);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (((sid == STREAM_ID_RGBIR) || (sid == STREAM_ID_RGBIR_IR))
	&& !isp_is_host_processing_raw(isp, cid)) {
		isp_dbg_print_err
			("-><- %s fail bad para,none RGBIR sensor\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);

	isp_dbg_print_info("-> %s,cid:%d,str:%d\n", __func__, cid, sid);

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (sid < STREAM_ID_RGBIR) {
		sif = &isp->sensor_info[cid].str_info[sid];
	} else {
		ret_val = IMF_RET_SUCCESS;
		goto goon;
	}

	if (sif && (sif->start_status == START_STATUS_NOT_START))
		goto goon;

	switch (sid) {
	case STREAM_ID_PREVIEW:
		cmd = CMD_ID_DISABLE_PREVIEW;
		break;
	case STREAM_ID_VIDEO:
		cmd = CMD_ID_DISABLE_VIDEO;
		break;
	case STREAM_ID_ZSL:
		cmd = CMD_ID_DISABLE_ZSL;
		break;
	case STREAM_ID_RGBIR_IR:
		goto rgbir_ir_stop;
	default:
		isp_dbg_print_info("In %s,never here\n", __func__);
		ret_val = IMF_RET_FAIL;
		goto quit;
	};

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, pt_cmd_str,
			PT_STATUS_SUCC);

	if (sif->start_status == START_STATUS_STARTED) {
		if (!sif->is_perframe_ctl) {
			cur = (struct isp_mapped_buf_info *)
				isp_list_get_first_without_rm(&sif->buf_in_fw);

			timeout = (g_isp_env_setting == ISP_ENV_SILICON) ?
						(1000 * 2) : (1000 * 10);

#ifdef DO_SYNCHRONIZED_STOP_STREAM
			if (isp_send_fw_cmd_sync(isp, cmd,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0,
					timeout,
					NULL, NULL) != RET_SUCCESS) {
#else  //DO_SYNCHRONIZED_STOP_STREAM
			if (isp_send_fw_cmd(isp, cmd,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
#endif

				isp_dbg_print_err
					("in %s,send disable str fail\n",
					__func__);
			} else {
				isp_dbg_print_info
					("in %s wait disable suc\n", __func__);
			};
		};
	};

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, pt_cmd_str,
			PT_STATUS_SUCC);
rgbir_ir_stop:
	if (0) {
//wait for current buffers done for fw's disable bug in which
//fw will return immediately after
//receiving disable command without waiting current frame to finish
		uint32 i;
		struct isp_mapped_buf_info *img_info;

		for (i = 0; i < 7; i++) {
			img_info = (struct isp_mapped_buf_info *)
				isp_list_get_first_without_rm(&sif->buf_in_fw);
			if (img_info == NULL)
				break;
			if (cur != img_info)
				break;
			isp_dbg_print_info(
				"wait stream buf done for the %ith\n", i + 1);
			usleep_range(5000, 6000);
		}
	}
	isp_take_back_str_buf(isp, sif, cid, sid);
	sif->start_status = START_STATUS_NOT_START;
	ret_val = IMF_RET_SUCCESS;
	if (sid != STREAM_ID_RGBIR)
		goto quit;
goon:

	isp_reset_str_info(isp, cid, sid, pause);
	isp_get_stream_output_bits(isp, cid, &out_cnt);
	if (isp_is_host_processing_raw(isp, cid)
	&& (sid != STREAM_ID_RGBIR)) {
		ret_val = IMF_RET_SUCCESS;
		isp_dbg_print_err
			("in %s,not stop for none RGBIR stream\n", __func__);
		goto quit;
	};

	if (out_cnt > 0) {
		ret_val = IMF_RET_SUCCESS;
		goto quit;
	}

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_ES,
			PT_STATUS_SUCC);

	isp_uninit_stream(isp, cid);

quit:

	if (ret_val != IMF_RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail\n", __func__);
	} else {
		isp_dbg_print_info
			("<- %s suc\n", __func__);
	}

	isp_mutex_unlock(&isp->ops_mutex);
	/*
	 *isp_mutex_unlock(&isp->sensor_info[cam_id].stream_op_mutex);
	 *isp_mutex_unlock(&isp->sensor_info[cam_id].snr_mutex);
	 */
	return ret_val;
}

/*refer to aidt_api_stop_streaming*/
int32 stop_preview_imp(pvoid context, enum camera_id cam_id, bool pause)
{
	int32 ret;
	enum stream_id sid = STREAM_ID_PREVIEW;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_EP,
			PT_STATUS_SUCC);
#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	ret = stop_stream_imp(context, cam_id, sid, pause);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_EP,
			PT_STATUS_SUCC);
	return ret;
}

int32 set_stream_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl, enum stream_id str_id)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	struct isp_picture_buffer_param buf_para;
	//uint32 pipe_id = 0;
	result_t result;
	int32 ret = IMF_RET_FAIL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;
	struct isp_stream_info *str_info;

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (buf_hdl->virtual_addr == 0)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d,sid:%d\n",
			__func__, context, cam_id, str_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!isp_get_pic_buf_param
	(&isp->sensor_info[cam_id].str_info[str_id], &buf_para)) {
		isp_dbg_print_err
			("-><- %s fail for get para\n", __func__);
		return IMF_RET_FAIL;
	}

	y_len = buf_para.channel_a_height * buf_para.channel_a_stride;
	u_len = buf_para.channel_b_height * buf_para.channel_b_stride;
	v_len = buf_para.channel_c_height * buf_para.channel_c_stride;

	if (buf_hdl->len < (y_len + u_len + v_len)) {
		isp_dbg_print_err
			("-><- %s fail for small buf,%u need,real %u\n",
			__func__, y_len + u_len + v_len, buf_hdl->len);
		return IMF_RET_FAIL;
	} else if (buf_hdl->len > (y_len + u_len + v_len)) {
		/*isp_dbg_print_info
		 *	("in %s, in buf len(%u)>y+u+v(%u)\n",
		 *	__func__, buf_hdl->len, (y_len + u_len + v_len));
		 */
		if (v_len != 0)
			v_len = buf_hdl->len - y_len - u_len;
		else if (u_len != 0)
			u_len = buf_hdl->len - y_len;
		else
			y_len = buf_hdl->len;
	}

#ifdef USING_WIRTECOMBINE_BUFFER_FOR_STAGE2
	{
		uint32 tempi;
		uint32 size = 1920 * 1080 * 3 / 2;

		for (tempi = 0; tempi < DBG_STAGE2_WB_BUF_CNT; tempi++) {
			if (g_dbg_stage2_buf[tempi])
				continue;

			g_dbg_stage2_buf[tempi] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}
	}
#endif

	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("-> %s,cid %d,sid:%d,%p(%u)\n", __func__,
		cam_id, str_id, buf_hdl->virtual_addr, buf_hdl->len);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_info("<- %s,fail fsm %d\n", __func__,
			ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		isp_dbg_print_err
			("<- %s fail for sys_img_buf_handle_cpy\n", __func__);
		goto quit;
	}

	gen_img = isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
				cam_id, str_id, y_len, u_len, v_len);

	/*isp_dbg_show_map_info(gen_img); */
	if (gen_img == NULL) {
		isp_dbg_print_err
			("<- %s fail for isp_map_sys_2_mc\n", __func__);
		ret = IMF_RET_FAIL;
		goto quit;
	}

	/*isp_dbg_print_info
	 *	("in %s,y:u:v %u:%u:%u,addr:%p(%u) map to %llx\n", __func__
	 *	y_len, u_len, v_len, buf_hdl->virtual_addr, buf_hdl->len,
	 *	gen_img->y_map_info.mc_addr);
	 */

	str_info = &isp->sensor_info[cam_id].str_info[str_id];
	if (str_info->format == PVT_IMG_FMT_L8) {
		if (str_info->uv_tmp_buf
			&& (str_info->uv_tmp_buf->mem_size < y_len / 2)) {

			isp_gpu_mem_free(str_info->uv_tmp_buf);
			str_info->uv_tmp_buf = NULL;
		}
		if (str_info->uv_tmp_buf == NULL) {
			str_info->uv_tmp_buf =
				isp_gpu_mem_alloc(y_len / 2,
				ISP_GPU_MEM_TYPE_NLFB);
		}

		if (str_info->uv_tmp_buf) {
			gen_img->u_map_info.len =
				(uint32) str_info->uv_tmp_buf->mem_size;
			gen_img->u_map_info.mc_addr =
				str_info->uv_tmp_buf->gpu_mc_addr;
			gen_img->u_map_info.sys_addr =
				(uint64) str_info->uv_tmp_buf->sys_addr;
		} else {
			isp_dbg_print_err("%s fail alloc uv_tmp_buf for L8\n",
				__func__);
			result = RET_FAILURE;
			goto quit;
		}
	}

	result = fw_if_send_img_buf(isp, gen_img, cam_id, str_id);
	if (str_info->format == PVT_IMG_FMT_L8) {
		gen_img->u_map_info.len = 0;
		gen_img->u_map_info.mc_addr = 0;
		gen_img->u_map_info.sys_addr = 0;
	}

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for fw_if_send_img_buf\n", __func__);
		goto quit;
	}

	isp->sensor_info[cam_id].str_info[str_id].buf_num_sent++;
	isp_list_insert_tail(
		&isp->sensor_info[cam_id].str_info[str_id].buf_in_fw,
		(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
quit:

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (buf_hdl)
			sys_img_buf_handle_free(buf_hdl);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
	}

	return ret;
};

int32 set_preview_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl)
{
	int32 ret;
	enum stream_id sid = STREAM_ID_PREVIEW;

	/*isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
	 *	PT_CMD_PB, PT_STATUS_SUCC,
	 *	buf_hdl ? buf_hdl->virtual_addr : 0);
	 */

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	ret = set_stream_buf_imp(context, cam_id, buf_hdl, sid);
	/*isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
	 *	PT_CMD_PB, PT_STATUS_SUCC,
	 *	buf_hdl ? buf_hdl->virtual_addr : 0);
	 */

	return ret;
};

int32 set_video_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl)
{
	int32 ret;

	/*isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
	 *		PT_CMD_VB, PT_STATUS_SUCC,
	 *		buf_hdl ? buf_hdl->virtual_addr : 0);
	 */

	ret = set_stream_buf_imp(context, cam_id, buf_hdl, STREAM_ID_VIDEO);
	/*isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
	 *		PT_CMD_VB, PT_STATUS_SUCC,
	 *		buf_hdl ? buf_hdl->virtual_addr : 0);
	 */

	return ret;
};

int32 start_video_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SV,
			PT_STATUS_SUCC);
	ret = start_stream_imp(context, cam_id, STREAM_ID_VIDEO, false);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_SV,
			PT_STATUS_SUCC);

	return ret;
};

int32 start_rgbir_ir_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;

//	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n",
//		PT_EVT_RC, PT_CMD_SV, PT_STATUS_SUCC);
	ret = start_stream_imp(context, cam_id, STREAM_ID_RGBIR_IR, false);
//	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n",
//		PT_EVT_SCD, PT_CMD_SV, PT_STATUS_SUCC);

	return ret;
};

/*refer to aidt_api_stop_streaming*/
int32 stop_video_imp(pvoid context, enum camera_id cam_id, bool pause)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_EV,
			PT_STATUS_SUCC);
	ret = stop_stream_imp(context, cam_id, STREAM_ID_VIDEO, pause);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_EV,
			PT_STATUS_SUCC);

	return ret;
}

/*refer to aidt_api_stop_streaming*/
int32 stop_rgbir_ir_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;

//	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n",
//		PT_EVT_RC, PT_CMD_EV, PT_STATUS_SUCC);
	ret = stop_stream_imp(context, cam_id, STREAM_ID_RGBIR_IR, false);
//	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n",
//		PT_EVT_SCD, PT_CMD_EV, PT_STATUS_SUCC);

	return ret;
}

int32 start_rgbir_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SV,
			PT_STATUS_SUCC);
	ret = start_stream_imp(context, cam_id, STREAM_ID_RGBIR, false);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_SV,
			PT_STATUS_SUCC);

	return ret;
}

int32 stop_rgbir_imp(pvoid context, enum camera_id cam_id)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_EV,
			PT_STATUS_SUCC);
	ret = stop_stream_imp(context, cam_id, STREAM_ID_RGBIR, false);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_EV,
			PT_STATUS_SUCC);

	return ret;
}

int32 set_rgbir_output_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl,
			uint32 bayer_raw_len, uint32 ir_raw_len,
			uint32 frame_info_len)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
//	struct isp_picture_buffer_param buf_para;
//	uint32 pipe_id = 0;
	result_t result;
	int32 ret = IMF_RET_FAIL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;
//	struct isp_stream_info *str_info;

	/*isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
	 *		PT_CMD_RB, PT_STATUS_SUCC,
	 *		buf_hdl ? buf_hdl->virtual_addr : 0);
	 */

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (buf_hdl->virtual_addr == 0)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!isp_is_host_processing_raw(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s,cid:%d fail not rgbir sensor(%u)\n",
			__func__,
			cam_id, isp->sensor_info[cam_id].cam_type);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (buf_hdl->len < (bayer_raw_len + ir_raw_len + frame_info_len)) {
		isp_dbg_print_err
			("-><- %s,cid:%d fail buf len %u(%u)\n",
			__func__, cam_id, buf_hdl->len,
			(bayer_raw_len + ir_raw_len + frame_info_len));
		return IMF_RET_INVALID_PARAMETER;
	};

	if ((bayer_raw_len == 0) || (ir_raw_len == 0)
	|| (frame_info_len == 0)) {
		isp_dbg_print_err
		("-><- %s,cid:%d fail buflen %u len %u,%u,%u\n", __func__,
			cam_id, buf_hdl->len,
			bayer_raw_len, ir_raw_len, frame_info_len);
		return IMF_RET_INVALID_PARAMETER;
	}

	y_len = bayer_raw_len;
	u_len = ir_raw_len;
	v_len = buf_hdl->len - bayer_raw_len - ir_raw_len;

	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("-> %s,cid %d,raw cnt %u\n",
		__func__, cam_id, ++g_rgbir_raw_count_in_fw);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_info("<- %s,fail fsm %d\n",
			__func__, ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		isp_dbg_print_err
			("<- %s fail for sys_img_buf_handle_cpy\n", __func__);
		goto quit;
	};

//	todo to be deleted later
//	memset(buf_hdl->virtual_addr, 0x80, buf_hdl->len);

	if (0) {
		uint8 *tmp_p;
		uint32 *y, *u, *v;
//		uint64 ymc, umc, vmc;
		tmp_p = buf_hdl->virtual_addr;
		y = (uint32 *) tmp_p;
		y[0] = 0x11335577;
		y[1] = 0x99bbddff;
//		ymc = gen_img->y_map_info.mc_addr;
		u = (uint32 *) (tmp_p + y_len);
		u[0] = 0x00224466;
		u[1] = 0x88aaccee;
//		umc = gen_img->u_map_info.mc_addr;
		v = (uint32 *) (tmp_p + y_len + u_len);
		v[0] = 0x55aa55aa;
		v[1] = 0x01234567;
//		vmc = gen_img->v_map_info.mc_addr;

	}

	gen_img = isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
				cam_id, STREAM_ID_RGBIR, y_len, u_len,
				v_len);

//	isp_dbg_show_map_info(gen_img);
	if (gen_img == NULL) {
		isp_dbg_print_err
			("<- %s fail for isp_map_sys_2_mc\n", __func__);
		ret = IMF_RET_FAIL;
		goto quit;
	}

	isp_dbg_print_info
		("in %s,y:u:v %u:%u:%u,addr:%p(%u) map to %llx\n", __func__,
		y_len, u_len, v_len, buf_hdl->virtual_addr, buf_hdl->len,
		gen_img->y_map_info.mc_addr);
	if (0) {
		uint8 *tmp_p;
		uint32 *y, *u, *v;
		uint64 ymc, umc, vmc;

		tmp_p = buf_hdl->virtual_addr;

		y = (uint32 *) gen_img->y_map_info.sys_addr;
		u = (uint32 *) gen_img->u_map_info.sys_addr;
		v = (uint32 *) gen_img->v_map_info.sys_addr;

		ymc = gen_img->y_map_info.mc_addr;
		umc = gen_img->u_map_info.mc_addr;
		vmc = gen_img->v_map_info.mc_addr;
		v[0] = 0x12304567;
		v[1] = 0x89abcdef;
		isp_dbg_print_info
("rgbir:outIn %p(%llx),0x%x 0x%x %p(%llx),0x%x 0x%x %p(%llx),0x%x 0x%x\n",
		y, ymc, y[0], y[1], u, umc, u[0], u[1], v, vmc, v[0], v[1]);

	}

	result = isp_send_rgbir_raw_info_buf(isp, gen_img, cam_id);

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for fw_if_send_img_buf\n", __func__);
		goto quit;
	};
	isp_list_insert_tail(&isp->sensor_info[cam_id].rgbir_raw_output_in_fw,
				(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
 quit:

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (buf_hdl)
			sys_img_buf_handle_free(buf_hdl);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
	};
//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
//			PT_CMD_VB, PT_STATUS_SUCC,
//			buf_hdl ? buf_hdl->virtual_addr : 0);
	return ret;
}

int32 set_rgbir_input_buf_imp(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw_buf,
				sys_img_buf_handle_t frame_info)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	sys_img_buf_handle_t raw_temp = NULL;
	sys_img_buf_handle_t frame_temp = NULL;
	struct isp_context *isp = context;
	CmdSendMode3FrameInfo_t *frame_info_cmd;
	enum fw_cmd_resp_stream_id stream;
	result_t result;
	int32 ret = IMF_RET_FAIL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;

//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
//			PT_CMD_RB, PT_STATUS_SUCC,
//			bayer_raw_buf ? bayer_raw_buf->virtual_addr : 0);

	if (!is_para_legal(context, cam_id)
	|| (bayer_raw_buf == NULL)
	|| (bayer_raw_buf->virtual_addr == 0)
	|| (frame_info == NULL)
	|| (frame_info->virtual_addr == 0)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d\n", __func__,
			context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!isp_is_host_processing_raw(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s,cid:%d fail no rgbir sensor\n", __func__,
			cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_mutex_lock(&isp->ops_mutex);

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_err
			("-><- %s,cid %d,fail fsm %d\n", __func__,
			cam_id, ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}
	raw_temp = sys_img_buf_handle_cpy(bayer_raw_buf);
	if (raw_temp == NULL) {
		isp_dbg_print_err
			("-><- %s,cid %d fail sys_img_buf_handle_cpy\n",
			__func__, cam_id);
		goto quit;
	};

	frame_temp = sys_img_buf_handle_cpy(frame_info);
	if (frame_temp == NULL) {
		isp_dbg_print_err
			("-><- %s,cid %d fail sys_img_buf_handle_cpy1\n",
			__func__, cam_id);
		goto quit;
	};

	frame_info_cmd = (CmdSendMode3FrameInfo_t *) frame_temp->virtual_addr;
	isp_dbg_print_info("-> %s,cid %d,poc %u\n", __func__,
		cam_id, frame_info_cmd->frameInfo.baseMode3FrameInfo.poc);

	y_len = raw_temp->len;
	u_len = 0;
	v_len = 0;
	gen_img = isp_map_sys_2_mc(isp, raw_temp, ISP_MC_ADDR_ALIGN,
				cam_id, STREAM_ID_RGBIR, y_len, u_len,
				v_len);

//	isp_dbg_show_map_info(gen_img);
	if (gen_img == NULL) {
		isp_dbg_print_err
			("<- %s fail for map raw\n", __func__);
		goto quit;
	}

	isp_dbg_print_info
	("in %s,poc %u,cfa %u,y:u:v %u:%u:%u,addr:%p(%u) map to %llx(%u)\n",
		__func__,
		frame_info_cmd->frameInfo.baseMode3FrameInfo.poc,
		frame_info_cmd->frameInfo.baseMode3FrameInfo.cfaPattern,
		y_len, u_len, v_len, raw_temp->virtual_addr, raw_temp->len,
		gen_img->y_map_info.mc_addr, gen_img->y_map_info.len);

	frame_info_cmd->frameInfo.baseMode3FrameInfo.rawBufferFrameInfo.source =
		BUFFER_SOURCE_HOST_POST;
	frame_info_cmd->frameInfo.baseMode3FrameInfo.rawBufferFrameInfo.buffer.bufSizeA =
		gen_img->y_map_info.len;
	frame_info_cmd->frameInfo.baseMode3FrameInfo.rawBufferFrameInfo.buffer.vmidSpace = 4;
	isp_split_addr64(gen_img->y_map_info.mc_addr,
		&frame_info_cmd->frameInfo.baseMode3FrameInfo.rawBufferFrameInfo.buffer.bufBaseALo,
		&frame_info_cmd->frameInfo.baseMode3FrameInfo.rawBufferFrameInfo.buffer.bufBaseAHi);

	stream = isp_get_stream_id_from_cid(isp, cam_id);

	result = isp_send_fw_cmd(isp, CMD_ID_SEND_MODE3_FRAME_INFO,
				stream,
				FW_CMD_PARA_TYPE_INDIRECT, frame_info_cmd,
				sizeof(CmdSendMode3FrameInfo_t));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for send frame info\n", __func__);
		goto quit;
	};
	isp_list_insert_tail(&isp->sensor_info[cam_id].rgbraw_input_in_fw,
				(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
quit:

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (raw_temp)
			sys_img_buf_handle_free(raw_temp);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
		if (frame_temp)
			sys_img_buf_handle_free(frame_temp);
	};
//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
//			PT_CMD_VB, PT_STATUS_SUCC,
//			raw_temp ? raw_temp->virtual_addr : 0);
	return ret;
}

int32 set_mem_cam_input_buf_imp(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw_buf,
				sys_img_buf_handle_t frame_info)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	sys_img_buf_handle_t raw_temp = NULL;
	sys_img_buf_handle_t frame_temp = NULL;
	struct isp_context *isp = context;
	CmdSendMode4FrameInfo_t *frame_info_cmd;
	enum fw_cmd_resp_stream_id stream;
	result_t result;
	int32 ret = IMF_RET_FAIL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;

//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
//			PT_CMD_RB, PT_STATUS_SUCC,
//			bayer_raw_buf ? bayer_raw_buf->virtual_addr : 0);

	if (!is_para_legal(context, cam_id)
	|| (bayer_raw_buf == NULL)
	|| (bayer_raw_buf->virtual_addr == 0)
	|| (frame_info == NULL)
	|| (frame_info->virtual_addr == 0)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d\n", __func__,
			context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if ((cam_id != CAMERA_ID_MEM)
	&& (isp->sensor_info[cam_id].cam_type != CAMERA_TYPE_MEM)
	&& (isp->sensor_info[cam_id].cam_type != CAMERA_TYPE_RGBIR)) {
		isp_dbg_print_err
			("-><- %s,cid:%d fail not mem sensor\n",
			__func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_err("-><- %s,fail fsm %d\n", __func__,
			ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	raw_temp = sys_img_buf_handle_cpy(bayer_raw_buf);
	if (raw_temp == NULL) {
		isp_dbg_print_err
			("-><- %s fail sys_img_buf_handle_cpy\n", __func__);
		goto quit;
	};

	frame_temp = sys_img_buf_handle_cpy(frame_info);
	if (frame_temp == NULL) {
		isp_dbg_print_err
			("-><- %s fail sys_img_buf_handle_cpy1\n", __func__);
		goto quit;
	};

	frame_info_cmd = (CmdSendMode4FrameInfo_t *) frame_temp->virtual_addr;
	isp_dbg_print_info("-> %s,cid %d,poc %d\n", __func__,
			cam_id, frame_info_cmd->frameInfo.poc);

	if (isp->sensor_info[cam_id].cam_type == CAMERA_TYPE_RGBIR) {
		isp_dbg_print_info
			("in %s,change cid from %u to %u\n", __func__,
			cam_id, CAMERA_ID_MEM);
		cam_id = CAMERA_ID_MEM;
	}

	y_len = raw_temp->len;
	u_len = 0;
	v_len = 0;
	gen_img = isp_map_sys_2_mc(isp, raw_temp, ISP_MC_ADDR_ALIGN,
				cam_id, STREAM_ID_RGBIR, y_len, u_len,
				v_len);

//	isp_dbg_show_map_info(gen_img);
	if (gen_img == NULL) {
		isp_dbg_print_err
			("<- %s fail for map raw\n", __func__);
		goto quit;
	}
//	isp_dbg_print_info
//		("in %s,y:u:v %u:%u:%u,addr:%p(%u) map to %llx(%u)\n",
//		__func__,
//		y_len, u_len, v_len, raw_temp->virtual_addr, raw_temp->len,
//		gen_img->y_map_info.mc_addr, gen_img->y_map_info.len);

	frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufSizeA =
		gen_img->y_map_info.len;
	frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.vmidSpace = 4;

	isp_split_addr64(gen_img->y_map_info.mc_addr,
	&frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufBaseALo,
	&frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufBaseAHi);

	/*because lsc profile will take effect after one frame,
	 *according logic in fw,driver
	 *sets the opposite value as workaround
	 */
	if (isp->drv_settings.ir_illuminator_ctrl == 1) {
		frame_info_cmd->frameInfo.iRIlluStatus = IR_ILLU_STATUS_OFF;
		isp_dbg_print_info
			("in %s change ir illu to on for always on\n",
			__func__);
	} else if (isp->drv_settings.ir_illuminator_ctrl == 0) {
		frame_info_cmd->frameInfo.iRIlluStatus = IR_ILLU_STATUS_ON;
		isp_dbg_print_info
			("in %s change ir illu to off for always off\n",
			__func__);
	} else {
		char *ill_status_str;

		switch (frame_info_cmd->frameInfo.iRIlluStatus) {
		case IR_ILLU_STATUS_UNKNOWN:
			ill_status_str = "unknown"; break;
		case IR_ILLU_STATUS_ON:
			ill_status_str = "on"; break;
		case IR_ILLU_STATUS_OFF:
			ill_status_str = "off"; break;
		default:
			ill_status_str = "err"; break;
		}

		/*isp_dbg_print_verb
		 *	("in %s illu %s,poc %u\n", __func__,
		 *	ill_status_str,
		 *	frame_info_cmd->frameInfo.poc);
		 */
	};
	stream = isp_get_stream_id_from_cid(isp, cam_id);
	result = isp_send_fw_cmd(isp, CMD_ID_SEND_MODE4_FRAME_INFO,
				stream,
				FW_CMD_PARA_TYPE_INDIRECT, frame_info_cmd,
				sizeof(CmdSendMode4FrameInfo_t));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for send frame info\n", __func__);
		goto quit;
	};
	isp_list_insert_tail(&isp->sensor_info[cam_id].irraw_input_in_fw,
				(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
quit:

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (raw_temp)
			sys_img_buf_handle_free(raw_temp);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
		if (frame_temp)
			sys_img_buf_handle_free(frame_temp);
	};

	return ret;
}

int32 get_original_wnd_size_imp(pvoid context, enum camera_id cam_id,
				uint32 *width, uint32 *height)
{
	struct isp_context *isp = context;
	uint32 w, h;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	w = isp->sensor_info[cam_id].asprw_wind.h_size;
	h = isp->sensor_info[cam_id].asprw_wind.v_size;
	if ((w == 0) || (h == 0)) {
		isp_dbg_print_err
			("-><- %s fail bad asprw %u:%u\n", __func__, w, h);
		return IMF_RET_FAIL;
	};
	if (width)
		*width = w;
	if (height)
		*height = h;
	isp_dbg_print_info
		("-><- %s suc w:%u h:%u\n", __func__, w, h);

	return IMF_RET_SUCCESS;
};

void reg_notify_cb_imp(pvoid context, enum camera_id cam_id,
			func_isp_module_cb cb, pvoid cb_context)
{
	struct isp_context *isp = context;
//	struct camera_dev_info cam_dev_info;

	if (!is_para_legal(context, cam_id) || (cb == NULL)) {
		isp_dbg_print_err
			("-><- %s cid %u fail for bad para\n",
			__func__, cam_id);
		return;
	};
	isp->evt_cb[cam_id] = cb;
	isp->evt_cb_context[cam_id] = cb_context;
	isp_dbg_print_info("-><- %s cid %u suc\n", __func__, cam_id);
};

void unreg_notify_cb_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	isp_dbg_print_info("-> %s cid %u\n", __func__, cam_id);
	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s cid %u fail for bad para\n",
			__func__, cam_id);
		return;
	};
	isp->evt_cb[cam_id] = NULL;
	isp->evt_cb_context[cam_id] = NULL;
	isp_dbg_print_info("-><- %s cid %u suc\n", __func__, cam_id);
};

int32 i2c_read_mem_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size)
{
	result_t result;

	unreferenced_parameter(context);

	result =
		i2_c_read_mem(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			p_read_buffer, byte_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
};

int32 i2c_write_mem_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size)
{
	result_t result;

	unreferenced_parameter(context);

	result = i2_c_write_mem(bus_num, slave_addr, slave_addr_mode,
				reg_addr, reg_addr_size, p_write_buffer,
				byte_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
};

int32 i2c_read_reg_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value, uint8 reg_size)
{
	result_t result;

	unreferenced_parameter(context);

	result =
		i2c_read_reg(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			preg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
};

int32 i2c_read_reg_fw_imp(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value, uint8 reg_size)
{
	result_t result;

	result =
		i2c_read_reg_fw(context, id, bus_num, slave_addr,
			slave_addr_mode, enable_restart, reg_addr,
			reg_addr_size, preg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

int32 i2c_write_reg_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint8 reg_addr_size,
			uint32 reg_value, uint8 reg_size)
{
	result_t result;

	unreferenced_parameter(context);
	result = i2c_write_reg(bus_num, slave_addr, slave_addr_mode,
			reg_addr, reg_addr_size, reg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
};

int32 i2c_write_reg_fw_imp(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint8 reg_addr_size,
			uint32 reg_value, uint8 reg_size)
{
	result_t result;

	result =
		i2c_write_reg_fw(context, id, bus_num, slave_addr,
			slave_addr_mode, reg_addr, reg_addr_size,
			reg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
};

void reg_snr_op_imp(pvoid context, enum camera_id cam_id,
			struct sensor_operation_t *ops)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		isp_dbg_print_err
		("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("-><- %s, cid %d, ops:%p\n", __func__, cam_id, ops);

	isp->p_sensor_ops[cam_id] = ops;

	if (!isp->snr_res_fps_info_got[cam_id]) {
		struct sensor_res_fps_t *res_fps;

		res_fps = isp->p_sensor_ops[cam_id]->get_res_fps(cam_id);
		if (res_fps) {
			uint32 i;

			memcpy(&isp->sensor_info[cam_id].res_fps, res_fps,
				   sizeof(struct sensor_res_fps_t));
			isp_dbg_print_info
				("in %s,res_fps info for cam_id:%d,count:%d\n",
				__func__, cam_id, res_fps->count);
			for (i = 0; i < res_fps->count; i++) {
				isp_dbg_print_info
					("idx:%d,fps:%d,w:%d,h:%d\n", i,
					res_fps->res_fps[i].fps,
					res_fps->res_fps[i].width,
					res_fps->res_fps[i].height);
			}
		} else {
			isp_dbg_print_err
				("in %s,cam_id:%d, no fps\n",
				__func__, cam_id);
		}

		isp->snr_res_fps_info_got[cam_id] = 1;
	}
};

void unreg_snr_op_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("-><- %s, cid %d\n", __func__, cam_id);
	isp->p_sensor_ops[cam_id] = NULL;
};

void reg_vcm_op_imp(pvoid context, enum camera_id cam_id,
			struct vcm_operation_t *ops)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info
		("-><- %s suc, cid %d, ops:%p\n", __func__, cam_id, ops);

	isp->p_vcm_ops[cam_id] = ops;
};

void unreg_vcm_op_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("-><- %s suc, cid %d\n", __func__, cam_id);

	isp->p_vcm_ops[cam_id] = NULL;
};

void reg_flash_op_imp(pvoid context, enum camera_id cam_id,
			struct flash_operation_t *ops)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("-><- %s, cid %d, ops:%p\n", __func__, cam_id, ops);

	isp->p_flash_ops[cam_id] = ops;
};

void unreg_flash_op_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("-><- %s suc, cid %d\n", __func__, cam_id);

	isp->p_flash_ops[cam_id] = NULL;
};

int32 take_one_pic_imp(pvoid context, enum camera_id cam_id,
			struct take_one_pic_para *para,
			sys_img_buf_handle_t in_buf)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	sys_img_buf_handle_t buf = NULL;
	enum fw_cmd_resp_stream_id stream_id;
	uint32 len;
	result_t ret;
	CmdCaptureYuv_t cmd_para;
	struct isp_mapped_buf_info *gen_img = NULL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;
	char pt_start_stream = 0;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SCY,
			PT_STATUS_SUCC);
	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	memset(&cmd_para, 0, sizeof(cmd_para));
	if ((para == NULL) || (in_buf == NULL) || !in_buf->len) {
		isp_dbg_print_err
			("-><- %s fail for bad para,para:%p,buf:%p\n",
			__func__, para, in_buf);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (!para->width || !para->height ||
	(para->width > para->luma_pitch)) {
		isp_dbg_print_err
			("-><- %s fail for bad pic para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	len = isp_cal_buf_len_by_para(para->fmt, para->width, para->height,
				para->luma_pitch, para->chroma_pitch,
				&y_len, &u_len, &v_len);
	if ((len == 0) || (in_buf->len < len)) {
		isp_dbg_print_err
			("-><- %s fail for bad pic para or buf\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	} else if (in_buf->len > (y_len + u_len + v_len)) {
		isp_dbg_print_info
			("in %s, in buf len(%u)>y+u+v(%u)\n", __func__,
			in_buf->len, (y_len + u_len + v_len));

		if (v_len != 0)
			v_len = in_buf->len - y_len - u_len;
		else if (u_len != 0)
			u_len = in_buf->len - y_len;
		else
			y_len = in_buf->len;
	};

	sif = &isp->sensor_info[cam_id];

	if ((sif->status == START_STATUS_STARTED) &&
	(sif->str_info[STREAM_ID_ZSL].start_status == START_STATUS_STARTED)) {
		isp_dbg_print_err
			("-><- %s fail for zsl start,cid:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	cmd_para.imageProp.imageFormat = isp_trans_to_fw_img_fmt(para->fmt);
	cmd_para.imageProp.width = para->width;
	cmd_para.imageProp.height = para->height;
	cmd_para.imageProp.lumaPitch = para->luma_pitch;
	cmd_para.imageProp.chromaPitch = para->chroma_pitch;

	if (cmd_para.imageProp.imageFormat == IMAGE_FORMAT_INVALID) {
		isp_dbg_print_err
			("-><- %s fail for bad fmt,cid:%d, fmt %d\n",
			__func__, cam_id, para->fmt);
		return IMF_RET_FAIL;
	};

	buf = sys_img_buf_handle_cpy(in_buf);

	if (buf == NULL) {
		isp_dbg_print_err
			("-><- %s fail cpy buf,cid:%d\n", __func__, cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	gen_img = isp_map_sys_2_mc(isp, buf, ISP_MC_ADDR_ALIGN,
				cam_id, stream_id, y_len, u_len, v_len);
	if (gen_img == NULL) {
		if (buf)
			sys_img_buf_handle_free(buf);
		isp_dbg_print_err
			("-><- %s fail map sys,cid:%d\n", __func__, cam_id);
		return IMF_RET_FAIL;
	}

	cmd_para.usePreCap = 0;
	cmd_para.buffer.vmidSpace = 4;
	isp_split_addr64(gen_img->y_map_info.mc_addr,
			&cmd_para.buffer.bufBaseALo,
			&cmd_para.buffer.bufBaseAHi);
	cmd_para.buffer.bufSizeA = gen_img->y_map_info.len;

	isp_split_addr64(gen_img->u_map_info.mc_addr,
			&cmd_para.buffer.bufBaseBLo,
			&cmd_para.buffer.bufBaseBHi);
	cmd_para.buffer.bufSizeB = gen_img->u_map_info.len;

	isp_split_addr64(gen_img->v_map_info.mc_addr,
			&cmd_para.buffer.bufBaseCLo,
			&cmd_para.buffer.bufBaseCHi);
	cmd_para.buffer.bufSizeC = gen_img->v_map_info.len;

	isp_mutex_lock(&isp->ops_mutex);

	isp_dbg_print_info
		("-> %s, cid:%d, precap %d,focus_first %d\n", __func__,
		cam_id, para->use_precapture, para->focus_first);

	if ((cam_id < CAMERA_ID_MAX)
	&& (isp->sensor_info[cam_id].status == START_STATUS_NOT_START))
		pt_start_stream = 1;

	if (pt_start_stream)
		isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC,
				PT_CMD_SS, PT_STATUS_SUCC);

	if (isp_init_stream(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_init_stream\n", __func__);
		ret = RET_FAILURE;
		goto fail;
	};

	if (isp_setup_3a(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for isp_setup_3a\n", __func__);
		ret = RET_FAILURE;
		goto fail;
	};

	if (isp_kickoff_stream(isp, cam_id, para->width, para->height) !=
	RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_kickoff_stream\n", __func__);
		ret = RET_FAILURE;
		goto fail;
	};
	if (pt_start_stream)
		isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD,
				PT_CMD_SS, PT_STATUS_SUCC);

	if (para->use_precapture) {
		cmd_para.usePreCap = 1;
		ret = isp_send_fw_cmd(isp, CMD_ID_AE_PRECAPTURE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT, NULL,
				0);
		if (ret != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s fail for send AE_PRECAPTURE\n",
				__func__);
			goto fail;
		};
	}

	isp_dbg_show_img_prop("in take one pic imp", &cmd_para.imageProp);
	isp_dbg_print_info("in %s y sys:mc:len %llx:%llx:%u\n", __func__,
			gen_img->y_map_info.sys_addr,
			gen_img->y_map_info.mc_addr,
			gen_img->y_map_info.len);
//	isp_dbg_show_map_info(gen_img);
//	isp_dbg_print_info("Y: 0x%x:%x, %u(should %u)\n",
//		cmd_para.buffer.bufBaseAHi, cmd_para.buffer.bufBaseALo,
//		cmd_para.buffer.bufSizeA, 640 * 480);
//	isp_dbg_print_info("U: 0x%x:%x, %u(should %u)\n",
//		cmd_para.buffer.bufBaseBHi, cmd_para.buffer.bufBaseBLo,
//		cmd_para.buffer.bufSizeB, 640 * 480 / 2);
//	isp_dbg_print_info("v: 0x%x:%x, %u(should 0)\n",
//		cmd_para.buffer.bufBaseCHi, cmd_para.buffer.bufBaseCLo,
//		cmd_para.buffer.bufSizeC);

	if (!para->focus_first) {
		ret = isp_send_fw_cmd(isp, CMD_ID_CAPTURE_YUV,
				stream_id, FW_CMD_PARA_TYPE_INDIRECT,
				&cmd_para, sizeof(cmd_para));
	} else {
		AfMode_t orig_mod = sif->af_mode;

		isp_fw_set_focus_mode(isp, cam_id, AF_MODE_ONE_SHOT);
		isp_fw_start_af(isp, cam_id);
		ret = isp_send_fw_cmd(isp, CMD_ID_CAPTURE_YUV,
				stream_id, FW_CMD_PARA_TYPE_INDIRECT,
				&cmd_para, sizeof(cmd_para));
		isp_fw_set_focus_mode(isp, cam_id, orig_mod);
		isp_fw_start_af(isp, cam_id);
	};

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_SCY,
			PT_STATUS_SUCC);
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for send CAPTURE_YUV\n", __func__);
		goto fail;
	};

	sif->take_one_pic_left_cnt++;
	if (cam_id < CAMERA_ID_MAX)
		isp_list_insert_tail(&
			(isp->sensor_info[cam_id].take_one_pic_buf_list),
			(struct list_node *)gen_img);
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
fail:
	if (gen_img)
		isp_unmap_sys_2_mc(isp, gen_img);
	if (buf)
		sys_img_buf_handle_free(buf);
	isp_mutex_unlock(&isp->ops_mutex);
	return IMF_RET_FAIL;
}

int32 stop_take_one_pic_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	int32 ret_val;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_ECY,
			PT_STATUS_SUCC);

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	sif = &isp->sensor_info[cam_id];

	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_ECY,
			PT_STATUS_SUCC);
	while (sif->take_one_pic_left_cnt) {
		/*
		 *isp_dbg_print_info
		 *	("in %s, front %p,%p\n", __func__,
		 *	isp->take_one_pic_buf_list.front,
		 *	isp->take_one_pic_buf_list.rear);
		 *
		 *if (isp->take_one_pic_buf_list.front ==
		 *isp->take_one_pic_buf_list.rear)
		 *	break;
		 */
		usleep_range(10000, 11000);
	}
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_ECY,
			PT_STATUS_SUCC);
	isp_mutex_lock(&isp->ops_mutex);

	ret_val = IMF_RET_SUCCESS;
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_ES,
			PT_STATUS_SUCC);

	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_ECY,
			PT_STATUS_SUCC);
	isp_dbg_print_info("<- %s, succ\n", __func__);

	return ret_val;
}

void write_isp_reg32_imp(pvoid context, uint64 reg_addr, uint32 value)
{
	unreferenced_parameter(context);
	isp_hw_reg_write32((uint32) reg_addr, value);
};

uint32 read_isp_reg32_imp(pvoid context, uint64 reg_addr)
{
	unreferenced_parameter(context);
	return isp_hw_reg_read32((uint32) reg_addr);
};

int32 set_common_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_id /*enum para_id */,
			void *para_value)
{
	int ret = IMF_RET_SUCCESS;
	bool_t stream_on = 0;
//	enum fw_cmd_resp_stream_id stream_id;
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (isp->sensor_info[cam_id].status == START_STATUS_STARTED)
		stream_on = 1;

	switch (para_id) {
	case PARA_ID_SCENE:
		isp->sensor_info[cam_id].para_scene_mode_set =
			*((enum pvt_scene_mode *)para_value);
		if (isp->sensor_info[cam_id].para_scene_mode_set ==
		isp->sensor_info[cam_id].para_scene_mode_cur) {
			isp_dbg_print_info("not diff value, do none\n");
			break;
		};

		isp->sensor_info[cam_id].para_scene_mode_cur =
			isp->sensor_info[cam_id].para_scene_mode_set;

		if (fw_if_set_scene_mode(isp, cam_id,
		isp->sensor_info[cam_id].para_scene_mode_set) != RET_SUCCESS)
			ret = IMF_RET_FAIL;
		break;
	default:
		isp_dbg_print_info
			("-> %s, camera_id %d,not supported para_id %d\n",
			__func__, cam_id, para_id);
		ret = IMF_RET_INVALID_PARAMETER;
		break;
	}

	if (ret == RET_SUCCESS) {
		isp_dbg_print_info
			("<- %s, success\n", __func__);
	} else {
		isp_dbg_print_err
			("<- %s, fail with %d\n", __func__, ret);
	}

	return ret;
}

int32 get_common_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_id /*enum para_id */,
			void *para_value)
{
	int ret = IMF_RET_SUCCESS;
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		isp_dbg_print_err
			("-><- %s fail for illegal camera:%d para_id %d\n",
			__func__, cam_id, para_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	switch (para_id) {
	case PARA_ID_BRIGHTNESS:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_brightness_cur;
		isp_dbg_print_info
			("-> %s,cid %d,get BRIGHTNESS to %d\n", __func__,
			cam_id, isp->sensor_info[cam_id].para_brightness_cur);
		break;
	case PARA_ID_CONTRAST:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_contrast_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get CONTRAST %d\n", __func__,
			cam_id, isp->sensor_info[cam_id].para_contrast_cur);
		break;
	case PARA_ID_HUE:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_hue_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get HUE %d\n", __func__,
			cam_id, isp->sensor_info[cam_id].para_hue_cur);
		break;
	case PARA_ID_SATURATION:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_satuaration_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get SATURATION %d\n", __func__,
			cam_id, isp->sensor_info[cam_id].para_satuaration_cur);
		break;
	case PARA_ID_COLOR_ENABLE:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_color_enable_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get COLOR_ENABLE %d\n", __func__,
			cam_id,
			isp->sensor_info[cam_id].para_color_enable_cur);
		break;
	case PARA_ID_ANTI_BANDING:
		((struct pvt_anti_banding *)para_value)->value =
			isp->sensor_info[cam_id].flick_type_cur;
		isp_dbg_print_info
			("-> %s,cid %d,get ANTI_BANDING %d\n", __func__,
			cam_id, isp->sensor_info[cam_id].flick_type_cur);
		break;
	case PARA_ID_ISO:
		*((AeIso_t *) para_value) =
			isp->sensor_info[cam_id].para_iso_cur;
		isp_dbg_print_info
			("-> %s,cid %d,get iso %u\n", __func__, cam_id,
			isp->sensor_info[cam_id].para_iso_cur.iso);
		break;

	case PARA_ID_EV:
		*((AeEv_t *) para_value) =
			isp->sensor_info[cam_id].para_ev_compensate_cur;
		isp_dbg_print_info
			("-> %s,cid %d,get ev %u/%u\n", __func__, cam_id,
		isp->sensor_info[cam_id].para_ev_compensate_cur.numerator,
		isp->sensor_info[cam_id].para_ev_compensate_cur.denominator);
		break;
	case PARA_ID_GAMMA:
		*((int32 *) para_value) =
			isp->sensor_info[cam_id].para_gamma_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get GAMMA %d\n", __func__, cam_id,
			isp->sensor_info[cam_id].para_gamma_cur);
		break;
	case PARA_ID_SCENE:
		*((enum pvt_scene_mode *)para_value) =
			isp->sensor_info[cam_id].para_scene_mode_cur;
		isp_dbg_print_info
			("-> %s, cid %d,get SCENE %d\n", __func__, cam_id,
			isp->sensor_info[cam_id].para_scene_mode_cur);
		break;
	default:
		isp_dbg_print_info
			("-> %s, cid %d,no supported paraid %d\n",
			__func__, cam_id, para_id);
		ret = IMF_RET_INVALID_PARAMETER;
		break;
	}

	if (ret == IMF_RET_SUCCESS) {
		isp_dbg_print_info
			("<- %s, success\n", __func__);
	} else {
		isp_dbg_print_err
			("<- %s, fail with %d\n", __func__, ret);
	}

	return ret;
};

int32 set_stream_para_imp(pvoid context, enum camera_id cam_id,
			enum stream_id str_id, enum para_id para_type,
			pvoid para_value)
{
	int ret = RET_SUCCESS;
	int32 func_ret = IMF_RET_SUCCESS;

	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (str_id > STREAM_ID_NUM)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp%p,cid%d,sid%d\n",
			__func__, isp, cam_id, str_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("-> %s,cid %d,sid %d,para %s(%d)\n", __func__,
		cam_id, str_id, isp_dbg_get_para_str(para_type), para_type);

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			enum pvt_img_fmt data_fmat =
				(*(enum pvt_img_fmt *)para_value);
			ret =
				isp_set_stream_data_fmt(context,
						cam_id, str_id,
						data_fmat);
			if (ret != RET_SUCCESS) {
				isp_dbg_print_err
					("<- %s(FMT) fail for set fmt:%s\n",
					__func__,
					isp_dbg_get_pvt_fmt_str(data_fmat));
				func_ret = IMF_RET_FAIL;
				break;
			}
			isp_dbg_print_info
				("<- %s(FMT) suc set fmt:%s\n", __func__,
				isp_dbg_get_pvt_fmt_str(data_fmat));
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;
			ret = isp_set_str_res_fps_pitch(context, cam_id,
							str_id, data_pitch);
			if (ret != RET_SUCCESS) {
				isp_dbg_print_err
					("<- %s(RES_FPS_PITCH) fail for set\n",
					__func__);
				func_ret = IMF_RET_FAIL;
				break;
			}

			isp_dbg_print_info("<- %s(RES_FPS_PITCH) suc\n",
				__func__);
			break;
		};
	default:
		isp_dbg_print_err
			("<- %s fail for not supported\n", __func__);
		func_ret = IMF_RET_INVALID_PARAMETER;
		break;
	}
	isp_mutex_unlock(&isp->ops_mutex);
	return func_ret;
};

int32 set_preview_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	enum stream_id sid = STREAM_ID_PREVIEW;

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	return set_stream_para_imp
		(context, cam_id, sid, para_type, para_value);
}

int32 get_preview_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;
	enum stream_id sid = STREAM_ID_PREVIEW;

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif

	if ((context == NULL) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		isp_dbg_print_err
			("-><- %s fail for illegal camera:%d\n",
			__func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
			isp_context->sensor_info[cam_id].str_info[sid].format;
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *value =
				(struct pvt_img_res_fps_pitch *)para_value;
			enum stream_id stream_type = sid;

			value->width =
		isp_context->sensor_info[cam_id].str_info[stream_type].width;
			value->height =
		isp_context->sensor_info[cam_id].str_info[stream_type].height;
			value->fps =
		isp_context->sensor_info[cam_id].str_info[stream_type].fps;
			value->luma_pitch =
	isp_context->sensor_info[cam_id].str_info[stream_type].luma_pitch_set;
			value->chroma_pitch =
isp_context->sensor_info[cam_id].str_info[stream_type].chroma_pitch_set;
			break;
		};
	default:
		isp_dbg_print_err
			("-><- %s fail for not supported para_type %d\n",
			__func__, para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info
		("-><- %s(%d) success for camera %d\n", __func__,
		para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int32 set_video_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_VIDEO,
				para_type, para_value);
};

int32 get_video_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;

	if ((context == NULL) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		isp_dbg_print_err
			("-><- %s fail for illegal camera:%d\n",
			__func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
	isp_context->sensor_info[cam_id].str_info[STREAM_ID_VIDEO].format;
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *value =
				(struct pvt_img_res_fps_pitch *)para_value;
			enum stream_id stream_type = STREAM_ID_VIDEO;

			value->width =
		isp_context->sensor_info[cam_id].str_info[stream_type].width;
			value->height =
		isp_context->sensor_info[cam_id].str_info[stream_type].height;
			value->fps =
		isp_context->sensor_info[cam_id].str_info[stream_type].fps;
			value->luma_pitch =
	isp_context->sensor_info[cam_id].str_info[stream_type].luma_pitch_set;
			value->chroma_pitch =
isp_context->sensor_info[cam_id].str_info[stream_type].chroma_pitch_set;
			break;
		};
	default:
		isp_dbg_print_err
			("-><- %s fail for not supported para_type %d\n",
			__func__, para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info
		("-><- %s(%d) success for camera %d\n", __func__,
		para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int32 set_rgbir_ir_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_RGBIR_IR,
			para_type, para_value);
};

int32 get_rgbir_ir_para_imp(pvoid context, enum camera_id cam_id,
			    enum para_id para_type, pvoid para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;
	enum stream_id stream_type;

	if ((context == NULL) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		isp_dbg_print_err
			("-><- %s fail for illegal camera:%d\n",
			__func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	stream_type = STREAM_ID_VIDEO;
	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp_context->sensor_info[cam_id].str_info[stream_type].format;
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *value =
				(struct pvt_img_res_fps_pitch *)para_value;

			value->width =
		isp_context->sensor_info[cam_id].str_info[stream_type].width;
			value->height =
		isp_context->sensor_info[cam_id].str_info[stream_type].height;
			value->fps =
		isp_context->sensor_info[cam_id].str_info[stream_type].fps;
			value->luma_pitch =
	isp_context->sensor_info[cam_id].str_info[stream_type].luma_pitch_set;
			value->chroma_pitch =
isp_context->sensor_info[cam_id].str_info[stream_type].chroma_pitch_set;
			break;
		};
	default:
		isp_dbg_print_err
			("-><- %s fail for not supported para_type %d\n",
			__func__, para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info
		("-><- %s(%d) success for camera %d\n", __func__,
		para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int32 set_zsl_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_ZSL,
				para_type, para_value);
}


int32 get_zsl_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	/*int ret = RET_SUCCESS; */
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].format;
			isp_dbg_print_info
				("%s success for camera %d, get format %d\n",
				__func__, cam_id,
				(*(enum pvt_img_fmt *)para_value));
			break;
		};
	case PARA_ID_IMAGE_SIZE:
		{
			struct pvt_img_size *img_size =
				(struct pvt_img_size *)para_value;
			img_size->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].width;
			img_size->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].height;
			isp_dbg_print_info
			("%s success for camera %d, set jpeg size %d:%d\n",
				__func__, cam_id,
				img_size->width, img_size->height);
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;

			if (data_pitch == NULL) {
				isp_dbg_print_info
			("-><- %s,came %d, NULL para for DATA_RES_FPS_PITCH\n",
					__func__, cam_id);
				return IMF_RET_INVALID_PARAMETER;
			}

			data_pitch->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].width;
			data_pitch->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].height;
			data_pitch->luma_pitch =
	isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].luma_pitch_set;
			data_pitch->chroma_pitch =
	isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].chroma_pitch_set;

			isp_dbg_print_info
("%s suc,came %d for DATA_RES_FPS_PITCH w(%d):h(%d),lpitch:%d cpitch:%d\n",
				__func__, cam_id, data_pitch->width,
				data_pitch->height, data_pitch->luma_pitch,
				data_pitch->chroma_pitch);
			break;
		}
	default:
		isp_dbg_print_err
			("-><- %s fail for not supported para_type %d\n",
			__func__, para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

//	isp_dbg_print_info
//		("-><- %s(%d) success for camera %d\n",
//		__func__, para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int32 set_rgbir_raw_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_RGBIR,
				para_type, para_value);
}

int32 get_rgbir_raw_para_imp(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value)
{
	/*int ret = RET_SUCCESS; */
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context or para_value\n",
			__func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].format;
			isp_dbg_print_info
				("%s success for camera %d, get format %d\n",
				__func__, cam_id,
				(*(enum pvt_img_fmt *)para_value));
			break;
		};
	case PARA_ID_IMAGE_SIZE:
		{
			struct pvt_img_size *img_size =
				(struct pvt_img_size *)para_value;
			img_size->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].width;
			img_size->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].height;
			isp_dbg_print_info
			("%s success for camera %d, set jpeg size %d:%d\n",
				__func__, cam_id,
				img_size->width, img_size->height);
			break;
		};
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;

			if (data_pitch == NULL) {
				isp_dbg_print_info
			("%s,came %d, NULL para for DATA_RES_FPS_PITCH\n",
					__func__, cam_id);
				return IMF_RET_INVALID_PARAMETER;
			}

			data_pitch->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].width;
			data_pitch->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].height;
			data_pitch->luma_pitch =
	isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].luma_pitch_set;
			data_pitch->chroma_pitch =
	isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].chroma_pitch_set;

			isp_dbg_print_info
("%s suc,came %d for DATA_RES_FPS_PITCH w(%d):h(%d),lpitch:%d cpitch:%d\n",
				__func__, cam_id, data_pitch->width,
				data_pitch->height, data_pitch->luma_pitch,
				data_pitch->chroma_pitch);
			break;
		}
	default:
		isp_dbg_print_err
			("-><- %s fail for not supported para_type %d\n",
			__func__, para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

//	isp_dbg_print_info
//		("-><- %s(%d) success for camera %d\n",
//		__func__, para_type, cam_id);

	return IMF_RET_SUCCESS;
}

int32 get_capabilities_imp(pvoid context, struct isp_capabilities *cap)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (cap == NULL)) {
		isp_dbg_print_err
			("-><- %s fail for null context\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	memcpy(cap, &isp->isp_cap, sizeof(struct isp_capabilities));
	return IMF_RET_SUCCESS;
};

int32 snr_clk_set_imp(pvoid context, enum camera_id cam_id,
			uint32 clk /*in k_h_z */)
{
	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_err
		("-><- %s succ,cid:%u,clk:%ukhz\n", __func__, cam_id, clk);
	return IMF_RET_SUCCESS;
};

struct sensor_res_fps_t *get_camera_res_fps_imp(pvoid context,
						enum camera_id cam_id)
{
	struct isp_context *isp = (struct isp_context *)context;
	isp_time_tick_t start;
	isp_time_tick_t end;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return NULL;
	}
	isp_get_cur_time_tick(&start);

	do {
		if (isp->snr_res_fps_info_got[cam_id])
			return &isp->sensor_info[cam_id].res_fps;
		isp_get_cur_time_tick(&end);
		if (isp_is_timeout(&start, &end, 500)) {
			isp_dbg_print_err
				("-><- %s fail for timeout\n", __func__);
			return NULL;
		}
	} while (1 > 0);

	return NULL;
}

bool_t enable_tnr_imp(pvoid context, enum camera_id cam_id,
			enum stream_id str_id, uint32 strength /*[0,100] */)
{
	context = context;
	cam_id = cam_id;
	str_id = str_id;
	strength = strength;

	return 0;
	/*todo  to be added later */
};

bool_t disable_tnr_imp(pvoid context, enum camera_id cam_id,
			enum stream_id str_id)
{
	context = context;
	cam_id = cam_id;
	str_id = str_id;

	return 0;
	/*todo  to be added later */
};

int32 take_raw_pic_imp(pvoid context, enum camera_id cam_id,
		sys_img_buf_handle_t raw_pic_buf, uint32 use_precapture)
{
	struct isp_context *isp;
	uint32 raw_width = 0;
	uint32 raw_height = 0;
	struct sensor_info *sif;
	CmdCaptureRaw_t cmd_para;
	sys_img_buf_handle_t buf;
	struct isp_mapped_buf_info *gen_img;
	uint32 y_len;
	enum fw_cmd_resp_stream_id stream_id;
	result_t ret;
	char pt_start_stream = 0;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SCR,
			PT_STATUS_SUCC);
	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (!raw_pic_buf || !raw_pic_buf->len || !raw_pic_buf->virtual_addr) {
		isp_dbg_print_err("-><- %s fail bad raw buf\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	memset(&cmd_para, 0, sizeof(cmd_para));
	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];

	raw_width = isp->sensor_info[cam_id].asprw_wind.h_size;
	raw_height = isp->sensor_info[cam_id].asprw_wind.v_size;
	y_len = raw_width * raw_height * 2;
	if (y_len == 0)
		y_len = raw_pic_buf->len;
	if (raw_pic_buf->len < y_len) {
		isp_dbg_print_err
			("-><- %s fail by small buf %u(%u=%ux%ux2 needed)\n",
			__func__, raw_pic_buf->len,
			(raw_width * raw_height * 2),
			raw_width, raw_height);
		return IMF_RET_FAIL;
	};
	y_len = raw_pic_buf->len;

	buf = sys_img_buf_handle_cpy(raw_pic_buf);

	if (buf == NULL) {
		isp_dbg_print_err
		("-><- %s fail cpy buf,cid:%d\n", __func__, cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	gen_img = isp_map_sys_2_mc(isp, buf, ISP_MC_ADDR_ALIGN,
				cam_id, stream_id, y_len, 0, 0);

	if (gen_img == NULL) {
		if (buf)
			sys_img_buf_handle_free(buf);
		isp_dbg_print_err
			("-><- %s fail map sys,cid:%d\n", __func__, cam_id);
		return IMF_RET_FAIL;
	}
	cmd_para.usePreCap = 0;
	cmd_para.buffer.vmidSpace = 4;
	isp_split_addr64(gen_img->y_map_info.mc_addr,
			&cmd_para.buffer.bufBaseALo,
			&cmd_para.buffer.bufBaseAHi);
	cmd_para.buffer.bufSizeA = gen_img->y_map_info.len;

	isp_split_addr64(gen_img->u_map_info.mc_addr,
			&cmd_para.buffer.bufBaseBLo,
			&cmd_para.buffer.bufBaseBHi);
	cmd_para.buffer.bufSizeB = gen_img->u_map_info.len;

	isp_split_addr64(gen_img->v_map_info.mc_addr,
			&cmd_para.buffer.bufBaseCLo,
			&cmd_para.buffer.bufBaseCHi);
	cmd_para.buffer.bufSizeC = gen_img->v_map_info.len;

	isp_mutex_lock(&isp->ops_mutex);

	isp_dbg_print_info
		("-> %s, cid:%d, precap %d, sys %llx(%u), mc %llx\n", __func__,
		cam_id, use_precapture, gen_img->y_map_info.sys_addr,
		gen_img->y_map_info.len, gen_img->y_map_info.mc_addr);
	if (gen_img->y_map_info.sys_addr) {
		uint8 *ch = (uint8 *) gen_img->y_map_info.sys_addr;

		ch[0] = 0xaa;
		ch[1] = 0x55;
		ch[2] = 0xbd;
		ch[3] = 0xce;
	}

	if ((cam_id < CAMERA_ID_MAX)
	&& (isp->sensor_info[cam_id].status) == START_STATUS_NOT_START)
		pt_start_stream = 1;

	if (pt_start_stream)
		isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC,
				PT_CMD_SS, PT_STATUS_SUCC);


	if (isp_init_stream(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_init_stream\n", __func__);
		ret = IMF_RET_FAIL;
		goto fail;
	};

	if (isp_setup_3a(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_setup_3a\n", __func__);
		ret = IMF_RET_FAIL;
		goto fail;
	};

	if (isp_kickoff_stream(isp, cam_id, raw_width, raw_height) !=
	RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_kickoff_stream\n", __func__);
		ret = IMF_RET_FAIL;
		goto fail;
	};
	if (pt_start_stream)
		isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD,
				PT_CMD_SS, PT_STATUS_SUCC);

	if (use_precapture) {
		cmd_para.usePreCap = 1;
		ret = isp_send_fw_cmd(isp, CMD_ID_AE_PRECAPTURE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT, NULL, 0);
		if (ret != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s fail for send AE_PRECAPTURE\n",
				__func__);
			goto fail;
		};
	}

	ret = isp_send_fw_cmd(isp, CMD_ID_CAPTURE_RAW,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&cmd_para, sizeof(cmd_para));

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_SCR,
			PT_STATUS_SUCC);

	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for send CAPTURE_RAW\n", __func__);
		goto fail;
	};
	if (cam_id < CAMERA_ID_MAX) {
		isp->sensor_info[cam_id].take_one_raw_left_cnt++;
		isp_list_insert_tail(
			&isp->sensor_info[cam_id].take_one_raw_buf_list,
			(struct list_node *)gen_img);
	}
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_info("<- %s(%ubytes), succ\n", __func__, y_len);
	return IMF_RET_SUCCESS;
fail:
	if (gen_img)
		isp_unmap_sys_2_mc(isp, gen_img);
	if (buf)
		sys_img_buf_handle_free(buf);
	isp_mutex_unlock(&isp->ops_mutex);
	return IMF_RET_FAIL;

	/*todo  to be added later */
};

int32 stop_take_raw_pic_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	int32 ret_val;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_ECR,
			PT_STATUS_SUCC);

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	sif = &isp->sensor_info[cam_id];

	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_ECR,
			PT_STATUS_SUCC);
	while (sif->take_one_raw_left_cnt) {
		/*
		 *isp_dbg_print_info
		 *	("in %s, front %p,%p\n", __func__,
		 *	isp->take_one_pic_buf_list.front,
		 *	isp->take_one_pic_buf_list.rear);
		 *
		 *if (isp->take_one_pic_buf_list.front ==
		 *isp->take_one_pic_buf_list.rear) {
		 *	break;
		 *}
		 */
		usleep_range(10000, 11000);
	}
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_ECR,
			PT_STATUS_SUCC);
	isp_mutex_lock(&isp->ops_mutex);

	ret_val = IMF_RET_SUCCESS;
	isp_mutex_unlock(&isp->ops_mutex);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_ECR,
			PT_STATUS_SUCC);
	isp_dbg_print_info("<- %s, succ\n", __func__);

	return ret_val;
}

int32 set_focus_mode_imp(pvoid context, enum camera_id cam_id, AfMode_t mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d,mode %s\n", __func__, cam_id,
			isp_dbg_get_af_mode_str(mode));

	if (isp_fw_set_focus_mode(isp, cam_id, mode) != RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_fw_start_af(isp, cam_id);
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 auto_focus_lock_imp(pvoid context, enum camera_id cam_id,
			LockType_t lock_mode)
{
#ifdef ISP_TUNING_TOOL_SUPPORT
	unreferenced_parameter(context);
	unreferenced_parameter(cam_id);
	unreferenced_parameter(lock_mode);

	isp_dbg_print_info("-><- %s, fail for not support\n", __func__);
	return IMF_RET_FAIL;
#else
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);
	if (isp_fw_3a_lock(isp, cam_id, ISP_3A_TYPE_AF, lock_mode) !=
	RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
#endif
};

int32 auto_focus_unlock_imp(pvoid context, enum camera_id cam_id)
{
#ifdef ISP_TUNING_TOOL_SUPPORT
	unreferenced_parameter(context);
	unreferenced_parameter(cam_id);

	isp_dbg_print_info("-><- %s, fail for not support\n", __func__);
	return IMF_RET_FAIL;
#else
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
		("-><- %s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);
	if (isp_fw_3a_unlock(isp, cam_id, ISP_3A_TYPE_AF) != RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
#endif
};

int32 auto_exposure_lock_imp(pvoid context, enum camera_id cam_id,
			LockType_t lock_mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);
	if (isp_fw_3a_lock(isp, cam_id, ISP_3A_TYPE_AE, lock_mode) !=
	RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 auto_exposure_unlock_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	isp_dbg_print_info("%s, cam %d\n", __func__, cam_id);
	if (isp_fw_3a_unlock(isp, cam_id, ISP_3A_TYPE_AE) != RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 auto_wb_lock_imp(pvoid context, enum camera_id cam_id,
			LockType_t lock_mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	isp_dbg_print_info("%s, cam %d\n", __func__, cam_id);

	if (isp_fw_3a_lock(isp, cam_id, ISP_3A_TYPE_AWB, lock_mode) !=
	RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 auto_wb_unlock_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	if (isp_fw_3a_unlock(isp, cam_id, ISP_3A_TYPE_AWB) != RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for isp_fw_set_focus_mode, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_lens_position_imp(pvoid context, enum camera_id cam_id, uint32 pos)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	if (pos > FOCUS_MAX_VALUE) {
		isp_dbg_print_err
			("%s fail bad para,cid:%d,[%u]\n",
			__func__, cam_id, pos);
		return IMF_RET_INVALID_PARAMETER;
	};

	pos = IspMapValueFromRange(FOCUS_MIN_VALUE, FOCUS_MAX_VALUE,
				pos, sif->lens_range.minLens,
				sif->lens_range.maxLens);
	isp_dbg_print_info("%s, cid %d, pos %u\n", __func__, cam_id, pos);
	if (isp_fw_set_lens_pos(isp, cam_id, pos) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_fw_set_lens_pos\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 get_focus_position_imp(pvoid context, enum camera_id cam_id, uint32 *pos)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id) || (pos == NULL)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 set_exposure_mode_imp(pvoid context, enum camera_id cam_id,
				AeMode_t mode)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if ((mode != AE_MODE_MANUAL) && (mode != AE_MODE_AUTO)) {
		isp_dbg_print_err
			("%s fail bad mode:%d\n", __func__, mode);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	isp_dbg_print_info("%s, cam %d,mode %s\n", __func__, cam_id,
			isp_dbg_get_ae_mode_str(mode));

	if (isp_fw_set_exposure_mode(isp, cam_id, mode) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail by isp_fw_set_exposure_mode\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	if ((mode == AE_MODE_AUTO) &&
	sif->calib_enable && sif->calib_set_suc) {
		sif->sensor_cfg.exposure = sif->sensor_cfg.exposure_orig;
		isp_setup_ae_range(isp, cam_id);
	}
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_wb_mode_imp(pvoid context, enum camera_id cam_id, AwbMode_t mode)
{
	struct isp_context *isp;

	isp = (struct isp_context *)context;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp_dbg_print_info("%s, cam %d,mode %s\n", __func__, cam_id,
			isp_dbg_get_awb_mode_str(mode));

	if (isp_fw_set_wb_mode(isp, cam_id, mode) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_fw_set_wb_mode\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_snr_ana_gain_imp(pvoid context, enum camera_id cam_id,
			uint32 ana_gain)
{
	struct isp_context *isp;
	struct sensor_info *snr_info;

	if (!is_para_legal(context, cam_id) || (ana_gain == 0)) {
		isp_dbg_print_err
		("%s fail for illegal para,context:%p,cam_id:%d,ana_gain %d\n",
			__func__, context, cam_id, ana_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	snr_info = &isp->sensor_info[cam_id];

	if ((ana_gain < snr_info->sensor_cfg.exposure.min_gain)
	|| (ana_gain > snr_info->sensor_cfg.exposure.max_gain)) {
		isp_dbg_print_err
			("%s fail bad again %u,should[%u,%u]\n", __func__,
			ana_gain, snr_info->sensor_cfg.exposure.min_gain,
			snr_info->sensor_cfg.exposure.max_gain);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_dbg_print_info
		("%s, cam %d, ana_gain %u\n", __func__, cam_id, ana_gain);

	if (isp->fw_ctrl_3a &&
	(isp->drv_settings.set_again_by_sensor_drv != 1)) {
		if (isp_fw_set_snr_ana_gain(isp, cam_id, ana_gain) !=
		RET_SUCCESS) {
			isp_dbg_print_err
		("%s fail for isp_fw_set_snr_ana_gain,context:%p,cam_id:%d\n",
				__func__, context, cam_id);
			return IMF_RET_INVALID_PARAMETER;
		};
	} else {
		uint32 set_gain;
		uint32 set_integration;
		result_t tmp_ret;

		tmp_ret = isp_snr_exposure_ctrl(isp, cam_id, ana_gain,
					0, &set_gain, &set_integration);
	}

	isp_dbg_print_info("<- %s, succ\n", __func__);

	return IMF_RET_SUCCESS;
};

int32 set_snr_dig_gain_imp(pvoid context, enum camera_id cam_id,
			uint32 dig_gain)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
	("%s fail for illegal para, context:%p,cam_id:%d,ana_gain 0x%x\n",
			__func__, context, cam_id, dig_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	dig_gain = dig_gain;

	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 get_snr_gain_imp(pvoid context, enum camera_id cam_id,
			uint32 *ana_gain, uint32 *dig_gain)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	ana_gain = ana_gain;
	dig_gain = dig_gain;

	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 get_snr_gain_cap_imp(pvoid context, enum camera_id cam_id,
				struct para_capability_u32 *ana,
				struct para_capability_u32 *dig)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp_snr_get_gain_caps(isp, cam_id, ana, dig) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_snr_get_caps,context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_dbg_print_info("-><- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_snr_itime_imp(pvoid context, enum camera_id cam_id, uint32 itime)
{
	struct isp_context *isp;
	struct sensor_info *snr_info;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	snr_info = &isp->sensor_info[cam_id];

	if ((itime < snr_info->sensor_cfg.exposure.min_integration_time)
	|| (itime > snr_info->sensor_cfg.exposure.max_integration_time)) {
		uint32 min = 0;
		uint32 max = itime + 1000;
		CmdAeSetItimeRange_t i_time;
		enum fw_cmd_resp_stream_id stream_id;

		if (max < 500000)
			max = 500000;

		stream_id = isp_get_stream_id_from_cid(isp, cam_id);
		i_time.minItime = min;
		i_time.maxItime = max;

		if (isp_send_fw_cmd(isp,
				CMD_ID_AE_SET_ITIME_RANGE,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&i_time, sizeof(i_time)) != RET_SUCCESS) {
			isp_dbg_print_err
				("-><- %s fail set range[%u,%u] for %u\n",
				__func__, min, max, itime);
			return IMF_RET_INVALID_PARAMETER;
		}

		snr_info->sensor_cfg.exposure.min_integration_time = min;
		snr_info->sensor_cfg.exposure.max_integration_time = max;
		isp_dbg_print_info("%s,itime range (%d,%d)\n",
			__func__, i_time.minItime, i_time.maxItime);

		isp_dbg_print_info
			("%s,cid:%d,itime %u,reset range[%u,%u]\n",
			__func__, cam_id, itime, min, max);
	} else {
		isp_dbg_print_info
			("%s, cam %d, itime %u\n", __func__, cam_id, itime);
	}

	if (isp->fw_ctrl_3a &&
	(isp->drv_settings.set_itime_by_sensor_drv != 1)) {
		if (isp_fw_set_itime(isp, cam_id, itime) != RET_SUCCESS) {
			isp_dbg_print_err
			("%s fail for isp_fw_set_itime,context:%p,cam_id:%d\n",
				__func__, context, cam_id);
			return IMF_RET_INVALID_PARAMETER;
		};
	} else {
		uint32 set_gain;
		uint32 set_integration;
		result_t tmp_ret;

		tmp_ret = isp_snr_exposure_ctrl(isp, cam_id,
						0,
						itime,
						&set_gain, &set_integration);
	}

	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 get_snr_itime_imp(pvoid context, enum camera_id cam_id, uint32 *itime)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id) || (itime == NULL)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp = (struct isp_context *)context;
	if (isp->p_sensor_ops[cam_id]->get_itime(cam_id, itime)
	== RET_SUCCESS) {
		isp_dbg_print_info
			("%s suc,cid:%d,itime %u\n", __func__, cam_id, *itime);
		return IMF_RET_SUCCESS;
	}

	isp_dbg_print_err("%s fail,cid:%d\n", __func__, cam_id);
	return IMF_RET_FAIL;
};

int32 get_snr_itime_cap_imp(pvoid context, enum camera_id cam_id,
			uint32 *min, uint32 *max, uint32 *step)
{
	struct isp_context *isp;
	struct para_capability_u32 para;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp_snr_get_itime_caps(isp, cam_id, &para) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_snr_get_caps,context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	if (max)
		*max = para.max;
	if (min)
		*min = para.min;
	if (step)
		*step = para.step;
	isp_dbg_print_info("-><- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

void reg_fw_parser_imp(pvoid context, enum camera_id cam_id,
			struct fw_parser_operation_t *parser)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (parser == NULL)) {
		isp_dbg_print_err
			("%s fail for bad context\n", __func__);
		return;
	}

	isp_dbg_print_info
		("%s suc, cid %d, ops:%p\n", __func__, cam_id, parser);

	isp->fw_parser[cam_id] = parser;
};

void unreg_fw_parser_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for bad para\n", __func__);
		return;
	}

	isp_dbg_print_info("%s suc, cid %d\n", __func__, cam_id);

	isp->fw_parser[cam_id] = NULL;
};

int32 set_fw_gv_imp(pvoid context, enum camera_id id, enum fw_gv_type type,
		    ScriptFuncArg_t *gv)
{
	struct isp_context *isp;
//	isp_sensor_ctrl_script_t *script;
//	isp_sensor_ctrl_script_func_arg_t *para;
	ScriptFuncArg_t *para;

	if (!is_para_legal(context, id) || (gv == NULL)) {
		isp_dbg_print_err
		("%s fail for illegal para,context:%p,cam_id:%d,type %d\n",
			__func__, context, id, type);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
//	script = &isp->sensor_info[id].script_cmd.script;

	switch (type) {
	case FW_GV_AF:
		para = &isp->sensor_info[id].script_cmd.deviceScript.argLens;
		break;
	case FW_GV_HDR:
//		para = &isp->sensor_info[id].script_cmd.script.arg_hdr;
//		break;
	case FW_GV_AE_ANA_GAIN:
//		para = &isp->sensor_info[id].script_cmd.script.arg_ae_ana_gain;
//		break;
	case FW_GV_AE_DIG_GAIN:
//		para = &isp->sensor_info[id].script_cmd.script.
//			arg_ae_digi_gain;
//		break;
	case FW_GV_AE_ITIME:
//		para = &isp->sensor_info[id].script_cmd.script.arg_ae_int_time;
//		break;
		para = &isp->sensor_info[id].script_cmd.deviceScript.argSensor;
		break;
	case FW_GV_FL:
		para = &isp->sensor_info[id].script_cmd.deviceScript.argFlash;
		break;
	default:
		isp_dbg_print_err
		("%s fail for illegal para, context:%p,cam_id:%d,type:%d\n",
			__func__, context, id, type);

		return IMF_RET_INVALID_PARAMETER;
	}

	memcpy(para, gv, sizeof(ScriptFuncArg_t));

	isp_dbg_print_info("%s succ for cam:%d,type %d\n", __func__, id, type);

	return IMF_RET_SUCCESS;
}

int32 set_digital_zoom_imp(pvoid context, enum camera_id cam_id,
			uint32 h_off, uint32 v_off, uint32 width,
			uint32 height)
{
	struct isp_context *isp;
	CmdSetZoomWindow_t zoom_command;
	result_t result;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (width == 0) || (height == 0)) {
		isp_dbg_print_err
			("%s fail bad para,isp:%p,cid:%d,w:h(%u:%u)\n",
			__func__, context, cam_id, width, height);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	h_off = h_off;
	v_off = v_off;
	width = width;
	height = height;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	zoom_command.window.h_offset = h_off;
	zoom_command.window.v_offset = v_off;
	zoom_command.window.h_size = width;
	zoom_command.window.v_size = height;
	result = isp_send_fw_cmd(isp, CMD_ID_SET_ZOOM_WINDOW, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &zoom_command,
				sizeof(zoom_command));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for send cmd, cid:%d\n", __func__, cam_id);
		return IMF_RET_FAIL;
	}

	/*save the zoom window here */
	isp->sensor_info[cam_id].zoom_info.window = zoom_command.window;

	/*===do not wait the return of zoom command here*/
	/*return aidt_isp_event_wait(&g_isp_context.zoom_command); */

	isp_dbg_print_info
		("%s suc,cid:%d,h_off %u,v_off %u,w %u,h %u\n",
		__func__, cam_id, h_off, v_off, width, height);

	return IMF_RET_SUCCESS;
};


int32 get_snr_raw_img_type_imp(pvoid context, enum camera_id cam_id,
			       enum raw_image_type *type)
{
	struct isp_context *isp;
	MipiDataType_t data_type;

	if (!is_para_legal(context, cam_id) || (type == NULL)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	data_type =
	isp->sensor_info[cam_id].sensor_cfg.prop.intfProp.mipi.dataType;

	switch (data_type) {
	case MIPI_DATA_TYPE_RAW_12:
		*type = RAW_IMAGE_TYPE_12BIT;
		break;
	case MIPI_DATA_TYPE_RAW_10:
		*type = RAW_IMAGE_TYPE_10BIT;
		break;
	case MIPI_DATA_TYPE_RAW_8:
		*type = RAW_IMAGE_TYPE_8BIT;
		break;
	default:
		isp_dbg_print_err
			("%s fail for bad dataType %d,cid:%d\n",
			__func__, data_type, cam_id);
		return IMF_RET_FAIL;
	}
	isp_dbg_print_err
		("%s suc,cam_id:%d, type %d\n", __func__, cam_id, *type);

	return IMF_RET_SUCCESS;
};

int32 set_iso_value_imp(pvoid context, enum camera_id cam_id, AeIso_t value)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("%s, cid %d, iso (%u)\n",
		__func__, cam_id, value.iso);

	if (isp_fw_set_iso(isp, cam_id, value) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_fw_set_iso\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_dbg_print_info("<- %s, succ\n", __func__);

	return IMF_RET_SUCCESS;
};

int32 set_ev_value_imp(pvoid context, enum camera_id cam_id, AeEv_t value)
{
	CmdAeSetEv_t ev;
	struct isp_context *isp = (struct isp_context *)context;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("%s fail, bad para,cam_id:%d\n", __func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ev.ev = value;
//	sif = &isp->sensor_info[cam_id];

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_EV, stream_id,
	FW_CMD_PARA_TYPE_DIRECT, &ev, sizeof(ev)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s cid:%u, fail set ev(%i/%i)\n", __func__,
			cam_id, value.numerator, value.denominator);
		return IMF_RET_SUCCESS;
	}

	isp_dbg_print_info("%s cid:%u, set ev(%i/%i) suc\n", __func__,
		cam_id, value.numerator, value.denominator);

	return RET_SUCCESS;
};


int32 set_zsl_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl)
{
	int32 ret;

//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
//			PT_CMD_ZB, PT_STATUS_SUCC,
//			buf_hdl ? buf_hdl->virtual_addr : 0);

	ret = set_stream_buf_imp(context, cam_id, buf_hdl, STREAM_ID_ZSL);

//	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
//			PT_CMD_ZB, PT_STATUS_SUCC,
//			buf_hdl ? buf_hdl->virtual_addr : 0);

	return ret;
};

int32 start_zsl_imp(pvoid context, enum camera_id cid, bool is_perframe_ctl)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_SZ,
			PT_STATUS_SUCC);
	ret = start_stream_imp(context, cid, STREAM_ID_ZSL, is_perframe_ctl);

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_SZ,
			PT_STATUS_SUCC);
	return ret;
};


int32 stop_zsl_imp(pvoid context, enum camera_id cam_id, bool pause)
{
	int32 ret;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RC, PT_CMD_EZ,
			PT_STATUS_SUCC);
	ret = stop_stream_imp(context, cam_id, STREAM_ID_ZSL, pause);

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SCD, PT_CMD_EZ,
			PT_STATUS_SUCC);

	return ret;
};


int32 set_digital_zoom_ratio_imp(pvoid context, enum camera_id cam_id,
					uint32 zoom)
{
	struct isp_context *isp;
	int32 result;
	Window_t zoom_wnd;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if ((zoom < (unsigned int)isp->isp_cap.zoom.min)
	|| (zoom > (unsigned int)isp->isp_cap.zoom.max)) {
		isp_dbg_print_err
			("%s fail for bad zoom %d\n", __func__, zoom);
		return IMF_RET_INVALID_PARAMETER;
	};

	result = isp_get_zoom_window
			(isp->sensor_info[cam_id].asprw_wind.h_size,
			isp->sensor_info[cam_id].asprw_wind.v_size,
			zoom, &zoom_wnd);

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail by isp_get_zoom_window,cid %u,zoom %d\n",
			__func__, cam_id, zoom);
	};

	isp->sensor_info[cam_id].zoom_info.ratio = zoom;
	result =
		set_digital_zoom_imp(isp, cam_id, zoom_wnd.h_offset,
				zoom_wnd.v_offset, zoom_wnd.h_size,
				zoom_wnd.v_size);

	isp_dbg_print_info("<- %s ret %d\n", __func__, result);

	return result;
};

int32 set_digital_zoom_pos_imp(pvoid context, enum camera_id cam_id,
			int16 h_off, int16 v_off)
{
	struct isp_context *isp;
	int32 result = RET_FAILURE;

	h_off = h_off;
	v_off = v_off;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	isp_dbg_print_err
		("%s ret %d for not implemented\n", __func__, result);

	return result;
};


void test_gfx_interface_imp(pvoid context)
{
//	struct isp_context *isp = context;
//	struct isp_mapped_buf_info *gen_img;
//	struct sys_img_buf_handle buf_hdl;
//	uint32 aloc_mem_size = 3 * 1024 * 1024;

	if (context == NULL) {
		isp_dbg_print_info
			("-><- %s do none for NULL\n", __func__);
		return;
	};
	if (g_isp_env_setting != ISP_ENV_SILICON) {
		isp_dbg_print_info
			("-><- %s do none for fpga\n", __func__);
		return;
	};

	isp_dbg_print_info("<- %s\n", __func__);
}

int32 set_metadata_buf_imp(pvoid context, enum camera_id cam_id,
			   sys_img_buf_handle_t buf_hdl)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	result_t result;
	int32 ret = IMF_RET_SUCCESS;

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (buf_hdl->virtual_addr == 0)) {
		isp_dbg_print_err
		("%s fail for illegal para,context:%p,cam_id:%d,buf_hdl:%p\n",
			__func__, context, cam_id, buf_hdl);
		return IMF_RET_INVALID_PARAMETER;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		isp_dbg_print_err
			("%s fail for sys_img_buf_handle_cpy\n", __func__);
		return IMF_RET_FAIL;
	};

	gen_img =
		isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
			cam_id, STREAM_ID_PREVIEW, buf_hdl->len, 0, 0);

	if (gen_img == NULL) {
		isp_dbg_print_err
			("%s fail for create isp buffer fail\n", __func__);
		ret = IMF_RET_FAIL;
		goto quit;
	}

	isp_dbg_print_info
		("%s,cid %d,sys:%p,mc:%llx(%u)\n", __func__, cam_id,
		buf_hdl->virtual_addr, gen_img->y_map_info.mc_addr,
		gen_img->y_map_info.len);

	result = isp_fw_send_metadata_buf(isp, cam_id, gen_img);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail1 for isp_fw_send_metadata_buf fail\n",
			__func__);
		ret = IMF_RET_FAIL;
		goto quit;
	};

	isp_list_insert_tail(&isp->sensor_info[cam_id].meta_data_in_fw,
				(struct list_node *)gen_img);
	isp_dbg_print_info("%s,suc\n", __func__);
quit:
	if (ret != IMF_RET_SUCCESS) {
		if (buf_hdl)
			sys_img_buf_handle_free(buf_hdl);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
	}
	return ret;
};


int32 get_camera_raw_type_imp(pvoid context, enum camera_id cam_id,
				enum raw_image_type *raw_type,
				uint32 *raw_bayer_pattern)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (raw_type == NULL)
	|| (raw_bayer_pattern == NULL)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (isp->sensor_info[cam_id].sensor_cfg.prop.intfType ==
	SENSOR_INTF_TYPE_MIPI) {

		MipiDataType_t data_type =
	isp->sensor_info[cam_id].sensor_cfg.prop.intfProp.mipi.dataType;

		*raw_bayer_pattern =
			isp->sensor_info[cam_id].sensor_cfg.prop.cfaPattern;

		switch (data_type) {
		case MIPI_DATA_TYPE_RAW_12:
			*raw_type = RAW_IMAGE_TYPE_12BIT;
			break;
		case MIPI_DATA_TYPE_RAW_10:
			*raw_type = RAW_IMAGE_TYPE_10BIT;
			break;
		case MIPI_DATA_TYPE_RAW_8:
			*raw_type = RAW_IMAGE_TYPE_8BIT;
			break;
		default:
			/*will never get to here */
			isp_dbg_print_err
				("%s fail for cid:%d, bad type %d for mipi\n",
				__func__, cam_id, data_type);
			return IMF_RET_FAIL;
		}
	} else if (
	isp->sensor_info[cam_id].sensor_cfg.prop.intfType ==
	SENSOR_INTF_TYPE_PARALLEL) {

		ParallelDataType_t data_type =
	isp->sensor_info[cam_id].sensor_cfg.prop.intfProp.parallel.dataType;

		*raw_bayer_pattern =
			isp->sensor_info[cam_id].sensor_cfg.prop.cfaPattern;

		switch (data_type) {
		case PARALLEL_DATA_TYPE_RAW12:
			*raw_type = RAW_IMAGE_TYPE_12BIT;
			break;
		case PARALLEL_DATA_TYPE_RAW10:
			*raw_type = RAW_IMAGE_TYPE_10BIT;
			break;
		case PARALLEL_DATA_TYPE_RAW8:
			*raw_type = RAW_IMAGE_TYPE_8BIT;
			break;
		default:
			/*will never get to here */
			isp_dbg_print_err
				("%s fail for cid:%d, bad type %d for para\n",
				__func__, cam_id, data_type);
			return IMF_RET_FAIL;
		};
	} else {
		/*will never get to here */
		isp_dbg_print_err("%s fail for cid:%d, bad if type %d\n",
			__func__, cam_id,
			isp->sensor_info[cam_id].sensor_cfg.prop.intfType);
		return IMF_RET_FAIL;
	}
	isp_dbg_print_info("%s suc for cid:%d, raw_type %d\n",
		__func__, cam_id, *raw_type);

	return IMF_RET_SUCCESS;
}

int32 get_camera_dev_info_imp(pvoid context, enum camera_id cid,
				struct camera_dev_info *cam_dev_info)
{
	struct sensor_hw_parameter para;
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cid) || cam_dev_info == NULL) {
		isp_dbg_print_err
			("-><- %s fail para,isp:%p,cid:%d,info:%p\n",
			__func__, context, cid, cam_dev_info);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (cid == CAMERA_ID_MEM) {
		memcpy(cam_dev_info, &isp->cam_dev_info[cid],
			sizeof(struct camera_dev_info));
		isp_dbg_print_info
			("-><- %s suc,cid:%d,info:%p\n", __func__,
			cid, cam_dev_info);
		return IMF_RET_SUCCESS;
	}

	if (isp_snr_get_hw_parameter(isp, cid, &para) == RET_SUCCESS) {
		isp->cam_dev_info[cid].type = para.type;
		isp->cam_dev_info[cid].focus_len = para.focus_len;
	};

	memcpy(cam_dev_info, &isp->cam_dev_info[cid],
		sizeof(struct camera_dev_info));

	if (cid == CAMERA_ID_REAR) {
		if (strlen(isp->drv_settings.rear_sensor))
			strncpy(cam_dev_info->cam_name,
				isp->drv_settings.rear_sensor,
				CAMERA_DEVICE_NAME_LEN);
		if (isp->drv_settings.rear_sensor_type != -1)
			cam_dev_info->type =
				isp->drv_settings.rear_sensor_type;
//commented out since not necessary for mero.
//		if (strlen(isp->drv_settings.rear_vcm))
//			strncpy(cam_dev_info->vcm_name,
//				isp->drv_settings.rear_vcm,
//				CAMERA_DEVICE_NAME_LEN);
//		if (strlen(isp->drv_settings.rear_flash))
//			strncpy(cam_dev_info->flash_name,
//				isp->drv_settings.rear_flash,
//				CAMERA_DEVICE_NAME_LEN);
	} else if (cid == CAMERA_ID_FRONT_LEFT) {
		if (strlen(isp->drv_settings.frontl_sensor))
			strncpy(cam_dev_info->cam_name,
				isp->drv_settings.frontl_sensor,
				CAMERA_DEVICE_NAME_LEN);
		if (isp->drv_settings.frontl_sensor_type != -1)
			cam_dev_info->type =
				isp->drv_settings.frontl_sensor_type;
	} else if (cid == CAMERA_ID_FRONT_RIGHT) {
		if (strlen(isp->drv_settings.frontr_sensor))
			strncpy(cam_dev_info->cam_name,
				isp->drv_settings.frontr_sensor,
				CAMERA_DEVICE_NAME_LEN);
		if (isp->drv_settings.frontr_sensor_type != -1)
			cam_dev_info->type =
				isp->drv_settings.frontr_sensor_type;
	};

	isp_dbg_print_info
		("-><- %s suc for cid:%d\n", __func__, cid);
	return IMF_RET_SUCCESS;
};

int32 set_wb_light_imp(pvoid context, enum camera_id cam_id, uint32 light_idx)
{
	struct isp_context *isp;
	CmdAwbSetLight_t light;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp->sensor_info[cam_id].wb_idx == NULL) {
		isp_dbg_print_err("%s no wb_idx in calib data\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	light.lightIndex =
		(uint8) isp->sensor_info[cam_id].wb_idx->index_map[light_idx];
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LIGHT, stream_id,
	FW_CMD_PARA_TYPE_DIRECT, &light, sizeof(light)) !=
	RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cid %u,light %u\n", __func__,
			cam_id, light_idx);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc,cid %u,light %u\n", __func__,
		cam_id, light_idx);
	return IMF_RET_SUCCESS;
}

int32 disable_color_effect_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

static int32 map_to_roi_window_from_preview(window_t *roi_zoom,
					window_t *window_zoom,
					window_t *roi_preview,
					uint32 preview_width,
					uint32 preview_height)
{
	/*suppose resolution and offset < 65536, no overflow */
	if (preview_width != 0 && preview_height != 0) {
		roi_zoom->h_offset =
			window_zoom->h_size * roi_preview->h_offset /
			preview_width;
		roi_zoom->v_offset =
			window_zoom->v_size * roi_preview->v_offset /
			preview_height;

		roi_zoom->h_size =
			window_zoom->h_size * roi_preview->h_size /
			preview_width;
		roi_zoom->v_size =
			window_zoom->v_size * roi_preview->v_size /
			preview_height;
	}

	return IMF_RET_SUCCESS;
}

int32 set_awb_roi_imp(pvoid context, enum camera_id cam_id, window_t *window)
{
	struct isp_context *isp;
	CmdAwbSetRegion_t cmd;
	enum fw_cmd_resp_stream_id stream_id;
	window_t window_roi = { 0, 0, 0, 0 };
//	result_t result;

	if (!is_para_legal(context, cam_id)
	|| (window == NULL)) {
		isp_dbg_print_err("%s fail bad para, isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_err("%s fail fsm %d, cid %u\n", __func__,
			ISP_GET_STATUS(isp), cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
//	cmd.window = *(Window_t *)window;

	if (isp->sensor_info[cam_id].zoom_info.window.h_size == 0 &&
	isp->sensor_info[cam_id].zoom_info.window.v_size == 0) {
/*
 *		No Digital Zoom applied, so we need to use aspect
 *		ratio windows as stage 1 RAW output resolution
 */
		map_to_roi_window_from_preview(&window_roi,
			&isp->sensor_info[cam_id].asprw_wind, window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);
	} else {
		map_to_roi_window_from_preview(&window_roi,
			&isp->sensor_info[cam_id].zoom_info.window,     window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);
	}

	cmd.window = window_roi;

	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_REGION,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&cmd, sizeof(cmd)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cid %u,[%u,%u,%u,%u]\n",
			__func__, cam_id,
			cmd.window.h_offset, cmd.window.v_offset,
			cmd.window.h_size, cmd.window.v_size);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc,cid %u,[%u,%u,%u,%u]\n",
		__func__, cam_id,
		cmd.window.h_offset, cmd.window.v_offset,
		cmd.window.h_size, cmd.window.v_size);
	return IMF_RET_SUCCESS;
};

int32 set_ae_roi_imp(pvoid context, enum camera_id cam_id, window_t *window)
{
	struct isp_context *isp;
	CmdAeSetRegion_t cmd;
	enum fw_cmd_resp_stream_id stream_id;
//	window_t window_roi = { 0, 0, 0, 0 };

	if (!is_para_legal(context, cam_id)
	|| (window == NULL)) {
		isp_dbg_print_err("%s fail bad para, isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_err("%s fail fsm %d, cid %u\n", __func__,
			ISP_GET_STATUS(isp), cam_id);
		return IMF_RET_FAIL;
	}

//	cmd.window = window_roi;
	cmd.window.h_offset = window->h_offset;
	cmd.window.h_size = window->h_size;
	cmd.window.v_offset = window->v_offset;
	cmd.window.v_size = window->v_size;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_REGION, stream_id,
	FW_CMD_PARA_TYPE_DIRECT, &cmd, sizeof(cmd)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cid %u,[%u,%u,%u,%u]\n",
			__func__, cam_id,
			cmd.window.h_offset, cmd.window.v_offset,
			cmd.window.h_size, cmd.window.v_size);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc,cid %u,[%u,%u,%u,%u]\n",
		__func__, cam_id,
		cmd.window.h_offset, cmd.window.v_offset,
		cmd.window.h_size, cmd.window.v_size);
	return IMF_RET_SUCCESS;
};

int32 set_af_roi_imp(pvoid context, enum camera_id cam_id,
			AfWindowId_t id, window_t *window)
{
	struct isp_context *isp;
	CmdAfSetRegion_t cmd;
	enum fw_cmd_resp_stream_id stream_id;
	window_t window_roi = { 0, 0, 0, 0 };
//	result_t result;

	if (!is_para_legal(context, cam_id)
	|| (window == NULL)
	|| (id >= AF_WINDOW_ID_MAX)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d,windid:%d\n",
			__func__, context, cam_id, id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_err("%s fail fsm %d, cid %u\n",
			__func__, ISP_GET_STATUS(isp), cam_id);
		return IMF_RET_FAIL;
	}
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.windowId = id;
//	cmd.window = *(Window_t *)window;

	if (isp->sensor_info[cam_id].zoom_info.window.h_size == 0 &&
	isp->sensor_info[cam_id].zoom_info.window.v_size == 0) {
//		No Digital Zoom applied, so we need to
//		use aspect ratio windows as stage 1 RAW output resolution
		map_to_roi_window_from_preview(&window_roi,
			&isp->sensor_info[cam_id].asprw_wind,
			window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);
	} else {
		map_to_roi_window_from_preview(&window_roi,
			&isp->sensor_info[cam_id].zoom_info.window,
			window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);
	}

	cmd.window = window_roi;

	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_REGION,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&cmd, sizeof(cmd)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cid %u,winid %u,[%u,%u,%u,%u]\n",
			__func__, cam_id, id,
			cmd.window.h_offset, cmd.window.v_offset,
			cmd.window.h_size, cmd.window.v_size);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc,cid %u,winid %u,[%u,%u,%u,%u]\n",
		__func__, cam_id, id,
		cmd.window.h_offset, cmd.window.v_offset,
		cmd.window.h_size, cmd.window.v_size);
	return IMF_RET_SUCCESS;
};

int32 start_af_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 stop_af_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s (cam_id:%d)\n", __func__, cam_id);

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 cancel_af_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 start_wb_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int32 stop_wb_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int32 start_ae_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 stop_ae_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp;
	SensorId_t sensor_id;
//	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
};

int32 set_scene_mode_imp(pvoid context, enum camera_id cid,
			 enum isp_scene_mode mode)
{
	struct isp_context *isp;
	struct sensor_info *sif;
	scene_mode_table_header *sv;
	enum fw_cmd_resp_stream_id stream_id;
	result_t result;

	if (!is_para_legal(context, cid) || (mode >= ISP_SCENE_MODE_MAX)) {
		isp_dbg_print_err("%s fail bad para,cid:%d,mode %s\n",
			__func__, cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp->sensor_info[cid].scene_tbl == NULL) {
		isp_dbg_print_err("%s fail no calib,cid:%d,mode %s\n",
			__func__, cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_FAIL;
	};

	sv = isp->sensor_info[cid].scene_tbl;
	isp->sensor_info[cid].scene_mode = mode;
	if (!sv->sm_valid[mode]) {
		isp_dbg_print_err("%s fail mode not support,cid:%d, %s\n",
			__func__, cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s, cid:%d, mode %s\n", __func__, cid,
		isp_dbg_get_scene_mode_str(mode));

	stream_id = isp_get_stream_id_from_cid(isp, cid);

	{
		CmdAeSetSetPoint_t para;

		if (sv->ae_sp[mode] == -1)
			para.setPoint = (uint32) -1;
		else
			para.setPoint = sv->ae_sp[mode] * 1000;
		result =
			isp_send_fw_cmd(isp, CMD_ID_AE_SET_SETPOINT,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info("%s set ae point %d suc\n",
				__func__, sv->ae_sp[mode]);
		} else {
			isp_dbg_print_info("%s set ae point %d fail\n",
				__func__, sv->ae_sp[mode]);
		};
	};

	{
		CmdAeSetIso_t para;

		if (sv->ae_iso[mode] == -1)
			para.iso.iso = AE_ISO_AUTO;
		else
			para.iso.iso = sv->ae_iso[mode];
		result =
			isp_send_fw_cmd(isp, CMD_ID_AE_SET_ISO,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info("%s set ae_iso %d suc\n",
				__func__, sv->ae_iso[mode]);
		} else {
			isp_dbg_print_info("%s set ae_iso %d fail\n",
				__func__, sv->ae_iso[mode]);
		};
	};

	if ((sv->ae_fps[mode][0] != -1)
		&& (sv->ae_fps[mode][1] != -1)) {
		CmdAeSetItimeRange_t para = {0};

		para.minItime = 0;

		if (sv->ae_maxitime[mode] > 0)
			para.maxItime =
				(uint32) (min((int32)sv->ae_maxitime[mode],
				(int32)(1000.0f / sv->ae_fps[mode][0])) * 990);
		else
			para.maxItime =
				(uint32) (1000.0f / sv->ae_fps[mode][0] * 990);
		if (isp->drv_settings.low_light_fps)
			para.maxItimeLowLight =
				1000000 / isp->drv_settings.low_light_fps;

		result =
			isp_send_fw_cmd(isp, CMD_ID_AE_SET_ITIME_RANGE,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info
	("set ae_fps [%u,%u] suc,itime range normal(%d,%d),lowlight(%d,%d)\n",
				sv->ae_fps[mode][0], sv->ae_fps[mode][1],
				para.minItime, para.maxItime,
				para.minItimeLowLight, para.maxItimeLowLight);
		} else {
			isp_dbg_print_info("set ae_fps [%u,%u] fail\n",
				sv->ae_fps[mode][0], sv->ae_fps[mode][1]);
		};
	} else {
		isp_dbg_print_info("no need set ae_fps\n");
	};

	if (isp->drv_settings.awb_default_auto != 0) {
		// User explicitly disabled AWB
		CmdAwbSetMode_t para;

		if (sv->awb_index[mode] == -1)
			para.mode = AWB_MODE_AUTO;
		else
			para.mode = AWB_MODE_MANUAL;
		result =
			isp_send_fw_cmd(isp, CMD_ID_AWB_SET_MODE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info
				("set awb %d suc\n", sv->ae_iso[mode]);
		} else {
			isp_dbg_print_info
				("set awb %d fail\n", sv->ae_iso[mode]);
		};

		if (sv->awb_index[mode] != -1) {
			CmdAwbSetLight_t light;

			light.lightIndex = (uint8) sv->awb_index[mode];
			result =
				isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LIGHT,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&light, sizeof(light));
			if (result == RET_SUCCESS) {
				isp_dbg_print_info("set awb light %d suc\n",
					sv->ae_iso[mode]);
			} else {
				isp_dbg_print_info("set awb light%d fail\n",
					sv->ae_iso[mode]);
			};
		};
	};

	sif = &isp->sensor_info[cid];

	if ((sv->af[mode][0] != -1)
	&& (sv->af[mode][1] != -1)
	&& (sif->sensor_cfg.lens_pos.max_pos != 0)) {
		CmdAfSetLensRange_t para;

		para.lensRange.minLens = sv->af[mode][0];
		para.lensRange.maxLens = sv->af[mode][1];
		para.lensRange.stepLens = 10;
		result = isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_RANGE,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info("set af [%u,%u] suc\n",
				sv->af[mode][0], sv->af[mode][1]);
		} else {
			isp_dbg_print_info("set af [%u,%u] fail\n",
				sv->af[mode][0], sv->af[mode][1]);
		};
	};

	if ((sv->denoise[mode][0] != -1)
	&& (sv->denoise[mode][1] != -1)
	&& (sv->denoise[mode][2] != -1)
	&& (sv->denoise[mode][3] != -1)
	&& (sv->denoise[mode][4] != -1)
	&& (sv->denoise[mode][5] != -1)
	&& (sv->denoise[mode][6] != -1)) {
		CmdFilterSetCurve_t para;

		para.curve.coeff[0] = (uint16) sv->denoise[mode][0];
		para.curve.coeff[1] = (uint16) sv->denoise[mode][1];
		para.curve.coeff[2] = (uint16) sv->denoise[mode][2];
		para.curve.coeff[3] = (uint16) sv->denoise[mode][3];
		para.curve.coeff[4] = (uint16) sv->denoise[mode][4];
		para.curve.coeff[5] = (uint16) sv->denoise[mode][5];
		para.curve.coeff[6] = (uint16) sv->denoise[mode][6];

		result = isp_send_fw_cmd(isp, CMD_ID_FILTER_SET_CURVE,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info("set denoise suc\n");
		} else {
			isp_dbg_print_info("set denoise fail\n");
		};
	};

	if ((sv->cproc[mode][0] >= SATURATION_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][0] <= SATURATION_MAX_VALUE_IN_FW)) {
		fw_if_set_satuation(isp, cid, sv->cproc[mode][0]);
	};

	if ((sv->cproc[mode][1] >= BRIGHTNESS_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][1] <= BRIGHTNESS_MAX_VALUE_IN_FW)) {
		fw_if_set_brightness(isp, cid, sv->cproc[mode][1]);
	};

	if ((sv->cproc[mode][2] >= HUE_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][2] <= HUE_MAX_VALUE_IN_FW)) {
		fw_if_set_hue(isp, cid, sv->cproc[mode][2]);
	};

	if ((sv->cproc[mode][3] >= CONTRAST_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][3] <= CONTRAST_MAX_VALUE_IN_FW)) {
		fw_if_set_contrast(isp, cid, sv->cproc[mode][3]);
	};

	if ((sv->sharpness[mode][0] != -1)
	&& (sv->sharpness[mode][1] != -1)
	&& (sv->sharpness[mode][2] != -1)
	&& (sv->sharpness[mode][3] != -1)
	&& (sv->sharpness[mode][4] != -1)) {
		CmdFilterSetSharpen_t para;

		para.curve.coeff[0] = (int8) sv->sharpness[mode][0];
		para.curve.coeff[1] = (int8) sv->sharpness[mode][1];
		para.curve.coeff[2] = (int8) sv->sharpness[mode][2];
		para.curve.coeff[3] = (int8) sv->sharpness[mode][3];
		para.curve.coeff[4] = (int8) sv->sharpness[mode][4];
		result = isp_send_fw_cmd(isp, CMD_ID_FILTER_SET_SHARPEN,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&para, sizeof(para));
		if (result == RET_SUCCESS) {
			isp_dbg_print_info("set sharpness suc\n");
		} else {
			isp_dbg_print_info("set sharpness fail\n");
		};
	};

	if (sv->flashlight[mode] != 2) {
		CmdAeSetFlashMode_t para;

		if (sv->flashlight[mode] == -1)
			para.mode = FLASH_MODE_AUTO;
		else if (sv->flashlight[mode] == 1)
			para.mode = FLASH_MODE_ON;
		else
			para.mode = FLASH_MODE_OFF;
		result = RET_SUCCESS;
		//todo to be added later

		if (result == RET_SUCCESS) {
			isp_dbg_print_info("set flashlight %d suc\n",
				sv->flashlight[mode]);
		} else {
			isp_dbg_print_info("set flashlight %d fail\n",
				sv->flashlight[mode]);
		};
	} else {
		isp_dbg_print_info("set flashlight %d, ignore\n",
			sv->flashlight[mode]);
	};

	isp_dbg_print_info("<- %s, suc\n", __func__);

	return IMF_RET_SUCCESS;
}

int32 set_gamma_imp(pvoid context, enum camera_id cam_id, int32 value)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("%s fail, bad para,cam_id:%d\n", __func__, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp->sensor_info[cam_id].para_gamma_set = value;
	isp_dbg_print_info
		("-> %s, camera_id %d,set PARA_ID_GAMMA to %d\n",
		__func__, cam_id,
		isp->sensor_info[cam_id].para_gamma_set);

	if (isp->sensor_info[cam_id].para_gamma_cur ==
	isp->sensor_info[cam_id].para_gamma_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return RET_SUCCESS;
	};

	isp->sensor_info[cam_id].para_gamma_cur =
		isp->sensor_info[cam_id].para_gamma_set;

	if (isp_fw_set_gamma(isp, cam_id,
		isp->sensor_info[cam_id].para_gamma_set) != RET_SUCCESS)
		return IMF_RET_FAIL;

	return RET_SUCCESS;
}

int32 set_color_temperature_imp(pvoid context, enum camera_id cam_id,
			uint32 value)
{
	CmdAwbSetColorTemperature_t ct;
	struct isp_context *isp = (struct isp_context *)context;
	enum fw_cmd_resp_stream_id stream_id;

	isp->sensor_info[cam_id].para_color_temperature_set = value;
	isp_dbg_print_info("-> %s,cid %d,set color temp to %d\n", __func__,
		cam_id, isp->sensor_info[cam_id].para_color_temperature_set);

	if (isp->sensor_info[cam_id].para_color_temperature_cur ==
	isp->sensor_info[cam_id].para_color_temperature_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return RET_SUCCESS;
	};

	isp->sensor_info[cam_id].para_color_temperature_cur =
		isp->sensor_info[cam_id].para_color_temperature_set;

	ct.colorTemperature =
		isp->sensor_info[cam_id].para_color_temperature_set;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_COLOR_TEMPERATURE,
	stream_id, FW_CMD_PARA_TYPE_DIRECT, &ct, sizeof(ct)) != RET_SUCCESS)
		return IMF_RET_FAIL;

	return RET_SUCCESS;
}


int32 set_snr_calib_data_imp(pvoid context, enum camera_id cam_id,
			     pvoid data, uint32 len)
{
	struct isp_context *isp;

	result_t result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)
	|| (data == NULL)
	|| (len == 0)) {
		isp_dbg_print_err
			("%s fail for illegal para,context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	/*to be added later*/

	if (result != RET_SUCCESS) {
		isp_dbg_print_err("%s (cam_id:%d) fail for send cmd\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s (cam_id:%d) suc\n", __func__, cam_id);
	return IMF_RET_SUCCESS;
};

int32 map_handle_to_vram_addr_imp(pvoid context, void *handle,
				uint64 *vram_addr)
{
	struct isp_context *isp;

	if ((context == NULL)
	|| (handle == NULL)
	|| (vram_addr == NULL)) {
		isp_dbg_print_err("%s fail para,isp %p,hdl %p,addr %p\n",
			__func__, context, handle, vram_addr);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;

	isp_dbg_print_err("%s fail to be implemented\n", __func__);
	return IMF_RET_FAIL;
};

int32 set_per_frame_ctl_info_imp(pvoid context, enum camera_id cam_id,
				struct frame_ctl_info *ctl_info,
				sys_img_buf_handle_t zsl_buf)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	struct isp_picture_buffer_param buf_para;
	sys_img_buf_handle_t buf_hdl = zsl_buf;
	result_t result;
	int32 ret = IMF_RET_FAIL;
	enum stream_id str_id = STREAM_ID_ZSL;
	uint32 y_len;
	uint32 u_len;
	uint32 v_len;
	CmdFrameControl_t fc_cmd;
	enum fw_cmd_resp_stream_id stream;
	struct sensor_info *sif;

/*	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_RC,
 *			PT_CMD_FC, PT_STATUS_SUCC,
 *			zsl_buf ? zsl_buf->virtual_addr : 0);
 */

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (ctl_info == NULL) || (buf_hdl->virtual_addr == 0)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid %d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!isp_get_pic_buf_param
	(&isp->sensor_info[cam_id].str_info[str_id], &buf_para)) {
		isp_dbg_print_err("%s fail for get para\n", __func__);
		return IMF_RET_FAIL;
	};

	y_len = buf_para.channel_a_height * buf_para.channel_a_stride;
	u_len = buf_para.channel_b_height * buf_para.channel_b_stride;
	v_len = buf_para.channel_c_height * buf_para.channel_c_stride;

	if (buf_hdl->len < (y_len + u_len + v_len)) {
		isp_dbg_print_err("%s fail small buf,%u need,real %u\n",
			__func__, (y_len + u_len + v_len), buf_hdl->len);
		return IMF_RET_FAIL;
	} else if (buf_hdl->len > (y_len + u_len + v_len)) {
		isp_dbg_print_info("%s,in buf len(%u)>y+u+v(%u)\n",
			__func__, buf_hdl->len, (y_len + u_len + v_len));
		if (v_len != 0)
			v_len = buf_hdl->len - y_len - u_len;
		else if (u_len != 0)
			u_len = buf_hdl->len - y_len;
		else
			y_len = buf_hdl->len;
	};

	stream = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("%s,cid %d,sid:%d\n", __func__, cam_id, str_id);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_info("%sp,fail fsm %d\n",
			__func__, ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		isp_dbg_print_err
			("%s fail for sys_img_buf_handle_cpy\n", __func__);
		goto quit;
	};

	gen_img = isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
				cam_id, str_id, y_len, u_len, v_len);

	/*isp_dbg_show_map_info(gen_img); */
	if (gen_img == NULL) {
		isp_dbg_print_err("%s fail for isp_map_sys_2_mc\n", __func__);
		ret = IMF_RET_FAIL;
		goto quit;
	}
	isp_dbg_print_info("%s,y:u:v %u:%u:%u,addr:%p(%u) map to %llx\n",
		__func__, y_len, u_len, v_len,
		buf_hdl->virtual_addr, buf_hdl->len,
		gen_img->y_map_info.mc_addr);

	init_frame_ctl_cmd(sif, &fc_cmd.frameControl, ctl_info, gen_img);
	isp_get_str_out_prop(&isp->sensor_info[cam_id].str_info[str_id],
				&fc_cmd.frameControl.zslBuf.imageProp);
	if (fc_cmd.frameControl.zslBuf.enabled)
		isp_dbg_show_img_prop("frame_ctl1",
				&fc_cmd.frameControl.zslBuf.imageProp);
	else
		isp_dbg_print_err("fail:zsl buf not enable\n");
	result = isp_send_fw_cmd(isp, CMD_ID_SEND_FRAME_CONTROL, stream,
				FW_CMD_PARA_TYPE_INDIRECT, &fc_cmd,
				sizeof(fc_cmd));
	if (result != RET_SUCCESS) {
		isp_dbg_print_err("%s fail for isp_send_fw_cmd\n", __func__);
		goto quit;
	};
	isp->sensor_info[cam_id].str_info[str_id].buf_num_sent++;
	isp_list_insert_tail(
		&isp->sensor_info[cam_id].str_info[str_id].buf_in_fw,
		(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
quit:

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (buf_hdl)
			sys_img_buf_handle_free(buf_hdl);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
	};

/*	isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" "[%p]\n", PT_EVT_SC,
 *				PT_CMD_FC, PT_STATUS_SUCC,
 *				zsl_buf ? zsl_buf->virtual_addr : 0);
 */
	return ret;
};


int32 set_drv_settings_imp(pvoid context, struct driver_settings *setting)
{
	struct isp_context *isp = context;

	if ((context == NULL)
	|| (setting == NULL)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	memcpy(&isp->drv_settings, setting, sizeof(isp->drv_settings));
	//just add to test acpi function
	//isp_acpi_set_sensor_pwr(isp, CAMERA_ID_FRONT_LEFT, 1);
	//isp_acpi_fch_clk_enable(isp, 0);
	dbg_show_drv_settings(&isp->drv_settings);
	g_log_level_ispm = isp->drv_settings.drv_log_level;
	return IMF_RET_SUCCESS;
};

int32 enable_dynamic_frame_rate_imp(pvoid context, enum camera_id cam_id,
				bool enable)
{
	struct isp_context *isp = context;
	result_t ret;
	CmdAeSetItimeRange_t i_time = {0};
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp_dbg_print_info("%s,cid %d,enable %d\n",
		__func__, cam_id, enable);

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp->sensor_info[cam_id].enable_dynamic_frame_rate = 1;
		isp_dbg_print_info("%s, set later for fw not run\n", __func__);
		return IMF_RET_SUCCESS;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];

	i_time.minItime = sif->sensor_cfg.exposure.min_integration_time;
	i_time.maxItime = sif->sensor_cfg.exposure.max_integration_time;

	if (enable && isp->drv_settings.low_light_fps) {
		i_time.maxItimeLowLight =
			1000000 / isp->drv_settings.low_light_fps;
	};


	if ((i_time.maxItime == 0) || (i_time.minItime > i_time.maxItime)
	|| (i_time.maxItimeLowLight &&
	(i_time.maxItime > i_time.maxItimeLowLight))) {
		isp_dbg_print_err
		("%s fail bad itime range normal(%d,%d),lowlight(%d,%d)\n",
			__func__, i_time.minItime, i_time.maxItime,
			i_time.minItimeLowLight, i_time.maxItimeLowLight);
		ret = RET_FAILURE;
	} else if (isp_send_fw_cmd(isp,
				CMD_ID_AE_SET_ITIME_RANGE,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&i_time, sizeof(i_time)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail set i_time\n", __func__);
		ret = RET_FAILURE;
	} else {
		isp_dbg_print_info("%s itime range (%d,%d),lowlight(%d,%d)\n",
			__func__, i_time.minItime, i_time.maxItime,
			i_time.minItimeLowLight, i_time.maxItimeLowLight);
		ret = RET_SUCCESS;
	};

	if (ret == RET_SUCCESS) {
		isp_dbg_print_info("<- %s suc\n", __func__);
		return IMF_RET_SUCCESS;
	}

	isp_dbg_print_info("<- %s fail\n", __func__);
	return IMF_RET_FAIL;
}

int32 set_max_frame_rate_imp(pvoid context, enum camera_id cam_id,
			enum stream_id sid, uint32 frame_rate_numerator,
			uint32 frame_rate_denominator)
{
	struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (frame_rate_denominator == 0)
	|| ((frame_rate_numerator / frame_rate_denominator) == 0)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	sif = &isp->sensor_info[cam_id];
	if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
		isp_dbg_print_err("%s fail bad stream status,%d\n",
			__func__, sif->str_info[sid].start_status);
		return IMF_RET_FAIL;
	};
	sif->str_info[sid].max_fps_numerator = frame_rate_numerator;
	sif->str_info[sid].max_fps_denominator = frame_rate_denominator;
	isp_dbg_print_info("%s suc, cid %d, sid %d\n", __func__, cam_id, sid);

	return IMF_RET_SUCCESS;
};

int32 set_flicker_imp(pvoid context, enum camera_id cam_id,
		enum anti_banding_mode anti_banding)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (anti_banding == 0)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (fw_if_set_anti_banding_mode(isp, cam_id, anti_banding) !=
		RET_SUCCESS)
		return IMF_RET_FAIL;

	isp_dbg_print_info("%s suc, cid %d\n", __func__, cam_id);

	return IMF_RET_SUCCESS;
}

int32 set_iso_priority_imp(pvoid context, enum camera_id cam_id,
			enum stream_id sid, AeIso_t *iso)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (iso == NULL)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	isp_dbg_print_err("%s fail bad stream status,%d\n",
	 *		__func__, sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *};
	 */
	isp->sensor_info[cam_id].para_iso_set = *iso;

	if (isp->sensor_info[cam_id].para_iso_cur.iso ==
	isp->sensor_info[cam_id].para_iso_set.iso) {
		isp_dbg_print_info("not diff value, do none\n");
		return IMF_RET_SUCCESS;
	};

	isp->sensor_info[cam_id].para_iso_cur =
		isp->sensor_info[cam_id].para_iso_set;

	if (fw_if_set_iso(isp, cam_id,
	isp->sensor_info[cam_id].para_iso_cur) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc, cid %d, sid %d\n", __func__, cam_id, sid);

	return IMF_RET_SUCCESS;
}

int32 set_ev_compensation_imp(pvoid context, enum camera_id cam_id,
			AeEv_t *ev)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ev == NULL)) {
		isp_dbg_print_err("%s fail bad para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	isp_dbg_print_err("%s fail bad stream status,%d\n",
	 *		__func__, sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *};
	 */
	isp->sensor_info[cam_id].para_ev_compensate_set = *ev;

	isp_dbg_print_info
		("-> %s,cid %d,set ev to %u/%u\n", __func__, cam_id,
	isp->sensor_info[cam_id].para_ev_compensate_set.numerator,
	isp->sensor_info[cam_id].para_ev_compensate_set.denominator);

	/*
	 *if (
	 *(isp->sensor_info[cam_id].para_ev_compensate_cur.numerator *
	 *isp->sensor_info[cam_id].para_ev_compensate_set.denominator) ==
	 *(isp->sensor_info[cam_id].para_ev_compensate_set.numerator *
	 *isp->sensor_info[cam_id].para_ev_compensate_cur.denominator)) {
	 *	isp_dbg_print_info("not diff value, do none\n");
	 *	return RET_SUCCESS;
	 *}
	 */

	isp->sensor_info[cam_id].para_ev_compensate_cur =
		isp->sensor_info[cam_id].para_ev_compensate_set;

	if (fw_if_set_ev_compensation(isp, cam_id,
		&isp->sensor_info[cam_id].para_ev_compensate_cur) !=
		RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("%s suc, cid %d\n", __func__, cam_id);

	return IMF_RET_SUCCESS;
}


int32 set_lens_range_imp(pvoid context, enum camera_id cam_id,
			uint32 near_value, uint32 far_value)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	if ((near_value > far_value) || (far_value > FOCUS_MAX_VALUE)) {
		isp_dbg_print_err("%s fail bad para,cid:%d,[%u,%u]\n",
			__func__, cam_id, near_value, far_value);
		return IMF_RET_INVALID_PARAMETER;
	};

	near_value =
		IspMapValueFromRange(FOCUS_MIN_VALUE, FOCUS_MAX_VALUE,
				near_value,
				sif->lens_range.minLens,
				sif->lens_range.maxLens);
	far_value =
		IspMapValueFromRange(FOCUS_MIN_VALUE, FOCUS_MAX_VALUE,
				far_value,
				sif->lens_range.minLens,
				sif->lens_range.maxLens);

	isp_dbg_print_info("%s, cid %d, [%u, %u]\n",
		__func__, cam_id, near_value, far_value);
	if (isp_fw_set_lens_range(isp, cam_id,
				near_value, far_value) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_fw_set_lens_range\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 set_flash_mode_imp(pvoid context, enum camera_id cam_id, FlashMode_t mode)
{
	struct isp_context *isp;
	CmdAeSetFlashMode_t flash_mode;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	flash_mode.mode = mode;
	isp_dbg_print_info("%s, cid %d, %s\n", __func__,
		cam_id, isp_dbg_get_flash_mode_str(mode));
	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLASH_MODE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&flash_mode, sizeof(flash_mode)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	sif->flash_mode = mode;
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_flash_power_imp(pvoid context, enum camera_id cam_id, uint32 pwr)
{
	struct isp_context *isp;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	CmdAeSetFlashPowerLevel_t pwr_level;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (pwr > 100) {
		isp_dbg_print_err("%s fail bad para,cid:%d, pwr %u\n",
			__func__, cam_id, pwr);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	pwr_level.powerLevel = pwr;
	isp_dbg_print_info("%s, cid %d, pwr %u\n", __func__, cam_id, pwr);

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLASH_POWER_LEVEL,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&pwr_level, sizeof(pwr_level)) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	sif->flash_pwr = pwr;
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_red_eye_mode_imp(pvoid context, enum camera_id cam_id,
				RedEyeMode_t mode)
{
	struct isp_context *isp;
	CmdAeSetRedEyeMode_t cmd;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	cmd.mode = mode;
	isp_dbg_print_info("-> %s, cid %d, %s\n", __func__,
		cam_id, isp_dbg_get_red_eye_mode_str(mode));

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_RED_EYE_MODE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&cmd, sizeof(cmd)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	sif->redeye_mode = mode;
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_flash_assist_mode_imp(pvoid context, enum camera_id cam_id,
				FocusAssistMode_t mode)
{
	struct isp_context *isp;
	CmdAfSetFocusAssistMode_t flash_mode;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	flash_mode.mode = mode;
	isp_dbg_print_info("-> %s, cid %d, %s\n", __func__,
		cam_id, isp_dbg_get_flash_assist_mode_str(mode));
	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_FOCUS_ASSIST_MODE,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			&flash_mode, sizeof(flash_mode)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	sif->flash_assist_mode = mode;
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_flash_assist_power_imp(pvoid context, enum camera_id cam_id,
				uint32 pwr)
{
	struct isp_context *isp;
	struct sensor_info *sif;
	enum fw_cmd_resp_stream_id stream_id;
	CmdAfSetFocusAssistPowerLevel_t pwr_level;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail bad para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (pwr > 100) {
		isp_dbg_print_err("-><- %s fail bad para,cid:%d, pwr %u\n",
			__func__, cam_id, pwr);
		return IMF_RET_INVALID_PARAMETER;
	};

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	pwr_level.powerLevel = pwr;
	isp_dbg_print_info("-> %s, cid %d, pwr %u\n", __func__, cam_id, pwr);

	if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_FOCUS_ASSIST_POWER_LEVEL,
			stream_id, FW_CMD_PARA_TYPE_DIRECT, &pwr_level,
			sizeof(pwr_level)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	sif->flash_assist_pwr = pwr;
	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_video_hdr_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("%s fail bad para,isp:%p,cid:%d,enable:%d\n",
			__func__, context, cam_id, enable);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	sif->hdr_enable = (char)enable;
	isp_dbg_print_info("-><- %s suc,isp:%p,cid:%d,enable:%d\n",
		__func__, context, cam_id, enable);
	return IMF_RET_SUCCESS;
};

int32 set_lens_shading_matrix_imp(pvoid context, enum camera_id cam_id,
				LscMatrix_t *matrix)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (matrix == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LSC_MATRIX,
			stream_id, FW_CMD_PARA_TYPE_INDIRECT,
			matrix, sizeof(LscMatrix_t)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 set_lens_shading_sector_imp(pvoid context, enum camera_id cam_id,
				PrepLscSector_t *sectors)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (sectors == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LSC_SECTOR,
			stream_id, FW_CMD_PARA_TYPE_INDIRECT,
			sectors,
			sizeof(CmdAwbSetLscSector_t)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 set_awb_cc_matrix_imp(pvoid context, enum camera_id cam_id,
				CcMatrix_t *matrix)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (matrix == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_CC_MATRIX,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			matrix, sizeof(CcMatrix_t)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 set_awb_cc_offset_imp(pvoid context, enum camera_id cam_id,
				CcOffset_t *ccOffset)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (ccOffset == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_CC_OFFSET,
			stream_id, FW_CMD_PARA_TYPE_DIRECT,
			ccOffset, sizeof(CcOffset_t)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 gamma_enable_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, enable ? CMD_ID_GAMMA_ENABLE :
			CMD_ID_GAMMA_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 wdr_enable_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, enable ? CMD_ID_WDR_ENABLE :
			CMD_ID_WDR_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 tnr_enable_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (enable && !isp->sensor_info[cam_id].tnr_tmp_buf_set) {
		isp_dbg_print_err("<- %s, enable fail for buf\n", __func__);
		return IMF_RET_FAIL;
	}

	if (isp_send_fw_cmd(isp, enable ? CMD_ID_TNR_ENABLE :
			CMD_ID_TNR_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 snr_enable_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, enable ? CMD_ID_SNR_ENABLE :
			CMD_ID_SNR_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 dpf_enable_imp(pvoid context, enum camera_id cam_id, uint32 enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (isp_send_fw_cmd(isp, enable ? CMD_ID_DPF_ENABLE :
			CMD_ID_DPF_DISABLE, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for send cmd\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 get_cproc_status_imp(pvoid context, enum camera_id cam_id,
				CprocStatus_t *cproc)
{
	struct isp_context *isp = (struct isp_context *)context;
	result_t ret;

	if (!is_para_legal(context, cam_id) || (cproc == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d,cproc %p\n",
			__func__, context, cam_id, cproc);
		return IMF_RET_INVALID_PARAMETER;
	};

	ret = isp_fw_get_cproc_status(isp, cam_id, cproc);
	isp_dbg_print_info("-> %s, cid:%d\n", __func__, cam_id);

	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_fw_get_cproc_status\n", __func__);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 cproc_enable_imp(pvoid context, enum camera_id cam_id,
			uint32 enable)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	isp->sensor_info[cam_id].para_color_enable_set = enable;

	if (fw_if_set_color_enable(isp, cam_id, enable) != RET_SUCCESS) {
		isp_dbg_print_err
		("%s fail for fw_if_set_color_enable,cam_id:%d, en:%d\n",
			__func__, cam_id, enable);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 cproc_set_contrast_imp(pvoid context, enum camera_id cam_id,
			uint32 contrast)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p, cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	isp->sensor_info[cam_id].para_contrast_set = contrast;

	if (isp->sensor_info[cam_id].para_contrast_cur ==
	isp->sensor_info[cam_id].para_contrast_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_contrast_cur =
		isp->sensor_info[cam_id].para_contrast_set;

	if (fw_if_set_contrast(isp, cam_id,
	isp->sensor_info[cam_id].para_contrast_set) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
}

int32 cproc_set_brightness_imp(pvoid context, enum camera_id cam_id,
			uint32 brightness)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p, cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	isp->sensor_info[cam_id].para_brightness_set = brightness;

	if (isp->sensor_info[cam_id].para_brightness_cur ==
	isp->sensor_info[cam_id].para_brightness_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_brightness_cur =
		isp->sensor_info[cam_id].para_brightness_set;

	if (fw_if_set_brightness(isp, cam_id,
	isp->sensor_info[cam_id].para_brightness_set) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 cproc_set_saturation_imp(pvoid context, enum camera_id cam_id,
			uint32 saturation)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p, cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	isp->sensor_info[cam_id].para_satuaration_set = saturation;

	if (isp->sensor_info[cam_id].para_satuaration_cur ==
	isp->sensor_info[cam_id].para_satuaration_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_satuaration_cur =
		isp->sensor_info[cam_id].para_satuaration_set;

	if (fw_if_set_satuation(isp, cam_id,
	isp->sensor_info[cam_id].para_satuaration_set) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 cproc_set_hue_imp(pvoid context, enum camera_id cam_id,
			uint32 hue)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p, cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s, cam %d\n", __func__, cam_id);

	isp->sensor_info[cam_id].para_hue_set = hue;

	if (isp->sensor_info[cam_id].para_hue_cur ==
	isp->sensor_info[cam_id].para_hue_set) {
		isp_dbg_print_info("not diff value, do none\n");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_hue_cur =
		isp->sensor_info[cam_id].para_hue_set;

	if (fw_if_set_hue(isp, cam_id,
	isp->sensor_info[cam_id].para_hue_set) != RET_SUCCESS) {
		isp_dbg_print_err("%s fail,cam_id:%d\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 get_init_para_imp(pvoid context, void *sw_init,
			void *hw_init,
			void *isp_env)
{
	if (isp_env)
		*((enum isp_environment_setting *)isp_env) = g_isp_env_setting;
	isp_dbg_print_info("-><- %s suc\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 lfb_resv_imp(pvoid context, uint32 size, uint64 *sys, uint64 *mc)
{
	struct isp_context *isp = context;
//	enum cgs_result ret;
	struct isp_gpu_mem_info *ifb_mem_info = NULL;

	if ((isp == NULL)
	|| (size == 0)
	|| (g_cgs_srv->ops->alloc_gpu_mem == NULL)
	|| (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		isp_dbg_print_err("-><- %s fail no functions\n", __func__);
		return IMF_RET_FAIL;
	};

	isp_dbg_print_info("-> %s size %uM, isp %p\n", __func__,
		size / (1024 * 1024), context);

	if ((isp->fb_buf.gpu_mc_addr != 0)
	&& (isp->fb_buf.sys_addr != NULL)) {
		if (sys)
			*sys = (uint64)isp->fb_buf.sys_addr;
		if (mc)
			*mc = isp->fb_buf.gpu_mc_addr;
		isp_dbg_print_info
			("<- %s,succ already,mc %llx,mem %p,size %lluM\n",
			__func__,
			isp->fb_buf.gpu_mc_addr, isp->fb_buf.sys_addr,
			isp->fb_buf.mem_size / (1024 * 1024));
		return IMF_RET_SUCCESS;
	};


	ifb_mem_info = isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

	if (ifb_mem_info) {
		isp_dbg_print_info
		("%s isp_gpu_mem_alloc(NLFB) succ,mc:%llx, sys:%p,len:%lld\n",
			__func__,
			ifb_mem_info->gpu_mc_addr,
			ifb_mem_info->sys_addr,
			ifb_mem_info->mem_size);
	} else {
		isp_dbg_print_err
			("%s isp_gpu_mem_alloc(NLFB) fail\n", __func__);
		return IMF_RET_FAIL;
	};


	isp->fb_buf.mem_src = ISP_GPU_MEM_TYPE_NLFB;
	isp->fb_buf.mem_size = ifb_mem_info->mem_size;
	isp->fb_buf.handle = ifb_mem_info->handle;
	isp->fb_buf.gpu_mc_addr = ifb_mem_info->gpu_mc_addr;
	isp->fb_buf.sys_addr = ifb_mem_info->sys_addr;
	isp->fb_buf.mem_handle = ifb_mem_info->mem_handle;
	isp->fb_buf.acc_handle = ifb_mem_info->acc_handle;

	if (sys)
		*sys = (uint64)isp->fb_buf.sys_addr;
	if (mc)
		*mc = isp->fb_buf.gpu_mc_addr;
	isp_dbg_print_info("<- %s,succ, mc %llu, sys %llu, size %lluM\n", __func__,
		*mc, *sys, isp->fb_buf.mem_size / (1024 * 1024));

	return IMF_RET_SUCCESS;
};

int32 lfb_free_imp(pvoid context)
{
	struct isp_context *isp = context;
	enum cgs_result ret;

	if ((isp == NULL)
	|| (g_cgs_srv->ops->alloc_gpu_mem == NULL)
	|| (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		isp_dbg_print_err("-><- %s fail no functions\n", __func__);
		return IMF_RET_FAIL;
	};

	if ((isp->fb_buf.sys_addr == 0)
	|| (isp->fb_buf.gpu_mc_addr == 0)
	|| (isp->fb_buf.mem_size == 0)) {
		isp_dbg_print_err("-><- %s fail no reserv info\n", __func__);
		return IMF_RET_FAIL;
	};

	isp_dbg_print_info("-> %s size %llu, sys %p, mc %llx\n", __func__,
		isp->fb_buf.mem_size,
		isp->fb_buf.sys_addr,
		isp->fb_buf.gpu_mc_addr);

	ret = isp_gpu_mem_free(&(isp->fb_buf));
	if (ret != CGS_RESULT__OK) {
		isp_dbg_print_err("in %s,isp_gpu_mem_free fail 0x%x\n",
			__func__, ret);
	};
	memset(&isp->fb_buf, 0, sizeof(struct isp_gpu_mem_info));
	isp_dbg_print_info("<- %s size\n", __func__);

	return IMF_RET_SUCCESS;
};

uint32 get_index_from_res_fps_id(pvoid context, enum camera_id cid,
				 uint32 res_fps_id)
{
	uint32 i = 0;
	struct isp_context *isp = (struct isp_context *)context;

	for (i = 0; i < isp->sensor_info[cid].res_fps.count; i++) {
		if ((uint32) res_fps_id ==
		isp->sensor_info[cid].res_fps.res_fps[i].index) {
			break;
		}

		continue;
	}

	return i;
};

int32 get_raw_img_info_imp(pvoid context, enum camera_id cam_id,
			uint32 *w, uint32 *h, uint32 *raw_type,
			uint32 *raw_pkt_fmt)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	uint32 l_w;
	uint32 l_h;
	uint32 l_raw_type = 1;
	uint32 l_raw_pkt_fmt = 0;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	sif = &isp->sensor_info[cam_id];
	l_w = sif->asprw_wind.h_size;
	l_h = sif->asprw_wind.v_size;

	if (sif->sensor_cfg.prop.intfType == SENSOR_INTF_TYPE_MIPI) {
		MipiDataType_t data_type =
			sif->sensor_cfg.prop.intfProp.mipi.dataType;
		switch (data_type) {
		case MIPI_DATA_TYPE_RAW_12:
			l_raw_type = 2;
			break;
		case MIPI_DATA_TYPE_RAW_10:
			l_raw_type = 1;
			break;
		case MIPI_DATA_TYPE_RAW_8:
			l_raw_type = 0;
			break;
		default:
			/*will never get to here */
			isp_dbg_print_err
				("in %s bad mipi datatype %u for cid:%d\n",
				__func__, data_type, cam_id);
			break;
		}
	} else if (sif->sensor_cfg.prop.intfType ==
	SENSOR_INTF_TYPE_PARALLEL) {
		ParallelDataType_t data_type =
			sif->sensor_cfg.prop.intfProp.parallel.dataType;
		switch (data_type) {
		case PARALLEL_DATA_TYPE_RAW12:
			l_raw_type = 2;
			break;
		case PARALLEL_DATA_TYPE_RAW10:
			l_raw_type = 1;
			break;
		case PARALLEL_DATA_TYPE_RAW8:
			l_raw_type = 0;
			break;
		default:
			/*will never get to here */
			isp_dbg_print_err
				("in %s bad paral datatype %u for cid:%d\n",
				__func__, data_type, cam_id);
			break;
		};
	}

	l_raw_pkt_fmt = sif->raw_pkt_fmt;
	if (w)
		*w = l_w;
	if (h)
		*h = l_h;
	if (raw_type)
		*raw_type = l_raw_type;
	if (raw_pkt_fmt)
		*raw_pkt_fmt = l_raw_pkt_fmt;

	isp_dbg_print_info
		("-><- %s suc cid:%d,w:%u,h:%u,rawType:%u,rawPktFmt:%u\n",
		__func__, cam_id, l_w, l_h, l_raw_type, l_raw_pkt_fmt);
	return IMF_RET_SUCCESS;

};


int32 fw_cmd_send_imp(pvoid context, enum camera_id cam_id,
			uint32 cmd, uint32 is_set_cmd,
			uint16 is_stream_cmd, uint16 to_ir,
			uint32 is_direct_cmd,
			pvoid para, uint32 *para_len)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	result_t ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	void *package = NULL;
	uint32 package_size = 0;
	uint32 resp_by_pl = 1;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (para && ((para_len == NULL) || (*para_len == 0))) {
		isp_dbg_print_err("-><- %s fail para len,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (to_ir && (sif->cam_type == CAMERA_TYPE_RGBIR)) {
		isp_dbg_print_info(
		"in %s change streamid from %u to %u in toir case\n", __func__,
		stream_id, isp_get_stream_id_from_cid(isp, CAMERA_ID_MEM));
		stream_id = isp_get_stream_id_from_cid(isp, CAMERA_ID_MEM);
	};


	package = para;
	if (para_len)
		package_size = *para_len;
	if (is_set_cmd) {
		ret = isp_send_fw_cmd(isp, cmd, is_stream_cmd ? stream_id :
			FW_CMD_RESP_STREAM_ID_GLOBAL,
			is_direct_cmd ?
			FW_CMD_PARA_TYPE_DIRECT : FW_CMD_PARA_TYPE_INDIRECT,
			package, package_size);
		goto quit;
	} else if (para == NULL) {
		isp_dbg_print_err("%s fail null para for get,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	//process cmd which is to receive value
	switch (cmd) {
	case CMD_ID_GET_FW_VERSION:
		*((uint32 *) para) = isp->isp_fw_ver;
		ret = RET_SUCCESS;
		goto quit;
	case CMD_ID_AWB_GET_STATUS:
	case CMD_ID_AWB_GET_LSC_MATRIX:	//CmdAwbGetLscMatrix_t
	case CMD_ID_AWB_GET_LSC_SECTOR:	//CmdAwbGetLscSector_t
	case CMD_ID_AE_GET_STATUS:	//CmdAeGetStatus_t
	case CMD_ID_AF_GET_STATUS:	//CmdAfGetStatus_t
	case CMD_ID_DEGAMMA_GET_STATUS:	//CmdDegammaGetStatus_t
	case CMD_ID_DPF_GET_STATUS:	//CmdDpfGetStatus_t
	case CMD_ID_DPCC_GET_STATUS:	//CmdDpccGetStatus_t
	case CMD_ID_CAC_GET_STATUS:	//CmdCacGetStatus_t
	case CMD_ID_WDR_GET_STATUS:	//CmdWdrGetStatus_t
	case CMD_ID_CNR_GET_STATUS:	//CmdCnrGetStatus_t
	case CMD_ID_FILTER_GET_STATUS:	//CmdFilterGetStatus_t
	case CMD_ID_GAMMA_GET_STATUS:   //GammaStatus_t
	case CMD_ID_STNR_GET_STATUS: //StnrStatus_t

	case CMD_ID_AWB_GET_WB_GAIN:	//WbGain_t
	case CMD_ID_AWB_GET_CC_MATRIX:	//CcMatrix_t
	case CMD_ID_AWB_GET_CC_OFFSET:	//CcOffset_t
	case CMD_ID_AE_GET_ISO_CAPABILITY:	//AeIsoCapability_t
	case CMD_ID_AE_GET_EV_CAPABILITY:	//AeEvCapability_t
	case CMD_ID_AE_GET_SETPOINT:	//CmdAeGetSetpoint_t
	case CMD_ID_AE_GET_ISO_RANGE:	//AeIsoRange_t
	case CMD_ID_CPROC_GET_STATUS:	//CprocStatus_t
	case CMD_ID_BLS_GET_STATUS:	//BlsStatus_t
	case CMD_ID_DEMOSAIC_GET_THRESH:	//DemosaicThresh_t
		resp_by_pl = 1;
		break;
	default:
		resp_by_pl = 1;
		break;

	};

	if (resp_by_pl) {
		if (!is_direct_cmd) {
			isp_dbg_print_err
		("%s fail none direct for get cmd,cid:%d,cmd %s(0x%08x)\n",
			__func__, cam_id, isp_dbg_get_cmd_str(cmd), cmd);
			return IMF_RET_INVALID_PARAMETER;
		}

		ret = isp_send_fw_cmd_sync(isp, cmd,
			is_stream_cmd ?
			stream_id : FW_CMD_RESP_STREAM_ID_GLOBAL,
			is_direct_cmd ?
			FW_CMD_PARA_TYPE_DIRECT : FW_CMD_PARA_TYPE_INDIRECT,
			NULL, 0, 1000 * 10, para, para_len);
	} else {
		struct isp_gpu_mem_info *resp_buf;
		CmdAwbGetStatus_t pkg;
		uint32 *pbuf;

		if (sif->cmd_resp_buf == NULL) {
			sif->cmd_resp_buf =
				isp_gpu_mem_alloc(CMD_RESPONSE_BUF_SIZE,
						ISP_GPU_MEM_TYPE_NLFB);
		}

		resp_buf = sif->cmd_resp_buf;
		if (resp_buf == NULL) {
			isp_dbg_print_err
			("-><- %s fail aloc respbuf,cid:%d,cmd %s(0x%08x)\n",
			__func__, cam_id, isp_dbg_get_cmd_str(cmd), cmd);
			return IMF_RET_INVALID_PARAMETER;
		};

		pbuf = (uint32 *) resp_buf->sys_addr;
		if (pbuf) {
			pbuf[0] = 0x11223344;
			pbuf[1] = 0x55667788;
			pbuf[2] = 0x99aabbcc;
			pbuf[3] = 0xddeeff00;
			isp_dbg_print_info("%s:0x%x 0x%x 0x%x 0x%x\n",
				__func__, pbuf[0], pbuf[1], pbuf[2], pbuf[3]);
		}
		isp_split_addr64(resp_buf->gpu_mc_addr, &pkg.bufAddrLo,
				&pkg.bufAddrHi);
		pkg.bufSize = *para_len;
		ret = isp_send_fw_cmd_sync(isp, cmd,
			is_stream_cmd ?
			stream_id : FW_CMD_RESP_STREAM_ID_GLOBAL,
			is_direct_cmd ?
			FW_CMD_PARA_TYPE_DIRECT : FW_CMD_PARA_TYPE_INDIRECT,
			&pkg, sizeof(pkg), 1000 * 10, NULL, 0);
		if (ret != RET_SUCCESS)
			goto quit;
		if (resp_buf->sys_addr) {
			memcpy(para, resp_buf->sys_addr,
				min((uint32)*para_len,
				(uint32)CMD_RESPONSE_BUF_SIZE));
			isp_dbg_print_info("%s:%llx,0x%x 0x%x 0x%x 0x%x\n",
				__func__, resp_buf->gpu_mc_addr, pbuf[0],
				pbuf[1], pbuf[2], pbuf[3]);
		};
	}

quit:
	if (ret != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail cid:%d,cmd %s(0x%08x),is_set_cmd %u,",
			__func__, cam_id,
			isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
		isp_dbg_print_err
			("is_stream_cmd %u,toIR %u,is_direct_cmd %u,para %p\n",
			is_stream_cmd, to_ir, is_direct_cmd, para);
		return IMF_RET_FAIL;
	}

	isp_dbg_print_info
		("%s suc cid:%d,cmd %s(0x%08x),is_set_cmd %u,",
		__func__, cam_id,
		isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
	isp_dbg_print_info
		("is_stream_cmd %u,toIR %u,is_direct_cmd %u,para %p\n",
		is_stream_cmd, to_ir, is_direct_cmd, para);
	if (cmd == CMD_ID_AE_GET_STATUS)
		isp_dbg_dump_ae_status(para);

	return IMF_RET_SUCCESS;
};

int32 get_tuning_data_info_imp(pvoid context, enum camera_id cam_id,
				uint32 *struct_ver, uint32 *date,
				char *author, char *cam_name)
{
	struct isp_context *isp = (struct isp_context *)context;
	package_td_header *ph;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};
	ph = (package_td_header *) isp->calib_data[cam_id];
	if (ph == NULL) {
		isp_dbg_print_err("-><- %s fail for cid:%d, no calib\n",
			__func__, cam_id);
		return IMF_RET_FAIL;
	};
	if ((ph->magic_no_high != 0x33ffffff)
	|| (ph->magic_no_low != 0xffffffff)) {
		isp_dbg_print_err("-><- %s fail for cid:%d, old calib\n",
		__func__, cam_id);
		return IMF_RET_FAIL;
	}

	if (struct_ver)
		*struct_ver = ph->struct_rev;
	if (date)
		*date = ph->date;
	if (author)
		memcpy(author, ph->author, 32);
	if (cam_name)
		memcpy(cam_name, ph->cam_name, 32);

	isp_dbg_print_info
		("-><- %s suc,struct_rev:%u,date:%u,author:%s,cam_name:%s\n",
		__func__, ph->struct_rev, ph->date, ph->author, ph->cam_name);
	return IMF_RET_SUCCESS;

};

int32 loopback_start_imp(pvoid context, enum camera_id cam_id,
			uint32 yuv_format, uint32 yuv_width,
			uint32 yuv_height, uint32 yuv_ypitch,
			uint32 yuv_uvpitch)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	struct isp_stream_info *str_info;
	enum stream_id stream = STREAM_ID_PREVIEW;
	enum pvt_img_fmt fmt;
	struct pvt_img_res_fps_pitch pitch;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	sif = &isp->sensor_info[cam_id];
	str_info = &sif->str_info[stream];

	isp_dbg_print_info
		("-> %s,cid:%u,format:%u,w:%u,h:%u,yPitch:%u,uvPitch:%u\n",
		__func__, cam_id, yuv_format, yuv_width,
		yuv_height, yuv_ypitch, yuv_uvpitch);

	switch (yuv_format) {
	case IMAGE_FORMAT_NV12:
		fmt = PVT_IMG_FMT_NV12;
		break;
	case IMAGE_FORMAT_NV21:
		fmt = PVT_IMG_FMT_NV21;
		break;
	case IMAGE_FORMAT_YV12:
		fmt = PVT_IMG_FMT_YV12;
		break;
	case IMAGE_FORMAT_I420:
		fmt = PVT_IMG_FMT_I420;
		break;
	case IMAGE_FORMAT_YUV422PLANAR:
		fmt = PVT_IMG_FMT_YUV422P;
		break;
	case IMAGE_FORMAT_YUV422SEMIPLANAR:
		fmt = PVT_IMG_FMT_YUV422_SEMIPLANAR;
		break;
	case IMAGE_FORMAT_YUV422INTERLEAVED:
		fmt = PVT_IMG_FMT_YUV422_INTERLEAVED;
		break;
	default:
		isp_dbg_print_err("<- %s fail format %u\n",
			__func__, yuv_format);
		return IMF_RET_INVALID_PARAMETER;
	}

	set_stream_para_imp
		(context, cam_id, stream, PARA_ID_DATA_FORMAT, &fmt);

	pitch.width = yuv_width;
	pitch.height = yuv_height;
	pitch.fps = 30;
	pitch.luma_pitch = yuv_ypitch;
	pitch.chroma_pitch = yuv_uvpitch;

	set_stream_para_imp
		(context, cam_id, stream, PARA_ID_DATA_RES_FPS_PITCH, &pitch);

	sif->cam_type = CAMERA_TYPE_MEM;

	isp_init_loopback_camera_info(isp, cam_id);
	if (open_camera_imp(isp, cam_id, 0, 0) != IMF_RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail open_camera_imp\n", __func__);
		return IMF_RET_FAIL;
	};
	if (start_stream_imp(context, cam_id, stream, false)
						!= IMF_RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail start stream\n", __func__);
		return IMF_RET_FAIL;

	};

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;

}

int32 loopback_stop_imp(pvoid context, enum camera_id cam_id)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	struct isp_stream_info *str_info;
	enum stream_id stream = STREAM_ID_PREVIEW;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	sif = &isp->sensor_info[cam_id];
	str_info = &sif->str_info[stream];

	isp_dbg_print_info("-> %s,cid:%u\n", __func__, cam_id);

	if (stop_stream_imp(context, cam_id, stream, false) !=
		IMF_RET_SUCCESS) {
		isp_dbg_print_err("in %s stop_stream_imp fail\n",
				__func__);
	}
	if (close_camera_imp(isp, cam_id) != IMF_RET_SUCCESS) {
		isp_dbg_print_err("in %s close_camera_imp fail\n", __func__);
	};
	isp_uninit_loopback_camera_info(isp, cam_id);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;

}

int32 loopback_set_raw_imp(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t raw_buf,
				sys_img_buf_handle_t frame_info)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	struct isp_stream_info *str_info;
	Mode4FrameInfo_t *fi;
	enum stream_id stream = STREAM_ID_PREVIEW;
	uint32 raw_size;

	if (!is_para_legal(context, cam_id)
	|| (raw_buf == NULL)
	|| (raw_buf->virtual_addr == NULL)
	|| (frame_info == NULL)
	|| (frame_info->virtual_addr == NULL)) {
		isp_dbg_print_err
			("-><- %s fail para,isp:%p,cid:%d,raw %p,info %p\n",
			__func__, context, cam_id, raw_buf, frame_info);
		return IMF_RET_INVALID_PARAMETER;
	};

	sif = &isp->sensor_info[cam_id];
	str_info = &sif->str_info[stream];
	fi = (Mode4FrameInfo_t *) frame_info->virtual_addr;
	raw_size = fi->rawBufferFrameInfo.imageProp.width *
		fi->rawBufferFrameInfo.imageProp.height * 2;
	if (raw_size != raw_buf->len) {
		isp_dbg_print_err("%s cid:%d,fail rawsize %u(%u wanted)\n",
			__func__, cam_id, raw_buf->len, raw_size);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s,cid:%u,rawsize %u\n",
		__func__, cam_id, raw_size);
	isp_alloc_loopback_buf(isp, cam_id, raw_buf->len);
	if ((sif->loopback_raw_buf == NULL)
	|| (sif->loopback_raw_info == NULL)) {
		isp_dbg_print_err("-><- %s fail alloc buf\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}
	memcpy(sif->loopback_raw_buf->sys_addr, raw_buf->virtual_addr,
		raw_size);
	memcpy(sif->loopback_raw_info->sys_addr, frame_info->virtual_addr,
		sizeof(Mode4FrameInfo_t));

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;

}

int32 loopback_process_raw_imp(pvoid context, enum camera_id cam_id,
				uint32 sub_tuning_cmd_count,
				struct loopback_sub_tuning_cmd *sub_tuning_cmd,
				sys_img_buf_handle_t buf,
				uint32 yuv_buf_len, uint32 meta_buf_len)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	//struct isp_stream_info *str_info;
	//Mode4FrameInfo_t *fi;
	enum stream_id stream = STREAM_ID_PREVIEW;

	uint32 i;

	meta_buf_len = meta_buf_len;
	yuv_buf_len = yuv_buf_len;
	if (!is_para_legal(context, cam_id)
	|| (buf == NULL)
	|| (buf->virtual_addr == NULL)) {
		isp_dbg_print_err("-><- %s fail para,isp:%p,cid:%d,buf %p\n",
			__func__, context, cam_id, buf);
		return IMF_RET_INVALID_PARAMETER;
	};
	sif = &isp->sensor_info[cam_id];
	if ((sif->loopback_raw_buf == NULL)
	|| (sif->loopback_raw_info == NULL)) {
		isp_dbg_print_err("-><- %s fail no raw set\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_dbg_print_info("-> %s,cid:%u,cmdCnt %u\n", __func__,
		cam_id, sub_tuning_cmd_count);

	for (i = 0; i < sub_tuning_cmd_count; i++) {
		fw_cmd_send_imp(isp, cam_id,
				sub_tuning_cmd[i].fw_cmd,
				1,
				(uint16)sub_tuning_cmd[i].is_stream_channel,
				0,
				sub_tuning_cmd[i].is_direct,
				sub_tuning_cmd[i].param,
				&sub_tuning_cmd[i].param_len);

	};

	if (set_stream_buf_imp(isp, cam_id, buf, stream) != IMF_RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for set_stream_buf\n", __func__);
		return IMF_RET_FAIL;

	};

	if (loopback_set_raw_info_2_fw(isp, cam_id, sif->loopback_raw_buf,
	sif->loopback_raw_info) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for setRawInfo\n", __func__);
		return IMF_RET_FAIL;

	};

	isp_dbg_print_info("<- %s suc\n", __func__);
	return IMF_RET_SUCCESS;

}

int32 set_face_authtication_mode_imp(pvoid context, enum camera_id cam_id,
					bool_t mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		isp_dbg_print_err
			("%s fail for illegal para, context:%p,cam_id:%d\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	isp_dbg_print_info("-> %s, cam %d, mode %s\n", __func__, cam_id,
		mode == TRUE ? "Enabled" : "Disabled");

	isp->sensor_info[cam_id].face_auth_mode = mode;

	isp_dbg_print_info("<- %s, succ\n", __func__);
	return IMF_RET_SUCCESS;
};

int32 get_isp_work_buf_imp(pvoid context, pvoid work_buf_info)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (work_buf_info == NULL)) {
		isp_dbg_print_err("-><- %s, fail para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	if (&isp->work_buf == work_buf_info) {
		isp_dbg_print_err("-><- %s, fail isp_context\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	};
	memcpy(work_buf_info, &isp->work_buf, sizeof(isp->work_buf));

	isp_dbg_print_info("-><- %s, suc\n", __func__);

	return IMF_RET_SUCCESS;
}
