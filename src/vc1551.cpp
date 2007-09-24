
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
#include "serial.hpp"
#include "p4floppy.hpp"
#include "vc1541.hpp"
#include "vc1551.hpp"

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

  VC1551::M7501_::M7501_(VC1551& vc1551_)
    : M7501(),
      vc1551(vc1551_)
  {
    // initialize memory map
    setMemoryCallbackUserData((void *) &vc1551_);
    for (uint16_t i = 0x0000; i <= 0x0FFF; i++) {
      setMemoryReadCallback(i, &VC1551::readMemory_RAM);
      setMemoryWriteCallback(i, &VC1551::writeMemory_RAM);
    }
    setMemoryWriteCallback(0x0001, &VC1551::writeMemory_0001);
    for (uint16_t i = 0x1000; i <= 0x3FFF; i++) {
      setMemoryReadCallback(i, &VC1551::readMemory_Dummy);
      setMemoryWriteCallback(i, &VC1551::writeMemory_Dummy);
    }
    for (uint16_t i = 0x4000; i <= 0x7FFF; i++) {
      setMemoryReadCallback(i, &VC1551::readMemory_TIA);
      setMemoryWriteCallback(i, &VC1551::writeMemory_TIA);
    }
    for (uint16_t i = 0x8000; i <= 0xBFFF; i++) {
      setMemoryReadCallback(i, &VC1551::readMemory_Dummy);
      setMemoryWriteCallback(i, &VC1551::writeMemory_Dummy);
    }
    for (uint32_t i = 0xC000; i <= 0xFFFF; i++) {
      setMemoryReadCallback(uint16_t(i), &VC1551::readMemory_ROM);
      setMemoryWriteCallback(uint16_t(i), &VC1551::writeMemory_Dummy);
    }
  }

  VC1551::M7501_::~M7501_()
  {
  }

  void VC1551::M7501_::breakPointCallback(int type,
                                          uint16_t addr, uint8_t value)
  {
    if (vc1551.noBreakOnDataRead && type == 1)
      return;
    vc1551.breakPointCallback(vc1551.breakPointCallbackUserData,
                              (vc1551.deviceNumber & 3) + 1, type, addr, value);
  }

  // --------------------------------------------------------------------------

  uint8_t VC1551::readMemory_RAM(void *userData, uint16_t addr)
  {
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.dataBusState = vc1551.memory_ram[addr & 0x07FF];
    return vc1551.dataBusState;
  }

  uint8_t VC1551::readMemory_Dummy(void *userData, uint16_t addr)
  {
    (void) addr;
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    return vc1551.dataBusState;
  }

  uint8_t VC1551::readMemory_TIA(void *userData, uint16_t addr)
  {
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.dataBusState = vc1551.tpi1.readRegister(addr);
    return vc1551.dataBusState;
  }

  uint8_t VC1551::readMemory_ROM(void *userData, uint16_t addr)
  {
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    if (vc1551.memory_rom)
      vc1551.dataBusState = vc1551.memory_rom[addr & 0x3FFF];
    return vc1551.dataBusState;
  }

  void VC1551::writeMemory_RAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.dataBusState = value & 0xFF;
    vc1551.memory_ram[addr & 0x07FF] = vc1551.dataBusState;
  }

  void VC1551::writeMemory_0001(void *userData, uint16_t addr, uint8_t value)
  {
    (void) addr;
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.memory_ram[0x0001] &= uint8_t(0x90);
    vc1551.memory_ram[0x0001] |= (value & uint8_t(0x6F));
  }

  void VC1551::writeMemory_Dummy(void *userData, uint16_t addr, uint8_t value)
  {
    (void) addr;
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.dataBusState = value & 0xFF;
  }

  void VC1551::writeMemory_TIA(void *userData, uint16_t addr, uint8_t value)
  {
    VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(userData));
    vc1551.dataBusState = value & 0xFF;
    vc1551.tpi1.writeRegister(addr, vc1551.dataBusState);
    vc1551.updateParallelInterface();
  }

  // --------------------------------------------------------------------------

  bool VC1551::updateMotors()
  {
    int     prvTrackPosFrac = currentTrackFrac;
    // 16 * (65536 / 128) cycles / 1000000 Hz = ~8.2 ms seek time
    currentTrackFrac = currentTrackFrac + (steppingDirection * 128);
    currentTrackFrac = currentTrackFrac & (~(int(127)));
    if (((currentTrackFrac ^ prvTrackPosFrac) & 0xC000) == 0x4000) {
      if (steppingDirection > 0)
        currentTrackStepperMotorPhase = (currentTrackStepperMotorPhase + 1) & 3;
      else
        currentTrackStepperMotorPhase = (currentTrackStepperMotorPhase + 3) & 3;
    }
    uint8_t stepperMotorPhase = memory_ram[0x0001] & 3;
    switch ((stepperMotorPhase - currentTrackStepperMotorPhase) & 3) {
    case 1:
      steppingDirection = 1;    // stepping in
      break;
    case 3:
      steppingDirection = -1;   // stepping out
      break;
    default:                    // not stepping
      if (!(currentTrackFrac & 0x4000))
        steppingDirection = (!(currentTrackFrac & 0x7FFF) ? 0 : -1);
      else
        steppingDirection = 1;
      break;
    }
    if (currentTrackFrac <= -65536 || currentTrackFrac >= 65536) {
      // done stepping one track
      // FIXME: should report errors ?
      (void) setCurrentTrack(currentTrack + (currentTrackFrac > 0 ? 1 : -1));
    }
    if (diskChangeCnt) {
      diskChangeCnt--;
      if (diskChangeCnt == 0) {
        // after disk change, the write protection flag is inverted for 0.25
        // seconds
        memory_ram[0x0001] = uint8_t((memory_ram[0x0001] & 0xEF)
                                     | (writeProtectFlag ? 0x00 : 0x10));
        spindleMotorSpeed = 0;
      }
      return false;
    }
    // update spindle motor speed
    // spin up/down time is 16 * (65536 / 4) cycles / 1000000 Hz = ~262 ms
    if (!(memory_ram[0x0001] & 0x04)) {
      spindleMotorSpeed = spindleMotorSpeed - 4;
      spindleMotorSpeed = (spindleMotorSpeed >= 0 ? spindleMotorSpeed : 0);
    }
    else {
      spindleMotorSpeed = spindleMotorSpeed + 4;
      spindleMotorSpeed =
          (spindleMotorSpeed < 65536 ? spindleMotorSpeed : 65536);
    }
    // return true if data can be read/written by the head
    return (currentTrackFrac == 0 && spindleMotorSpeed == 65536 &&
            (currentTrack >= 1 && currentTrack <= nTracks));
  }

  bool VC1551::setCurrentTrack(int trackNum)
  {
    currentTrackFrac = 0;
    int     oldTrackNum = currentTrack;
    bool    retval = D64Image::setCurrentTrack(trackNum);
    if (currentTrack != oldTrackNum) {
      // recalculate head position
      headPosition = (headPosition * trackSizeTable[trackNum])
                     / trackSizeTable[currentTrack];
    }
    return retval;
  }

  // --------------------------------------------------------------------------

  VC1551::VC1551(SerialBus& serialBus_, int driveNum_)
    : FloppyDrive(serialBus_, driveNum_),
      D64Image(),
      cpu(*this),
      memory_rom((uint8_t *) 0),
      tpi1(),
      tpi2(),
      deviceNumber(uint8_t(driveNum_)),
      dataBusState(0x00),
      headLoadedFlag(false),
      prvByteWasFF(false),
      syncFlag(false),
      motorUpdateCnt(0),
      shiftRegisterBitCnt(0),
      shiftRegisterBitCntFrac(0x0000),
      interruptTimer(8325),
      headPosition(0),
      currentTrackFrac(0),
      steppingDirection(0),
      currentTrackStepperMotorPhase(0),
      spindleMotorSpeed(0),
      diskChangeCnt(15625),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0),
      noBreakOnDataRead(false)
  {
    // clear RAM
    for (int i = 0; i < 2048; i++)
      memory_ram[i] = 0x00;
    this->reset();
    tpi2.writeRegister(2, 0x40);
    tpi2.writeRegister(3, 0xFF);
    tpi2.writeRegister(4, 0x00);
    tpi2.writeRegister(5, 0x40);
    updateParallelInterface();
  }

  VC1551::~VC1551()
  {
  }

  void VC1551::setROMImage(int n, const uint8_t *romData_)
  {
    if (n == 3)
      memory_rom = romData_;
  }

  void VC1551::setDiskImageFile(const std::string& fileName_)
  {
    headLoadedFlag = false;
    prvByteWasFF = false;
    syncFlag = false;
    spindleMotorSpeed = 0;
    diskChangeCnt = 15625;
    currentTrackStepperMotorPhase = 0;
    (void) setCurrentTrack(18);         // FIXME: should report errors ?
    memory_ram[0x0001] &= uint8_t(0xEF);
    D64Image::setImageFile(fileName_);
    // invert write protect sense input for 0.25 seconds so that the DOS can
    // detect the disk change
    memory_ram[0x0001] = uint8_t((memory_ram[0x0001] & 0xEF)
                                 | (writeProtectFlag ? 0x10 : 0x00));
  }

  bool VC1551::haveDisk() const
  {
    return (imageFile != (std::FILE *) 0);
  }

  void VC1551::runOneCycle()
  {
    if (--interruptTimer < 0) {
      if (interruptTimer <= -7)
        interruptTimer = 8325;
      cpu.interruptRequest();
    }
    cpu.runOneCycle();
    cpu.runOneCycle();
    if (!motorUpdateCnt) {
      motorUpdateCnt = 16;
      headLoadedFlag = updateMotors();
    }
    motorUpdateCnt--;
    shiftRegisterBitCntFrac = shiftRegisterBitCntFrac
                              + trackSpeedTable[currentTrack];
    if (shiftRegisterBitCntFrac >= 65536) {
      shiftRegisterBitCntFrac = shiftRegisterBitCntFrac & 0xFFFF;
      shiftRegisterBitCnt = (shiftRegisterBitCnt + 1) & 7;
      if (shiftRegisterBitCnt == 0) {
        syncFlag = false;
        if (tpi1.getPortCOutput() & 0x10) {
          // read mode
          uint8_t readByte = 0x00;
          if (headLoadedFlag) {
            readByte = trackBuffer_GCR[headPosition];
            if (readByte == 0xFF)
              syncFlag = prvByteWasFF;
          }
          prvByteWasFF = (readByte == 0xFF);
          tpi1.setPortB(readByte);
        }
        else {
          // write mode
          tpi1.setPortB(0xFF);
          if (headLoadedFlag && !writeProtectFlag) {
            trackDirtyFlag = true;
            trackBuffer_GCR[headPosition] = tpi1.getPortBOutput();
          }
          prvByteWasFF = false;
        }
        tpi1.setPortCBits(0x40, uint8_t(syncFlag ? 0x00 : 0x40));
        // set byte ready flag
        if (!syncFlag) {
          memory_ram[0x0001] |= uint8_t(0x80);
        }
        // update head position
        if (spindleMotorSpeed >= 32768) {
          headPosition = headPosition + 1;
          if (headPosition >= trackSizeTable[currentTrack])
            headPosition = 0;
        }
      }
      else if (shiftRegisterBitCnt == 2) {
        // clear byte ready flag
        // FIXME: setting this for two bits is a hack
        // to work around bytes getting lost sometimes
        memory_ram[0x0001] &= uint8_t(0x7F);
      }
    }
  }

  void VC1551::reset()
  {
    (void) flushTrack();        // FIXME: should report errors ?
    cpu.reset();
    memory_ram[0x0000] = 0x00;
    memory_ram[0x0001] &= uint8_t(0x90);
    tpi1.reset();
    tpi2.reset();
    // set device number
    tpi1.setPortC(uint8_t(0xDF | ((deviceNumber & 0x01) << 5)));
    updateParallelInterface();
  }

  M7501 * VC1551::getCPU()
  {
    return (&cpu);
  }

  const M7501 * VC1551::getCPU() const
  {
    return (&cpu);
  }

  void VC1551::setBreakPointCallback(void (*breakPointCallback_)(
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

  void VC1551::setNoBreakOnDataRead(bool n)
  {
    noBreakOnDataRead = n;
  }

  uint8_t VC1551::readMemoryDebug(uint16_t addr) const
  {
    if (addr < 0x8000) {
      if (addr < 0x1000)
        return memory_ram[addr & 0x07FF];
      else if (addr >= 0x4000)
        return tpi1.readRegister(addr);
    }
    else if (addr >= 0xC000) {
      if (memory_rom)
        return memory_rom[addr & 0x3FFF];
    }
    return uint8_t(0xFF);
  }

  void VC1551::writeMemoryDebug(uint16_t addr, uint8_t value)
  {
    if (addr < 0x1000) {
      if (addr != 0x0001) {
        memory_ram[addr & 0x07FF] = value;
      }
      else {
        memory_ram[0x0001] &= uint8_t(0x90);
        memory_ram[0x0001] |= (value & uint8_t(0x6F));
      }
    }
    else if ((addr & 0xC000) == 0x4000) {
      tpi1.writeRegister(addr, value);
      updateParallelInterface();
    }
  }

  uint8_t VC1551::getLEDState() const
  {
    return uint8_t(((memory_ram[0x0001] ^ 0xFF) & 0x08) >> 2);
  }

  uint16_t VC1551::getHeadPosition() const
  {
    if (!imageFile)
      return 0xFFFF;
    uint16_t  retval = uint16_t((currentTrack & 0x7F) << 8);
    // FIXME: this is not accurate
    retval |= uint16_t((headPosition / 367) & 0x7F);
    return retval;
  }

  void VC1551::saveState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1551::saveState(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void VC1551::loadState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1551::registerChunkTypes(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void VC1551::updateParallelInterface()
  {
    uint8_t tmp = tpi1.getPortAOutput() & tpi2.getPortAOutput();
    tpi1.setPortA(tmp);
    tpi2.setPortA(tmp);
    uint8_t tmp2 = tpi1.getPortCOutput();
    tmp = (tpi2.getPortBOutput() & tmp2) & uint8_t(0x03);
    tpi1.setPortCBits(0x03, tmp);
    tpi2.setPortB(tmp);
    tmp = tpi2.getPortCOutput();
    tmp2 &= uint8_t(0x88);
    tmp = uint8_t(((tmp & (tmp2 << 4)) | (tmp & (tmp2 >> 1))) & 0xC0);
    tpi1.setPortCBits(0x88, uint8_t(((tmp & 0x80) >> 4) | ((tmp & 0x40) << 1)));
    tpi2.setPortC(tmp);
  }

  bool VC1551::parallelIECRead(uint16_t addr, uint8_t& value)
  {
    if ((uint8_t(addr >> 3) ^ tpi1.getPortCOutput()) & uint8_t(0x04))
      return false;
    value = tpi2.readRegister(addr);
    return true;
  }

  bool VC1551::parallelIECWrite(uint16_t addr, uint8_t value)
  {
    if ((uint8_t(addr >> 3) ^ tpi1.getPortCOutput()) & uint8_t(0x04))
      return false;
    tpi2.writeRegister(addr, value);
    updateParallelInterface();
    return true;
  }

}       // namespace Plus4

