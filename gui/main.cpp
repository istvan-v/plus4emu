
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

#include "gui.hpp"
#include "system.hpp"
#include "guicolor.hpp"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

static void cfgErrorFunc(void *userData, const char *msg)
{
  (void) userData;
  std::cerr << "WARNING: " << msg << std::endl;
}

static void writeKeyboardBuffer(Plus4Emu::VirtualMachine& vm, const char *s)
{
  for (int i = 0; true; i++) {
    if (s[i] == '\0') {
      vm.writeMemory(0x00EF, uint8_t(i), true);
      break;
    }
    else if (s[i] != '\n')
      vm.writeMemory(uint32_t(0x0527 + i), uint8_t(s[i]), true);
    else
      vm.writeMemory(uint32_t(0x0527 + i), 0x0D, true);
  }
}

int main(int argc, char **argv)
{
  Fl_Window *w = (Fl_Window *) 0;
  Plus4Emu::VirtualMachine  *vm = (Plus4Emu::VirtualMachine *) 0;
  Plus4Emu::AudioOutput     *audioOutput = (Plus4Emu::AudioOutput *) 0;
  Plus4Emu::EmulatorConfiguration   *config =
      (Plus4Emu::EmulatorConfiguration *) 0;
  Plus4Emu::VMThread        *vmThread = (Plus4Emu::VMThread *) 0;
  Plus4EmuGUI               *gui_ = (Plus4EmuGUI *) 0;
  const char  *cfgFileName = "plus4cfg.dat";
  int       prgNameIndex = 0;
  int       diskNameIndex = 0;
  int       tapeNameIndex = 0;
  int       snapshotNameIndex = 0;
  int       colorScheme = 0;
  int       retval = 0;
  bool      glEnabled = true;
  bool      configLoaded = false;

  try {
    // find out machine type to be emulated
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-cfg") == 0 && i < (argc - 1)) {
        i++;
      }
      else if (std::strcmp(argv[i], "-prg") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing program file name");
        prgNameIndex = i;
      }
      else if (std::strcmp(argv[i], "-disk") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing disk image file name");
        diskNameIndex = i;
      }
      else if (std::strcmp(argv[i], "-tape") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing tape image file name");
        tapeNameIndex = i;
      }
      else if (std::strcmp(argv[i], "-snapshot") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing snapshot file name");
        snapshotNameIndex = i;
      }
      else if (std::strcmp(argv[i], "-colorscheme") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing color scheme number");
        colorScheme = int(std::atoi(argv[i]));
        colorScheme = (colorScheme >= 0 && colorScheme <= 2 ? colorScheme : 0);
      }
      else if (std::strcmp(argv[i], "-plus4") == 0) {
        cfgFileName = "plus4cfg.dat";
      }
      else if (std::strcmp(argv[i], "-opengl") == 0) {
        glEnabled = true;
      }
      else if (std::strcmp(argv[i], "-no-opengl") == 0) {
        glEnabled = false;
      }
      else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "-help") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS...]" << std::endl;
        std::cerr << "The allowed options are:" << std::endl;
        std::cerr << "    -h | -help | --help "
                     "print this message" << std::endl;
        std::cerr << "    -cfg <FILENAME>     "
                     "load ASCII format configuration file" << std::endl;
        std::cerr << "    -prg <FILENAME>     "
                     "load program file on startup" << std::endl;
        std::cerr << "    -disk <FILENAME>    "
                     "load and automatically start disk image on startup"
                  << std::endl;
        std::cerr << "    -tape <FILENAME>    "
                     "load and automatically start tape image on startup"
                  << std::endl;
        std::cerr << "    -snapshot <FNAME>   "
                     "load snapshot or demo file on startup" << std::endl;
        std::cerr << "    -opengl             "
                     "use OpenGL video driver (this is the default)"
                  << std::endl;
        std::cerr << "    -no-opengl          "
                     "use software video driver" << std::endl;
        std::cerr << "    -colorscheme <N>    "
                     "use GUI color scheme N (0, 1, or 2)" << std::endl;
        std::cerr << "    OPTION=VALUE        "
                     "set configuration variable 'OPTION' to 'VALUE'"
                  << std::endl;
        std::cerr << "    OPTION              "
                     "set boolean configuration variable 'OPTION' to true"
                  << std::endl;
        std::cerr << "    FILENAME            "
                     "load and start .prg, .p00, .d64, .d81, or .tap file"
                  << std::endl;
        return 0;
      }
      else {
        size_t  n = std::strlen(argv[i]);
        if (n >= 4 && argv[i][n - 4] == '.') {
          const char  *s = &(argv[i][n - 3]);
          if ((s[0] == 'P' || s[0] == 'p') &&
              (((s[1] == 'R' || s[1] == 'r') && (s[2] == 'G' || s[2] == 'g')) ||
               (s[1] == '0' && s[2] == '0'))) {
            prgNameIndex = i;
          }
          else if ((s[0] == 'D' || s[0] == 'd') &&
                   ((s[1] == '6' && s[2] == '4') ||
                    (s[1] == '8' && s[2] == '1'))) {
            diskNameIndex = i;
          }
          else if ((s[0] == 'T' || s[0] == 't') &&
                   (s[1] == 'A' || s[1] == 'a') &&
                   (s[2] == 'P' || s[2] == 'p')) {
            tapeNameIndex = i;
          }
        }
      }
    }

    Fl::lock();
    Plus4Emu::setGUIColorScheme(colorScheme);
    audioOutput = new Plus4Emu::AudioOutput_PortAudio();
    if (glEnabled)
      w = new Plus4Emu::OpenGLDisplay(32, 32, 384, 288, "");
    else
      w = new Plus4Emu::FLTKDisplay(32, 32, 384, 288, "");
    w->end();
    vm = new Plus4::Plus4VM(*(dynamic_cast<Plus4Emu::VideoDisplay *>(w)),
                            *audioOutput);
    config = new Plus4Emu::EmulatorConfiguration(
        *vm, *(dynamic_cast<Plus4Emu::VideoDisplay *>(w)), *audioOutput);
    config->setErrorCallback(&cfgErrorFunc, (void *) 0);
    // load base configuration (if available)
    {
      Plus4Emu::File  *f = (Plus4Emu::File *) 0;
      try {
        try {
          f = new Plus4Emu::File(cfgFileName, true);
        }
        catch (Plus4Emu::Exception& e) {
          std::string cmdLine = "\"";
          cmdLine += argv[0];
          size_t  i = cmdLine.length();
          while (i > 0) {
            i--;
            if (cmdLine[i] == '/' || cmdLine[i] == '\\') {
              i++;
              break;
            }
          }
          cmdLine.resize(i);
          cmdLine += "makecfg\"";
#ifdef __APPLE__
          cmdLine += " -f";
#endif
          std::system(cmdLine.c_str());
          f = new Plus4Emu::File(cfgFileName, true);
        }
        config->registerChunkType(*f);
        f->processAllChunks();
        delete f;
      }
      catch (...) {
        if (f)
          delete f;
      }
    }
    configLoaded = true;
    // check command line for any additional configuration
    for (int i = 1; i < argc; i++) {
      if (std::strcmp(argv[i], "-plus4") == 0 ||
          std::strcmp(argv[i], "-opengl") == 0 ||
          std::strcmp(argv[i], "-no-opengl") == 0)
        continue;
      if (std::strcmp(argv[i], "-cfg") == 0) {
        if (++i >= argc)
          throw Plus4Emu::Exception("missing configuration file name");
        config->loadState(argv[i], false);
      }
      else if (std::strcmp(argv[i], "-prg") == 0 ||
               std::strcmp(argv[i], "-disk") == 0 ||
               std::strcmp(argv[i], "-tape") == 0 ||
               std::strcmp(argv[i], "-snapshot") == 0 ||
               std::strcmp(argv[i], "-colorscheme") == 0) {
        i++;
      }
      else if (i != prgNameIndex && i != diskNameIndex && i != tapeNameIndex) {
        const char  *s = argv[i];
#ifdef __APPLE__
        if (std::strncmp(s, "-psn_", 5) == 0)
          continue;
#endif
        if (*s == '-')
          s++;
        if (*s == '-')
          s++;
        const char  *p = std::strchr(s, '=');
        if (!p)
          (*config)[s] = bool(true);
        else {
          std::string optName;
          while (s != p) {
            optName += (*s);
            s++;
          }
          p++;
          (*config)[optName] = p;
        }
      }
    }
    config->applySettings();
    if (snapshotNameIndex >= 1) {
      Plus4Emu::File  f(argv[snapshotNameIndex], false);
      vm->registerChunkTypes(f);
      f.processAllChunks();
    }
    else if (prgNameIndex >= 1) {
      vm->setEnableDisplay(false);
      vm->setEnableAudioOutput(false);
      vm->run(900000);
      dynamic_cast<Plus4::Plus4VM *>(vm)->loadProgram(argv[prgNameIndex]);
      writeKeyboardBuffer(*vm, "Ru\n");
      vm->setEnableDisplay(config->display.enabled);
      vm->setEnableAudioOutput(config->sound.enabled);
    }
    else if (diskNameIndex >= 1) {
      (*config)["floppy.a.imageFile"] = argv[diskNameIndex];
      config->applySettings();
      vm->setEnableDisplay(false);
      vm->setEnableAudioOutput(false);
      vm->run(2475000);
      writeKeyboardBuffer(*vm, "Lo\"*\",8,1\n");
      for (int i = 0; i < 1000; i++) {
        vm->run(1000);
        if (vm->readMemory(0x00EF, true) == 0x00)
          i = (i < 998 ? 998 : i);
      }
      writeKeyboardBuffer(*vm, "Ru\n");
      vm->setEnableDisplay(config->display.enabled);
      vm->setEnableAudioOutput(config->sound.enabled);
    }
    else if (tapeNameIndex >= 1) {
      (*config)["tape.imageFile"] = argv[tapeNameIndex];
      config->applySettings();
      vm->setEnableDisplay(false);
      vm->setEnableAudioOutput(false);
      vm->run(900000);
      writeKeyboardBuffer(*vm, "Lo\"\",1,1\n");
      for (int i = 0; i < 1000; i++) {
        vm->run(1000);
        if (vm->readMemory(0x00EF, true) == 0x00)
          i = (i < 998 ? 998 : i);
      }
      writeKeyboardBuffer(*vm, "Ru\n");
      vm->setEnableDisplay(config->display.enabled);
      vm->setEnableAudioOutput(config->sound.enabled);
      // FIXME: this does not set the tape button status display on the GUI
      vm->tapePlay();
    }
    vmThread = new Plus4Emu::VMThread(*vm);
    gui_ = new Plus4EmuGUI(*(dynamic_cast<Plus4Emu::VideoDisplay *>(w)),
                           *audioOutput, *vm, *vmThread, *config);
    gui_->run();
  }
  catch (std::exception& e) {
    if (gui_)
      gui_->errorMessage(e.what());
    else
      std::cerr << " *** error: " << e.what() << std::endl;
    retval = -1;
  }
  if (gui_)
    delete gui_;
  if (vmThread)
    delete vmThread;
  if (config) {
    if (configLoaded) {
      try {
        Plus4Emu::File  f;
        config->saveState(f);
        f.writeFile(cfgFileName, true);
      }
      catch (...) {
      }
    }
    delete config;
  }
  if (vm)
    delete vm;
  if (w)
    delete w;
  if (audioOutput)
    delete audioOutput;
  return retval;
}

