
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

#include "gui.hpp"
#include "monitor.hpp"

#include <cstdio>
#include <vector>

#define MONITOR_MAX_LINES   (100)

// ----------------------------------------------------------------------------

static void tokenizeString(std::vector<std::string>& args, const char *s)
{
  args.resize(0);
  if (!s)
    return;
  std::string curToken = "";
  int         mode = 0;         // 0: skipping space, 1: token, 2: string
  while (*s != '\0') {
    if (mode == 0) {
      if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        s++;
        continue;
      }
      mode = (*s != '"' ? 1 : 2);
    }
    if (mode == 1) {
      if (args.size() == 0 && curToken.length() > 0) {
        // allow no space between command and first hexadecimal argument
        if ((*s >= '0' && *s <= '9') ||
            (*s >= 'A' && *s <= 'F') || (*s >= 'a' && *s <= 'f')) {
          args.push_back(curToken);
          curToken = "";
          continue;
        }
      }
      if ((*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') ||
          *s == '_' || *s == '?') {
        curToken += (*s);
        s++;
        continue;
      }
      else if (*s >= 'a' && *s <= 'z') {
        // convert to upper case
        curToken += ((*s - 'a') + 'A');
        s++;
        continue;
      }
      else {
        if (curToken.length() > 0) {
          args.push_back(curToken);
          curToken = "";
        }
        if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
          mode = 0;
          s++;
          continue;
        }
        if (*s != '"') {
          curToken = (*s);
          args.push_back(curToken);
          curToken = "";
          mode = 0;
          s++;
          continue;
        }
        mode = 2;
      }
    }
    if (*s == '"' && curToken.length() > 0) {
      // closing quote character is not stored
      args.push_back(curToken);
      curToken = "";
      mode = 0;
    }
    else {
      curToken += (*s);
    }
    s++;
  }
  if (curToken.length() > 0)
    args.push_back(curToken);
}

static bool parseHexNumber(uint32_t& n, const char *s)
{
  if (*s == '\0')
    return false;
  n = 0U;
  do {
    n = n << 4;
    if (*s >= '0' && *s <= '9')
      n = n | uint32_t(*s - '0');
    else if (*s >= 'A' && *s <= 'F')
      n = n | uint32_t((*s - 'A') + 10);
    else if (*s >= 'a' && *s <= 'f')
      n = n | uint32_t((*s - 'a') + 10);
    else
      return false;
    s++;
  } while (*s != '\0');
  return true;
}

static uint32_t parseHexNumberEx(const char *s, uint32_t mask_ = 0xFFFFFFFFU)
{
  uint32_t  n = 0U;
  if (!parseHexNumber(n, s))
    throw Plus4Emu::Exception("invalid hexadecimal number format");
  return (n & mask_);
}

// ----------------------------------------------------------------------------

struct AssemblerOpcodeTableEntry {
  const char  *name;
  uint8_t     opcode;
};

// A: implied
// B: immediate
// C: zeropage
// D: zeropage, X
// E: zeropage, Y
// F: absolute / relative
// G: absolute, X
// H: absolute, Y
// I: (indirect)
// J: (indirect, X)
// K: (indirect), Y

