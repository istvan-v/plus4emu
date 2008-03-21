
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
#include "hiresnofli.hpp"
#include "mcnofli.hpp"

namespace Plus4FLIConv {

  P4FLI_HiResNoFLI::Line320::Line320()
  {
    buf = new float[352];
    for (size_t i = 0; i < 352; i++)
      buf[i] = 0.0f;
  }

  P4FLI_HiResNoFLI::Line320::Line320(const Line320& r)
  {
    buf = new float[352];
    for (size_t i = 0; i < 352; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_HiResNoFLI::Line320::~Line320()
  {
    delete[] buf;
  }

  P4FLI_HiResNoFLI::Line320&
      P4FLI_HiResNoFLI::Line320::operator=(const Line320& r)
  {
    for (size_t i = 0; i < 352; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  void P4FLI_HiResNoFLI::Line320::clear()
  {
    for (size_t i = 16; i < 336; i++)
      buf[i] = 0.0f;
  }

  void P4FLI_HiResNoFLI::Line320::setBorderColor(float c)
  {
    for (size_t i = 0; i < 16; i++) {
      buf[i] = c;
      buf[i + 336] = c;
    }
  }

  // --------------------------------------------------------------------------

  P4FLI_HiResNoFLI::Image320x200::Image320x200()
  {
    buf = new Line320[200];
  }

  P4FLI_HiResNoFLI::Image320x200::Image320x200(const Image320x200& r)
  {
    buf = new Line320[200];
    for (size_t i = 0; i < 200; i++)
      buf[i] = r.buf[i];
  }

  P4FLI_HiResNoFLI::Image320x200&
      P4FLI_HiResNoFLI::Image320x200::operator=(const Image320x200& r)
  {
    for (size_t i = 0; i < 200; i++)
      buf[i] = r.buf[i];
    return (*this);
  }

  P4FLI_HiResNoFLI::Image320x200::~Image320x200()
  {
    delete[] buf;
  }

  // --------------------------------------------------------------------------

  P4FLI_HiResNoFLI::YUVImage320x200::YUVImage320x200()
  {
  }

  P4FLI_HiResNoFLI::YUVImage320x200::YUVImage320x200(const YUVImage320x200& r)
    : imageY(r.imageY),
      imageU(r.imageU),
      imageV(r.imageV)
  {
  }

  P4FLI_HiResNoFLI::YUVImage320x200::~YUVImage320x200()
  {
  }

  P4FLI_HiResNoFLI::YUVImage320x200&
      P4FLI_HiResNoFLI::YUVImage320x200::operator=(const YUVImage320x200& r)
  {
    imageY = r.imageY;
    imageU = r.imageU;
    imageV = r.imageV;
    return (*this);
  }

  // --------------------------------------------------------------------------

  P4FLI_HiResNoFLI::P4FLI_HiResNoFLI()
    : monitorGamma(1.33),
      ditherLimit(0.125),
      ditherScale(0.9),
      ditherMode(1),
      luminanceSearchMode(2),
      luminanceSearchModeParam(4.0),
      borderColor(0x00),
      disablePAL(false),
      luminance1BitMode(false)
  {
    createYTable();
    createUVTables();
  }

  P4FLI_HiResNoFLI::~P4FLI_HiResNoFLI()
  {
  }

  void P4FLI_HiResNoFLI::pixelStoreCallback(void *userData, int xc, int yc,
                                            float y, float u, float v)
  {
    P4FLI_HiResNoFLI&  this_ =
        *(reinterpret_cast<P4FLI_HiResNoFLI *>(userData));
    float   c = float(std::sqrt(double(u * u) + double(v * v)));
    if (c > FLIConverter::defaultColorSaturation) {
      u = u * FLIConverter::defaultColorSaturation / c;
      v = v * FLIConverter::defaultColorSaturation / c;
    }
    this_.resizedImage.y()[yc >> 1][xc >> 1] += (y * 0.25f);
    this_.resizedImage.u()[yc >> 1][xc >> 1] += (u * 0.25f);
    this_.resizedImage.v()[yc >> 1][xc >> 1] += (v * 0.25f);
  }

  void P4FLI_HiResNoFLI::colorToUV(int c, float& u, float& v)
  {
    float   y = 0.0f;
    FLIConverter::convertPlus4Color(c, y, u, v, monitorGamma);
  }

  void P4FLI_HiResNoFLI::createYTable()
  {
    for (int i = 0; i < 9; i++) {
      float   u = 0.0f;
      float   v = 0.0f;
      FLIConverter::convertPlus4Color((i == 0 ? 0 : (((i - 1) << 4) + 1)),
                                      yTable[i], u, v, monitorGamma);
    }
  }

  void P4FLI_HiResNoFLI::createUVTables()
  {
    for (int i = 0; i < 15; i++) {
      uvTable[i].c = i + 1;
      colorToUV(uvTable[i].c, uvTable[i].u, uvTable[i].v);
    }
  }

  void P4FLI_HiResNoFLI::checkParameters()
  {
    limitValue(monitorGamma, 0.25, 4.0);
    limitValue(ditherLimit, 0.0, 2.0);
    limitValue(ditherScale, 0.0, 1.0);
    limitValue(ditherMode, 0, 4);
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
    borderColor = (borderColor & 0x7F) | 0x80;
  }

  void P4FLI_HiResNoFLI::ditherPixel(PRGData& prgData, long xc, long yc)
  {
    if (xc < 0L || xc >= 320L || yc < 0L || yc >= 200L)
      return;
    int     l0 = prgData.l0(xc, (yc & (~(long(7)))) << 1);
    int     l1 = prgData.l1(xc, (yc & (~(long(7)))) << 1);
    float   pixelValueOriginal = resizedImage.y()[yc].getPixel(xc);
    float   ditherError = ditherErrorImage[yc].getPixel(xc);
    float   pixelValueDithered = pixelValueOriginal + ditherError;
    float   pixelValue0 = yTable[l0];
    float   pixelValue1 = yTable[l1];
    bool    bitValue = false;
    if (ditherMode < 2 && pixelValue1 > pixelValue0) {
      // ordered dithering
      float   tmp = pixelValueOriginal;
      if (tmp < pixelValue0)
        tmp = pixelValue0;
      if (tmp > pixelValue1)
        tmp = pixelValue1;
      tmp = (tmp - pixelValue0) / (pixelValue1 - pixelValue0);
      if (ditherMode == 0) {
        if (ditherPixelValue_Bayer(xc, yc, tmp))
          pixelValueDithered = pixelValue1;
        else
          pixelValueDithered = pixelValue0;
      }
      else {
        if (ditherPixelValue(xc, yc, tmp))
          pixelValueDithered = pixelValue1;
        else
          pixelValueDithered = pixelValue0;
      }
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
    prgData.setPixel(xc, yc << 1, bitValue);
    if (ditherMode < 2)
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
    if (ditherMode == 2) {
      // Floyd-Steinberg dithering
      static const int    xOffsTbl[4] = { 1, -1, 0, 1 };
      static const int    yOffsTbl[4] = { 0, 1, 1, 1 };
      static const float  errMultTbl[4] = {
        0.4375f, 0.1875f, 0.3125f, 0.0625f
      };
      for (int i = 0; i < 4; i++) {
        long    yc_ = yc + yOffsTbl[i];
        if (yc_ >= 200L)
          break;
        long    xc_ = xOffsTbl[i];
        xc_ = ((yc & 1L) == 0L ? (xc + xc_) : (xc - xc_));
        ditherErrorImage[yc_].setPixel(xc_,
                                       ditherErrorImage[yc_].getPixel(xc_)
                                       + (float(err) * errMultTbl[i]));
      }
    }
    else if (ditherMode == 3) {
      // Jarvis dithering
      for (int i = 0; i < 3; i++) {
        long    yc_ = yc + i;
        if (yc_ >= 200L)
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
        if (yc_ >= 200L)
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

  inline double P4FLI_HiResNoFLI::calculateLuminanceError(float n,
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

  void P4FLI_HiResNoFLI::findLuminanceCodes(PRGData& prgData, long xc, long yc)
  {
    int     l0 = 0;
    int     l1 = 8;
    xc = xc & (~(long(7)));
    yc = yc & (~(long(7)));
    float   tmpBuf[64];
    const int nPixels = 64;
    for (int i = 0; i < nPixels; i++) {
      long    x_ = xc | long(i & 7);
      long    y_ = yc | long((i & 56) >> 3);
      tmpBuf[i] = resizedImage.y()[y_].getPixel(x_);
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
    prgData.l0(xc, yc << 1) = l0;
    prgData.l1(xc, yc << 1) = l1;
  }

  void P4FLI_HiResNoFLI::generateBitmaps(PRGData& prgData)
  {
    for (int yc = 0; yc < 200; yc += 2) {
      for (int xc = 0; xc < 320; xc++)
        ditherPixel(prgData, xc, yc);
      for (int xc = 319; xc >= 0; xc--)
        ditherPixel(prgData, xc, yc + 1);
    }
    for (int yc = 0; yc < 200; yc += 8) {
      for (int xc = 0; xc < 320; xc += 8) {
        // if all bits are 0 or 1, replace the bitmaps with
        // a dither pattern to improve color conversion
        int     b0 = 0;
        int     b1 = 1;
        for (int i = 0; i < 64; i++) {
          bool    tmp =
              prgData.getPixel(xc | (i & 7), (yc | ((i & 56) >> 3)) << 1);
          b0 = b0 | int(tmp);
          b1 = b1 & int(tmp);
        }
        int&    l0 = prgData.l0(xc, yc << 1);
        int&    l1 = prgData.l1(xc, yc << 1);
        if ((b0 == 0 && l0 != 0) || (b1 == 1 && l1 != 0)) {
          if (b0 == 0)
            l1 = l0;
          else
            l0 = l1;
          for (int i = 0; i < 64; i++) {
            int     tmpY = yc | ((i & 56) >> 3);
            int     tmpX = xc | (i & 7);
            prgData.setPixel(xc | (i & 7), tmpY << 1,
                             ditherPixelValue(tmpX, tmpY, 0.66667f));
          }
        }
      }
    }
  }

  void P4FLI_HiResNoFLI::findColorCodes(PRGData& prgData, long xc, long yc)
  {
    float   savedU[8][9];
    float   savedV[8][9];
    for (int l = 0; l < 8; l++) {
      for (int i = 0; i < 9; i++) {
        savedU[l][i] = lineU[l].getPixel(xc + i);
        savedV[l][i] = lineV[l].getPixel(xc + i);
      }
    }
    // find the pair of colors that gives the least amount of error
    int     c0 = 0;
    int     c1 = 0;
    int     l0 = prgData.l0(xc, yc << 1);
    int     l1 = prgData.l1(xc, yc << 1);
    double  minColorErr = 1000000.0;
    for (int i0 = 0; i0 < 15; i0++) {
      for (int i1 = 0; i1 < 15; i1++) {
        int     c0tmp = 0;
        int     c1tmp = 0;
        double  err = 0.0;
        {
          float   u0 = 0.0f;
          float   v0 = 0.0f;
          float   u1 = 0.0f;
          float   v1 = 0.0f;
          if (l0 > 0) {
            c0tmp = uvTable[i0].c;
            u0 = uvTable[i0].u;
            v0 = uvTable[i0].v;
          }
          if (l1 > 0) {
            c1tmp = uvTable[i1].c;
            u1 = uvTable[i1].u;
            v1 = uvTable[i1].v;
          }
          for (int l = 0; l < 8; l++) {
            for (int x = 0; x < 9; x++) {
              bool    b =
                  prgData.getPixel(xc + (x <= 7 ? x : 7), (yc + l) << 1);
              float   u_ = (b ? u1 : u0);
              float   v_ = (b ? v1 : v0);
              lineU[l].setPixel(xc + x, u_);
              lineV[l].setPixel(xc + x, v_);
            }
          }
        }
        for (int l = 0; l < 8; l++) {
          Line320 *l0U = &(lineU[l]);
          Line320 *l0V = &(lineV[l]);
          for (int x = 0; x < 8; x++) {
            float   u_ = l0U->getPixel(xc + x);
            float   v_ = l0V->getPixel(xc + x);
            if (!disablePAL) {
              // assume PAL filtering if requested
              Line320 *lm1U = &prvLineU;
              Line320 *lm1V = &prvLineV;
              if (l > 0) {
                lm1U = &(lineU[l - 1]);
                lm1V = &(lineV[l - 1]);
              }
              u_ += lm1U->getPixel(xc + x);
              v_ += lm1V->getPixel(xc + x);
              u_ *= 0.96f;
              v_ *= 0.96f;
              u_ += (l0U->getPixel(xc + (x - 1)) * 0.52f);
              v_ += (l0V->getPixel(xc + (x - 1)) * 0.52f);
              u_ += (l0U->getPixel(xc + (x + 1)) * 0.52f);
              v_ += (l0V->getPixel(xc + (x + 1)) * 0.52f);
              u_ += (lm1U->getPixel(xc + (x - 1)) * 0.52f);
              v_ += (lm1V->getPixel(xc + (x - 1)) * 0.52f);
              u_ += (lm1U->getPixel(xc + (x + 1)) * 0.52f);
              v_ += (lm1V->getPixel(xc + (x + 1)) * 0.52f);
              u_ *= 0.25f;
              v_ *= 0.25f;
            }
            double  errU =
                double(u_) - double(resizedImage.u()[yc + l].getPixel(xc + x));
            double  errV =
                double(v_) - double(resizedImage.v()[yc + l].getPixel(xc + x));
            err = err + (errU * errU) + (errV * errV);
            if (err > (minColorErr * 1.000001))
              break;
          }
        }
        if (err < minColorErr) {
          c0 = c0tmp;
          c1 = c1tmp;
          minColorErr = err;
        }
        else {
          for (int l = 0; l < 8; l++) {
            for (int i = 0; i < 9; i++) {
              lineU[l].setPixel(xc + i, savedU[l][i]);
              lineV[l].setPixel(xc + i, savedV[l][i]);
            }
          }
        }
      }
    }
    // store color codes
    prgData.c0(xc, yc << 1) = c0;
    prgData.c1(xc, yc << 1) = c1;
  }

  bool P4FLI_HiResNoFLI::processImage(PRGData& prgData,
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
      borderColor = config["borderColor"];
      disablePAL = !(bool(config["enablePAL"]));
      luminance1BitMode = config["luminance1BitMode"];
      checkParameters();
      createYTable();
      createUVTables();
      float   borderY = 0.0f;
      float   borderU = 0.0f;
      float   borderV = 0.0f;
      FLIConverter::convertPlus4Color(borderColor, borderY, borderU, borderV,
                                      monitorGamma);
      prgData.setConversionType(6);
      prgData.clear();
      prgData.borderColor() = (unsigned char) borderColor;
      for (int yc = 0; yc < 200; yc++) {
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
      for (int l = 0; l < 8; l++) {
        lineU[l].setBorderColor(borderU);
        lineV[l].setBorderColor(borderV);
      }
      imgConv.setImageSize(640, 400);
      imgConv.setPixelAspectRatio(1.0f);
      imgConv.setPixelStoreCallback(&pixelStoreCallback, (void *) this);
      imgConv.convertImageFile(infileName);
      progressMessage("Calculating FLI data");
      for (int yc = 0; yc < 200; yc += 8) {
        if (!setProgressPercentage(yc * 33 / 200)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        for (int xc = 0; xc < 320; xc += 8)
          findLuminanceCodes(prgData, xc, yc);
      }
      generateBitmaps(prgData);
      // convert color information
      for (int xc = 0; xc < 320; xc++) {
        prvLineU[xc] = borderU;
        prvLineV[xc] = borderV;
      }
      for (int yc = 0; yc < 200; yc += 8) {
        if (!setProgressPercentage((yc * 67 / 200) + 33)) {
          prgData[0] = 0x01;
          prgData[1] = 0x10;
          prgData[2] = 0x00;
          prgData[3] = 0x00;
          prgEndAddr = 0x1003U;
          progressMessage("");
          return false;
        }
        for (int l = 0; l < 8; l++) {
          lineU[l].clear();
          lineV[l].clear();
        }
        for (int xc = 0; xc < 320; xc += 8)
          findColorCodes(prgData, xc, yc);
        prvLineU = lineU[7];
        prvLineV = lineV[7];
      }
      setProgressPercentage(100);
      progressMessage("");
      // write PRG output
      prgData.convertImageData();
      // make the use of attribute values more consistent for easier
      // editing of the output file
      P4FLI_MultiColorNoFLI::optimizeAttributes(prgData);
      prgEndAddr = prgData.getImageDataEndAddress();
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

