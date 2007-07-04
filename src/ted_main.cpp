
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

  void TED7360::runOneCycle()
  {
    if (video_buf_pos >= 450) {
      videoOutputCallback(&(video_buf[0]), video_buf_pos);
      video_buf_pos = 0;
    }
    {
      TEDCallback *p = firstCallback0;
      while (p) {
        TEDCallback *nxt = p->nxt0;
        p->func(p->userData);
        p = nxt;
      }
    }
    if (ted_disabled) {
      tedRegisterWriteMask = 0U;
      dmaCycleCounter = 0;
      M7501::setIsCPURunning(true);
      M7501::run(cpu_clock_multiplier);
      TEDCallback *p = firstCallback1;
      while (p) {
        TEDCallback *nxt = p->nxt1;
        p->func(p->userData);
        p = nxt;
      }
      if (!cycle_count)
        playSample(0);
      video_buf[video_buf_pos] = uint8_t((videoOutputFlags & 0x01) | 0x30);
      video_buf[video_buf_pos + 1] = 0x00;
      video_buf[video_buf_pos + 2] = uint8_t((videoOutputFlags & 0x01) | 0x30);
      video_buf[video_buf_pos + 3] = 0x00;
      video_buf_pos = video_buf_pos + 4;
      if (!ted_disabled)
        video_column |= uint8_t(0x01);
      cycle_count = (cycle_count + 1) & 3;
      return;
    }

    if (video_column & uint8_t(0x01)) {
      // -------- ODD HALF-CYCLE (FF1E bit 1 == 1) --------
      switch (video_column) {
      case 1:                           // start display (38 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0)) {
          displayActive = true;
        }
        break;
      case 37:                          // horizontal equalization 1 start
        if (vsyncFlags) {
          videoOutputFlags =
              uint8_t((videoOutputFlags & 0x7F) | ((vsyncFlags ^ 0xFF) & 0x80));
        }
        break;
      case 73:                          // DRAM refresh start
        singleClockModeFlags |= uint8_t(0x01);
        break;
      case 75:
        bitmapAddressDisableFlags = bitmapAddressDisableFlags | 0x01;
        renderingDisplay = false;
        // terminate DMA transfer
        dmaCycleCounter = 0;
        M7501::setIsCPURunning(true);
        break;
      case 77:                          // end of display (38 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) == uint8_t(0))
          displayActive = false;
        break;
      case 79:                          // end of display (40 column mode)
        if ((tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0))
          displayActive = false;
        videoShiftRegisterEnabled = displayActive;
        break;
      case 83:                          // DRAM refresh end
        singleClockModeFlags &= uint8_t(0x02);
        break;
      case 87:                          // horizontal blanking start
        videoOutputFlags |= uint8_t(0x20);
        break;
      case 89:                          // horizontal sync start
        if (!vsyncFlags)
          videoOutputFlags |= uint8_t(0x80);
        break;
      case 95:
        if (renderWindow) {
          // if done attribute DMA in this line, continue with character
          // DMA in next one
          if ((uint8_t(savedVideoLine) ^ tedRegisters[0x06]) & 0x07)
            dmaFlags = 0;
          else if (dmaEnabled)
            dmaFlags = 2;
        }
        break;
      case 97:                          // increment line number
        if (!(videoOutputFlags & uint8_t(0x01))) {              // PAL
          savedVideoLine =
              (savedVideoLine != 311 ? ((video_line + 1) & 0x01FF) : 0);
          videoOutputFlags = uint8_t((videoOutputFlags & 0xF9)
                                     | ((savedVideoLine & 0x01) << 2));
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
              (savedVideoLine != 261 ? ((video_line + 1) & 0x01FF) : 0);
          videoOutputFlags &= uint8_t(0xF9);
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
        if (savedVideoLine == 0) {      // end of screen
          character_position = 0x0000;
          character_position_reload = 0x0000;
        }
        prvCharacterLine = uint8_t(character_line);
        if (!vsyncFlags)                // horizontal sync end
          videoOutputFlags &= uint8_t(0x7D);
        if (!(videoOutputFlags & 0x10)) // burst start
          videoOutputFlags |= uint8_t(0x08);
        break;
      case 99:
        if (dmaWindow)                  // increment character sub-line
          character_line = (character_line + 1) & 7;
        if (video_line == 205) {
          dma_position = 0x03FF;
          dma_position_reload = 0x03FF;
          dmaFlags = 0;
        }
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
            dma_position = dma_position & 0x03FF;
          }
          else if (dmaFlags & 2) {
            // done reading attribute data in previous line,
            // now continue DMA to get character data
            dmaCycleCounter = 1;
            dma_position = dma_position | 0x0400;
          }
        }
        if (vsyncFlags) {               // horizontal equalization 2 end
          videoOutputFlags =
              uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
        }
        break;
      case 101:                         // external fetch single clock start
        if (renderWindow) {
          if (savedVideoLine == 0) {    // initialize character sub-line
            // FIXME: this check is a hack
            if (bitmapAddressDisableFlags & 0x02) {
              character_line = 7;
              prvCharacterLine = uint8_t(7);
            }
          }
        }
        character_column = 0x3C;
        break;
      case 103:                         // burst end
        videoOutputFlags &= uint8_t(0xF5);
        break;
      case 105:                         // horizontal blanking end
        videoOutputFlags &= uint8_t(0xDD);
        break;
      case 107:
        dma_position = (dma_position & 0x0400) | dma_position_reload;
        incrementingDMAPosition = renderWindow;
        break;
      case 109:                         // start DMA and/or bitmap fetch
        if (renderWindow | displayWindow | displayActive) {
          bitmapAddressDisableFlags = bitmapAddressDisableFlags & 0x02;
          renderingDisplay = true;
        }                               // initialize character position
        character_position = character_position_reload;
        break;
      case 111:
        if (renderWindow | displayWindow | displayActive) {
          renderingDisplay = true;
          videoShiftRegisterEnabled = true;
        }
        break;
      case 113:                         // start display (40 column mode)
        if (displayWindow &&
            (tedRegisters[0x07] & uint8_t(0x08)) != uint8_t(0)) {
          displayActive = true;
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
      // check for video interrupt
      if (video_line == videoInterruptLine) {
        if (!prvVideoInterruptState) {
          prvVideoInterruptState = true;
          tedRegisters[0x09] |= uint8_t(0x02);
        }
      }
      else
        prvVideoInterruptState = false;
      // run CPU and render display
      tedRegisterWriteMask = 0U;
      if (dmaCycleCounter < 7) {
        if (tedRegisters[0x09] & tedRegisters[0x0A])
          M7501::interruptRequest();
        M7501::run(cpu_clock_multiplier);
      }
      // perform DMA fetches on odd cycle counts
      if (incrementingDMAPosition) {
        if (dmaCycleCounter >= 4) {
          if (character_column < uint8_t(40)) {
            if (dmaCycleCounter >= 7) {
              memoryReadMap = tedDMAReadMap;
              (void) readMemory(uint16_t(attr_base_addr | dma_position));
              memoryReadMap = cpuMemoryReadMap;
            }
            if (dmaFlags & 1)
              attr_buf_tmp[character_column] = dataBusState;
            if (dmaFlags & 2)
              char_buf[character_column] = dataBusState;
          }
        }
        if (video_column != 73)
          dma_position =
              (dma_position & 0x0400) | ((dma_position + 1) & 0x03FF);
      }
      // calculate video output
      {
        bool    tmpFlag = videoShiftRegisterEnabled;
        uint8_t *bufp = &(video_buf[video_buf_pos]);
        if (videoOutputFlags & uint8_t(0xB0)) {
          bufp[0] = videoOutputFlags;
          bufp[1] = 0x00;
          video_buf_pos = video_buf_pos + 2;
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
        }
        else {
          bufp[0] = videoOutputFlags | uint8_t(0x02);
          prv_render_func(*this, &(bufp[1]), 0);
          video_buf_pos = video_buf_pos + 5;
          tmpFlag = false;
        }
        if (tmpFlag)
          render_invalid_mode(*this, (uint8_t *) 0, 0);
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
        // update timer 1 on odd cycle count (886 kHz rate)
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
      // update horizontal position
      video_column =
          uint8_t(!(tedRegisterWriteMask & 0x40000000U) ?
                  (video_column != 113 ? ((video_column + 1) & 0x7F) : 0)
                  : (video_column & 0x7E));
    }

    // -------- EVEN HALF-CYCLE (FF1E bit 1 == 0) --------
    {
      TEDCallback *p = firstCallback1;
      while (p) {
        TEDCallback *nxt = p->nxt1;
        p->func(p->userData);
        p = nxt;
      }
    }
    switch (video_column) {
    case 42:                            // horizontal equalization 1 end
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags & 0x7F) | (vsyncFlags & 0x80));
      }
      break;
    case 70:                            // update DMA read position
      if (renderWindow && character_line == 6)
        dma_position_reload =
            (dma_position + (incrementingDMAPosition ? 1 : 0)) & 0x03FF;
      break;
    case 74:
      incrementingDMAPosition = false;
      // update character position reload (FF1A, FF1B)
      if (!bitmapAddressDisableFlags && prvCharacterLine == uint8_t(6)) {
        if (!(tedRegisterWriteMask & 0x0C000000U))
          character_position_reload = (character_position + 1) & 0x03FF;
      }
      break;
    case 88:                            // increment flash counter
      if (video_line == 205) {
        tedRegisters[0x1F] =
            (tedRegisters[0x1F] & uint8_t(0x7F)) + uint8_t(0x08);
        if (tedRegisters[0x1F] & uint8_t(0x80))
          flashState = uint8_t(flashState == 0x00 ? 0xFF : 0x00);
      }
      break;
    case 94:                            // horizontal equalization 2 start
      if (vsyncFlags) {
        videoOutputFlags =
            uint8_t((videoOutputFlags & 0x7F) | ((vsyncFlags ^ 0xFF) & 0x80));
      }
      break;
    case 98:
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
      if (!(tedRegisterWriteMask & uint32_t(0x30000000)))
        video_line = savedVideoLine;
      break;
    }
    // check for video interrupt
    if (video_line == videoInterruptLine) {
      if (!prvVideoInterruptState) {
        prvVideoInterruptState = true;
        tedRegisters[0x09] |= uint8_t(0x02);
      }
    }
    else
      prvVideoInterruptState = false;
    // run CPU
    tedRegisterWriteMask = 0U;
    if (!prvSingleClockModeFlags) {
      if (tedRegisters[0x09] & tedRegisters[0x0A])
        M7501::interruptRequest();
      M7501::run(cpu_clock_multiplier);
    }
    // calculate video output
    {
      bool    tmpFlag = videoShiftRegisterEnabled;
      uint8_t *bufp = &(video_buf[video_buf_pos]);
      if (videoOutputFlags & uint8_t(0xB0)) {
        bufp[0] = videoOutputFlags;
        bufp[1] = 0x00;
        video_buf_pos = video_buf_pos + 2;
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
      }
      else {
        bufp[0] = videoOutputFlags | uint8_t(0x02);
        prv_render_func(*this, &(bufp[1]), 4);
        video_buf_pos = video_buf_pos + 5;
        tmpFlag = false;
      }
      if (tmpFlag)
        render_invalid_mode(*this, (uint8_t *) 0, 4);
    }
    // delay video mode changes by one cycle
    prv_render_func = render_func;
    // delay horizontal scroll changes
    horiz_scroll = tedRegisters[0x07] & 0x07;
    // bitmap fetches and rendering display are done on even cycle counts
    if (videoShiftRegisterEnabled) {
      currentCharacter = nextCharacter;
      nextCharacter.bitmap_() = 0x00;
    }
    if (renderingDisplay) {
      nextCharacter.attr_() = attr_buf[character_column];
      nextCharacter.char_() = char_buf[character_column];
      // read bitmap data from memory
      if (!bitmapAddressDisableFlags) {
        uint16_t  addr_ = uint16_t(character_line);
        if (!(tedRegisters[0x06] & uint8_t(0x80))) {
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | (character_position << 3));
          else
            addr_ |= uint16_t(charset_base_addr
                              | (int(nextCharacter.char_() & characterMask)
                                 << 3));
        }
        else {
          // IC test mode (FF06 bit 7 set)
          int     tmp = (int(attr_buf_tmp[character_column]) << 3) | 0xF800;
          if (bitmapMode)
            addr_ |= uint16_t(bitmap_base_addr
                              | ((character_position << 3) & tmp));
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
          (character_position == cursor_position ? 0xFF : 0x00);
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
      attr_buf[character_column] = attr_buf_tmp[character_column];
      if (!bitmapAddressDisableFlags)
        character_position = (character_position + 1) & 0x03FF;
      else
        character_position = 0x03FF;
    }
    character_column = (character_column + 1) & 0x3F;
    // update timer 2 and 3 on even cycle count (886 kHz rate)
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
    // update horizontal position
    if (!ted_disabled)
      video_column |= uint8_t(0x01);
    cycle_count = (cycle_count + 1) & 3;
    // delay single/double clock mode switching by one cycle
    prvSingleClockModeFlags = singleClockModeFlags;
  }

}       // namespace Plus4

