
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
#include "display.hpp"
#include "soundio.hpp"
#include "plus4vm.hpp"
#include "disasm.hpp"

#include <cstdio>
#include <cmath>
#include <vector>

#include "resid/sid.hpp"
#include "p4floppy.hpp"
#include "vc1526.hpp"
#include "vc1541.hpp"
#include "vc1551.hpp"
#include "vc1581.hpp"

static void writeDemoTimeCnt(Plus4Emu::File::Buffer& buf, uint64_t n)
{
  uint64_t  mask = uint64_t(0x7F) << 49;
  uint8_t   rshift = 49;
  while (rshift != 0 && !(n & mask)) {
    mask >>= 7;
    rshift -= 7;
  }
  while (rshift != 0) {
    buf.writeByte(uint8_t((n & mask) >> rshift) | 0x80);
    mask >>= 7;
    rshift -= 7;
  }
  buf.writeByte(uint8_t(n) & 0x7F);
}

static uint64_t readDemoTimeCnt(Plus4Emu::File::Buffer& buf)
{
  uint64_t  n = 0U;
  uint8_t   i = 8, c;
  do {
    c = buf.readByte();
    n = (n << 7) | uint64_t(c & 0x7F);
    i--;
  } while ((c & 0x80) != 0 && i != 0);
  return n;
}

namespace Plus4 {

  Plus4VM::TED7360_::TED7360_(Plus4VM& vm_)
    : TED7360(),
      vm(vm_),
      lineCnt_(0),
      serialPort()
  {
    setMemoryReadCallback(0x0001, memoryRead0001Callback);
    setMemoryWriteCallback(0x0001, memoryWrite0001Callback);
    for (uint16_t i = 0x00; i <= 0x1F; i++) {
      setMemoryReadCallback(uint16_t(0xFD40) + i, &sidRegisterRead);
      setMemoryWriteCallback(uint16_t(0xFD40) + i, &sidRegisterWrite);
      setMemoryReadCallback(uint16_t(0xFE80) + i, &sidRegisterRead);
      setMemoryWriteCallback(uint16_t(0xFE80) + i, &sidRegisterWrite);
    }
    for (uint16_t i = 0xFEC0; i <= 0xFEFF; i++) {
      setMemoryReadCallback(i, &parallelIECRead);
      setMemoryWriteCallback(i, &parallelIECWrite);
    }
  }

  Plus4VM::TED7360_::~TED7360_()
  {
  }

  void Plus4VM::TED7360_::playSample(int16_t sampleValue)
  {
    int32_t tmp = vm.soundOutputAccumulator;
    if (tmp != 0) {
      vm.soundOutputAccumulator = 0;
      tmp = (tmp * int32_t(3)) / int32_t(176);
      tmp = (tmp >= -24576 ? (tmp < 24576 ? tmp : 24576) : -24576);
    }
    tmp += int32_t(sampleValue);
    vm.sendMonoAudioOutput(tmp);
  }

  void Plus4VM::TED7360_::videoOutputCallback(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.sendVideoOutput(buf, nBytes);
  }

  void Plus4VM::TED7360_::ntscModeChangeCallback(bool isNTSC_)
  {
    vm.updateTimingParameters(isNTSC_);
  }

  bool Plus4VM::TED7360_::systemCallback(uint8_t n)
  {
    if (getSegmentType(getMemoryPage(int(reg_PC >> 14))) != 1 ||
        !(vm.fileIOEnabled && reg_PC >= 0x8004 && reg_PC < 0xFD00))
      return false;
    switch (n) {
    case 0x01:                          // load file
    case 0x02:                          // verify file
      vm.stopDemoPlayback();
      vm.stopDemoRecording(false);
      {
        std::string fileName;
        uint8_t   nameLen = readMemoryCPU(0x00AB);
        uint16_t  nameAddr = uint16_t(readMemoryCPU(0x00AF))
                             | (uint16_t(readMemoryCPU(0x00B0)) << 8);
        while (nameLen) {
          char    c = char(readMemoryCPU(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
          nameAddr = (nameAddr + 1) & 0xFFFF;
        }
        std::FILE *f = (std::FILE *) 0;
        int       err = vm.openFileInWorkingDirectory(f, fileName, "rb");
        if (!err) {
          reg_AC = 0xF9;
          int     c;
          c = std::fgetc(f);
          if (c != EOF) {
            uint16_t  addr = uint16_t(c & 0xFF);
            c = std::fgetc(f);
            if (c != EOF) {
              addr |= (uint16_t(c & 0xFF) << 8);
              reg_AC = 0x00;
              if (readMemoryCPU(0x00AD) == 0x00)
                addr = uint16_t(readMemoryCPU(0x00B4))
                       | (uint16_t(readMemoryCPU(0x00B5)) << 8);
              unsigned int  nBytes = 0U;
              do {
                c = std::fgetc(f);
                if (c == EOF)
                  break;
                if (n == 0x01)          // load
                  writeMemoryCPU(addr, uint8_t(c & 0xFF));
                else if (uint8_t(c & 0xFF) != readMemoryCPU(addr, true)) {
                  reg_AC = 0xF8;        // verify error
                  break;
                }
                addr = (addr + 1) & 0xFFFF;
              } while (++nBytes < 0xFFFFU);
              writeMemoryCPU(0x009D, uint8_t(addr) & 0xFF);
              writeMemoryCPU(0x009E, uint8_t(addr >> 8) & 0xFF);
            }
          }
          std::fclose(f);
        }
        else
          reg_AC = uint8_t(err + 256);
        return true;
      }
      break;
    case 0x03:                          // save file
      vm.stopDemoPlayback();
      vm.stopDemoRecording(false);
      {
        std::string fileName;
        uint8_t   nameLen = readMemoryCPU(0x00AB);
        uint16_t  nameAddr = uint16_t(readMemoryCPU(0x00AF))
                             | (uint16_t(readMemoryCPU(0x00B0)) << 8);
        while (nameLen) {
          char    c = char(readMemoryCPU(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
          nameAddr = (nameAddr + 1) & 0xFFFF;
        }
        std::FILE *f = (std::FILE *) 0;
        int       err = vm.openFileInWorkingDirectory(f, fileName, "wb");
        if (!err) {
          reg_AC = 0xFA;
          uint8_t   c = readMemoryCPU(0x00B2);
          uint16_t  addr = uint16_t(c);
          if (std::fputc(c, f) != EOF) {
            c = readMemoryCPU(0x00B3);
            addr |= (uint16_t(c) << 8);
            if (std::fputc(c, f) != EOF) {
              uint16_t  endAddr = uint16_t(readMemoryCPU(0x009D))
                                  | (uint16_t(readMemoryCPU(0x009E)) << 8);
              endAddr = endAddr & 0xFFFF;
              while (addr != endAddr) {
                c = readMemoryCPU(addr, true);
                if (std::fputc(c, f) == EOF)
                  break;
                addr = (addr + 1) & 0xFFFF;
              }
              if (addr == endAddr)
                reg_AC = 0x00;
            }
          }
          std::fclose(f);
        }
        else
          reg_AC = uint8_t(err + 256);
        return true;
      }
      break;
    }
    return false;
  }

  void Plus4VM::TED7360_::breakPointCallback(int type,
                                             uint16_t addr, uint8_t value)
  {
    if (vm.noBreakOnDataRead && type == 1)
      return;
    vm.breakPointCallback(vm.breakPointCallbackUserData, 0, type, addr, value);
  }

  void Plus4VM::TED7360_::reset(bool cold_reset)
  {
    if (cold_reset) {
      serialPort.removeDevices(0xFFFF);
      serialPort.setATN(true);
    }
    TED7360::reset(cold_reset);
  }

  uint8_t Plus4VM::TED7360_::sidRegisterRead(void *userData, uint16_t addr)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    if (ted.vm.sidEnabled) {
      ted.dataBusState =
          uint8_t(ted.vm.sid_->read(uint8_t(addr) & uint8_t(0x1F)));
    }
    return ted.dataBusState;
  }

  void Plus4VM::TED7360_::sidRegisterWrite(void *userData,
                                           uint16_t addr, uint8_t value)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    ted.dataBusState = value;
    if (!ted.vm.sidEnabled) {
      ted.vm.sidEnabled = true;
      ted.setCallback(&(ted.vm.sidCallback), &(ted.vm), 1);
    }
    ted.vm.sid_->write(uint8_t(addr) & uint8_t(0x1F), value);
  }

  uint8_t Plus4VM::TED7360_::parallelIECRead(void *userData, uint16_t addr)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
#if 0
    if (!(addr & 0x0007)) {
      // work around kernal parallel interface test bug
      ted.dataBusState |= uint8_t(0x02);
    }
#endif
    if (!(ted.vm.isRecordingDemo | ted.vm.isPlayingDemo)) {
      for (int i = 0; i < 2; i++) {
        FloppyDrive *p = ted.vm.floppyDrives[i].floppyDrive;
        if (p != (FloppyDrive *) 0) {
          if (typeid(*p) == typeid(VC1551)) {
            VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(p));
            if (vc1551.parallelIECRead(addr, ted.dataBusState))
              break;
          }
        }
      }
    }
    return ted.dataBusState;
  }

