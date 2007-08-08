
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

#include "plus4emu.hpp"
#include "system.hpp"

#include <cstring>
#include <typeinfo>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include "fldisp.hpp"

static int defaultFLTKEventCallback(void *userData, int event)
{
  (void) userData;
  (void) event;
  return 0;
}

namespace Plus4Emu {

  FLTKDisplay_::Message::~Message()
  {
  }

  FLTKDisplay_::Message_LineData::~Message_LineData()
  {
  }

  FLTKDisplay_::Message_FrameDone::~Message_FrameDone()
  {
  }

  FLTKDisplay_::Message_SetParameters::~Message_SetParameters()
  {
  }

  void FLTKDisplay_::deleteMessage(Message *m)
  {
    m->~Message();
    m->prv = (Message *) 0;
    messageQueueMutex.lock();
    m->nxt = freeMessageStack;
    if (freeMessageStack)
      freeMessageStack->prv = m;
    freeMessageStack = m;
    messageQueueMutex.unlock();
  }

  void FLTKDisplay_::queueMessage(Message *m)
  {
    messageQueueMutex.lock();
    if (exitFlag) {
      messageQueueMutex.unlock();
      m->~Message();
      std::free(m);
      return;
    }
    m->prv = lastMessage;
    m->nxt = (Message *) 0;
    if (lastMessage)
      lastMessage->nxt = m;
    else
      messageQueue = m;
    lastMessage = m;
    messageQueueMutex.unlock();
    if (typeid(*m) == typeid(Message_FrameDone)) {
      if (!videoResampleEnabled) {
        Fl::awake();
        threadLock.wait(1);
      }
    }
  }

  // --------------------------------------------------------------------------

  FLTKDisplay_::FLTKDisplay_()
    : VideoDisplay(),
      messageQueue((Message *) 0),
      lastMessage((Message *) 0),
      freeMessageStack((Message *) 0),
      messageQueueMutex(),
      lineBuffers((Message_LineData **) 0),
      nextLine((Message_LineData *) 0),
      curLine(0),
      vsyncCnt(0),
      framesPending(0),
      skippingFrame(false),
      oddFrame(false),
      burstValue(0x08),
      syncLengthCnt(0U),
      hsyncCnt(0U),
      hsyncPeriodLength(570U),
      lineLengthCnt(0U),
      lineLength(570U),
      lineStart(80U),
      hsyncPeriodMin(494U),
      hsyncPeriodMax(646U),
      lineLengthMin(513U),
      lineLengthMax(627U),
      lineLengthFilter(570.0f),
      vsyncThreshold1(338),
      vsyncThreshold2(264),
      vsyncReload(-16),
      lineReload(-6),
      videoResampleEnabled(false),
      exitFlag(false),
      limitFrameRateFlag(false),
      displayParameters(),
      savedDisplayParameters(),
      fltkEventCallback(&defaultFLTKEventCallback),
      fltkEventCallbackUserData((void *) 0),
      screenshotCallback((void (*)(void *, const unsigned char *, int, int)) 0),
      screenshotCallbackUserData((void *) 0),
      screenshotCallbackCnt(0)
  {
    try {
      lineBuffers = new Message_LineData*[578];
      for (size_t n = 0; n < 578; n++)
        lineBuffers[n] = (Message_LineData *) 0;
      nextLine = allocateMessage<Message_LineData>();
    }
    catch (...) {
      if (lineBuffers)
        delete[] lineBuffers;
      throw;
    }
  }

  FLTKDisplay_::~FLTKDisplay_()
  {
    messageQueueMutex.lock();
    exitFlag = true;
    while (freeMessageStack) {
      Message *m = freeMessageStack;
      freeMessageStack = m->nxt;
      if (freeMessageStack)
        freeMessageStack->prv = (Message *) 0;
      std::free(m);
    }
    while (messageQueue) {
      Message *m = messageQueue;
      messageQueue = m->nxt;
      if (messageQueue)
        messageQueue->prv = (Message *) 0;
      m->~Message();
      std::free(m);
    }
    lastMessage = (Message *) 0;
    messageQueueMutex.unlock();
    for (size_t n = 0; n < 578; n++) {
      Message *m = lineBuffers[n];
      if (m) {
        lineBuffers[n] = (Message_LineData *) 0;
        m->~Message();
        std::free(m);
      }
    }
    delete[] lineBuffers;
    if (nextLine) {
      Message *m = nextLine;
      nextLine = (Message_LineData *) 0;
      m->~Message();
      std::free(m);
    }
  }

