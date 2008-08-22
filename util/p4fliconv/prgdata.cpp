
// p4fliconv: high resolution interlaced FLI converter utility
// Copyright (C) 2007-2008 Istvan Varga <istvanv@users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The Plus/4 program files generated by this utility are not covered by the
// GNU General Public License, and can be used, modified, and distributed
// without any restrictions.

#include "p4fliconv.hpp"
#include "prgdata.hpp"

namespace Plus4FLIConv {

  const unsigned char PRGData::prgHeader_FLI[0x0801] = {
    0x01, 0x10, 0x0B, 0x10, 0x0A, 0x00, 0x9E, 0x34, 0x31, 0x30, 0x39, 0x00,
    0x00, 0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0xA9, 0xCA, 0x8D, 0xFC, 0xFF,
    0xA9, 0x10, 0x8D, 0xFD, 0xFF, 0x20, 0x70, 0x12, 0x20, 0x93, 0x10, 0xAE,
    0xEC, 0x17, 0xCA, 0xEC, 0x1D, 0xFF, 0xF0, 0xFB, 0xE8, 0xEC, 0x1D, 0xFF,
    0xF0, 0xFB, 0x8E, 0x0B, 0xFF, 0xAD, 0xEE, 0x17, 0x8D, 0x0A, 0xFF, 0xAD,
    0xF0, 0x17, 0x8D, 0xFE, 0xFF, 0xAD, 0xF2, 0x17, 0x8D, 0xFF, 0xFF, 0xCE,
    0x09, 0xFF, 0x58, 0xA2, 0x00, 0x20, 0xE1, 0x10, 0xAD, 0xFC, 0x17, 0xD0,
    0x24, 0xAD, 0xFD, 0x17, 0xF0, 0x28, 0x10, 0x1D, 0xA9, 0x7F, 0x8D, 0x30,
    0xFD, 0x8D, 0x08, 0xFF, 0xAD, 0x08, 0xFF, 0x29, 0x10, 0xD0, 0x0F, 0x8A,
    0xF0, 0x0C, 0xAD, 0xFB, 0x1F, 0xF0, 0x0F, 0xA5, 0xD9, 0x4A, 0x8D, 0xFD,
    0x17, 0x8A, 0x48, 0x20, 0x96, 0x10, 0x68, 0xAA, 0x10, 0xCB, 0x78, 0xA9,
    0x0B, 0x8D, 0x06, 0xFF, 0x8D, 0x3E, 0xFF, 0x8D, 0xD0, 0xFD, 0x6C, 0xFA,
    0x17, 0x6C, 0xF4, 0x17, 0x6C, 0xF6, 0x17, 0xAE, 0xFC, 0x17, 0xF0, 0x14,
    0xCA, 0x8E, 0xFC, 0x17, 0x20, 0x3F, 0x11, 0xA5, 0xD9, 0x18, 0xED, 0xFC,
    0x17, 0xAA, 0x20, 0x3F, 0x11, 0x6C, 0xF8, 0x17, 0xAE, 0xFD, 0x17, 0x30,
    0x13, 0xF0, 0x11, 0xCA, 0x8E, 0xFD, 0x17, 0x20, 0xEF, 0x10, 0xA5, 0xD9,
    0x18, 0xED, 0xFD, 0x17, 0xAA, 0x20, 0xEF, 0x10, 0x6C, 0xF8, 0x17, 0x78,
    0x8D, 0x3E, 0xFF, 0x8D, 0xD0, 0xFD, 0x6C, 0xFC, 0xFF, 0x2C, 0x07, 0xFF,
    0x70, 0x03, 0xA9, 0xFA, 0x2C, 0xA9, 0xE1, 0x4C, 0xE4, 0x10, 0xAD, 0xF2,
    0x15, 0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xCD, 0x1D, 0xFF, 0xF0, 0xFB, 0x60,
    0x86, 0xE0, 0x20, 0xF1, 0x11, 0x24, 0xDB, 0x30, 0x2F, 0xA9, 0x8E, 0xAA,
    0xA0, 0x00, 0x20, 0xF0, 0x16, 0x20, 0x03, 0x12, 0x20, 0x43, 0x12, 0xA9,
    0x0A, 0x20, 0x62, 0x12, 0x20, 0x03, 0x12, 0xA5, 0xE0, 0x29, 0x03, 0xAA,
    0xBD, 0xD8, 0x17, 0x2C, 0xFD, 0x1F, 0x70, 0x03, 0xA8, 0xD0, 0x03, 0xBC,
    0xE0, 0x17, 0xAA, 0xA9, 0x14, 0x4C, 0x4C, 0x12, 0xA9, 0x14, 0xA2, 0x08,
    0xA0, 0x08, 0x20, 0x4C, 0x12, 0x20, 0x43, 0x12, 0x20, 0x60, 0x12, 0x20,
    0x03, 0x12, 0x20, 0x60, 0x12, 0x4C, 0x43, 0x12, 0x86, 0xE0, 0x20, 0xF1,
    0x11, 0x24, 0xDB, 0x30, 0x17, 0xA5, 0xE0, 0x29, 0x03, 0xD0, 0x03, 0xA9,
    0x8D, 0x2C, 0xA9, 0xEE, 0xAA, 0xA0, 0x00, 0x20, 0xF0, 0x16, 0x20, 0x03,
    0x12, 0x4C, 0x7E, 0x11, 0xA5, 0xE0, 0xC9, 0x64, 0x29, 0x03, 0x90, 0x02,
    0x09, 0x04, 0xAA, 0xBD, 0xD8, 0x17, 0x2C, 0xFD, 0x1F, 0x70, 0x03, 0xA8,
    0xD0, 0x03, 0xBC, 0xE0, 0x17, 0xAA, 0xA9, 0x14, 0x20, 0x4C, 0x12, 0x06,
    0xE0, 0xA6, 0xE0, 0xBC, 0x00, 0x1D, 0xBD, 0x00, 0x19, 0xAA, 0xA9, 0x07,
    0x20, 0x4C, 0x12, 0x20, 0x60, 0x12, 0x24, 0xDB, 0x30, 0x14, 0x20, 0x60,
    0x12, 0x20, 0x03, 0x12, 0xA6, 0xE0, 0xBC, 0x01, 0x1D, 0xBD, 0x01, 0x19,
    0xAA, 0xA9, 0x07, 0x4C, 0x4C, 0x12, 0x20, 0x03, 0x12, 0x20, 0x60, 0x12,
    0xA6, 0xE0, 0xBD, 0x01, 0x19, 0xDD, 0x00, 0x19, 0xD0, 0x09, 0xBD, 0x01,
    0x1A, 0x85, 0xE3, 0xA9, 0x15, 0xD0, 0x04, 0x85, 0xE3, 0xA9, 0x07, 0x85,
    0xE2, 0xBD, 0x01, 0x1D, 0xDD, 0x00, 0x1D, 0xD0, 0x09, 0xBD, 0x01, 0x1E,
    0x85, 0xE8, 0xA9, 0x15, 0xD0, 0x04, 0x85, 0xE8, 0xA9, 0x07, 0x85, 0xE4,
    0xA0, 0x01, 0xA5, 0xE3, 0xA6, 0xE8, 0x20, 0xF0, 0x16, 0xA0, 0x03, 0xA5,
    0xE2, 0xA6, 0xE4, 0x4C, 0xF0, 0x16, 0xBD, 0x40, 0x20, 0x85, 0xDC, 0x85,
    0xDE, 0xBD, 0x40, 0x60, 0x85, 0xDD, 0x18, 0x65, 0xE5, 0x85, 0xDF, 0x60,
    0xA0, 0x00, 0xB1, 0xDC, 0xC9, 0xA0, 0xD0, 0x01, 0x60, 0xC9, 0xEA, 0xF0,
    0x25, 0xC9, 0x24, 0xF0, 0x24, 0xC9, 0xA2, 0xF0, 0x20, 0xC9, 0xA9, 0xF0,
    0x1C, 0xC9, 0x4C, 0xD0, 0x1B, 0xC8, 0xB1, 0xDC, 0x48, 0xC8, 0xB1, 0xDC,
    0x85, 0xDD, 0x18, 0x65, 0xE5, 0x85, 0xDF, 0x68, 0x85, 0xDC, 0x85, 0xDE,
    0x90, 0xCE, 0xA9, 0x01, 0x2C, 0xA9, 0x02, 0x2C, 0xA9, 0x03, 0x20, 0x62,
    0x12, 0x4C, 0x05, 0x12, 0xAD, 0x00, 0x19, 0x29, 0x40, 0xAA, 0xA8, 0xA9,
    0x07, 0x85, 0xE2, 0x84, 0xE3, 0x8A, 0xA6, 0xE3, 0xA0, 0x01, 0x20, 0xF0,
    0x16, 0xA5, 0xE2, 0xAA, 0xA0, 0x03, 0x20, 0xF0, 0x16, 0xA9, 0x05, 0x18,
    0x65, 0xDC, 0x85, 0xDC, 0x85, 0xDE, 0x90, 0x04, 0xE6, 0xDD, 0xE6, 0xDF,
    0x60, 0x78, 0x8D, 0x3E, 0xFF, 0x8D, 0xD0, 0xFD, 0xAD, 0x07, 0xFF, 0x29,
    0xDF, 0x8D, 0x07, 0xFF, 0x20, 0xD4, 0x10, 0x20, 0x84, 0xFF, 0xA9, 0x0B,
    0x8D, 0x06, 0xFF, 0x8D, 0x3F, 0xFF, 0xAD, 0xFE, 0x17, 0x0D, 0xFF, 0x17,
    0xF0, 0x0B, 0x20, 0x90, 0x10, 0xA9, 0x00, 0x8D, 0xFE, 0x17, 0x8D, 0xFF,
    0x17, 0xAD, 0xFC, 0x1F, 0x8D, 0x19, 0xFF, 0xAE, 0xFE, 0x1F, 0xE0, 0x01,
    0xA9, 0x00, 0x6A, 0x85, 0xDA, 0xAD, 0xFF, 0x1F, 0xE0, 0x01, 0x90, 0x01,
    0x6A, 0x4A, 0x29, 0x7E, 0xC9, 0x40, 0xB0, 0x02, 0xA9, 0x40, 0x85, 0xD9,
    0x2C, 0x07, 0xFF, 0x70, 0x03, 0xA9, 0x7C, 0x2C, 0xA9, 0x74, 0xC5, 0xD9,
    0x90, 0x02, 0xA5, 0xD9, 0x85, 0xD9, 0x0A, 0x85, 0xD8, 0xAD, 0xFB, 0x1F,
    0xF0, 0x3E, 0xAD, 0xFC, 0x1F, 0x29, 0x70, 0x85, 0xE0, 0x4A, 0x4A, 0x4A,
    0x4A, 0x05, 0xE0, 0xA2, 0x00, 0x9D, 0x00, 0x08, 0x9D, 0x00, 0x09, 0x9D,
    0x00, 0x0A, 0x9D, 0x00, 0x0B, 0xE8, 0xD0, 0xF1, 0xAD, 0xFC, 0x1F, 0x29,
    0x0F, 0x85, 0xE0, 0x0A, 0x0A, 0x0A, 0x0A, 0x05, 0xE0, 0x9D, 0x00, 0x0C,
    0x9D, 0x00, 0x0D, 0x9D, 0x00, 0x0E, 0x9D, 0x00, 0x0F, 0xE8, 0xD0, 0xF1,
    0xA5, 0xD9, 0x4A, 0x2C, 0xA9, 0x00, 0x8D, 0xFC, 0x17, 0xA9, 0x80, 0x8D,
    0xFD, 0x17, 0xAD, 0xFD, 0x1F, 0x29, 0xC0, 0x8D, 0xFD, 0x1F, 0xAD, 0x07,
    0xFF, 0x29, 0x40, 0x85, 0xE0, 0xA6, 0xD8, 0xA9, 0x00, 0x1D, 0xFF, 0x18,
    0xCA, 0xD0, 0xFA, 0xEC, 0xFD, 0x1F, 0xF0, 0x08, 0xA6, 0xD8, 0x1D, 0xFF,
    0x1C, 0xCA, 0xD0, 0xFA, 0x29, 0x07, 0xD0, 0x06, 0xA5, 0xE0, 0x09, 0x08,
    0x85, 0xE0, 0xBD, 0x00, 0x19, 0x29, 0x17, 0x05, 0xE0, 0x9D, 0x00, 0x19,
    0xBD, 0x00, 0x1D, 0x29, 0x17, 0x05, 0xE0, 0x9D, 0x00, 0x1D, 0xE8, 0xE0,
    0xF8, 0xD0, 0xE7, 0xA5, 0xD8, 0xC9, 0xC9, 0xA9, 0x00, 0xAA, 0x86, 0xDC,
    0x86, 0xDE, 0x6A, 0x85, 0xDB, 0x0A, 0xAD, 0xFD, 0x1F, 0x2A, 0x2A, 0x2A,
    0xA8, 0xB9, 0xC8, 0x17, 0x85, 0xE6, 0x85, 0xDD, 0xB9, 0xD0, 0x17, 0x85,
    0xE7, 0x85, 0xDF, 0x38, 0xF9, 0xC8, 0x17, 0x85, 0xE5, 0xB9, 0xC0, 0x17,
    0x85, 0xE1, 0xC0, 0x04, 0xB0, 0x78, 0xA9, 0x08, 0x20, 0xB6, 0x16, 0x86,
    0xE0, 0xA0, 0xFF, 0xA9, 0x40, 0x2C, 0xFD, 0x1F, 0x70, 0x03, 0xA2, 0x40,
    0x2C, 0xA2, 0x80, 0x20, 0xF0, 0x16, 0xA6, 0xE0, 0x8A, 0x29, 0x03, 0xD0,
    0x03, 0xA9, 0x09, 0x2C, 0xA9, 0x0A, 0x20, 0xAA, 0x16, 0xA9, 0x0D, 0x20,
    0xB6, 0x16, 0xA0, 0xF7, 0x38, 0x20, 0x23, 0x15, 0xA0, 0xFC, 0x38, 0x20,
    0x37, 0x15, 0xE8, 0xE4, 0xD9, 0xA9, 0x0B, 0x69, 0x00, 0x20, 0xB6, 0x16,
    0xA9, 0x0D, 0x20, 0xB6, 0x16, 0xA0, 0xF7, 0x18, 0x20, 0x23, 0x15, 0xA0,
    0xFC, 0x18, 0x20, 0x37, 0x15, 0xE4, 0xD9, 0x90, 0xC3, 0xB0, 0x6E, 0xA9,
    0x05, 0x2C, 0xA9, 0x01, 0x20, 0xB6, 0x16, 0xA9, 0x07, 0x20, 0xB6, 0x16,
    0xCA, 0xA0, 0xEF, 0x38, 0x20, 0x37, 0x15, 0xE8, 0xA0, 0xFC, 0x18, 0x20,
    0x37, 0x15, 0xE4, 0xD9, 0xB0, 0x4F, 0xA9, 0x00, 0x20, 0xAA, 0x16, 0xA0,
    0xFC, 0x38, 0x20, 0x23, 0x15, 0xE8, 0xC6, 0xE1, 0xF0, 0x0C, 0xE4, 0xD9,
    0xB0, 0xCD, 0xE0, 0x65, 0xD0, 0xCC, 0xA9, 0x04, 0xD0, 0xCA, 0xA9, 0x15,
    0x85, 0xE1, 0xE4, 0xD9, 0xB0, 0x03, 0xA9, 0x02, 0x2C, 0xA9, 0x06, 0x20,
    0xB6, 0x16, 0x86, 0xE0, 0xA5, 0xDF, 0x18, 0x69, 0x02, 0x48, 0xAA, 0x38,
    0xE5, 0xE5, 0x48, 0xA0, 0xFF, 0x20, 0xF0, 0x16, 0xA6, 0xE0, 0xA9, 0x00,
    0x85, 0xDC, 0x85, 0xDE, 0x68, 0x85, 0xDD, 0x68, 0x85, 0xDF, 0x4C, 0x02,
    0x14, 0xA0, 0x00, 0xA9, 0x60, 0xAA, 0x20, 0xF0, 0x16, 0xA9, 0xFF, 0x2C,
    0x07, 0xFF, 0x70, 0x06, 0xA2, 0x36, 0xA0, 0xF9, 0xD0, 0x04, 0xA2, 0x04,
    0xA0, 0xE0, 0x8D, 0x62, 0x15, 0x8E, 0x5D, 0x15, 0x29, 0xA3, 0x48, 0xA5,
    0xD9, 0xC9, 0x75, 0xB0, 0x20, 0xE9, 0x63, 0x48, 0x18, 0x69, 0xCE, 0x8D,
    0xF2, 0x15, 0x68, 0x30, 0x0C, 0x49, 0xFF, 0x38, 0x6D, 0x5D, 0x15, 0xAA,
    0x68, 0xE9, 0x00, 0xD0, 0x0F, 0x49, 0xFF, 0x38, 0xE9, 0x01, 0x18, 0x90,
    0xF2, 0xE9, 0x66, 0x48, 0x69, 0xD1, 0x90, 0xDF, 0x8D, 0xEE, 0x17, 0x8E,
    0xEC, 0x17, 0x24, 0xDA, 0x30, 0x13, 0x8D, 0xEF, 0x17, 0x8E, 0xED, 0x17,
    0xA9, 0x15, 0x8D, 0xF3, 0x17, 0xA9, 0x46, 0x8D, 0xF1, 0x17, 0x4C, 0xE6,
    0x14, 0xA9, 0xA2, 0x8D, 0xEF, 0x17, 0x8C, 0xED, 0x17, 0xA9, 0x16, 0x8D,
    0xF3, 0x17, 0xA9, 0x05, 0x8D, 0xF1, 0x17, 0xA2, 0x00, 0x8E, 0xB0, 0x15,
    0xAD, 0xFD, 0x1F, 0xC9, 0x40, 0x8A, 0x2A, 0x8D, 0xB8, 0x15, 0x2C, 0xFD,
    0x1F, 0x30, 0x06, 0xA9, 0xC8, 0xA2, 0xE8, 0xD0, 0x04, 0xA9, 0xD8, 0xA2,
    0xF0, 0x8D, 0xE9, 0x17, 0x8E, 0xEB, 0x17, 0xA6, 0xD9, 0xCA, 0x8A, 0x48,
    0xAD, 0xFB, 0x1F, 0xD0, 0x06, 0x20, 0x3F, 0x11, 0x4C, 0x1D, 0x15, 0x20,
    0xEF, 0x10, 0x68, 0xAA, 0xCA, 0x10, 0xEB, 0x60, 0x86, 0xE0, 0x8A, 0x2A,
    0xAA, 0xBD, 0xFF, 0x19, 0x48, 0xBD, 0xFF, 0x1D, 0xAA, 0x68, 0x20, 0xF0,
    0x16, 0xA6, 0xE0, 0x60, 0x86, 0xE0, 0x8A, 0x2A, 0xAA, 0xBD, 0xFF, 0x1A,
    0x48, 0xBD, 0xFF, 0x1E, 0x4C, 0x2F, 0x15, 0x8D, 0x03, 0x16, 0xAD, 0x1E,
    0xFF, 0x29, 0x0E, 0x4A, 0x8D, 0x53, 0x15, 0x10, 0x00, 0xA9, 0xA9, 0xA9,
    0xA9, 0xA9, 0xA9, 0xA5, 0xEA, 0xA9, 0x36, 0x8D, 0x1D, 0xFF, 0xA9, 0xFF,
    0x8D, 0x1C, 0xFF, 0x8A, 0x48, 0x98, 0x48, 0xA2, 0x0D, 0xA9, 0x3B, 0xCA,
    0xD0, 0xFB, 0x8D, 0x06, 0xFF, 0xA2, 0x17, 0xCA, 0xD0, 0xFD, 0xEA, 0xAD,
    0x1F, 0xFF, 0xA0, 0x18, 0xA2, 0x00, 0x8C, 0x1B, 0xFF, 0x8E, 0x1A, 0xFF,
    0x29, 0xFE, 0x8D, 0x1F, 0xFF, 0x09, 0x07, 0xEA, 0xEA, 0xA2, 0x7F, 0xA0,
    0xD1, 0x8E, 0x1E, 0xFF, 0xEA, 0xEA, 0xEA, 0xEA, 0x8C, 0x1E, 0xFF, 0xA2,
    0x03, 0xA0, 0x00, 0xCA, 0xD0, 0xFB, 0x8D, 0x1F, 0xFF, 0xA0, 0x51, 0x8C,
    0x1E, 0xFF, 0xA2, 0x05, 0xA9, 0x00, 0xCA, 0xD0, 0xFB, 0xAA, 0x49, 0x01,
    0x29, 0x01, 0x8D, 0xB0, 0x15, 0xB5, 0xE6, 0x8D, 0xCE, 0x15, 0xBD, 0xE8,
    0x17, 0x8D, 0x12, 0xFF, 0xBD, 0xEA, 0x17, 0xA2, 0x0A, 0x20, 0x00, 0xA0,
    0xAE, 0xB0, 0x15, 0xBD, 0xEC, 0x17, 0x8D, 0x0B, 0xFF, 0xBD, 0xEE, 0x17,
    0x8D, 0x0A, 0xFF, 0xBD, 0xF0, 0x17, 0x8D, 0xFE, 0xFF, 0xBD, 0xF2, 0x17,
    0x8D, 0xFF, 0xFF, 0xA9, 0xCE, 0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xA9, 0xCE,
    0x8D, 0x1D, 0xFF, 0xA9, 0x0B, 0x8D, 0x06, 0xFF, 0xCE, 0x09, 0xFF, 0x68,
    0xA8, 0x68, 0xAA, 0xA9, 0x00, 0x40, 0x8D, 0x45, 0x16, 0xAD, 0x1E, 0xFF,
    0x29, 0x0E, 0x4A, 0x8D, 0x12, 0x16, 0x10, 0x00, 0xA9, 0xA9, 0xA9, 0xA9,
    0xA9, 0xA9, 0xA5, 0xEA, 0x8A, 0x48, 0xA2, 0x0D, 0xA9, 0x6D, 0xCA, 0xD0,
    0xFB, 0x18, 0x8D, 0x1E, 0xFF, 0xAD, 0x0B, 0xFF, 0x69, 0x06, 0x8D, 0x0B,
    0xFF, 0x90, 0x03, 0xEE, 0x0A, 0xFF, 0xA9, 0x47, 0x8D, 0xFE, 0xFF, 0xA9,
    0x16, 0x8D, 0xFF, 0xFF, 0xCE, 0x09, 0xFF, 0x68, 0xAA, 0xA9, 0x00, 0x40,
    0x8D, 0x87, 0x16, 0xAD, 0x1E, 0xFF, 0x29, 0x0E, 0x4A, 0x8D, 0x54, 0x16,
    0x10, 0x00, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA5, 0xEA, 0x8A, 0x48,
    0xA9, 0x89, 0x8D, 0xFE, 0xFF, 0xA9, 0x16, 0x8D, 0xFF, 0xFF, 0xA2, 0x07,
    0xA9, 0xAD, 0xCA, 0xD0, 0xFB, 0x18, 0x8D, 0x1E, 0xFF, 0xAD, 0x0B, 0xFF,
    0x69, 0x06, 0x8D, 0x0B, 0xFF, 0x90, 0x03, 0xEE, 0x0A, 0xFF, 0xCE, 0x09,
    0xFF, 0x68, 0xAA, 0xA9, 0x00, 0x40, 0x48, 0xCE, 0x1D, 0xFF, 0xAD, 0xEC,
    0x17, 0x8D, 0x0B, 0xFF, 0xAD, 0xEE, 0x17, 0x8D, 0x0A, 0xFF, 0xAD, 0xF0,
    0x17, 0x8D, 0xFE, 0xFF, 0xAD, 0xF2, 0x17, 0x8D, 0xFF, 0xFF, 0xCE, 0x09,
    0xFF, 0x68, 0x40, 0x48, 0xA5, 0xDC, 0x9D, 0x40, 0x20, 0xA5, 0xDD, 0x9D,
    0x40, 0x60, 0x68, 0x86, 0xE0, 0xAA, 0xBC, 0xB6, 0x17, 0x88, 0x98, 0x48,
    0x18, 0x7D, 0xA8, 0x17, 0xAA, 0xAD, 0xFD, 0x1F, 0xD0, 0x0B, 0xBD, 0x0A,
    0x17, 0x91, 0xDC, 0xCA, 0x88, 0x10, 0xF7, 0x30, 0x0B, 0xBD, 0x0A, 0x17,
    0x91, 0xDC, 0x91, 0xDE, 0xCA, 0x88, 0x10, 0xF5, 0x68, 0x38, 0x65, 0xDC,
    0x85, 0xDC, 0x85, 0xDE, 0x90, 0x04, 0xE6, 0xDD, 0xE6, 0xDF, 0xA6, 0xE0,
    0x60, 0xC0, 0x80, 0x90, 0x04, 0xC6, 0xDD, 0xC6, 0xDF, 0x91, 0xDC, 0xAD,
    0xFD, 0x1F, 0xF0, 0x03, 0x8A, 0x91, 0xDE, 0x98, 0x10, 0x04, 0xE6, 0xDD,
    0xE6, 0xDF, 0x60, 0xA0, 0x00, 0x8C, 0x14, 0xFF, 0xA0, 0x00, 0x8C, 0x07,
    0xFF, 0xA0, 0x00, 0x8C, 0x15, 0xFF, 0xEA, 0x24, 0xEA, 0xEA, 0x4C, 0x00,
    0x00, 0xA9, 0x00, 0x24, 0xEA, 0x9D, 0x08, 0xFF, 0xA2, 0xCA, 0x24, 0xEA,
    0xA2, 0xCA, 0x4C, 0x00, 0x00, 0xA0, 0x00, 0x8C, 0x16, 0xFF, 0xA0, 0x00,
    0x8C, 0x07, 0xFF, 0x8E, 0x1D, 0xFF, 0xA0, 0x00, 0x8C, 0x16, 0xFF, 0x8D,
    0x14, 0xFF, 0xEE, 0x14, 0xFF, 0xA2, 0xCA, 0x8E, 0x1D, 0xFF, 0xA0, 0x00,
    0x8C, 0x07, 0xFF, 0xA0, 0x00, 0x8C, 0x15, 0xFF, 0xA0, 0x00, 0x8C, 0x16,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x12, 0x16, 0x1A, 0x1D, 0x21,
    0x26, 0x16, 0x38, 0x3B, 0x40, 0x3E, 0x43, 0x0F, 0x03, 0x04, 0x04, 0x03,
    0x04, 0x05, 0x12, 0x02, 0x03, 0x03, 0x03, 0x05, 0x0F, 0xFF, 0x15, 0x15,
    0x15, 0x61, 0x61, 0x80, 0xA0, 0x61, 0xA9, 0xA9, 0xA9, 0x70, 0x70, 0x8F,
    0xAF, 0x73, 0xC9, 0xC9, 0xC9, 0x40, 0x48, 0x50, 0x58, 0x18, 0xA8, 0xB0,
    0xB8, 0x80, 0x88, 0x90, 0x98, 0xE0, 0xC8, 0xD0, 0xD8, 0xC8, 0xD8, 0xE8,
    0xF0, 0x36, 0xF9, 0xA3, 0xA2, 0x46, 0x05, 0x15, 0x16, 0xCA, 0x10, 0xEE,
    0x10, 0xEE, 0x10, 0x0D, 0x10, 0x00, 0x00, 0x00, 0x00
  };

