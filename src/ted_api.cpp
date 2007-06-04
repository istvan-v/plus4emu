
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
#include "cpu.hpp"
#include "ted.hpp"

#include <cmath>

static const float brightnessToYTable[8] = {
   0.180f,  0.235f,  0.261f,  0.341f,  0.506f,  0.661f,  0.753f,  0.993f
};

static const float colorPhaseTable[32] = {
     0.0f,    0.0f,  103.0f,  283.0f,   53.0f,  241.0f,  347.0f,  167.0f,
   124.5f,  148.0f,  195.0f,   83.0f,  265.0f,  323.0f,    1.5f,  213.0f,
  // the second half of the palette is used for emulating PAL color artifacts
    27.5f,   68.0f,  136.0f,  227.0f,  303.0f,   33.5f,   48.0f,   60.0f,
    70.5f,   80.5f,   99.5f,  109.5f,  120.0f,  132.0f,  146.5f,  180.0f
};

namespace Plus4 {

  void TED7360::convertPixelToRGB(uint8_t color,
                                  float& red, float& green, float& blue)
  {
    uint8_t c = (color & 0x0F) | ((color & 0x80) >> 3);
    uint8_t b = (color & 0x70) >> 4;
    float   y = 0.035f;
    float   u = 0.0f, v = 0.0f;
    if (c)
      y = brightnessToYTable[b];
    if (c > 1) {
      float   phs = colorPhaseTable[c] * 3.14159265f / 180.0f;
      u = float(std::cos(phs)) * 0.18f;
      v = float(std::sin(phs)) * (c < 0x15 ? 0.18f : 0.0f);
    }
    y *= 0.95f;
    // R = (V / 0.877) + Y
    // B = (U / 0.492) + Y
    // G = (Y - ((R * 0.299) + (B * 0.114))) / 0.587
    red = (v / 0.877f) + y;
    blue = (u / 0.492f) + y;
    green = (y - ((red * 0.299f) + (blue * 0.114f))) / 0.587f;
  }

  void TED7360::setCPUClockMultiplier(int clk)
  {
    if (clk < 1)
      cpu_clock_multiplier = 1;
    else if (clk > 100)
      cpu_clock_multiplier = 100;
    else
      cpu_clock_multiplier = clk;
  }

  // set the state of the specified key
  // valid key numbers are:
  //
  //     0: Del          1: Return       2: £            3: Help
  //     4: F1           5: F2           6: F3           7: @
  //     8: 3            9: W           10: A           11: 4
  //    12: Z           13: S           14: E           15: Shift
  //    16: 5           17: R           18: D           19: 6
  //    20: C           21: F           22: T           23: X
  //    24: 7           25: Y           26: G           27: 8
  //    28: B           29: H           30: U           31: V
  //    32: 9           33: I           34: J           35: 0
  //    36: M           37: K           38: O           39: N
  //    40: Down        41: P           42: L           43: Up
  //    44: .           45: :           46: -           47: ,
  //    48: Left        49: *           50: ;           51: Right
  //    52: Esc         53: =           54: +           55: /
  //    56: 1           57: Home        58: Ctrl        59: 2
  //    60: Space       61: C=          62: Q           63: Stop
  //
  //    72: Joy2 Up     73: Joy2 Down   74: Joy2 Left   75: Joy2 Right
  //    79: Joy2 Fire
  //    80: Joy1 Up     81: Joy1 Down   82: Joy1 Left   83: Joy1 Right
  //    86: Joy1 Fire

  void TED7360::setKeyState(int keyNum, bool isPressed)
  {
    int     ndx = (keyNum & 0x78) >> 3;
    int     mask = 1 << (keyNum & 7);

    if (isPressed)
      keyboard_matrix[ndx] &= ((uint8_t) mask ^ (uint8_t) 0xFF);
    else
      keyboard_matrix[ndx] |= (uint8_t) mask;
  }

