
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
#include "fileio.hpp"
#include "system.hpp"
#include "cfg_db.hpp"
#include "mkcfg_fl.hpp"

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

static int keyboardMap_P4[256] = {
  0xFF08,     -1, 0xFF0D,     -1, 0xFFFF,     -1, 0xFFC1,     -1,
  0xFFBE,     -1, 0xFFBF,     -1, 0xFFC0,     -1, 0x002D,     -1,
  0x0033,     -1, 0x0077,     -1, 0x0061,     -1, 0x0034,     -1,
  0x007A,     -1, 0x0073,     -1, 0x0065,     -1, 0xFFE1, 0xFFE2,
  0x0035,     -1, 0x0072,     -1, 0x0064,     -1, 0x0036,     -1,
  0x0063,     -1, 0x0066,     -1, 0x0074,     -1, 0x0078,     -1,
  0x0037,     -1, 0x0079,     -1, 0x0067,     -1, 0x0038,     -1,
  0x0062,     -1, 0x0068,     -1, 0x0075,     -1, 0x0076,     -1,
  0x0039,     -1, 0x0069,     -1, 0x006A,     -1, 0x0030,     -1,
  0x006D,     -1, 0x006B,     -1, 0x006F,     -1, 0x006E,     -1,
  0xFF54,     -1, 0x0070,     -1, 0x006C,     -1, 0xFF52,     -1,
  0x002E,     -1, 0x003B,     -1, 0x005D, 0xFFAD, 0x002C,     -1,
  0xFF51,     -1, 0x005C, 0xFFAA, 0x0027,     -1, 0xFF53,     -1,
  0xFF1B, 0x0060, 0x003D,     -1, 0x005B,     -1, 0x002F,     -1,
  0x0031,     -1, 0xFF50,     -1, 0xFFE4,     -1, 0x0032,     -1,
  0x0020,     -1, 0xFFE3,     -1, 0x0071,     -1, 0xFF61, 0xFF09,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB8,     -1, 0xFFB2,     -1, 0xFFB4,     -1, 0xFFB6,     -1,
      -1,     -1,     -1,     -1,     -1,     -1, 0xFF8D, 0xFFB0,
  0xFFAF,     -1, 0xFFB5,     -1, 0xFFB7,     -1, 0xFFB9,     -1,
      -1,     -1,     -1,     -1, 0xFFAB,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};

#ifndef WIN32

static int keyboardMap_P4_HU[256] = {
  0xFF08,     -1, 0xFF0D,     -1, 0xFFFF,     -1, 0xFFC1,     -1,
  0xFFBE,     -1, 0xFFBF,     -1, 0xFFC0,     -1, 0x00FC,     -1,
  0x0033,     -1, 0x0077,     -1, 0x0061,     -1, 0x0034,     -1,
  0x0079,     -1, 0x0073,     -1, 0x0065,     -1, 0xFFE1, 0xFFE2,
  0x0035,     -1, 0x0072,     -1, 0x0064,     -1, 0x0036,     -1,
  0x0063,     -1, 0x0066,     -1, 0x0074,     -1, 0x0078,     -1,
  0x0037,     -1, 0x007A,     -1, 0x0067,     -1, 0x0038,     -1,
  0x0062,     -1, 0x0068,     -1, 0x0075,     -1, 0x0076,     -1,
  0x0039,     -1, 0x0069,     -1, 0x006A,     -1, 0x00F6,     -1,
  0x006D,     -1, 0x006B,     -1, 0x006F,     -1, 0x006E,     -1,
  0xFF54,     -1, 0x0070,     -1, 0x006C,     -1, 0xFF52,     -1,
  0x002E,     -1, 0x00E9,     -1, 0x00FA, 0xFFAD, 0x002C,     -1,
  0xFF51,     -1, 0x01FB, 0xFFAA, 0x00E1,     -1, 0xFF53,     -1,
  0xFF1B, 0x0030, 0x00F3,     -1, 0x01F5,     -1, 0x002D,     -1,
  0x0031,     -1, 0xFF50,     -1, 0xFFE4,     -1, 0x0032,     -1,
  0x0020,     -1, 0xFFE3,     -1, 0x0071,     -1, 0xFF61, 0xFF09,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB8,     -1, 0xFFB2,     -1, 0xFFB4,     -1, 0xFFB6,     -1,
      -1,     -1,     -1,     -1,     -1,     -1, 0xFF8D, 0xFFB0,
  0xFFAF,     -1, 0xFFB5,     -1, 0xFFB7,     -1, 0xFFB9,     -1,
      -1,     -1,     -1,     -1, 0xFFAB,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};