static const AssemblerOpcodeTableEntry  assemblerOpcodeTable[221] = {
  { "???A", 0x02 },   { "ADCB", 0x69 },   { "ADCC", 0x65 },   { "ADCD", 0x75 },
  { "ADCF", 0x6D },   { "ADCG", 0x7D },   { "ADCH", 0x79 },   { "ADCJ", 0x61 },
  { "ADCK", 0x71 },   { "ANCB", 0x0B },   { "ANDB", 0x29 },   { "ANDC", 0x25 },
  { "ANDD", 0x35 },   { "ANDF", 0x2D },   { "ANDG", 0x3D },   { "ANDH", 0x39 },
  { "ANDJ", 0x21 },   { "ANDK", 0x31 },   { "ANEB", 0x8B },   { "ARRB", 0x6B },
  { "ASLA", 0x0A },   { "ASLC", 0x06 },   { "ASLD", 0x16 },   { "ASLF", 0x0E },
  { "ASLG", 0x1E },   { "ASRB", 0x4B },   { "BCCF", 0x90 },   { "BCSF", 0xB0 },
  { "BEQF", 0xF0 },   { "BITC", 0x24 },   { "BITF", 0x2C },   { "BMIF", 0x30 },
  { "BNEF", 0xD0 },   { "BPLF", 0x10 },   { "BRKA", 0x00 },   { "BVCF", 0x50 },
  { "BVSF", 0x70 },   { "CLCA", 0x18 },   { "CLDA", 0xD8 },   { "CLIA", 0x58 },
  { "CLVA", 0xB8 },   { "CMPB", 0xC9 },   { "CMPC", 0xC5 },   { "CMPD", 0xD5 },
  { "CMPF", 0xCD },   { "CMPG", 0xDD },   { "CMPH", 0xD9 },   { "CMPJ", 0xC1 },
  { "CMPK", 0xD1 },   { "CPXB", 0xE0 },   { "CPXC", 0xE4 },   { "CPXF", 0xEC },
  { "CPYB", 0xC0 },   { "CPYC", 0xC4 },   { "CPYF", 0xCC },   { "DCPC", 0xC7 },
  { "DCPD", 0xD7 },   { "DCPF", 0xCF },   { "DCPG", 0xDF },   { "DCPH", 0xDB },
  { "DCPJ", 0xC3 },   { "DCPK", 0xD3 },   { "DECC", 0xC6 },   { "DECD", 0xD6 },
  { "DECF", 0xCE },   { "DECG", 0xDE },   { "DEXA", 0xCA },   { "DEYA", 0x88 },
  { "EORB", 0x49 },   { "EORC", 0x45 },   { "EORD", 0x55 },   { "EORF", 0x4D },
  { "EORG", 0x5D },   { "EORH", 0x59 },   { "EORJ", 0x41 },   { "EORK", 0x51 },
  { "INCC", 0xE6 },   { "INCD", 0xF6 },   { "INCF", 0xEE },   { "INCG", 0xFE },
  { "INXA", 0xE8 },   { "INYA", 0xC8 },   { "ISBC", 0xE7 },   { "ISBD", 0xF7 },
  { "ISBF", 0xEF },   { "ISBG", 0xFF },   { "ISBH", 0xFB },   { "ISBJ", 0xE3 },
  { "ISBK", 0xF3 },   { "JMPF", 0x4C },   { "JMPI", 0x6C },   { "JSRF", 0x20 },
  { "LASH", 0xBB },   { "LAXC", 0xA7 },   { "LAXE", 0xB7 },   { "LAXF", 0xAF },
  { "LAXH", 0xBF },   { "LAXJ", 0xA3 },   { "LAXK", 0xB3 },   { "LDAB", 0xA9 },
  { "LDAC", 0xA5 },   { "LDAD", 0xB5 },   { "LDAF", 0xAD },   { "LDAG", 0xBD },
  { "LDAH", 0xB9 },   { "LDAJ", 0xA1 },   { "LDAK", 0xB1 },   { "LDXB", 0xA2 },
  { "LDXC", 0xA6 },   { "LDXE", 0xB6 },   { "LDXF", 0xAE },   { "LDXH", 0xBE },
  { "LDYB", 0xA0 },   { "LDYC", 0xA4 },   { "LDYD", 0xB4 },   { "LDYF", 0xAC },
  { "LDYG", 0xBC },   { "LSRA", 0x4A },   { "LSRC", 0x46 },   { "LSRD", 0x56 },
  { "LSRF", 0x4E },   { "LSRG", 0x5E },   { "LXAB", 0xAB },   { "NOPA", 0xEA },
  { "NOPB", 0x89 },   { "NOPC", 0x04 },   { "NOPD", 0x14 },   { "NOPF", 0x0C },
  { "NOPG", 0x1C },   { "ORAB", 0x09 },   { "ORAC", 0x05 },   { "ORAD", 0x15 },
  { "ORAF", 0x0D },   { "ORAG", 0x1D },   { "ORAH", 0x19 },   { "ORAJ", 0x01 },
  { "ORAK", 0x11 },   { "PHAA", 0x48 },   { "PHPA", 0x08 },   { "PLAA", 0x68 },
  { "PLPA", 0x28 },   { "RLAC", 0x27 },   { "RLAD", 0x37 },   { "RLAF", 0x2F },
  { "RLAG", 0x3F },   { "RLAH", 0x3B },   { "RLAJ", 0x23 },   { "RLAK", 0x33 },
  { "ROLA", 0x2A },   { "ROLC", 0x26 },   { "ROLD", 0x36 },   { "ROLF", 0x2E },
  { "ROLG", 0x3E },   { "RORA", 0x6A },   { "RORC", 0x66 },   { "RORD", 0x76 },
  { "RORF", 0x6E },   { "RORG", 0x7E },   { "RRAC", 0x67 },   { "RRAD", 0x77 },
  { "RRAF", 0x6F },   { "RRAG", 0x7F },   { "RRAH", 0x7B },   { "RRAJ", 0x63 },
  { "RRAK", 0x73 },   { "RTIA", 0x40 },   { "RTSA", 0x60 },   { "SAXC", 0x87 },
  { "SAXE", 0x97 },   { "SAXF", 0x8F },   { "SAXJ", 0x83 },   { "SBCB", 0xE9 },
  { "SBCC", 0xE5 },   { "SBCD", 0xF5 },   { "SBCF", 0xED },   { "SBCG", 0xFD },
  { "SBCH", 0xF9 },   { "SBCJ", 0xE1 },   { "SBCK", 0xF1 },   { "SBXB", 0xCB },
  { "SECA", 0x38 },   { "SEDA", 0xF8 },   { "SEIA", 0x78 },   { "SHAH", 0x9F },
  { "SHAK", 0x93 },   { "SHSH", 0x9B },   { "SHXH", 0x9E },   { "SHYG", 0x9C },
  { "SLOC", 0x07 },   { "SLOD", 0x17 },   { "SLOF", 0x0F },   { "SLOG", 0x1F },
  { "SLOH", 0x1B },   { "SLOJ", 0x03 },   { "SLOK", 0x13 },   { "SREC", 0x47 },
  { "SRED", 0x57 },   { "SREF", 0x4F },   { "SREG", 0x5F },   { "SREH", 0x5B },
  { "SREJ", 0x43 },   { "SREK", 0x53 },   { "STAC", 0x85 },   { "STAD", 0x95 },
  { "STAF", 0x8D },   { "STAG", 0x9D },   { "STAH", 0x99 },   { "STAJ", 0x81 },
  { "STAK", 0x91 },   { "STXC", 0x86 },   { "STXE", 0x96 },   { "STXF", 0x8E },
  { "STYC", 0x84 },   { "STYD", 0x94 },   { "STYF", 0x8C },   { "TAXA", 0xAA },
  { "TAYA", 0xA8 },   { "TSXA", 0xBA },   { "TXAA", 0x8A },   { "TXSA", 0x9A },
  { "TYAA", 0x98 }
};

