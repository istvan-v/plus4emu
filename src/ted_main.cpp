
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
#include "cpu.hpp"
#include "ted.hpp"

static const unsigned char  memoryMapIndexTable[16] = {
  4, 0, 0, 0,  1, 1, 1, 1,  2, 2, 2, 2,  3, 3, 3, 3
};

namespace Plus4 {

  void TED7360::runOneCycle_freezeMode()
  {
    delayedEvents0.stopDMA();
    dmaActive = false;
    cpuHaltedFlag = false;
    M7501::setIsCPURunning(true);
    // -------- EVEN HALF-CYCLE (FF1E bit 1 == 1) --------
    if (!(videoColumn & 0x01)) {
      characterPosition = 0x03FF;
      if (incrementingCharacterPosition) {
        savedCharacterPosition = (savedCharacterPosition + 1) & 0x03FF;
        characterPosition = savedCharacterPosition;
      }
      if (delayedEvents0) {
        DelayedEventsMask delayedEvents(delayedEvents0);
        delayedEvents0 = 0U;
        processDelayedEvents(delayedEvents);
      }
      TEDCallback *p = firstCallback0;
      while (p) {
        TEDCallback *nxt = p->nxt0;
        p->func(p->userData);
        p = nxt;
      }
      M7501::run(cpu_clock_multiplier);
      if (incrementingDMAPosition)
        dmaPosition = (dmaPosition + 1) & 0x03FF;
      prv_video_buf_pos = video_buf_pos;
      uint8_t *bufp = &(video_buf[video_buf_pos]);
      (void) render_blank(*this, bufp, 0);
      bufp[0] = (bufp[0] & 0x01) | 0x30;
      video_buf_pos = video_buf_pos + 2;
      tedRegisters[0x1E] = videoColumn;
      videoColumn |= uint8_t(0x01);
    }
    if (!ted_disabled) {
      runOneCycle();
      return;
    }
    // -------- ODD HALF-CYCLE (FF1E bit 1 == 0) --------
    if (delayedEvents1) {
      DelayedEventsMask delayedEvents(delayedEvents1);
      delayedEvents1 = 0U;
      processDelayedEvents(delayedEvents);
    }
    TEDCallback *p = firstCallback1;
    while (p) {
      TEDCallback *nxt = p->nxt1;
      p->func(p->userData);
      p = nxt;
    }
    if (!singleClockModeFlags)
      M7501::run(cpu_clock_multiplier);
    else
      idleMemoryRead();
    prv_video_buf_pos = video_buf_pos;
    uint8_t *bufp = &(video_buf[video_buf_pos]);
    (void) render_blank(*this, bufp, 4);
    bufp[0] = (bufp[0] & 0x01) | 0x30;
    video_buf_pos = video_buf_pos + 2;
    if (!cycle_count)
      calculateSoundOutput();
    tedRegisters[0x1E] = videoColumn;
    if (!ted_disabled)
      videoColumn = uint8_t(videoColumn != 113 ? (videoColumn + 1) : 0);
    videoColumn &= uint8_t(0x7E);
    cycle_count = (cycle_count + 1) & 3;
  }

