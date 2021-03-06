
// p4fliconv: high resolution interlaced FLI converter utility
// Copyright (C) 2007-2017 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "imageconv.hpp"

#include <vector>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

// C64 palette from VICE 1.22
static const unsigned char  c64Palette[48] = {
    0,   0,   0,  255, 255, 255,  137,  64,  54,  122, 191, 199,
  138,  70, 174,  104, 169,  65,   62,  49, 162,  208, 220, 113,
  144,  95,  37,   92,  71,   0,  187, 119, 109,   85,  85,  85,
  128, 128, 128,  172, 234, 136,  124, 112, 218,  171, 171, 171
};

static void defaultProgressMessageCb(void *userData, const char *msg)
{
  (void) userData;
  if (msg != (char *) 0 && msg[0] != '\0')
    std::fprintf(stderr, "%s\n", msg);
}

static bool defaultProgressPercentageCb(void *userData, int n)
{
  (void) userData;
  if (n != 100)
    std::fprintf(stderr, "\r  %3d%%    ", n);
  else
    std::fprintf(stderr, "\r  %3d%%    \n", n);
  return true;
}

static void parseXPMHeader(std::vector< long >& buf, const char *s)
{
  buf.resize(0);
  if (!s)
    return;
  while (true) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
      s++;
    if (*s == '\0')
      break;
    char    *endp = (char *) 0;
    long    n = std::strtol(s, &endp, 0);
    if (endp == (char *) 0 || endp == s ||
        !(*endp == ' ' || *endp == '\t' || *endp == '\r' || *endp == '\n' ||
          *endp == '\0')) {
      throw Plus4Emu::Exception("invalid XPM header");
    }
    s = endp;
    buf.push_back(n);
    if (buf.size() > 4)
      throw Plus4Emu::Exception("invalid XPM header");
  }
  if (buf.size() != 4)
    throw Plus4Emu::Exception("invalid XPM header");
}

static void parseXPMColor(uint32_t& c, std::string& pattern,
                          const char *s, size_t patternLen)
{
  c = 0U;
  pattern = "";
  if (!s)
    throw Plus4Emu::Exception("invalid XPM colormap entry");
  for (size_t i = 0; i < (patternLen + 4); i++) {
    if (s[i] == '\0')
      throw Plus4Emu::Exception("invalid XPM colormap entry");
  }
  if ((s[patternLen] != ' ' && s[patternLen] != '\t') ||
      s[patternLen + 1] != 'c' ||
      (s[patternLen + 2] != ' ' && s[patternLen + 2] != '\t')) {
    throw Plus4Emu::Exception("invalid XPM colormap entry");
  }
  for (size_t i = 0; i < patternLen; i++)
    pattern += s[i];
  s = s + (patternLen + 3);
  if (s[0] == '#') {
    // hexadecimal RGB color
    for (size_t i = 1; i < 7; i++) {
      c = c << 4;
      if (s[i] >= '0' && s[i] <= '9')
        c = c | uint32_t(s[i] - '0');
      else if (s[i] >= 'A' && s[i] <= 'F')
        c = c | (uint32_t(s[i] - 'A') + 10U);
      else if (s[i] >= 'a' && s[i] <= 'f')
        c = c | (uint32_t(s[i] - 'a') + 10U);
      else
        throw Plus4Emu::Exception("invalid XPM colormap entry");
    }
    if (s[7] != '\0')
      throw Plus4Emu::Exception("invalid XPM colormap entry");
  }
  else {
    // named color
    char    tmpBuf[16];
    for (size_t i = 0; true; i++) {
      if (s[i] == '\0' || i == 15) {
        tmpBuf[i] = '\0';
        break;
      }
      tmpBuf[i] = s[i] | (char) 0x20;
    }
    if (std::strcmp(&(tmpBuf[0]), "none") == 0)
      c = 0xFF000000U;          // transparent color
    else if (std::strcmp(&(tmpBuf[0]), "black") == 0)
      c = 0x00000000U;
    else if (std::strcmp(&(tmpBuf[0]), "red") == 0)
      c = 0x00FF0000U;
    else if (std::strcmp(&(tmpBuf[0]), "yellow") == 0)
      c = 0x00FFFF00U;
    else if (std::strcmp(&(tmpBuf[0]), "green") == 0)
      c = 0x0000FF00U;
    else if (std::strcmp(&(tmpBuf[0]), "cyan") == 0)
      c = 0x0000FFFFU;
    else if (std::strcmp(&(tmpBuf[0]), "blue") == 0)
      c = 0x000000FFU;
    else if (std::strcmp(&(tmpBuf[0]), "magenta") == 0)
      c = 0x00FF00FFU;
    else if (std::strcmp(&(tmpBuf[0]), "white") == 0)
      c = 0x00FFFFFFU;
    else if (std::strncmp(&(tmpBuf[0]), "gray", 4) == 0 ||
             std::strncmp(&(tmpBuf[0]), "grey", 4) == 0) {
      s = s + 4;
      if (*s != '\0') {
        while (*s != '\0') {
          if (*s >= '0' && *s <= '9')
            c = (c * 10U) + uint32_t(*s - '0');
          else
            throw Plus4Emu::Exception("invalid XPM colormap entry");
          s++;
        }
        c = ((((c * 255U) + 50U) / 100U) & 0xFFU) * 0x00010101U;
      }
      else
        c = 0x00BEBEBEU;
    }
    else
      throw Plus4Emu::Exception("invalid XPM colormap entry");
  }
}

