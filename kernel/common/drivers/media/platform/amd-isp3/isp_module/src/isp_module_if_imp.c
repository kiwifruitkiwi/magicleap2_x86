/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "amd_stream.h"
#include "isp_common.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "buffer_mgr.h"
#include "isp_fw_if.h"
#include "i2c.h"
#include "log.h"
#include "isp_para_capability.h"
#include "isp_soc_adpt.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_module_if_imp]"

const unsigned int sqrtmap[] = {
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


int isp_get_zoom_window(unsigned int orig_w, unsigned int orig_h,
				unsigned int zoom,
				struct _Window_t *zoom_wnd)
{
	/*
	 *
	 * X*Y = zoom*X'*Y'
	 * X/Y = X'/Y'
	 * ==>  X'=X/sqrt(zoom), Y'=Y/sqrt(zoom)
	 *
	 */
	int ret = RET_FAILURE;

	if ((zoom >= 100) && (zoom <= 400) && zoom_wnd) {
		/*calcluate current width and height */
		unsigned int w = orig_w * 100000 / sqrtmap[zoom];
		unsigned int h = orig_h * 100000 / sqrtmap[zoom];
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
			ISP_PR_INFO("suc,%u:%u zoom %u [%u,%u,%u,%u]",
				    orig_w, orig_h, zoom, zoom_wnd->h_offset,
				    zoom_wnd->v_offset, zoom_wnd->h_size,
				    zoom_wnd->v_size);
		} else {
			ISP_PR_ERR("fail orig %u:%u zoom %u", orig_w, orig_h,
				   zoom);
		}
	} else {
		ISP_PR_ERR("fail bad zoom %u or wnd %p", zoom, zoom_wnd);
	}

	return ret;
}

void ref_no_op(void __maybe_unused *context)
{
}

void deref_no_op(void    __maybe_unused *context)
{
}

void isp_module_if_init_ext(struct isp_isp_module_if *p_interface)
{
	ISP_PR_INFO("inf %p", p_interface);
	if (p_interface) {
		p_interface->get_init_para = get_init_para_imp;
		p_interface->get_isp_work_buf = get_isp_work_buf_imp;
	}
	EXIT();
}

void isp_module_if_init(struct isp_isp_module_if *p_interface)
{
	ENTER();
	if (p_interface) {
	/*rtl_zero_memory(p_interface, sizeof(struct isp_isp_module_if)); */
		//memset(p_interface, 0, sizeof(struct isp_isp_module_if));
		p_interface->size = sizeof(struct isp_isp_module_if);
		p_interface->version = ISP_MODULE_IF_VERSION;
		p_interface->context = &g_isp_context;

		ISP_PR_INFO("interface_context %p", p_interface->context);
		p_interface->if_reference = ref_no_op;
		p_interface->if_dereference = deref_no_op;
		p_interface->set_fw_bin = set_fw_bin_imp;
		p_interface->set_calib_bin = set_calib_bin_imp;
		p_interface->de_init = de_init;
		p_interface->open_camera = open_camera_imp;
		p_interface->open_camera_fps = open_camera_fps_imp;
		p_interface->close_camera = close_camera_imp;
		p_interface->start_preview = start_preview_imp;
		p_interface->stop_preview = stop_preview_imp;
		p_interface->set_preview_buf = set_preview_buf_imp;
		p_interface->reg_notify_cb = reg_notify_cb_imp;
		p_interface->unreg_notify_cb = unreg_notify_cb_imp;
		p_interface->i2c_read_mem = i2c_read_mem_imp;
		p_interface->i2c_write_mem = i2c_write_mem_imp;
		p_interface->i2c_read_reg = i2c_read_reg_imp;
		p_interface->i2c_write_reg = i2c_write_reg_imp;
		p_interface->i2c_write_reg_group = i2c_write_reg_imp_group;
		p_interface->reg_snr_op = reg_snr_op_imp;
		p_interface->unreg_snr_op = unreg_snr_op_imp;
		p_interface->take_one_pic = take_one_pic_imp;
		p_interface->stop_take_one_pic = stop_take_one_pic_imp;
		p_interface->online_isp_tune = online_tune_imp;
		p_interface->isp_reg_write32 = write_isp_reg32_imp;
		p_interface->isp_reg_read32 = read_isp_reg32_imp;
		p_interface->set_common_para = set_common_para_imp;
		p_interface->get_common_para = get_common_para_imp;
		p_interface->set_preview_para = set_preview_para_imp;
		p_interface->get_prev_para = get_preview_para_imp;
		p_interface->set_video_para = set_video_para_imp;
		p_interface->get_video_para = get_video_para_imp;
		p_interface->set_zsl_para = set_zsl_para_imp;
		p_interface->get_zsl_para = get_zsl_para_imp;
		p_interface->set_video_buf = set_video_buf_imp;
		p_interface->start_video = start_video_imp;
		p_interface->start_stream = start_stream_imp;
		p_interface->set_test_pattern = set_test_pattern_imp;
		p_interface->switch_profile = switch_profile_imp;
		p_interface->stop_video = stop_video_imp;
		p_interface->get_capabilities = get_capabilities_imp;
		p_interface->snr_clk_set = snr_clk_set_imp;
		p_interface->get_camera_res_fps = get_camera_res_fps_imp;
		p_interface->take_raw_pic = take_raw_pic_imp;
		p_interface->stop_take_raw_pic = stop_take_raw_pic_imp;
		p_interface->auto_exposure_lock = auto_exposure_lock_imp;
		p_interface->auto_exposure_unlock = auto_exposure_unlock_imp;
		p_interface->cproc_enable = cproc_enable_imp;
		p_interface->cproc_set_contrast = cproc_set_contrast_imp;
		p_interface->cproc_set_brightness = cproc_set_brightness_imp;
		p_interface->cproc_set_saturation = cproc_set_saturation_imp;
		p_interface->cproc_set_hue = cproc_set_hue_imp;
		p_interface->auto_wb_lock = auto_wb_lock_imp;
		p_interface->auto_wb_unlock = auto_wb_unlock_imp;
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
		p_interface->set_zsl_buf = set_zsl_buf_imp;
		p_interface->start_zsl = start_zsl_imp;
		p_interface->stop_zsl = stop_zsl_imp;
		p_interface->set_digital_zoom_ratio =
		    set_digital_zoom_ratio_imp;
		p_interface->set_digital_zoom_pos = set_digital_zoom_pos_imp;
		p_interface->set_metadata_buf = set_metadata_buf_imp;
		p_interface->get_camera_dev_info = get_camera_dev_info_imp;
		p_interface->get_caps_data_format = get_caps_data_format_imp;
		p_interface->get_caps = get_caps_imp;
		p_interface->set_wb_light = set_wb_light_imp;
		p_interface->set_wb_colorT = set_wb_colorT_imp;
		p_interface->set_wb_gain = set_wb_gain_imp;
		p_interface->disable_color_effect = disable_color_effect_imp;
		p_interface->set_awb_roi = set_awb_roi_imp;
		p_interface->set_ae_roi = set_ae_roi_imp;
		p_interface->start_ae = start_ae_imp;
		p_interface->stop_ae = stop_ae_imp;
		p_interface->start_wb = start_wb_imp;
		p_interface->stop_wb = stop_wb_imp;
		p_interface->set_scene_mode = set_scene_mode_imp;
		p_interface->set_snr_calib_data = set_snr_calib_data_imp;

		p_interface->map_handle_to_vram_addr =
		    map_handle_to_vram_addr_imp;
		p_interface->set_drv_settings = set_drv_settings_imp;
		p_interface->enable_dynamic_frame_rate =
		    enable_dynamic_frame_rate_imp;
		p_interface->set_max_frame_rate = set_max_frame_rate_imp;
		p_interface->set_frame_rate_info = set_frame_rate_info_imp;
		p_interface->set_iso_priority = set_iso_priority_imp;
		p_interface->set_flicker = set_flicker_imp;
		p_interface->set_ev_compensation = set_ev_compensation_imp;
		p_interface->sharpen_enable = sharpen_enable_imp;
		p_interface->sharpen_disable = sharpen_disable_imp;
		p_interface->sharpen_config = sharpen_config_imp;
		p_interface->clk_gating_enable = clk_gating_enable_imp;
		p_interface->power_gating_enable = power_gating_enable_imp;
		p_interface->dump_raw_enable = dump_raw_enable_imp;
		p_interface->image_effect_enable = image_effect_enable_imp;
		p_interface->set_video_hdr = set_video_hdr_imp;
		p_interface->set_photo_hdr = set_video_hdr_imp;

		p_interface->set_lens_shading_matrix =
						set_lens_shading_matrix_imp;
		p_interface->set_lens_shading_sector =
						set_lens_shading_sector_imp;
		p_interface->set_awb_cc_matrix = set_awb_cc_matrix_imp;
		p_interface->set_awb_cc_offset = set_awb_cc_offset_imp;
		p_interface->gamma_enable = gamma_enable_imp;
		p_interface->tnr_enable = tnr_enable_imp;
		p_interface->snr_enable = snr_enable_imp;
		p_interface->snr_config = snr_config_imp;
		p_interface->tnr_config = tnr_config_imp;
		p_interface->get_cproc_status = get_cproc_status_imp;
		p_interface->fw_cmd_send = fw_cmd_send_imp;
		p_interface->set_frame_ctrl = set_frame_ctrl_imp;
		p_interface->start_cvip_sensor = start_cvip_sensor_imp;
	}
	EXIT();
}

/*corresponding to aidt_ctrl_open_board*/
int set_fw_bin_imp(void *context, void *fw_data, unsigned int fw_len)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((isp == NULL)
	|| (fw_data == NULL)
	|| (fw_len == 0)) {
		ISP_PR_ERR("fail for illegal parameter");
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->fw_len && isp->fw_data) {
		ISP_PR_INFO("suc, do none for already done");
		return IMF_RET_SUCCESS;
	}

	isp->fw_len = fw_len;
	if (isp->fw_data) {
		isp_sys_mem_free(isp->fw_data);
		isp->fw_data = NULL;
	}

	isp->fw_data = isp_sys_mem_alloc(fw_len);
	if (isp->fw_data == NULL) {
		ISP_PR_ERR("fail for aloc fw buf,len %u", isp->fw_len);
		goto quit;
	} else {
		memcpy(isp->fw_data, fw_data, isp->fw_len);
		ISP_PR_INFO("fw %p,len %u", isp->fw_data, isp->fw_len);
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;

quit:

	return IMF_RET_FAIL;
}

int set_calib_bin_imp(void *context, enum camera_id cam_id,
			void *calib_data, unsigned int len)
{
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id)
	|| (calib_data == NULL)
	|| (len == 0)) {
		ISP_PR_ERR("fail for illegal parameter");
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->sensor_info[cam_id].status != START_STATUS_NOT_START) {
		ISP_PR_ERR("cam[%d] fail for bad status %d",
			   cam_id, isp->sensor_info[cam_id].status);
		return IMF_RET_FAIL;
	}

	uninit_cam_calib_item(isp, cam_id);
	init_cam_calib_item(isp, cam_id);

	ISP_PR_INFO("cid[%d]", cam_id);

	if (!parse_calib_data(isp, cam_id, calib_data, len)) {
		ISP_PR_ERR("fail, bad calib %p, len %u", calib_data, len);
		goto quit;
	} else {
		ISP_PR_INFO("suc,good calib, %p, len %u", calib_data, len);
	}

	return IMF_RET_SUCCESS;

quit:
	uninit_cam_calib_item(isp, cam_id);
	return IMF_RET_FAIL;
}

void de_init(void __maybe_unused *context)
{
	unit_isp();
}
enum camera_id get_actual_cid(void *context, enum camera_id cid)
{
	struct isp_context *isp = (struct isp_context *)context;

	return isp->sensor_info[cid].actual_cid;
}

int open_camera_imp(void *context, enum camera_id cid,
			unsigned int res_fps_id, uint32_t flag)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct camera_dev_info cam_dev_info;
	unsigned int pix_size;
	unsigned int res_fps;
	unsigned int index;
	enum camera_id actual_cid = cid;

	if (cid == CAMERA_ID_MEM)
		actual_cid = get_actual_cid(context, cid);

	if (!is_para_legal(context, cid)) {
		ISP_PR_ERR("fail for para");
		return IMF_RET_INVALID_PARAMETER;
	}

#ifndef USING_PSP_TO_LOAD_ISP_FW
	if (isp->fw_data == NULL) {
		ISP_PR_ERR("fail for no fw");
		return IMF_RET_FAIL;
	}
#endif

	/*res_fps_id = isp_get_res_fps_id_for_str(isp,cid, str_id); */
	index = get_index_from_res_fps_id(context, actual_cid, res_fps_id);

	if (ISP_GET_STATUS(isp) == ISP_STATUS_UNINITED) {
		ISP_PR_ERR("cid[%d] fail for isp uninit", actual_cid);
		return IMF_RET_FAIL;
	}

	if ((cid < CAMERA_ID_MEM) && (flag & OPEN_CAMERA_FLAG_HDR)
	&& (!isp->sensor_info[cid].res_fps.res_fps[index].hdr_support)) {
		ISP_PR_ERR("cid[%d] fpsid[%d] fail for hdr open not support",
			   cid, res_fps_id);
		return IMF_RET_FAIL;
	}

	if ((cid < CAMERA_ID_MEM) && (!(flag & OPEN_CAMERA_FLAG_HDR))
	&& (!isp->sensor_info[cid].res_fps.res_fps[index].none_hdr_support)) {
		ISP_PR_ERR("cid[%d] fpsid[%d] fail for normal open not support",
			   cid, res_fps_id);
		return IMF_RET_FAIL;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (isp->sensor_info[actual_cid].sensor_opened ||
		isp->sensor_info[cid].sensor_opened) {
		goto init_stream;
	}

	if (is_camera_started(isp, actual_cid)) {
		isp_mutex_unlock(&isp->ops_mutex);
		if (actual_cid < CAMERA_ID_MEM) {
			if ((isp->sensor_info[actual_cid].cur_res_fps_id !=
			     (char)res_fps_id) ||
			    (isp->sensor_info[actual_cid].open_flag != flag)) {
				ISP_PR_ERR("cid[%d] fail for incompatible",
					   actual_cid);
				ISP_PR_ERR("fps id:%u->%u, flag:%u->%u)",
				    isp->sensor_info[actual_cid].cur_res_fps_id,
				    res_fps_id,
				    isp->sensor_info[actual_cid].open_flag,
				    flag);
				return IMF_RET_FAIL;
			}

			ISP_PR_INFO("cid[%d] suc for already", actual_cid);
			return IMF_RET_SUCCESS;

		} else if (cid == CAMERA_ID_MEM) {
			if ((res_fps_id !=
				isp->sensor_info[actual_cid].raw_width)
				|| (flag !=
				isp->sensor_info[actual_cid].raw_height)) {
				ISP_PR_ERR("fail for incompatible");
				ISP_PR_ERR("w: %u->%u, h: %u->%u)",
					isp->sensor_info[actual_cid].raw_width,
					res_fps_id,
					isp->sensor_info[actual_cid].raw_height,
					flag);
				return IMF_RET_FAIL;
			}

			ISP_PR_INFO("cid[%d] suc for already", actual_cid);
			return IMF_RET_SUCCESS;
		}
	}

	if (actual_cid < CAMERA_ID_MEM) {
		ISP_PR_INFO("cid[%d] fpsid[%d]  flag:0x%x", actual_cid,
			    res_fps_id, flag);
	} else {
		ISP_PR_INFO("cid[%d] fpsid(w):%d, flag(h):%u", actual_cid,
			    res_fps_id, flag);
	}
	if (actual_cid < CAMERA_ID_MEM) {
		isp->sensor_info[actual_cid].cam_type = CAMERA_TYPE_RGB_BAYER;
		if (IMF_RET_SUCCESS ==
		    get_camera_dev_info_imp(isp, actual_cid, &cam_dev_info)) {
			isp->sensor_info[actual_cid].cam_type =
				cam_dev_info.type;
		} else {
			ISP_PR_ERR("get_camera_dev_info_imp fail!");
		}
		pix_size =
		isp->sensor_info[actual_cid].res_fps.res_fps[index].width
		* isp->sensor_info[actual_cid].res_fps.res_fps[index].height;
		res_fps =
			isp->sensor_info[actual_cid].res_fps.res_fps[index].fps;
		if (isp->sensor_info[actual_cid].cam_type == CAMERA_TYPE_RGBIR)
			g_rgbir_raw_count_in_fw = 0;
	}
	if (cid == CAMERA_ID_MEM) {

		isp->sensor_info[cid].cam_type = CAMERA_TYPE_MEM;
		isp->sensor_info[cid].res_fps.res_fps[index].width =
								res_fps_id;
		isp->sensor_info[cid].res_fps.res_fps[index].height = flag;
		isp->sensor_info[cid].res_fps.res_fps[index].fps =
			isp->sensor_info[actual_cid].res_fps.res_fps[index].fps;
		//pix_size = res_fps_id * flag;
		//res_fps = 30;
	}

	if (isp_ip_pwr_on(isp, actual_cid, index,
		flag & OPEN_CAMERA_FLAG_HDR) != RET_SUCCESS) {
		ISP_PR_ERR("isp_ip_pwr_on fail");
		goto fail;
	}


	if (isp_dphy_pwr_on(isp) != RET_SUCCESS) {
		ISP_PR_ERR("isp_dphy_pwr_on fail");
		goto fail;
	}
#ifdef OUTPUT_FW_LOG_TO_FILE
	open_fw_log_file();
#endif
	if (isp_fw_start(isp) != RET_SUCCESS) {
		ISP_PR_ERR("isp_fw_start fail");
		goto fail;
	}

		if (1) {//to do check for isp3.0
			//to do change sensor id, don't hard code
			if (isp_snr_pwr_toggle(isp, cid, 1) != RET_SUCCESS) {
				ISP_PR_ERR("isp_snr_pwr_toggle fail");
				goto fail;
			}

			if (isp_snr_clk_toggle(isp, cid, 1) != RET_SUCCESS) {
				ISP_PR_ERR("isp_snr_clk_toggle fail");
				goto fail;
			}

			if (isp_snr_open(isp, actual_cid, res_fps_id, flag)
				!= RET_SUCCESS) {
				ISP_PR_ERR("isp_snr_open fail");
				goto fail;
			} else {
				isp_hw_reg_write32(mmISP_GPIO_PRIV_LED, 0xff);
				ISP_PR_INFO("GPIO access is success!");
			}
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
	}


#ifdef SET_P_STAGE_WATERMARK_REGISTER
	// Program watermark registers
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_Y = 0x628c0;
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CB = 0x628c4;
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CR = 0x628c8;
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_Y = 0x62cc0;
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CB = 0x62cc4;
	unsigned int MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CR = 0x62cc8;

	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_Y,
			0x10087);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CB,
			0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI0_PSTATE_CTRL_CR,
			0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_Y,
			0x10087);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CB,
			0x10080);
	isp_hw_reg_write32(MI_WR_MIPI0_ISP_MIPI_MI_MIPI1_PSTATE_CTRL_CR,
			0x10080);