  void Plus4VM::TED7360_::parallelIECWrite(void *userData,
                                           uint16_t addr, uint8_t value)
  {
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    ted.dataBusState = value;
    if (!(ted.vm.isRecordingDemo | ted.vm.isPlayingDemo)) {
      for (int i = 0; i < 2; i++) {
        FloppyDrive *p = ted.vm.floppyDrives[i].floppyDrive;
        if (p != (FloppyDrive *) 0) {
          if (typeid(*p) == typeid(VC1551)) {
            VC1551& vc1551 = *(reinterpret_cast<VC1551 *>(p));
            (void) vc1551.parallelIECWrite(addr, ted.dataBusState);
          }
        }
      }
    }
  }

  uint8_t Plus4VM::TED7360_::memoryRead0001Callback(void *userData,
                                                    uint16_t addr)
  {
    (void) addr;
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    uint8_t tmp = ted.ioPortRead();
    uint8_t mask_ = ted.ioRegister_0000;
    uint8_t nmask_ = mask_ ^ uint8_t(0xFF);
    if (!(ted.vm.isRecordingDemo | ted.vm.isPlayingDemo)) {
      tmp &= (ted.serialPort.getCLK() | uint8_t(0xBF));
      tmp &= (ted.serialPort.getDATA() | uint8_t(0x7F));
    }
    else {
      uint8_t tmp2 = ted.ioRegister_0001 | nmask_;
      tmp2 = ((tmp2 & uint8_t(0x01)) << 7) | ((tmp2 & uint8_t(0x02)) << 5);
      tmp &= (tmp2 ^ uint8_t(0xFF));
    }
    tmp = (tmp & nmask_) | (ted.ioRegister_0001 & mask_);
    return tmp;
  }

  void Plus4VM::TED7360_::memoryWrite0001Callback(void *userData,
                                                  uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360_& ted = *(reinterpret_cast<TED7360_ *>(userData));
    ted.ioRegister_0001 = value;
    uint8_t tmp = value | (ted.ioRegister_0000 ^ uint8_t(0xFF));
    uint8_t tmp2 = tmp ^ uint8_t(0xFF);
    tmp |= uint8_t(((tmp2 & 0x80) >> 7) | ((tmp2 & 0x40) >> 5));
    // FIXME: tape output should also be affected by other devices on the
    // serial bus
    ted.ioPortWrite(tmp);
    if (!(ted.vm.isRecordingDemo | ted.vm.isPlayingDemo)) {
      ted.serialPort.setDATA(0, !(tmp & uint8_t(0x01)));
      ted.serialPort.setCLK(0, !(tmp & uint8_t(0x02)));
      ted.serialPort.setATN(!(tmp & uint8_t(0x04)));
    }
  }

  void Plus4VM::TED7360_::floppyCallback(void *userData)
  {
    Plus4VM::FloppyDrive_&  floppyDrive =
        *(reinterpret_cast<Plus4VM::FloppyDrive_ *>(userData));
    TED7360_& ted = *(floppyDrive.ted);
    Plus4VM&  vm = ted.vm;
    floppyDrive.timeRemaining += vm.tedTimesliceLength;
    while (floppyDrive.timeRemaining > 0) {
      // use a timeslice of fixed 1 us length (1 or 2 cycles, depending
      // on drive type)
      floppyDrive.timeRemaining -= (int64_t(1) << 32);
      floppyDrive.floppyDrive->runOneCycle(ted.serialPort);
    }
  }

  void Plus4VM::TED7360_::floppy1541Callback(void *userData)
  {
    Plus4VM::FloppyDrive_&  floppyDrive =
        *(reinterpret_cast<Plus4VM::FloppyDrive_ *>(userData));
    TED7360_& ted = *(floppyDrive.ted);
    Plus4VM&  vm = ted.vm;
    floppyDrive.timeRemaining =
        reinterpret_cast<VC1541 *>(floppyDrive.floppyDrive)->run(
            ted.serialPort,
            floppyDrive.timeRemaining + (vm.tedTimesliceLength >> 1));
  }

  // --------------------------------------------------------------------------

