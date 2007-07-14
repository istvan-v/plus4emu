
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

#ifndef PLUS4EMU_P4FLOPPY_HPP
#define PLUS4EMU_P4FLOPPY_HPP

#include "plus4emu.hpp"
#include "fileio.hpp"

namespace Plus4 {

  class M7501;
  class SerialBus;

  class FloppyDrive {
   public:
    FloppyDrive(int driveNum_ = 8)
    {
      (void) driveNum_;
    }
    virtual ~FloppyDrive()
    {
    }
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
    virtual void setROMImage(int n, const uint8_t *romData_) = 0;
    /*!
     * Open disk image file 'fileName_' (an empty file name means no disk).
     */
    virtual void setDiskImageFile(const std::string& fileName_) = 0;
    /*!
     * Returns true if there is a disk image file opened.
     */
    virtual bool haveDisk() const = 0;
    /*!
     * Run floppy emulation for one microsecond.
     */
    virtual void runOneCycle(SerialBus& serialBus_) = 0;
    /*!
     * Reset floppy drive.
     */
    virtual void reset() = 0;
    /*!
     * Returns pointer to the drive CPU.
     */
    virtual M7501 * getCPU() = 0;
    virtual const M7501 * getCPU() const = 0;
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
                                       void *userData_) = 0;
    /*!
     * If 'n' is true, breakpoints will not be triggered on reads from
     * any memory address other than the current value of the program
     * counter.
     */
    virtual void setNoBreakOnDataRead(bool n) = 0;
    /*!
     * Read a byte from drive memory (used for debugging).
     */
    virtual uint8_t readMemoryDebug(uint16_t addr) const = 0;
    /*!
     * Write a byte to drive memory (used for debugging).
     */
    virtual void writeMemoryDebug(uint16_t addr, uint8_t value) = 0;
    /*!
     * Returns the current state of drive LEDs. Bit 0 is set if the red LED
     * is on, and bit 1 is set if the green LED is on.
     */
    virtual uint8_t getLEDState() const = 0;
    // snapshot save/load functions
    virtual void saveState(Plus4Emu::File::Buffer&) = 0;
    virtual void saveState(Plus4Emu::File&) = 0;
    virtual void loadState(Plus4Emu::File::Buffer&) = 0;
    virtual void registerChunkTypes(Plus4Emu::File&) = 0;
  };

}       // namespace Plus4

#endif  // PLUS4EMU_P4FLOPPY_HPP

