
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

#ifndef PLUS4EMU_EMUCFG_HPP
#define PLUS4EMU_EMUCFG_HPP

#include "plus4emu.hpp"
#include "cfg_db.hpp"
#include "display.hpp"
#include "soundio.hpp"
#include "vm.hpp"

#include <map>

namespace Plus4Emu {

  class EmulatorConfiguration : public ConfigurationDB {
   private:
    VirtualMachine& vm_;
    VideoDisplay&   videoDisplay;
    AudioOutput&    audioOutput;
    std::map< int, int >  keyboardMap;
    void            (*errorCallback)(void *, const char *);
    void            *errorCallbackUserData;
   public:
    struct {
      unsigned int  cpuClockFrequency;
      unsigned int  videoClockFrequency;
      bool          enableFileIO;
    } vm;
    bool          vmConfigurationChanged;
    // --------
    struct {
      struct {
        int       size;
      } ram;
      struct ROMSegmentConfig {
        std::string file;
        int         offset;
        ROMSegmentConfig()
          : file(""),
            offset(0)
        {
        }
      };
      // ROM files can be loaded to segments 0x00 to 0x07, 0x10 to 0x13,
      // 0x20 to 0x23, and 0x30 to 0x33
      ROMSegmentConfig  rom[52];
    } memory;
    bool          memoryConfigurationChanged;
    // --------
    struct {
      bool        enabled;
      int         bufferingMode;
      int         quality;
      double      brightness;
      double      contrast;
      double      gamma;
      double      saturation;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } red;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } green;
      struct {
        double    brightness;
        double    contrast;
        double    gamma;
      } blue;
      struct {
        double    param1;
        double    param2;
        double    param3;
      } effects;
      int         width;
      int         height;
      double      pixelAspectRatio;
    } display;
    bool          displaySettingsChanged;
    // --------
    struct {
      bool        enabled;
      bool        highQuality;
      int         device;
      double      sampleRate;
      double      latency;
      int         hwPeriods;
      int         swPeriods;
      std::string file;
      double      volume;
      double      dcBlockFilter1Freq;
      double      dcBlockFilter2Freq;
      struct {
        int       mode;
        double    frequency;
        double    level;
        double    q;
      } equalizer;
    } sound;
    bool          soundSettingsChanged;
    // --------
    int           keyboard[128][2];
    bool          keyboardMapChanged;
    // --------
    struct FloppyDriveSettings {
      std::string imageFile;
      int         tracks;
      int         sides;
      int         sectorsPerTrack;
      FloppyDriveSettings()
        : imageFile(""),
          tracks(-1),
          sides(2),
          sectorsPerTrack(9)
      {
      }
    };
    struct {
      FloppyDriveSettings a;
      FloppyDriveSettings b;
      FloppyDriveSettings c;
      FloppyDriveSettings d;
    } floppy;
    bool          floppyAChanged;
    bool          floppyBChanged;
    bool          floppyCChanged;
    bool          floppyDChanged;
    // --------
    struct {
      std::string imageFile;
      int         defaultSampleRate;
      bool        fastMode;
    } tape;
    bool          tapeSettingsChanged;
    bool          tapeDefaultSampleRateChanged;
    bool          fastTapeModeChanged;
    // --------
    struct {
      std::string workingDirectory;
    } fileio;
    bool          fileioSettingsChanged;
    // --------
    struct {
      int         bpPriorityThreshold;
      bool        noBreakOnDataRead;
    } debug;
    bool          debugSettingsChanged;
    // ----------------
    EmulatorConfiguration(VirtualMachine& vm__,
                          VideoDisplay& videoDisplay_,
                          AudioOutput& audioOutput_);
    virtual ~EmulatorConfiguration();
    void applySettings();
    int convertKeyCode(int keyCode);
    void setErrorCallback(void (*func)(void *userData, const char *msg),
                          void *userData_);
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_EMUCFG_HPP

