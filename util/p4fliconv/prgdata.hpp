
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

#ifndef P4FLICONV_PRGDATA_HPP
#define P4FLICONV_PRGDATA_HPP

#include "p4fliconv.hpp"

namespace Plus4FLIConv {

  class PRGData {
   private:
    static const unsigned char prgHeader_320x400[0x0401];
    static const unsigned char prgHeader_320x456[0x0401];
    static const unsigned char prgHeader_320x464[0x0401];
    static const unsigned char prgHeader_320x496[0x0401];
    unsigned char   *buf;
    int     *luminanceCodeTable;
    int     *colorCodeTable;
    bool    *bitmapTable;
    const unsigned char *prgHeader;
    int     nLines;
   public:
    PRGData();
    virtual ~PRGData();
    void clear();
    void convertImageData();
    void setVerticalSize(int n);
    inline unsigned char& operator[](long n)
    {
      return buf[n];
    }
    inline unsigned char& borderColor()
    {
      return buf[0x0500L];
    }
    inline unsigned char& interlaceDisabled()
    {
      return buf[0x0800L];
    }
    inline unsigned char& lineColor0(long yc)
    {
      return buf[(0x0400L | ((yc & 1L) << 8) | ((yc & (~(long(3)))) >> 1))
                 + 1L];
    }
    inline unsigned char& lineColor1(long yc)
    {
      return buf[(0x0400L | ((yc & 1L) << 8) | ((yc & (~(long(3)))) >> 1))
                 + 2L];
    }
    inline unsigned char& lineXShift(long yc)
    {
      return buf[(0x0600L | ((yc & 1L) << 8) | (yc >> 1)) + 1L];
    }
    inline bool getPixel(long xc, long yc) const
    {
      return bitmapTable[(yc * 320L) + xc];
    }
    inline void setPixel(long xc, long yc, bool n)
    {
      bitmapTable[(yc * 320L) + xc] = n;
    }
    inline int& l0(long xc, long yc)
    {
      return luminanceCodeTable[((yc & (~(long(2)))) * 40L) + (xc >> 3)];
    }
    inline int& l1(long xc, long yc)
    {
      return luminanceCodeTable[((yc | 2L) * 40L) + (xc >> 3)];
    }
    inline int& c0(long xc, long yc)
    {
      return colorCodeTable[((yc & (~(long(2)))) * 40L) + (xc >> 3)];
    }
    inline int& c1(long xc, long yc)
    {
      return colorCodeTable[((yc | 2L) * 40L) + (xc >> 3)];
    }
  };

}       // namespace Plus4FLIConv

#endif  // P4FLICONV_PRGDATA_HPP

