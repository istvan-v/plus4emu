
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

namespace Plus4Emu {

  void FLTKDisplay_::decodeLine(unsigned char *outBuf,
                                const unsigned char *inBuf, size_t nBytes)
  {
    if (!inBuf)
      nBytes = 0;
    if (nBytes <= 384) {
      const unsigned char *bufp = inBuf;
      size_t  n = nBytes << 1;
      size_t  i = 0;
      for ( ; i < n; i += 2) {
        unsigned char c = *(bufp++);
        outBuf[i] = c;
        outBuf[i + 1] = c;
      }
      for ( ; i < 768; i += 2) {
        outBuf[i] = 0;
        outBuf[i + 1] = 0;
      }
    }
    else {
      size_t  n = (nBytes < 768 ? nBytes : 768);
      std::memcpy(outBuf, inBuf, n);
      if (n < 768)
        std::memset(&(outBuf[n]), 0, 768 - n);
    }
  }

  // --------------------------------------------------------------------------

  FLTKDisplay_::Message::~Message()
  {
  }

  FLTKDisplay_::Message_LineData::~Message_LineData()
  {
  }

  void FLTKDisplay_::Message_LineData::copyLine(const uint8_t *buf,
                                                size_t nBytes)
  {
    unsigned char *p = reinterpret_cast<unsigned char *>(&(buf_[0]));
    size_t  i = 0;
    if (nBytes & 1) {
      p[0] = buf[0];
      i++;
    }
    for ( ; i < nBytes; i += 2) {
      p[i] = buf[i];
      p[i + 1] = buf[i + 1];
    }
    nBytes_ = i;
    for ( ; (i & 3) != 0; i++)
      p[i] = 0;
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
      curLine(0),
      lineCnt(0),
      prvLineCnt(312),
      avgLineCnt(312.0f),
      lineReload(-40),
      framesPending(0),
      skippingFrame(false),
      vsyncState(false),
      videoResampleEnabled(false),
      exitFlag(false),
      displayParameters(),
      savedDisplayParameters(),
      screenshotCallback((void (*)(void *, const unsigned char *, int, int)) 0),
      screenshotCallbackUserData((void *) 0),
      screenshotCallbackCnt(0)
  {
    try {
      lineBuffers = new Message_LineData*[578];
      for (size_t n = 0; n < 578; n++)
        lineBuffers[n] = (Message_LineData *) 0;
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
  }

  void FLTKDisplay_::draw()
  {
  }

  int FLTKDisplay_::handle(int event)
  {
    (void) event;
    return 0;
  }

  void FLTKDisplay_::setDisplayParameters(const DisplayParameters& dp)
  {
    if (dp.displayQuality != savedDisplayParameters.displayQuality ||
        dp.bufferingMode != savedDisplayParameters.bufferingMode) {
      vsyncStateChange(true, 8);
      vsyncStateChange(false, 28);
    }
    Message_SetParameters *m = allocateMessage<Message_SetParameters>();
    m->dp = dp;
    savedDisplayParameters = dp;
    queueMessage(m);
  }

  const VideoDisplay::DisplayParameters&
      FLTKDisplay_::getDisplayParameters() const
  {
    return savedDisplayParameters;
  }

  void FLTKDisplay_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (!skippingFrame) {
      if (curLine >= 0 && curLine < 578) {
        Message_LineData  *m = allocateMessage<Message_LineData>();
        m->lineNum = curLine;
        m->copyLine(buf, nBytes);
        queueMessage(m);
      }
    }
    if (lineCnt < 500) {
      curLine += 2;
      lineCnt++;
    }
  }

  void FLTKDisplay_::vsyncStateChange(bool newState, unsigned int currentSlot_)
  {
    (void) currentSlot_;
    if (newState == vsyncState)
      return;
    vsyncState = newState;
    if (newState) {
      avgLineCnt = (avgLineCnt * 0.95f) + (float(prvLineCnt) * 0.05f);
      int   tmp = int(avgLineCnt + 0.5f);
      tmp = (savedDisplayParameters.displayQuality == 0 ? 272 : 274) - tmp;
      if (lineCnt == (prvLineCnt + 1))
        lineReload = lineReload | 1;
      else
        lineReload = lineReload & (~(int(1)));
      if (tmp <= (lineReload - 2)) {
        if (tmp <= (lineReload - 16))
          lineReload = lineReload - 8;
        else
          lineReload = lineReload - 2;
      }
      else if (tmp >= (lineReload + 2)) {
        if (tmp >= (lineReload + 16))
          lineReload = lineReload + 8;
        else
          lineReload = lineReload + 2;
      }
      curLine = lineReload;
      prvLineCnt = lineCnt;
      lineCnt = 0;
      return;
    }
    messageQueueMutex.lock();
    bool    skippedFrame = skippingFrame;
    if (!skippedFrame)
      framesPending++;
    skippingFrame = (framesPending > 3);    // should this be configurable ?
    messageQueueMutex.unlock();
    if (skippedFrame) {
      Fl::awake();
      threadLock.wait(1);
      return;
    }
    Message *m = allocateMessage<Message_FrameDone>();
    queueMessage(m);
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
      imageBuf_ = new unsigned char[768 * 576 + 768];
      unsigned char *p = imageBuf_;
      for (int c = 0; c <= 255; c++) {
        float   r, g, b;
        r = float(c) / 255.0f;
        g = r;
        b = r;
        if (displayParameters.indexToRGBFunc)
          displayParameters.indexToRGBFunc(uint8_t(c), r, g, b);
        r = r * 255.0f + 0.5f;
        g = g * 255.0f + 0.5f;
        b = b * 255.0f + 0.5f;
        *(p++) = (unsigned char) (r > 0.0f ? (r < 255.5f ? r : 255.5f) : 0.0f);
        *(p++) = (unsigned char) (g > 0.0f ? (g < 255.5f ? g : 255.5f) : 0.0f);
        *(p++) = (unsigned char) (b > 0.0f ? (b < 255.5f ? b : 255.5f) : 0.0f);
      }
      unsigned char lineBuf_[768];
      for (size_t yc = 1; yc < 578; yc++) {
        if (lineBuffers[yc]) {
          const unsigned char *bufp = (unsigned char *) 0;
          size_t  nBytes = 0;
          lineBuffers[yc]->getLineData(bufp, nBytes);
          decodeLine(&(lineBuf_[0]), bufp, nBytes);
        }
        else if (yc == 1 || lineBuffers[yc - 1] == (Message_LineData *) 0)
          std::memset(&(lineBuf_[0]), 0, 768);
        if (yc > 1) {
          std::memcpy(p, &(lineBuf_[0]), 768);
          p = p + 768;
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

  // --------------------------------------------------------------------------

  FLTKDisplay::Colormap::Colormap()
  {
    palette = new uint32_t[256];
    try {
      palette2 = new uint32_t[65536];
    }
    catch (...) {
      delete[] palette;
      throw;
    }
    DisplayParameters dp;
    setParams(dp);
  }

  FLTKDisplay::Colormap::~Colormap()
  {
    delete[] palette;
    delete[] palette2;
  }

  void FLTKDisplay::Colormap::setParams(const DisplayParameters& dp)
  {
    float   rTbl[256];
    float   gTbl[256];
    float   bTbl[256];
    for (size_t i = 0; i < 256; i++) {
      float   r = float(uint8_t(i)) / 255.0f;
      float   g = float(uint8_t(i)) / 255.0f;
      float   b = float(uint8_t(i)) / 255.0f;
      if (dp.indexToRGBFunc)
        dp.indexToRGBFunc(uint8_t(i), r, g, b);
      dp.applyColorCorrection(r, g, b);
      rTbl[i] = r;
      gTbl[i] = g;
      bTbl[i] = b;
    }
    for (size_t i = 0; i < 256; i++) {
      palette[i] = pixelConv(rTbl[i], gTbl[i], bTbl[i]);
    }
    for (size_t i = 0; i < 256; i++) {
      for (size_t j = 0; j < 256; j++) {
        double  r = (rTbl[i] + rTbl[j]) * dp.blendScale1;
        double  g = (gTbl[i] + gTbl[j]) * dp.blendScale1;
        double  b = (bTbl[i] + bTbl[j]) * dp.blendScale1;
        palette2[(i << 8) + j] = pixelConv(r, g, b);
      }
    }
  }

  uint32_t FLTKDisplay::Colormap::pixelConv(double r, double g, double b)
  {
    unsigned int  ri, gi, bi;
    ri = (r > 0.0 ? (r < 1.0 ? (unsigned int) (r * 255.0 + 0.5) : 255U) : 0U);
    gi = (g > 0.0 ? (g < 1.0 ? (unsigned int) (g * 255.0 + 0.5) : 255U) : 0U);
    bi = (b > 0.0 ? (b < 1.0 ? (unsigned int) (b * 255.0 + 0.5) : 255U) : 0U);
    return ((uint32_t(ri) << 16) + (uint32_t(gi) << 8) + uint32_t(bi));
  }

  // --------------------------------------------------------------------------

  FLTKDisplay::FLTKDisplay(int xx, int yy, int ww, int hh, const char *lbl)
    : Fl_Window(xx, yy, ww, hh, lbl),
      FLTKDisplay_(),
      colormap(),
      linesChanged((bool *) 0),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false)
  {
    displayParameters.displayQuality = 0;
    displayParameters.bufferingMode = 0;
    savedDisplayParameters.displayQuality = 0;
    savedDisplayParameters.bufferingMode = 0;
    try {
      linesChanged = new bool[576];
      for (size_t n = 0; n < 576; n++)
        linesChanged[n] = false;
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

    unsigned char lineBuf_[768];
    unsigned char *pixelBuf_ =
        (unsigned char *) std::calloc(size_t(displayWidth_ * 3),
                                      sizeof(unsigned char));
    if (pixelBuf_) {
      int   prvLine_ = -1;
      int   curLine_ = 0;
      int   fracX_ = 0;
      int   fracY_ = 0;
      bool  skippingLine_ = true;
      Message_LineData  *prvLineRendered_ = (Message_LineData *) 0;
      for (int yc = y0; yc < y1; yc++) {
        if (curLine_ != prvLine_) {
          prvLine_ = curLine_;
          Message_LineData  *lBuf_ = lineBuffers[curLine_];
          if (linesChanged[curLine_] || linesChanged[curLine_ ^ 1])
            skippingLine_ = false;
          if (!skippingLine_) {
            if (!lBuf_)
              lBuf_ = lineBuffers[curLine_ ^ 1];
            if (halfResolutionY_ || curLine_ == 0 ||
                !((prvLineRendered_ == (Message_LineData *) 0 &&
                   lBuf_ == (Message_LineData *) 0) ||
                  (prvLineRendered_ != (Message_LineData *) 0 &&
                   lBuf_ != (Message_LineData *) 0 &&
                   (*lBuf_) == (*prvLineRendered_)))) {
              prvLineRendered_ = lBuf_;
              if (lBuf_) {
                // decode video data
                const unsigned char *bufp = (unsigned char *) 0;
                size_t  nBytes = 0;
                lBuf_->getLineData(bufp, nBytes);
                decodeLine(&(lineBuf_[0]), bufp, nBytes);
                // convert to RGB
                bufp = &(lineBuf_[0]);
                unsigned char *p = pixelBuf_;
                uint32_t      c = 0U;
                fracX_ = displayWidth_;
                if (!halfResolutionX_) {
                  fracX_ = (fracX_ >= 768 ? fracX_ : 768);
                  while (true) {
                    if (fracX_ >= displayWidth_) {
                      if (bufp >= &(lineBuf_[768]))
                        break;
                      do {
                        c = colormap(*bufp);
                        fracX_ -= displayWidth_;
                        bufp++;
                      } while (fracX_ >= displayWidth_);
                    }
                    {
                      uint32_t  tmp = c;
                      p[2] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[1] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[0] = (unsigned char) tmp & (unsigned char) 0xFF;
                    }
                    fracX_ += 768;
                    p += 3;
                  }
                }
                else {
                  fracX_ = (fracX_ >= 384 ? fracX_ : 384);
                  while (true) {
                    if (fracX_ >= displayWidth_) {
                      if (bufp >= &(lineBuf_[768]))
                        break;
                      do {
                        c = colormap(bufp[0], bufp[1]);
                        fracX_ -= displayWidth_;
                        bufp += 2;
                      } while (fracX_ >= displayWidth_);
                    }
                    {
                      uint32_t  tmp = c;
                      p[2] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[1] = (unsigned char) tmp & (unsigned char) 0xFF;
                      tmp = tmp >> 8;
                      p[0] = (unsigned char) tmp & (unsigned char) 0xFF;
                    }
                    fracX_ += 384;
                    p += 3;
                  }
                }
              }
              else
                std::memset(pixelBuf_, 0, size_t(displayWidth_ * 3));
            }
          }
        }
        if (!skippingLine_)
          fl_draw_image(pixelBuf_, x0, yc, displayWidth_, 1);
        if (!halfResolutionY_) {
          fracY_ += 576;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            if (curLine_ & 1) {
              linesChanged[curLine_ ^ 1] = false;
              linesChanged[curLine_] = false;
              skippingLine_ = true;
            }
            curLine_ = (curLine_ < 575 ? (curLine_ + 1) : curLine_);
          }
        }
        else {
          fracY_ += 288;
          while (fracY_ >= displayHeight_) {
            fracY_ -= displayHeight_;
            linesChanged[curLine_] = false;
            linesChanged[curLine_ | 1] = false;
            skippingLine_ = true;
            curLine_ = (curLine_ < 573 ? (curLine_ + 2) : curLine_);
          }
        }
      }
      std::free(pixelBuf_);
    }

    // make sure that all lines are updated at a slow rate
    if (!screenshotCallbackCnt) {
      if (forceUpdateLineMask) {
        for (size_t yc = 0; yc < 576; yc++) {
          if (!(forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 1) & 7))))
            continue;
          if (lineBuffers[yc] != (Message_LineData *) 0) {
            Message *m = lineBuffers[yc];
            lineBuffers[yc] = (Message_LineData *) 0;
            deleteMessage(m);
          }
          linesChanged[yc] = true;
        }
        forceUpdateLineMask = 0;
      }
    }
    else
      checkScreenshotCallback();

    messageQueueMutex.lock();
    framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
    messageQueueMutex.unlock();
  }

  void FLTKDisplay::draw()
  {
    if (redrawFlag) {
      redrawFlag = false;
      displayFrame();
      noInputTimer.reset();
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
        if (msg->lineNum >= 0 && msg->lineNum < 576) {
          // check if this line has changed
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0) {
            if (*(lineBuffers[msg->lineNum]) == *msg) {
              deleteMessage(m);
              continue;
            }
          }
          linesChanged[msg->lineNum] = true;
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
        colormap.setParams(tmp_dp);
        for (size_t n = 0; n < 576; n++)
          linesChanged[n] = true;
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 0.33) {
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
    else if (forceUpdateTimer.getRealTime() >= 0.125) {
      forceUpdateLineMask |= (uint8_t(1) << forceUpdateLineCnt);
      forceUpdateLineCnt++;
      forceUpdateLineCnt &= uint8_t(7);
      forceUpdateTimer.reset();
    }
    return redrawFlag;
  }

  int FLTKDisplay::handle(int event)
  {
    (void) event;
    return 0;
  }

  void FLTKDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    DisplayParameters dp_(dp);
    dp_.displayQuality = 0;
    dp_.bufferingMode = 0;
    FLTKDisplay_::setDisplayParameters(dp_);
  }

}       // namespace Plus4Emu

