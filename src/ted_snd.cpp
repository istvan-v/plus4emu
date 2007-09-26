
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
      0,     5,    13,    21,    29,    37,    45,    53,
     60,    60,    60,    60,    60,    60,    60,    60
};

static const int16_t  tedSoundDistortionTable[241] = {
      0,    52,   108,   165,   222,   280,   339,   398,
    457,   516,   575,   635,   695,   755,   815,   875,
    935,   996,  1056,  1117,  1177,  1238,  1299,  1360,
   1421,  1482,  1543,  1604,  1665,  1726,  1787,  1849,
   1910,  1972,  2033,  2095,  2156,  2218,  2280,  2341,
   2403,  2465,  2527,  2589,  2651,  2713,  2775,  2837,
   2899,  2962,  3024,  3086,  3149,  3211,  3274,  3336,
   3399,  3461,  3524,  3587,  3650,  3713,  3776,  3839,
   3902,  3965,  4028,  4091,  4155,  4218,  4281,  4345,
   4408,  4472,  4536,  4600,  4663,  4727,  4791,  4855,
   4919,  4983,  5047,  5112,  5176,  5240,  5305,  5369,
   5434,  5499,  5563,  5628,  5693,  5758,  5823,  5888,
   5953,  6018,  6084,  6149,  6215,  6280,  6346,  6411,
   6477,  6543,  6609,  6675,  6741,  6807,  6873,  6940,
   7006,  7073,  7139,  7206,  7273,  7339,  7406,  7473,
   7540,  7608,  7675,  7742,  7810,  7877,  7945,  8013,
   8080,  8148,  8216,  8284,  8352,  8421,  8489,  8558,
   8626,  8695,  8763,  8832,  8901,  8970,  9039,  9109,
   9178,  9247,  9317,  9387,  9456,  9526,  9596,  9666,
   9736,  9806,  9877,  9947, 10018, 10089, 10159, 10230,
  10301, 10372, 10444, 10515, 10586, 10658, 10729, 10801,
  10873, 10945, 11017, 11090, 11162, 11234, 11307, 11380,
  11452, 11525, 11598, 11672, 11745, 11818, 11892, 11965,
  12039, 12113, 12187, 12261, 12336, 12410, 12484, 12559,
  12634, 12709, 12784, 12859, 12934, 13010, 13085, 13161,
  13237, 13313, 13389, 13465, 13541, 13618, 13694, 13771,
  13848, 13925, 14002, 14080, 14157, 14235, 14312, 14390,
  14468, 14546, 14625, 14703, 14782, 14860, 14939, 15018,
  15097, 15177, 15256, 15336, 15415, 15495, 15575, 15655,
  15736, 15816, 15897, 15978, 16059, 16140, 16221, 16302,
  16384
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
          soundChannel1Decay = 120000U;
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
          soundChannel2Decay = 120000U;
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
    int16_t tmp = tedSoundDistortionTable[prvSoundOutput + soundOutput];
    prvSoundOutput = soundOutput;
    // send sound output signal (sample rate = 221 kHz)
    playSample(tmp);
  }

}       // namespace Plus4