  void FLTKDisplay_::draw()
  {
  }

  int FLTKDisplay_::handle(int event)
  {
    return fltkEventCallback(fltkEventCallbackUserData, event);
  }

  void FLTKDisplay_::setDisplayParameters(const DisplayParameters& dp)
  {
    Message_SetParameters *m = allocateMessage<Message_SetParameters>();
    m->dp = dp;
    if (dp.ntscMode != savedDisplayParameters.ntscMode) {
      deleteMessage(nextLine);
      nextLine = allocateMessage<Message_LineData>();
      if (!dp.ntscMode) {
        burstValue = 0x08;
        syncLengthCnt = 0U;
        hsyncCnt = 0U;
        hsyncPeriodLength = 570U;
        lineLengthCnt = 0U;
        lineLength = 570U;
        lineStart = 80U;
        hsyncPeriodMin = 494U;
        hsyncPeriodMax = 646U;
        lineLengthMin = 513U;
        lineLengthMax = 627U;
        lineLengthFilter = 570.0f;
        vsyncThreshold1 = 338;
        vsyncThreshold2 = 264;
        vsyncReload = -16;
        lineReload = -6;
      }
      else {
        burstValue = 0x09;
        syncLengthCnt = 0U;
        hsyncCnt = 0U;
        hsyncPeriodLength = 456U;
        lineLengthCnt = 0U;
        lineLength = 456U;
        lineStart = 64U;
        hsyncPeriodMin = 380U;
        hsyncPeriodMax = 532U;
        lineLengthMin = 399U;
        lineLengthMax = 513U;
        lineLengthFilter = 456.0f;
        vsyncThreshold1 = 292;
        vsyncThreshold2 = 242;
        vsyncReload = 0;
        lineReload = 12;
      }
    }
    savedDisplayParameters = dp;
    queueMessage(m);
  }

  const VideoDisplay::DisplayParameters&
      FLTKDisplay_::getDisplayParameters() const
  {
    return savedDisplayParameters;
  }

  void FLTKDisplay_::lineDone()
  {
    lineLengthCnt = lineLengthCnt - lineLength;
    while (hsyncCnt >= hsyncPeriodMax) {
      hsyncCnt -= hsyncPeriodLength;
      hsyncPeriodLength = (hsyncPeriodLength * 3U + hsyncPeriodMax) >> 2;
    }
    lineLengthFilter =
        (lineLengthFilter * 0.9f) + (float(int(hsyncPeriodLength)) * 0.1f);
    lineLength = (unsigned int) (int(lineLengthFilter + 0.5f));
    if (lineLengthCnt != hsyncCnt) {
      int     hsyncPhaseError = int(lineLengthCnt) - int(hsyncCnt);
      if (hsyncPhaseError >= int(hsyncPeriodLength >> 1))
        hsyncPhaseError -= int(hsyncPeriodLength);
      if (hsyncPhaseError <= -(int(hsyncPeriodLength >> 1)))
        hsyncPhaseError += int(hsyncPeriodLength);
      unsigned int  tmp = (unsigned int) (hsyncPhaseError >= 0 ?
                                          hsyncPhaseError : (-hsyncPhaseError));
      tmp = (tmp + 6U) >> 2;
      tmp = (tmp < 10U ? tmp : 10U);
      if (hsyncPhaseError >= 0)
        lineLength += tmp;
      else
        lineLength -= tmp;
      if (lineLength > lineLengthMax)
        lineLength = lineLengthMax;
      else if (lineLength < lineLengthMin)
        lineLength = lineLengthMin;
    }
    if (!skippingFrame) {
      if (curLine >= 0 && curLine < 578) {
        Message_LineData  *m = nextLine;
        nextLine = (Message_LineData *) 0;
        m->lineNum = curLine;
        queueMessage(m);
      }
    }
    if (nextLine) {
      Message_LineData  *m = nextLine;
      nextLine = (Message_LineData *) 0;
      deleteMessage(m);
    }
    nextLine = allocateMessage<Message_LineData>();
    curLine += 2;
    if (vsyncCnt >= vsyncThreshold1) {
      vsyncCnt = vsyncReload;
      oddFrame = false;
    }
    if (vsyncCnt == 0) {
      curLine = lineReload - (!oddFrame ? 0 : 1);
      frameDone();
    }
    vsyncCnt++;
  }

