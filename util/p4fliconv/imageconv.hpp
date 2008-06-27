
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

#ifndef P4FLICONV_IMAGECONV_HPP
#define P4FLICONV_IMAGECONV_HPP

#include "p4fliconv.hpp"

namespace Plus4FLIConv {

  class YUVImageConverter {
   private:
    int     width;
    int     height;
    float   pixelAspectRatio;
    float   scaleX;
    float   scaleY;
    float   offsetX;
    float   offsetY;
    float   gammaCorrection;
    float   monitorGamma;
    float   yMin;
    float   yMax;
    float   colorSaturationMult;
    float   colorSaturationPow;
    float   borderColorY;
    float   borderColorU;
    float   borderColorV;
    unsigned char c64ColorTable[16];
    void    (*storePixelFunc)(void *userData, int xc, int yc,
                              float y, float u, float v);
    void    *storePixelFuncUserData;
    void    (*progressMessageCallback)(void *userData, const char *msg);
    void    *progressMessageUserData;
    bool    (*progressPercentageCallback)(void *userData, int n);
    void    *progressPercentageUserData;
    int     prvProgressPercentage;
    static void defaultStorePixelFunc(void *userData, int xc, int yc,
                                      float y, float u, float v);
    static bool isC64ImageFile(const char *fileName);
    bool convertC64ImageFile(const char *fileName);
    static bool isPlus4Colormap(const std::vector< uint32_t >& colorMap);
    bool convertPlus4ColormapImage(const std::vector< uint16_t >& pixelBuf,
                                   const std::vector< uint32_t >& colorMap,
                                   int w, int h);
   public:
    YUVImageConverter();
    virtual ~YUVImageConverter();
    // the return value is false if the processing has been stopped
    bool convertImageFile(const char *fileName);
    inline void setImageSize(int w, int h)
    {
      width = w;
      height = h;
    }
    inline void setPixelAspectRatio(float r)
    {
      pixelAspectRatio = r;
    }
    inline void setXYScaleAndOffset(float xs, float ys, float xo, float yo)
    {
      scaleX = xs;
      scaleY = ys;
      offsetX = xo;
      offsetY = yo;
    }
    inline void setGammaCorrection(float gammaCorrection_, float monitorGamma_)
    {
      gammaCorrection = gammaCorrection_;
      monitorGamma = monitorGamma_;
    }
    inline void setLuminanceRange(float minVal, float maxVal)
    {
      yMin = minVal;
      yMax = maxVal;
    }
    inline void setColorSaturation(float m, float p)
    {
      colorSaturationMult = m;
      colorSaturationPow = p;
    }
    inline void setBorderColor(float y, float u, float v)
    {
      borderColorY = y;
      borderColorU = u;
      borderColorV = v;
    }
    inline void setC64Color(int colorIndex, int plus4ColorCode)
    {
      c64ColorTable[colorIndex & 15] = (unsigned char) (plus4ColorCode & 0x7F);
    }
    inline void setPixelStoreCallback(void (*func)(void *userData,
                                                   int xc, int yc,
                                                   float y, float u, float v),
                                      void *userData_)
    {
      storePixelFunc = func;
      storePixelFuncUserData = userData_;
    }
    void setProgressMessageCallback(void (*func)(void *userData,
                                                 const char *msg),
                                    void *userData_);
    void setProgressPercentageCallback(bool (*func)(void *userData, int n),
                                       void *userData_);
   protected:
    void progressMessage(const char *msg);
    bool setProgressPercentage(int n);
  };

}       // namespace Plus4FLIConv

#endif  // P4FLICONV_IMAGECONV_HPP

