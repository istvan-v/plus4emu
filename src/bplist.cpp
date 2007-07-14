
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
#include "bplist.hpp"

#include <map>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Plus4Emu {

  BreakPointList::BreakPointList(const std::string& lst)
  {
    std::vector<std::string>  tokens;
    std::string curToken = "";
    char        ch = '\0';
    bool        wasSpace = true;

    for (size_t i = 0; i < lst.length(); i++) {
      ch = lst[i];
      bool  isSpace = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
      if (isSpace && wasSpace)
        continue;
      if (isSpace) {
        tokens.push_back(curToken);
        curToken = "";
      }
      else
        curToken += ch;
      wasSpace = isSpace;
    }
    if (!wasSpace)
      tokens.push_back(curToken);

    std::map<uint32_t, BreakPoint>  bpList;

    for (size_t i = 0; i < tokens.size(); i++) {
      size_t    j;
      uint16_t  addr = 0, lastAddr;
      int       type = 0;
      int       rwMode = 0;
      int       priority = -1;
      uint32_t  n;

      curToken = tokens[i];
      n = 0;
      for (j = 0; j < curToken.length(); j++) {
        char    c = curToken[j];
        if (c >= '0' && c <= '9')
          n = (n << 4) + uint32_t(c - '0');
        else if (c >= 'A' && c <= 'F')
          n = (n << 4) + uint32_t(c - 'A') + 10;
        else if (c >= 'a' && c <= 'f')
          n = (n << 4) + uint32_t(c - 'a') + 10;
        else
          break;
      }
      if (j != 4)
        throw Exception("syntax error in breakpoint list");
      if (j < curToken.length() && curToken[j] == ':') {
        if (n >= 0x0200)
          throw Exception("syntax error in breakpoint list");
        j++;
        type = 4;       // break on video position
        addr = uint16_t(n << 7);
        n = 0;
        for ( ; j < curToken.length(); j++) {
          char    c = curToken[j];
          if (c >= '0' && c <= '9')
            n = (n << 4) + uint32_t(c - '0');
          else if (c >= 'A' && c <= 'F')
            n = (n << 4) + uint32_t(c - 'A') + 10;
          else if (c >= 'a' && c <= 'f')
            n = (n << 4) + uint32_t(c - 'a') + 10;
          else
            break;
        }
        if (j != 7)
          throw Exception("syntax error in breakpoint list");
        addr |= uint16_t((n & 0xFEU) >> 1);
      }
      else
        addr = uint16_t(n);
      lastAddr = addr;
      if (j < curToken.length() && curToken[j] == '-') {
        size_t  len = 0;
        n = 0;
        j++;
        for ( ; j < curToken.length(); j++, len++) {
          char    c = curToken[j];
          if (c >= '0' && c <= '9')
            n = (n << 4) + uint32_t(c - '0');
          else if (c >= 'A' && c <= 'F')
            n = (n << 4) + uint32_t(c - 'A') + 10;
          else if (c >= 'a' && c <= 'f')
            n = (n << 4) + uint32_t(c - 'a') + 10;
          else
            break;
        }
        if (type == 4) {
          if (len != 2 || n < ((uint32_t(addr) & 0x7FU) << 1))
            throw Exception("syntax error in breakpoint list");
          lastAddr = uint16_t((addr & 0xFF80) | ((n & 0xFE) >> 1));
        }
        else {
          if (len != 4 || n < addr)
            throw Exception("syntax error in breakpoint list");
          lastAddr = n;
        }
      }
      for ( ; j < curToken.length(); j++) {
        switch (curToken[j]) {
        case 'r':
          rwMode |= 1;
          break;
        case 'w':
          rwMode |= 2;
          break;
        case 'i':
          if (type == 4)
            throw Exception("ignore flag is not allowed for video breakpoints");
          type = 5;
          break;
        case 'p':
          if (++j >= curToken.length())
            throw Exception("syntax error in breakpoint list");
          if (curToken[j] < '0' || curToken[j] > '3')
            throw Exception("syntax error in breakpoint list");
          priority = int(curToken[j] - '0');
          break;
        default:
          throw Exception("syntax error in breakpoint list");
        }
      }
      if (type == 4 && rwMode != 0) {
        throw Exception("read/write flags are not allowed "
                        "for video breakpoints");
      }
      if (type == 5) {
        if (rwMode != 0 || priority >= 0) {
          throw Exception("read/write flags and priority are not allowed "
                          "for ignore breakpoints");
        }
        priority = 3;
      }
      type |= rwMode;
      if (type == 0)
        type = 3;       // default to read/write mode
      if (priority < 0)
        priority = 2;   // default to priority = 2
      while (true) {
        uint32_t    addr_ = uint32_t(addr);
        if (type == 4)  // use separate address space for video breakpoints
          addr_ |= uint32_t(0x80000000UL);
        std::map<uint32_t, BreakPoint>::iterator  i_ = bpList.find(addr_);
        if (i_ == bpList.end()) {
          // add new breakpoint
          BreakPoint  bp(type, addr, priority);
          bpList.insert(std::pair<uint32_t, BreakPoint>(addr_, bp));
        }
        else {
          // update existing breakpoint
          BreakPoint  *bpp = &((*i_).second);
          BreakPoint  bp((type == 5 || bpp->type() == 5 ?
                          5 : (type | bpp->type())),
                         addr,
                         (priority > bpp->priority() ?
                          priority : bpp->priority()));
          (*bpp) = bp;
        }
        if (addr == lastAddr)
          break;
        addr++;
      }
    }

    std::map<uint32_t, BreakPoint>::iterator  i_;
    for (i_ = bpList.begin(); i_ != bpList.end(); i_++)
      lst_.push_back((*i_).second);
  }

  void BreakPointList::addBreakPoint(int type, uint16_t addr, int priority)
  {
    lst_.push_back(BreakPoint(type, addr, priority));
  }

  std::string BreakPointList::getBreakPointList()
  {
    std::ostringstream  lst;
    BreakPoint          prv_bp(0, 0, 0);
    size_t              i;
    uint16_t            firstAddr = 0;

    lst << std::hex << std::uppercase << std::right << std::setfill('0');
    std::stable_sort(lst_.begin(), lst_.end());
    for (i = 0; i < lst_.size(); i++) {
      BreakPoint  bp(lst_[i]);
      if (!i) {
        prv_bp = bp;
        firstAddr = bp.addr();
        continue;
      }
      if (bp.type() == prv_bp.type() &&
          (bp.addr() == prv_bp.addr() ||
           (bp.addr() == (prv_bp.addr() + 1) &&
            (bp.type() != 4 || ((bp.addr() ^ prv_bp.addr()) & 0xFF80) == 0))) &&
          bp.priority() == prv_bp.priority()) {
        prv_bp = bp;
        continue;
      }
      uint16_t  lastAddr = lst_[i - 1].addr();
      if (prv_bp.type() == 4) {
        lst << std::setw(4) << uint32_t((firstAddr & 0xFF80) >> 7) << ":";
        lst << std::setw(2) << uint32_t((firstAddr & 0x007F) << 1);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(2) << uint32_t((lastAddr & 0x007F) << 1);
      }
      else {
        lst << std::setw(4) << uint32_t(firstAddr);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(4) << uint32_t(lastAddr);
      }
      if (prv_bp.type() != 5) {
        if (prv_bp.type() == 1)
          lst << "r";
        else if (prv_bp.type() == 2)
          lst << "w";
        if (prv_bp.priority() != 2)
          lst << "p" << std::setw(1) << prv_bp.priority();
      }
      else
        lst << "i";
      lst << "\n";
      prv_bp = bp;
      firstAddr = bp.addr();
    }
    if (i) {
      uint16_t  lastAddr = lst_[i - 1].addr();
      if (prv_bp.type() == 4) {
        lst << std::setw(4) << uint32_t((firstAddr & 0xFF80) >> 7) << ":";
        lst << std::setw(2) << uint32_t((firstAddr & 0x007F) << 1);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(2) << uint32_t((lastAddr & 0x007F) << 1);
      }
      else {
        lst << std::setw(4) << uint32_t(firstAddr);
        if (lastAddr != firstAddr)
          lst << "-" << std::setw(4) << uint32_t(lastAddr);
      }
      if (prv_bp.type() != 5) {
        if (prv_bp.type() == 1)
          lst << "r";
        else if (prv_bp.type() == 2)
          lst << "w";
        if (prv_bp.priority() != 2)
          lst << "p" << std::setw(1) << prv_bp.priority();
      }
      else
        lst << "i";
      lst << "\n";
    }
    lst << "\n";
    return lst.str();
  }

  // --------------------------------------------------------------------------

  class ChunkType_BPList : public File::ChunkTypeHandler {
   private:
    BreakPointList& ref;
   public:
    ChunkType_BPList(BreakPointList& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_BPList()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::PLUS4EMU_CHUNKTYPE_BREAKPOINTS;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void BreakPointList::saveState(File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000002);        // version number
    for (size_t i = 0; i < lst_.size(); i++) {
      buf.writeByte(uint8_t(lst_[i].type()));
      buf.writeUInt32(lst_[i].addr());
      buf.writeByte(uint8_t(lst_[i].priority()));
    }
  }

  void BreakPointList::saveState(File& f)
  {
    File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(File::PLUS4EMU_CHUNKTYPE_BREAKPOINTS, buf);
  }

  void BreakPointList::loadState(File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000002) {
      buf.setPosition(buf.getDataSize());
      throw Exception("incompatible breakpoint list format");
    }
    // reset breakpoint list
    lst_.clear();
    // load saved state
    while (buf.getPosition() < buf.getDataSize()) {
      int       type = buf.readByte();
      uint16_t  addr = uint16_t(buf.readUInt32() & 0xFFFFU);
      int       priority = buf.readByte();
      BreakPoint  bp(type, addr, priority);
      lst_.push_back(bp);
    }
  }

  void BreakPointList::registerChunkType(File& f)
  {
    ChunkType_BPList  *p;
    p = new ChunkType_BPList(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Plus4Emu

