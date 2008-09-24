
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

#ifndef P4COMPRESS_COMPRESS2_HPP
#define P4COMPRESS_COMPRESS2_HPP

#include "plus4emu.hpp"
#include "compress.hpp"
#include "compress1.hpp"

#include <vector>

namespace Plus4Compress {

  class Compressor_M2 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 65535;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
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
                                        - Compressor_M2::minRepeatLen][bufPos]);
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
      LZMatchParameters()
        : d(0),
          len(1),
          nBits(9)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len),
          nBits(r.nBits)
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
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
        nBits = 9;
      }
    };
    struct SplitOptimizationBlock {
      size_t  startPos;
      size_t  nBytes;
    };
    // --------
    Compressor_M1::CompressionParameters  config;
    Compressor_M1::EncodeTable  lengthEncodeTable;
    Compressor_M1::EncodeTable  offs1EncodeTable;
    Compressor_M1::EncodeTable  offs2EncodeTable;
    Compressor_M1::EncodeTable  offs3EncodeTable;
    size_t        offs3NumSlots;
    size_t        offs3PrefixSize;
    SearchTable   *searchTable;
    size_t        savedOutBufPos;
    unsigned char outputShiftReg;
    int           outputBitCnt;
    // --------
    void writeRepeatCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getRepeatCodeLength(size_t d, size_t n) const;
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
    Compressor_M2(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M2();
    void getCompressionParameters(
        Compressor_M1::CompressionParameters& cfg) const;
    void setCompressionParameters(
        const Compressor_M1::CompressionParameters& cfg);
    virtual void setCompressionLevel(int n);
    virtual void addZeroPageUpdate(unsigned int endAddr, bool isLastBlock);
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
  };

}       // namespace Plus4Compress

#endif  // P4COMPRESS_COMPRESS2_HPP

