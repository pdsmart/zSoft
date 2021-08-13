/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            bitmaps.h
// Created:         May 2021
// Version:         v1.0
// Author(s):       Williams, Philip Smart
// Description:     The Bitmap Library.
//                  This is a bitmap definition and manipulation library.
//                  for use with the Sharp MZ Series OSD./
// Credits:         
// Copyright:       (c) 2019-2021 Philip Smart <philip.smart@net2net.org>
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
#ifndef BITMAPS_H
#define BITMAPS_H

#ifdef __cplusplus
    extern "C" {
#endif

// Supported bitmaps.
enum BITMAPS {
    BITMAP_ARGO                      = 0x00,                             // Large Argo logo.
    BITMAP_ARGO_MEDIUM               = 0x01,                             // Medium Argo logo.
    BITMAP_ARGO_SMALL                = 0x02                              // Small Argo logo.
};


typedef struct {
	uint8_t                          *bitmap;                            // The bitmap data to represent the image
	uint16_t                         width;                              // The width of the font in pixels
	uint16_t                         height;                             // The height of the font in pixels
} bitmapStruct;

extern const bitmapStruct argo256x128;
extern const bitmapStruct argo128x64;
extern const bitmapStruct argo64x32;


// Prototypes.
//

#ifdef __cplusplus
}
#endif
#endif // BITMAPS_H