static void readColormapImage(std::vector< uint16_t >& pixelBuf,
                              std::vector< uint32_t >& palette,
                              Fl_Image& image_)
{
  int     w = image_.w();
  int     h = image_.h();
  int     cnt = image_.count();
  const char * const  *imageData = image_.data();
  if (image_.d() != 1 || cnt < 3 || imageData == (char **) 0)
    throw Plus4Emu::Exception("image is not a pixmap");
  std::vector< long > xpmParams;
  parseXPMHeader(xpmParams, imageData[0]);
  if (xpmParams[0] != w || xpmParams[1] != h)
    throw Plus4Emu::Exception("invalid XPM header");
  int     nColors = int(xpmParams[2]);
  int     patternSize = int(xpmParams[3]);
  if (nColors >= 1 && patternSize >= 1 && cnt == (nColors + h + 1)) {
    // pixmap formats
    if (nColors < 1 || nColors > 65536 || patternSize > 3)
      throw Plus4Emu::Exception("invalid XPM header");
    pixelBuf.resize(size_t(w) * size_t(h));
    palette.resize(size_t(nColors));
    std::map< std::string, uint16_t > colorMap;
    for (int i = 0; i < nColors; i++) {
      // read palette
      uint32_t  c = 0U;
      std::string pattern;
      parseXPMColor(c, pattern, imageData[i + 1], size_t(patternSize));
      palette[i] = c;
      colorMap[pattern] = uint16_t(i);
    }
    std::string pattern;
    for (int yc = 0; yc < h; yc++) {
      // read and convert image data
      const char  *s = imageData[yc + nColors + 1];
      for (int xc = 0; xc < w; xc++) {
        pattern.clear();
        for (int i = 0; i < patternSize; i++) {
          if (s[i] == '\0')
            throw Plus4Emu::Exception("error in XPM image data");
          pattern += s[i];
        }
        s = s + patternSize;
        std::map< std::string, uint16_t >::iterator i_ = colorMap.find(pattern);
        if (i_ == colorMap.end())
          throw Plus4Emu::Exception("error in XPM image data");
        pixelBuf[(yc * w) + xc] = (*i_).second;
      }
    }
  }
  else {
    // GIF format
    nColors = std::abs(nColors);
    if (nColors < 1 || nColors > 256 || patternSize > 1 || cnt != (h + 2))
      throw Plus4Emu::Exception("invalid image format information");
    pixelBuf.resize(size_t(w) * size_t(h));
    palette.resize(size_t(nColors));
    std::vector< uint16_t > colorMap;
    colorMap.resize(256);
    for (int i = 0; i < nColors; i++) {
      // read palette
      uint32_t  c =
          (uint32_t((unsigned char) imageData[1][(i << 2) + 1]) << 16)
          | (uint32_t((unsigned char) imageData[1][(i << 2) + 2]) << 8)
          | uint32_t((unsigned char) imageData[1][(i << 2) + 3]);
      unsigned char n = (unsigned char) imageData[1][i << 2] & 0xFF;
      if (i == 0 && n == 0x20)
        c = c | 0xFF000000U;    // transparent color
      palette[i] = c;
      colorMap[n] = uint16_t(i);
    }
    for (int yc = 0; yc < h; yc++) {
      // read and convert image data
      for (int xc = 0; xc < w; xc++) {
        pixelBuf[(yc * w) + xc] =
            colorMap[(unsigned char) imageData[yc + 2][xc] & 0xFF];
      }
    }
  }
}

namespace Plus4FLIConv {

  void YUVImageConverter::defaultStorePixelFunc(void *userData, int xc, int yc,
                                                float y, float u, float v)
  {
    (void) userData;
    (void) xc;
    (void) yc;
    float   r = 0.0f;
    float   g = 0.0f;
    float   b = 0.0f;
    yuvToRGB(r, g, b, y, u, v);
    limitRGBColor(r, g, b);
    std::fputc(int(r * 255.0f + 0.5f), stdout);
    std::fputc(int(g * 255.0f + 0.5f), stdout);
    std::fputc(int(b * 255.0f + 0.5f), stdout);
  }