  void FLTKDisplay_::sendVideoOutput(const uint8_t *buf, size_t nBytes)
  {
    const uint8_t *bufp = buf;
    const uint8_t *startp = bufp;
    const uint8_t *endp = buf + nBytes;
    while (bufp < endp) {
      uint8_t   c = *bufp;
      if (c & 0x80) {                                   // sync
        if (syncLengthCnt == 0U) {                      // hsync start
          while (hsyncCnt >= hsyncPeriodMax) {
            hsyncCnt -= hsyncPeriodLength;
            hsyncPeriodLength = (hsyncPeriodLength * 3U + hsyncPeriodMax) >> 2;
          }
          if (hsyncCnt >= hsyncPeriodMin) {
            hsyncPeriodLength = hsyncCnt;
            hsyncCnt = 0U;
          }
        }
        syncLengthCnt++;
        if (syncLengthCnt >= 26U) {                     // vsync
          if (vsyncCnt >= vsyncThreshold2) {
            vsyncCnt = vsyncReload;
            oddFrame = ((lineLengthCnt + 6U) > (lineLength >> 1));
          }
        }
      }
      else
        syncLengthCnt = 0U;
      nextLine->flags |= uint8_t(0x80 - ((c ^ burstValue) & 0x09));
      const uint8_t *nextBufPtr = bufp + size_t((1 << (c & 0x02)) + 1);
      unsigned int  l = ((unsigned int) c & 0x01U) ^ 0x05U;
      if (lineLengthCnt < lineStart) {
        startp = nextBufPtr;
        nextLine->lineLength = lineLengthCnt + l;
      }
      else if (lineLengthCnt >= lineLength) {
        nextLine->lineLength = size_t(lineLengthCnt) - nextLine->lineLength;
        nextLine->appendData(startp, size_t(bufp - startp));
        startp = nextBufPtr;
        lineDone();
      }
      bufp = nextBufPtr;
      lineLengthCnt = lineLengthCnt + l;
      hsyncCnt = hsyncCnt + l;
    }
    nextLine->appendData(startp, size_t(bufp - startp));
  }

  void FLTKDisplay_::setScreenshotCallback(void (*func)(void *,
                                                        const unsigned char *,
                                                        int, int),
                                           void *userData_)
  {
    if (!screenshotCallback || !func) {
      screenshotCallback = func;
      if (func) {
        screenshotCallbackUserData = userData_;
        screenshotCallbackCnt = 3;
      }
      else {
        screenshotCallbackUserData = (void *) 0;
        screenshotCallbackCnt = 0;
      }
    }
  }

  void FLTKDisplay_::setFLTKEventCallback(int (*func)(void *userData,
                                                      int event),
                                          void *userData_)
  {
    if (func)
      fltkEventCallback = func;
    else
      fltkEventCallback = &defaultFLTKEventCallback;
    fltkEventCallbackUserData = userData_;
  }

  void FLTKDisplay_::limitFrameRate(bool isEnabled)
  {
    limitFrameRateFlag = isEnabled;
  }

