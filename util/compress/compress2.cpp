
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
#include "compress1.hpp"
#include "compress2.hpp"

#include <list>
#include <map>

namespace Plus4Compress {

  const size_t Compressor_M2::lengthPrefixSizeTable[lengthNumSlots] = {
    1, 2, 3, 4, 5, 6, 7, 8
  };

  const size_t Compressor_M2::offs3SlotCntTable[4] = {
    4, 8, 16, 32
  };

  // --------------------------------------------------------------------------

  void Compressor_M2::SearchTable::sortFunc(size_t startPos, size_t endPos,
                                            std::vector< unsigned int >& tmpBuf)
  {
    if ((startPos + 1) >= endPos)
      return;
    size_t  splitPos = (startPos + endPos) >> 1;
    sortFunc(startPos, splitPos, tmpBuf);
    sortFunc(splitPos, endPos, tmpBuf);
    size_t  i = startPos;
    size_t  j = splitPos;
    for (size_t k = 0; k < (endPos - startPos); k++) {
      if (i >= splitPos) {
        tmpBuf[k] = suffixArray[j];
        j++;
      }
      else if (j >= endPos) {
        tmpBuf[k] = suffixArray[i];
        i++;
      }
      else {
        size_t  pos1 = suffixArray[i];
        size_t  pos2 = suffixArray[j];
        size_t  len1 = buf.size() - pos1;
        if (len1 > Compressor_M2::maxRepeatLen)
          len1 = Compressor_M2::maxRepeatLen;
        size_t  len2 = buf.size() - pos2;
        if (len2 > Compressor_M2::maxRepeatLen)
          len2 = Compressor_M2::maxRepeatLen;
        size_t  l = (len1 < len2 ? len1 : len2);
        const void  *ptr1 = (const void *) (&(buf.front()) + pos1);
        const void  *ptr2 = (const void *) (&(buf.front()) + pos2);
        int     c = std::memcmp(ptr1, ptr2, l);
        if (c == 0)
          c = (pos1 < pos2 ? -1 : 1);
        if (c < 0) {
          tmpBuf[k] = suffixArray[i];
          i++;
        }
        else {
          tmpBuf[k] = suffixArray[j];
          j++;
        }
      }
    }
    for (size_t k = 0; k < (endPos - startPos); k++)
      suffixArray[startPos + k] = tmpBuf[k];
  }

