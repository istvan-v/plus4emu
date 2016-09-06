
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

#ifndef P4COMPRESS_COMPRESS1_HPP
#define P4COMPRESS_COMPRESS1_HPP

#include "plus4emu.hpp"
#include "compress.hpp"

#include <vector>

namespace Plus4Compress {

  class Compressor_M1 : public Compressor {
   public:
    struct CompressionParameters {
      size_t  optimizeIterations;
      size_t  splitOptimizationDepth;
      CompressionParameters();
      CompressionParameters(const CompressionParameters& r);
      ~CompressionParameters()
      {
      }
      CompressionParameters& operator=(const CompressionParameters& r);
      void setCompressionLevel(int n);
    };
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 65535;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
    static const size_t maxSequenceDist = 32;   // for matches with delta value
    static const size_t seqOffsetBits = 5;
    static const unsigned int lengthMaxValue = 65535U;
    static const size_t lengthNumSlots = 8;
    static const size_t lengthPrefixSizeTable[lengthNumSlots];
    static const unsigned int offs1MaxValue = 4096U;
    static const size_t offs1NumSlots = 4;
    static const size_t offs1PrefixSize = 2;
    static const unsigned int offs2MaxValue = 16384U;
    static const size_t offs2NumSlots = 8;
    static const size_t offs2PrefixSize = 3;
    static const unsigned int offs3MaxValue = (unsigned int) maxRepeatDist;
    static const size_t offs3SlotCntTable[4];
    static const size_t literalSequenceMinLength = lengthNumSlots + 9;
    // --------
   public:
    class EncodeTable {
     private:
      size_t    nSlots;
      size_t    nSymbols;
      size_t    nSymbolsUsed;
      size_t    nSymbolsEncoded;
      size_t    totalSlotWeight;
      size_t    unusedSymbolSize;
      size_t    minPrefixSize;
      size_t    maxPrefixSize;
      size_t    prefixOnlySymbolCnt;
      std::vector< size_t >   prefixSlotCntTable;
      std::vector< size_t >   slotPrefixSizeTable;
      std::vector< size_t >   slotWeightTable;
      std::vector< size_t >   slotBitsTable;
      std::vector< unsigned int >   slotBaseSymbolTable;
      std::vector< unsigned int >   symbolCntTable;
      std::vector< unsigned int >   unencodedSymbolCostTable;
      std::vector< unsigned short > symbolSlotNumTable;
      std::vector< unsigned short > symbolSizeTable;
      void setPrefixSize(size_t n);
      inline size_t calculateEncodedSize() const;
      inline size_t calculateEncodedSize(size_t firstSlot,
                                         unsigned int firstSymbol,
                                         size_t baseSize) const;
      size_t optimizeSlotBitsTable_fast();
      size_t optimizeSlotBitsTable();
     public:
      // If 'slotPrefixSizeTable_' is non-NULL, a variable prefix length
      // encoding is generated with 'nSlots_' slots, and the table is expected
      // to contain the prefix size in bits for each slot.
      // Otherwise, if 'minPrefixSize_' is greater than or equal to
      // 'maxPrefixSize_', a fixed prefix size of 'minPrefixSize_' bits will be
      // used with 'nSlots_' slots, and 'nSlots_' must be less than or equal to
      // 2 ^ 'minPrefixSize_'.
      // Finally, if 'maxPrefixSize_' is greater than 'minPrefixSize_', then
      // all fixed prefix sizes in the specified range are tried, and the one
      // that results in the smallest encoded size will be used. The number of
      // slots, which must be less than or equal to 2 ^ prefix_size, can be
      // specified for each prefix size in 'prefixSlotCntTable_' (the number of
      // elements is 'maxPrefixSize_' + 1 - 'minPrefixSize_'); if the table is
      // NULL, then the number of slots defaults to the maximum possible value
      // (2 ^ prefix_size).
      // In all cases, 'nSymbols_' is the highest value to be encoded + 1, so
      // the valid range will be 0 to 'nSymbols_' - 1.
      EncodeTable(size_t nSlots_, size_t nSymbols_,
                  const size_t *slotPrefixSizeTable_ = (size_t *) 0,
                  size_t minPrefixSize_ = 4,
                  size_t maxPrefixSize_ = 0,
                  const size_t *prefixSlotCntTable_ = (size_t *) 0);
      virtual ~EncodeTable();
      inline void addSymbol(unsigned int n, size_t unencodedCost = 16384)
      {
        symbolCntTable[n] += 1U;
        unencodedSymbolCostTable[n] += (unsigned int) unencodedCost;
        if (size_t(n) >= nSymbolsUsed)
          nSymbolsUsed = size_t(n) + 1;
      }
      inline void addPrefixOnlySymbol()
      {
        prefixOnlySymbolCnt++;
      }
      inline size_t getSymbolSize(unsigned int n) const
      {
        if (size_t(n) >= nSymbolsEncoded)
          return unusedSymbolSize;
        return symbolSizeTable[n];
      }
      inline size_t getSymbolSlotIndex(unsigned int n) const
      {
        if (size_t(n) >= nSymbolsEncoded)
          throw Plus4Emu::Exception("internal error: encoding invalid symbol");
        return size_t(symbolSlotNumTable[n]);
      }
      inline unsigned int encodeSymbol(unsigned int n) const
      {
        size_t  slotNum = getSymbolSlotIndex(n);
        return ((unsigned int) (slotBitsTable[slotNum] << 24)
                | (n - slotBaseSymbolTable[slotNum]));
      }
      inline size_t getSlotCnt() const
      {
        return slotBitsTable.size();
      }
      inline size_t getSlotPrefixSize(size_t n) const
      {
        return slotPrefixSizeTable[n];
      }
      inline size_t getSlotSize(size_t n) const
      {
        return slotBitsTable[n];
      }
      void updateTables(bool fastMode = false);
      void clear();
    };
    // --------
   private:
    class SearchTable {
     private:
      const std::vector< unsigned char >&   buf;
      // buffer positions sorted alphabetically
      // by bytes at each position (buf.size() elements)
      std::vector< unsigned int >   suffixArray;
      // suffixArray[n] matches prvMatchLenTable[n] characters with
      // suffixArray[n - 1] (buf.size() elements)
      std::vector< unsigned short > prvMatchLenTable;
      // suffixArray[n] matches nxtMatchLenTable[n] characters with
      // suffixArray[n + 1] (buf.size() elements)
      std::vector< unsigned short > nxtMatchLenTable;
      // for each match length N and buffer position P,
      // bestMatchPosTable[N - minRepeatLen][P] is the position of the nearest
      // match (minRepeatDist to maxRepeatDist), or zero if there is no match
      // of that length
      // ((maxRepeatLen + 1 - minRepeatLen) * buf.size() elements)
      std::vector< std::vector< unsigned short > >  bestMatchPosTable;
      // maximum match length for each buffer position (buf.size() elements)
      std::vector< unsigned short > maxMatchLenTable;
      std::vector< std::vector< unsigned char > >   seqDiffTable;
      std::vector< std::vector< unsigned short > >  maxSeqLenTable;
      std::vector< unsigned short > rleLengthTable;
      void sortFunc(size_t startPos, size_t endPos,
                    std::vector< unsigned int >& tmpBuf);
     public:
      SearchTable(const std::vector< unsigned char >& inBuf);
      virtual ~SearchTable();
      inline size_t getMaxMatchLength(size_t bufPos) const
      {
        return size_t(maxMatchLenTable[bufPos]);
      }
      inline size_t getDistanceForMatchLength(size_t bufPos, size_t len) const
      {
        return size_t(bestMatchPosTable[len
                                        - Compressor_M1::minRepeatLen][bufPos]);
      }
      inline size_t getMaxSequenceLength(size_t bufPos) const
      {
        return size_t(maxSeqLenTable[0][bufPos]);
      }
      inline size_t getSequenceLength(size_t bufPos, size_t d) const
      {
        return size_t(maxSeqLenTable[d + 1
                                     - Compressor_M1::minRepeatDist][bufPos]);
      }
      inline unsigned char getSequenceDeltaValue(size_t bufPos, size_t d) const
      {
        return seqDiffTable[d - Compressor_M1::minRepeatDist][bufPos];
      }
      inline size_t getRLELength(size_t bufPos) const
      {
        return size_t(rleLengthTable[bufPos]);
      }
    };
    // --------
    struct LZMatchParameters {
      unsigned short  d;
      unsigned short  len;
      unsigned short  nBits;
      bool            seqFlag;
      unsigned char   seqDiff;
      LZMatchParameters()
        : d(0),
          len(1),
          nBits(9),
          seqFlag(false),
          seqDiff(0x00)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len),
          nBits(r.nBits),
          seqFlag(r.seqFlag),
          seqDiff(r.seqDiff)
      {
      }
      ~LZMatchParameters()
      {
      }
      inline LZMatchParameters& operator=(const LZMatchParameters& r)
      {
        d = r.d;
        len = r.len;
        nBits = r.nBits;
        seqFlag = r.seqFlag;
        seqDiff = r.seqDiff;
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
        nBits = 9;
        seqFlag = false;
        seqDiff = 0x00;
      }
    };
    struct SplitOptimizationBlock {
      size_t  startPos;
      size_t  nBytes;
    };
    // --------
    CompressionParameters config;
    EncodeTable   lengthEncodeTable;
    EncodeTable   offs1EncodeTable;
    EncodeTable   offs2EncodeTable;
    EncodeTable   offs3EncodeTable;
    size_t        offs3NumSlots;
    size_t        offs3PrefixSize;
    SearchTable   *searchTable;
    size_t        savedOutBufPos;
    unsigned char outputShiftReg;
    int           outputBitCnt;
    // --------
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getRepeatCodeLength(size_t d, size_t n) const;
    void writeSequenceCode(std::vector< unsigned int >& buf,
                           unsigned char seqDiff, size_t d, size_t n);
    inline size_t getSequenceCodeLength(size_t d, size_t n) const;
    inline void findBestMatch(LZMatchParameters& p, size_t i, size_t maxLen);
    size_t compressData_(std::vector< unsigned int >& tmpOutBuf,
                         const std::vector< unsigned char >& inBuf,
                         size_t offs, size_t nBytes, bool optimizeEncodeTables,
                         bool fastMode = false);
    bool compressData(std::vector< unsigned int >& tmpOutBuf,
                      const std::vector< unsigned char >& inBuf,
                      unsigned int startAddr, bool isLastBlock,
                      size_t offs = 0, size_t nBytes = 0x7FFFFFFFUL,
                      bool fastMode = false);
   public:
    Compressor_M1(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M1();
    void getCompressionParameters(CompressionParameters& cfg) const;
    void setCompressionParameters(const CompressionParameters& cfg);
    virtual void setCompressionLevel(int n);
    virtual void addZeroPageUpdate(unsigned int endAddr, bool isLastBlock);
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Plus4Compress

#endif  // P4COMPRESS_COMPRESS1_HPP