#else   // WIN32

static int keyboardMap_P4_HU[256] = {
  0xFF08,     -1, 0xFF0D,     -1, 0xFFFF,     -1, 0xFFC1,     -1,
  0xFFBE,     -1, 0xFFBF,     -1, 0xFFC0,     -1, 0x002F,     -1,
  0x0033,     -1, 0x0077,     -1, 0x0061,     -1, 0x0034,     -1,
  0x0079,     -1, 0x0073,     -1, 0x0065,     -1, 0xFFE1, 0xFFE2,
  0x0035,     -1, 0x0072,     -1, 0x0064,     -1, 0x0036,     -1,
  0x0063,     -1, 0x0066,     -1, 0x0074,     -1, 0x0078,     -1,
  0x0037,     -1, 0x007A,     -1, 0x0067,     -1, 0x0038,     -1,
  0x0062,     -1, 0x0068,     -1, 0x0075,     -1, 0x0076,     -1,
  0x0039,     -1, 0x0069,     -1, 0x006A,     -1, 0x0060,     -1,
  0x006D,     -1, 0x006B,     -1, 0x006F,     -1, 0x006E,     -1,
  0xFF54,     -1, 0x0070,     -1, 0x006C,     -1, 0xFF52,     -1,
  0x002E,     -1, 0x003B,     -1, 0x005D, 0xFFAD, 0x002C,     -1,
  0xFF51,     -1, 0x005C, 0xFFAA, 0x0027,     -1, 0xFF53,     -1,
  0xFF1B, 0x0030, 0x003D,     -1, 0x005B,     -1, 0x002D,     -1,
  0x0031,     -1, 0xFF50,     -1, 0xFFE4,     -1, 0x0032,     -1,
  0x0020,     -1, 0xFFE3,     -1, 0x0071,     -1, 0xFF61, 0xFF09,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  0xFFB8,     -1, 0xFFB2,     -1, 0xFFB4,     -1, 0xFFB6,     -1,
      -1,     -1,     -1,     -1,     -1,     -1, 0xFF8D, 0xFFB0,
  0xFFAF,     -1, 0xFFB5,     -1, 0xFFB7,     -1, 0xFFB9,     -1,
      -1,     -1,     -1,     -1, 0xFFAB,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
      -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};

#endif  // WIN32

static const char *keyboardConfigFileNames[8] = {
  (char *) 0,                   // 0
  (char *) 0,                   // 1
  (char *) 0,                   // 2
  (char *) 0,                   // 3
  "P4_Keyboard_US.cfg",         // 4
  "P4_Keyboard_HU.cfg",         // 5
  (char *) 0,                   // 6
  (char *) 0                    // 7
};

static void setKeyboardConfiguration(Plus4Emu::ConfigurationDB& config, int n)
{
  int     *keyboardMapPtr = &(keyboardMap_P4[0]);
  switch (n) {
  case 4:                               // Plus/4
    keyboardMapPtr = &(keyboardMap_P4[0]);
    break;
  case 5:                               // Plus/4 (HU)
    keyboardMapPtr = &(keyboardMap_P4_HU[0]);
    break;
  }
  for (unsigned int i = 0U; i < 256U; i++) {
    char    tmpBuf[16];
    std::sprintf(&(tmpBuf[0]), "keyboard.%02X.%X", (i >> 1), (i & 1U));
    config.createKey(std::string(&(tmpBuf[0])), keyboardMapPtr[i]);
  }
}

class Plus4EmuMachineConfiguration {
 private:
  struct {
    unsigned int  cpuClockFrequency;
    unsigned int  videoClockFrequency;
    unsigned int  soundClockFrequency;
    unsigned int  videoMemoryLatency;
    bool          enableMemoryTimingEmulation;
  } vm;
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
 public:
  Plus4EmuMachineConfiguration(Plus4Emu::ConfigurationDB& config, int n,
                               const std::string& romDirectory);
  ~Plus4EmuMachineConfiguration()
  {
  }
};

