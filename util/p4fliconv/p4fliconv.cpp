
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

#include "p4fliconv.hpp"
#include "interlace7.hpp"
#include "mcfli.hpp"
#include "compress.hpp"
#include "guicolor.hpp"

#include <string>
#include <vector>
#include <map>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

static const float brightnessToYTable[9] = {
  2.00f,  2.40f,  2.55f,  2.70f,  2.90f,  3.30f,  3.60f,  4.10f,  4.80f
};

static const float colorPhaseTablePAL[16] = {
    0.0f,    0.0f,  103.0f,  283.0f,   53.0f,  241.0f,  347.0f,  167.0f,
  129.0f,  148.0f,  195.0f,   83.0f,  265.0f,  323.0f,    3.0f,  213.0f
};

static void defaultProgressMessageCb(void *userData, const char *msg)
{
  (void) userData;
  if (msg != (char *) 0 && msg[0] != '\0')
    std::fprintf(stderr, "%s\n", msg);
}

static bool defaultProgressPercentageCb(void *userData, int n)
{
  (void) userData;
  if (n != 100)
    std::fprintf(stderr, "\r  %3d%%    ", n);
  else
    std::fprintf(stderr, "\r  %3d%%    \n", n);
  return true;
}

namespace Plus4FLIConv {

  const float FLIConverter::defaultColorSaturation = 0.205f;

  void FLIConverter::convertPlus4Color(int c, float& y, float& u, float& v,
                                       double monitorGamma_)
  {
    int     l = ((c & 0x70) >> 4) + 1;
    c = c & 0x0F;
    if (c != 0) {
      y = (brightnessToYTable[l] - brightnessToYTable[0])
          / (brightnessToYTable[8] - brightnessToYTable[0]);
      y = float(std::pow(double(y), monitorGamma_));
      if (c != 1) {
        double  phs = double(colorPhaseTablePAL[c]) * 3.14159265 / 180.0;
        u = float(std::cos(phs)) * defaultColorSaturation;
        v = float(std::sin(phs)) * defaultColorSaturation;
      }
      else {
        u = 0.0f;
        v = 0.0f;
      }
    }
    else {
      y = 0.0f;
      u = 0.0f;
      v = 0.0f;
    }
  }

  FLIConverter::FLIConverter()
    : progressMessageCallback(&defaultProgressMessageCb),
      progressMessageUserData((void *) 0),
      progressPercentageCallback(&defaultProgressPercentageCb),
      progressPercentageUserData((void *) 0),
      prvProgressPercentage(-1)
  {
  }

  FLIConverter::~FLIConverter()
  {
  }

  bool FLIConverter::processImage(PRGData& prgData, unsigned int& prgEndAddr,
                                  const char *infileName,
                                  YUVImageConverter& imgConv,
                                  Plus4Emu::ConfigurationDB& config)
  {
    (void) infileName;
    (void) imgConv;
    (void) config;
    prgData[0] = 0x01;
    prgData[1] = 0x10;
    prgData[2] = 0x00;
    prgData[3] = 0x00;
    prgEndAddr = 0x1003U;
    return true;
  }

  void FLIConverter::setProgressMessageCallback(void (*func)(void *userData,
                                                             const char *msg),
                                                void *userData_)
  {
    if (func) {
      progressMessageCallback = func;
      progressMessageUserData = userData_;
    }
    else {
      progressMessageCallback = &defaultProgressMessageCb;
      progressMessageUserData = (void *) 0;
    }
  }

  void FLIConverter::setProgressPercentageCallback(bool (*func)(void *userData,
                                                                int n),
                                                   void *userData_)
  {
    if (func) {
      progressPercentageCallback = func;
      progressPercentageUserData = userData_;
    }
    else {
      progressPercentageCallback = &defaultProgressPercentageCb;
      progressPercentageUserData = (void *) 0;
    }
  }

  void FLIConverter::progressMessage(const char *msg)
  {
    if (msg == (char *) 0)
      msg = "";
    progressMessageCallback(progressMessageUserData, msg);
  }

  bool FLIConverter::setProgressPercentage(int n)
  {
    limitValue(n, 0, 100);
    if (n != prvProgressPercentage) {
      prvProgressPercentage = n;
      return progressPercentageCallback(progressPercentageUserData, n);
    }
    return true;
  }

}       // namespace Plus4FLIConv

// ----------------------------------------------------------------------------

Plus4FLIConvGUI_TED7360::Plus4FLIConvGUI_TED7360(Plus4FLIConvGUI& gui_)
  : Plus4::TED7360(),
    gui(gui_)
{
}