  const unsigned char PRGData::prgHeader_320x200[0x00C1] = {
    0x01, 0x10, 0x0C, 0x10, 0x0A, 0x00, 0x9E, 0x20, 0x34, 0x31, 0x31, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0x8D, 0x3E,
    0xFF, 0x8D, 0xD0, 0xFD, 0x20, 0x9D, 0x10, 0x20, 0x84, 0xFF, 0xAD, 0xF9,
    0x7B, 0x8D, 0x19, 0xFF, 0xA9, 0x3B, 0x8D, 0x06, 0xFF, 0xAD, 0x07, 0xFF,
    0x29, 0x40, 0x09, 0x08, 0x8D, 0x07, 0xFF, 0xA9, 0xE0, 0x8D, 0x12, 0xFF,
    0xA9, 0x78, 0x8D, 0x14, 0xFF, 0xAD, 0xFF, 0x7B, 0x4A, 0x4A, 0x4A, 0x4A,
    0x8D, 0x15, 0xFF, 0xAD, 0xFF, 0x7B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x15,
    0xFF, 0x8D, 0x15, 0xFF, 0xAD, 0xFE, 0x7B, 0x4A, 0x4A, 0x4A, 0x4A, 0x8D,
    0x16, 0xFF, 0xAD, 0xFE, 0x7B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x16, 0xFF,
    0x8D, 0x16, 0xFF, 0x20, 0xB2, 0x10, 0x78, 0xA2, 0x00, 0x20, 0x9D, 0x10,
    0x8A, 0x48, 0x20, 0xB5, 0x10, 0x68, 0xAA, 0xA9, 0x7F, 0x8D, 0x30, 0xFD,
    0x8D, 0x08, 0xFF, 0xAD, 0x08, 0xFF, 0x29, 0x10, 0xF0, 0x03, 0xAA, 0xD0,
    0xE4, 0x8A, 0xF0, 0xE1, 0x78, 0x8D, 0x3E, 0xFF, 0x8D, 0xD0, 0xFD, 0x6C,
    0xBE, 0x10, 0x2C, 0x07, 0xFF, 0x70, 0x03, 0xA9, 0xFA, 0x2C, 0xA9, 0xE1,
    0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xCD, 0x1D, 0xFF, 0xF0, 0xFB, 0x60, 0x6C,
    0xBA, 0x10, 0x6C, 0xBC, 0x10, 0x00, 0x00, 0xAC, 0x10, 0xAC, 0x10, 0x10,
    0x10
  };

