
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
      // 3: full horizontal resolution, interlace (768x576), TV emulation
      int     displayQuality;
      // 0: single buffered display
      // 1: double buffered display
      // 2: double buffered display with resampling to monitor refresh rate
      int     bufferingMode;
      // false: PAL (this is the default)
      // true:  NTSC
      bool    ntscMode;
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
    /*!
     * Set color correction and other display parameters.
     * (see 'struct DisplayParameters' above for more information)
     */
    virtual void setDisplayParameters(const DisplayParameters& dp) = 0;
    virtual const DisplayParameters& getDisplayParameters() const = 0;
    /*!
     * Read and process 'nBytes' bytes of video data from 'buf'. A group of
     * four pixels is encoded as a flags byte followed by 1 or 4 colormap
     * indices (in the first case, all four pixels have the same color).
     * The flags byte can be the sum of any of the following values:
     *   128: composite sync
     *    64: vertical sync
     *    32: horizontal blanking
     *    16: vertical blanking
     *     8: burst
     *     4: PAL even line
     *     2: number of data bytes: 0: 1 byte, 1: 4 bytes
     *     1: NTSC mode (dot clock multiplied by 1.25)
     */
    virtual void sendVideoOutput(const uint8_t *buf, size_t nBytes) = 0;
    /*!
     * If enabled, limit the number of frames displayed per second to a
     * maximum of 50.
     */
    virtual void limitFrameRate(bool isEnabled);
  };

  template <typename T>
  class VideoDisplayColormap {
   private:
    // 16x256 colormap entries:
    //   0x0000-0x00FF: normal colors (no phase shift)
    //   0x0100-0x01FF: +33 degrees phase shift (PAL input on NTSC display)
    //   0x0200-0x02FF: -33 degrees phase shift (NTSC input on PAL display)
    //   0x0300-0x03FF: invert phase
    //   0x0400-0x04FF: +33 degrees phase shift, invert phase
    //   0x0500-0x05FF: -33 degrees phase shift, invert phase
    //   0x0600-0x06FF: no colors (U = 0, V = 0)
    //   0x0700-0x07FF: no video (Y = 0, U = 0, V = 0)
    //   0x0800-0x08FF: PAL burst, normal colors (no phase shift)
    //   0x0900-0x09FF: PAL burst, +33 degrees phase shift
    //   0x0A00-0x0AFF: PAL burst, +33 degrees phase shift, invert phase
    //   0x0B00-0x0BFF: PAL burst, invert phase
    //   0x0C00-0x0CFF: NTSC burst, normal colors (no phase shift)
    //   0x0D00-0x0DFF: NTSC burst, -33 degrees phase shift
    //   0x0E00-0x0EFF: NTSC burst, -33 degrees phase shift, invert phase
    //   0x0F00-0x0FFF: NTSC burst, invert phase
    T       *colormapData;
    T       **colormapTable;
    static T pixelConv(float r, float g, float b);
   public:
    VideoDisplayColormap();
    ~VideoDisplayColormap();
    void setDisplayParameters(const VideoDisplay::DisplayParameters&
                                  displayParameters,
                              bool yuvFormat = false);
    // Read and convert four pixels of video data from 'inBuf', and store
    // the result in 'outBuf'. 'flags_' can be the sum of the following
    // values:
    //   0x02: PAL even line (invert color phase)
    //   0x10: NTSC mode
    //   0x20: have burst signal (if not, disable colors)
    // Returns the number of bytes read from 'inBuf'.
    inline size_t convertFourPixels(T *outBuf, const unsigned char *inBuf,
                                    unsigned char flags_) const
    {
      unsigned char c = inBuf[0];
      const T *colormap_ = colormapTable[(c & 0x8D) | flags_];
      if (c & 0x02) {
        outBuf[0] = colormap_[inBuf[1]];
        outBuf[1] = colormap_[inBuf[2]];
        outBuf[2] = colormap_[inBuf[3]];
        outBuf[3] = colormap_[inBuf[4]];
        return 5;
      }
      T       tmp = colormap_[inBuf[1]];
      outBuf[0] = tmp;
      outBuf[1] = tmp;
      outBuf[2] = tmp;
      outBuf[3] = tmp;
      return 2;
    }
    inline size_t convertFourToEightPixels(T *outBuf,
                                           const unsigned char *inBuf,
                                           unsigned char flags_) const
    {
      unsigned char c = inBuf[0];
      const T *colormap_ = colormapTable[(c & 0x8D) | flags_];
      if (c & 0x02) {
        T       tmp = colormap_[inBuf[1]];
        outBuf[0] = tmp;
        outBuf[1] = tmp;
        tmp = colormap_[inBuf[2]];
        outBuf[2] = tmp;
        outBuf[3] = tmp;
        tmp = colormap_[inBuf[3]];
        outBuf[4] = tmp;
        outBuf[5] = tmp;
        tmp = colormap_[inBuf[4]];
        outBuf[6] = tmp;
        outBuf[7] = tmp;
        return 5;
      }
      T       tmp = colormap_[inBuf[1]];
      outBuf[0] = tmp;
      outBuf[1] = tmp;
      outBuf[2] = tmp;
      outBuf[3] = tmp;
      outBuf[4] = tmp;
      outBuf[5] = tmp;
      outBuf[6] = tmp;
      outBuf[7] = tmp;
      return 2;
    }
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_DISPLAY_HPP

