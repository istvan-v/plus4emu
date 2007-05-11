
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

#ifndef PLUS4EMU_JOYSTICK_HPP
#define PLUS4EMU_JOYSTICK_HPP

#include "plus4emu.hpp"
#include "system.hpp"
#include "emucfg.hpp"

namespace Plus4Emu {

  class JoystickInput {
   private:
    struct Axis_ {
      int     devNum;
      int     axisNum;
      int     prvInputState;
      int     prvOutputState;
    };
    struct Button_ {
      int     devNum;
      int     buttonNum;
      bool    prvInputState;
      bool    prvOutputState;
    };
    struct POVHat_ {
      int     devNum;
      int     hatNum;
      int     prvState;
    };
    Axis_   axes[8];
    Button_ buttons[8];
    POVHat_ povHats[2];
    int     events[32];
    int     eventCnt;
    int     eventIndex;
    int     axisCnt;
    int     buttonCnt;
    int     hatCnt;
    Timer   pwmTimer;
    Timer   autoFireTimer;
    void    *sdlDevices[2];
    bool    haveJoystick;
    bool    sdlInitialized;
   public:
    static const int  keyCodeBase = 0xC000;
    JoystickInput(bool sdlInitFlag = false);
    virtual ~JoystickInput();
    /*!
     * Poll joystick input for events. Returns zero if there are none,
     * +keyCode on key press, and -keyCode on key release. The key code
     * can be one of the following values:
     *   keyCodeBase + 0:   axis 1 negative
     *   keyCodeBase + 1:   axis 1 positive
     *   keyCodeBase + 2:   axis 2 negative
     *   keyCodeBase + 3:   axis 2 positive
     *   keyCodeBase + 4:   axis 3 negative
     *   keyCodeBase + 5:   axis 3 positive
     *   keyCodeBase + 6:   axis 4 negative
     *   keyCodeBase + 7:   axis 4 positive
     *   keyCodeBase + 8:   axis 5 negative
     *   keyCodeBase + 9:   axis 5 positive
     *   keyCodeBase + 10:  axis 6 negative
     *   keyCodeBase + 11:  axis 6 positive
     *   keyCodeBase + 12:  axis 7 negative
     *   keyCodeBase + 13:  axis 7 positive
     *   keyCodeBase + 14:  axis 8 negative
     *   keyCodeBase + 15:  axis 8 positive
     *   keyCodeBase + 16:  button 1
     *   keyCodeBase + 17:  button 2
     *   keyCodeBase + 18:  button 3
     *   keyCodeBase + 19:  button 4
     *   keyCodeBase + 20:  button 5
     *   keyCodeBase + 21:  button 6
     *   keyCodeBase + 22:  button 7
     *   keyCodeBase + 23:  button 8
     *   keyCodeBase + 24:  POV hat 1 right
     *   keyCodeBase + 25:  POV hat 1 up
     *   keyCodeBase + 26:  POV hat 1 left
     *   keyCodeBase + 27:  POV hat 1 down
     *   keyCodeBase + 28:  POV hat 2 right
     *   keyCodeBase + 29:  POV hat 2 up
     *   keyCodeBase + 30:  POV hat 2 left
     *   keyCodeBase + 31:  POV hat 2 down
     */
    int getEvent(EmulatorConfiguration::JoystickConfiguration& config);
    int getEvent();
    void flushEvents();
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_JOYSTICK_HPP

