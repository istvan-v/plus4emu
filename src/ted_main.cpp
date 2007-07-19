
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
#include "cpu.hpp"
#include "ted.hpp"

namespace Plus4 {

  void TED7360::runOneCycle_freezeMode()
  {
    tedRegisterWriteMask = 0U;
    dmaCycleCounter = 0;
    M7501::setIsCPURunning(true);
    TEDCallback *p = firstCallback0;
    while (p) {
      TEDCallback *nxt = p->nxt0;
      p->func(p->userData);
      p = nxt;
    }
    M7501::run(cpu_clock_multiplier);
    p = firstCallback1;
    while (p) {
      TEDCallback *nxt = p->nxt1;
      p->func(p->userData);
      p = nxt;
    }
    if (!cycle_count)
      playSample(0);
    prv_video_buf_pos = video_buf_pos;
    video_buf[video_buf_pos] = uint8_t((videoOutputFlags & 0x01) | 0x30);
    video_buf[video_buf_pos + 1] = 0x00;
    video_buf[video_buf_pos + 2] = uint8_t((videoOutputFlags & 0x01) | 0x30);
    video_buf[video_buf_pos + 3] = 0x00;
    video_buf_pos = video_buf_pos + 4;
    if (!prvSingleClockModeFlags)
      videoColumn ^= uint8_t(0x01);
    tedRegisters[0x1E] = videoColumn;
    if (!ted_disabled && !prvSingleClockModeFlags) {
      videoColumn =
          uint8_t(videoColumn != 113 ? ((videoColumn + 1) & 0x7F) : 0);
    }
    cycle_count = (cycle_count + 1) & 3;
    prvSingleClockModeFlags = singleClockModeFlags;
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
      switch (videoColumn) {
      case 0:                           // start display (40 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
        }
        break;
      case 2:                           // start display (38 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
        }
        break;
      case 38:                          // equalization pulse 1 start
        if (vsyncFlags) {
          videoOutputFlags =
              uint8_t((videoOutputFlags | 0x80) ^ (vsyncFlags & 0x80));
        }
        break;
      case 74:                          // DRAM refresh start
        singleClockModeFlags |= uint8_t(0x01);
        break;
      case 76:
        bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x01;
        renderingDisplay = false;
        // terminate DMA transfer
        dmaCycleCounter = 0;
        M7501::setIsCPURunning(true);
        break;
      case 78:                          // end of display (38 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0))
          displayActive = false;
        break;
      case 80:                          // end of display (40 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0))
          displayActive = false;
        videoShiftRegisterEnabled = displayActive;
        break;
      case 84:                          // DRAM refresh end
        singleClockModeFlags &= uint8_t(0x02);
        break;
      case 88:                          // horizontal blanking start
        videoOutputFlags |= uint8_t(0x20);
        break;
      case 90:                          // horizontal sync start
        if (!vsyncFlags)
          videoOutputFlags |= uint8_t(0x80);
        break;
      case 96:
        if (renderWindow) {
          // if done attribute DMA in this line, continue with character
          // DMA in next one
          if ((uint8_t(savedVideoLine) ^ tedRegisters[0x06]) & 0x07)
            dmaFlags = 0;
          else if (dmaEnabled)
            dmaFlags = 2;
        }
        videoOutputFlags = uint8_t((videoOutputFlags & 0xF9)
                                   | ((((savedVideoLine | videoOutputFlags)
                                        & 0x01) ^ 0x01) << 2));
        break;
      case 98:                          // increment line number
        if (!(videoOutputFlags & uint8_t(0x01))) {              // PAL
          savedVideoLine =
              (savedVideoLine != 311 ? ((videoLine + 1) & 0x01FF) : 0);
          switch (savedVideoLine) {
          case 251:                     // vertical blanking, equalization start
            videoOutputFlags |= uint8_t(0x10);
            vsyncFlags |= uint8_t(0x40);
            break;
          case 254:                     // vertical sync start
            videoOutputFlags |= uint8_t(0x40);
            vsyncFlags |= uint8_t(0x80);
            break;
          case 257:                     // vertical sync end
            videoOutputFlags &= uint8_t(0xBD);
            vsyncFlags &= uint8_t(0x40);
            break;
          case 260:                     // equalization end
            vsyncFlags &= uint8_t(0x80);
            break;
          case 269:                     // vertical blanking end
            videoOutputFlags &= uint8_t(0xED);
            break;
          }
        }
        else {                                                  // NTSC
          savedVideoLine =
              (savedVideoLine != 261 ? ((videoLine + 1) & 0x01FF) : 0);
          switch (savedVideoLine) {
          case 226:                     // vertical blanking, equalization start
            videoOutputFlags |= uint8_t(0x10);
            vsyncFlags |= uint8_t(0x40);
            break;
          case 229:                     // vertical sync start
            videoOutputFlags |= uint8_t(0x40);
            vsyncFlags |= uint8_t(0x80);
            break;
          case 232:                     // vertical sync end
            videoOutputFlags &= uint8_t(0xBD);
            vsyncFlags &= uint8_t(0x40);
            break;
          case 235:                     // equalization end
            vsyncFlags &= uint8_t(0x80);
            break;
          case 244:                     // vertical blanking end
            videoOutputFlags &= uint8_t(0xED);
            break;
          }
        }
        videoLine = savedVideoLine;
        if (savedVideoLine == 0) {      // end of screen
          characterPosition = 0x0000;
          characterPositionReload = 0x0000;
        }
        prvCharacterLine = uint8_t(characterLine);
        if (!vsyncFlags)                // horizontal sync end
          videoOutputFlags &= uint8_t(0x7D);
        if (!(videoOutputFlags & 0x10)) // burst start
          videoOutputFlags |= uint8_t(0x08);
        break;
      case 100:
        if (dmaWindow)                  // increment character sub-line
          characterLine = (characterLine + 1) & 7;
        if (savedVideoLine == 203)
          dmaEnabled = false;
        if (savedVideoLine == 204) {    // end of display
          renderWindow = false;
          dmaWindow = false;
          bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x02;
        }
        else if (renderWindow) {
          singleClockModeFlags |= uint8_t(0x01);
          if (dmaFlags & 2)
            bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x01;
          // check if DMA should be requested
          if (!((uint8_t(savedVideoLine) ^ tedRegisters[0x06]) & 0x07) &&
              dmaEnabled) {
            // start a new DMA at character line 7
            dmaWindow = true;
            dmaCycleCounter = 1;
            dmaFlags = dmaFlags | 1;
            dmaPosition = dmaPosition & 0x03FF;
          }
          else if (dmaFlags & 2) {
            // done reading attribute data in previous line,
            // now continue DMA to get character data
            dmaCycleCounter = 1;
            dmaPosition = dmaPosition | 0x0400;
          }
        }
        if (vsyncFlags) {               // equalization pulse 2 end
          videoOutputFlags =
              uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
        }
        break;
      case 102:                         // external fetch single clock start
        if (renderWindow) {
          if (savedVideoLine == 0) {    // initialize character sub-line
            // FIXME: this check is a hack
            if (bitmapAddressDisableFlags & 0x02) {
              characterLine = 7;
              prvCharacterLine = uint8_t(7);
              tedRegisters[0x1F] |= uint8_t(0x07);
            }
          }
        }
        characterColumn = 0x3C;
        break;
      case 104:                         // burst end
        videoOutputFlags &= uint8_t(0xF5);
        break;
      case 106:                         // horizontal blanking end
        videoOutputFlags &= uint8_t(0xDD);
        break;
      case 108:
        dmaPosition = (dmaPosition & 0x0400) | dmaPositionReload;
        incrementingDMAPosition = renderWindow;
        break;
      case 110:                         // start DMA and/or bitmap fetch
        if (renderWindow | displayWindow | displayActive) {
          bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x02;
          renderingDisplay = true;
        }                               // initialize character position
        characterPosition = characterPositionReload;
        break;
      case 112:
        if (renderWindow | displayWindow | displayActive) {
          renderingDisplay = true;
          videoShiftRegisterEnabled = true;
        }
        break;
      }
      // initialize DMA if requested
      if (dmaCycleCounter != uint8_t(0)) {
        dmaCycleCounter++;
        switch (dmaCycleCounter) {
        case 2:
          singleClockModeFlags |= uint8_t(0x01);
          break;
        case 4:
          M7501::setIsCPURunning(false);
          break;
        }
      }
      // run CPU and render display
      tedRegisterWriteMask = 0U;
      {
        TEDCallback *p = firstCallback0;
        while (p) {
          TEDCallback *nxt = p->nxt0;
          p->func(p->userData);
          p = nxt;
        }
      }
      if (dmaCycleCounter < 7) {
        if (tedRegisters[0x09] & tedRegisters[0x0A])
          M7501::interruptRequest();
        M7501::run(cpu_clock_multiplier);
      }
      // perform DMA fetches on even cycle counts
      if (incrementingDMAPosition) {
        if (dmaCycleCounter >= 4) {
          if (characterColumn < uint8_t(40)) {
            if (dmaCycleCounter >= 7) {
              memoryReadMap = tedDMAReadMap;
              (void) readMemory(uint16_t(attr_base_addr | dmaPosition));
              memoryReadMap = cpuMemoryReadMap;
            }
            if (dmaFlags & 1)
              attr_buf_tmp[characterColumn] = dataBusState;
            if (dmaFlags & 2)
              char_buf[characterColumn] = dataBusState;
          }
        }
        if (videoColumn != 74)
          dmaPosition = (dmaPosition & 0x0400) | ((dmaPosition + 1) & 0x03FF);
      }
      // calculate video output
      {
        prv_video_buf_pos = video_buf_pos;
        uint8_t *bufp = &(video_buf[video_buf_pos]);
        if (videoOutputFlags & uint8_t(0xB0)) {
          bufp[0] = videoOutputFlags;
          bufp[1] = 0x00;
          video_buf_pos = video_buf_pos + 2;
          if (videoShiftRegisterEnabled)
            render_invalid_mode(*this, (uint8_t *) 0, 0);
        }
        else if (!displayActive) {
          if (!(tedRegisterWriteMask & 0x02000000U)) {
            bufp[0] = videoOutputFlags;
            bufp[1] = tedRegisters[0x19];
            video_buf_pos = video_buf_pos + 2;
          }
          else {
            bufp[0] = videoOutputFlags | uint8_t(0x02);
            bufp[1] = 0xFF;
            uint8_t c = tedRegisters[0x19];
            bufp[2] = c;
            bufp[3] = c;
            bufp[4] = c;
            video_buf_pos = video_buf_pos + 5;
          }
          if (videoShiftRegisterEnabled)
            render_invalid_mode(*this, (uint8_t *) 0, 0);
        }
        else {
          bufp[0] = videoOutputFlags | uint8_t(0x02);
          prv_render_func(*this, &(bufp[1]), 0);
          video_buf_pos = video_buf_pos + 5;
        }
      }
      // delay video mode changes by one cycle
      prv_render_func = render_func;
      // check timer interrupts
      if (timer1_run) {
        if (!timer1_state) {
          tedRegisters[0x09] |= uint8_t(0x08);
          // reload timer
          timer1_state = timer1_reload_value;
        }
        // update timer 1 on even cycle count (886 kHz rate)
        timer1_state = (timer1_state - 1) & 0xFFFF;
      }
      if (!timer2_state) {
        if (timer2_run)
          tedRegisters[0x09] |= uint8_t(0x10);
      }
      if (!timer3_state) {
        if (timer3_run)
          tedRegisters[0x09] |= uint8_t(0x40);
      }
      // update horizontal position (reads are delayed by one cycle)
      tedRegisters[0x1E] = videoColumn;
      videoColumn |= uint8_t(0x01);
    }

    // -------- ODD HALF-CYCLE (FF1E bit 1 == 0) --------
    switch (videoColumn) {
    case 43:                            // equalization pulse 1 end
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
      }
      break;
    case 71:                            // update DMA read position
      if (renderWindow && characterLine == 6)
        dmaPositionReload =
            (dmaPosition + (incrementingDMAPosition ? 1 : 0)) & 0x03FF;
      break;
    case 75:
      incrementingDMAPosition = false;
      // update character position reload (FF1A, FF1B)
      if (!bitmapAddressDisableFlags && prvCharacterLine == uint8_t(6)) {
        if (!(tedRegisterWriteMask & 0x0C000000U))
          characterPositionReload = (characterPosition + 1) & 0x03FF;
      }
      break;
    case 89:                            // increment flash counter
      if (videoLine == 205) {
        tedRegisters[0x1F] =
            (tedRegisters[0x1F] & uint8_t(0x7F)) + uint8_t(0x08);
        if (tedRegisters[0x1F] & uint8_t(0x80))
          flashState = uint8_t(flashState == 0x00 ? 0xFF : 0x00);
      }
      break;
    case 95:                            // equalization pulse 2 start
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags | 0x80) ^ (vsyncFlags & 0x80));
      }
      break;
    case 99:
      if ((tedRegisters[0x06] & uint8_t(0x10)) != uint8_t(0)) {
        if (savedVideoLine == 0) {
          renderWindow = true;
          dmaEnabled = true;
        }
        else if ((savedVideoLine == 4 &&
                  (tedRegisters[0x06] & uint8_t(0x08)) != uint8_t(0)) ||
                 (savedVideoLine == 8 &&
                  (tedRegisters[0x06] & uint8_t(0x08)) == uint8_t(0)))
          displayWindow = true;
      }
      if ((savedVideoLine == 200 &&
           (tedRegisters[0x06] & uint8_t(0x08)) == uint8_t(0)) ||
          (savedVideoLine == 204 &&
           (tedRegisters[0x06] & uint8_t(0x08)) != uint8_t(0)))
        displayWindow = false;
      checkDMAPositionReset();
      // delay video line reads by one cycle
      tedRegisters[0x1D] = uint8_t(videoLine & 0x00FF);
      tedRegisters[0x1C] = uint8_t((videoLine & 0x0100) >> 8);
      checkVideoInterrupt();
      break;
    case 101:
      // delay character sub-line reads by one cycle
      tedRegisters[0x1F] = (tedRegisters[0x1F] & 0xF8) | uint8_t(characterLine);
      break;
    }
    // run CPU
    tedRegisterWriteMask = 0U;
    {
      TEDCallback *p = firstCallback1;
      while (p) {
        TEDCallback *nxt = p->nxt1;
        p->func(p->userData);
        p = nxt;
      }
    }
    if (!prvSingleClockModeFlags) {
      if (tedRegisters[0x09] & tedRegisters[0x0A])
        M7501::interruptRequest();
      M7501::run(cpu_clock_multiplier);
    }
    // calculate video output
    {
      prv_video_buf_pos = video_buf_pos;
      uint8_t *bufp = &(video_buf[video_buf_pos]);
      if (videoOutputFlags & uint8_t(0xB0)) {
        bufp[0] = videoOutputFlags;
        bufp[1] = 0x00;
        video_buf_pos = video_buf_pos + 2;
        if (videoShiftRegisterEnabled)
          render_invalid_mode(*this, (uint8_t *) 0, 4);
      }
      else if (!displayActive) {
        if (!(tedRegisterWriteMask & 0x02000000U)) {
          bufp[0] = videoOutputFlags;
          bufp[1] = tedRegisters[0x19];
          video_buf_pos = video_buf_pos + 2;
        }
        else {
          bufp[0] = videoOutputFlags | uint8_t(0x02);
          bufp[1] = 0xFF;
          uint8_t c = tedRegisters[0x19];
          bufp[2] = c;
          bufp[3] = c;
          bufp[4] = c;
          video_buf_pos = video_buf_pos + 5;
        }
        if (videoShiftRegisterEnabled)
          render_invalid_mode(*this, (uint8_t *) 0, 4);
      }
      else {
        bufp[0] = videoOutputFlags | uint8_t(0x02);
        prv_render_func(*this, &(bufp[1]), 4);
        video_buf_pos = video_buf_pos + 5;
      }
    }
    // delay video mode changes by one cycle
    prv_render_func = render_func;
    // delay horizontal scroll changes
    horiz_scroll = tedRegisters[0x07] & 0x07;
    // bitmap fetches and rendering display are done on odd cycle counts
    if (videoShiftRegisterEnabled) {
      currentCharacter = nextCharacter;
      nextCharacter.bitmap_() = 0x00;
    }
    if (renderingDisplay) {
      nextCharacter.attr_() = attr_buf[characterColumn];
      nextCharacter.char_() = char_buf[characterColumn];
      // read bitmap data from memory
      if (!bitmapAddressDisableFlags) {
        uint16_t  addr_ = uint16_t(characterLine);
        if (!(tedRegisters[0x06] & uint8_t(0x80))) {
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | (characterPosition << 3));
          else
            addr_ |= uint16_t(charset_base_addr
                              | (int(nextCharacter.char_() & characterMask)
                                 << 3));
        }
        else {
          // IC test mode (FF06 bit 7 set)
          int     tmp = (int(attr_buf_tmp[characterColumn]) << 3) | 0xF800;
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | ((characterPosition << 3) & tmp));
          else
            addr_ |= uint16_t(tmp);
        }
        if (addr_ >= uint16_t(0x8000) || !(tedBitmapReadMap & 0x80U)) {
          memoryReadMap = tedBitmapReadMap;
          (void) readMemory(addr_);
        }
      }
      else if (prvSingleClockModeFlags) {
        memoryReadMap = tedDMAReadMap;  // not sure if this is correct
        (void) readMemory(0xFFFF);
      }
      memoryReadMap = cpuMemoryReadMap;
      nextCharacter.bitmap_() = dataBusState;
      nextCharacter.cursor_() =
          (characterPosition == cursor_position ? 0xFF : 0x00);
      if (!(videoMode & 0x70)) {
        if (nextCharacter.attr_() & 0x80) {
          // FIXME: this should probably be done when loading the bitmap
          // shift register in the render functions
          if (!flashState && !nextCharacter.cursor_())
            nextCharacter.bitmap_() = 0x00;
        }
        if (!(videoMode & 0x80)) {
          if (nextCharacter.char_() & 0x80)
            nextCharacter.bitmap_() ^= uint8_t(0xFF);
        }
      }
      attr_buf[characterColumn] = attr_buf_tmp[characterColumn];
      if (!bitmapAddressDisableFlags)
        characterPosition = (characterPosition + 1) & 0x03FF;
      else
        characterPosition = 0x03FF;
    }
    characterColumn = (characterColumn + 1) & 0x3F;
    // update timer 2 and 3 on odd cycle count (886 kHz rate)
    if (timer2_run) {
      if (!(tedRegisterWriteMask & 0x00000008U))
        timer2_state = (timer2_state - 1) & 0xFFFF;
    }
    if (timer3_run)
      timer3_state = (timer3_state - 1) & 0xFFFF;
    // update sound generators on every 8th cycle (221 kHz)
    if (!cycle_count) {
      int     sound_output = 0;
      int     sound_volume;
      uint8_t sound_register = tedRegisters[0x11];
      sound_volume = int(sound_register & uint8_t(0x0F)) << 10;
      sound_volume = (sound_volume < 8192 ? sound_volume : 8192);
      if (sound_register & uint8_t(0x80)) {
        // DAC mode
        sound_channel_1_cnt = sound_channel_1_reload;
        sound_channel_2_cnt = sound_channel_2_reload;
        sound_channel_1_state = uint8_t(1);
        sound_channel_2_state = uint8_t(1);
        sound_channel_2_noise_state = uint8_t(0xFF);
        sound_channel_2_noise_output = uint8_t(1);
      }
      else {
        sound_channel_1_cnt = (sound_channel_1_cnt + 1) & 1023;
        if (sound_channel_1_cnt == 1023) {
          // channel 1 square wave
          sound_channel_1_cnt = sound_channel_1_reload;
          if (sound_channel_1_reload != 1022) {
            sound_channel_1_state ^= uint8_t(1);
            sound_channel_1_state &= uint8_t(1);
          }
        }
        sound_channel_2_cnt = (sound_channel_2_cnt + 1) & 1023;
        if (sound_channel_2_cnt == 1023) {
          // channel 2 square wave
          sound_channel_2_cnt = sound_channel_2_reload;
          if (sound_channel_2_reload != 1022) {
            sound_channel_2_state ^= uint8_t(1);
            sound_channel_2_state &= uint8_t(1);
            // channel 2 noise, 8 bit polycnt (10110011)
            uint8_t tmp = sound_channel_2_noise_state & uint8_t(0xB3);
            tmp = tmp ^ (tmp >> 1);
            tmp = tmp ^ (tmp >> 2);
            tmp = (tmp ^ (tmp >> 4)) & uint8_t(1);
            sound_channel_2_noise_output ^= tmp;
            sound_channel_2_noise_output &= uint8_t(1);
            sound_channel_2_noise_state <<= 1;
            sound_channel_2_noise_state |= sound_channel_2_noise_output;
          }
        }
      }
      // mix sound outputs
      if (sound_register & uint8_t(0x10))
        sound_output += (sound_channel_1_state ? sound_volume : 0);
      if (sound_register & uint8_t(0x20))
        sound_output += (sound_channel_2_state ? sound_volume : 0);
      else if (sound_register & uint8_t(0x40))
        sound_output += (sound_channel_2_noise_output ? 0 : sound_volume);
      // send sound output signal (sample rate = 221 kHz)
      playSample(int16_t(sound_output));
    }
    // update horizontal position (reads are delayed by one cycle)
    tedRegisters[0x1E] = videoColumn;
    if (!ted_disabled) {
      videoColumn =
          uint8_t(videoColumn != 113 ? ((videoColumn + 1) & 0x7F) : 0);
    }
    cycle_count = (cycle_count + 1) & 3;
    // delay single/double clock mode switching by one cycle
    prvSingleClockModeFlags = singleClockModeFlags;
  }

}       // namespace Plus4

