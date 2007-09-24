
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
    if (vc1526.noBreakOnDataRead && type == 1)
      return;
    vc1526.breakPointCallback(vc1526.breakPointCallbackUserData,
                              5, type, addr, value);
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

  VC1526::VC1526(SerialBus& serialBus_, int devNum_)
    : cpu(*this),
      via(*this),
      riot1(),
      riot2(),
      serialBus(serialBus_),
      memory_rom((uint8_t *) 0),
      deviceNumber(devNum_ & 7),
      updatePinCnt(0),
      updateMotorCnt(0),
      headPosX(marginLeft),
      headPosY(pixelToYPos(marginTop)),
      motorXPhase(0),
      prvMotorXPhase(0),
      motorXCnt(0),
      motorYPhase(0),
      prvMotorYPhase(0),
      motorYCnt(0),
      pinState(0x00),
      prvPinState(0x00),
      viaInterruptFlag(false),
      changeFlag(true),
      pageBuf((uint8_t *) 0),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0),
      noBreakOnDataRead(false)
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

  void VC1526::updatePins()
  {
    pinState = riot2.getPortB() ^ 0xFF;
    pinState = pinState & (((via.getPortB() & 0x01) - 1) & 0xFF);
    if (pinState != prvPinState) {
      uint8_t tmp = (pinState ^ prvPinState) & pinState;
      if (tmp != 0x00 && headPosX >= 0 && headPosX < pageWidth) {
        changeFlag = true;
        int     headPosYInt = yPosToPixel(headPosY);
        for (int i = 0; i < 8; i++) {
          if (tmp & 0x80) {
            int     y = headPosYInt + i;
            if (y >= 0 && y < pageHeight) {
              int     n = (y * pageWidth) + headPosX;
              pageBuf[n] = (pageBuf[n] >> 3) + (pageBuf[n] >> 4);
            }
          }
          tmp = tmp << 1;
        }
      }
      prvPinState = pinState;
    }
  }

  void VC1526::updateMotors()
  {
    uint8_t viaPortAOutput = via.getPortA();
    int     newMotorXPhase = motorXPhase;
    if (riot2.getPortA() & 0x02) {
      switch (viaPortAOutput & 0x0F) {
      case 0x05:
        newMotorXPhase = 0;
        break;
      case 0x06:
        newMotorXPhase = 1;
        break;
      case 0x0A:
        newMotorXPhase = 2;
        break;
      case 0x09:
        newMotorXPhase = 3;
        break;
      }
    }
    int     newMotorYPhase = motorYPhase;
    if (riot2.getPortA() & 0x01) {
      switch (viaPortAOutput & 0xF0) {
      case 0x90:
        newMotorYPhase = 0;
        break;
      case 0xA0:
        newMotorYPhase = 1;
        break;
      case 0x60:
        newMotorYPhase = 2;
        break;
      case 0x50:
        newMotorYPhase = 3;
        break;
      }
    }
    if (newMotorXPhase != motorXPhase) {
      motorXPhase = newMotorXPhase;
      if (motorXPhase != prvMotorXPhase)
        motorXCnt = 3;
    }
    if (newMotorYPhase != motorYPhase) {
      motorYPhase = newMotorYPhase;
      if (motorYPhase != prvMotorYPhase)
        motorYCnt = 3;
    }
    if (motorXCnt) {
      motorXCnt--;
      if (!motorXCnt) {
        switch ((motorXPhase - prvMotorXPhase) & 3) {
        case 1:
          headPosX = (headPosX < (pageWidth - 1) ? (headPosX + 1) : headPosX);
          break;
        case 3:
          headPosX = (headPosX > 0 ? (headPosX - 1) : headPosX);
          break;
        }
        prvMotorXPhase = motorXPhase;
      }
    }
    if (motorYCnt) {
      motorYCnt--;
      if (!motorYCnt) {
        switch ((motorYPhase - prvMotorYPhase) & 3) {
        case 1:
          headPosY = (headPosY < pixelToYPos(pageHeight - 1) ?
                      (headPosY + 1) : headPosY);
          break;
        case 3:
          headPosY = (headPosY > 0 ? (headPosY - 1) : headPosY);
          break;
        }
        prvMotorYPhase = motorYPhase;
      }
    }
    if (motorXCnt != 0) {
      via.setCA1(true);
      riot2.setPortA(riot2.getPortAInput() | 0x40);
    }
    else {
      via.setCA1(false);
      riot2.setPortA(riot2.getPortAInput() & 0xBF);
    }
    if (headPosX <= marginLeft) {
      via.setCA2(false);
      riot2.setPortA(riot2.getPortAInput() & 0x7F);
    }
    else {
      via.setCA2(true);
      riot2.setPortA(riot2.getPortAInput() | 0x80);
    }
    if (headPosY < pixelToYPos(pageHeight - marginBottom))
      riot2.setPortA(riot2.getPortAInput() | 0x04);
    else
      riot2.setPortA(riot2.getPortAInput() & 0xFB);
  }

  void VC1526::runOneCycle()
  {
    {
      bool    dataOut = !(riot1.getPortA() & 0x40);
      bool    atnAck = !(riot1.getPortA() & 0x20);
      atnAck = atnAck && !(serialBus.getATN());
      serialBus.setDATA(deviceNumber, dataOut && !atnAck);
      uint8_t riot1PortAInput = riot1.getPortAInput() & 0x7C;
      riot1PortAInput = riot1PortAInput
                        | uint8_t(((serialBus.getATN() & 0x01)
                                   | (serialBus.getCLK() & 0x02)
                                   | (serialBus.getDATA() & 0x80)) ^ 0x83);
      riot1.setPortA(riot1PortAInput);
    }
    if (viaInterruptFlag
        | riot1.isInterruptRequest() | riot2.isInterruptRequest()) {
      cpu.interruptRequest();
    }
    cpu.runOneCycle();
    via.runOneCycle();
    riot1.runOneCycle();
    riot2.runOneCycle();
    if (!updatePinCnt) {
      updatePinCnt = updatePinReload;
      if (updateMotorCnt <= 0) {
        updateMotorCnt = updateMotorCnt + updateMotorReload;
        updateMotors();
      }
      updateMotorCnt = updateMotorCnt - updatePinReload;
      updatePins();
    }
    updatePinCnt--;
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
    changeFlag = true;
    for (size_t i = 0; i < size_t(pageWidth * pageHeight); i++)
      pageBuf[i] = 0xFF;        // clear to white
    headPosY = pixelToYPos(marginTop);
  }

  uint8_t VC1526::getLEDState() const
  {
    return uint8_t(((riot2.getPortA() ^ 0xFF) & 0x20) >> 5);
  }

  void VC1526::getHeadPosition(int& xPos, int& yPos)
  {
    xPos = headPosX;
    yPos = yPosToPixel(headPosY);
  }

  bool VC1526::getIsOutputChanged() const
  {
    return changeFlag;
  }

  void VC1526::clearOutputChangedFlag()
  {
    changeFlag = false;
  }

  void VC1526::setEnable1525Mode(bool isEnabled)
  {
    if (isEnabled)
      riot1.setPortB(riot1.getPortBInput() & 0x7F);
    else
      riot1.setPortB(riot1.getPortBInput() | 0x80);
  }

  void VC1526::setFormFeedOn(bool isEnabled)
  {
    if (isEnabled)
      riot2.setPortA(riot2.getPortAInput() & 0xF7);
    else
      riot2.setPortA(riot2.getPortAInput() | 0x08);
  }

  void VC1526::reset()
  {
    via.reset();
    riot1.reset();
    riot2.reset();
    viaInterruptFlag = false;
    cpu.reset();
    riot1.setPortB((riot1.getPortBInput() & 0xF8) | uint8_t(deviceNumber & 3));
  }

  M7501 * VC1526::getCPU()
  {
    return (&cpu);
  }

  const M7501 * VC1526::getCPU() const
  {
    return (&cpu);
  }

  void VC1526::setBreakPointCallback(void (*breakPointCallback_)(
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

  void VC1526::setNoBreakOnDataRead(bool n)
  {
    noBreakOnDataRead = n;
  }

  uint8_t VC1526::readMemoryDebug(uint16_t addr) const
  {
    if ((addr & 0x1FFF) < 0x0400) {
      switch (addr & 0x02C0) {
      case 0x0000:
      case 0x0040:
        return riot1.readMemory(addr);
      case 0x0080:
      case 0x00C0:
        return riot2.readMemory(addr);
      case 0x0200:
        return riot1.readRegisterDebug(addr);
      case 0x0240:
        return via.readRegisterDebug(addr);
      case 0x0280:
        return riot2.readRegisterDebug(addr);
      case 0x02C0:
        return via.readRegisterDebug(addr);
      }
    }
    if (memory_rom)
      return memory_rom[addr & 0x1FFF];
    return uint8_t(0xFF);
  }

  void VC1526::writeMemoryDebug(uint16_t addr, uint8_t value)
  {
    if ((addr & 0x1FFF) >= 0x0400)
      return;
    switch (addr & 0x02C0) {
    case 0x0000:
    case 0x0040:
      riot1.writeMemory(addr, value);
      break;
    case 0x0080:
    case 0x00C0:
      riot2.writeMemory(addr, value);
      break;
    case 0x0200:
      riot1.writeRegister(addr, value);
      break;
    case 0x0240:
      via.writeRegister(addr, value);
      break;
    case 0x0280:
      riot2.writeRegister(addr, value);
      break;
    case 0x02C0:
      via.writeRegister(addr, value);
      break;
    }
  }

}       // namespace Plus4

