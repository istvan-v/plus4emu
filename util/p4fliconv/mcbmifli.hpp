
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

#ifndef P4FLICONV_MCBMIFLI_HPP
#define P4FLICONV_MCBMIFLI_HPP

#include "p4fliconv.hpp"

namespace Plus4FLIConv {

  class P4FLI_MultiColorBitmapInterlace : public FLIConverter {
   public:
    class Line304 {
     private:
      float   *buf;
      int     xShift;
      bool    multiColorFlag;
     public:
      Line304();
      Line304(const Line304& r);
      virtual ~Line304();
      Line304& operator=(const Line304& r);
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
        if (x >= 0L && x < 304L)
          buf[x + 16L] = n;
      }
      inline void setPixelShifted(long x, float n)
      {
        long    x_ = x + xShift;
        if (x_ >= 0L && x_ < 304L)
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
      inline bool getMultiColorFlag() const
      {
        return multiColorFlag;
      }
      inline void setMultiColorFlag(bool n)
      {
        multiColorFlag = n;
      }
    };
    // ------------------------
    class Image304x248 {
     private:
      Line304 *buf;
     public:
      Image304x248();
      Image304x248(const Image304x248& r);
      virtual ~Image304x248();
      Image304x248& operator=(const Image304x248& r);
      inline Line304& operator[](long n)
      {
        return buf[n];
      }
    };
    // ------------------------
    class YUVImage304x248 {
     private:
      Image304x248  imageY;
      Image304x248  imageU;
      Image304x248  imageV;
     public:
      YUVImage304x248();
      YUVImage304x248(const YUVImage304x248& r);
      virtual ~YUVImage304x248();
      YUVImage304x248& operator=(const YUVImage304x248& r);
      inline Image304x248& y()
      {
        return imageY;
      }
      inline Image304x248& u()
      {
        return imageU;
      }
      inline Image304x248& v()
      {
        return imageV;
      }
    };
    // ------------------------
    double  monitorGamma;
    double  ditherLimit;
    double  ditherScale;
    int     ditherMode;
    int     xShift0;
    int     borderColor;
    int     nLines;
    int     conversionQuality;
    bool    luminance1BitMode;
   private:
    struct FLIBlock8x2 {
      const double  *errorTable;
      int     *color0;
      int     *color3;
      int     color1;
      int     color2;
      int     nColors;
      int     pixelColorCodes[16];
      int     pixelColorCounts[16];
      int     lineColors[4];
      int     linePixelColorCodes[4][4];
      int     linePixelColorCounts[4][4];
      FLIBlock8x2(const double *errorTable_, int *color0_, int *color3_);
      FLIBlock8x2(const FLIBlock8x2& r);
      ~FLIBlock8x2()
      {
      }
      void addPixel(int l, int c);
      inline double calculateError() const;
      inline double calculateLineError(int n) const;
      double optimizeColors(const std::vector< int >& colorTable_);
    };
    YUVImage304x248 resizedImage;
    YUVImage304x248 ditherErrorImage;
    int     *ditheredImage;
    double  *errorTable;
    int     *xShiftTable;
    // ----------------
    static void pixelStoreCallback(void *, int, int, float, float, float);
    void checkParameters();
    void createErrorTable(double colorErrorScale);
    void ditherLine(long yc);
    double convertTwoLines(PRGData& prgData, long yc);
   public:
    P4FLI_MultiColorBitmapInterlace();
    virtual ~P4FLI_MultiColorBitmapInterlace();
    virtual bool processImage(PRGData& prgData, unsigned int& prgEndAddr,
                              const char *infileName,
                              YUVImageConverter& imgConv,
                              Plus4Emu::ConfigurationDB& config);
  };

}       // namespace Plus4FLIConv

#endif  // P4FLICONV_MCBMIFLI_HPP