  YUVImageConverter::YUVImageConverter()
    : width(640),
      height(400),
      pixelAspectRatio(1.0f),
      scaleX(1.0f),
      scaleY(1.0f),
      offsetX(0.0f),
      offsetY(0.0f),
      gammaCorrection(1.0f),
      monitorGamma(1.0f),
      yMin(0.0f),
      yMax(1.0f),
      colorSaturationMult(1.0f),
      colorSaturationPow(1.0f),
      borderColorY(0.0f),
      borderColorU(0.0f),
      borderColorV(0.0f),
      storePixelFunc(&defaultStorePixelFunc),
      storePixelFuncUserData((void *) 0),
      progressMessageCallback(&defaultProgressMessageCb),
      progressMessageUserData((void *) 0),
      progressPercentageCallback(&defaultProgressPercentageCb),
      progressPercentageUserData((void *) 0),
      prvProgressPercentage(-1),
      interpolationEnabled(true)
  {
    c64ColorTable[0] = 0x00;
    c64ColorTable[1] = 0x71;
    c64ColorTable[2] = 0x22;
    c64ColorTable[3] = 0x53;
    c64ColorTable[4] = 0x34;
    c64ColorTable[5] = 0x4F;
    c64ColorTable[6] = 0x1E;
    c64ColorTable[7] = 0x67;
    c64ColorTable[8] = 0x38;
    c64ColorTable[9] = 0x19;
    c64ColorTable[10] = 0x42;
    c64ColorTable[11] = 0x21;
    c64ColorTable[12] = 0x41;
    c64ColorTable[13] = 0x6F;
    c64ColorTable[14] = 0x4E;
    c64ColorTable[15] = 0x51;
  }

  YUVImageConverter::~YUVImageConverter()
  {
  }

  bool YUVImageConverter::isC64ImageFile(const char *fileName)
  {
    if (!(Plus4Emu::checkFileNameExtension(fileName, ".koa") ||
          Plus4Emu::checkFileNameExtension(fileName, ".ocp"))) {
      return false;
    }
    std::FILE *f = Plus4Emu::fileOpen(fileName, "rb");
    if (!f)
      return false;
    if (std::fseek(f, 0L, SEEK_END) < 0) {
      std::fclose(f);
      return false;
    }
    long    fileSize = std::ftell(f);
    if (std::fseek(f, 0L, SEEK_SET) < 0) {
      std::fclose(f);
      return false;
    }
    unsigned char startAddr[2];
    if (std::fread(&(startAddr[0]), sizeof(unsigned char), 2, f) != 2) {
      std::fclose(f);
      return false;
    }
    std::fclose(f);
    f = (std::FILE *) 0;
    if (startAddr[0] != 0x00)
      return false;
    if (Plus4Emu::checkFileNameExtension(fileName, ".koa") &&
        fileSize == 10003L && startAddr[1] == 0x60) {
      // Koala Painter format
      return true;
    }
    if (Plus4Emu::checkFileNameExtension(fileName, ".ocp") &&
        fileSize == 10018L && startAddr[1] == 0x20) {
      // Advanced Art Studio format
      return true;
    }
    return false;
  }