#endif

#ifdef SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER
//1.DAGB0_WRCLI12/15/16/17 set as same as DAGB0_WRCLI10. (FE5FE0F8)
//2.MMEA0_DRAM_WR_CLI2GRP_MAP0->CID10_GROUP/CID11_GROUP/
//CID12_GROUP/CID15_GROUP
//	set to 0.
//3.MMEA0_DRAM_WR_CLI2GRP_MAP1->CID16_GROUP/CID17_GROUP set to 0.
#ifdef SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER_SWRT
	unsigned int DAGB0_WRCLI10 = 0x00068128;
	unsigned int DAGB0_WRCLI11 = 0x0006812C;
	unsigned int DAGB0_WRCLI12 = 0x00068130;
	unsigned int DAGB0_WRCLI15 = 0x0006813C;
	unsigned int DAGB0_WRCLI16 = 0x00068140;
	unsigned int DAGB0_WRCLI17 = 0x00068144;
	unsigned int MMEA0_DRAM_WR_CLI2GRP_MAP0 = 0x00068408;
	unsigned int MMEA0_DRAM_WR_CLI2GRP_MAP1 = 0x0006840C;
	unsigned int reg_value;
//1.DAGB0_WRCLI12/15/16/17 set as same as DAGB0_WRCLI10. (FE5FE0F8)
//2.MMEA0_DRAM_WR_CLI2GRP_MAP0->CID10_GROUP/CID11_GROUP/
//CID12_GROUP/CID15_GROUP
//	set to 0.
//3.MMEA0_DRAM_WR_CLI2GRP_MAP1->CID16_GROUP/CID17_GROUP set to 0.
	ISP_PR_INFO("SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER");
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
		const unsigned int shiftBits = 12;
	//const unsigned int VMC_TAP_CONTEXT0_PDE_REQUEST_SNOOP = 8;
	//const unsigned int VMC_TAP_CONTEXT0_PTE_REQUEST_SNOOP = 11;
		unsigned int reg_value;
		unsigned long long reg_value2;
		unsigned int DAGB0_WRCLI6 = 0x0006811C;
		unsigned int DAGB0_WRCLI7 = 0x00068120;
		unsigned int DAGB0_WRCLI8 = 0x00068124;
		unsigned int DAGB0_WRCLI10 = 0x0006812C;
		unsigned int DAGB0_WRCLI11 = 0x00068130;
		unsigned int DAGB0_WRCLI14 = 0x0006813C;
		unsigned int DAGB0_WRCLI15 = 0x00068140;
		unsigned int DAGB0_WRCLI16 = 0x00068144;
		unsigned int DAGB0_WRCLI17 = 0x00068148;
		unsigned int DAGB0_WRCLI21 = 0x00068158;
		unsigned int DAGB0_WRCLI22 = 0x0006815C;
		unsigned int MMEA0_DRAM_WR_CLI2GRP_MAP0 = 0x00068408;
		unsigned int MMEA0_DRAM_WR_CLI2GRP_MAP1 = 0x0006840C;
		unsigned int VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32 =
			0x0006A500;
		unsigned int VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32
			= 0x0006A504;
		unsigned int VM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32
			= 0x0006A508;
		unsigned int VM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32
			= 0x0006A50C;
		unsigned int VM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32
			= 0x0006A510;
		unsigned int VM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32
			= 0x0006A514;

		unsigned int VM_L2_SAW_CNTL = 0x000699D8;
		unsigned int VM_L2_SAW_CNTL2 = 0x000699DC;
		unsigned int VM_L2_SAW_CNTL3 = 0x000699E0;
		unsigned int VM_L2_SAW_CNTL4 = 0x000699E4;
		unsigned int VM_L2_SAW_CONTEXT0_CNTL = 0x000699E8;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32 =
			0x000699F0;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32 =
			0x000699F4;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_LO32 =
			0x000699F8;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_START_ADDR_HI32 =
			0x000699FC;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_LO32
			= 0x00069A00;
		unsigned int VM_L2_SAW_CONTEXT0_PAGE_TABLE_END_ADDR_HI32
			= 0x00069A04;

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
		reg_value2 = (unsigned long long)
			isp_hw_reg_read32
			(VM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32) <<
			32 | (unsigned long long) reg_value;
		isp_hw_reg_write32(
			VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			(unsigned int) (reg_value2 >> shiftBits));
		isp_hw_reg_write32(
			VM_L2_SAW_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			(unsigned int) (reg_value2 >> (shiftBits + 32)));

		// MMHUB DAGB setup
		isp_hw_reg_write32(DAGB0_WRCLI6, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI7, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI8, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI10, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI11, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI14, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI15, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI16, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI17, 0xfe5feeeb);
		isp_hw_reg_write32(DAGB0_WRCLI21, 0xfe5feaa8);
		isp_hw_reg_write32(DAGB0_WRCLI22, 0xfe5feaa8);
		isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP0, 0xf5f7f885);
		isp_hw_reg_write32(MMEA0_DRAM_WR_CLI2GRP_MAP1, 0x4095415f);
	}
#endif
#endif

init_stream:
	if (isp_init_stream(isp, cid) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_init_stream");
		goto fail;
	}

	//Put ISP DS4 bit to busy
	isp_program_mmhub_ds_reg(TRUE);
	isp->sensor_info[actual_cid].sensor_opened = 1;
	isp->sensor_info[cid].sensor_opened = 1;
	isp_mutex_unlock(&isp->ops_mutex);
	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	close_camera_imp(isp, cid);
	RET(IMF_RET_FAIL);
	return IMF_RET_FAIL;
}

int open_camera_fps_imp(void *context, enum camera_id cid,
			unsigned int fps, uint32_t flag)
{
	struct isp_context *isp = (struct isp_context *)context;
	unsigned int i = 0;

	for (i = 0; i < isp->sensor_info[cid].res_fps.count; i++) {
		if (fps ==
			isp->sensor_info[cid].res_fps.res_fps[i].fps) {
			break;
		}

		continue;
	}

	return open_camera_imp(context, cid, i, flag);
}

int close_camera_imp(void *context, enum camera_id cid)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	unsigned int cnt;
	unsigned int index;
	struct sensor_info *snr_info;

	struct isp_cmd_element *ele = NULL;
	int tmpi;

	if (!is_para_legal(context, cid)) {
		ISP_PR_ERR("fail for para");
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_INFO("cid[%d]", cid);
	if (isp_uninit_stream(isp, cid) != RET_SUCCESS) {
		isp_mutex_unlock(&isp->ops_mutex);
		ISP_PR_INFO("cid[%d] fail by isp_uninit_stream", cid);
		return IMF_RET_FAIL;
	}
	sif = &isp->sensor_info[cid];
	if (sif->status == START_STATUS_STARTED) {
		ISP_PR_ERR("fail, stream still running");
		goto fail;
	}
	sif->status = START_STATUS_NOT_START;

	if ((cid < CAMERA_ID_MEM)
	&& (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM)) {
		isp_snr_close(isp, cid);
		isp_snr_pwr_toggle(isp, cid, 0);
		// no this for ISP3.0 for AIDT CL3940445
		//isp_snr_clk_toggle(isp, cid, 0);	//problematic

	} else {//(cid == CAMERA_ID_MEM)
		//|| (CAMERA_TYPE_MEM == isp->sensor_info[cid].cam_type)
		struct isp_pwr_unit *pwr_unit;

		pwr_unit = &isp->isp_pu_cam[cid];
		isp_mutex_lock(&pwr_unit->pwr_status_mutex);
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	}
	isp->sensor_info[cid].raw_width = 0;
	isp->sensor_info[cid].raw_height = 0;

	cnt = isp_get_started_stream_count(isp);
	if (cnt > 0) {
		ISP_PR_INFO("no need power off isp");
		goto suc;
	}

	ISP_PR_INFO("off privacy led");
	isp_hw_reg_write32(mmISP_GPIO_PRIV_LED, 0);

	ISP_PR_INFO("power off isp");
	isp_hw_ccpu_stall(isp);
	isp_hw_ccpu_disable(isp);
	snr_info = &isp->sensor_info[cid];
	index = get_index_from_res_fps_id
			(isp, cid, snr_info->cur_res_fps_id);
	isp_clk_change(isp, cid, index, snr_info->hdr_enable, false);

	isp_dphy_pwr_off(isp);
	isp_ip_pwr_off(isp);
	for (tmpi = 0; tmpi < MAX_CALIB_NUM_IN_FW; tmpi++)
		if (sif->calib_gpu_mem[tmpi]) {
			isp_gpu_mem_free(sif->calib_gpu_mem[tmpi]);
			sif->calib_gpu_mem[tmpi] = NULL;
		}
	if (sif->m2m_calib_gpu_mem) {
		isp_gpu_mem_free(sif->m2m_calib_gpu_mem);
		sif->m2m_calib_gpu_mem = NULL;
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
#ifdef OUTPUT_FW_LOG_TO_FILE
	close_fw_log_file();
#endif
	//Put ISP DS4 bit to idle
	isp_program_mmhub_ds_reg(FALSE);

suc:
	isp_mutex_unlock(&isp->ops_mutex);
	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	RET(IMF_RET_FAIL);
	return IMF_RET_FAIL;
}

int start_stream_imp(void *context, enum camera_id cam_id)
{
	int ret;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_mutex_unlock(&isp->ops_mutex);
		ISP_PR_ERR("cid[%d] fail, bad fsm %d", cam_id,
			   ISP_GET_STATUS(isp));
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("cid[%d]", cam_id);
	ret = isp_start_stream(isp, cam_id, false);
	isp_mutex_unlock(&isp->ops_mutex);

	if (ret != RET_SUCCESS)
		close_camera_imp(isp, cam_id);

	RET(ret);
	return ret;
}

int switch_profile_imp(void *context, enum camera_id cid, unsigned int prf_id)
{
	int ret;
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cid)) {
		ISP_PR_ERR("fail for para");
		return IMF_RET_INVALID_PARAMETER;
	}

	ret = isp_switch_profile(isp, cid, prf_id);
	RET(ret);
	return ret;
}

int set_test_pattern_imp(void *context, enum camera_id cam_id, void *param)
{
	int ret = IMF_RET_SUCCESS;
	enum _TestPattern_t cvip_pattern;
	enum sensor_test_pattern pattern;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		cvip_pattern = *(enum _TestPattern_t *)param;
		if (cvip_pattern != TEST_PATTERN_NO)
			ret = isp_config_cvip_sensor(isp, cam_id, cvip_pattern);
	} else {
		pattern = *(enum sensor_test_pattern *)param;
		ret = isp_snr_set_test_pattern(isp, cam_id, pattern);
	}

	RET(ret);
	return ret;
}

int start_preview_imp(void *context, enum camera_id cam_id)
{
	int ret;
	//enum stream_id sid = STREAM_ID_PREVIEW;

	ret = start_stream_imp(context, cam_id);
	return ret;
}

int stop_stream_imp(void *context, enum camera_id cid, enum stream_id sid,
		    bool pause)
{
	struct isp_context *isp = context;
	struct isp_stream_info *sif = NULL;
	int ret_val = IMF_RET_SUCCESS;
	struct _CmdDisableOutCh_t cmdChDisable;
	unsigned int out_cnt;
	unsigned int timeout;
//	struct _CmdStopStream_t stop_stream_para;
	enum fw_cmd_resp_stream_id stream_id;
	struct isp_mapped_buf_info *cur = NULL;
//	int result;

	if (!is_para_legal(context, cid) || (sid > STREAM_ID_NUM)) {
		ISP_PR_ERR("fail, bad para, isp[%p] cid[%d] sid[%d]",
			context, cid, sid);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (((sid == STREAM_ID_RGBIR) || (sid == STREAM_ID_RGBIR_IR))
	&& !isp_is_host_processing_raw(isp, cid)) {
		ISP_PR_ERR("fail bad para, none RGBIR sensor");
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);

	ISP_PR_INFO("cid[%d] str[%d]", cid, sid);

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
		cmdChDisable.ch = ISP_PIPE_OUT_CH_PREVIEW;
		break;
	case STREAM_ID_VIDEO:
		cmdChDisable.ch = ISP_PIPE_OUT_CH_VIDEO;
		break;
	case STREAM_ID_ZSL:
		cmdChDisable.ch = ISP_PIPE_OUT_CH_STILL;
		break;
	case STREAM_ID_RGBIR_IR:
		goto rgbir_ir_stop;
	default:
		ISP_PR_INFO("never here");
		ret_val = IMF_RET_FAIL;
		goto quit;
	}

	if (sif->start_status == START_STATUS_STARTED) {
		if (!sif->is_perframe_ctl) {
			cur = (struct isp_mapped_buf_info *)
				isp_list_get_first_without_rm(&sif->buf_in_fw);

			timeout = (g_isp_env_setting == ISP_ENV_SILICON) ?
						(1000 * 2) : (1000 * 10);

#ifdef DO_SYNCHRONIZED_STOP_STREAM
			if (isp_send_fw_cmd_sync(isp, CMD_ID_DISABLE_OUT_CH,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&cmdChDisable, sizeof(cmdChDisable),
					timeout,
					NULL, NULL)
					!= RET_SUCCESS) {
#else  //DO_SYNCHRONIZED_STOP_STREAM
			if (isp_fw_set_out_ch_disable(isp, cid,
					cmdChDisable.ch) != RET_SUCCESS) {
#endif
				ISP_PR_ERR("send disable str fail");
			} else {
				ISP_PR_INFO("wait disable suc");
			}
		}
	}

rgbir_ir_stop:
	if (0) {
//wait for current buffers done for fw's disable bug in which
//fw will return immediately after
//receiving disable command without waiting current frame to finish
		unsigned int i;
		struct isp_mapped_buf_info *img_info;

		for (i = 0; i < 7; i++) {
			img_info = (struct isp_mapped_buf_info *)
				isp_list_get_first_without_rm(&sif->buf_in_fw);
			if (img_info == NULL)
				break;
			if (cur != img_info)
				break;
			ISP_PR_INFO("wait stream buf done for the %ith", i + 1);
			usleep_range(5000, 6000);
		}
	}
#if	defined(DELETED)
	isp_take_back_str_buf(isp, sif, cid, sid);
#endif
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
		ISP_PR_ERR("not stop for none RGBIR stream");
		goto quit;
	}

	if (out_cnt > 0) {
		ret_val = IMF_RET_SUCCESS;
		goto quit;
	}

	isp_uninit_stream(isp, cid);

quit:
	isp_mutex_unlock(&isp->ops_mutex);
	/*
	 *isp_mutex_unlock(&isp->sensor_info[cam_id].stream_op_mutex);
	 *isp_mutex_unlock(&isp->sensor_info[cam_id].snr_mutex);
	 */
	RET(ret_val);
	return ret_val;
}

/*refer to aidt_api_stop_streaming*/
int stop_preview_imp(void *context, enum camera_id cam_id, bool pause)
{
	int ret;
	enum stream_id sid = STREAM_ID_PREVIEW;

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	ret = stop_stream_imp(context, cam_id, sid, pause);
	return ret;
}

