
// compress.cpp: simple compressor utility for Commodore Plus/4 programs
// Copyright (C) 2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <list>

// start address (0 - 0xFFFF), or -1 for run, -2 for basic, or -3 for monitor
static long   runAddr = -2L;
// use all RAM (up to $4000) on the C16
static bool   c16Mode = false;
// compression level (1: fast, low compression ... 9: slow, high compression)
static int    compressionLevel = 5;
// do not clean up after decompression if this is set to true
static bool   noCleanup = false;
// do not enable interrupts after decompression if this is set to true
static bool   noCLI = false;
// do not enable ROM after decompression if this is set to true
static bool   noROM = false;
// do not update zeropage variables after decompression if this is set to true
static bool   noZPUpdate = false;

// compression tuning parameters
static size_t bitsPerByte = 7;
static size_t repeatCodeBits = 7;

static const size_t minRepeatDist = 1;
static const size_t maxRepeatDist = 65536;
static const size_t minRepeatLen = 2;
static const size_t maxRepeatLen = 32769;

static const unsigned char decomp_code[0x0213] = {
  0x01, 0x10, 0x0C, 0x10, 0x0A, 0x00, 0x9E, 0x20, 0x34, 0x31, 0x32, 0x30,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0xAD, 0x06, 0xFF, 0x48, 0xAD, 0x19,
  0xFF, 0x48, 0xA0, 0xB2, 0xB9, 0x53, 0x10, 0x99, 0x24, 0x01, 0xB9, 0x05,
  0x11, 0x99, 0x3C, 0x03, 0xB9, 0x5F, 0x11, 0x99, 0x44, 0x09, 0x88, 0xD0,
  0xEB, 0xA2, 0x01, 0xB5, 0x2D, 0x95, 0xDE, 0xB5, 0x37, 0x95, 0xDA, 0x95,
  0xDC, 0xCA, 0x10, 0xF3, 0x8C, 0x06, 0xFF, 0x8C, 0x3F, 0xFF, 0x4C, 0xA4,
  0x03, 0xA2, 0x00, 0x20, 0x70, 0x03, 0xAA, 0xBD, 0x50, 0x09, 0x85, 0xE2,
  0x85, 0xE4, 0xBD, 0x60, 0x09, 0x85, 0xE3, 0x69, 0x01, 0x85, 0xE5, 0xBD,
  0x70, 0x09, 0xAA, 0x20, 0x70, 0x03, 0xA8, 0xB1, 0xE4, 0x4A, 0xB1, 0xE2,
  0xA0, 0x00, 0x60, 0xA2, 0x09, 0x20, 0x57, 0x03, 0x46, 0xE7, 0x60, 0x20,
  0x88, 0x09, 0xE6, 0xE0, 0xF0, 0x13, 0x20, 0x4B, 0x01, 0xB0, 0x15, 0x91,
  0xDE, 0x8D, 0x19, 0xFF, 0xE6, 0xDE, 0xD0, 0xEE, 0xE6, 0xDF, 0x4C, 0x56,
  0x01, 0xE6, 0xE1, 0xD0, 0xE9, 0x4C, 0x40, 0x03, 0x48, 0x29, 0x38, 0x4A,
  0x4A, 0xAA, 0xE8, 0x20, 0x57, 0x03, 0xA9, 0xFE, 0xE5, 0xE6, 0x85, 0xE2,
  0xA9, 0xFF, 0xE5, 0xE7, 0x85, 0xE3, 0x68, 0x29, 0x07, 0x0A, 0x69, 0x02,
  0xAA, 0x20, 0x57, 0x03, 0xA5, 0xDE, 0x18, 0xE5, 0xE6, 0x85, 0xE4, 0xA5,
  0xDF, 0xE5, 0xE7, 0x85, 0xE5, 0xB1, 0xE4, 0x91, 0xDE, 0x8D, 0x19, 0xFF,
  0xE6, 0xDE, 0xD0, 0x02, 0xE6, 0xDF, 0xE6, 0xE0, 0xD0, 0x04, 0xE6, 0xE1,
  0xF0, 0xBB, 0xE6, 0xE2, 0xD0, 0x04, 0xE6, 0xE3, 0xF0, 0x9C, 0xE6, 0xE4,
  0xD0, 0xDF, 0xE6, 0xE5, 0x4C, 0xA1, 0x01, 0x18, 0x24, 0x38, 0x48, 0xE9,
  0x00, 0xA2, 0x08, 0x0A, 0xB0, 0x03, 0xCA, 0xD0, 0xFA, 0x68, 0x60, 0x20,
  0x6E, 0x03, 0x20, 0x6E, 0x03, 0x85, 0xDF, 0x20, 0x6E, 0x03, 0x85, 0xDE,
  0x20, 0x6E, 0x03, 0x85, 0xE1, 0x20, 0x6E, 0x03, 0x85, 0xE0, 0x4C, 0x53,
  0x01, 0xE0, 0x09, 0x90, 0x0A, 0x8A, 0xE9, 0x08, 0xAA, 0x20, 0x70, 0x03,
  0xA2, 0x08, 0x24, 0x98, 0x85, 0xE7, 0x20, 0x70, 0x03, 0x85, 0xE6, 0x60,
  0xA2, 0x08, 0x8A, 0xF0, 0x0B, 0x98, 0xC6, 0xD9, 0x30, 0x09, 0x06, 0xD8,
  0x2A, 0xCA, 0xD0, 0xF6, 0xC9, 0x00, 0x60, 0x48, 0xA5, 0xDA, 0xC5, 0xDC,
  0xF0, 0x16, 0xA5, 0xE8, 0x85, 0xD8, 0xA9, 0x07, 0x85, 0xD9, 0xE6, 0xDA,
  0xD0, 0x02, 0xE6, 0xDB, 0xB1, 0xDA, 0x85, 0xE8, 0x68, 0x4C, 0x78, 0x03,
  0xA5, 0xDB, 0xC5, 0xDD, 0xD0, 0xE4, 0x84, 0xD9, 0x86, 0xE8, 0xA5, 0xDE,
  0xF0, 0x14, 0xC9, 0x12, 0xD0, 0x12, 0xA5, 0xDF, 0xC9, 0x12, 0xD0, 0x0C,
  0xA5, 0xE8, 0xF0, 0x83, 0x8C, 0x3E, 0xFF, 0x6C, 0xFC, 0xFF, 0xC6, 0xDF,
  0xC6, 0xDE, 0xA5, 0xDA, 0xD0, 0x02, 0xC6, 0xDB, 0xC6, 0xDA, 0xB1, 0xDE,
  0x91, 0xDA, 0x45, 0xE8, 0x0A, 0x69, 0xC4, 0x85, 0xE8, 0x4C, 0xA8, 0x03,
  0x20, 0x6E, 0x03, 0xF0, 0x64, 0x20, 0xC7, 0x01, 0x8E, 0x26, 0x01, 0xA2,
  0x25, 0x8E, 0x5B, 0x01, 0x84, 0xE2, 0x85, 0xE3, 0x84, 0xE4, 0xA9, 0x08,
  0x85, 0xE5, 0xA6, 0xE2, 0xA5, 0xE4, 0x9D, 0x50, 0x09, 0xA5, 0xE5, 0x9D,
  0x60, 0x09, 0x20, 0x6E, 0x03, 0x20, 0xC9, 0x01, 0x38, 0x65, 0xE4, 0x85,
  0xE4, 0x90, 0x02, 0xE6, 0xE5, 0x8A, 0xA6, 0xE2, 0x9D, 0x70, 0x09, 0xE6,
  0xE2, 0xC6, 0xE3, 0xD0, 0xD9, 0x84, 0xE2, 0xA9, 0x08, 0x85, 0xE3, 0x20,
  0x4B, 0x01, 0x91, 0xE2, 0x98, 0x2A, 0xE6, 0xE3, 0xE6, 0xE3, 0x91, 0xE2,
  0xE6, 0xE2, 0xF0, 0x02, 0xC6, 0xE3, 0xC6, 0xE3, 0xA5, 0xE2, 0xC5, 0xE4,
  0xD0, 0xE5, 0xA5, 0xE3, 0xC5, 0xE5, 0xD0, 0xDF, 0x60, 0xA2, 0x4B, 0x8E,
  0x5B, 0x01, 0x60
};