static const char *machineConfigFileNames[16] = {
  "P4_16k_PAL.cfg",                     // 0
  "P4_16k_NTSC.cfg",                    // 1
  "P4_64k_PAL.cfg",                     // 2
  "P4_64k_NTSC.cfg",                    // 3
  "P4_16k_PAL_3PLUS1.cfg",              // 4
  "P4_16k_NTSC_3PLUS1.cfg",             // 5
  "P4_64k_PAL_3PLUS1.cfg",              // 6
  "P4_64k_NTSC_3PLUS1.cfg",             // 7
  "P4_16k_PAL_FileIO.cfg",              // 8
  "P4_16k_NTSC_FileIO.cfg",             // 9
  "P4_64k_PAL_FileIO.cfg",              // 10
  "P4_64k_NTSC_FileIO.cfg",             // 11
  "P4_16k_PAL_3PLUS1_FileIO.cfg",       // 12
  "P4_16k_NTSC_3PLUS1_FileIO.cfg",      // 13
  "P4_64k_PAL_3PLUS1_FileIO.cfg",       // 14
  "P4_64k_NTSC_3PLUS1_FileIO.cfg"       // 15
};

Plus4EmuMachineConfiguration::Plus4EmuMachineConfiguration(
    Plus4Emu::ConfigurationDB& config, int n, const std::string& romDirectory)
{
  vm.cpuClockFrequency = 1U;
  vm.videoClockFrequency = ((n & 1) == 0 ? 17734475U : 14318180U);
  vm.soundClockFrequency = 0U;
  vm.videoMemoryLatency = 0U;
  vm.enableMemoryTimingEmulation = true;
  memory.ram.size = ((n & 2) == 0 ? 16 : 64);
  memory.rom[0x00].file = romDirectory + "p4_basic.rom";
  if ((n & 1) == 0)
    memory.rom[0x01].file = romDirectory + "p4kernal.rom";
  else
    memory.rom[0x01].file = romDirectory + "p4_ntsc.rom";
  if ((n & 4) != 0) {
    memory.rom[0x02].file = romDirectory + "3plus1lo.rom";
    memory.rom[0x03].file = romDirectory + "3plus1hi.rom";
  }
  if (n >= 8)
    memory.rom[0x06].file = romDirectory + "p4fileio.rom";
  memory.rom[0x10].file = romDirectory + "dos1541.rom";
  memory.rom[0x30].file = romDirectory + "dos1581.rom";
  memory.rom[0x31].file = romDirectory + "dos1581.rom";
  memory.rom[0x31].offset = 16384;
  config.createKey("vm.cpuClockFrequency", vm.cpuClockFrequency);
  config.createKey("vm.videoClockFrequency", vm.videoClockFrequency);
  config.createKey("vm.soundClockFrequency", vm.soundClockFrequency);
  config.createKey("vm.videoMemoryLatency", vm.videoMemoryLatency);
  config.createKey("vm.enableMemoryTimingEmulation",
                   vm.enableMemoryTimingEmulation);
  config.createKey("memory.ram.size", memory.ram.size);
  config.createKey("memory.rom.00.file", memory.rom[0x00].file);
  config.createKey("memory.rom.00.offset", memory.rom[0x00].offset);
  config.createKey("memory.rom.01.file", memory.rom[0x01].file);
  config.createKey("memory.rom.01.offset", memory.rom[0x01].offset);
  config.createKey("memory.rom.02.file", memory.rom[0x02].file);
  config.createKey("memory.rom.02.offset", memory.rom[0x02].offset);
  config.createKey("memory.rom.03.file", memory.rom[0x03].file);
  config.createKey("memory.rom.03.offset", memory.rom[0x03].offset);
  config.createKey("memory.rom.04.file", memory.rom[0x04].file);
  config.createKey("memory.rom.04.offset", memory.rom[0x04].offset);
  config.createKey("memory.rom.05.file", memory.rom[0x05].file);
  config.createKey("memory.rom.05.offset", memory.rom[0x05].offset);
  config.createKey("memory.rom.06.file", memory.rom[0x06].file);
  config.createKey("memory.rom.06.offset", memory.rom[0x06].offset);
  config.createKey("memory.rom.07.file", memory.rom[0x07].file);
  config.createKey("memory.rom.07.offset", memory.rom[0x07].offset);
  config.createKey("memory.rom.10.file", memory.rom[0x10].file);
  config.createKey("memory.rom.10.offset", memory.rom[0x10].offset);
  config.createKey("memory.rom.20.file", memory.rom[0x20].file);
  config.createKey("memory.rom.20.offset", memory.rom[0x20].offset);
  config.createKey("memory.rom.21.file", memory.rom[0x21].file);
  config.createKey("memory.rom.21.offset", memory.rom[0x21].offset);
  config.createKey("memory.rom.22.file", memory.rom[0x22].file);
  config.createKey("memory.rom.22.offset", memory.rom[0x22].offset);
  config.createKey("memory.rom.23.file", memory.rom[0x23].file);
  config.createKey("memory.rom.23.offset", memory.rom[0x23].offset);
  config.createKey("memory.rom.30.file", memory.rom[0x30].file);
  config.createKey("memory.rom.30.offset", memory.rom[0x30].offset);
  config.createKey("memory.rom.31.file", memory.rom[0x31].file);
  config.createKey("memory.rom.31.offset", memory.rom[0x31].offset);
}

