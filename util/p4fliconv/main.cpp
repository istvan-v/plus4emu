
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
#include "hiresfli.hpp"
#include "hiresnofli.hpp"
#include "hrbmifli.hpp"
#include "interlace7.hpp"
#include "mcbmifli.hpp"
#include "mcchar.hpp"
#include "mcfli.hpp"
#include "mcifli.hpp"
#include "mcnofli.hpp"
#include "guicolor.hpp"
#include "imgwrite.hpp"

#include <vector>
#include <map>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

int main(int argc, char **argv)
{
  bool    printUsageFlag = false;
  bool    helpFlag = false;
  try {
    Fl::lock();
    fl_register_images();
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    Plus4Emu::setGUIColorScheme(1);
#else
    Plus4Emu::setGUIColorScheme(0);
#endif
    Plus4FLIConv::FLIConfiguration  config;
    config.resetDefaultSettings();
    std::string infileName = "";
    std::string outfileName = "";
    {
      std::vector< std::string >    args;
      std::map< std::string, std::vector< std::string > >   optionTable;
      optionTable["-mode"].push_back("i:conversionType");
      optionTable["-ymin"].push_back("f:yMin");
      optionTable["-ymax"].push_back("f:yMax");
      optionTable["-scale"].push_back("f:scaleX");
      optionTable["-scale"].push_back("f:scaleY");
      optionTable["-offset"].push_back("f:offsetX");
      optionTable["-offset"].push_back("f:offsetY");
      optionTable["-saturation"].push_back("f:saturationMult");
      optionTable["-saturation"].push_back("f:saturationPow");
      optionTable["-gamma"].push_back("f:gammaCorrection");
      optionTable["-gamma"].push_back("f:monitorGamma");
      optionTable["-dither"].push_back("i:ditherMode");
      optionTable["-dither"].push_back("f:ditherLimit");
      optionTable["-dither"].push_back("f:ditherDiffusion");
      optionTable["-pal"].push_back("b:enablePAL");
      optionTable["-xshift"].push_back("i:xShift0");
      optionTable["-xshift"].push_back("i:xShift1");
      optionTable["-border"].push_back("i:borderColor");
      optionTable["-size"].push_back("i:verticalSize");
      optionTable["-y1bit"].push_back("b:luminance1BitMode");
      optionTable["-no_li"].push_back("b:noLuminanceInterlace");
      optionTable["-ci"].push_back("i:colorInterlaceMode");
      optionTable["-searchmode"].push_back("i:luminanceSearchMode");
      optionTable["-searchmode"].push_back("f:luminanceSearchModeParam");
      optionTable["-mcchromaerr"].push_back("f:mcColorErrorScale");
      optionTable["-mcquality"].push_back("i:multiColorQuality");
      optionTable["-c64color0"].push_back("i:c64Color0");
      optionTable["-c64color1"].push_back("i:c64Color1");
      optionTable["-c64color2"].push_back("i:c64Color2");
      optionTable["-c64color3"].push_back("i:c64Color3");
      optionTable["-c64color4"].push_back("i:c64Color4");
      optionTable["-c64color5"].push_back("i:c64Color5");
      optionTable["-c64color6"].push_back("i:c64Color6");
      optionTable["-c64color7"].push_back("i:c64Color7");
      optionTable["-c64color8"].push_back("i:c64Color8");
      optionTable["-c64color9"].push_back("i:c64Color9");
      optionTable["-c64color10"].push_back("i:c64Color10");
      optionTable["-c64color11"].push_back("i:c64Color11");
      optionTable["-c64color12"].push_back("i:c64Color12");
      optionTable["-c64color13"].push_back("i:c64Color13");
      optionTable["-c64color14"].push_back("i:c64Color14");
      optionTable["-c64color15"].push_back("i:c64Color15");
      optionTable["-outfmt"].push_back("i:outputFileFormat");
      optionTable["-compress"].push_back("i:prgCompressionLevel");
      bool    endOfOptions = false;
      size_t  skipCnt = 0;
      for (int i = 1; i < argc; i++) {
        const char  *s = argv[i];
        if (s == (char *) 0 || s[0] == '\0')
          continue;
        if (skipCnt > 0) {
          args.push_back(s);
          skipCnt--;
          continue;
        }
#ifdef __APPLE__
        if (std::strncmp(s, "-psn_", 5) == 0)
          continue;
#endif
        if (s[0] != '-' || endOfOptions) {
          if (infileName == "") {
            infileName = s;
          }
          else if (outfileName == "") {
            outfileName = s;
          }
          else {
            printUsageFlag = true;
            throw Plus4Emu::Exception("too many file name arguments");
          }
          continue;
        }
        if (std::strcmp(s, "--") == 0) {
          endOfOptions = true;
          continue;
        }
        if (std::strcmp(s, "-h") == 0 ||
            std::strcmp(s, "-help") == 0 ||
            std::strcmp(s, "--help") == 0) {
          helpFlag = true;
          throw Plus4Emu::Exception("");
        }
        if (optionTable.find(s) == optionTable.end()) {
          printUsageFlag = true;
          throw Plus4Emu::Exception("invalid command line option");
        }
        args.push_back(s);
        skipCnt = optionTable[s].size();
      }
      if (skipCnt > 0) {
        printUsageFlag = true;
        throw Plus4Emu::Exception("missing argument(s) "
                                  "for command line option");
      }
      if (infileName != "" && outfileName == "") {
        printUsageFlag = true;
        throw Plus4Emu::Exception("missing file name");
      }
      // if there are no file name arguments, run in GUI mode,
      // but still use any command line options specified
      if (infileName == "") {
        try {
          // load configuration
          Plus4Emu::File  f("p4flicfg.dat", true);
          config.registerChunkType(f);
          f.processAllChunks();
        }
        catch (...) {
        }
      }
      for (size_t i = 0; i < args.size(); i++) {
        std::vector< std::string >&   v = optionTable[args[i]];
        for (size_t j = 0; j < v.size(); j++) {
          char        optionType = v[j][0];
          const char  *optionName = v[j].c_str() + 2;
          i++;
          if (optionType == 'b')
            config[optionName] = bool(std::atoi(args[i].c_str()));
          else if (optionType == 'i')
            config[optionName] = int(std::atoi(args[i].c_str()));
          else if (optionType == 'f')
            config[optionName] = double(std::atof(args[i].c_str()));
        }
      }
    }
    if (infileName != "") {
      // run in command line mode
      Plus4FLIConv::FLIConverter  *fliConv = (Plus4FLIConv::FLIConverter *) 0;
      Plus4FLIConv::PRGData       prgData;
      unsigned int  prgEndAddr = 0x1003U;
      int     convType = config["conversionType"];
      try {
        if (convType == 0)
          fliConv = new Plus4FLIConv::P4FLI_Interlace7();
        else if (convType == 1)
          fliConv = new Plus4FLIConv::P4FLI_MultiColor();
        else if (convType == 2)
          fliConv = new Plus4FLIConv::P4FLI_HiResBitmapInterlace();
        else if (convType == 3)
          fliConv = new Plus4FLIConv::P4FLI_MultiColorBitmapInterlace();
        else if (convType == 4)
          fliConv = new Plus4FLIConv::P4FLI_HiResNoInterlace();
        else if (convType == 5)
          fliConv = new Plus4FLIConv::P4FLI_MultiColorNoInterlace();
        else if (convType == 6)
          fliConv = new Plus4FLIConv::P4FLI_HiResNoFLI();
        else if (convType == 7)
          fliConv = new Plus4FLIConv::P4FLI_MultiColorNoFLI();
        else if (convType == 8)
          fliConv = new Plus4FLIConv::P4FLI_MultiColorChar();
        else
          throw Plus4Emu::Exception("invalid conversion type");
        Plus4FLIConv::YUVImageConverter imgConv;
        imgConv.setXYScaleAndOffset(float(double(config["scaleX"])),
                                    float(double(config["scaleY"])),
                                    float(double(config["offsetX"])),
                                    float(double(config["offsetY"])));
        imgConv.setGammaCorrection(
            float(double(config["gammaCorrection"])),
            float(double(config["monitorGamma"]) * 0.625));
        imgConv.setLuminanceRange(float(double(config["yMin"])),
                                  float(double(config["yMax"])));
        imgConv.setColorSaturation(float(double(config["saturationMult"])),
                                   float(double(config["saturationPow"])));
        float   borderY = 0.0f;
        float   borderU = 0.0f;
        float   borderV = 0.0f;
        Plus4FLIConv::FLIConverter::convertPlus4Color(
            int(config["borderColor"]), borderY, borderU, borderV, 1.0);
        imgConv.setBorderColor(borderY, borderU, borderV);
        imgConv.setC64Color(0, int(config["c64Color0"]));
        imgConv.setC64Color(1, int(config["c64Color1"]));
        imgConv.setC64Color(2, int(config["c64Color2"]));
        imgConv.setC64Color(3, int(config["c64Color3"]));
        imgConv.setC64Color(4, int(config["c64Color4"]));
        imgConv.setC64Color(5, int(config["c64Color5"]));
        imgConv.setC64Color(6, int(config["c64Color6"]));
        imgConv.setC64Color(7, int(config["c64Color7"]));
        imgConv.setC64Color(8, int(config["c64Color8"]));
        imgConv.setC64Color(9, int(config["c64Color9"]));
        imgConv.setC64Color(10, int(config["c64Color10"]));
        imgConv.setC64Color(11, int(config["c64Color11"]));
        imgConv.setC64Color(12, int(config["c64Color12"]));
        imgConv.setC64Color(13, int(config["c64Color13"]));
        imgConv.setC64Color(14, int(config["c64Color14"]));
        imgConv.setC64Color(15, int(config["c64Color15"]));
        prgData.clear();
        prgData.lineBlankFXEnabled() = 0x01;    // TODO: make this configurable
        prgData.borderColor() =
            (unsigned char) ((int(config["borderColor"]) & 0x7F) | 0x80);
        fliConv->processImage(prgData, prgEndAddr,
                              infileName.c_str(), imgConv, config);
        delete fliConv;
        fliConv = (Plus4FLIConv::FLIConverter *) 0;
      }
      catch (...) {
        if (fliConv)
          delete fliConv;
        throw;
      }
      config.clearConfigurationChangeFlag();
      Plus4FLIConv::writeConvertedImageFile(outfileName.c_str(), prgData,
                                            prgEndAddr, convType,
                                            int(config["outputFileFormat"]),
                                            int(config["prgCompressionLevel"]));
      return 0;
    }
    config.clearConfigurationChangeFlag();
    Plus4FLIConvGUI *gui = new Plus4FLIConvGUI(config);
    gui->run();
    delete gui;
    try {
      // save configuration
      Plus4Emu::File  f;
      config.saveState(f);
      f.writeFile("p4flicfg.dat", true);
    }
    catch (...) {
    }
  }
  catch (std::exception& e) {
    if (printUsageFlag || helpFlag) {
      std::fprintf(stderr, "Usage: %s [OPTIONS...] infile.jpg outfile.prg\n",
                           argv[0]);
      std::fprintf(stderr, "Options:\n");
      std::fprintf(stderr, "    -mode <N>           (0 to 8, default: 0)\n");
      std::fprintf(stderr, "        select video mode (0: interlaced hires "
                           "FLI, 1: interlaced\n        multicolor FLI, 2: "
                           "hires FLI, bitmap interlace only,\n        3: "
                           "multicolor FLI, bitmap interlace only, 4: hires "
                           "FLI,\n        5: multicolor FLI, 6: hires 320x200 "
                           "(no FLI), 7: multicolor\n        160x200 (no "
                           "FLI), 8: Logo Editor 2.0 multicolor (128x64))\n");
      std::fprintf(stderr, "    -ymin <MIN>         (default: -0.02)\n");
      std::fprintf(stderr, "    -ymax <MAX>         (default: 1.03)\n");
      std::fprintf(stderr, "        scale RGB input range from 0..1 to "
                           "MIN..MAX\n");
      std::fprintf(stderr, "    -scale <X> <Y>      (defaults: 1.0, 1.0)\n");
      std::fprintf(stderr, "        scale image size\n");
      std::fprintf(stderr, "    -offset <X> <Y>     (defaults: 0.0, 0.0)\n");
      std::fprintf(stderr, "        set image position offset\n");
      std::fprintf(stderr, "    -saturation <M> <P> (defaults: 1.0, 0.9)\n");
      std::fprintf(stderr, "        color saturation scale and power\n");
      std::fprintf(stderr, "    -gamma <G> <M>      (defaults: 1.0, 2.2)\n");
      std::fprintf(stderr, "        set gamma correction (G) and assumed "
                           "monitor gamma (M)\n");
      std::fprintf(stderr, "    -dither <M> <L> <S> (defaults: 1, 0.25, "
                           "0.95)\n");
      std::fprintf(stderr, "        dither mode (0: ordered (Bayer), 1: "
                           "ordered (randomized),\n        2: "
                           "Floyd-Steinberg, 3: Jarvis, 4: Stucki, 5: "
                           "Sierra2), limit,\n        and error diffusion "
                           "factor\n");
      std::fprintf(stderr, "    -pal <N>            (0 or 1, default: 1)\n");
      std::fprintf(stderr, "        assume PAL chrominance filtering "
                           "if set to 1\n");
      std::fprintf(stderr, "    -xshift <S0> <S1>   "
                           "(-2 to 7, defaults: -1, -1)\n");
      std::fprintf(stderr, "        set horizontal shift for each field (-2 is "
                           "random, -1 finds\n        optimal values)\n");
      std::fprintf(stderr, "    -border <N>         (0 to 255, default: 0)\n");
      std::fprintf(stderr, "        set border color\n");
      std::fprintf(stderr, "    -size <N>           "
                           "(128 to 496, default: 464)\n");
      std::fprintf(stderr, "        set vertical resolution (< 256 implies "
                           "no interlace)\n");
      std::fprintf(stderr, "    -y1bit <N>          (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        use 1 bit (black and white) luminance\n");
      std::fprintf(stderr, "    -no_li <N>          (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        do not interlace luminance attributes\n");
      std::fprintf(stderr, "    -ci <N>             (0 to 2, default: 1)\n");
      std::fprintf(stderr, "        color interlace mode (0: none, 1: hue "
                           "only, 2: hue and\n        saturation)\n");
      std::fprintf(stderr, "    -searchmode <M> <P> (defaults: 2, 4.0)\n");
      std::fprintf(stderr, "        select luminance search algorithm (0 to 5),"
                           " and parameter for\n        modes 2, 4, and 5\n");
      std::fprintf(stderr, "    -mcchromaerr <N>    "
                           "(0.05 to 1.0, default: 0.5)\n");
      std::fprintf(stderr, "        scale factor applied to squared "
                           "chrominance error in multicolor\n        modes\n");
      std::fprintf(stderr, "    -mcquality <N>      (1 to 30, default: 6)\n");
      std::fprintf(stderr, "        multicolor conversion quality\n");
      std::fprintf(stderr, "    -c64color<N> <C>    "
                           "(N: 0 to 15, C: 0 to 255)\n");
      std::fprintf(stderr, "        map C64 color N to Plus/4 color C when "
                           "reading C64 image files\n");
      std::fprintf(stderr, "    -outfmt <N>         (0 to 3, default: 0)\n");
      std::fprintf(stderr, "        output file format, 0: PRG with viewer, "
                           "1: raw PRG,\n        2: PixelShop P4S, 3: FED "
                           "160x200 multicolor FLI\n");
      std::fprintf(stderr, "    -compress <N>       (0 to 9, default: 0)\n");
      std::fprintf(stderr, "        compress output file if N is not zero\n");
    }
    if (!helpFlag) {
      const char  *errMsg = e.what();
      if (!errMsg)
        errMsg = "";
      std::fprintf(stderr, " *** p4fliconv error: %s\n", errMsg);
      return -1;
    }
    return 0;
  }
  return 0;
}