static int searchAssemblerOpcodeTable(const char *s)
{
  size_t  l = 0;
  size_t  h = sizeof(assemblerOpcodeTable) / sizeof(AssemblerOpcodeTableEntry);
  while (h > l) {
    size_t  n = (l + h) >> 1;
    int     d = 0;
    for (int i = 0; i < 5; i++) {
      if (s[i] == '*' && i < 4)
        continue;
      if (s[i] < assemblerOpcodeTable[n].name[i]) {
        d = -1;
        break;
      }
      if (s[i] > assemblerOpcodeTable[n].name[i]) {
        d = 1;
        break;
      }
    }
    if (d == 0)
      return int(n);
    if (d < 0) {
      if (h == n)
        break;
      h = n;
    }
    else {
      if (l == n)
        break;
      l = n;
    }
  }
  return -1;
}

static int assembleInstruction(uint8_t *buf, uint32_t& addr,
                               const std::vector<std::string>& args)
{
  if (args.size() < 3)
    return 0;
  addr = 0U;
  if (!parseHexNumber(addr, args[1].c_str()))
    return 0;
  size_t    nameOffs = 2;
  char      tmpBuf[5];
  tmpBuf[3] = '*';
  tmpBuf[4] = '\0';
  while (nameOffs < args.size()) {
    if (args[nameOffs].length() == 3) {
      tmpBuf[0] = args[nameOffs][0];
      tmpBuf[1] = args[nameOffs][1];
      tmpBuf[2] = args[nameOffs][2];
      if (searchAssemblerOpcodeTable(&(tmpBuf[0])) >= 0)
        break;
    }
    if (args[nameOffs] != "*") {
      uint32_t  tmp = 0U;
      if (!parseHexNumber(tmp, args[nameOffs].c_str()))
        return 0;
    }
    nameOffs++;
  }
  if (nameOffs >= args.size())
    return 0;
  char      addressingMode = 'A';
  uint32_t  operand = 0U;
  size_t    offs = nameOffs + 1;
  if (offs < args.size()) {
    if (args[offs] == "(") {                    // indirect
      offs++;
      if (offs >= args.size())
        return 0;
      if (args[offs] == "$")
        offs++;
      if ((offs + 2) > args.size())
        return 0;
      if (!parseHexNumber(operand, args[offs].c_str()))
        return 0;
      offs++;
      if (args[offs] == ")") {
        if ((offs + 1) == args.size()) {
          addressingMode = 'I';
        }
        else if ((offs + 3) == args.size()) {   // (indirect), Y
          if (args[offs + 1] != ",")
            return 0;
          if (args[offs + 2] != "Y")
            return 0;
          addressingMode = 'K';
        }
        else
          return 0;
      }
      else if (args[offs] == ",") {             // (indirect, X)
        if ((offs + 3) != args.size())
          return 0;
        if (args[offs + 1] != "X")
          return 0;
        if (args[offs + 2] != ")")
          return 0;
        addressingMode = 'J';
      }
    }
    else if (args[offs] == "#") {               // immediate
      offs++;
      if (offs >= args.size())
        return 0;
      if (args[offs] == "$")
        offs++;
      if ((offs + 1) != args.size())
        return 0;
      if (!parseHexNumber(operand, args[offs].c_str()))
        return 0;
      addressingMode = 'B';
    }
    else {                              // zeropage, absolute, or relative
      if (args[offs] == "$")
        offs++;
      if (offs >= args.size())
        return 0;
      if (!parseHexNumber(operand, args[offs].c_str()))
        return 0;
      if (args[offs].length() > 2)
        addressingMode = 'F';
      else
        addressingMode = 'C';
      offs++;
      if (offs != args.size()) {                // indexed
        if ((offs + 2) != args.size())
          return 0;
        if (args[offs] != ",")
          return 0;
        addressingMode++;
        if (args[offs + 1] == "Y")
          addressingMode++;
        else if (args[offs + 1] != "X")
          return 0;
      }
    }
  }
  tmpBuf[3] = addressingMode;
  int     opcode = searchAssemblerOpcodeTable(&(tmpBuf[0]));
  if (opcode < 0) {
    if (addressingMode >= 'C' && addressingMode <= 'E') {
      // if no zeropage version is available, try absolute
      addressingMode += ('F' - 'C');
      tmpBuf[3] = addressingMode;
      opcode = searchAssemblerOpcodeTable(&(tmpBuf[0]));
    }
    if (opcode < 0)
      return 0;
  }
  opcode = assemblerOpcodeTable[opcode].opcode;
  if ((opcode & 0x1F) == 0x10) {                // branch instructions
    operand = (operand - (addr + 2U)) & 0xFFFFU;
    if ((operand & 0xFF80U) != 0x0000U && (operand & 0xFF80U) != 0xFF80U)
      return 0;
    operand = operand & 0xFFU;
    buf[0] = uint8_t(opcode);
    buf[1] = uint8_t(operand);
    return 2;
  }
  switch (addressingMode) {
  case 'A':
    buf[0] = uint8_t(opcode);
    return 1;
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'J':
  case 'K':
    if (operand >= 0x0100U)
      return 0;
    buf[0] = uint8_t(opcode);
    buf[1] = uint8_t(operand);
    return 2;
  case 'F':
  case 'G':
  case 'H':
  case 'I':
    if (operand >= 0x00010000U)
      return 0;
    buf[0] = uint8_t(opcode);
    buf[1] = uint8_t(operand & 0xFFU);
    buf[2] = uint8_t((operand >> 8) & 0xFFU);
    return 3;
  }
  return 0;
}