static const unsigned char decomp_code_2[0x0027] = {
  0xA2, 0xFD, 0x9A, 0x68, 0x8D, 0x19, 0xFF, 0xAD, 0x3B, 0x05, 0x99, 0x00,
  0x08, 0x99, 0x00, 0x09, 0x99, 0x00, 0x0A, 0x99, 0x40, 0x0A, 0xC8, 0xD0,
  0xF1, 0x68, 0x8D, 0x06, 0xFF, 0x8C, 0x3E, 0xFF, 0x58, 0x20, 0xBE, 0x8B,
  0x4C, 0xDC, 0x8B
};

// ----------------------------------------------------------------------------

class CharCountTable {
 private:
  std::vector< unsigned int >   charValues;
  std::vector< size_t >         charCounts;
  std::vector< size_t >         charCountsSum;
  size_t  nCharValues;
  size_t  totalBits;
  bool    tableSortedFlag;
 public:
  CharCountTable();
  virtual ~CharCountTable();
  inline void addChar(unsigned int n)
  {
    if (n <= 0x01FFU) {
      tableSortedFlag = false;
      charCounts[n]++;
    }
  }
  inline size_t charCount(size_t n) const
  {
    return charCounts[n];
  }
  inline size_t charRangeCount(size_t n0, size_t n1) const
  {
    return (charCountsSum[n1] - charCountsSum[n0]);
  }
  inline unsigned int charValue(size_t n) const
  {
    return charValues[n];
  }
  inline size_t getTotalBitCount() const
  {
    return totalBits;
  }
  inline size_t size() const
  {
    return nCharValues;
  }
  void sortTable();
};