  const unsigned char PRGData::prgHeader_160x200[0x00C1] = {
    0x01, 0x10, 0x0C, 0x10, 0x0A, 0x00, 0x9E, 0x20, 0x34, 0x31, 0x31, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0x8D, 0x3E,
    0xFF, 0x8D, 0xD0, 0xFD, 0x20, 0x9D, 0x10, 0x20, 0x84, 0xFF, 0xAD, 0xF9,
    0x7B, 0x8D, 0x19, 0xFF, 0xA9, 0x3B, 0x8D, 0x06, 0xFF, 0xAD, 0x07, 0xFF,
    0x29, 0x40, 0x09, 0x18, 0x8D, 0x07, 0xFF, 0xA9, 0xE0, 0x8D, 0x12, 0xFF,
    0xA9, 0x78, 0x8D, 0x14, 0xFF, 0xAD, 0xFF, 0x7B, 0x4A, 0x4A, 0x4A, 0x4A,
    0x8D, 0x15, 0xFF, 0xAD, 0xFF, 0x7B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x15,
    0xFF, 0x8D, 0x15, 0xFF, 0xAD, 0xFE, 0x7B, 0x4A, 0x4A, 0x4A, 0x4A, 0x8D,
    0x16, 0xFF, 0xAD, 0xFE, 0x7B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x16, 0xFF,
    0x8D, 0x16, 0xFF, 0x20, 0xB2, 0x10, 0x78, 0xA2, 0x00, 0x20, 0x9D, 0x10,
    0x8A, 0x48, 0x20, 0xB5, 0x10, 0x68, 0xAA, 0xA9, 0x7F, 0x8D, 0x30, 0xFD,
    0x8D, 0x08, 0xFF, 0xAD, 0x08, 0xFF, 0x29, 0x10, 0xF0, 0x03, 0xAA, 0xD0,
    0xE4, 0x8A, 0xF0, 0xE1, 0x78, 0x8D, 0x3E, 0xFF, 0x8D, 0xD0, 0xFD, 0x6C,
    0xBE, 0x10, 0x2C, 0x07, 0xFF, 0x70, 0x03, 0xA9, 0xFA, 0x2C, 0xA9, 0xE1,
    0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xCD, 0x1D, 0xFF, 0xF0, 0xFB, 0x60, 0x6C,
    0xBA, 0x10, 0x6C, 0xBC, 0x10, 0x00, 0x00, 0xAC, 0x10, 0xAC, 0x10, 0x10,
    0x10
  };

