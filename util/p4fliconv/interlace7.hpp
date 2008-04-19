
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

#ifndef P4FLICONV_INTERLACE7_HPP
#define P4FLICONV_INTERLACE7_HPP

#include "p4fliconv.hpp"

namespace Plus4FLIConv {

  class P4FLI_Interlace7 : public FLIConverter {
   public:
    class Line320 {
     private:
      float   *buf;
      int     xShift;
     public:
      Line320();
      Line320(const Line320& r);
      virtual ~Line320();
      Line320& operator=(const Line320& r);
      void clear();
      void setBorderColor(float c);
      inline float& operator[](long n)
      {
        return buf[n + 16L];
      }
      inline float& pixel(long x)
      {
        return buf[x + 16L];
      }
      inline float& pixelShifted(long x)
      {
        return buf[x + 16L + xShift];
      }
      inline float getPixel(long x) const
      {
        return buf[x + 16L];
      }
      inline float getPixelShifted(long x) const
      {
        return buf[x + 16L + xShift];
      }
      inline void setPixel(long x, float n)
      {
        if (x >= 0L && x < 320L)
          buf[x + 16L] = n;
      }
      inline void setPixelShifted(long x, float n)
      {
        long    x_ = x + xShift;
        if (x_ >= 0L && x_ < 320L)
          buf[x_ + 16L] = n;
      }
      inline int getXShift() const
      {
        return xShift;
      }
      inline void setXShift(int n)
      {
        xShift = n & 7;
      }
    };
    // ------------------------
    class Image320x496 {
     private:
      Line320 *buf;
     public:
      Image320x496();
      Image320x496(const Image320x496& r);
      virtual ~Image320x496();
      Image320x496& operator=(const Image320x496& r);
      inline Line320& operator[](long n)
      {
        return buf[n];
      }
    };
    // ------------------------
    class YUVImage320x496 {
     private:
      Image320x496  imageY;
      Image320x496  imageU;
      Image320x496  imageV;
     public:
      YUVImage320x496();
      YUVImage320x496(const YUVImage320x496& r);
      virtual ~YUVImage320x496();
      YUVImage320x496& operator=(const YUVImage320x496& r);
      inline Image320x496& y()
      {
        return imageY;
      }
      inline Image320x496& u()
      {
        return imageU;
      }
      inline Image320x496& v()
      {
        return imageV;
      }
    };
    // ------------------------
    double  monitorGamma;
    double  ditherLimit;
    double  ditherScale;
    int     ditherMode;
    int     luminanceSearchMode;
    double  luminanceSearchModeParam;
    int     xShift0;
    int     xShift1;
    int     borderColor;
    int     nLines;
    int     colorInterlaceMode;
    bool    disablePAL;
    bool    disableInterlace;
    bool    luminance1BitMode;
    bool    noLuminanceInterlace;
    bool    enable40ColumnMode;
   private:
    struct UVTableEntry {
      int     c0;
      int     c1;
      float   u;
      float   v;
      double  err;
    };
    float   ditherYTable[9];
    float   errorYTable[9];
    UVTableEntry        uvTable[43];
    YUVImage320x496     resizedImage;
    Image320x496        ditherErrorImage;
    Line320 prvLineU;
    Line320 prvLineV;
    Line320 line0U;
    Line320 line0V;
    Line320 line1U;
    Line320 line1V;
    // ----------------
    static void pixelStoreCallback(void *, int, int, float, float, float);
    void colorToUV(int c, float& u, float& v);
    void createYTable();
    void createUVTables();
    void checkParameters();
    void ditherPixel(PRGData& prgData, long xc, long yc);
    inline double calculateLuminanceError(float n, int l0, int l1);
    double findLuminanceCodes(PRGData& prgData, long xc, long yc);
    void generateBitmaps(PRGData& prgData);
    void findColorCodes(PRGData& prgData, long xc, long yc, int dir_);
   public:
    P4FLI_Interlace7();
    virtual ~P4FLI_Interlace7();
    virtual bool processImage(PRGData& prgData, unsigned int& prgEndAddr,
                              const char *infileName,
                              YUVImageConverter& imgConv,
                              Plus4Emu::ConfigurationDB& config);
  };

}       // namespace Plus4FLIConv

#endif  // P4FLICONV_INTERLACE7_HPP

