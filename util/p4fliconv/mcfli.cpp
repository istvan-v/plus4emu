
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
#include "dither.hpp"
#include "imageconv.hpp"
#include "prgdata.hpp"
#include "mcfli.hpp"

namespace Plus4FLIConv {

  P4FLI_MultiColor::Line304::Line304()
    : xShift(0),
      multiColorFlag(false)
  {
    buf = new float[336];
    for (size_t i = 0; i < 336; i++)
      buf[i] = 0.0f;
  }

  P4FLI_MultiColor::Line304::Line304(const Line304& r)
    : xShift(r.xShift),
      multiColorFlag(r.multiColorFlag)
  {
    buf = new float[336];
    for (size_t i = 0; i < 336; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_MultiColor::Line304::~Line304()
  {
    delete[] buf;
  }

  P4FLI_MultiColor::Line304&
      P4FLI_MultiColor::Line304::operator=(const Line304& r)
  {
    xShift = r.xShift;
    multiColorFlag = r.multiColorFlag;
    for (size_t i = 0; i < 336; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  void P4FLI_MultiColor::Line304::clear()
  {
    for (size_t i = 16; i < 320; i++)
      buf[i] = 0.0f;
  }

  void P4FLI_MultiColor::Line304::setBorderColor(float c)
  {
    for (size_t i = 0; i < 16; i++) {
      buf[i] = c;
      buf[i + 320] = c;
    }
  }

  // --------------------------------------------------------------------------

  P4FLI_MultiColor::Image304x248::Image304x248()
  {
    buf = new Line304[248];
  }

  P4FLI_MultiColor::Image304x248::Image304x248(const Image304x248& r)
  {
    buf = new Line304[248];
    for (size_t i = 0; i < 248; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_MultiColor::Image304x248&
      P4FLI_MultiColor::Image304x248::operator=(const Image304x248& r)
  {
    for (size_t i = 0; i < 248; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  P4FLI_MultiColor::Image304x248::~Image304x248()
  {
    delete[] buf;
  }

  // --------------------------------------------------------------------------

  P4FLI_MultiColor::YUVImage304x248::YUVImage304x248()
  {
  }

  P4FLI_MultiColor::YUVImage304x248::YUVImage304x248(const YUVImage304x248& r)
    : imageY(r.imageY),
      imageU(r.imageU),
      imageV(r.imageV)
  {
  }

  P4FLI_MultiColor::YUVImage304x248::~YUVImage304x248()
  {
  }

  P4FLI_MultiColor::YUVImage304x248&
      P4FLI_MultiColor::YUVImage304x248::operator=(const YUVImage304x248& r)
  {
    imageY = r.imageY;
    imageU = r.imageU;
    imageV = r.imageV;
    return (*this);
  }

  // --------------------------------------------------------------------------

  P4FLI_MultiColor::P4FLI_MultiColor()
    : monitorGamma(1.33),
      ditherLimit(0.125),
      ditherScale(0.75),
      ditherMode(0),
      xShift0(-1),
      xShift1(-1),
      borderColor(0x00),
      nLines(232),
      luminance1BitMode(false),
      ditheredImage((int *) 0),
      errorTable((double *) 0),
      xShiftTable((int *) 0)
  {
    try {
      ditheredImage = new int[304 * 248];
      errorTable = new double[128 * 128];
      xShiftTable = new int[496];
      for (int i = 0; i < (304 * 248); i++)
        ditheredImage[i] = 0;
      for (int i = 0; i < (128 * 128); i++)
        errorTable[i] = 0.0;
      for (int i = 0; i < 496; i++)
        xShiftTable[i] = i & 1;
    }
    catch (...) {
      if (ditheredImage)
        delete[] ditheredImage;
      if (errorTable)
        delete[] errorTable;
      if (xShiftTable)
        delete[] xShiftTable;
      throw;
    }
  }

  P4FLI_MultiColor::~P4FLI_MultiColor()
  {
    delete[] ditheredImage;
    delete[] errorTable;
    delete[] xShiftTable;
  }

  void P4FLI_MultiColor::pixelStoreCallback(void *userData, int xc, int yc,
                                            float y, float u, float v)
  {
    P4FLI_MultiColor&  this_ =
        *(reinterpret_cast<P4FLI_MultiColor *>(userData));
    this_.resizedImage.y()[yc >> 1][xc >> 1] += (y * 0.25f);
    this_.resizedImage.u()[yc >> 1][xc >> 1] += (u * 0.25f);
    this_.resizedImage.v()[yc >> 1][xc >> 1] += (v * 0.25f);
  }

  void P4FLI_MultiColor::checkParameters()
  {
    limitValue(monitorGamma, 0.25, 4.0);
    limitValue(ditherLimit, 0.0, 2.0);
    limitValue(ditherScale, 0.0, 1.0);
    limitValue(ditherMode, 0, 3);
    limitValue(xShift0, -2, 7);
    limitValue(xShift1, -2, 7);
    borderColor = (borderColor & 0x7F) | 0x80;
    nLines = (nLines <= 228 ? (nLines <= 200 ? 200 : 228)
                              : (nLines <= 232 ? 232 : 248));
  }

  P4FLI_MultiColor::FLIBlock4x2::FLIBlock4x2(const double *errorTable_,
                                             int& color0_, int& color3_)
    : errorTable(errorTable_),
      color0(color0_),
      color3(color3_),
      color1(0x00),
      color2(0x00),
      nColors(0)
  {
    for (int i = 0; i < 8; i++) {
      pixelColorCodes[i] = 0x00;
      pixelColorCounts[i] = 0;
    }
  }

  P4FLI_MultiColor::FLIBlock4x2::FLIBlock4x2(const FLIBlock4x2& r)
    : errorTable(r.errorTable),
      color0(r.color0),
      color3(r.color3),
      color1(r.color1),
      color2(r.color2),
      nColors(r.nColors)
  {
    for (int i = 0; i < 8; i++) {
      pixelColorCodes[i] = r.pixelColorCodes[i];
      pixelColorCounts[i] = r.pixelColorCounts[i];
    }
  }

  void P4FLI_MultiColor::FLIBlock4x2::addPixel(int c)
  {
    c = c & 0x7F;
    if ((c & 0x0F) == 0)
      c = 0x00;
    {
      int     i = 0;
      while (i < nColors) {
        if (pixelColorCodes[i] == c) {
          pixelColorCounts[i]++;
          break;
        }
        i++;
      }
      if (i >= nColors) {
        pixelColorCodes[nColors] = c;
        pixelColorCounts[nColors] = 1;
        nColors++;
      }
    }
    for (int i = 0; i < nColors; i++) {
      for (int j = i + 1; j < nColors; j++) {
        if (pixelColorCounts[j] > pixelColorCounts[i]) {
          int     tmp = pixelColorCodes[i];
          pixelColorCodes[i] = pixelColorCodes[j];
          pixelColorCodes[j] = tmp;
          tmp = pixelColorCounts[i];
          pixelColorCounts[i] = pixelColorCounts[j];
          pixelColorCounts[j] = tmp;
        }
      }
    }
  }

  inline double P4FLI_MultiColor::FLIBlock4x2::calculateError() const
  {
    double  totalErr = 0.0;
    for (int i = 0; i < nColors; i++) {
      int     c = pixelColorCodes[i];
      double  minErr = errorTable[(c << 7) | color0];
      double  err = errorTable[(c << 7) | color1];
      if (err < minErr)
        minErr = err;
      err = errorTable[(c << 7) | color2];
      if (err < minErr)
        minErr = err;
      err = errorTable[(c << 7) | color3];
      if (err < minErr)
        minErr = err;
      totalErr += (minErr * double(pixelColorCounts[i]));
    }
    return totalErr;
  }

  void P4FLI_MultiColor::createErrorTable()
  {
    for (int c0 = 0; c0 < 128; c0++) {
      float   c0y = 0.0f;
      float   c0u = 0.0f;
      float   c0v = 0.0f;
      FLIConverter::convertPlus4Color(c0, c0y, c0u, c0v, monitorGamma);
      for (int c1 = 0; c1 < 128; c1++) {
        float   c1y = 0.0f;
        float   c1u = 0.0f;
        float   c1v = 0.0f;
        FLIConverter::convertPlus4Color(c1, c1y, c1u, c1v, monitorGamma);
        errorTable[(c0 << 7) | c1] = calculateErrorSqr(c0y, c1y)
                                     + (calculateErrorSqr(c0u, c1u) * 0.5)
                                     + (calculateErrorSqr(c0v, c1v) * 0.5);
      }
    }
  }

  void P4FLI_MultiColor::ditherLine(long yc)
  {
    float   paletteY[128];
    float   paletteU[128];
    float   paletteV[128];
    for (int i = 0; i < 128; i++) {
      FLIConverter::convertPlus4Color(i, paletteY[i], paletteU[i], paletteV[i],
                                      monitorGamma);
    }
    if (ditherMode == 0) {
      // ordered dithering
      float   luminanceTable[9];
      float   hueTable[15];
      int     hueIndexTable[15];
      for (int i = 0; i < 9; i++)
        luminanceTable[i] = paletteY[(i == 0 ? 0 : (((i - 1) << 4) + 1))];
      for (int i = 0; i < 14; i++) {
        float   u = paletteU[66 + i];
        float   v = paletteV[66 + i];
        double  phs = std::atan2(double(v), double(u)) / (2.0 * 3.14159265);
        if (phs < 0.0)
          phs = phs + 1.0;
        hueTable[i] = float(phs);
        hueIndexTable[i] = i;
      }
      for (int i = 0; i < 14; i++) {
        for (int j = i + 1; j < 14; j++) {
          if (hueTable[j] < hueTable[i]) {
            {
              float   tmp = hueTable[i];
              hueTable[i] = hueTable[j];
              hueTable[j] = tmp;
            }
            {
              int     tmp = hueIndexTable[i];
              hueIndexTable[i] = hueIndexTable[j];
              hueIndexTable[j] = tmp;
            }
          }
        }
      }
      hueTable[14] = hueTable[0] + 1.0f;
      hueIndexTable[14] = hueIndexTable[0];
      for (long xc = 0L; xc < 304L; xc++) {
        float   y = resizedImage.y()[yc].getPixel(xc);
        float   u = resizedImage.u()[yc].getPixel(xc);
        float   v = resizedImage.v()[yc].getPixel(xc);
        float   s = float(std::sqrt(double(u * u) + double(v * v)));
        if (s > FLIConverter::defaultColorSaturation)
          s = FLIConverter::defaultColorSaturation;
        float   h = float(std::atan2(double(v), double(u)));
        h = h / (2.0f * 3.14159265f);
        if (h < 0.0f)
          h = h + 1.0f;
        int     li0 = 0;
        int     li1 = 8;
        if (!luminance1BitMode) {
          while (y > luminanceTable[li0 + 1] && li0 < 7)
            li0++;
          li1 = li0 + 1;
        }
        if (calculateError(calculateError(y, luminanceTable[li0]),
                           calculateError(y, luminanceTable[li1]))
            >= ditherLimit) {
          if (calculateError(y, luminanceTable[li0])
              > calculateError(y, luminanceTable[li1])) {
            li0 = li1;
          }
        }
        else {
          float   f = (y - luminanceTable[li0])
                      / (luminanceTable[li1] - luminanceTable[li0]);
          if (ditherPixelValue(xc, yc, f))
            li0 = li1;
        }
        int     si = 0;
        if (ditherPixelValue(xc, yc, s / FLIConverter::defaultColorSaturation))
          si++;
        int     hi = 0;
        if (h < hueTable[0])    // special case for hue wrap-around
          h = h + 1.0f;
        while (h > hueTable[hi + 1] && hi < 13)
          hi++;
        float   f = (h - hueTable[hi]) / (hueTable[hi + 1] - hueTable[hi]);
        if (ditherPixelValue(xc, yc, f))
          hi++;
        hi = hueIndexTable[hi];
        ditheredImage[yc * 304L + xc] =
            (li0 == 0 ? 0 : ((si == 0 ? 1 : (hi + 2)) + ((li0 - 1) << 4)));
      }
      return;
    }
    // error diffusion dithering
    for (long xc = 0L; xc < 304L; xc++) {
      if (yc & 1L)
        xc = 303L - xc;
      // find the palette color nearest the original pixel
      float   y0 = resizedImage.y()[yc].getPixel(xc);
      float   u0 = resizedImage.u()[yc].getPixel(xc);
      float   v0 = resizedImage.v()[yc].getPixel(xc);
      {
        float   tmp = float(std::sqrt(double(u0 * u0) + double(v0 * v0)));
        if (tmp > FLIConverter::defaultColorSaturation) {
          tmp = FLIConverter::defaultColorSaturation / tmp;
          u0 = u0 * tmp;
          v0 = v0 * tmp;
        }
      }
      int     c0 = 0;
      double  minErr0 = 1000000.0;
      for (int i = (luminance1BitMode ? 112 : 0); i < 128; i++) {
        double  err = calculateErrorSqr(paletteY[i], y0)
                      + (calculateErrorSqr(paletteU[i], u0) * 0.0625)
                      + (calculateErrorSqr(paletteV[i], v0) * 0.0625);
        if (err < minErr0) {
          c0 = i;
          minErr0 = err;
        }
      }
      // find the palette color nearest the original pixel with error added
      float   y = y0 + ditherErrorImage.y()[yc].getPixel(xc);
      float   u = u0 + ditherErrorImage.u()[yc].getPixel(xc);
      float   v = v0 + ditherErrorImage.v()[yc].getPixel(xc);
      y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
      {
        float   tmp = float(std::sqrt(double(u * u) + double(v * v)));
        if (tmp > FLIConverter::defaultColorSaturation) {
          tmp = FLIConverter::defaultColorSaturation / tmp;
          u = u * tmp;
          v = v * tmp;
        }
      }
      int     c = 0;
      double  minErr = 1000000.0;
      for (int i = (luminance1BitMode ? 112 : 0); i < 128; i++) {
        double  err = calculateErrorSqr(paletteY[i], y)
                      + (calculateErrorSqr(paletteU[i], u) * 0.0625)
                      + (calculateErrorSqr(paletteV[i], v) * 0.0625);
        if (err < minErr) {
          c = i;
          minErr = err;
        }
      }
      if (calculateError(
              std::sqrt(minErr0),
              std::sqrt(calculateErrorSqr(paletteY[c], y0)
                        + (calculateErrorSqr(paletteU[c], u0) * 0.0625)
                        + (calculateErrorSqr(paletteV[c], v0) * 0.0625)))
          < ditherLimit) {
        ditheredImage[yc * 304L + xc] = c;
      }
      else {
        ditheredImage[yc * 304L + xc] = c0;
      }
      y = y0 + ((y - y0) * float(ditherScale));
      u = u0 + ((u - u0) * float(ditherScale));
      v = v0 + ((v - v0) * float(ditherScale));
      float   errY = y - paletteY[c];
      float   errU = u - paletteU[c];
      float   errV = v - paletteV[c];
      if (ditherMode == 1) {
        // Floyd-Steinberg dithering
        static const int    xOffsTbl[4] = { 1, -1, 0, 1 };
        static const int    yOffsTbl[4] = { 0, 1, 1, 1 };
        static const float  errMultTbl[4] = {
          0.4375f, 0.1875f, 0.3125f, 0.0625f
        };
        for (int i = 0; i < 4; i++) {
          long    yc_ = yc + yOffsTbl[i];
          long    xc_ = xOffsTbl[i];
          xc_ = ((yc & 1L) == 0L ? (xc + xc_) : (xc - xc_));
          if (yc_ >= 0L && yc_ < long(nLines) && xc_ >= 0L && xc_ < 304L) {
            float   errMult = errMultTbl[i];
            ditherErrorImage.y()[yc_].setPixel(
                xc_,
                ditherErrorImage.y()[yc_].getPixel(xc_) + (errY * errMult));
            ditherErrorImage.u()[yc_].setPixel(
                xc_,
                ditherErrorImage.u()[yc_].getPixel(xc_) + (errU * errMult));
            ditherErrorImage.v()[yc_].setPixel(
                xc_,
                ditherErrorImage.v()[yc_].getPixel(xc_) + (errV * errMult));
          }
        }
      }
      else if (ditherMode == 2) {
        // Jarvis dithering
        for (int i = 0; i < 3; i++) {
          for (int j = (i == 0 ? 1 : -2); j < 3; j++) {
            if (yc & 1L)
              j = (-j);
            long    yc_ = yc + i;
            long    xc_ = xc + j;
            if (yc_ >= 0L && yc_ < long(nLines) && xc_ >= 0L && xc_ < 304L) {
              float   errMult = (4.5f - float(i + (j >= 0 ? j : (-j)))) / 24.0f;
              ditherErrorImage.y()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.y()[yc_].getPixel(xc_) + (errY * errMult));
              ditherErrorImage.u()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.u()[yc_].getPixel(xc_) + (errU * errMult));
              ditherErrorImage.v()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.v()[yc_].getPixel(xc_) + (errV * errMult));
            }
            if (yc & 1L)
              j = (-j);
          }
        }
      }
      else {
        // Stucki dithering
        for (int i = 0; i < 3; i++) {
          for (int j = (i == 0 ? 1 : -2); j < 3; j++) {
            if (yc & 1L)
              j = (-j);
            long    yc_ = yc + i;
            long    xc_ = xc + j;
            if (yc_ >= 0L && yc_ < long(nLines) && xc_ >= 0L && xc_ < 304L) {
              float   errMult = float(16 >> (i + (j >= 0 ? j : (-j)))) / 42.0f;
              ditherErrorImage.y()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.y()[yc_].getPixel(xc_) + (errY * errMult));
              ditherErrorImage.u()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.u()[yc_].getPixel(xc_) + (errU * errMult));
              ditherErrorImage.v()[yc_].setPixel(
                  xc_,
                  ditherErrorImage.v()[yc_].getPixel(xc_) + (errV * errMult));
            }
            if (yc & 1L)
              j = (-j);
          }
        }
      }
      if (yc & 1L)
        xc = 303L - xc;
    }
  }

  double P4FLI_MultiColor::convertTwoLines(PRGData& prgData,
                                           long yc, bool oddField)
  {
    int     color0 = 0;
    int     color3 = 0;
    // find the color indexes used in each attribute block area
    std::vector< FLIBlock4x2 >
        attrBlocks(40, FLIBlock4x2(errorTable, color0, color3));
    for (int i = 0; i < 2; i++) {
      int     xs = xShiftTable[((yc + i) << 1) | long(oddField)];
      if (oddField)
        attrBlocks[(7 - xs) >> 3].addPixel(ditheredImage[(yc + i) * 304L]);
      for (int xc = int(oddField); xc < 304; xc += 2) {
        int     n = (xc + 8 - xs) >> 3;
        attrBlocks[n].addPixel(ditheredImage[(yc + i) * 304L + xc]);
      }
    }
    // find the set of colors that needs to be searched for optimal conversion
    std::vector< int >  colorTable0;                    // for color #0 and #3
    std::vector< std::vector< int > >   colorTables;    // for color #1 and #2
    colorTables.resize(40);
    {
      float   minY0 = 1.0f;
      float   maxY0 = 0.0f;
      float   minU0 = 1.0f;
      float   maxU0 = -1.0f;
      float   minV0 = 1.0f;
      float   maxV0 = -1.0f;
      for (int i = 0; i < 40; i++) {
        float   minY = 1.0f;
        float   maxY = 0.0f;
        float   minU = 1.0f;
        float   maxU = -1.0f;
        float   minV = 1.0f;
        float   maxV = -1.0f;
        for (int j = 0; j < attrBlocks[i].nColors; j++) {
          float   y = 0.0f;
          float   u = 0.0f;
          float   v = 0.0f;
          FLIConverter::convertPlus4Color(attrBlocks[i].pixelColorCodes[j],
                                          y, u, v, monitorGamma);
          minY = (y < minY ? y : minY);
          maxY = (y > maxY ? y : maxY);
          minU = (u < minU ? u : minU);
          maxU = (u > maxU ? u : maxU);
          minV = (v < minV ? v : minV);
          maxV = (v > maxV ? v : maxV);
          if (attrBlocks[i].nColors > 2) {
            minY0 = (y < minY0 ? y : minY0);
            maxY0 = (y > maxY0 ? y : maxY0);
            minU0 = (u < minU0 ? u : minU0);
            maxU0 = (u > maxU0 ? u : maxU0);
            minV0 = (v < minV0 ? v : minV0);
            maxV0 = (v > maxV0 ? v : maxV0);
          }
        }
        if (minV < 0.0f && maxV > 0.0f) {
          if (minU < 0.0f)
            minU = -(FLIConverter::defaultColorSaturation);
          if (maxU > 0.0f)
            maxU = FLIConverter::defaultColorSaturation;
        }
        if (minU < 0.0f && maxU > 0.0f) {
          if (minV < 0.0f)
            minV = -(FLIConverter::defaultColorSaturation);
          if (maxV > 0.0f)
            maxV = FLIConverter::defaultColorSaturation;
        }
        minY = minY - 0.0001f;
        maxY = maxY + 0.0001f;
        minU = minU - 0.0001f;
        maxU = maxU + 0.0001f;
        minV = minV - 0.0001f;
        maxV = maxV + 0.0001f;
        for (int j = 0; j < 128; j++) {
          float   y = 0.0f;
          float   u = 0.0f;
          float   v = 0.0f;
          FLIConverter::convertPlus4Color(j, y, u, v, monitorGamma);
          if (y > minY && y < maxY &&
              u > minU && u < maxU &&
              v > minV && v < maxV) {
            colorTables[i].push_back(j);
          }
        }
      }
      if (minV0 < 0.0f && maxV0 > 0.0f) {
        if (minU0 < 0.0f)
          minU0 = -(FLIConverter::defaultColorSaturation);
        if (maxU0 > 0.0f)
          maxU0 = FLIConverter::defaultColorSaturation;
      }
      if (minU0 < 0.0f && maxU0 > 0.0f) {
        if (minV0 < 0.0f)
          minV0 = -(FLIConverter::defaultColorSaturation);
        if (maxV0 > 0.0f)
          maxV0 = FLIConverter::defaultColorSaturation;
      }
      minY0 = minY0 - 0.0001f;
      maxY0 = maxY0 + 0.0001f;
      minU0 = minU0 - 0.0001f;
      maxU0 = maxU0 + 0.0001f;
      minV0 = minV0 - 0.0001f;
      maxV0 = maxV0 + 0.0001f;
      for (int j = 0; j < 128; j++) {
        float   y = 0.0f;
        float   u = 0.0f;
        float   v = 0.0f;
        FLIConverter::convertPlus4Color(j, y, u, v, monitorGamma);
        if (y > minY0 && y < maxY0 &&
            u > minU0 && u < maxU0 &&
            v > minV0 && v < maxV0) {
          colorTable0.push_back(j);
        }
      }
    }
    double  bestErr = 1000000.0;
    int     bestColors[82];
    std::vector< int >  colorCnts(128);
    for (int l = 0; l < 4; l++) {
      // set initial palette with four different methods, and choose the one
      // that results in the least error after optimization
      for (int i = 0; i < 128; i++)
        colorCnts[i] = 0;
      for (int i = 0; i < 40; i++) {
        int     nColors = attrBlocks[i].nColors;
        if (nColors <= 2 || l == 0) {
          attrBlocks[i].color1 = attrBlocks[i].pixelColorCodes[0];
          attrBlocks[i].color2 = attrBlocks[i].pixelColorCodes[1];
        }
        else if (l == 1) {
          attrBlocks[i].color1 = attrBlocks[i].pixelColorCodes[nColors - 1];
          attrBlocks[i].color2 = attrBlocks[i].pixelColorCodes[nColors - 2];
        }
        else if (l == 2) {
          attrBlocks[i].color1 = attrBlocks[i].pixelColorCodes[0];
          attrBlocks[i].color2 = attrBlocks[i].pixelColorCodes[nColors - 1];
        }
        else {
          attrBlocks[i].color1 = attrBlocks[i].pixelColorCodes[0];
          double  maxErr = 0.0;
          for (int j = 1; j < nColors; j++) {
            double  err = errorTable[(attrBlocks[i].color1 << 7)
                                     | (attrBlocks[i].pixelColorCodes[j])];
            if (err >= maxErr) {
              attrBlocks[i].color2 = attrBlocks[i].pixelColorCodes[j];
              maxErr = err;
            }
          }
        }
        if (nColors > 2) {
          for (int j = 0; j < nColors; j++) {
            int     c = attrBlocks[i].pixelColorCodes[j];
            if (c != attrBlocks[i].color1 && c != attrBlocks[i].color2)
              colorCnts[c] = colorCnts[c] + attrBlocks[i].pixelColorCounts[j];
          }
        }
      }
      int     maxCnt1 = 0;
      int     maxCnt2 = 0;
      for (int i = 0; i < 128; i++) {
        if (colorCnts[i] > maxCnt1) {
          maxCnt2 = maxCnt1;
          color3 = color0;
          maxCnt1 = colorCnts[i];
          color0 = i;
        }
        else if (colorCnts[i] > maxCnt2) {
          maxCnt2 = colorCnts[i];
          color3 = i;
        }
      }
      // optimize attributes and color registers
      for (int i = 15; i >= 0; i--) {
        double  prvErr = 0.0;
        for (int k = 0; k < 40; k++)
          prvErr += attrBlocks[k].calculateError();
        double  minErr;
        int     bestColor;
        if (colorTable0.size() <= 24) {
          // color #0 (FF15) and color #3 (FF16)
          minErr = 1000000.0;
          int     bestColor0 = color0;
          int     bestColor3 = color3;
          for (size_t c0i = 0; c0i < colorTable0.size(); c0i++) {
            for (size_t c3i = c0i + 1; c3i < colorTable0.size(); c3i++) {
              color0 = colorTable0[c0i];
              color3 = colorTable0[c3i];
              double  err = 0.0;
              for (int k = 0; k < 40; k++) {
                if (attrBlocks[k].nColors <= 2)
                  continue;
                err += attrBlocks[k].calculateError();
              }
              if (err < minErr) {
                bestColor0 = color0;
                bestColor3 = color3;
                minErr = err;
              }
            }
          }
          color0 = bestColor0;
          color3 = bestColor3;
        }
        else {
          // color #0 (FF15)
          minErr = 1000000.0;
          bestColor = color0;
          for (size_t j = 0; j < colorTable0.size(); j++) {
            color0 = colorTable0[j];
            double  err = 0.0;
            for (int k = 0; k < 40; k++) {
              if (attrBlocks[k].nColors <= 2)
                continue;
              err += attrBlocks[k].calculateError();
            }
            if (err < minErr) {
              bestColor = color0;
              minErr = err;
            }
          }
          color0 = bestColor;
          // color #3 (FF16)
          minErr = 1000000.0;
          bestColor = color3;
          for (size_t j = 0; j < colorTable0.size(); j++) {
            color3 = colorTable0[j];
            double  err = 0.0;
            for (int k = 0; k < 40; k++) {
              if (attrBlocks[k].nColors <= 2)
                continue;
              err += attrBlocks[k].calculateError();
            }
            if (err < minErr) {
              bestColor = color3;
              minErr = err;
            }
          }
          color3 = bestColor;
        }
        for (int k = 0; k < 40; k++) {
          if (attrBlocks[k].nColors <= 2)
            continue;
          if (colorTables[k].size() <= 32) {
            // color #1 and color #2
            minErr = 1000000.0;
            int     bestColor1 = attrBlocks[k].color1;
            int     bestColor2 = attrBlocks[k].color2;
            for (size_t c1i = 0; c1i < colorTables[k].size(); c1i++) {
              for (size_t c2i = c1i + 1; c2i < colorTables[k].size(); c2i++) {
                attrBlocks[k].color1 = colorTables[k][c1i];
                attrBlocks[k].color2 = colorTables[k][c2i];
                double  err = attrBlocks[k].calculateError();
                if (err < minErr) {
                  bestColor1 = attrBlocks[k].color1;
                  bestColor2 = attrBlocks[k].color2;
                  minErr = err;
                }
              }
            }
            attrBlocks[k].color1 = bestColor1;
            attrBlocks[k].color2 = bestColor2;
          }
          else {
            // color #1
            minErr = 1000000.0;
            bestColor = attrBlocks[k].color1;
            for (size_t j = 0; j < colorTables[k].size(); j++) {
              attrBlocks[k].color1 = colorTables[k][j];
              double  err = attrBlocks[k].calculateError();
              if (err < minErr) {
                bestColor = attrBlocks[k].color1;
                minErr = err;
              }
            }
            attrBlocks[k].color1 = bestColor;
            // color #2
            minErr = 1000000.0;
            bestColor = attrBlocks[k].color2;
            for (size_t j = 0; j < colorTables[k].size(); j++) {
              attrBlocks[k].color2 = colorTables[k][j];
              double  err = attrBlocks[k].calculateError();
              if (err < minErr) {
                bestColor = attrBlocks[k].color2;
                minErr = err;
              }
            }
            attrBlocks[k].color2 = bestColor;
          }
        }
        // quit the optimization loop earlier if the error could not be reduced
        double  err = 0.0;
        for (int k = 0; k < 40; k++)
          err += attrBlocks[k].calculateError();
        if (err >= (prvErr * 0.99999999))
          break;
      }
      double  err = 0.0;
      for (int i = 0; i < 40; i++)
        err += attrBlocks[i].calculateError();
      if (err < bestErr) {
        for (int i = 0; i < 80; i += 2) {
          bestColors[i] = attrBlocks[i >> 1].color1;
          bestColors[i + 1] = attrBlocks[i >> 1].color2;
        }
        bestColors[80] = color0;
        bestColors[81] = color3;
        bestErr = err;
      }
    }
    for (int i = 0; i < 80; i += 2) {
      attrBlocks[i >> 1].color1 = bestColors[i];
      attrBlocks[i >> 1].color2 = bestColors[i + 1];
    }
    color0 = bestColors[80];
    color3 = bestColors[81];
    // store the attributes and color registers
    prgData.lineColor0((yc << 1) | long(oddField)) = (unsigned char) color0;
    prgData.lineColor1((yc << 1) | long(oddField)) = (unsigned char) color3;
    for (int i = 0; i < 40; i++) {
      int     l0 = (attrBlocks[i].color2 >> 4) & 0x07;
      int     l1 = (attrBlocks[i].color1 >> 4) & 0x07;
      int     c0 = attrBlocks[i].color2 & 0x0F;
      int     c1 = attrBlocks[i].color1 & 0x0F;
      if (c0 == 0)
        l0 = 0;
      else
        l0++;
      if (c1 == 0)
        l1 = 0;
      else
        l1++;
      prgData.l0(i << 3, (yc << 1) | long(oddField)) = l0;
      prgData.l1(i << 3, (yc << 1) | long(oddField)) = l1;
      prgData.c0(i << 3, (yc << 1) | long(oddField)) = c0;
      prgData.c1(i << 3, (yc << 1) | long(oddField)) = c1;
    }
    // generate bitmaps
    for (int i = 0; i < 2; i++) {
      for (int j = -(int(oddField)); j < 304; j += 2) {
        int     xc = j + 8 - xShiftTable[((yc + i) << 1) | long(oddField)];
        int     n = xc >> 3;
        int     c = ditheredImage[(yc + i) * 304L + (j >= 0 ? j : 0)];
        int     ci = 0;
        double  minErr = errorTable[(c << 7) | color0];
        double  err = errorTable[(c << 7) | attrBlocks[n].color1];
        if (err < minErr) {
          ci = 1;
          minErr = err;
        }
        err = errorTable[(c << 7) | attrBlocks[n].color2];
        if (err < minErr) {
          ci = 2;
          minErr = err;
        }
        err = errorTable[(c << 7) | color3];
        if (err < minErr)
          ci = 3;
        prgData.setPixel(xc & (~(int(1))), ((yc + i) << 1) | long(oddField),
                         bool(ci & 2));
        prgData.setPixel(xc | 1, ((yc + i) << 1) | long(oddField),
                         bool(ci & 1));
      }
    }
    // return the total amount of error
    double  err = 0.0;
    for (int i = 0; i < 40; i++)
      err += attrBlocks[i].calculateError();
    return err;
  }

  bool P4FLI_MultiColor::processImage(PRGData& prgData,
                                      unsigned int& prgEndAddr,
                                      const char *infileName,
                                      YUVImageConverter& imgConv,
                                      Plus4Emu::ConfigurationDB& config)
  {
    try {
      monitorGamma = config["monitorGamma"];
      ditherLimit = config["ditherLimit"];
      ditherScale = config["ditherDiffusion"];
      ditherMode = config["ditherMode"];
      xShift0 = config["xShift0"];
      xShift1 = config["xShift1"];
      borderColor = config["borderColor"];
      nLines = config["verticalSize"];
      if (nLines >= 400)
        nLines = nLines >> 1;
      luminance1BitMode = config["luminance1BitMode"];
      checkParameters();
      createErrorTable();
      float   borderY = 0.0f;
      float   borderU = 0.0f;
      float   borderV = 0.0f;
      FLIConverter::convertPlus4Color(borderColor, borderY, borderU, borderV,
                                      monitorGamma);
      prgData.clear();
      prgData.borderColor() = (unsigned char) borderColor;
      prgData.setVerticalSize(nLines);
      prgData.interlaceDisabled() = 0x01;
      for (int yc = 0; yc < 248; yc++) {
        resizedImage.y()[yc].clear();
        resizedImage.y()[yc].setBorderColor(borderY);
        resizedImage.u()[yc].clear();
        resizedImage.u()[yc].setBorderColor(borderU);
        resizedImage.v()[yc].clear();
        resizedImage.v()[yc].setBorderColor(borderV);
        ditherErrorImage.y()[yc].clear();
        ditherErrorImage.u()[yc].clear();
        ditherErrorImage.v()[yc].clear();
      }
      imgConv.setImageSize(608, nLines * 2);
      imgConv.setPixelAspectRatio(1.0f);
      imgConv.setPixelStoreCallback(&pixelStoreCallback, (void *) this);
      imgConv.convertImageFile(infileName);
      // initialize horizontal scroll table
      for (int i = 0; i < (nLines << 1); i++) {
        int     xShift_ = (!(i & 1) ? xShift0 : xShift1);
        if (xShift_ == -2)
          xShift_ = int(std::rand() & 0x7000) >> 12;
        else if (xShift_ == -1)
          xShift_ = 0;
        xShiftTable[i] = (xShift_ & 6) | (i & 1);
      }
      // convert input image to 121 colors with dithering
      progressMessage("Calculating FLI data");
      for (int yc = 0; yc < nLines; yc++) {
        if (!setProgressPercentage(yc * 20 / nLines)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        ditherLine(yc);
      }
      // generate FLI data
      for (int yc = 0; yc < nLines; yc += 2) {
        // field 0 (x = 0, 2, 4, ...)
        if (!setProgressPercentage((yc * 40 / nLines) + 20)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        int     bestXShift0 = xShiftTable[(yc << 1) + 0];
        int     bestXShift1 = xShiftTable[(yc << 1) + 2];
        if (xShift0 == -1) {
          // find optimal horizontal shifts
          double  minErr = 1000000.0;
          for (int xs0 = 0; xs0 < 8; xs0 += 2) {
            for (int xs1 = 0; xs1 < 8; xs1 += 2) {
              xShiftTable[(yc << 1) + 0] = xs0;
              xShiftTable[(yc << 1) + 2] = xs1;
              double  err = convertTwoLines(prgData, yc, false);
              if (err < minErr) {
                bestXShift0 = xs0;
                bestXShift1 = xs1;
                minErr = err;
              }
            }
          }
        }
        xShiftTable[(yc << 1) + 0] = bestXShift0;
        xShiftTable[(yc << 1) + 2] = bestXShift1;
        convertTwoLines(prgData, yc, false);
      }
      for (int yc = 0; yc < nLines; yc += 2) {
        // field 1 (x = 1, 3, 5, ...)
        if (!setProgressPercentage((yc * 40 / nLines) + 60)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        int     bestXShift0 = xShiftTable[(yc << 1) + 1];
        int     bestXShift1 = xShiftTable[(yc << 1) + 3];
        if (xShift1 == -1) {
          // find optimal horizontal shifts
          double  minErr = 1000000.0;
          for (int xs0 = 1; xs0 < 8; xs0 += 2) {
            for (int xs1 = 1; xs1 < 8; xs1 += 2) {
              xShiftTable[(yc << 1) + 1] = xs0;
              xShiftTable[(yc << 1) + 3] = xs1;
              double  err = convertTwoLines(prgData, yc, true);
              if (err < minErr) {
                bestXShift0 = xs0;
                bestXShift1 = xs1;
                minErr = err;
              }
            }
          }
        }
        xShiftTable[(yc << 1) + 1] = bestXShift0;
        xShiftTable[(yc << 1) + 3] = bestXShift1;
        convertTwoLines(prgData, yc, true);
      }
      setProgressPercentage(100);
      progressMessage("");
      // write PRG output
      for (int yc = 0; yc < nLines; yc++) {
        int     xs0 = (xShiftTable[(yc << 1) + 0] & 0x06) | 0x10;
        int     xs1 = (xShiftTable[(yc << 1) + 1] & 0x06) | 0x11;
        prgData.lineXShift((yc << 1) + 0) = (unsigned char) xs0;
        prgData.lineXShift((yc << 1) + 1) = (unsigned char) xs1;
      }
      prgData.convertImageData();
      prgEndAddr = (nLines <= 200 ? 0x9800U : 0xE500U);
    }
    catch (...) {
      prgData[0] = 0x01;
      prgData[1] = 0x10;
      prgData[2] = 0x00;
      prgData[3] = 0x00;
      prgEndAddr = 0x1003U;
      progressMessage("");
      throw;
    }
    return true;
  }

}       // namespace Plus4FLIConv

