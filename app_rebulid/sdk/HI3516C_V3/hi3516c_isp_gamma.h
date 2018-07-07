#ifndef __HI3516C_ISP_GAMMA_H__
#define __HI3516C_ISP_GAMMA_H__

const HI_U16 gs_Gamma[14][GAMMA_NODE_NUMBER] = {
	//[0]
	{	0x0,0x28,0x60,0xb0,0xf8,0x140,0x170,0x194,0x1db,0x207,0x24f,0x295,0x2d7,0x316,0x352,0x38c,
		0x3c3,0x3f7,0x429,0x459,0x487,0x4b4,0x4de,0x507,0x52e,0x553,0x578,0x59b,0x5be,0x5df,0x600,0x620,
		0x63f,0x65e,0x67c,0x69a,0x6b7,0x6d3,0x6ef,0x70b,0x726,0x741,0x75b,0x775,0x78e,0x7a7,0x7c0,0x7d9,
		0x7f1,0x80a,0x822,0x83a,0x851,0x869,0x881,0x898,0x8b0,0x8c8,0x8df,0x8f7,0x90f,0x927,0x93f,0x957,
		0x970,0x988,0x9a1,0x9b9,0x9d2,0x9ea,0xa02,0xa1a,0xa32,0xa49,0xa60,0xa76,0xa8c,0xaa2,0xab6,0xacb,
		0xade,0xaf1,0xb03,0xb14,0xb24,0xb33,0xb42,0xb4f,0xb5b,0xb66,0xb6f,0xb78,0xb7f,0xb91,0xba2,0xbb4,
		0xbc5,0xbd7,0xbe8,0xbf9,0xc09,0xc1a,0xc2a,0xc3a,0xc4a,0xc59,0xc69,0xc78,0xc87,0xc95,0xca3,0xcb1,
		0xcbf,0xccc,0xcd9,0xce5,0xcf1,0xcfd,0xd08,0xd14,0xd1e,0xd28,0xd32,0xd3b,0xd44,0xd4d,0xd55,0xd5c,
		0xd63,0xd6a,0xd70,0xd75,0xd7a,0xd7f,0xd88,0xd92,0xd9c,0xda5,0xdaf,0xdb8,0xdc1,0xdca,0xdd4,0xddd,
		0xde6,0xdee,0xdf7,0xe00,0xe08,0xe10,0xe19,0xe21,0xe29,0xe30,0xe38,0xe3f,0xe47,0xe4e,0xe55,0xe5b,
		0xe62,0xe68,0xe6e,0xe74,0xe7a,0xe80,0xe85,0xe8a,0xe8f,0xe93,0xe98,0xe9c,0xea0,0xea3,0xea6,0xea9,
		0xeac,0xeaf,0xeb3,0xeb7,0xebb,0xec0,0xec4,0xec9,0xecd,0xed2,0xed7,0xedc,0xee0,0xee5,0xeea,0xeef,
		0xef4,0xef9,0xefe,0xf03,0xf08,0xf0d,0xf12,0xf18,0xf1d,0xf22,0xf27,0xf2c,0xf32,0xf37,0xf3c,0xf41,
		0xf46,0xf4b,0xf51,0xf56,0xf5b,0xf60,0xf65,0xf6a,0xf6f,0xf74,0xf79,0xf7e,0xf83,0xf88,0xf8c,0xf91,
		0xf96,0xf9a,0xf9f,0xfa4,0xfa8,0xfac,0xfb1,0xfb5,0xfb9,0xfbd,0xfc1,0xfc5,0xfc9,0xfcc,0xfd0,0xfd4,
		0xfd7,0xfda,0xfde,0xfe1,0xfe4,0xfe7,0xfea,0xfec,0xfef,0xff1,0xff3,0xff6,0xff8,0xffa,0xffb,0xffd,
		0xfff,
	},
	//[1]
	{
		96,160,205,237,262,294,330,374,422,474,528,592,672,739,795,843,886,926,963,997,1029,1061,1090,
		1118,1147,1176,1205,1232,1259,1286,1314,1341,1368,1395,1421,1446,1472,1498,1523,1549,1574,1600,
		1626,1651,1677,1702,1728,1754,1779,1805,1830,1854,1878,1902,1926,1950,1974,1994,2013,2032,2051,
		2070,2090,2109,2127,2146,2164,2182,2201,2219,2238,2255,2273,2290,2308,2326,2343,2361,2378,2394,
		2410,2426,2442,2458,2474,2490,2506,2522,2538,2554,2570,2586,2602,2618,2634,2649,2664,2679,2694,
		2710,2723,2737,2750,2764,2778,2791,2805,2818,2832,2846,2858,2871,2884,2897,2910,2922,2935,2948,
		2961,2973,2985,2997,3009,3021,3033,3045,3056,3067,3078,3090,3101,3112,3123,3134,3146,3155,3165,
		3174,3184,3194,3203,3213,3222,3232,3242,3251,3261,3270,3280,3290,3299,3307,3315,3323,3331,3339,
		3347,3355,3363,3371,3379,3387,3395,3403,3411,3419,3427,3435,3443,3450,3456,3462,3469,3475,3482,
		3488,3494,3501,3507,3514,3520,3526,3533,3539,3546,3552,3558,3565,3571,3576,3581,3586,3590,3595,
		3600,3605,3610,3614,3619,3624,3629,3634,3638,3643,3648,3653,3658,3662,3667,3672,3677,3682,3686,
		3691,3696,3701,3706,3710,3715,3720,3725,3730,3734,3739,3744,3749,3754,3758,3763,3768,3773,3778,
		3782,3787,3792,3797,3802,3806,3811,3816,3821,3826,3830,3835,3840,3846,3854,3864,3875,3888,3901,
		3915,3930,3946,3962,3981,4000,
	},
	//[2]
	{				
		0x0, 0x50, 0xA3, 0xF7, 0x14B, 0x19E, 0x1EE, 0x23A, 0x27F, 0x2BE, 0x303, 0x33E, 0x372, 0x39F, 0x3C9, 
		0x3F3, 0x41D, 0x442, 0x464, 0x485, 0x4A4, 0x4C3, 0x4E2, 0x502, 0x524, 0x546, 0x567, 0x588, 0x5AA, 0x5CD, 
		0x5F3, 0x61C, 0x649, 0x662, 0x67D, 0x699, 0x6B7, 0x6D5, 0x6F5, 0x715, 0x735, 0x755, 0x775, 0x794, 0x7B2, 
		0x7CF, 0x7EB, 0x805, 0x81D, 0x833, 0x847, 0x85A, 0x86C, 0x87C, 0x88C, 0x89B, 0x8AA, 0x8B8, 0x8C6, 0x8D4, 
		0x8E3, 0x8F2, 0x902, 0x912, 0x924, 0x937, 0x94A, 0x95E, 0x972, 0x986, 0x99B, 0x9B0, 0x9C5, 0x9DA, 0x9EF, 
		0xA04, 0xA18, 0xA2C, 0xA40, 0xA53, 0xA66, 0xA78, 0xA8A, 0xA9C, 0xAAE, 0xAC0, 0xAD1, 0xAE3, 0xAF3, 0xB04, 
		0xB14, 0xB24, 0xB34, 0xB43, 0xB51, 0xB5F, 0xB6D, 0xB7A, 0xB86, 0xB91, 0xB9B, 0xBA5, 0xBAE, 0xBB7, 0xBC0, 
		0xBC8, 0xBD1, 0xBDA, 0xBE4, 0xBEE, 0xBF8, 0xC04, 0xC10, 0xC1D, 0xC2B, 0xC3A, 0xC49, 0xC59, 0xC69, 0xC7A, 
		0xC8A, 0xC9B, 0xCAB, 0xCBC, 0xCCB, 0xCDB, 0xCEA, 0xCF8, 0xD06, 0xD13, 0xD20, 0xD2D, 0xD3A, 0xD46, 0xD52, 
		0xD5E, 0xD6A, 0xD75, 0xD80, 0xD8A, 0xD94, 0xD9D, 0xDA6, 0xDAE, 0xDB6, 0xDBD, 0xDC2, 0xDC7, 0xDCB, 0xDCF, 
		0xDD2, 0xDD4, 0xDD7, 0xDD9, 0xDDB, 0xDDD, 0xDE0, 0xDE3, 0xDE7, 0xDEB, 0xDF0, 0xDF6, 0xDFC, 0xE03, 0xE0A, 
		0xE12, 0xE1A, 0xE22, 0xE2A, 0xE33, 0xE3B, 0xE43, 0xE4A, 0xE52, 0xE59, 0xE5F, 0xE65, 0xE6A, 0xE6F, 0xE74, 
		0xE78, 0xE7C, 0xE7F, 0xE83, 0xE86, 0xE89, 0xE8C, 0xE90, 0xE93, 0xE96, 0xE99, 0xE9C, 0xEA0, 0xEA4, 0xEA7, 
		0xEAB, 0xEAE, 0xEB1, 0xEB5, 0xEB8, 0xEBB, 0xEBF, 0xEC2, 0xEC6, 0xEC9, 0xECD, 0xED1, 0xED6, 0xEDA, 0xEDF, 
		0xEE4, 0xEE9, 0xEEF, 0xEF4, 0xEFA, 0xF00, 0xF06, 0xF0C, 0xF12, 0xF18, 0xF1D, 0xF23, 0xF28, 0xF2D, 0xF32, 
		0xF36, 0xF3A, 0xF3E, 0xF41, 0xF44, 0xF46, 0xF49, 0xF4C, 0xF4F, 0xF52, 0xF55, 0xF59, 0xF5D, 0xF62, 0xF67, 
		0xF6D, 0xF74, 0xF7B, 0xF83, 0xF8B, 0xF94, 0xF9D, 0xFA7, 0xFB1, 0xFBA, 0xFC4, 0xFCE, 0xFD8, 0xFE2, 0xFEC, 
		0xFF6, 0xFFF
		},
		//[3]
	{
		0x0,0x11,0x2b,0x4d,0x75,0xa1,0xd0,0xff,0x12f,0x15e,0x18d,0x1bc,0x1ea,0x217,0x244,0x271,
		0x29d,0x2c9,0x2f4,0x31f,0x349,0x373,0x39d,0x3c6,0x3ef,0x417,0x43f,0x466,0x48d,0x4b4,0x4da,0x500,
		0x525,0x54a,0x56f,0x593,0x5b7,0x5da,0x5fd,0x620,0x642,0x664,0x685,0x6a7,0x6c7,0x6e7,0x707,0x727,
		0x746,0x765,0x783,0x7a1,0x7bf,0x7dc,0x7f9,0x816,0x832,0x84e,0x869,0x884,0x89f,0x8b9,0x8d3,0x8ed,
		0x907,0x920,0x938,0x951,0x969,0x980,0x998,0x9af,0x9c6,0x9dc,0x9f2,0xa08,0xa1e,0xa33,0xa48,0xa5d,
		0xa71,0xa85,0xa99,0xaad,0xac0,0xad3,0xae6,0xaf9,0xb0b,0xb1d,0xb2f,0xb40,0xb52,0xb63,0xb74,0xb84,
		0xb95,0xba5,0xbb5,0xbc5,0xbd5,0xbe4,0xbf3,0xc02,0xc11,0xc20,0xc2e,0xc3c,0xc4b,0xc58,0xc66,0xc74,
		0xc81,0xc8f,0xc9c,0xca9,0xcb5,0xcc2,0xccf,0xcdb,0xce7,0xcf4,0xd00,0xd0b,0xd17,0xd23,0xd2e,0xd3a,
		0xd45,0xd50,0xd5b,0xd66,0xd71,0xd7b,0xd86,0xd90,0xd9b,0xda5,0xdaf,0xdb9,0xdc3,0xdcc,0xdd6,0xddf,
		0xde9,0xdf2,0xdfb,0xe04,0xe0d,0xe16,0xe1f,0xe27,0xe30,0xe38,0xe40,0xe48,0xe50,0xe58,0xe60,0xe68,
		0xe70,0xe77,0xe7f,0xe86,0xe8d,0xe95,0xe9c,0xea3,0xeaa,0xeb0,0xeb7,0xebe,0xec4,0xecb,0xed1,0xed7,
		0xedd,0xee4,0xeea,0xef0,0xef5,0xefb,0xf01,0xf06,0xf0c,0xf11,0xf17,0xf1c,0xf21,0xf26,0xf2b,0xf30,
		0xf35,0xf3a,0xf3f,0xf44,0xf48,0xf4d,0xf51,0xf56,0xf5a,0xf5e,0xf63,0xf67,0xf6b,0xf6f,0xf73,0xf77,
		0xf7b,0xf7f,0xf82,0xf86,0xf8a,0xf8d,0xf91,0xf94,0xf98,0xf9b,0xf9e,0xfa2,0xfa5,0xfa8,0xfab,0xfae,
		0xfb1,0xfb4,0xfb7,0xfba,0xfbd,0xfc0,0xfc2,0xfc5,0xfc8,0xfca,0xfcd,0xfd0,0xfd2,0xfd5,0xfd7,0xfda,
		0xfdc,0xfde,0xfe1,0xfe3,0xfe5,0xfe7,0xfea,0xfec,0xfee,0xff0,0xff2,0xff4,0xff6,0xff8,0xffb,0xffd,
		0xfff
	},
	//[4]
	{
		0x0, 0x36, 0x6a, 0x9e, 0xd1, 0x103, 0x134, 0x164, 0x193, 0x1c2, 0x1ef, 0x21c, 0x248, 0x274, 0x29e, 0x2c9, 
		0x2f2, 0x31b, 0x343, 0x36a, 0x391, 0x3b7, 0x3dd, 0x402, 0x426, 0x44a, 0x46d, 0x490, 0x4b3, 0x4d4, 0x4f6, 0x517, 
		0x537, 0x557, 0x576, 0x595, 0x5b4, 0x5d2, 0x5f0, 0x60d, 0x62a, 0x647, 0x663, 0x67f, 0x69a, 0x6b5, 0x6d0, 0x6ea, 
		0x704, 0x71e, 0x738, 0x751, 0x769, 0x782, 0x79a, 0x7b2, 0x7c9, 0x7e1, 0x7f8, 0x80e, 0x825, 0x83b, 0x851, 0x866, 
		0x87c, 0x891, 0x8a6, 0x8bb, 0x8cf, 0x8e3, 0x8f7, 0x90b, 0x91f, 0x932, 0x945, 0x958, 0x96b, 0x97d, 0x98f, 0x9a1, 
		0x9b3, 0x9c5, 0x9d7, 0x9e8, 0x9f9, 0xa0a, 0xa1b, 0xa2b, 0xa3c, 0xa4c, 0xa5c, 0xa6c, 0xa7c, 0xa8c, 0xa9b, 0xaab, 
		0xaba, 0xac9, 0xad8, 0xae6, 0xaf5, 0xb03, 0xb12, 0xb20, 0xb2e, 0xb3c, 0xb4a, 0xb57, 0xb65, 0xb72, 0xb80, 0xb8d, 
		0xb9a, 0xba7, 0xbb4, 0xbc0, 0xbcd, 0xbd9, 0xbe6, 0xbf2, 0xbfe, 0xc0a, 0xc16, 0xc22, 0xc2e, 0xc39, 0xc45, 0xc50, 
		0xc5c, 0xc67, 0xc72, 0xc7d, 0xc88, 0xc93, 0xc9e, 0xca8, 0xcb3, 0xcbe, 0xcc8, 0xcd2, 0xcdd, 0xce7, 0xcf1, 0xcfb, 
		0xd05, 0xd0f, 0xd18, 0xd22, 0xd2c, 0xd35, 0xd3f, 0xd48, 0xd52, 0xd5b, 0xd64, 0xd6d, 0xd76, 0xd7f, 0xd88, 0xd91, 
		0xd9a, 0xda2, 0xdab, 0xdb4, 0xdbc, 0xdc5, 0xdcd, 0xdd5, 0xdde, 0xde6, 0xdee, 0xdf6, 0xdfe, 0xe06, 0xe0e, 0xe16, 
		0xe1e, 0xe25, 0xe2d, 0xe35, 0xe3c, 0xe44, 0xe4c, 0xe53, 0xe5a, 0xe62, 0xe69, 0xe70, 0xe77, 0xe7f, 0xe86, 0xe8d, 
		0xe94, 0xe9b, 0xea2, 0xea8, 0xeaf, 0xeb6, 0xebd, 0xec3, 0xeca, 0xed1, 0xed7, 0xede, 0xee4, 0xeeb, 0xef1, 0xef7, 
		0xefe, 0xf04, 0xf0a, 0xf10, 0xf17, 0xf1d, 0xf23, 0xf29, 0xf2f, 0xf35, 0xf3b, 0xf41, 0xf46, 0xf4c, 0xf52, 0xf58, 
		0xf5d, 0xf63, 0xf69, 0xf6e, 0xf74, 0xf7a, 0xf7f, 0xf85, 0xf8a, 0xf8f, 0xf95, 0xf9a, 0xf9f, 0xfa5, 0xfaa, 0xfaf, 
		0xfb4, 0xfba, 0xfbf, 0xfc4, 0xfc9, 0xfce, 0xfd3, 0xfd8, 0xfdd, 0xfe2, 0xfe7, 0xfec, 0xff1, 0xff5, 0xffa, 0xfff, 
		0xfff, 
	},
	//[5]
	{
		0x0,   0xe,    0x20, 0x35,  0x4d, 0x68, 0x85,  0xa5,  0xc7, 0xeb,  0x110,0x138,0x162,0x18d,0x1b9,0x1e6,
		0x215,0x244,0x274,0x2a5,0x2d6,0x308,0x33a,0x36b,0x39d,0x3ce,0x3ff,0x42f,0x45f,0x48d,0x4bb,0x4e8,
		0x515,0x540,0x56b,0x595,0x5bd,0x5e5,0x60c,0x633,0x658,0x67c,0x69f,0x6c1,0x6e2,0x702,0x721,0x73f,
		0x75c,0x778,0x792,0x7ac,0x7c5,0x7de,0x7f5,0x80c,0x822,0x838,0x84d,0x861,0x876,0x88a,0x89e,0x8b1,
		0x8c5,0x8d8,0x8eb,0x8ff,0x912,0x926,0x93a,0x94e,0x962,0x975,0x989,0x99d,0x9b1,0x9c5,0x9d9,0x9ec,
		0xa00,0xa13,0xa27,0xa3a,0xa4d,0xa5f,0xa72,0xa84,0xa97,0xaa8,0xaba,0xacb,0xadc,0xaed,0xafd,0xb0d,
		0xb1d,0xb2c,0xb3b,0xb49,0xb57,0xb64,0xb71,0xb7d,0xb89,0xb95,0xb9f,0xba9,0xbb3,0xbbc,0xbc4,0xbcc,
		0xbd3,0xbd9,0xbdf,0xbeb,0xbf8,0xc04,0xc10,0xc1c,0xc29,0xc35,0xc41,0xc4c,0xc58,0xc64,0xc70,0xc7b,
		0xc87,0xc92,0xc9e,0xca9,0xcb4,0xcbf,0xccb,0xcd6,0xce1,0xceb,0xcf6,0xd01,0xd0c,0xd16,0xd21,0xd2b,
		0xd36,0xd40,0xd4a,0xd54,0xd5e,0xd68,0xd72,0xd7c,0xd86,0xd90,0xd99,0xda3,0xdac,0xdb6,0xdbf,0xdc8,
		0xdd2,0xddb,0xde4,0xded,0xdf6,0xdff,0xe07,0xe10,0xe19,0xe21,0xe2a,0xe32,0xe3b,0xe43,0xe4b,0xe53,
		0xe5b,0xe63,0xe6b,0xe73,0xe7b,0xe83,0xe8a,0xe92,0xe9a,0xea1,0xea8,0xeb0,0xeb7,0xebe,0xec5,0xecc,
		0xed3,0xeda,0xee1,0xee7,0xeee,0xef5,0xefb,0xf02,0xf08,0xf0e,0xf15,0xf1b,0xf21,0xf27,0xf2d,0xf33,
		0xf39,0xf3e,0xf44,0xf4a,0xf4f,0xf55,0xf5a,0xf5f,0xf65,0xf6a,0xf6f,0xf74,0xf79,0xf7e,0xf83,0xf87,
		0xf8c,0xf91,0xf95,0xf9a,0xf9e,0xfa3,0xfa7,0xfab,0xfaf,0xfb3,0xfb7,0xfbb,0xfbf,0xfc3,0xfc7,0xfcb,
		0xfce,0xfd2,0xfd5,0xfd9,0xfdc,0xfdf,0xfe2,0xfe5,0xfe8,0xfeb,0xfee,0xff1,0xff4,0xff7,0xff9,0xffc,
		0xffe,
	},
	//[6] 16c v300 ar0237 -night
	{
		0x0,0x3C,0x7A,0xB9,0xF8,0x138,0x178,0x1B8,0x1F7,0x235,0x273,0x2AE,0x2E8,0x320,0x356,0x388,
		0x3B8,0x3E5,0x40F,0x436,0x45C,0x480,0x4A1,0x4C2,0x4E1,0x4FE,0x51B,0x537,0x553,0x56E,0x589,0x5A5,
		0x5C0,0x5DB,0x5F6,0x60F,0x628,0x640,0x658,0x66F,0x685,0x69B,0x6B1,0x6C6,0x6DB,0x6F0,0x705,0x719,
		0x72E,0x742,0x756,0x76A,0x77D,0x790,0x7A2,0x7B4,0x7C6,0x7D8,0x7E9,0x7FB,0x80C,0x81D,0x82E,0x83F,
		0x850,0x860,0x871,0x882,0x892,0x8A2,0x8B2,0x8C2,0x8D2,0x8E1,0x8F1,0x900,0x910,0x91F,0x92E,0x93D,
		0x94C,0x95A,0x969,0x977,0x986,0x994,0x9A2,0x9B0,0x9BE,0x9CC,0x9DA,0x9E7,0x9F5,0xA02,0xA0F,0xA1C,
		0xA2A,0xA36,0xA43,0xA50,0xA5D,0xA69,0xA75,0xA82,0xA8E,0xA9A,0xAA6,0xAB2,0xABE,0xACA,0xAD5,0xAE1,
		0xAED,0xAF8,0xB03,0xB0E,0xB1A,0xB25,0xB2F,0xB3A,0xB45,0xB50,0xB5A,0xB65,0xB70,0xB7B,0xB86,0xB90,
		0xB9C,0xBA7,0xBB2,0xBBD,0xBC8,0xBD4,0xBDF,0xBEB,0xBF6,0xC01,0xC0D,0xC18,0xC23,0xC2E,0xC39,0xC44,
		0xC4F,0xC59,0xC63,0xC6E,0xC78,0xC82,0xC8C,0xC96,0xCA0,0xCAA,0xCB3,0xCBD,0xCC7,0xCD1,0xCDB,0xCE5,
		0xCEF,0xCF9,0xD03,0xD0D,0xD17,0xD21,0xD2C,0xD36,0xD40,0xD4A,0xD54,0xD5E,0xD68,0xD72,0xD7C,0xD85,
		0xD8F,0xD98,0xDA1,0xDAA,0xDB3,0xDBB,0xDC4,0xDCC,0xDD5,0xDDD,0xDE6,0xDEE,0xDF6,0xDFF,0xE07,0xE0F,
		0xE18,0xE20,0xE28,0xE31,0xE39,0xE41,0xE49,0xE51,0xE5A,0xE62,0xE6A,0xE72,0xE7A,0xE82,0xE8A,0xE92,
		0xE9A,0xEA2,0xEAA,0xEB2,0xEB9,0xEC1,0xEC9,0xED1,0xED9,0xEE0,0xEE8,0xEF0,0xEF8,0xEFF,0xF07,0xF0F,
		0xF16,0xF1E,0xF25,0xF2D,0xF34,0xF3C,0xF43,0xF4B,0xF52,0xF5A,0xF61,0xF69,0xF70,0xF77,0xF7F,0xF86,
		0xF8D,0xF95,0xF9C,0xFA3,0xFAA,0xFB2,0xFB9,0xFC0,0xFC7,0xFCE,0xFD6,0xFDD,0xFE4,0xFEB,0xFF2,0xFF9,
		0xFFF,
	
	},
	//[7] 16c v300 ar0237 -day
	{
		0x0,0x78,0xDC,0x136,0x186,0x1D6,0x21C,0x262,0x29E,0x2DA,0x312,0x34A,0x37E,0x3B0,0x3E2,0x41A,
		0x448,0x472,0x49A,0x4C2,0x4E6,0x500,0x522,0x542,0x562,0x580,0x59E,0x5BB,0x5D5,0x5EF,0x607,0x620,
		0x638,0x64F,0x666,0x67D,0x693,0x6A9,0x6BE,0x6D4,0x6E9,0x6FD,0x712,0x726,0x739,0x74D,0x760,0x773,
		0x786,0x799,0x7AB,0x7BD,0x7CF,0x7E1,0x7F2,0x804,0x815,0x826,0x836,0x847,0x858,0x868,0x878,0x888,
		0x898,0x8A8,0x8B7,0x8C7,0x8D6,0x8E5,0x8F4,0x903,0x912,0x921,0x92F,0x93E,0x94C,0x95A,0x968,0x976,
		0x984,0x992,0x9A0,0x9AD,0x9BB,0x9C8,0x9D6,0x9E3,0x9F0,0x9FD,0xA0A,0xA17,0xA24,0xA31,0xA3D,0xA4A,
		0xA56,0xA63,0xA6F,0xA7B,0xA88,0xA94,0xAA0,0xAAC,0xAB8,0xAC4,0xACF,0xADB,0xAE7,0xAF2,0xAFE,0xB09,
		0xB15,0xB20,0xB2B,0xB37,0xB42,0xB4D,0xB58,0xB63,0xB6E,0xB79,0xB84,0xB8F,0xB99,0xBA4,0xBAF,0xBB9,
		0xBC4,0xBCF,0xBD9,0xBE3,0xBEE,0xBF8,0xC02,0xC0D,0xC17,0xC21,0xC2B,0xC35,0xC3F,0xC49,0xC53,0xC5D,
		0xC67,0xC71,0xC7A,0xC84,0xC8E,0xC98,0xCA1,0xCAB,0xCB4,0xCBE,0xCC7,0xCD1,0xCDA,0xCE4,0xCED,0xCF6,
		0xCFF,0xD09,0xD12,0xD1B,0xD24,0xD2D,0xD36,0xD3F,0xD48,0xD51,0xD5A,0xD63,0xD6C,0xD75,0xD7E,0xD87,
		0xD8F,0xD98,0xDA1,0xDAA,0xDB2,0xDBB,0xDC3,0xDCC,0xDD4,0xDDD,0xDE5,0xDEE,0xDF6,0xDFF,0xE07,0xE10,
		0xE18,0xE20,0xE28,0xE31,0xE39,0xE41,0xE49,0xE51,0xE5A,0xE62,0xE6A,0xE72,0xE7A,0xE82,0xE8A,0xE92,
		0xE9A,0xEA2,0xEAA,0xEB2,0xEB9,0xEC1,0xEC9,0xED1,0xED9,0xEE0,0xEE8,0xEF0,0xEF8,0xEFF,0xF07,0xF0F,
		0xF16,0xF1E,0xF25,0xF2D,0xF34,0xF3C,0xF43,0xF4B,0xF52,0xF5A,0xF61,0xF69,0xF70,0xF77,0xF7F,0xF86,
		0xF8D,0xF95,0xF9C,0xFA3,0xFAA,0xFB2,0xFB9,0xFC0,0xFC7,0xFCE,0xFD6,0xFDD,0xFE4,0xFEB,0xFF2,0xFF9,
		0xFFF,
	
	},
	
	//[8] 16c v300 ar0237 WDR-day
	{
			0,27,51,74,96,118,139,160,180,201,221,241,260,280,299,318,337,356,375,394,412,431,449,468,486,504,522,
			540,558,576,594,612,630,647,665,683,700,718,735,753,770,787,804,822,839,856,873,890,907,924,941,
			958,975,992,1009,1026,1042,1059,1076,1092,1109,1126,1142,1159,1175,1192,1209,1225,1241,1258,1274,
			1291,1307,1323,1340,1356,1372,1388,1405,1421,1437,1453,1469,1485,1502,1518,1534,1550,1566,1582,
			1598,1614,1630,1646,1662,1677,1693,1709,1725,1741,1757,1773,1788,1804,1820,1836,1851,1867,1883,
			1898,1914,1930,1945,1961,1977,1992,2008,2023,2039,2055,2070,2086,2101,2117,2132,2148,2163,2179,
			2194,2209,2225,2240,2256,2271,2286,2302,2317,2332,2348,2363,2378,2394,2409,2424,2439,2455,2470,
			2485,2500,2515,2531,2546,2561,2576,2591,2606,2622,2637,2652,2667,2682,2697,2712,2727,2742,2757,
			2772,2787,2802,2817,2832,2847,2862,2877,2892,2907,2922,2937,2952,2967,2982,2997,3012,3027,3042,
			3056,3071,3086,3101,3116,3131,3146,3160,3175,3190,3205,3220,3234,3249,3264,3279,3293,3308,3323,
			3338,3352,3367,3382,3396,3411,3426,3441,3455,3470,3485,3499,3514,3529,3543,3558,3572,3587,3602,
			3616,3631,3645,3660,3675,3689,3704,3718,3733,3747,3762,3776,3791,3805,3820,3834,3849,3863,3878,
			3892,3907,3921,3936,3950,3965,3979,3994,4008,4022,4037,4051,4066,4080,4095

	},
	//[9] gamma20 sc2300 day
	{	
		0x0,0x42,0x85,0xCA,0x10F,0x155,0x19C,0x1E1,0x227,0x26B,0x2AE,0x2EF,0x32E,0x36B,0x3A5,0x3DC,0x410,
		0x440,0x46E,0x498,0x4C1,0x4E7,0x50B,0x52E,0x54F,0x56E,0x58D,0x5AA,0x5C7,0x5E3,0x5FF,0x61A,0x636,
		0x651,0x66B,0x684,0x69B,0x6B2,0x6C7,0x6DC,0x6F1,0x705,0x718,0x72B,0x73E,0x750,0x763,0x775,0x788,
		0x79B,0x7AD,0x7BF,0x7D0,0x7E2,0x7F3,0x803,0x814,0x824,0x834,0x844,0x854,0x864,0x873,0x883,0x892,
		0x8A2,0x8B1,0x8C0,0x8CF,0x8DE,0x8ED,0x8FB,0x90A,0x918,0x927,0x935,0x943,0x951,0x95F,0x96D,0x97A,
		0x988,0x995,0x9A3,0x9B0,0x9BD,0x9CB,0x9D8,0x9E5,0x9F1,0x9FE,0xA0B,0xA18,0xA24,0xA31,0xA3D,0xA4A,
		0xA56,0xA62,0xA6E,0xA7B,0xA87,0xA93,0xA9E,0xAAA,0xAB6,0xAC2,0xACD,0xAD9,0xAE5,0xAF0,0xAFC,0xB07,
		0xB12,0xB1D,0xB29,0xB34,0xB3F,0xB4A,0xB55,0xB60,0xB6B,0xB76,0xB80,0xB8B,0xB96,0xBA1,0xBAB,0xBB6,
		0xBC0,0xBCB,0xBD5,0xBE0,0xBEA,0xBF4,0xBFE,0xC09,0xC13,0xC1D,0xC27,0xC31,0xC3B,0xC45,0xC4F,0xC59,
		0xC63,0xC6D,0xC76,0xC80,0xC8A,0xC93,0xC9D,0xCA7,0xCB0,0xCBA,0xCC3,0xCCD,0xCD6,0xCE0,0xCE9,0xCF2,
		0xCFC,0xD05,0xD0E,0xD17,0xD21,0xD2A,0xD33,0xD3C,0xD45,0xD4E,0xD57,0xD60,0xD69,0xD72,0xD7B,0xD84,
		0xD8C,0xD95,0xD9E,0xDA7,0xDAF,0xDB8,0xDC1,0xDCA,0xDD2,0xDDB,0xDE3,0xDEC,0xDF4,0xDFD,0xE05,0xE0E,
		0xE16,0xE1F,0xE27,0xE2F,0xE38,0xE40,0xE48,0xE50,0xE59,0xE61,0xE69,0xE71,0xE79,0xE82,0xE8A,0xE92,
		0xE9A,0xEA2,0xEAA,0xEB2,0xEBA,0xEC2,0xECA,0xED2,0xED9,0xEE1,0xEE9,0xEF1,0xEF9,0xF01,0xF08,0xF10,
		0xF18,0xF20,0xF27,0xF2F,0xF37,0xF3E,0xF46,0xF4E,0xF55,0xF5D,0xF64,0xF6C,0xF73,0xF7B,0xF82,0xF8A,
		0xF91,0xF99,0xFA0,0xFA7,0xFAF,0xFB6,0xFBE,0xFC5,0xFCC,0xFD4,0xFDB,0xFE2,0xFE9,0xFF1,0xFF8,0xFFF,
	},
	//[10] sc2232 day night
	{
		0x0,0x42,0x85,0xCA,0x10F,0x155,0x19C,0x1E1,0x227,0x26B,0x2AE,0x2EF,0x32E,0x36B,0x3A5,0x3DC,
		0x410,0x440,0x46E,0x498,0x4C1,0x4E7,0x50B,0x52E,0x54F,0x56E,0x58D,0x5AA,0x5C7,0x5E3,0x5FF,0x61A,
		0x636,0x651,0x66B,0x684,0x69B,0x6B2,0x6C7,0x6DC,0x6F1,0x705,0x718,0x72B,0x73E,0x750,0x763,0x775,
		0x788,0x79B,0x7AD,0x7BF,0x7D0,0x7E2,0x7F3,0x803,0x814,0x824,0x834,0x844,0x854,0x864,0x873,0x883,
		0x892,0x8A2,0x8B1,0x8C0,0x8CF,0x8DE,0x8ED,0x8FB,0x90A,0x918,0x927,0x935,0x943,0x951,0x95F,0x96D,
		0x97A,0x988,0x995,0x9A3,0x9B0,0x9BD,0x9CB,0x9D8,0x9E5,0x9F1,0x9FE,0xA0B,0xA18,0xA24,0xA31,0xA3D,
		0xA4A,0xA56,0xA62,0xA6E,0xA7B,0xA87,0xA93,0xA9E,0xAAA,0xAB6,0xAC2,0xACD,0xAD9,0xAE5,0xAF0,0xAFC,
		0xB07,0xB12,0xB1D,0xB29,0xB34,0xB3F,0xB4A,0xB55,0xB60,0xB6B,0xB76,0xB80,0xB8B,0xB96,0xBA1,0xBAB,
		0xBB6,0xBC0,0xBCB,0xBD5,0xBE0,0xBEA,0xBF4,0xBFE,0xC09,0xC13,0xC1D,0xC27,0xC31,0xC3B,0xC45,0xC4F,
		0xC59,0xC63,0xC6D,0xC76,0xC80,0xC8A,0xC93,0xC9D,0xCA7,0xCB0,0xCBA,0xCC3,0xCCD,0xCD6,0xCE0,0xCE9,
		0xCF2,0xCFC,0xD05,0xD0E,0xD17,0xD21,0xD2A,0xD33,0xD3C,0xD45,0xD4E,0xD57,0xD60,0xD69,0xD72,0xD7B,
		0xD84,0xD8C,0xD95,0xD9E,0xDA7,0xDAF,0xDB8,0xDC1,0xDCA,0xDD2,0xDDB,0xDE3,0xDEC,0xDF4,0xDFD,0xE05,
		0xE0E,0xE16,0xE1F,0xE27,0xE2F,0xE38,0xE40,0xE48,0xE50,0xE59,0xE61,0xE69,0xE71,0xE79,0xE82,0xE8A,
		0xE92,0xE9A,0xEA2,0xEAA,0xEB2,0xEBA,0xEC2,0xECA,0xED2,0xED9,0xEE1,0xEE9,0xEF1,0xEF9,0xF01,0xF08,
		0xF10,0xF18,0xF20,0xF27,0xF2F,0xF37,0xF3E,0xF46,0xF4E,0xF55,0xF5D,0xF64,0xF6C,0xF73,0xF7B,0xF82,
		0xF8A,0xF91,0xF99,0xFA0,0xFA7,0xFAF,0xFB6,0xFBE,0xFC5,0xFCC,0xFD4,0xFDB,0xFE2,0xFE9,0xFF1,0xFF8,
		0xFFF,
	},
	{//[11]weisente 2232
		0x0,0x3D,0x7B,0xBA,0xF9,0x139,0x179,0x1B9,0x1F9,0x237,0x275,0x2B2,0x2ED,0x326,0x35E,0x394,0x3C7,0x3F7,
		0x427,0x455,0x481,0x4AC,0x4D6,0x4FE,0x525,0x54B,0x570,0x593,0x5B6,0x5D7,0x5F8,0x617,0x636,0x653,0x66E,
		0x688,0x6A0,0x6B7,0x6CC,0x6E1,0x6F5,0x708,0x71B,0x72D,0x73F,0x751,0x763,0x775,0x788,0x79B,0x7AD,0x7BF,
		0x7D0,0x7E2,0x7F3,0x803,0x814,0x824,0x834,0x844,0x854,0x864,0x873,0x883,0x892,0x8A2,0x8B1,0x8C0,0x8CF,
		0x8DE,0x8ED,0x8FB,0x90A,0x918,0x927,0x935,0x943,0x951,0x95F,0x96D,0x97A,0x988,0x995,0x9A3,0x9B0,0x9BD,
		0x9CB,0x9D8,0x9E5,0x9F1,0x9FE,0xA0B,0xA18,0xA24,0xA31,0xA3D,0xA4A,0xA56,0xA62,0xA6E,0xA7B,0xA87,0xA93,
		0xA9E,0xAAA,0xAB6,0xAC2,0xACD,0xAD9,0xAE5,0xAF0,0xAFC,0xB07,0xB12,0xB1D,0xB29,0xB34,0xB3F,0xB4A,0xB55,
		0xB60,0xB6B,0xB76,0xB80,0xB8B,0xB96,0xBA1,0xBAB,0xBB6,0xBC1,0xBCB,0xBD6,0xBE1,0xBEC,0xBF7,0xC01,0xC0C,
		0xC17,0xC21,0xC2B,0xC35,0xC3E,0xC48,0xC51,0xC59,0xC61,0xC69,0xC70,0xC77,0xC7D,0xC84,0xC8A,0xC90,0xC95,
		0xC9B,0xCA1,0xCA6,0xCAC,0xCB2,0xCB8,0xCBE,0xCC4,0xCCA,0xCD0,0xCD6,0xCDC,0xCE1,0xCE7,0xCED,0xCF3,0xCF9,
		0xCFF,0xD05,0xD0B,0xD12,0xD18,0xD1F,0xD26,0xD2E,0xD35,0xD3E,0xD46,0xD4E,0xD57,0xD5F,0xD68,0xD70,0xD78,
		0xD80,0xD88,0xD8F,0xD96,0xD9C,0xDA2,0xDA7,0xDAC,0xDB1,0xDB6,0xDBA,0xDBE,0xDC2,0xDC5,0xDC9,0xDCD,0xDD1,
		0xDD5,0xDD9,0xDDD,0xDE2,0xDE7,0xDEB,0xDEF,0xDF4,0xDF8,0xDFC,0xE00,0xE05,0xE09,0xE0E,0xE13,0xE19,0xE1F,
		0xE26,0xE2D,0xE35,0xE3D,0xE46,0xE4F,0xE59,0xE63,0xE6D,0xE78,0xE83,0xE8E,0xE9A,0xEA6,0xEB3,0xEC0,0xECD,
		0xEDB,0xEE9,0xEF8,0xF07,0xF17,0xF28,0xF39,0xF4A,0xF5C,0xF6E,0xF80,0xF92,0xFA5,0xFB7,0xFC9,0xFDC,0xFED,
		0xFFF,
	},
	{//[12]2232 night:high ISO/high contrast
		0,53,108,164,221,278,335,392,449,504,559,612,664,713,761,806,848,887,923,957,989,1020,1048,1075,1101,1126,1151,
		1174,1198,1222,1245,1270,1295,1320,1344,1368,1391,1414,1437,1459,1482,1503,1525,1547,1569,1591,1613,1635,1658,
		1680,1703,1726,1749,1773,1796,1819,1842,1865,1888,1910,1932,1954,1975,1996,2017,2036,2056,2074,2093,2111,2129,
		2147,2165,2183,2200,2217,2234,2250,2267,2283,2299,2315,2331,2346,2361,2377,2392,2407,2422,2436,2451,2465,2480,
		2494,2508,2522,2536,2550,2563,2577,2590,2602,2615,2627,2640,2652,2664,2675,2687,2698,2710,2721,2732,2743,2754,
		2765,2776,2787,2798,2809,2819,2830,2841,2852,2863,2874,2885,2896,2907,2918,2929,2941,2952,2963,2974,2985,2996,
		3007,3018,3029,3040,3051,3062,3073,3084,3094,3105,3116,3127,3138,3148,3159,3170,3180,3191,3201,3212,3223,3233,
		3244,3254,3264,3275,3285,3296,3306,3317,3327,3338,3348,3358,3369,3379,3390,3400,3410,3420,3430,3441,3451,3461,
		3471,3480,3490,3500,3510,3519,3529,3538,3548,3557,3566,3575,3584,3593,3601,3610,3618,3627,3635,3643,3651,3659,
		3667,3675,3683,3691,3698,3706,3714,3721,3729,3737,3744,3752,3759,3767,3775,3783,3790,3798,3806,3814,3822,3830,
		3838,3846,3854,3863,3871,3879,3887,3896,3904,3912,3920,3929,3937,3945,3954,3962,3970,3979,3987,3995,4004,4012,
		4020,4029,4037,4045,4053,4062,4070,4078,4087,4095,
	},
	{//[13]2232 night:low ISO/low contrast
		0,67,134,202,269,336,403,468,531,592,651,709,766,822,880,938,999,1030,1062,1095,1129,1162,1196,1231,1265,
		1299,1332,1365,1398,1430,1460,1490,1519,1547,1573,1599,1625,1650,1674,1697,1721,1743,1766,1788,1810,1831,
		1853,1875,1896,1917,1939,1959,1980,2001,2021,2041,2060,2080,2099,2117,2136,2154,2172,2190,2207,2224,2240,
		2256,2272,2287,2302,2317,2332,2346,2360,2374,2388,2402,2417,2431,2445,2459,2474,2488,2502,2516,2530,2544,
		2558,2572,2586,2599,2613,2626,2640,2653,2666,2679,2692,2705,2717,2730,2742,2755,2767,2779,2791,2803,2815,
		2826,2838,2850,2861,2872,2883,2894,2905,2916,2927,2937,2948,2958,2968,2979,2989,2999,3009,3019,3029,3039,
		3049,3058,3068,3077,3087,3096,3106,3115,3124,3133,3143,3152,3161,3171,3180,3189,3199,3209,3218,3228,3237,
		3247,3257,3266,3276,3285,3294,3304,3313,3322,3331,3340,3349,3358,3366,3375,3384,3393,3401,3410,3418,3426,
		3434,3443,3450,3458,3466,3473,3481,3488,3494,3501,3507,3514,3520,3526,3533,3539,3546,3553,3560,3567,3575,
		3583,3591,3600,3608,3617,3626,3636,3645,3654,3663,3672,3682,3691,3700,3708,3717,3725,3734,3742,3750,3759,
		3767,3775,3783,3791,3799,3807,3815,3823,3830,3838,3846,3854,3861,3869,3876,3883,3890,3898,3905,3912,3919,
		3927,3934,3941,3949,3956,3964,3971,3979,3987,3995,4003,4012,4020,4028,4036,4045,4053,4061,4070,4078,4086,
		4095,
	}
};

#endif //__HI3516C_ISP_GAMMA_H__