// ----------------------------------------------------------------------------

Plus4EmuGUIMonitor::Plus4EmuGUIMonitor(int xx, int yy, int ww, int hh,
                                       const char *ll)
  : Fl_Text_Editor(xx, yy, ww, hh, ll),
    buf_((Fl_Text_Buffer *) 0),
    debugWindow((Plus4EmuGUI_DebugWindow *) 0),
    gui((Plus4EmuGUI *) 0),
    assembleOffset(int32_t(0)),
    disassembleAddress(0U),
    disassembleOffset(int32_t(0)),
    memoryDumpAddress(0U),
    addressMask(0xFFFFU),
    cpuAddressMode(true)
{
  buf_ = new Fl_Text_Buffer();
  buffer(buf_);
  add_key_binding(FL_Enter, FL_TEXT_EDITOR_ANY_STATE, &enterKeyCallback);
  insert_mode(0);
}

Plus4EmuGUIMonitor::~Plus4EmuGUIMonitor()
{
  buffer((Fl_Text_Buffer *) 0);
  delete buf_;
}

void Plus4EmuGUIMonitor::command_assemble(const std::vector<std::string>& args)
{
  if (args.size() == 2) {
    (void) parseHexNumberEx(args[1].c_str());
    return;
  }
  uint8_t   opcodeBuf[4];
  uint32_t  addr = 0U;
  int       nBytes = assembleInstruction(&(opcodeBuf[0]), addr, args);
  if (nBytes < 1)
    throw Plus4Emu::Exception("assembler syntax error");
  uint32_t  writeAddr =
      (uint32_t(int32_t(addr) + assembleOffset)) & addressMask;
  for (int i = 0; i < nBytes; i++) {
    gui->vm.writeMemory((writeAddr + uint32_t(i)) & addressMask, opcodeBuf[i],
                        cpuAddressMode);
  }
  this->move_up();
  disassembleOffset = -assembleOffset;
  disassembleAddress = writeAddr;
  disassembleInstruction(true);
  addr = (addr + uint32_t(nBytes)) & addressMask;
  char    tmpBuf[16];
  if (cpuAddressMode)
    std::sprintf(&(tmpBuf[0]), "A   %04X  ", (unsigned int) addr);
  else
    std::sprintf(&(tmpBuf[0]), "A %06X  ", (unsigned int) addr);
  this->overstrike(&(tmpBuf[0]));
}