class Plus4EmuDisplaySndConfiguration {
 private:
    struct {
      int         quality;
    } display;
    struct {
      double      latency;
      int         hwPeriods;
      double      dcBlockFilter1Freq;
      double      dcBlockFilter2Freq;
      struct {
        int       mode;
        double    frequency;
        double    level;
        double    q;
      } equalizer;
    } sound;
 public:
  Plus4EmuDisplaySndConfiguration(Plus4Emu::ConfigurationDB& config)
  {
    display.quality = 1;
    sound.latency = 0.075;
    sound.hwPeriods = 12;
    sound.dcBlockFilter1Freq = 10.0;
    sound.dcBlockFilter2Freq = 10.0;
    sound.equalizer.mode = 2;
    sound.equalizer.frequency = 14000.0;
    sound.equalizer.level = 0.355;
    sound.equalizer.q = 0.7071;
    config.createKey("display.quality", display.quality);
    config.createKey("sound.latency", sound.latency);
    config.createKey("sound.hwPeriods", sound.hwPeriods);
    config.createKey("sound.dcBlockFilter1Freq", sound.dcBlockFilter1Freq);
    config.createKey("sound.dcBlockFilter2Freq", sound.dcBlockFilter2Freq);
    config.createKey("sound.equalizer.mode", sound.equalizer.mode);
    config.createKey("sound.equalizer.frequency", sound.equalizer.frequency);
    config.createKey("sound.equalizer.level", sound.equalizer.level);
    config.createKey("sound.equalizer.q", sound.equalizer.q);
  }
};

class Plus4EmuGUIConfiguration {
 private:
  struct {
    std::string snapshotDirectory;
    std::string demoDirectory;
    std::string soundFileDirectory;
    std::string configDirectory;
    std::string loadFileDirectory;
    std::string tapeImageDirectory;
    std::string diskImageDirectory;
    std::string romImageDirectory;
    std::string prgFileDirectory;
    std::string debuggerDirectory;
    std::string screenshotDirectory;
  } gui;
 public:
  Plus4EmuGUIConfiguration(Plus4Emu::ConfigurationDB& config,
                           const std::string& installDirectory)
  {
    gui.snapshotDirectory = ".";
    gui.demoDirectory = installDirectory + "demo";
    gui.soundFileDirectory = ".";
    gui.configDirectory = installDirectory + "config";
    gui.loadFileDirectory = ".";
    gui.tapeImageDirectory = installDirectory + "tape";
    gui.diskImageDirectory = installDirectory + "disk";
    gui.romImageDirectory = installDirectory + "roms";
    gui.prgFileDirectory = installDirectory + "progs";
    gui.debuggerDirectory = ".";
    gui.screenshotDirectory = ".";
    config.createKey("gui.snapshotDirectory", gui.snapshotDirectory);
    config.createKey("gui.demoDirectory", gui.demoDirectory);
    config.createKey("gui.soundFileDirectory", gui.soundFileDirectory);
    config.createKey("gui.configDirectory", gui.configDirectory);
    config.createKey("gui.loadFileDirectory", gui.loadFileDirectory);
    config.createKey("gui.tapeImageDirectory", gui.tapeImageDirectory);
    config.createKey("gui.diskImageDirectory", gui.diskImageDirectory);
    config.createKey("gui.romImageDirectory", gui.romImageDirectory);
    config.createKey("gui.prgFileDirectory", gui.prgFileDirectory);
    config.createKey("gui.debuggerDirectory", gui.debuggerDirectory);
    config.createKey("gui.screenshotDirectory", gui.screenshotDirectory);
  }
};