  bool YUVImageConverter::convertC64ImageFile(const char *fileName)
  {
    std::vector< unsigned char >  fileData;
    std::vector< unsigned char >  bitmapData;
    std::vector< unsigned char >  attr12Data;
    std::vector< unsigned char >  attr3Data;
    std::vector< unsigned char >  imageData;
    unsigned char color0 = 0x00;
    fileData.resize(10240);
    bitmapData.resize(8000);
    attr12Data.resize(1000);
    attr3Data.resize(1000);
    imageData.resize(64000);    // 320x200 Plus/4 color codes
    std::FILE *f = Plus4Emu::fileOpen(fileName, "rb");
    if (!f)
      throw Plus4Emu::Exception("error opening image file");
    size_t  fileSize =
        std::fread(&(fileData.front()), sizeof(unsigned char), 10240, f);
    std::fclose(f);
    f = (std::FILE *) 0;
    if (fileSize == 10003 && fileData[0] == 0x00 && fileData[1] == 0x60) {
      // Koala Painter format
      for (int i = 0; i < 8000; i++)
        bitmapData[i] = fileData[i + 0x0002];
      for (int i = 0; i < 1000; i++)
        attr12Data[i] = fileData[i + 0x1F42];
      for (int i = 0; i < 1000; i++)
        attr3Data[i] = fileData[i + 0x232A];
      color0 = fileData[0x2712];
    }
    else if (fileSize == 10018 && fileData[0] == 0x00 && fileData[1] == 0x20) {
      // Advanced Art Studio format
      for (int i = 0; i < 8000; i++)
        bitmapData[i] = fileData[i + 0x0002];
      for (int i = 0; i < 1000; i++)
        attr12Data[i] = fileData[i + 0x1F42];
      for (int i = 0; i < 1000; i++)
        attr3Data[i] = fileData[i + 0x233A];
      color0 = fileData[0x232B];
    }
    else {
      throw Plus4Emu::Exception("image format is not supported");
    }
    progressMessage("Resizing image");
    setProgressPercentage(0);
    // read C64 image data, and convert it to a temporary 320x200 Plus/4 image
    for (int yc = 0; yc < 200; yc++) {
      for (int xc = 0; xc < 320; xc++) {
        int     n = ((yc >> 3) * 40) + (xc >> 3);
        unsigned char color1 = (attr12Data[n] & 0xF0) >> 4;
        unsigned char color2 = attr12Data[n] & 0x0F;
        unsigned char color3 = attr3Data[n] & 0x0F;
        int     c = color0;
        int     b = int(bitmapData[(n << 3) | (yc & 7)]);
        b = (b & (3 << (6 - (xc & 6)))) >> (6 - (xc & 6));
        switch (b) {
        case 1:
          c = color1;
          break;
        case 2:
          c = color2;
          break;
        case 3:
          c = color3;
          break;
        }
        imageData[(yc * 320) + xc] = c64ColorTable[c];
      }
    }
    // calculate scale and offset
    float   aspectScale = (float(width) * pixelAspectRatio / float(height))
                          / (320.0f / 200.0f);
    float   xScale = 320.0f / float(width);
    float   yScale = 200.0f / float(height);
    if (aspectScale < 1.0f)
      yScale = yScale / aspectScale;
    else
      xScale = xScale * aspectScale;
    xScale = xScale / scaleX;
    yScale = yScale / scaleY;
    int     xScale_i = int((1.0f / xScale) + 0.5f);
    int     yScale_i = int((1.0f / yScale) + 0.5f);
    xScale_i = (xScale_i > 1 ? xScale_i : 1);
    yScale_i = (yScale_i > 1 ? yScale_i : 1);
    xScale = 1.0f / float(xScale_i);
    yScale = 1.0f / float(yScale_i);
    float   xOffs = (320.0f * 0.5f) - (float(width) * 0.5f * xScale);
    float   yOffs = (200.0f * 0.5f) - (float(height) * 0.5f * yScale);
    xOffs = xOffs - (offsetX * xScale);
    yOffs = yOffs - (offsetY * yScale);
    int     xOffs_i = int(xOffs + (xOffs >= 0.0f ? 0.5f : -0.5f));
    int     yOffs_i = int(yOffs + (yOffs >= 0.0f ? 0.5f : -0.5f));
    // scale image to the specified width and height
    float   borderY = borderColorY;
    borderY = (borderY > 0.0f ? (borderY < 1.0f ? borderY : 1.0f) : 0.0f);
    borderY = float(std::pow(borderY, monitorGamma));
    float   borderU = borderColorU;
    float   borderV = borderColorV;
    for (int yc = 0; yc < height; yc++) {
      if (!setProgressPercentage(yc * 100 / height)) {
        for (int tmpY = 0; tmpY < height; tmpY++) {
          for (int tmpX = 0; tmpX < width; tmpX++) {
            storePixelFunc(storePixelFuncUserData, tmpX, tmpY,
                           borderY, borderU, borderV);
          }
        }
        progressMessage("");
        return false;
      }
      int     yi = (yc / yScale_i) + yOffs_i;
      for (int xc = 0; xc < width; xc++) {
        int     xi = (xc / xScale_i) + xOffs_i;
        float   y = borderY;
        float   u = borderU;
        float   v = borderV;
        if (xi >= 0 && xi < 320 && yi >= 0 && yi < 200) {
          FLIConverter::convertPlus4Color(imageData[(yi * 320) + xi], y, u, v,
                                          monitorGamma);
        }
        y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
        u = (u > -0.436f ? (u < 0.436f ? u : 0.436f) : -0.436f);
        v = (v > -0.615f ? (v < 0.615f ? v : 0.615f) : -0.615f);
        storePixelFunc(storePixelFuncUserData, xc, yc, y, u, v);
      }
    }
    setProgressPercentage(100);
    progressMessage("");
    progressMessage("Loaded 320x200 image");
    return true;
  }

  bool YUVImageConverter::isPlus4Colormap(
      const std::vector< uint32_t >& colorMap)
  {
    size_t  nColors = 0;
    for (size_t i = 0; i < colorMap.size(); i++) {
      if (colorMap[i] < 0x80000000U)
        nColors++;
    }
    if (!(nColors == 16 || (nColors >= 98 && nColors <= 512)))
      return false;
    bool    isC64Palette = (nColors == 16);
    double  totalError = 0.0;
    size_t  n = 0;
    for (size_t i = 0; i < colorMap.size(); i++) {
      uint32_t  c = colorMap[i];
      if (c >= 0x80000000U)     // ignore transparent color
        continue;
      float   r = float(int((c >> 16) & 0xFFU)) * (1.0f / 255.0f);
      float   g = float(int((c >> 8) & 0xFFU)) * (1.0f / 255.0f);
      float   b = float(int(c & 0xFFU)) * (1.0f / 255.0f);
      float   y = 0.0f;
      float   u = 0.0f;
      float   v = 0.0f;
      rgbToYUV(y, u, v, r, g, b);
      float   tmp = float(std::sqrt(double(u * u) + double(v * v)));
      if (tmp > 0.000001f) {
        tmp = float(std::sqrt(double(FLIConverter::defaultColorSaturation
                                     / tmp)));
        u *= tmp;
        v *= tmp;
      }
      float   y_ = 0.0f;
      float   u_ = 0.0f;
      float   v_ = 0.0f;
      if (!isC64Palette) {
        FLIConverter::convertPlus4Color(int(n & 0x7F), y_, u_, v_, 1.0);
      }
      else {
        float   r_ = float(int(c64Palette[(n & 0x0F) * 3 + 0])) / 255.0f;
        float   g_ = float(int(c64Palette[(n & 0x0F) * 3 + 1])) / 255.0f;
        float   b_ = float(int(c64Palette[(n & 0x0F) * 3 + 2])) / 255.0f;
        rgbToYUV(y_, u_, v_, r_, g_, b_);
      }
      totalError += calculateYUVErrorSqr(y, u, v, y_, u_, v_, 0.5);
      n++;
    }
    totalError = totalError / double(int(nColors));
    return (totalError < 0.015);
  }

