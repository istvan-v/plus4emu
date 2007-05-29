
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

#ifndef PLUS4EMU_FLDISP_HPP
#define PLUS4EMU_FLDISP_HPP

#include "plus4emu.hpp"
#include "system.hpp"
#include "display.hpp"

#include <FL/Fl_Window.H>

namespace Plus4Emu {

  class FLTKDisplay_ : public VideoDisplay {
   protected:
    class Message {
     public:
      Message *prv;
      Message *nxt;
      // --------
      Message()
      {
        prv = (Message *) 0;
        nxt = (Message *) 0;
      }
      virtual ~Message();
    };
    class Message_LineData : public Message {
     private:
      // a line of 768 pixels needs a maximum space of 768 * (17 / 16) = 816
      // ( = 204 * 4) bytes in compressed format
      uint32_t  buf_[204];
      // number of bytes in buffer
      size_t    nBytes_;
     public:
      // line number
      int       lineNum;
      Message_LineData()
        : Message()
      {
        nBytes_ = 0;
        lineNum = 0;
      }
      virtual ~Message_LineData();
      // copy a line (768 pixels in compressed format) to the buffer
      void copyLine(const uint8_t *buf, size_t nBytes);
      inline void getLineData(const unsigned char*& buf, size_t& nBytes)
      {
        buf = reinterpret_cast<unsigned char *>(&(buf_[0]));
        nBytes = nBytes_;
      }
      bool operator==(const Message_LineData& r) const
      {
        if (r.nBytes_ != nBytes_)
          return false;
        size_t  n = (nBytes_ + 3) >> 2;
        for (size_t i = 0; i < n; i++) {
          if (r.buf_[i] != buf_[i])
            return false;
        }
        return true;
      }
    };
    class Message_FrameDone : public Message {
     public:
      Message_FrameDone()
        : Message()
      {
      }
      virtual ~Message_FrameDone();
    };
    class Message_SetParameters : public Message {
     public:
      DisplayParameters dp;
      Message_SetParameters()
        : Message(),
          dp()
      {
      }
      virtual ~Message_SetParameters();
    };
    template <typename T>
    T * allocateMessage()
    {
      void  *m_ = (void *) 0;
      messageQueueMutex.lock();
      if (freeMessageStack) {
        Message *m = freeMessageStack;
        freeMessageStack = m->nxt;
        if (freeMessageStack)
          freeMessageStack->prv = (Message *) 0;
        m_ = m;
      }
      messageQueueMutex.unlock();
      if (!m_) {
        // allocate space that is enough for the largest message type
        m_ = std::malloc((sizeof(Message_LineData) | 15) + 1);
        if (!m_)
          throw std::bad_alloc();
      }
      T *m;
      try {
        m = new(m_) T();
      }
      catch (...) {
        std::free(m_);
        throw;
      }
      return m;
    }
    void deleteMessage(Message *m);
    void queueMessage(Message *m);
    static void decodeLine(unsigned char *outBuf,
                           const unsigned char *inBuf, size_t nBytes);
    void checkScreenshotCallback();
    void frameDone();
    // ----------------
    Message       *messageQueue;
    Message       *lastMessage;
    Message       *freeMessageStack;
    Mutex         messageQueueMutex;
    // for 578 lines (576 + 2 border)
    Message_LineData  **lineBuffers;
    int           curLine;
    int           lineCnt;      // nr. of lines received so far in this frame
    int           prvLineCnt;
    int           vsyncCnt;
    int           framesPending;
    bool          skippingFrame;
    bool          vsyncState;
    bool          ntscMode;
    bool          oddFrame;
    volatile bool videoResampleEnabled;
    volatile bool exitFlag;
    DisplayParameters   displayParameters;
    DisplayParameters   savedDisplayParameters;
    ThreadLock    threadLock;
    void          (*screenshotCallback)(void *,
                                        const unsigned char *, int, int);
    void          *screenshotCallbackUserData;
    int           screenshotCallbackCnt;
   public:
    FLTKDisplay_();
    virtual ~FLTKDisplay_();
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    virtual void setDisplayParameters(const DisplayParameters& dp);
    virtual const DisplayParameters& getDisplayParameters() const;
    // Draw next line of display. 'nBytes' should be 768 for full resolution,
    // or 384 for half resolution. With values in the range 0 to 383, or
    // 385 to 767, the remaining pixels are filled with color 0.
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    // Should be called at the beginning (newState = true) and end
    // (newState = false) of VSYNC. 'currentSlot_' is the position within
    // the current line (0 to 56).
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    // Read and process messages sent by the child thread. Returns true if
    // redraw() needs to be called to update the display.
    virtual bool checkEvents() = 0;
    // Set function to be called once by checkEvents() after video data for
    // a complete frame has been received. 'buf' contains 768 bytes of
    // colormap data (256*3 interleaved red, green, and blue values) followed
    // by 'w_' * 'h_' bytes of image data.
    virtual void setScreenshotCallback(void (*func)(void *userData,
                                                    const unsigned char *buf,
                                                    int w_, int h_),
                                       void *userData_);
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

  // --------------------------------------------------------------------------

  class FLTKDisplay : public Fl_Window, public FLTKDisplay_ {
   private:
    class Colormap {
     private:
      uint32_t  *palette;
      uint32_t  *palette2;
      static uint32_t pixelConv(double r, double g, double b);
     public:
      Colormap();
      ~Colormap();
      void setParams(const DisplayParameters& dp);
      inline uint32_t operator()(uint8_t c) const
      {
        return palette[c];
      }
      inline uint32_t operator()(uint8_t c1, uint8_t c2) const
      {
        return palette2[(size_t(c1) << 8) + c2];
      }
    };
    void displayFrame();
    // ----------------
    Colormap      colormap;
    // linesChanged[n] & 0x01 is non-zero if video data for line n has been
    // received in the current frame; linesChanged[n] & 0x80 is non-zero if
    // line n has changed in the current frame
    uint8_t       *linesChanged;
    uint8_t       forceUpdateLineCnt;
    uint8_t       forceUpdateLineMask;
    bool          redrawFlag;
    Timer         noInputTimer;
    Timer         forceUpdateTimer;
   public:
    FLTKDisplay(int xx = 0, int yy = 0, int ww = 768, int hh = 576,
                const char *lbl = (char *) 0);
    virtual ~FLTKDisplay();
    // set color correction and other display parameters
    // (see 'struct DisplayParameters' above for more information)
    virtual void setDisplayParameters(const DisplayParameters& dp);
    // Read and process messages sent by the child thread. Returns true if
    // redraw() needs to be called to update the display.
    virtual bool checkEvents();
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_FLDISP_HPP