  void FLTKDisplay_::checkScreenshotCallback()
  {
    if (!screenshotCallbackCnt)
      return;
    screenshotCallbackCnt--;
    if (screenshotCallbackCnt)
      return;
    void    (*func)(void *, const unsigned char *, int, int);
    void    *userData_ = screenshotCallbackUserData;
    func = screenshotCallback;
    screenshotCallback = (void (*)(void *, const unsigned char *, int, int)) 0;
    screenshotCallbackUserData = (void *) 0;
    if (!func)
      return;
    unsigned char *imageBuf_ = (unsigned char *) 0;
    try {
      VideoDisplayColormap<uint32_t>  colormap_;
      VideoDisplay::DisplayParameters dp;
      dp.indexToRGBFunc = displayParameters.indexToRGBFunc;
      colormap_.setDisplayParameters(dp);
      imageBuf_ = new unsigned char[768 * 576 * 3];
      for (size_t yc = 2; yc < 578; yc++) {
        unsigned char *outBuf = &(imageBuf_[(yc - 2) * 768 * 3]);
        Message_LineData  *l = (Message_LineData *) 0;
        if (lineBuffers[yc] != (Message_LineData *) 0)
          l = lineBuffers[yc];
        else
          l = lineBuffers[yc ^ 1];
        size_t  xc = 0;
        if (l) {
          const unsigned char *bufp = (unsigned char *) 0;
          size_t    nBytes = 0;
          size_t    bufPos = 0;
          uint8_t   videoFlags = uint8_t((yc & 2) | ((l->flags & 0x80) >> 2));
          size_t    pixelSample2 = l->lineLength;
          l->getLineData(bufp, nBytes);
          if (displayParameters.ntscMode)
            videoFlags = videoFlags | 0x10;
          uint32_t  tmpBuf[4];
          size_t    pixelSample1 = 980;
          size_t    pixelSampleCnt = 0;
          uint8_t   readPos = 4;
          do {
            if (readPos >= 4) {
              readPos = readPos & 3;
              if (bufPos >= nBytes)
                break;
              pixelSample1 = ((bufp[bufPos] & 0x01) ? 784 : 980);
              size_t  n = colormap_.convertFourPixels(&(tmpBuf[0]),
                                                      &(bufp[bufPos]),
                                                      videoFlags);
              bufPos += n;
            }
            uint32_t  c = tmpBuf[readPos];
            outBuf[xc * 3 + 0] = (unsigned char) (c & 0xFFU);
            outBuf[xc * 3 + 1] = (unsigned char) ((c >> 8) & 0xFFU);
            outBuf[xc * 3 + 2] = (unsigned char) ((c >> 16) & 0xFFU);
            pixelSampleCnt += pixelSample2;
            if (pixelSampleCnt >= pixelSample1) {
              pixelSampleCnt -= pixelSample1;
              readPos++;
            }
            xc++;
          } while (xc < 768);
          for ( ; xc < 768; xc++) {
            outBuf[xc * 3 + 0] = 0x00;
            outBuf[xc * 3 + 1] = 0x00;
            outBuf[xc * 3 + 2] = 0x00;
          }
        }
      }
      func(userData_, imageBuf_, 768, 576);
    }
    catch (...) {
      if (imageBuf_)
        delete[] imageBuf_;
      imageBuf_ = (unsigned char *) 0;
    }
    if (imageBuf_)
      delete[] imageBuf_;
  }

  void FLTKDisplay_::frameDone()
  {
    messageQueueMutex.lock();
    bool    skippedFrame = skippingFrame;
    if (!skippedFrame)
      framesPending++;
    bool    overrunFlag = (framesPending > 3);  // should this be configurable ?
    skippingFrame = overrunFlag;
    if (limitFrameRateFlag) {
      if (limitFrameRateTimer.getRealTime() < 0.02)
        skippingFrame = true;
      else
        limitFrameRateTimer.reset();
    }
    messageQueueMutex.unlock();
    if (skippedFrame) {
      if (overrunFlag || !limitFrameRateFlag) {
        Fl::awake();
        threadLock.wait(1);
      }
      return;
    }
    Message *m = allocateMessage<Message_FrameDone>();
    queueMessage(m);
  }

  // --------------------------------------------------------------------------

