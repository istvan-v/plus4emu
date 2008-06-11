
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "plus4emu.hpp"
#include "system.hpp"
#include "soundio.hpp"

#include <sndfile.h>
#include <portaudio.h>
#include <iostream>
#include <vector>

#ifdef ENABLE_SOUND_DEBUG

static bool isPortAudioError(const char *msg, PaError paError)
{
  std::cerr << " === " << msg << std::endl;
  if (paError == paNoError)
    return false;
  std::cerr << " *** PortAudio error (error code = " << int(paError) << ")"
            << std::endl;
  std::cerr << " ***   " << Pa_GetErrorText(paError) << std::endl;
#  ifndef USING_OLD_PORTAUDIO_API
  if (paError == paUnanticipatedHostError) {
    const PaHostErrorInfo   *errInfo = Pa_GetLastHostErrorInfo();
    std::cerr << " *** host API error:" << std::endl;
    std::cerr << " ***   " << errInfo->errorText << std::endl;
  }
#  endif
  return true;
}

#else

static inline bool isPortAudioError(const char *msg, PaError paError)
{
  (void) msg;
  return (paError != paNoError);
}

#endif

namespace Plus4Emu {

  AudioOutput::AudioOutput()
    : outputFileName(""),
      soundFile((SNDFILE *) 0),
      deviceNumber(-1),
      sampleRate(0.0f),
      totalLatency(0.0f),
      nPeriodsHW(0),
      nPeriodsSW(0)
  {
  }

  AudioOutput::~AudioOutput()
  {
    // NOTE: the destructor of a derived class is responsible for closing
    // the audio device if it is open
    if (soundFile) {
      sf_close(soundFile);
      soundFile = (SNDFILE *) 0;
    }
  }

  void AudioOutput::setParameters(int deviceNumber_, float sampleRate_,
                                  float totalLatency_,
                                  int nPeriodsHW_, int nPeriodsSW_)
  {
    deviceNumber_ = (deviceNumber_ >= 0 ? deviceNumber_ : -1);
    sampleRate_ = (sampleRate_ > 11025.0f ?
                   (sampleRate_ < 192000.0f ? sampleRate_ : 192000.0f)
                   : 11025.0f);
    totalLatency_ = (totalLatency_ > 0.005f ?
                     (totalLatency_ < 0.5f ? totalLatency_ : 0.5f)
                     : 0.005f);
    nPeriodsHW_ = (nPeriodsHW_ > 2 ? (nPeriodsHW_ < 16 ? nPeriodsHW_ : 16) : 2);
    nPeriodsSW_ = (nPeriodsSW_ > 2 ? (nPeriodsSW_ < 16 ? nPeriodsSW_ : 16) : 2);
    if (deviceNumber_ == deviceNumber &&
        sampleRate_ == sampleRate &&
        totalLatency_ == totalLatency &&
        nPeriodsHW_ == nPeriodsHW &&
        nPeriodsSW_ == nPeriodsSW)
      return;
    if (deviceNumber >= 0) {
      try {
        closeDevice();
      }
      catch (...) {
        // FIXME: should not ignore errors, although not likely to happen here
      }
    }
    deviceNumber = -1;
    totalLatency = totalLatency_;
    nPeriodsHW = nPeriodsHW_;
    nPeriodsSW = nPeriodsSW_;
    if (sampleRate_ != sampleRate) {
      sampleRate = sampleRate_;
      if (soundFile != (SNDFILE *) 0) {
        sf_close(soundFile);
        soundFile = (SNDFILE *) 0;
      }
      if (outputFileName.length() != 0) {
        SF_INFO sfinfo;
        std::memset(&sfinfo, 0, sizeof(SF_INFO));
        sfinfo.frames = -1;
        sfinfo.samplerate = int(sampleRate + 0.5);
        sfinfo.channels = 1;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        soundFile = sf_open(outputFileName.c_str(), SFM_WRITE, &sfinfo);
        if (!soundFile) {
          outputFileName = "";
          throw Exception("error opening output sound file");
        }
      }
    }
    deviceNumber = deviceNumber_;
    sampleRate = sampleRate_;
    if (deviceNumber >= 0) {
      try {
        openDevice();
      }
      catch (...) {
        deviceNumber = -1;
        throw;
      }
    }
  }

  void AudioOutput::setOutputFile(const std::string& fileName)
  {
    if (fileName == outputFileName)
      return;
    outputFileName = "";
    if (soundFile != (SNDFILE *) 0) {
      sf_close(soundFile);
      soundFile = (SNDFILE *) 0;
    }
    if (fileName.length() != 0) {
      SF_INFO sfinfo;
      std::memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.frames = -1;
      sfinfo.samplerate = int(sampleRate + 0.5);
      sfinfo.channels = 1;
      sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
      soundFile = sf_open(fileName.c_str(), SFM_WRITE, &sfinfo);
      if (!soundFile)
        throw Exception("error opening output sound file");
      outputFileName = fileName;
    }
  }