CharCountTable::CharCountTable()
  : nCharValues(0),
    totalBits(0),
    tableSortedFlag(true)
{
  charValues.resize(512);
  charCounts.resize(512);
  charCountsSum.resize(513);
  for (size_t i = 0; i < 512; i++) {
    charValues[i] = (unsigned int) i;
    charCounts[i] = 0;
    charCountsSum[i] = 0;
  }
  charCountsSum[512] = 0;
}

CharCountTable::~CharCountTable()
{
}

void CharCountTable::sortTable()
{
  if (tableSortedFlag)
    return;
  for (size_t i = 511; i > 0; i--) {
    for (size_t j = 0; j < i; j++) {
      if (charCounts[j] < charCounts[j + 1]) {
        {
          size_t  tmp = charCounts[j];
          charCounts[j] = charCounts[j + 1];
          charCounts[j + 1] = tmp;
        }
        {
          unsigned int  tmp = charValues[j];
          charValues[j] = charValues[j + 1];
          charValues[j + 1] = tmp;
        }
      }
    }
  }
  nCharValues = 0;
  totalBits = 0;
  for (size_t i = 0; i < 512; i++) {
    charCountsSum[i] = totalBits;
    if (charCounts[i] > 0) {
      nCharValues++;
      totalBits += charCounts[i];
    }
  }
  charCountsSum[512] = totalBits;
  totalBits = totalBits * 9;
  tableSortedFlag = true;
}

static long tryCompression(CharCountTable& tbl,
                           const std::vector< int >& bitTable,
                           int nPrefixBits)
{
  long    bitsSaved = (-1L - long(bitTable.size())) * 8L;
  if (bitTable.size() < 1)
    return bitsSaved;
  {
    // filter out obvious bad tables to improve performance
    size_t  cnt1 = 0;
    size_t  cnt2 = 0;
    for (size_t i = 0; i < bitTable.size(); i++) {
      cnt1 = cnt2;
      cnt2 = cnt2 + (size_t(1) << bitTable[i]);
    }
    if (cnt1 >= tbl.size() || cnt2 < tbl.size())
      return -(0x7FFFFFFFL);
  }
  bitsSaved += long(tbl.getTotalBitCount());    // get original size
  size_t  n = 0;                                // calculate compressed size
  for (size_t i = 0; i < bitTable.size(); i++) {
    int     nBits = bitTable[i];
    size_t  nn = n + (size_t(1) << nBits);
    if (nn > tbl.size())
      nn = tbl.size();
    bitsSaved -= long((tbl.charRangeCount(n, nn) * size_t(nPrefixBits + nBits))
                      + ((nn - n) * 9));
    n = nn;
  }
  return bitsSaved;
}