  void TED7360::runOneCycle()
  {
    if (video_buf_pos >= 450) {
      videoOutputCallback(&(video_buf[0]), video_buf_pos);
      video_buf_pos = 0;
    }

    if (ted_disabled) {
      runOneCycle_freezeMode();
      return;
    }
    if (!(videoColumn & uint8_t(0x01))) {
      // -------- EVEN HALF-CYCLE (FF1E bit 1 == 1) --------
      characterPosition = 0x03FF;
      if (incrementingCharacterPosition) {
        savedCharacterPosition = (savedCharacterPosition + 1) & 0x03FF;
        characterPosition = savedCharacterPosition;
      }
      DelayedEventsMask delayedEvents(delayedEvents0);
      delayedEvents0 = 0U;
      switch (videoColumn) {
      case 0:                           // start display (40 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
          selectRenderFunction();
        }
        break;
      case 2:                           // start display (38 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
          selectRenderFunction();
        }
        break;
      case 38:                          // equalization pulse 1 start
        if (vsyncFlags) {
          videoOutputFlags =
              uint8_t((videoOutputFlags | 0x80) ^ (vsyncFlags & 0x80));
          selectRenderFunction();
        }
        break;
      case 72:
        if (incrementingDMAPosition)    // stop incrementing DMA position
          delayedEvents0.stopIncrementingDMAPosition();
        if (renderWindow) {
          if (characterLine == 6)       // latch DMA pos. at character line 6
            delayedEvents0.latchDMAPosition();
          if (prvCharacterLine == 6) {  // latch character position to reload
            if (!(bitmapAddressDisableFlags & 0x02))
              delayedEvents0.latchCharacterPosition();
          }
        }
        break;
      case 74:                          // DRAM refresh start
        delayedEvents0.dramRefreshOn();
        externalFetchSingleClockFlag = false;
        dmaFlags = dmaFlags & 0x03;     // disable DMA
        incrementingCharacterPosition = false;
        break;
      case 76:
        bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x01;
        // terminate DMA transfer
        delayedEvents.stopDMA();
        delayedEvents0.stopDMA();
        dmaActive = false;
        cpuHaltedFlag = false;
        M7501::setIsCPURunning(true);
        break;
      case 78:                          // end of display (38 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = false;
          selectRenderFunction();
        }
        videoShiftRegisterEnabled = false;
        break;
      case 80:                          // end of display (40 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = false;
          selectRenderFunction();
        }
        break;
      case 84:                          // DRAM refresh end
        delayedEvents0.dramRefreshOff();
        break;
      case 88:                          // horizontal blanking start
        videoOutputFlags |= uint8_t(0x20);
        current_render_func = &render_blank;
        break;
      case 90:                          // horizontal sync start
        if (!vsyncFlags) {
          videoOutputFlags |= uint8_t(0x80);
          current_render_func = &render_blank;
        }
        break;
      case 96:                          // increment line number
        delayedEvents0.incrementVideoLine();
        break;
      case 98:                          // end of screen
        if (savedVideoLineDelay1 == ((videoOutputFlags & 0x01) ? 261 : 311)) {
          videoLine = 0x01FF;           // FIXME
          characterPosition = 0x0000;   // reset character position
          savedCharacterPosition = 0x0000;
          characterPositionReload = 0x0000;
          delayedEvents1.updateCharPosReloadRegisters();
          dramRefreshAddrL = 0x00;
        }
        if (incrementingCharacterLine)
          delayedEvents0.incrementVerticalSub();
        if (!vsyncFlags) {              // horizontal sync end
          videoOutputFlags &= uint8_t(0x7D);
          selectRenderFunction();
        }
        if (!(videoOutputFlags & 0x10)) // burst start
          videoOutputFlags |= uint8_t(0x08);
        break;
      case 100:                         // external fetch single clock start
        externalFetchSingleClockFlag = true;
        if (renderWindow)
          delayedEvents0.singleClockModeOn();
        if (vsyncFlags) {               // equalization pulse 2 end
          videoOutputFlags =
              uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
          selectRenderFunction();
        }
        break;
      case 102:                         // enable / start DMA
        dmaFlags = dmaFlags | 0x80;
        characterColumn = (characterColumn == 40 ? 0x3C : characterColumn);
        if (renderWindow) {
          // check if DMA should be requested:
          if ((savedVideoLineDelay1 & 7) == int(verticalScroll) && dmaEnabled) {
            // start a new DMA at character line 7
            delayedEvents0.stopDMA();
            dmaActive = true;
            delayedEvents.startDMA();
            dmaFlags = dmaFlags | 0x01;
            dmaBaseAddr = dmaBaseAddr & 0xF800;
          }
          else if (dmaFlags & 0x02) {
            // done reading attribute data in previous line,
            // now continue DMA to get character data
            delayedEvents0.stopDMA();
            dmaActive = true;
            delayedEvents.startDMA();
            dmaBaseAddr = dmaBaseAddr | 0x0400;
          }
        }
        break;
      case 104:                         // burst end
        videoOutputFlags &= uint8_t(0xF5);
        break;
      case 106:                         // horizontal blanking end
        videoOutputFlags &= uint8_t(0xDD);
        selectRenderFunction();
        break;
      case 108:                         // initialize DMA position
        if (renderWindow) {
          if (incrementingDMAPosition)
            dmaPositionReload = dmaPositionReload & dmaPosition;
          dmaPosition = dmaPositionReload;
          incrementingDMAPosition = true;
        }                               // initialize character position
        delayedEvents0.reloadCharacterPosition();
        break;
      case 110:                         // start DMA and/or bitmap fetch
        bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x02;
        if (!bitmapAddressDisableFlags)
          incrementingCharacterPosition = true;
        break;
      case 112:
        if (renderWindow | displayWindow | displayActive)
          videoShiftRegisterEnabled = true;
        break;
      }
      if (delayedEvents)
        processDelayedEvents(delayedEvents);
      // run external callbacks
      {
        TEDCallback *p = firstCallback0;
        while (p) {
          TEDCallback *nxt = p->nxt0;
          p->func(p->userData);
          p = nxt;
        }
      }
      // run CPU
      if (!cpuHaltedFlag) {
        M7501::run(cpu_clock_multiplier);
      }
      // perform DMA fetches on even cycle counts
      if (!(M7501::getIsCPURunning())) {
        if (cpuHaltedFlag) {
          memoryReadMap = tedDMAReadMap;
          (void) readMemory(uint16_t(dmaBaseAddr | dmaPosition));
          memoryReadMap = cpuMemoryReadMap;
        }
        if (characterColumn < 40) {
          if (dmaFlags & 0x01)
            attr_buf_tmp[characterColumn] = dataBusState;
          if (dmaFlags & 0x02)
            char_buf[characterColumn] = dataBusState;
        }
      }
      if (incrementingDMAPosition)
        dmaPosition = (dmaPosition + 1) & 0x03FF;
      // calculate video output
      {
        prv_video_buf_pos = video_buf_pos;
        uint8_t *bufp = &(video_buf[video_buf_pos]);
        int     n = current_render_func(*this, bufp, 0);
        video_buf_pos = video_buf_pos + n;
      }
      // check timer interrupts
      if (timer1_run) {
        if (!timer1_state) {
          tedRegisters[0x09] |= uint8_t(0x08);
          updateInterruptFlag();
          // reload timer
          timer1_state = timer1_reload_value;
        }
        // update timer 1 on even cycle count (886 kHz rate)
        timer1_state = (timer1_state - 1) & 0xFFFF;
      }
      if (!timer2_state) {
        if (timer2_run) {
          tedRegisters[0x09] |= uint8_t(0x10);
          updateInterruptFlag();
        }
      }
      if (!timer3_state) {
        if (timer3_run) {
          tedRegisters[0x09] |= uint8_t(0x40);
          updateInterruptFlag();
        }
      }
      // update horizontal position (reads are delayed by one cycle)
      tedRegisters[0x1E] = videoColumn;
      videoColumn |= uint8_t(0x01);
    }

    if (ted_disabled) {
      runOneCycle_freezeMode();
      return;
    }
    // -------- ODD HALF-CYCLE (FF1E bit 1 == 0) --------
    if (delayedEvents1) {
      DelayedEventsMask delayedEvents(delayedEvents1);
      delayedEvents1 = 0U;
      processDelayedEvents(delayedEvents);
    }
    switch (videoColumn) {
    case 43:                            // equalization pulse 1 end
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
        selectRenderFunction();
      }
      break;
    case 87:                            // increment flash counter
      if (videoLine == 205)
        delayedEvents1.incrementFlashCounter();
      break;
    case 95:                            // equalization pulse 2 start
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags | 0x80) ^ (vsyncFlags & 0x80));
        selectRenderFunction();
      }
      break;
    }
    // run external callbacks
    {
      TEDCallback *p = firstCallback1;
      while (p) {
        TEDCallback *nxt = p->nxt1;
        p->func(p->userData);
        p = nxt;
      }
    }
    // run CPU
    if (!singleClockModeFlags) {
      M7501::run(cpu_clock_multiplier);
    }
    else {
      // bitmap fetches are done on odd cycle counts
      // FIXME: if single clock mode is not turned on, the CPU and TED could
      // both use the address bus at the same time, setting it to the bitwise
      // AND of the two addresses
      if (!bitmapAddressDisableFlags) {
        // read bitmap data from memory
        uint16_t  addr_ = uint16_t(characterLine);
        if (!(tedRegisters[0x06] & 0x80)) {
          if (!(tedRegisters[0x06] & 0x20)) {   // normal character mode
            addr_ |= uint16_t(charset_base_addr
                              | (int(char_buf[characterColumn] & characterMask)
                                 << 3));
          }
          else {                                // normal bitmap mode
            addr_ |= uint16_t(bitmap_base_addr | (characterPosition << 3));
          }
        }
        else {
          if (!(tedRegisters[0x06] & 0x20)) {   // IC test / character mode
            addr_ |= uint16_t((int(attr_buf_tmp[characterColumn]) << 3)
                              | 0xF800);
          }
          else {                                // IC test / bitmap mode
            addr_ |= uint16_t(bitmap_base_addr
                              | ((characterPosition
                                  & (int(attr_buf_tmp[characterColumn])
                                     | 0xFF00)) << 3));
          }
        }
        if (!(tedBitmapReadMap & 0x80U)) {
          if ((addr_ & 0xFFE0) != 0xFF00) {
            // read bitmap data from RAM
            unsigned int  tmp = (unsigned int) memoryMapIndexTable[addr_ >> 12];
            uint8_t *p = segmentTable[memoryMapTable[tedBitmapReadMap | tmp]];
            dataBusState = p[addr_ & 0x3FFF];
          }
          else {
            // read bitmap data from TED registers
            (void) readMemory(addr_);
          }
        }
        else if (addr_ >= 0x8000) {
          if (addr_ < 0xFC00) {
            // read bitmap data from ROM
            unsigned int  tmp = (unsigned int) memoryMapIndexTable[addr_ >> 12];
            uint8_t *p = segmentTable[memoryMapTable[tedBitmapReadMap | tmp]];
            if (p)
              dataBusState = p[addr_ & 0x3FFF];
          }
          else if (addr_ < 0xFD00 || addr_ >= 0xFF00) {
            // read bitmap data from ROM or TED registers
            memoryReadMap = tedBitmapReadMap;
            (void) readMemory(addr_);
            memoryReadMap = cpuMemoryReadMap;
          }
        }
      }
      else
        idleMemoryRead();
    }
    // calculate video output
    {
      prv_video_buf_pos = video_buf_pos;
      uint8_t *bufp = &(video_buf[video_buf_pos]);
      int     n = current_render_func(*this, bufp, 4);
      video_buf_pos = video_buf_pos + n;
    }
    // update video shift register
    currentCharacter.bitmap_() = 0x00;
    if (videoShiftRegisterEnabled)
      currentCharacter = nextCharacter;
    if (characterColumn != 40) {
      nextCharacter.attr_() = attr_buf[characterColumn];
      attr_buf[characterColumn] = attr_buf_tmp[characterColumn];
      nextCharacter.char_() = char_buf[characterColumn];
      if (++characterColumn >= 64)
        characterColumn = (renderWindow ? 0 : 40);
    }
    else {
      nextCharacter.attr_() = 0x00;
      nextCharacter.char_() = 0x00;
    }
    nextCharacter.bitmap_() = dataBusState;
    nextCharacter.flags_() =
        uint8_t(characterPosition == cursor_position ? 0xFF : 0x0F) ^ videoMode;
    // update timer 2 and 3 on odd cycle count (886 kHz rate)
    if (timer2_run)
      timer2_state = (timer2_state - 1) & 0xFFFF;
    if (timer3_run)
      timer3_state = (timer3_state - 1) & 0xFFFF;
    // update sound generators on every 8th cycle (221 kHz)
    if (!cycle_count)
      calculateSoundOutput();
    // update horizontal position (reads are delayed by one cycle)
    tedRegisters[0x1E] = videoColumn;
    if (!ted_disabled)
      videoColumn = uint8_t(videoColumn != 113 ? (videoColumn + 1) : 0);
    videoColumn &= uint8_t(0x7E);
    cycle_count = (cycle_count + 1) & 3;
  }

  void TED7360::processDelayedEvents(uint32_t n)
  {
    if (n & 0x0000FFFFU) {
      if (n & 0x000000FFU) {
        if (n & 0x00000001U) {
          //   bit 0:   DRAM refresh on / external fetch single clock mode off
          singleClockModeFlags = uint8_t((singleClockModeFlags & 0x02) | 0x80);
          if (!(n & 0xFFFFFFFEU))
            return;
        }
        if (n & 0x00000002U) {
          //   bit 1:   DRAM refresh off
          singleClockModeFlags &= uint8_t(0x03);
          if (!(n & 0xFFFFFFFCU))
            return;
        }
        if (n & 0x00000004U) {
          //   bit 2:   increment video line
          if (renderWindow) {
            dmaFlags = dmaFlags & 0x80;
            // if done attribute DMA in the previous line,
            // continue with character DMA in next one
            if ((savedVideoLineDelay1 & 7) == int(verticalScroll) && dmaEnabled)
              dmaFlags = dmaFlags | 0x02;
            else if (dmaActive)
              delayedEvents0.stopDMADelay1();
          }
          videoOutputFlags &= uint8_t(0xF9);
          videoOutputFlags |=
              uint8_t(((savedVideoLine & int(~videoOutputFlags)) & 1) << 2);
          savedVideoLine = (videoLine + 1) & 0x01FF;
          renderWindow = renderWindow | dmaEnabled;
          if (savedVideoLine >= 226 && savedVideoLine <= 269) {
            checkVerticalEvents();
          }
          else if (savedVideoLine <= 8) {
            if ((tedRegisters[0x06] & uint8_t(0x10)) != uint8_t(0)) {
              if (savedVideoLine == 0)
                delayedEvents0.initializeDisplay();
              if (savedVideoLine == (8 - (int(tedRegisters[0x06] & 0x08) >> 1)))
                displayWindow = true;
            }
          }
          else if (savedVideoLine >= 200 && savedVideoLine <= 204) {
            if (savedVideoLine == (200 + (int(tedRegisters[0x06] & 0x08) >> 1)))
              displayWindow = false;
            if (savedVideoLine == 204) {
              renderWindow = false;
              incrementingCharacterLine =
                  incrementingCharacterLine & dmaEnabled;
            }
          }
          videoLine = savedVideoLine;
          prvCharacterLine = uint8_t(characterLine);
          delayedEvents1.updateVideoLineRegisters();
          delayedEvents0.incrementVideoLineCycle2();
          if (!(n & 0xFFFFFFF8U))
            return;
        }
        if (n & 0x00000008U) {
          //   bit 3:   update video line registers, check interrupt
          checkDMAPositionReset();
          // delay video line reads by one cycle
          tedRegisters[0x1D] = uint8_t(videoLine & 0x00FF);
          tedRegisters[0x1C] = uint8_t((videoLine & 0x0100) >> 8);
          checkVideoInterrupt();
          if (!(n & 0xFFFFFFF0U))
            return;
        }
        if (n & 0x00000010U) {
          //   bit 4:   stop incrementing DMA position
          incrementingDMAPosition = false;
          if (videoLine == 205)
            dmaPositionReload = 0x03FF;
          if (!(n & 0xFFFFFFE0U))
            return;
        }
        if (n & 0x00000020U) {
          //   bit 5:   latch DMA position
          dmaPositionReload = dmaPosition;
          if (!(n & 0xFFFFFFC0U))
            return;
        }
        if (n & 0x00000040U) {
          //   bit 6:   latch character position
          characterPositionReload = (characterPosition + 1) & 0x03FF;
          delayedEvents1.updateCharPosReloadRegisters();
          if (!(n & 0xFFFFFF80U))
            return;
        }
        if (n & 0x00000080U) {
          //   bit 7:   initialize display (at line 0, if FF06 bit 4 is set)
          dmaEnabled = true;
          if (!renderWindow) {
            renderWindow = true;
            delayedEvents1.resetVerticalSub();
            if (externalFetchSingleClockFlag)
              delayedEvents0.singleClockModeOn();
          }
          if (!(n & 0xFFFFFF00U))
            return;
        }
      }
      if (n & 0x0000FF00U) {
        if (n & 0x00000100U) {
          //   bit 8:   increment video line, second cycle
          savedVideoLineDelay1 = savedVideoLine;
          if (savedVideoLine == 203)
            dmaEnabled = false;
          if (savedVideoLine == 204) {  // end of display
            bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x02;
            singleClockModeFlags &= uint8_t(0x82);
            dmaFlags = 0x00;
          }
          else if (renderWindow) {
            if (((savedVideoLineDelay1 ^ int(tedRegisters[0x06])) & 7) == 0 &&
                dmaEnabled) {
              incrementingCharacterLine = true;
            }
            if (dmaFlags & 0x02)
              bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x01;
          }
          if (videoColumn != 100) {     // if horizontal counter was changed:
            DelayedEventsMask   tmp(n);
            tmp.setVerticalScroll();    // check DMA
            n = tmp;
          }
          if (!(n & 0xFFFFFE00U))
            return;
        }
        if (n & 0x00000200U) {
          //   bit 9:   increment vertical sub-address
          characterLine = (characterLine + 1) & 7;
          delayedEvents1.updateVerticalSubRegister();
          if (!(n & 0xFFFFFC00U))
            return;
        }
        if (n & 0x00000400U) {
          //   bit 10:  update vertical sub-address register (FF1F)
          tedRegisters[0x1F] =
              uint8_t((tedRegisters[0x1F] & 0xF8) | characterLine);
          if (!(n & 0xFFFFF800U))
            return;
        }
        if (n & 0x00000800U) {
          //   bit 11:  FF15 write
          colorRegisters[0] = tedRegisters[0x15];
          if (!(n & 0xFFFFF000U))
            return;
        }
        if (n & 0x00001000U) {
          //   bit 12:  FF16 write
          colorRegisters[1] = tedRegisters[0x16];
          if (!(n & 0xFFFFE000U))
            return;
        }
        if (n & 0x00002000U) {
          //   bit 13:  FF17 write
          colorRegisters[2] = tedRegisters[0x17];
          if (!(n & 0xFFFFC000U))
            return;
        }
        if (n & 0x00004000U) {
          //   bit 14:  FF18 write
          colorRegisters[3] = tedRegisters[0x18];
          if (!(n & 0xFFFF8000U))
            return;
        }
        if (n & 0x00008000U) {
          //   bit 15:  FF19 write
          colorRegisters[4] = tedRegisters[0x19];
          if (!(n & 0xFFFF0000U))
            return;
        }
      }
    }
    if (n & 0x00FF0000U) {
      if (n & 0x00010000U) {
        //   bit 16:  reload character position
        savedCharacterPosition = characterPositionReload;
        characterPosition = savedCharacterPosition;
        if (!(n & 0xFFFE0000U))
          return;
      }
      if (n & 0x00020000U) {
        //   bit 17:  internal single clock mode flag on
        singleClockModeFlags |= uint8_t(0x01);
        if (!(n & 0xFFFC0000U))
          return;
      }
      if (n & 0x00040000U) {
        //   bit 18:  DMA cycle 1
        singleClockModeFlags |= uint8_t(0x01);
        delayedEvents0.dmaCycle(2);
        if (!(n & 0xFFF80000U))
          return;
      }
      if (n & 0x00080000U) {
        //   bit 19:  DMA cycle 2
        M7501::setIsCPURunning(false);
        delayedEvents0.dmaCycle(3);
        if (!(n & 0xFFF00000U))
          return;
      }
      if (n & 0x00100000U) {
        //   bit 20:  DMA cycle 3
        delayedEvents0.dmaCycle(4);
        if (!(n & 0xFFE00000U))
          return;
      }
      if (n & 0x00200000U) {
        //   bit 21:  DMA cycle 4
        delayedEvents0.dmaCycle(5);
        if (!(n & 0xFFC00000U))
          return;
      }
      if (n & 0x00400000U) {
        //   bit 22:  DMA cycle 5
        cpuHaltedFlag = true;
        if (!(n & 0xFF800000U))
          return;
      }
      if (n & 0x00800000U) {
        //   bit 23:  abort an already started DMA (on writing to FF06)
        delayedEvents0.stopDMA();
        dmaActive = false;
        cpuHaltedFlag = false;
        M7501::setIsCPURunning(true);
        if (!(n & 0xFF000000U))
          return;
      }
    }
    if (n & 0x01000000U) {
      //   bit 24:  start timer 2
      timer2_run = true;
      if (!(n & 0xFE000000U))
        return;
    }
    if (n & 0x02000000U) {
      //   bit 25:  vertical scroll write
      verticalScroll = tedRegisters[0x06] & 0x07;
      if (renderWindow) {
        // check if DMA should be requested
        if ((savedVideoLineDelay1 & 7) == int(verticalScroll)) {
          if (dmaEnabled) {
            incrementingCharacterLine = true;
            if (videoColumn == 98)
              delayedEvents0.incrementVerticalSub();
            if ((dmaFlags & 0x80) != 0) {
              if (!dmaActive) {
                dmaActive = true;
                singleClockModeFlags |= uint8_t(0x01);
                delayedEvents0.dmaCycle(2);
              }
              dmaFlags = dmaFlags | 0x01;
              dmaBaseAddr = dmaBaseAddr & 0xF800;
            }
          }
        }
        else {
          dmaFlags = dmaFlags & 0x82;
          if (!(dmaFlags & 0x03)) {     // abort an already started DMA transfer
            if (dmaActive)
              delayedEvents0.stopDMADelay1();
          }
          else                          // or continue with character DMA
            dmaBaseAddr = dmaBaseAddr | 0x0400;
        }
      }
      if (!(n & 0xFC000000U))
        return;
    }
    if (n & 0x04000000U) {
      //   bit 26:  horizontal scroll write
      horizontalScroll = tedRegisters[0x07] & 0x07;
      if (!(n & 0xF8000000U))
        return;
    }
    if (n & 0x08000000U) {
      //   bit 27:  select renderer
      switch (videoMode) {
      case 0x00:
        render_func = &render_char_std;
        break;
      case 0x01:
        render_func = &render_char_MCM;
        break;
      case 0x02:
        render_func = &render_BMM_hires;
        break;
      case 0x03:
        render_func = &render_BMM_multicolor;
        break;
      case 0x04:
        render_func = &render_char_ECM;
        break;
      case 0x05:
        render_func = &render_blank;
        break;
      case 0x06:
        render_func = &render_blank;
        break;
      case 0x07:
        render_func = &render_blank;
        break;
      case 0x08:
        render_func = &render_char_std;
        break;
      case 0x09:
        render_func = &render_char_MCM;
        break;
      case 0x0A:
        render_func = &render_BMM_hires;
        break;
      case 0x0B:
        render_func = &render_BMM_multicolor;
        break;
      case 0x0C:
        render_func = &render_char_ECM;
        break;
      case 0x0D:
        render_func = &render_blank;
        break;
      case 0x0E:
        render_func = &render_blank;
        break;
      case 0x0F:
        render_func = &render_blank;
        break;
      }
      selectRenderFunction();
      if (!(n & 0xF0000000U))
        return;
    }
    if (n & 0x10000000U) {
      //   bit 28:  FF13 write (single clock mode control)
      singleClockModeFlags = uint8_t((singleClockModeFlags & 0x81)
                                     | (tedRegisters[0x13] & 0x02));
      if (!(n & 0xE0000000U))
        return;
    }
    if (n & 0x20000000U) {
      //   bit 29:  update character position reload registers (FF1A, FF1B)
      tedRegisters[0x1A] = uint8_t(characterPositionReload >> 8);
      tedRegisters[0x1B] = uint8_t(characterPositionReload & 0xFF);
      if (!(n & 0xC0000000U))
        return;
    }
    if (n & 0x40000000U) {
      //   bit 30:  reset vertical sub-address
      characterLine = 7;
      prvCharacterLine = uint8_t(7);
      delayedEvents0.updateVerticalSubRegister();
    }
    if (n & 0x80000000U) {
      //   bit 31:  increment flash counter
      tedRegisters[0x1F] = (tedRegisters[0x1F] & uint8_t(0x7F)) + uint8_t(0x08);
      if (tedRegisters[0x1F] & uint8_t(0x80))
        flashState = uint8_t(flashState == 0x00 ? 0xFF : 0x00);
    }
  }

  void TED7360::checkVerticalEvents()
  {
    bool    updateRenderFunctionFlag = false;
    if (!(videoOutputFlags & uint8_t(0x01))) {            // PAL
      switch (savedVideoLine) {
      case 251:                   // vertical blanking, equalization start
        videoOutputFlags = uint8_t((videoOutputFlags & 0xF7) | 0x10);
        vsyncFlags |= uint8_t(0x40);
        updateRenderFunctionFlag = true;
        break;
      case 254:                   // vertical sync start
        videoOutputFlags |= uint8_t(0x40);
        vsyncFlags |= uint8_t(0x80);
        break;
      case 257:                   // vertical sync end
        videoOutputFlags &= uint8_t(0xBD);
        vsyncFlags &= uint8_t(0x40);
        break;
      case 260:                   // equalization end
        videoOutputFlags &= uint8_t(0x7D);
        vsyncFlags &= uint8_t(0x80);
        updateRenderFunctionFlag = true;
        break;
      case 269:                   // vertical blanking end
        videoOutputFlags &= uint8_t(0xED);
        if (videoColumn == 98)
          videoOutputFlags |= uint8_t(0x08);
        updateRenderFunctionFlag = true;
        break;
      }
    }
    else {                                                // NTSC
      switch (savedVideoLine) {
      case 226:                   // vertical blanking, equalization start
        videoOutputFlags = uint8_t((videoOutputFlags & 0xF7) | 0x10);
        vsyncFlags |= uint8_t(0x40);
        updateRenderFunctionFlag = true;
        break;
      case 229:                   // vertical sync start
        videoOutputFlags |= uint8_t(0x40);
        vsyncFlags |= uint8_t(0x80);
        break;
      case 232:                   // vertical sync end
        videoOutputFlags &= uint8_t(0xBD);
        vsyncFlags &= uint8_t(0x40);
        break;
      case 235:                   // equalization end
        videoOutputFlags &= uint8_t(0x7D);
        vsyncFlags &= uint8_t(0x80);
        updateRenderFunctionFlag = true;
        break;
      case 244:                   // vertical blanking end
        videoOutputFlags &= uint8_t(0xED);
        if (videoColumn == 98)
          videoOutputFlags |= uint8_t(0x08);
        updateRenderFunctionFlag = true;
        break;
      }
    }
    if (updateRenderFunctionFlag)
      selectRenderFunction();
  }

}       // namespace Plus4

