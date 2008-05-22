
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2008 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN   1
#  include <windows.h>
#else
#  include <unistd.h>
#  include <pthread.h>
#endif

void Plus4EmuGUI::init_()
{
  flDisplay = (Plus4Emu::FLTKDisplay_ *) 0;
  emulatorWindow = dynamic_cast<Fl_Window *>(&display);
  flDisplay = dynamic_cast<Plus4Emu::FLTKDisplay_ *>(&display);
#ifdef WIN32
  mainThreadID = uintptr_t(GetCurrentThreadId());
#else
  mainThreadID = uintptr_t(pthread_self());
#endif
  displayMode = 0;
  exitFlag = false;
  errorFlag = false;
  lightPenEnabled = false;
  oldWindowWidth = -1;
  oldWindowHeight = -1;
  oldDisplayMode = 0;
  oldDemoStatus = -1;
  oldSpeedPercentage = 100;
  oldPauseFlag = true;
  oldTapeSampleRate = -1L;
  oldTapeSampleSize = -1;
  oldFloppyDriveLEDState = 0U;
  oldFloppyDriveHeadPositions = (~(uint64_t(0)));
  oldTapeReadOnlyFlag = false;
  oldTapePosition = -2L;
  functionKeyState = 0U;
  tapeButtonState = 0;
  oldTapeButtonState = -1;
  demoRecordFileName = "";
  demoRecordFile = (Plus4Emu::File *) 0;
  quickSnapshotFileName = "";
  updateDisplayEntered = false;
  debugWindowShowFlag = false;
  debugWindowOpenFlag = false;
  browseFileWindowShowFlag = false;
  browseFileStatus = 1;
  browseFileWindow = (Fl_Native_File_Chooser *) 0;
  windowToShow = (Fl_Window *) 0;
  diskConfigWindow = (Plus4EmuGUI_DiskConfigWindow *) 0;
  displaySettingsWindow = (Plus4EmuGUI_DisplayConfigWindow *) 0;
  keyboardConfigWindow = (Plus4EmuGUI_KbdConfigWindow *) 0;
  soundSettingsWindow = (Plus4EmuGUI_SoundConfigWindow *) 0;
  machineConfigWindow = (Plus4EmuGUI_MachineConfigWindow *) 0;
  debugWindow = (Plus4EmuGUI_DebugWindow *) 0;
  printerWindow = (Plus4EmuGUI_PrinterWindow *) 0;
  aboutWindow = (Plus4EmuGUI_AboutWindow *) 0;
  savedSpeedPercentage = 0U;
  std::string defaultDir_(".");
  snapshotDirectory = defaultDir_;
  demoDirectory = defaultDir_;
  soundFileDirectory = defaultDir_;
  configDirectory = defaultDir_;
  loadFileDirectory = defaultDir_;
  tapeImageDirectory = defaultDir_;
  diskImageDirectory = defaultDir_;
  romImageDirectory = defaultDir_;
  prgFileDirectory = defaultDir_;
  prgFileName = "";
  debuggerDirectory = defaultDir_;
  screenshotDirectory = defaultDir_;
  screenshotFileName = "";
  guiConfig.createKey("gui.snapshotDirectory", snapshotDirectory);
  guiConfig.createKey("gui.demoDirectory", demoDirectory);
  guiConfig.createKey("gui.soundFileDirectory", soundFileDirectory);
  guiConfig.createKey("gui.configDirectory", configDirectory);
  guiConfig.createKey("gui.loadFileDirectory", loadFileDirectory);
  guiConfig.createKey("gui.tapeImageDirectory", tapeImageDirectory);
  guiConfig.createKey("gui.diskImageDirectory", diskImageDirectory);
  guiConfig.createKey("gui.romImageDirectory", romImageDirectory);
  guiConfig.createKey("gui.prgFileDirectory", prgFileDirectory);
  guiConfig.createKey("gui.debuggerDirectory", debuggerDirectory);
  guiConfig.createKey("gui.screenshotDirectory", screenshotDirectory);
  browseFileWindow = new Fl_Native_File_Chooser();
  Fl::add_check(&fltkCheckCallback, (void *) this);
}

void Plus4EmuGUI::updateDisplay_windowTitle()
{
  if (oldPauseFlag) {
    std::sprintf(&(windowTitleBuf[0]), "plus4emu 1.2.6 (paused)");
  }
  else {
    std::sprintf(&(windowTitleBuf[0]), "plus4emu 1.2.6 (%d%%)",
                 int(oldSpeedPercentage));
  }
  mainWindow->label(&(windowTitleBuf[0]));
}

void Plus4EmuGUI::updateDisplay_windowMode()
{
  if (((displayMode ^ oldDisplayMode) & 2) != 0) {
    if ((displayMode & 2) != 0)
      mainWindow->fullscreen();
    else
      mainWindow->fullscreen_off(32, 32,
                                 config.display.width, config.display.height);
  }
  resizeWindow(config.display.width, config.display.height);
  Fl::redraw();
  Fl::flush();
  int     newWindowWidth = mainWindow->w();
  int     newWindowHeight = mainWindow->h();
  if ((displayMode & 1) == 0) {
    if (newWindowWidth >= 745)
      statusDisplayGroup->resize(newWindowWidth - 360, 0, 360, 30);
    else
      statusDisplayGroup->resize(newWindowWidth - 360, newWindowHeight - 30,
                                 360, 30);
    statusDisplayGroup->show();
    mainMenuBar->resize(0, 2, 300, 26);
    mainMenuBar->show();
    diskStatusDisplayGroup->resize(345, 0, 30, 30);
    diskStatusDisplayGroup->show();
  }
  else {
    statusDisplayGroup->hide();
    mainMenuBar->hide();
    diskStatusDisplayGroup->hide();
  }
  oldWindowWidth = -1;
  oldWindowHeight = -1;
  oldDisplayMode = displayMode;
  if ((displayMode & 1) == 0)
    emulatorWindow->cursor(FL_CURSOR_DEFAULT);
  else
    emulatorWindow->cursor(FL_CURSOR_NONE);
  mainWindow->redraw();
  mainMenuBar->redraw();
  diskStatusDisplayGroup->redraw();
  statusDisplayGroup->redraw();
}

void Plus4EmuGUI::updateDisplay_windowSize()
{
  int     newWindowWidth = mainWindow->w();
  int     newWindowHeight = mainWindow->h();
  if ((displayMode & 1) == 0) {
    int   h = newWindowHeight - (newWindowWidth >= 745 ? 30 : 60);
    emulatorWindow->resize(0, 30, newWindowWidth, h);
    if ((displayMode & 2) == 0) {
      config.display.width = newWindowWidth;
      config.display.height = h;
    }
    if (newWindowWidth >= 745)
      statusDisplayGroup->resize(newWindowWidth - 360, 0, 360, 30);
    else
      statusDisplayGroup->resize(newWindowWidth - 360, newWindowHeight - 30,
                                 360, 30);
    mainMenuBar->resize(0, 2, 300, 26);
    diskStatusDisplayGroup->resize(345, 0, 30, 30);
  }
  else {
    emulatorWindow->resize(0, 0, newWindowWidth, newWindowHeight);
    if ((displayMode & 2) == 0) {
      config.display.width = newWindowWidth;
      config.display.height = newWindowHeight;
    }
  }
  oldWindowWidth = newWindowWidth;
  oldWindowHeight = newWindowHeight;
  mainWindow->redraw();
  mainMenuBar->redraw();
  diskStatusDisplayGroup->redraw();
  statusDisplayGroup->redraw();
}

static void updateFloppyStatsWidgets(uint16_t headPos,
                                     Fl_Valuator *trackDisplay,
                                     Fl_Valuator *sideDisplay,
                                     Fl_Valuator *sectorDisplay,
                                     Fl_Widget *ledDisplay,
                                     Fl_Widget *headDisplay)
{
  if (headPos == 0xFFFF) {
    trackDisplay->value(-1.0);
    sideDisplay->value(-1.0);
    sectorDisplay->value(-1.0);
    if (headDisplay->visible()) {
      headDisplay->hide();
      ledDisplay->redraw();
    }
  }
  else {
    int     trackNum = int(headPos >> 8) & 0x7F;
    trackDisplay->value(double(trackNum));
    sideDisplay->value(double((headPos >> 7) & 0x01));
    sectorDisplay->value(double(headPos & 0x7F));
    int     xPos = (trackNum * 14) / ((headPos & 0x8000) ? 80 : 40);
    headDisplay->position(short(xPos + 352), headDisplay->y());
    headDisplay->color(Fl_Color((trackNum & 1) ? FL_WHITE : FL_BLACK));
    if (!(headDisplay->visible()))
      headDisplay->show();
    ledDisplay->redraw();
    headDisplay->redraw();
  }
}

