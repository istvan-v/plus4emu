
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
#include "system.hpp"
#include "emucfg.hpp"
#include "joystick.hpp"

#include <SDL/SDL.h>
#include <cmath>

namespace Plus4Emu {

  JoystickInput::JoystickInput(bool sdlInitFlag)
    : eventCnt(0),
      eventIndex(0),
      axisCnt(0),
      buttonCnt(0),
      hatCnt(0),
      pwmTimer(),
      autoFireTimer(),
      haveJoystick(false),
      sdlInitialized(false)
  {
    sdlDevices[0] = (void *) 0;
    sdlDevices[1] = (void *) 0;
#ifdef HAVE_SDL_H
    if (sdlInitFlag) {
      if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
        return;
      sdlInitialized = true;
    }
    int   nDevices = SDL_NumJoysticks();
    int   j = 0;
    int   prvDevNum = -1;
    for (int i = 0; i < nDevices && j < 2; i++) {
      SDL_Joystick  *joy_ = SDL_JoystickOpen(i);
      if (!joy_)
        continue;
      int   axisCnt_ = SDL_JoystickNumAxes(joy_);
      int   buttonCnt_ = SDL_JoystickNumButtons(joy_);
      int   hatCnt_ = SDL_JoystickNumHats(joy_);
      if (!(axisCnt_ >= 4 && buttonCnt_ >= 4 && hatCnt_ >= 1)) {
        SDL_JoystickClose(joy_);
        continue;
      }
      sdlDevices[j++] = joy_;
      prvDevNum = i;
    }
    for (int i = 0; i < nDevices && j < 2; i++) {
      if (i == prvDevNum)
        continue;
      SDL_Joystick  *joy_ = SDL_JoystickOpen(i);
      if (!joy_)
        continue;
      int   axisCnt_ = SDL_JoystickNumAxes(joy_);
      int   buttonCnt_ = SDL_JoystickNumButtons(joy_);
      int   hatCnt_ = SDL_JoystickNumHats(joy_);
      if (!(axisCnt_ >= 2 && buttonCnt_ >= 1 && hatCnt_ >= 1)) {
        SDL_JoystickClose(joy_);
        continue;
      }
      sdlDevices[j++] = joy_;
      prvDevNum = i;
    }
    for (int i = 0; i < nDevices && j < 2; i++) {
      if (i == prvDevNum)
        continue;
      SDL_Joystick  *joy_ = SDL_JoystickOpen(i);
      if (!joy_)
        continue;
      sdlDevices[j++] = joy_;
    }
    if (!j)
      return;
    nDevices = j;
    for (j = 0; j < nDevices; j++) {
      SDL_Joystick  *joy_ = reinterpret_cast<SDL_Joystick *>(sdlDevices[j]);
      int   axisCnt_ = SDL_JoystickNumAxes(joy_);
      int   buttonCnt_ = SDL_JoystickNumButtons(joy_);
      int   hatCnt_ = SDL_JoystickNumHats(joy_);
      if (j == 0 && nDevices > 1) {
        axisCnt_ = (axisCnt_ < 4 ? axisCnt_ : 4);
        buttonCnt_ = (buttonCnt_ < 4 ? buttonCnt_ : 4);
        hatCnt_ = (hatCnt_ < 1 ? hatCnt_ : 1);
      }
      for (int i = 0; i < axisCnt_ && axisCnt < 8; i++) {
        axes[axisCnt].devNum = j;
        axes[axisCnt].axisNum = i;
        axes[axisCnt].prvInputState = 0;
        axes[axisCnt].prvOutputState = 0;
        axisCnt++;
      }
      for (int i = 0; i < buttonCnt_ && buttonCnt < 8; i++) {
        buttons[buttonCnt].devNum = j;
        buttons[buttonCnt].buttonNum = i;
        buttons[buttonCnt].prvInputState = false;
        buttons[buttonCnt].prvOutputState = false;
        buttonCnt++;
      }
      for (int i = 0; i < hatCnt_ && hatCnt < 2; i++) {
        povHats[hatCnt].devNum = j;
        povHats[hatCnt].hatNum = i;
        povHats[hatCnt].prvState = SDL_HAT_CENTERED;
        hatCnt++;
      }
    }
    haveJoystick = (axisCnt > 0 || buttonCnt > 0 || hatCnt > 0);
#endif  // HAVE_SDL_H
  }

  JoystickInput::~JoystickInput()
  {
#ifdef HAVE_SDL_H
    if (sdlDevices[0])
      SDL_JoystickClose(reinterpret_cast<SDL_Joystick *>(sdlDevices[0]));
    if (sdlDevices[1])
      SDL_JoystickClose(reinterpret_cast<SDL_Joystick *>(sdlDevices[1]));
    if (sdlInitialized)
      SDL_Quit();
#endif
  }