  bool YUVImageConverter::convertPlus4ColormapImage(
      const std::vector< uint16_t >& pixelBuf,
      const std::vector< uint32_t >& colorMap, int w, int h)
  {
    progressMessage("Resizing image");
    setProgressPercentage(0);
    // convert colormap
    size_t  nColors = 0;
    for (size_t i = 0; i < colorMap.size(); i++) {
      if (colorMap[i] < 0x80000000U)
        nColors++;
    }
    std::vector< float >  paletteY;
    std::vector< float >  paletteU;
    std::vector< float >  paletteV;
    paletteY.resize(colorMap.size());
    paletteU.resize(colorMap.size());
    paletteV.resize(colorMap.size());
    float   borderY = borderColorY;
    borderY = (borderY > 0.0f ? (borderY < 1.0f ? borderY : 1.0f) : 0.0f);
    borderY = float(std::pow(borderY, monitorGamma));
    float   borderU = borderColorU;
    float   borderV = borderColorV;
    bool    isC64Palette = (nColors == 16);
    size_t  n = 0;
    for (size_t i = 0; i < colorMap.size(); i++) {
      paletteY[i] = borderY;    // replace transparent pixels with border color
      paletteU[i] = borderU;
      paletteV[i] = borderV;
      uint32_t  c = colorMap[i];
      if (c < 0x80000000U) {
        int     tmp = int(n & 0x7F);
        if (isC64Palette)
          tmp = c64ColorTable[tmp & 0x0F];
        FLIConverter::convertPlus4Color(tmp,
                                        paletteY[i], paletteU[i], paletteV[i],
                                        monitorGamma);
        n++;
      }
    }
    // calculate scale and offset
    float   aspectScale = (float(width) * pixelAspectRatio / float(height))
                          / (float(w) / float(h));
    float   xScale = float(w) / float(width);
    float   yScale = float(h) / float(height);
    if (aspectScale < 1.0f)
      yScale = yScale / aspectScale;
    else
      xScale = xScale * aspectScale;
    xScale = xScale / scaleX;
    yScale = yScale / scaleY;
    int     xScale_i = int((1.0f / xScale) + 0.5f);
    int     yScale_i = int((1.0f / yScale) + 0.5f);
    xScale_i = (xScale_i > 1 ? xScale_i : 1);
    yScale_i = (yScale_i > 1 ? yScale_i : 1);
    xScale = 1.0f / float(xScale_i);
    yScale = 1.0f / float(yScale_i);
    float   xOffs = (float(w) * 0.5f) - (float(width) * 0.5f * xScale);
    float   yOffs = (float(h) * 0.5f) - (float(height) * 0.5f * yScale);
    xOffs = xOffs - (offsetX * xScale);
    yOffs = yOffs - (offsetY * yScale);
    int     xOffs_i = int(xOffs + (xOffs >= 0.0f ? 0.5f : -0.5f));
    int     yOffs_i = int(yOffs + (yOffs >= 0.0f ? 0.5f : -0.5f));
    // scale image to the specified width and height
    for (int yc = 0; yc < height; yc++) {
      if (!setProgressPercentage(yc * 100 / height)) {
        for (int tmpY = 0; tmpY < height; tmpY++) {
          for (int tmpX = 0; tmpX < width; tmpX++) {
            storePixelFunc(storePixelFuncUserData, tmpX, tmpY,
                           borderY, borderU, borderV);
          }
        }
        progressMessage("");
        return false;
      }
      int     yi = (yc / yScale_i) + yOffs_i;
      for (int xc = 0; xc < width; xc++) {
        int     xi = (xc / xScale_i) + xOffs_i;
        float   y = borderY;
        float   u = borderU;
        float   v = borderV;
        if (xi >= 0 && xi < w && yi >= 0 && yi < h) {
          int     c = pixelBuf[(yi * w) + xi];
          y = paletteY[c];
          u = paletteU[c];
          v = paletteV[c];
        }
        y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
        u = (u > -0.436f ? (u < 0.436f ? u : 0.436f) : -0.436f);
        v = (v > -0.615f ? (v < 0.615f ? v : 0.615f) : -0.615f);
        storePixelFunc(storePixelFuncUserData, xc, yc, y, u, v);
      }
    }
    setProgressPercentage(100);
    progressMessage("");
    char    tmpBuf[64];
    std::sprintf(&(tmpBuf[0]), "Loaded %dx%d image", w, h);
    progressMessage(&(tmpBuf[0]));
    return true;
  }