void Plus4EmuGUI::updateDisplay_floppyStats(
    uint64_t newFloppyDriveHeadPositions)
{
  uint16_t  oldHeadPos = uint16_t(oldFloppyDriveHeadPositions) & 0xFFFF;
  uint16_t  headPos = uint16_t(newFloppyDriveHeadPositions) & 0xFFFF;
  if (headPos != oldHeadPos) {
    updateFloppyStatsWidgets(
        headPos,
        driveATrackDisplay, driveASideDisplay, driveASectorDisplay,
        driveALEDDisplay, driveAHeadDisplay);
  }
  oldHeadPos = uint16_t(oldFloppyDriveHeadPositions >> 16) & 0xFFFF;
  headPos = uint16_t(newFloppyDriveHeadPositions >> 16) & 0xFFFF;
  if (headPos != oldHeadPos) {
    updateFloppyStatsWidgets(
        headPos,
        driveBTrackDisplay, driveBSideDisplay, driveBSectorDisplay,
        driveBLEDDisplay, driveBHeadDisplay);
  }
  oldHeadPos = uint16_t(oldFloppyDriveHeadPositions >> 32) & 0xFFFF;
  headPos = uint16_t(newFloppyDriveHeadPositions >> 32) & 0xFFFF;
  if (headPos != oldHeadPos) {
    updateFloppyStatsWidgets(
        headPos,
        driveCTrackDisplay, driveCSideDisplay, driveCSectorDisplay,
        driveCLEDDisplay, driveCHeadDisplay);
  }
  oldHeadPos = uint16_t(oldFloppyDriveHeadPositions >> 48) & 0xFFFF;
  headPos = uint16_t(newFloppyDriveHeadPositions >> 48) & 0xFFFF;
  if (headPos != oldHeadPos) {
    updateFloppyStatsWidgets(
        headPos,
        driveDTrackDisplay, driveDSideDisplay, driveDSectorDisplay,
        driveDLEDDisplay, driveDHeadDisplay);
  }
  oldFloppyDriveHeadPositions = newFloppyDriveHeadPositions;
}

void Plus4EmuGUI::updateDisplay(double t)
{
#ifdef WIN32
  uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
  uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
  if (currentThreadID != mainThreadID) {
    Plus4Emu::Timer::wait(t * 0.5);
    return;
  }
  if (browseFileWindowShowFlag | debugWindowShowFlag) {
    if (browseFileWindowShowFlag) {
      browseFileStatus = browseFileWindow->show();
      browseFileWindowShowFlag = false;
    }
    if (debugWindowShowFlag) {
      debugWindow->show();
      debugWindowShowFlag = false;
    }
  }
  if (windowToShow) {
    windowToShow->show();
    windowToShow = (Fl_Window *) 0;
  }
  if (updateDisplayEntered) {
    // if re-entering this function:
    Fl::wait(t);
    return;
  }
  updateDisplayEntered = true;
  Plus4Emu::VMThread::VMThreadStatus  vmThreadStatus(vmThread);
  if (vmThreadStatus.threadStatus != 0) {
    exitFlag = true;
    if (vmThreadStatus.threadStatus < 0)
      errorFlag = true;
  }
  if (displayMode != oldDisplayMode) {
    updateDisplay_windowMode();
  }
  else if (mainWindow->w() != oldWindowWidth ||
           mainWindow->h() != oldWindowHeight) {
    updateDisplay_windowSize();
  }
  if (vmThreadStatus.isPaused != oldPauseFlag) {
    oldPauseFlag = vmThreadStatus.isPaused;
    updateDisplay_windowTitle();
  }
  int   newDemoStatus = (vmThreadStatus.isRecordingDemo ?
                         2 : (vmThreadStatus.isPlayingDemo ? 1 : 0));
  if (newDemoStatus != oldDemoStatus) {
    const Fl_Menu_Item *m = mainMenuBar->find_item("File/Stop demo (Ctrl+F12)");
    if (newDemoStatus == 0) {
      if (oldDemoStatus == 2)
        closeDemoFile(false);
      demoStatusDisplay1->hide();
      demoStatusDisplay2->hide();
      const_cast<Fl_Menu_Item *>(m)->deactivate();
    }
    else {
      demoStatusDisplay1->show();
      demoStatusDisplay2->show();
      if (newDemoStatus == 1) {
        demoStatusDisplay2->labelcolor(Fl_Color(6));
        demoStatusDisplay2->label("P");
      }
      else {
        demoStatusDisplay2->labelcolor(Fl_Color(1));
        demoStatusDisplay2->label("R");
      }
      const_cast<Fl_Menu_Item *>(m)->activate();
    }
    oldDemoStatus = newDemoStatus;
    mainWindow->redraw();
  }
  if (vmThreadStatus.tapeSampleRate != oldTapeSampleRate ||
      vmThreadStatus.tapeSampleSize != oldTapeSampleSize ||
      vmThreadStatus.tapeReadOnly != oldTapeReadOnlyFlag) {
    oldTapeSampleRate = vmThreadStatus.tapeSampleRate;
    oldTapeSampleSize = vmThreadStatus.tapeSampleSize;
    oldTapeReadOnlyFlag = vmThreadStatus.tapeReadOnly;
    if (vmThreadStatus.tapeSampleRate < 1L || vmThreadStatus.tapeSampleSize < 1)
      tapeInfoDisplay->label("Tape: none");
    else {
      char  tmpBuf[256];
      std::sprintf(&(tmpBuf[0]), "Tape: %ldHz %dbit %s",
                   vmThreadStatus.tapeSampleRate, vmThreadStatus.tapeSampleSize,
                   (vmThreadStatus.tapeReadOnly ? "RO" : "RW"));
      tapeInfoDisplay->copy_label(&(tmpBuf[0]));
    }
    mainWindow->redraw();
  }
  if (tapeButtonState != oldTapeButtonState) {
    oldTapeButtonState = tapeButtonState;
    switch (tapeButtonState) {
    case 1:
      tapeStatusDisplay->labelcolor(Fl_Color(6));
      tapeStatusDisplay->label("P");
      break;
    case 2:
      tapeStatusDisplay->labelcolor(Fl_Color(1));
      tapeStatusDisplay->label("R");
      break;
    default:
      tapeStatusDisplay->labelcolor(Fl_Color(6));
      tapeStatusDisplay->label("");
      break;
    }
    tapePositionDisplay->redraw();
    tapeStatusDisplay->redraw();
  }
  long  newTapePosition = long(vmThreadStatus.tapePosition * 10.0 + 0.25);
  newTapePosition = (newTapePosition >= 0L ? newTapePosition : -1L);
  if (newTapePosition != oldTapePosition) {
    oldTapePosition = newTapePosition;
    if (newTapePosition >= 0L) {
      char  tmpBuf[256];
      int   h, m, s, ds;
      ds = int(newTapePosition % 10L);
      s = int((newTapePosition / 10L) % 60L);
      m = int((newTapePosition / 600L) % 60L);
      h = int((newTapePosition / 36000L) % 100L);
      std::sprintf(&(tmpBuf[0]), "%2d:%02d:%02d.%d ", h, m, s, ds);
      tapePositionDisplay->copy_label(&(tmpBuf[0]));
    }
    else
      tapePositionDisplay->label("-:--:--.- ");
    tapePositionDisplay->redraw();
    tapeStatusDisplay->redraw();
  }
  if (vmThreadStatus.floppyDriveLEDState != oldFloppyDriveLEDState) {
    uint32_t  tmp = vmThreadStatus.floppyDriveLEDState;
    oldFloppyDriveLEDState = tmp;
    Fl_Color  ledColors_[4] = {
      FL_BLACK, Fl_Color(128), FL_GREEN, Fl_Color(87)
    };
    driveALEDDisplay->color(ledColors_[tmp & 3U]);
    driveALEDDisplay->redraw();
    driveBLEDDisplay->color(ledColors_[(tmp >> 8) & 3U]);
    driveBLEDDisplay->redraw();
    driveCLEDDisplay->color(ledColors_[(tmp >> 16) & 3U]);
    driveCLEDDisplay->redraw();
    driveDLEDDisplay->color(ledColors_[(tmp >> 24) & 3U]);
    driveDLEDDisplay->redraw();
  }
  if (printerWindow->window->shown())
    printerWindow->updateWindow(vmThreadStatus);
  if (statsTimer.getRealTime() >= 0.25) {
    statsTimer.reset();
    int32_t newSpeedPercentage = int32_t(vmThreadStatus.speedPercentage + 0.5f);
    if (newSpeedPercentage != oldSpeedPercentage) {
      oldSpeedPercentage = newSpeedPercentage;
      updateDisplay_windowTitle();
    }
    if (vmThreadStatus.floppyDriveHeadPositions
        != oldFloppyDriveHeadPositions) {
      updateDisplay_floppyStats(vmThreadStatus.floppyDriveHeadPositions);
    }
    else if (driveAHeadDisplay->visible() | driveBHeadDisplay->visible()
             | driveCHeadDisplay->visible() | driveDHeadDisplay->visible()) {
      driveAHeadDisplay->hide();
      driveALEDDisplay->redraw();
      driveBHeadDisplay->hide();
      driveBLEDDisplay->redraw();
      driveCHeadDisplay->hide();
      driveCLEDDisplay->redraw();
      driveDHeadDisplay->hide();
      driveDLEDDisplay->redraw();
    }
  }
  Fl::wait(t);
  updateDisplayEntered = false;
}