  Compressor_M2::SearchTable::SearchTable(
      const std::vector< unsigned char >& inBuf)
    : buf(inBuf)
  {
    if (buf.size() < 1) {
      throw Plus4Emu::Exception("Compressor_M2::SearchTable::SearchTable(): "
                                "zero input buffer size");
    }
    std::vector< unsigned int > tmpBuf;
    tmpBuf.resize(buf.size());
    suffixArray.resize(buf.size());
    prvMatchLenTable.resize(buf.size());
    nxtMatchLenTable.resize(buf.size());
    size_t  matchLenCnt =
        Compressor_M2::maxRepeatLen + 1 - Compressor_M2::minRepeatLen;
    bestMatchPosTable.resize(matchLenCnt);
    for (size_t i = 0; i < matchLenCnt; i++) {
      bestMatchPosTable[i].resize(buf.size());
      for (size_t j = 0; j < buf.size(); j++)
        bestMatchPosTable[i][j] = 0;
    }
    maxMatchLenTable.resize(buf.size());
    rleLengthTable.resize(buf.size());
    for (size_t i = 0; i < buf.size(); i++) {
      maxMatchLenTable[i] = 0;
      suffixArray[i] = (unsigned int) i;
      rleLengthTable[i] = 0;
    }
    sortFunc(0, buf.size(), tmpBuf);
    for (size_t i = 0; i < buf.size(); i++) {
      if (i == 0) {
        prvMatchLenTable[i] = 0;
      }
      else {
        size_t  len = 0;
        size_t  p1 = suffixArray[i - 1];
        size_t  p2 = suffixArray[i];
        while (len < Compressor_M2::maxRepeatLen &&
               p1 < buf.size() && p2 < buf.size() &&
               buf[p1] == buf[p2]) {
          len++;
          p1++;
          p2++;
        }
        prvMatchLenTable[i] = (unsigned short) len;
        nxtMatchLenTable[i - 1] = prvMatchLenTable[i];
      }
      nxtMatchLenTable[i] = 0;
    }
    {
      size_t        rleLength = 0;
      unsigned int  rleByte = 0x7FFFFFFFU;
      for (size_t i = buf.size() - 1; i > 0; ) {
        i--;
        if ((unsigned int) buf[i] != rleByte || rleLength >= 65535) {
          rleByte = (unsigned int) buf[i];
          rleLength = 0;
        }
        rleLengthTable[i + 1] = (unsigned short) rleLength;
        rleLength++;
      }
    }
    for (size_t i_ = 1; (i_ + 1) < buf.size(); i_++) {
      size_t  i = suffixArray[i_];
      size_t  matchLen = buf.size() - i;
      if (matchLen > Compressor_M2::maxRepeatLen)
        matchLen = Compressor_M2::maxRepeatLen;
      size_t  minLen = Compressor_M2::minRepeatLen;
      size_t  ndx = i_;
      while (true) {
        if (size_t(prvMatchLenTable[ndx]) < matchLen)
          matchLen = size_t(prvMatchLenTable[ndx]);
        if (matchLen < Compressor_M2::minRepeatLen)
          break;
        ndx--;
        size_t  matchPos = suffixArray[ndx];
        if (matchPos >= i || (matchPos + Compressor_M2::maxRepeatDist) < i)
          continue;
        if (matchLen > size_t(maxMatchLenTable[i]))
          maxMatchLenTable[i] = (unsigned short) matchLen;
        size_t  d = i - matchPos;
        size_t  k = matchLen - Compressor_M2::minRepeatLen;
        size_t  prvDist = size_t(bestMatchPosTable[k][i]);
        if (prvDist == 0 || d < prvDist)
          bestMatchPosTable[k][i] = (unsigned short) d;
        if (d == 1) {
          minLen = matchLen + 1;
          break;
        }
      }
      matchLen = buf.size() - i;
      if (matchLen > Compressor_M2::maxRepeatLen)
        matchLen = Compressor_M2::maxRepeatLen;
      ndx = i_;
      while (true) {
        if (size_t(nxtMatchLenTable[ndx]) < matchLen)
          matchLen = size_t(nxtMatchLenTable[ndx]);
        if (matchLen < minLen)
          break;
        ndx++;
        size_t  matchPos = suffixArray[ndx];
        if (matchPos >= i || (matchPos + Compressor_M2::maxRepeatDist) < i)
          continue;
        if (matchLen > size_t(maxMatchLenTable[i]))
          maxMatchLenTable[i] = (unsigned short) matchLen;
        size_t  d = i - matchPos;
        size_t  k = matchLen - Compressor_M2::minRepeatLen;
        size_t  prvDist = size_t(bestMatchPosTable[k][i]);
        if (prvDist == 0 || d < prvDist)
          bestMatchPosTable[k][i] = (unsigned short) d;
        if (d == 1)
          break;
      }
      size_t  k = size_t(maxMatchLenTable[i]);
      if (k > Compressor_M2::minRepeatLen) {
        k = k - Compressor_M2::minRepeatLen;
        do {
          if (bestMatchPosTable[k - 1][i] == 0 ||
              bestMatchPosTable[k][i] < bestMatchPosTable[k - 1][i]) {
            bestMatchPosTable[k - 1][i] = bestMatchPosTable[k][i];
          }
        } while (--k > 0);
      }
    }
  }

  Compressor_M2::SearchTable::~SearchTable()
  {
  }

  // --------------------------------------------------------------------------

