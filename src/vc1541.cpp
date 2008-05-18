
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
#include "via6522.hpp"
#include "serial.hpp"
#include "p4floppy.hpp"
#include "vc1541.hpp"

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

  const int D64Image::d64TrackOffsetTable[44] = {
        -1,      0,   5376,  10752,  16128,  21504,  26880,  32256,
     37632,  43008,  48384,  53760,  59136,  64512,  69888,  75264,
     80640,  86016,  91392,  96256, 101120, 105984, 110848, 115712,
    120576, 125440, 130048, 134656, 139264, 143872, 148480, 153088,
    157440, 161792, 166144, 170496, 174848, 179200, 183552, 187904,
    192256, 196608, 200960, 205312
  };

  const int D64Image::sectorsPerTrackTable[44] = {
     0, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 19, 19, 19, 19, 19, 19,
    19, 18, 18, 18, 18, 18, 18, 17,
    17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17
  };

  const int D64Image::trackSizeTable[44] = {
    7692, 7692, 7692, 7692, 7692, 7692, 7692, 7692,
    7692, 7692, 7692, 7692, 7692, 7692, 7692, 7692,
    7692, 7692, 7143, 7143, 7143, 7143, 7143, 7143,
    7143, 6667, 6667, 6667, 6667, 6667, 6667, 6250,
    6250, 6250, 6250, 6250, 6250, 6250, 6250, 6250,
    6250, 6250, 6250, 6250
  };

  // number of bits per 1 MHz cycle, multiplied by 65536
  const int D64Image::trackSpeedTable[44] = {
    0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5,
    0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5, 0x4EC5,
    0x4EC5, 0x4EC5, 0x4925, 0x4925, 0x4925, 0x4925, 0x4925, 0x4925,
    0x4925, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4000,
    0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
    0x4000, 0x4000, 0x4000, 0x4000
  };

  const uint8_t D64Image::gcrEncodeTable[16] = {
    0x0A, 0x0B, 0x12, 0x13, 0x0E, 0x0F, 0x16, 0x17,
    0x09, 0x19, 0x1A, 0x1B, 0x0D, 0x1D, 0x1E, 0x15
  };

  const uint8_t D64Image::gcrDecodeTable[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x08, 0x00, 0x01, 0xFF, 0x0C, 0x04, 0x05,
    0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x0F, 0x06, 0x07,
    0xFF, 0x09, 0x0A, 0x0B, 0xFF, 0x0D, 0x0E, 0xFF
  };

  // --------------------------------------------------------------------------

  void D64Image::gcrEncodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf)
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

  bool D64Image::gcrDecodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf)
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

  void D64Image::gcrEncodeTrack(int trackNum, int nSectors, int nBytes)
  {
    int     readPos = 0;
    int     writePos = 0;
    uint8_t tmpBuf1[8];
    uint8_t tmpBuf2[5];
    for (int i = 0; i < nSectors; i++) {
      int     gapSize = 9;
      if (i & 1) {
        switch (nSectors) {
        case 19:
          gapSize = 19;
          break;
        case 18:
          gapSize = 13;
          break;
        case 17:
          gapSize = 10;
          break;
        }
      }
      if (badSectorTable[i] & 0xFA) {
        // bad sector, fill with zero bytes
        for (int j = 0; j < (354 + gapSize); j++)
          trackBuffer_GCR[writePos++] = 0x00;
        readPos += 256;
        continue;
      }
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
      uint8_t crcValue = 0;
      for (int j = 2; j < 6; j++)
        crcValue = crcValue ^ tmpBuf1[j];
      if (badSectorTable[i] == 0x05)
        crcValue = (~crcValue);     // CRC error
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
      if (badSectorTable[i] == 0x04)
        tmpBuf1[0] = 0x00;          // invalid data block
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
      do {
        trackBuffer_GCR[writePos++] = 0x55;
      } while (--gapSize);
    }
    // pad track data to requested length
    for ( ; writePos < nBytes; writePos++)
      trackBuffer_GCR[writePos] = 0x55;
  }

  int D64Image::gcrDecodeTrack(int trackNum, int nSectors, int nBytes)
  {
    int     sectorsDecoded = 0;
    int     readPos = 0;
    int     firstSyncPos = -1;
    for (int i = 0; i < nSectors; i++)
      badSectorTable[i] = 0x02;
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
            badSectorTable[currentSector] = 0x01;
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

  bool D64Image::readTrack(int trackNum)
  {
    bool    retval = true;
    if (trackNum < 0)
      trackNum = currentTrack;
    for (int i = 0; i < trackSizeTable[trackNum]; i++)
      trackBuffer_GCR[i] = 0x00;
    for (int i = 0; i < 24; i++)
      badSectorTable[i] = 0x00;
    if (trackNum >= 1 && trackNum <= nTracks) {
      retval = false;
      int     nSectors = sectorsPerTrackTable[trackNum];
      if (haveBadSectorTable) {
        // read bad sector table (FIXME: errors are ignored)
        if (std::fseek(imageFile, long((d64TrackOffsetTable[trackNum] >> 8)
                                       + d64TrackOffsetTable[nTracks + 1]),
                       SEEK_SET) >= 0) {
          std::fread(&(badSectorTable[0]),
                     sizeof(uint8_t), size_t(nSectors), imageFile);
        }
      }
      if (std::fseek(imageFile, long(d64TrackOffsetTable[trackNum]),
                     SEEK_SET) >= 0) {
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

  bool D64Image::flushTrack(int trackNum)
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
      if (haveBadSectorTable) {
        retval = false;
        // update bad sector table
        if (std::fseek(imageFile, long((d64TrackOffsetTable[trackNum] >> 8)
                                       + d64TrackOffsetTable[nTracks + 1]),
                       SEEK_SET) >= 0) {
          if (std::fwrite(&(badSectorTable[0]),
                          sizeof(uint8_t), size_t(nSectors), imageFile)
              == size_t(nSectors)) {
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

  bool D64Image::setCurrentTrack(int trackNum)
  {
    bool    retval = true;
    trackNum = (trackNum >= 1 ? (trackNum <= 42 ? trackNum : 42) : 1);
    if (trackNum != currentTrack) {
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

  D64Image::D64Image()
    : trackDirtyFlag(false),
      currentTrack(42),
      nTracks(0),
      imageFile((std::FILE *) 0),
      writeProtectFlag(false),
      diskID(0x00),
      idCharacter1(0x41),
      idCharacter2(0x41),
      haveBadSectorTable(false)
  {
    // clear track buffers
    for (int i = 0; i < 8192; i++)
      trackBuffer_GCR[i] = 0x00;
    for (int i = 0; i < 5376; i++)
      trackBuffer_D64[i] = 0x00;
    for (int i = 0; i < 24; i++)
      badSectorTable[i] = 0x00;
  }

  D64Image::~D64Image()
  {
    if (imageFile) {
      (void) flushTrack();
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
      nTracks = 0;
    }
  }

  void D64Image::setImageFile(const std::string& fileName_)
  {
    if (imageFile) {
      (void) flushTrack();              // FIXME: should report errors ?
      std::fclose(imageFile);
      imageFile = (std::FILE *) 0;
      nTracks = 0;
    }
    writeProtectFlag = false;
    haveBadSectorTable = false;
    (void) setCurrentTrack(18);         // FIXME: should report errors ?
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
      long    nSectors = fSize / 256L;
      if ((nSectors * 256L) != fSize) {
        nSectors = fSize / 257L;
        if ((nSectors * 257L) != fSize)
          nSectors = 0L;
      }
      nSectors -= 683L;
      // allow any number of tracks from 35 to 42
      if (nSectors < 0L || nSectors > 119L ||
          ((nSectors / 17L) * 17L) != nSectors) {
        std::fclose(imageFile);
        imageFile = (std::FILE *) 0;
        throw Plus4Emu::Exception("D64 image file has invalid length");
      }
      std::fseek(imageFile, 0L, SEEK_SET);
      writeProtectFlag = isReadOnly;
      nTracks = 35L + (nSectors / 17L);
      haveBadSectorTable = (((nSectors + 683L) * 256L) < fSize);
      diskID = (diskID + 1) & 0xFF;
      if (((diskID >> 4) + 0x41) == idCharacter1 &&
          ((diskID & 0x0F) + 0x41) == idCharacter2)
        diskID = (diskID + 1) & 0xFF;   // make sure that the disk ID changes
      idCharacter1 = (diskID >> 4) + 0x41;
      idCharacter2 = (diskID & 0x0F) + 0x41;
      currentTrack = 42;
      (void) setCurrentTrack(18);       // FIXME: should report errors ?
    }
  }

  // --------------------------------------------------------------------------

  VC1541::M7501_::M7501_(VC1541& vc1541_)
    : M7501(),
      vc1541(vc1541_)
  {
    // initialize memory map
    setMemoryCallbackUserData((void *) &vc1541_);
    for (uint16_t i = 0x0000; i <= 0x7FFF; i++) {
      switch (i & 0x1C00) {
      case 0x0000:
      case 0x0400:
        setMemoryReadCallback(i, &VC1541::readMemory_RAM);
        setMemoryWriteCallback(i, &VC1541::writeMemory_RAM);
        break;
      case 0x1800:
        setMemoryReadCallback(i, &VC1541::readMemory_VIA1);
        setMemoryWriteCallback(i, &VC1541::writeMemory_VIA1);
        break;
      case 0x1C00:
        setMemoryReadCallback(i, &VC1541::readMemory_VIA2);
        setMemoryWriteCallback(i, &VC1541::writeMemory_VIA2);
        break;
      default:
        setMemoryReadCallback(i, &VC1541::readMemory_Dummy);
        setMemoryWriteCallback(i, &VC1541::writeMemory_Dummy);
        break;
      }
    }
    for (uint32_t i = 0x8000; i <= 0xFFFF; i++) {
      setMemoryReadCallback(uint16_t(i), &VC1541::readMemory_ROM);
      setMemoryWriteCallback(uint16_t(i), &VC1541::writeMemory_Dummy);
    }
  }

  VC1541::M7501_::~M7501_()
  {
  }

  void VC1541::M7501_::breakPointCallback(int type,
                                          uint16_t addr, uint8_t value)
  {
    if (vc1541.noBreakOnDataRead && type == 1)
      return;
    vc1541.breakPointCallback(vc1541.breakPointCallbackUserData,
                              (vc1541.deviceNumber & 3) + 1, type, addr, value);
  }

  VC1541::VIA6522_::VIA6522_(VC1541& vc1541_)
    : VIA6522(),
      vc1541(vc1541_),
      interruptFlag(false)
  {
  }

  VC1541::VIA6522_::~VIA6522_()
  {
  }

  void VC1541::VIA6522_::irqStateChangeCallback(bool newState)
  {
    interruptFlag = newState;
    vc1541.cpu.interruptRequest(vc1541.via1.interruptFlag
                                | vc1541.via2.interruptFlag);
  }

  // --------------------------------------------------------------------------

  uint8_t VC1541::readMemory_RAM(void *userData, uint16_t addr)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = vc1541.memory_ram[addr & 0x07FF];
    return vc1541.dataBusState;
  }

  uint8_t VC1541::readMemory_Dummy(void *userData, uint16_t addr)
  {
    (void) addr;
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    return vc1541.dataBusState;
  }

  uint8_t VC1541::readMemory_VIA1(void *userData, uint16_t addr)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    uint8_t serialBusInput = uint8_t((vc1541.serialBus.getDATA() & 0x01)
                                     | (vc1541.serialBus.getCLK() & 0x04)
                                     | (vc1541.serialBus.getATN() & 0x80));
    vc1541.via1.setPortB(serialBusInput ^ vc1541.via1PortBInput);
    vc1541.dataBusState = vc1541.via1.readRegister(addr & 0x000F);
    return vc1541.dataBusState;
  }

  uint8_t VC1541::readMemory_VIA2(void *userData, uint16_t addr)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = vc1541.via2.readRegister(addr & 0x000F);
    return vc1541.dataBusState;
  }

  uint8_t VC1541::readMemory_ROM(void *userData, uint16_t addr)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    if (vc1541.memory_rom)
      vc1541.dataBusState = vc1541.memory_rom[addr & 0x3FFF];
    return vc1541.dataBusState;
  }

  void VC1541::writeMemory_RAM(void *userData, uint16_t addr, uint8_t value)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = value & 0xFF;
    vc1541.memory_ram[addr & 0x07FF] = vc1541.dataBusState;
  }

  void VC1541::writeMemory_Dummy(void *userData, uint16_t addr, uint8_t value)
  {
    (void) addr;
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = value & 0xFF;
  }

  void VC1541::writeMemory_VIA1(void *userData, uint16_t addr, uint8_t value)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = value & 0xFF;
    addr = addr & 0x000F;
    vc1541.via1.writeRegister(addr, vc1541.dataBusState);
  }

  void VC1541::writeMemory_VIA2(void *userData, uint16_t addr, uint8_t value)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.dataBusState = value & 0xFF;
    addr = addr & 0x000F;
    vc1541.via2.writeRegister(addr, vc1541.dataBusState);
  }

  // --------------------------------------------------------------------------

  bool VC1541::updateMotors()
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
    uint8_t stepperMotorPhase = via2.getPortB() & 3;
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
        // write protect sense input is inverted for 0.25 seconds after
        // disk change
        via2PortBInput = uint8_t(writeProtectFlag ? (via2PortBInput & 0xEF)
                                                    : (via2PortBInput | 0x10));
        via2.setPortB(via2PortBInput);
        spindleMotorSpeed = 0;
      }
      return false;
    }
    // update spindle motor speed
    // spin up/down time is 16 * (65536 / 4) cycles / 1000000 Hz = ~262 ms
    if (!(via2.getPortB() & 0x04)) {
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

  bool VC1541::setCurrentTrack(int trackNum)
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

  void VC1541::updateHead()
  {
    bool    syncFlag = false;
    if (via2.getCB2()) {
      // read mode
      uint8_t readByte = 0x00;
      if (headLoadedFlag) {
        readByte = trackBuffer_GCR[headPosition];
        if (readByte == 0xFF)
          syncFlag = prvByteWasFF;
      }
      prvByteWasFF = (readByte == 0xFF);
      via2.setPortA(readByte);
    }
    else {
      // write mode
      via2.setPortA(0xFF);
      if (headLoadedFlag && !writeProtectFlag) {
        trackDirtyFlag = true;
        trackBuffer_GCR[headPosition] = via2.getPortA();
      }
      prvByteWasFF = false;
    }
    via2PortBInput = uint8_t(syncFlag ? (via2PortBInput & 0x7F)
                                        : (via2PortBInput | 0x80));
    via2.setPortB(via2PortBInput);
    // set byte ready flag
    if (via2.getCA2() && !syncFlag) {
      cpu.setOverflowFlag();
      via2.setCA1(false);
    }
    // update head position
    if (spindleMotorSpeed >= 32768) {
      headPosition = headPosition + 1;
      if (headPosition >= trackSizeTable[currentTrack])
        headPosition = 0;
    }
  }

  // --------------------------------------------------------------------------

  VC1541::VC1541(SerialBus& serialBus_, int driveNum_)
    : FloppyDrive(serialBus_, driveNum_),
      D64Image(),
      cpu(*this),
      via1(*this),
      via2(*this),
      memory_rom((uint8_t *) 0),
      serialBusDelay(715827882U),
      deviceNumber(uint8_t(driveNum_)),
      dataBusState(0x00),
      via1PortBInput(0xFF),
      halfCycleFlag(false),
      headLoadedFlag(false),
      prvByteWasFF(false),
      via2PortBInput(0xEF),
      motorUpdateCnt(0),
      shiftRegisterBitCnt(0),
      shiftRegisterBitCntFrac(0x0000),
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
    // set device number
    via1PortBInput = uint8_t(0x9F | ((deviceNumber & 0x03) << 5));
    via1.setPortB(via1PortBInput);
    via1.setPortA(0xFE);
    via2.setPortB(0xEF);
    this->reset();
  }

  VC1541::~VC1541()
  {
  }

  void VC1541::setROMImage(int n, const uint8_t *romData_)
  {
    if (n == 2)
      memory_rom = romData_;
  }

  void VC1541::setDiskImageFile(const std::string& fileName_)
  {
    headLoadedFlag = false;
    prvByteWasFF = false;
    spindleMotorSpeed = 0;
    diskChangeCnt = 15625;
    currentTrackStepperMotorPhase = 0;
    (void) setCurrentTrack(18);         // FIXME: should report errors ?
    via2PortBInput &= uint8_t(0xEF);
    via2.setPortB(via2PortBInput);
    D64Image::setImageFile(fileName_);
    // invert write protect sense input for 0.25 seconds so that the DOS can
    // detect the disk change
    via2PortBInput = uint8_t(writeProtectFlag ? (via2PortBInput | 0x10)
                                                : (via2PortBInput & 0xEF));
    via2.setPortB(via2PortBInput);
  }

  bool VC1541::haveDisk() const
  {
    return (imageFile != (std::FILE *) 0);
  }

  void VC1541::processCallback(void *userData)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.timeRemaining += vc1541.serialBus.timesliceLength;
    while (vc1541.timeRemaining >= 0) {
      vc1541.timeRemaining -= (int64_t(1) << 32);
      {
        uint8_t via1PortBOutput = vc1541.via1.getPortB();
        uint8_t atnInput = (~(uint8_t(vc1541.serialBus.getATN())));
        uint8_t atnAck_ = via1PortBOutput ^ atnInput;
        atnAck_ = uint8_t((atnAck_ & 0x10) | (via1PortBOutput & 0x02));
        vc1541.serialBus.setCLKAndDATA(vc1541.deviceNumber,
                                       !(via1PortBOutput & 0x08), !(atnAck_));
        vc1541.via1.setCA1(bool(atnInput));
      }
      vc1541.via1.runOneCycle();
      vc1541.via2.runOneCycle();
      vc1541.cpu.runOneCycle();
      if (!vc1541.motorUpdateCnt) {
        vc1541.motorUpdateCnt = 16;
        vc1541.headLoadedFlag = vc1541.updateMotors();
      }
      vc1541.motorUpdateCnt--;
      vc1541.shiftRegisterBitCntFrac =
          vc1541.shiftRegisterBitCntFrac
          + vc1541.trackSpeedTable[vc1541.currentTrack];
      if (vc1541.shiftRegisterBitCntFrac >= 65536) {
        vc1541.shiftRegisterBitCntFrac =
            vc1541.shiftRegisterBitCntFrac & 0xFFFF;
        if (vc1541.shiftRegisterBitCnt >= 7) {
          vc1541.shiftRegisterBitCnt = 0;
          // read/write next byte
          vc1541.updateHead();
        }
        else if (++(vc1541.shiftRegisterBitCnt) == 1) {
          // clear byte ready flag
          vc1541.via2.setCA1(true);
        }
      }
    }
  }

  void VC1541::processCallbackHighAccuracy(void *userData)
  {
    VC1541& vc1541 = *(reinterpret_cast<VC1541 *>(userData));
    vc1541.timeRemaining += (vc1541.serialBus.timesliceLength >> 1);
    while (vc1541.timeRemaining >= 0) {
      if (!vc1541.halfCycleFlag) {
        // delay serial port output by ~833.3 ns
        vc1541.halfCycleFlag = true;
        uint8_t via1PortBOutput = vc1541.via1.getPortB();
        uint8_t atnAck_ =
            via1PortBOutput ^ (~(uint8_t(vc1541.serialBus.getATN())));
        atnAck_ = uint8_t((atnAck_ & 0x10) | (via1PortBOutput & 0x02));
        vc1541.serialBus.setCLKAndDATA(vc1541.deviceNumber,
                                       !(via1PortBOutput & 0x08), !(atnAck_));
      }
      if (vc1541.timeRemaining >= int64_t(vc1541.serialBusDelay)) {
        vc1541.via1.setCA1(!vc1541.serialBus.getATN());
        vc1541.via1.runOneCycle();
        vc1541.via2.runOneCycle();
        vc1541.cpu.runOneCycle();
        if (!vc1541.motorUpdateCnt) {
          vc1541.motorUpdateCnt = 16;
          vc1541.headLoadedFlag = vc1541.updateMotors();
        }
        vc1541.motorUpdateCnt--;
        vc1541.shiftRegisterBitCntFrac =
            vc1541.shiftRegisterBitCntFrac
            + vc1541.trackSpeedTable[vc1541.currentTrack];
        if (vc1541.shiftRegisterBitCntFrac >= 65536) {
          vc1541.shiftRegisterBitCntFrac =
              vc1541.shiftRegisterBitCntFrac & 0xFFFF;
          if (vc1541.shiftRegisterBitCnt >= 7) {
            vc1541.shiftRegisterBitCnt = 0;
            // read/write next byte
            vc1541.updateHead();
          }
          else if (++(vc1541.shiftRegisterBitCnt) == 1) {
            // clear byte ready flag
            vc1541.via2.setCA1(true);
          }
        }
        vc1541.halfCycleFlag = false;
        vc1541.timeRemaining -= (int64_t(1) << 32);
      }
      else
        break;
    }
  }

  SerialDevice::ProcessCallbackPtr VC1541::getProcessCallback()
  {
    return &processCallback;
  }

  SerialDevice::ProcessCallbackPtr VC1541::getHighAccuracyProcessCallback()
  {
    return &processCallbackHighAccuracy;
  }

  void VC1541::reset()
  {
    (void) flushTrack();        // FIXME: should report errors ?
    via1.reset();
    via2.reset();
    cpu.reset();
    via1.setPortA(0xFE);
    via1PortBInput = uint8_t(0x9F | ((deviceNumber & 0x03) << 5));
    via1.setPortB(via1PortBInput);
  }

  M7501 * VC1541::getCPU()
  {
    return (&cpu);
  }

  const M7501 * VC1541::getCPU() const
  {
    return (&cpu);
  }

  void VC1541::setBreakPointCallback(void (*breakPointCallback_)(
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

  void VC1541::setNoBreakOnDataRead(bool n)
  {
    noBreakOnDataRead = n;
  }

  uint8_t VC1541::readMemoryDebug(uint16_t addr) const
  {
    if (addr < 0x8000) {
      switch (addr & 0x1C00) {
      case 0x0000:
      case 0x0400:
        return memory_ram[addr & 0x07FF];
      case 0x1800:
        return via1.readRegisterDebug(addr & 0x000F);
      case 0x1C00:
        return via2.readRegisterDebug(addr & 0x000F);
      }
    }
    else if (memory_rom)
      return memory_rom[addr & 0x3FFF];
    return uint8_t(0xFF);
  }

  void VC1541::writeMemoryDebug(uint16_t addr, uint8_t value)
  {
    if (addr < 0x8000) {
      switch (addr & 0x1C00) {
      case 0x0000:
      case 0x0400:
        memory_ram[addr & 0x07FF] = value;
        break;
      case 0x1800:
        via1.writeRegister(addr & 0x000F, value);
        break;
      case 0x1C00:
        via2.writeRegister(addr & 0x000F, value);
        break;
      }
    }
  }

  uint8_t VC1541::getLEDState() const
  {
    return uint8_t((via2.getPortB() & 0x08) >> 3);
  }

  uint16_t VC1541::getHeadPosition() const
  {
    if (!imageFile)
      return 0xFFFF;
    uint16_t  retval = uint16_t((currentTrack & 0x7F) << 8);
    // FIXME: this is not accurate
    retval |= uint16_t((headPosition / 367) & 0x7F);
    return retval;
  }

  void VC1541::setSerialBusDelayOffset(int n)
  {
    n = (n > -100 ? (n < 100 ? n : 100) : -100);
    serialBusDelay = uint32_t(715827882 - (n * 4294967));
  }

  void VC1541::saveState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1541::saveState(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

  void VC1541::loadState(Plus4Emu::File::Buffer& buf)
  {
    // TODO: implement this
    (void) buf;
  }

  void VC1541::registerChunkTypes(Plus4Emu::File& f)
  {
    // TODO: implement this
    (void) f;
  }

}       // namespace Plus4

