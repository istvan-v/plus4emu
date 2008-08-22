
// compressor utility for Commodore Plus/4 programs
// Copyright (C) 2007-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
//
// The Plus/4 program files generated by this utility are not covered by the
// GNU General Public License, and can be used, modified, and distributed
// without any restrictions.

#include "plus4emu.hpp"
#include "compress.hpp"
#include "decompress0.hpp"
#include <vector>

namespace Plus4Compress {

  unsigned int Decompressor_M0::readBits(size_t nBits)
  {
    unsigned int  retval = 0U;
    for (size_t i = 0; i < nBits; i++) {
      if (shiftRegisterCnt < 1) {
        if (inputBufferPosition >= inputBufferSize)
          throw Plus4Emu::Exception("unexpected end of compressed data");
        shiftRegister = inputBuffer[inputBufferPosition];
        shiftRegisterCnt = 8;
        inputBufferPosition++;
      }
      retval = (retval << 1) | (unsigned int) ((shiftRegister >> 7) & 0x01);
      shiftRegister = (shiftRegister & 0x7F) << 1;
      shiftRegisterCnt--;
    }
    return retval;
  }

  unsigned int Decompressor_M0::readLZMatchParameterBits(unsigned char n)
  {
    if (n < 0x08)
      return (unsigned int) n;
    if (n >= 0x3C)
      throw Plus4Emu::Exception("error in compressed data");
    unsigned int  retval = (unsigned int) ((n & 0x03) | 0x04);
    unsigned int  nBits = (unsigned int) (n >> 2) - 1U;
    retval = (retval << nBits) | readBits(nBits);
    return retval;
  }

  unsigned int Decompressor_M0::gammaDecode()
  {
    unsigned int  retval = 1U;
    unsigned int  cnt = 1U;
    while (true) {
      if (readBits(1) == 0U)
        break;
      retval = (retval << 1) | readBits(1);
      cnt++;
      if (cnt > 16U)
        throw Plus4Emu::Exception("error in compressed data");
    }
    return retval;
  }

  unsigned int Decompressor_M0::huffmanDecode(int huffTable)
  {
    if (huffTable == 0 && !usingHuffTable0)
      return readBits(9);
    if (huffTable != 0 && !usingHuffTable1)
      return readBits(5);
    unsigned int  tmp = 1U;
    unsigned int  cnt = 0U;
    const unsigned int  *limitTable = huffmanLimitTable0;
    const unsigned int  *offsetTable = huffmanOffsetTable0;
    const unsigned int  *decodeTable = huffmanDecodeTable0;
    if (huffTable != 0) {
      limitTable = huffmanLimitTable1;
      offsetTable = huffmanOffsetTable1;
      decodeTable = huffmanDecodeTable1;
    }
    while (true) {
      if (cnt >= 16U)
        throw Plus4Emu::Exception("error in compressed data");
      tmp = (tmp << 1) | readBits(1);
      if (tmp < limitTable[cnt])
        break;
      cnt++;
    }
    tmp = (tmp + offsetTable[cnt]) & 0xFFFFU;
    if (huffTable == 0 && tmp >= 324U)
      throw Plus4Emu::Exception("error in compressed data");
    if (huffTable != 0 && tmp >= 28U)
      throw Plus4Emu::Exception("error in compressed data");
    return decodeTable[tmp];
  }

  unsigned char Decompressor_M0::readDeltaValue()
  {
    unsigned char retval = (unsigned char) readBits(7);
    if (retval < 0x40)
      retval = retval + 0xC0;
    else
      retval = retval - 0x3F;
    return retval;
  }

  unsigned int Decompressor_M0::readMatchLength()
  {
    unsigned char n = 0x00;
    if (usingHuffTable1)
      n = (unsigned char) huffmanDecode(1);
    else
      n = (unsigned char) readBits(5);
    unsigned int  retval = readLZMatchParameterBits(n);
    if (retval > 254U)
      throw Plus4Emu::Exception("error in compressed data");
    retval = retval + 2U;
    return retval;
  }

  void Decompressor_M0::huffmanInit(int huffTable)
  {
    unsigned int  *limitTable = huffmanLimitTable0;
    unsigned int  *offsetTable = huffmanOffsetTable0;
    unsigned int  *decodeTable = huffmanDecodeTable0;
    size_t        nSymbols = 324;
    if (huffTable != 0) {
      limitTable = huffmanLimitTable1;
      offsetTable = huffmanOffsetTable1;
      decodeTable = huffmanDecodeTable1;
      nSymbols = 28;
    }
    // clear tables
    for (size_t i = 0; i < 16; i++) {
      limitTable[i] = 0U;
      offsetTable[i] = 0xFFFFFFFFU;
    }
    for (size_t i = 0; i < nSymbols; i++)
      decodeTable[i] = 0xFFFFFFFFU;
    if (readBits(1) == 0U) {
      // Huffman coding is disabled, nothing to do
      if (huffTable == 0)
        usingHuffTable0 = false;
      else
        usingHuffTable1 = false;
      return;
    }
    if (huffTable == 0)
      usingHuffTable0 = true;
    else
      usingHuffTable1 = true;
    unsigned int  tmp = 1U;
    size_t        tablePos = 0;
    bool          doneFlag = false;
    for (size_t i = 0; i < 16; i++) {
      tmp = tmp << 1;
      offsetTable[i] = tablePos - tmp;
      size_t  cnt = gammaDecode() - 1U;
      if (cnt > nSymbols)
        throw Plus4Emu::Exception("error in compressed data");
      unsigned int  decodedValue = 0U - 1U;
      for (size_t j = 0; j < cnt; j++) {
        if (doneFlag)
          throw Plus4Emu::Exception("error in compressed data");
        decodedValue = decodedValue + gammaDecode();
        if (decodedValue >= (unsigned int) nSymbols || tablePos >= nSymbols)
          throw Plus4Emu::Exception("error in compressed data");
        if (tmp == 0xFFU)       // 1111111 is never output by the compressor
          throw Plus4Emu::Exception("error in compressed data");
        decodeTable[tablePos] = decodedValue;
        tablePos++;
        tmp++;
        if (tmp >= (1U << (i + 2)))
          doneFlag = true;      // overflow (only allowed for the last symbol)
      }
      limitTable[i] = tmp;
    }
  }