static long compressBlock(std::vector< unsigned int >& ioBuf)
{
  CharCountTable  tbl;
  for (size_t i = 0; i < ioBuf.size(); i++)
    tbl.addChar(ioBuf[i]);
  tbl.sortTable();
  std::vector< int >  bitTable;
  std::vector< int >  bestBitTable;
  long    bestCompression = tryCompression(tbl, bitTable, 0);
  int     maxPrefixBits = 3;
  if (compressionLevel <= 1)
    maxPrefixBits = 2;
  if (compressionLevel >= 9)
    maxPrefixBits = 4;
  // find the best statistical compression
  for (int i = 0; i <= maxPrefixBits; i++) {
    int     n = 1 << i;
    bitTable.resize(size_t(n));
    for (int j = 0; j < n; j++)
      bitTable[j] = 0;
    bool    doneFlag = false;
    do {
      long    bitsSaved = tryCompression(tbl, bitTable, i);
      if (bitsSaved > bestCompression) {
        bestCompression = bitsSaved;
        bestBitTable.resize(size_t(n));
        for (int j = 0; j < n; j++)
          bestBitTable[j] = bitTable[j];
      }
      if (bitTable[0] == 8)
        break;
      int     j = n - 1;
      while (j >= 0 && bitTable[j] == 8)
        j--;
      bitTable[j]++;
      for (int k = j + 1; k < n; k++)
        bitTable[k] = bitTable[j];
      // stop if there are already too many possible values
      // with this table size
      if (j == 0) {
        size_t  tmp = 0;
        for (size_t k = 0; k < bitTable.size(); k++) {
          tmp = tmp + (size_t(1) << bitTable[k]);
          if (tmp >= tbl.size() && (k + 1) < bitTable.size()) {
            doneFlag = true;
            break;
          }
        }
      }
    } while (!doneFlag);
  }
  // insert the decode table after the start address and number of bytes
  std::vector< unsigned int >::iterator   i_ = ioBuf.begin();
  for (size_t i = 0; i < 4 && i_ != ioBuf.end(); i++)
    i_++;
  if (bestBitTable.size() > 0) {
    unsigned int  table1[512];      // prefix value for each 9 bit character
    unsigned int  table2[512];      // index value for each 9 bit character
    // count the number of unique character values
    int     nCharValues = int(tbl.size());
    int     n = 0;
    for (size_t i = 0; i < bestBitTable.size(); i++) {
      n = n + (1 << bestBitTable[i]);
      if (n >= nCharValues) {
        i_ = ioBuf.insert(i_, (unsigned int) (i + 1) | 0x08000000U);
        i_++;
        break;
      }
    }
    n = 0;
    for (size_t i = 0; i < bestBitTable.size(); i++) {
      int     tmp = 1 << bestBitTable[i];
      if ((n + tmp) > nCharValues) {
        tmp = nCharValues - n;
        if (tmp <= 0)
          break;
      }
      for (int j = n; j < (n + tmp); j++)
        table1[tbl.charValue(size_t(j))] = (unsigned int) i;
      n = n + tmp;
      i_ = ioBuf.insert(i_, (unsigned int) (tmp - 1) | 0x08000000U);
      i_++;
    }
    n = 0;
    for (size_t i = 0; i < bestBitTable.size(); i++) {
      int     tmp = 1 << bestBitTable[i];
      if ((n + tmp) > nCharValues) {
        tmp = nCharValues - n;
        if (tmp <= 0)
          break;
      }
      for (int j = n; j < (n + tmp); j++) {
        table2[tbl.charValue(size_t(j))] = (unsigned int) (j - n);
        i_ = ioBuf.insert(i_, (unsigned int) tbl.charValue(size_t(j))
                              | 0x09000000U);
        i_++;
      }
      n = n + tmp;
    }
    int     m = 0;              // calculate the number of prefix bits
    while ((1 << m) < int(bestBitTable.size()))
      m++;
    for (size_t i = 0; i < ioBuf.size(); i++) {
      unsigned int  c = ioBuf[i];
      if (c > 0x01FFU)
        continue;
      unsigned int  tmp1 = table1[c];           // prefix value
      unsigned int  tmp2 = table2[c];           // index value
      int           nBits = bestBitTable[tmp1]; // number of index bits
      ioBuf[i] = (unsigned int) ((m + nBits) << 24) | (tmp1 << nBits) | tmp2;
    }
  }
  else {
    // store 9-bit characters without any statistical compression
    i_ = ioBuf.insert(i_, 0x08000000U);
    i_++;
    for (size_t i = 0; i < ioBuf.size(); i++) {
      if (ioBuf[i] <= 0x01FFU)
        ioBuf[i] = ioBuf[i] | 0x09000000U;
    }
  }
  return bestCompression;
}

