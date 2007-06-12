
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

#ifndef PLUS4EMU_VC1551_HPP
#define PLUS4EMU_VC1551_HPP

#include "plus4emu.hpp"
#include "cpu.hpp"
#include "serial.hpp"
#include "p4floppy.hpp"

namespace Plus4 {

  class TPI6523 {
   private:
    uint8_t portAInput;
    uint8_t portAOutput;
    uint8_t portADataDirection;     // NOTE: data directions are XOR'd by 0xFF
    uint8_t portBInput;
    uint8_t portBOutput;
    uint8_t portBDataDirection;
    uint8_t portCInput;
    uint8_t portCOutput;
    uint8_t portCDataDirection;
   public:
    TPI6523()
    {
      portAInput = 0xFF;
      portAOutput = 0x00;
      portADataDirection = 0xFF;
      portBInput = 0xFF;
      portBOutput = 0x00;
      portBDataDirection = 0xFF;
      portCInput = 0xFF;
      portCOutput = 0x00;
      portCDataDirection = 0xFF;
    }
    inline void reset()
    {
      portAOutput = 0x00;
      portADataDirection = 0xFF;
      portBOutput = 0x00;
      portBDataDirection = 0xFF;
      portCOutput = 0x00;
      portCDataDirection = 0xFF;
    }
    inline void setPortA(uint8_t value)
    {
      portAInput = value;
    }
    inline void setPortABits(uint8_t mask_, uint8_t value)
    {
      portAInput = (portAInput & (mask_ ^ uint8_t(0xFF))) | value;
    }
    inline uint8_t getPortA() const
    {
      return (portAInput & (portAOutput | portADataDirection));
    }
    inline uint8_t getPortAOutput() const
    {
      return (portAOutput | portADataDirection);
    }
    inline void setPortB(uint8_t value)
    {
      portBInput = value;
    }
    inline void setPortBBits(uint8_t mask_, uint8_t value)
    {
      portBInput = (portBInput & (mask_ ^ uint8_t(0xFF))) | value;
    }
    inline uint8_t getPortB() const
    {
      return (portBInput & (portBOutput | portBDataDirection));
    }
    inline uint8_t getPortBOutput() const
    {
      return (portBOutput | portBDataDirection);
    }
    inline void setPortC(uint8_t value)
    {
      portCInput = value;
    }
    inline void setPortCBits(uint8_t mask_, uint8_t value)
    {
      portCInput = (portCInput & (mask_ ^ uint8_t(0xFF))) | value;
    }
    inline uint8_t getPortC() const
    {
      return (portCInput & (portCOutput | portCDataDirection));
    }
    inline uint8_t getPortCOutput() const
    {
      return (portCOutput | portCDataDirection);
    }
    inline void writeRegister(uint16_t addr, uint8_t value)
    {
      switch (addr & 0x0007) {
      case 0:
        portAOutput = value;
        break;
      case 1:
        portBOutput = value;
        break;
      case 2:
        portCOutput = value;
        break;
      case 3:
        portADataDirection = value ^ uint8_t(0xFF);
        break;
      case 4:
        portBDataDirection = value ^ uint8_t(0xFF);
        break;
      case 5:
        portCDataDirection = value ^ uint8_t(0xFF);
        break;
      }
    }
    inline uint8_t readRegister(uint16_t addr) const
    {
      switch (addr & 0x0007) {
      case 0:
        return getPortA();
      case 1:
        return getPortB();
      case 2:
        return getPortC();
      case 3:
        return (portADataDirection ^ uint8_t(0xFF));
      case 4:
        return (portBDataDirection ^ uint8_t(0xFF));
      case 5:
        return (portCDataDirection ^ uint8_t(0xFF));
      }
      return uint8_t(0xFF);
    }
  };