void Plus4EmuGUI::errorMessage(const char *msg)
{
  Fl::lock();
  while (errorMessageWindow->shown()) {
    Fl::unlock();
    updateDisplay();
    Fl::lock();
  }
  if (msg)
    errorMessageText->copy_label(msg);
  else
    errorMessageText->label("");
  errorMessageWindow->set_modal();
  windowToShow = errorMessageWindow;
  while (true) {
    Fl::unlock();
    try {
      updateDisplay();
    }
    catch (...) {
    }
    Fl::lock();
    if (windowToShow != errorMessageWindow && !errorMessageWindow->shown()) {
      errorMessageText->label("");
      Fl::unlock();
      break;
    }
  }
}

void Plus4EmuGUI::run()
{
  config.setErrorCallback(&errorMessageCallback, (void *) this);
  // set initial window size from saved configuration
  flDisplay->setFLTKEventCallback(&handleFLTKEvent, (void *) this);
  mainWindow->resizable((Fl_Widget *) 0);
  emulatorWindow->color(47, 47);
  resizeWindow(config.display.width, config.display.height);
  updateDisplay_windowTitle();
  // create menu bar
  mainMenuBar->add("File/Configuration/Load from ASCII file",
                   (char *) 0, &menuCallback_File_LoadConfig, (void *) this);
  mainMenuBar->add("File/Configuration/Load from binary file",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Configuration/Save as ASCII file",
                   (char *) 0, &menuCallback_File_SaveConfig, (void *) this);
  mainMenuBar->add("File/Configuration/Save",
                   (char *) 0, &menuCallback_File_SaveMainCfg, (void *) this);
  mainMenuBar->add("File/Configuration/Revert",
                   (char *) 0, &menuCallback_File_RevertCfg, (void *) this);
  mainMenuBar->add("File/Save snapshot",
                   (char *) 0, &menuCallback_File_SaveSnapshot, (void *) this);
  mainMenuBar->add("File/Load snapshot (F7)",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Set file name",
                   (char *) 0, &menuCallback_File_QSFileName, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Save (Ctrl+F9)",
                   (char *) 0, &menuCallback_File_QSSave, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Load (Ctrl+F10)",
                   (char *) 0, &menuCallback_File_QSLoad, (void *) this);
  mainMenuBar->add("File/Record demo",
                   (char *) 0, &menuCallback_File_RecordDemo, (void *) this);
  mainMenuBar->add("File/Stop demo (Ctrl+F12)",
                   (char *) 0, &menuCallback_File_StopDemo, (void *) this);
  mainMenuBar->add("File/Load demo (F7)",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Record audio/Start...",
                   (char *) 0, &menuCallback_File_RecordSound, (void *) this);
  mainMenuBar->add("File/Record audio/Stop",
                   (char *) 0, &menuCallback_File_StopSndRecord, (void *) this);
  mainMenuBar->add("File/Record video/Start...",
                   (char *) 0, &menuCallback_File_RecordVideo, (void *) this);
  mainMenuBar->add("File/Record video/Stop",
                   (char *) 0, &menuCallback_File_StopAVIRecord, (void *) this);
  mainMenuBar->add("File/Save screenshot (F6)",
                   (char *) 0, &menuCallback_File_Screenshot, (void *) this);
  mainMenuBar->add("File/Load program (F8)",
                   (char *) 0, &menuCallback_File_LoadPRG, (void *) this);
  mainMenuBar->add("File/Save program (Shift+F8)",
                   (char *) 0, &menuCallback_File_SavePRG, (void *) this);
  mainMenuBar->add("File/Quit (Shift+F12)",
                   (char *) 0, &menuCallback_File_Quit, (void *) this);
  mainMenuBar->add("Machine/Speed/No limit (Alt+W)",
                   (char *) 0, &menuCallback_Machine_FullSpeed, (void *) this);
  mainMenuBar->add("Machine/Speed/10%",
                   (char *) 0, &menuCallback_Machine_Speed_10, (void *) this);
  mainMenuBar->add("Machine/Speed/25%",
                   (char *) 0, &menuCallback_Machine_Speed_25, (void *) this);
  mainMenuBar->add("Machine/Speed/50%",
                   (char *) 0, &menuCallback_Machine_Speed_50, (void *) this);
  mainMenuBar->add("Machine/Speed/100%",
                   (char *) 0, &menuCallback_Machine_Speed_100, (void *) this);
  mainMenuBar->add("Machine/Speed/200%",
                   (char *) 0, &menuCallback_Machine_Speed_200, (void *) this);
  mainMenuBar->add("Machine/Speed/400%",
                   (char *) 0, &menuCallback_Machine_Speed_400, (void *) this);
  mainMenuBar->add("Machine/Tape/Select image file (Alt+T)",
                   (char *) 0, &menuCallback_Machine_OpenTape, (void *) this);
  mainMenuBar->add("Machine/Tape/Play (F5)",
                   (char *) 0, &menuCallback_Machine_TapePlay, (void *) this);
  mainMenuBar->add("Machine/Tape/Stop (Shift+F5)",
                   (char *) 0, &menuCallback_Machine_TapeStop, (void *) this);
  mainMenuBar->add("Machine/Tape/Record (Shift+F6)",
                   (char *) 0, &menuCallback_Machine_TapeRecord, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To beginning of tape (Alt+R)",
                   (char *) 0, &menuCallback_Machine_TapeRewind, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To previous marker",
                   (char *) 0, &menuCallback_Machine_TapePrvCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/To next marker (F12)",
                   (char *) 0, &menuCallback_Machine_TapeNxtCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeFwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeFwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Add",
                   (char *) 0, &menuCallback_Machine_AddCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Delete nearest",
                   (char *) 0, &menuCallback_Machine_DeleteCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Delete all",
                   (char *) 0, &menuCallback_Machine_DeleteCPs, (void *) this);
  mainMenuBar->add("Machine/Tape/Close",
                   (char *) 0, &menuCallback_Machine_TapeClose, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset (F11)",
                   (char *) 0, &menuCallback_Machine_Reset, (void *) this);
  mainMenuBar->add("Machine/Reset/Force reset (Ctrl+F11)",
                   (char *) 0, &menuCallback_Machine_ColdReset, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset clock frequencies",
                   (char *) 0, &menuCallback_Machine_ResetFreqs, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset machine configuration (Shift+F11)",
                   (char *) 0, &menuCallback_Machine_ResetAll, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Load config 1 (PageDown)",
                   (char *) 0, &menuCallback_Machine_QuickCfgL1, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Load config 2 (PageUp)",
                   (char *) 0, &menuCallback_Machine_QuickCfgL2, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Save config 1",
                   (char *) 0, &menuCallback_Machine_QuickCfgS1, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Save config 2",
                   (char *) 0, &menuCallback_Machine_QuickCfgS2, (void *) this);
  mainMenuBar->add("Machine/Printer/Enable printer",
                   (char *) 0, &menuCallback_Machine_PrtEnable, (void *) this);
  mainMenuBar->add("Machine/Printer/1525 mode",
                   (char *) 0, &menuCallback_Machine_PrtMode, (void *) this);
  mainMenuBar->add("Machine/Printer/View printer output",
                   (char *) 0, &menuCallback_Machine_PrtShowWin, (void *) this);
  mainMenuBar->add("Machine/Enable light pen",
                   (char *) 0, &menuCallback_Machine_EnableLP, (void *) this);
  mainMenuBar->add("Machine/SID emulation/Enable",
                   (char *) 0, &menuCallback_Machine_EnableSID, (void *) this);
  mainMenuBar->add("Machine/SID emulation/Disable",
                   (char *) 0, &menuCallback_Machine_DisableSID, (void *) this);
  mainMenuBar->add("Machine/Toggle pause (F10)",
                   (char *) 0, &menuCallback_Machine_Pause, (void *) this);
  mainMenuBar->add("Machine/Configure... (Shift+F10)",
                   (char *) 0, &menuCallback_Machine_Configure, (void *) this);
  mainMenuBar->add("Options/Display/Configure... (Shift+F9)",
                   (char *) 0, &menuCallback_Options_DpyConfig, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 384x288",
                   (char *) 0, &menuCallback_Options_DpySize1, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 768x576",
                   (char *) 0, &menuCallback_Options_DpySize2, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 1152x864",
                   (char *) 0, &menuCallback_Options_DpySize3, (void *) this);
  mainMenuBar->add("Options/Display/Cycle display mode (F9)",
                   (char *) 0, &menuCallback_Options_DpyMode, (void *) this);
  mainMenuBar->add("Options/Sound/Increase volume",
                   (char *) 0, &menuCallback_Options_SndIncVol, (void *) this);
  mainMenuBar->add("Options/Sound/Decrease volume",
                   (char *) 0, &menuCallback_Options_SndDecVol, (void *) this);
  mainMenuBar->add("Options/Sound/Configure...",
                   (char *) 0, &menuCallback_Options_SndConfig, (void *) this);
  mainMenuBar->add("Options/Floppy/Configure... (Alt+D)",
                   (char *) 0, &menuCallback_Options_FloppyCfg, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk/Unit 8",
                   (char *) 0, &menuCallback_Options_FloppyRmA, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk/Unit 9",
                   (char *) 0, &menuCallback_Options_FloppyRmB, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk/Unit 10",
                   (char *) 0, &menuCallback_Options_FloppyRmC, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk/Unit 11",
                   (char *) 0, &menuCallback_Options_FloppyRmD, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk/All drives",
                   (char *) 0, &menuCallback_Options_FloppyRmv, (void *) this);
  mainMenuBar->add("Options/Floppy/Replace disk/Unit 8",
                   (char *) 0, &menuCallback_Options_FloppyRpA, (void *) this);
  mainMenuBar->add("Options/Floppy/Replace disk/Unit 9",
                   (char *) 0, &menuCallback_Options_FloppyRpB, (void *) this);
  mainMenuBar->add("Options/Floppy/Replace disk/Unit 10",
                   (char *) 0, &menuCallback_Options_FloppyRpC, (void *) this);
  mainMenuBar->add("Options/Floppy/Replace disk/Unit 11",
                   (char *) 0, &menuCallback_Options_FloppyRpD, (void *) this);
  mainMenuBar->add("Options/Floppy/Replace disk/All drives",
                   (char *) 0, &menuCallback_Options_FloppyRpl, (void *) this);
  mainMenuBar->add("Options/Floppy/Reset/Unit 8",
                   (char *) 0, &menuCallback_Options_FloppyRsA, (void *) this);
  mainMenuBar->add("Options/Floppy/Reset/Unit 9",
                   (char *) 0, &menuCallback_Options_FloppyRsB, (void *) this);
  mainMenuBar->add("Options/Floppy/Reset/Unit 10",
                   (char *) 0, &menuCallback_Options_FloppyRsC, (void *) this);
  mainMenuBar->add("Options/Floppy/Reset/Unit 11",
                   (char *) 0, &menuCallback_Options_FloppyRsD, (void *) this);
  mainMenuBar->add("Options/Floppy/Reset/All drives",
                   (char *) 0, &menuCallback_Options_FloppyRst, (void *) this);
  mainMenuBar->add("Options/Floppy/Disable unused drives",
                   (char *) 0, &menuCallback_Options_FloppyGC, (void *) this);
  mainMenuBar->add("Options/Keyboard map (Alt+I)",
                   (char *) 0, &menuCallback_Options_KbdConfig, (void *) this);
  mainMenuBar->add("Options/Set working directory (Alt+Q)",
                   (char *) 0, &menuCallback_Options_FileIODir, (void *) this);
  mainMenuBar->add("Debug/Start debugger (Alt+M)",
                   (char *) 0, &menuCallback_Debug_OpenDebugger, (void *) this);
  mainMenuBar->add("Debug/Show drive stats",
                   (char *) 0, &menuCallback_Debug_DriveStats, (void *) this);
  mainMenuBar->add("Help/About",
                   (char *) 0, &menuCallback_Help_About, (void *) this);
  const_cast<Fl_Menu_Item *>(mainMenuBar->find_item(
                                 "File/Record video/Stop"))->deactivate();
  mainMenuBar->mode(int(mainMenuBar->find_item("Machine/Speed/No limit (Alt+W)")
                        - mainMenuBar->menu()),
                    FL_MENU_TOGGLE);
  mainMenuBar->mode(int(mainMenuBar->find_item("Machine/Printer/Enable printer")
                        - mainMenuBar->menu()),
                    FL_MENU_TOGGLE);
  mainMenuBar->mode(int(mainMenuBar->find_item("Machine/Printer/1525 mode")
                        - mainMenuBar->menu()),
                    FL_MENU_TOGGLE);
  mainMenuBar->mode(int(mainMenuBar->find_item("Machine/Enable light pen")
                        - mainMenuBar->menu()),
                    FL_MENU_TOGGLE);
  updateMenu();
  mainWindow->show();
  emulatorWindow->show();
  // load and apply GUI configuration
  try {
    Plus4Emu::File  f("gui_cfg.dat", true);
    try {
      guiConfig.registerChunkType(f);
      f.processAllChunks();
    }
    catch (std::exception& e) {
      errorMessage(e.what());
    }
  }
  catch (...) {
  }
  vmThread.lock(0x7FFFFFFF);
  vmThread.setUserData((void *) this);
  vmThread.setErrorCallback(&errorMessageCallback);
  vmThread.setProcessCallback(&pollJoystickInput);
  vm.setFileNameCallback(&fileNameCallback, (void *) this);
  vm.setBreakPointCallback(&Plus4EmuGUI_DebugWindow::breakPointCallback,
                           (void *) debugWindow);
  applyEmulatorConfiguration();
  vmThread.unlock();
  // run emulation
  vmThread.pause(false);
  do {
    updateDisplay();
  } while (mainWindow->shown() && !exitFlag);
  // close windows and stop emulation thread
  browseFileWindowShowFlag = false;
  windowToShow = (Fl_Window *) 0;
  if (errorMessageWindow->shown())
    errorMessageWindow->hide();
  debugWindowShowFlag = false;
  if (debugWindow->shown())
    debugWindow->hide();
  if (debugWindowOpenFlag) {
    debugWindowOpenFlag = false;
    unlockVMThread();
  }
  updateDisplay();
  Fl::unlock();
  vmThread.quit(true);
  Fl::lock();
  if (mainWindow->shown())
    mainWindow->hide();
  updateDisplay();
  if (errorFlag)
    errorMessage("exiting due to a fatal error");
  // if still recording a demo, stop now, and write file
  try {
    vm.stopDemo();
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
  closeDemoFile(false);
  // save GUI configuration
  try {
    Plus4Emu::File  f;
    guiConfig.saveState(f);
    f.writeFile("gui_cfg.dat", true);
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
}

void Plus4EmuGUI::resizeWindow(int w, int h)
{
  if ((displayMode & 2) == 0) {
    config.display.width = w;
    config.display.height = h;
    if ((displayMode & 1) == 0)
      h += (w >= 745 ? 30 : 60);
    mainWindow->resize(mainWindow->x(), mainWindow->y(), w, h);
    if ((displayMode & 1) == 0)
      mainWindow->size_range(384, 348, 1536, 1182);
    else
      mainWindow->size_range(384, 288, 1536, 1152);
  }
  if ((displayMode & 1) == 0)
    emulatorWindow->cursor(FL_CURSOR_DEFAULT);
  else
    emulatorWindow->cursor(FL_CURSOR_NONE);
}

int Plus4EmuGUI::handleFLTKEvent(void *userData, int event)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(userData));
  switch (event) {
  case FL_FOCUS:
    return 1;
  case FL_UNFOCUS:
    gui_.functionKeyState = 0U;
    try {
      gui_.vmThread.resetKeyboard();
    }
    catch (std::exception& e) {
      gui_.errorMessage(e.what());
    }
    return 1;
  case FL_PUSH:
  case FL_DRAG:
    gui_.emulatorWindow->take_focus();
    if (gui_.lightPenEnabled) {
      double  xPos = double(Fl::event_x());
      double  yPos = double(Fl::event_y());
      double  w = double(gui_.emulatorWindow->w());
      double  h = double(gui_.emulatorWindow->h());
      w = (w >= 1.0 ? w : 1.0);
      h = (h >= 1.0 ? h : 1.0);
      double  aspectRatio = gui_.config.display.pixelAspectRatio;
      aspectRatio = (aspectRatio >= 0.125 ? aspectRatio : 0.125);
      aspectRatio = (aspectRatio * w / h) * 0.75;
      xPos = xPos / w;
      yPos = yPos / h;
      if (aspectRatio >= 1.0)
        xPos = (xPos - 0.5) * aspectRatio + 0.5;
      else
        yPos = (yPos - 0.5) / aspectRatio + 0.5;
      xPos = xPos - 0.002604;
      if (xPos >= 0.0 && xPos < 1.0 && yPos >= 0.0 && yPos < 1.0) {
        gui_.vmThread.setLightPenPosition(int(xPos * 65536.0),
                                          int(yPos * 65536.0));
      }
      else
        gui_.vmThread.setLightPenPosition(-1, -1);
    }
    return 1;
  case FL_RELEASE:
    gui_.vmThread.setLightPenPosition(-1, -1);
    return 1;
  case FL_KEYUP:
  case FL_KEYDOWN:
    {
      int   keyCode = Fl::event_key();
      bool  isKeyPress = (event == FL_KEYDOWN);
      if (!(keyCode >= (FL_F + 9) && keyCode <= (FL_F + 12))) {
        int   n = gui_.config.convertKeyCode(keyCode);
        if (n >= 0 && (gui_.functionKeyState == 0U || !isKeyPress)) {
          try {
#if 0 && defined(WIN32)
            if (keyCode == FL_Shift_L || keyCode == FL_Shift_R) {
              // work around FLTK bug
              int   tmp = gui_.config.convertKeyCode(FL_Shift_L);
              if (tmp >= 0)
                gui_.vmThread.setKeyboardState(uint8_t(tmp),
                                               (GetKeyState(VK_LSHIFT) < 0));
              tmp = gui_.config.convertKeyCode(FL_Shift_R);
              if (tmp >= 0)
                gui_.vmThread.setKeyboardState(uint8_t(tmp),
                                               (GetKeyState(VK_RSHIFT) < 0));
            }
            else
#endif
            {
              gui_.vmThread.setKeyboardState(uint8_t(n), isKeyPress);
            }
          }
          catch (std::exception& e) {
            gui_.errorMessage(e.what());
          }
          if (gui_.functionKeyState == 0U || isKeyPress)
            return 1;
        }
      }
      int   n = -1;
      switch (keyCode) {
      case (FL_F + 5):
      case (FL_F + 6):
      case (FL_F + 7):
      case (FL_F + 8):
      case (FL_F + 9):
      case (FL_F + 10):
      case (FL_F + 11):
      case (FL_F + 12):
        n = keyCode - (FL_F + 5);
        if (isKeyPress) {
          if (Fl::event_shift()) {
            n += 8;
          }
          else if (Fl::event_ctrl()) {
            if (n >= 4)
              n += 12;
            else
              n = -1;
          }
        }
        break;
      case 0x64:                // Alt + D
        if (Fl::event_alt() || !isKeyPress)
          n = 20;
        break;
      case 0x69:                // Alt + I
        if (Fl::event_alt() || !isKeyPress)
          n = 21;
        break;
      case 0x6D:                // Alt + M
        if (Fl::event_alt() || !isKeyPress)
          n = 22;
        break;
      case 0x71:                // Alt + Q
        if (Fl::event_alt() || !isKeyPress)
          n = 23;
        break;
      case 0x72:                // Alt + R
        if (Fl::event_alt() || !isKeyPress)
          n = 24;
        break;
      case 0x74:                // Alt + T
        if (Fl::event_alt() || !isKeyPress)
          n = 25;
        break;
      case 0x77:                // Alt + W
        if (Fl::event_alt() || !isKeyPress)
          n = 26;
        break;
      case FL_Pause:
        n = 27;
        break;
      case FL_Alt_L:
        n = 28;
        break;
      case FL_Alt_R:
        n = 29;
        break;
      case FL_Page_Down:
        n = 30;
        break;
      case FL_Page_Up:
        n = 31;
        break;
      }
      if (n >= 0) {
        uint32_t  bitMask = 1U << n;
        bool      wasPressed = bool(gui_.functionKeyState & bitMask);
        if (isKeyPress != wasPressed) {
          if (n < 20) {
            if (n >= 16)
              bitMask = bitMask >> 12;
            else if (n >= 8)
              bitMask = bitMask >> 8;
            if (bitMask < 0x00000010U)
              bitMask = bitMask | (bitMask << 8);
            else
              bitMask = bitMask | (bitMask << 8) | (bitMask << 12);
          }
          if (isKeyPress)
            gui_.functionKeyState |= bitMask;
          else
            gui_.functionKeyState &= (bitMask ^ uint32_t(0xFFFFFFFFU));
          if (isKeyPress) {
            switch (n) {
            case 0:                                     // F5:
              gui_.menuCallback_Machine_TapePlay((Fl_Widget *) 0, userData);
              break;
            case 1:                                     // F6:
              gui_.menuCallback_File_Screenshot((Fl_Widget *) 0, userData);
              break;
            case 2:                                     // F7:
              gui_.menuCallback_File_LoadFile((Fl_Widget *) 0, userData);
              break;
            case 3:                                     // F8:
              gui_.menuCallback_File_LoadPRG((Fl_Widget *) 0, userData);
              break;
            case 4:                                     // F9:
              gui_.menuCallback_Options_DpyMode((Fl_Widget *) 0, userData);
              break;
            case 5:                                     // F10:
              gui_.menuCallback_Machine_Pause((Fl_Widget *) 0, userData);
              break;
            case 6:                                     // F11:
              gui_.menuCallback_Machine_Reset((Fl_Widget *) 0, userData);
              break;
            case 7:                                     // F12:
              gui_.menuCallback_Machine_TapeNxtCP((Fl_Widget *) 0, userData);
              break;
            case 8:                                     // Shift + F5:
              gui_.menuCallback_Machine_TapeStop((Fl_Widget *) 0, userData);
              break;
            case 9:                                     // Shift + F6:
              gui_.menuCallback_Machine_TapeRecord((Fl_Widget *) 0, userData);
              break;
            case 10:                                    // Shift + F7:
              gui_.menuCallback_File_SaveSnapshot((Fl_Widget *) 0, userData);
              break;
            case 11:                                    // Shift + F8:
              gui_.menuCallback_File_SavePRG((Fl_Widget *) 0, userData);
              break;
            case 12:                                    // Shift + F9:
              gui_.menuCallback_Options_DpyConfig((Fl_Widget *) 0, userData);
              break;
            case 13:                                    // Shift + F10:
              gui_.menuCallback_Machine_Configure((Fl_Widget *) 0, userData);
              break;
            case 14:                                    // Shift + F11:
              gui_.menuCallback_Machine_ResetAll((Fl_Widget *) 0, userData);
              break;
            case 15:                                    // Shift + F12:
              gui_.menuCallback_File_Quit((Fl_Widget *) 0, userData);
              break;
            case 16:                                    // Ctrl + F9:
              gui_.menuCallback_File_QSSave((Fl_Widget *) 0, userData);
              break;
            case 17:                                    // Ctrl + F10:
              gui_.menuCallback_File_QSLoad((Fl_Widget *) 0, userData);
              break;
            case 18:                                    // Ctrl + F11:
              gui_.menuCallback_Machine_ColdReset((Fl_Widget *) 0, userData);
              break;
            case 19:                                    // Ctrl + F12:
              gui_.menuCallback_File_StopDemo((Fl_Widget *) 0, userData);
              break;
            case 20:                                    // Alt + D:
              gui_.menuCallback_Options_FloppyCfg((Fl_Widget *) 0, userData);
              break;
            case 21:                                    // Alt + I:
              gui_.menuCallback_Options_KbdConfig((Fl_Widget *) 0, userData);
              break;
            case 22:                                    // Alt + M:
              gui_.menuCallback_Debug_OpenDebugger((Fl_Widget *) 0, userData);
              break;
            case 23:                                    // Alt + Q:
              gui_.menuCallback_Options_FileIODir((Fl_Widget *) 0, userData);
              break;
            case 24:                                    // Alt + R:
              gui_.menuCallback_Machine_TapeRewind((Fl_Widget *) 0, userData);
              break;
            case 25:                                    // Alt + T:
              gui_.menuCallback_Machine_OpenTape((Fl_Widget *) 0, userData);
              break;
            case 26:                                    // Alt + W:
              gui_.menuCallback_Machine_FullSpeed((Fl_Widget *) 0, userData);
              break;
            case 27:                                    // Pause:
              gui_.menuCallback_Machine_Pause((Fl_Widget *) 0, userData);
              break;
            case 30:                                    // PageDown:
              gui_.menuCallback_Machine_QuickCfgL1((Fl_Widget *) 0, userData);
              break;
            case 31:                                    // PageUp:
              gui_.menuCallback_Machine_QuickCfgL2((Fl_Widget *) 0, userData);
              break;
            }
          }
        }
      }
    }
    return 1;
  }
  return 0;
}