// ----------------------------------------------------------------------------

static void writeRepeatCode(std::vector< unsigned int >& buf,
                            size_t d, size_t n)
{
  unsigned int  c = 0x0100U;
  int     lBits = 1;
  int     dBits = 2;
  while (n > ((size_t(1) << lBits) + 1)) {
    lBits += 2;
  }
  while (d > (size_t(1) << dBits)) {
    dBits += 2;
  }
  c = c | ((((unsigned int) lBits - 1U) >> 1) << 3)
        | (((unsigned int) dBits - 2U) >> 1);
  buf.push_back(c);
  buf.push_back((unsigned int) ((lBits << 24) | (n - minRepeatLen)));
  buf.push_back((unsigned int) ((dBits << 24) | (d - minRepeatDist)));
}

static size_t getRepeatCodeLength(size_t d, size_t n)
{
  int     lBits = 1;
  int     dBits = 2;
  while (n > ((size_t(1) << lBits) + 1)) {
    lBits += 2;
  }
  while (d > (size_t(1) << dBits)) {
    dBits += 2;
  }
  return (size_t(lBits + dBits) + repeatCodeBits);
}

static long compressData_(
    std::vector< unsigned int >& outBuf,
    const std::vector< unsigned char >& inBuf,
    const std::vector< std::list< unsigned int > >& searchTable,
    unsigned int startAddr, size_t offs, size_t nBytes)
{
  size_t  endPos = offs + nBytes;
  long    bitsSaved = long((endPos - offs) * 9);
  // write data header (start address and 1's complement of the number of bytes)
  outBuf.push_back(0x08000000U | (unsigned int) (((startAddr + offs) >> 8)
                                                 & 0xFF));
  outBuf.push_back(0x08000000U | (unsigned int) ((startAddr + offs) & 0xFF));
  outBuf.push_back(0x08000000U | (unsigned int) (((~nBytes) >> 8) & 0xFF));
  outBuf.push_back(0x08000000U | (unsigned int) ((~nBytes) & 0xFF));
  // compress data by searching for repeated byte sequences,
  // and replacing them with length/distance codes
  for (size_t i = offs; i < endPos; ) {
    size_t  bestPos = 0;
    size_t  bestLen = 0;
    long    bestComp = -1000L;
    if (i > 0 && (i + 1) < endPos) {
      unsigned int  n = (((unsigned int) inBuf[i] << 8)
                         | (unsigned int) inBuf[i + 1]) & 0xFFFFU;
      const std::list< unsigned int >&  searchList = searchTable[n];
      std::list< unsigned int >::const_iterator i_ = searchList.begin();
      size_t  j = i;
      while (i_ != searchList.end()) {
        size_t  tmp = size_t(*i_);
        if ((tmp + maxRepeatDist) >= i) {
          j = tmp;
          break;
        }
        i_++;
      }
      size_t  minLen = minRepeatLen;
      while (j < i) {
        size_t  k = i;
        size_t  l = j;
        do {
          if (inBuf[k] != inBuf[l])
            break;
          if (++l >= i)
            l = j;
        } while (++k < endPos);
        if (compressionLevel < 8 && (k - i) >= 256) {
          do {
            i_++;
          } while (i_ != searchList.end() && size_t(*i_) < k);
        }
        else
          i_++;
        k -= i;
        if (k >= minLen) {
          k = (k < maxRepeatLen ? k : maxRepeatLen);
          size_t  d = i - j;
          long    tmp = long(k * bitsPerByte) - getRepeatCodeLength(d, k);
          if (tmp >= bestComp) {
            bestPos = d;
            bestLen = k;
            bestComp = tmp;
            if (k > 8)
              minLen = k - 6;
          }
        }
        if (i_ == searchList.end())
          break;
        j = size_t(*i_);
      }
    }
    if (bestComp >= 0L) {
      writeRepeatCode(outBuf, bestPos, bestLen);
      i = i + bestLen;
    }
    else {
      outBuf.push_back((unsigned int) inBuf[i]);
      i++;
    }
  }
  // apply statistical compression
  compressBlock(outBuf);
  // return the amount of size reduction in bits
  for (size_t i = 0; i < outBuf.size(); i++)
    bitsSaved -= long(outBuf[i] >> 24);
  return bitsSaved;
}