  FLTKDisplay::FLTKDisplay(int xx, int yy, int ww, int hh, const char *lbl)
    : Fl_Window(xx, yy, ww, hh, lbl),
      FLTKDisplay_(),
      colormap(),
      linesChanged((uint8_t *) 0),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false)
  {
    displayParameters.displayQuality = 0;
    displayParameters.bufferingMode = 0;
    savedDisplayParameters.displayQuality = 0;
    savedDisplayParameters.bufferingMode = 0;
    try {
      linesChanged = new uint8_t[578];
      for (size_t n = 0; n < 578; n++)
        linesChanged[n] = 0x00;
    }
    catch (...) {
      if (linesChanged)
        delete[] linesChanged;
      throw;
    }
  }

  FLTKDisplay::~FLTKDisplay()
  {
    delete[] linesChanged;
  }

  void FLTKDisplay::displayFrame()
  {
    int     windowWidth_ = this->w();
    int     windowHeight_ = this->h();
    int     displayWidth_ = windowWidth_;
    int     displayHeight_ = windowHeight_;
    bool    halfResolutionX_ = false;
    bool    halfResolutionY_ = false;
    int     x0 = 0;
    int     y0 = 0;
    int     x1 = displayWidth_;
    int     y1 = displayHeight_;
    double  aspectScale_ = (768.0 / 576.0)
                           / ((double(windowWidth_) / double(windowHeight_))
                              * displayParameters.pixelAspectRatio);
    if (aspectScale_ > 1.0001) {
      displayHeight_ = int((double(windowHeight_) / aspectScale_) + 0.5);
      y0 = (windowHeight_ - displayHeight_) >> 1;
      y1 = y0 + displayHeight_;
    }
    else if (aspectScale_ < 0.9999) {
      displayWidth_ = int((double(windowWidth_) * aspectScale_) + 0.5);
      x0 = (windowWidth_ - displayWidth_) >> 1;
      x1 = x0 + displayWidth_;
    }
    if (displayWidth_ < 576)
      halfResolutionX_ = true;
    if (displayHeight_ < 432)
      halfResolutionY_ = true;

    if (x0 > 0) {
      fl_color(FL_BLACK);
      fl_rectf(0, 0, x0, windowHeight_);
    }
    if (x1 < windowWidth_) {
      fl_color(FL_BLACK);
      fl_rectf(x1, 0, (windowWidth_ - x1), windowHeight_);
    }
    if (y0 > 0) {
      fl_color(FL_BLACK);
      fl_rectf(0, 0, windowWidth_, y0);
    }
    if (y1 < windowHeight_) {
      fl_color(FL_BLACK);
      fl_rectf(0, y1, windowWidth_, (windowHeight_ - y1));
    }

    if (displayWidth_ <= 0 || displayHeight_ <= 0)
      return;

    unsigned char *pixelBuf_ =
        (unsigned char *) std::calloc(size_t(displayWidth_ * 4 * 3),
                                      sizeof(unsigned char));
    if (pixelBuf_) {
      int     curLine_ = 2;
      int     fracY_ = 0;
      bool    skippingLines_ = true;
      size_t  pixelSample1p = 490 * size_t(displayWidth_);
      size_t  pixelSample1n = 392 * size_t(displayWidth_);
      int     lineNumbers_[5];
      lineNumbers_[3] = -2;
      for (int yc = 0; yc < displayHeight_; yc++) {
        int   ycAnd3 = yc & 3;
        if (ycAnd3 == 0) {
          skippingLines_ = true;
          lineNumbers_[4] = lineNumbers_[3];
        }
        int   l0 = curLine_;
        int   l1 = curLine_ ^ 1;
        if (lineBuffers[l0] != (Message_LineData *) 0)
          lineNumbers_[ycAnd3] = l0;
        else if (lineBuffers[l1] != (Message_LineData *) 0)
          lineNumbers_[ycAnd3] = l1;
        else
          lineNumbers_[ycAnd3] = -1;
        if ((linesChanged[l0] | linesChanged[l1]) & 0x80)
          skippingLines_ = false;
        if (ycAnd3 == 3 || yc == (displayHeight_ - 1)) {
          if (!skippingLines_) {
            int   nLines_ = 4;
            if (ycAnd3 != 3)
              nLines_ = displayHeight_ & 3;
            for (int yTmp = 0; yTmp < nLines_; yTmp++) {
              unsigned char *p = &(pixelBuf_[displayWidth_ * yTmp * 3]);
              if (yTmp == 0) {
                if (lineNumbers_[0] == lineNumbers_[4] ||
                    (lineNumbers_[0] >= 0 && lineNumbers_[4] >= 0 &&
                     *(lineBuffers[lineNumbers_[0]])
                     == *(lineBuffers[lineNumbers_[4]]))) {
                  std::memcpy(p, &(pixelBuf_[displayWidth_ * 3 * 3]),
                              size_t(displayWidth_ * 3));
                  continue;
                }
              }
              else {
                if (lineNumbers_[yTmp] == lineNumbers_[yTmp - 1] ||
                    (lineNumbers_[yTmp - 1] >= 0 && lineNumbers_[yTmp] >= 0 &&
                     *(lineBuffers[lineNumbers_[yTmp]])
                     == *(lineBuffers[lineNumbers_[yTmp - 1]]))) {
                  std::memcpy(p, &(pixelBuf_[displayWidth_ * (yTmp - 1) * 3]),
                              size_t(displayWidth_ * 3));
                  continue;
                }
              }
              int     xc = 0;
              if (lineNumbers_[yTmp] >= 0) {
                // decode video data and convert to RGB
                const unsigned char *bufp = (unsigned char *) 0;
                size_t    nBytes = 0;
                int       lineNum = lineNumbers_[yTmp];
                Message_LineData    *l = lineBuffers[lineNum];
                l->getLineData(bufp, nBytes);
                uint32_t  tmpBuf[4];
                size_t    bufPos = 0;
                uint8_t   videoFlags =
                    uint8_t((lineNum & 2) | ((l->flags & 0x80) >> 2));
                size_t    pixelSample2 = l->lineLength * 384;
                if (displayParameters.ntscMode)
                  videoFlags = videoFlags | 0x10;
                size_t    pixelSample1 = pixelSample1p;
                size_t    pixelSampleCnt = 0;
                uint8_t   readPos = 4;
                do {
                  if (readPos >= 4) {
                    if (bufPos >= nBytes)
                      break;
                    pixelSample1 = ((bufp[bufPos] & 0x01) ?
                                    pixelSample1n : pixelSample1p);
                    size_t  n = colormap.convertFourPixels(&(tmpBuf[0]),
                                                           &(bufp[bufPos]),
                                                           videoFlags);
                    bufPos += n;
                    readPos = readPos & 3;
                  }
                  uint32_t  tmp = tmpBuf[readPos];
                  p[0] = (unsigned char) tmp & (unsigned char) 0xFF;
                  tmp = tmp >> 8;
                  p[1] = (unsigned char) tmp & (unsigned char) 0xFF;
                  tmp = tmp >> 8;
                  p[2] = (unsigned char) tmp & (unsigned char) 0xFF;
                  p = p + 3;
                  pixelSampleCnt += pixelSample2;
                  while (pixelSampleCnt >= pixelSample1) {
                    pixelSampleCnt -= pixelSample1;
                    readPos++;
                  }
                  xc++;
                } while (xc < displayWidth_);
              }
              for ( ; xc < displayWidth_; xc++) {
                p[0] = 0x00;
                p[1] = 0x00;
                p[2] = 0x00;
                p = p + 3;
              }
            }
            fl_draw_image(pixelBuf_, x0, y0 + (yc & (~(int(3)))),
                          displayWidth_, nLines_);
          }
          else
            lineNumbers_[3] = -2;
        }
        if (!halfResolutionY_) {
          fracY_ += 576;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            curLine_ = (curLine_ < 577 ? (curLine_ + 1) : curLine_);
          }
        }
        else {
          fracY_ += 288;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            curLine_ = (curLine_ < 575 ? (curLine_ + 2) : curLine_);
          }
        }
      }
      std::free(pixelBuf_);
    }

    // make sure that all lines are updated at a slow rate
    if (!screenshotCallbackCnt) {
      if (forceUpdateLineMask) {
        for (size_t yc = 0; yc < 578; yc++) {
          if (linesChanged[yc] & 0x01) {
            linesChanged[yc] = 0x00;
            continue;
          }
          if (!(forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 3) & 7)))) {
            linesChanged[yc] = 0x00;
            continue;
          }
          if (lineBuffers[yc] != (Message_LineData *) 0) {
            Message *m = lineBuffers[yc];
            lineBuffers[yc] = (Message_LineData *) 0;
            deleteMessage(m);
          }
          linesChanged[yc] = 0x80;
        }
        forceUpdateLineMask = 0;
      }
      else {
        std::memset(linesChanged, 0x00, 578);
      }
    }
    else {
      std::memset(linesChanged, 0x00, 578);
      checkScreenshotCallback();
    }

    messageQueueMutex.lock();
    framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
    messageQueueMutex.unlock();
  }

  void FLTKDisplay::draw()
  {
    if (redrawFlag) {
      redrawFlag = false;
      displayFrame();
    }
  }

  bool FLTKDisplay::checkEvents()
  {
    threadLock.notify();
    while (true) {
      messageQueueMutex.lock();
      Message *m = messageQueue;
      if (m) {
        messageQueue = m->nxt;
        if (messageQueue) {
          messageQueue->prv = (Message *) 0;
          if (!messageQueue->nxt)
            lastMessage = messageQueue;
        }
        else
          lastMessage = (Message *) 0;
      }
      messageQueueMutex.unlock();
      if (!m)
        break;
      if (typeid(*m) == typeid(Message_LineData)) {
        Message_LineData  *msg;
        msg = static_cast<Message_LineData *>(m);
        if (msg->lineNum >= 0 && msg->lineNum < 578) {
          // check if this line has changed
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0) {
            if (*(lineBuffers[msg->lineNum]) == *msg) {
              linesChanged[msg->lineNum] = 0x01;
              deleteMessage(m);
              continue;
            }
          }
          linesChanged[msg->lineNum] = 0x81;
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0)
            deleteMessage(lineBuffers[msg->lineNum]);
          lineBuffers[msg->lineNum] = msg;
          continue;
        }
      }
      else if (typeid(*m) == typeid(Message_FrameDone)) {
        // need to update display
        if (redrawFlag) {
          // lost a frame
          messageQueueMutex.lock();
          framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
          messageQueueMutex.unlock();
        }
        redrawFlag = true;
        deleteMessage(m);
        noInputTimer.reset();
        if (screenshotCallbackCnt)
          checkScreenshotCallback();
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = static_cast<Message_SetParameters *>(m);
        displayParameters = msg->dp;
        DisplayParameters tmp_dp(displayParameters);
        tmp_dp.blendScale1 = 0.5;
        colormap.setDisplayParameters(tmp_dp);
        for (size_t n = 0; n < 578; n++)
          linesChanged[n] |= uint8_t(0x80);
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 0.6) {
      noInputTimer.reset(0.4);
      if (redrawFlag) {
        // lost a frame
        messageQueueMutex.lock();
        framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
        messageQueueMutex.unlock();
      }
      redrawFlag = true;
      if (screenshotCallbackCnt)
        checkScreenshotCallback();
    }
    if (this->damage() & FL_DAMAGE_EXPOSE) {
      forceUpdateLineMask = 0xFF;
      forceUpdateLineCnt = 0;
      forceUpdateTimer.reset();
    }
    else if (forceUpdateTimer.getRealTime() >= 0.085) {
      forceUpdateLineMask |= (uint8_t(1) << forceUpdateLineCnt);
      forceUpdateLineCnt++;
      forceUpdateLineCnt &= uint8_t(7);
      forceUpdateTimer.reset();
    }
    return redrawFlag;
  }

  int FLTKDisplay::handle(int event)
  {
    return fltkEventCallback(fltkEventCallbackUserData, event);
  }

  void FLTKDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    DisplayParameters dp_(dp);
    dp_.displayQuality = 0;
    dp_.bufferingMode = 0;
    FLTKDisplay_::setDisplayParameters(dp_);
  }

}       // namespace Plus4Emu

