
// compress.cpp: simple compressor utility for Commodore Plus/4 programs
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

#include "p4fliconv/compress.hpp"
#include "p4fliconv/compress.cpp"

#include <string>

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
// do not include decompressor code in the PRG output
static bool   noDecompCode = false;

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
    else if (tmp == "-raw") {
      noDecompCode = true;
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
    std::fprintf(stderr, "    -raw\n");
    std::fprintf(stderr, "        write the compressed data only\n");
    std::fprintf(stderr, "    -start <ADDR>\n");
    std::fprintf(stderr, "        start program at address ADDR (decimal), or "
                         "RUN if ADDR is -1,\n        return to basic if -2 "
                         "(default), or monitor if ADDR is -3\n");
    return (printUsageFlag ? 0 : -1);
  }
  std::vector< unsigned char >  outBuf;
  std::vector< unsigned char >  inBuf;
  Plus4FLIConv::PRGCompressor   compress(outBuf);
  Plus4FLIConv::PRGCompressor::CompressionParameters  cfg;
  compress.getCompressionParameters(cfg);
  if (!noDecompCode)
    compress.addDecompressCode(c16Mode);
  compress.addDecompressEndCode(runAddr, false, noCleanup, noCLI, noROM);
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
    unsigned int  endAddr = startAddr + (unsigned int) inBuf.size();
    if (i == int(fileNames.size() - 2) && !noZPUpdate)
      compress.addZeroPageUpdate(endAddr, false);
    std::fprintf(stderr, "%s: ", fileNames[i].c_str());
    cfg.setCompressionLevel(compressionLevel);
    compress.setCompressionParameters(cfg);
    compress.compressData(inBuf, startAddr,
                          (i == int(fileNames.size() - 2)), true);
    cfg.setCompressionLevel(1);
    compress.setCompressionParameters(cfg);
  }
  // write output file
  unsigned int  startAddr = 0x1001U;
  if (noDecompCode)
    startAddr = compress.getCompressedDataStartAddress();
  outBuf.insert(outBuf.begin(), (unsigned char) ((startAddr >> 8) & 0xFFU));
  outBuf.insert(outBuf.begin(), (unsigned char) (startAddr & 0xFFU));
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