int set_stream_buf_imp(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buf_hdl,
			enum stream_id str_id)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	struct isp_picture_buffer_param buf_para;
	//unsigned int pipe_id = 0;
	int result;
	int ret = IMF_RET_FAIL;
	unsigned int y_len;
	unsigned int u_len;
	unsigned int v_len;
	struct isp_stream_info *str_info;

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (buf_hdl->virtual_addr == 0)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d] sid[%d]",
			   context, cam_id, str_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!isp_get_pic_buf_param
	(&isp->sensor_info[cam_id].str_info[str_id], &buf_para)) {
		ISP_PR_ERR("fail for get para");
		return IMF_RET_FAIL;
	}

	y_len = buf_para.channel_a_height * buf_para.channel_a_stride;
	u_len = buf_para.channel_b_height * buf_para.channel_b_stride;
	v_len = buf_para.channel_c_height * buf_para.channel_c_stride;

	if (buf_hdl->len < (y_len + u_len + v_len)) {
		ISP_PR_ERR("fail for small buf, %u need, real %u",
			   y_len + u_len + v_len, buf_hdl->len);
		return IMF_RET_FAIL;
	} else if (buf_hdl->len > (y_len + u_len + v_len)) {
		/*ISP_PR_INFO("in buf len(%u)>y+u+v(%u)",
		 *	      buf_hdl->len, (y_len + u_len + v_len));
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
		unsigned int tempi;
		unsigned int size = 1920 * 1080 * 3 / 2;

		for (tempi = 0; tempi < DBG_STAGE2_WB_BUF_CNT; tempi++) {
			if (g_dbg_stage2_buf[tempi])
				continue;

			g_dbg_stage2_buf[tempi] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}
	}
#endif

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_INFO("cid[%d] sid[%d] %p(%u)",
		    cam_id, str_id, buf_hdl->virtual_addr, buf_hdl->len);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_INFO("fail fsm %d", ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		ISP_PR_ERR("fail for sys_img_buf_handle_cpy");
		goto quit;
	}

	gen_img = isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
				cam_id, str_id, y_len, u_len, v_len);

	/*isp_dbg_show_map_info(gen_img); */
	if (gen_img == NULL) {
		ISP_PR_ERR("fail for isp_map_sys_2_mc");
		ret = IMF_RET_FAIL;
		goto quit;
	}

	/*ISP_PR_INFO("y:u:v %u:%u:%u, addr:%p(%u) map to %llx"
	 *	      y_len, u_len, v_len, buf_hdl->virtual_addr, buf_hdl->len,
	 *	      gen_img->y_map_info.mc_addr);
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
				(unsigned int) str_info->uv_tmp_buf->mem_size;
			gen_img->u_map_info.mc_addr =
				str_info->uv_tmp_buf->gpu_mc_addr;
			gen_img->u_map_info.sys_addr =
			(unsigned long long) str_info->uv_tmp_buf->sys_addr;
		} else {
			ISP_PR_ERR("fail alloc uv_tmp_buf for L8");
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
		ISP_PR_ERR("fail for fw_if_send_img_buf");
		goto quit;
	}

	isp->sensor_info[cam_id].str_info[str_id].buf_num_sent++;
	isp_list_insert_tail(
		&isp->sensor_info[cam_id].str_info[str_id].buf_in_fw,
		(struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

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

	RET(ret);
	return ret;
}

int set_preview_buf_imp(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buf_hdl)
{
	int ret;
	enum stream_id sid = STREAM_ID_PREVIEW;
#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	ret = set_stream_buf_imp(context, cam_id, buf_hdl, sid);

	return ret;
}

int set_video_buf_imp(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buf_hdl)
{
	int ret;

	ret = set_stream_buf_imp(context, cam_id, buf_hdl, STREAM_ID_VIDEO);

	return ret;
}

int set_frame_ctrl_imp(void *context, enum camera_id cam_id,
		       struct _CmdFrameCtrl_t *frm_ctrl)
{
	unsigned int index;
	int ret = IMF_RET_FAIL;
	struct sensor_info *snr_info;
	struct isp_context *isp = context;

	ISP_PR_DBG("cid[%d] fc[%p]", cam_id, frm_ctrl);
	if (!is_para_legal(context, cam_id) || !frm_ctrl) {
		ISP_PR_ERR("fail bad para isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("fail fsm[%d]", ISP_GET_STATUS(isp));
		goto quit;
	}

	if (ISP_NEED_CLKSWITCH(frm_ctrl)) {
		snr_info = &isp->sensor_info[cam_id];
		index = get_index_from_res_fps_id(isp, cam_id,
						  snr_info->cur_res_fps_id);
		isp_clk_change(isp, cam_id, index, snr_info->hdr_enable, true);
	}

	ret = isp_fw_send_frame_ctrl(isp, cam_id, frm_ctrl);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("cid[%d] fail! ret[%d]", cam_id, ret);
		goto quit;
	}
quit:
	isp_mutex_unlock(&isp->ops_mutex);
	RET(ret);
	return ret;
};

int start_cvip_sensor_imp(void *context, enum camera_id cam_id)
{
	int ret = IMF_RET_FAIL;
	struct _CmdStartCvipSensor_t param;
	struct isp_context *isp = context;

	ISP_PR_DBG("cid[%d] fc[%p]", cam_id);
	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("fail fsm[%d]", ISP_GET_STATUS(isp));
		goto quit;
	}

	param.sensorId = cam_id;
	ret = isp_fw_start_cvip_sensor(isp, cam_id, param);
	if (ret != RET_SUCCESS)
		ISP_PR_ERR("cid[%d] fail! ret[%d]", cam_id, ret);

quit:
	isp_mutex_unlock(&isp->ops_mutex);
	RET(ret);
	return ret;
}

int start_video_imp(void *context, enum camera_id cam_id)
{
	int ret;

	ret = start_stream_imp(context, cam_id);

	return ret;
}

/*refer to aidt_api_stop_streaming*/
int stop_video_imp(void *context, enum camera_id cam_id, bool pause)
{
	int ret;

	ret = stop_stream_imp(context, cam_id, STREAM_ID_VIDEO, pause);

	return ret;
}

void reg_notify_cb_imp(void *context, enum camera_id cam_id,
			func_isp_module_cb cb, void *cb_context)
{
	struct isp_context *isp = context;
//	struct camera_dev_info cam_dev_info;

	if (!is_para_legal(context, cam_id) || (cb == NULL)) {
		ISP_PR_ERR("cid[%d] fail for bad para", cam_id);
		return;
	}
	isp->evt_cb[cam_id] = cb;
	isp->evt_cb_context[cam_id] = cb_context;
	ISP_PR_INFO("cid[%d] suc", cam_id);
}

void unreg_notify_cb_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	ISP_PR_INFO("cid %u", cam_id);
	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("cid[%d] fail for bad para", cam_id);
		return;
	}
	isp->evt_cb[cam_id] = NULL;
	isp->evt_cb_context[cam_id] = NULL;
	ISP_PR_INFO("cid[%d] suc", cam_id);
}