  void TED7360::setCallback(void (*func)(void *userData), void *userData_,
                            int flags_)
  {
    if (!func)
      return;
    flags_ = flags_ & 3;
    int     ndx = -1;
    for (size_t i = 0; i < (sizeof(callbacks) / sizeof(TEDCallback)); i++) {
      if (callbacks[i].func == func && callbacks[i].userData == userData_) {
        ndx = int(i);
        break;
      }
    }
    if (ndx >= 0) {
      TEDCallback *prv = (TEDCallback *) 0;
      TEDCallback *p = firstCallback0;
      while (p) {
        if (p == &(callbacks[ndx])) {
          if (prv)
            prv->nxt0 = p->nxt0;
          else
            firstCallback0 = p->nxt0;
          break;
        }
        prv = p;
        p = p->nxt0;
      }
      prv = (TEDCallback *) 0;
      p = firstCallback1;
      while (p) {
        if (p == &(callbacks[ndx])) {
          if (prv)
            prv->nxt1 = p->nxt1;
          else
            firstCallback1 = p->nxt1;
          break;
        }
        prv = p;
        p = p->nxt1;
      }
      if (flags_ == 0) {
        callbacks[ndx].func = (void (*)(void *)) 0;
        callbacks[ndx].userData = (void *) 0;
        callbacks[ndx].nxt0 = (TEDCallback *) 0;
        callbacks[ndx].nxt1 = (TEDCallback *) 0;
      }
    }
    if (flags_ == 0)
      return;
    if (ndx < 0) {
      for (size_t i = 0; i < (sizeof(callbacks) / sizeof(TEDCallback)); i++) {
        if (callbacks[i].func == (void (*)(void *)) 0) {
          ndx = int(i);
          break;
        }
      }
      if (ndx < 0)
        throw Plus4Emu::Exception("TED7360: too many callbacks");
    }
    callbacks[ndx].func = func;
    callbacks[ndx].userData = userData_;
    callbacks[ndx].nxt0 = (TEDCallback *) 0;
    callbacks[ndx].nxt1 = (TEDCallback *) 0;
    if (flags_ & 1) {
      TEDCallback *prv = (TEDCallback *) 0;
      TEDCallback *p = firstCallback0;
      while (p) {
        prv = p;
        p = p->nxt0;
      }
      p = &(callbacks[ndx]);
      if (prv)
        prv->nxt0 = p;
      else
        firstCallback0 = p;
    }
    if (flags_ & 2) {
      TEDCallback *prv = (TEDCallback *) 0;
      TEDCallback *p = firstCallback1;
      while (p) {
        prv = p;
        p = p->nxt1;
      }
      p = &(callbacks[ndx]);
      if (prv)
        prv->nxt1 = p;
      else
        firstCallback1 = p;
    }
  }

  // --------------------------------------------------------------------------

