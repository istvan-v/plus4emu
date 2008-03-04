
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

#ifndef P4FLICONV_MCFLI_HPP
#define P4FLICONV_MCFLI_HPP

#include "p4fliconv.hpp"

namespace Plus4FLIConv {

  class P4FLI_MultiColorNoInterlace : public FLIConverter {
   public:
    class Line160 {
     private:
      float   *buf;
      int     xShift;
      bool    multiColorFlag;
     public:
      Line160();
      Line160(const Line160& r);
      virtual ~Line160();
      Line160& operator=(const Line160& r);
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
        if (x >= 0L && x < 160L)
          buf[x + 16L] = n;
      }
      inline void setPixelShifted(long x, float n)
      {
        long    x_ = x + xShift;
        if (x_ >= 0L && x_ < 160L)
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
    class Image160x248 {
     private:
      Line160 *buf;
     public:
      Image160x248();
      Image160x248(const Image160x248& r);
      virtual ~Image160x248();
      Image160x248& operator=(const Image160x248& r);
      inline Line160& operator[](long n)
      {
        return buf[n];
      }
    };
    // ------------------------
    class YUVImage160x248 {
     private:
      Image160x248  imageY;
      Image160x248  imageU;
      Image160x248  imageV;
     public:
      YUVImage160x248();
      YUVImage160x248(const YUVImage160x248& r);
      virtual ~YUVImage160x248();
      YUVImage160x248& operator=(const YUVImage160x248& r);
      inline Image160x248& y()
      {
        return imageY;
      }
      inline Image160x248& u()
      {
        return imageU;
      }
      inline Image160x248& v()
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
    bool    enable40ColumnMode;
   private:
    struct FLIBlock4x2 {
      const double  *errorTable;
      int&    color0_0;
      int&    color0_1;
      int&    color3_0;
      int&    color3_1;
      int     color1;
      int     color2;
      int     nColors;
      int     pixelColorCodes[8];
      int     pixelColorCounts[8];
      int     nColors_0;
      int     pixelColorCodes_0[4];
      int     pixelColorCounts_0[4];
      int     nColors_1;
      int     pixelColorCodes_1[4];
      int     pixelColorCounts_1[4];
      FLIBlock4x2(const double *errorTable_,
                  int& color0_0_, int& color0_1_,
                  int& color3_0_, int& color3_1_);
      FLIBlock4x2(const FLIBlock4x2& r);
      ~FLIBlock4x2()
      {
      }
      void addPixel(int l, int c);
      inline double calculateError() const;
      inline double calculateErrorLine0() const;
      inline double calculateErrorLine1() const;
      double optimizeColors(const std::vector< int >& colorTable_);
    };
    YUVImage160x248 resizedImage;
    YUVImage160x248 ditherErrorImage;
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
    P4FLI_MultiColorNoInterlace();
    virtual ~P4FLI_MultiColorNoInterlace();
    virtual bool processImage(PRGData& prgData, unsigned int& prgEndAddr,
                              const char *infileName,
                              YUVImageConverter& imgConv,
                              Plus4Emu::ConfigurationDB& config);
  };

}       // namespace Plus4FLIConv

#endif  // P4FLICONV_MCFLI_HPP