int i2c_read_mem_imp(void __maybe_unused *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size,
			unsigned char *p_read_buffer,
			unsigned int byte_size)
{
	int result;

	result =
		i2_c_read_mem(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			p_read_buffer, byte_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

int i2c_write_mem_imp(void __maybe_unused *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned int reg_addr_size,
			unsigned char *p_write_buffer, unsigned int byte_size)
{
	int result;

	result = i2_c_write_mem(bus_num, slave_addr, slave_addr_mode,
				reg_addr, reg_addr_size, p_write_buffer,
				byte_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

int i2c_read_reg_imp(void __maybe_unused *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size, void *preg_value,
			unsigned char reg_size)
{
	int result;

	result =
		i2c_read_reg(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			preg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

int i2c_write_reg_imp(void __maybe_unused *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned char reg_addr_size,
			unsigned int reg_value, unsigned char reg_size)
{
	int result;

	result = i2c_write_reg(bus_num, slave_addr, slave_addr_mode,
			reg_addr, reg_addr_size, reg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

int i2c_write_reg_imp_group(void __maybe_unused *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned char reg_addr_size,
			unsigned int *reg_value, unsigned char reg_size)
{
	int result;

	result = i2c_write_reg_group(bus_num, slave_addr, slave_addr_mode,
			reg_addr, reg_addr_size, reg_value, reg_size);
	if (result == RET_SUCCESS)
		return IMF_RET_SUCCESS;
	else
		return IMF_RET_FAIL;
}

void reg_snr_op_imp(void *context, enum camera_id cam_id,
			struct sensor_operation_t *ops)
{
	struct isp_context *isp = context;
	struct sensor_res_fps_t *res_fps;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("cid[%d], ops:%p", cam_id, ops);

	isp->p_sensor_ops[cam_id] = ops;

	res_fps = isp->p_sensor_ops[cam_id]->get_res_fps(cam_id);
	if (res_fps) {
		unsigned int i;

		memcpy(&isp->sensor_info[cam_id].res_fps, res_fps,
			   sizeof(struct sensor_res_fps_t));
		ISP_PR_INFO("res_fps info for cid[%d]count:%d",
			    cam_id, res_fps->count);
		for (i = 0; i < res_fps->count; i++) {
			ISP_PR_INFO("idx:%d, fps:%d, w:%d, h:%d", i,
				    res_fps->res_fps[i].fps,
				    res_fps->res_fps[i].width,
				    res_fps->res_fps[i].height);
		}
	} else {
		ISP_PR_ERR("cid[%d] no fps", cam_id);
	}

}

void unreg_snr_op_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("cid[%d]", cam_id);
	isp->p_sensor_ops[cam_id] = NULL;
}

void reg_vcm_op_imp(void *context, enum camera_id cam_id,
			struct vcm_operation_t *ops)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("suc, cid[%d] ops:%p", cam_id, ops);

	isp->p_vcm_ops[cam_id] = ops;
}

void unreg_vcm_op_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("suc, cid[%d]", cam_id);

	isp->p_vcm_ops[cam_id] = NULL;
}

void reg_flash_op_imp(void *context, enum camera_id cam_id,
			struct flash_operation_t *ops)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (ops == NULL)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("cid[%d, ops:%p", cam_id, ops);

	isp->p_flash_ops[cam_id] = ops;
}

void unreg_flash_op_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("suc, cid[%d]", cam_id);

	isp->p_flash_ops[cam_id] = NULL;
}

int take_one_pic_imp(void *context, enum camera_id cam_id,
			struct take_one_pic_para *para,
			struct sys_img_buf_handle *in_buf)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	struct sys_img_buf_handle *buf = NULL;
	enum fw_cmd_resp_stream_id stream_id;
	//unsigned int len;
	int ret;
	struct _CmdCaptureYuv_t cmd_para;
	struct isp_mapped_buf_info *gen_img = NULL;
	unsigned int y_len;
	unsigned int u_len;
	unsigned int v_len;
	char pt_start_stream = 0;
	struct isp_stream_info str_info;
	struct isp_picture_buffer_param buf_para;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for bad para, isp[%p] cid[%d]", context,
			   cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	memset(&cmd_para, 0, sizeof(cmd_para));
	memset(&str_info, 0, sizeof(struct isp_stream_info));
	memset(&buf_para, 0, sizeof(struct isp_picture_buffer_param));
	if ((para == NULL) || (in_buf == NULL) || !in_buf->len) {
		ISP_PR_ERR("fail for bad para, para:%p, buf:%p", para, in_buf);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!para->width || !para->height ||
	(para->width > para->luma_pitch)) {
		ISP_PR_ERR("fail for bad pic para");
		return IMF_RET_INVALID_PARAMETER;
	}

		str_info.format = para->fmt;
		str_info.width = para->width;
		str_info.height = para->height;
		str_info.luma_pitch_set = para->luma_pitch;
		str_info.chroma_pitch_set = para->chroma_pitch;

		if (!isp_get_pic_buf_param(&str_info, &buf_para)) {
			ISP_PR_ERR("fail for bad pic para or buf");
			return IMF_RET_INVALID_PARAMETER;
		}
		y_len = buf_para.channel_a_stride * buf_para.channel_a_height;
		u_len = buf_para.channel_b_stride * buf_para.channel_b_height;
		v_len = buf_para.channel_c_stride * buf_para.channel_c_height;
/*
 *	len = isp_cal_buf_len_by_para(para->fmt, para->width, para->height,
 *					  para->luma_pitch, para->chroma_pitch,
 *					  &y_len, &u_len, &v_len);
 *	if ((0 == len) || (in_buf->len < len)) {
 *		ISP_PR_ERR("take_one_pic_imp fail for bad pic para or buf");
 *		return IMF_RET_INVALID_PARAMETER;
 *	} else
 */
		if (in_buf->len > (y_len + u_len + v_len)) {
			ISP_PR_INFO("in buf len(%u)>y+u+v(%u)",
				    in_buf->len, (y_len + u_len + v_len));

		if (v_len != 0)
			v_len = in_buf->len - y_len - u_len;
		else if (u_len != 0)
			u_len = in_buf->len - y_len;
		else
			y_len = in_buf->len;
	}

	sif = &isp->sensor_info[cam_id];

	if ((sif->status == START_STATUS_STARTED) &&
	(sif->str_info[STREAM_ID_ZSL].start_status == START_STATUS_STARTED)) {
		ISP_PR_ERR("fail for zsl start, cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}

	cmd_para.imageProp.imageFormat = isp_trans_to_fw_img_fmt(para->fmt);
	cmd_para.imageProp.width = para->width;
	cmd_para.imageProp.height = para->height;
	cmd_para.imageProp.lumaPitch = para->luma_pitch;
	cmd_para.imageProp.chromaPitch = para->chroma_pitch;

	if (cmd_para.imageProp.imageFormat == IMAGE_FORMAT_INVALID) {
		ISP_PR_ERR("fail for bad fmt, cid[%d] fmt %d", cam_id,
			   para->fmt);
		return IMF_RET_FAIL;
	}

	buf = sys_img_buf_handle_cpy(in_buf);

	if (buf == NULL) {
		ISP_PR_ERR("fail cpy buf, cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	gen_img = isp_map_sys_2_mc(isp, buf, ISP_MC_ADDR_ALIGN,
				cam_id, stream_id, y_len, u_len, v_len);
	if (gen_img == NULL) {
		if (buf)
			sys_img_buf_handle_free(buf);
		ISP_PR_ERR("fail map sys, cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}

	cmd_para.buffer.vmidSpace.bit.vmid = 0;
	cmd_para.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
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

	ISP_PR_INFO("cid[%d], precap %d, focus_first %d",
		    cam_id, para->use_precapture, para->focus_first);

	if ((cam_id < CAMERA_ID_MAX)
	&& (isp->sensor_info[cam_id].status == START_STATUS_NOT_START))
		pt_start_stream = 1;

	if (isp_init_stream(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_init_stream");
		ret = RET_FAILURE;
		goto fail;
	}

	if (isp_setup_3a(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_3a");
		ret = RET_FAILURE;
		goto fail;
	}

	if (isp_kickoff_stream(isp, cam_id, para->width, para->height) !=
	RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_kickoff_stream");
		ret = RET_FAILURE;
		goto fail;
	}

	if (para->use_precapture) {
		//cmd_para.usePreCap = 1;
		ret = isp_fw_set_ae_precapture(isp, cam_id);
		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("fail for send AE_PRECAPTURE");
			goto fail;
		}
	}

	isp_dbg_show_img_prop("in take one pic imp", &cmd_para.imageProp);
	ISP_PR_INFO("y sys:mc:len %llx:%llx:%u", gen_img->y_map_info.sys_addr,
		    gen_img->y_map_info.mc_addr, gen_img->y_map_info.len);
//	isp_dbg_show_map_info(gen_img);
//	ISP_PR_INFO("Y: 0x%x:%x, %u(should %u)",
//		    cmd_para.buffer.bufBaseAHi, cmd_para.buffer.bufBaseALo,
//		    cmd_para.buffer.bufSizeA, 640 * 480);
//	ISP_PR_INFO("U: 0x%x:%x, %u(should %u)",
//		    cmd_para.buffer.bufBaseBHi, cmd_para.buffer.bufBaseBLo,
//		    cmd_para.buffer.bufSizeB, 640 * 480 / 2);
//	ISP_PR_INFO("v: 0x%x:%x, %u(should 0)",
//		    cmd_para.buffer.bufBaseCHi, cmd_para.buffer.bufBaseCLo,
//		    cmd_para.buffer.bufSizeC);
	if (!para->focus_first) {
		ret = isp_fw_set_capture_yuv(isp, cam_id,
				cmd_para.imageProp, cmd_para.buffer);
	} else {
		enum _AfMode_t orig_mod = sif->af_mode;

		isp_fw_set_focus_mode(isp, cam_id, AF_MODE_ONE_SHOT);
		isp_fw_start_af(isp, cam_id);
		ret = isp_fw_set_capture_yuv(isp, cam_id,
				cmd_para.imageProp, cmd_para.buffer);
		isp_fw_set_focus_mode(isp, cam_id, orig_mod);
		isp_fw_start_af(isp, cam_id);
	}

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail for send CAPTURE_YUV");
		goto fail;
	}

	sif->take_one_pic_left_cnt++;
	if (cam_id < CAMERA_ID_MAX)
		isp_list_insert_tail(&
			(isp->sensor_info[cam_id].take_one_pic_buf_list),
			(struct list_node *)gen_img);
	isp_mutex_unlock(&isp->ops_mutex);
	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
fail:
	if (gen_img)
		isp_unmap_sys_2_mc(isp, gen_img);
	if (buf)
		sys_img_buf_handle_free(buf);
	isp_mutex_unlock(&isp->ops_mutex);
	return IMF_RET_FAIL;
}

int stop_take_one_pic_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	int ret_val;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for para, isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	sif = &isp->sensor_info[cam_id];

	ISP_PR_INFO("cid[%d]", cam_id);
	while (sif->take_one_pic_left_cnt) {
		/*
		 *ISP_PR_INFO("front %p,%p",
		 *	      isp->take_one_pic_buf_list.front,
		 *	      isp->take_one_pic_buf_list.rear);
		 *
		 *if (isp->take_one_pic_buf_list.front ==
		 *isp->take_one_pic_buf_list.rear)
		 *	break;
		 */
		usleep_range(10000, 11000);
	}
	isp_mutex_lock(&isp->ops_mutex);

	ret_val = IMF_RET_SUCCESS;

	isp_mutex_unlock(&isp->ops_mutex);
	RET(ret_val);
	return ret_val;
}

int online_tune_imp(void *context, enum camera_id cam_id,
			unsigned int cmd, unsigned int is_set_cmd,
			unsigned short is_stream_cmd,
			unsigned int is_direct_cmd,
			void *para, unsigned int *para_len,
			void *resp, unsigned int resp_len)
{
	struct isp_context *isp = (struct isp_context *)context;
	int ret = 0;
	enum fw_cmd_resp_stream_id stream_id;
	void *package = NULL;
	unsigned int package_size = 0;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("%s: fail para,isp:%p,cid:%#x\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	if (para && ((para_len == NULL) || (*para_len == 0))) {
		ISP_PR_ERR("%s: fail para len,isp:%p,cid:%#x\n",
			__func__, context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	package = para;
	package_size = *para_len;

	/*
	 * if the given command is a set command, no need to read
	 * payload from response channel.
	 */
	if (is_set_cmd || is_direct_cmd)
		goto direct_cmd;

	/*
	 * otherwise, goto indirect_cmd (read out data from response
	 * channel.
	 */
	goto indirect_cmd;

direct_cmd:
	/* this method will wait until response channel ack */
	ret = isp_send_fw_cmd_sync(isp,
			cmd,
			is_stream_cmd ?
				stream_id : FW_CMD_RESP_STREAM_ID_GLOBAL,
			is_direct_cmd ?
				FW_CMD_PARA_TYPE_DIRECT :
				FW_CMD_PARA_TYPE_INDIRECT,
			package, package_size, /* param */
			1000 * 10, /* timeout in mulluseconds */
			resp, &resp_len
			);
	goto exit;

indirect_cmd:
	/* this method will wait until response channel ack */
	ret = isp_send_fw_cmd_online_tune(isp,
			cmd,
			is_stream_cmd ?
				stream_id : FW_CMD_RESP_STREAM_ID_GLOBAL,
			is_direct_cmd ?
				FW_CMD_PARA_TYPE_DIRECT :
				FW_CMD_PARA_TYPE_INDIRECT,
			para,  /* param */
			*para_len,  /* size of param */
			1000 * 10,  /* timeout in millisceonds */
			resp,  /* payload buffer to be stored */
			&resp_len  /* payload size written */
			);
	goto exit;

exit:
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail cid:%d,cmd %s(0x%08x),is_set_cmd %u,",
			__func__, cam_id,
			isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
		ISP_PR_ERR("%s: is_stream_cmd %u, is_direct_cmd %u,para %p\n",
			__func__, is_stream_cmd, is_direct_cmd, para);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("%s: suc cid:%d,cmd %s(0x%08x),is_set_cmd %u,",
		__func__, cam_id,
		isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
	ISP_PR_INFO("%s: is_stream_cmd %u,is_direct_cmd %u,para %p\n",
		__func__, is_stream_cmd, is_direct_cmd, para);

	return IMF_RET_SUCCESS;
}

void write_isp_reg32_imp(void __maybe_unused *context,
	unsigned long long reg_addr, unsigned int value)
{
	isp_hw_reg_write32((unsigned int) reg_addr, value);
}

unsigned int read_isp_reg32_imp(void __maybe_unused *context,
	unsigned long long reg_addr)
{
	return isp_hw_reg_read32((unsigned int) reg_addr);
}

int set_common_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_id /*enum para_id */,
			void *para_value)
{
	int ret = IMF_RET_SUCCESS;
	int stream_on = 0;
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		ISP_PR_ERR("fail for bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	if (isp->sensor_info[cam_id].status == START_STATUS_STARTED)
		stream_on = 1;

	switch (para_id) {
	case PARA_ID_GAMMA:
		isp->sensor_info[cam_id].para_gamma_set =
			*((int *) para_value);
		ISP_PR_INFO("cid[%d] set PARA_ID_GAMMA to %d", cam_id,
			    isp->sensor_info[cam_id].para_gamma_set);

		if (isp->sensor_info[cam_id].para_gamma_cur ==
		isp->sensor_info[cam_id].para_gamma_set) {
			ISP_PR_INFO("not diff value, do none");
			break;
		}
		isp->sensor_info[cam_id].para_gamma_cur =
			isp->sensor_info[cam_id].para_gamma_set;
		if (!stream_on) {
			ISP_PR_INFO("stream not on, set later");
			break;
		}

		if (isp_fw_set_gamma(isp, cam_id,
		isp->sensor_info[cam_id].para_gamma_set) != RET_SUCCESS)
			ret = IMF_RET_FAIL;
		break;
	case PARA_ID_SCENE:
		isp->sensor_info[cam_id].para_scene_mode_set =
			*((enum pvt_scene_mode *)para_value);
		if (isp->sensor_info[cam_id].para_scene_mode_set ==
		isp->sensor_info[cam_id].para_scene_mode_cur) {
			ISP_PR_INFO("not diff value, do none");
			break;
		}

		isp->sensor_info[cam_id].para_scene_mode_cur =
			isp->sensor_info[cam_id].para_scene_mode_set;

		if (fw_if_set_scene_mode(isp, cam_id,
		isp->sensor_info[cam_id].para_scene_mode_set) != RET_SUCCESS)
			ret = IMF_RET_FAIL;
		break;
	default:
		ISP_PR_INFO("cid[%d] not support para_id %d", cam_id, para_id);
		ret = IMF_RET_INVALID_PARAMETER;
		break;
	}

	RET(ret);
	return ret;
}

int get_common_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_id /*enum para_id */,
			void *para_value)
{
	int ret = IMF_RET_SUCCESS;
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}

	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		ISP_PR_ERR("fail for illegal cid[%d] para_id %d", cam_id,
			   para_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_id) {
	case PARA_ID_BRIGHTNESS:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_brightness_cur;
		ISP_PR_INFO("cid[%d] get BRIGHTNESS to %d", cam_id,
			    isp->sensor_info[cam_id].para_brightness_cur);
		break;
	case PARA_ID_CONTRAST:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_contrast_cur;
		ISP_PR_INFO("cid[%d] get CONTRAST %d", cam_id,
			    isp->sensor_info[cam_id].para_contrast_cur);
		break;
	case PARA_ID_HUE:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_hue_cur;
		ISP_PR_INFO("cid[%d] get HUE %d", cam_id,
			    isp->sensor_info[cam_id].para_hue_cur);
		break;
	case PARA_ID_SATURATION:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_satuaration_cur;
		ISP_PR_INFO("cid[%d] get SATURATION %d", cam_id,
			    isp->sensor_info[cam_id].para_satuaration_cur);
		break;
	case PARA_ID_COLOR_ENABLE:
		((struct pvt_int32 *)para_value)->value =
			isp->sensor_info[cam_id].para_color_enable_cur;
		ISP_PR_INFO("cid[%d] get COLOR_ENABLE %d", cam_id,
			    isp->sensor_info[cam_id].para_color_enable_cur);
		break;
	case PARA_ID_FLICKER_MODE:
		((struct pvt_flicker_mode *)para_value)->value =
			isp->sensor_info[cam_id].flick_type_cur;
		ISP_PR_INFO("cid[%d] get FLICKER_MODE %d", cam_id,
			    isp->sensor_info[cam_id].flick_type_cur);
		break;
	case PARA_ID_ISO:
		*((struct _AeIso_t *) para_value) =
			isp->sensor_info[cam_id].para_iso_cur;
		ISP_PR_INFO("cid[%d] get iso %u", cam_id,
			    isp->sensor_info[cam_id].para_iso_cur.iso);
		break;

	case PARA_ID_EV:
		*((struct _AeEv_t *) para_value) =
			isp->sensor_info[cam_id].para_ev_compensate_cur;
		ISP_PR_INFO("cid[%d] get ev %u/%u", cam_id,
		isp->sensor_info[cam_id].para_ev_compensate_cur.numerator,
		isp->sensor_info[cam_id].para_ev_compensate_cur.denominator);
		break;
	case PARA_ID_GAMMA:
		*((int *) para_value) =
			isp->sensor_info[cam_id].para_gamma_cur;
		ISP_PR_INFO("cid[%d] get GAMMA %d", cam_id,
			isp->sensor_info[cam_id].para_gamma_cur);
		break;
	case PARA_ID_SCENE:
		*((enum pvt_scene_mode *)para_value) =
			isp->sensor_info[cam_id].para_scene_mode_cur;
		ISP_PR_INFO("cid[%d] get SCENE %d", cam_id,
			isp->sensor_info[cam_id].para_scene_mode_cur);
		break;
	default:
		ISP_PR_INFO("cid[%d] no supported paraid %d",
			cam_id, para_id);
		ret = IMF_RET_INVALID_PARAMETER;
		break;
	}

	RET(ret);
	return ret;
}

int set_stream_para_imp(void *context, enum camera_id cam_id,
			enum stream_id str_id, enum para_id para_type,
			void *para_value)
{
	int ret = RET_SUCCESS;
	int func_ret = IMF_RET_SUCCESS;

	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (str_id > STREAM_ID_NUM)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d] sid[%d]", isp,
			   cam_id, str_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_INFO("cid[%d],sid %d,para %s(%d)", cam_id, str_id,
		    isp_dbg_get_para_str(para_type), para_type);

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
				ISP_PR_ERR("(FMT) fail for set fmt:%s",
					   isp_dbg_get_pvt_fmt_str(data_fmat));
				func_ret = IMF_RET_FAIL;
				break;
			}
			ISP_PR_INFO("(FMT) suc set fmt:%s",
				    isp_dbg_get_pvt_fmt_str(data_fmat));
			break;
		}
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;
			ret = isp_set_str_res_fps_pitch(context, cam_id,
							str_id, data_pitch);
			if (ret != RET_SUCCESS) {
				ISP_PR_ERR("(RES_FPS_PITCH) fail for set");
				func_ret = IMF_RET_FAIL;
				break;
			}

			ISP_PR_INFO("(RES_FPS_PITCH) suc");
			break;
		}
	default:
		ISP_PR_ERR("fail for not supported");
		func_ret = IMF_RET_INVALID_PARAMETER;
		break;
	}
	isp_mutex_unlock(&isp->ops_mutex);
	return func_ret;
}

int set_preview_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	enum stream_id sid = STREAM_ID_PREVIEW;

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif
	return set_stream_para_imp
		(context, cam_id, sid, para_type, para_value);
}

int get_preview_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;
	enum stream_id sid = STREAM_ID_PREVIEW;

#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
	sid = STREAM_ID_ZSL;
#endif

	if ((context == NULL) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		ISP_PR_ERR("fail for illegal camera:%d", cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
			isp_context->sensor_info[cam_id].str_info[sid].format;
			break;
		}
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
		}
	default:
		ISP_PR_ERR("fail for not supported para_type %d", para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO("(%d) success for camera %d", para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int set_video_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_VIDEO,
				para_type, para_value);
}

int get_video_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;

	if ((context == NULL) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		ISP_PR_ERR("fail for illegal camera:%d", cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
	isp_context->sensor_info[cam_id].str_info[STREAM_ID_VIDEO].format;
			break;
		}
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
		}
	default:
		ISP_PR_ERR("fail for not supported para_type %d", para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO("(%d) success for camera %d", para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int set_rgbir_ir_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_RGBIR_IR,
			para_type, para_value);
}

int get_rgbir_ir_para_imp(void *context, enum camera_id cam_id,
			    enum para_id para_type, void *para_value)
{
	struct isp_context *isp_context = (struct isp_context *)context;
	enum stream_id stream_type;

	if ((context == NULL) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((cam_id < CAMERA_ID_REAR) || (cam_id > CAMERA_ID_FRONT_RIGHT)) {
		ISP_PR_ERR("fail for illegal camera:%d", cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	stream_type = STREAM_ID_VIDEO;
	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp_context->sensor_info[cam_id].str_info[stream_type].format;
			break;
		}
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
		}
	default:
		ISP_PR_ERR("fail for not supported para_type %d", para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO("(%d) success for camera %d", para_type, cam_id);
	return IMF_RET_SUCCESS;
}

int set_zsl_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_ZSL,
				para_type, para_value);
}


int get_zsl_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	/*int ret = RET_SUCCESS; */
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].format;
			ISP_PR_INFO("suc for camera %d, get format %d", cam_id,
				    (*(enum pvt_img_fmt *)para_value));
			break;
		}
	case PARA_ID_IMAGE_SIZE:
		{
			struct pvt_img_size *img_size =
				(struct pvt_img_size *)para_value;
			img_size->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].width;
			img_size->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].height;
			ISP_PR_INFO("suc for camera %d, set jpeg size %d:%d",
				    cam_id,
				    img_size->width, img_size->height);
			break;
		}
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;

			if (data_pitch == NULL) {
				ISP_PR_INFO("cam[%d] DATA_RES_FPS_PITCH error",
					    cam_id);
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

			ISP_PR_INFO("suc, cam[%d] for DATA_RES_FPS_PITCH",
				    cam_id);
			ISP_PR_INFO("w(%d):h(%d), lpitch:%d cpitch:%d",
				    data_pitch->width, data_pitch->height,
				    data_pitch->luma_pitch,
				    data_pitch->chroma_pitch);
			break;
		}
	default:
		ISP_PR_ERR("fail for not supported para_type %d", para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_rgbir_raw_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	return set_stream_para_imp(context, cam_id, STREAM_ID_RGBIR,
				para_type, para_value);
}

int get_rgbir_raw_para_imp(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value)
{
	/*int ret = RET_SUCCESS; */
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cam_id) || (para_value == NULL)) {
		ISP_PR_ERR("fail for null context or para_value");
		return IMF_RET_INVALID_PARAMETER;
	}

	switch (para_type) {
	case PARA_ID_DATA_FORMAT:
		{
			(*(enum pvt_img_fmt *)para_value) =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].format;
			ISP_PR_INFO("success for camera %d, get format %d",
				    cam_id,
				    (*(enum pvt_img_fmt *)para_value));
			break;
		}
	case PARA_ID_IMAGE_SIZE:
		{
			struct pvt_img_size *img_size =
				(struct pvt_img_size *)para_value;
			img_size->width =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].width;
			img_size->height =
		isp->sensor_info[cam_id].str_info[STREAM_ID_RGBIR].height;
			ISP_PR_INFO("suc for camera %d, set jpeg size %d:%d",
				    cam_id,
				    img_size->width, img_size->height);
			break;
		}
	case PARA_ID_DATA_RES_FPS_PITCH:
		{
			struct pvt_img_res_fps_pitch *data_pitch =
				(struct pvt_img_res_fps_pitch *)para_value;

			if (data_pitch == NULL) {
				ISP_PR_INFO("cam[%d] DATA_RES_FPS_PITCH error",
					    cam_id);
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

			ISP_PR_INFO("suc, cam[%d] for DATA_RES_FPS_PITCH",
				    cam_id);
			ISP_PR_INFO("w(%d):h(%d),lpitch:%d cpitch:%d",
				    data_pitch->width,
				    data_pitch->height, data_pitch->luma_pitch,
				    data_pitch->chroma_pitch);
			break;
		}
	default:
		ISP_PR_ERR("fail for not supported para_type %d", para_type);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int get_capabilities_imp(void *context, struct isp_capabilities *cap)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (cap == NULL)) {
		ISP_PR_ERR("fail for null context");
		return IMF_RET_INVALID_PARAMETER;
	}
	memcpy(cap, &isp->isp_cap, sizeof(struct isp_capabilities));
	return IMF_RET_SUCCESS;
}

int snr_clk_set_imp(void *context, enum camera_id cam_id,
			unsigned int clk /*in k_h_z */)
{
	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_ERR("succ,cid:%u,clk:%ukhz", cam_id, clk);
	return IMF_RET_SUCCESS;
}

struct sensor_res_fps_t *get_camera_res_fps_imp(void *context,
						enum camera_id cam_id)
{
	struct isp_context *isp = (struct isp_context *)context;
	long long start;
	long long end;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return NULL;
	}
	isp_get_cur_time_tick(&start);

	do {
		if (isp->snr_res_fps_info_got[cam_id])
			return &isp->sensor_info[cam_id].res_fps;
		isp_get_cur_time_tick(&end);
		if (isp_is_timeout(&start, &end, 500)) {
			ISP_PR_ERR("fail for timeout");
			return NULL;
		}
	} while (1 > 0);

	return NULL;
}