Plus4FLIConvGUI_TED7360::~Plus4FLIConvGUI_TED7360()
{
}

void Plus4FLIConvGUI_TED7360::videoOutputCallback(const uint8_t *buf,
                                                  size_t nBytes)
{
  if (gui.previewEnabled && !gui.busyFlag)
    gui.display->sendVideoOutput(buf, nBytes);
}

// ----------------------------------------------------------------------------

void Plus4FLIConvGUI::init_()
{
  display = new Plus4FLIConvGUI_Display(*this, 6, 35, 768, 576, "", false);
  ted = new Plus4FLIConvGUI_TED7360(*this);
  emulatorWindow = display;
  imageFileData = new unsigned char[768 * 576 * 3];
  unsigned char r = 0x00;
  unsigned char g = 0x00;
  unsigned char b = 0x00;
  Fl::get_color(Fl_Color(48), r, g, b);
  for (int i = 0; i < (768 * 576 * 3); i += 3) {
    imageFileData[i] = r;
    imageFileData[i + 1] = g;
    imageFileData[i + 2] = b;
  }
  imageFileName = "";
  busyFlag = false;
  stopFlag = false;
  previewEnabled = false;
  fileChangedFlag = false;
  fileNotSavedFlag = false;
  confirmStatus = false;
  browseFileWindow = new Fl_File_Chooser("", "*", Fl_File_Chooser::SINGLE, "");
  imageFileDirectory = "";
  configDirectory = "";
  prgFileDirectory = "";
  guiConfig.createKey("imageFileDirectory", imageFileDirectory);
  guiConfig.createKey("configDirectory", configDirectory);
  guiConfig.createKey("prgFileDirectory", prgFileDirectory);
  emulationTimer.reset();
  prgData.clear();
  prgData[2] = 0x00;
  prgData[3] = 0x00;
  prgEndAddress = 0x1003U;
  Fl::add_check(&fltkCheckCallback, (void *) this);
}

void Plus4FLIConvGUI::updateDisplay(double t)
{
  Fl::wait(t);
}

bool Plus4FLIConvGUI::confirmMessage(const char *msg)
{
  while (confirmMessageWindow->shown())
    updateDisplay();
  confirmStatus = false;
  if (msg)
    confirmMessageText->copy_label(msg);
  else
    confirmMessageText->label("");
  confirmMessageWindow->set_modal();
  confirmMessageWindow->show();
  do {
    updateDisplay();
  } while (confirmMessageWindow->shown());
  confirmMessageText->label("");
  return confirmStatus;
}

void Plus4FLIConvGUI::errorMessage(const char *msg)
{
  while (errorMessageWindow->shown())
    updateDisplay();
  if (msg)
    errorMessageText->copy_label(msg);
  else
    errorMessageText->label("");
  errorMessageWindow->set_modal();
  errorMessageWindow->show();
  do {
    updateDisplay();
  } while (errorMessageWindow->shown());
  errorMessageText->label("");
}

bool Plus4FLIConvGUI::browseFile(std::string& fileName, std::string& dirName,
                                 const char *pattern, int type,
                                 const char *title)
{
  while (browseFileWindow->shown())
    updateDisplay();
  browseFileWindow->directory(dirName.c_str());
  browseFileWindow->filter(pattern);
  browseFileWindow->type(type);
  browseFileWindow->label(title);
  browseFileWindow->show();
  do {
    updateDisplay();
  } while (browseFileWindow->shown());
  try {
    fileName.clear();
    if (browseFileWindow->value() != (char *) 0) {
      fileName = browseFileWindow->value();
      Plus4Emu::stripString(fileName);
      std::string tmp;
      Plus4Emu::splitPath(fileName, dirName, tmp);
      return true;
    }
  }
  catch (std::exception& e) {
    fileName.clear();
    errorMessage(e.what());
  }
  return false;
}

