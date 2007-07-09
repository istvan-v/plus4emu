
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

#ifndef PLUS4EMU_GUI_MONITOR_HPP
#define PLUS4EMU_GUI_MONITOR_HPP

#include "gui.hpp"

#include <vector>

#include <FL/Fl.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>

class Plus4EmuGUIMonitor : public Fl_Text_Editor {
 protected:
  Fl_Text_Buffer            *buf_;
  Plus4EmuGUI_DebugWindow   *debugWindow;
  Plus4EmuGUI               *gui;
  int32_t                   assembleOffset;
  uint32_t                  disassembleAddress;
  int32_t                   disassembleOffset;
  uint32_t                  memoryDumpAddress;
  uint32_t                  addressMask;
  bool                      cpuAddressMode;
  // --------
  void command_assemble(const std::vector<std::string>& args);
  void command_disassemble(const std::vector<std::string>& args);
  void command_memoryDump(const std::vector<std::string>& args);
  void command_memoryModify(const std::vector<std::string>& args);
  void command_toggleCPUAddressMode(const std::vector<std::string>& args);
  static int enterKeyCallback(int c, Fl_Text_Editor *e_);
  void moveDown();
  void parseCommand(const char *s);
  void printMessage(const char *s);
  void disassembleInstruction(bool assembleMode = false);
  void memoryDump();
 public:
  Plus4EmuGUIMonitor(int xx, int yy, int ww, int hh, const char *ll = 0);
  virtual ~Plus4EmuGUIMonitor();
  void setDebugger(Plus4EmuGUI_DebugWindow& debugWindow_)
  {
    debugWindow = &debugWindow_;
    gui = &(debugWindow->gui);
  }
};

#endif  // PLUS4EMU_GUI_MONITOR_HPP
