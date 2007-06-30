
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/plus4emu/
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

#include "plus4emu.hpp"
#include "cpu.hpp"
#include "ted.hpp"

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

namespace Plus4 {

  REGPARM void TED7360::render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if (nextCharCnt == 0) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      ted.shiftRegisterCharacter.bitmap_() = b << 4;
      uint8_t c = ted.shiftRegisterCharacter.char_();
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
      bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? c1 : c0);
    }
    else {
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t c = ted.shiftRegisterCharacter.char_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      uint8_t c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      uint8_t c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 3;
        c = ted.shiftRegisterCharacter.char_();
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[1] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? c1 : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        c = ted.shiftRegisterCharacter.char_();
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[2] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? c1 : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 1;
        c = ted.shiftRegisterCharacter.char_();
        c0 = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c1 = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        bufp[3] = ((b & uint8_t(0x80)) ? c1 : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? c1 : c0);
        bufp[1] = ((b & uint8_t(0x40)) ? c1 : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? c1 : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? c1 : c0);
        ted.shiftRegisterCharacter.bitmap_() =
            ted.shiftRegisterCharacter.bitmap_() << 4;
        break;
      }
    }
  }

  REGPARM void TED7360::render_BMM_multicolor(TED7360& ted,
                                              uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c_[4];
    c_[0] = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
             ted.tedRegisters[0x15] : uint8_t(0xFF));
    c_[3] = (!(ted.tedRegisterWriteMask & 0x00400000U) ?
             ted.tedRegisters[0x16] : uint8_t(0xFF));
    if (nextCharCnt == 0) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      ted.shiftRegisterCharacter.bitmap_() = b << 4;
      uint8_t c = ted.shiftRegisterCharacter.char_();
      c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      bufp[0] = c_[(b >> 6) & 3];
      c_[0] = ted.tedRegisters[0x15];
      c_[3] = ted.tedRegisters[0x16];
      bufp[1] = c_[(b >> 6) & 3];
      bufp[3] = bufp[2] = c_[(b >> 4) & 3];
    }
    else {
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t c = ted.shiftRegisterCharacter.char_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
      c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
      switch (nextCharCnt) {
      case 1:
        bufp[0] = c_[(b >> 6) & 3];
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        c = ted.shiftRegisterCharacter.char_();
        c_[0] = ted.tedRegisters[0x15];
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        c_[3] = ted.tedRegisters[0x16];
        bufp[2] = bufp[1] = c_[(b >> 6) & 3];
        bufp[3] = c_[(b >> 4) & 3];
        break;
      case 2:
        bufp[0] = c_[(b >> 6) & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        bufp[1] = c_[(b >> 6) & 3];
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        c = ted.shiftRegisterCharacter.char_();
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        bufp[3] = bufp[2] = c_[(b >> 6) & 3];
        break;
      case 3:
        bufp[0] = c_[(b >> 6) & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        bufp[2] = bufp[1] = c_[(b >> 4) & 3];
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        c = ted.shiftRegisterCharacter.char_();
        b = ted.shiftRegisterCharacter.bitmap_();
        c_[1] = ((a & uint8_t(0x07)) << 4) | ((c & uint8_t(0xF0)) >> 4);
        c_[2] = (a & uint8_t(0x70)) | (c & uint8_t(0x0F));
        bufp[3] = c_[(b >> 6) & 3];
        break;
      default:
        bufp[0] = c_[(b >> 6) & 3];
        c_[0] = ted.tedRegisters[0x15];
        c_[3] = ted.tedRegisters[0x16];
        if (!(nextCharCnt & 1)) {
          bufp[1] = c_[(b >> 6) & 3];
          bufp[3] = bufp[2] = c_[(b >> 4) & 3];
        }
        else {
          bufp[2] = bufp[1] = c_[(b >> 4) & 3];
          bufp[3] = c_[(b >> 2) & 3];
        }
        ted.shiftRegisterCharacter.bitmap_() =
            ted.shiftRegisterCharacter.bitmap_() << 4;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_std(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c0 = (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                  ted.tedRegisters[0x15] : uint8_t(0xFF));
    if (nextCharCnt == 0) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      ted.shiftRegisterCharacter.bitmap_() = b << 4;
      b = b ^ (ted.shiftRegisterCharacter.cursor_() & ted.flashState);
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[0x15];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      b = b ^ (ted.shiftRegisterCharacter.cursor_() & ted.flashState);
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 3;
        b = b ^ (ted.shiftRegisterCharacter.cursor_() & ted.flashState);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        b = b ^ (ted.shiftRegisterCharacter.cursor_() & ted.flashState);
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 1;
        b = b ^ (ted.shiftRegisterCharacter.cursor_() & ted.flashState);
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[0x15];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
        ted.shiftRegisterCharacter.bitmap_() =
            ted.shiftRegisterCharacter.bitmap_() << 4;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_ECM(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if (nextCharCnt == 0) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      ted.shiftRegisterCharacter.bitmap_() = b << 4;
      uint8_t c = (ted.shiftRegisterCharacter.char_() >> 6) + uint8_t(0x15);
      uint8_t c0 = (!(ted.tedRegisterWriteMask & (1U << c)) ?
                    ted.tedRegisters[c] : uint8_t(0xFF));
      bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
      c0 = ted.tedRegisters[c];
      bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
      bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
      bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
    }
    else {
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      uint8_t c = (ted.shiftRegisterCharacter.char_() >> 6) + uint8_t(0x15);
      uint8_t c0 = (!(ted.tedRegisterWriteMask & (1U << c)) ?
                    ted.tedRegisters[c] : uint8_t(0xFF));
      switch (nextCharCnt) {
      case 1:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 3;
        c = ted.shiftRegisterCharacter.char_();
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[1] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x20)) ? a : c0);
        break;
      case 2:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        c = ted.shiftRegisterCharacter.char_();
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[2] = ((b & uint8_t(0x80)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x40)) ? a : c0);
        break;
      case 3:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 1;
        c = ted.shiftRegisterCharacter.char_();
        c0 = ted.tedRegisters[(c >> 6) + uint8_t(0x15)];
        bufp[3] = ((b & uint8_t(0x80)) ? a : c0);
        break;
      default:
        bufp[0] = ((b & uint8_t(0x80)) ? a : c0);
        c0 = ted.tedRegisters[c];
        bufp[1] = ((b & uint8_t(0x40)) ? a : c0);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c0);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c0);
        ted.shiftRegisterCharacter.bitmap_() =
            ted.shiftRegisterCharacter.bitmap_() << 4;
        break;
      }
    }
  }

  REGPARM void TED7360::render_char_MCM(TED7360& ted, uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    uint8_t c_[4];
    c_[0] = ted.tedRegisters[0x15];
    c_[1] = ted.tedRegisters[0x16];
    c_[2] = ted.tedRegisters[0x17];
    if (nextCharCnt == 0) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      ted.shiftRegisterCharacter.bitmap_() = b << 4;
      if (a & uint8_t(0x08)) {
        c_[3] = a & uint8_t(0x77);
        int     tmp = (b >> 6) & 3;
        if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
          bufp[0] = c_[tmp];
        else
          bufp[0] = uint8_t(0xFF);
        bufp[1] = c_[tmp];
        bufp[3] = bufp[2] = c_[(b >> 4) & 3];
      }
      else {
        bufp[0] = ((b & uint8_t(0x80)) ?
                   a : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                        c_[0] : uint8_t(0xFF)));
        bufp[1] = ((b & uint8_t(0x40)) ? a : c_[0]);
        bufp[2] = ((b & uint8_t(0x20)) ? a : c_[0]);
        bufp[3] = ((b & uint8_t(0x10)) ? a : c_[0]);
      }
    }
    else {
      uint8_t a = ted.shiftRegisterCharacter.attr_();
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      c_[3] = a & uint8_t(0x77);
      switch (nextCharCnt) {
      case 1:
        if (a & uint8_t(0x08)) {
          int     tmp = (b >> 6) & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0xFF);
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     a : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                          c_[0] : uint8_t(0xFF)));
        }
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        if (a & uint8_t(0x08)) {
          ted.shiftRegisterCharacter.bitmap_() = b << 2;
          c_[3] = a & uint8_t(0x77);
          bufp[2] = bufp[1] = c_[(b >> 6) & 3];
          bufp[3] = c_[(b >> 4) & 3];
        }
        else {
          ted.shiftRegisterCharacter.bitmap_() = b << 3;
          bufp[1] = ((b & uint8_t(0x80)) ? a : c_[0]);
          bufp[2] = ((b & uint8_t(0x40)) ? a : c_[0]);
          bufp[3] = ((b & uint8_t(0x20)) ? a : c_[0]);
        }
        break;
      case 2:
        if (a & uint8_t(0x08)) {
          int     tmp = (b >> 6) & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0xFF);
          bufp[1] = c_[tmp];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     a : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                          c_[0] : uint8_t(0xFF)));
          bufp[1] = ((b & uint8_t(0x40)) ? a : c_[0]);
        }
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        ted.shiftRegisterCharacter.bitmap_() = b << 2;
        if (a & uint8_t(0x08)) {
          c_[3] = a & uint8_t(0x77);
          bufp[3] = bufp[2] = c_[(b >> 6) & 3];
        }
        else {
          bufp[2] = ((b & uint8_t(0x80)) ? a : c_[0]);
          bufp[3] = ((b & uint8_t(0x40)) ? a : c_[0]);
        }
        break;
      case 3:
        if (a & uint8_t(0x08)) {
          int     tmp = (b >> 6) & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0xFF);
          bufp[2] = bufp[1] = c_[(b >> 4) & 3];
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     a : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                          c_[0] : uint8_t(0xFF)));
          bufp[1] = ((b & uint8_t(0x40)) ? a : c_[0]);
          bufp[2] = ((b & uint8_t(0x20)) ? a : c_[0]);
        }
        ted.shiftRegisterCharacter = ted.currentCharacter;
        a = ted.shiftRegisterCharacter.attr_();
        b = ted.shiftRegisterCharacter.bitmap_();
        if (a & uint8_t(0x08)) {
          c_[3] = a & uint8_t(0x77);
          bufp[3] = c_[(b >> 6) & 3];
        }
        else {
          ted.shiftRegisterCharacter.bitmap_() = b << 1;
          bufp[3] = ((b & uint8_t(0x80)) ? a : c_[0]);
        }
        break;
      default:
        if (a & uint8_t(0x08)) {
          int     tmp = (b >> 6) & 3;
          if (tmp == 3 || !(ted.tedRegisterWriteMask & (0x00200000U << tmp)))
            bufp[0] = c_[tmp];
          else
            bufp[0] = uint8_t(0xFF);
          if (!(nextCharCnt & 1)) {
            bufp[1] = c_[tmp];
            bufp[3] = bufp[2] = c_[(b >> 4) & 3];
          }
          else {
            bufp[2] = bufp[1] = c_[(b >> 4) & 3];
            bufp[3] = c_[(b >> 2) & 3];
          }
        }
        else {
          bufp[0] = ((b & uint8_t(0x80)) ?
                     a : (!(ted.tedRegisterWriteMask & 0x00200000U) ?
                          c_[0] : uint8_t(0xFF)));
          bufp[1] = ((b & uint8_t(0x40)) ? a : c_[0]);
          bufp[2] = ((b & uint8_t(0x20)) ? a : c_[0]);
          bufp[3] = ((b & uint8_t(0x10)) ? a : c_[0]);
        }
        ted.shiftRegisterCharacter.bitmap_() =
            ted.shiftRegisterCharacter.bitmap_() << 4;
        break;
      }
    }
  }

  REGPARM void TED7360::render_invalid_mode(TED7360& ted,
                                            uint8_t *bufp, int offs)
  {
    int     nextCharCnt = int(ted.horiz_scroll) - offs;
    if ((unsigned int) nextCharCnt < 4U) {
      ted.shiftRegisterCharacter = ted.currentCharacter;
      uint8_t b = ted.shiftRegisterCharacter.bitmap_();
      if (!(ted.videoMode & 0x10))
        ted.shiftRegisterCharacter.bitmap_() = b << (4 - nextCharCnt);
      else
        ted.shiftRegisterCharacter.bitmap_() = b << ((4 - nextCharCnt) & 6);
    }
    else {
      ted.shiftRegisterCharacter.bitmap_() =
          ted.shiftRegisterCharacter.bitmap_() << 4;
    }
    if (bufp) {
      bufp[3] = bufp[2] = bufp[1] = bufp[0] = uint8_t(0x00);
    }
  }

  void TED7360::selectRenderer()
  {
    switch (videoMode) {
    case 0x00:
      bitmapMode = false;
      render_func = &render_char_std;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xFC)) << 8;
      characterMask = uint8_t(0x7F);
      break;
    case 0x10:
      bitmapMode = false;
      render_func = &render_char_MCM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xFC)) << 8;
      characterMask = uint8_t(0x7F);
      break;
    case 0x20:
      bitmapMode = true;
      render_func = &render_BMM_hires;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x30:
      bitmapMode = true;
      render_func = &render_BMM_multicolor;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x40:
      bitmapMode = false;
      render_func = &render_char_ECM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0x50:
      bitmapMode = false;
      render_func = &render_invalid_mode;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0x60:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x70:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0x80:
      bitmapMode = false;
      render_func = &render_char_std;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0xFF);
      break;
    case 0x90:
      bitmapMode = false;
      render_func = &render_char_MCM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0xFF);
      break;
    case 0xA0:
      bitmapMode = true;
      render_func = &render_BMM_hires;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xB0:
      bitmapMode = true;
      render_func = &render_BMM_multicolor;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xC0:
      bitmapMode = false;
      render_func = &render_char_ECM;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0xD0:
      bitmapMode = false;
      render_func = &render_invalid_mode;
      charset_base_addr = int(tedRegisters[0x13] & uint8_t(0xF8)) << 8;
      characterMask = uint8_t(0x3F);
      break;
    case 0xE0:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    case 0xF0:
      bitmapMode = true;
      render_func = &render_invalid_mode;
      charset_base_addr = 0;
      characterMask = uint8_t(0);
      break;
    }
  }

}       // namespace Plus4