bool Plus4EmuGUI::lockVMThread(size_t t)
{
  if (vmThread.lock(t) != 0) {
    errorMessage("cannot lock virtual machine: "
                 "the emulation thread does not respond");
    return false;
  }
  return true;
}

void Plus4EmuGUI::unlockVMThread()
{
  vmThread.unlock();
}

bool Plus4EmuGUI::browseFile(std::string& fileName, std::string& dirName,
                             const char *pattern, int type, const char *title)
{
  bool    retval = false;
  bool    lockFlag = false;
  try {
#ifdef WIN32
    uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
    uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
    if (currentThreadID != mainThreadID) {
      Fl::lock();
      lockFlag = true;
    }
    browseFileWindow->type(type);
    browseFileWindow->title(title);
    browseFileWindow->filter(pattern);
    browseFileWindow->directory(dirName.c_str());
    if (type != Fl_Native_File_Chooser::BROWSE_DIRECTORY &&
        type != Fl_Native_File_Chooser::BROWSE_MULTI_DIRECTORY &&
        type != Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY) {
      std::string tmp;
      std::string tmp2;
      Plus4Emu::splitPath(fileName, tmp2, tmp);
#if !(defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER) ||    \
      defined(__APPLE__))
      if (dirName.length() > 0) {
        tmp2 = dirName;
        if (dirName[dirName.length() - 1] != '/' &&
            dirName[dirName.length() - 1] != '\\') {
          tmp2 += '/';
        }
        tmp2 += tmp;
        tmp = tmp2;
      }
#endif
      browseFileWindow->preset_file(tmp.c_str());
    }
    else {
      browseFileWindow->preset_file((char *) 0);
    }
    fileName.clear();
    browseFileStatus = 1;
    if (currentThreadID != mainThreadID) {
      browseFileWindowShowFlag = true;
      do {
        Fl::unlock();
        lockFlag = false;
        updateDisplay();
        Fl::lock();
        lockFlag = true;
      } while (browseFileWindowShowFlag);
    }
    else {
      browseFileStatus = browseFileWindow->show();
    }
    if (browseFileStatus < 0) {
      const char  *s = browseFileWindow->errmsg();
      if (s == (char *) 0 || s[0] == '\0')
        s = "error selecting file";
      std::string tmp(s);
      if (currentThreadID != mainThreadID) {
        Fl::unlock();
        lockFlag = false;
      }
      errorMessage(tmp.c_str());
      if (currentThreadID != mainThreadID) {
        Fl::lock();
        lockFlag = true;
      }
    }
    else if (browseFileStatus == 0) {
      const char  *s = browseFileWindow->filename();
      if (s != (char *) 0) {
        fileName = s;
        Plus4Emu::stripString(fileName);
        std::string tmp;
        Plus4Emu::splitPath(fileName, dirName, tmp);
        retval = true;
      }
    }
    if (currentThreadID != mainThreadID) {
      Fl::unlock();
      lockFlag = false;
    }
  }
  catch (std::exception& e) {
    if (lockFlag)
      Fl::unlock();
    fileName.clear();
    errorMessage(e.what());
  }
  return retval;
}