void Plus4FLIConvGUI::applyConfigurationChanges()
{
  if (!busyFlag) {
    if (fileChangedFlag || config.isFLIConfigurationChanged()) {
      Plus4FLIConv::FLIConverter  *fliConv = (Plus4FLIConv::FLIConverter *) 0;
      bool    doneConversion = false;
      try {
        int     convType = config["conversionType"];
        if (imageFileName != "") {
          if (convType == 0)
            fliConv = new Plus4FLIConv::P4FLI_Interlace7();
          else if (convType == 1)
            fliConv = new Plus4FLIConv::P4FLI_MultiColor();
        }
        if (fliConv) {
          setBusyFlag(true);
          Plus4FLIConv::YUVImageConverter imgConv;
          imgConv.setXYScaleAndOffset(float(double(config["scaleX"])),
                                      float(double(config["scaleY"])),
                                      float(double(config["offsetX"])),
                                      float(double(config["offsetY"])));
          imgConv.setYGamma(float(double(config["monitorGamma"])
                                  / double(config["gammaCorrection"])));
          imgConv.setLuminanceRange(float(double(config["yMin"])),
                                    float(double(config["yMax"])));
          imgConv.setColorSaturation(float(double(config["saturationMult"])),
                                     float(double(config["saturationPow"])));
          float   borderY = 0.0f;
          float   borderU = 0.0f;
          float   borderV = 0.0f;
          Plus4FLIConv::FLIConverter::convertPlus4Color(
              int(config["borderColor"]), borderY, borderU, borderV,
              double(config["monitorGamma"]));
          imgConv.setBorderColor(borderY, borderU, borderV);
          imgConv.setProgressMessageCallback(&progressMessageCallback,
                                             (void *) this);
          imgConv.setProgressPercentageCallback(&progressPercentageCallback,
                                                (void *) this);
          fliConv->setProgressMessageCallback(&progressMessageCallback,
                                              (void *) this);
          fliConv->setProgressPercentageCallback(&progressPercentageCallback,
                                                 (void *) this);
          doneConversion = fliConv->processImage(prgData, prgEndAddress,
                                                 imageFileName.c_str(),
                                                 imgConv, config);
          delete fliConv;
          fliConv = (Plus4FLIConv::FLIConverter *) 0;
          setBusyFlag(false);
        }
      }
      catch (std::exception& e) {
        if (fliConv) {
          delete fliConv;
          fliConv = (Plus4FLIConv::FLIConverter *) 0;
        }
        prgData.clear();
        prgData[2] = 0x00;
        prgData[3] = 0x00;
        prgEndAddress = 0x1003U;
        setBusyFlag(false);
        errorMessage(e.what());
        return;
      }
      fileChangedFlag = false;
      config.clearConfigurationChangeFlag();
      if (!doneConversion) {
        prgData.clear();
        prgData[2] = 0x00;
        prgData[3] = 0x00;
        prgEndAddress = 0x1003U;
      }
      else
        fileNotSavedFlag = true;
      // store the new PRG in the Plus/4 memory, and run it
      setBusyFlag(true);
      ted->reset(true);
      for (int i = 0; i < 400000; i++)
        ted->runOneCycle();
      for (unsigned int i = 0x1001U; i < prgEndAddress; i++)
        ted->writeMemoryCPU(uint16_t(i), uint8_t(prgData[i - 0x0FFFU]));
      ted->writeMemoryCPU(0x0527, 0x52);        // 'R'
      ted->writeMemoryCPU(0x0528, 0x55);        // 'U'
      ted->writeMemoryCPU(0x0529, 0x4E);        // 'N'
      ted->writeMemoryCPU(0x052A, 0x0D);        // Return
      ted->writeMemoryCPU(0x00EF, 0x04);
      setBusyFlag(false);
      // set display parameters depending on whether interlacing is enabled
      try {
        Plus4Emu::VideoDisplay::DisplayParameters dp;
        dp = display->getDisplayParameters();
        if (prgEndAddress <= 0x1003U ||
            int(config["conversionType"]) != 0 ||
            int(config["verticalSize"]) < 400) {
          dp.lineShade = 1.0f;
          dp.blendScale = 1.0f;
          dp.motionBlur = 0.333f;
        }
        else {
          dp.lineShade = 0.25f;
          dp.blendScale = 1.6f;
          dp.motionBlur = 0.75f;
        }
        display->setDisplayParameters(dp);
      }
      catch (...) {
      }
    }
  }
}