  int JoystickInput::getEvent(EmulatorConfiguration::JoystickConfiguration&
                                  config)
  {
#ifdef HAVE_SDL_H
    if (!(haveJoystick && config.enableJoystick))
      return 0;
    if (eventIndex < eventCnt) {
      // consume any buffered events first
      int     retval = events[eventIndex];
      if (++eventIndex == eventCnt) {
        eventCnt = 0;
        eventIndex = 0;
      }
      return retval;
    }
    SDL_JoystickUpdate();
    eventCnt = 0;
    eventIndex = 0;
    int     thresholdInt = int(config.axisThreshold * 32767.0 + 0.5);
    bool    autoFireState = false;
    double  pwmPhase = 0.0;
    if (config.enablePWM) {
      double  tt = 1.0 / config.pwmFrequency;
      double  t = pwmTimer.getRealTime();
      if (t >= tt) {
        t = std::fmod(t, tt);
        pwmTimer.reset(t);
      }
      pwmPhase = t * config.pwmFrequency;
      pwmPhase *= (1.0 - config.axisThreshold);
      pwmPhase += (config.axisThreshold * 0.5);
    }
    if (config.enableAutoFire) {
      double  tt = 1.0 / config.autoFireFrequency;
      double  t = autoFireTimer.getRealTime();
      if (t >= tt) {
        t = std::fmod(t, tt);
        autoFireTimer.reset(t);
      }
      autoFireState = (t < (tt * config.autoFirePulseWidth));
    }
    for (int i = 0; i < axisCnt; i++) {
      int     newState =
          int(SDL_JoystickGetAxis(reinterpret_cast<SDL_Joystick *>(
                                      sdlDevices[axes[i].devNum]),
                                  axes[i].axisNum));
      if (!config.enablePWM) {
        if (newState > axes[i].prvInputState) {
          if (newState >= (-thresholdInt)) {
            if (axes[i].prvOutputState < 0)
              events[eventCnt++] = -(keyCodeBase + (i << 1));
            if (newState >= thresholdInt) {
              if (axes[i].prvOutputState <= 0)
                events[eventCnt++] = (keyCodeBase + (i << 1) + 1);
              axes[i].prvOutputState = 1;
            }
            else
              axes[i].prvOutputState = 0;
          }
        }
        else if (newState < axes[i].prvInputState) {
          if (newState < thresholdInt) {
            if (axes[i].prvOutputState > 0)
              events[eventCnt++] = -(keyCodeBase + (i << 1) + 1);
            if (newState < (-thresholdInt)) {
              if (axes[i].prvOutputState >= 0)
                events[eventCnt++] = (keyCodeBase + (i << 1));
              axes[i].prvOutputState = -1;
            }
            else
              axes[i].prvOutputState = 0;
          }
        }
      }
      else {
        int     newOutputState = 0;
        if (newState > 0) {
          if ((double(newState) * (1.0 / 32768.0)) >= pwmPhase)
            newOutputState = 1;
        }
        else if (newState < 0) {
          if ((double(newState) * (-1.0 / 32768.0)) > pwmPhase)
            newOutputState = -1;
        }
        if (newOutputState != axes[i].prvOutputState) {
          if (axes[i].prvOutputState < 0)
            events[eventCnt++] = -(keyCodeBase + (i << 1));
          else if (axes[i].prvOutputState > 0)
            events[eventCnt++] = -(keyCodeBase + (i << 1) + 1);
          if (newOutputState < 0)
            events[eventCnt++] = (keyCodeBase + (i << 1));
          else if (newOutputState > 0)
            events[eventCnt++] = (keyCodeBase + (i << 1) + 1);
          axes[i].prvOutputState = newOutputState;
        }
      }
      axes[i].prvInputState = newState;
    }
    for (int i = 0; i < buttonCnt; i++) {
      bool    newState =
          bool(SDL_JoystickGetButton(reinterpret_cast<SDL_Joystick *>(
                                         sdlDevices[buttons[i].devNum]),
                                     buttons[i].buttonNum));
      if (config.enableAutoFire) {
        if (newState && !buttons[i].prvInputState) {
          autoFireState = true;
          autoFireTimer.reset();
        }
        buttons[i].prvInputState = newState;
        newState = newState && autoFireState;
      }
      else
        buttons[i].prvInputState = newState;
      if (newState != buttons[i].prvOutputState) {
        if (newState)
          events[eventCnt++] = (keyCodeBase + 16 + i);
        else
          events[eventCnt++] = -(keyCodeBase + 16 + i);
        buttons[i].prvOutputState = newState;
      }
    }
    for (int i = 0; i < hatCnt; i++) {
      int     newState =
          int(SDL_JoystickGetHat(reinterpret_cast<SDL_Joystick *>(
                                     sdlDevices[povHats[i].devNum]),
                                 povHats[i].hatNum));
      int     changeMask = newState ^ povHats[i].prvState;
      if (changeMask) {
        if (changeMask & int(SDL_HAT_RIGHT)) {
          if (newState & int(SDL_HAT_RIGHT))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2));
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2));
        }
        if (changeMask & int(SDL_HAT_UP)) {
          if (newState & int(SDL_HAT_UP))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 1);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 1);
        }
        if (changeMask & int(SDL_HAT_LEFT)) {
          if (newState & int(SDL_HAT_LEFT))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 2);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 2);
        }
        if (changeMask & int(SDL_HAT_DOWN)) {
          if (newState & int(SDL_HAT_DOWN))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 3);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 3);
        }
        povHats[i].prvState = newState;
      }
    }
    if (eventCnt) {
      // return first event from buffer
      int     retval = events[0];
      if (++eventIndex == eventCnt) {
        eventCnt = 0;
        eventIndex = 0;
      }
      return retval;
    }