  void AudioOutput::sendAudioData(const int16_t *buf, size_t nFrames)
  {
    // NOTE: AudioOutput::sendAudioData() should be called by derived classes
    // so that the sound file can be written
    if (soundFile) {
      // need to cast away const qualification to work around compile
      // error with old versions of libsndfile
      if (sf_writef_short(soundFile,
                          reinterpret_cast<short *>(const_cast<int16_t *>(buf)),
                          sf_count_t(nFrames))
          != sf_count_t(nFrames)) {
        sf_close(soundFile);
        soundFile = (SNDFILE *) 0;
        outputFileName = "";
        throw Exception("error writing sound file -- is the disk full ?");
      }
    }
  }

  void AudioOutput::closeDevice()
  {
    // NOTE: AudioOutput::closeDevice() should be called by derived classes
    // to reset internal data
    deviceNumber = -1;
  }

  std::vector< std::string > AudioOutput::getDeviceList()
  {
    std::vector< std::string >  tmp;
    return tmp;
  }

  void AudioOutput::openDevice()
  {
  }

  // --------------------------------------------------------------------------

  AudioOutput_PortAudio::AudioOutput_PortAudio()
    : AudioOutput(),
      paInitialized(false),
      disableRingBuffer(false),
      usingBlockingInterface(false),
      paLockTimeout(0U),
      writeBufIndex(0),
      readBufIndex(0),
      paStream((PaStream *) 0),
      nextTime(0.0),
      closeDeviceLock(true)
  {
    // initialize PortAudio
    if (isPortAudioError("calling Pa_Initialize()", Pa_Initialize()))
      throw Exception("error initializing PortAudio");
    paInitialized = true;
  }

  AudioOutput_PortAudio::~AudioOutput_PortAudio()
  {
    paLockTimeout = 0U;
    if (paStream) {
      (void) isPortAudioError("calling Pa_StopStream()",
                              Pa_StopStream(paStream));
      closeDeviceLock.wait();
      (void) isPortAudioError("calling Pa_CloseStream()",
                              Pa_CloseStream(paStream));
      closeDeviceLock.notify();
      paStream = (PaStream *) 0;
    }
    disableRingBuffer = false;
    usingBlockingInterface = false;
    writeBufIndex = 0;
    readBufIndex = 0;
    buffers.clear();
    if (paInitialized) {
      (void) isPortAudioError("calling Pa_Terminate()", Pa_Terminate());
      paInitialized = false;
    }
  }

  void AudioOutput_PortAudio::sendAudioData(const int16_t *buf, size_t nFrames)
  {
    if (paStream) {
#ifndef USING_OLD_PORTAUDIO_API
      if (usingBlockingInterface) {
        // ring buffer is not used for blocking I/O,
        // so assume nPeriodsSW == 1
#  if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
        // reduce timing jitter on Win32
        double  t = nextTime - timer_.getRealTime();
        double  periodTime = double(long(nFrames)) / double(sampleRate);
        long    framesToWrite = Pa_GetStreamWriteAvailable(paStream);
        switch (int((framesToWrite << 3)
                    / (long(buffers[0].audioData.size() >> 1) * nPeriodsHW))) {
        case 0:
          periodTime = periodTime * 2.0;
          break;
        case 1:
          periodTime = periodTime * 1.25;
          break;
        case 2:
          periodTime = periodTime * 1.1;
          break;
        case 3:
          periodTime = periodTime * 1.05;
          break;
        case 4:
          periodTime = periodTime * 0.95;
          break;
        case 5:
          periodTime = periodTime * 0.9;
          break;
        default:
          timer_.reset();
          nextTime = 0.0;
          periodTime = 0.0;
          break;
        }
        nextTime = nextTime + periodTime;
        if (t > 0.00075) {
          Timer::wait(t);
        }
        else if (t < -0.5) {
          timer_.reset();
          nextTime = 0.0;
        }
#  endif
        for (size_t i = 0; i < nFrames; i++) {
          Buffer& buf_ = buffers[0];
          buf_.audioData[buf_.writePos++] = buf[i];
          buf_.audioData[buf_.writePos++] = buf[i];
          if (buf_.writePos >= buf_.audioData.size()) {
            buf_.writePos = 0;
            Pa_WriteStream(paStream,
                           &(buf_.audioData[0]), buf_.audioData.size() >> 1);
          }
        }
      }
      else
#endif
      {
        for (size_t i = 0; i < nFrames; i++) {
          Buffer& buf_ = buffers[writeBufIndex];
          buf_.audioData[buf_.writePos++] = buf[i];
          buf_.audioData[buf_.writePos++] = buf[i];
          if (buf_.writePos >= buf_.audioData.size()) {
            buf_.writePos = 0;
            buf_.paLock.notify();
            if (buf_.epLock.wait(1000)) {
              if (++writeBufIndex >= buffers.size())
                writeBufIndex = 0;
            }
          }
        }
      }
    }
    else {
      // if there is no audio device, only synchronize to real time
      double  curTime = timer_.getRealTime();
      double  waitTime = nextTime - curTime;
      if (waitTime > 0.0)
        Timer::wait(waitTime);
      if (waitTime < double(-totalLatency)) {
        timer_.reset();
        nextTime = 0.0;
      }
      nextTime += (double(long(nFrames)) / sampleRate);
    }
    // call base class to write sound file
    AudioOutput::sendAudioData(buf, nFrames);
  }