void Plus4EmuGUI::applyEmulatorConfiguration(bool updateWindowFlag_)
{
  if (lockVMThread()) {
    try {
      bool    updateMenuFlag_ = config.soundSettingsChanged;
      config.applySettings();
      vmThread.setSpeedPercentage(config.vm.speedPercentage == 100U &&
                                  config.sound.enabled ?
                                  0 : int(config.vm.speedPercentage));
      if (config.joystickSettingsChanged) {
        joystickInput.setConfiguration(config.joystick);
        config.joystickSettingsChanged = false;
      }
      if (updateWindowFlag_) {
        if (diskConfigWindow->shown())
          diskConfigWindow->updateWindow();
        if (displaySettingsWindow->shown())
          displaySettingsWindow->updateWindow();
        if (keyboardConfigWindow->shown())
          keyboardConfigWindow->updateWindow();
        if (soundSettingsWindow->shown())
          soundSettingsWindow->updateWindow();
        if (machineConfigWindow->shown())
          machineConfigWindow->updateWindow();
        if (debugWindow->shown())
          debugWindow->updateWindow();
      }
      if (updateMenuFlag_)
        updateMenu();
    }
    catch (...) {
      unlockVMThread();
      throw;
    }
    unlockVMThread();
  }
}

void Plus4EmuGUI::updateMenu()
{
  const Fl_Menu_Item  *m;
  m = mainMenuBar->find_item("Machine/Speed/No limit (Alt+W)");
  if (config.vm.speedPercentage == 0U)
    const_cast<Fl_Menu_Item *>(m)->set();
  else
    const_cast<Fl_Menu_Item *>(m)->clear();
  m = mainMenuBar->find_item("File/Record audio/Stop");
  if (config.sound.file.length() > 0)
    const_cast<Fl_Menu_Item *>(m)->activate();
  else
    const_cast<Fl_Menu_Item *>(m)->deactivate();
}