void Plus4EmuGUIMonitor::command_disassemble(const std::vector<std::string>&
                                                 args)
{
  if (args.size() > 4)
    throw Plus4Emu::Exception("invalid number of disassemble arguments");
  uint32_t  startAddr = disassembleAddress & addressMask;
  if (args.size() > 1)
    startAddr = parseHexNumberEx(args[1].c_str(), addressMask);
  disassembleAddress = startAddr;
  uint32_t  endAddr = (startAddr + 20U) & addressMask;
  if (args.size() > 2)
    endAddr = parseHexNumberEx(args[2].c_str(), addressMask);
  if (args.size() > 3) {
    uint32_t  tmp = parseHexNumberEx(args[3].c_str(), addressMask);
    disassembleOffset = int32_t(tmp) - int32_t(startAddr);
    if (disassembleOffset > int32_t(addressMask >> 1))
      disassembleOffset -= int32_t(addressMask + 1U);
    else if (disassembleOffset < -(int32_t((addressMask >> 1) + 1U)))
      disassembleOffset += int32_t(addressMask + 1U);
  }
  std::string tmpBuf;
  while (((endAddr - disassembleAddress) & addressMask)
         > (MONITOR_MAX_LINES * 4U)) {
    uint32_t  nextAddr = gui->vm.disassembleInstruction(tmpBuf,
                                                        disassembleAddress,
                                                        cpuAddressMode,
                                                        disassembleOffset);
    disassembleAddress = nextAddr & addressMask;
  }
  while (true) {
    uint32_t  prvAddr = disassembleAddress;
    disassembleInstruction();
    while (prvAddr != disassembleAddress) {
      if (prvAddr == endAddr)
        return;
      prvAddr = (prvAddr + 1U) & addressMask;
    }
  }
}