  void AudioOutput_PortAudio::closeDevice()
  {
    paLockTimeout = 0U;
    if (paStream) {
      (void) isPortAudioError("calling Pa_StopStream()",
                              Pa_StopStream(paStream));
      closeDeviceLock.wait();
      (void) isPortAudioError("calling Pa_CloseStream()",
                              Pa_CloseStream(paStream));
      closeDeviceLock.notify();
      paStream = (PaStream *) 0;
    }
    disableRingBuffer = false;
    usingBlockingInterface = false;
    writeBufIndex = 0;
    readBufIndex = 0;
    buffers.clear();
    // call base class to reset internal state
    AudioOutput::closeDevice();
  }

  std::vector< std::string > AudioOutput_PortAudio::getDeviceList()
  {
    std::vector< std::string >  tmp;
    if (paInitialized) {
#ifndef USING_OLD_PORTAUDIO_API
      PaDeviceIndex devCnt = Pa_GetDeviceCount();
      PaDeviceIndex i;
#else
      PaDeviceID    devCnt = PaDeviceID(Pa_CountDevices());
      PaDeviceID    i;
#endif
      for (i = 0; i < devCnt; i++) {
        const PaDeviceInfo  *devInfo = Pa_GetDeviceInfo(i);
        if (devInfo) {
          if (devInfo->maxOutputChannels >= 2) {
            std::string s("");
            if (devInfo->name)
              s = devInfo->name;
#ifndef USING_OLD_PORTAUDIO_API
            const PaHostApiInfo *apiInfo = Pa_GetHostApiInfo(devInfo->hostApi);
            if (apiInfo) {
              s += " (";
              if (apiInfo->name)
                s += apiInfo->name;
              s += ")";
            }
#endif
            tmp.push_back(s);
          }
        }
      }
    }
    return tmp;
  }

#ifndef USING_OLD_PORTAUDIO_API
  int AudioOutput_PortAudio::portAudioCallback(const void *input, void *output,
                                               unsigned long frameCount,
                                               const PaStreamCallbackTimeInfo
                                                   *timeInfo,
                                               PaStreamCallbackFlags
                                                   statusFlags,
                                               void *userData)
#else
  int AudioOutput_PortAudio::portAudioCallback(void *input, void *output,
                                               unsigned long frameCount,
                                               PaTimestamp outTime,
                                               void *userData)
#endif
  {
    AudioOutput_PortAudio *p =
        reinterpret_cast<AudioOutput_PortAudio *>(userData);
    if (!p->closeDeviceLock.wait(0)) {
#ifndef USING_OLD_PORTAUDIO_API
      return int(paAbort);
#else
      return 1;
#endif
    }
    int16_t *buf = reinterpret_cast<int16_t *>(output);
    size_t  i = 0, nFrames = frameCount;
    (void) input;
#ifndef USING_OLD_PORTAUDIO_API
    (void) timeInfo;
    (void) statusFlags;
#else
    (void) outTime;
#endif
    if (nFrames > (p->buffers[p->readBufIndex].audioData.size() >> 1))
      nFrames = p->buffers[p->readBufIndex].audioData.size() >> 1;
    nFrames <<= 1;
    if (p->buffers[p->readBufIndex].paLock.wait(p->paLockTimeout)) {
      for ( ; i < nFrames; i++)
        buf[i] = p->buffers[p->readBufIndex].audioData[i];
    }
    p->buffers[p->readBufIndex].epLock.notify();
    if (++(p->readBufIndex) >= p->buffers.size())
      p->readBufIndex = 0;
    for ( ; i < (frameCount << 1); i++)
      buf[i] = 0;
    p->closeDeviceLock.notify();
#ifndef USING_OLD_PORTAUDIO_API
    return int(paContinue);
#else
    return 0;
#endif
  }