static void compressData(std::vector< unsigned int >& outBuf,
                         const std::vector< unsigned char >& inBuf,
                         unsigned int startAddr,
                         size_t offs = 0,
                         size_t nBytes = 0xFFFFFFFFUL)
{
  // the 'offs' and 'nBytes' parameters allow compressing a buffer
  // as multiple chunks for possibly improved statistical compression
  if (offs >= inBuf.size())
    return;
  if (nBytes > (inBuf.size() - offs))
    nBytes = inBuf.size() - offs;
  size_t  endPos = offs + nBytes;
  // create a lookup table of all two-byte strings for faster compression
  std::vector< std::list< unsigned int > >  searchTable;
  searchTable.resize(0x10000);
  for (size_t i = 0; (i + 1) < endPos; i++) {
    unsigned int  n = (((unsigned int) inBuf[i] << 8)
                       | (unsigned int) inBuf[i + 1]) & 0xFFFFU;
    searchTable[n].push_back((unsigned int) i);
  }
  // find the best combination of compression parameters
  size_t  bestBitsPerByte = 7;
  size_t  bestRepeatCodeBits = 7;
  long    bestCompression = -(0x7FFFFFFFL);
  std::vector< unsigned int >   tmpBuf;
  if (compressionLevel >= 3) {
    for (size_t i = 4; i <= 8; i++) {
      for (size_t j = (compressionLevel >= 7 ? 6 : 7);
           j <= (compressionLevel >= 7 ? 9 : 7);
           j++) {
        bitsPerByte = i;
        repeatCodeBits = j;
        tmpBuf.resize(0);
        long    bitsSaved = compressData_(tmpBuf, inBuf, searchTable,
                                          startAddr, offs, nBytes);
        if (bitsSaved > bestCompression) {
          bestBitsPerByte = bitsPerByte;
          bestRepeatCodeBits = repeatCodeBits;
          bestCompression = bitsSaved;
        }
        if (nBytes >= 4096)
          std::fprintf(stderr, ".");
      }
    }
  }
  bitsPerByte = bestBitsPerByte;
  repeatCodeBits = bestRepeatCodeBits;
  tmpBuf.resize(0);
  compressData_(tmpBuf, inBuf, searchTable, startAddr, offs, nBytes);
  // append compressed data to output buffer
  for (size_t i = 0; i < tmpBuf.size(); i++)
    outBuf.push_back(tmpBuf[i]);
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv)
{
  std::vector< std::string >  fileNames;
  bool    printUsageFlag = false;
  for (int i = 1; i < argc; i++) {
    std::string tmp = argv[i];
    if (tmp == "-h" || tmp == "-help" || tmp == "--help") {
      printUsageFlag = true;
    }
    else if (tmp.length() == 2 &&
             (tmp[0] == '-' && tmp[1] >= '1' && tmp[1] <= '9')) {
      compressionLevel = int(tmp[1] - '0');
    }
    else if (tmp == "-c16") {
      c16Mode = true;
    }
    else if (tmp == "-nocleanup") {
      noCleanup = true;
    }
    else if (tmp == "-nocli") {
      noCLI = true;
    }
    else if (tmp == "-norom") {
      noROM = true;
    }
    else if (tmp == "-nozp") {
      noZPUpdate = true;
    }
    else if (tmp == "-start") {
      runAddr = -1L;
      if ((i + 1) < argc) {
        i++;
        tmp = argv[i];
        runAddr = long(std::atoi(tmp.c_str()));
      }
    }
    else {
      fileNames.push_back(tmp);
    }
  }
  if (printUsageFlag || fileNames.size() < 2) {
    std::fprintf(stderr, "Usage: %s [OPTIONS...] <infile...> <outfile>\n",
                 argv[0]);
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "    -1 ... -9\n");
    std::fprintf(stderr, "        set compression level vs. speed (default: "
                         "5)\n");
    std::fprintf(stderr, "    -c16\n");
    std::fprintf(stderr, "        generate decompression code for the C16\n");
    std::fprintf(stderr, "    -nocleanup\n");
    std::fprintf(stderr, "        do not clean up after decompression "
                         "(slightly reduces size)\n");
    std::fprintf(stderr, "    -nocli\n");
    std::fprintf(stderr, "        do not enable interrupts after "
                         "decompression\n");
    std::fprintf(stderr, "    -norom\n");
    std::fprintf(stderr, "        do not enable ROM after decompression\n");
    std::fprintf(stderr, "    -nozp\n");
    std::fprintf(stderr, "        do not update zeropage variables at $2D-$32 "
                         "and $9D-$9E\n");
    std::fprintf(stderr, "    -start <ADDR>\n");
    std::fprintf(stderr, "        start program at address ADDR (decimal), or "
                         "RUN if ADDR is -1,\n        return to basic if -2 "
                         "(default), or monitor if ADDR is -3\n");
    return (printUsageFlag ? 0 : -1);
  }
  std::vector< unsigned char >  outBuf;
  std::vector< unsigned int >   outBufTmp;
  for (size_t i = 0; i < 0x0213; i++)
    outBuf.push_back(decomp_code[i]);
  if (c16Mode) {
    outBuf[0x0A] = 0x31;
    outBuf[0x0B] = 0x32;
    outBuf[0x11] = 0xA9;
    outBuf[0x12] = 0x00;
    outBuf[0x13] = 0x85;
    outBuf[0x14] = 0x37;
    outBuf[0x15] = 0xA9;
    outBuf[0x16] = 0x40;
    outBuf[0x17] = 0x85;
    outBuf[0x18] = 0x38;
  }
  std::vector< unsigned char >  inBuf;
  {
    std::vector< unsigned char >  tmpBuf;
    // add autostart module
    for (size_t i = 0; i < size_t(runAddr == -1L ? 0x0027 : 0x0021); i++) {
      if ((noCleanup && (i >= 0x0004 && i <= 0x0018)) ||
          (noCLI && i == 0x0020) ||
          (noROM && (i >= 0x001D && i <= 0x001F))) {
        continue;
      }
      tmpBuf.push_back(decomp_code_2[i]);
    }
    if (runAddr != -1L) {
      // if not 'run':
      if (!(runAddr >= 0L && runAddr <= 0xFFFFL)) {
        if (runAddr == -2L)
          runAddr = 0x867EL;            // 'ready'
        else
          runAddr = 0xFF52L;            // monitor
      }
      tmpBuf.push_back((unsigned char) 0x4C);
      tmpBuf.push_back((unsigned char) (runAddr & 0xFFL));
      tmpBuf.push_back((unsigned char) ((runAddr >> 8) & 0xFFL));
    }
    compressData(outBufTmp, tmpBuf, 0x03A4U);
  }
  for (int i = 0; i < int(fileNames.size() - 1); i++) {
    inBuf.resize(0);
    std::FILE *f = std::fopen(fileNames[i].c_str(), "rb");
    if (!f) {
      std::fprintf(stderr, " *** %s: error opening input file '%s'\n",
                   argv[0], fileNames[i].c_str());
      return -1;
    }
    unsigned int  startAddr = 0U;
    int     c;
    c = std::fgetc(f);
    if (c == EOF) {
      std::fclose(f);
      return -1;
    }
    startAddr = (unsigned int) c & 0xFFU;
    c = std::fgetc(f);
    if (c == EOF) {
      std::fclose(f);
      return -1;
    }
    startAddr = startAddr | (((unsigned int) c & 0xFFU) << 8);
    while (true) {
      c = std::fgetc(f);
      if (c == EOF)
        break;
      inBuf.push_back((unsigned char) c);
    }
    std::fclose(f);
    f = (std::FILE *) 0;
    if (i == int(fileNames.size() - 2) && !noZPUpdate) {
      unsigned int  endAddr = startAddr + (unsigned int) inBuf.size();
      // update zeropage variables according to the last file packed
      std::vector< unsigned char >  tmpBuf;
      tmpBuf.resize(6);
      tmpBuf[0] = (unsigned char) (endAddr & 0xFFU);
      tmpBuf[1] = (unsigned char) ((endAddr >> 8) & 0xFFU);
      tmpBuf[2] = tmpBuf[0];
      tmpBuf[3] = tmpBuf[1];
      tmpBuf[4] = tmpBuf[0];
      tmpBuf[5] = tmpBuf[1];
      compressData(outBufTmp, tmpBuf, 0x002DU);
      tmpBuf.resize(2);
      compressData(outBufTmp, tmpBuf, 0x009DU);
    }
    std::fprintf(stderr, "%s:\n", fileNames[i].c_str());
    // split large files to improve statistical compression
    size_t  bestSplit = 1;
    size_t  bestSize = 0xFFFFFFFFUL;
    size_t  maxSplit = inBuf.size() >> 10;
    size_t  splitFailCnt = 0;
    if (maxSplit > size_t((compressionLevel * 2) - 2))
      maxSplit = size_t((compressionLevel * 2) - 2);
    if (maxSplit < 1)
      maxSplit = 1;
    for (size_t splitCnt = 1; splitCnt <= maxSplit; splitCnt++) {
      std::vector< unsigned int >   tmpBuf;
      tmpBuf.resize(0);
      for (size_t j = 0; j < splitCnt; j++) {
        std::fprintf(stderr, "\r  %3d%%                      "
                             "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
                     int((splitCnt - 1) * 100 / maxSplit));
        size_t  startPos = (inBuf.size() * j) / splitCnt;
        size_t  endPos = (inBuf.size() * (j + 1)) / splitCnt;
        compressData(tmpBuf, inBuf, startAddr, startPos, endPos - startPos);
      }
      size_t  newSize = 0;
      for (size_t j = 0; j < tmpBuf.size(); j++)
        newSize += size_t(tmpBuf[j] >> 24);
      if (newSize < bestSize) {
        bestSplit = splitCnt;
        bestSize = newSize;
        splitFailCnt = 0;
      }
      else if (++splitFailCnt >= 8)
        break;
    }
    for (size_t j = 0; j < bestSplit; j++) {
      std::fprintf(stderr, "\r   99%%                      "
                           "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
      size_t  startPos = (inBuf.size() * j) / bestSplit;
      size_t  endPos = (inBuf.size() * (j + 1)) / bestSplit;
      compressData(outBufTmp, inBuf, startAddr, startPos, endPos - startPos);
    }
    std::fprintf(stderr, "\r  100%%                      \n");
  }
  // pack output data
  size_t  crcPos = outBuf.size();
  outBuf.push_back((unsigned char) 0x00);       // reserve space for CRC value
  unsigned char  shiftReg = 0x00;
  unsigned char  bitCnt = 8;
  for (size_t i = 0; i < outBufTmp.size(); i++) {
    unsigned int  c = outBufTmp[i];
    unsigned int  nBits = c >> 24;
    c = c & 0x00FFFFFFU;
    for (unsigned int j = nBits; j > 0U; ) {
      j--;
      unsigned int  b = (unsigned int) (bool(c & (1U << j)));
      shiftReg = ((shiftReg & 0x7F) << 1) | (unsigned char) b;
      if (--bitCnt == 0) {
        outBuf.push_back(shiftReg);
        shiftReg = 0x00;
        bitCnt = 8;
      }
    }
  }
  while (bitCnt != 8) {
    shiftReg = ((shiftReg & 0x7F) << 1);
    if (--bitCnt == 0) {
      outBuf.push_back(shiftReg);
      shiftReg = 0x00;
      bitCnt = 8;
    }
  }
  // calculate CRC
  unsigned char crcVal = 0xFF;
  for (size_t i = outBuf.size() - 1; i > crcPos; i--) {
    unsigned int  tmp = (unsigned int) crcVal ^ (unsigned int) outBuf[i];
    tmp = ((tmp << 1) + ((tmp & 0x80U) >> 7) + 0xC4U) & 0xFFU;
    crcVal = (unsigned char) tmp;
  }
  crcVal = (unsigned char) ((256 - 0xC4) >> 1) ^ crcVal;
  outBuf[crcPos] = crcVal;
  // write output file
  std::FILE *f = std::fopen(fileNames[fileNames.size() - 1].c_str(), "wb");
  if (!f) {
    std::fprintf(stderr, " *** %s: error opening output file\n", argv[0]);
    return -1;
  }
  for (size_t i = 0; i < outBuf.size(); i++) {
    std::fputc(int(outBuf[i]), f);
  }
  std::fclose(f);
  return 0;
}

