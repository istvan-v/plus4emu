
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
#include "display.hpp"

#include <cmath>

static const float phaseShiftTable[18] = {
     0.0f,   33.0f,  327.0f,    0.0f,   33.0f,  327.0f,    0.0f,    0.0f,
     0.0f,   33.0f,   33.0f,    0.0f,    0.0f,  327.0f,  327.0f,    0.0f,
     0.0f,    0.0f
};

static const float yScaleTable[18] = {
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    0.0f,
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,
     1.0f,    1.0f
};

static const float uScaleTable[18] = {
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    0.0f,    0.0f,
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,
     1.0f,    1.0f
};

static const float vScaleTable[18] = {
     1.0f,    1.0f,    1.0f,   -1.0f,   -1.0f,   -1.0f,    0.0f,    0.0f,
     1.0f,    1.0f,   -1.0f,   -1.0f,    1.0f,    1.0f,   -1.0f,   -1.0f,
     1.0f,   -1.0f
};

static const bool ntscModeTable[18] = {
  false, false,  true, false, false,  true, false, false,
  false, false, false, false,  true,  true,  true,  true,
   true,  true
};

static const unsigned short colormapIndexTable[32] = {
  0x0000, 0x0200, 0x0300, 0x0500, 0x0300, 0x0500, 0x0000, 0x0200,
  0x0800, 0x0D00, 0x0B00, 0x0E00, 0x0B00, 0x0E00, 0x0800, 0x0D00,
  0x0100, 0x1000, 0x0100, 0x1000, 0x0400, 0x1100, 0x0400, 0x1100,
  0x0900, 0x0C00, 0x0900, 0x0C00, 0x0A00, 0x0F00, 0x0A00, 0x0F00
};

namespace Plus4Emu {

  void VideoDisplay::DisplayParameters::defaultIndexToYUVFunc(
      uint8_t color, bool isNTSC, float& y, float& u, float& v)
  {
    (void) isNTSC;
    y = float(color) / 255.0f;
    u = 0.0f;
    v = 0.0f;
  }

  void VideoDisplay::DisplayParameters::copyDisplayParameters(
      const DisplayParameters& src)
  {
    displayQuality = (src.displayQuality > 0 ?
                      (src.displayQuality < 3 ? src.displayQuality : 3)
                      : 0);
    bufferingMode = (src.bufferingMode > 0 ?
                     (src.bufferingMode < 2 ? src.bufferingMode : 2) : 0);
    ntscMode = src.ntscMode;
    if (src.indexToYUVFunc)
      indexToYUVFunc = src.indexToYUVFunc;
    else
      indexToYUVFunc = &defaultIndexToYUVFunc;
    brightness = (src.brightness > -0.5f ?
                  (src.brightness < 0.5f ? src.brightness : 0.5f)
                  : -0.5f);
    contrast = (src.contrast > 0.5f ?
                (src.contrast < 2.0f ? src.contrast : 2.0f)
                : 0.5f);
    gamma = (src.gamma > 0.25f ?
             (src.gamma < 4.0f ? src.gamma : 4.0f)
             : 0.25f);
    hueShift = (src.hueShift > -180.0f ?
                (src.hueShift < 180.0f ? src.hueShift : 180.0f)
                : -180.0f);
    saturation = (src.saturation > 0.0f ?
                  (src.saturation < 2.0f ? src.saturation : 2.0f)
                  : 0.0f);
    redBrightness = (src.redBrightness > -0.5f ?
                     (src.redBrightness < 0.5f ? src.redBrightness : 0.5f)
                     : -0.5f);
    redContrast = (src.redContrast > 0.5f ?
                   (src.redContrast < 2.0f ? src.redContrast : 2.0f)
                   : 0.5f);
    redGamma = (src.redGamma > 0.25f ?
                (src.redGamma < 4.0f ? src.redGamma : 4.0f)
                : 0.25f);
    greenBrightness = (src.greenBrightness > -0.5f ?
                       (src.greenBrightness < 0.5f ? src.greenBrightness : 0.5f)
                       : -0.5f);
    greenContrast = (src.greenContrast > 0.5f ?
                     (src.greenContrast < 2.0f ? src.greenContrast : 2.0f)
                     : 0.5f);
    greenGamma = (src.greenGamma > 0.25f ?
                  (src.greenGamma < 4.0f ? src.greenGamma : 4.0f)
                  : 0.25f);
    blueBrightness = (src.blueBrightness > -0.5f ?
                      (src.blueBrightness < 0.5f ? src.blueBrightness : 0.5f)
                      : -0.5f);
    blueContrast = (src.blueContrast > 0.5f ?
                    (src.blueContrast < 2.0f ? src.blueContrast : 2.0f)
                    : 0.5f);
    blueGamma = (src.blueGamma > 0.25f ?
                 (src.blueGamma < 4.0f ? src.blueGamma : 4.0f)
                 : 0.25f);
    lineShade = (src.lineShade > 0.0f ?
                 (src.lineShade < 1.0f ? src.lineShade : 1.0f) : 0.0f);
    blendScale = (src.blendScale > 0.5f ?
                  (src.blendScale < 2.0f ? src.blendScale : 2.0f) : 0.5f);
    motionBlur = (src.motionBlur > 0.0f ?
                  (src.motionBlur < 0.95f ? src.motionBlur : 0.95f) : 0.0f);
    pixelAspectRatio = (src.pixelAspectRatio > 0.5f ?
                        (src.pixelAspectRatio < 2.0f ?
                         src.pixelAspectRatio : 2.0f) : 0.5f);
  }