  class ChunkType_TED7360Snapshot : public Plus4Emu::File::ChunkTypeHandler {
   private:
    TED7360&  ref;
   public:
    ChunkType_TED7360Snapshot(TED7360& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_TED7360Snapshot()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_TED_STATE;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_Plus4Program : public Plus4Emu::File::ChunkTypeHandler {
   private:
    TED7360&  ref;
   public:
    ChunkType_Plus4Program(TED7360& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4Program()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_PLUS4_PRG;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadProgram(buf);
    }
  };

  void TED7360::saveState(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000001);        // version number
    uint8_t   romBitmap = 0;
    for (uint8_t i = 0; i < 8; i++) {
      // find non-empty ROM segments
      romBitmap = romBitmap >> 1;
      if (segmentTable[i] != (uint8_t *) 0)
        romBitmap = romBitmap | uint8_t(0x80);
    }
    buf.writeByte(romBitmap);
    buf.writeByte(ramSegments);
    // save RAM segments
    for (size_t i = 0x08; i <= 0xFF; i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
    // save ROM segments
    for (size_t i = 0x00; i < 0x08; i++) {
      if (segmentTable[i] != (uint8_t *) 0) {
        for (size_t j = 0; j < 16384; j++)
          buf.writeByte(segmentTable[i][j]);
      }
    }
    // save I/O and TED registers
    buf.writeByte(ioRegister_0000);
    buf.writeByte(ioRegister_0001);
    for (uint8_t i = 0x00; i <= 0x1F; i++)
      buf.writeByte(readMemoryCPU(uint16_t(0xFF00) | uint16_t(i)));
    // save memory paging
    buf.writeByte(hannesRegister);
    buf.writeByte((uint8_t(cpuMemoryReadMap) & uint8_t(0x80))
                  | (uint8_t(cpuMemoryReadMap >> 11) & uint8_t(0x0F)));
    // save internal registers
    buf.writeUInt32(tedRegisterWriteMask);
    buf.writeByte(cycle_count);
    buf.writeByte(video_column);
    buf.writeUInt32(uint32_t(video_line));
    buf.writeByte(uint8_t(character_line));
    buf.writeUInt32(uint32_t(character_position));
    buf.writeUInt32(uint32_t(character_position_reload));
    buf.writeByte(uint8_t(character_column));
    buf.writeUInt32(uint32_t(dma_position));
    buf.writeUInt32(uint32_t(dma_position_reload));
    buf.writeByte(flashState);
    buf.writeBoolean(renderWindow);
    buf.writeBoolean(dmaWindow);
    buf.writeByte(bitmapAddressDisableFlags);
    buf.writeBoolean(displayWindow);
    buf.writeBoolean(renderingDisplay);
    buf.writeBoolean(displayActive);
    buf.writeByte(displayBlankingFlags);
    buf.writeBoolean(timer1_run);
    buf.writeBoolean(timer2_run);
    buf.writeBoolean(timer3_run);
    buf.writeUInt32(uint32_t(timer1_state));
    buf.writeUInt32(uint32_t(timer1_reload_value));
    buf.writeUInt32(uint32_t(timer2_state));
    buf.writeUInt32(uint32_t(timer3_state));
    buf.writeUInt32(uint32_t(sound_channel_1_cnt));
    buf.writeUInt32(uint32_t(sound_channel_2_cnt));
    buf.writeByte(sound_channel_1_state);
    buf.writeByte(sound_channel_2_state);
    buf.writeByte(sound_channel_2_noise_state);
    buf.writeByte(sound_channel_2_noise_output);
    buf.writeBoolean(videoShiftRegisterEnabled);
    buf.writeByte(shiftRegisterCharacter.bitmap_());
    buf.writeByte(horiz_scroll);
    buf.writeByte(shiftRegisterCharacter.attr_());
    buf.writeByte(shiftRegisterCharacter.char_());
    buf.writeBoolean(shiftRegisterCharacter.cursor_() != 0x00);
    buf.writeByte(currentCharacter.attr_());
    buf.writeByte(currentCharacter.char_());
    buf.writeByte(currentCharacter.bitmap_());
    buf.writeBoolean(currentCharacter.cursor_() != 0x00);
    buf.writeByte(nextCharacter.attr_());
    buf.writeByte(nextCharacter.char_());
    buf.writeByte(nextCharacter.bitmap_());
    buf.writeBoolean(nextCharacter.cursor_() != 0x00);
    buf.writeBoolean(dmaEnabled);
    buf.writeByte(prvSingleClockModeFlags);
    buf.writeBoolean(!!(singleClockModeFlags & uint8_t(0x01)));
    buf.writeByte(dmaCycleCounter);
    buf.writeByte(dmaFlags);
    buf.writeBoolean(incrementingDMAPosition);
    buf.writeUInt32(uint32_t(savedVideoLine));
    buf.writeBoolean(prvVideoInterruptState);
    buf.writeByte(prvCharacterLine);
    buf.writeByte(invertColorPhaseFlags);
    buf.writeByte(dataBusState);
    buf.writeUInt32(uint32_t(keyboard_row_select_mask));
    for (int i = 0; i < 16; i++)
      buf.writeByte(keyboard_matrix[i]);
    buf.writeByte(user_port_state);
  }

  void TED7360::saveState(Plus4Emu::File& f)
  {
    {
      Plus4Emu::File::Buffer  buf;
      this->saveState(buf);
      f.addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_TED_STATE, buf);
    }
    M7501::saveState(f);
  }

