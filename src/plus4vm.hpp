
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

#ifndef PLUS4EMU_PLUS4VM_HPP
#define PLUS4EMU_PLUS4VM_HPP

#include "plus4emu.hpp"
#include "ted.hpp"
#include "display.hpp"
#include "soundio.hpp"
#include "vm.hpp"
#include "p4floppy.hpp"

namespace Plus4 {

  class SID;

  class Plus4VM : public Plus4Emu::VirtualMachine {
   private:
    class TED7360_ : public TED7360 {
     private:
      Plus4VM&  vm;
      int       lineCnt_;
      static uint8_t sidRegisterRead(void *userData, uint16_t addr);
      static void sidRegisterWrite(void *userData,
                                   uint16_t addr, uint8_t value);
      static uint8_t parallelIECRead(void *userData, uint16_t addr);
      static void parallelIECWrite(void *userData,
                                   uint16_t addr, uint8_t value);
     public:
      TED7360_(Plus4VM& vm_);
      virtual ~TED7360_();
     protected:
      virtual void playSample(int16_t sampleValue);
      virtual void drawLine(const uint8_t *buf, size_t nBytes);
      virtual void verticalSync(bool newState_, unsigned int currentSlot_);
      virtual void ntscModeChangeCallback(bool isNTSC_);
      virtual bool systemCallback(uint8_t n);
      virtual void breakPointCallback(bool isWrite,
                                      uint16_t addr, uint8_t value);
    };
    // ----------------
    TED7360_  *ted;
    size_t    cpuClockFrequency;        // defaults to 1
    size_t    tedInputClockFrequency;   // defaults to 17734475 Hz
    size_t    soundClockFrequency;      // fixed at single clock frequency / 4
    int64_t   tedTimesliceLength;       // in 2^-32 microsecond units
    int64_t   tedTimeRemaining;         // -"-
    int64_t   tapeTimesliceLength;      // -"-
    int64_t   tapeTimeRemaining;        // -"-
    int64_t   floppyTimeRemaining;      // -"-
    Plus4Emu::File  *demoFile;
    // contains demo data, which is the emulator version number as a 32-bit
    // integer ((MAJOR << 16) + (MINOR << 8) + PATCHLEVEL), followed by a
    // sequence of events in the following format:
    //   uint64_t   deltaTime   (in TED cycles, stored as MSB first dynamic
    //                          length (1 to 8 bytes) value)
    //   uint8_t    eventType   (currently allowed values are 0 for end of
    //                          demo (zero data bytes), 1 for key press, and
    //                          2 for key release)
    //   uint8_t    dataLength  number of event data bytes
    //   ...        eventData   (dataLength bytes)
    // the event data for event types 1 and 2 is the key code with a length of
    // one byte:
    //   uint8_t    keyCode     key code in the range 0 to 127
    Plus4Emu::File::Buffer  demoBuffer;
    // true while recording a demo
    bool      isRecordingDemo;
    // true while playing a demo
    bool      isPlayingDemo;
    // true after loading a snapshot; if not playing a demo as well, the
    // keyboard state will be cleared
    bool      snapshotLoadFlag;
    // used for counting time between demo events (in TED cycles)
    uint64_t  demoTimeCnt;
    SID       *sid_;
    int32_t   soundOutputAccumulator;
    bool      sidEnabled;
    FloppyDrive *floppyDrives[4];
    uint8_t   *floppyROM_1541;
    uint8_t   *floppyROM_1551;
    uint8_t   *floppyROM_1581_0;
    uint8_t   *floppyROM_1581_1;
    // ----------------
    void stopDemoPlayback();
    void stopDemoRecording(bool writeFile_);
    void updateTimingParameters(bool ntscMode_);
    void resetFloppyDrives(uint8_t driveMask_, bool deleteUnusedDrives_);
    inline M7501 * getDebugCPU()
    {
      if (currentDebugContext == 0)
        return ted;
      else if (floppyDrives[currentDebugContext - 1] != (FloppyDrive *) 0)
        return (floppyDrives[currentDebugContext - 1]->getCPU());
      return (M7501 *) 0;
    }
    inline const M7501 * getDebugCPU() const
    {
      if (currentDebugContext == 0)
        return ted;
      else if (floppyDrives[currentDebugContext - 1] != (FloppyDrive *) 0)
        return (floppyDrives[currentDebugContext - 1]->getCPU());
      return (M7501 *) 0;
    }
   public:
    Plus4VM(Plus4Emu::VideoDisplay&, Plus4Emu::AudioOutput&);
    virtual ~Plus4VM();
    /*!
     * Run emulation for the specified number of microseconds.
     */
    virtual void run(size_t microseconds);
    /*!
     * Reset emulated machine; if 'isColdReset' is true, RAM is cleared.
     */
    virtual void reset(bool isColdReset = false);
    /*!
     * Delete all ROM segments, and resize RAM to 'memSize' kilobytes;
     * implies calling reset(true).
     */
    virtual void resetMemoryConfiguration(size_t memSize);
    /*!
     * Load ROM segment 'n' from the specified file, skipping 'offs' bytes.
     */
    virtual void loadROMSegment(uint8_t n, const char *fileName, size_t offs);
    /*!
     * Set CPU clock frequency (in Hz, or clock multiplier if a small value
     * is specified); defaults to 1.
     */
    virtual void setCPUFrequency(size_t freq_);
    /*!
     * Set TED input clock frequency (defaults to 17734475 Hz).
     */
    virtual void setVideoFrequency(size_t freq_);
    /*!
     * Set state of key 'keyCode' (0 to 127).
     */
    virtual void setKeyboardState(int keyCode, bool isPressed);
    // -------------------------- DISK AND FILE I/O ---------------------------
    /*!
     * Load disk image for drive 'n' (counting from zero); an empty file
     * name means no disk.
     */
    virtual void setDiskImageFile(int n, const std::string& fileName_,
                                  int nTracks_ = -1, int nSides_ = 2,
                                  int nSectorsPerTrack_ = 9);
    // ---------------------------- TAPE EMULATION ----------------------------
    /*!
     * Set tape image file name (if the file name is NULL or empty, tape
     * emulation is disabled).
     */
    virtual void setTapeFileName(const std::string& fileName);
    /*!
     * Start tape playback.
     */
    virtual void tapePlay();
    /*!
     * Start tape recording; if the tape file is read-only, this is
     * equivalent to calling tapePlay().
     */
    virtual void tapeRecord();
    /*!
     * Stop tape playback and recording.
     */
    virtual void tapeStop();
    // ------------------------------ DEBUGGING -------------------------------
    /*!
     * Set the debugging context (CPU number).
     *   0: main CPU
     *   1: floppy drive (unit 8)
     *   2: floppy drive (unit 9)
     *   3: floppy drive (unit 10)
     *   4: floppy drive (unit 11)
     */
    virtual void setDebugContext(int n);
    /*!
     * Add breakpoints from the specified breakpoint list (see also
     * bplist.hpp).
     */
    virtual void setBreakPoints(const Plus4Emu::BreakPointList& bpList);
    /*!
     * Returns the currently defined breakpoints.
     */
    virtual Plus4Emu::BreakPointList getBreakPoints();
    /*!
     * Clear all breakpoints.
     */
    virtual void clearBreakPoints();
    /*!
     * Set breakpoint priority threshold (0 to 4); breakpoints with a
     * priority less than this value will not trigger a break.
     */
    virtual void setBreakPointPriorityThreshold(int n);
    /*!
     * If 'n' is true, breakpoints will not be triggered on reads from
     * any memory address other than the current value of the program
     * counter.
     */
    virtual void setNoBreakOnDataRead(bool n);
    /*!
     * Set if the breakpoint callback should be called whenever the first byte
     * of a CPU instruction is read from memory. Breakpoints are ignored in
     * this mode.
     */
    virtual void setSingleStepMode(bool isEnabled, bool stepOverFlag = false);
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
     * Returns the segment at page 'n' (0 to 3).
     */
    virtual uint8_t getMemoryPage(int n) const;
    /*!
     * Read a byte from memory. If 'isCPUAddress' is false, bits 14 to 21 of
     * 'addr' define the segment number, while bits 0 to 13 are the offset
     * (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
     * a 16-bit CPU address.
     */
    virtual uint8_t readMemory(uint32_t addr, bool isCPUAddress = false) const;
    /*!
     * Write a byte to memory. If 'isCPUAddress' is false, bits 14 to 21 of
     * 'addr' define the segment number, while bits 0 to 13 are the offset
     * (0 to 0x3FFF) within the segment; otherwise, 'addr' is interpreted as
     * a 16-bit CPU address.
     * NOTE: calling this function will stop any demo recording or playback.
     */
    virtual void writeMemory(uint32_t addr, uint8_t value,
                             bool isCPUAddress = false);
    /*!
     * Returns the current value of the CPU program counter (PC).
     */
    virtual uint16_t getProgramCounter() const;
    /*!
     * Returns the CPU address of the last byte pushed to the stack.
     */
    virtual uint16_t getStackPointer() const;
    /*!
     * Dumps the current values of all CPU registers to 'buf' in ASCII format.
     * The register list may be written as multiple lines separated by '\n'
     * characters, however, there is no newline character at the end of the
     * buffer. The maximum line width is 40 characters.
     */
    virtual void listCPURegisters(std::string& buf) const;
    /*!
     * Disassemble one CPU instruction, starting from memory address 'addr',
     * and write the result to 'buf' (not including a newline character).
     * 'offs' is added to the instruction address that is printed.
     * The maximum line width is 40 characters.
     * Returns the address of the next instruction. If 'isCPUAddress' is
     * true, 'addr' is interpreted as a 16-bit CPU address, otherwise it
     * is assumed to be a 22-bit physical address (8 bit segment + 14 bit
     * offset).
     */
    virtual uint32_t disassembleInstruction(std::string& buf, uint32_t addr,
                                            bool isCPUAddress = false,
                                            int32_t offs = 0) const;
    /*!
     * Returns read-only reference to a structure containing all CPU
     * registers; see plus4/cpu.hpp for more information.
     */
    virtual const M7501Registers& getCPURegisters() const;
    // ------------------------------- FILE I/O -------------------------------
    /*!
     * Save snapshot of virtual machine state, including all ROM and RAM
     * segments, as well as all hardware registers. Note that the clock
     * frequency and timing settings, tape and disk state, and breakpoint list
     * are not saved.
     */
    virtual void saveState(Plus4Emu::File&);
    /*!
     * Save clock frequency and timing settings.
     */
    virtual void saveMachineConfiguration(Plus4Emu::File&);
    /*!
     * Save program.
     */
    virtual void saveProgram(Plus4Emu::File&);
    virtual void saveProgram(const char *fileName);
    /*!
     * Load program.
     */
    virtual void loadProgram(const char *fileName);
    /*!
     * Register all types of file data supported by this class, for use by
     * Plus4Emu::File::processAllChunks(). Note that loading snapshot data
     * will clear all breakpoints.
     */
    virtual void registerChunkTypes(Plus4Emu::File&);
    /*!
     * Start recording a demo to the file object, which will be used until
     * the recording is stopped for some reason.
     * Implies calling saveMachineConfiguration() and saveState() first.
     */
    virtual void recordDemo(Plus4Emu::File&);
    /*!
     * Stop playing or recording demo.
     */
    virtual void stopDemo();
    /*!
     * Returns true if a demo is currently being recorded. The recording stops
     * when stopDemo() is called, any tape or disk I/O is attempted, clock
     * frequency and timing settings are changed, or a snapshot is loaded.
     * This function will also flush demo data to the associated file object
     * after recording is stopped for some reason other than calling
     * stopDemo().
     */
    virtual bool getIsRecordingDemo();
    /*!
     * Returns true if a demo is currently being played. The playback stops
     * when the end of the demo is reached, stopDemo() is called, any tape or
     * disk I/O is attempted, clock frequency and timing settings are changed,
     * or a snapshot is loaded. Note that keyboard events are ignored while
     * playing a demo.
     */
    virtual bool getIsPlayingDemo() const;
    // ----------------
    virtual void loadState(Plus4Emu::File::Buffer&);
    virtual void loadMachineConfiguration(Plus4Emu::File::Buffer&);
    virtual void loadDemo(Plus4Emu::File::Buffer&);
  };

}       // namespace Plus4

#endif  // PLUS4EMU_PLUS4VM_HPP

