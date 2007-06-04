
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
#include "vc1551.hpp"

static const int d64TrackOffsetTable[41] = {
      -1,      0,   5376,  10752,  16128,  21504,  26880,  32256,
   37632,  43008,  48384,  53760,  59136,  64512,  69888,  75264,
   80640,  86016,  91392,  96256, 101120, 105984, 110848, 115712,
  120576, 125440, 130048, 134656, 139264, 143872, 148480, 153088,
  157440, 161792, 166144, 170496, 174848, 179200, 183552, 187904,
  192256
};

static const int sectorsPerTrackTable[41] = {
   0, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 19, 19, 19, 19, 19, 19,
  19, 18, 18, 18, 18, 18, 18, 17,
  17, 17, 17, 17, 17, 17, 17, 17,
  17
};

static const int trackSizeTable[41] = {
  7692, 7692, 7692, 7692, 7692, 7692, 7692, 7692,
  7692, 7692, 7692, 7692, 7692, 7692, 7692, 7692,
  7692, 7692, 7143, 7143, 7143, 7143, 7143, 7143,
  7143, 6667, 6667, 6667, 6667, 6667, 6667, 6250,
  6250, 6250, 6250, 6250, 6250, 6250, 6250, 6250,
  6250
};

// number of bits per 1 MHz cycle, multiplied by 65536
static const int trackSpeedTable[41] = {
  0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5,
  0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5,
  0x4EC5, 0x4EC5, 0x4925, 0x4925, 0x4925, 0x4925, 0x4925, 0x4925,
  0x4925, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4000,
  0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
  0x4000
};

static const uint8_t gcrEncodeTable[16] = {
  0x0A, 0x0B, 0x12, 0x13, 0x0E, 0x0F, 0x16, 0x17,
  0x09, 0x19, 0x1A, 0x1B, 0x0D, 0x1D, 0x1E, 0x15
};

static const uint8_t gcrDecodeTable[32] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x08, 0x00, 0x01, 0xFF, 0x0C, 0x04, 0x05,
  0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x0F, 0x06, 0x07,
  0xFF, 0x09, 0x0A, 0x0B, 0xFF, 0x0D, 0x0E, 0xFF
};