int main(int argc, char **argv)
{
  Fl::lock();
  std::string installDirectory = "";
  if (argc > 1)
    installDirectory = argv[argc - 1];
  if (installDirectory.length() == 0)
    return -1;
#ifdef WIN32
  uint8_t c = '\\';
#else
  uint8_t c = '/';
#endif
  if (installDirectory[installDirectory.length() - 1] != c)
    installDirectory += c;
  std::string configDirectory = installDirectory + "config";
  configDirectory += c;
  std::string romDirectory = installDirectory + "roms";
  romDirectory += c;
  Plus4EmuConfigInstallerGUI  *gui = new Plus4EmuConfigInstallerGUI();
  gui->mainWindow->show();
  do {
    Fl::wait(0.05);
  } while (gui->mainWindow->shown());
  try {
    Plus4Emu::ConfigurationDB     *config = (Plus4Emu::ConfigurationDB *) 0;
    Plus4EmuMachineConfiguration  *mCfg = (Plus4EmuMachineConfiguration *) 0;
    if (gui->enableCfgInstall) {
      Plus4EmuDisplaySndConfiguration   *dsCfg =
          (Plus4EmuDisplaySndConfiguration *) 0;
      config = new Plus4Emu::ConfigurationDB();
      {
        Plus4EmuGUIConfiguration  *gCfg =
            new Plus4EmuGUIConfiguration(*config, installDirectory);
        try {
          Plus4Emu::File  f;
          config->saveState(f);
          f.writeFile("gui_cfg.dat", true);
        }
        catch (std::exception& e) {
          gui->errorMessage(e.what());
        }
        delete gCfg;
      }
      delete config;
      config = new Plus4Emu::ConfigurationDB();
      mCfg = new Plus4EmuMachineConfiguration(*config, 30, romDirectory);
      dsCfg = new Plus4EmuDisplaySndConfiguration(*config);
      setKeyboardConfiguration(*config, (gui->keyboardMapHU ? 5 : 4));
      try {
        Plus4Emu::File  f;
        config->saveState(f);
        f.writeFile("plus4cfg.dat", true);
      }
      catch (std::exception& e) {
        gui->errorMessage(e.what());
      }
      delete config;
      delete mCfg;
      delete dsCfg;
      config = (Plus4Emu::ConfigurationDB *) 0;
      mCfg = (Plus4EmuMachineConfiguration *) 0;
    }
    for (int i = 0; i < 16; i++) {
      config = new Plus4Emu::ConfigurationDB();
      mCfg = new Plus4EmuMachineConfiguration(*config, i, romDirectory);
      try {
        std::string fileName = configDirectory;
        fileName += machineConfigFileNames[i];
        config->saveState(fileName.c_str(), false);
      }
      catch (std::exception& e) {
        gui->errorMessage(e.what());
      }
      delete config;
      delete mCfg;
      config = (Plus4Emu::ConfigurationDB *) 0;
      mCfg = (Plus4EmuMachineConfiguration *) 0;
    }
    for (int i = 0; i < 8; i++) {
      if (keyboardConfigFileNames[i] != (char *) 0) {
        config = new Plus4Emu::ConfigurationDB();
        try {
          setKeyboardConfiguration(*config, i);
          std::string fileName = configDirectory;
          fileName += keyboardConfigFileNames[i];
          config->saveState(fileName.c_str(), false);
        }
        catch (std::exception& e) {
          gui->errorMessage(e.what());
        }
        delete config;
        config = (Plus4Emu::ConfigurationDB *) 0;
      }
    }
  }
  catch (std::exception& e) {
    gui->errorMessage(e.what());
    delete gui;
    return -1;
  }
  delete gui;
  return 0;
}