  void Compressor_M2::writeRepeatCode(std::vector< unsigned int >& buf,
                                      size_t d, size_t n)
  {
    n = n - minRepeatLen;
    unsigned int  slotNum =
        (unsigned int) lengthEncodeTable.getSymbolSlotIndex((unsigned int) n);
    buf.push_back(((slotNum + 2U) << 24U) | ((1U << (slotNum + 2U)) - 2U));
    if (lengthEncodeTable.getSlotSize(slotNum) > 0)
      buf.push_back(lengthEncodeTable.encodeSymbol((unsigned int) n));
    d = d - minRepeatDist;
    if ((n + minRepeatLen) > 2) {
      slotNum =
          (unsigned int) offs3EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs3PrefixSize << 24) | slotNum);
      if (offs3EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs3EncodeTable.encodeSymbol((unsigned int) d));
    }
    else if ((n + minRepeatLen) > 1) {
      slotNum =
          (unsigned int) offs2EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs2PrefixSize << 24) | slotNum);
      if (offs2EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs2EncodeTable.encodeSymbol((unsigned int) d));
    }
    else {
      slotNum =
          (unsigned int) offs1EncodeTable.getSymbolSlotIndex((unsigned int) d);
      buf.push_back((unsigned int) (offs1PrefixSize << 24) | slotNum);
      if (offs1EncodeTable.getSlotSize(slotNum) > 0)
        buf.push_back(offs1EncodeTable.encodeSymbol((unsigned int) d));
    }
  }

  inline size_t Compressor_M2::getRepeatCodeLength(size_t d, size_t n) const
  {
    n = n - minRepeatLen;
    size_t  nBits = lengthEncodeTable.getSymbolSize(n) + 1;
    nBits = (nBits < (lengthNumSlots + 16) ? nBits : (lengthNumSlots + 16));
    d = d - minRepeatDist;
    if ((n + minRepeatLen) > 2)
      nBits += offs3EncodeTable.getSymbolSize(d);
    else if ((n + minRepeatLen) > 1)
      nBits += offs2EncodeTable.getSymbolSize(d);
    else
      nBits += offs1EncodeTable.getSymbolSize(d);
    return nBits;
  }

  inline void Compressor_M2::findBestMatch(
      LZMatchParameters& p, size_t i, size_t maxLen)
  {
    size_t  k = searchTable->getMaxMatchLength(i);
    if (k >= minRepeatLen) {
      k = (k < maxLen ? k : maxLen);
      size_t  d = searchTable->getDistanceForMatchLength(i, k);
      p.d = (unsigned short) d;
      p.len = (unsigned short) k;
      p.nBits = (unsigned short) getRepeatCodeLength(d, k);
      if ((k == 1 && d > offs1MaxValue) || (k == 2 && d > offs2MaxValue))
        p.clear();
    }
    else {
      p.clear();
    }
  }

  size_t Compressor_M2::compressData_(std::vector< unsigned int >& tmpOutBuf,
                                      const std::vector< unsigned char >& inBuf,
                                      size_t offs, size_t nBytes,
                                      bool optimizeEncodeTables, bool fastMode)
  {
    size_t  endPos = offs + nBytes;
    size_t  nSymbols = 0;
    tmpOutBuf.resize(0);
    if (optimizeEncodeTables) {
      // generate optimal encode tables for offset values
      offs1EncodeTable.updateTables(false);
      offs2EncodeTable.updateTables(false);
      offs3EncodeTable.updateTables(fastMode);
      offs3NumSlots = offs3EncodeTable.getSlotCnt();
      offs3PrefixSize = offs3EncodeTable.getSlotPrefixSize(0);
    }
    // compress data by searching for repeated byte sequences,
    // and replacing them with length/distance codes
    std::vector< LZMatchParameters >  matchTable;
    matchTable.resize(nBytes);
    for (size_t i = offs; i < endPos; i++) {
      matchTable[i - offs].clear();
      matchTable[i - offs].nBits = 9;
      size_t  maxLen = endPos - i;
      if (maxLen > maxRepeatLen)
        maxLen = maxRepeatLen;
      if (maxLen >= minRepeatLen)
        findBestMatch(matchTable[i - offs], i, maxLen);
    }
    {
      std::vector< long >   bitCountTable;
      bitCountTable.resize(nBytes + 1);
      bitCountTable[nBytes] = 0L;
      for (size_t i = endPos; i > offs; ) {
        i--;
        LZMatchParameters tmp;
        size_t  rleLength = searchTable->getRLELength(i);
        if (rleLength > (endPos - i))
          rleLength = endPos - i;
        if (rleLength > maxRepeatLen) {
          // if a long RLE match is possible, use that
          long    nxtMatchBitCnt = bitCountTable[i + rleLength - offs];
          long    curMatchBitCnt = nxtMatchBitCnt;
          tmp.clear();
          tmp.d = 1;
          tmp.len = (unsigned short) rleLength;
          tmp.nBits = (unsigned short) getRepeatCodeLength(1, rleLength);
          curMatchBitCnt += long(tmp.nBits);
          matchTable[i - offs] = tmp;
          bitCountTable[i - offs] = curMatchBitCnt;
        }
        else {
          size_t  maxLen = matchTable[i - offs].len;
          long    bestSize = 0x7FFFFFFFL;
          for (size_t k = maxLen; k >= minRepeatLen; k--) {
            // otherwise check all possible LZ77 match lengths,
            findBestMatch(tmp, i, k);
            if (tmp.d < 1)
              continue;
            long    nxtMatchBitCnt = bitCountTable[i + k - offs];
            long    curMatchBitCnt = nxtMatchBitCnt;
            curMatchBitCnt += long(tmp.nBits);
            if (curMatchBitCnt <= bestSize) {
              bestSize = curMatchBitCnt;
              matchTable[i - offs] = tmp;
              bitCountTable[i - offs] = curMatchBitCnt;
            }
          }
          {
            // literal byte,
            long    nxtMatchBitCnt = bitCountTable[i + 1 - offs];
            long    curMatchBitCnt = nxtMatchBitCnt;
            tmp.clear();
            tmp.nBits = 9;
            curMatchBitCnt += long(tmp.nBits);
            if (curMatchBitCnt <= bestSize) {
              bestSize = curMatchBitCnt;
              matchTable[i - offs] = tmp;
              bitCountTable[i - offs] = curMatchBitCnt;
            }
          }
          for (size_t k = ((maxLen + 1) <= literalSequenceMinLength ?
                           literalSequenceMinLength : (maxLen + 1));
               k <= (literalSequenceMinLength + 255) && (i + k) <= endPos;
               k++) {
            // and all possible literal sequence lengths
            long    nxtMatchBitCnt = bitCountTable[i + k - offs];
            long    curMatchBitCnt = nxtMatchBitCnt;
            tmp.clear();
            tmp.len = (unsigned short) k;
            tmp.nBits = (unsigned short) (k * 8 + literalSequenceMinLength);
            curMatchBitCnt += long(tmp.nBits);
            if (curMatchBitCnt > (bestSize + long(literalSequenceMinLength)))
              break;    // quit the loop earlier if the data can be compressed
            if (curMatchBitCnt <= bestSize) {
              bestSize = curMatchBitCnt;
              matchTable[i - offs] = tmp;
              bitCountTable[i - offs] = curMatchBitCnt;
            }
          }
        }
      }
    }
    // generate optimal encode table for length values
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        long    unencodedCost = long(tmp.len) * 9L - 1L;
        unencodedCost -=
            (tmp.len > 1 ? long(offs2PrefixSize) : long(offs1PrefixSize));
        unencodedCost = (unencodedCost > 0L ? unencodedCost : 0L);
        lengthEncodeTable.addSymbol(tmp.len - minRepeatLen,
                                    size_t(unencodedCost));
      }
      i += size_t(tmp.len);
    }
    lengthEncodeTable.updateTables(false);
    // update LZ77 offset statistics for calculating encode tables later
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        if (lengthEncodeTable.getSymbolSize(tmp.len - minRepeatLen) <= 64) {
          long    unencodedCost = long(tmp.len) * 9L - 1L;
          unencodedCost -=
              long(lengthEncodeTable.getSymbolSize(tmp.len - minRepeatLen));
          unencodedCost = (unencodedCost > 0L ? unencodedCost : 0L);
          if (tmp.len > 2) {
            offs3EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
          else if (tmp.len > 1) {
            offs2EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
          else {
            offs1EncodeTable.addSymbol(tmp.d - minRepeatDist,
                                       size_t(unencodedCost));
          }
        }
      }
      i += size_t(tmp.len);
    }
    // first pass: there are no offset encode tables yet, so no data is written
    if (!optimizeEncodeTables)
      return 0;
    // write encode tables
    tmpOutBuf.push_back(0x02000000U | (unsigned int) (offs3PrefixSize - 2));
    for (size_t i = 0; i < lengthNumSlots; i++) {
      unsigned int  c = (unsigned int) lengthEncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs1NumSlots; i++) {
      unsigned int  c = (unsigned int) offs1EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs2NumSlots; i++) {
      unsigned int  c = (unsigned int) offs2EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    for (size_t i = 0; i < offs3NumSlots; i++) {
      unsigned int  c = (unsigned int) offs3EncodeTable.getSlotSize(i);
      tmpOutBuf.push_back(0x04000000U | c);
    }
    // write compressed data
    for (size_t i = offs; i < endPos; ) {
      LZMatchParameters&  tmp = matchTable[i - offs];
      if (tmp.d > 0) {
        // check if this match needs to be replaced with literals:
        size_t  nBits = getRepeatCodeLength(tmp.d, tmp.len);
        if (nBits > 64 ||
            lengthEncodeTable.getSymbolSize(tmp.len - minRepeatLen) > 64) {
          // if the match cannot be encoded, assume "infinite" size
          nBits = 0x7FFFFFFF;
        }
        if ((size_t(tmp.len) >= literalSequenceMinLength &&
             nBits > (literalSequenceMinLength + (size_t(tmp.len) * 8))) ||
            nBits >= (size_t(tmp.len) * 9)) {
          tmp.d = 0;
        }
      }
      if (tmp.d > 0) {
        // write LZ77 match
        writeRepeatCode(tmpOutBuf, tmp.d, tmp.len);
        i = i + tmp.len;
        nSymbols++;
      }
      else {
        while (size_t(tmp.len) >= literalSequenceMinLength) {
          // write literal sequence
          size_t  len = tmp.len;
          len = (len < (literalSequenceMinLength + 255) ?
                 len : (literalSequenceMinLength + 255));
          tmpOutBuf.push_back((unsigned int) ((lengthNumSlots + 1) << 24)
                              | ((1U << (unsigned int) (lengthNumSlots + 1))
                                 - 1U));
          tmpOutBuf.push_back(0x08000000U
                              | (unsigned int) (len
                                                - literalSequenceMinLength));
          for (size_t j = 0; j < len; j++) {
            tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
            i++;
          }
          nSymbols++;
          tmp.len -= (unsigned short) len;
        }
        while (tmp.len > 0) {
          // write literal byte(s)
          tmpOutBuf.push_back(0x01000000U);
          tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
          i++;
          nSymbols++;
          tmp.len--;
        }
      }
    }
    return nSymbols;
  }

  bool Compressor_M2::compressData(std::vector< unsigned int >& tmpOutBuf,
                                   const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   size_t offs, size_t nBytes, bool fastMode)
  {
    // the 'offs' and 'nBytes' parameters allow compressing a buffer
    // as multiple chunks for possibly improved statistical compression
    if (nBytes < 1 || offs >= inBuf.size())
      return true;
    if (nBytes > (inBuf.size() - offs))
      nBytes = inBuf.size() - offs;
    size_t  endPos = offs + nBytes;
    lengthEncodeTable.clear();
    offs1EncodeTable.clear();
    offs2EncodeTable.clear();
    offs3EncodeTable.clear();
    std::vector< uint64_t >     hashTable;
    std::vector< unsigned int > bestBuf;
    std::vector< unsigned int > tmpBuf;
    const size_t  headerSize = 34;
    size_t  bestSize = 0x7FFFFFFF;
    size_t  nSymbols = 0;
    bool    doneFlag = false;
    for (size_t i = 0; i < config.optimizeIterations; i++) {
      if (progressDisplayEnabled) {
        if (!setProgressPercentage(int(progressCnt * 100 / progressMax)))
          return false;
        progressCnt++;
      }
      if (doneFlag)     // if the compression cannot be optimized further,
        continue;       // quit the loop earlier
      tmpBuf.resize(0);
      size_t  tmp =
          compressData_(tmpBuf, inBuf, offs, nBytes, (i != 0), fastMode);
      if (i == 0)       // the first optimization pass writes no data
        continue;
      // calculate compressed size and hash value
      size_t    compressedSize = headerSize;
      uint64_t  h = 1UL;
      for (size_t j = 0; j < tmpBuf.size(); j++) {
        compressedSize += size_t((tmpBuf[j] & 0x7F000000U) >> 24);
        h = h ^ uint64_t(tmpBuf[j]);
        h = uint32_t(h) * uint64_t(0xC2B0C3CCUL);
        h = (h ^ (h >> 32)) & 0xFFFFFFFFUL;
      }
      h = h | (uint64_t(compressedSize) << 32);
      if (compressedSize < bestSize) {
        // found a better compression, so save it
        nSymbols = tmp;
        bestSize = compressedSize;
        bestBuf.resize(tmpBuf.size());
        for (size_t j = 0; j < tmpBuf.size(); j++)
          bestBuf[j] = tmpBuf[j];
      }
      for (size_t j = 0; j < hashTable.size(); j++) {
        if (hashTable[j] == h) {
          // if the exact same compressed data was already generated earlier,
          // the remaining optimize iterations can be skipped
          doneFlag = true;
          break;
        }
      }
      if (!doneFlag)
        hashTable.push_back(h);         // save hash value
    }
    size_t  uncompressedSize = headerSize + (nBytes * 8);
    tmpOutBuf.push_back(0x10000000U | (startAddr + (unsigned int) offs));
    if (bestSize >= uncompressedSize) {
      // if cannot reduce the data size, store without compression
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (nBytes - 1));
      tmpOutBuf.push_back(0x01000000U | (unsigned int) isLastBlock);
      tmpOutBuf.push_back(0x01000000U);
      for (size_t i = offs; i < endPos; i++)
        tmpOutBuf.push_back(0x88000000U | (unsigned int) inBuf[i]);
    }
    else {
      tmpOutBuf.push_back(0x10000000U | (unsigned int) (nSymbols - 1));
      tmpOutBuf.push_back(0x01000000U | (unsigned int) isLastBlock);
      tmpOutBuf.push_back(0x01000001U);
      // append compressed data to output buffer
      for (size_t i = 0; i < bestBuf.size(); i++)
        tmpOutBuf.push_back(bestBuf[i]);
    }
    return true;
  }

  // --------------------------------------------------------------------------

  Compressor_M2::Compressor_M2(std::vector< unsigned char >& outBuf_)
    : Compressor(outBuf_),
      config(),
      lengthEncodeTable(lengthNumSlots, lengthMaxValue,
                        &(lengthPrefixSizeTable[0])),
      offs1EncodeTable(offs1NumSlots, offs1MaxValue, (size_t *) 0,
                       offs1PrefixSize),
      offs2EncodeTable(offs2NumSlots, offs2MaxValue, (size_t *) 0,
                       offs2PrefixSize),
      offs3EncodeTable(0, offs3MaxValue, (size_t *) 0,
                       2, 5, &(offs3SlotCntTable[0])),
      offs3NumSlots(4),
      offs3PrefixSize(2),
      searchTable((SearchTable *) 0),
      savedOutBufPos(0x7FFFFFFF),
      outputShiftReg(0x00),
      outputBitCnt(0)
  {
  }

  Compressor_M2::~Compressor_M2()
  {
    if (searchTable)
      delete searchTable;
  }

  void Compressor_M2::getCompressionParameters(
      Compressor_M1::CompressionParameters& cfg) const
  {
    cfg = config;
  }

  void Compressor_M2::setCompressionParameters(
      const Compressor_M1::CompressionParameters& cfg)
  {
    config = cfg;
  }

  void Compressor_M2::setCompressionLevel(int n)
  {
    config.setCompressionLevel(n);
  }

  void Compressor_M2::addZeroPageUpdate(unsigned int endAddr, bool isLastBlock)
  {
    std::vector< unsigned char >  tmpBuf;
    tmpBuf.push_back((unsigned char) (endAddr & 0xFFU));
    tmpBuf.push_back((unsigned char) ((endAddr >> 8) & 0xFFU));
    tmpBuf.push_back((unsigned char) (endAddr & 0xFFU));
    tmpBuf.push_back((unsigned char) ((endAddr >> 8) & 0xFFU));
    tmpBuf.push_back((unsigned char) (endAddr & 0xFFU));
    tmpBuf.push_back((unsigned char) ((endAddr >> 8) & 0xFFU));
    compressData(tmpBuf, 0x002DU, false, false);
    tmpBuf.resize(2);
    compressData(tmpBuf, 0x009DU, isLastBlock, false);
  }

  bool Compressor_M2::compressData(const std::vector< unsigned char >& inBuf,
                                   unsigned int startAddr, bool isLastBlock,
                                   bool enableProgressDisplay)
  {
    if (outputBitCnt < 0)
      throw Plus4Emu::Exception("internal error: compressing to closed buffer");
    if (inBuf.size() < 1)
      return true;
    progressDisplayEnabled = enableProgressDisplay;
    try {
      if (enableProgressDisplay) {
        progressMessage("Compressing data");
        setProgressPercentage(0);
      }
      if (searchTable) {
        delete searchTable;
        searchTable = (SearchTable *) 0;
      }
      searchTable = new SearchTable(inBuf);
      // split large files to improve statistical compression
      std::list< SplitOptimizationBlock >   splitPositions;
      std::map< uint64_t, size_t >          splitOptimizationCache;
      size_t  splitDepth = config.splitOptimizationDepth - 1;
      size_t  splitCnt = size_t(1) << splitDepth;
      if (splitCnt > inBuf.size())
        splitCnt = inBuf.size();
      progressCnt = 0;
      progressMax = splitCnt
                    + (splitCnt > 1 ? (splitCnt - 1) : 0)
                    + (splitCnt > 2 ? (splitCnt - 2) : 0)
                    + (splitCnt > 3 ? (splitCnt - 3) : 0);
      progressMax = progressMax * config.optimizeIterations;
      progressMax =
          progressMax * ((splitDepth / 2) + 2) / ((splitDepth / 2) + 1);
      // create initial block list
      for (size_t i = 0; i < splitCnt; i++) {
        SplitOptimizationBlock  tmpBlock;
        tmpBlock.startPos = i * inBuf.size() / splitCnt;
        tmpBlock.nBytes =
            ((i + 1) * inBuf.size() / splitCnt) - tmpBlock.startPos;
        splitPositions.push_back(tmpBlock);
      }
      while (true) {
        size_t  bestMergePos = 0;
        long    bestMergeBits = 0x7FFFFFFFL;
        // find the pair of blocks that reduce the total compressed size
        // the most when merged
        std::list< SplitOptimizationBlock >::iterator curBlock =
            splitPositions.begin();
        while (curBlock != splitPositions.end()) {
          std::list< SplitOptimizationBlock >::iterator nxtBlock = curBlock;
          nxtBlock++;
          if (nxtBlock == splitPositions.end())
            break;
          size_t  nBitsSplit = 0;
          size_t  nBitsMerged = 0;
          for (size_t i = 0; i < 3; i++) {
            // i = 0: merged block, i = 1: first block, i = 2: second block
            size_t  startPos = 0;
            size_t  endPos = 0;
            switch (i) {
            case 0:
              startPos = (*curBlock).startPos;
              endPos = startPos + (*curBlock).nBytes + (*nxtBlock).nBytes;
              break;
            case 1:
              startPos = (*curBlock).startPos;
              endPos = startPos + (*curBlock).nBytes;
              break;
            case 2:
              startPos = (*nxtBlock).startPos;
              endPos = startPos + (*nxtBlock).nBytes;
              break;
            }
            uint64_t  cacheKey = (uint64_t(startPos) << 32) | uint64_t(endPos);
            if (splitOptimizationCache.find(cacheKey)
                == splitOptimizationCache.end()) {
              // if this block is not in the cache yet, compress it,
              // and store the compressed size in the cache
              std::vector< unsigned int > tmpBuf;
              tmpBuf.resize(0);
              if (!compressData(tmpBuf, inBuf, startAddr, false,
                                startPos, endPos - startPos, true)) {
                delete searchTable;
                searchTable = (SearchTable *) 0;
                if (progressDisplayEnabled)
                  progressMessage("");
                return false;
              }
              // calculate compressed size
              size_t  nBits = 0;
              for (size_t j = 0; j < tmpBuf.size(); j++)
                nBits += size_t((tmpBuf[j] & 0x7F000000U) >> 24);
              splitOptimizationCache[cacheKey] = nBits;
            }
            size_t  nBits = splitOptimizationCache[cacheKey];
            switch (i) {
            case 0:
              nBitsMerged = nBits;
              break;
            default:
              nBitsSplit += nBits;
              break;
            }
          }
          // calculate size change when merging blocks
          long    sizeDiff = long(nBitsMerged) - long(nBitsSplit);
          if (sizeDiff < bestMergeBits) {
            bestMergePos = (*curBlock).startPos;
            bestMergeBits = sizeDiff;
          }
          curBlock++;
        }
        if (bestMergeBits > 0L)         // no more blocks can be merged
          break;
        // merge the best pair of blocks and continue
        curBlock = splitPositions.begin();
        while ((*curBlock).startPos != bestMergePos)
          curBlock++;
        std::list< SplitOptimizationBlock >::iterator nxtBlock = curBlock;
        nxtBlock++;
        (*curBlock).nBytes = (*curBlock).nBytes + (*nxtBlock).nBytes;
        splitPositions.erase(nxtBlock);
      }
      // compress all blocks again with full optimization
      std::vector< unsigned int >   outBufTmp;
      progressMax = config.optimizeIterations * splitPositions.size();
      progressCnt = progressMax * ((splitDepth / 2) + 1);
      progressMax = progressMax + progressCnt;
      outBufTmp.resize(0);
      std::list< SplitOptimizationBlock >::iterator i_ = splitPositions.begin();
      while (i_ != splitPositions.end()) {
        std::vector< unsigned int > tmpBuf;
        tmpBuf.resize(0);
        if (!compressData(tmpBuf, inBuf, startAddr,
                          (isLastBlock &&
                           ((*i_).startPos + (*i_).nBytes) >= inBuf.size()),
                          (*i_).startPos, (*i_).nBytes, false)) {
          delete searchTable;
          searchTable = (SearchTable *) 0;
          if (progressDisplayEnabled)
            progressMessage("");
          return false;
        }
        for (size_t i = 0; i < tmpBuf.size(); i++)
          outBufTmp.push_back(tmpBuf[i]);
        i_++;
      }
      delete searchTable;
      searchTable = (SearchTable *) 0;
      if (progressDisplayEnabled) {
        setProgressPercentage(100);
        progressMessage("");
      }
      // pack output data
      if (outBuf.size() == 0)
        outBuf.push_back((unsigned char) 0x00); // reserve space for CRC value
      for (size_t i = 0; i < outBufTmp.size(); i++) {
        unsigned int  c = outBufTmp[i];
        if (c >= 0x80000000U) {
          // special case for literal bytes, which are stored byte-aligned
          if (outputBitCnt > 0 && savedOutBufPos >= outBuf.size()) {
            // reserve space for the shift register to be stored later when
            // it is full, and save the write position
            savedOutBufPos = outBuf.size();
            outBuf.push_back((unsigned char) 0x00);
          }
          unsigned int  nBytes = ((c & 0x7F000000U) + 0x07000000U) >> 27;
          while (nBytes > 0U) {
            nBytes--;
            outBuf.push_back((unsigned char) ((c >> (nBytes * 8U)) & 0xFFU));
          }
        }
        else {
          unsigned int  nBits = c >> 24;
          c = c & 0x00FFFFFFU;
          for (unsigned int j = nBits; j > 0U; ) {
            j--;
            unsigned int  b = (unsigned int) (bool(c & (1U << j)));
            outputShiftReg = ((outputShiftReg & 0x7F) << 1) | (unsigned char) b;
            if (++outputBitCnt >= 8) {
              if (savedOutBufPos >= outBuf.size()) {
                outBuf.push_back(outputShiftReg);
              }
              else {
                // store at saved position if any literal bytes were inserted
                outBuf[savedOutBufPos] = outputShiftReg;
                savedOutBufPos = 0x7FFFFFFF;
              }
              outputShiftReg = 0x00;
              outputBitCnt = 0;
            }
          }
        }
      }
      if (isLastBlock) {
        while (outputBitCnt != 0) {
          outputShiftReg = ((outputShiftReg & 0x7F) << 1);
          if (++outputBitCnt >= 8) {
            if (savedOutBufPos >= outBuf.size()) {
              outBuf.push_back(outputShiftReg);
            }
            else {
              // store at saved position if any literal bytes were inserted
              outBuf[savedOutBufPos] = outputShiftReg;
              savedOutBufPos = 0x7FFFFFFF;
            }
            outputShiftReg = 0x00;
            outputBitCnt = 0;
          }
        }
        // calculate CRC
        unsigned char crcVal = 0xFF;
        for (size_t i = outBuf.size() - 1; i > 0; i--) {
          unsigned int  tmp = (unsigned int) crcVal ^ (unsigned int) outBuf[i];
          tmp = ((tmp << 1) + ((tmp & 0x80U) >> 7) + 0xACU) & 0xFFU;
          crcVal = (unsigned char) tmp;
        }
        crcVal = (unsigned char) ((0x0180 - 0xAC) >> 1) ^ crcVal;
        outBuf[0] = crcVal;
        outputBitCnt = -1;              // set output buffer closed flag
      }
    }
    catch (...) {
      if (searchTable) {
        delete searchTable;
        searchTable = (SearchTable *) 0;
      }
      if (progressDisplayEnabled)
        progressMessage("");
      throw;
    }
    return true;
  }

}       // namespace Plus4Compress
