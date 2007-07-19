
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
#include "serial.hpp"
#include "cpu.hpp"
#include "via6522.hpp"
#include "riot6532.hpp"
#include "vc1526.hpp"

namespace Plus4 {

  VC1526::M6504_::M6504_(VC1526& vc1526_)
    : M7501(),
      vc1526(vc1526_)
  {
  }

  VC1526::M6504_::~M6504_()
  {
  }

  void VC1526::M6504_::breakPointCallback(int type,
                                          uint16_t addr, uint8_t value)
  {
    (void) type;
    (void) addr;
    (void) value;
  }

  VC1526::VIA6522_::VIA6522_(VC1526& vc1526_)
    : VIA6522(),
      vc1526(vc1526_)
  {
  }

  VC1526::VIA6522_::~VIA6522_()
  {
  }

  void VC1526::VIA6522_::irqStateChangeCallback(bool newState)
  {
    vc1526.viaInterruptFlag = newState;
  }

  // --------------------------------------------------------------------------

  uint8_t VC1526::readRIOT1RAM(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    return vc1526.riot1.readMemory(addr);
  }

  void VC1526::writeRIOT1RAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    vc1526.riot1.writeMemory(addr, value);
  }

  uint8_t VC1526::readRIOT2RAM(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    return vc1526.riot2.readMemory(addr);
  }

  void VC1526::writeRIOT2RAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    vc1526.riot2.writeMemory(addr, value);
  }

  uint8_t VC1526::readMemoryROM(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    if (vc1526.memory_rom != (uint8_t *) 0)
      return vc1526.memory_rom[addr & 0x1FFF];
    return 0xFF;
  }

  void VC1526::writeMemoryDummy(void *userData, uint16_t addr, uint8_t value)
  {
    (void) userData;
    (void) addr;
    (void) value;
  }

  uint8_t VC1526::readVIARegister(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    return vc1526.via.readRegister(addr);
  }

  void VC1526::writeVIARegister(void *userData, uint16_t addr, uint8_t value)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    vc1526.via.writeRegister(addr, value);
  }

  uint8_t VC1526::readRIOT1Register(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    return vc1526.riot1.readRegister(addr);
  }

  void VC1526::writeRIOT1Register(void *userData, uint16_t addr, uint8_t value)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    vc1526.riot1.writeRegister(addr, value);
  }

  uint8_t VC1526::readRIOT2Register(void *userData, uint16_t addr)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    return vc1526.riot2.readRegister(addr);
  }

  void VC1526::writeRIOT2Register(void *userData, uint16_t addr, uint8_t value)
  {
    VC1526& vc1526 = *(reinterpret_cast<VC1526 *>(userData));
    vc1526.riot2.writeRegister(addr, value);
  }

  // --------------------------------------------------------------------------

  VC1526::VC1526(int devNum_)
    : cpu(*this),
      via(*this),
      riot1(),
      riot2(),
      memory_rom((uint8_t *) 0),
      deviceNumber(devNum_ & 7),
      updateMotorCnt(0),
      headPosX(marginLeft),
      headPosY(marginTop),
      motorXPhase(0),
      prvMotorXPhase(0),
      motorYPhase(0),
      prvMotorYPhase(0),
      pinState(0x00),
      prvPinState(0x00),
      viaInterruptFlag(false),
      pageBuf((uint8_t *) 0)
  {
    pageBuf = new uint8_t[pageWidth * pageHeight];
    for (size_t i = 0; i < size_t(pageWidth * pageHeight); i++)
      pageBuf[i] = 0xFF;        // clear to white
    cpu.setMemoryCallbackUserData((void *) this);
    for (uint16_t i = 0x0000; i <= 0x1FFF; i++) {
      uint8_t (*readCallback_)(void *, uint16_t) = &readMemoryROM;
      void    (*writeCallback_)(void *, uint16_t, uint8_t) = &writeMemoryDummy;
      if (i < 0x0200) {
        if (!(i & 0x0080)) {
          readCallback_ = &readRIOT1RAM;
          writeCallback_ = &writeRIOT1RAM;
        }
        else {
          readCallback_ = &readRIOT2RAM;
          writeCallback_ = &writeRIOT2RAM;
        }
      }
      else if (i < 0x0400) {
        if (i & 0x0040) {
          readCallback_ = &readVIARegister;
          writeCallback_ = &writeVIARegister;
        }
        else if (!(i & 0x0080)) {
          readCallback_ = &readRIOT1Register;
          writeCallback_ = &writeRIOT1Register;
        }
        else {
          readCallback_ = &readRIOT2Register;
          writeCallback_ = &writeRIOT2Register;
        }
      }
      for (uint32_t j = 0x0000U; j <= 0xE000U; j += 0x2000U) {
        cpu.setMemoryReadCallback(i | uint16_t(j), readCallback_);
        cpu.setMemoryWriteCallback(i | uint16_t(j), writeCallback_);
      }
    }
    this->reset();
  }

  VC1526::~VC1526()
  {
    delete[] pageBuf;
  }

  void VC1526::setROMImage(const uint8_t *romData_)
  {
    memory_rom = romData_;
  }

  void VC1526::updateMotors()
  {
  }

  void VC1526::runOneCycle(SerialBus& serialBus_)
  {
    {
      bool    dataOut = !(riot1.getPortA() & 0x40);
      bool    atnAck = !(riot1.getPortA() & 0x20);
      atnAck = atnAck && !(serialBus_.getATN());
      serialBus_.setDATA(deviceNumber, dataOut && !atnAck);
      uint8_t riot1PortAInput = riot1.getPortAInput() & 0x7C;
      riot1PortAInput = riot1PortAInput
                        | uint8_t(((serialBus_.getATN() & 0x01)
                                   | (serialBus_.getCLK() & 0x02)
                                   | (serialBus_.getDATA() & 0x80)) ^ 0x83);
      riot1.setPortA(riot1PortAInput);
    }
    if (viaInterruptFlag
        | riot1.isInterruptRequest() | riot2.isInterruptRequest()) {
      cpu.interruptRequest();
    }
    cpu.run(1);
    via.runOneCycle();
    riot1.runOneCycle();
    riot2.runOneCycle();
    if (!updateMotorCnt) {
      updateMotorCnt = updateMotorReload;
      updateMotors();
    }
    updateMotorCnt--;
  }

  const uint8_t * VC1526::getPageData() const
  {
    return pageBuf;
  }

  int VC1526::getPageWidth() const
  {
    return pageWidth;
  }

  int VC1526::getPageHeight() const
  {
    return pageHeight;
  }

  void VC1526::clearPage()
  {
    for (size_t i = 0; i < size_t(pageWidth * pageHeight); i++)
      pageBuf[i] = 0xFF;        // clear to white
    headPosY = marginTop;
  }

  uint8_t VC1526::getLEDState() const
  {
    return 0x00;
  }

  void VC1526::reset()
  {
    via.reset();
    riot1.reset();
    riot2.reset();
    viaInterruptFlag = false;
    cpu.reset();
    riot1.setPortB(uint8_t(deviceNumber | 0xF8));
  }

}       // namespace Plus4

