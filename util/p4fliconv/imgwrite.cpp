
// p4fliconv: high resolution interlaced FLI converter utility
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

#include "p4fliconv.hpp"
#include "prgdata.hpp"
#include "compress.hpp"
#include "p4slib.hpp"
#include "imgwrite.hpp"

#include <vector>

static void defaultProgressMessageCb(void *userData, const char *msg)
{
  (void) userData;
  if (msg != (char *) 0 && msg[0] != '\0')
    std::fprintf(stderr, "%s\n", msg);
}

static bool defaultProgressPercentageCb(void *userData, int n)
{
  (void) userData;
  if (n != 100)
    std::fprintf(stderr, "\r  %3d%%    ", n);
  else
    std::fprintf(stderr, "\r  %3d%%    \n", n);
  return true;
}

namespace Plus4FLIConv {

  void writeConvertedImageFile(
      const char *fileName, PRGData& prgData, unsigned int prgEndAddr,
      int convType, int outputFormat, int compressionLevel,
      void (*progressMessageCallback)(void *userData, const char *msg),
      bool (*progressPercentageCallback)(void *userData, int n),
      void *progressCallbackUserData)
  {
    // check parameters
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid file name");
    if (convType < -1 || convType > 8)
      throw Plus4Emu::Exception("invalid conversion type");
    if (outputFormat < 0 || outputFormat > 3)
      throw Plus4Emu::Exception("invalid output file format");
    if (compressionLevel < 0 || compressionLevel > 9)
      throw Plus4Emu::Exception("invalid compression level");
    if (!progressMessageCallback)
      progressMessageCallback = &defaultProgressMessageCb;
    if (!progressPercentageCallback)
      progressPercentageCallback = &defaultProgressPercentageCb;
    if (convType < 0) {
      // auto-detect conversion type from PRG data
      convType = prgData.getConversionType();
    }
    if (outputFormat == 2) {
      // P4S format
      writeP4SFile(fileName, prgData);
      return;
    }
    std::FILE *f = (std::FILE *) 0;
    try {
      if (outputFormat == 3) {
        // FED 1.5 format
        if (convType != 5 && convType != 7) {
          throw Plus4Emu::Exception("FED format only supports "
                                    "non-interlaced multicolor modes");
        }
        std::vector< unsigned char >  outBuf;
        if (convType == 5) {
          if (prgData.getVerticalSize() != 200) {
            throw Plus4Emu::Exception("FED format only supports "
                                      "160x200 resolution");
          }
          for (int i = 0; i < 200; i++) {
            if (prgData.lineXShift(i << 1) & 0x07)
              throw Plus4Emu::Exception("FED format does not support X shift");
          }
          // write attributes
          for (int i = 0x4018; i < 0x6000; i++)
            outBuf.push_back(prgData[i - 0x0FFF]);
          for (int i = 0; i < 24; i++)
            outBuf.push_back((unsigned char) 0x00);
          // write bitmap data
          for (int i = 0x20C0; i < 0x4000; i++)
            outBuf.push_back(prgData[i - 0x0FFF]);
          for (int i = 0; i < 192; i++)
            outBuf.push_back((unsigned char) 0x00);
          // write background colors
          for (int i = 0; i < 200; i++) {
            outBuf.push_back(prgData.lineColor0(i << 1));
            outBuf.push_back(prgData.lineColor3(i << 1));
          }
        }
        else {
          // convert attributes to FLI
          for (int i = 0; i < 4; i++) {
            for (int j = 0x7800; j < 0x7BE8; j++)
              outBuf.push_back(prgData[j - 0x0FFF]);
            for (int j = 0; j < 24; j++)
              outBuf.push_back((unsigned char) 0x00);
            for (int j = 0x7C00; j < 0x7FE8; j++)
              outBuf.push_back(prgData[j - 0x0FFF]);
            for (int j = 0; j < 24; j++)
              outBuf.push_back((unsigned char) 0x00);
          }
          // write bitmap data
          for (int i = 0x8000; i < 0x9F40; i++)
            outBuf.push_back(prgData[i - 0x0FFF]);
          for (int i = 0; i < 192; i++)
            outBuf.push_back((unsigned char) 0x00);
          // write background colors
          unsigned char color0 = prgData[0x7BFF - 0x0FFF];
          unsigned char color3 = prgData[0x7BFE - 0x0FFF];
          color0 = ((color0 & 0x07) << 4) | ((color0 & 0xF0) >> 4);
          color3 = ((color3 & 0x07) << 4) | ((color3 & 0xF0) >> 4);
          for (int i = 0; i < 200; i++) {
            outBuf.push_back(color0);
            outBuf.push_back(color3);
          }
        }
        f = std::fopen(fileName, "wb");
        if (!f)
          throw Plus4Emu::Exception("error opening output file");
        if (std::fwrite(&(outBuf.front()), sizeof(unsigned char), outBuf.size(),
                        f) != outBuf.size()) {
          throw Plus4Emu::Exception("error writing output file "
                                    "- is the disk full ?");
        }
      }
      else {
        // PRG format
        bool    rawMode = (outputFormat != 0);
        unsigned int  prgStartAddr = 0x1001U;
        if (rawMode)
          prgStartAddr = prgData.getImageDataStartAddress();
        f = std::fopen(fileName, "wb");
        if (!f)
          throw Plus4Emu::Exception("error opening PRG file");
        if (compressionLevel > 0) {
          std::vector< unsigned char >  compressInBuf;
          std::vector< unsigned char >  compressOutBuf;
          PRGCompressor   compress_(compressOutBuf);
          PRGCompressor::CompressionParameters  compressCfg;
          compress_.getCompressionParameters(compressCfg);
          compressCfg.setCompressionLevel(compressionLevel);
          compress_.setCompressionParameters(compressCfg);
          compress_.setProgressMessageCallback(progressMessageCallback,
                                               progressCallbackUserData);
          compress_.setProgressPercentageCallback(progressPercentageCallback,
                                                  progressCallbackUserData);
          if (!rawMode) {
            compress_.addDecompressCode(false);
            compress_.addDecompressEndCode(-1L, false, true, false, false);
            compress_.addZeroPageUpdate(prgEndAddr, false);
          }
          else {
            compress_.addDecompressEndCode(-2L, false, false, false, false);
          }
          unsigned int  startAddr = prgStartAddr;
          if (prgData.getImageDataStartAddress()
              > (prgData.getViewerCodeEndAddress() + 256U) &&
              !rawMode) {
            // compress non-FLI programs with the viewer and image data
            // stored in separate chunks, to avoid having to compress a large
            // empty memory area
            for (unsigned int i = 0x0002U;
                 i < (prgData.getViewerCodeEndAddress() - 0x0FFFU);
                 i++) {
              compressInBuf.push_back(prgData[i]);
            }
            compress_.compressData(compressInBuf, startAddr, false, false);
            startAddr = prgData.getImageDataStartAddress();
            compressInBuf.clear();
          }
          for (unsigned int i = (startAddr - 0x0FFFU);
               i < (prgEndAddr - 0x0FFFU);
               i++) {
            compressInBuf.push_back(prgData[i]);
          }
          if (!compress_.compressData(compressInBuf, startAddr, true, true))
            compressionLevel = 0;
          if (compressionLevel > 0) {
            if (rawMode) {
              if (convType < 6) {
                size_t  nBytes = compressOutBuf.size() & 0xFFFF;
                compressOutBuf.insert(compressOutBuf.begin(),
                                      (unsigned char) (nBytes & 0xFF));
                compressOutBuf.insert(compressOutBuf.begin(),
                                      (unsigned char) (nBytes >> 8));
              }
            }
            compressOutBuf.insert(compressOutBuf.begin(),
                                  (unsigned char) (prgStartAddr >> 8));
            compressOutBuf.insert(compressOutBuf.begin(),
                                  (unsigned char) (prgStartAddr & 0xFFU));
            for (size_t i = 0; i < compressOutBuf.size(); i++) {
              if (std::fputc(int(compressOutBuf[i]), f) == EOF)
                throw Plus4Emu::Exception("error writing PRG file");
            }
          }
        }
        if (compressionLevel < 1) {
          if (std::fputc(int(prgStartAddr & 0xFFU), f) == EOF ||
              std::fputc(int(prgStartAddr >> 8), f) == EOF) {
            throw Plus4Emu::Exception("error writing PRG file");
          }
          for (unsigned int i = prgStartAddr - 0x0FFFU;
               i < (prgEndAddr - 0x0FFFU);
               i++) {
            if (std::fputc(int(prgData[i]), f) == EOF)
              throw Plus4Emu::Exception("error writing PRG file");
          }
        }
      }
      if (std::fflush(f) != 0) {
        throw Plus4Emu::Exception("error writing output file "
                                  "- is the disk full ?");
      }
      std::fclose(f);
      f = (std::FILE *) 0;
    }
    catch (...) {
      if (f) {
        std::fclose(f);
        std::remove(fileName);
      }
      throw;
    }
  }

}       // namespace Plus4FLIConv