  VideoDisplay::DisplayParameters::DisplayParameters()
    : displayQuality(2),
      bufferingMode(0),
      ntscMode(false),
      indexToYUVFunc(&defaultIndexToYUVFunc),
      brightness(0.0f), contrast(1.0f), gamma(1.0f),
      hueShift(0.0f), saturation(1.0f),
      redBrightness(0.0f), redContrast(1.0f), redGamma(1.0f),
      greenBrightness(0.0f), greenContrast(1.0f), greenGamma(1.0f),
      blueBrightness(0.0f), blueContrast(1.0f), blueGamma(1.0f),
      lineShade(0.8f),
      blendScale(1.0f),
      motionBlur(0.25f),
      pixelAspectRatio(1.0f)
  {
  }

  VideoDisplay::DisplayParameters::DisplayParameters(
      const DisplayParameters& dp)
  {
    copyDisplayParameters(dp);
  }

  VideoDisplay::DisplayParameters& VideoDisplay::DisplayParameters::operator=(
      const DisplayParameters& dp)
  {
    copyDisplayParameters(dp);
    return (*this);
  }

  void VideoDisplay::DisplayParameters::yuvToRGBWithColorCorrection(
      float& red, float& green, float& blue, float y, float u, float v) const
  {
    float   hueShiftU = float(std::cos(double(hueShift) * 0.01745329252));
    float   hueShiftV = float(std::sin(double(hueShift) * 0.01745329252));
    float   tmpU = ((u * hueShiftU) - (v * hueShiftV)) * saturation;
    float   tmpV = ((v * hueShiftU) + (u * hueShiftV)) * saturation;
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    float   r = (tmpV / 0.877f) + y;
    float   b = (tmpU / 0.492f) + y;
    float   g = (y - ((r * 0.299f) + (b * 0.114f))) / 0.587f;
    r = (r - 0.5f) * (contrast * redContrast) + 0.5f;
    g = (g - 0.5f) * (contrast * greenContrast) + 0.5f;
    b = (b - 0.5f) * (contrast * blueContrast) + 0.5f;
    r = r + (brightness + redBrightness);
    g = g + (brightness + greenBrightness);
    b = b + (brightness + blueBrightness);
    r = (r > 0.0f ? r : 0.0f);
    g = (g > 0.0f ? g : 0.0f);
    b = (b > 0.0f ? b : 0.0f);
    r = float(std::pow(double(r), double(1.0f / (gamma * redGamma))));
    g = float(std::pow(double(g), double(1.0f / (gamma * greenGamma))));
    b = float(std::pow(double(b), double(1.0f / (gamma * blueGamma))));
    red = (r < 1.0f ? r : 1.0f);
    green = (g < 1.0f ? g : 1.0f);
    blue = (b < 1.0f ? b : 1.0f);
  }

  VideoDisplay::~VideoDisplay()
  {
  }

  void VideoDisplay::limitFrameRate(bool isEnabled)
  {
    (void) isEnabled;
  }

  // --------------------------------------------------------------------------

  template <>
  uint16_t VideoDisplayColormap<uint16_t>::pixelConv(float r, float g, float b)
  {
    int ri = int(r >= 0.0f ? (r < 1.0f ? (r * 248.0f + 4.0f) : 252.0f) : 4.0f);
    int gi = int(g >= 0.0f ? (g < 1.0f ? (g * 504.0f + 4.0f) : 508.0f) : 4.0f);
    int bi = int(b >= 0.0f ? (b < 1.0f ? (b * 248.0f + 4.0f) : 252.0f) : 4.0f);
    if (((ri | bi) & 7) < 2 && (gi & 7) >= 6)
      gi = gi + 4;
    if (((ri & bi) & 7) >= 6 && (gi & 7) < 2)
      gi = gi - 4;
    return uint16_t(((ri & 0x00F8) << 8) | ((gi & 0x01F8) << 2) | (bi >> 3));
  }