  void AudioOutput_PortAudio::openDevice()
  {
    writeBufIndex = 0;
    readBufIndex = 0;
    paStream = (PaStream *) 0;
    // find audio device
#ifndef USING_OLD_PORTAUDIO_API
    int     devCnt = int(Pa_GetDeviceCount());
#else
    int     devCnt = int(Pa_CountDevices());
#endif
    if (devCnt < 1)
      throw Exception("no audio device is available");
    int     devIndex;
    int     devNum = deviceNumber;
    for (devIndex = 0; devIndex < devCnt; devIndex++) {
      const PaDeviceInfo  *devInfo;
#ifndef USING_OLD_PORTAUDIO_API
      devInfo = Pa_GetDeviceInfo(PaDeviceIndex(devIndex));
#else
      devInfo = Pa_GetDeviceInfo(PaDeviceID(devIndex));
#endif
      if (!devInfo)
        throw Exception("error querying audio device information");
      if (devInfo->maxOutputChannels >= 2) {
        if (--devNum == -1) {
          devNum = devIndex;
          break;
        }
      }
    }
    if (devIndex >= devCnt)
      throw Exception("device number is out of range");
    usingBlockingInterface = false;
#ifndef USING_OLD_PORTAUDIO_API
    const PaDeviceInfo  *devInfo = Pa_GetDeviceInfo(PaDeviceIndex(devIndex));
    if (devInfo) {
      const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(devInfo->hostApi);
      if (hostApiInfo) {
        if (hostApiInfo->type == paDirectSound ||
            hostApiInfo->type == paMME ||
            hostApiInfo->type == paOSS ||
            hostApiInfo->type == paALSA) {
          disableRingBuffer = true;
          if (hostApiInfo->type != paDirectSound)
            usingBlockingInterface = true;
        }
      }
    }
#else
    disableRingBuffer = true;
#endif
    // calculate buffer size
    int   nPeriodsSW_ = (disableRingBuffer ? 1 : nPeriodsSW);
    int   periodSize = int(totalLatency * sampleRate + 0.5)
                       / (nPeriodsHW + nPeriodsSW_ - 2);
    for (int i = 16; i < 16384; i <<= 1) {
      if (i >= periodSize) {
        periodSize = i;
        break;
      }
    }
    if (periodSize > 16384)
      periodSize = 16384;
    if (disableRingBuffer)
      paLockTimeout = (unsigned int) (double(periodSize) * double(nPeriodsHW)
                                      * 1000.0 / double(sampleRate)
                                      + 0.999);
    else
      paLockTimeout = 0U;
    // initialize buffers
    buffers.resize(size_t(nPeriodsSW_));
    for (int i = 0; i < nPeriodsSW_; i++) {
      buffers[i].audioData.resize(size_t(periodSize) << 1);
      for (int j = 0; j < (periodSize << 1); j++)
        buffers[i].audioData[j] = 0;
    }
    // open audio stream
#ifndef USING_OLD_PORTAUDIO_API
    PaStreamParameters  streamParams;
    std::memset(&streamParams, 0, sizeof(PaStreamParameters));
    streamParams.device = PaDeviceIndex(devNum);
    streamParams.channelCount = 2;
    streamParams.sampleFormat = paInt16;
    streamParams.suggestedLatency = PaTime(double(periodSize) * nPeriodsHW
                                           / sampleRate);
    streamParams.hostApiSpecificStreamInfo = (void *) 0;
    PaError err = paNoError;
    if (usingBlockingInterface)
      err = Pa_OpenStream(&paStream, (PaStreamParameters *) 0, &streamParams,
                          sampleRate, unsigned(periodSize),
                          paNoFlag, (PaStreamCallback *) 0, (void *) this);
    else
      err = Pa_OpenStream(&paStream, (PaStreamParameters *) 0, &streamParams,
                          sampleRate, unsigned(periodSize),
                          paNoFlag, &portAudioCallback, (void *) this);
#else
    PaError err =
        Pa_OpenStream(&paStream,
                      paNoDevice, 0, paInt16, (void *) 0,
                      PaDeviceID(devNum), 2, paInt16, (void *) 0,
                      sampleRate, unsigned(periodSize), unsigned(nPeriodsHW),
                      paNoFlag, &portAudioCallback, (void *) this);
#endif
    if (isPortAudioError("calling Pa_OpenStream()", err)) {
      paStream = (PaStream *) 0;
      throw Exception("error opening audio device");
    }
    Pa_StartStream(paStream);
  }

}       // namespace Plus4Emu