void Plus4EmuGUI::errorMessageCallback(void *userData, const char *msg)
{
  reinterpret_cast<Plus4EmuGUI *>(userData)->errorMessage(msg);
}

void Plus4EmuGUI::fileNameCallback(void *userData, std::string& fileName)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(userData));
  try {
    std::string tmp(gui_.config.fileio.workingDirectory);
    gui_.browseFile(fileName, tmp, "All files\t*",
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
                    Fl_Native_File_Chooser::BROWSE_FILE,
#else
                    Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                    "Open file");
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::fltkCheckCallback(void *userData)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(userData));
  try {
    if (gui_.flDisplay->checkEvents())
      gui_.emulatorWindow->redraw();
  }
  catch (...) {
  }
}

void Plus4EmuGUI::screenshotCallback(void *userData,
                                     const unsigned char *buf, int w_, int h_)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(userData));
  std::string   fName;
  std::FILE     *f = (std::FILE *) 0;
  try {
    fName = gui_.screenshotFileName;
    if (!gui_.browseFile(fName, gui_.screenshotDirectory, "TGA files\t*.tga",
                         Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                         "Save screenshot"))
      return;
    gui_.screenshotFileName = fName;
    f = std::fopen(fName.c_str(), "wb");
    if (!f)
      throw Plus4Emu::Exception("error opening screenshot file");
    unsigned char tgaHeader[24];
    unsigned char lineBuf[3080];
    tgaHeader[0] = 0;                   // ID length
    tgaHeader[1] = 0;                   // no colormap
    tgaHeader[2] = 10;                  // image type (RLE true color)
    tgaHeader[3] = 0;                   // color map specification
    tgaHeader[4] = 0;
    tgaHeader[5] = 0;
    tgaHeader[6] = 0;
    tgaHeader[7] = 0;
    tgaHeader[8] = 0;                   // X origin
    tgaHeader[9] = 0;
    tgaHeader[10] = 0;                  // Y origin
    tgaHeader[11] = 0;
    tgaHeader[12] = (unsigned char) (w_ & 0xFF);        // image width
    tgaHeader[13] = (unsigned char) (w_ >> 8);
    tgaHeader[14] = (unsigned char) (h_ & 0xFF);        // image height
    tgaHeader[15] = (unsigned char) (h_ >> 8);
    tgaHeader[16] = 24;                 // pixel depth
    tgaHeader[17] = 0x20;               // image descriptor (origin: top/left)
    if (std::fwrite(&(tgaHeader[0]), sizeof(unsigned char), 18, f) != 18)
      throw Plus4Emu::Exception("error writing screenshot file "
                                "- is the disk full ?");
    for (int yc = 0; yc < h_; yc++) {
      const unsigned char *rBuf = &(buf[(yc * w_ * 3) + 0]);
      const unsigned char *gBuf = &(buf[(yc * w_ * 3) + 1]);
      const unsigned char *bBuf = &(buf[(yc * w_ * 3) + 2]);
      // RLE encode line
      unsigned char   *p = &(lineBuf[0]);
      int     xc = 0;
      while (xc < w_) {
        if (xc == (w_ - 1)) {
          *(p++) = 0x00;
          *(p++) = bBuf[xc * 3];
          *(p++) = gBuf[xc * 3];
          *(p++) = rBuf[xc * 3];
          xc++;
        }
        else if (rBuf[xc * 3] == rBuf[(xc * 3) + 3] &&
                 gBuf[xc * 3] == gBuf[(xc * 3) + 3] &&
                 bBuf[xc * 3] == bBuf[(xc * 3) + 3]) {
          int     tmp = xc + 2;
          while (tmp < w_ && (tmp - xc) < 128) {
            if (!(rBuf[tmp * 3] == rBuf[xc * 3] &&
                  gBuf[tmp * 3] == gBuf[xc * 3] &&
                  bBuf[tmp * 3] == bBuf[xc * 3]))
              break;
            tmp++;
          }
          *(p++) = (unsigned char) (((tmp - xc) - 1) | 0x80);
          *(p++) = bBuf[xc * 3];
          *(p++) = gBuf[xc * 3];
          *(p++) = rBuf[xc * 3];
          xc = tmp;
        }
        else {
          int     tmp = xc + 2;
          while (tmp < w_ && (tmp - xc) < 128) {
            if (rBuf[tmp * 3] == rBuf[(tmp - 1) * 3] &&
                gBuf[tmp * 3] == gBuf[(tmp - 1) * 3] &&
                bBuf[tmp * 3] == bBuf[(tmp - 1) * 3]) {
              tmp--;
              break;
            }
            tmp++;
          }
          *(p++) = (unsigned char) ((tmp - xc) - 1);
          while (xc < tmp) {
            *(p++) = bBuf[xc * 3];
            *(p++) = gBuf[xc * 3];
            *(p++) = rBuf[xc * 3];
            xc++;
          }
        }
      }
      size_t  nBytes = size_t(p - &(lineBuf[0]));
      if (std::fwrite(&(lineBuf[0]), sizeof(unsigned char), nBytes, f)
          != nBytes)
        throw Plus4Emu::Exception("error writing screenshot file "
                                  "- is the disk full ?");
    }
  }
  catch (std::exception& e) {
    if (f) {
      std::fclose(f);
      f = (std::FILE *) 0;
      if (fName.length() > 0)
        std::remove(fName.c_str());
    }
    gui_.errorMessage(e.what());
  }
  catch (...) {
    if (f) {
      std::fclose(f);
      f = (std::FILE *) 0;
      if (fName.length() > 0)
        std::remove(fName.c_str());
    }
    throw;
  }
  if (f)
    std::fclose(f);
}