  const unsigned char PRGData::prgHeader_128x64[0x012B] = {
    0x01, 0x10, 0x0C, 0x10, 0x0A, 0x00, 0x9E, 0x20, 0x34, 0x31, 0x31, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0x8E, 0x3E,
    0xFF, 0x8E, 0xD0, 0xFD, 0xAD, 0x07, 0xFF, 0x29, 0xDF, 0x8D, 0x07, 0xFF,
    0x20, 0x0A, 0x11, 0x20, 0x84, 0xFF, 0xAD, 0x00, 0x11, 0x29, 0x77, 0x8D,
    0x15, 0xFF, 0x8D, 0x19, 0xFF, 0xA9, 0x0B, 0x8D, 0x06, 0xFF, 0xAD, 0x07,
    0xFF, 0x29, 0x40, 0x09, 0x98, 0x8D, 0x07, 0xFF, 0xA9, 0xC0, 0x8D, 0x12,
    0xFF, 0xA9, 0x60, 0x8D, 0x13, 0xFF, 0xAD, 0x01, 0x11, 0x8D, 0x16, 0xFF,
    0xAD, 0x02, 0x11, 0x8D, 0x17, 0xFF, 0xA2, 0x00, 0x8A, 0x9D, 0x00, 0x0C,
    0x9D, 0x00, 0x0D, 0x9D, 0x00, 0x0E, 0x9D, 0xE8, 0x0E, 0xE8, 0xD0, 0xF1,
    0xAD, 0x00, 0x11, 0x29, 0x77, 0x9D, 0x00, 0x08, 0x9D, 0x00, 0x09, 0x9D,
    0x00, 0x0A, 0x9D, 0xE8, 0x0A, 0xE8, 0xD0, 0xF1, 0xAD, 0x03, 0x11, 0x29,
    0x77, 0x09, 0x08, 0x9D, 0x44, 0x09, 0x9D, 0x6C, 0x09, 0x9D, 0x94, 0x09,
    0x9D, 0xBC, 0x09, 0x9D, 0xE4, 0x09, 0x9D, 0x0C, 0x0A, 0x9D, 0x34, 0x0A,
    0x9D, 0x5C, 0x0A, 0xE8, 0xE0, 0x20, 0xD0, 0xE3, 0xA2, 0x00, 0x8A, 0x9D,
    0x44, 0x0D, 0x18, 0x69, 0x01, 0x9D, 0x6C, 0x0D, 0x69, 0x01, 0x9D, 0x94,
    0x0D, 0x69, 0x01, 0x9D, 0xBC, 0x0D, 0x69, 0x01, 0x9D, 0xE4, 0x0D, 0x69,
    0x01, 0x9D, 0x0C, 0x0E, 0x69, 0x01, 0x9D, 0x34, 0x0E, 0x69, 0x01, 0x9D,
    0x5C, 0x0E, 0x69, 0x01, 0xE8, 0xE0, 0x20, 0xD0, 0xD2, 0x20, 0x1F, 0x11,
    0x20, 0x0A, 0x11, 0xA9, 0x1B, 0x8D, 0x06, 0xFF, 0xA2, 0x00, 0x8A, 0x48,
    0x20, 0x22, 0x11, 0x68, 0xAA, 0xA9, 0x7F, 0x8D, 0x30, 0xFD, 0x8D, 0x08,
    0xFF, 0xAD, 0x08, 0xFF, 0x29, 0x10, 0xF0, 0x03, 0xAA, 0xD0, 0xE7, 0x8A,
    0xF0, 0xE4, 0x6C, 0x08, 0x11, 0x00, 0x21, 0x51, 0x71, 0x1E, 0x11, 0x1E,
    0x11, 0x10, 0x10, 0x2C, 0x07, 0xFF, 0x70, 0x03, 0xA9, 0xFA, 0x2C, 0xA9,
    0xE1, 0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xCD, 0x1D, 0xFF, 0xF0, 0xFB, 0x60,
    0x6C, 0x04, 0x11, 0xA9, 0xCE, 0x20, 0x14, 0x11, 0x6C, 0x06, 0x11
  };