#endif  // HAVE_SDL_H
    return 0;
  }

  int JoystickInput::getEvent()
  {
#ifdef HAVE_SDL_H
    if (!haveJoystick)
      return 0;
    if (eventIndex < eventCnt) {
      // consume any buffered events first
      int     retval = events[eventIndex];
      if (++eventIndex == eventCnt) {
        eventCnt = 0;
        eventIndex = 0;
      }
      return retval;
    }
    SDL_JoystickUpdate();
    eventCnt = 0;
    eventIndex = 0;
    for (int i = 0; i < axisCnt; i++) {
      int     newState =
          int(SDL_JoystickGetAxis(reinterpret_cast<SDL_Joystick *>(
                                      sdlDevices[axes[i].devNum]),
                                  axes[i].axisNum));
      if (newState > axes[i].prvInputState) {
        if (newState >= -8192) {
          if (axes[i].prvOutputState < 0)
            events[eventCnt++] = -(keyCodeBase + (i << 1));
          if (newState >= 24576) {
            if (axes[i].prvOutputState <= 0)
              events[eventCnt++] = (keyCodeBase + (i << 1) + 1);
            axes[i].prvOutputState = 1;
          }
          else if (newState < 8192)
            axes[i].prvOutputState = 0;
        }
      }
      else if (newState < axes[i].prvInputState) {
        if (newState < 8192) {
          if (axes[i].prvOutputState > 0)
            events[eventCnt++] = -(keyCodeBase + (i << 1) + 1);
          if (newState < -24576) {
            if (axes[i].prvOutputState >= 0)
              events[eventCnt++] = (keyCodeBase + (i << 1));
            axes[i].prvOutputState = -1;
          }
          else if (newState >= -8192)
            axes[i].prvOutputState = 0;
        }
      }
      axes[i].prvInputState = newState;
    }
    for (int i = 0; i < buttonCnt; i++) {
      bool    newState =
          bool(SDL_JoystickGetButton(reinterpret_cast<SDL_Joystick *>(
                                         sdlDevices[buttons[i].devNum]),
                                     buttons[i].buttonNum));
      if (newState != buttons[i].prvInputState) {
        if (newState)
          events[eventCnt++] = (keyCodeBase + 16 + i);
        else
          events[eventCnt++] = -(keyCodeBase + 16 + i);
        buttons[i].prvInputState = newState;
        buttons[i].prvOutputState = newState;
      }
    }
    for (int i = 0; i < hatCnt; i++) {
      int     newState =
          int(SDL_JoystickGetHat(reinterpret_cast<SDL_Joystick *>(
                                     sdlDevices[povHats[i].devNum]),
                                 povHats[i].hatNum));
      int     changeMask = newState ^ povHats[i].prvState;
      if (changeMask) {
        if (changeMask & int(SDL_HAT_RIGHT)) {
          if (newState & int(SDL_HAT_RIGHT))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2));
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2));
        }
        if (changeMask & int(SDL_HAT_UP)) {
          if (newState & int(SDL_HAT_UP))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 1);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 1);
        }
        if (changeMask & int(SDL_HAT_LEFT)) {
          if (newState & int(SDL_HAT_LEFT))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 2);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 2);
        }
        if (changeMask & int(SDL_HAT_DOWN)) {
          if (newState & int(SDL_HAT_DOWN))
            events[eventCnt++] = (keyCodeBase + 24 + (i << 2) + 3);
          else
            events[eventCnt++] = -(keyCodeBase + 24 + (i << 2) + 3);
        }
        povHats[i].prvState = newState;
      }
    }
    if (eventCnt) {
      // return first event from buffer
      int     retval = events[0];
      if (++eventIndex == eventCnt) {
        eventCnt = 0;
        eventIndex = 0;
      }
      return retval;
    }
#endif  // HAVE_SDL_H
    return 0;
  }

  void JoystickInput::flushEvents()
  {
    eventCnt = 0;
    eventIndex = 0;
  }

}       // namespace Plus4Emu

