
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

#ifndef PLUS4EMU_VIDEOREC_HPP
#define PLUS4EMU_VIDEOREC_HPP

#include "plus4emu.hpp"
#include "display.hpp"
#include "snd_conv.hpp"

namespace Plus4Emu {

  class VideoCapture {
   public:
    static const int  videoWidth = 384;
    static const int  videoHeight = 288;
    static const int  frameRate = 25;
    static const int  sampleRate = 48000;
   private:
    class AudioConverter_ : public AudioConverterHighQuality {
     private:
      VideoCapture& videoCapture;
     public:
      AudioConverter_(VideoCapture& videoCapture_,
                      float inputSampleRate_, float outputSampleRate_,
                      float dcBlockFreq1 = 10.0f, float dcBlockFreq2 = 10.0f,
                      float ampScale_ = 0.7071f);
      virtual ~AudioConverter_();
     protected:
      virtual void audioOutput(int16_t left, int16_t right);
    };
    std::FILE   *aviFile;
    uint32_t    *videoBuf;              // 384x288 YUV
    uint8_t     *lineBuf;               // 720 bytes
    uint8_t     *frameBuf0Y;            // 384x288
    uint8_t     *frameBuf0V;            // 192x144
    uint8_t     *frameBuf0U;            // 192x144
    uint8_t     *frameBuf1Y;            // 384x288
    uint8_t     *frameBuf1V;            // 192x144
    uint8_t     *frameBuf1U;            // 192x144
    int32_t     *interpBufY;            // 384x288
    int32_t     *interpBufV;            // 192x144
    int32_t     *interpBufU;            // 192x144
    uint8_t     *outBufY;               // 384x288
    uint8_t     *outBufV;               // 192x144
    uint8_t     *outBufU;               // 192x144
    int16_t     *audioBuf;              // 8 * (sampleRate / frameRate) samples
    int         audioBufReadPos;
    int         audioBufWritePos;
    int         audioBufSamples;        // write position - read position
    int         audioBufSize;           // sampleRate / frameRate
    size_t      clockFrequency;
    int64_t     timesliceLength;
    int64_t     curTime;
    int64_t     frame0Time;
    int64_t     frame1Time;
    int32_t     soundOutputAccumulator;
    int         cycleCnt;
    int32_t     interpTime;
    int         curLine;
    int         vsyncCnt;
    bool        oddFrame;
    uint8_t     burstValue;
    unsigned int  syncLengthCnt;
    unsigned int  hsyncCnt;
    unsigned int  hsyncPeriodLength;
    unsigned int  lineLengthCnt;
    unsigned int  lineLength;
    unsigned int  lineStart;
    unsigned int  hsyncPeriodMin;
    unsigned int  hsyncPeriodMax;
    unsigned int  lineLengthMin;
    unsigned int  lineLengthMax;
    float       lineLengthFilter;
    int         vsyncThreshold1;
    int         vsyncThreshold2;
    int         vsyncReload;
    int         lineReload;
    size_t      lineBufBytes;
    size_t      lineBufLength;
    uint8_t     lineBufFlags;
    size_t      framesWritten;
    VideoDisplay::DisplayParameters displayParameters;
    AudioConverter                  *audioConverter;
    VideoDisplayColormap<uint32_t>  colormap;
    // ----------------
    void lineDone();
    void decodeLine();
    void frameDone();
    void resampleFrame();
    void writeFrame();
    void writeAVIHeader();
    void closeFile();
    static void aviHeader_writeFourCC(uint8_t*& bufp, const char *s);
    static void aviHeader_writeUInt16(uint8_t*& bufp, uint16_t n);
    static void aviHeader_writeUInt32(uint8_t*& bufp, uint32_t n);
   public:
    VideoCapture(void indexToYUVFunc(uint8_t color, bool isNTSC,
                                     float& y, float& u, float& v) =
                     (void (*)(uint8_t, bool, float&, float&, float&)) 0);
    virtual ~VideoCapture();
    void runOneCycle(const uint8_t *videoInput, int16_t audioInput);
    void setClockFrequency(size_t freq_);
    void setNTSCMode(bool ntscMode);
    void openFile(const char *fileName);
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_VIDEOREC_HPP

