
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

#ifndef PLUS4EMU_VC1581_HPP
#define PLUS4EMU_VC1581_HPP

#include "plus4emu.hpp"
#include "p4floppy.hpp"
#include "cpu.hpp"
#include "cia8520.hpp"
#include "wd177x.hpp"
#include "serial.hpp"

namespace Plus4 {

  class VC1581 : public FloppyDrive {
   private:
    class M7501_ : public M7501 {
     private:
      VC1581& vc1581;
     public:
      M7501_(VC1581& vc1581_);
      virtual ~M7501_();
     protected:
      virtual void breakPointCallback(int type, uint16_t addr, uint8_t value);
    };
    class CIA8520_ : public CIA8520 {
     private:
      VC1581& vc1581;
     public:
      CIA8520_(VC1581& vc1581_)
        : CIA8520(),
          vc1581(vc1581_)
      {
      }
      virtual ~CIA8520_();
     protected:
      virtual void interruptCallback(bool irqState);
    };
    M7501_    cpu;
    CIA8520_  cia;                      // 4000 to 43FF
    Plus4Emu::WD177x  wd177x;           // 6000 to 63FF
    const uint8_t *memory_rom_0;        // 8000 to BFFF
    const uint8_t *memory_rom_1;        // C000 to FFFF
    uint8_t   memory_ram[8192];         // 0000 to 1FFF
    int       deviceNumber;
    uint8_t   dataBusState;
    bool      interruptRequestFlag;
    uint8_t   ciaPortAInput;
    uint8_t   ciaPortBInput;
    size_t    diskChangeCnt;
    void      (*breakPointCallback)(void *userData,
                                    int debugContext_, int type,
                                    uint16_t addr, uint8_t value);
    void      *breakPointCallbackUserData;
    bool      noBreakOnDataRead;
    // memory read/write callbacks
    static uint8_t readRAM(void *userData, uint16_t addr);
    static uint8_t readDummy(void *userData, uint16_t addr);
    static uint8_t readCIA8520(void *userData, uint16_t addr);
    static uint8_t readWD177x(void *userData, uint16_t addr);
    static uint8_t readROM0(void *userData, uint16_t addr);
    static uint8_t readROM1(void *userData, uint16_t addr);
    static void writeRAM(void *userData, uint16_t addr, uint8_t value);
    static void writeDummy(void *userData, uint16_t addr, uint8_t value);
    static void writeCIA8520(void *userData, uint16_t addr, uint8_t value);
    static void writeWD177x(void *userData, uint16_t addr, uint8_t value);
   public:
    VC1581(int driveNum_ = 8);
    virtual ~VC1581();
    /*!
     * Use 'romData_' (should point to 16384 bytes of data which is expected
     * to remain valid until either a new address is set or the object is
     * destroyed, or can be NULL for no ROM data) for ROM bank 'n'; allowed
     * values for 'n' are:
     *   0: 1581 low
     *   1: 1581 high
     *   2: 1541
     *   3: 1551
     * if this drive type does not use the selected ROM bank, the function call
     * is ignored.
     */
    virtual void setROMImage(int n, const uint8_t *romData_);
    /*!
     * Open disk image file 'fileName_' (an empty file name means no disk).
     */
    virtual void setDiskImageFile(const std::string& fileName_);
    /*!
     * Returns true if there is a disk image file opened.
     */
    virtual bool haveDisk() const;
    /*!
     * Run floppy emulation for one microsecond.
     */
    virtual void runOneCycle(SerialBus& serialBus_);
    /*!
     * Reset floppy drive.
     */
    virtual void reset();
    /*!
     * Returns pointer to the drive CPU.
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
     * Read a byte from drive memory (used for debugging).
     */
    virtual uint8_t readMemoryDebug(uint16_t addr) const;
    /*!
     * Write a byte to drive memory (used for debugging).
     */
    virtual void writeMemoryDebug(uint16_t addr, uint8_t value);
    /*!
     * Returns the current state of drive LEDs. Bit 0 is set if the red LED
     * is on, and bit 1 is set if the green LED is on.
     */
    virtual uint8_t getLEDState() const;
    // snapshot save/load functions
    virtual void saveState(Plus4Emu::File::Buffer&);
    virtual void saveState(Plus4Emu::File&);
    virtual void loadState(Plus4Emu::File::Buffer&);
    virtual void registerChunkTypes(Plus4Emu::File&);
  };

}       // namespace Plus4

#endif  // PLUS4EMU_VC1581_HPP