  bool Decompressor_M0::decompressDataBlock(std::vector< unsigned char >& buf,
                                            std::vector< bool >& bytesUsed)
  {
    unsigned int  startAddr = readBits(16);
    unsigned int  nBytes = (readBits(16) ^ 0xFFFFU) + 1U;
    bool          isLastBlock = readBits(1);
    bool          compressionEnabled = readBits(1);
    if (!compressionEnabled) {
      // copy uncompressed data
      for (unsigned int i = 0U; i < nBytes; i++) {
        if (startAddr > 0xFFFFU || bytesUsed[startAddr])
          throw Plus4Emu::Exception("error in compressed data");
        buf[startAddr] = (unsigned char) readBits(8);
        bytesUsed[startAddr] = true;
        startAddr++;
      }
      return isLastBlock;
    }
    huffmanInit(0);
    huffmanInit(1);
    prvMatchOffsetsPos = 0U;
    for (int i = 0; i < 4; i++)         // clear recent match offsets buffer
      prvMatchOffsets[i] = 0xFFFFFFFFU;
    for (unsigned int i = 0U; i < nBytes; ) {
      unsigned int  tmp = 0U;
      if (usingHuffTable0)
        tmp = huffmanDecode(0);
      else
        tmp = readBits(9);
      if (tmp >= 0x0144U)
        throw Plus4Emu::Exception("error in compressed data");
      if (tmp <= 0xFFU) {
        // literal character
        if (startAddr > 0xFFFFU || bytesUsed[startAddr])
          throw Plus4Emu::Exception("error in compressed data");
        buf[startAddr] = (unsigned char) tmp;
        bytesUsed[startAddr] = true;
        startAddr++;
        i++;
        continue;
      }
      unsigned int  offs = 0U;
      unsigned char deltaValue = 0x00;
      if (tmp >= 0x013CU) {
        // check for special match codes:
        if (tmp < 0x0140U) {
          // match with delta value
          deltaValue = readDeltaValue();
          offs = tmp & 0x03U;
        }
        else {
          // use recent match offset
          offs = prvMatchOffsets[(prvMatchOffsetsPos + tmp) & 3U];
        }
      }
      else if (tmp < 0x0108) {
        // short match offset
        offs = tmp & 0x07U;
      }
      else {
        offs = readLZMatchParameterBits((unsigned char) (tmp & 0xFFU));
        // store in recent offsets buffer
        prvMatchOffsetsPos = (prvMatchOffsetsPos - 1U) & 3U;
        prvMatchOffsets[prvMatchOffsetsPos] = offs;
      }
      if (offs >= 0xFFFFU)
        throw Plus4Emu::Exception("error in compressed data");
      offs++;
      unsigned int  lzMatchReadAddr = (startAddr - offs) & 0xFFFFU;
      unsigned int  matchLength = readMatchLength();
      for (unsigned int j = 0U; j < matchLength; j++) {
        if (i >= nBytes)                // block should not end within a match
          throw Plus4Emu::Exception("error in compressed data");
        if (!bytesUsed[lzMatchReadAddr])    // byte does not exist yet
          throw Plus4Emu::Exception("error in compressed data");
        if (startAddr > 0xFFFFU || bytesUsed[startAddr])
          throw Plus4Emu::Exception("error in compressed data");
        buf[startAddr] = (buf[lzMatchReadAddr] + deltaValue) & 0xFF;
        bytesUsed[startAddr] = true;
        startAddr++;
        lzMatchReadAddr = (lzMatchReadAddr + 1U) & 0xFFFFU;
        i++;
      }
    }
    return isLastBlock;
  }

  // --------------------------------------------------------------------------

  Decompressor_M0::Decompressor_M0()
    : Decompressor(),
      huffmanLimitTable0((unsigned int *) 0),
      huffmanOffsetTable0((unsigned int *) 0),
      huffmanDecodeTable0((unsigned int *) 0),
      huffmanLimitTable1((unsigned int *) 0),
      huffmanOffsetTable1((unsigned int *) 0),
      huffmanDecodeTable1((unsigned int *) 0),
      usingHuffTable0(false),
      usingHuffTable1(false),
      shiftRegister(0x00),
      shiftRegisterCnt(0),
      inputBuffer((unsigned char *) 0),
      inputBufferSize(0),
      inputBufferPosition(0),
      prvMatchOffsetsPos(0U)
  {
    size_t  totalTableSize = 16 + 16 + 324 + 16 + 16 + 28;
    huffmanLimitTable0 = new unsigned int[totalTableSize];
    for (size_t i = 0; i < totalTableSize; i++)
      huffmanLimitTable0[i] = 0U;
    huffmanOffsetTable0 = &(huffmanLimitTable0[16]);
    huffmanDecodeTable0 = &(huffmanLimitTable0[16 + 16]);
    huffmanLimitTable1 = &(huffmanLimitTable0[16 + 16 + 324]);
    huffmanOffsetTable1 = &(huffmanLimitTable1[16]);
    huffmanDecodeTable1 = &(huffmanLimitTable1[16 + 16]);
    for (size_t i = 0; i < 4; i++)
      prvMatchOffsets[i] = 0xFFFFFFFFU;
  }