  void TED7360::loadState(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000 && version != 0x01000001) {
      buf.setPosition(buf.getDataSize());
      throw Plus4Emu::Exception("incompatible Plus/4 snapshot format");
    }
    try {
      this->reset(true);
      // load saved state
      uint8_t   romBitmap = buf.readByte();
      ramSegments = buf.readByte();
      if (!(ramSegments == 1 || ramSegments == 2 || ramSegments == 4 ||
            ramSegments == 16 || ramSegments == 64))
        throw Plus4Emu::Exception("incompatible Plus/4 snapshot data");
      // load RAM segments
      setRAMSize(size_t(ramSegments) << 4);
      for (size_t i = 0x08; i <= 0xFF; i++) {
        if (segmentTable[i] != (uint8_t *) 0) {
          for (size_t j = 0; j < 16384; j++)
            segmentTable[i][j] = buf.readByte();
        }
      }
      // load ROM segments
      for (uint8_t i = 0x00; i < 0x08; i++) {
        if (!(romBitmap & (uint8_t(1) << i)))
          loadROM(int(i >> 1), int(i & 1) << 14, 0, (uint8_t *) 0);
        else {
          uint8_t tmp = 0;
          loadROM(int(i >> 1), int(i & 1) << 14, 1, &tmp);
          for (size_t j = 0; j < 16384; j++)
            segmentTable[i][j] = buf.readByte();
        }
      }
      // load I/O and TED registers
      ioRegister_0000 = buf.readByte();
      ioRegister_0001 = buf.readByte();
      writeMemory(0x0000, ioRegister_0000);
      writeMemory(0x0001, ioRegister_0001);
      for (uint8_t i = 0x00; i <= 0x1F; i++) {
        uint8_t c = buf.readByte();
        if (i == 0x06 || i == 0x07 || (i >= 0x0A && i <= 0x19))
          writeMemory(uint16_t(0xFF00) | uint16_t(i), c);
        else
          tedRegisters[i] = c;
      }
      tedRegisters[0x09] &= uint8_t(0x5E);
      // load memory paging
      hannesRegister = buf.readByte();
      uint8_t romSelect_ = buf.readByte() & uint8_t(0x8F);
      // update internal registers according to the new RAM image loaded
      write_register_FD16(this, 0xFD16, hannesRegister);
      if (romSelect_ & uint8_t(0x80))
        write_register_FF3E(this, 0xFF3E, 0x00);
      else
        write_register_FF3F(this, 0xFF3F, 0x00);
      write_register_FDDx(this, uint16_t(0xFDD0) | uint16_t(romSelect_), 0x00);
      // load remaining internal registers from snapshot data
      tedRegisterWriteMask = buf.readUInt32();
      cycle_count = buf.readByte() & 0x03;
      video_column = buf.readByte() & 0x7F;
      video_line = int(buf.readUInt32() & 0x01FF);
      character_line = buf.readByte() & 7;
      character_position = int(buf.readUInt32() & 0x03FF);
      character_position_reload = int(buf.readUInt32() & 0x03FF);
      character_column = buf.readByte() & 0x3F;
      dma_position = int(buf.readUInt32() & 0x07FF);
      dma_position_reload = int(buf.readUInt32() & 0x03FF);
      flashState = uint8_t(buf.readByte() == 0x00 ? 0x00 : 0xFF);
      renderWindow = buf.readBoolean();
      dmaWindow = buf.readBoolean();
      bitmapAddressDisableFlags = buf.readByte() & 0x03;
      displayWindow = buf.readBoolean();
      renderingDisplay = buf.readBoolean();
      displayActive = buf.readBoolean();
      displayBlankingFlags = buf.readByte() & 0x03;
      timer1_run = buf.readBoolean();
      timer2_run = buf.readBoolean();
      timer3_run = buf.readBoolean();
      timer1_state = int(buf.readUInt32() & 0xFFFF);
      timer1_reload_value = int(buf.readUInt32() & 0xFFFF);
      timer2_state = int(buf.readUInt32() & 0xFFFF);
      timer3_state = int(buf.readUInt32() & 0xFFFF);
      sound_channel_1_cnt = int(buf.readUInt32() & 0x03FF);
      sound_channel_2_cnt = int(buf.readUInt32() & 0x03FF);
      sound_channel_1_state = uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      sound_channel_2_state = uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      sound_channel_2_noise_state = buf.readByte();
      sound_channel_2_noise_output =
          uint8_t(buf.readByte() == uint8_t(0) ? 0 : 1);
      videoShiftRegisterEnabled = buf.readBoolean();
      shiftRegisterCharacter.bitmap_() = buf.readByte();
      if (version == 0x01000000)
        (void) buf.readUInt32();        // was bitmapMShiftRegister
      horiz_scroll = buf.readByte() & 7;
      shiftRegisterCharacter.attr_() = buf.readByte();
      shiftRegisterCharacter.char_() = buf.readByte();
      shiftRegisterCharacter.cursor_() = (buf.readBoolean() ? 0xFF : 0x00);
      currentCharacter.attr_() = buf.readByte();
      currentCharacter.char_() = buf.readByte();
      currentCharacter.bitmap_() = buf.readByte();
      currentCharacter.cursor_() = (buf.readBoolean() ? 0xFF : 0x00);
      nextCharacter.attr_() = buf.readByte();
      nextCharacter.char_() = buf.readByte();
      nextCharacter.bitmap_() = buf.readByte();
      nextCharacter.cursor_() = (buf.readBoolean() ? 0xFF : 0x00);
      dmaEnabled = buf.readBoolean();
      prvSingleClockModeFlags = buf.readByte() & 0x03;
      singleClockModeFlags &= uint8_t(0x02);
      singleClockModeFlags |= uint8_t(buf.readBoolean() ? 0x01 : 0x00);
      dmaCycleCounter = buf.readByte();
      dmaFlags = buf.readByte() & 3;
      incrementingDMAPosition = buf.readBoolean();
      savedVideoLine = int(buf.readUInt32() & 0x01FF);
      prvVideoInterruptState = buf.readBoolean();
      prvCharacterLine = buf.readByte() & 7;
      invertColorPhaseFlags = buf.readByte() & 0x07;
      dataBusState = buf.readByte();
      keyboard_row_select_mask = int(buf.readUInt32() & 0xFFFF);
      for (int i = 0; i < 16; i++)
        keyboard_matrix[i] = buf.readByte();
      user_port_state = buf.readByte();
      selectRenderer();
      if (buf.getPosition() != buf.getDataSize())
        throw Plus4Emu::Exception("trailing garbage at end of "
                                  "Plus/4 snapshot data");
    }
    catch (...) {
      for (int i = 0; i < 16; i++)
        keyboard_matrix[i] = 0xFF;
      try {
        this->reset(true);
      }
      catch (...) {
      }
      throw;
    }
  }

