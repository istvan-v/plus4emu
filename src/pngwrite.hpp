
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/plus4emu/
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

#ifndef PLUS4EMU_PNGWRITE_HPP
#define PLUS4EMU_PNGWRITE_HPP

#include "plus4emu.hpp"
#include <vector>

namespace Plus4Emu {

  class Compressor_ZLib {
   public:
    static const size_t minMatchDist = 1;
    static const size_t maxMatchDist = 32768;
    static const size_t minMatchLen = 3;
    static const size_t maxMatchLen = 258;
    // --------------------------------
    class HuffmanEncoder {
     private:
      size_t  minSymbolCnt;
      size_t  symbolRangeUsed;
      std::vector< unsigned int > symbolCounts;
      std::vector< unsigned int > encodeTable;
      void    *nodeBuf;
     public:
      HuffmanEncoder(size_t maxSymbolCnt_ = 256, size_t minSymbolCnt_ = 0);
      virtual ~HuffmanEncoder();
      // Create encode table and reset symbol counts.
      // If codeLengthTable is not NULL, it specifies a preset encoding.
      void updateTables(bool reverseBits = true, size_t maxCodeLen = 16,
                        const unsigned char *codeLengthTable =
                            (unsigned char *) 0);
      // Send symbol count information to 'symLenEncoder' for Huffman
      // encoding the RLE compressed symbol length table of this encoder.
      // To add complete decoding information to a Deflate stream, the
      // following steps are needed:
      //   huffmanEncoderL.updateDeflateSymLenCnts(symLenEncoder);
      //   huffmanEncoderD.updateDeflateSymLenCnts(symLenEncoder);
      //   symLenEncoder.updateTables(true, 7);
      //   outBuf.push_back(0x0E000000
      //                    | (huffmanEncoderL.getSymbolRangeUsed() - 257))
      //                    | ((huffmanEncoderD.getSymbolRangeUsed() - 1)) << 5)
      //                    | ((symLenEncoder.getSymbolRangeUsed() - 4)) << 10);
      //   symLenEncoder.writeDeflateEncoding(outBuf);
      //   huffmanEncoderL.writeDeflateEncoding(outBuf, symLenEncoder);
      //   huffmanEncoderD.writeDeflateEncoding(outBuf, symLenEncoder);
      void updateDeflateSymLenCnts(HuffmanEncoder& symLenEncoder) const;
      // write Huffman encoding to 'outBuf' in Deflate format
      void writeDeflateEncoding(std::vector< unsigned int >& outBuf) const;
      void writeDeflateEncoding(std::vector< unsigned int >& outBuf,
                                const HuffmanEncoder& symLenEncoder) const;
      inline void addSymbol(unsigned int c)
      {
        symbolCounts[c]++;
      }
      inline unsigned int encodeSymbol(unsigned int c) const
      {
        if (encodeTable[c] == 0U)
          throw Exception("internal error in HuffmanEncoder::encodeSymbol()");
        return encodeTable[c];
      }
      inline size_t getSymbolSize(unsigned int c) const
      {
        if (encodeTable[c] == 0U)
          return 0x3FFF;
        return size_t((encodeTable[c] >> 24) & 0x7FU);
      }
      inline size_t getSymbolRangeUsed() const
      {
        return symbolRangeUsed;
      }
      // reset symbol counts and clear encode table
      void clear();
    };
    // --------------------------------
    class SearchTable {
     private:
      // for each buffer position P, matchTableBuf[matchTable[P]] is the
      // first element of an array of interleaved length/offset pairs,
      // terminated with zero length and offset
      std::vector< unsigned int >   matchTable;
      // space allocated for matchTable
      std::vector< unsigned short > matchTableBuf;
      size_t  matchTableBufPos;
      static void sortFunc(unsigned int *suffixArray,
                           const unsigned char *buf, size_t bufSize,
                           size_t startPos, size_t endPos,
                           unsigned int *tmpBuf);
      void addMatch(size_t bufPos, size_t matchLen, size_t offs);
     public:
      SearchTable(const unsigned char *buf, size_t bufSize);
      virtual ~SearchTable();
      inline const unsigned short * getMatches(size_t bufPos) const
      {
        return (&(matchTableBuf.front()) + matchTable[bufPos]);
      }
    };
    // --------
    struct LZMatchParameters {
      unsigned short  d;
      unsigned short  len;
      LZMatchParameters()
        : d(0),
          len(1)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len)
      {
      }
      ~LZMatchParameters()
      {
      }
      inline LZMatchParameters& operator=(const LZMatchParameters& r)
      {
        d = r.d;
        len = r.len;
        return (*this);
      }
      inline void clear()
      {
        d = 0;
        len = 1;
      }
    };
    // --------------------------------
   private:
    SearchTable     *searchTable;
    // for literals and length codes (size = 288)
    HuffmanEncoder  huffmanEncoderL;
    // for distance codes (size = 32)
    HuffmanEncoder  huffmanEncoderD;
    // for symbol length codes in the header (size = 19)
    HuffmanEncoder  huffmanEncoderC;
    // --------
    // initialize Huffman encoders with the fixed codes
    void setDefaultEncoding();
    void writeMatchCode(std::vector< unsigned int >& buf, size_t d, size_t n);
    inline size_t getLengthCodeLength(size_t n) const;
    inline size_t getDistanceCodeLength(size_t d) const;
    inline size_t getMatchCodeLength(size_t n, size_t d) const;
    void optimizeMatches(LZMatchParameters *matchTable,
                         const unsigned char *inBuf, size_t *bitCountTable,
                         size_t *offsSumTable, size_t offs, size_t nBytes);
    void compressData(std::vector< unsigned int >& outBuf,
                      const unsigned char *inBuf, size_t offs, size_t nBytes,
                      bool firstPass);
   public:
    Compressor_ZLib();
    virtual ~Compressor_ZLib();
    void compressDataBlock(std::vector< unsigned int >& outBuf,
                           const unsigned char *inBuf, size_t offs,
                           size_t nBytes, size_t bufSize, bool isLastBlock);
    // 'blockSize' must be a power of two and not greater than 32768,
    // the optimal value depends on the input data
    static void compressData(std::vector< unsigned char >& outBuf,
                             const unsigned char *inBuf, size_t inBufSize,
                             size_t blockSize = 16384);
  };

  // ==========================================================================

  /*!
   * Save a PNG format image to 'fileName' from the data in 'inBuf'.
   * 'w' and 'h' are the image width and height in pixels, 'nColors' is the
   * palette size (0 to 256, 0 means RGB format), and 'blockSize' is the block
   * size to be used by Compressor_ZLib::compressData(). If 'optimizePalette'
   * is true, unused colors are removed from the palette.
   * The size of 'inBuf' should be nColors * 3 + w * h bytes if a palette is
   * used, and w * h * 3 in the case of RGB format. If a palette is present,
   * it is expected to be at the beginning of 'inBuf' as nColors * 3
   * interleaved R, G, B values.
   */
  void writePNGImage(const char *fileName,
                     const unsigned char *inBuf, int w, int h, int nColors,
                     bool optimizePalette = false, size_t blockSize = 16384);

}       // namespace Plus4Emu

#endif  // PLUS4EMU_PNGWRITE_HPP

