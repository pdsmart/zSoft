/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            fonts.h
// Created:         May 2021
// Version:         v1.0
// Author(s):       Baron Williams, Philip Smart
// Description:     The Font Library.
//                  This is a font definition and manipulation library, the fonts being based on
//                  Baron Williams (https://github.com/BaronWilliams) font definitions and modified
//                  for use with the Sharp MZ Series OSD./
// Credits:         
// Copyright:       (c) 2015 Baron Williams, (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
//
// History:         May 2021 - Initial write of the OSD software.
//
// Notes:           See Makefile to enable/disable conditional components
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This source file is free software: you can redistribute it and#or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef FONTS_H
#define FONTS_H

#ifdef __cplusplus
    extern "C" {
#endif

// Supported fonts.
enum FONTS {
    FONT_3X6                         = 0x00,                             // 3x6 small font.
    FONT_5X7                         = 0x01,                             // 5x7 default font.
    FONT_7X8                         = 0x02,                             // 7X8 large default font.
    FONT_9X16                        = 0x03,                             // 9X16 large font for titles.
    FONT_11X16                       = 0x04                              // 11X16 large font for titles.
};


typedef struct fontStruct {
	uint8_t                          *bitmap;                            // The bitmap data to represent the font
	uint16_t                         characters;                         // The number of valid characters in a font
	uint8_t                          start;                              // The first valid character in a font
	uint8_t                          end;                                // The last valid character in a font
	uint8_t                          width;                              // The width of the font in pixels
	uint8_t                          height;                             // The height of the font in pixels
	uint8_t                          spacing;                            // The horizontal spacing required for a font
	bool                             bitAlignVertical;                   // True if the data is stored vertically
} fontStruct;

extern const fontStruct font11x16;
extern const fontStruct font3x6;
extern const fontStruct font3x6limited;
extern const fontStruct font5x7extended;
extern const fontStruct font5x7;
extern const fontStruct font7x8;
extern const fontStruct font7x8extended;
extern const fontStruct font9x16;




// Prototypes.
//

#ifdef __cplusplus
}
#endif
#endif // FONTS_H