void Plus4EmuGUI::pollJoystickInput(void *userData)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(userData));
  while (true) {
    int     e = gui_.joystickInput.getEvent();
    if (!e)
      break;
    int     keyCode = (e > 0 ? e : (-e));
    bool    isKeyPress = (e > 0);
    // NOTE: since this function is called from the emulation thread, it
    // could be unsafe to access the configuration from here; however, the
    // emulation thread is locked while the keyboard map is updated (see
    // applyEmulatorConfiguration())
    int     n = gui_.config.convertKeyCode(keyCode);
    if (n >= 0)
      gui_.vm.setKeyboardState(uint8_t(n), isKeyPress);
  }
}

bool Plus4EmuGUI::closeDemoFile(bool stopDemo_)
{
  if (demoRecordFile) {
    if (stopDemo_) {
      if (lockVMThread()) {
        try {
          vm.stopDemo();
        }
        catch (std::exception& e) {
          unlockVMThread();
          delete demoRecordFile;
          demoRecordFile = (Plus4Emu::File *) 0;
          demoRecordFileName.clear();
          errorMessage(e.what());
          return true;
        }
        catch (...) {
          unlockVMThread();
          delete demoRecordFile;
          demoRecordFile = (Plus4Emu::File *) 0;
          demoRecordFileName.clear();
          throw;
        }
        unlockVMThread();
      }
      else
        return false;
    }
    try {
      demoRecordFile->writeFile(demoRecordFileName.c_str());
    }
    catch (std::exception& e) {
      errorMessage(e.what());
    }
    delete demoRecordFile;
    demoRecordFile = (Plus4Emu::File *) 0;
  }
  demoRecordFileName.clear();
  return true;
}