  Decompressor_M0::~Decompressor_M0()
  {
    delete[] huffmanLimitTable0;
  }

  void Decompressor_M0::decompressData(
      std::vector< std::vector< unsigned char > >& outBuf,
      const std::vector< unsigned char >& inBuf)
  {
    outBuf.clear();
    if (inBuf.size() < 1)
      return;
    bool    startPosTable[1024];
    for (size_t i = 0; i < 1024; i++)
      startPosTable[i] = false;
    unsigned char crcValue = 0x00;
    // find possible start offsets of compressed data
    for (size_t i = inBuf.size(); i > 0; ) {
      i--;
      crcValue = crcValue ^ inBuf[i];
      crcValue = ((crcValue & 0x7F) << 1) | ((crcValue & 0x80) >> 7);
      crcValue = (crcValue + 0xC4) & 0xFF;
      if (crcValue == 0x80) {
        if (i < 64 || (i >= 576 && i < 1024))
          startPosTable[i] = true;
      }
    }
    std::vector< unsigned char >  tmpBuf;
    std::vector< bool > bytesUsed;
    tmpBuf.resize(65536);
    bytesUsed.resize(65536);
    bool    doneFlag = false;
    for (size_t i = 0; i < 1024; i++) {
      if (!startPosTable[i])
        continue;
      for (size_t j = 0; j < 65536; j++) {
        tmpBuf[j] = 0x00;
        bytesUsed[j] = false;
      }
      // if found a position where the checksum matches, try to decompress data
      inputBuffer = &(inBuf.front());
      inputBufferSize = inBuf.size();
      inputBufferPosition = i + 1;
      shiftRegister = 0x00;
      shiftRegisterCnt = 0;
      try {
        while (!decompressDataBlock(tmpBuf, bytesUsed))
          ;
        // on successful decompression, all input data must be consumed
        if (!(inputBufferPosition >= inputBufferSize && shiftRegister == 0x00))
          throw Plus4Emu::Exception("error in compressed data");
        doneFlag = true;
      }
      catch (Plus4Emu::Exception) {
      }
      if (doneFlag)
        break;
    }
    if (!doneFlag)
      throw Plus4Emu::Exception("error in compressed data");
    unsigned int  startPos = 0U;
    size_t        nBytes = 0;
    // find all data blocks
    for (size_t i = 0; i <= 65536; i++) {
      if (i >= 65536 || !bytesUsed[i]) {
        if (nBytes > 0) {
          std::vector< unsigned char >  newBuf;
          newBuf.push_back((unsigned char) (startPos & 0xFFU));
          newBuf.push_back((unsigned char) ((startPos >> 8) & 0xFFU));
          for (size_t j = 0U; j < nBytes; j++)
            newBuf.push_back(tmpBuf[startPos + j]);
          outBuf.push_back(newBuf);
        }
        startPos = (unsigned int) (i + 1);
        nBytes = 0;
      }
      else {
        nBytes++;
      }
    }
  }

  // --------------------------------------------------------------------------

