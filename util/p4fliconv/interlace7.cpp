
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
#include "interlace7.hpp"

namespace Plus4FLIConv {

  P4FLI_Interlace7::Line320::Line320()
    : xShift(0),
      multiColorFlag(false)
  {
    buf = new float[352];
    for (size_t i = 0; i < 352; i++)
      buf[i] = 0.0f;
  }

  P4FLI_Interlace7::Line320::Line320(const Line320& r)
    : xShift(r.xShift),
      multiColorFlag(r.multiColorFlag)
  {
    buf = new float[352];
    for (size_t i = 0; i < 352; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_Interlace7::Line320::~Line320()
  {
    delete[] buf;
  }

  P4FLI_Interlace7::Line320&
      P4FLI_Interlace7::Line320::operator=(const Line320& r)
  {
    xShift = r.xShift;
    multiColorFlag = r.multiColorFlag;
    for (size_t i = 0; i < 352; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  void P4FLI_Interlace7::Line320::clear()
  {
    for (size_t i = 16; i < 336; i++)
      buf[i] = 0.0f;
  }

  void P4FLI_Interlace7::Line320::setBorderColor(float c)
  {
    for (size_t i = 0; i < 16; i++) {
      buf[i] = c;
      buf[i + 336] = c;
    }
  }

  // --------------------------------------------------------------------------

  P4FLI_Interlace7::Image320x496::Image320x496()
  {
    buf = new Line320[496];
  }

  P4FLI_Interlace7::Image320x496::Image320x496(const Image320x496& r)
  {
    buf = new Line320[496];
    for (size_t i = 0; i < 496; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_Interlace7::Image320x496&
      P4FLI_Interlace7::Image320x496::operator=(const Image320x496& r)
  {
    for (size_t i = 0; i < 496; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  P4FLI_Interlace7::Image320x496::~Image320x496()
  {
    delete[] buf;
  }

  // --------------------------------------------------------------------------

  P4FLI_Interlace7::YUVImage320x496::YUVImage320x496()
  {
  }

  P4FLI_Interlace7::YUVImage320x496::YUVImage320x496(const YUVImage320x496& r)
    : imageY(r.imageY),
      imageU(r.imageU),
      imageV(r.imageV)
  {
  }

  P4FLI_Interlace7::YUVImage320x496::~YUVImage320x496()
  {
  }

  P4FLI_Interlace7::YUVImage320x496&
      P4FLI_Interlace7::YUVImage320x496::operator=(const YUVImage320x496& r)
  {
    imageY = r.imageY;
    imageU = r.imageU;
    imageV = r.imageV;
    return (*this);
  }

  // --------------------------------------------------------------------------

  P4FLI_Interlace7::P4FLI_Interlace7()
    : monitorGamma(1.33),
      ditherLimit(0.125),
      ditherScale(0.75),
      ditherMode(0),
      luminanceSearchMode(2),
      luminanceSearchModeParam(4.0),
      xShift0(-1),
      xShift1(-1),
      borderColor(0x00),
      nLines(464),
      colorInterlaceMode(1),
      disablePAL(false),
      disableInterlace(false),
      luminance1BitMode(false),
      noLuminanceInterlace(false)
  {
    createYTable();
    createUVTables();
  }

  P4FLI_Interlace7::~P4FLI_Interlace7()
  {
  }

  void P4FLI_Interlace7::pixelStoreCallback(void *userData, int xc, int yc,
                                            float y, float u, float v)
  {
    P4FLI_Interlace7&  this_ =
        *(reinterpret_cast<P4FLI_Interlace7 *>(userData));
    float   c = float(std::sqrt(double(u * u) + double(v * v)));
    if (c > FLIConverter::defaultColorSaturation) {
      u = u * FLIConverter::defaultColorSaturation / c;
      v = v * FLIConverter::defaultColorSaturation / c;
    }
    this_.resizedImage.y()[yc][xc >> 1] += (y * 0.5f);
    this_.resizedImage.u()[yc][xc >> 1] += (u * 0.5f);
    this_.resizedImage.v()[yc][xc >> 1] += (v * 0.5f);
  }

  void P4FLI_Interlace7::colorToUV(int c, float& u, float& v)
  {
    float   y = 0.0f;
    FLIConverter::convertPlus4Color(c, y, u, v, monitorGamma);
  }

  void P4FLI_Interlace7::createYTable()
  {
    for (int i = 0; i < 9; i++) {
      float   u = 0.0f;
      float   v = 0.0f;
      FLIConverter::convertPlus4Color((i == 0 ? 0 : (((i - 1) << 4) + 1)),
                                      yTable[i], u, v, monitorGamma);
    }
  }

  void P4FLI_Interlace7::createUVTables()
  {
    static const unsigned char  colorIndexTable[86] = {
       1,  1,   2,  2,  12, 12,   7,  7,   6,  6,  15, 15,   4,  4,   8,  8,
       3,  3,  10, 10,  14, 14,   9,  9,  13, 13,   5,  5,  11, 11,   2,  8,
       3, 12,  10,  7,   6, 14,   5, 15,   4, 11,   9,  8,   3, 13,  10, 15,
       4, 14,   9,  7,   6, 13,   5, 12,   2, 11,   1,  2,  12,  1,   7,  1,
       1,  6,  15,  1,   1,  4,   8,  1,   1,  3,   1, 10,  14,  1,   1,  9,
      13,  1,   1,  5,  11,  1
    };
    for (int i = 0; i < 43; i++) {
      int     c0 = colorIndexTable[i << 1];
      int     c1 = colorIndexTable[(i << 1) + 1];
      uvTable[i].c0 = c0;
      uvTable[i].c1 = c1;
      float   u0 = 0.0f;
      float   v0 = 0.0f;
      float   u1 = 0.0f;
      float   v1 = 0.0f;
      colorToUV(c0, u0, v0);
      colorToUV(c1, u1, v1);
      uvTable[i].u = (u0 + u1) * 0.5f;
      uvTable[i].v = (v0 + v1) * 0.5f;
      uvTable[i].err = (calculateErrorSqr(u0, u1) + calculateErrorSqr(v0, v1))
                       / 20.0;
    }
  }

  void P4FLI_Interlace7::checkParameters()
  {
    limitValue(monitorGamma, 0.25, 4.0);
    limitValue(ditherLimit, 0.0, 2.0);
    limitValue(ditherScale, 0.0, 1.0);
    limitValue(ditherMode, 0, 3);
    limitValue(luminanceSearchMode, 0, 5);
    switch (luminanceSearchMode) {
    case 2:
      limitValue(luminanceSearchModeParam, 1.0, 16.0);
      break;
    case 4:
      limitValue(luminanceSearchModeParam, 0.0, 0.5);
      break;
    case 5:
      limitValue(luminanceSearchModeParam, 0.0, 0.25);
      break;
    default:
      limitValue(luminanceSearchModeParam, 0.0, 1.0);
      break;
    }
    limitValue(xShift0, -2, 7);
    limitValue(xShift1, -2, 7);
    borderColor = (borderColor & 0x7F) | 0x80;
    nLines = (nLines <= 456 ? (nLines <= 400 ? 400 : 456)
                              : (nLines <= 464 ? 464 : 496));
    limitValue(colorInterlaceMode, 0, 2);
  }

  void P4FLI_Interlace7::ditherPixel(PRGData& prgData, long xc, long yc)
  {
    if (xc < 0L || xc >= 320L || yc < 0L || yc >= 496L)
      return;
    long    xcShifted = xc - resizedImage.y()[yc].getXShift();
    if (xcShifted < 0L)
      return;
    int     l0 = prgData.l0(xcShifted, yc);
    int     l1 = prgData.l1(xcShifted, yc);
    float   pixelValueOriginal = resizedImage.y()[yc].getPixel(xc);
    float   ditherError = ditherErrorImage[yc].getPixel(xc);
    float   pixelValueDithered = pixelValueOriginal + ditherError;
    float   pixelValue0 = yTable[l0];
    float   pixelValue1 = yTable[l1];
    bool    bitValue = false;
    if (ditherMode == 0 && pixelValue1 > pixelValue0) {
      // ordered dithering
      float   tmp = pixelValueOriginal;
      if (tmp < pixelValue0)
        tmp = pixelValue0;
      if (tmp > pixelValue1)
        tmp = pixelValue1;
      tmp = (tmp - pixelValue0) / (pixelValue1 - pixelValue0);
      if (ditherPixelValue(xc, yc, tmp))
        pixelValueDithered = pixelValue1;
      else
        pixelValueDithered = pixelValue0;
    }
    bitValue = (calculateError(pixelValue1, pixelValueDithered)
                < calculateError(pixelValue0, pixelValueDithered));
    // save quantized pixel value for error calculation
    float   newPixelValue = (bitValue ? pixelValue1 : pixelValue0);
    if (calculateError(calculateError(pixelValue1, pixelValueOriginal),
                       calculateError(pixelValue0, pixelValueOriginal))
        >= ditherLimit) {
      bitValue = (calculateError(pixelValue1, pixelValueOriginal)
                  < calculateError(pixelValue0, pixelValueOriginal));
    }
    prgData.setPixel(xcShifted, yc, bitValue);
    if (ditherMode == 0)
      return;
    // diffuse error
    pixelValueDithered = pixelValueOriginal
                         + ((pixelValueDithered - pixelValueOriginal)
                            * float(ditherScale));
    if (pixelValueDithered < pixelValue0)
      pixelValueDithered = pixelValue0;
    if (pixelValueDithered > pixelValue1)
      pixelValueDithered = pixelValue1;
    double  err = double(pixelValueDithered) - double(newPixelValue);
    if (ditherMode == 1) {
      // Floyd-Steinberg dithering
      static const int    xOffsTbl[4] = { 1, -1, 0, 1 };
      static const int    yOffsTbl[4] = { 0, 1, 1, 1 };
      static const float  errMultTbl[4] = {
        0.4375f, 0.1875f, 0.3125f, 0.0625f
      };
      for (int i = 0; i < 4; i++) {
        long    yc_ = yc + yOffsTbl[i];
        if (yc_ >= 496L)
          break;
        long    xc_ = xOffsTbl[i];
        xc_ = ((yc & 1L) == 0L ? (xc + xc_) : (xc - xc_));
        ditherErrorImage[yc_].setPixel(xc_,
                                       ditherErrorImage[yc_].getPixel(xc_)
                                       + (float(err) * errMultTbl[i]));
      }
    }
    else if (ditherMode == 2) {
      // Jarvis dithering
      for (int i = 0; i < 3; i++) {
        long    yc_ = yc + i;
        if (yc_ >= 496L)
          break;
        for (int j = (i == 0 ? 1 : -2); j < 3; j++) {
          long    xc_ = j;
          if (yc & 1L)
            xc_ = -xc_;
          xc_ += xc;
          int     tmp = 9 - ((i + (j >= 0 ? j : (-j))) << 1);
          ditherErrorImage[yc_].setPixel(xc_,
                                         ditherErrorImage[yc_].getPixel(xc_)
                                         + float(err * (double(tmp) / 48.0)));
        }
      }
    }
    else {
      // Stucki dithering
      for (int i = 0; i < 3; i++) {
        long    yc_ = yc + i;
        if (yc_ >= 496L)
          break;
        for (int j = (i == 0 ? 1 : -2); j < 3; j++) {
          long    xc_ = j;
          if (yc & 1L)
            xc_ = -xc_;
          xc_ += xc;
          int     tmp = 16 >> (i + (j >= 0 ? j : (-j)));
          ditherErrorImage[yc_].setPixel(xc_,
                                         ditherErrorImage[yc_].getPixel(xc_)
                                         + float(err * (double(tmp) / 42.0)));
        }
      }
    }
  }

  inline double P4FLI_Interlace7::calculateLuminanceError(float n,
                                                          int l0, int l1)
  {
    float   l_0 = yTable[l0];
    float   l_1 = yTable[l1];
    double  err0 = calculateErrorSqr(l_0, n);
    double  err1 = calculateErrorSqr(l_1, n);
    double  err = (err0 < err1 ? err0 : err1);
    if (luminanceSearchMode == 2 && (n < l_0 || n > l_1))
      err *= luminanceSearchModeParam;
    if (luminanceSearchMode == 4) {
      double  tmp = 0.5 + luminanceSearchModeParam;
      double  l_0_ = (double(l_0) * tmp) + (double(l_1) * (1.0 - tmp));
      double  l_1_ = (double(l_0) * (1.0 - tmp)) + (double(l_1) * tmp);
      double  err0_ = calculateErrorSqr(l_0_, n);
      double  err1_ = calculateErrorSqr(l_1_, n);
      double  err_ = (err0_ > err1_ ? err0_ : err1_);
      err = (err_ < err ? err_ : err);
    }
    return err;
  }

  double P4FLI_Interlace7::findLuminanceCodes(PRGData& prgData,
                                              long xc, long yc)
  {
    int     l0 = 0;
    int     l1 = 8;
    xc = xc & (~(long(7)));
    yc = yc & (~(long(noLuminanceInterlace ? 3 : 2)));
    float   tmpBuf[32];
    int     nPixels = (noLuminanceInterlace ? 32 : 16);
    for (int i = 0; i < nPixels; i++) {
      long    x_ = xc | long(i & 7);
      long    y_ = yc | long((i & 8) >> 2) | long(i >> 4);
      tmpBuf[i] = resizedImage.y()[y_].getPixelShifted(x_);
    }
    if (!luminance1BitMode) {
      // find the best pair of luminance values, depending on the search mode
      int     l0min = 8;
      int     l1max = 0;
      if (luminanceSearchMode >= 2 && luminanceSearchMode <= 4) {
        for (int i = 0; i < nPixels; i++) {
          while (l0min > 0 && tmpBuf[i] < yTable[l0min])
            l0min--;
          while (l1max < 8 && tmpBuf[i] > yTable[l1max])
            l1max++;
        }
        if (l0min == l1max) {
          if (l1max < 8)
            l1max++;
          else
            l0min--;
        }
      }
      else {
        float   minVal = 1.0f;
        float   maxVal = 0.0f;
        for (int i = 0; i < nPixels; i++) {
          if (tmpBuf[i] < minVal)
            minVal = tmpBuf[i];
          if (tmpBuf[i] > maxVal)
            maxVal = tmpBuf[i];
        }
        double  minErr0 = 1000000.0;
        double  minErr1 = 1000000.0;
        for (int i = 0; i < 9; i++) {
          double  err = calculateError(yTable[i], minVal);
          if (err < minErr0) {
            l0min = i;
            minErr0 = err;
          }
          err = calculateError(yTable[i], maxVal);
          if (err < minErr1) {
            l1max = i;
            minErr1 = err;
          }
        }
        if (l0min == l1max) {
          if (minErr0 < minErr1) {
            if (l1max < 8)
              l1max++;
            else
              l0min--;
          }
          else {
            if (l0min > 0)
              l0min--;
            else
              l1max++;
          }
        }
      }
      if (luminanceSearchMode == 1 || luminanceSearchMode == 3) {
        l0 = l0min;
        l1 = l1max;
      }
      else {
        double  minErr = 1000000.0f;
        for (int l0tmp = l0min; l0tmp < l1max; l0tmp++) {
          for (int l1tmp = l0tmp + 1; l1tmp <= l1max; l1tmp++) {
            float   minVal = yTable[l0tmp];
            float   maxVal = yTable[l1tmp];
            double  err = 0.0;
            for (int i = 0; i < nPixels; i++) {
              err += calculateLuminanceError(tmpBuf[i], l0tmp, l1tmp);
              err += (calculateErrorSqr(tmpBuf[i], (minVal + maxVal) * 0.5f)
                      * 0.00001);
            }
            if (err < minErr) {
              minErr = err;
              l0 = l0tmp;
              l1 = l1tmp;
            }
          }
        }
        if (luminanceSearchMode == 5) {
          bool    l0DecFlag;
          bool    l1IncFlag;
          do {
            l0DecFlag = false;
            l1IncFlag = false;
            if (l1 < l1max) {
              l1++;
              double  err = 0.0;
              for (int i = 0; i < nPixels; i++)
                err += calculateLuminanceError(tmpBuf[i], l0, l1);
              if ((err - minErr) < (double(nPixels) * luminanceSearchModeParam))
                l1IncFlag = true;
              else
                l1--;
            }
            if (l0 > l0min) {
              l0--;
              double  err = 0.0;
              for (int i = 0; i < nPixels; i++)
                err += calculateLuminanceError(tmpBuf[i], l0, l1);
              if ((err - minErr) < (double(nPixels) * luminanceSearchModeParam))
                l0DecFlag = true;
              else
                l0++;
            }
          } while (l0DecFlag || l1IncFlag);
        }
      }
    }
    // store luminance codes
    prgData.l0(xc, yc) = l0;
    prgData.l1(xc, yc) = l1;
    if (noLuminanceInterlace) {
      prgData.l0(xc, yc + 1L) = l0;
      prgData.l1(xc, yc + 1L) = l1;
    }
    // return the total amount of error (used when optimizing horizontal shifts)
    double  err = 0.0;
    for (int i = 0; i < nPixels; i++) {
      long    y_ = yc | long((i & 8) >> 2) | long(i >> 4);
      long    x_ = (xc | long(i & 7)) + resizedImage.y()[y_].getXShift();
      if (x_ >= 8L && x_ < 312L)  // ignore pixels that are not visible
        err += calculateLuminanceError(tmpBuf[i], l0, l1);
    }
    return err;
  }

  void P4FLI_Interlace7::generateBitmaps(PRGData& prgData)
  {
    for (int yc = 0; yc < 496; yc += 2) {
      for (int xc = 0; xc < 320; xc++)
        ditherPixel(prgData, xc, yc);
      for (int xc = 319; xc >= 0; xc--)
        ditherPixel(prgData, xc, yc + 1);
    }
    for (int yc = 0; yc < 496; yc++) {
      if (yc & 2)
        continue;
      for (int xc = 0; xc < 320; xc += 8) {
        // if all bits are 0 or 1, replace the bitmaps with
        // a dither pattern to improve color conversion
        int     b0 = 0;
        int     b1 = 1;
        for (int i = 0; i < 16; i++) {
          bool    tmp = prgData.getPixel(xc | (i & 7), yc | ((i & 8) >> 2));
          b0 = b0 | int(tmp);
          b1 = b1 & int(tmp);
        }
        int&    l0 = prgData.l0(xc, yc);
        int&    l1 = prgData.l1(xc, yc);
        if ((b0 == 0 && l0 != 0) || (b1 == 1 && l1 != 0)) {
          if (b0 == 0)
            l1 = l0;
          else
            l0 = l1;
          for (int i = 0; i < 16; i++) {
            int     tmpY = yc | ((i & 8) >> 2);
            int     tmpX = (xc | (i & 7)) + resizedImage.y()[tmpY].getXShift();
            prgData.setPixel(xc | (i & 7), tmpY,
                             ditherPixelValue(tmpX, tmpY, 0.66667f));
          }
        }
      }
    }
  }

  void P4FLI_Interlace7::findColorCodes(PRGData& prgData,
                                        long xc, long yc, int dir_)
  {
    bool    oddField = bool(yc & 1L);
    float   savedU0[9];
    float   savedV0[9];
    float   savedU1[9];
    float   savedV1[9];
    for (int i = 0; i < 9; i++) {
      savedU0[i] = line0U.getPixelShifted(xc + (i * dir_));
      savedV0[i] = line0V.getPixelShifted(xc + (i * dir_));
      savedU1[i] = line1U.getPixelShifted(xc + (i * dir_));
      savedV1[i] = line1V.getPixelShifted(xc + (i * dir_));
    }
    // find the pair of colors that gives the least amount of error
    int     c0 = 0;
    int     c1 = 0;
    int     l0 = prgData.l0(xc, yc);
    int     l1 = prgData.l1(xc, yc);
    double  minColorErr = 1000000.0;
    int     colorCnt =
        (colorInterlaceMode == 0 ? 15 : (colorInterlaceMode == 2 ? 43 : 29));
    for (int i0 = 0; i0 < colorCnt; i0++) {
      for (int i1 = 0; i1 < colorCnt; i1++) {
        int     c0tmp = 0;
        int     c1tmp = 0;
        double  err = 0.0;
        {
          float   u0 = 0.0f;
          float   v0 = 0.0f;
          float   u1 = 0.0f;
          float   v1 = 0.0f;
          double  err0 = 0.0;
          double  err1 = 0.0;
          if (l0 > 0) {
            c0tmp = (oddField ? uvTable[i0].c1 : uvTable[i0].c0);
            u0 = uvTable[i0].u;
            v0 = uvTable[i0].v;
            err0 = uvTable[i0].err;
          }
          if (l1 > 0) {
            c1tmp = (oddField ? uvTable[i1].c1 : uvTable[i1].c0);
            u1 = uvTable[i1].u;
            v1 = uvTable[i1].v;
            err1 = uvTable[i1].err;
          }
          for (int x = 0; x < 9; x++) {
            bool    b = prgData.getPixel(xc + ((x <= 7 ? x : 7) * dir_), yc);
            float   u_ = (b ? u1 : u0);
            float   v_ = (b ? v1 : v0);
            if (x < 8)
              err += (b ? err1 : err0);
            line0U.setPixelShifted(xc + (x * dir_), u_);
            line0V.setPixelShifted(xc + (x * dir_), v_);
            b = prgData.getPixel(xc + ((x <= 7 ? x : 7) * dir_), yc + 2L);
            u_ = (b ? u1 : u0);
            v_ = (b ? v1 : v0);
            if (x < 8)
              err += (b ? err1 : err0);
            line1U.setPixelShifted(xc + (x * dir_), u_);
            line1V.setPixelShifted(xc + (x * dir_), v_);
          }
        }
        for (int j = 0; j < 16; j++) {
          int     x = j & 7;
          Line320 *l0U = (j < 8 ? (&line0U) : (&line1U));
          Line320 *l0V = (j < 8 ? (&line0V) : (&line1V));
          float   u_ = l0U->getPixelShifted(xc + (x * dir_));
          float   v_ = l0V->getPixelShifted(xc + (x * dir_));
          if (!disablePAL) {
            // assume PAL filtering if requested
            Line320 *lm1U = (j < 8 ? (&prvLineU) : (&line0U));
            Line320 *lm1V = (j < 8 ? (&prvLineV) : (&line0V));
            u_ += lm1U->getPixelShifted(xc + (x * dir_));
            v_ += lm1V->getPixelShifted(xc + (x * dir_));
            u_ *= 0.96f;
            v_ *= 0.96f;
            u_ += (l0U->getPixelShifted(xc + ((x - 1) * dir_)) * 0.52f);
            v_ += (l0V->getPixelShifted(xc + ((x - 1) * dir_)) * 0.52f);
            u_ += (l0U->getPixelShifted(xc + ((x + 1) * dir_)) * 0.52f);
            v_ += (l0V->getPixelShifted(xc + ((x + 1) * dir_)) * 0.52f);
            u_ += (lm1U->getPixelShifted(xc + ((x - 1) * dir_)) * 0.52f);
            v_ += (lm1V->getPixelShifted(xc + ((x - 1) * dir_)) * 0.52f);
            u_ += (lm1U->getPixelShifted(xc + ((x + 1) * dir_)) * 0.52f);
            v_ += (lm1V->getPixelShifted(xc + ((x + 1) * dir_)) * 0.52f);
            u_ *= 0.25f;
            v_ *= 0.25f;
          }
          double  errU =
              double(u_)
              - double(resizedImage.u()[yc + ((j & 8) >> 2)].getPixelShifted(
                           xc + (x * dir_)));
          double  errV =
              double(v_)
              - double(resizedImage.v()[yc + ((j & 8) >> 2)].getPixelShifted(
                           xc + (x * dir_)));
          err = err + (errU * errU) + (errV * errV);
          if (err > (minColorErr * 1.000001))
            break;
        }
        if (err < minColorErr) {
          c0 = c0tmp;
          c1 = c1tmp;
          minColorErr = err;
        }
        else {
          for (int i = 0; i < 9; i++) {
            line0U.setPixelShifted(xc + (i * dir_), savedU0[i]);
            line0V.setPixelShifted(xc + (i * dir_), savedV0[i]);
            line1U.setPixelShifted(xc + (i * dir_), savedU1[i]);
            line1V.setPixelShifted(xc + (i * dir_), savedV1[i]);
          }
        }
      }
    }
    // store color codes
    prgData.c0(xc, yc) = c0;
    prgData.c1(xc, yc) = c1;
  }

  bool P4FLI_Interlace7::processImage(PRGData& prgData,
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
      luminanceSearchMode = config["luminanceSearchMode"];
      luminanceSearchModeParam = config["luminanceSearchModeParam"];
      xShift0 = config["xShift0"];
      xShift1 = config["xShift1"];
      borderColor = config["borderColor"];
      nLines = config["verticalSize"];
      colorInterlaceMode = config["colorInterlaceMode"];
      disablePAL = !(bool(config["enablePAL"]));
      disableInterlace = (nLines < 400);
      if (disableInterlace)
        nLines = nLines << 1;
      luminance1BitMode = config["luminance1BitMode"];
      noLuminanceInterlace = config["noLuminanceInterlace"];
      checkParameters();
      createYTable();
      createUVTables();
      float   borderY = 0.0f;
      float   borderU = 0.0f;
      float   borderV = 0.0f;
      FLIConverter::convertPlus4Color(borderColor, borderY, borderU, borderV,
                                      monitorGamma);
      prgData.clear();
      prgData.borderColor() = (unsigned char) borderColor;
      prgData.setVerticalSize(nLines);
      prgData.interlaceDisabled() = (unsigned char) disableInterlace;
      for (int yc = 0; yc < 496; yc++) {
        resizedImage.y()[yc].clear();
        resizedImage.y()[yc].setBorderColor(borderY);
        resizedImage.u()[yc].clear();
        resizedImage.u()[yc].setBorderColor(borderU);
        resizedImage.v()[yc].clear();
        resizedImage.v()[yc].setBorderColor(borderV);
        ditherErrorImage[yc].clear();
      }
      prvLineU.setBorderColor(borderU);
      prvLineV.setBorderColor(borderV);
      line0U.setBorderColor(borderU);
      line0V.setBorderColor(borderV);
      line1U.setBorderColor(borderU);
      line1V.setBorderColor(borderV);
      imgConv.setImageSize(640, nLines);
      imgConv.setPixelAspectRatio(1.0f);
      imgConv.setPixelStoreCallback(&pixelStoreCallback, (void *) this);
      imgConv.convertImageFile(infileName);
      progressMessage("Calculating FLI data");
      for (int yc = 0; yc < 496; yc += 2) {
        for (int xc = 0; xc < 320; xc++) {
          if (disableInterlace) {
            float   y0 = resizedImage.y()[yc].getPixel(xc);
            float   y1 = resizedImage.y()[yc + 1].getPixel(xc);
            resizedImage.y()[yc].setPixel(xc, (y0 + y1) * 0.5f);
            resizedImage.y()[yc + 1].setPixel(xc, (y0 + y1) * 0.5f);
          }
          float   u0 = resizedImage.u()[yc].getPixel(xc);
          float   u1 = resizedImage.u()[yc + 1].getPixel(xc);
          float   v0 = resizedImage.v()[yc].getPixel(xc);
          float   v1 = resizedImage.v()[yc + 1].getPixel(xc);
          resizedImage.u()[yc].setPixel(xc, (u0 + u1) * 0.5f);
          resizedImage.u()[yc + 1].setPixel(xc, (u0 + u1) * 0.5f);
          resizedImage.v()[yc].setPixel(xc, (v0 + v1) * 0.5f);
          resizedImage.v()[yc + 1].setPixel(xc, (v0 + v1) * 0.5f);
        }
      }
      for (int yc = 0; yc < 496; yc++) {
        int     xShift_ = (!(yc & 1) ? xShift0 : xShift1);
        if (xShift_ == -2)
          xShift_ = int(std::rand() & 0x7000) >> 12;
        else if (xShift_ == -1)
          xShift_ = 0;
        resizedImage.y()[yc].setXShift(xShift_);
        resizedImage.u()[yc].setXShift(xShift_);
        resizedImage.v()[yc].setXShift(xShift_);
      }
      if (!(xShift0 == 0 && xShift1 == 0)) {
        for (int yc = 0; yc < 496; yc++) {
          for (int i = 0; i < 8; i++) {
            resizedImage.y()[yc][i] = resizedImage.y()[yc][15 - i];
            resizedImage.y()[yc][312 + i] = resizedImage.y()[yc][311 - i];
            resizedImage.u()[yc][i] = resizedImage.u()[yc][15 - i];
            resizedImage.u()[yc][312 + i] = resizedImage.u()[yc][311 - i];
            resizedImage.v()[yc][i] = resizedImage.v()[yc][15 - i];
            resizedImage.v()[yc][312 + i] = resizedImage.v()[yc][311 - i];
          }
        }
      }
      for (int yc = 0; yc < nLines; yc++) {
        if ((yc & (noLuminanceInterlace ? 3 : 2)) != 0)
          continue;
        if (!setProgressPercentage(yc * 33 / nLines)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        if (!(xShift0 == -1 || xShift1 == -1)) {
          for (int xc = 0; xc < 320; xc += 8)
            findLuminanceCodes(prgData, xc, yc);
        }
        else if (!(yc & 3)) {
          // find optimal horizontal shifts
          double  minErr = 1000000.0;
          int     bestXShift[4];
          int     xs[4];
          for (int i = 0; i < 4; i++) {
            bestXShift[i] = 0;
            xs[i] = 0;
          }
          do {
            for (int i = 0; i < 4; i++) {
              xs[i] = xs[i] & 7;
              resizedImage.y()[yc + i].setXShift(xs[i]);
              resizedImage.u()[yc + i].setXShift(xs[i]);
              resizedImage.v()[yc + i].setXShift(xs[i]);
            }
            bool    skipFlag = false;
            for (int i = 0; i < 4; i++) {
              // do not allow stepping by more than one pixel at once
              if ((yc + i) > 0) {
                int     d = resizedImage.y()[yc + i].getXShift()
                            - resizedImage.y()[yc + i - 1].getXShift();
                if (!(d == 0 || d == 1 || d == -1 || d == 7 || d == -7)) {
                  skipFlag = true;
                  break;
                }
              }
            }
            if (!skipFlag) {
              // calculate the total error for four lines
              double  err = 0.0;
              for (int xc = 0; xc < 320; xc += 8)
                err += findLuminanceCodes(prgData, xc, yc);
              if (!noLuminanceInterlace) {
                for (int xc = 0; xc < 320; xc += 8)
                  err += findLuminanceCodes(prgData, xc, yc + 1);
              }
              if (err < minErr) {
                for (int i = 0; i < 4; i++)
                  bestXShift[i] = xs[i];
                minErr = err;
              }
            }
            for (int i = 0; i < 4; i++) {
              xs[i] = xs[i] + 1;
              if (xs[i] < 8)
                break;
            }
          } while (xs[3] < 8);
          // use the best horizontal shift that was found
          for (int i = 0; i < 4; i++) {
            resizedImage.y()[yc + i].setXShift(bestXShift[i]);
            resizedImage.u()[yc + i].setXShift(bestXShift[i]);
            resizedImage.v()[yc + i].setXShift(bestXShift[i]);
          }
          for (int xc = 0; xc < 320; xc += 8)
            findLuminanceCodes(prgData, xc, yc);
          if (!noLuminanceInterlace) {
            for (int xc = 0; xc < 320; xc += 8)
              findLuminanceCodes(prgData, xc, yc + 1);
          }
        }
      }
      generateBitmaps(prgData);
      // convert color information
      {
        Line320 savedPrvLineU;
        Line320 savedPrvLineV;
        savedPrvLineU.setBorderColor(borderU);
        savedPrvLineV.setBorderColor(borderV);
        for (int xc = 0; xc < 320; xc++) {
          prvLineU[xc] = borderU;
          prvLineV[xc] = borderV;
          savedPrvLineU[xc] = borderU;
          savedPrvLineV[xc] = borderV;
        }
        for (int yc = 0; yc < nLines; yc++) {
          if (yc & 2)
            continue;
          if (!setProgressPercentage((yc * 67 / nLines) + 33)) {
            prgData[0] = 0x01;
            prgData[1] = 0x10;
            prgData[2] = 0x00;
            prgData[3] = 0x00;
            prgEndAddr = 0x1003U;
            progressMessage("");
            return false;
          }
          line0U.clear();
          line0V.clear();
          line1U.clear();
          line1V.clear();
          line0U.setXShift(resizedImage.u()[yc].getXShift());
          line0V.setXShift(resizedImage.v()[yc].getXShift());
          line1U.setXShift(resizedImage.u()[yc + 2].getXShift());
          line1V.setXShift(resizedImage.v()[yc + 2].getXShift());
          if (resizedImage.y()[yc].getXShift()
              >= resizedImage.y()[yc + 2].getXShift()) {
            for (int xc = 0; xc < 320; xc += 8)
              findColorCodes(prgData, xc, yc, 1);
          }
          else {
            for (int xc = 319; xc >= 0; xc -= 8)
              findColorCodes(prgData, xc, yc, -1);
          }
          for (int xc = 0; xc < 320; xc++) {
            prvLineU[xc] = (line1U[xc] * 0.5f) + (savedPrvLineU[xc] * 0.5f);
            prvLineV[xc] = (line1V[xc] * 0.5f) + (savedPrvLineV[xc] * 0.5f);
          }
          savedPrvLineU = line1U;
          savedPrvLineV = line1V;
        }
      }
      setProgressPercentage(100);
      progressMessage("");
      for (int yc = 0; yc < 496; yc++) {
        prgData.lineXShift(yc) =
            (unsigned char) resizedImage.y()[yc].getXShift();
      }
      prgData.convertImageData();
      prgEndAddr = (nLines <= 400 ? 0x9800U : 0xE500U);
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

