
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

#include "plus4emu.hpp"
#include "p4fliconv/compress.hpp"
#include "p4fliconv/decompress.hpp"

#include <vector>

// extract compressed file
static bool   extractMode = false;
// test compressed file(s)
static bool   testMode = false;
// compression level (1: fast, low compression ... 9: slow, high compression)
static int    compressionLevel = 5;
// use all RAM (up to $4000) on the C16
static bool   c16Mode = false;
// use fast self-extracting module (does not verify checksum, no read buffer)
static bool   useFastSFXModule = false;
// do not clean up after decompression if this is set to true
static bool   noCleanup = false;
// do not enable interrupts after decompression if this is set to true
static bool   noCLI = false;
// do not enable ROM after decompression if this is set to true
static bool   noROM = false;
// do not update zeropage variables after decompression if this is set to true
static bool   noZPUpdate = false;
// do not include decompressor code in the PRG output if non-zero
static int    rawLoadAddr = 0;
// do not read or write PRG/P00 header
static bool   noPRGMode = false;
// write output file as p4fliconv raw compressed image
static bool   fliImageFormat = false;
// override the load address of the first input file if greater than zero
static int    loadAddr = 0;
// start address (0 - 0xFFFF), or -1 for run, -2 for basic, or -3 for monitor
static long   runAddr = -2L;

static void readInputFile(std::vector< unsigned char >& buf,
                          unsigned int& startAddress,
                          const char *fileName)
{
  startAddress = 0U;
  buf.clear();
  if (fileName == (char *) 0 || fileName[0] == '\0')
    throw Plus4Emu::Exception("invalid input file name");
  std::FILE *f = (std::FILE *) 0;
  int       fileType = 0;       // 0: raw, 1: .prg, 2: .p00, 3: .r00/.s00/.u00
  try {
    f = std::fopen(fileName, "rb");
    if (!f)
      throw Plus4Emu::Exception("error opening input file");
    if (!noPRGMode) {
      uint8_t tmpBuf[28];
      size_t  bytesRead = std::fread(&(tmpBuf[0]), sizeof(uint8_t), 28, f);
      size_t  nameLen = std::strlen(fileName);
      fileType = 1;
      if (nameLen >= 4 && bytesRead >= 26) {
        if (fileName[nameLen - 4] == '.' &&
            ((fileName[nameLen - 3] | char(0x20)) == 'p' ||
             (fileName[nameLen - 3] | char(0x20)) == 'r' ||
             (fileName[nameLen - 3] | char(0x20)) == 's' ||
             (fileName[nameLen - 3] | char(0x20)) == 'u') &&
            fileName[nameLen - 2] >= '0' && fileName[nameLen - 2] <= '9' &&
            fileName[nameLen - 1] >= '0' && fileName[nameLen - 1] <= '9') {
          if (tmpBuf[0] == 0x43 && tmpBuf[1] == 0x36 && tmpBuf[2] == 0x34 &&
              tmpBuf[3] == 0x46 && tmpBuf[4] == 0x69 && tmpBuf[5] == 0x6C &&
              tmpBuf[6] == 0x65 && tmpBuf[7] == 0x00) {
            fileType = ((fileName[nameLen - 3] | char(0x20)) == 'p' ? 2 : 3);
          }
        }
      }
      if ((fileType == 1 && bytesRead < 2) || (fileType == 2 && bytesRead < 28))
        throw Plus4Emu::Exception("unexpected end of input file");
      if (fileType == 1) {                      // .prg
        startAddress =
            (unsigned int) tmpBuf[0] | ((unsigned int) tmpBuf[1] << 8);
        for (size_t i = 2; i < bytesRead; i++)
          buf.push_back((unsigned char) tmpBuf[i]);
      }
      else if (fileType == 2) {                 // .p00
        startAddress =
            (unsigned int) tmpBuf[26] | ((unsigned int) tmpBuf[27] << 8);
      }
      else {                                    // .r00/.s00/.u00
        for (size_t i = 26; i < bytesRead; i++)
          buf.push_back((unsigned char) tmpBuf[i]);
      }
    }
    while (true) {
      int     c = std::fgetc(f);
      if (c == EOF)
        break;
      buf.push_back((unsigned char) (c & 0xFF));
    }
    std::fclose(f);
    f = (std::FILE *) 0;
  }
  catch (...) {
    if (f)
      std::fclose(f);
    throw;
  }
}