  const unsigned char Decompressor_M0::sfxModuleLibrary[1898] = {
    0x00, 0x20, 0x42, 0x20, 0x00, 0xD8, 0x7B, 0x62, 0x69, 0x75, 0x56, 0xB9,
    0x25, 0x55, 0xCF, 0xB7, 0xCA, 0xA9, 0x75, 0x6A, 0x7D, 0x2E, 0xE2, 0x94,
    0x5F, 0xAA, 0x9C, 0x3C, 0xA9, 0xD4, 0xB7, 0x73, 0xE6, 0x53, 0x68, 0xD4,
    0xCA, 0x95, 0xB5, 0x6A, 0xA8, 0xAD, 0xFA, 0xA5, 0x6A, 0x2E, 0x9D, 0x6A,
    0xEF, 0x70, 0xCB, 0x56, 0xB8, 0x98, 0x84, 0x0A, 0x02, 0xF2, 0x52, 0x5E,
    0xBD, 0x27, 0x3E, 0x42, 0x4E, 0x5E, 0xD4, 0xA4, 0xA4, 0x4E, 0x49, 0x35,
    0xC3, 0xF9, 0xCE, 0x12, 0xE9, 0x32, 0x95, 0x6E, 0x5F, 0xCF, 0xDB, 0xDA,
    0x9D, 0xAE, 0xD9, 0x52, 0xF5, 0x5A, 0x51, 0x6B, 0x4B, 0x73, 0x4D, 0x6D,
    0xF4, 0x7E, 0x7B, 0x96, 0x97, 0xB8, 0xAD, 0xE9, 0x66, 0xB6, 0xFF, 0x55,
    0xFB, 0xE4, 0xC0, 0xDA, 0x9C, 0x77, 0xA9, 0x53, 0x8A, 0x51, 0xA8, 0xE8,
    0xCE, 0x5E, 0x9E, 0xA5, 0x2D, 0xD3, 0x52, 0x92, 0xA5, 0xAA, 0x4C, 0x9E,
    0xA4, 0x8E, 0x4A, 0x2D, 0x5B, 0x68, 0xE5, 0x42, 0x29, 0x78, 0x0A, 0x29,
    0x2B, 0xDE, 0xD9, 0x3F, 0x76, 0xA8, 0xC4, 0x24, 0x8A, 0x59, 0x69, 0x5E,
    0x00, 0x3D, 0xE2, 0x63, 0x06, 0x34, 0xA9, 0x41, 0xBC, 0x97, 0xBB, 0x09,
    0x09, 0x8F, 0x70, 0x2F, 0xA6, 0x89, 0x05, 0x33, 0x90, 0x47, 0xC8, 0xED,
    0x42, 0xC2, 0x82, 0xE4, 0x34, 0x28, 0xBE, 0x10, 0x29, 0x36, 0x08, 0x57,
    0x52, 0xDD, 0x06, 0x7D, 0x3C, 0x26, 0x7B, 0x2A, 0x34, 0xB2, 0x7B, 0xC7,
    0x82, 0x7C, 0x9B, 0x8B, 0x30, 0xDC, 0xA9, 0xF6, 0x03, 0xA1, 0x54, 0xEB,
    0x47, 0x37, 0x38, 0x41, 0xB6, 0xAC, 0x64, 0x1F, 0x4D, 0x6B, 0x40, 0xE0,
    0x79, 0x4D, 0x43, 0x67, 0xBA, 0x82, 0x62, 0xF9, 0x96, 0x81, 0x4B, 0x5C,
    0xB4, 0x32, 0x3C, 0xEC, 0x40, 0x59, 0xE7, 0xE0, 0x04, 0xD2, 0xB6, 0x21,
    0xCB, 0xF4, 0x63, 0x1E, 0x6D, 0x81, 0xC7, 0x5A, 0xC6, 0x09, 0xC8, 0xDE,
    0xAC, 0x0F, 0x02, 0xCB, 0x88, 0x17, 0x9E, 0x9E, 0x38, 0x8F, 0x53, 0x92,
    0x1B, 0x1E, 0x04, 0x47, 0x0A, 0xE0, 0x92, 0x15, 0xAB, 0x1B, 0x07, 0xE5,
    0xC2, 0x82, 0x5D, 0x5B, 0x5B, 0x86, 0xD3, 0x86, 0xBC, 0x0A, 0xF5, 0xF1,
    0x83, 0x79, 0xC5, 0x2A, 0x21, 0x9C, 0x6B, 0xB0, 0xC4, 0x5F, 0x09, 0x9E,
    0x5F, 0x1F, 0x4B, 0x34, 0x3D, 0xA8, 0x27, 0x22, 0xEA, 0xD4, 0x3C, 0x2B,
    0xBB, 0x90, 0x6E, 0xD1, 0xD8, 0x2B, 0x6B, 0xEF, 0x87, 0x83, 0x7A, 0xD0,
    0x30, 0xEF, 0x9A, 0xFF, 0x18, 0xE2, 0xF4, 0x3A, 0xBE, 0xEC, 0x11, 0x97,
    0xBC, 0x40, 0x22, 0x72, 0xB8, 0x0A, 0xE4, 0xBA, 0x0B, 0x0F, 0x83, 0x08,
    0x25, 0x31, 0x59, 0x07, 0x2B, 0x1A, 0x31, 0x5B, 0x8E, 0x71, 0xD4, 0xF8,
    0xA0, 0x8B, 0xF9, 0x76, 0x81, 0xDE, 0xBC, 0x5A, 0x03, 0x49, 0xC9, 0x49,
    0xC8, 0x9E, 0x69, 0x43, 0x3D, 0x3C, 0x94, 0xF7, 0xE7, 0x9B, 0xD5, 0xE8,
    0x13, 0xCD, 0x77, 0xC3, 0x93, 0x67, 0x2A, 0x33, 0xB9, 0x5D, 0x8B, 0x34,
    0x72, 0x4E, 0xF3, 0x3B, 0x25, 0x08, 0xF8, 0x63, 0x33, 0xDB, 0x21, 0xCA,
    0x2D, 0xEE, 0x0C, 0x7C, 0x0B, 0x9E, 0xC1, 0x6E, 0x92, 0xE5, 0x3C, 0x0C,
    0xAF, 0x6D, 0x4E, 0x0C, 0xA4, 0x05, 0x2D, 0x99, 0xA6, 0xE9, 0x3A, 0x6E,
    0xFE, 0x58, 0x97, 0x56, 0xEC, 0xD7, 0x03, 0xD6, 0x95, 0x30, 0x13, 0x2B,
    0x4A, 0x42, 0x52, 0x81, 0xE2, 0x67, 0x8D, 0x2E, 0x1C, 0xB3, 0x5E, 0x97,
    0x70, 0x92, 0x47, 0x77, 0xCC, 0x21, 0x23, 0x12, 0xFF, 0x06, 0xCD, 0x32,
    0x7B, 0xC2, 0xDF, 0x38, 0xB3, 0xF8, 0xD2, 0x17, 0x1C, 0x49, 0xFA, 0xB9,
    0x71, 0xB8, 0xB5, 0x10, 0x2C, 0x7A, 0xD0, 0x09, 0x1E, 0xDE, 0x6D, 0x12,
    0xC1, 0xEF, 0xA6, 0xD8, 0x52, 0x65, 0x49, 0xE6, 0x92, 0x08, 0xAD, 0x87,
    0xAA, 0x52, 0xE1, 0x62, 0xDF, 0x20, 0x65, 0xC3, 0xA2, 0x55, 0x25, 0x99,
    0xAA, 0xA2, 0x2A, 0x21, 0xCA, 0xD9, 0x2F, 0xA6, 0x7B, 0x4C, 0xFF, 0x60,
    0xF6, 0x92, 0x1B, 0x8F, 0x69, 0x4B, 0xE9, 0xD7, 0xBF, 0xC8, 0xAB, 0xE7,
    0x8E, 0xF6, 0x9D, 0x76, 0x66, 0x4D, 0x31, 0x55, 0xD9, 0x28, 0x17, 0x77,
    0xD6, 0xC9, 0xD1, 0x63, 0xC3, 0x45, 0x7A, 0x9D, 0x84, 0xE7, 0x1E, 0x7E,
    0x1C, 0x01, 0x11, 0xA8, 0x93, 0xD7, 0x2D, 0xC6, 0xA9, 0x80, 0xAE, 0x98,
    0x89, 0xA6, 0x3F, 0xAA, 0xA7, 0x23, 0x6B, 0x78, 0x78, 0x98, 0x5C, 0x4A,
    0x3E, 0xA0, 0x43, 0xE9, 0xE5, 0x7F, 0x97, 0x96, 0xE3, 0x21, 0xCC, 0xD6,
    0xCB, 0x76, 0x7A, 0x26, 0x99, 0x45, 0xE7, 0x31, 0x4D, 0xA6, 0x55, 0x32,
    0xE0, 0x88, 0xC3, 0x18, 0x5C, 0xFD, 0xE5, 0x21, 0x14, 0x8B, 0xB8, 0x45,
    0x2F, 0xC9, 0x50, 0x54, 0x9C, 0xC0, 0xEA, 0x49, 0x90, 0xFE, 0xB0, 0xBA,
    0x49, 0xF4, 0xD1, 0x7F, 0x5E, 0xB9, 0xC2, 0xB9, 0xBD, 0x14, 0xBF, 0xC3,
    0x9C, 0xE5, 0x4F, 0xAE, 0xBB, 0x95, 0xB2, 0xE0, 0xDA, 0x51, 0x25, 0x3C,
    0xA0, 0xF6, 0xDD, 0x02, 0x16, 0x28, 0xBD, 0x09, 0x27, 0x03, 0x47, 0xA4,
    0x2E, 0x26, 0x0B, 0xC7, 0x2B, 0xBA, 0x6F, 0x8B, 0xF6, 0x8D, 0x8F, 0xD1,
    0x19, 0x9C, 0xB0, 0xD0, 0xF8, 0x54, 0x85, 0x03, 0x72, 0x4F, 0xB0, 0x9B,
    0xB8, 0xB8, 0xC5, 0x7C, 0x36, 0xAD, 0x99, 0xE8, 0xBD, 0x29, 0x2B, 0xE9,
    0xC7, 0x00, 0xC1, 0x43, 0x16, 0x15, 0x8E, 0x20, 0x11, 0x3F, 0x22, 0x45,
    0x03, 0x69, 0xDC, 0xE5, 0x0C, 0x15, 0x36, 0x47, 0x9D, 0x58, 0xC3, 0x7C,
    0x89, 0xB0, 0xE4, 0x50, 0xE4, 0x8C, 0xDB, 0x57, 0xD1, 0x14, 0x4F, 0x28,
    0x4B, 0x3E, 0xF2, 0x89, 0xFF, 0x8B, 0x9E, 0xA0, 0xB0, 0xCF, 0xAC, 0xD2,
    0x92, 0xB9, 0xFD, 0x40, 0x1C, 0x2B, 0xA0, 0xD4, 0x62, 0xF9, 0xD7, 0xA8,
    0xFA, 0xE3, 0x69, 0xF8, 0x2A, 0x75, 0xF8, 0x3C, 0x47, 0xD7, 0x0B, 0xE7,
    0x65, 0xF1, 0x26, 0x3B, 0xB7, 0xFC, 0x6A, 0x99, 0x04, 0x18, 0x69, 0xE4,
    0x62, 0x79, 0x87, 0xE4, 0xCC, 0xF8, 0xC5, 0xB2, 0x7E, 0xC4, 0x98, 0x0E,
    0x06, 0xB6, 0x4F, 0x97, 0x1E, 0x91, 0xFA, 0x59, 0x92, 0xBB, 0xFF, 0xA1,
    0x41, 0x59, 0xAB, 0xB6, 0x7D, 0xAA, 0x2E, 0x3A, 0xF0, 0xF7, 0xF6, 0xC5,
    0x33, 0x2E, 0x9D, 0x1C, 0x8F, 0x6C, 0x1B, 0xC4, 0x53, 0xF9, 0xD1, 0x89,
    0x4F, 0xB4, 0x73, 0x7E, 0xA8, 0x8A, 0x8E, 0xD0, 0xDF, 0xEF, 0x88, 0x4F,
    0x52, 0x1C, 0xEA, 0x8E, 0xC8, 0xE6, 0xA1, 0x54, 0x2C, 0x8B, 0xE6, 0x12,
    0x87, 0x6A, 0x4A, 0x0E, 0xC1, 0x1C, 0xC7, 0xFC, 0x2F, 0xD9, 0x9F, 0xF7,
    0x75, 0xA6, 0x93, 0xD4, 0xA5, 0x83, 0xD4, 0xCD, 0xA8, 0xE6, 0x48, 0x48,
    0xE5, 0x29, 0x14, 0x6D, 0xB4, 0xE9, 0xAB, 0xB5, 0x09, 0xFD, 0x5D, 0xF8,
    0xFE, 0xCB, 0xD5, 0x5B, 0x5D, 0xF7, 0x29, 0xAA, 0xAA, 0xBF, 0xCA, 0xAF,
    0x98, 0x1D, 0x09, 0xFE, 0xDB, 0x9A, 0x1A, 0x1D, 0x45, 0x44, 0xB0, 0x77,
    0x60, 0x85, 0x84, 0x3F, 0x5A, 0x17, 0x87, 0xF0, 0x69, 0x63, 0x99, 0x9E,
    0x41, 0x96, 0x5F, 0x57, 0x31, 0xF0, 0xF7, 0x05, 0xE6, 0xD7, 0xAD, 0x94,
    0x81, 0xFA, 0x1F, 0xF7, 0xC5, 0x51, 0x03, 0x75, 0x57, 0xEB, 0x5B, 0x98,
    0xEB, 0x24, 0x74, 0x66, 0x32, 0x4E, 0x0F, 0x10, 0x95, 0xC2, 0x82, 0x13,
    0x1C, 0xB2, 0x1D, 0xDA, 0x9D, 0x7B, 0x43, 0x1A, 0x14, 0x11, 0x87, 0xD1,
    0xB0, 0x18, 0xDA, 0xB0, 0x85, 0x58, 0x42, 0x7F, 0xB7, 0x17, 0xC3, 0x19,
    0xEA, 0x7D, 0xE2, 0x39, 0xA2, 0xE2, 0x36, 0x17, 0x52, 0xD8, 0x5B, 0xE8,
    0xB7, 0xA2, 0xC3, 0xF5, 0x88, 0xFA, 0x85, 0xA7, 0xEC, 0x14, 0x78, 0x98,
    0x18, 0xBF, 0x64, 0xD6, 0x2B, 0xB1, 0x31, 0x3D, 0xA0, 0x3A, 0xFE, 0x71,
    0x6A, 0x1A, 0xC1, 0xF8, 0xC8, 0xC5, 0x27, 0x86, 0x4B, 0x21, 0x6D, 0x18,
    0x88, 0xD0, 0x15, 0xE6, 0x38, 0xB3, 0x21, 0x25, 0x5C, 0xB0, 0xC6, 0xA7,
    0xFF, 0x7F, 0x98, 0xC3, 0x7A, 0x1D, 0x1D, 0xCC, 0x74, 0x77, 0xBA, 0x6A,
    0x2C, 0x0D, 0x45, 0xED, 0xDC, 0xA5, 0x12, 0x30, 0xBB, 0x8E, 0x89, 0xA9,
    0x7C, 0xCB, 0x23, 0x11, 0xDE, 0xC0, 0xEB, 0xE7, 0x8B, 0x68, 0xC3, 0xF4,
    0xD0, 0x54, 0xD0, 0x5C, 0x33, 0x50, 0x2C, 0xD3, 0xA9, 0xE1, 0x48, 0x09,
    0x84, 0x55, 0x2E, 0x9D, 0x6E, 0xAC, 0x8D, 0x42, 0xAE, 0x99, 0xDB, 0x8D,
    0x4E, 0xA6, 0xB1, 0x54, 0x3E, 0x78, 0xA1, 0x8C, 0x65, 0x01, 0x15, 0x18,
    0xFD, 0xA1, 0x88, 0x69, 0xA1, 0xD3, 0xC6, 0xA8, 0xCF, 0x18, 0x7B, 0xBF,
    0x34, 0xE2, 0xEB, 0xCC, 0x2D, 0x67, 0xCE, 0xBF, 0x97, 0x5B, 0x5F, 0x15,
    0xA0, 0xAA, 0xD2, 0x29, 0xDE, 0x78, 0x52, 0x42, 0x61, 0x14, 0xC4, 0xF5,
    0x69, 0x82, 0xA9, 0x69, 0xD3, 0xC6, 0x88, 0x4C, 0x09, 0xFE, 0xB9, 0xEE,
    0xC7, 0x13, 0x5A, 0xCE, 0xB1, 0x22, 0x95, 0x67, 0x85, 0x21, 0x1B, 0xC0,
    0x8A, 0x67, 0x71, 0x80, 0x63, 0x3A, 0xC0, 0x3A, 0x78, 0xD5, 0x69, 0xEF,
    0xFF, 0x73, 0xDD, 0xEB, 0x9E, 0x2C, 0x98, 0xEB, 0x77, 0x53, 0x18, 0xA7,
    0x99, 0xE1, 0x4F, 0x46, 0x7B, 0x15, 0x16, 0xDB, 0x5F, 0x8E, 0xC8, 0xE9,
    0xE3, 0x4D, 0x27, 0x84, 0xFF, 0x5C, 0xEB, 0x65, 0xDC, 0x92, 0x5F, 0xC5,
    0x4A, 0xCF, 0x18, 0xBE, 0x7A, 0x2A, 0x01, 0xE3, 0x23, 0x0F, 0x53, 0x21,
    0x3F, 0x35, 0x2E, 0x78, 0xC3, 0x73, 0xDA, 0xE7, 0x2F, 0x45, 0x66, 0xB6,
    0x75, 0x2C, 0x8A, 0x6B, 0x9E, 0x17, 0x88, 0x8F, 0x0A, 0xFD, 0xCB, 0xCA,
    0x57, 0x57, 0x2A, 0x5F, 0xB5, 0x75, 0xF2, 0xA7, 0x2B, 0xFE, 0x55, 0xDC,
    0xEE, 0x1C, 0xAE, 0x55, 0xD6, 0xE5, 0x77, 0x67, 0x73, 0xFF, 0xDB, 0x28,
    0x33, 0xD7, 0x23, 0xDB, 0x29, 0xB3, 0xBF, 0x96, 0x57, 0xBB, 0x2A, 0x88,
    0xE8, 0xCE, 0xE7, 0xD7, 0x2F, 0x33, 0xE5, 0xD5, 0x00, 0x33, 0xE8, 0xDA,
    0xAD, 0xAE, 0xC9, 0x54, 0xE7, 0x25, 0x2D, 0xE9, 0xD6, 0xDF, 0x3A, 0x00,
    0x13, 0x5F, 0xFF, 0x89, 0x5B, 0x2B, 0x18, 0x0F, 0x31, 0xC9, 0xE6, 0x0A,
    0x61, 0x67, 0x39, 0xBF, 0x7F, 0x18, 0xD5, 0x4E, 0x24, 0x92, 0xF2, 0x4F,
    0xB1, 0xD7, 0xF8, 0x09, 0x83, 0x1F, 0x68, 0x6B, 0x36, 0xC9, 0xE6, 0x55,
    0x7B, 0xAC, 0x9B, 0xDE, 0x73, 0x8F, 0x06, 0xB8, 0xF7, 0xCA, 0xC9, 0xDD,
    0xE4, 0xF7, 0x63, 0xAB, 0xA0, 0x51, 0xD8, 0xB0, 0x07, 0xA4, 0x24, 0xF3,
    0x1C, 0x3C, 0xB6, 0xDC, 0xE7, 0xAE, 0xC8, 0xF3, 0xCB, 0x0A, 0x77, 0x7D,
    0x3C, 0x18, 0x90, 0x51, 0x19, 0xD0, 0x08, 0x24, 0x16, 0x7B, 0x27, 0xBC,
    0x0E, 0xBC, 0xE7, 0x5D, 0x77, 0xE3, 0xA9, 0x2F, 0x93, 0x9B, 0xC9, 0x55,
    0x69, 0x2F, 0x66, 0x65, 0x02, 0x66, 0x59, 0x3C, 0xC1, 0xCC, 0x2D, 0x07,
    0x3B, 0xEF, 0xE1, 0x1B, 0x09, 0xC0, 0x94, 0x5E, 0x48, 0x4F, 0xE0, 0x26,
    0x3C, 0x7D, 0x81, 0x94, 0xD9, 0x27, 0x99, 0x51, 0xEE, 0x9B, 0x3D, 0xB7,
    0x3A, 0x70, 0x74, 0x47, 0xAE, 0x68, 0x27, 0x57, 0x93, 0xD3, 0xCD, 0x28,
    0x7C, 0x57, 0xE1, 0x79, 0xC9, 0xE6, 0x32, 0x79, 0x5A, 0xE7, 0x1D, 0x7F,
    0xF2, 0x16, 0xA9, 0x48, 0xBC, 0x17, 0x30, 0x4F, 0x99, 0x75, 0x83, 0xB2,
    0xB5, 0x9E, 0xC9, 0x2F, 0x03, 0x4F, 0x39, 0xBB, 0x45, 0xE8, 0xE5, 0xCB,
    0xC4, 0xCE, 0xF2, 0x55, 0x5A, 0x4A, 0x59, 0x38, 0x80, 0x69, 0x8A, 0x4F,
    0x30, 0x43, 0x0A, 0xF2, 0x41, 0xE1, 0xFA, 0xE6, 0xBF, 0x5E, 0x55, 0x43,
    0xD1, 0x7A, 0xF9, 0xC6, 0x9E, 0x73, 0x24, 0x57, 0x92, 0x96, 0x9C, 0xFF,
    0xC1, 0xE5, 0xF3, 0x93, 0xE3, 0xFB, 0x23, 0x23, 0xFF, 0x7F, 0x64, 0xC3,
    0xD7, 0x85, 0x50, 0xF6, 0x5C, 0xF7, 0x87, 0x33, 0x7C, 0x9E, 0x65, 0x87,
    0x83, 0x6D, 0x90, 0xCB, 0xCB, 0x9C, 0xB4, 0xDA, 0x20, 0xBC, 0xC9, 0xF5,
    0xE4, 0xC4, 0xE8, 0x65, 0x1F, 0x51, 0x4C, 0xF5, 0xDD, 0x28, 0xB8, 0x69,
    0x08, 0xA6, 0x99, 0x3C, 0xC8, 0x0F, 0x2D, 0xF7, 0x3C, 0xEB, 0xD7, 0x1F,
    0x39, 0xAC, 0xAB, 0x5E, 0x4F, 0x06, 0xB9, 0x21, 0x27, 0x97, 0x3E, 0xAF,
    0xB5, 0x94, 0x2F, 0x65, 0x77, 0x82, 0x10, 0xD5, 0x0E, 0x80, 0xD1, 0x86,
    0x3C, 0x13, 0x09, 0x41, 0xBC, 0x9E, 0x01, 0x73, 0x1D, 0x08, 0xA3, 0x38,
    0x1B, 0x33, 0x93, 0x73, 0xCC, 0x2C, 0xC2, 0xD4, 0xBA, 0x2B, 0xC5, 0x26,
    0x10, 0x1D, 0x0F, 0x00, 0x3A, 0xEE, 0x80, 0xB2, 0xE0, 0x1D, 0x39, 0x60,
    0x3A, 0x13, 0x7D, 0xD1, 0x5E, 0xF1, 0x42, 0xBD, 0x3D, 0x90, 0x6F, 0xB9,
    0x80, 0xC7, 0x40, 0x20, 0x28, 0xF3, 0x28, 0xFE, 0x80, 0x64, 0xD0, 0x43,
    0xE6, 0xE8, 0xAF, 0x06, 0x80, 0x03, 0xC0, 0x49, 0xE2, 0x77, 0x79, 0x5E,
    0x01, 0x5F, 0x47, 0x98, 0x41, 0xE5, 0xF5, 0x12, 0x8F, 0x0F, 0xC7, 0x3D,
    0xFA, 0xF3, 0xAA, 0x1E, 0xAB, 0xD7, 0xCA, 0x36, 0x73, 0x91, 0x2A, 0xBC,
    0x93, 0xEC, 0x81, 0xFE, 0x06, 0x4A, 0xE3, 0xF4, 0x4D, 0x9D, 0x78, 0xD5,
    0x0F, 0x75, 0xCF, 0x70, 0x71, 0x37, 0x49, 0xE6, 0x5B, 0x7B, 0xAC, 0xC5,
    0xDE, 0x73, 0xEE, 0xFB, 0x44, 0xDB, 0x99, 0x41, 0xBC, 0x9D, 0x9D, 0x0C,
    0x9F, 0xE9, 0x29, 0xAE, 0xBB, 0xE5, 0x17, 0x0B, 0x02, 0x19, 0x64, 0x9E,
    0x63, 0xE7, 0x96, 0xF3, 0x9F, 0xF5, 0xEA, 0x8F, 0x84, 0xD4, 0x55, 0x2F,
    0x27, 0xBA, 0xA4, 0xAD, 0x07, 0x40, 0x6F, 0xB5, 0x9F, 0xF7, 0xD9, 0x55,
    0xE0, 0x81, 0xE7, 0x40, 0xEF, 0xC1, 0x1D, 0xD9, 0x80, 0x9F, 0xDE, 0x4F,
    0x00, 0x99, 0x6E, 0x82, 0xD1, 0x98, 0x0C, 0x99, 0x89, 0xA9, 0xE6, 0x14,
    0x61, 0x69, 0xDD, 0x1A, 0xE2, 0x92, 0xE8, 0x0E, 0x83, 0x80, 0x1C, 0xF7,
    0x40, 0x19, 0x68, 0x0E, 0x9C, 0x50, 0x1D, 0x01, 0xBE, 0xE8, 0xD7, 0x78,
    0xA0, 0x5E, 0x9E, 0xC0, 0x35, 0xD4, 0xC0, 0x63, 0x80, 0x10, 0x14, 0x79,
    0x93, 0xFF, 0x40, 0x31, 0x68, 0x20, 0xF1, 0x74, 0x6B, 0x81, 0x40, 0x01,
    0xD0, 0x24, 0xF1, 0x39, 0xBC, 0xAF, 0x00, 0x6F, 0xA3, 0xCC, 0x1C, 0xF2,
    0xFB, 0x40
  };

