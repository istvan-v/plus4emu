
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

static const float phaseShiftTable[16] = {
     0.0f,   33.0f,  327.0f,    0.0f,   33.0f,  327.0f,    0.0f,    0.0f,
     0.0f,   33.0f,   33.0f,    0.0f,    0.0f,  327.0f,  327.0f,    0.0f
};

static const float yScaleTable[16] = {
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    0.0f,
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f
};

static const float uScaleTable[16] = {
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    0.0f,    0.0f,
     1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f,    1.0f
};

static const float vScaleTable[16] = {
     1.0f,    1.0f,    1.0f,   -1.0f,   -1.0f,   -1.0f,    0.0f,    0.0f,
     1.0f,    1.0f,   -1.0f,   -1.0f,    1.0f,    1.0f,   -1.0f,   -1.0f
};

static const size_t colormapIndexTable[32] = {
  0x0000, 0x0200, 0x0300, 0x0500, 0x0300, 0x0500, 0x0000, 0x0200,
  0x0800, 0x0D00, 0x0B00, 0x0E00, 0x0B00, 0x0E00, 0x0800, 0x0D00,
  0x0100, 0x0000, 0x0100, 0x0000, 0x0400, 0x0300, 0x0400, 0x0300,
  0x0900, 0x0C00, 0x0900, 0x0C00, 0x0A00, 0x0F00, 0x0A00, 0x0F00
};

namespace Plus4Emu {

  void VideoDisplay::DisplayParameters::defaultIndexToRGBFunc(uint8_t color,
                                                              float& red,
                                                              float& green,
                                                              float& blue)
  {
    blue = green = red = (float(color) / 255.0f);
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
    if (src.indexToRGBFunc)
      indexToRGBFunc = src.indexToRGBFunc;
    else
      indexToRGBFunc = &defaultIndexToRGBFunc;
    brightness = (src.brightness > -0.5 ?
                  (src.brightness < 0.5 ? src.brightness : 0.5)
                  : -0.5);
    contrast = (src.contrast > 0.5 ?
                (src.contrast < 2.0 ? src.contrast : 2.0)
                : 0.5);
    gamma = (src.gamma > 0.25 ?
             (src.gamma < 4.0 ? src.gamma : 4.0)
             : 0.25);
    saturation = (src.saturation > 0.0 ?
                  (src.saturation < 2.0 ? src.saturation : 2.0)
                  : 0.0);
    redBrightness = (src.redBrightness > -0.5 ?
                     (src.redBrightness < 0.5 ? src.redBrightness : 0.5)
                     : -0.5);
    redContrast = (src.redContrast > 0.5 ?
                   (src.redContrast < 2.0 ? src.redContrast : 2.0)
                   : 0.5);
    redGamma = (src.redGamma > 0.25 ?
                (src.redGamma < 4.0 ? src.redGamma : 4.0)
                : 0.25);
    greenBrightness = (src.greenBrightness > -0.5 ?
                       (src.greenBrightness < 0.5 ? src.greenBrightness : 0.5)
                       : -0.5);
    greenContrast = (src.greenContrast > 0.5 ?
                     (src.greenContrast < 2.0 ? src.greenContrast : 2.0)
                     : 0.5);
    greenGamma = (src.greenGamma > 0.25 ?
                  (src.greenGamma < 4.0 ? src.greenGamma : 4.0)
                  : 0.25);
    blueBrightness = (src.blueBrightness > -0.5 ?
                      (src.blueBrightness < 0.5 ? src.blueBrightness : 0.5)
                      : -0.5);
    blueContrast = (src.blueContrast > 0.5 ?
                    (src.blueContrast < 2.0 ? src.blueContrast : 2.0)
                    : 0.5);
    blueGamma = (src.blueGamma > 0.25 ?
                 (src.blueGamma < 4.0 ? src.blueGamma : 4.0)
                 : 0.25);
    blendScale1 = (src.blendScale1 > 0.0 ?
                   (src.blendScale1 < 0.5 ? src.blendScale1 : 0.5) : 0.0);
    blendScale2 = (src.blendScale2 > 0.0 ?
                   (src.blendScale2 < 1.0 ? src.blendScale2 : 1.0) : 0.0);
    blendScale3 = (src.blendScale3 > 0.0 ?
                   (src.blendScale3 < 1.0 ? src.blendScale3 : 1.0) : 0.0);
    pixelAspectRatio = (src.pixelAspectRatio > 0.5 ?
                        (src.pixelAspectRatio < 2.0 ?
                         src.pixelAspectRatio : 2.0) : 0.5);
  }

  VideoDisplay::DisplayParameters::DisplayParameters()
    : displayQuality(2),
      bufferingMode(0),
      ntscMode(false),
      indexToRGBFunc(&defaultIndexToRGBFunc),
      brightness(0.0), contrast(1.0), gamma(1.0), saturation(1.0),
      redBrightness(0.0), redContrast(1.0), redGamma(1.0),
      greenBrightness(0.0), greenContrast(1.0), greenGamma(1.0),
      blueBrightness(0.0), blueContrast(1.0), blueGamma(1.0),
      blendScale1(0.5),
      blendScale2(0.7),
      blendScale3(0.3),
      pixelAspectRatio(1.0)
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