static void defaultBreakPointCallback(void *userData,
                                      int debugContext_,
                                      bool isIO, bool isWrite,
                                      uint16_t addr, uint8_t value)
{
  (void) userData;
  (void) debugContext_;
  (void) isIO;
  (void) isWrite;
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

  void VC1551::M7501_::breakPointCallback(bool isWrite,
                                          uint16_t addr, uint8_t value)
  {
    if (vc1551.noBreakOnDataRead && !isWrite) {
      if (reg_PC != addr)
        return;
    }
    vc1551.breakPointCallback(vc1551.breakPointCallbackUserData,
                              (vc1551.deviceNumber & 3) + 1,
                              false, isWrite, addr, value);
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
    vc1551.dataBusState = vc1551.tia6523Registers[addr & 0x0007];
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
    vc1551.dataBusState = value & 0xFF;
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
    addr = addr & 0x0007;
    switch (addr) {
    case 0:
    case 1:
      {
        uint8_t mask_ = vc1551.tia6523Registers[addr + 3];
        vc1551.tia6523Registers[addr] &= (mask_ ^ uint8_t(0xFF));
        vc1551.tia6523Registers[addr] |= (value & mask_);
      }
      break;
    case 2:
      {
        uint8_t mask_ = vc1551.tia6523Registers[addr + 3] & uint8_t(0x1F);
        vc1551.tia6523Registers[addr] &= (mask_ ^ uint8_t(0xFF));
        vc1551.tia6523Registers[addr] |= (value & mask_);
      }
      break;
    case 3:
    case 4:
    case 5:
      vc1551.tia6523Registers[addr] = vc1551.dataBusState;
      break;
    }
  }

  // --------------------------------------------------------------------------

  void VC1551::gcrEncodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf)
  {
    uint8_t bitBuf = 0;
    uint8_t bitCnt = 0;
    for (int i = 0; i < 8; i++) {
      uint8_t n = inBuf[i >> 1];
      n = ((i & 1) == 0 ? (n >> 4) : n) & 0x0F;
      n = gcrEncodeTable[n];
      for (int j = 0; j < 5; j++) {
        bitBuf = (bitBuf << 1) | ((n & 0x10) >> 4);
        n = n << 1;
        if (++bitCnt >= 8) {
          *(outBuf++) = bitBuf;
          bitBuf = 0;
          bitCnt = 0;
        }
      }
    }
  }

  bool VC1551::gcrDecodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf)
  {
    bool    retval = true;
    uint8_t bitBuf = 0;
    uint8_t bitCnt = 0;
    for (int i = 0; i < 8; i++) {
      uint8_t n = 0;
      for (int j = 0; j < 5; j++) {
        if (bitCnt == 0) {
          bitBuf = *(inBuf++);
          bitCnt = 8;
        }
        bitCnt--;
        n = (n << 1) | ((bitBuf & 0x80) >> 7);
        bitBuf = bitBuf << 1;
      }
      n = gcrDecodeTable[n];
      if (n >= 0x80) {          // return false on invalid GCR data
        n = 0x00;
        retval = false;
      }
      if (!(i & 1))
        outBuf[i >> 1] = n << 4;
      else
        outBuf[i >> 1] = outBuf[i >> 1] | n;
    }
    return retval;
  }

  void VC1551::gcrEncodeTrack(int trackNum, int nSectors, int nBytes)
  {
    int     readPos = 0;
    int     writePos = 0;
    uint8_t tmpBuf1[8];
    uint8_t tmpBuf2[5];
    for (int i = 0; i < nSectors; i++) {
      uint8_t crcValue;
      // write header sync
      for (int j = 0; j < 5; j++)
        trackBuffer_GCR[writePos++] = 0xFF;
      // write header
      tmpBuf1[0] = 0x08;                // block ID
      tmpBuf1[2] = uint8_t(i);          // sector (0 to 20)
      tmpBuf1[3] = uint8_t(trackNum);   // track (1 to 35)
      tmpBuf1[4] = idCharacter2;        // format ID
      tmpBuf1[5] = idCharacter1;        // -"-
      tmpBuf1[6] = 0x0F;                // padding
      tmpBuf1[7] = 0x0F;                // -"-
      crcValue = 0;
      for (int j = 2; j < 6; j++)
        crcValue = crcValue ^ tmpBuf1[j];
      tmpBuf1[1] = crcValue;            // checksum
      gcrEncodeFourBytes(&(tmpBuf2[0]), &(tmpBuf1[0]));
      for (int j = 0; j < 5; j++)
        trackBuffer_GCR[writePos++] = tmpBuf2[j];
      gcrEncodeFourBytes(&(tmpBuf2[0]), &(tmpBuf1[4]));
      for (int j = 0; j < 5; j++)
        trackBuffer_GCR[writePos++] = tmpBuf2[j];
      // write gap
      for (int j = 0; j < 9; j++)
        trackBuffer_GCR[writePos++] = 0x55;
      // write sector data sync
      for (int j = 0; j < 5; j++)
        trackBuffer_GCR[writePos++] = 0xFF;
      int     bufPos = 0;
      tmpBuf1[bufPos++] = 0x07;         // block ID
      // write sector data
      crcValue = 0;
      for (int j = 0; j < 256; j++) {
        uint8_t tmp = trackBuffer_D64[readPos++];
        tmpBuf1[bufPos++] = tmp;
        crcValue = crcValue ^ tmp;
        if (bufPos >= 4) {
          bufPos = 0;
          gcrEncodeFourBytes(&(tmpBuf2[0]), &(tmpBuf1[0]));
          for (int k = 0; k < 5; k++)
            trackBuffer_GCR[writePos++] = tmpBuf2[k];
        }
      }
      tmpBuf1[1] = crcValue;            // checksum
      tmpBuf1[2] = 0x00;                // padding
      tmpBuf1[3] = 0x00;                // -"-
      gcrEncodeFourBytes(&(tmpBuf2[0]), &(tmpBuf1[0]));
      for (int j = 0; j < 5; j++)
        trackBuffer_GCR[writePos++] = tmpBuf2[j];
      // write gap
      int   j = 9;
      if (i & 1) {
        switch (nSectors) {
        case 19:
          j = 19;
          break;
        case 18:
          j = 13;
          break;
        case 17:
          j = 10;
          break;
        }
      }
      do {
        trackBuffer_GCR[writePos++] = 0x55;
      } while (--j);
    }
    // pad track data to requested length
    for ( ; writePos < nBytes; writePos++)
      trackBuffer_GCR[writePos] = 0x55;
  }

  int VC1551::gcrDecodeTrack(int trackNum, int nSectors, int nBytes)
  {
    int     sectorsDecoded = 0;
    int     readPos = 0;
    int     firstSyncPos = -1;
    // find first header sync
    while (readPos <= (nBytes - 4)) {
      if (trackBuffer_GCR[readPos] == 0xFF) {
        if (trackBuffer_GCR[readPos + 1] == 0xFF) {
          if (trackBuffer_GCR[readPos + 2] == 0x52) {
            if ((trackBuffer_GCR[readPos + 3] & 0xC0) == 0x40) {
              firstSyncPos = readPos;
              break;
            }
          }
        }
      }
      readPos++;
    }
    if (firstSyncPos < 0 || nBytes <= 0)
      return 0;
    // process track data
    readPos = firstSyncPos;
    int     syncCnt = 0;
    // 0: searching for header sync, 1: reading header,
    // 2: searching for sector data sync, 3: reading sector data
    int     currentMode = 0;
    int     gcrBytesToDecode = 0;
    int     gcrByteCnt = 0;
    int     currentSector = 0;
    uint8_t tmpBuf1[325];
    uint8_t tmpBuf2[260];
    do {
      uint8_t c = trackBuffer_GCR[readPos];
      switch (currentMode) {
      case 0:                           // search for header sync
        if (c == 0xFF)
          syncCnt++;
        else {
          if (syncCnt >= 2) {
            currentMode = 1;
            gcrBytesToDecode = 10;
            gcrByteCnt = 0;
            readPos = (readPos != 0 ? readPos : nBytes) - 1;
          }
          syncCnt = 0;
        }
        break;
      case 1:                           // read header
        if (gcrByteCnt < gcrBytesToDecode) {
          tmpBuf1[gcrByteCnt++] = c;
        }
        else {
          int     j = 0;
          bool    errorFlag = false;
          for (int i = 0; i < gcrBytesToDecode; i += 5) {
            if (!gcrDecodeFourBytes(&(tmpBuf2[j]), &(tmpBuf1[i])))
              errorFlag = true;
            j += 4;
          }
          uint8_t crcValue = 0;
          for (int i = 1; i < 6; i++)
            crcValue = crcValue ^ tmpBuf2[i];
          if (errorFlag || tmpBuf2[0] != 0x08 || int(tmpBuf2[3]) != trackNum ||
              int(tmpBuf2[2]) >= nSectors || crcValue != 0x00) {
            currentMode = 0;
          }
          else {
            currentSector = int(tmpBuf2[2]);
            currentMode = 2;
            idCharacter2 = tmpBuf2[4];
            idCharacter1 = tmpBuf2[5];
          }
        }
        break;
      case 2:                           // search for sector data sync
        if (c == 0xFF)
          syncCnt++;
        else {
          if (syncCnt >= 2) {
            currentMode = 3;
            gcrBytesToDecode = 325;
            gcrByteCnt = 0;
            readPos = (readPos != 0 ? readPos : nBytes) - 1;
          }
          syncCnt = 0;
        }
        break;
      case 3:                           // read sector data
        if (gcrByteCnt < gcrBytesToDecode) {
          tmpBuf1[gcrByteCnt++] = c;
        }
        else {
          int     j = 0;
          bool    errorFlag = false;
          for (int i = 0; i < gcrBytesToDecode; i += 5) {
            if (!gcrDecodeFourBytes(&(tmpBuf2[j]), &(tmpBuf1[i])))
              errorFlag = true;
            j += 4;
          }
          uint8_t crcValue = 0;
          for (int i = 1; i < 258; i++)
            crcValue = crcValue ^ tmpBuf2[i];
          if (!(errorFlag || tmpBuf2[0] != 0x07 || crcValue != 0x00)) {
            j = currentSector * 256;
            for (int i = 0; i < 256; i++)
              trackBuffer_D64[j + i] = tmpBuf2[i + 1];
            sectorsDecoded++;
          }
          currentSector = 0;
          currentMode = 0;
        }
        break;
      }
      if (++readPos >= nBytes)
        readPos = 0;
    } while (readPos != firstSyncPos);
    // return the number of sectors successfully decoded
    return sectorsDecoded;
  }

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

  bool VC1551::readTrack(int trackNum)
  {
    bool    retval = true;
    if (trackNum < 0)
      trackNum = currentTrack;
    for (int i = 0; i < trackSizeTable[trackNum]; i++)
      trackBuffer_GCR[i] = 0x00;
    if (trackNum >= 1 && trackNum <= nTracks) {
      retval = false;
      if (std::fseek(imageFile, long(d64TrackOffsetTable[trackNum]),
                     SEEK_SET) >= 0) {
        int     nSectors = sectorsPerTrackTable[trackNum];
        if (std::fread(&(trackBuffer_D64[0]), sizeof(uint8_t),
                       (size_t(nSectors) * 256), imageFile)
            == (size_t(nSectors) * 256)) {
          gcrEncodeTrack(trackNum, nSectors, trackSizeTable[trackNum]);
          retval = true;
        }
      }
    }
    // return true on success
    return retval;
  }

  bool VC1551::flushTrack(int trackNum)
  {
    bool    retval = true;
    if (trackNum < 0)
      trackNum = currentTrack;
    if (trackDirtyFlag && !writeProtectFlag &&
        (trackNum >= 1 && trackNum <= nTracks)) {
      int     nSectors = sectorsPerTrackTable[trackNum];
      if (gcrDecodeTrack(trackNum, nSectors, trackSizeTable[trackNum]) > 0) {
        retval = false;
        if (std::fseek(imageFile, long(d64TrackOffsetTable[trackNum]),
                       SEEK_SET) >= 0) {
          if (std::fwrite(&(trackBuffer_D64[0]), sizeof(uint8_t),
                          (size_t(nSectors) * 256), imageFile)
              == (size_t(nSectors) * 256)) {
            if (std::fflush(imageFile) == 0)
              retval = true;
          }
        }
      }
    }
    trackDirtyFlag = false;
    // return true on success
    return retval;
  }

  bool VC1551::setCurrentTrack(int trackNum)
  {
    bool    retval = true;
    trackNum = (trackNum >= 1 ? (trackNum <= 40 ? trackNum : 40) : 1);
    currentTrackFrac = 0;
    if (trackNum != currentTrack) {
      // recalculate head position
      headPosition = (headPosition * trackSizeTable[trackNum])
                     / trackSizeTable[currentTrack];
      // write old track to disk if it has been changed
      if (!flushTrack(currentTrack))
        retval = false;
      // read new track from disk
      currentTrack = trackNum;
      if (!readTrack(currentTrack))
        retval = false;
    }
    return retval;
  }

  // --------------------------------------------------------------------------

  VC1551::VC1551(int driveNum_)
    : FloppyDrive(driveNum_),
      cpu(*this),
      memory_rom((uint8_t *) 0),
      deviceNumber(uint8_t(driveNum_)),
      diskID(0x00),
      dataBusState(0x00),
      writeProtectFlag(false),
      trackDirtyFlag(false),
      headLoadedFlag(false),
      prvByteWasFF(false),
      syncFlag(false),
      motorUpdateCnt(0),
      shiftRegisterBitCnt(0),
      shiftRegisterBitCntFrac(0x0000),
      interruptTimer(8325),
      headPosition(0),
      currentTrack(40),
      currentTrackFrac(0),
      steppingDirection(0),
      currentTrackStepperMotorPhase(0),
      spindleMotorSpeed(0),
      nTracks(0),
      diskChangeCnt(15625),
      idCharacter1(0x41),
      idCharacter2(0x41),
      imageFile((std::FILE *) 0),
      breakPointCallback(&defaultBreakPointCallback),
      breakPointCallbackUserData((void *) 0),
      noBreakOnDataRead(false)
  {
    for (int i = 0; i < 8; i++)
      tia6523Registers[i] = 0xFF;
    // clear RAM and track buffers
    for (int i = 0; i < 2048; i++)
      memory_ram[i] = 0x00;
    for (int i = 0; i < 8192; i++)
      trackBuffer_GCR[i] = 0x00;
    for (int i = 0; i < 5376; i++)
      trackBuffer_D64[i] = 0x00;
    this->reset();
  }

  VC1551::~VC1551()
  {
    if (imageFile) {
      (void) flushTrack();
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
      nTracks = 0;
    }
  }

  void VC1551::setROMImage(int n, const uint8_t *romData_)
  {
    if (n == 3)
      memory_rom = romData_;
  }

  void VC1551::setDiskImageFile(const std::string& fileName_)
  {
    if (imageFile) {
      (void) flushTrack();              // FIXME: should report errors ?
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
      nTracks = 0;
    }
    writeProtectFlag = false;
    headLoadedFlag = false;
    prvByteWasFF = false;
    syncFlag = false;
    spindleMotorSpeed = 0;
    diskChangeCnt = 15625;
    (void) setCurrentTrack(18);         // FIXME: should report errors ?
    currentTrackStepperMotorPhase = 0;
    memory_ram[0x0001] &= uint8_t(0xEF);
    if (fileName_.length() > 0) {
      bool    isReadOnly = false;
      imageFile = std::fopen(fileName_.c_str(), "r+b");
      if (!imageFile) {
        imageFile = std::fopen(fileName_.c_str(), "rb");
        isReadOnly = true;
      }
      if (!imageFile)
        throw Plus4Emu::Exception("error opening disk image file");
      if (std::fseek(imageFile, 0L, SEEK_END) < 0) {
        std::fclose(imageFile);
        imageFile = (std::FILE *) 0;
        throw Plus4Emu::Exception("error seeking to end of disk image file");
      }
      long    fSize = std::ftell(imageFile);
      if (fSize != 174848L && fSize != 175531L && fSize != 196608L) {
        std::fclose(imageFile);
        imageFile = (std::FILE *) 0;
        throw Plus4Emu::Exception("disk image file has invalid length "
                                  "- must be 174848, 175531, or 196608 bytes");
      }
      std::fseek(imageFile, 0L, SEEK_SET);
      writeProtectFlag = isReadOnly;
      nTracks = (fSize != 196608L ? 35 : 40);
      diskID = (diskID + 1) & 0xFF;
      if (((diskID >> 4) + 0x41) == idCharacter1 &&
          ((diskID & 0x0F) + 0x41) == idCharacter2)
        diskID = (diskID + 1) & 0xFF;   // make sure that the disk ID changes
      idCharacter1 = (diskID >> 4) + 0x41;
      idCharacter2 = (diskID & 0x0F) + 0x41;
      currentTrack = 40;
      (void) setCurrentTrack(18);       // FIXME: should report errors ?
      currentTrackStepperMotorPhase = 0;
    }
    // invert write protect sense input for 0.25 seconds so that the DOS can
    // detect the disk change
    memory_ram[0x0001] = uint8_t((memory_ram[0x0001] & 0xEF)
                                 | (writeProtectFlag ? 0x10 : 0x00));
  }

  bool VC1551::haveDisk() const
  {
    return (imageFile != (std::FILE *) 0);
  }

  void VC1551::runOneCycle(SerialBus& serialBus_)
  {
    (void) serialBus_;
    if (--interruptTimer < 0) {
      if (interruptTimer <= -7)
        interruptTimer = 8325;
      cpu.interruptRequest();
    }
    cpu.run(2);
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
        if (tia6523Registers[2] & 0x10) {
          // read mode
          uint8_t readByte = 0x00;
          if (headLoadedFlag) {
            readByte = trackBuffer_GCR[headPosition];
            if (readByte == 0xFF)
              syncFlag = prvByteWasFF;
          }
          prvByteWasFF = (readByte == 0xFF);
          tia6523Registers[1] = readByte;
        }
        else {
          // write mode
          if (headLoadedFlag && !writeProtectFlag) {
            trackDirtyFlag = true;
            trackBuffer_GCR[headPosition] = tia6523Registers[1];
          }
          prvByteWasFF = false;
        }
        tia6523Registers[2] = uint8_t((tia6523Registers[2] & 0xBF)
                                      | (syncFlag ? 0x00 : 0x40));
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

  void VC1551::runHalfCycle(SerialBus& serialBus_)
  {
    (void) serialBus_;
    throw Plus4Emu::Exception("VC1551::runHalfCycle() is unimplemented");
  }

  void VC1551::reset()
  {
    (void) flushTrack();        // FIXME: should report errors ?
    cpu.reset();
    for (int i = 0; i < 6; i++)
      tia6523Registers[i] = 0x00;
    // set device number
    tia6523Registers[2] = uint8_t(0xDF | ((deviceNumber & 0x01) << 5));
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
                                         int debugContext_,
                                         bool isIO, bool isWrite,
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
        return tia6523Registers[addr & 0x0007];
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
      addr = addr & 0x0007;
      switch (addr) {
      case 0:
      case 1:
        {
          uint8_t mask_ = tia6523Registers[addr + 3];
          tia6523Registers[addr] &= (mask_ ^ uint8_t(0xFF));
          tia6523Registers[addr] |= (value & mask_);
        }
        break;
      case 2:
        {
          uint8_t mask_ = tia6523Registers[addr + 3] & uint8_t(0x1F);
          tia6523Registers[addr] &= (mask_ ^ uint8_t(0xFF));
          tia6523Registers[addr] |= (value & mask_);
        }
        break;
      case 3:
      case 4:
      case 5:
        tia6523Registers[addr] = value;
        break;
      }
    }
  }

  uint8_t VC1551::getLEDState() const
  {
    return uint8_t(((memory_ram[0x0001] ^ 0xFF) & 0x08) >> 2);
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

  bool VC1551::parallelIECRead(uint16_t addr, uint8_t& value)
  {
    if ((addr & 0x0020) != (uint16_t(tia6523Registers[2] & 0x04) << 3))
      return false;
    switch (addr & 0x0007) {
    case 0:
      value = tia6523Registers[0];
      break;
    case 1:
      value = tia6523Registers[2] & uint8_t(0x03);
      break;
    case 2:
      value = (((tia6523Registers[2] >> 1) & uint8_t(0x40))
               | ((tia6523Registers[2] << 4) & uint8_t(0x80)));
      break;
    default:
      value = 0x00;
    }
    return true;
  }

  bool VC1551::parallelIECWrite(uint16_t addr, uint8_t value)
  {
    if ((addr & 0x0020) != (uint16_t(tia6523Registers[2] & 0x04) << 3))
      return false;
    switch (addr & 0x0007) {
    case 0:
      tia6523Registers[0] &= tia6523Registers[3];
      tia6523Registers[0] |= (value & (tia6523Registers[3] ^ uint8_t(0xFF)));
      break;
    case 2:
      tia6523Registers[2] &= uint8_t(0x7F);
      tia6523Registers[2] |= ((value << 1) & uint8_t(0x80));
      break;
    }
    return true;
  }

}       // namespace Plus4

