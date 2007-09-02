
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
#include "cia8520.hpp"
#include "wd177x.hpp"
#include "serial.hpp"
#include "p4floppy.hpp"
#include "vc1581.hpp"

static void defaultBreakPointCallback(void *userData,
                                      int debugContext_, int type,
                                      uint16_t addr, uint8_t value)
{
  (void) userData;
  (void) debugContext_;
  (void) type;
  (void) addr;
  (void) value;
}

namespace Plus4 {

  VC1581::M7501_::M7501_(VC1581& vc1581_)
    : M7501(),
      vc1581(vc1581_)
  {
    // initialize memory map
    setMemoryCallbackUserData((void *) &vc1581_);
    for (uint16_t i = 0x0000; i <= 0x1FFF; i++) {
      setMemoryReadCallback(i, &VC1581::readRAM);
      setMemoryWriteCallback(i, &VC1581::writeRAM);
    }
    for (uint16_t i = 0x2000; i <= 0x7FFF; i++) {
      setMemoryReadCallback(i, &VC1581::readDummy);
      setMemoryWriteCallback(i, &VC1581::writeDummy);
    }
    for (uint16_t i = 0x4000; i <= 0x43FF; i++) {
      setMemoryReadCallback(i, &VC1581::readCIA8520);
      setMemoryWriteCallback(i, &VC1581::writeCIA8520);
    }
    for (uint16_t i = 0x6000; i <= 0x63FF; i++) {
      setMemoryReadCallback(i, &VC1581::readWD177x);
      setMemoryWriteCallback(i, &VC1581::writeWD177x);
    }
    for (uint16_t i = 0x8000; i <= 0xBFFF; i++)
      setMemoryReadCallback(i, &VC1581::readROM0);
    for (uint32_t i = 0xC000; i <= 0xFFFF; i++)
      setMemoryReadCallback(uint16_t(i), &VC1581::readROM1);
    for (uint32_t i = 0x8000; i <= 0xFFFF; i++)
      setMemoryWriteCallback(uint16_t(i), &VC1581::writeDummy);
  }

  VC1581::M7501_::~M7501_()
  {
  }

  void VC1581::M7501_::breakPointCallback(int type,
                                          uint16_t addr, uint8_t value)
  {
    if (vc1581.noBreakOnDataRead && type == 1)
      return;
    vc1581.breakPointCallback(vc1581.breakPointCallbackUserData,
                              (vc1581.deviceNumber & 3) + 1, type, addr, value);
  }

  VC1581::CIA8520_::~CIA8520_()
  {
  }

  void VC1581::CIA8520_::interruptCallback(bool irqState)
  {
    vc1581.interruptRequestFlag = irqState;
  }