void Plus4FLIConvGUI::updateConfigWindow()
{
  try {
    conversionTypeValuator->value(int(config["conversionType"]));
    verticalSizeValuator->value(int(config["verticalSize"]));
    scaleXValuator->value(double(config["scaleX"]));
    scaleYValuator->value(double(config["scaleY"]));
    offsetXValuator->value(double(config["offsetX"]));
    offsetYValuator->value(double(config["offsetY"]));
    yMinValuator->value(double(config["yMin"]));
    yMaxValuator->value(double(config["yMax"]));
    saturationMultValuator->value(double(config["saturationMult"]));
    saturationPowValuator->value(double(config["saturationPow"]));
    gammaCorrectionValuator->value(double(config["gammaCorrection"]));
    monitorGammaValuator->value(double(config["monitorGamma"]));
    borderColorValuator->value(int(config["borderColor"]));
    rawPRGModeValuator->value(int(bool(config["rawPRGMode"])));
    prgCompressionLevelValuator->value(
        double(int(config["prgCompressionLevel"])));
    ditherModeValuator->value(int(config["ditherMode"]));
    ditherLimitValuator->value(double(config["ditherLimit"]));
    ditherDiffusionValuator->value(double(config["ditherDiffusion"]));
    xShift0Valuator->value(int(config["xShift0"]) + 2);
    xShift1Valuator->value(int(config["xShift1"]) + 2);
    luminance1BitModeValuator->value(int(bool(config["luminance1BitMode"])));
    enablePALValuator->value(int(bool(config["enablePAL"])));
    noLuminanceInterlaceValuator->value(
        int(bool(config["noLuminanceInterlace"])));
    colorInterlaceModeValuator->value(int(config["colorInterlaceMode"]));
    luminanceSearchModeValuator->value(int(config["luminanceSearchMode"]));
    lumSearchModeParamValuator->value(
        double(config["luminanceSearchModeParam"]));
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
}

void Plus4FLIConvGUI::openImageFile()
{
  if (busyFlag)
    return;
  try {
    if (!browseFile(imageFileName, imageFileDirectory,
                    "Image files (*.{bmp,jpg,png})", Fl_File_Chooser::SINGLE,
                    "Open image file")) {
      imageFileName = "";
    }
    if (imageFileName != "") {
      Plus4FLIConv::YUVImageConverter imgConv;
      imgConv.setImageSize(768, 576);
      imgConv.setPixelAspectRatio(1.0f);
      unsigned char r = 0x00;
      unsigned char g = 0x00;
      unsigned char b = 0x00;
      Fl::get_color(Fl_Color(48), r, g, b);
      float   y =
          (float(r) * 0.299f) + (float(g) * 0.587f) + (float(b) * 0.114f);
      float   u = (float(b) - y) * 0.492f;
      float   v = (float(r) - y) * 0.877f;
      imgConv.setBorderColor(y / 255.0f, u / 255.0f, v / 255.0f);
      imgConv.setPixelStoreCallback(&pixelStoreCallback, (void *) this);
      imgConv.setProgressMessageCallback(&progressMessageCallback,
                                         (void *) this);
      imgConv.setProgressPercentageCallback(&progressPercentageCallback,
                                            (void *) this);
      setBusyFlag(true);
      fileChangedFlag = imgConv.convertImageFile(imageFileName.c_str());
      setBusyFlag(false);
    }
  }
  catch (std::exception& e) {
    setBusyFlag(false);
    errorMessage(e.what());
  }
  if (fileChangedFlag && previewEnabled)
    applyConfigurationChanges();
}

void Plus4FLIConvGUI::savePRGFile()
{
  if (!busyFlag) {
    applyConfigurationChanges();
    if (prgEndAddress <= 0x1003U) {
      fileNotSavedFlag = false;
      return;
    }
    std::FILE *f = (std::FILE *) 0;
    try {
      std::string prgFileName;
      if (!browseFile(prgFileName, prgFileDirectory,
                      "Plus/4 program files (*.prg)", Fl_File_Chooser::CREATE,
                      "Save PRG file")) {
        prgFileName = "";
      }
      if (prgFileName != "") {
        f = std::fopen(prgFileName.c_str(), "wb");
        if (!f)
          throw Plus4Emu::Exception("error opening PRG file");
        bool    rawMode = config["rawPRGMode"];
        int     compressionLevel = config["prgCompressionLevel"];
        unsigned int  prgStartAddress = (rawMode ? 0x1400U : 0x1001U);
        if (compressionLevel > 0) {
          std::vector< unsigned char >  compressInBuf;
          std::vector< unsigned char >  compressOutBuf;
          Plus4FLIConv::PRGCompressor   compress_(compressOutBuf);
          Plus4FLIConv::PRGCompressor::CompressionParameters  compressCfg;
          compress_.getCompressionParameters(compressCfg);
          compressCfg.setCompressionLevel(compressionLevel);
          compress_.setCompressionParameters(compressCfg);
          compress_.setProgressMessageCallback(&progressMessageCallback,
                                               (void *) this);
          compress_.setProgressPercentageCallback(&progressPercentageCallback,
                                                  (void *) this);
          for (unsigned int i = prgStartAddress - 0x0FFFU;
               i < (prgEndAddress - 0x0FFFU);
               i++) {
            compressInBuf.push_back(prgData[i]);
          }
          if (!rawMode) {
            compress_.addDecompressCode(false);
            compress_.addDecompressEndCode(-1L, false, true, false, false);
            compress_.addZeroPageUpdate(prgEndAddress, false);
          }
          else {
            compress_.addDecompressEndCode(-2L, false, false, false, false);
          }
          setBusyFlag(true);
          if (!compress_.compressData(compressInBuf, prgStartAddress,
                                      true, true)) {
            compressionLevel = 0;
          }
          setBusyFlag(false);
          if (compressionLevel > 0) {
            prgStartAddress = 0x1001U;
            if (rawMode)
              prgStartAddress = compress_.getCompressedDataStartAddress();
            if (std::fputc(int(prgStartAddress & 0xFFU), f) == EOF ||
                std::fputc(int(prgStartAddress >> 8), f) == EOF) {
              throw Plus4Emu::Exception("error writing PRG file");
            }
            for (size_t i = 0; i < compressOutBuf.size(); i++) {
              if (std::fputc(int(compressOutBuf[i]), f) == EOF)
                throw Plus4Emu::Exception("error writing PRG file");
            }
          }
        }
        if (compressionLevel < 1) {
          prgStartAddress = (rawMode ? 0x1400U : 0x1001U);
          if (std::fputc(int(prgStartAddress & 0xFFU), f) == EOF ||
              std::fputc(int(prgStartAddress >> 8), f) == EOF) {
            throw Plus4Emu::Exception("error writing PRG file");
          }
          for (unsigned int i = prgStartAddress - 0x0FFFU;
               i < (prgEndAddress - 0x0FFFU);
               i++) {
            if (std::fputc(int(prgData[i]), f) == EOF)
              throw Plus4Emu::Exception("error writing PRG file");
          }
        }
        if (std::fflush(f) != 0)
          throw Plus4Emu::Exception("error writing PRG file");
        std::fclose(f);
        f = (std::FILE *) 0;
        fileNotSavedFlag = false;
      }
    }
    catch (std::exception& e) {
      if (f)
        std::fclose(f);
      setBusyFlag(false);
      errorMessage(e.what());
    }
  }
}

void Plus4FLIConvGUI::setBusyFlag(bool newState)
{
  if (newState != busyFlag) {
    busyFlag = newState;
    stopFlag = false;
    progressMessageCallback((void *) this, "");
    if (busyFlag) {
      openImageButton->deactivate();
      savePRGButton->deactivate();
      stopButton->activate();
    }
    else {
      openImageButton->activate();
      savePRGButton->activate();
      stopButton->deactivate();
    }
    emulatorWindow->redraw();
  }
}

void Plus4FLIConvGUI::fltkCheckCallback(void *userData)
{
  Plus4FLIConvGUI&  this_ = *(reinterpret_cast<Plus4FLIConvGUI *>(userData));
  try {
    if (this_.display->checkEvents())
      this_.emulatorWindow->redraw();
  }
  catch (...) {
  }
}

void Plus4FLIConvGUI::progressMessageCallback(void *userData, const char *msg)
{
  Plus4FLIConvGUI&  this_ = *(reinterpret_cast<Plus4FLIConvGUI *>(userData));
  if (msg == (char *) 0 || msg[0] == '\0') {
    msg = "";
    this_.progressDisplay->label(msg);
  }
  else
    this_.progressDisplay->copy_label(msg);
  if (msg[0] == '\0')
    this_.progressDisplay->value(0.0f);
}

bool Plus4FLIConvGUI::progressPercentageCallback(void *userData, int n)
{
  Plus4FLIConvGUI&  this_ = *(reinterpret_cast<Plus4FLIConvGUI *>(userData));
  this_.progressDisplay->value(float(n));
  this_.updateDisplay(0.0);
  return !(this_.stopFlag);
}

void Plus4FLIConvGUI::pixelStoreCallback(void *userData, int xc, int yc,
                                         float y, float u, float v)
{
  Plus4FLIConvGUI&  this_ = *(reinterpret_cast<Plus4FLIConvGUI *>(userData));
  float   r = (v / 0.877f) + y;
  float   b = (u / 0.492f) + y;
  float   g = (y - ((r * 0.299f) + (b * 0.114f))) / 0.587f;
  int     ri = int((r > 0.0f ? (r < 1.0f ? r : 1.0f) : 0.0f) * 255.0f + 0.5f);
  int     gi = int((g > 0.0f ? (g < 1.0f ? g : 1.0f) : 0.0f) * 255.0f + 0.5f);
  int     bi = int((b > 0.0f ? (b < 1.0f ? b : 1.0f) : 0.0f) * 255.0f + 0.5f);
  int     offs = ((yc * 768) + xc) * 3;
  this_.imageFileData[offs] = (unsigned char) ri;
  this_.imageFileData[offs + 1] = (unsigned char) gi;
  this_.imageFileData[offs + 2] = (unsigned char) bi;
}

void Plus4FLIConvGUI::run()
{
  aboutWindowText->value(
      "p4fliconv: high resolution interlaced FLI converter utility\n"
      "Copyright (C) 2007-2008 by Istvan Varga; plus4emu is copyright\n"
      "(C) 2003-2008 by Istvan Varga <istvanv@users.sourceforge.net>\n"
      "\n"
      "This program is free software; you can redistribute it and/or\n"
      "modify it under the terms of the GNU General Public License as\n"
      "published by the Free Software Foundation; either version 2 of\n"
      "the License, or (at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be\n"
      "useful, but WITHOUT ANY WARRANTY; without even the implied\n"
      "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
      "PURPOSE.  See the GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public\n"
      "License along with this program; if not, write to the Free\n"
      "Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,\n"
      "MA  02111-1307  USA\n"
      "\n"
      "The Plus/4 program files generated by this utility are not\n"
      "covered by the GNU General Public License, and can be used,\n"
      "modified, and distributed without any restrictions.");
  try {
    // load configuration
    Plus4Emu::File  f("p4fligui.dat", true);
    guiConfig.registerChunkType(f);
    f.processAllChunks();
  }
  catch (...) {
  }
  try {
    // initialize Plus/4 emulation
    Plus4Emu::ConfigurationDB plus4Config;
    std::string romFileNames[8];
    int     romFileOffsets[8];
    bool    ntscMode = false;
    for (int i = 0; i < 8; i++) {
      char    tmpBuf[64];
      romFileNames[i] = "";
      romFileOffsets[i] = 0;
      char    *cvName = &(tmpBuf[0]);
      std::sprintf(cvName, "memory.rom.%02X.file", (unsigned int) i);
      plus4Config.createKey(cvName, romFileNames[i]);
      plus4Config[cvName].setStripString(true);
      std::sprintf(cvName, "memory.rom.%02X.offset", (unsigned int) i);
      plus4Config.createKey(cvName, romFileOffsets[i]);
    }
    plus4Config.createKey("display.ntscMode", ntscMode);
    try {
      Plus4Emu::File  f("plus4cfg.dat", true);
      plus4Config.registerChunkType(f);
      f.processAllChunks();
    }
    catch (...) {
    }
    ted->setRAMSize(64);
    for (int i = 0; i < 8; i++) {
      if (romFileNames[i] != "") {
        std::FILE *f = std::fopen(romFileNames[i].c_str(), "rb");
        if (f) {
          std::fseek(f, long(romFileOffsets[i]), SEEK_SET);
          for (int j = 0; j < 64; j++) {
            uint8_t tmpBuf[256];
            try {
              std::fread(&(tmpBuf[0]), sizeof(uint8_t), 256, f);
              ted->loadROM(i >> 1, (((i & 1) << 6) | j) << 8, 256,
                           &(tmpBuf[0]));
            }
            catch (...) {
            }
          }
          std::fclose(f);
        }
      }
    }
    Plus4Emu::VideoDisplay::DisplayParameters dp;
    dp.displayQuality = 3;
    dp.bufferingMode = 0;
    dp.ntscMode = ntscMode;
    dp.indexToYUVFunc = &Plus4::TED7360::convertPixelToYUV;
    dp.lineShade = 1.0f;
    dp.blendScale = 1.0f;
    dp.motionBlur = 0.333f;
    display->setDisplayParameters(dp);
  }
  catch (...) {
  }
  ted->reset(true);
  emulatorWindow->resize(6, 35, 768, 576);
  mainWindow->show();
  emulatorWindow->show();
  do {
    updateDisplay();
  } while (mainWindow->shown());
  try {
    // save configuration
    Plus4Emu::File  f;
    guiConfig.saveState(f);
    f.writeFile("p4fligui.dat", true);
  }
  catch (...) {
  }
}

