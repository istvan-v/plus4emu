
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

#include "plus4emu.hpp"
#include "cpu.hpp"
#include "ted.hpp"

static const uint8_t  tedSoundVolumeTable[16] = {
      0,     6,    16,    26,    36,    46,    56,    66,
     75,    75,    75,    75,    75,    75,    75,    75
};

static const int16_t  tedSoundDistortionTable[301] = {
      0,     8,    21,    37,    56,    77,    99,   123,
    148,   175,   202,   231,   261,   292,   324,   357,
    391,   425,   461,   497,   534,   572,   610,   649,
    689,   729,   771,   812,   855,   897,   941,   985,
   1030,  1075,  1121,  1167,  1214,  1261,  1309,  1357,
   1406,  1455,  1505,  1555,  1606,  1657,  1709,  1761,
   1813,  1866,  1920,  1974,  2028,  2083,  2138,  2193,
   2249,  2305,  2362,  2419,  2476,  2534,  2592,  2651,
   2710,  2769,  2829,  2889,  2950,  3011,  3072,  3133,
   3195,  3257,  3320,  3383,  3446,  3510,  3574,  3638,
   3703,  3768,  3833,  3899,  3965,  4031,  4097,  4164,
   4232,  4299,  4367,  4435,  4504,  4573,  4642,  4711,
   4781,  4851,  4921,  4992,  5063,  5134,  5206,  5278,
   5350,  5422,  5495,  5568,  5641,  5715,  5789,  5863,
   5938,  6012,  6088,  6163,  6239,  6315,  6391,  6467,
   6544,  6621,  6698,  6776,  6854,  6932,  7011,  7089,
   7168,  7248,  7327,  7407,  7487,  7567,  7648,  7729,
   7810,  7892,  7973,  8055,  8138,  8220,  8303,  8386,
   8469,  8553,  8637,  8721,  8805,  8890,  8975,  9060,
   9146,  9231,  9317,  9403,  9490,  9577,  9664,  9751,
   9838,  9926, 10014, 10103, 10191, 10280, 10369, 10458,
  10548, 10638, 10728, 10818, 10909, 11000, 11091, 11182,
  11274, 11366, 11458, 11550, 11643, 11736, 11829, 11922,
  12016, 12110, 12204, 12298, 12393, 12488, 12583, 12678,
  12774, 12870, 12966, 13063, 13159, 13256, 13353, 13451,
  13548, 13646, 13745, 13843, 13942, 14040, 14140, 14239,
  14339, 14439, 14539, 14639, 14740, 14841, 14942, 15043,
  15145, 15247, 15349, 15451, 15554, 15657, 15760, 15864,
  15967, 16071, 16175, 16280, 16384, 16489, 16594, 16700,
  16805, 16911, 17017, 17124, 17230, 17337, 17444, 17552,
  17659, 17767, 17875, 17984, 18092, 18201, 18310, 18420,
  18529, 18639, 18749, 18860, 18970, 19081, 19192, 19304,
  19415, 19527, 19639, 19752, 19864, 19977, 20090, 20204,
  20317, 20431, 20545, 20660, 20774, 20889, 21004, 21120,
  21235, 21351, 21467, 21584, 21700, 21817, 21934, 22052,
  22169, 22287, 22406, 22524, 22643, 22762, 22881, 23000,
  23120, 23240, 23360, 23480, 23601, 23722, 23843, 23965,
  24087, 24209, 24331, 24453, 24576
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