int take_raw_pic_imp(void *context, enum camera_id cam_id,
		struct sys_img_buf_handle *raw_pic_buf,
		unsigned int use_precapture)
{
	struct isp_context *isp;
	unsigned int raw_width = 0;
	unsigned int raw_height = 0;
	struct sensor_info *sif;
	struct _CmdCaptureRaw_t cmd_para;
	struct sys_img_buf_handle *buf;
	struct isp_mapped_buf_info *gen_img;
	unsigned int y_len;
	enum fw_cmd_resp_stream_id stream_id;
	int ret;
	char pt_start_stream = 0;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (!raw_pic_buf || !raw_pic_buf->len || !raw_pic_buf->virtual_addr) {
		ISP_PR_ERR("fail bad raw buf");
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
		ISP_PR_ERR("fail by small buf %u(%u=%ux%ux2 needed)",
			   raw_pic_buf->len,
			   (raw_width * raw_height * 2),
			   raw_width, raw_height);
		return IMF_RET_FAIL;
	}
	y_len = raw_pic_buf->len;

	buf = sys_img_buf_handle_cpy(raw_pic_buf);

	if (buf == NULL) {
		ISP_PR_ERR("fail cpy buf,cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	gen_img = isp_map_sys_2_mc(isp, buf, ISP_MC_ADDR_ALIGN,
				cam_id, STREAM_ID_RAW, y_len, 0, 0);

	if (gen_img == NULL) {
		if (buf)
			sys_img_buf_handle_free(buf);
		ISP_PR_ERR("fail map sys,cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}
	cmd_para.buffer.vmidSpace.bit.vmid = 0;
	cmd_para.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
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

	ISP_PR_INFO("cid[%d], precap %d, sys %llx(%u), mc %llx",
		    cam_id, use_precapture, gen_img->y_map_info.sys_addr,
		    gen_img->y_map_info.len, gen_img->y_map_info.mc_addr);
	if (gen_img->y_map_info.sys_addr) {
		unsigned char *ch =
			(unsigned char *) gen_img->y_map_info.sys_addr;

		ch[0] = 0xaa;
		ch[1] = 0x55;
		ch[2] = 0xbd;
		ch[3] = 0xce;
	}

	if ((cam_id < CAMERA_ID_MAX)
	&& (isp->sensor_info[cam_id].status) == START_STATUS_NOT_START)
		pt_start_stream = 1;

	if (isp_init_stream(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_init_stream");
		ret = IMF_RET_FAIL;
		goto fail;
	}

	if (isp_setup_3a(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_3a");
		ret = IMF_RET_FAIL;
		goto fail;
	}

	if (isp_kickoff_stream(isp, cam_id, raw_width, raw_height) !=
	RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_kickoff_stream");
		ret = IMF_RET_FAIL;
		goto fail;
	}

	if (use_precapture) {
		//cmd_para.usePreCap = 1;
		ret = isp_fw_set_ae_precapture(isp, cam_id);
		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("fail for send AE_PRECAPTURE");
			goto fail;
		}
	}

	ret = isp_fw_set_capture_raw(isp, cam_id, cmd_para.buffer);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail for send CAPTURE_RAW");
		goto fail;
	}
	if (cam_id < CAMERA_ID_MAX) {
		isp->sensor_info[cam_id].str_info[STREAM_ID_RAW].buf_num_sent++;
		isp_list_insert_tail(
		&isp->sensor_info[cam_id].str_info[STREAM_ID_RAW].buf_in_fw,
			(struct list_node *)gen_img);
	}
	isp_mutex_unlock(&isp->ops_mutex);
	ISP_PR_INFO("(%ubytes), succ", y_len);
	return IMF_RET_SUCCESS;
fail:
	if (gen_img)
		isp_unmap_sys_2_mc(isp, gen_img);
	if (buf)
		sys_img_buf_handle_free(buf);
	isp_mutex_unlock(&isp->ops_mutex);
	return IMF_RET_FAIL;

	/*todo  to be added later */
}

int stop_take_raw_pic_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;
	struct sensor_info *sif;
	int ret_val;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	sif = &isp->sensor_info[cam_id];

	ISP_PR_INFO("cid[%d]", cam_id);
	isp_mutex_lock(&isp->ops_mutex);

	ret_val = IMF_RET_SUCCESS;
	isp_mutex_unlock(&isp->ops_mutex);
	RET(ret_val);
	return ret_val;
}

int auto_exposure_lock_imp(void *context, enum camera_id cam_id,
			enum _LockType_t lock_mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);
	if (isp_fw_3a_lock(isp, cam_id, ISP_3A_TYPE_AE, lock_mode) !=
	RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_set_focus_mode context:%p, cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int cproc_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	isp->sensor_info[cam_id].para_color_enable_set = enable;

	if (fw_if_set_color_enable(isp, cam_id, enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for fw_if_set_color_enable,cid[%d] en:%d",
			   cam_id, enable);
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int cproc_set_contrast_imp(void *context, enum camera_id cam_id,
			unsigned int contrast)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	isp->sensor_info[cam_id].para_contrast_set = contrast;

	if (isp->sensor_info[cam_id].para_contrast_cur ==
	isp->sensor_info[cam_id].para_contrast_set) {
		ISP_PR_INFO("not diff value, do none");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_contrast_cur =
		isp->sensor_info[cam_id].para_contrast_set;

	if (fw_if_set_contrast(isp, cam_id,
	isp->sensor_info[cam_id].para_contrast_set) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int cproc_set_brightness_imp(void *context, enum camera_id cam_id,
			unsigned int brightness)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	isp->sensor_info[cam_id].para_brightness_set = brightness;

	if (isp->sensor_info[cam_id].para_brightness_cur ==
	isp->sensor_info[cam_id].para_brightness_set) {
		ISP_PR_INFO("not diff value, do none");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_brightness_cur =
		isp->sensor_info[cam_id].para_brightness_set;

	if (fw_if_set_brightness(isp, cam_id,
	isp->sensor_info[cam_id].para_brightness_set) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int cproc_set_saturation_imp(void *context, enum camera_id cam_id,
			unsigned int saturation)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	isp->sensor_info[cam_id].para_satuaration_set = saturation;

	if (isp->sensor_info[cam_id].para_satuaration_cur ==
	isp->sensor_info[cam_id].para_satuaration_set) {
		ISP_PR_INFO("not diff value, do none");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_satuaration_cur =
		isp->sensor_info[cam_id].para_satuaration_set;

	if (fw_if_set_satuation(isp, cam_id,
	isp->sensor_info[cam_id].para_satuaration_set) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int cproc_set_hue_imp(void *context, enum camera_id cam_id,
			unsigned int hue)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	isp->sensor_info[cam_id].para_hue_set = hue;

	if (isp->sensor_info[cam_id].para_hue_cur ==
	isp->sensor_info[cam_id].para_hue_set) {
		ISP_PR_INFO("not diff value, do none");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_hue_cur =
		isp->sensor_info[cam_id].para_hue_set;

	if (fw_if_set_hue(isp, cam_id,
	isp->sensor_info[cam_id].para_hue_set) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int auto_exposure_unlock_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	ISP_PR_INFO(" cam %d", cam_id);
	if (isp_fw_3a_unlock(isp, cam_id, ISP_3A_TYPE_AE) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_set_focus_mode, context:%p,cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int auto_wb_lock_imp(void *context, enum camera_id cam_id,
			enum _LockType_t lock_mode)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	ISP_PR_INFO(" cam %d", cam_id);

	if (isp_fw_3a_lock(isp, cam_id, ISP_3A_TYPE_AWB, lock_mode) !=
	RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_set_focus_mode, context:%p,cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int auto_wb_unlock_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	ISP_PR_INFO("cam %d", cam_id);

	if (isp_fw_3a_unlock(isp, cam_id, ISP_3A_TYPE_AWB) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_set_focus_mode, context:%p, cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_exposure_mode_imp(void *context, enum camera_id cam_id,
				enum _AeMode_t mode)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if ((mode != AE_MODE_MANUAL) && (mode != AE_MODE_AUTO)) {
		ISP_PR_ERR("fail bad mode:%d", mode);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	ISP_PR_INFO(" cam %d,mode %s", cam_id, isp_dbg_get_ae_mode_str(mode));

	if (isp_fw_set_exposure_mode(isp, cam_id, mode) != RET_SUCCESS) {
		ISP_PR_ERR("fail by isp_fw_set_exposure_mode");
		return IMF_RET_INVALID_PARAMETER;
	}
	if ((mode == AE_MODE_AUTO) &&
	sif->calib_enable && sif->calib_set_suc) {
		sif->sensor_cfg.exposure = sif->sensor_cfg.exposure_orig;
		isp_setup_ae_range(isp, cam_id);
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_wb_mode_imp(void *context, enum camera_id cam_id,
		enum _AwbMode_t mode)
{
	struct isp_context *isp;

	isp = (struct isp_context *)context;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	ISP_PR_INFO(" cam %d,mode %s", cam_id, isp_dbg_get_awb_mode_str(mode));

	if (isp_fw_set_wb_mode(isp, cam_id, mode) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_set_wb_mode");
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_snr_ana_gain_imp(void *context, enum camera_id cam_id,
			unsigned int ana_gain)
{
	struct isp_context *isp;
	struct sensor_info *snr_info;

	if (!is_para_legal(context, cam_id) || (ana_gain == 0)) {
		ISP_PR_ERR("fail for illegal para, isp[%p] cid[%d]ana_gain %d",
			   context, cam_id, ana_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	snr_info = &isp->sensor_info[cam_id];

	if ((ana_gain < snr_info->sensor_cfg.exposure.min_gain)
	|| (ana_gain > snr_info->sensor_cfg.exposure.max_gain)) {
		ISP_PR_ERR("fail bad again %u,should[%u,%u]",
			   ana_gain, snr_info->sensor_cfg.exposure.min_gain,
			   snr_info->sensor_cfg.exposure.max_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO(" cam %d, ana_gain %u", cam_id, ana_gain);

	if (isp->fw_ctrl_3a &&
	(isp->drv_settings.set_again_by_sensor_drv != 1)) {
		if (isp_fw_set_snr_ana_gain(isp, cam_id, ana_gain) !=
		RET_SUCCESS) {
			ISP_PR_ERR("fail for set_snr_ana_gain, isp:%p,cid:%d",
				   context, cam_id);
			return IMF_RET_INVALID_PARAMETER;
		}
	} else {
		unsigned int set_gain;
		unsigned int set_integration;
		int tmp_ret;

		tmp_ret = isp_snr_exposure_ctrl(isp, cam_id, ana_gain,
					0, &set_gain, &set_integration);
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_snr_dig_gain_imp(void *context, enum camera_id cam_id,
			unsigned int dig_gain)
{
	struct isp_context *isp;
	struct sensor_info *snr_info;

	if (!is_para_legal(context, cam_id) || (dig_gain == 0)) {
		ISP_PR_ERR("fail for illegal para, isp:%p,cid[%d]dig_gain %d",
			   context, cam_id, dig_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	snr_info = &isp->sensor_info[cam_id];

	if ((dig_gain < snr_info->sensor_cfg.exposure.min_digi_gain)
	|| (dig_gain > snr_info->sensor_cfg.exposure.max_digi_gain)) {
		ISP_PR_ERR("fail bad dig_gain %u,should[%u,%u]",
			   dig_gain,
			   snr_info->sensor_cfg.exposure.min_digi_gain,
			   snr_info->sensor_cfg.exposure.max_digi_gain);
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO(" cam %d, dig_gain %u", cam_id, dig_gain);

	if (isp->fw_ctrl_3a &&
	(isp->drv_settings.set_dgain_by_sensor_drv != 1)) {
		if (isp_fw_set_snr_dig_gain(isp, cam_id, dig_gain) !=
		RET_SUCCESS) {
			ISP_PR_ERR("fail set_snr_dig_gain, isp:%p, cid:%d",
				   context, cam_id);
			return IMF_RET_INVALID_PARAMETER;
		}
	} else {
		//Todo later
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int get_snr_gain_imp(void *context, enum camera_id cam_id,
	unsigned int __maybe_unused *ana_gain,
	unsigned int __maybe_unused *dig_gain)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p, cid:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int get_snr_gain_cap_imp(void *context, enum camera_id cam_id,
				struct para_capability_u32 *ana,
				struct para_capability_u32 *dig)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp_snr_get_gain_caps(isp, cam_id, ana, dig) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_snr_get_caps,context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_snr_itime_imp(void *context, enum camera_id cam_id,
	unsigned int itime)
{
	struct isp_context *isp;
	struct sensor_info *snr_info;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	snr_info = &isp->sensor_info[cam_id];

	if ((itime < snr_info->sensor_cfg.exposure.min_integration_time)
	|| (itime > snr_info->sensor_cfg.exposure.max_integration_time)) {
		unsigned int min = 0;
		unsigned int max = itime + 1000;
		//CmdAeSetItimeRange_t i_time;
		enum fw_cmd_resp_stream_id stream_id;

		if (max < 500000)
			max = 500000;

		stream_id = isp_get_stream_id_from_cid(isp, cam_id);
		//i_time.minItime = min;
		//i_time.maxItime = max;

		/*
		 * if (isp_send_fw_cmd(isp,
		 *		CMD_ID_AE_SET_ITIME_RANGE,
		 *		stream_id,
		 *		FW_CMD_PARA_TYPE_DIRECT,
		 *		&i_time, sizeof(i_time)) != RET_SUCCESS) {
		 * ISP_PR_ERR("fail set range[%u,%u] for %u", min, max, itime);
		 * return IMF_RET_INVALID_PARAMETER;
		 *}
		 */

		snr_info->sensor_cfg.exposure.min_integration_time = min;
		snr_info->sensor_cfg.exposure.max_integration_time = max;
		//ISP_PR_INFO("itime range (%d,%d)",
			//i_time.minItime, i_time.maxItime);

		ISP_PR_INFO("cid[%d],itime %u,reset range[%u,%u]",
			    cam_id, itime, min, max);
	} else {
		ISP_PR_INFO(" cam %d, itime %u", cam_id, itime);
	}

	if (isp->fw_ctrl_3a &&
	(isp->drv_settings.set_itime_by_sensor_drv != 1)) {
		if (isp_fw_set_itime(isp, cam_id, itime) != RET_SUCCESS) {
			ISP_PR_ERR("fail for set_itime, context:%p, cid:%d",
				   context, cam_id);
			return IMF_RET_INVALID_PARAMETER;
		}
	} else {
		unsigned int set_gain;
		unsigned int set_integration;
		int tmp_ret;

		tmp_ret = isp_snr_exposure_ctrl(isp, cam_id,
						0,
						itime,
						&set_gain, &set_integration);
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int get_snr_itime_imp(void *context, enum camera_id cam_id,
		unsigned int *itime)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id) || (itime == NULL)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (isp->p_sensor_ops[cam_id]->get_itime(cam_id, itime)
	== RET_SUCCESS) {
		ISP_PR_INFO("suc,cid[%d],itime %u", cam_id, *itime);
		return IMF_RET_SUCCESS;
	}

	ISP_PR_ERR("fail,cid[%d]", cam_id);
	return IMF_RET_FAIL;
}

int get_snr_itime_cap_imp(void *context, enum camera_id cam_id,
			unsigned int *min, unsigned int *max,
			unsigned int *step)
{
	struct isp_context *isp;
	struct para_capability_u32 para;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp_snr_get_itime_caps(isp, cam_id, &para) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_snr_get_caps,context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	if (max)
		*max = para.max;
	if (min)
		*min = para.min;
	if (step)
		*step = para.step;

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

void reg_fw_parser_imp(void *context, enum camera_id cam_id,
			struct fw_parser_operation_t *parser)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (parser == NULL)) {
		ISP_PR_ERR("fail for bad context");
		return;
	}

	ISP_PR_INFO("suc, cid[%d], ops:%p", cam_id, parser);

	isp->fw_parser[cam_id] = parser;
}

void unreg_fw_parser_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for bad para");
		return;
	}

	ISP_PR_INFO("suc, cid[%d]", cam_id);

	isp->fw_parser[cam_id] = NULL;
}

int set_fw_gv_imp(void *context, enum camera_id id, enum fw_gv_type type,
		    struct _ScriptFuncArg_t *gv)
{
	struct isp_context *isp;
//	isp_sensor_ctrl_script_t *script;
//	isp_sensor_ctrl_script_func_arg_t *para;
	struct _ScriptFuncArg_t *para;

	if (!is_para_legal(context, id) || (gv == NULL)) {
		ISP_PR_ERR("fail for illegal para,context:%p,cid[%d]type %d",
			   context, id, type);
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
		ISP_PR_ERR("fail for illegal para, context:%p,cid[%d]type:%d",
			   context, id, type);

		return IMF_RET_INVALID_PARAMETER;
	}

	memcpy(para, gv, sizeof(struct _ScriptFuncArg_t));

	ISP_PR_INFO("succ for cam:%d,type %d", id, type);

	return IMF_RET_SUCCESS;
}

int set_digital_zoom_imp(void *context, enum camera_id cam_id,
	unsigned int __maybe_unused h_off,
	unsigned int __maybe_unused v_off,
	unsigned int __maybe_unused width,
	unsigned int __maybe_unused height)
{
	struct isp_context *isp;
	struct _CmdSetZoomWindow_t zoom_command;
	int result;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id) || (width == 0) || (height == 0)) {
		ISP_PR_ERR("fail bad para,isp[%p] cid[%d],w:h(%u:%u)",
			   context, cam_id, width, height);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	zoom_command.window.h_offset = h_off;
	zoom_command.window.v_offset = v_off;
	zoom_command.window.h_size = width;
	zoom_command.window.v_size = height;
	result = isp_fw_set_zoom_window(isp, cam_id, zoom_command.window);

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for send cmd, cid[%d]", cam_id);
		return IMF_RET_FAIL;
	}

	/*save the zoom window here */
	isp->sensor_info[cam_id].zoom_info.window = zoom_command.window;

	/*===do not wait the return of zoom command here*/
	/*return aidt_isp_event_wait(&g_isp_context.zoom_command); */

	ISP_PR_INFO("suc,cid[%d],h_off %u,v_off %u,w %u,h %u",
		    cam_id, h_off, v_off, width, height);

	return IMF_RET_SUCCESS;
}

int set_zsl_buf_imp(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buf_hdl)
{
	int ret;

	ret = set_stream_buf_imp(context, cam_id, buf_hdl, STREAM_ID_ZSL);

	return ret;
}

int start_zsl_imp(void *context, enum camera_id cid, bool is_perframe_ctl)
{
	int ret;

	ret = start_stream_imp(context, cid);

	return ret;
}


int stop_zsl_imp(void *context, enum camera_id cam_id, bool pause)
{
	int ret;

	ret = stop_stream_imp(context, cam_id, STREAM_ID_ZSL, pause);


	return ret;
}


int set_digital_zoom_ratio_imp(void *context, enum camera_id cam_id,
					unsigned int zoom)
{
	struct isp_context *isp;
	int result;
	struct _Window_t zoom_wnd;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if ((zoom < (unsigned int)isp->isp_cap.zoom.min)
	|| (zoom > (unsigned int)isp->isp_cap.zoom.max)) {
		ISP_PR_ERR("fail for bad zoom %d", zoom);
		return IMF_RET_INVALID_PARAMETER;
	}

	result = isp_get_zoom_window
			(isp->sensor_info[cam_id].asprw_wind.h_size,
			isp->sensor_info[cam_id].asprw_wind.v_size,
			zoom, &zoom_wnd);

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail by isp_get_zoom_window,cid %u,zoom %d",
			   cam_id, zoom);
	}

	isp->sensor_info[cam_id].zoom_info.ratio = zoom;
	result =
		set_digital_zoom_imp(isp, cam_id, zoom_wnd.h_offset,
				zoom_wnd.v_offset, zoom_wnd.h_size,
				zoom_wnd.v_size);

	ISP_PR_INFO("ret %d", result);

	return result;
}

int set_digital_zoom_pos_imp(void *context, enum camera_id cam_id,
	short __maybe_unused h_off, short __maybe_unused v_off)
{
	struct isp_context *isp;
	int result = RET_FAILURE;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	ISP_PR_ERR("ret %d for not implemented", result);

	return result;
}

int set_metadata_buf_imp(void *context, enum camera_id cam_id,
			   struct sys_img_buf_handle *buf_hdl)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	int result;
	int ret = IMF_RET_SUCCESS;

	if (!is_para_legal(context, cam_id) || (buf_hdl == NULL)
	|| (buf_hdl->virtual_addr == 0)) {
		ISP_PR_ERR("fail for illegal para,context:%p,cid[%d]buf_hdl:%p",
			   context, cam_id, buf_hdl);
		return IMF_RET_INVALID_PARAMETER;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (buf_hdl == NULL) {
		ISP_PR_ERR("fail for sys_img_buf_handle_cpy");
		return IMF_RET_FAIL;
	}

	gen_img =
		isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
			cam_id, STREAM_ID_PREVIEW, buf_hdl->len, 0, 0);

	if (gen_img == NULL) {
		ISP_PR_ERR("fail for create isp buffer fail");
		ret = IMF_RET_FAIL;
		goto quit;
	}

	ISP_PR_INFO("cid[%d],sys:%p,mc:%llx(%u)", cam_id,
		    buf_hdl->virtual_addr, gen_img->y_map_info.mc_addr,
		    gen_img->y_map_info.len);

	result = isp_fw_send_metadata_buf(isp, cam_id, gen_img);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail1 for isp_fw_send_metadata_buf fail");
		ret = IMF_RET_FAIL;
		goto quit;
	}

	isp_list_insert_tail(&isp->sensor_info[cam_id].meta_data_in_fw,
				(struct list_node *)gen_img);
	ISP_PR_INFO("suc");
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
}

int get_camera_dev_info_imp(void *context, enum camera_id cid,
				struct camera_dev_info *cam_dev_info)
{
	struct sensor_hw_parameter para;
	struct isp_context *isp = (struct isp_context *)context;

	if (!is_para_legal(context, cid) || cam_dev_info == NULL) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d],info:%p",
			   context, cid, cam_dev_info);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (cid == CAMERA_ID_MEM) {
		memcpy(cam_dev_info, &isp->cam_dev_info[cid],
			sizeof(struct camera_dev_info));
		ISP_PR_INFO("suc,cid[%d],info:%p", cid, cam_dev_info);
		return IMF_RET_SUCCESS;
	}

	if (isp_snr_get_hw_parameter(isp, cid, &para) == RET_SUCCESS) {
		isp->cam_dev_info[cid].type = para.type;
		isp->cam_dev_info[cid].focus_len = para.focus_len;
	}

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
	}

	ISP_PR_INFO("suc for cid[%d]", cid);
	return IMF_RET_SUCCESS;
}
int get_caps_data_format_imp(void *context, enum camera_id cam_id,
			enum pvt_img_fmt fmt)
{
	switch (fmt) {
	// add more
	case PVT_IMG_FMT_YV12:
	case PVT_IMG_FMT_I420:
	case PVT_IMG_FMT_NV21:
	case PVT_IMG_FMT_NV12:
	case PVT_IMG_FMT_YUV422P:
		return 0;
	default:
		ISP_PR_ERR("fail for not supported");
		return -1;
	}
}

int get_caps_imp(void *context, enum camera_id cid)
{
	int ret = RET_SUCCESS;
	struct isp_context *isp = (struct isp_context *)context;

	ret = isp_get_caps(isp, cid);
	if (ret != RET_SUCCESS)
		ISP_PR_ERR("cam[%d] fail", cid);

	return ret;
}

int set_wb_colorT_imp(void *context, enum camera_id cam_id,
	unsigned int colorT)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

#if	defined(PER_FRAME_CTRL)
	if (isp->sensor_info[cam_id].wb_idx == NULL) {
		ISP_PR_ERR("no wb_idx in calib data");
		return IMF_RET_INVALID_PARAMETER;
	}
#endif

	isp->sensor_info[cam_id].para_color_temperature_set = colorT;
	if (isp->sensor_info[cam_id].para_color_temperature_cur ==
		isp->sensor_info[cam_id].para_color_temperature_set) {
		ISP_PR_INFO("not diff value, do none");
		return RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_color_temperature_cur =
		isp->sensor_info[cam_id].para_color_temperature_set;

	if (isp_fw_set_wb_colorT(isp, cam_id, colorT) !=
		RET_SUCCESS) {
		ISP_PR_ERR("fail,cid %u,colorT %u", cam_id, colorT);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc, cid %u, colorT %u", cam_id, colorT);
	return IMF_RET_SUCCESS;
}

int set_wb_light_imp(void *context, enum camera_id cam_id,
	unsigned int light_idx)
{
	struct isp_context *isp;
	unsigned char light;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

#if	defined(PER_FRAME_CTRL)
	if (isp->sensor_info[cam_id].wb_idx == NULL) {
		ISP_PR_ERR("no wb_idx in calib data");
		return IMF_RET_INVALID_PARAMETER;
	}
#endif

#if	defined(PER_FRAME_CTRL)
	light =
	(unsigned char) isp->sensor_info[cam_id].wb_idx->index_map[light_idx];
#else
	light = (unsigned char)light_idx;
#endif

	if (isp_fw_set_wb_light(isp, cam_id, light)
	!= RET_SUCCESS) {
		ISP_PR_ERR("fail,cid %u,light %u", cam_id, light_idx);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc,cid %u,light %u", cam_id, light_idx);

	return IMF_RET_SUCCESS;
}

int set_wb_gain_imp(void *context, enum camera_id cam_id,
	struct _WbGain_t wb_gain)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

#if	defined(PER_FRAME_CTRL)
	if (isp->sensor_info[cam_id].wb_idx == NULL) {
		ISP_PR_ERR("no wb_idx in calib data");
		return IMF_RET_INVALID_PARAMETER;
	}
#endif

	if (isp_fw_set_wb_gain(isp, cam_id, wb_gain)
		!= RET_SUCCESS) {
		ISP_PR_ERR("fail,cid %u", cam_id);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc,cid %u", cam_id);
	return IMF_RET_SUCCESS;
}

int disable_color_effect_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

static int map_to_roi_window_from_preview(
					struct _Window_t *roi_zoom,
					struct _Window_t *window_zoom,
					struct _Window_t *roi_preview,
					unsigned int preview_width,
					unsigned int preview_height)
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

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_awb_roi_imp(void *context, enum camera_id cam_id,
	struct _Window_t *window)
{
#if	defined(PER_FRAME_CTRL)
	struct isp_context *isp;
	CmdAwbSetRegion_t cmd;
	enum fw_cmd_resp_stream_id stream_id;
	struct _Window_t window_roi = { 0, 0, 0, 0 }
//	int result;

	if (!is_para_legal(context, cam_id)
	|| (window == NULL)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("fail fsm %d, cid %u", ISP_GET_STATUS(isp), cam_id);
		return IMF_RET_FAIL;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
//	cmd.window = *(struct _Window_t *)window;

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

	if (isp_fw_set_wb_region(isp, cam_id,
			cmd.window) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cid %u,[%u,%u,%u,%u]", cam_id,
			   cmd.window.h_offset, cmd.window.v_offset,
			   cmd.window.h_size, cmd.window.v_size);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc,cid %u,[%u,%u,%u,%u]", cam_id,
		    cmd.window.h_offset, cmd.window.v_offset,
		    cmd.window.h_size, cmd.window.v_size);
#endif
	return IMF_RET_SUCCESS;
}

int set_ae_roi_imp(void *context, enum camera_id cam_id,
	struct _Window_t *window)
{
	struct isp_context *isp;
	struct _Window_t window_roi = { 0, 0, 0, 0 };

	if (!is_para_legal(context, cam_id) || (window == NULL)) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("fail fsm %d, cid %u", ISP_GET_STATUS(isp), cam_id);
		return IMF_RET_FAIL;
	}

	if (isp->sensor_info[cam_id].zoom_info.window.h_size == 0 &&
	isp->sensor_info[cam_id].zoom_info.window.v_size == 0) {
		struct _Window_t asprw_wind =
			isp->sensor_info[cam_id].asprw_wind;
		struct _Window_t base_wind = asprw_wind;

/*		In Hello and RGBIR camera case,
 *		AE ROI should be mapped to the crop window
 */
		if (isp->sensor_info[cam_id].cam_type &&
		isp->sensor_info[cam_id].face_auth_mode == CAMERA_TYPE_RGBIR) {
			struct _Window_t ir_crop_window;

			if (isp_get_rgbir_crop_window(isp, CAMERA_ID_MEM,
			&ir_crop_window) != RET_SUCCESS) {
				ISP_PR_ERR("fail,cid %u,cannot get crop window",
					   cam_id);
				return IMF_RET_FAIL;
			}

			base_wind = ir_crop_window;
		}

/*		No Digital Zoom applied, so we need to use
 *		aspect ratio windows as stage 1 RAW output resolution
 */
		map_to_roi_window_from_preview(&window_roi, &base_wind, window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);

/*		In Hello and RGBIR camera case, AE ROI should be
 *		based on HALF of the aspect ratio window
 */
		if (isp->sensor_info[cam_id].cam_type &&
		isp->sensor_info[cam_id].face_auth_mode == CAMERA_TYPE_RGBIR) {
			window_roi.h_offset +=
				(asprw_wind.h_size / 2 - base_wind.h_size) / 2;
			window_roi.v_offset +=
				(asprw_wind.v_size / 2 - base_wind.v_size) / 2;
		}
	} else {	// Digital zoom case
		struct _Window_t base_wind =
			isp->sensor_info[cam_id].zoom_info.window;

		map_to_roi_window_from_preview(&window_roi, &base_wind, window,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].width,
		isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].height);

		// Apply offsets per Digital Zoom
		window_roi.h_offset +=
			isp->sensor_info[cam_id].zoom_info.window.h_offset;
		window_roi.v_offset +=
			isp->sensor_info[cam_id].zoom_info.window.v_offset;
	}

	// In Hello and RGBIR camera case, AE ROI should be set to IR stream
	if (isp->sensor_info[cam_id].cam_type == CAMERA_TYPE_RGBIR &&
	isp->sensor_info[cam_id].face_auth_mode) {
		cam_id = CAMERA_ID_MEM;
	} else if (isp->drv_settings.min_ae_roi_size_filter > 0) {
//		A workaround to filter out small AE ROI
//		settings if it is enabled in registry
		if (window_roi.h_size > 0 &&
		window_roi.v_size > 0 &&
		window_roi.h_size * window_roi.v_size <
		isp->drv_settings.min_ae_roi_size_filter) {
			if (isp->drv_settings.reset_small_ae_roi) {
				ISP_PR_INFO("cid %u,[%u,%u,%u,%u] reset to def",
					    cam_id,
					    window_roi.h_offset,
					    window_roi.v_offset,
					    window_roi.h_size,
					    window_roi.v_size);

				memset(&window_roi, 0,
					sizeof(struct _Window_t));
			} else {
				ISP_PR_ERR
			("fail,cid %u,[%u,%u,%u,%u] smaller than threshold %u",
					cam_id, window_roi.h_offset,
					window_roi.v_offset, window_roi.h_size,
					window_roi.v_size,
				isp->drv_settings.min_ae_roi_size_filter);
				return IMF_RET_FAIL;
			}
		}
	}

	if (isp_fw_set_ae_region(isp, cam_id,
			window_roi) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cid %u,[%u,%u,%u,%u]", cam_id,
			   window_roi.h_offset, window_roi.v_offset,
			   window_roi.h_size, window_roi.v_size);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc,cid %u,[%u,%u,%u,%u]", cam_id,
		    window_roi.h_offset, window_roi.v_offset,
		    window_roi.h_size, window_roi.v_size);
	return IMF_RET_SUCCESS;
}

int start_wb_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;
//	int result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int stop_wb_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;
	enum _SensorId_t sensor_id;
//	int result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int start_ae_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;
	enum _SensorId_t sensor_id;
//	int result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int stop_ae_imp(void *context, enum camera_id cam_id)
{
	struct isp_context *isp;
	enum _SensorId_t sensor_id;
//	int result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail for illegal para, context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sensor_id = cam_id;
	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int set_scene_mode_imp(void *context, enum camera_id cid,
			 enum isp_scene_mode mode)
{
	struct isp_context *isp;
	struct _scene_mode_table *sv;
	enum fw_cmd_resp_stream_id stream_id;
	int result;
	struct sensor_info *sif = NULL;

	if (!is_para_legal(context, cid) || (mode >= ISP_SCENE_MODE_MAX)) {
		ISP_PR_ERR("fail bad para,cid[%d],mode %s",
			   cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;

	if (isp->sensor_info[cid].scene_tbl == NULL) {
		ISP_PR_ERR("fail no calib,cid[%d],mode %s",
			   cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_FAIL;
	}

	sv = isp->sensor_info[cid].scene_tbl;
	isp->sensor_info[cid].scene_mode = mode;
	if (!sv->sm_valid[mode]) {
		ISP_PR_ERR("fail mode not support,cid[%d], %s",
			   cid, isp_dbg_get_scene_mode_str(mode));
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO(" cid[%d], mode %s", cid, isp_dbg_get_scene_mode_str(mode));

	stream_id = isp_get_stream_id_from_cid(isp, cid);

	{
		//CmdAeSetSetpoint_t para;

		//if (sv->ae_sp[mode] == -1)
			//para.setPoint = (unsigned int) -1;
		//else
			//para.setPoint = sv->ae_sp[mode] * 1000;

		/*
		 *result =
		 *	isp_send_fw_cmd(isp, CMD_ID_AE_SET_SETPOINT,
		 *		stream_id, FW_CMD_PARA_TYPE_DIRECT,
		 *		&para, sizeof(para));
		 *if (result == RET_SUCCESS) {
		 *	ISP_PR_INFO("set ae point %d suc",
		 *		sv->ae_sp[mode]);
		 *} else {
		 *	ISP_PR_INFO("set ae point %d fail",
		 *		sv->ae_sp[mode]);
		 *}
		 */
	}

	{
		//CmdAeSetIso_t para;

		//if (sv->ae_iso[mode] == -1)
			//para.iso.iso = AE_ISO_AUTO;
		//else
			//para.iso.iso = sv->ae_iso[mode];
		/*
		 *result =
		 *	isp_send_fw_cmd(isp, CMD_ID_AE_SET_ISO,
		 *		stream_id, FW_CMD_PARA_TYPE_DIRECT,
		 *		&para, sizeof(para));
		 *if (result == RET_SUCCESS) {
		 *	ISP_PR_INFO("set ae_iso %d suc",
		 *		sv->ae_iso[mode]);
		 *} else {
		 *	ISP_PR_INFO("set ae_iso %d fail",
		 *		sv->ae_iso[mode]);
		 *}
		 */
	}

	if ((sv->ae_fps[mode][0] != -1)
		&& (sv->ae_fps[mode][1] != -1)) {
	//CmdAeSetItimeRange_t para = {0}

	//para.minItime = 0;

	//if (sv->ae_maxitime[mode] > 0)
		//para.maxItime = (unsigned int) (min(sv->ae_maxitime[mode],
				//1000.0f / sv->ae_fps[mode][0]) * 990);
	//else
		//para.maxItime =
			//(unsigned int) (1000.0f / sv->ae_fps[mode][0] * 990);
	//if (isp->drv_settings.low_light_fps)
		//para.maxItimeLowLight =
			//1000000 / isp->drv_settings.low_light_fps;

	/*
	 *result =
	 *	isp_send_fw_cmd(isp, CMD_ID_AE_SET_ITIME_RANGE,
	 *			stream_id, FW_CMD_PARA_TYPE_DIRECT,
	 *			&para, sizeof(para));
	 *if (result == RET_SUCCESS) {
	 *	ISP_PR_INFO
	 *("set ae_fps [%u,%u] suc,itime range normal(%d,%d),lowlight(%d,%d)",
	 *		sv->ae_fps[mode][0], sv->ae_fps[mode][1],
	 *		para.minItime, para.maxItime,
	 *		para.minItimeLowLight, para.maxItimeLowLight);
	 *} else {
	 *	ISP_PR_INFO("set ae_fps [%u,%u] fail",
	 *		sv->ae_fps[mode][0], sv->ae_fps[mode][1]);
	 *}
	 */
	} else {
		ISP_PR_INFO("no need set ae_fps");
	}

	if (isp->drv_settings.awb_default_auto != 0) {
		// User explicitly disabled AWB
		struct _CmdAwbSetMode_t para;

		if (sv->awb_index[mode] == -1)
			para.mode = AWB_MODE_AUTO;
		else
			para.mode = AWB_MODE_MANUAL;

		result =
			isp_fw_set_wb_mode(isp, cid,
				para.mode);
		if (result == RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_INFO("set awb %d suc", sv->ae_iso[mode]);
		} else {
			//add this line to trick CP
			ISP_PR_INFO("set awb %d fail", sv->ae_iso[mode]);
		}

		if (sv->awb_index[mode] != -1) {
			unsigned int lightIndex;

			lightIndex = (unsigned char) sv->awb_index[mode];
			result =
				isp_fw_set_wb_light(isp, cid, lightIndex);
			if (result == RET_SUCCESS) {
				ISP_PR_INFO("set awb light %d suc",
					    sv->ae_iso[mode]);
			} else {
				ISP_PR_INFO("set awb light%d fail",
					    sv->ae_iso[mode]);
			}
		}
	}

	sif = &isp->sensor_info[cid];

	if ((sv->af[mode][0] != -1)
	&& (sv->af[mode][1] != -1)
	&& (sif->sensor_cfg.lens_pos.max_pos != 0)) {
		struct _CmdAfSetLensRange_t para;

		para.lensRange.minLens = sv->af[mode][0];
		para.lensRange.maxLens = sv->af[mode][1];
		para.lensRange.stepLens = 10;

		result = isp_fw_set_lens_range(isp, cid,
					para.lensRange);
		if (result == RET_SUCCESS) {
			ISP_PR_INFO("set af [%u,%u] suc",
				    sv->af[mode][0], sv->af[mode][1]);
		} else {
			ISP_PR_INFO("set af [%u,%u] fail",
				    sv->af[mode][0], sv->af[mode][1]);
		}
	}

	if ((sv->denoise[mode][0] != -1)
	&& (sv->denoise[mode][1] != -1)
	&& (sv->denoise[mode][2] != -1)
	&& (sv->denoise[mode][3] != -1)
	&& (sv->denoise[mode][4] != -1)
	&& (sv->denoise[mode][5] != -1)
	&& (sv->denoise[mode][6] != -1)) {

		/*
		 *CmdFilterSetCurve_t para;
		 *para.curve.coeff[0] = (unsigned short) sv->denoise[mode][0];
		 *para.curve.coeff[1] = (unsigned short) sv->denoise[mode][1];
		 *para.curve.coeff[2] = (unsigned short) sv->denoise[mode][2];
		 *para.curve.coeff[3] = (unsigned short) sv->denoise[mode][3];
		 *para.curve.coeff[4] = (unsigned short) sv->denoise[mode][4];
		 *para.curve.coeff[5] = (unsigned short) sv->denoise[mode][5];
		 *para.curve.coeff[6] = (unsigned short) sv->denoise[mode][6];
		 *
		 *result = isp_send_fw_cmd(isp, CMD_ID_FILTER_SET_CURVE,
		 *			stream_id, FW_CMD_PARA_TYPE_DIRECT,
		 *			&para, sizeof(para));
		 *if (result == RET_SUCCESS) {
		 *	ISP_PR_INFO("set denoise suc");
		 *} else {
		 *	ISP_PR_INFO("set denoise fail");
		 *}
		 */
	}

	if ((sv->cproc[mode][0] >= SATURATION_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][0] <= SATURATION_MAX_VALUE_IN_FW)) {
		fw_if_set_satuation(isp, cid, sv->cproc[mode][0]);
	}

	if ((sv->cproc[mode][1] >= BRIGHTNESS_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][1] <= BRIGHTNESS_MAX_VALUE_IN_FW)) {
		fw_if_set_brightness(isp, cid, sv->cproc[mode][1]);
	}

	if ((sv->cproc[mode][2] >= HUE_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][2] <= HUE_MAX_VALUE_IN_FW)) {
		fw_if_set_hue(isp, cid, sv->cproc[mode][2]);
	}

	if ((sv->cproc[mode][3] >= CONTRAST_MIN_VALUE_IN_FW)
	&& (sv->cproc[mode][3] <= CONTRAST_MAX_VALUE_IN_FW)) {
		fw_if_set_contrast(isp, cid, sv->cproc[mode][3]);
	}

	if ((sv->sharpness[mode][0] != -1)
	&& (sv->sharpness[mode][1] != -1)
	&& (sv->sharpness[mode][2] != -1)
	&& (sv->sharpness[mode][3] != -1)
	&& (sv->sharpness[mode][4] != -1)) {

		/*
		 *CmdFilterSetSharpen_t para;
		 *
		 *para.curve.coeff[0] = (char) sv->sharpness[mode][0];
		 *para.curve.coeff[1] = (char) sv->sharpness[mode][1];
		 *para.curve.coeff[2] = (char) sv->sharpness[mode][2];
		 *para.curve.coeff[3] = (char) sv->sharpness[mode][3];
		 *para.curve.coeff[4] = (char) sv->sharpness[mode][4];
		 *result = isp_send_fw_cmd(isp, CMD_ID_FILTER_SET_SHARPEN,
		 *			stream_id, FW_CMD_PARA_TYPE_DIRECT,
		 *			&para, sizeof(para));
		 *if (result == RET_SUCCESS) {
		 *	ISP_PR_INFO("set sharpness suc");
		 *} else {
		 *	ISP_PR_INFO("set sharpness fail");
		 *}
		 */
	}

	if (sv->flashlight[mode] != 2) {
		#if	defined(PER_FRAME_CTRL)
		unsigned int/*CmdAeSetFlashMode_t*/ para;
		if (sv->flashlight[mode] == -1)
			para = FLASH_MODE_AUTO;
		else if (sv->flashlight[mode] == 1)
			para = FLASH_MODE_ON;
		else
			para = FLASH_MODE_OFF;
		#endif
		result = RET_SUCCESS;
		//todo to be added later

		if (result == RET_SUCCESS) {
			ISP_PR_INFO("set flashlight %d suc",
				    sv->flashlight[mode]);
		} else {
			ISP_PR_INFO("set flashlight %d fail",
				    sv->flashlight[mode]);
		}
	} else {
		ISP_PR_INFO("set flashlight %d, ignore", sv->flashlight[mode]);
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_snr_calib_data_imp(void *context, enum camera_id cam_id,
			     void *data, unsigned int len)
{
	struct isp_context *isp;

	int result = RET_SUCCESS;

	if (!is_para_legal(context, cam_id)
	|| (data == NULL)
	|| (len == 0)) {
		ISP_PR_ERR("fail for illegal para,context:%p,cam_id:%d",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;
	/*to be added later*/

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("(cam_id:%d) fail for send cmd", cam_id);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("(cam_id:%d) suc", cam_id);
	return IMF_RET_SUCCESS;
}

int map_handle_to_vram_addr_imp(void *context, void *handle,
				unsigned long long *vram_addr)
{
	struct isp_context *isp;

	if ((context == NULL)
	|| (handle == NULL)
	|| (vram_addr == NULL)) {
		ISP_PR_ERR("fail para,isp %p,hdl %p,addr %p",
			   context, handle, vram_addr);
		return IMF_RET_INVALID_PARAMETER;
	}
	isp = (struct isp_context *)context;

	ISP_PR_ERR("fail to be implemented");
	return IMF_RET_FAIL;
}

int set_drv_settings_imp(void *context,
	struct driver_settings *setting)
{
	struct isp_context *isp = context;

	if ((context == NULL)
	|| (setting == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	memcpy(&isp->drv_settings, setting, sizeof(isp->drv_settings));
	//just add to test acpi function
	//isp_acpi_set_sensor_pwr(isp, CAMERA_ID_FRONT_LEFT, 1);
	//isp_acpi_fch_clk_enable(isp, 0);
	dbg_show_drv_settings(&isp->drv_settings);

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int enable_dynamic_frame_rate_imp(void *context,
				enum camera_id cam_id,
				bool enable)
{
	struct isp_context *isp = context;
	int ret = RET_SUCCESS;
	//CmdAeSetItimeRange_t i_time = {0}
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	ISP_PR_INFO("cid[%d],enable %d", cam_id, enable);

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp->sensor_info[cam_id].enable_dynamic_frame_rate = 1;
		ISP_PR_INFO(" set later for fw not run");
		return IMF_RET_SUCCESS;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];

	//i_time.minItime = sif->sensor_cfg.exposure.min_integration_time;
	//i_time.maxItime = sif->sensor_cfg.exposure.max_integration_time;

	if (enable && isp->drv_settings.low_light_fps) {
		//i_time.maxItimeLowLight =
			//1000000 / isp->drv_settings.low_light_fps;
	}

	if (ret == RET_SUCCESS) {
		RET(IMF_RET_SUCCESS);
		return IMF_RET_SUCCESS;
	}

	RET(IMF_RET_FAIL);
	return IMF_RET_FAIL;
}

int set_max_frame_rate_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, unsigned int frame_rate_numerator,
			unsigned int frame_rate_denominator)
{
	struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (frame_rate_denominator == 0)
	|| ((frame_rate_numerator / frame_rate_denominator) == 0)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}
	sif = &isp->sensor_info[cam_id];
	if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
		ISP_PR_ERR("fail bad stream status,%d",
			   sif->str_info[sid].start_status);
		return IMF_RET_FAIL;
	}
	sif->str_info[sid].max_fps_numerator = frame_rate_numerator;
	sif->str_info[sid].max_fps_denominator = frame_rate_denominator;
	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int set_frame_rate_info_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, unsigned int frame_rate)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (frame_rate == 0)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_frame_rate_info(isp, cam_id, frame_rate)
		!= RET_SUCCESS) {
		ISP_PR_ERR("fail for set frame rate info");
		return RET_FAILURE;
	}

	ISP_PR_INFO(" set fps %d", frame_rate);
	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int set_flicker_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _AeFlickerType_t mode)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (fw_if_set_flicker_mode(isp,
				cam_id,
				mode)
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail for fw_if_set_flicker_mode");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int set_iso_priority_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _AeIso_t *iso)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (iso == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	isp->sensor_info[cam_id].para_iso_set = *iso;

	if (isp->sensor_info[cam_id].para_iso_cur.iso ==
	isp->sensor_info[cam_id].para_iso_set.iso) {
		ISP_PR_INFO("not diff value, do none");
		return IMF_RET_SUCCESS;
	}

	isp->sensor_info[cam_id].para_iso_cur =
		isp->sensor_info[cam_id].para_iso_set;

	if (isp_fw_set_iso(isp, cam_id,
	isp->sensor_info[cam_id].para_iso_cur.iso) != RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int set_ev_compensation_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _AeEv_t *ev)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (ev == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	isp->sensor_info[cam_id].para_ev_compensate_set = *ev;

	ISP_PR_INFO("cid[%d],set ev to %u/%u", cam_id,
		   isp->sensor_info[cam_id].para_ev_compensate_set.numerator,
		   isp->sensor_info[cam_id].para_ev_compensate_set.denominator);

#if	defined(PER_FRAME_CTRL)
	if (
	(isp->sensor_info[cam_id].para_ev_compensate_cur.numerator *
	isp->sensor_info[cam_id].para_ev_compensate_set.denominator) ==
	(isp->sensor_info[cam_id].para_ev_compensate_set.numerator *
	isp->sensor_info[cam_id].para_ev_compensate_cur.denominator)) {
		ISP_PR_INFO("not diff value, do none");
		return RET_SUCCESS;
	}
#endif

	isp->sensor_info[cam_id].para_ev_compensate_cur =
		isp->sensor_info[cam_id].para_ev_compensate_set;

	if (fw_if_set_ev_compensation(isp, cam_id,
		isp->sensor_info[cam_id].para_ev_compensate_cur) !=
		RET_SUCCESS) {
		ISP_PR_ERR("fail,cam_id:%d", cam_id);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int sharpen_enable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t *sharpen_id)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (sharpen_id == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_sharpen_enable(isp,
				cam_id, sharpen_id)
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail for sharpen enable");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int sharpen_disable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t *sharpen_id)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (sharpen_id == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_sharpen_disable(isp,
				cam_id, sharpen_id)
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail for set sharpen disable");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int sharpen_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t sharpen_id,
			struct _TdbSharpRegByChannel_t param)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_sharpen_config(isp, cam_id,
		sharpen_id, param) != RET_SUCCESS) {
		ISP_PR_ERR("fail for sharpen config");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int clk_gating_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_clk_gating_enable(isp, cam_id,
		enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for set clk gating:%d", enable);
		return RET_FAILURE;
	}

	ISP_PR_INFO(" clk gating enable:%d", enable);

	return IMF_RET_SUCCESS;
}

int power_gating_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_power_gating_enable(isp, cam_id,
		enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for set power gating:%d", enable);
		return RET_FAILURE;
	}

	ISP_PR_INFO(" power gating enable:%d", enable);

	return IMF_RET_SUCCESS;
}

int dump_raw_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable)
{
	struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	sif = &isp->sensor_info[cam_id];
	sif->dump_raw_enable = enable;

	ISP_PR_INFO("success to enable dump raw, cam_id:%d, enable:%d",
		cam_id, enable);

	return IMF_RET_SUCCESS;
}

int image_effect_enable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, unsigned int enable)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) ||
		(sid > STREAM_ID_NUM)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_image_effect_enable(isp, cam_id,
		enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for ie enable:%d", enable);
		return RET_FAILURE;
	}

	ISP_PR_INFO(" ie enable:%d", enable);
	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int image_effect_config_imp(void *context, enum camera_id cam_id,
	enum stream_id sid, struct _IeConfig_t *param, int enable)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (param == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_image_effect_config(isp, cam_id,
		param, enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for image effect config");
		return RET_FAILURE;
	}

	ISP_PR_INFO(" image effect config");
	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int set_video_hdr_imp(void *context, enum camera_id cam_id,
	unsigned int enable)
{
	struct isp_context *isp;
	struct sensor_info *sif;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail bad para,isp[%p] cid[%d],enable:%d",
			   context, cam_id, enable);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	sif = &isp->sensor_info[cam_id];
	sif->hdr_enable = (char)enable;
	ISP_PR_INFO("suc,isp[%p] cid[%d],enable:%d", context, cam_id, enable);
	return IMF_RET_SUCCESS;
}

int set_lens_shading_matrix_imp(void *context,
				enum camera_id cam_id,
				struct _LscMatrix_t *matrix)
{
	/*
	 *struct isp_context *isp;
	 *enum fw_cmd_resp_stream_id stream_id;
	 *
	 *if (!is_para_legal(context, cam_id) || (matrix == NULL)) {
	 *	ISP_PR_ERR("fail para,isp[%p] cid[%d]",
	 *		context, cam_id);
	 *	return IMF_RET_INVALID_PARAMETER;
	 *}
	 *
	 *isp = (struct isp_context *)context;
	 *stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	 *ISP_PR_INFO("cid[%d]", cam_id);
	 *
	 *if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LSC_MATRIX,
	 *		stream_id, FW_CMD_PARA_TYPE_INDIRECT,
	 *		matrix, sizeof(struct _LscMatrix_t)) != RET_SUCCESS) {
	 *	ISP_PR_ERR("fail for send cmd");
	 *	return IMF_RET_FAIL;
	 *}
	 *
	 *ISP_PR_INFO("suc");
	 */
	return IMF_RET_SUCCESS;
}

int set_lens_shading_sector_imp(void *context,
				enum camera_id cam_id,
				struct _PrepLscSector_t *sectors)
{
	/*
	 *struct isp_context *isp;
	 *enum fw_cmd_resp_stream_id stream_id;
	 *
	 *if (!is_para_legal(context, cam_id) || (sectors == NULL)) {
	 *	ISP_PR_ERR("fail para,isp[%p] cid[%d]",
	 *		context, cam_id);
	 *	return IMF_RET_INVALID_PARAMETER;
	 *}
	 *
	 *isp = (struct isp_context *)context;
	 *stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	 *ISP_PR_INFO("cid[%d]", cam_id);
	 *
	 *if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_LSC_SECTOR,
	 *		stream_id, FW_CMD_PARA_TYPE_INDIRECT,
	 *		sectors,
	 *		sizeof(CmdAwbSetLscSector_t)) != RET_SUCCESS) {
	 *	ISP_PR_ERR("fail for send cmd");
	 *	return IMF_RET_FAIL;
	 *}
	 *
	 *ISP_PR_INFO("suc");
	 */

	return IMF_RET_SUCCESS;
}

int set_awb_cc_matrix_imp(void *context, enum camera_id cam_id,
				struct _CcMatrix_t *matrix)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id) || (matrix == NULL)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	ISP_PR_INFO("cid[%d]", cam_id);

	if (isp_fw_set_wb_cc_matrix(isp, cam_id, matrix)
		!= RET_SUCCESS) {
		ISP_PR_ERR("fail for send cc matrix");
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int set_awb_cc_offset_imp(void *context, enum camera_id cam_id,
				struct _CcOffset_t *ccOffset)
{
	/*
	 *struct isp_context *isp;
	 *enum fw_cmd_resp_stream_id stream_id;
	 *
	 *if (!is_para_legal(context, cam_id) || (ccOffset == NULL)) {
	 *	ISP_PR_ERR("fail para,isp[%p] cid[%d]",
	 *		context, cam_id);
	 *	return IMF_RET_INVALID_PARAMETER;
	 *}
	 *
	 *isp = (struct isp_context *)context;
	 *stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	 *ISP_PR_INFO("cid[%d]", cam_id);
	 *
	 *if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_CC_OFFSET,
	 *		stream_id, FW_CMD_PARA_TYPE_DIRECT,
	 *		ccOffset, sizeof(struct _CcOffset_t)) != RET_SUCCESS) {
	 *	ISP_PR_ERR("fail for send cmd");
	 *	return IMF_RET_FAIL;
	 *}
	 *
	 *ISP_PR_INFO("suc");
	 */
	return IMF_RET_SUCCESS;
}

int gamma_enable_imp(void *context, enum camera_id cam_id,
		unsigned int enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ISP_PR_INFO("cid[%d]", cam_id);

	if (isp_fw_set_gamma_enable(isp, cam_id,
			enable) != RET_SUCCESS) {
		ISP_PR_ERR("fail for send cmd");
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int tnr_enable_imp(void *context, enum camera_id cam_id,
		unsigned int enable)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	ISP_PR_INFO("cid[%d]", cam_id);

	if (enable && !isp->sensor_info[cam_id].tnr_tmp_buf_set) {
		ISP_PR_ERR("enable fail for buf");
		return IMF_RET_FAIL;
	}

	if (isp_fw_set_tnr_enable(isp, cam_id, enable)
		!= RET_SUCCESS) {
		ISP_PR_ERR("fail for send cmd");
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int snr_enable_imp(void *context, enum camera_id cam_id,
		unsigned int enable)
{
	struct isp_context *isp;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	ISP_PR_INFO("cid[%d]", cam_id);

	if (isp_fw_set_snr_enable(isp, cam_id, enable)
		!= RET_SUCCESS) {
		ISP_PR_ERR("fail for send cmd");
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int snr_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _SinterParams_t *param)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (param == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_snr_config(isp,
				cam_id,
				param)
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail for snr config");
		return RET_FAILURE;
	}

	ISP_PR_INFO(" snr config");

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int tnr_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _TemperParams_t *param)
{
	//struct sensor_info *sif;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || (sid > STREAM_ID_NUM)
	|| (param == NULL)) {
		ISP_PR_ERR("fail bad para");
		return IMF_RET_INVALID_PARAMETER;
	}

	/*
	 *sif = &isp->sensor_info[cam_id];
	 *if (sif->str_info[sid].start_status != START_STATUS_NOT_START) {
	 *	ISP_PR_ERR("fail bad stream status,%d",
	 *		sif->str_info[sid].start_status);
	 *	return IMF_RET_FAIL;
	 *}
	 */
	if (isp_fw_set_tnr_config(isp,
				cam_id,
				param)
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail for tnr config");
		return RET_FAILURE;
	}

	ISP_PR_INFO(" tnr config");

	ISP_PR_INFO("suc, cid[%d], sid %d", cam_id, sid);

	return IMF_RET_SUCCESS;
}

int get_cproc_status_imp(void *context, enum camera_id cam_id,
				struct _CprocStatus_t *cproc)
{
	struct isp_context *isp = (struct isp_context *)context;
	int ret;

	if (!is_para_legal(context, cam_id) || (cproc == NULL)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d],cproc %p",
			   context, cam_id, cproc);
		return IMF_RET_INVALID_PARAMETER;
	}

	ret = isp_fw_get_cproc_status(isp, cam_id, cproc);
	ISP_PR_INFO("cid[%d]", cam_id);

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_fw_get_cproc_status");
		return IMF_RET_FAIL;
	}

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int get_init_para_imp(void *context, void *sw_init,
			void *hw_init,
			void *isp_env)
{
	if (isp_env)
		*((enum isp_environment_setting *)isp_env) = g_isp_env_setting;

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

int lfb_resv_imp(void *context, unsigned int size,
		unsigned long long *sys, unsigned long long *mc)
{
	struct isp_context *isp = context;
	struct isp_gpu_mem_info *ifb_mem_info = NULL;

	if ((isp == NULL)
	|| (size == 0)
	|| (g_cgs_srv->ops->alloc_gpu_mem == NULL)
	|| (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		ISP_PR_ERR("fail no functions");
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("size %uM, isp %p", size / (1024 * 1024), context);

	if (isp->fb_buf && (isp->fb_buf->gpu_mc_addr != 0)
	&& (isp->fb_buf->sys_addr != NULL)) {
		if (sys)
			*sys = (unsigned long long)isp->fb_buf->sys_addr;
		if (mc)
			*mc = isp->fb_buf->gpu_mc_addr;
		ISP_PR_INFO("succ already,mc %llu,mem %p,size %llu",
			    isp->fb_buf->gpu_mc_addr, isp->fb_buf->sys_addr,
			    isp->fb_buf->mem_size / (1024 * 1024));
		return IMF_RET_SUCCESS;
	}


	ifb_mem_info = isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

	if (ifb_mem_info) {
		ISP_PR_INFO("isp_gpu_mem_alloc suc, mc:%llu, sys:%p, len:%llu",
			    ifb_mem_info->gpu_mc_addr,
			    ifb_mem_info->sys_addr,
			    ifb_mem_info->mem_size);
	} else {
		ISP_PR_ERR("isp_gpu_mem_alloc(NLFB) fail");
		return IMF_RET_FAIL;
	}

	isp->fb_buf = ifb_mem_info;
	isp->fb_buf->mem_src = ISP_GPU_MEM_TYPE_NLFB;

	if (sys)
		*sys = (unsigned long long)isp->fb_buf->sys_addr;
	if (mc)
		*mc = isp->fb_buf->gpu_mc_addr;
	ISP_PR_INFO("succ, mc %llu, sys %llu, size %llu",
		    *mc, *sys, isp->fb_buf->mem_size / (1024 * 1024));

	return IMF_RET_SUCCESS;
}

int lfb_free_imp(void *context)
{
	struct isp_context *isp = context;
	enum cgs_result ret;

	if ((isp == NULL)
	|| (g_cgs_srv->ops->alloc_gpu_mem == NULL)
	|| (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		ISP_PR_ERR("fail no functions");
		return IMF_RET_FAIL;
	}

	if (!isp->fb_buf) {
		ISP_PR_ERR("fail no reserv fb_buf");
		return IMF_RET_FAIL;
	}

	if ((isp->fb_buf->sys_addr == 0)
	|| (isp->fb_buf->gpu_mc_addr == 0)
	|| (isp->fb_buf->mem_size == 0)) {
		ISP_PR_ERR("fail no reserv info");
		isp_sys_mem_free(isp->fb_buf);
		isp->fb_buf = NULL;
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("size %llu, sys %p, mc %llu", isp->fb_buf->mem_size,
		    isp->fb_buf->sys_addr,
		isp->fb_buf->gpu_mc_addr);

	ret = isp_gpu_mem_free(isp->fb_buf);
	if (ret != CGS_RESULT__OK) {
		//add this line to trick CP
		ISP_PR_ERR("isp_gpu_mem_free fail 0x%x", ret);
	}
	isp->fb_buf = NULL;
	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}

unsigned int get_index_from_res_fps_id(void *context,
		enum camera_id cid,
		unsigned int res_fps_id)
{
	unsigned int i = 0;
	struct isp_context *isp = (struct isp_context *)context;

	//isp->sensor_info[cid].res_fps.res_fps[res_fps_id].hdr_support;

	for (i = 0; i < isp->sensor_info[cid].res_fps.count; i++) {
		if ((unsigned int) res_fps_id ==
		isp->sensor_info[cid].res_fps.res_fps[i].index) {
			break;
		}

		continue;
	}

	return i;
}

int fw_cmd_send_imp(void *context, enum camera_id cam_id,
			unsigned int cmd, unsigned int is_set_cmd,
			unsigned short is_stream_cmd, unsigned short to_ir,
			unsigned int is_direct_cmd,
			void *para, unsigned int *para_len)
{
#if	defined(PER_FRAME_CTRL)
	struct isp_context *isp = (struct isp_context *)context;
	struct sensor_info *sif;
	int ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	void *package = NULL;
	unsigned int package_size = 0;
	unsigned int resp_by_pl = 1;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("fail para,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (para && ((para_len == NULL) || (*para_len == 0))) {
		ISP_PR_ERR("fail para len,isp[%p] cid[%d]", context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	sif = &isp->sensor_info[cam_id];
	if (to_ir && (sif->cam_type == CAMERA_TYPE_RGBIR)) {
		ISP_PR_INFO("change streamid from %u to %u in toir case",
			    stream_id,
			    isp_get_stream_id_from_cid(isp, CAMERA_ID_MEM));
		stream_id = isp_get_stream_id_from_cid(isp, CAMERA_ID_MEM);
	}


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
		ISP_PR_ERR("fail null para for get,isp[%p] cid[%d]",
			   context, cam_id);
		return IMF_RET_INVALID_PARAMETER;
	}
	//process cmd which is to receive value
	switch (cmd) {
	case CMD_ID_GET_FW_VERSION:
		*((unsigned int *) para) = isp->isp_fw_ver;
		ret = RET_SUCCESS;
		goto quit;
	case CMD_ID_AWB_GET_STATUS:
//	case CMD_ID_AWB_GET_LSC_MATRIX:	//CmdAwbGetLscMatrix_t
//	case CMD_ID_AWB_GET_LSC_SECTOR:	//CmdAwbGetLscSector_t
	case CMD_ID_AE_GET_STATUS:	//CmdAeGetStatus_t
	case CMD_ID_AF_GET_STATUS:	//CmdAfGetStatus_t
	case CMD_ID_DEGAMMA_GET_STATUS:	//CmdDegammaGetStatus_t
//	case CMD_ID_DPF_GET_STATUS:	//CmdDpfGetStatus_t
//	case CMD_ID_DPCC_GET_STATUS:	//CmdDpccGetStatus_t
	case CMD_ID_CAC_GET_STATUS:	//CmdCacGetStatus_t
//	case CMD_ID_WDR_GET_STATUS:	//CmdWdrGetStatus_t
	case CMD_ID_CNR_GET_STATUS:	//CmdCnrGetStatus_t
//	case CMD_ID_FILTER_GET_STATUS:	//CmdFilterGetStatus_t
	case CMD_ID_GAMMA_GET_STATUS:   //GammaStatus_t
	//case CMD_ID_STNR_GET_STATUS: //StnrStatus_t

	case CMD_ID_AWB_GET_WB_GAIN:	//struct _WbGain_t
	case CMD_ID_AWB_GET_CC_MATRIX:	//struct _CcMatrix_t
//	case CMD_ID_AWB_GET_CC_OFFSET:	//struct _CcOffset_t
//	case CMD_ID_AE_GET_ISO_CAPABILITY:	//AeIsoCapability_t
//	case CMD_ID_AE_GET_EV_CAPABILITY:	//AeEvCapability_t
//	case CMD_ID_AE_GET_SETPOINT:	//CmdAeGetSetpoint_t
//	case CMD_ID_AE_GET_ISO_RANGE:	//AeIsoRange_t
	case CMD_ID_CPROC_GET_STATUS:	//struct _CprocStatus_t
	case CMD_ID_BLS_GET_STATUS:	//BlsStatus_t
//	case CMD_ID_DEMOSAIC_GET_THRESH:	//DemosaicThresh_t
		resp_by_pl = 1;
		break;
	default:
		resp_by_pl = 1;
		break;

	}

	if (resp_by_pl) {
		if (!is_direct_cmd) {
			ISP_PR_ERR
			("fail none direct for get cmd,cid[%d],cmd %s(0x%08x)",
			cam_id, isp_dbg_get_cmd_str(cmd), cmd);
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
		unsigned int *pbuf;

		if (sif->cmd_resp_buf == NULL) {
			sif->cmd_resp_buf =
				isp_gpu_mem_alloc(CMD_RESPONSE_BUF_SIZE,
						ISP_GPU_MEM_TYPE_NLFB);
		}

		resp_buf = sif->cmd_resp_buf;
		if (resp_buf == NULL) {
			ISP_PR_ERR("fail aloc respbuf,cid[%d],cmd %s(0x%08x)",
				   cam_id, isp_dbg_get_cmd_str(cmd), cmd);
			return IMF_RET_INVALID_PARAMETER;
		}

		pbuf = (unsigned int *) resp_buf->sys_addr;
		if (pbuf) {
			pbuf[0] = 0x11223344;
			pbuf[1] = 0x55667788;
			pbuf[2] = 0x99aabbcc;
			pbuf[3] = 0xddeeff00;
			ISP_PR_INFO("%s:0x%x 0x%x 0x%x 0x%x", pbuf[0],
				    pbuf[1], pbuf[2], pbuf[3]);
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
				min(*para_len, CMD_RESPONSE_BUF_SIZE));
			ISP_PR_INFO("%s:%llx,0x%x 0x%x 0x%x 0x%x",
				    resp_buf->gpu_mc_addr, pbuf[0],
				pbuf[1], pbuf[2], pbuf[3]);
		}
	}

quit:
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("fail cid[%d],cmd %s(0x%08x),is_set_cmd %u,", cam_id,
			   isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
		ISP_PR_ERR("is_stream_cmd %u,toIR %u,is_direct_cmd %u,para %p",
			   is_stream_cmd, to_ir, is_direct_cmd, para);
		return IMF_RET_FAIL;
	}

	ISP_PR_INFO("suc cid[%d],cmd %s(0x%08x),is_set_cmd %u,", cam_id,
		    isp_dbg_get_cmd_str(cmd), cmd, is_set_cmd);
	ISP_PR_INFO("is_stream_cmd %u,toIR %u,is_direct_cmd %u,para %p",
		    is_stream_cmd, to_ir, is_direct_cmd, para);
	if (cmd == CMD_ID_AE_GET_STATUS)
		isp_dbg_dump_ae_status(para);
#endif
	return IMF_RET_SUCCESS;
}

int get_isp_work_buf_imp(void *context, void *work_buf_info)
{
	struct isp_context *isp = (struct isp_context *)context;

	if ((context == NULL) || (work_buf_info == NULL)) {
		ISP_PR_ERR("fail para");
		return IMF_RET_INVALID_PARAMETER;
	}
	if (&isp->work_buf == work_buf_info) {
		ISP_PR_ERR("fail isp_context");
		return IMF_RET_INVALID_PARAMETER;
	}
	memcpy(work_buf_info, &isp->work_buf, sizeof(isp->work_buf));

	RET(IMF_RET_SUCCESS);
	return IMF_RET_SUCCESS;
}
