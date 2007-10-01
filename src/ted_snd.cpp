
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
#include "cpu.hpp"
#include "ted.hpp"

static const uint8_t  tedSoundVolumeTable[16] = {
      0,     6,    16,    26,    36,    46,    56,    66,
     75,    75,    75,    75,    75,    75,    75,    75
};

static const int16_t  tedSoundDistortionTable[301] = {
      0,    44,    89,   136,   183,   231,   278,   326,
    374,   422,   471,   519,   568,   616,   665,   713,
    762,   811,   860,   909,   958,  1007,  1056,  1105,
   1154,  1203,  1252,  1301,  1351,  1400,  1449,  1499,
   1548,  1597,  1647,  1696,  1745,  1795,  1844,  1894,
   1943,  1993,  2042,  2092,  2142,  2191,  2241,  2290,
   2340,  2390,  2440,  2489,  2539,  2589,  2639,  2689,
   2738,  2788,  2838,  2888,  2938,  2988,  3038,  3088,
   3138,  3189,  3239,  3289,  3339,  3389,  3439,  3490,
   3540,  3590,  3641,  3691,  3742,  3792,  3842,  3893,
   3944,  3994,  4045,  4095,  4146,  4197,  4247,  4298,
   4349,  4400,  4451,  4502,  4553,  4604,  4655,  4706,
   4757,  4808,  4859,  4910,  4961,  5013,  5064,  5115,
   5167,  5218,  5270,  5321,  5373,  5424,  5476,  5528,
   5579,  5631,  5683,  5735,  5787,  5838,  5890,  5942,
   5994,  6047,  6099,  6151,  6203,  6255,  6308,  6360,
   6413,  6465,  6517,  6570,  6623,  6675,  6728,  6781,
   6834,  6886,  6939,  6992,  7045,  7098,  7151,  7204,
   7258,  7311,  7364,  7418,  7471,  7524,  7578,  7632,
   7685,  7739,  7793,  7846,  7900,  7954,  8008,  8062,
   8116,  8170,  8224,  8279,  8333,  8387,  8442,  8496,
   8551,  8605,  8660,  8715,  8769,  8824,  8879,  8934,
   8989,  9044,  9099,  9154,  9210,  9265,  9320,  9376,
   9431,  9487,  9542,  9598,  9654,  9710,  9766,  9822,
   9878,  9934,  9990, 10046, 10102, 10159, 10215, 10272,
  10328, 10385, 10442, 10498, 10555, 10612, 10669, 10726,
  10783, 10840, 10898, 10955, 11012, 11070, 11128, 11185,
  11243, 11301, 11358, 11416, 11474, 11532, 11591, 11649,
  11707, 11766, 11824, 11883, 11941, 12000, 12059, 12117,
  12176, 12235, 12294, 12354, 12413, 12472, 12532, 12591,
  12651, 12710, 12770, 12830, 12890, 12950, 13010, 13070,
  13130, 13190, 13251, 13311, 13372, 13432, 13493, 13554,
  13615, 13676, 13737, 13798, 13859, 13920, 13982, 14043,
  14105, 14167, 14228, 14290, 14352, 14414, 14476, 14539,
  14601, 14663, 14726, 14788, 14851, 14914, 14977, 15040,
  15103, 15166, 15229, 15293, 15356, 15419, 15483, 15547,
  15611, 15675, 15739, 15803, 15867, 15931, 15996, 16060,
  16125, 16189, 16254, 16319, 16384
};

namespace Plus4 {