  bool YUVImageConverter::convertImageFile(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid image file name");
    if (isC64ImageFile(fileName))
      return convertC64ImageFile(fileName);
    float     *windowX = (float *) 0;
    float     *windowY = (float *) 0;
    float     *inputImage = (float *) 0;
    Fl_Shared_Image *f = Fl_Shared_Image::get(fileName);
    if (!f)
      throw Plus4Emu::Exception("error opening image file");
    try {
      int     cnt = f->count();
      int     d = f->d();
      size_t  w = size_t(f->w());
      size_t  h = size_t(f->h());
      const char  *p = (char *) 0;
      std::vector< uint16_t > pixelBuf;
      std::vector< uint32_t > palette;
      // read input image, and convert it to YUV format
      if (d == 1 && cnt > 2) {
        // colormap format
        readColormapImage(pixelBuf, palette, *f);
        f->release();
        f = (Fl_Shared_Image *) 0;
        if (isPlus4Colormap(palette))
          return convertPlus4ColormapImage(pixelBuf, palette, int(w), int(h));
      }
      else {
        // RGB or greyscale format
        if (cnt == 1)
          p = f->data()[0];
        if ((d < 1 || d > 4) || p == (char *) 0)
          throw Plus4Emu::Exception("image format is not supported");
      }
      if (w < 32 || w > 8192 || h < 32 || h > 6144)
        throw Plus4Emu::Exception("image size is out of range");
      progressMessage("Resizing image");
      inputImage = new float[w * h * 3];
      bool    haveAlpha = !(d & 1);
      float   borderY = borderColorY;
      borderY = (borderY > 0.0f ? (borderY < 1.0f ? borderY : 1.0f) : 0.0f);
      borderY = float(std::pow(borderY, monitorGamma));
      float   borderU = borderColorU;
      float   borderV = borderColorV;
      float   yGamma = monitorGamma / gammaCorrection;
      for (size_t yc = 0; yc < h; yc++) {
        if (!setProgressPercentage(int(yc) * (interpolationEnabled ? 50 : 90)
                                   / int(h))) {
          for (int tmpY = 0; tmpY < height; tmpY++) {
            for (int tmpX = 0; tmpX < width; tmpX++) {
              storePixelFunc(storePixelFuncUserData, tmpX, tmpY,
                             borderY, borderU, borderV);
            }
          }
          delete[] inputImage;
          progressMessage("");
          return false;
        }
        for (size_t xc = 0; xc < w; xc++) {
          float   r = 0.0f;
          float   g = 0.0f;
          float   b = 0.0f;
          const char  *pixelPtr = (char *) 0;
          if (p) {
            // RGB or greyscale format
            pixelPtr = &(p[((yc * w) + xc) * size_t(d)]);
            if (d < 3) {
              r = float((unsigned char) pixelPtr[0]) * (1.0f / 255.0f);
              g = r;
              b = r;
            }
            else {
              r = float((unsigned char) pixelPtr[0]) * (1.0f / 255.0f);
              g = float((unsigned char) pixelPtr[1]) * (1.0f / 255.0f);
              b = float((unsigned char) pixelPtr[2]) * (1.0f / 255.0f);
            }
          }
          else {
            // colormap format
            uint32_t  tmp = palette[pixelBuf[(yc * w) + xc]];
            r = float(int((tmp >> 16) & 0xFFU)) * (1.0f / 255.0f);
            g = float(int((tmp >> 8) & 0xFFU)) * (1.0f / 255.0f);
            b = float(int(tmp & 0xFFU)) * (1.0f / 255.0f);
            haveAlpha = (tmp >= 0x80000000U);
          }
          float   y = 0.0f;
          float   u = 0.0f;
          float   v = 0.0f;
          rgbToYUV(y, u, v, r, g, b);
          u *= (colorSaturationMult * (yMax - yMin));
          v *= (colorSaturationMult * (yMax - yMin));
          y = (y * (yMax - yMin)) + yMin;
          double  c = std::sqrt((u * u) + (v * v));
          if (double(y) < (c - 0.05) || double(y) > (1.05 - c)) {
            double  tmp = 0.5 / c;
            if (double(y) < (c - 0.05))
              tmp = tmp * (double(y) + c + 0.05);
            else
              tmp = tmp * ((1.05 + c) - double(y));
            if (tmp < 0.0) {
              u = 0.0f;
              v = 0.0f;
              c = 0.0;
            }
            else {
              u = u * float(tmp);
              v = v * float(tmp);
              c = c * tmp;
            }
          }
          y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
          y = float(std::pow(y, yGamma));
          if (c > 0.000001) {
            c = c * (1.0 / double(FLIConverter::defaultColorSaturation));
            c = std::pow(c, double(colorSaturationPow)) / c;
            u = float(c * u);
            v = float(c * v);
          }
          if (haveAlpha) {
            float   a = 0.0f;
            if (pixelPtr)
              a = float((unsigned char) pixelPtr[d - 1]) * (1.0f / 255.0f);
            y = (y * a) + (borderY * (1.0f - a));
            u = (u * a) + (borderU * (1.0f - a));
            v = (v * a) + (borderV * (1.0f - a));
          }
          float   *ptr = &(inputImage[((yc * w) + xc) * 3]);
          ptr[0] = y;
          ptr[1] = u;
          ptr[2] = v;
        }
      }
      if (f) {
        f->release();
        f = (Fl_Shared_Image *) 0;
      }
      // calculate X and Y scale
      float   aspectScale = (float(width) * pixelAspectRatio / float(height))
                            / (float(int(w)) / float(int(h)));
      float   xScale = float(int(w)) / float(width);
      float   yScale = float(int(h)) / float(height);
      if (aspectScale < 1.0f)
        yScale = yScale / aspectScale;
      else
        xScale = xScale * aspectScale;
      xScale = xScale / scaleX;
      yScale = yScale / scaleY;
      if (!interpolationEnabled) {
        // ---- resize image by integer ratio without interpolation ----
        int     xScale_i = int((1.0f / xScale) + 0.5f);
        int     yScale_i = int((1.0f / yScale) + 0.5f);
        xScale_i = (xScale_i > 1 ? xScale_i : 1);
        yScale_i = (yScale_i > 1 ? yScale_i : 1);
        xScale = 1.0f / float(xScale_i);
        yScale = 1.0f / float(yScale_i);
        float   xOffs =
            (float(int(w)) * 0.5f) - (float(width) * 0.5f * xScale);
        float   yOffs =
            (float(int(h)) * 0.5f) - (float(height) * 0.5f * yScale);
        xOffs = xOffs - (offsetX * xScale);
        yOffs = yOffs - (offsetY * yScale);
        int     xOffs_i = int(xOffs + (xOffs >= 0.0f ? 0.5f : -0.5f));
        int     yOffs_i = int(yOffs + (yOffs >= 0.0f ? 0.5f : -0.5f));
        // scale image to the specified width and height
        for (int yc = 0; yc < height; yc++) {
          if (!setProgressPercentage((yc * 10 / height) + 90)) {
            for (int tmpY = 0; tmpY < height; tmpY++) {
              for (int tmpX = 0; tmpX < width; tmpX++) {
                storePixelFunc(storePixelFuncUserData, tmpX, tmpY,
                               borderY, borderU, borderV);
              }
            }
            progressMessage("");
            return false;
          }
          int     yi = (yc / yScale_i) + yOffs_i;
          for (int xc = 0; xc < width; xc++) {
            int     xi = (xc / xScale_i) + xOffs_i;
            float   y = borderY;
            float   u = borderU;
            float   v = borderV;
            if (xi >= 0 && xi < int(w) && yi >= 0 && yi < int(h)) {
              float   *ptr = &(inputImage[(yi * int(w) + xi) * 3]);
              y = ptr[0];
              u = ptr[1];
              v = ptr[2];
            }
            y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
            u = (u > -0.436f ? (u < 0.436f ? u : 0.436f) : -0.436f);
            v = (v > -0.615f ? (v < 0.615f ? v : 0.615f) : -0.615f);
            storePixelFunc(storePixelFuncUserData, xc, yc, y, u, v);
          }
        }
      }
      else {
        // ---- resize image with interpolation and anti-aliasing ----
        float   xOffs =
            (float(int(w)) * 0.5f) - (float(width) * 0.5f * xScale);
        float   yOffs =
            (float(int(h)) * 0.5f) - (float(height) * 0.5f * yScale);
        xOffs = xOffs - (offsetX * xScale);
        yOffs = yOffs - (offsetY * yScale);
        // initialize interpolation window
        windowX = new float[1025];
        windowY = new float[1025];
        for (int x = 0; x < 1025; x++) {
          double  xf = double(x - 512) * (3.14159265 / 1024.0);
          double  wx = std::cos(xf);
          wx = wx * wx;
          float   xs = (xScale <= 1.0f ? 1.0f : (1.0f / xScale));
          xs = (xs > 0.2f ? xs : 0.2f);
          xf = xf * 16.0 * xs;
          if (xf < -0.000001 || xf > 0.000001)
            wx = wx * std::sin(xf) / xf;
          windowX[x] = float(wx * xs);
        }
        for (int y = 0; y < 1025; y++) {
          double  yf = double(y - 512) * (3.14159265 / 1024.0);
          double  wy = std::cos(yf);
          wy = wy * wy;
          float   ys = (yScale <= 1.0f ? 1.0f : (1.0f / yScale));
          ys = (ys > 0.2f ? ys : 0.2f);
          yf = yf * 16.0 * ys;
          if (yf < -0.000001 || yf > 0.000001)
            wy = wy * std::sin(yf) / yf;
          windowY[y] = float(wy * ys);
        }
        // scale image to the specified width and height
        for (int yc = 0; yc < height; yc++) {
          if (!setProgressPercentage((yc * 50 / height) + 50)) {
            for (int tmpY = 0; tmpY < height; tmpY++) {
              for (int tmpX = 0; tmpX < width; tmpX++) {
                storePixelFunc(storePixelFuncUserData, tmpX, tmpY,
                               borderY, borderU, borderV);
              }
            }
            delete[] windowX;
            delete[] windowY;
            delete[] inputImage;
            progressMessage("");
            return false;
          }
          double  yf = double(yc) * yScale + yOffs;
          int     yi = int(yf);
          yf = yf - double(yi);
          if (yf < 0.0) {
            yf += 1.0;
            yi--;
          }
          for (int xc = 0; xc < width; xc++) {
            double  xf = double(xc) * xScale + xOffs;
            int     xi = int(xf);
            xf = xf - double(xi);
            if (xf < 0.0) {
              xf += 1.0;
              xi--;
            }
            double  wxf = 63.999999 * (1.0 - xf);
            double  wyf = 63.999999 * (1.0 - yf);
            int     wxi = int(wxf);
            wxf = wxf - double(wxi);
            int     wyi = int(wyf);
            wyf = wyf - double(wyi);
            float   xs0 = float(1.0 - wxf);
            float   xs1 = float(wxf);
            float   ys0 = float(1.0 - wyf);
            float   ys1 = float(wyf);
            float   y = 0.0f;
            float   u = 0.0f;
            float   v = 0.0f;
            if (xi >= 7 && xi < int(w - 8) && yi >= 7 && yi < int(h - 8)) {
              // faster code for the case when no pixels are clipped
              float   *ptr =
                  &(inputImage[(((yi - 7) * int(w)) + (xi - 7)) * 3]);
              for (int wy = -7; wy <= 8; wy++) {
                float   wsy = (windowY[wyi] * ys0) + (windowY[wyi + 1] * ys1);
                for (int wx = -7; wx <= 8; wx++) {
                  float   wsx = (windowX[wxi] * xs0) + (windowX[wxi + 1] * xs1);
                  float   w_ = wsx * wsy;
                  y += (ptr[0] * w_);
                  u += (ptr[1] * w_);
                  v += (ptr[2] * w_);
                  wxi = wxi + 64;
                  ptr = ptr + 3;
                }
                wxi = wxi - 1024;
                wyi = wyi + 64;
                ptr = ptr + ((int(w) - 16) * 3);
              }
            }
            else if (xi < -1 || xi > int(w) || yi < -1 || yi > int(h)) {
              y = borderY;
              u = borderU;
              v = borderV;
            }
            else {
              for (int wy = -7; wy <= 8; wy++) {
                float   wsy = (windowY[wyi] * ys0) + (windowY[wyi + 1] * ys1);
                for (int wx = -7; wx <= 8; wx++) {
                  int     x_ = xi + wx;
                  int     y_ = yi + wy;
                  float   wsx = (windowX[wxi] * xs0) + (windowX[wxi + 1] * xs1);
                  float   w_ = wsx * wsy;
                  if (x_ < 0 || x_ >= int(w) || y_ < 0 || y_ >= int(h)) {
                    y += (borderY * w_);
                    u += (borderU * w_);
                    v += (borderV * w_);
                  }
                  else {
                    float   *ptr = &(inputImage[((y_ * int(w)) + x_) * 3]);
                    y += (ptr[0] * w_);
                    u += (ptr[1] * w_);
                    v += (ptr[2] * w_);
                  }
                  wxi = wxi + 64;
                }
                wxi = wxi - 1024;
                wyi = wyi + 64;
              }
            }
            y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
            u = (u > -0.436f ? (u < 0.436f ? u : 0.436f) : -0.436f);
            v = (v > -0.615f ? (v < 0.615f ? v : 0.615f) : -0.615f);
            storePixelFunc(storePixelFuncUserData, xc, yc, y, u, v);
          }
        }
      }
      delete[] windowX;
      delete[] windowY;
      delete[] inputImage;
      setProgressPercentage(100);
      progressMessage("");
      char    tmpBuf[64];
      std::sprintf(&(tmpBuf[0]), "Loaded %dx%d image", int(w), int(h));
      progressMessage(&(tmpBuf[0]));
    }
    catch (...) {
      if (windowX)
        delete[] windowX;
      if (windowY)
        delete[] windowY;
      if (inputImage)
        delete[] inputImage;
      if (f)
        f->release();
      progressMessage("");
      throw;
    }
    return true;
  }

  void YUVImageConverter::setProgressMessageCallback(
      void (*func)(void *userData, const char *msg), void *userData_)
  {
    if (func) {
      progressMessageCallback = func;
      progressMessageUserData = userData_;
    }
    else {
      progressMessageCallback = &defaultProgressMessageCb;
      progressMessageUserData = (void *) 0;
    }
  }

  void YUVImageConverter::setProgressPercentageCallback(
      bool (*func)(void *userData, int n), void *userData_)
  {
    if (func) {
      progressPercentageCallback = func;
      progressPercentageUserData = userData_;
    }
    else {
      progressPercentageCallback = &defaultProgressPercentageCb;
      progressPercentageUserData = (void *) 0;
    }
  }

  void YUVImageConverter::progressMessage(const char *msg)
  {
    if (msg == (char *) 0)
      msg = "";
    progressMessageCallback(progressMessageUserData, msg);
  }

  bool YUVImageConverter::setProgressPercentage(int n)
  {
    limitValue(n, 0, 100);
    if (n != prvProgressPercentage) {
      prvProgressPercentage = n;
      return progressPercentageCallback(progressPercentageUserData, n);
    }
    return true;
  }

}       // namespace Plus4FLIConv