void Plus4EmuGUI::saveQuickConfig(int n)
{
  const char  *fName = (char *) 0;
  if (n == 1)
    fName = "p4vmcfg1.cfg";
  else
    fName = "p4vmcfg2.cfg";
  try {
    Plus4Emu::ConfigurationDB tmpCfg;
    tmpCfg.createKey("vm.cpuClockFrequency", config.vm.cpuClockFrequency);
    tmpCfg.createKey("vm.videoClockFrequency", config.vm.videoClockFrequency);
    tmpCfg.createKey("vm.speedPercentage", config.vm.speedPercentage);
    tmpCfg.createKey("vm.serialBusDelayOffset", config.vm.serialBusDelayOffset);
    tmpCfg.saveState(fName, true);
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
}

// ----------------------------------------------------------------------------

void Plus4EmuGUI::menuCallback_File_LoadFile(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.loadFileDirectory, "All files\t*",
                        Fl_Native_File_Chooser::BROWSE_FILE,
                        "Load plus4emu binary file")) {
      if (gui_.lockVMThread()) {
        try {
          Plus4Emu::File  f(tmp.c_str());
          gui_.vm.registerChunkTypes(f);
          gui_.config.registerChunkType(f);
          f.processAllChunks();
          gui_.applyEmulatorConfiguration(true);
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_LoadConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.configDirectory,
                        "Configuration files\t*.cfg",
                        Fl_Native_File_Chooser::BROWSE_FILE,
                        "Load ASCII format configuration file")) {
      gui_.config.loadState(tmp.c_str());
      gui_.applyEmulatorConfiguration(true);
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_SaveConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.configDirectory,
                        "Configuration files\t*.cfg",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Save configuration as ASCII text file")) {
      gui_.config.saveState(tmp.c_str());
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_SaveMainCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::File  f;
    gui_.config.saveState(f);
    f.writeFile("plus4cfg.dat", true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_RevertCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::File  f("plus4cfg.dat", true);
    gui_.config.registerChunkType(f);
    f.processAllChunks();
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_QSFileName(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files\t*",
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
                        Fl_Native_File_Chooser::BROWSE_FILE,
#else
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                        "Select quick snapshot file"))
      gui_.quickSnapshotFileName = tmp;
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_QSLoad(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const char  *fName = gui_.quickSnapshotFileName.c_str();
        bool        useHomeDirectory = false;
        if (fName[0] == '\0') {
          fName = "qs_plus4.dat";
          useHomeDirectory = true;
        }
        Plus4Emu::File  f(fName, useHomeDirectory);
        gui_.vm.registerChunkTypes(f);
        f.processAllChunks();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_QSSave(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const char  *fName = gui_.quickSnapshotFileName.c_str();
        bool        useHomeDirectory = false;
        if (fName[0] == '\0') {
          fName = "qs_plus4.dat";
          useHomeDirectory = true;
        }
        Plus4Emu::File  f;
        gui_.vm.saveState(f);
        f.writeFile(fName, useHomeDirectory);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_SaveSnapshot(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files\t*",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Save snapshot")) {
      gui_.loadFileDirectory = gui_.snapshotDirectory;
      if (gui_.lockVMThread()) {
        try {
          Plus4Emu::File  f;
          gui_.vm.saveState(f);
          f.writeFile(tmp.c_str());
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_RecordDemo(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.demoDirectory, "Demo files\t*",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Record demo")) {
      gui_.loadFileDirectory = gui_.demoDirectory;
      if (gui_.lockVMThread()) {
        if (gui_.closeDemoFile(true)) {
          try {
            gui_.demoRecordFile = new Plus4Emu::File();
            gui_.demoRecordFileName = tmp;
            gui_.vm.recordDemo(*(gui_.demoRecordFile));
          }
          catch (...) {
            gui_.unlockVMThread();
            if (gui_.demoRecordFile) {
              delete gui_.demoRecordFile;
              gui_.demoRecordFile = (Plus4Emu::File *) 0;
            }
            gui_.demoRecordFileName.clear();
            throw;
          }
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_StopDemo(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.stopDemo();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_RecordSound(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.soundFileDirectory, "Sound files\t*.wav",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Record sound output to WAV file")) {
      gui_.config["sound.file"] = tmp;
      gui_.applyEmulatorConfiguration(true);
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_StopSndRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["sound.file"] = std::string("");
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_RecordVideo(Fl_Widget *o, void *v)
{
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (!gui_.browseFile(tmp, gui_.soundFileDirectory, "AVI files\t*.avi",
                         Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                         "Record video output to AVI file")) {
      return;
    }
    if (tmp.length() < 1) {
      gui_.menuCallback_File_StopAVIRecord(o, v);
      return;
    }
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.openVideoCapture(gui_.config.videoCapture.frameRate,
                                 gui_.config.videoCapture.yuvFormat,
                                 &Plus4EmuGUI::errorMessageCallback,
                                 &Plus4EmuGUI::fileNameCallback, v);
        const_cast<Fl_Menu_Item *>(gui_.mainMenuBar->find_item(
                                       "File/Record video/Stop"))->activate();
        gui_.vm.setVideoCaptureFile(tmp);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.menuCallback_File_StopAVIRecord(o, v);
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_StopAVIRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.closeVideoCapture();
        const_cast<Fl_Menu_Item *>(gui_.mainMenuBar->find_item(
                                       "File/Record video/Stop"))->deactivate();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_Screenshot(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.flDisplay->setScreenshotCallback(&screenshotCallback, v);
}

void Plus4EmuGUI::menuCallback_File_LoadPRG(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp(gui_.prgFileName);
    if (gui_.browseFile(tmp, gui_.prgFileDirectory, "PRG files\t*.{prg,p00}",
                        Fl_Native_File_Chooser::BROWSE_FILE,
                        "Load program file")) {
      gui_.prgFileName = tmp;
      if (gui_.lockVMThread()) {
        try {
          Plus4::Plus4VM  *p4vm = dynamic_cast<Plus4::Plus4VM *>(&(gui_.vm));
          p4vm->loadProgram(tmp.c_str());
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_SavePRG(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp(gui_.prgFileName);
    if (gui_.browseFile(tmp, gui_.prgFileDirectory, "PRG files\t*.prg",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Save program file")) {
      gui_.prgFileName = tmp;
      if (gui_.lockVMThread()) {
        try {
          Plus4::Plus4VM  *p4vm = dynamic_cast<Plus4::Plus4VM *>(&(gui_.vm));
          p4vm->saveProgram(tmp.c_str());
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_File_Quit(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.quit(false);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Pause(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.pause(!gui_.oldPauseFlag);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_FullSpeed(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["vm.speedPercentage"];
    if ((unsigned int) cv != 0U) {
      gui_.savedSpeedPercentage = (unsigned int) cv;
      cv = 0U;
    }
    else
      cv = gui_.savedSpeedPercentage;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_10(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 10U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_25(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 25U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_50(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 50U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_100(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 100U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_200(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 200U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_Speed_400(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 400U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_OpenTape(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.tapeImageDirectory,
                        "Tape files\t*.{tap,wav,aif,aiff,au,snd}",
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
                        Fl_Native_File_Chooser::BROWSE_FILE,
#else
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                        "Select tape image file")) {
      Plus4EmuGUI::menuCallback_Machine_TapeStop(o, v);
      gui_.config["tape.imageFile"] = tmp;
      gui_.applyEmulatorConfiguration();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapePlay(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.tapePlay();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 1;
}

void Plus4EmuGUI::menuCallback_Machine_TapeStop(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.tapeStop();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 0;
}

void Plus4EmuGUI::menuCallback_Machine_TapeRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.tapeRecord();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 2;
}

void Plus4EmuGUI::menuCallback_Machine_TapeRewind(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.tapeSeek(0.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapePrvCP(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.tapeSeekToCuePoint(false, 30.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeBwd10s(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 - 10.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeBwd60s(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 - 60.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeNxtCP(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
    gui_.vmThread.tapeSeekToCuePoint(true, 30.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeFwd10s(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 + 10.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeFwd60s(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 + 60.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_AddCP(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeAddCuePoint();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_DeleteCP(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeDeleteNearestCuePoint();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_DeleteCPs(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeDeleteAllCuePoints();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_TapeClose(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["tape.imageFile"] = std::string("");
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_PrtEnable(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const Fl_Menu_Item  *m =
            gui_.mainMenuBar->find_item("Machine/Printer/Enable printer");
        gui_.vm.setEnablePrinter(m->value() != 0);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_PrtMode(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const Fl_Menu_Item  *m =
            gui_.mainMenuBar->find_item("Machine/Printer/1525 mode");
        gui_.vm.setPrinter1525Mode(m->value() != 0);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_PrtShowWin(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.printerWindow->show();
}

void Plus4EmuGUI::menuCallback_Machine_Reset(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.reset(false);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_ColdReset(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.vmThread.reset(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_ResetFreqs(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config.vmConfigurationChanged = true;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_ResetAll(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.stopDemo();
        gui_.config.vmConfigurationChanged = true;
        gui_.config.memoryConfigurationChanged = true;
        gui_.config.applySettings();
        gui_.vm.reset(true);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_EnableSID(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.writeMemory(0x0010FD5FU, 0x00, false);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Machine_DisableSID(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.disableSIDEmulation();
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Machine_EnableLP(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  const Fl_Menu_Item  *m =
      gui_.mainMenuBar->find_item("Machine/Enable light pen");
  gui_.lightPenEnabled = (m->value() != 0);
  if (!gui_.lightPenEnabled)
    gui_.vmThread.setLightPenPosition(-1, -1);
}

void Plus4EmuGUI::menuCallback_Machine_QuickCfgL1(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config.loadState("p4vmcfg1.cfg", true);
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_QuickCfgL2(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config.loadState("p4vmcfg2.cfg", true);
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Machine_QuickCfgS1(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.saveQuickConfig(1);
}

void Plus4EmuGUI::menuCallback_Machine_QuickCfgS2(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.saveQuickConfig(2);
}

void Plus4EmuGUI::menuCallback_Machine_Configure(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.machineConfigWindow->show();
}

void Plus4EmuGUI::menuCallback_Options_DpyMode(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  switch (gui_.displayMode) {
  case 0:
    gui_.displayMode = 2;
    break;
  case 2:
    gui_.displayMode = 3;
    break;
  case 3:
    gui_.displayMode = 1;
    break;
  default:
    gui_.displayMode = 0;
    break;
  }
}

void Plus4EmuGUI::menuCallback_Options_DpySize1(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.resizeWindow(384, 288);
}

void Plus4EmuGUI::menuCallback_Options_DpySize2(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.resizeWindow(768, 576);
}

void Plus4EmuGUI::menuCallback_Options_DpySize3(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.resizeWindow(1152, 864);
}

void Plus4EmuGUI::menuCallback_Options_DpyConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.displaySettingsWindow->show();
}

void Plus4EmuGUI::menuCallback_Options_SndIncVol(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["sound.volume"];
    cv = double(cv) * 1.2589;
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_SndDecVol(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["sound.volume"];
    cv = double(cv) * 0.7943;
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_SndConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.soundSettingsWindow->show();
}

void Plus4EmuGUI::menuCallback_Options_FloppyCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.diskConfigWindow->show();
}

void Plus4EmuGUI::menuCallback_Options_FloppyRmA(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["floppy.a.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRmB(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["floppy.b.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRmC(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["floppy.c.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRmD(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["floppy.d.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRmv(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    gui_.config["floppy.a.imageFile"] = "";
    gui_.config["floppy.b.imageFile"] = "";
    gui_.config["floppy.c.imageFile"] = "";
    gui_.config["floppy.d.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRpA(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.a.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRpB(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.b.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRpC(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.c.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRpD(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.d.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRpl(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cvA =
            gui_.config["floppy.a.imageFile"];
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cvB =
            gui_.config["floppy.b.imageFile"];
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cvC =
            gui_.config["floppy.c.imageFile"];
        Plus4Emu::ConfigurationDB::ConfigurationVariable& cvD =
            gui_.config["floppy.d.imageFile"];
        std::string tmpA = std::string(cvA);
        std::string tmpB = std::string(cvB);
        std::string tmpC = std::string(cvC);
        std::string tmpD = std::string(cvD);
        cvA = "";
        cvB = "";
        cvC = "";
        cvD = "";
        gui_.applyEmulatorConfiguration();
        cvA = tmpA;
        cvB = tmpB;
        cvC = tmpC;
        cvD = tmpD;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRsA(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.resetFloppyDrive(0);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRsB(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.resetFloppyDrive(1);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRsC(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.resetFloppyDrive(2);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRsD(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.resetFloppyDrive(3);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyRst(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.resetFloppyDrive(-1);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_FloppyGC(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.disableUnusedFloppyDrives();
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Plus4EmuGUI::menuCallback_Options_KbdConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.keyboardConfigWindow->show();
}

void Plus4EmuGUI::menuCallback_Options_FileIODir(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  try {
    Plus4Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["fileio.workingDirectory"];
    std::string tmp;
    std::string tmp2 = std::string(cv);
    if (gui_.browseFile(tmp, tmp2, "*",
                        Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY,
                        "Select working directory for the emulated machine")) {
      cv = tmp;
      gui_.applyEmulatorConfiguration();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Plus4EmuGUI::menuCallback_Debug_OpenDebugger(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  if (!gui_.debugWindow->shown()) {
    if (gui_.debugWindowShowFlag)
      return;
    if (!gui_.lockVMThread())
      return;
    gui_.debugWindowOpenFlag = true;
  }
  gui_.debugWindow->show();
}

void Plus4EmuGUI::menuCallback_Debug_DriveStats(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.floppyStatsWindow->show();
}

void Plus4EmuGUI::menuCallback_Help_About(Fl_Widget *o, void *v)
{
  (void) o;
  Plus4EmuGUI&  gui_ = *(reinterpret_cast<Plus4EmuGUI *>(v));
  gui_.aboutWindow->show();
}

