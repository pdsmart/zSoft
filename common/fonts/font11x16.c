/*
 * font11x16.c
 *
 * Created: 4/8/2015 11:31:21 AM
 *  Author: Baron Williams
 */ 

#if defined(__K64F__)
  #include <stdio.h>
  #include <ctype.h>
  #include <stdint.h>
  #include <string.h>
  #include <unistd.h>
  #include <stdarg.h>
  #include <core_pins.h>
  #include <usb_serial.h>
  #include <Arduino.h>
  #include "k64f_soc.h"
  #include <../../libraries/include/stdmisc.h>
#elif defined(__ZPU__)
  #include <stdint.h>
  #include <stdio.h>
  #include "zpu_soc.h"
  #include <stdlib.h>
  #include <ctype.h>
  #include <stdmisc.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif

#include "fonts.h"

#define FONT_CHAR_FIRST 32
#define FONT_CHAR_LAST	126
#define FONT_CHARS 95
#define FONT_WIDTH 11
#define FONT_HEIGHT 16
#define FONT_SPACING 1

const uint16_t fontData11x16[FONT_CHARS][FONT_WIDTH] ={
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                             },     // ' '  32
	{ 0, 0, 0, 124, 13311, 13311, 124, 0, 0, 0, 0,                                 },     // '!'  33
	{ 0, 0, 60, 60, 0, 0, 60, 60, 0, 0, 0,                                         },     // '"'  34
	{ 512, 7696, 8080, 1008, 638, 7710, 8080, 1008, 638, 30, 16,                   },     // '#'  35
	{ 0, 1144, 3324, 3276, 16383, 16383, 3276, 4044, 1928, 0, 0,                   },     // '$'  36
	{ 12288, 14392, 7224, 3640, 1792, 896, 448, 14560, 14448, 14392, 28,           },     // '%'  37
	{ 0, 7936, 16312, 12796, 8646, 14306, 7742, 7196, 13824, 8704, 0,              },     // '&'  38
	{ 0, 0, 0, 39, 63, 31, 0, 0, 0, 0, 0,                                          },     // '''  39
	{ 0, 0, 1008, 4092, 8190, 14343, 8193, 8193, 0, 0, 0,                          },     // '('  40
	{ 0, 0, 8193, 8193, 14343, 8190, 4092, 1008, 0, 0, 0,                          },     // ')'  41
	{ 0, 3224, 3768, 992, 4088, 4088, 992, 3768, 3224, 0, 0,                       },     // '*'  42
	{ 0, 384, 384, 384, 4080, 4080, 384, 384, 384, 0, 0,                           },     // '+'  43
	{ 0, 0, 0, 47104, 63488, 30720, 0, 0, 0, 0, 0,                                 },     // ','  44
	{ 0, 384, 384, 384, 384, 384, 384, 384, 384, 0, 0,                             },     // '-'  45
	{ 0, 0, 0, 14336, 14336, 14336, 0, 0, 0, 0, 0,                                 },     // '.'  46
	{ 6144, 7168, 3584, 1792, 896, 448, 224, 112, 56, 28, 14,                      },     // '/'  47
	{ 2040, 8190, 7686, 13059, 12675, 12483, 12387, 12339, 6174, 8190, 2040,       },     // '0'  48
	{ 0, 0, 12300, 12300, 12302, 16383, 16383, 12288, 12288, 12288, 0,             },     // '1'  49
	{ 12316, 14366, 15367, 15875, 14083, 13187, 12739, 12515, 12407, 12350, 12316, },     // '2'  50
	{ 3084, 7182, 14343, 12483, 12483, 12483, 12483, 12483, 14823, 8062, 3644,     },     // '3'  51
	{ 960, 992, 880, 824, 796, 782, 775, 16383, 16383, 768, 768,                   },     // '4'  52
	{ 3135, 7295, 14435, 12387, 12387, 12387, 12387, 12387, 14563, 8131, 3971,     },     // '5'  53
	{ 4032, 8176, 14840, 12508, 12494, 12487, 12483, 12483, 14787, 8064, 3840,     },     // '6'  54
	{ 3, 3, 3, 12291, 15363, 3843, 963, 243, 63, 15, 3,                            },     // '7'  55
	{ 3840, 8124, 14846, 12519, 12483, 12483, 12483, 12519, 14846, 8124, 3840,     },     // '8'  56
	{ 60, 126, 12519, 12483, 12483, 14531, 7363, 3779, 2023, 1022, 252,            },     // '9'  57
	{ 0, 0, 0, 7280, 7280, 7280, 0, 0, 0, 0, 0,                                    },     // ':'  58
	{ 0, 0, 0, 40048, 64624, 31856, 0, 0, 0, 0, 0,                                 },     // ';'  59
	{ 0, 192, 480, 1008, 1848, 3612, 7182, 14343, 12291, 0, 0,                     },     // '<'  60
	{ 0, 1632, 1632, 1632, 1632, 1632, 1632, 1632, 1632, 1632, 0,                  },     // '='  61
	{ 0, 12291, 14343, 7182, 3612, 1848, 1008, 480, 192, 0, 0,                     },     // '>'  62
	{ 28, 30, 7, 3, 14211, 14275, 227, 119, 62, 28, 0,                             },     // '?'  63
	{ 4088, 8190, 6151, 13299, 14331, 13851, 14331, 14331, 13831, 1022, 504,       },     // '@'  64
	{ 14336, 16128, 2016, 1788, 1567, 1567, 1788, 2016, 16128, 14336, 0,           },     // 'A'  65
	{ 16383, 16383, 12483, 12483, 12483, 12483, 12519, 14846, 8124, 3840, 0,       },     // 'B'  66
	{ 1008, 4092, 7182, 14343, 12291, 12291, 12291, 14343, 7182, 3084, 0,          },     // 'C'  67
	{ 16383, 16383, 12291, 12291, 12291, 12291, 14343, 7182, 4092, 1008, 0,        },     // 'D'  68
	{ 16383, 16383, 12483, 12483, 12483, 12483, 12483, 12483, 12291, 12291, 0,     },     // 'E'  69
	{ 16383, 16383, 195, 195, 195, 195, 195, 195, 3, 3, 0,                         },     // 'F'  70
	{ 1008, 4092, 7182, 14343, 12291, 12483, 12483, 12483, 16327, 16326, 0,        },     // 'G'  71
	{ 16383, 16383, 192, 192, 192, 192, 192, 192, 16383, 16383, 0,                 },     // 'H'  72
	{ 0, 0, 12291, 12291, 16383, 16383, 12291, 12291, 0, 0, 0,                     },     // 'I'  73
	{ 3584, 7680, 14336, 12288, 12288, 12288, 12288, 14336, 8191, 2047, 0,         },     // 'J'  74
	{ 16383, 16383, 192, 480, 1008, 1848, 3612, 7182, 14343, 12291, 0,             },     // 'K'  75
	{ 16383, 16383, 12288, 12288, 12288, 12288, 12288, 12288, 12288, 12288, 0,     },     // 'L'  76
	{ 16383, 16383, 30, 120, 480, 480, 120, 30, 16383, 16383, 0,                   },     // 'M'  77
	{ 16383, 16383, 14, 56, 240, 960, 1792, 7168, 16383, 16383, 0,                 },     // 'N'  78
	{ 1008, 4092, 7182, 14343, 12291, 12291, 14343, 7182, 4092, 1008, 0,           },     // 'O'  79
	{ 16383, 16383, 387, 387, 387, 387, 387, 455, 254, 124, 0,                     },     // 'P'  80
	{ 1008, 4092, 7182, 14343, 12291, 13827, 15879, 7182, 16380, 13296, 0,         },     // 'Q'  81
	{ 16383, 16383, 387, 387, 899, 1923, 3971, 7623, 14590, 12412, 0,              },     // 'R'  82
	{ 3132, 7294, 14567, 12483, 12483, 12483, 12483, 14791, 8078, 3852, 0,         },     // 'S'  83
	{ 0, 3, 3, 3, 16383, 16383, 3, 3, 3, 0, 0,                                     },     // 'T'  84
	{ 2047, 8191, 14336, 12288, 12288, 12288, 12288, 14336, 8191, 2047, 0,         },     // 'U'  85
	{ 7, 63, 504, 4032, 15872, 15872, 4032, 504, 63, 7, 0,                         },     // 'V'  86
	{ 16383, 16383, 7168, 1536, 896, 896, 1536, 7168, 16383, 16383, 0,             },     // 'W'  87
	{ 12291, 15375, 3612, 816, 480, 480, 816, 3612, 15375, 12291, 0,               },     // 'X'  88
	{ 3, 15, 60, 240, 16320, 16320, 240, 60, 15, 3, 0,                             },     // 'Y'  89
	{ 12291, 15363, 15875, 13059, 12739, 12515, 12339, 12319, 12303, 12291, 0,     },     // 'Z'  90
	{ 0, 0, 16383, 16383, 12291, 12291, 12291, 12291, 0, 0, 0,                     },     // '['  91
	{ 14, 28, 56, 112, 224, 448, 896, 1792, 3584, 7168, 6144,                      },     // '\'  92
	{ 0, 0, 12291, 12291, 12291, 12291, 16383, 16383, 0, 0, 0,                     },     // ']'  93
	{ 96, 112, 56, 28, 14, 7, 14, 28, 56, 112, 96,                                 },     // '^'  94
	{ 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, },     // '_'  95
	{ 0, 0, 0, 0, 62, 126, 78, 0, 0, 0, 0,                                         },     // '`'  96
	{ 7168, 15936, 13152, 13152, 13152, 13152, 13152, 13152, 16352, 16320, 0,      },     // 'a'  97
	{ 16383, 16383, 12480, 12384, 12384, 12384, 12384, 14560, 8128, 3968, 0,       },     // 'b'  98
	{ 3968, 8128, 14560, 12384, 12384, 12384, 12384, 12384, 6336, 2176, 0,         },     // 'c'  99
	{ 3968, 8128, 14560, 12384, 12384, 12384, 12512, 12480, 16383, 16383, 0,       },     // 'd' 100
	{ 3968, 8128, 15328, 13152, 13152, 13152, 13152, 13152, 5056, 384, 0,          },     // 'e' 101
	{ 192, 192, 16380, 16382, 199, 195, 195, 3, 0, 0, 0,                           },     // 'f' 102
	{ 896, 51136, 52960, 52320, 52320, 52320, 52320, 58976, 32736, 16352, 0,       },     // 'g' 103
	{ 16383, 16383, 192, 96, 96, 96, 224, 16320, 16256, 0, 0,                      },     // 'h' 104
	{ 0, 0, 12288, 12384, 16364, 16364, 12288, 12288, 0, 0, 0,                     },     // 'i' 105
	{ 0, 0, 24576, 57344, 49152, 49248, 65516, 32748, 0, 0, 0,                     },     // 'j' 106
	{ 0, 16383, 16383, 768, 1920, 4032, 7392, 14432, 12288, 0, 0,                  },     // 'k' 107
	{ 0, 0, 12288, 12291, 16383, 16383, 12288, 12288, 0, 0, 0,                     },     // 'l' 108
	{ 16352, 16320, 224, 224, 16320, 16320, 224, 224, 16320, 16256, 0,             },     // 'm' 109
	{ 0, 16352, 16352, 96, 96, 96, 96, 224, 16320, 16256, 0,                       },     // 'n' 110
	{ 3968, 8128, 14560, 12384, 12384, 12384, 12384, 14560, 8128, 3968, 0,         },     // 'o' 111
	{ 65504, 65504, 3168, 6240, 6240, 6240, 6240, 7392, 4032, 1920, 0,             },     // 'p' 112
	{ 1920, 4032, 7392, 6240, 6240, 6240, 6240, 3168, 65504, 65504, 0,             },     // 'q' 113
	{ 0, 16352, 16352, 192, 96, 96, 96, 96, 224, 192, 0,                           },     // 'r' 114
	{ 4544, 13280, 13152, 13152, 13152, 13152, 16224, 7744, 0, 0, 0,               },     // 's' 115
	{ 96, 96, 8190, 16382, 12384, 12384, 12384, 12288, 0, 0, 0,                    },     // 't' 116
	{ 4064, 8160, 14336, 12288, 12288, 12288, 12288, 6144, 16352, 16352, 0,        },     // 'u' 117
	{ 96, 480, 1920, 7680, 14336, 14336, 7680, 1920, 480, 96, 0,                   },     // 'v' 118
	{ 2016, 8160, 14336, 7168, 4064, 4064, 7168, 14336, 8160, 2016, 0,             },     // 'w' 119
	{ 12384, 14560, 7616, 3968, 1792, 3968, 7616, 14560, 12384, 0, 0,              },     // 'x' 120
	{ 0, 96, 33248, 59264, 32256, 7680, 1920, 480, 96, 0, 0,                       },     // 'y' 121
	{ 12384, 14432, 15456, 13920, 13152, 12768, 12512, 12384, 12320, 0, 0,         },     // 'z' 122
	{ 0, 128, 448, 8188, 16254, 28679, 24579, 24579, 24579, 0, 0,                  },     // '{' 123
	{ 0, 0, 0, 0, 16383, 16383, 0, 0, 0, 0, 0,				       },  				     // '|' 124
	{ 0, 24579, 24579, 24579, 28679, 16254, 8188, 448, 128, 0, 0,                  },     // '}' 125
	{ 16, 24, 12, 4, 12, 24, 16, 24, 12, 4, 0                                      },     // '~' 126
};																					 

/*
 *	Font Size: 11x16
 *	Characters: 96 (ASCII 32-126).
 *	Bit format: vertical
 *	Memory: 2112 bytes (96x11x2)
 */
const fontStruct font11x16 = {
	(byte *) fontData11x16,
	FONT_CHARS,
	FONT_CHAR_FIRST,
	FONT_CHAR_LAST,
	FONT_WIDTH,
	FONT_HEIGHT,
	FONT_SPACING,
	true
};