  uint8_t VC1581::readRAM(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = vc1581.memory_ram[addr & 0x1FFF];
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readDummy(void *userData, uint16_t addr)
  {
    (void) addr;
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readCIA8520(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = vc1581.cia.readRegister(addr & 0x000F);
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readWD177x(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    switch (addr & 3) {
    case 0:
      vc1581.dataBusState = vc1581.wd177x.readStatusRegister();
      break;
    case 1:
      vc1581.dataBusState = vc1581.wd177x.readTrackRegister();
      break;
    case 2:
      vc1581.dataBusState = vc1581.wd177x.readSectorRegister();
      break;
    case 3:
      vc1581.dataBusState = vc1581.wd177x.readDataRegister();
      break;
    }
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readROM0(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    if (vc1581.memory_rom_0)
      vc1581.dataBusState = vc1581.memory_rom_0[addr & 0x3FFF];
    return vc1581.dataBusState;
  }

  uint8_t VC1581::readROM1(void *userData, uint16_t addr)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    if (vc1581.memory_rom_1)
      vc1581.dataBusState = vc1581.memory_rom_1[addr & 0x3FFF];
    return vc1581.dataBusState;
  }

  void VC1581::writeRAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    vc1581.memory_ram[addr & 0x1FFF] = vc1581.dataBusState;
  }

  void VC1581::writeDummy(void *userData, uint16_t addr, uint8_t value)
  {
    (void) addr;
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
  }

  void VC1581::writeCIA8520(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    vc1581.cia.writeRegister(addr & 0x000F, vc1581.dataBusState);
  }

  void VC1581::writeWD177x(void *userData, uint16_t addr, uint8_t value)
  {
    VC1581& vc1581 = *(reinterpret_cast<VC1581 *>(userData));
    vc1581.dataBusState = value & uint8_t(0xFF);
    switch (addr & 3) {
    case 0:
      vc1581.wd177x.writeCommandRegister(vc1581.dataBusState);
      break;
    case 1:
      vc1581.wd177x.writeTrackRegister(vc1581.dataBusState);
      break;
    case 2:
      vc1581.wd177x.writeSectorRegister(vc1581.dataBusState);
      break;
    case 3:
      vc1581.wd177x.writeDataRegister(vc1581.dataBusState);
      break;
    }
  }

  VC1581::VC1581(int driveNum_)
    : FloppyDrive(driveNum_),
      cpu(*this),
      cia(*this),
      wd177x(),
      memory_rom_0((uint8_t *) 0),
      memory_rom_1((uint8_t *) 0),
      deviceNumber(driveNum_),
      dataBusState(0),
      interruptRequestFlag(false),
      ciaPortAInput(0),
      ciaPortBInput(0),
      diskChangeCnt(0),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0),
      noBreakOnDataRead(false)
  {
    // clear RAM
    for (uint16_t i = 0; i < 8192; i++)
      memory_ram[i] = 0x00;
    // select drive number
    ciaPortAInput = uint8_t((driveNum_ & 3) << 3) | uint8_t(0x67);
    // configure WD177x emulation
    wd177x.setIsWD1773(false);
    wd177x.setEnableBusyFlagHack(true);
    this->reset();
  }

  VC1581::~VC1581()
  {
  }

  void VC1581::setROMImage(int n, const uint8_t *romData_)
  {
    switch (n) {
    case 0:
      memory_rom_0 = romData_;
      break;
    case 1:
      memory_rom_1 = romData_;
      break;
    }
  }

  void VC1581::setDiskImageFile(const std::string& fileName_)
  {
    try {
      wd177x.setDiskImageFile(fileName_, 80, 2, 10);
    }
    catch (...) {
      // not ready, disk changed
      diskChangeCnt = 350000;
      ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
      // update write protect flag
      ciaPortBInput |= uint8_t(0x40);
      if (wd177x.getIsWriteProtected())
        ciaPortBInput &= uint8_t(0xBF);
      throw;
    }
    // not ready, disk changed
    diskChangeCnt = 350000;
    ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
    // update write protect flag
    ciaPortBInput |= uint8_t(0x40);
    if (wd177x.getIsWriteProtected())
      ciaPortBInput &= uint8_t(0xBF);
  }

  bool VC1581::haveDisk() const
  {
    return wd177x.haveDisk();
  }

  void VC1581::runOneCycle(SerialBus& serialBus_)
  {
    {
      uint8_t n = ciaPortBInput & uint8_t(0x7A);
      n |= (serialBus_.getDATA() & uint8_t(0x01));
      n |= (serialBus_.getCLK() & uint8_t(0x04));
      n |= (serialBus_.getATN() & uint8_t(0x80));
      cia.setPortB(n ^ uint8_t(0x85));
    }
    cia.setFlagState(!!(serialBus_.getATN()));
    if (interruptRequestFlag)
      cpu.interruptRequest();
    cpu.run(2);
    if (diskChangeCnt) {
      diskChangeCnt--;
      if (!diskChangeCnt)
        ciaPortAInput = (ciaPortAInput | uint8_t(0x80)) & uint8_t(0xFD);
    }
    cia.setPortA(ciaPortAInput);
    cia.run(2);
    wd177x.setSide(cia.getPortA() & 0x01);
    uint8_t n = cia.getPortB();
    serialBus_.setCLK(deviceNumber, !(n & uint8_t(0x08)));
    if (!(((serialBus_.getATN() ^ uint8_t(0xFF)) & n) & uint8_t(0x10)))
      serialBus_.setDATA(deviceNumber, !(n & uint8_t(0x02)));
    else
      serialBus_.setDATA(deviceNumber, false);
  }

  void VC1581::reset()
  {
    interruptRequestFlag = false;
    cpu.reset(true);
    cia.reset();
    wd177x.reset();
    diskChangeCnt = 350000;
    ciaPortAInput = (ciaPortAInput & uint8_t(0x7F)) | uint8_t(0x02);
  }

  M7501 * VC1581::getCPU()
  {
    return (&cpu);
  }

  const M7501 * VC1581::getCPU() const
  {
    return (&cpu);
  }

  void VC1581::setBreakPointCallback(void (*breakPointCallback_)(
                                         void *userData,
                                         int debugContext_, int type,
                                         uint16_t addr, uint8_t value),
                                     void *userData_)
  {
    if (breakPointCallback_)
      breakPointCallback = breakPointCallback_;
    else
      breakPointCallback = &defaultBreakPointCallback;
    breakPointCallbackUserData = userData_;
  }

  void VC1581::setNoBreakOnDataRead(bool n)
  {
    noBreakOnDataRead = n;
  }

  uint8_t VC1581::readMemoryDebug(uint16_t addr) const
  {
    if (addr < 0x6000) {
      if (addr < 0x2000)
        return memory_ram[addr];
      else if (addr >= 0x4000 && addr <= 0x43FF)
        return cia.readRegisterDebug(addr & 0x000F);
    }
    else if (addr >= 0x8000) {
      if (addr < 0xC000) {
        if (memory_rom_0)
          return memory_rom_0[addr & 0x3FFF];
      }
      else {
        if (memory_rom_1)
          return memory_rom_1[addr & 0x3FFF];
      }
    }
    else if (addr < 0x6400) {
      switch (addr & 3) {
      case 0:
        return wd177x.readStatusRegisterDebug();
      case 1:
        return wd177x.readTrackRegister();
      case 2:
        return wd177x.readSectorRegister();
      case 3:
        return wd177x.readDataRegisterDebug();
      }
    }
    return uint8_t(0xFF);
  }

  void VC1581::writeMemoryDebug(uint16_t addr, uint8_t value)
  {
    if (addr < 0x4400) {
      if (addr < 0x2000)
        memory_ram[addr] = value;
      else if (addr >= 0x4000)
        cia.writeRegister(addr & 0x000F, value);
    }
    else if (addr >= 0x6000 && addr <= 0x63FF) {
      switch (addr & 3) {
      case 0:
        wd177x.writeCommandRegister(value);
        break;
      case 1:
        wd177x.writeTrackRegister(value);
        break;
      case 2:
        wd177x.writeSectorRegister(value);
        break;
      case 3:
        wd177x.writeDataRegister(value);
        break;
      }
    }
  }

  uint8_t VC1581::getLEDState() const
  {
    uint8_t n = cia.getPortA() & uint8_t(0x40);
    return uint8_t((n >> 5) | (n >> 6));
  }

  uint16_t VC1581::getHeadPosition() const
  {
    return uint16_t(wd177x.getHeadPosition() | 0x8000);
  }

  void VC1581::saveState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1581::saveState(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void VC1581::loadState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1581::registerChunkTypes(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

}       // namespace Plus4

