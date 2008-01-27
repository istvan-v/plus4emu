
// p4fliconv: high resolution interlaced FLI converter utility
// Copyright (C) 2007-2008 Istvan Varga <istvanv@users.sourceforge.net>
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
//
// The Plus/4 program files generated by this utility are not covered by the
// GNU General Public License, and can be used, modified, and distributed
// without any restrictions.

#ifndef P4FLICONV_FLIDISP_HPP
#define P4FLICONV_FLIDISP_HPP

#include "p4fliconv.hpp"
#include "gldisp.hpp"

class Plus4FLIConvGUI_Display : public Plus4Emu::OpenGLDisplay {
 private:
  Plus4FLIConvGUI&  gui;
 public:
  Plus4FLIConvGUI_Display(Plus4FLIConvGUI& gui_,
                          int xx = 0, int yy = 0, int ww = 768, int hh = 576,
                          const char *lbl = (char *) 0,
                          bool isDoubleBuffered = false);
  virtual ~Plus4FLIConvGUI_Display();
  // Read and process messages sent by the child thread. Returns true if
  // redraw() needs to be called to update the display.
  virtual bool checkEvents();
 protected:
  virtual void draw();
};

#endif  // P4FLICONV_FLIDISP_HPP