  void TED7360::write_register_FF0E(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0E] = value;
    int     tmp = int(value) | (int(ted.tedRegisters[0x12] & 0x03) << 8);
    ted.soundChannel1Reload = uint16_t((((tmp + 1) ^ 0x03FF) & 0x03FF) + 1);
  }

  void TED7360::write_register_FF0F(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x0F] = value;
    int     tmp = int(value) | (int(ted.tedRegisters[0x10] & 0x03) << 8);
    ted.soundChannel2Reload = uint16_t((((tmp + 1) ^ 0x03FF) & 0x03FF) + 1);
  }

  void TED7360::write_register_FF10(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x10] = value;
    int     tmp = int(ted.tedRegisters[0x0F]) | (int(value & 0x03) << 8);
    ted.soundChannel2Reload = uint16_t((((tmp + 1) ^ 0x03FF) & 0x03FF) + 1);
  }

  void TED7360::write_register_FF11(void *userData,
                                    uint16_t addr, uint8_t value)
  {
    (void) addr;
    TED7360&  ted = *(reinterpret_cast<TED7360 *>(userData));
    ted.dataBusState = value;
    ted.tedRegisters[0x11] = value;
    ted.soundVolume = tedSoundVolumeTable[value & 0x0F];
    if (value & 0x80) {
      ted.soundChannel1State = uint8_t(1);
      ted.soundChannel2State = uint8_t(1);
    }
    ted.updateSoundChannel1Output();
    ted.updateSoundChannel2Output();
  }

  void TED7360::calculateSoundOutput()
  {
    if (tedRegisters[0x11] & uint8_t(0x80)) {
      // DAC mode
      soundChannel1Cnt = soundChannel1Reload;
      soundChannel2Cnt = soundChannel2Reload;
      soundChannel1Decay--;
      if (!soundChannel1Decay)
        prvSoundChannel1Overflow = true;
      soundChannel2Decay--;
      if (!soundChannel2Decay)
        prvSoundChannel2Overflow = true;
      soundChannel2NoiseState = uint8_t(0xFF);
      soundChannel2NoiseOutput = uint8_t(1);
      updateSoundChannel1Output();
      updateSoundChannel2Output();
    }
    else {
      // channel 1
      soundChannel1Cnt--;
      bool    soundChannel1Overflow = !(soundChannel1Cnt);
      if (soundChannel1Overflow) {
        soundChannel1Cnt = soundChannel1Reload;
        if (!prvSoundChannel1Overflow) {
          soundChannel1Decay = 131072U;
          soundChannel1State = (~soundChannel1State) & uint8_t(1);
          updateSoundChannel1Output();
        }
      }
      prvSoundChannel1Overflow = soundChannel1Overflow;
      soundChannel1Decay--;
      if (!soundChannel1Decay) {
        soundChannel1State = uint8_t(1);
        updateSoundChannel1Output();
      }
      // channel 2
      soundChannel2Cnt--;
      bool    soundChannel2Overflow = !(soundChannel2Cnt);
      if (soundChannel2Overflow) {
        soundChannel2Cnt = soundChannel2Reload;
        if (!prvSoundChannel2Overflow) {
          soundChannel2Decay = 131072U;
          soundChannel2State = (~soundChannel2State) & uint8_t(1);
          // channel 2 noise, 8 bit polycnt (10110011)
          uint8_t tmp = soundChannel2NoiseState & uint8_t(0xB3);
          tmp = tmp ^ (tmp >> 1);
          tmp = tmp ^ (tmp >> 2);
          tmp = tmp ^ (tmp >> 4);
          soundChannel2NoiseOutput ^= tmp;
          soundChannel2NoiseOutput &= uint8_t(1);
          soundChannel2NoiseState <<= 1;
          soundChannel2NoiseState |= soundChannel2NoiseOutput;
          updateSoundChannel2Output();
        }
      }
      prvSoundChannel2Overflow = soundChannel2Overflow;
      soundChannel2Decay--;
      if (!soundChannel2Decay) {
        soundChannel2State = uint8_t(1);
        updateSoundChannel2Output();
      }
    }
    // mix sound outputs
    uint8_t soundOutput = soundChannel1Output + soundChannel2Output;
    int16_t tmp =
        tedSoundDistortionTable[size_t(prvSoundOutput) + size_t(soundOutput)];
    prvSoundOutput = soundOutput;
    // send sound output signal (sample rate = 221 kHz)
    playSample(tmp);
  }

}       // namespace Plus4