  template <>
  uint32_t VideoDisplayColormap<uint32_t>::pixelConv(float r, float g, float b)
  {
    uint32_t  ri, gi, bi;
    ri = uint32_t(r >= 0.0f ? (r < 1.0f ? (r * 255.0f + 0.5f) : 255.0f) : 0.0f);
    gi = uint32_t(g >= 0.0f ? (g < 1.0f ? (g * 255.0f + 0.5f) : 255.0f) : 0.0f);
    bi = uint32_t(b >= 0.0f ? (b < 1.0f ? (b * 255.0f + 0.5f) : 255.0f) : 0.0f);
    return ((bi << 16) | (gi << 8) | ri);
  }

  template <typename T>
  VideoDisplayColormap<T>::VideoDisplayColormap()
  {
    colormapData = new T[256 * 18];
    try {
      colormapTable = new T*[256];
    }
    catch (...) {
      delete[] colormapData;
      throw;
    }
    for (size_t i = 0; i < 0x1200; i++)
      colormapData[i] = T(0);
    for (size_t i = 0; i < 256; i++) {
      T       *p = &(colormapData[0x0700]);
      if (!(i & 0xC0)) {
        if (!(i & 0x20)) {
          p = &(colormapData[0x0600]);
        }
        else {
          p = &(colormapData[colormapIndexTable[i & 0x1F]]);
        }
      }
      colormapTable[i] = p;
    }
  }

  template <typename T>
  VideoDisplayColormap<T>::~VideoDisplayColormap()
  {
    delete[] colormapData;
    delete[] colormapTable;
  }

  template <typename T>
  void VideoDisplayColormap<T>::setDisplayParameters(
      const VideoDisplay::DisplayParameters& displayParameters, bool yuvFormat)
  {
    float   baseColormap[768];
    bool    prvNTSCMode = !(ntscModeTable[0]);
    for (size_t i = 0; i < 0x1200; i++) {
      size_t  j = i & 0xFF;
      size_t  k = i >> 8;
      if (ntscModeTable[k] != prvNTSCMode) {
        prvNTSCMode = ntscModeTable[k];
        for (size_t l = 0; l < 256; l++) {
          float   y = float(int(l)) / 255.0f;
          float   u = 0.0f;
          float   v = 0.0f;
          if (displayParameters.indexToYUVFunc)
            displayParameters.indexToYUVFunc(uint8_t(l), prvNTSCMode, y, u, v);
          baseColormap[l * 3 + 0] = y;
          baseColormap[l * 3 + 1] = u;
          baseColormap[l * 3 + 2] = v;
        }
      }
      float   y = baseColormap[j * 3 + 0];
      float   uTmp = baseColormap[j * 3 + 1];
      float   vTmp = baseColormap[j * 3 + 2];
      if (i & 0x0800) {
        if (i < 0x0C00) {
          uTmp = uTmp - 0.11314f;       // add PAL burst (135 degrees)
          vTmp = vTmp + 0.11314f;
        }
        else {
          uTmp = uTmp - 0.16f;          // add NTSC burst (180 degrees)
        }
        // limit chroma level
        float   c = float(std::sqrt(double((uTmp * uTmp) + (vTmp * vTmp))));
        if (c > 0.21f) {
          uTmp *= (0.21f / c);
          vTmp *= (0.21f / c);
        }
      }
      float   phaseShift = phaseShiftTable[k] * 0.01745329f;
      float   re = float(std::cos(phaseShift));
      float   im = float(std::sin(phaseShift));
      float   u = uTmp * re - vTmp * im;
      float   v = uTmp * im + vTmp * re;
      u = u * uScaleTable[k];
      v = v * vScaleTable[k];
      if (!yuvFormat) {
        float   r = 0.0f, g = 0.0f, b = 0.0f;
        displayParameters.yuvToRGBWithColorCorrection(r, g, b, y, u, v);
        r = r * yScaleTable[k];
        g = g * yScaleTable[k];
        b = b * yScaleTable[k];
        colormapData[i] = pixelConv(r, g, b);
      }
      else {
        float   r = 0.0f, g = 0.0f, b = 0.0f;
        displayParameters.yuvToRGBWithColorCorrection(r, g, b, y, u, v);
        y = (0.299f * r) + (0.587f * g) + (0.114f * b);
        u = 0.492f * (b - y);
        v = 0.877f * (r - y);
        y = y * yScaleTable[k];
        u = u * yScaleTable[k];
        v = v * yScaleTable[k];
        u = (u + 0.435912f) * 1.147020f;
        v = (v + 0.614777f) * 0.813303f;
        colormapData[i] = pixelConv(y, u, v);
      }
    }
  }

  template class VideoDisplayColormap<uint16_t>;
  template class VideoDisplayColormap<uint32_t>;

}       // namespace Plus4Emu