  void TED7360::saveProgram(Plus4Emu::File::Buffer& buf)
  {
    uint16_t  startAddr, endAddr, len;
    startAddr = uint16_t(readMemoryCPU(0x002B))
                | (uint16_t(readMemoryCPU(0x002C)) << 8);
    endAddr = uint16_t(readMemoryCPU(0x002D))
              | (uint16_t(readMemoryCPU(0x002E)) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    buf.writeUInt32(startAddr);
    buf.writeUInt32(len);
    while (len) {
      buf.writeByte(readMemoryCPU(startAddr, true));
      startAddr = (startAddr + 1) & 0xFFFF;
      len--;
    }
  }

  void TED7360::saveProgram(Plus4Emu::File& f)
  {
    Plus4Emu::File::Buffer  buf;
    this->saveProgram(buf);
    f.addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_PLUS4_PRG, buf);
  }

  void TED7360::saveProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "wb");
    if (!f)
      throw Plus4Emu::Exception("error opening plus4 program file");
    uint16_t  startAddr, endAddr, len;
    startAddr = uint16_t(readMemoryCPU(0x002B))
                | (uint16_t(readMemoryCPU(0x002C)) << 8);
    endAddr = uint16_t(readMemoryCPU(0x002D))
              | (uint16_t(readMemoryCPU(0x002E)) << 8);
    len = (endAddr > startAddr ? (endAddr - startAddr) : uint16_t(0));
    bool  err = true;
    if (std::fputc(int(startAddr & 0xFF), f) != EOF) {
      if (std::fputc(int((startAddr >> 8) & 0xFF), f) != EOF) {
        while (len) {
          int   c = readMemoryCPU(startAddr, true);
          if (std::fputc(c, f) == EOF)
            break;
          startAddr = (startAddr + 1) & 0xFFFF;
          len--;
        }
        err = (len != 0);
      }
    }
    if (std::fclose(f) != 0)
      err = true;
    if (err)
      throw Plus4Emu::Exception("error writing plus4 program file "
                                "-- is the disk full ?");
  }

  void TED7360::loadProgram(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    uint32_t  addr = buf.readUInt32();
    uint32_t  len = buf.readUInt32();
    if (addr >= 0x00010000U)
      throw Plus4Emu::Exception("invalid start address in plus4 program data");
    if (len >= 0x00010000U ||
        size_t(len) != (buf.getDataSize() - buf.getPosition()))
      throw Plus4Emu::Exception("invalid plus4 program length");
#if 0
    writeMemory(0x002B, uint8_t(addr & 0xFF));
    writeMemory(0x002C, uint8_t((addr >> 8) & 0xFF));
#endif
    while (len) {
      writeMemory(uint16_t(addr), buf.readByte());
      addr = (addr + 1) & 0xFFFF;
      len--;
    }
    writeMemory(0x002D, uint8_t(addr & 0xFF));
    writeMemory(0x002E, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x002F, uint8_t(addr & 0xFF));
    writeMemory(0x0030, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0031, uint8_t(addr & 0xFF));
    writeMemory(0x0032, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0033, readMemoryCPU(0x0037));
    writeMemory(0x0034, readMemoryCPU(0x0038));
#if 0
    writeMemory(0x0035, readMemoryCPU(0x0037));
    writeMemory(0x0036, readMemoryCPU(0x0038));
#endif
    writeMemory(0x009D, uint8_t(addr & 0xFF));
    writeMemory(0x009E, uint8_t((addr >> 8) & 0xFF));
  }

  void TED7360::loadProgram(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid plus4 program file name");
    std::FILE *f = std::fopen(fileName, "rb");
    if (!f)
      throw Plus4Emu::Exception("error opening plus4 program file");
    uint16_t  addr;
    int       c;
    c = std::fgetc(f);
    if (c == EOF)
      throw Plus4Emu::Exception("unexpected end of plus4 program file");
    addr = uint16_t(c & 0xFF);
    c = std::fgetc(f);
    if (c == EOF)
      throw Plus4Emu::Exception("unexpected end of plus4 program file");
    addr |= uint16_t((c & 0xFF) << 8);
#if 0
    writeMemory(0x002B, uint8_t(addr & 0xFF));
    writeMemory(0x002C, uint8_t((addr >> 8) & 0xFF));
#endif
    size_t  len = 0;
    while (true) {
      c = std::fgetc(f);
      if (c == EOF)
        break;
      if (++len > 0xFFFF) {
        std::fclose(f);
        throw Plus4Emu::Exception("plus4 program file has invalid length");
      }
      writeMemory(addr, uint8_t(c));
      addr = (addr + 1) & 0xFFFF;
    }
    std::fclose(f);
    writeMemory(0x002D, uint8_t(addr & 0xFF));
    writeMemory(0x002E, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x002F, uint8_t(addr & 0xFF));
    writeMemory(0x0030, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0031, uint8_t(addr & 0xFF));
    writeMemory(0x0032, uint8_t((addr >> 8) & 0xFF));
    writeMemory(0x0033, readMemoryCPU(0x0037));
    writeMemory(0x0034, readMemoryCPU(0x0038));
#if 0
    writeMemory(0x0035, readMemoryCPU(0x0037));
    writeMemory(0x0036, readMemoryCPU(0x0038));
#endif
    writeMemory(0x009D, uint8_t(addr & 0xFF));
    writeMemory(0x009E, uint8_t((addr >> 8) & 0xFF));
  }

  void TED7360::registerChunkTypes(Plus4Emu::File& f)
  {
    ChunkType_TED7360Snapshot *p1 = (ChunkType_TED7360Snapshot *) 0;
    ChunkType_Plus4Program    *p2 = (ChunkType_Plus4Program *) 0;

    try {
      p1 = new ChunkType_TED7360Snapshot(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_TED7360Snapshot *) 0;
      p2 = new ChunkType_Plus4Program(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Plus4Program *) 0;
    }
    catch (...) {
      if (p2)
        delete p2;
      if (p1)
        delete p1;
      throw;
    }
    M7501::registerChunkType(f);
  }

}       // namespace Plus4

