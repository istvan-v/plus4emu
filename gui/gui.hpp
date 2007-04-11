
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

#ifndef PLUS4EMU_GUI_HPP
#define PLUS4EMU_GUI_HPP

#include "plus4emu.hpp"
#include "fileio.hpp"
#include "system.hpp"
#include "display.hpp"
#include "fldisp.hpp"
#include "gldisp.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "vmthread.hpp"
#include "cfg_db.hpp"
#include "emucfg.hpp"

#include "plus4vm.hpp"

#include <FL/Fl_File_Chooser.H>

class Plus4EmuGUI_DiskConfigWindow;
class Plus4EmuGUI_DisplayConfigWindow;
class Plus4EmuGUI_SoundConfigWindow;
class Plus4EmuGUI_MachineConfigWindow;
class Plus4EmuGUI_DebugWindow;
class Plus4EmuGUI_AboutWindow;

#include "gui_fl.hpp"
#include "disk_cfg.hpp"
#include "disp_cfg.hpp"
#include "snd_cfg.hpp"
#include "vm_cfg.hpp"
#include "debug_fl.hpp"
#include "about_fl.hpp"

#endif  // PLUS4EMU_GUI_HPP