int main(int argc, char **argv)
{
  std::vector< std::string >  fileNames;
  bool    printUsageFlag = false;
  for (int i = 1; i < argc; i++) {
    std::string tmp = argv[i];
    if (tmp == "-h" || tmp == "-help" || tmp == "--help") {
      printUsageFlag = true;
    }
    else if (tmp == "-x") {
      extractMode = true;
      testMode = false;
    }
    else if (tmp == "-t") {
      testMode = true;
      extractMode = false;
    }
    else if (tmp.length() == 2 &&
             (tmp[0] == '-' && tmp[1] >= '1' && tmp[1] <= '9')) {
      compressionLevel = int(tmp[1] - '0');
    }
    else if (tmp == "-c16") {
      c16Mode = true;
    }
    else if (tmp == "-fastsfx") {
      useFastSFXModule = true;
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
    else if (tmp == "-zp") {
      noZPUpdate = false;
    }
    else if (tmp == "-raw") {
      rawLoadAddr = -1;
      if ((i + 1) < argc) {
        i++;
        tmp = argv[i];
        rawLoadAddr = int(std::atoi(tmp.c_str()));
      }
      if (rawLoadAddr >= 0)
        rawLoadAddr = rawLoadAddr & 0xFFFF;
      else
        rawLoadAddr = 0x1003;
      noZPUpdate = true;
    }
    else if (tmp == "-noprg") {
      noPRGMode = true;
    }
    else if (tmp == "-fli") {
      fliImageFormat = true;
      rawLoadAddr = 0x17FE;
      noZPUpdate = true;
    }
    else if (tmp == "-loadaddr") {
      loadAddr = 0;
      if ((i + 1) < argc) {
        i++;
        tmp = argv[i];
        loadAddr = int(std::atoi(tmp.c_str()));
      }
      if (loadAddr >= 0)
        loadAddr = loadAddr & 0xFFFF;
      else
        loadAddr = 0;
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
  if (printUsageFlag || fileNames.size() < (testMode ? 1 : 2)) {
    std::fprintf(stderr, "Usage:\n");
    std::fprintf(stderr, "%s [OPTIONS...] <infile...> <outfile>\n", argv[0]);
    std::fprintf(stderr, "    compress file(s)\n");
    std::fprintf(stderr, "%s -x [OPTIONS...] <infile> <outfile...>\n", argv[0]);
    std::fprintf(stderr, "    extract compressed file (experimental)\n");
    std::fprintf(stderr, "%s -t <infile...>\n", argv[0]);
    std::fprintf(stderr, "    test compressed file(s)\n");
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "    -1 ... -9\n");
    std::fprintf(stderr, "        set compression level vs. speed (default: "
                         "5)\n");
    std::fprintf(stderr, "    -c16\n");
    std::fprintf(stderr, "        generate decompression code for the C16\n");
    std::fprintf(stderr, "    -fastsfx\n");
    std::fprintf(stderr, "        use faster decompression code (does not "
                         "verify checksum, no read\n");
    std::fprintf(stderr, "        buffer (unsafe if end address is $FD00))\n");
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
    std::fprintf(stderr, "    -zp\n");
    std::fprintf(stderr, "        update zeropage variables at $2D-$32 and "
                         "$9D-$9E\n");
    std::fprintf(stderr, "    -raw <LOADADDR>\n");
    std::fprintf(stderr, "        write the compressed data only "
                         "(implies -nozp)\n");
    std::fprintf(stderr, "    -noprg\n");
    std::fprintf(stderr, "        read and write raw files without PRG or P00 "
                         "header\n");
    std::fprintf(stderr, "        (implies -raw and -nozp)\n");
    std::fprintf(stderr, "    -fli\n");
    std::fprintf(stderr, "        compress p4fliconv raw FLI image "
                         "(implies -raw and -nozp)\n");
    std::fprintf(stderr, "    -loadaddr <ADDR>\n");
    std::fprintf(stderr, "        override the load address of the first "
                         "input file\n");
    std::fprintf(stderr, "    -start <ADDR>\n");
    std::fprintf(stderr, "        start program at address ADDR (decimal), or "
                         "RUN if ADDR is -1,\n        return to basic if -2 "
                         "(default), or monitor if ADDR is -3\n");
    return (printUsageFlag ? 0 : -1);
  }
  std::FILE *f = (std::FILE *) 0;
  try {
    if (testMode) {
      // test compressed file(s)
      noPRGMode = true;
      std::vector< unsigned char >  inBuf;
      bool    errorFlag = false;
      for (size_t i = 0; i < fileNames.size(); i++) {
        std::printf("%s: ", fileNames[i].c_str());
        inBuf.clear();
        unsigned int  startAddr = 0U;
        readInputFile(inBuf, startAddr, fileNames[i].c_str());
        if (inBuf.size() < 1) {
          errorFlag = true;
          std::printf("FAILED (empty file)\n");
        }
        else {
          std::vector< std::vector< unsigned char > > tmpBuf;
          try {
            Plus4FLIConv::PRGDecompressor decompressor;
            decompressor.decompressData(tmpBuf, inBuf);
            std::printf("OK\n");
          }
          catch (Plus4Emu::Exception) {
            errorFlag = true;
            std::printf("FAILED\n");
          }
        }
      }
      return (errorFlag ? -1 : 0);
    }
    if (extractMode) {
      // extract compressed file
      bool    savedNoPRGMode = noPRGMode;
      noPRGMode = true;
      std::vector< unsigned char >  inBuf;
      unsigned int  startAddr = 0U;
      readInputFile(inBuf, startAddr, fileNames[0].c_str());
      if (inBuf.size() < 1)
        throw Plus4Emu::Exception("empty input file");
      noPRGMode = savedNoPRGMode;
      std::vector< std::vector< unsigned char > > outBufs;
      Plus4FLIConv::PRGDecompressor decompressor;
      decompressor.decompressData(outBufs, inBuf);
      size_t  n = 1;
      for (size_t i = 0; i < outBufs.size(); i++) {
        if (outBufs[i].size() < 2)
          continue;
        startAddr =
            (unsigned int) outBufs[i][0] | ((unsigned int) outBufs[i][1] << 8);
        if (startAddr == 0x096DU || startAddr == 0x09DDU ||
            startAddr == 0x03E7U) {
          continue;
        }
        if (loadAddr > 0) {
          startAddr = (unsigned int) loadAddr;
          loadAddr = 0;
        }
        if (n >= fileNames.size())
          throw Plus4Emu::Exception("too few output file names");
        try {
          f = std::fopen(fileNames[n].c_str(), "wb");
          if (!f)
            throw Plus4Emu::Exception("cannot open output file");
          if (!noPRGMode) {
            if (std::fputc(int(startAddr & 0xFFU), f) == EOF)
              throw Plus4Emu::Exception("error writing output file");
            if (std::fputc(int((startAddr >> 8) & 0xFFU), f) == EOF)
              throw Plus4Emu::Exception("error writing output file");
          }
          for (size_t j = 2; j < outBufs[i].size(); j++) {
            if (std::fputc(int(outBufs[i][j]), f) == EOF)
              throw Plus4Emu::Exception("error writing output file");
          }
          if (std::fflush(f) != 0)
            throw Plus4Emu::Exception("error writing output file");
          std::fclose(f);
          f = (std::FILE *) 0;
        }
        catch (...) {
          if (f) {
            std::fclose(f);
            f = (std::FILE *) 0;
            std::remove(fileNames[n].c_str());
          }
          throw;
        }
        n++;
      }
      if (n < fileNames.size()) {
        if (n == 1)
          throw Plus4Emu::Exception("empty input file");
        throw Plus4Emu::Exception("too many output file names");
      }
      return 0;
    }
    // compress file(s)
    if (noPRGMode) {
      if (fileNames.size() > 2) {
        throw Plus4Emu::Exception("-noprg mode does not support "
                                  "multiple input files");
      }
      if (fliImageFormat) {
        throw Plus4Emu::Exception("cannot use -noprg and -fli "
                                  "at the same time");
      }
      rawLoadAddr = 1;                  // not used, but must be > 0
      noZPUpdate = true;
    }
    std::vector< unsigned char >  outBuf;
    std::vector< unsigned char >  inBuf;
    Plus4FLIConv::PRGCompressor   compress(outBuf);
    Plus4FLIConv::PRGCompressor::CompressionParameters  cfg;
    compress.getCompressionParameters(cfg);
    for (int i = 0; i < int(fileNames.size() - 1); i++) {
      inBuf.resize(0);
      unsigned int  startAddr = 0U;
      readInputFile(inBuf, startAddr, fileNames[i].c_str());
      if (inBuf.size() < 1)
        throw Plus4Emu::Exception("empty input file");
      if (loadAddr > 0 && !noPRGMode) {
        startAddr = (unsigned int) loadAddr;
        loadAddr = 0;
      }
      unsigned int  endAddr = startAddr + (unsigned int) inBuf.size();
      if (fliImageFormat &&
          (endAddr < 0x6000U || endAddr > 0xE500U ||
           !(startAddr == 0x1800U ||
             (startAddr == 0x17FEU && inBuf[0] == 0x00 && inBuf[1] == 0x00)))) {
        throw Plus4Emu::Exception("input file is not a p4fliconv image");
      }
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
    f = std::fopen(fileNames[fileNames.size() - 1].c_str(), "wb");
    if (!f)
      throw Plus4Emu::Exception("cannot open output file");
    if (rawLoadAddr <= 0) {
      std::vector< unsigned char >  sfxBuf;
      Plus4FLIConv::PRGDecompressor::getSFXModule(sfxBuf, runAddr, c16Mode,
                                                  useFastSFXModule, noCleanup,
                                                  noROM, noCLI);
      for (size_t i = 0; i < sfxBuf.size(); i++) {
        if (std::fputc(int(sfxBuf[i]), f) == EOF)
          throw Plus4Emu::Exception("error writing output file");
      }
    }
    else if (!noPRGMode) {
      if (std::fputc(rawLoadAddr & 0xFF, f) == EOF)
        throw Plus4Emu::Exception("error writing output file");
      if (std::fputc((rawLoadAddr >> 8) & 0xFF, f) == EOF)
        throw Plus4Emu::Exception("error writing output file");
    }
    if (fliImageFormat) {
      size_t  nBytes = outBuf.size();
      if (std::fputc(int((nBytes >> 8) & 0xFF), f) == EOF)
        throw Plus4Emu::Exception("error writing output file");
      if (std::fputc(int(nBytes & 0xFF), f) == EOF)
        throw Plus4Emu::Exception("error writing output file");
    }
    for (size_t i = 0; i < outBuf.size(); i++) {
      if (std::fputc(int(outBuf[i]), f) == EOF)
        throw Plus4Emu::Exception("error writing output file");
    }
    if (std::fflush(f) != 0)
      throw Plus4Emu::Exception("error writing output file");
    std::fclose(f);
    f = (std::FILE *) 0;
  }
  catch (std::exception& e) {
    if (f) {
      std::fclose(f);
      std::remove(fileNames[fileNames.size() - 1].c_str());
    }
    std::fprintf(stderr, " *** %s: %s\n", argv[0], e.what());
    return -1;
  }
  return 0;
}