  void Plus4VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      ted->setCallback(&demoPlayCallback, this, 0);
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // tape button state sensing is disabled while recording or playing demo
      ted->setTapeButtonState(!isRecordingDemo && getTapeButtonState() != 0);
      // clear keyboard state at end of playback
      for (int i = 0; i < 128; i++)
        ted->setKeyState(i, false);
    }
  }

  void Plus4VM::stopDemoRecording(bool writeFile_)
  {
    if (isRecordingDemo) {
      isRecordingDemo = false;
      ted->setCallback(&demoRecordCallback, this, 0);
    }
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!isPlayingDemo && getTapeButtonState() != 0);
    if (writeFile_ && demoFile != (Plus4Emu::File *) 0) {
      // if file object is still open:
      try {
        // put end of demo event
        writeDemoTimeCnt(demoBuffer, demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x00);
        demoBuffer.writeByte(0x00);
        demoFile->addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_PLUS4_DEMO,
                           demoBuffer);
      }
      catch (...) {
        demoFile = (Plus4Emu::File *) 0;
        demoTimeCnt = 0U;
        demoBuffer.clear();
        throw;
      }
      demoFile = (Plus4Emu::File *) 0;
      demoTimeCnt = 0U;
      demoBuffer.clear();
    }
  }

  void Plus4VM::updateTimingParameters(bool ntscMode_)
  {
    size_t  singleClockFreq = tedInputClockFrequency;
    if (!ntscMode_)
      singleClockFreq = ((singleClockFreq + 40) / 80) << 2;
    else
      singleClockFreq = ((singleClockFreq + 32) / 64) << 2;
    tedTimesliceLength = int64_t(((uint64_t(1000000) << 32)
                                  + (singleClockFreq >> 1))
                                 / singleClockFreq);
    size_t  freqMult = cpuClockFrequency;
    if (freqMult > 1000)
      freqMult = (freqMult + singleClockFreq) / (singleClockFreq << 1);
    freqMult = (freqMult > 1 ? (freqMult < 100 ? freqMult : 100) : 1);
    ted->setCPUClockMultiplier(freqMult);
    if ((singleClockFreq >> 2) != soundClockFrequency) {
      soundClockFrequency = singleClockFreq >> 2;
      setAudioConverterSampleRate(float(long(soundClockFrequency)));
    }
  }

  void Plus4VM::addFloppyCallback(int n)
  {
    n = n & 3;
    if (is1541HighAccuracy &&
        typeid(*(floppyDrives[n].floppyDrive)) == typeid(VC1541))
      ted->setCallback(&TED7360_::floppy1541Callback, &(floppyDrives[n]), 3);
    else
      ted->setCallback(&TED7360_::floppyCallback, &(floppyDrives[n]), 1);
  }

  void Plus4VM::removeFloppyCallback(int n)
  {
    n = n & 3;
    ted->setCallback(&TED7360_::floppyCallback, &(floppyDrives[n]), 0);
    ted->setCallback(&TED7360_::floppy1541Callback, &(floppyDrives[n]), 0);
  }

  void Plus4VM::resetFloppyDrives(uint8_t driveMask_, bool deleteUnusedDrives_)
  {
    for (int i = 0; i < 4; i++) {
      if (driveMask_ & 0x01) {
        if (floppyDrives[i].floppyDrive) {
          if (floppyDrives[i].floppyDrive->haveDisk() || !deleteUnusedDrives_)
            floppyDrives[i].floppyDrive->reset();
          else {
            // "garbage collect" unused floppy drives to improve performance
            delete floppyDrives[i].floppyDrive;
            floppyDrives[i].floppyDrive = (FloppyDrive *) 0;
            ted->serialPort.removeDevice(i + 8);
            removeFloppyCallback(i);
          }
        }
      }
      driveMask_ = driveMask_ >> 1;
    }
  }

  M7501 * Plus4VM::getDebugCPU()
  {
    if (currentDebugContext == 0)
      return ted;
    else if (currentDebugContext <= 4) {
      if (floppyDrives[currentDebugContext - 1].floppyDrive
          != (FloppyDrive *) 0)
        return (floppyDrives[currentDebugContext - 1].floppyDrive->getCPU());
    }
    else if (printer_)
      return printer_->getCPU();
    return (M7501 *) 0;
  }

  const M7501 * Plus4VM::getDebugCPU() const
  {
    if (currentDebugContext == 0)
      return ted;
    else if (currentDebugContext <= 4) {
      if (floppyDrives[currentDebugContext - 1].floppyDrive
          != (FloppyDrive *) 0)
        return (floppyDrives[currentDebugContext - 1].floppyDrive->getCPU());
    }
    else if (printer_)
      return printer_->getCPU();
    return (M7501 *) 0;
  }

  void Plus4VM::tapeCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    vm.tapeTimeRemaining += vm.tedTimesliceLength;
    if (vm.tapeTimeRemaining > 0) {
      // assume tape sample rate < single clock frequency
      vm.tapeTimeRemaining -= vm.tapeTimesliceLength;
      vm.setTapeMotorState(vm.ted->getTapeMotorState());
      bool    tedTapeOutput = vm.ted->getTapeOutput();
      bool    tedTapeInput = (vm.runTape(tedTapeOutput ? 1 : 0) > 0);
      vm.ted->setTapeInput(tedTapeInput);
      int     tapeButtonState = vm.getTapeButtonState();
      bool    tapeFeedback = ((tapeButtonState == 1 && tedTapeInput) ||
                              (tapeButtonState == 2 && tedTapeOutput));
      vm.tapeFeedbackSignal = (tapeFeedback ?
                               vm.tapeFeedbackLevel : int32_t(0));
    }
    vm.soundOutputAccumulator += vm.tapeFeedbackSignal;
  }

  void Plus4VM::sidCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    vm.sid_->clock();
    vm.soundOutputAccumulator += int32_t(vm.sid_->fast_output());
  }

  void Plus4VM::demoPlayCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    while (!vm.demoTimeCnt) {
      if (vm.haveTape() &&
          vm.getIsTapeMotorOn() && vm.getTapeButtonState() != 0)
        vm.stopDemoPlayback();
      try {
        uint8_t evtType = vm.demoBuffer.readByte();
        uint8_t evtBytes = vm.demoBuffer.readByte();
        uint8_t evtData = 0;
        while (evtBytes) {
          evtData = vm.demoBuffer.readByte();
          evtBytes--;
        }
        switch (evtType) {
        case 0x00:
          vm.stopDemoPlayback();
          break;
        case 0x01:
          vm.ted->setKeyState(evtData, true);
          break;
        case 0x02:
          vm.ted->setKeyState(evtData, false);
          break;
        }
        vm.demoTimeCnt = readDemoTimeCnt(vm.demoBuffer);
      }
      catch (...) {
        vm.stopDemoPlayback();
      }
      if (!vm.isPlayingDemo) {
        vm.demoBuffer.clear();
        vm.demoTimeCnt = 0U;
        break;
      }
    }
    if (vm.demoTimeCnt)
      vm.demoTimeCnt--;
  }

  void Plus4VM::demoRecordCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    vm.demoTimeCnt++;
  }

  void Plus4VM::videoBreakPointCheckCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    TED7360_& ted_ = *(vm.ted);
    if (vm.videoBreakPoints) {
      uint16_t  n = (ted_.getVideoPositionY() << 7)
                    | uint16_t(ted_.getVideoPositionX() >> 1);
      if (vm.videoBreakPoints[n] != 0) {
        if (int(vm.videoBreakPoints[n]) > ted_.getBreakPointPriorityThreshold())
          vm.breakPointCallback(vm.breakPointCallbackUserData, 0, 4, n, 0x00);
      }
    }
  }

  void Plus4VM::lightPenCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    TED7360_& ted_ = *(vm.ted);
    if (vm.lightPenPositionX >= 0 && vm.lightPenPositionY >= 0) {
      if (ted_.checkLightPen(vm.lightPenPositionX, vm.lightPenPositionY)) {
        vm.lightPenCycleCounter = 150;
        vm.setKeyboardState(86, true);
      }
    }
    if (vm.lightPenCycleCounter) {
      vm.lightPenCycleCounter--;
      if (!vm.lightPenCycleCounter)
        vm.setKeyboardState(86, false);
    }
  }

  void Plus4VM::printerCallback(void *userData)
  {
    Plus4VM&  vm = *(reinterpret_cast<Plus4VM *>(userData));
    vm.printerTimeRemaining += vm.tedTimesliceLength;
    while (vm.printerTimeRemaining > 0) {
      vm.printerTimeRemaining -= (int64_t(1) << 32);
      vm.printer_->runOneCycle(vm.ted->serialPort);
    }
  }

  Plus4VM::Plus4VM(Plus4Emu::VideoDisplay& display_,
                   Plus4Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      ted((TED7360_ *) 0),
      cpuClockFrequency(1),
      tedInputClockFrequency(17734475),
      soundClockFrequency(0),
      tedTimesliceLength(0),
      tedTimeRemaining(0),
      tapeTimesliceLength(0),
      tapeTimeRemaining(0),
      demoFile((Plus4Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U),
      sid_((SID *) 0),
      soundOutputAccumulator(0),
      sidEnabled(false),
      tapeCallbackFlag(false),
      is1541HighAccuracy(true),
      floppyROM_1541((uint8_t *) 0),
      floppyROM_1551((uint8_t *) 0),
      floppyROM_1581_0((uint8_t *) 0),
      floppyROM_1581_1((uint8_t *) 0),
      printerROM_1526((uint8_t *) 0),
      videoBreakPointCnt(0),
      videoBreakPoints((uint8_t *) 0),
      tapeFeedbackSignal(0),
      tapeFeedbackLevel(0),
      lightPenPositionX(-1),
      lightPenPositionY(-1),
      lightPenCycleCounter(0),
      printer_((VC1526 *) 0),
      printerTimeRemaining(0),
      printerOutputChangedFlag(true),
      printer1525Mode(false),
      printerFormFeedOn(false)
  {
    sid_ = new SID();
    try {
      sid_->set_chip_model(MOS8580);
      sid_->enable_external_filter(false);
      sid_->reset();
      ted = new TED7360_(*this);
      for (int i = 0; i < 4; i++) {
        floppyDrives[i].floppyDrive = (FloppyDrive *) 0;
        floppyDrives[i].ted = ted;
        floppyDrives[i].timeRemaining = int64_t(0);
        floppyDrives[i].deviceNumber = i + 8;
      }
      updateTimingParameters(false);
      // reset
      ted->reset(true);
      // use PLUS/4 colormap
      Plus4Emu::VideoDisplay::DisplayParameters
          dp(display.getDisplayParameters());
      dp.indexToRGBFunc = &TED7360::convertPixelToRGB;
      display.setDisplayParameters(dp);
    }
    catch (...) {
      if (ted)
        delete ted;
      delete sid_;
      throw;
    }
  }

  Plus4VM::~Plus4VM()
  {
    try {
      // FIXME: cannot handle errors here
      stopDemo();
    }
    catch (...) {
    }
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i].floppyDrive) {
        delete floppyDrives[i].floppyDrive;
        floppyDrives[i].floppyDrive = (FloppyDrive *) 0;
      }
    }
    if (printer_) {
      delete printer_;
      printer_ = (VC1526 *) 0;
    }
    if (floppyROM_1541)
      delete[] floppyROM_1541;
    if (floppyROM_1551)
      delete[] floppyROM_1551;
    if (floppyROM_1581_0)
      delete[] floppyROM_1581_0;
    if (floppyROM_1581_1)
      delete[] floppyROM_1581_1;
    if (printerROM_1526)
      delete[] printerROM_1526;
    delete ted;
    delete sid_;
    if (videoBreakPoints)
      delete[] videoBreakPoints;
  }

  void Plus4VM::run(size_t microseconds)
  {
    Plus4Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo) {
        for (int i = 0; i < 128; i++)
          ted->setKeyState(i, false);
      }
    }
    bool    newTapeCallbackFlag = (getTapeButtonState() != 0);
    if (newTapeCallbackFlag != tapeCallbackFlag) {
      tapeCallbackFlag = newTapeCallbackFlag;
      if (!tapeCallbackFlag) {
        ted->setTapeInput(false);
        tapeFeedbackSignal = 0;
      }
      ted->setCallback(&tapeCallback, this, (tapeCallbackFlag ? 1 : 0));
    }
    tedTimeRemaining += (int64_t(microseconds) << 32);
    while (tedTimeRemaining > 0) {
      ted->runOneCycle();
      tedTimeRemaining -= tedTimesliceLength;
    }
  }

  void Plus4VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    ted->reset(isColdReset);
    setTapeMotorState(false);
    sid_->reset();
    if (isColdReset) {
      sidEnabled = false;
      ted->setCallback(&sidCallback, this, 0);
    }
    resetFloppyDrives(0x0F, isColdReset);
    if (printer_)
      printer_->reset();
  }

  void Plus4VM::resetMemoryConfiguration(size_t memSize, uint64_t ramPattern)
  {
    try {
      stopDemo();
      // delete all ROM segments
      for (uint8_t n = 0; n < 8; n++)
        loadROMSegment(n, (char *) 0, 0);
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i].floppyDrive) {
          for (int n = 0; n < 3; n++)
            floppyDrives[i].floppyDrive->setROMImage(n, (uint8_t *) 0);
        }
      }
      if (printer_)
        printer_->setROMImage((uint8_t *) 0);
      if (floppyROM_1541) {
        delete[] floppyROM_1541;
        floppyROM_1541 = (uint8_t *) 0;
      }
      if (floppyROM_1551) {
        delete[] floppyROM_1551;
        floppyROM_1551 = (uint8_t *) 0;
      }
      if (floppyROM_1581_0) {
        delete[] floppyROM_1581_0;
        floppyROM_1581_0 = (uint8_t *) 0;
      }
      if (floppyROM_1581_1) {
        delete[] floppyROM_1581_1;
        floppyROM_1581_1 = (uint8_t *) 0;
      }
      if (printerROM_1526) {
        delete[] printerROM_1526;
        printerROM_1526 = (uint8_t *) 0;
      }
      // set new RAM size
      ted->setRAMSize(memSize, ramPattern);
    }
    catch (...) {
      try {
        this->reset(true);
      }
      catch (...) {
      }
      throw;
    }
    // cold reset
    this->reset(true);
  }

  void Plus4VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    int     floppyROMSegment = -1;
    uint8_t **floppyROMPtr = (uint8_t **) 0;
    if (n >= 8) {
      switch (n) {
      case 0x0C:
        {
          // clear segment first
          if (printer_)
            printer_->setROMImage((uint8_t *) 0);
          if (fileName == (char *) 0 || fileName[0] == '\0') {
            // empty file name: delete segment
            return;
          }
          // load file into memory
          std::vector<uint8_t>  buf;
          buf.resize(0x2000);
          std::FILE   *f = std::fopen(fileName, "rb");
          if (!f)
            throw Plus4Emu::Exception("cannot open ROM file");
          std::fseek(f, 0L, SEEK_END);
          if (ftell(f) < long(offs + 0x2000)) {
            std::fclose(f);
            throw Plus4Emu::Exception("ROM file is shorter than expected");
          }
          std::fseek(f, long(offs), SEEK_SET);
          std::fread(&(buf.front()), 1, 0x2000, f);
          std::fclose(f);
          if (printerROM_1526 == (uint8_t *) 0)
            printerROM_1526 = new uint8_t[8192];
          for (int i = 0; i < 8192; i++)
            printerROM_1526[i] = buf[i];
          if (printer_)
            printer_->setROMImage(printerROM_1526);
          return;
        }
        break;
      case 0x10:
        floppyROMSegment = 2;
        floppyROMPtr = &floppyROM_1541;
        break;
      case 0x20:
        floppyROMSegment = 3;
        floppyROMPtr = &floppyROM_1551;
        break;
      case 0x30:
        floppyROMSegment = 0;
        floppyROMPtr = &floppyROM_1581_0;
        break;
      case 0x31:
        floppyROMSegment = 1;
        floppyROMPtr = &floppyROM_1581_1;
        break;
      default:
        return;
      }
    }
    // clear segment first
    if (floppyROMSegment < 0) {
      ted->loadROM(int(n >> 1), int(n & 1) << 14, 0, (uint8_t *) 0);
    }
    else {
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i].floppyDrive)
          floppyDrives[i].floppyDrive->setROMImage(floppyROMSegment,
                                                   (uint8_t *) 0);
      }
    }
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      return;
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000);
    std::FILE   *f = std::fopen(fileName, "rb");
    if (!f)
      throw Plus4Emu::Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    if (ftell(f) < long(offs + 0x4000)) {
      std::fclose(f);
      throw Plus4Emu::Exception("ROM file is shorter than expected");
    }
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), 1, 0x4000, f);
    std::fclose(f);
    if (floppyROMSegment < 0) {
      ted->loadROM(int(n) >> 1, int(n & 1) << 14, 16384, &(buf.front()));
    }
    else {
      if ((*floppyROMPtr) == (uint8_t *) 0)
        (*floppyROMPtr) = new uint8_t[16384];
      for (int i = 0; i < 16384; i++)
        (*floppyROMPtr)[i] = buf[i];
      for (int i = 0; i < 4; i++) {
        if (floppyDrives[i].floppyDrive)
          floppyDrives[i].floppyDrive->setROMImage(floppyROMSegment,
                                                   (*floppyROMPtr));
      }
    }
  }

  void Plus4VM::setCPUFrequency(size_t freq_)
  {
    size_t  freq = (freq_ > 1 ? (freq_ < 150000000 ? freq_ : 150000000) : 1);
    if (freq == cpuClockFrequency)
      return;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    cpuClockFrequency = freq;
    updateTimingParameters(ted->getIsNTSCMode());
  }

  void Plus4VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq = (freq_ > 7159090 ? (freq_ < 35468950 ? freq_ : 35468950)
                                      : 7159090);
    if (freq == tedInputClockFrequency)
      return;
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    tedInputClockFrequency = freq;
    updateTimingParameters(ted->getIsNTSCMode());
  }

  void Plus4VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo)
      ted->setKeyState(keyCode, isPressed);
    if (isRecordingDemo) {
      if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0) {
        stopDemoRecording(false);
        return;
      }
      writeDemoTimeCnt(demoBuffer, demoTimeCnt);
      demoTimeCnt = 0U;
      demoBuffer.writeByte(isPressed ? 0x01 : 0x02);
      demoBuffer.writeByte(0x01);
      demoBuffer.writeByte(uint8_t(keyCode & 0x7F));
    }
  }

  void Plus4VM::setLightPenPosition(int xPos, int yPos)
  {
    if (xPos < 0 || xPos > 65535 || yPos < 0 || yPos > 65535) {
      // disable light pen
      if (lightPenCycleCounter) {
        setKeyboardState(86, false);
        lightPenCycleCounter = 0;
      }
      if (lightPenPositionX >= 0 || lightPenPositionY >= 0) {
        ted->setCallback(&lightPenCallback, this, 0);
        lightPenPositionX = -1;
        lightPenPositionY = -1;
      }
    }
    else {
      if (lightPenPositionX < 0 || lightPenPositionY < 0)
        ted->setCallback(&lightPenCallback, this, 3);
      lightPenPositionX = ((xPos * 384) >> 16) + 424;
      if (lightPenPositionX >= 456)
        lightPenPositionX -= 456;
      if (!ted->getIsNTSCMode()) {
        lightPenPositionY = ((yPos * 288) >> 16) + 275;
        if (lightPenPositionY >= 312)
          lightPenPositionY -= 312;
      }
      else {
        lightPenPositionY = ((yPos * 288) >> 16) + 225;
        if (lightPenPositionY >= 262)
          lightPenPositionY -= 262;
      }
    }
  }

  void Plus4VM::setEnablePrinter(bool isEnabled)
  {
    if (isEnabled) {
      if (!printer_) {
        printer_ = new VC1526(4);       // TODO: allow setting device number ?
        printer_->setROMImage(printerROM_1526);
        printer_->setEnable1525Mode(printer1525Mode);
        printer_->setFormFeedOn(printerFormFeedOn);
        printerTimeRemaining = 0;
        ted->setCallback(&printerCallback, this, 1);
        printer_->setBreakPointCallback(breakPointCallback,
                                        breakPointCallbackUserData);
        M7501   *p = printer_->getCPU();
        if (p) {
          p->setBreakPointPriorityThreshold(
              ted->getBreakPointPriorityThreshold());
          p->setBreakOnInvalidOpcode(ted->getIsBreakOnInvalidOpcode());
        }
        printer_->setNoBreakOnDataRead(noBreakOnDataRead);
      }
    }
    else if (printer_) {
      printerOutputChangedFlag = true;
      ted->setCallback(&printerCallback, this, 0);
      ted->serialPort.removeDevice(4);
      delete printer_;
      printer_ = (VC1526 *) 0;
    }
  }

  void Plus4VM::getPrinterOutput(const uint8_t*& buf_, int& w_, int& h_) const
  {
    if (printer_) {
      buf_ = printer_->getPageData();
      w_ = printer_->getPageWidth();
      h_ = printer_->getPageHeight();
    }
    else {
      buf_ = (uint8_t *) 0;
      w_ = 0;
      h_ = 0;
    }
  }

  void Plus4VM::clearPrinterOutput()
  {
    if (printer_)
      printer_->clearPage();
  }

  uint8_t Plus4VM::getPrinterLEDState() const
  {
    if (printer_)
      return printer_->getLEDState();
    return 0x00;
  }

  void Plus4VM::getPrinterHeadPosition(int& xPos, int& yPos)
  {
    if (printer_) {
      printer_->getHeadPosition(xPos, yPos);
      return;
    }
    xPos = -1;
    yPos = -1;
  }

  bool Plus4VM::getIsPrinterOutputChanged() const
  {
    if (printer_)
      return printer_->getIsOutputChanged();
    return printerOutputChangedFlag;
  }

  void Plus4VM::clearPrinterOutputChangedFlag()
  {
    if (printer_)
      printer_->clearOutputChangedFlag();
    printerOutputChangedFlag = false;
  }

  void Plus4VM::setPrinter1525Mode(bool isEnabled)
  {
    printer1525Mode = isEnabled;
    if (printer_)
      printer_->setEnable1525Mode(isEnabled);
  }

  void Plus4VM::setPrinterFormFeedOn(bool isEnabled)
  {
    printerFormFeedOn = isEnabled;
    if (printer_)
      printer_->setFormFeedOn(isEnabled);
  }

  void Plus4VM::getVMStatus(VMStatus& vmStatus_)
  {
    vmStatus_.tapeReadOnly = getIsTapeReadOnly();
    vmStatus_.tapePosition = getTapePosition();
    vmStatus_.tapeLength = getTapeLength();
    vmStatus_.tapeSampleRate = getTapeSampleRate();
    vmStatus_.tapeSampleSize = getTapeSampleSize();
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
        n |= uint32_t(floppyDrives[i].floppyDrive->getLEDState() & 0xFF);
    }
    vmStatus_.floppyDriveLEDState = n;
    if (!printer_) {
      vmStatus_.printerHeadPositionX = -1;
      vmStatus_.printerHeadPositionY = -1;
      vmStatus_.printerOutputChanged = printerOutputChangedFlag;
      vmStatus_.printerLEDState = 0x00;
    }
    else {
      printer_->getHeadPosition(vmStatus_.printerHeadPositionX,
                                vmStatus_.printerHeadPositionY);
      vmStatus_.printerOutputChanged = printer_->getIsOutputChanged();
      vmStatus_.printerLEDState = printer_->getLEDState();
    }
    vmStatus_.isPlayingDemo = isPlayingDemo;
    if (demoFile != (Plus4Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    vmStatus_.isRecordingDemo = isRecordingDemo;
  }

  void Plus4VM::setDiskImageFile(int n, const std::string& fileName_,
                                 int driveType)
  {
    if (n < 0 || n > 3)
      throw Plus4Emu::Exception("invalid floppy drive number");
    if (fileName_.length() == 0) {
      // remove disk
      if (floppyDrives[n].floppyDrive)
        floppyDrives[n].floppyDrive->setDiskImageFile(fileName_);
    }
    else {
      // insert or replace disk
      bool    isD64 = false;
      {
        // find out file type
        std::FILE *f = std::fopen(fileName_.c_str(), "rb");
        if (f) {
          if (std::fseek(f, 0L, SEEK_END) >= 0) {
            long    fSize = std::ftell(f);
            isD64 = (fSize == 174848L || fSize == 175531L || fSize == 196608L);
          }
          std::fclose(f);
        }
        else
          throw Plus4Emu::Exception("error opening disk image file");
      }
      int     newDriveType = (isD64 ? driveType : 4);
      if (newDriveType == 1) {
        if (n >= 2) {
          throw Plus4Emu::Exception("1551 emulation is only allowed "
                                    "for unit 8 and unit 9");
        }
      }
      else if (!(newDriveType == 0 || newDriveType == 4))
        throw Plus4Emu::Exception("invalid floppy drive type");
      if (floppyDrives[n].floppyDrive) {
        int     oldDriveType = -1;
        if (typeid(*(floppyDrives[n].floppyDrive)) == typeid(VC1541))
          oldDriveType = 0;
        else if (typeid(*(floppyDrives[n].floppyDrive)) == typeid(VC1551))
          oldDriveType = 1;
        else if (typeid(*(floppyDrives[n].floppyDrive)) == typeid(VC1581))
          oldDriveType = 4;
        if (newDriveType != oldDriveType) {
          // need to change drive type
          delete floppyDrives[n].floppyDrive;
          floppyDrives[n].floppyDrive = (FloppyDrive *) 0;
          ted->serialPort.removeDevice(n + 8);
          removeFloppyCallback(n);
        }
      }
      if (!floppyDrives[n].floppyDrive) {
        switch (newDriveType) {
        case 0:
          floppyDrives[n].floppyDrive = new VC1541(n + 8);
          floppyDrives[n].floppyDrive->setROMImage(2, floppyROM_1541);
          break;
        case 1:
          floppyDrives[n].floppyDrive = new VC1551(n + 8);
          floppyDrives[n].floppyDrive->setROMImage(3, floppyROM_1551);
          break;
        case 4:
          floppyDrives[n].floppyDrive = new VC1581(n + 8);
          floppyDrives[n].floppyDrive->setROMImage(0, floppyROM_1581_0);
          floppyDrives[n].floppyDrive->setROMImage(1, floppyROM_1581_1);
          break;
        }
        addFloppyCallback(n);
        floppyDrives[n].floppyDrive->setBreakPointCallback(
            breakPointCallback, breakPointCallbackUserData);
        M7501   *p = floppyDrives[n].floppyDrive->getCPU();
        if (p) {
          p->setBreakPointPriorityThreshold(
              ted->getBreakPointPriorityThreshold());
          p->setBreakOnInvalidOpcode(ted->getIsBreakOnInvalidOpcode());
        }
        floppyDrives[n].floppyDrive->setNoBreakOnDataRead(noBreakOnDataRead);
      }
      floppyDrives[n].floppyDrive->setDiskImageFile(fileName_);
    }
  }

  uint32_t Plus4VM::getFloppyDriveLEDState() const
  {
    uint32_t  n = 0U;
    for (int i = 3; i >= 0; i--) {
      n = n << 8;
      if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
        n |= uint32_t(floppyDrives[i].floppyDrive->getLEDState() & 0xFF);
    }
    return n;
  }

  void Plus4VM::setFloppyDriveHighAccuracy(bool isEnabled)
  {
    if (isEnabled == is1541HighAccuracy)
      return;
    is1541HighAccuracy = isEnabled;
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0) {
        if (typeid(*(floppyDrives[i].floppyDrive)) == typeid(VC1541)) {
          removeFloppyCallback(i);
          addFloppyCallback(i);
        }
      }
    }
  }

  void Plus4VM::setTapeFileName(const std::string& fileName)
  {
    Plus4Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(ted->getTapeMotorState());
    if (haveTape()) {
      int64_t tmp = getTapeSampleRate();
      tapeTimesliceLength = ((int64_t(1000000) << 32) + (tmp >> 1)) / tmp;
    }
    else
      tapeTimesliceLength = 0;
    tapeTimeRemaining = 0;
  }

  void Plus4VM::setTapeFeedbackLevel(int n)
  {
    n = (n >= -10 ? (n <= 10 ? n : 10) : -10);
    if (n > 0) {
      tapeFeedbackLevel =
          int32_t(std::pow(2.0, double(n) * 0.5) * 11264.0 + 0.5);
    }
    else if (n < 0) {
      tapeFeedbackLevel =
          int32_t(std::pow(2.0, double(n) * -0.5) * -11264.0 - 0.5);
    }
    else
      tapeFeedbackLevel = 0;
  }

  void Plus4VM::tapePlay()
  {
    Plus4Emu::VirtualMachine::tapePlay();
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!(isRecordingDemo || isPlayingDemo) &&
                            getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeRecord()
  {
    Plus4Emu::VirtualMachine::tapeRecord();
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(!(isRecordingDemo || isPlayingDemo) &&
                            getTapeButtonState() != 0);
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Plus4VM::tapeStop()
  {
    Plus4Emu::VirtualMachine::tapeStop();
    ted->setTapeButtonState(false);
  }

  void Plus4VM::setDebugContext(int n)
  {
    currentDebugContext = (n >= 0 ? (n <= 5 ? n : 5) : 0);
    // disable single stepping in other debug contexts
    for (int i = 0; i < 6; i++) {
      if (i != currentDebugContext) {
        M7501   *p = (M7501 *) 0;
        if (i == 0) {
          p = ted;
        }
        else if (i <= 4) {
          if (floppyDrives[i - 1].floppyDrive != (FloppyDrive *) 0)
            p = floppyDrives[i - 1].floppyDrive->getCPU();
        }
        else if (printer_)
          p = printer_->getCPU();
        if (p)
          p->setSingleStepMode(0);
      }
    }
  }

  void Plus4VM::setBreakPoints(const Plus4Emu::BreakPointList& bpList)
  {
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
      const Plus4Emu::BreakPoint& bp = bpList.getBreakPoint(i);
      if (bp.type() == 4 && currentDebugContext != 0)
        throw Plus4Emu::Exception("video breakpoints can only be set "
                                  "for the main CPU");
    }
    M7501   *p = getDebugCPU();
    if (p) {
      for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
        const Plus4Emu::BreakPoint& bp = bpList.getBreakPoint(i);
        if (bp.type() != 4) {
          p->setBreakPoint(bp.type(), bp.addr(), bp.priority());
        }
        else {
          if (videoBreakPointCnt == 0) {
            if (!videoBreakPoints) {
              videoBreakPoints = new uint8_t[65536];
              for (size_t j = 0; j <= 0xFFFF; j++)
                videoBreakPoints[j] = 0;
            }
            ted->setCallback(&videoBreakPointCheckCallback, this, 3);
          }
          // correct video position for FF1E read delay
          uint16_t  addr = bp.addr();
          uint16_t  addrX = (addr & 0x7F) + 1;
          addr = addr & 0xFF80;
          if (addrX != 114)
            addr = addr | (addrX & 0x7F);
          videoBreakPoints[addr] = uint8_t(bp.priority() + 1);
          videoBreakPointCnt++;
        }
      }
    }
  }

  void Plus4VM::clearBreakPoints()
  {
    M7501   *p = getDebugCPU();
    if (p)
      p->clearBreakPoints();
    if (currentDebugContext == 0 && videoBreakPointCnt != 0) {
      ted->setCallback(&videoBreakPointCheckCallback, this, 0);
      videoBreakPointCnt = 0;
      delete[] videoBreakPoints;
      videoBreakPoints = (uint8_t *) 0;
    }
  }

  void Plus4VM::setBreakPointPriorityThreshold(int n)
  {
    ted->setBreakPointPriorityThreshold(n);
    for (int i = 0; i < 5; i++) {
      M7501   *p = (M7501 *) 0;
      if (i < 4) {
        if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
          p = floppyDrives[i].floppyDrive->getCPU();
      }
      else if (printer_)
        p = printer_->getCPU();
      if (p)
        p->setBreakPointPriorityThreshold(n);
    }
  }

  void Plus4VM::setNoBreakOnDataRead(bool n)
  {
    noBreakOnDataRead = n;
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
        floppyDrives[i].floppyDrive->setNoBreakOnDataRead(n);
    }
    if (printer_)
      printer_->setNoBreakOnDataRead(n);
  }

  void Plus4VM::setSingleStepMode(int mode_)
  {
    M7501   *p = getDebugCPU();
    if (p)
      p->setSingleStepMode(mode_);
  }

  void Plus4VM::setBreakOnInvalidOpcode(bool isEnabled)
  {
    ted->setBreakOnInvalidOpcode(isEnabled);
    for (int i = 0; i < 5; i++) {
      M7501   *p = (M7501 *) 0;
      if (i < 4) {
        if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
          p = floppyDrives[i].floppyDrive->getCPU();
      }
      else if (printer_)
        p = printer_->getCPU();
      if (p)
        p->setBreakOnInvalidOpcode(isEnabled);
    }
  }

  void Plus4VM::setBreakPointCallback(void (*breakPointCallback_)(
                                          void *userData,
                                          int debugContext_, int type,
                                          uint16_t addr, uint8_t value),
                                      void *userData_)
  {
    VirtualMachine::setBreakPointCallback(breakPointCallback_, userData_);
    for (int i = 0; i < 4; i++) {
      if (floppyDrives[i].floppyDrive != (FloppyDrive *) 0)
        floppyDrives[i].floppyDrive->setBreakPointCallback(breakPointCallback_,
                                                           userData_);
    }
    if (printer_)
      printer_->setBreakPointCallback(breakPointCallback_, userData_);
  }

  uint8_t Plus4VM::getMemoryPage(int n) const
  {
    if (currentDebugContext == 0)
      return ted->getMemoryPage(n);
    else if (currentDebugContext <= 4) {
      if (floppyDrives[currentDebugContext - 1].floppyDrive
          != (FloppyDrive *) 0) {
        // floppy drives are mapped to segments 60..6F
        return uint8_t((n & 3) | (((currentDebugContext - 1) & 3) << 2) | 0x60);
      }
    }
    else if (printer_) {
      // printer is mapped to segments 50..53
      return uint8_t(0x50 | (n & 3));
    }
    return uint8_t(0x7F);
  }

  uint8_t Plus4VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress) {
      if (currentDebugContext == 0) {
        return ted->readMemoryCPU(uint16_t(addr & 0xFFFFU));
      }
      else if (currentDebugContext <= 4) {
        const FloppyDrive *p =
            floppyDrives[currentDebugContext - 1].floppyDrive;
        if (p)
          return p->readMemoryDebug(uint16_t(addr & 0xFFFFU));
      }
      else if (printer_)
        return printer_->readMemoryDebug(uint16_t(addr & 0xFFFFU));
    }
    else {
      uint8_t segment = uint8_t((addr >> 14) & 0xFF);
      switch (segment) {
      case 0x0C:
        if (printerROM_1526)
          return printerROM_1526[addr & 0x1FFFU];
        break;
      case 0x10:
        if (floppyROM_1541)
          return floppyROM_1541[addr & 0x3FFFU];
        break;
      case 0x20:
        if (floppyROM_1551)
          return floppyROM_1551[addr & 0x3FFFU];
        break;
      case 0x30:
        if (floppyROM_1581_0)
          return floppyROM_1581_0[addr & 0x3FFFU];
        break;
      case 0x31:
        if (floppyROM_1581_1)
          return floppyROM_1581_1[addr & 0x3FFFU];
        break;
      case 0x50:
      case 0x51:
      case 0x52:
      case 0x53:
        if (printer_)
          return printer_->readMemoryDebug(uint16_t(addr & 0xFFFFU));
        break;
      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63:
      case 0x64:
      case 0x65:
      case 0x66:
      case 0x67:
      case 0x68:
      case 0x69:
      case 0x6A:
      case 0x6B:
      case 0x6C:
      case 0x6D:
      case 0x6E:
      case 0x6F:
        {
          const FloppyDrive *p = floppyDrives[(segment >> 2) & 3].floppyDrive;
          if (p)
            return p->readMemoryDebug(uint16_t(addr & 0xFFFFU));
        }
        break;
      default:
        return ted->readMemoryRaw(addr & uint32_t(0x003FFFFF));
      }
    }
    return uint8_t(0xFF);
  }

  void Plus4VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
  {
    if (isRecordingDemo || isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    if (isCPUAddress) {
      if (currentDebugContext == 0) {
        ted->writeMemoryCPU(uint16_t(addr & 0xFFFFU), value);
      }
      else if (currentDebugContext <= 4) {
        FloppyDrive *p = floppyDrives[currentDebugContext - 1].floppyDrive;
        if (p)
          p->writeMemoryDebug(uint16_t(addr & 0xFFFFU), value);
      }
      else if (printer_)
        printer_->writeMemoryDebug(uint16_t(addr & 0xFFFFU), value);
    }
    else {
      if (addr >= 0x00200000U) {
        ted->writeMemoryRaw(addr & uint32_t(0x003FFFFF), value);
      }
      else if (addr >= 0x00180000U && addr <= 0x001BFFFFU) {
        FloppyDrive *p = floppyDrives[(addr >> 16) & 3U].floppyDrive;
        if (p)
          p->writeMemoryDebug(uint16_t(addr & 0xFFFFU), value);
      }
      else if (addr >= 0x00140000U && addr <= 0x0014FFFFU) {
        if (printer_)
          printer_->writeMemoryDebug(uint16_t(addr & 0xFFFFU), value);
      }
    }
  }

  uint16_t Plus4VM::getProgramCounter() const
  {
    const M7501 *p = getDebugCPU();
    if (p) {
      M7501Registers  r;
      p->getRegisters(r);
      return r.reg_PC;
    }
    return uint16_t(0xFFFF);
  }

  uint16_t Plus4VM::getStackPointer() const
  {
    const M7501 *p = getDebugCPU();
    if (p) {
      M7501Registers  r;
      p->getRegisters(r);
      return (uint16_t(0x0100)
              | uint16_t((r.reg_SP + uint8_t(1)) & uint8_t(0xFF)));
    }
    return uint16_t(0xFFFF);
  }

  void Plus4VM::listCPURegisters(std::string& buf) const
  {
    char        tmpBuf[96];
    const M7501 *p = getDebugCPU();
    if (p) {
      M7501Registers  r;
      p->getRegisters(r);
      std::sprintf(&(tmpBuf[0]),
                   " PC  SR AC XR YR SP\n"
                   "%04X %02X %02X %02X %02X %02X",
                   (unsigned int) r.reg_PC, (unsigned int) r.reg_SR,
                   (unsigned int) r.reg_AC, (unsigned int) r.reg_XR,
                   (unsigned int) r.reg_YR, (unsigned int) r.reg_SP);
    }
    else {
      std::sprintf(&(tmpBuf[0]),
                   " PC  SR AC XR YR SP\n"
                   "FFFF FF FF FF FF FF");
    }
    buf = &(tmpBuf[0]);
  }

  uint32_t Plus4VM::disassembleInstruction(std::string& buf,
                                           uint32_t addr, bool isCPUAddress,
                                           int32_t offs) const
  {
    return M7501Disassembler::disassembleInstruction(buf, (*this),
                                                     addr, isCPUAddress, offs);
  }

  void Plus4VM::setCPURegisters(const M7501Registers& r)
  {
    M7501   *p = getDebugCPU();
    if (p)
      p->setRegisters(r);
  }

  void Plus4VM::getCPURegisters(M7501Registers& r) const
  {
    const M7501 *p = getDebugCPU();
    if (p) {
      p->getRegisters(r);
    }
    else {
      r.reg_PC = 0xFFFF;
      r.reg_SR = 0xFF;
      r.reg_AC = 0xFF;
      r.reg_XR = 0xFF;
      r.reg_YR = 0xFF;
      r.reg_SP = 0xFF;
    }
  }

  void Plus4VM::saveState(Plus4Emu::File& f)
  {
    ted->saveState(f);
    sid_->saveState(f);
    {
      Plus4Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      buf.writeUInt32(uint32_t(cpuClockFrequency));
      buf.writeUInt32(uint32_t(tedInputClockFrequency));
      buf.writeUInt32(uint32_t(soundClockFrequency));
      buf.writeBoolean(sidEnabled);
      f.addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_P4VM_STATE, buf);
    }
  }

  void Plus4VM::saveMachineConfiguration(Plus4Emu::File& f)
  {
    Plus4Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(cpuClockFrequency));
    buf.writeUInt32(uint32_t(tedInputClockFrequency));
    buf.writeUInt32(uint32_t(soundClockFrequency));
    f.addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_P4VM_CONFIG, buf);
  }

  void Plus4VM::saveProgram(Plus4Emu::File& f)
  {
    ted->saveProgram(f);
  }

  void Plus4VM::saveProgram(const char *fileName)
  {
    ted->saveProgram(fileName);
  }

  void Plus4VM::loadProgram(const char *fileName)
  {
    ted->loadProgram(fileName);
  }

  void Plus4VM::recordDemo(Plus4Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ted->setTapeMotorState(false);
    ted->setTapeInput(false);
    setTapeMotorState(false);
    ted->serialPort.removeDevices(0xFFFE);
    stopDemo();
    for (int i = 0; i < 128; i++)
      ted->setKeyState(i, false);
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00010200); // version 1.2.0
    demoFile = &f;
    isRecordingDemo = true;
    ted->setCallback(&demoRecordCallback, this, 1);
    demoTimeCnt = 0U;
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(false);
  }

  void Plus4VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Plus4VM::getIsRecordingDemo()
  {
    if (demoFile != (Plus4Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Plus4VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Plus4VM::loadState(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Plus4Emu::Exception("incompatible plus4 snapshot format");
    }
    ted->setTapeMotorState(false);
    setTapeMotorState(false);
    stopDemo();
    snapshotLoadFlag = true;
    resetFloppyDrives(0x0F, true);
    try {
      uint32_t  tmpCPUClockFrequency = buf.readUInt32();
      uint32_t  tmpTEDInputClockFrequency = buf.readUInt32();
      uint32_t  tmpSoundClockFrequency = buf.readUInt32();
      (void) tmpCPUClockFrequency;
      (void) tmpTEDInputClockFrequency;
      (void) tmpSoundClockFrequency;
      sidEnabled = buf.readBoolean();
      ted->setCallback(&sidCallback, this, (sidEnabled ? 1 : 0));
      if (buf.getPosition() != buf.getDataSize())
        throw Plus4Emu::Exception("trailing garbage at end of "
                                  "plus4 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Plus4VM::loadMachineConfiguration(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Plus4Emu::Exception("incompatible plus4 "
                                "machine configuration format");
    }
    try {
      uint32_t  tmpCPUClockFrequency = buf.readUInt32();
      uint32_t  tmpTEDInputClockFrequency = buf.readUInt32();
      uint32_t  tmpSoundClockFrequency = buf.readUInt32();
      (void) tmpSoundClockFrequency;
      setCPUFrequency(tmpCPUClockFrequency);
      setVideoFrequency(tmpTEDInputClockFrequency);
      if (buf.getPosition() != buf.getDataSize())
        throw Plus4Emu::Exception("trailing garbage at end of "
                                  "plus4 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Plus4VM::loadDemo(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00010200) {
      buf.setPosition(buf.getDataSize());
      throw Plus4Emu::Exception("incompatible plus4 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    ted->setTapeMotorState(false);
    ted->setTapeInput(false);
    setTapeMotorState(false);
    ted->serialPort.removeDevices(0xFFFE);
    stopDemo();
    for (int i = 0; i < 128; i++)
      ted->setKeyState(i, false);
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    ted->setCallback(&demoPlayCallback, this, 1);
    // tape button state sensing is disabled while recording or playing demo
    ted->setTapeButtonState(false);
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Plus4VMConfig : public Plus4Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4VMConfig(Plus4VM& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4VMConfig()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_P4VM_CONFIG;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Plus4VMSnapshot : public Plus4Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4VMSnapshot(Plus4VM& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4VMSnapshot()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_P4VM_STATE;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_Plus4DemoStream : public Plus4Emu::File::ChunkTypeHandler {
   private:
    Plus4VM&  ref;
   public:
    ChunkType_Plus4DemoStream(Plus4VM& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Plus4DemoStream()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_PLUS4_DEMO;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Plus4VM::registerChunkTypes(Plus4Emu::File& f)
  {
    ChunkType_Plus4VMConfig   *p1 = (ChunkType_Plus4VMConfig *) 0;
    ChunkType_Plus4VMSnapshot *p2 = (ChunkType_Plus4VMSnapshot *) 0;
    ChunkType_Plus4DemoStream *p3 = (ChunkType_Plus4DemoStream *) 0;

    try {
      p1 = new ChunkType_Plus4VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_Plus4VMConfig *) 0;
      p2 = new ChunkType_Plus4VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Plus4VMSnapshot *) 0;
      p3 = new ChunkType_Plus4DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_Plus4DemoStream *) 0;
    }
    catch (...) {
      if (p1)
        delete p1;
      if (p2)
        delete p2;
      if (p3)
        delete p3;
      throw;
    }
    ted->registerChunkTypes(f);
    sid_->registerChunkType(f);
  }

}       // namespace Plus4