void Plus4EmuGUIMonitor::command_memoryDump(const std::vector<std::string>&
                                                args)
{
  if (args.size() > 3)
    throw Plus4Emu::Exception("invalid number of memory dump arguments");
  uint32_t  startAddr = memoryDumpAddress & addressMask;
  if (args.size() > 1)
    startAddr = parseHexNumberEx(args[1].c_str(), addressMask);
  memoryDumpAddress = startAddr;
  uint32_t  endAddr = (startAddr + 95U) & addressMask;
  if (args.size() > 2)
    endAddr = parseHexNumberEx(args[2].c_str(), addressMask);
  while (((endAddr - memoryDumpAddress) & addressMask)
         > (MONITOR_MAX_LINES * 8U)) {
    memoryDumpAddress = (memoryDumpAddress + 8U) & addressMask;
  }
  while (true) {
    uint32_t  prvAddr = memoryDumpAddress;
    memoryDump();
    while (prvAddr != memoryDumpAddress) {
      if (prvAddr == endAddr)
        return;
      prvAddr = (prvAddr + 1U) & addressMask;
    }
  }
}

void Plus4EmuGUIMonitor::command_memoryModify(const std::vector<std::string>&
                                                  args)
{
  if (args.size() < 2)
    throw Plus4Emu::Exception("insufficient arguments for memory modify");
  uint32_t  addr = parseHexNumberEx(args[1].c_str(), addressMask);
  memoryDumpAddress = addr;
  for (size_t i = 2; i < args.size(); i++) {
    if (args[i] == ":")
      break;
    uint32_t  value = parseHexNumberEx(args[i].c_str());
    if (value >= 0x0100U)
      throw Plus4Emu::Exception("byte value is out of range");
    gui->vm.writeMemory(addr, uint8_t(value), cpuAddressMode);
    addr = (addr + 1U) & addressMask;
  }
  this->move_up();
  memoryDump();
}

void Plus4EmuGUIMonitor::command_toggleCPUAddressMode(
    const std::vector<std::string>& args)
{
  if (args.size() > 2)
    throw Plus4Emu::Exception("too many arguments for address mode");
  if (args.size() > 1) {
    uint32_t  n = parseHexNumberEx(args[1].c_str());
    cpuAddressMode = (n != 0U);
  }
  else
    cpuAddressMode = !cpuAddressMode;
  addressMask = (cpuAddressMode ? 0x0000FFFFU : 0x003FFFFFU);
  if (cpuAddressMode)
    printMessage("CPU address mode");
  else
    printMessage("Physical address mode");
}

int Plus4EmuGUIMonitor::enterKeyCallback(int c, Fl_Text_Editor *e_)
{
  (void) c;
  Plus4EmuGUIMonitor& e = *(reinterpret_cast<Plus4EmuGUIMonitor *>(e_));
  const char  *s = e.buf_->line_text(e.insert_position());
  e.moveDown();
  if (s) {
    if (s[0] != '\0' && s[0] != '\n') {
      try {
        e.parseCommand(s);
      }
      catch (...) {
        e.move_up();
        e.insert_position(e.buf_->line_end(e.insert_position()) - 1);
        e.overstrike("?");
        e.moveDown();
      }
    }
    std::free(const_cast<char *>(s));
  }
  return 1;
}