  void VideoDisplay::DisplayParameters::applyColorCorrection(float& red,
                                                             float& green,
                                                             float& blue) const
  {
    double  r = red;
    double  g = green;
    double  b = blue;
    // Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
    // U = 0.492 * (B - Y)
    // V = 0.877 * (R - Y)
    double  y = (0.299 * r) + (0.587 * g) + (0.114 * b);
    double  u = 0.492 * (b - y);
    double  v = 0.877 * (r - y);
    u = u * saturation;
    v = v * saturation;
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    r = (v / 0.877) + y;
    b = (u / 0.492) + y;
    g = (y - ((r * 0.299) + (b * 0.114))) / 0.587;
    r = std::pow((r > 0.0 ? r : 0.0), 1.0 / gamma);
    g = std::pow((g > 0.0 ? g : 0.0), 1.0 / gamma);
    b = std::pow((b > 0.0 ? b : 0.0), 1.0 / gamma);
    r = (r - 0.5) * contrast + 0.5 + brightness;
    g = (g - 0.5) * contrast + 0.5 + brightness;
    b = (b - 0.5) * contrast + 0.5 + brightness;
    r = std::pow((r > 0.0 ? r : 0.0), 1.0 / redGamma);
    g = std::pow((g > 0.0 ? g : 0.0), 1.0 / greenGamma);
    b = std::pow((b > 0.0 ? b : 0.0), 1.0 / blueGamma);
    r = (r - 0.5) * redContrast + 0.5 + redBrightness;
    g = (g - 0.5) * greenContrast + 0.5 + greenBrightness;
    b = (b - 0.5) * blueContrast + 0.5 + blueBrightness;
    red = float(r > 0.0 ? (r < 1.0 ? r : 1.0) : 0.0);
    green = float(g > 0.0 ? (g < 1.0 ? g : 1.0) : 0.0);
    blue = float(b > 0.0 ? (b < 1.0 ? b : 1.0) : 0.0);
  }

  VideoDisplay::~VideoDisplay()
  {
  }

  // --------------------------------------------------------------------------

  template <>
  uint16_t VideoDisplayColormap<uint16_t>::pixelConv(float r, float g, float b)
  {
    uint16_t  ri, gi, bi;
    ri = uint16_t(r >= 0.0f ? (r < 1.0f ? (r * 31.0f + 0.5f) : 31.0f) : 0.0f);
    gi = uint16_t(g >= 0.0f ? (g < 1.0f ? (g * 63.0f + 0.5f) : 63.0f) : 0.0f);
    bi = uint16_t(b >= 0.0f ? (b < 1.0f ? (b * 31.0f + 0.5f) : 31.0f) : 0.0f);
    return ((ri << 11) | (gi << 5) | bi);
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
    colormapData = new T[256 * 16];
    try {
      colormapTable = new T*[256];
    }
    catch (...) {
      delete[] colormapData;
      throw;
    }
    for (size_t i = 0; i < 0x1000; i++)
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
    for (size_t i = 0; i < 256; i++) {
      float   r = float(int(i)) / 255.0f;
      float   g = r;
      float   b = r;
      if (displayParameters.indexToRGBFunc)
        displayParameters.indexToRGBFunc(uint8_t(i), r, g, b);
      // Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
      // U = 0.492 * (B - Y)
      // V = 0.877 * (R - Y)
      float   y = (0.299f * r) + (0.587f * g) + (0.114f * b);
      float   u = 0.492f * (b - y);
      float   v = 0.877f * (r - y);
      baseColormap[i * 3 + 0] = y;
      baseColormap[i * 3 + 1] = u;
      baseColormap[i * 3 + 2] = v;
    }
    for (size_t i = 0; i < 0x1000; i++) {
      size_t  j = i & 0xFF;
      size_t  k = i >> 8;
      float   y = baseColormap[j * 3 + 0];
      float   uTmp = baseColormap[j * 3 + 1];
      float   vTmp = baseColormap[j * 3 + 2];
      if (i >= 0x0800) {
        if (i < 0x0C00) {
          uTmp = uTmp - 0.12728f;       // add PAL burst (135 degrees)
          vTmp = vTmp + 0.12728f;
        }
        else {
          uTmp = uTmp - 0.18f;          // add NTSC burst (180 degrees)
        }
      }
      float   phaseShift = phaseShiftTable[k] * 0.01745329f;
      float   re = float(std::cos(phaseShift));
      float   im = float(std::sin(phaseShift));
      float   u = uTmp * re - vTmp * im;
      float   v = uTmp * im + vTmp * re;
      u = u * uScaleTable[k];
      v = v * vScaleTable[k];
      {
        float   r = (v / 0.877f) + y;
        float   b = (u / 0.492f) + y;
        float   g = (y - ((r * 0.299f) + (b * 0.114f))) / 0.587f;
        displayParameters.applyColorCorrection(r, g, b);
        y = (0.299f * r) + (0.587f * g) + (0.114f * b);
        u = 0.492f * (b - y);
        v = 0.877f * (r - y);
      }
      y = y * yScaleTable[k];
      u = u * yScaleTable[k];
      v = v * yScaleTable[k];
      if (!yuvFormat) {
        // R = (V / 0.877) + Y
        // B = (U / 0.492) + Y
        // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
        float   r = (v / 0.877f) + y;
        float   b = (u / 0.492f) + y;
        float   g = (y - ((r * 0.299f) + (b * 0.114f))) / 0.587f;
        colormapData[i] = pixelConv(r, g, b);
      }
      else {
        u = (u + 0.435912f) * 1.147020f;
        v = (v + 0.614777f) * 0.813303f;
        colormapData[i] = pixelConv(y, u, v);
      }
    }
  }

  template class VideoDisplayColormap<uint16_t>;
  template class VideoDisplayColormap<uint32_t>;

}       // namespace Plus4Emu

