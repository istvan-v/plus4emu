
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
      // 720 ( = 180 * 4) bytes of space for line data
      uint32_t  buf_[180];
      // number of bytes in buffer
      size_t    nBytes_;
     public:
      // line number
      int       lineNum;
      // bit 7: have burst signal
      // bit 0: resample needed
      // other bits are undefined
      uint8_t   flags;
      size_t    lineLength;
      Message_LineData()
        : Message()
      {
        nBytes_ = 0;
        lineNum = 0;
        flags = 0x00;
        lineLength = 0;
      }
      virtual ~Message_LineData();
      inline void appendData(const uint8_t *buf, size_t nBytes)
      {
        if (nBytes > 0) {
          unsigned char *p = reinterpret_cast<unsigned char *>(&(buf_[0]));
          std::memcpy(&(p[nBytes_]), buf, nBytes);
          nBytes_ = nBytes_ + nBytes;
          for (size_t i = nBytes_; (i & 3) != 0; i++)
            p[i] = 0x00;
        }
      }
      inline void getLineData(const unsigned char*& buf, size_t& nBytes)
      {
        buf = reinterpret_cast<unsigned char *>(&(buf_[0]));
        nBytes = nBytes_;
      }
      bool operator==(const Message_LineData& r) const
      {
        if (r.nBytes_ != nBytes_ || r.flags != this->flags)
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
    void checkScreenshotCallback();
    void frameDone();
    void lineDone();
    // ----------------
    Message       *messageQueue;
    Message       *lastMessage;
    Message       *freeMessageStack;
    Mutex         messageQueueMutex;
    // for 578 lines (576 + 2 border)
    Message_LineData  **lineBuffers;
    Message_LineData  *nextLine;
    int           curLine;
    int           vsyncCnt;
    int           framesPending;
    bool          skippingFrame;
    bool          oddFrame;
    uint8_t       burstValue;
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
    float         lineLengthFilter;
    int           vsyncThreshold1;
    int           vsyncThreshold2;
    int           vsyncReload;
    int           lineReload;
    volatile bool videoResampleEnabled;
    volatile bool exitFlag;
    volatile bool limitFrameRateFlag;
    DisplayParameters   displayParameters;
    DisplayParameters   savedDisplayParameters;
    Timer         limitFrameRateTimer;
    ThreadLock    threadLock;
    int           (*fltkEventCallback)(void *, int);
    void          *fltkEventCallbackUserData;
    void          (*screenshotCallback)(void *,
                                        const unsigned char *, int, int);
    void          *screenshotCallbackUserData;
    int           screenshotCallbackCnt;
   public:
    FLTKDisplay_();
    virtual ~FLTKDisplay_();
    /*!
     * Set color correction and other display parameters.
     * (see 'struct DisplayParameters' above for more information)
     */
    virtual void setDisplayParameters(const DisplayParameters& dp);
    virtual const DisplayParameters& getDisplayParameters() const;
    /*!
     * Read and process 'nBytes' bytes of video data from 'buf'. A group of
     * four pixels is encoded as a flags byte followed by 1 or 4 colormap
     * indices (in the first case, all four pixels have the same color).
     * The flags byte can be the sum of any of the following values:
     *   128: composite sync
     *    64: vertical sync
     *    32: horizontal blanking
     *    16: vertical blanking
     *     8: burst
     *     4: PAL even line
     *     2: number of data bytes: 0: 1 byte, 1: 4 bytes
     *     1: NTSC mode (dot clock multiplied by 1.25)
     */
    virtual void sendVideoOutput(const uint8_t *buf, size_t nBytes);
    /*!
     * Read and process messages sent by the child thread. Returns true if
     * redraw() needs to be called to update the display.
     */
    virtual bool checkEvents() = 0;
    /*!
     * Set function to be called once by checkEvents() after video data for
     * a complete frame has been received. 'buf' contains 'w_' * 'h_' * 3
     * bytes of image data as interleaved red, green, and blue values.
     */
    virtual void setScreenshotCallback(void (*func)(void *userData,
                                                    const unsigned char *buf,
                                                    int w_, int h_),
                                       void *userData_);
    /*!
     * Set function to be called by handle().
     */
    virtual void setFLTKEventCallback(int (*func)(void *userData, int event),
                                      void *userData_ = (void *) 0);
    /*!
     * If enabled, limit the number of frames displayed per second to a
     * maximum of 50.
     */
    virtual void limitFrameRate(bool isEnabled);
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

  // --------------------------------------------------------------------------

  class FLTKDisplay : public Fl_Window, public FLTKDisplay_ {
   private:
    void displayFrame();
    // ----------------
    VideoDisplayColormap<uint32_t>  colormap;
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
    /*!
     * Set color correction and other display parameters.
     * (see 'struct DisplayParameters' above for more information)
     */
    virtual void setDisplayParameters(const DisplayParameters& dp);
    /*!
     * Read and process messages sent by the child thread. Returns true if
     * redraw() needs to be called to update the display.
     */
    virtual bool checkEvents();
   protected:
    virtual void draw();
   public:
    virtual int handle(int event);
  };

}       // namespace Plus4Emu

#endif  // PLUS4EMU_FLDISP_HPP