  class VC1551 : public FloppyDrive {
   private:
    class M7501_ : public M7501 {
     private:
      VC1551& vc1551;
     public:
      M7501_(VC1551& vc1551_);
      virtual ~M7501_();
     protected:
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    M7501_      cpu;
    const uint8_t *memory_rom;          // 16K ROM, 8000..FFFF
    uint8_t     memory_ram[2048];       // 2K RAM, 0000..0FFF
    uint8_t     trackBuffer_GCR[8192];
    uint8_t     trackBuffer_D64[5376];  // for 21 256-byte sectors
    TPI6523     tpi1;                   // disk controller (1551 4000..7FFF)
    TPI6523     tpi2;                   // parallel port (Plus/4 FEC0..FEFF)
    uint8_t     deviceNumber;
    uint8_t     diskID;
    uint8_t     dataBusState;
    bool        writeProtectFlag;
    bool        trackDirtyFlag;
    bool        headLoadedFlag;
    bool        prvByteWasFF;           // for finding sync
    bool        syncFlag;               // true if found sync
    uint8_t     motorUpdateCnt;         // decrements from 15 to 0
    uint8_t     shiftRegisterBitCnt;    // 0 to 7, byte ready on 0
    int         shiftRegisterBitCntFrac;    // 0 to 65535
    int         interruptTimer;         // decrements from 8325 to -7 at 1 MHz,
                                        // IRQ is active when negative
    int         headPosition;           // index to track buffer
    int         currentTrack;           // 0 to 40 (1 to 35 are valid tracks)
    int         currentTrackFrac;       // -65536 to 65536 (-32768 and 32768
                                        // are "half tracks")
    int         steppingDirection;      // 1: stepping in, -1: stepping out,
                                        // 0: not stepping
    int         currentTrackStepperMotorPhase;
    int         spindleMotorSpeed;      // 0 (stopped) to 65536 (full speed)
    int         nTracks;                // number of tracks (35, 40, or zero
                                        // if there is no disk image file)
    int         diskChangeCnt;          // decrements from 15625 to 0
    uint8_t     idCharacter1;
    uint8_t     idCharacter2;
    std::FILE   *imageFile;
    void        (*breakPointCallback)(void *userData,
                                      int debugContext_,
                                      bool isIO, bool isWrite,
                                      uint16_t addr, uint8_t value);
    void        *breakPointCallbackUserData;
    bool        noBreakOnDataRead;
    // ----------------
    static uint8_t readMemory_RAM(void *userData, uint16_t addr);
    static uint8_t readMemory_Dummy(void *userData, uint16_t addr);
    static uint8_t readMemory_TIA(void *userData, uint16_t addr);
    static uint8_t readMemory_ROM(void *userData, uint16_t addr);
    static void writeMemory_RAM(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_0001(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_Dummy(void *userData, uint16_t addr, uint8_t value);
    static void writeMemory_TIA(void *userData, uint16_t addr, uint8_t value);
    static void gcrEncodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf);
    static bool gcrDecodeFourBytes(uint8_t *outBuf, const uint8_t *inBuf);
    void gcrEncodeTrack(int trackNum, int nSectors, int nBytes);
    int gcrDecodeTrack(int trackNum, int nSectors, int nBytes);
    bool updateMotors();
    bool readTrack(int trackNum = -1);
    bool flushTrack(int trackNum = -1);
    bool setCurrentTrack(int trackNum);
    void updateParallelInterface();
   public:
    VC1551(int driveNum_ = 8);
    virtual ~VC1551();
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
     */
    virtual void setBreakPointCallback(void (*breakPointCallback_)(
                                           void *userData,
                                           int debugContext_,
                                           bool isIO, bool isWrite,
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
    // parallel IEC bus I/O
    // these functions return true if the address is valid for this drive
    bool parallelIECRead(uint16_t addr, uint8_t& value);
    bool parallelIECWrite(uint16_t addr, uint8_t value);
  };

}       // namespace Plus4

#endif  // PLUS4EMU_VC1551_HPP

