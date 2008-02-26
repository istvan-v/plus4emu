
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
#include "imageconv.hpp"

#include <vector>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

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

namespace Plus4FLIConv {

  void YUVImageConverter::defaultStorePixelFunc(void *userData, int xc, int yc,
                                                float y, float u, float v)
  {
    (void) userData;
    (void) xc;
    (void) yc;
    float   r = (v / 0.877f) + y;
    float   b = (u / 0.492f) + y;
    float   g = (y - ((r * 0.299f) + (b * 0.114f))) / 0.587f;
    r = (r > 0.0f ? (r < 1.0f ? r : 1.0f) : 0.0f);
    g = (g > 0.0f ? (g < 1.0f ? g : 1.0f) : 0.0f);
    b = (b > 0.0f ? (b < 1.0f ? b : 1.0f) : 0.0f);
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
      prvProgressPercentage(-1)
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
    if (fileName == (char *) 0 || fileName[0] == '\0')
      return false;
    size_t  n = std::strlen(fileName);
    if (n < 4)
      return false;
    if (!(fileName[n - 4] == '.' &&
          (((fileName[n - 3] == 'K' || fileName[n - 3] == 'k') &&
            (fileName[n - 2] == 'O' || fileName[n - 2] == 'o') &&
            (fileName[n - 1] == 'A' || fileName[n - 1] == 'a')) ||
           ((fileName[n - 3] == 'O' || fileName[n - 3] == 'o') &&
            (fileName[n - 2] == 'C' || fileName[n - 2] == 'c') &&
            (fileName[n - 1] == 'P' || fileName[n - 1] == 'p'))))) {
      return false;
    }
    std::FILE *f = std::fopen(fileName, "rb");
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
    if ((fileName[n - 3] == 'K' || fileName[n - 3] == 'k') &&
        fileSize == 10003L && startAddr[1] == 0x60) {
      // Koala Painter format
      return true;
    }
    if ((fileName[n - 3] == 'O' || fileName[n - 3] == 'o') &&
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
    std::FILE *f = std::fopen(fileName, "rb");
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
      if (cnt == 1)
        p = f->data()[0];
      if ((d < 1 || d > 4) || (w < 32 || w > 8192) || (h < 32 || h > 6144) ||
          p == (char *) 0) {
        throw Plus4Emu::Exception("image format is not supported");
      }
      // read input image, and convert it to YUV format
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
        if (!setProgressPercentage(int(yc) * 50 / int(h))) {
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
          const char  *pixelPtr = &(p[((yc * w) + xc) * size_t(d)]);
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
          r = (r * (yMax - yMin)) + yMin;
          g = (g * (yMax - yMin)) + yMin;
          b = (b * (yMax - yMin)) + yMin;
          float   y = (r * 0.299f) + (g * 0.587f) + (b * 0.114f);
          float   u = (b - y) * 0.492f * colorSaturationMult;
          float   v = (r - y) * 0.877f * colorSaturationMult;
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
            float   a =
                float((unsigned char) pixelPtr[d - 1]) * (1.0f / 255.0f);
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
      f->release();
      f = (Fl_Shared_Image *) 0;
      // initialize interpolation window
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
      float   xOffs = (float(int(w)) * 0.5f) - (float(width) * 0.5f * xScale);
      float   yOffs = (float(int(h)) * 0.5f) - (float(height) * 0.5f * yScale);
      xOffs = xOffs - (offsetX * xScale);
      yOffs = yOffs - (offsetY * yScale);
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
            float   *ptr = &(inputImage[(((yi - 7) * int(w)) + (xi - 7)) * 3]);
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