  void Decompressor_M0::getSFXModule(std::vector< unsigned char >& outBuf,
                                     int runAddr, bool c16Mode,
                                     bool fastSFXModule, bool noCleanup,
                                     bool noROM, bool noCLI)
  {
    bool    isBasicProgram = (runAddr == -1);
    if (runAddr < -3)
      runAddr = -2;
    else if (runAddr > 0xFFFF)
      runAddr = runAddr & 0xFFFF;
    outBuf.clear();
    int     decompressModuleIndex =
        (c16Mode ? 1 : 0) | (fastSFXModule ? 2 : 0) | (noCleanup ? 4 : 0)
        | (noROM ? 8 : 0) | (noCLI ? 16 : 0) | (isBasicProgram ? 32 : 0);
    std::vector< unsigned char >  tmpBuf1;
    std::vector< std::vector< unsigned char > > tmpBuf2;
    for (size_t i = 0; i < sizeof(sfxModuleLibrary); i++)
      tmpBuf1.push_back(sfxModuleLibrary[i]);
    Decompressor_M0 decompressor;
    decompressor.decompressData(tmpBuf2, tmpBuf1);
    unsigned int  baseAddr =
        (unsigned int) tmpBuf2[0][0] | ((unsigned int) tmpBuf2[0][1] << 8);
    unsigned char *p = &(tmpBuf2[0].front()) + (decompressModuleIndex * 4 + 2);
    unsigned int  offs =
        ((unsigned int) p[0] | ((unsigned int) p[1] << 8)) - baseAddr;
    unsigned int  len = (unsigned int) p[2] | ((unsigned int) p[3] << 8);
    p = &(tmpBuf2[0].front()) + (offs + 2U);
    for (unsigned int i = 0U; i < len; i++)
      outBuf.push_back(p[i]);
    if (!isBasicProgram) {
      if (runAddr == -3)
        runAddr = 0xFF52;
      else if (runAddr < 0)
        runAddr = 0x867E;
      outBuf[outBuf.size() - 2] = (unsigned char) (runAddr & 0xFF);
      outBuf[outBuf.size() - 1] = (unsigned char) ((runAddr >> 8) & 0xFF);
    }
  }

}       // namespace Plus4Compress