  PRGData::PRGData()
    : buf((unsigned char *) 0),
      luminanceCodeTable((int *) 0),
      colorCodeTable((int *) 0),
      bitmapTable((bool *) 0),
      nLines(464),
      conversionType(0),
      interlaceEnabled(true)
  {
    try {
      buf = new unsigned char[0xD501];
      luminanceCodeTable = new int[40 * 496];
      colorCodeTable = new int[40 * 496];
      bitmapTable = new bool[320 * 496];
      this->clear();
      setVerticalSize(464);
      lineBlankFXEnabled() = 0x01;
      borderColor() = 0x00;
      interlaceFlags() = 0xC0;
    }
    catch (...) {
      if (buf)
        delete[] buf;
      if (luminanceCodeTable)
        delete[] luminanceCodeTable;
      if (colorCodeTable)
        delete[] colorCodeTable;
      if (bitmapTable)
        delete[] bitmapTable;
      throw;
    }
  }

  PRGData::~PRGData()
  {
    delete[] buf;
    delete[] luminanceCodeTable;
    delete[] colorCodeTable;
    delete[] bitmapTable;
  }

  void PRGData::clear()
  {
    switch (conversionType) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      for (size_t i = 0x0000; i < 0x0801; i++)
        buf[i] = prgHeader_FLI[i];
      for (size_t i = 0x0801; i < 0x0FFC; i++)
        buf[i] = 0x00;
      for (size_t i = 0x1001; i < 0xD501; i++)
        buf[i] = 0x00;
      break;
    case 6:
      for (size_t i = 0x0000; i < 0x00C1; i++)
        buf[i] = prgHeader_320x200[i];
      for (size_t i = 0x00C1; i < 0xD501; i++)
        buf[i] = 0x00;
      break;
    case 7:
      for (size_t i = 0x0000; i < 0x00C1; i++)
        buf[i] = prgHeader_160x200[i];
      for (size_t i = 0x00C1; i < 0xD501; i++)
        buf[i] = 0x00;
      break;
    case 8:
      for (size_t i = 0x0000; i < 0x012B; i++)
        buf[i] = prgHeader_128x64[i];
      for (size_t i = 0x012B; i < 0xD501; i++)
        buf[i] = 0x00;
      break;
    }
    for (size_t i = 0; i < (40 * 496); i++) {
      luminanceCodeTable[i] = 0;
      colorCodeTable[i] = 0;
    }
    for (size_t i = 0; i < (320 * 496); i++)
      bitmapTable[i] = false;
  }

  void PRGData::optimizeAttributes()
  {
    if (conversionType >= 8)
      return;
    bool    mcFlag = bool(conversionType & 1);
    if (conversionType >= 6) {
      // simple non-FLI video modes:
      for (int k = 0; k < 2; k++) {
        for (int yc = 0; yc < 25; yc++) {
          for (int xc = 0; xc < 40; xc++) {
            // make the use of attribute values more consistent for easier
            // editing of the output file
            if (k != 0) {
              // scan backwards on second pass
              xc = 39 - xc;
              yc = 24 - yc;
            }
            int&    l0_ = luminanceCodeTable[((yc * 16) * 40) + xc];
            int&    c0_ = colorCodeTable[((yc * 16) * 40) + xc];
            int&    l1_ = luminanceCodeTable[(((yc * 16) | 2) * 40) + xc];
            int&    c1_ = colorCodeTable[(((yc * 16) | 2) * 40) + xc];
            int     c_0 = convertColorCode(l0_, c0_);
            int     c_1 = convertColorCode(l1_, c1_);
            int     c00Cnt = 0;
            int     c01Cnt = 0;
            int     c10Cnt = 0;
            int     c11Cnt = 0;
            for (int i = 0; i < 4; i++) {
              int     xc_ = (i % 3) - 1;
              int     yc_ = (i / 3) - 1;
              if (k == 0) {
                xc_ = xc + xc_;
                yc_ = yc + yc_;
              }
              else {
                xc_ = xc - xc_;
                yc_ = yc - yc_;
              }
              if (xc_ >= 0 && xc_ < 40 && yc_ >= 0 && yc_ < 25) {
                int     c_0_ = convertColorCode(this->l0(xc_ * 8, yc_ * 16),
                                                this->c0(xc_ * 8, yc_ * 16));
                int     c_1_ = convertColorCode(this->l1(xc_ * 8, yc_ * 16),
                                                this->c1(xc_ * 8, yc_ * 16));
                if (c_0 == c_0_)
                  c00Cnt++;
                if (c_0 == c_1_)
                  c01Cnt++;
                if (c_1 == c_0_)
                  c10Cnt++;
                if (c_1 == c_1_)
                  c11Cnt++;
              }
            }
            if (((c01Cnt - c00Cnt) + (c10Cnt - c11Cnt)) > 0) {
              // swap colors
              int     tmp = l0_;
              l0_ = l1_;
              l1_ = tmp;
              tmp = c0_;
              c0_ = c1_;
              c1_ = tmp;
              for (int yOffs = 0; yOffs < 8; yOffs++) {
                for (int xOffs = 0; xOffs < 8; xOffs += (int(mcFlag) + 1)) {
                  int     xc_ = (xc * 8) + xOffs;
                  int     yc_ = ((yc * 8) + yOffs) * 2;
                  bool    b = getPixel(xc_, yc_);
                  if (!mcFlag) {
                    setPixel(xc_, yc_, !b);
                  }
                  else {
                    setPixel(xc_, yc_, getPixel(xc_ + 1, yc_));
                    setPixel(xc_ + 1, yc_, b);
                  }
                }
              }
            }
            if (k != 0) {
              xc = 39 - xc;
              yc = 24 - yc;
            }
          }
        }
      }
    }
    else {
      // FLI video modes: make the use of attribute values more consistent
      // for improved compression of the output file
      for (int yc = 0; yc < nLines; yc++) {
        if ((yc & 3) == (conversionType < 2 ? 2 : 1)) {
          yc = yc | 3;
          continue;
        }
        for (int xc = 0; xc < 40; xc++) {
          int&    l0_ = luminanceCodeTable[(yc * 40) + xc];
          int&    c0_ = colorCodeTable[(yc * 40) + xc];
          int&    l1_ = luminanceCodeTable[((yc | 2) * 40) + xc];
          int&    c1_ = colorCodeTable[((yc | 2) * 40) + xc];
          int     c_0 = convertColorCode(l0_, c0_);
          int     c_1 = convertColorCode(l1_, c1_);
          if ((mcFlag ? c_1 : c_0) > (mcFlag ? c_0 : c_1)) {
            // swap colors
            int     tmp = l0_;
            l0_ = l1_;
            l1_ = tmp;
            tmp = c0_;
            c0_ = c1_;
            c1_ = tmp;
            for (int yOffs = 0; yOffs < 4; yOffs++) {
              if ((yOffs & 1) != 0 &&
                  !(conversionType >= 2 && conversionType < 4)) {
                continue;
              }
              for (int xOffs = 0; xOffs < 8; xOffs += (int(mcFlag) + 1)) {
                int     xc_ = (xc * 8) + xOffs;
                int     yc_ = yc + yOffs;
                bool    b = getPixel(xc_, yc_);
                if (!mcFlag) {
                  setPixel(xc_, yc_, !b);
                }
                else {
                  setPixel(xc_, yc_, getPixel(xc_ + 1, yc_));
                  setPixel(xc_ + 1, yc_, b);
                }
              }
            }
          }
        }
      }
      if (mcFlag) {
        for (int yc = 0; yc < nLines; yc++) {
          if ((yc & 3) == (conversionType < 4 ? 2 : 1)) {
            yc = yc | 3;
            continue;
          }
          unsigned char&  c0_0 = lineColor0(yc);
          unsigned char&  c0_1 = lineColor0(yc | 2);
          unsigned char&  c3_0 = lineColor3(yc);
          unsigned char&  c3_1 = lineColor3(yc | 2);
          c0_0 = c0_0 & 0x7F;
          c0_1 = c0_1 & 0x7F;
          c3_0 = c3_0 & 0x7F;
          c3_1 = c3_1 & 0x7F;
          bool    color0ChangeEnabled =
              (nLines <= 400 || lineXShift(yc | 2) == lineXShift(yc));
          if (!color0ChangeEnabled) {
            c0_1 = c0_0;
            if (c3_1 != c3_0)
              continue;
          }
          bool    swapFlag0 = false;
          bool    swapFlag1 = false;
          if (c3_0 < c0_0) {
            unsigned char tmp = c0_0;
            c0_0 = c3_0;
            c3_0 = tmp;
            swapFlag0 = !swapFlag0;
          }
          if (yc >= 4) {
            if (c0_0 == lineColor3(yc - 2) || c3_0 == lineColor0(yc - 2)) {
              unsigned char tmp = c0_0;
              c0_0 = c3_0;
              c3_0 = tmp;
              swapFlag0 = !swapFlag0;
            }
          }
          if (color0ChangeEnabled) {
            if (c3_1 < c0_1) {
              unsigned char tmp = c0_1;
              c0_1 = c3_1;
              c3_1 = tmp;
              swapFlag1 = !swapFlag1;
            }
            if (c0_1 == c3_0 || c3_1 == c0_0) {
              unsigned char tmp = c0_1;
              c0_1 = c3_1;
              c3_1 = tmp;
              swapFlag1 = !swapFlag1;
            }
          }
          else {
            c0_1 = c0_0;
            c3_1 = c3_0;
            swapFlag1 = swapFlag0;
          }
          for (int yOffs = 0; yOffs < 4; yOffs += 2) {
            if (!((yOffs == 0 && swapFlag0) || (yOffs != 0 && swapFlag1)))
              continue;
            for (int xc_ = 0; xc_ < 320; xc_ += 2) {
              int     yc_ = yc + yOffs;
              bool    b = getPixel(xc_, yc_);
              if (getPixel(xc_ + 1, yc_) == b) {
                setPixel(xc_, yc_, !b);
                setPixel(xc_ + 1, yc_, !b);
              }
            }
          }
        }
      }
    }
  }

  void PRGData::convertImageData()
  {
    if (conversionType >= 6) {
      switch (conversionType) {
      case 6:                           // hires Botticelli
      case 7:                           // Multi Botticelli
        {
          for (size_t i = 0x00C1; i < 0x6BFA; i++)
            buf[i] = 0x00;
          for (size_t i = 0x6C01; i < 0xD501; i++)
            buf[i] = 0x00;
          if (conversionType == 6) {
            for (size_t i = 0x0000; i < 0x00C1; i++)
              buf[i] = prgHeader_320x200[i];
            for (size_t i = 0x6BFB; i < 0x6C01; i++)
              buf[i] = 0x00;
          }
          else {
            for (size_t i = 0x0000; i < 0x00C1; i++)
              buf[i] = prgHeader_160x200[i];
            buf[0x6BFB] = 0x4D;         // 'M'
            buf[0x6BFC] = 0x55;         // 'U'
            buf[0x6BFD] = 0x4C;         // 'L'
            buf[0x6BFE] = 0x54;         // 'T'
          }
          for (int yc = 0; yc < 200; yc += 8) {
            for (int xc = 0; xc < 320; xc += 8) {
              int     l0_ = this->l0(xc, yc << 1);
              int     l1_ = this->l1(xc, yc << 1);
              int     c0_ = this->c0(xc, yc << 1);
              int     c1_ = this->c1(xc, yc << 1);
              if (l0_ == 0)
                c0_ = 0;
              else
                l0_--;
              if (l1_ == 0)
                c1_ = 0;
              else
                l1_--;
              unsigned char lCode =
                  (unsigned char) (((l0_ & 7) << 4) | (l1_ & 7));
              unsigned char cCode =
                  (unsigned char) ((c0_ & 15) | ((c1_ & 15) << 4));
              int     addr = ((yc >> 3) * 40) + (xc >> 3) + (0x7800 - 0x0FFF);
              buf[addr] = lCode;
              buf[addr + 0x0400] = cCode;
            }
          }
          for (int yc = 0; yc < 200; yc++) {
            for (int xc = 0; xc < 320; xc++) {
              bool    b = this->getPixel(xc, yc << 1);
              int     addr = ((((yc >> 3) * 40) + (xc >> 3)) << 3) + (yc & 7);
              addr = addr + (0x8000 - 0x0FFF);
              buf[addr] =
                  buf[addr] | (unsigned char) (int(b) << (7 - (xc & 7)));
            }
          }
        }
        break;
      default:
        // TODO: implement conversion for all video modes
        break;
      }
      return;
    }
    // FLI modes
    for (size_t i = 0x0000; i < 0x0801; i++)
      buf[i] = prgHeader_FLI[i];
    for (size_t i = 0x0801; i < 0x0901; i++)
      buf[i] = 0x00;
    for (size_t i = 0x0C01; i < 0x0D01; i++)
      buf[i] = 0x00;
    for (int i = (nLines >> 1); i < 0x0100; i++) {
      buf[0x0901 + i] = 0x00;
      buf[0x0A01 + i] = 0x00;
      buf[0x0B01 + i] = 0x00;
      buf[0x0D01 + i] = 0x00;
      buf[0x0E01 + i] = 0x00;
      if (i < 0x00FB)
        buf[0x0F01 + i] = 0x00;
    }
    for (size_t i = 0x1001; i < 0xD501; i++)
      buf[i] = 0x00;
    for (int yc = 0; yc < nLines; yc++) {
      if ((yc & 2) != 0 || ((yc & 1) != 0 && (interlaceFlags() & 0x40) == 0))
        continue;
      for (int xc = 0; xc < 320; xc += 8) {
        int     l0_ = this->l0(xc, yc);
        int     l1_ = this->l1(xc, yc);
        int     c0_ = this->c0(xc, yc);
        int     c1_ = this->c1(xc, yc);
        if (l0_ == 0)
          c0_ = 0;
        else
          l0_--;
        if (l1_ == 0)
          c1_ = 0;
        else
          l1_--;
        unsigned char lCode = (unsigned char) (((l0_ & 7) << 4) | (l1_ & 7));
        unsigned char cCode = (unsigned char) ((c0_ & 15) | ((c1_ & 15) << 4));
        int     addr = ((yc >> 4) * 40) + (xc >> 3) + 24;
        addr = ((addr & 0x0400) << 3) | ((yc & 12) << 9) | (addr & 0x03FF);
        if (!(yc & 1)) {
          if (addr < 0x2000)
            addr = addr + 0x4000;
          else if (addr < 0x2800)
            addr = (addr - 0x2000) + 0x1800;
          else
            addr = (addr - 0x2000) + 0xA000;
        }
        else {
          if (addr < 0x2000)
            addr = addr + 0x8000;
          else if (addr < 0x2800)
            addr = (addr - 0x2000) + 0xE000;
          else
            addr = (addr - 0x2000) + 0xC000;
        }
        addr = addr - 0x0FFF;
        buf[addr] = lCode;
        buf[addr + 0x0400] = cCode;
      }
    }
    for (int yc = 0; yc < nLines; yc++) {
      if ((yc & 1) != 0 && (interlaceFlags() & 0x80) == 0)
        continue;
      for (int xc = 0; xc < 320; xc++) {
        bool    b = this->getPixel(xc, yc);
        int     addr = ((((yc >> 4) * 40) + (xc >> 3) + 24) << 3)
                       + ((yc & 14) >> 1);
        if (!(yc & 1)) {
          if (addr < 0x2000)
            addr = addr + 0x2000;
          else
            addr = (addr - 0x2000) + 0xA000;
        }
        else {
          if (addr < 0x2000)
            addr = addr + 0x6000;
          else
            addr = (addr - 0x2000) + 0xC000;
        }
        addr = addr - 0x0FFF;
        buf[addr] = buf[addr] | (unsigned char) (int(b) << (7 - (xc & 7)));
      }
    }
  }

  void PRGData::setVerticalSize(int n)
  {
    if (conversionType >= 6)
      return;
    if (n < 256) {
      n = n << 1;
      interlaceEnabled = false;
    }
    else {
      interlaceEnabled = true;
    }
    n = (n > 256 ? (n < 496 ? n : 496) : 256);
    n = (n + 7) & (~(int(7)));
    nLines = n;
    if (interlaceEnabled) {
      buf[0x0FFF] = 0x01;
      buf[0x1000] = (unsigned char) (n & 0xFF);
    }
    else {
      buf[0x0FFF] = 0x00;
      buf[0x1000] = (unsigned char) (n >> 1);
    }
  }

  int PRGData::getVerticalSize() const
  {
    switch (conversionType) {
    case 6:
    case 7:
      return 200;
    case 8:
      return 64;
    }
    return ((int(buf[0x0FFF] & 0x01) << 8) | int(buf[0x1000] & 0xFF));
  }

  void PRGData::setConversionType(int n)
  {
    n = (n > 0 ? (n < 8 ? n : 8) : 0);
    if (n == conversionType)
      return;
    for (int i = 0; i < 0xD501; i++)
      buf[i] = 0x00;
    conversionType = n;
    if (n < 6) {
      setVerticalSize(464);
      lineBlankFXEnabled() = 0x01;
      borderColor() = 0x00;
      interlaceFlags() = (unsigned char) (n < 4 ? (n < 2 ? 0xC0 : 0x80) : 0x00);
    }
    else {
      nLines = (n < 8 ? 200 : 64);
    }
    this->clear();
  }

  int PRGData::getConversionType() const
  {
    return conversionType;
  }

  unsigned int PRGData::getViewerCodeEndAddress() const
  {
    switch (conversionType) {
    case 6:
    case 7:
      return 0x10C0U;
    case 8:
      return 0x112AU;
    }
    return 0x17FEU;
  }

  unsigned int PRGData::getImageDataStartAddress() const
  {
    switch (conversionType) {
    case 6:
    case 7:
      return 0x7800U;
    case 8:
      return 0x6000U;
    }
    return 0x17FEU;
  }

  unsigned int PRGData::getImageDataEndAddress() const
  {
    switch (conversionType) {
    case 0:
    case 1:
      return (nLines <= 400 ? 0xA000U : 0xE500U);
    case 2:
    case 3:
      return (nLines <= 400 ? 0x8000U : 0xC800U);
    case 4:
    case 5:
      return (nLines <= 400 ? 0x6000U : 0xC000U);
    case 6:
    case 7:
      return 0x9F40U;
    case 8:
      return 0x6800U;
    }
    return 0x17FEU;
  }

}       // namespace Plus4FLIConv

