
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

#ifndef PLUS4EMU_DISPLAY_HPP
#define PLUS4EMU_DISPLAY_HPP

#include "plus4emu.hpp"

namespace Plus4Emu {

  class VideoDisplay {
   public:
    class DisplayParameters {
     public:
      // 0: full horizontal resolution, no interlace (768x288),
      //    no texture filtering, no blend effects
      // 1: half horizontal resolution, no interlace (384x288)
      // 2: full horizontal resolution, no interlace (768x288)
      // 3: full horizontal resolution, interlace (768x576)
      int     displayQuality;
      // 0: single buffered display
      // 1: double buffered display
      // 2: double buffered display with resampling to monitor refresh rate
      int     bufferingMode;
      // function to convert 8-bit color indices to red, green, and blue
      // levels (in the range 0.0 to 1.0); if NULL, greyscale is assumed
      void    (*indexToRGBFunc)(uint8_t color,
                                float& red, float& green, float& blue);
      // brightness (default: 0.0)
      double  brightness;
      // contrast (default: 1.0)
      double  contrast;
      // gamma (default: 1.0, higher values result in a brighter display)
      double  gamma;
      // color saturation (default: 1.0)
      double  saturation;
      // brightness for red channel
      double  redBrightness;
      // contrast for red channel
      double  redContrast;
      // gamma for red channel
      double  redGamma;
      // brightness for green channel
      double  greenBrightness;
      // contrast for green channel
      double  greenContrast;
      // gamma for green channel
      double  greenGamma;
      // brightness for blue channel
      double  blueBrightness;
      // contrast for blue channel
      double  blueContrast;
      // gamma for blue channel
      double  blueGamma;
      // controls vertical filtering of textures (0 to 0.5)
      double  blendScale1;
      // scale applied to new pixels written to frame buffer
      double  blendScale2;
      // scale applied to old pixels in frame buffer
      double  blendScale3;
      // pixel aspect ratio to assume
      // (calculated as (screen_width / screen_height) / (X_res / Y_res))
      double  pixelAspectRatio;
     private:
      static void defaultIndexToRGBFunc(uint8_t color,
                                        float& red, float& green, float& blue);
      void copyDisplayParameters(const DisplayParameters& src);
     public:
      DisplayParameters();
      DisplayParameters(const DisplayParameters& dp);
      DisplayParameters& operator=(const DisplayParameters& dp);
      void applyColorCorrection(float& red, float& green, float& blue) const;
    };
    // ----------------
    VideoDisplay()
    {
    }
    virtual ~VideoDisplay();
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    virtual void setDisplayParameters(const DisplayParameters& dp) = 0;
    virtual const DisplayParameters& getDisplayParameters() const = 0;
    // Draw next line of display. 'nBytes' should be 768 for full resolution,
    // or 384 for half resolution. With values in the range 0 to 383, or
    // 385 to 767, the remaining pixels are filled with color 0.
    virtual void drawLine(const uint8_t *buf, size_t nBytes) = 0;
    // Should be called at the beginning (newState = true) and end
    // (newState = false) of VSYNC. 'currentSlot_' is the position within
    // the current line (0 to 56).
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_) = 0;
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_DISPLAY_HPP

