
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

#ifndef PLUS4EMU_VC1526_HPP
#define PLUS4EMU_VC1526_HPP

#include "plus4emu.hpp"
#include "serial.hpp"
#include "cpu.hpp"
#include "via6522.hpp"
#include "riot6532.hpp"

namespace Plus4 {

  class VC1526 {
   public:
    static const int    pageWidth = 700;
    static const int    pageHeight = 1000;
    static const int    marginLeft = 36;
    static const int    marginRight = 24;
    static const int    marginTop = 30;
    static const int    marginBottom = 30;
   private:
    static const int    updatePinReload = 10;           // 100 kHz
    static const int    updateMotorReload = 100;        // 10 kHz
    class M6504_ : public M7501 {
     private:
      VC1526& vc1526;
     public:
      M6504_(VC1526& vc1526_);
      virtual ~M6504_();
     protected:
      virtual void breakPointCallback(int type, uint16_t addr, uint8_t value);
    };
    class VIA6522_ : public VIA6522 {
     private:
      VC1526& vc1526;
     public:
      VIA6522_(VC1526& vc1526_);
      virtual ~VIA6522_();
      virtual void irqStateChangeCallback(bool newState);
    };
    M6504_      cpu;            // NOTE: A8, A13, A14, and A15 are ignored
    VIA6522_    via;            // 0240..027F, 02C0..02FF
    RIOT6532    riot1;          // I/O 0200..023F, RAM 0000..007F
    RIOT6532    riot2;          // I/O 0280..02BF, RAM 0080..00FF
    const uint8_t *memory_rom;  // 8K ROM (0400..1FFF)
    int         deviceNumber;
    int         updatePinCnt;
    int         updateMotorCnt;
    int         headPosX;
    int         headPosY;
    int         motorXPhase;
    int         prvMotorXPhase;
    int         motorXCnt;
    int         motorYPhase;
    int         prvMotorYPhase;
    int         motorYCnt;
    uint8_t     pinState;
    uint8_t     prvPinState;
    bool        viaInterruptFlag;
    uint8_t     *pageBuf;       // pageWidth * pageHeight bytes
    void        (*breakPointCallback)(void *userData,
                                      int debugContext_, int type,
                                      uint16_t addr, uint8_t value);
    void        *breakPointCallbackUserData;
    bool        noBreakOnDataRead;
    // --------
    static uint8_t readRIOT1RAM(void *userData, uint16_t addr);
    static void writeRIOT1RAM(void *userData, uint16_t addr, uint8_t value);
    static uint8_t readRIOT2RAM(void *userData, uint16_t addr);
    static void writeRIOT2RAM(void *userData, uint16_t addr, uint8_t value);
    static uint8_t readMemoryROM(void *userData, uint16_t addr);
    static void writeMemoryDummy(void *userData, uint16_t addr, uint8_t value);
    static uint8_t readVIARegister(void *userData, uint16_t addr);
    static void writeVIARegister(void *userData, uint16_t addr, uint8_t value);
    static uint8_t readRIOT1Register(void *userData, uint16_t addr);
    static void writeRIOT1Register(void *userData,
                                   uint16_t addr, uint8_t value);
    static uint8_t readRIOT2Register(void *userData, uint16_t addr);
    static void writeRIOT2Register(void *userData,
                                   uint16_t addr, uint8_t value);
    void updatePins();
    void updateMotors();
    static inline int yPosToPixel(int n)
    {
      return ((n * 7) / 18);
    }
    static inline int pixelToYPos(int n)
    {
      return ((n * 18) / 7);
    }
   public:
    VC1526(int devNum_ = 4);
    virtual ~VC1526();
    virtual void setROMImage(const uint8_t *romData_);
    /*!
     * Run printer emulation for one microsecond (1 MHz clock frequency).
     */
    virtual void runOneCycle(SerialBus& serialBus_);
    virtual const uint8_t *getPageData() const;
    virtual int getPageWidth() const;
    virtual int getPageHeight() const;
    virtual void clearPage();
    virtual uint8_t getLEDState() const;
    virtual void reset();
    /*!
     * Returns pointer to the printer CPU.
     */
    virtual M7501 * getCPU();
    virtual const M7501 * getCPU() const;
    /*!
     * Set function to be called when a breakpoint is triggered.
     * 'type' can be one of the following values:
     *   0: breakpoint at opcode read
     *   1: memory read
     *   2: memory write
     *   3: opcode read in single step mode
     */
    virtual void setBreakPointCallback(void (*breakPointCallback_)(
                                           void *userData,
                                           int debugContext_, int type,
                                           uint16_t addr, uint8_t value),
                                       void *userData_);
    /*!
     * If 'n' is true, breakpoints will not be triggered on reads from
     * any memory address other than the current value of the program
     * counter.
     */
    virtual void setNoBreakOnDataRead(bool n);
    /*!
     * Read a byte from printer memory (used for debugging).
     */
    virtual uint8_t readMemoryDebug(uint16_t addr) const;
    /*!
     * Write a byte to printer memory (used for debugging).
     */
    virtual void writeMemoryDebug(uint16_t addr, uint8_t value);
  };

}       // namespace Plus4

#endif  // PLUS4EMU_VC1526_HPP

