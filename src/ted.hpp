
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

#ifndef PLUS4EMU_TED_HPP
#define PLUS4EMU_TED_HPP

#include "plus4emu.hpp"
#include "fileio.hpp"
#include "cpu.hpp"

#ifdef REGPARM
#  undef REGPARM
#endif
#if defined(__GNUC__) && (__GNUC__ >= 3) && defined(__i386__) && !defined(__ICC)
#  define REGPARM __attribute__ ((__regparm__ (3)))
#else
#  define REGPARM
#endif

namespace Plus4 {

  class TED7360 : public M7501 {
   private:
    class VideoShiftRegisterCharacter {
     private:
      uint32_t  buf_;
     public:
      VideoShiftRegisterCharacter()
      {
        this->buf_ = 0U;
      }
      VideoShiftRegisterCharacter(const VideoShiftRegisterCharacter& r)
      {
        this->buf_ = r.buf_;
      }
      inline VideoShiftRegisterCharacter&
          operator=(const VideoShiftRegisterCharacter& r)
      {
        this->buf_ = r.buf_;
        return (*this);
      }
      inline unsigned char& attr_()
      {
        return (((unsigned char *) &(this->buf_))[0]);
      }
      inline unsigned char& char_()
      {
        return (((unsigned char *) &(this->buf_))[1]);
      }
      inline unsigned char& bitmap_()
      {
        return (((unsigned char *) &(this->buf_))[2]);
      }
      // used in standard character modes only:
      //   bits 4 to 7 are set if the cursor is at this character
      //   bit 3 is set if reverse mode is allowed
      //   bits 0 to 2 are undefined
      inline unsigned char& flags_()
      {
        return (((unsigned char *) &(this->buf_))[3]);
      }
    };
    class DelayedEventsMask {
     private:
      uint32_t  mask_;
     public:
      DelayedEventsMask()
        : mask_(0U)
      {
      }
      DelayedEventsMask(const DelayedEventsMask& r)
        : mask_(r.mask_)
      {
      }
      DelayedEventsMask(uint32_t n)
        : mask_(n)
      {
      }
      inline DelayedEventsMask& operator=(const DelayedEventsMask& r)
      {
        this->mask_ = r.mask_;
        return (*this);
      }
      inline DelayedEventsMask& operator=(uint32_t n)
      {
        this->mask_ = n;
        return (*this);
      }
      inline operator uint32_t() const
      {
        return this->mask_;
      }
      inline operator bool() const
      {
        return (this->mask_ != 0U);
      }
      inline void dramRefreshOn()
      {
        this->mask_ |= uint32_t(0x00000001U);
      }
      inline void singleClockModeOff()
      {
        this->mask_ |= uint32_t(0x00000002U);
      }
      inline void incrementVideoLine()
      {
        this->mask_ |= uint32_t(0x00000004U);
      }
      inline void updateVideoLineRegisters()
      {
        this->mask_ |= uint32_t(0x00000008U);
      }
      inline void stopIncrementingDMAPosition()
      {
        this->mask_ |= uint32_t(0x00000010U);
      }
      inline void latchDMAPosition()
      {
        this->mask_ |= uint32_t(0x00000020U);
      }
      inline void latchCharacterPosition()
      {
        this->mask_ |= uint32_t(0x00000040U);
      }
      inline void initializeDisplay()
      {
        this->mask_ |= uint32_t(0x00000080U);
      }
      inline void incrementVideoLineCycle2()
      {
        this->mask_ |= uint32_t(0x00000100U);
      }
      inline void incrementVerticalSub()
      {
        this->mask_ |= uint32_t(0x00000200U);
      }
      inline void updateVerticalSubRegister()
      {
        this->mask_ |= uint32_t(0x00000400U);
      }
      inline void setColorRegister(uint8_t n)
      {
        this->mask_ |= uint32_t(0x00000800U << n);
      }
      inline void singleClockModeOn()
      {
        this->mask_ |= uint32_t(0x00010000U);
      }
      inline void startDMA()
      {
        this->mask_ &= uint32_t(0xFF01FFFFU);
        this->mask_ |= uint32_t(0x00040000U);
      }
      inline void stopDMA()
      {
        this->mask_ &= uint32_t(0xFF01FFFFU);
      }
      inline void dmaCycle(uint8_t n)
      {
        this->mask_ |= uint32_t(0x00010000U << n);
      }
      inline bool dmaStarted() const
      {
        return bool(this->mask_ & 0x007E0000U);
      }
      inline void stopDMADelay1()
      {
        this->mask_ |= uint32_t(0x00800000U);
      }
      inline void timer2Start()
      {
        this->mask_ |= uint32_t(0x01000000U);
      }
      inline void stopTimer2Start()
      {
        this->mask_ &= uint32_t(0xFEFFFFFFU);
      }
      inline void setVerticalScroll()
      {
        this->mask_ |= uint32_t(0x02000000U);
      }
      inline void setHorizontalScroll()
      {
        this->mask_ |= uint32_t(0x04000000U);
      }
      inline void selectRenderer()
      {
        this->mask_ |= uint32_t(0x08000000U);
      }
      inline void setForceSingleClockFlag()
      {
        this->mask_ |= uint32_t(0x10000000U);
      }
      inline void updateCharPosReloadRegisters()
      {
        this->mask_ |= uint32_t(0x20000000U);
      }
      inline void resetVerticalSub()
      {
        this->mask_ |= uint32_t(0x40000000U);
      }
      inline void incrementFlashCounter()
      {
        this->mask_ |= uint32_t(0x80000000U);
      }
    };
    // memory and register read functions
    static uint8_t  read_memory_0000_to_0FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_1000_to_3FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_4000_to_7FFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_8000_to_BFFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_C000_to_FBFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FC00_to_FCFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FD00_to_FEFF(void *userData, uint16_t addr);
    static uint8_t  read_memory_FF00_to_FFFF(void *userData, uint16_t addr);
    static uint8_t  read_register_0000(void *userData, uint16_t addr);
    static uint8_t  read_register_0001(void *userData, uint16_t addr);
    static uint8_t  read_register_FD0x(void *userData, uint16_t addr);
    static uint8_t  read_register_FD1x(void *userData, uint16_t addr);
    static uint8_t  read_register_FD16(void *userData, uint16_t addr);
    static uint8_t  read_register_FD3x(void *userData, uint16_t addr);
    static uint8_t  read_register_FFxx(void *userData, uint16_t addr);
    static uint8_t  read_register_FF00(void *userData, uint16_t addr);
    static uint8_t  read_register_FF01(void *userData, uint16_t addr);
    static uint8_t  read_register_FF02(void *userData, uint16_t addr);
    static uint8_t  read_register_FF03(void *userData, uint16_t addr);
    static uint8_t  read_register_FF04(void *userData, uint16_t addr);
    static uint8_t  read_register_FF05(void *userData, uint16_t addr);
    static uint8_t  read_register_FF06(void *userData, uint16_t addr);
    static uint8_t  read_register_FF09(void *userData, uint16_t addr);
    static uint8_t  read_register_FF0A(void *userData, uint16_t addr);
    static uint8_t  read_register_FF0C(void *userData, uint16_t addr);
    static uint8_t  read_register_FF10(void *userData, uint16_t addr);
    static uint8_t  read_register_FF12(void *userData, uint16_t addr);
    static uint8_t  read_register_FF13(void *userData, uint16_t addr);
    static uint8_t  read_register_FF14(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1A(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1B(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1C(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1E(void *userData, uint16_t addr);
    static uint8_t  read_register_FF1F(void *userData, uint16_t addr);
    static uint8_t  read_register_FF3E_FF3F(void *userData, uint16_t addr);
    // memory and register write functions
    static void     write_memory_0000_to_0FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_1000_to_3FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_4000_to_7FFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_8000_to_BFFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_C000_to_FBFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FC00_to_FCFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FD00_to_FEFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_memory_FF00_to_FFFF(void *userData,
                                              uint16_t addr, uint8_t value);
    static void     write_register_0000(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_0001(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD1x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD16(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FD3x(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FDDx(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FFxx(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF00(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF01(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF02(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF03(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF04(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF05(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF06(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF07(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF08(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF09(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0A(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0B(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0C(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0D(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF0F(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF10(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF12(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF13(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF14(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF15_to_FF19(void *userData,
                                                uint16_t addr, uint8_t value);
    static void     write_register_FF1A(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1B(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1C(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1D(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF1F(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF3E(void *userData,
                                        uint16_t addr, uint8_t value);
    static void     write_register_FF3F(void *userData,
                                        uint16_t addr, uint8_t value);
    // render functions
    static REGPARM int render_BMM_hires(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM int render_BMM_multicolor(TED7360& ted,
                                             uint8_t *bufp, int offs);
    static REGPARM int render_char_std(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM int render_char_ECM(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM int render_char_MCM(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM int render_blank(TED7360& ted, uint8_t *bufp, int offs);
    static REGPARM int render_border(TED7360& ted, uint8_t *bufp, int offs);
    void updateVideoMode();
    void initRegisters();
    void initializeRAMSegment(uint8_t *p);
    void runOneCycle_freezeMode();
    void processDelayedEvents(uint32_t n);
    // -----------------------------------------------------------------
   protected:
    // CPU I/O registers
    uint8_t     ioRegister_0000;
    uint8_t     ioRegister_0001;
   private:
    // TED cycle counter (0 to 3)
    uint8_t     cycle_count;
    // current video column (0 to 113, = (FF1E) / 2)
    uint8_t     videoColumn;
    // base index to memoryMapTable[] (see below) to be used by readMemory()
    unsigned int  memoryReadMap;
    // base index to memoryMapTable[] to be used by writeMemory()
    unsigned int  memoryWriteMap;
    // memoryReadMap is set to one of these before calling readMemory()
    unsigned int  cpuMemoryReadMap;
    unsigned int  tedDMAReadMap;
    unsigned int  tedBitmapReadMap;
   protected:
    // copy of TED registers at FF00 to FF1F
    // NOTE: FF1E is stored shifted right by one bit
    uint8_t     tedRegisters[32];
   private:
    // render function selected by bits of FF06 and FF07
    // returns the number of bytes written to 'bufp'
    REGPARM int (*render_func)(TED7360& ted, uint8_t *bufp, int offs);
    // currently used render function (may be blanking or border)
    REGPARM int (*current_render_func)(TED7360& ted, uint8_t *bufp, int offs);
    // CPU clock multiplier
    int         cpu_clock_multiplier;
    // current video line (0 to 311, = (FF1D, FF1C))
    int         videoLine;
    // character sub-line (0 to 7, bits 0..2 of FF1F)
    int         characterLine;
    // character position (FF1A, FF1B)
    int         prvCharacterPosition;
    int         characterPosition;
    int         nextCharacterPosition;
    int         characterPositionReload;
    int         characterColumn;
    int         dmaPosition;
    int         dmaPositionReload;
    // base address for attribute or character data (FF14 bits 3..7)
    // bit 10 is 0 for attribute DMA, and 1 for character DMA
    int         dmaBaseAddr;
    // base address for bitmap data (FF12 bits 3..5)
    int         bitmap_base_addr;
    // base address for character set (FF13 bits 2..7)
    int         charset_base_addr;
    // cursor position (FF0C, FF0D)
    int         cursor_position;
    // FF07 bit 5
    bool        ted_disabled;
    // flash state (0x00 or 0xFF) for cursor etc.,
    // updated on every 16th video frame
    uint8_t     flashState;
    // display state flags, set depending on video line and column
    bool        renderWindow;
    bool        incrementingCharacterLine;
    // if non-zero, disable address generation for bitmap fetches, and always
    // read from FFFF; bit 0 is cleared at cycle 110 and set at cycle 76, bit 1
    // is cleared at the first character DMA and set at the end of the display
    uint8_t     bitmapAddressDisableFlags;
    bool        displayWindow;
    bool        displayActive;
    // bit 7: horizontal sync
    // bit 6: vertical sync
    // bit 5: horizontal blanking
    // bit 4: vertical blanking
    // bit 3: burst
    // bit 2: PAL odd line (FF1D bit 0)
    // bit 1: always zero
    // bit 0: NTSC mode (FF07 bit 6)
    uint8_t     videoOutputFlags;
    // bit 7: set if vertical sync is on
    // bit 6: set if equalization is on
    uint8_t     vsyncFlags;
    // timers (FF00 to FF05)
    bool        timer1_run;
    bool        timer2_run;
    bool        timer3_run;
    int         timer1_state;
    int         timer1_reload_value;
    int         timer2_state;
    int         timer3_state;
    // sound generators
    int         sound_channel_1_cnt;
    int         sound_channel_1_reload;
    int         sound_channel_2_cnt;
    int         sound_channel_2_reload;
    uint8_t     sound_channel_1_state;
    uint8_t     sound_channel_2_state;
    uint8_t     sound_channel_2_noise_state;
    uint8_t     sound_channel_2_noise_output;
    // video buffers
    uint8_t     attr_buf[64];
    uint8_t     attr_buf_tmp[64];
    uint8_t     char_buf[64];
    uint8_t     video_buf[464];
    int         prv_video_buf_pos;
    int         video_buf_pos;
    bool        videoShiftRegisterEnabled;
    uint8_t     videoMode;      // (FF06 bit 5, 6 OR FF07 bit 4, 7) >> 4
    bool        bitmapMode;
    uint8_t     characterMask;
    VideoShiftRegisterCharacter shiftRegisterCharacter;
    VideoShiftRegisterCharacter currentCharacter;
    VideoShiftRegisterCharacter nextCharacter;
    // copy of color registers at FF15-FF19, used when rendering the first
    // pixel in a cycle; the value stored is FF for one cycle after writing
    // to a register
    uint8_t     colorRegisters[5];
    // horizontal scroll (0 to 7)
    uint8_t     horizontalScroll;
    // vertical scroll (0 to 7)
    uint8_t     verticalScroll;
    // least significant bits (delayed by one cycle) of video line
    // for DMA checking
    uint8_t     savedVideoLineBits0to2;
    bool        dmaEnabled;
    // bit 0: single clock mode controlled by TED
    // bit 1: copied from FF13 bit 1
    // bit 7: DRAM refresh
    uint8_t     singleClockModeFlags;
    // sum of:
    //    1: attribute DMA
    //    2: character DMA
    //  128: DMA enabled in this line
    uint8_t     dmaFlags;
    bool        incrementingDMAPosition;
    bool        incrementingCharacterPosition;
    bool        cpuHaltedFlag;
    DelayedEventsMask   delayedEvents0;
    DelayedEventsMask   delayedEvents1;
    int         savedVideoLine;
    int         videoInterruptLine;
    bool        prvVideoInterruptState;
    uint8_t     prvCharacterLine;
   protected:
    // for reading data from invalid memory address
    uint8_t     dataBusState;
   private:
    // lower 8 bits of DRAM refresh address (upper 8 bits are always 1)
    uint8_t     dramRefreshAddrL;
    // keyboard matrix
    int         keyboard_row_select_mask;
    uint8_t     keyboard_matrix[16];
    // external ports
    uint8_t     user_port_state;
    bool        tape_motor_state;
    bool        tape_read_state;
    bool        tape_write_state;
    bool        tape_button_state;
    // --------
    // number of RAM segments; can be one of the following values:
    //   1: 16K (segment FF)
    //   2: 32K (segments FE, FF)
    //   4: 64K (segments FC to FF)
    //  16: 256K Hannes (segments F0 to FF)
    //  64: 1024K Hannes (segments C0 to FF)
    uint8_t     ramSegments;
    // value written to FD16:
    //   bit 7:       enable memory expansion at 1000-FFFF if set to 0, or
    //                at 4000-FFFF if set to 1
    //   bit 6:       if set to 1, allow access to memory expansion by TED
    //   bits 0 to 3: RAM bank selected
    uint8_t     hannesRegister;
    uint8_t     *segmentTable[256];
    // table of 4096 memory maps, indexed by a 12-bit value multiplied by 8:
    //   bits 8 to 11: ROM banks selected by writing to FDD0 + n
    //   bit 7:        1: do not allow RAM expansion at 1000-3FFF
    //   bit 6:        1: allow use of RAM expansion by TED
    //   bit 5:        memory access by CPU (0) or TED (1)
    //   bit 4:        RAM (0) or ROM (1) selected
    //   bits 0 to 3:  Hannes memory bank
    // each memory map consists of 8 bytes selecting segments for the
    // following address ranges:
    //   0: 1000-3FFF
    //   1: 4000-7FFF
    //   2: 8000-BFFF
    //   3: C000-FBFF
    //   4: 0000-0FFF
    //   5: FC00-FCFF
    //   6: FD00-FEFF
    //   7: FF00-FFFF
    uint8_t     memoryMapTable[32768];
    // --------
    struct TEDCallback {
      void        (*func)(void *);
      void        *userData;
      TEDCallback *nxt0;
      TEDCallback *nxt1;
    };
    TEDCallback callbacks[16];
    TEDCallback *firstCallback0;
    TEDCallback *firstCallback1;
    uint64_t    ramPatternCode;
    uint32_t    randomSeed;
    // -----------------------------------------------------------------
    inline void checkVideoInterrupt()
    {
      if (videoLine == videoInterruptLine) {
        if (!prvVideoInterruptState) {
          prvVideoInterruptState = true;
          tedRegisters[0x09] |= uint8_t(0x02);
        }
      }
      else
        prvVideoInterruptState = false;
    }
    inline void checkDMAPositionReset()
    {
      if (videoLine == 205) {
        if (!incrementingDMAPosition)
          dmaPositionReload = 0x03FF;
      }
    }
    inline void selectRenderFunction()
    {
      if (videoOutputFlags & 0xB0)
        current_render_func = &render_blank;
      else if (!displayActive)
        current_render_func = &render_border;
      else
        current_render_func = render_func;
    }
    inline void idleMemoryRead()
    {
      memoryReadMap = tedDMAReadMap;    // not sure if this is correct
      if (!(singleClockModeFlags & 0x80)) {
        (void) readMemory(0xFFFF);
      }
      else {
        (void) readMemory(0xFF00 | uint16_t(dramRefreshAddrL));
        dramRefreshAddrL = (dramRefreshAddrL + 1) & 0xFF;
      }
      memoryReadMap = cpuMemoryReadMap;
    }
   protected:
    virtual void playSample(int16_t sampleValue)
    {
      (void) sampleValue;
    }
    /*!
     * 'buf' contains 'nBytes' bytes of video data. A group of four pixels
     * is encoded as a flags byte followed by 1 or 4 colormap indices (in the
     * first case, all four pixels have the same color) which can be converted
     * to RGB format with the convertPixelToRGB() function.
     * The flags byte can be the sum of any of the following values:
     *   128: composite sync
     *    64: vertical sync
     *    32: horizontal blanking
     *    16: vertical blanking
     *     8: burst
     *     4: PAL odd line (FF1D bit 0)
     *     2: number of data bytes: 0: 1 byte, 1: 4 bytes
     *     1: NTSC mode (dot clock multiplied by 1.25)
     */
    virtual void videoOutputCallback(const uint8_t *buf, size_t nBytes)
    {
      (void) buf;
      (void) nBytes;
    }
    virtual void ntscModeChangeCallback(bool isNTSC_)
    {
      (void) isNTSC_;
    }
    inline uint8_t ioPortRead() const
    {
      return uint8_t(tape_read_state ? 0xDF : 0xCF);
    }
    inline void ioPortWrite(uint8_t n)
    {
      tape_motor_state = !(n & uint8_t(0x08));
      tape_write_state = bool(n & uint8_t(0x02));
    }
   public:
    TED7360();
    virtual ~TED7360();
    // Load 'cnt' bytes of ROM data from 'buf' to bank 'bankNum' (0 to 3),
    // starting the write from byte 'offs' (0 to 32767). If 'buf' is NULL,
    // the ROM segment at the starting position is deleted, and 'cnt' is
    // ignored.
    void loadROM(int bankNum, int offs, int cnt, const uint8_t *buf);
    // Resize RAM to 'n' kilobytes (16, 32, 64, 256, or 1024), and clear all
    // RAM data to a pattern defined by 'ramPattern':
    //   bits 0 to 2:   address line (0 to 7) for initial value of data bit 0
    //   bit 3:         invert bit 0
    //   bits 4 to 6:   address line (0 to 7) for initial value of data bit 1
    //   bit 7:         invert bit 1
    //   bits 8 to 10:  address line (0 to 7) for initial value of data bit 2
    //   bit 11:        invert bit 2
    //   bits 12 to 14: address line (0 to 7) for initial value of data bit 3
    //   bit 15:        invert bit 3
    //   bits 16 to 18: address line (0 to 7) for initial value of data bit 4
    //   bit 19:        invert bit 4
    //   bits 20 to 22: address line (0 to 7) for initial value of data bit 5
    //   bit 23:        invert bit 5
    //   bits 24 to 26: address line (0 to 7) for initial value of data bit 6
    //   bit 27:        invert bit 6
    //   bits 28 to 30: address line (0 to 7) for initial value of data bit 7
    //   bit 31:        invert bit 7
    //   bits 32 to 39: XOR value for bytes at the beginning of 256 byte pages
    //   bits 40 to 47: probability of random bytes (0: none, 255: maximum)
    void setRAMSize(size_t n, uint64_t ramPattern = 0UL);
    void runOneCycle();
    virtual void reset(bool cold_reset = false);
    void setCPUClockMultiplier(int clk);
    void setKeyState(int keyNum, bool isPressed);
    // Returns memory segment at page 'n' (0 to 3). Segments 0x00 to 0x07 are
    // used for ROM, while segments 0xFC to 0xFF are RAM.
    uint8_t getMemoryPage(int n) const;
    // Returns the type of segment 'n' (0 to 255), which is 0 for no memory,
    // 1 for ROM, and 2 for RAM.
    int getSegmentType(uint8_t n) const;
    // Read memory directly without paging. Valid address ranges are
    // 0x00000000 to 0x0001FFFF for ROM, and 0x003F0000 to 0x003FFFFF for RAM
    // (assuming 64K size).
    uint8_t readMemoryRaw(uint32_t addr) const;
    // Write memory directly without paging. Valid addresses are in the
    // range 0x003F0000 to 0x003FFFFF (i.e. 64K RAM).
    void writeMemoryRaw(uint32_t addr, uint8_t value);
    // Read memory at 16-bit CPU address with paging (this also allows access
    // to I/O and TED registers). If 'forceRAM_' is true, data is always read
    // from RAM.
    uint8_t readMemoryCPU(uint16_t addr, bool forceRAM_ = false) const;
    // Write memory at 16-bit CPU address with paging (this also allows access
    // to I/O and TED registers).
    void writeMemoryCPU(uint16_t addr, uint8_t value);
    inline void setUserPort(uint8_t value)
    {
      user_port_state = value;
    }
    inline uint8_t getUserPort() const
    {
      return user_port_state;
    }
    inline void setTapeInput(bool state)
    {
      tape_read_state = state;
    }
    inline bool getTapeOutput() const
    {
      return tape_write_state;
    }
    inline void setTapeButtonState(bool state)
    {
      tape_button_state = state;
    }
    inline void setTapeMotorState(bool state)
    {
      tape_motor_state = state;
      ioRegister_0001 = (ioRegister_0001 & 0xF7) | (state ? 0x00 : 0x08);
    }
    inline bool getTapeMotorState() const
    {
      return tape_motor_state;
    }
    inline bool getIsNTSCMode() const
    {
      return !!(tedRegisters[0x07] & 0x40);
    }
    inline uint8_t getVideoPositionX() const
    {
      return uint8_t((videoColumn & 0x7F) << 1);
    }
    inline uint16_t getVideoPositionY() const
    {
      return uint16_t(savedVideoLine & 0x01FF);
    }
    // returns true if the raster position is at xPos (0..455), yPos (0..311),
    // and the pixel at that position is not black
    bool checkLightPen(int xPos, int yPos) const;
    // Set function to be called by runOneCycle(). 'flags_' can be one of
    // the following values:
    //   0: do not call the function (removes a previously set callback)
    //   1: call function at single clock frequency, first phase
    //   2: call function at single clock frequency, second phase
    //   3: call the function at double clock frequency
    // The functions are called in the order of being registered; up to 16
    // callbacks can be set.
    void setCallback(void (*func)(void *userData), void *userData_,
                     int flags_ = 1);
    static void convertPixelToRGB(uint8_t color,
                                  float& red, float& green, float& blue);
    // save snapshot
    void saveState(Plus4Emu::File::Buffer&);
    void saveState(Plus4Emu::File&);
    // load snapshot
    void loadState(Plus4Emu::File::Buffer&);
    // save program
    void saveProgram(Plus4Emu::File::Buffer&);
    void saveProgram(Plus4Emu::File&);
    void saveProgram(const char *fileName);
    // load program
    void loadProgram(Plus4Emu::File::Buffer&);
    void loadProgram(const char *fileName);
    void registerChunkTypes(Plus4Emu::File&);
  };

}       // namespace Plus4

#ifdef REGPARM
#  undef REGPARM
#endif

#endif  // PLUS4EMU_TED_HPP