void Plus4EmuGUIMonitor::moveDown()
{
  try {
    insert_position(buf_->line_end(insert_position()));
    if (insert_position() >= buf_->length()) {
      int     n = buf_->count_lines(0, buf_->length());
      while (n >= MONITOR_MAX_LINES) {
        buf_->remove(0, buf_->line_end(0) + 1);
        n--;
      }
      insert_position(buf_->length());
      this->insert("\n");
    }
    else {
      move_down();
      insert_position(buf_->line_start(insert_position()));
    }
    show_insert_position();
  }
  catch (...) {
  }
}

void Plus4EmuGUIMonitor::parseCommand(const char *s)
{
  std::vector<std::string>  args;
  tokenizeString(args, s);
  if (args.size() == 0)
    return;
  if (args[0] == ">")
    command_memoryModify(args);
  else if (args[0] == "A" || args[0] == ".")
    command_assemble(args);
  else if (args[0] == "D")
    command_disassemble(args);
  else if (args[0] == "M")
    command_memoryDump(args);
  else if (args[0] == "AM")
    command_toggleCPUAddressMode(args);
  else
    throw Plus4Emu::Exception("invalid monitor command");
}

void Plus4EmuGUIMonitor::printMessage(const char *s)
{
  if (!s)
    return;
  try {
    std::string tmpBuf;
    while (true) {
      tmpBuf = "";
      while (*s != '\0' && *s != '\n') {
        tmpBuf += (*s);
        s++;
      }
      insert_position(buf_->line_start(insert_position()));
      overstrike(tmpBuf.c_str());
      if (insert_position() < buf_->line_end(insert_position()))
        buf_->remove(insert_position(), buf_->line_end(insert_position()));
      moveDown();
      if (*s == '\0')
        break;
      s++;
    }
  }
  catch (...) {
  }
}

void Plus4EmuGUIMonitor::disassembleInstruction(bool assembleMode)
{
  disassembleAddress = disassembleAddress & addressMask;
  std::string tmpBuf;
  uint32_t  nextAddr = gui->vm.disassembleInstruction(tmpBuf,
                                                      disassembleAddress,
                                                      cpuAddressMode,
                                                      disassembleOffset);
  disassembleAddress = nextAddr & addressMask;
  if (!assembleMode)
    tmpBuf = std::string(". ") + tmpBuf;
  else
    tmpBuf = std::string("A ") + tmpBuf;
  printMessage(tmpBuf.c_str());
}

void Plus4EmuGUIMonitor::memoryDump()
{
  char      tmpBuf[64];
  uint8_t   dataBuf[8];
  uint32_t  startAddr = memoryDumpAddress & addressMask;
  memoryDumpAddress = startAddr;
  for (int i = 0; i < 8; i++) {
    dataBuf[i] = gui->vm.readMemory(memoryDumpAddress, cpuAddressMode) & 0xFF;
    memoryDumpAddress = (memoryDumpAddress + 1U) & addressMask;
  }
  char    *bufp = &(tmpBuf[0]);
  int     n = 0;
  if (cpuAddressMode)
    n = std::sprintf(bufp, ">%04X", (unsigned int) startAddr);
  else
    n = std::sprintf(bufp, ">%06X", (unsigned int) startAddr);
  bufp = bufp + n;
  n = std::sprintf(bufp, "  %02X %02X %02X %02X %02X %02X %02X %02X",
                   (unsigned int) dataBuf[0], (unsigned int) dataBuf[1],
                   (unsigned int) dataBuf[2], (unsigned int) dataBuf[3],
                   (unsigned int) dataBuf[4], (unsigned int) dataBuf[5],
                   (unsigned int) dataBuf[6], (unsigned int) dataBuf[7]);
  bufp = bufp + n;
  for (int i = 0; i < 8; i++) {
    dataBuf[i] &= uint8_t(0x7F);
    if (dataBuf[i] < uint8_t(' ') || dataBuf[i] == uint8_t(0x7F))
      dataBuf[i] = uint8_t('.');
  }
  std::sprintf(bufp, "  :%c%c%c%c%c%c%c%c",
               int(dataBuf[0]), int(dataBuf[1]), int(dataBuf[2]),
               int(dataBuf[3]), int(dataBuf[4]), int(dataBuf[5]),
               int(dataBuf[6]), int(dataBuf[7]));
  printMessage(&(tmpBuf[0]));
}

