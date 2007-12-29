
// interlace7.cpp: simple high resolution interlaced FLI converter utility
// Copyright (C) 2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

static  double  yMin = 0.0;
static  double  yMax = 1.0;
static  double  colorSaturationMult = 1.0;
static  double  colorSaturationPow = 0.5;
static  double  monitorGamma = 1.33;
static  double  ditherLimit = 0.1;
static  int     colorSearchMode = 1;
static  bool    disablePAL = false;
static  bool    disableXShift = false;

static const unsigned char prgHeader[1025] = {
  0x01, 0x10, 0x0C, 0x10, 0x0A, 0x00, 0x9E, 0x20, 0x34, 0x31, 0x31, 0x32,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xD8, 0xA2, 0xFF, 0x9A, 0x20, 0x80,
  0x10, 0xA9, 0x00, 0x8D, 0x15, 0xFF, 0x8D, 0x19, 0xFF, 0xA9, 0xF0, 0x8D,
  0xFC, 0xFF, 0xA9, 0x10, 0x8D, 0xFD, 0xFF, 0xA9, 0x00, 0x8D, 0xFE, 0xFF,
  0xA9, 0x11, 0x8D, 0xFF, 0xFF, 0xA9, 0x36, 0x8D, 0x0B, 0xFF, 0xA9, 0xA3,
  0x8D, 0x0A, 0xFF, 0xA9, 0xFF, 0x8D, 0x09, 0x36, 0x8D, 0x3F, 0xFF, 0x58,
  0x4C, 0x47, 0x10, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0x20, 0x84, 0xFF,
  0x2C, 0x07, 0xFF, 0x70, 0x15, 0xA9, 0x36, 0x8D, 0x42, 0x10, 0x8D, 0xBE,
  0x12, 0x8D, 0x56, 0x13, 0xA2, 0xF9, 0x8E, 0xBE, 0x11, 0xE8, 0x8E, 0x08,
  0x13, 0x60, 0xA9, 0x04, 0x8D, 0x42, 0x10, 0x8D, 0xBE, 0x12, 0x8D, 0x56,
  0x13, 0xA2, 0xE0, 0x8E, 0xBE, 0x11, 0xE8, 0x8E, 0x08, 0x13, 0x60, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0x78, 0x8D, 0x3E, 0xFF, 0x6C, 0xFC, 0xFF, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0x48, 0x8A, 0x48, 0x98, 0x48, 0xA9, 0x38,
  0x8D, 0x06, 0xFF, 0xA9, 0x18, 0x8D, 0x14, 0xFF, 0xA9, 0xC8, 0x8D, 0x12,
  0xFF, 0xA9, 0x02, 0xCD, 0x1D, 0xFF, 0xD0, 0xFB, 0xEA, 0xEA, 0xAD, 0x1E,
  0xFF, 0x29, 0x1C, 0x4A, 0x4A, 0x8D, 0x28, 0x11, 0x10, 0x00, 0xA9, 0xA9,
  0xA9, 0xA9, 0xA9, 0xA9, 0xA5, 0xEA, 0xA0, 0x19, 0xA2, 0x01, 0xCA, 0xD0,
  0xFD, 0xA9, 0xEA, 0xA9, 0xEA, 0xEA, 0xA9, 0x3B, 0x8D, 0x06, 0xFF, 0xA9,
  0x18, 0x8D, 0x14, 0xFF, 0xEA, 0xEA, 0xA9, 0xFF, 0x8D, 0x1F, 0xFF, 0xA9,
  0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xA9, 0x3D, 0x8D, 0x06, 0xFF, 0xA9, 0x60, 0x8D, 0x14, 0xFF,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9,
  0x00, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x3F, 0x8D, 0x06,
  0xFF, 0xA9, 0x68, 0x8D, 0x14, 0xFF, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9,
  0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xA9, 0x39, 0x8D, 0x06, 0xFF, 0xA9, 0x70, 0x8D, 0x14, 0xFF,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9,
  0x00, 0xEA, 0xEA, 0x24, 0xEA, 0xEA, 0x88, 0xD0, 0x85, 0xA9, 0x3B, 0x8D,
  0x06, 0xFF, 0xA9, 0xF9, 0x8D, 0x0B, 0xFF, 0xA9, 0xA2, 0x8D, 0x0A, 0xFF,
  0xA9, 0xFF, 0x8D, 0x09, 0xFF, 0xA9, 0x00, 0x8D, 0xFE, 0xFF, 0xA9, 0x13,
  0x8D, 0xFF, 0xFF, 0xAD, 0x07, 0xFF, 0x09, 0x0C, 0xAE, 0x1D, 0xFF, 0xEC,
  0x1D, 0xFF, 0xF0, 0xFB, 0x8D, 0x07, 0xFF, 0x68, 0xA8, 0x68, 0xAA, 0x68,
  0x40, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0x48, 0x8A, 0x48,
  0x98, 0x48, 0xA9, 0x38, 0x8D, 0x06, 0xFF, 0xA9, 0x78, 0x8D, 0x14, 0xFF,
  0xA9, 0xD0, 0x8D, 0x12, 0xFF, 0xA9, 0x02, 0xCD, 0x1D, 0xFF, 0xD0, 0xFB,
  0xEA, 0xEA, 0xAD, 0x1E, 0xFF, 0x29, 0x1C, 0x4A, 0x4A, 0x8D, 0x28, 0x12,
  0x10, 0x00, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA5, 0xEA, 0xA0, 0x19,
  0xA2, 0x01, 0xCA, 0xD0, 0xFD, 0xA9, 0xEA, 0xA9, 0xEA, 0xEA, 0xA9, 0x3B,
  0x8D, 0x06, 0xFF, 0xA9, 0x78, 0x8D, 0x14, 0xFF, 0xEA, 0xEA, 0xA9, 0xFF,
  0x8D, 0x1F, 0xFF, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x3D, 0x8D, 0x06, 0xFF, 0xA9,
  0x80, 0x8D, 0x14, 0xFF, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0xA9,
  0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xA9, 0x3F, 0x8D, 0x06, 0xFF, 0xA9, 0x88, 0x8D, 0x14, 0xFF, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x39, 0x8D, 0x06, 0xFF, 0xA9,
  0x90, 0x8D, 0x14, 0xFF, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0xA9,
  0x00, 0xA9, 0x00, 0xA9, 0x00, 0xEA, 0xEA, 0x24, 0xEA, 0xEA, 0x88, 0xD0,
  0x85, 0xA9, 0x3B, 0x8D, 0x06, 0xFF, 0xA9, 0x36, 0x8D, 0x0B, 0xFF, 0xA9,
  0xA3, 0x8D, 0x0A, 0xFF, 0xA9, 0xFF, 0x8D, 0x09, 0xFF, 0xA9, 0x00, 0x8D,
  0xFE, 0xFF, 0xA9, 0x11, 0x8D, 0xFF, 0xFF, 0xAD, 0x07, 0xFF, 0x29, 0x48,
  0xAE, 0x1D, 0xFF, 0xEC, 0x1D, 0xFF, 0xF0, 0xFB, 0x8D, 0x07, 0xFF, 0x68,
  0xA8, 0x68, 0xAA, 0x68, 0x40, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0x48, 0x8A, 0x48, 0x98, 0x48, 0xA2, 0x6F, 0xA9, 0xFA, 0xCD, 0x1D,
  0xFF, 0xD0, 0xFB, 0xAD, 0x1E, 0xFF, 0x4A, 0x29, 0x07, 0x8D, 0x18, 0x13,
  0x10, 0x00, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0xA9, 0x24, 0xEA, 0x8E, 0x1E,
  0xFF, 0xA0, 0x05, 0xD0, 0x01, 0xEA, 0xA2, 0x14, 0xCA, 0xD0, 0xFD, 0xEA,
  0xEA, 0x88, 0xD0, 0xF6, 0xA9, 0xAF, 0xA2, 0x00, 0x8E, 0xFE, 0xFF, 0x8D,
  0x1E, 0xFF, 0xCE, 0x09, 0xFF, 0xA9, 0x12, 0x8D, 0xFF, 0xFF, 0xA2, 0x05,
  0xAD, 0x1D, 0xFF, 0xCD, 0x1D, 0xFF, 0xF0, 0xFB, 0xCA, 0xD0, 0xF5, 0xCE,
  0x1D, 0xFF, 0xA9, 0x36, 0x8D, 0x0B, 0xFF, 0xA9, 0xA3, 0x8D, 0x0A, 0xFF,
  0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
  0xEA, 0xEA, 0xEA, 0xEA, 0xEA
};

static const float yTableSrc[9] = {
  2.00f,  2.40f,  2.55f,  2.70f,  2.90f,  3.30f,  3.60f,  4.10f,  4.80f
};

static const float colorPhaseTablePAL[16] = {
    0.0f,    0.0f,  103.0f,  283.0f,   53.0f,  241.0f,  347.0f,  167.0f,
  129.0f,  148.0f,  195.0f,   83.0f,  265.0f,  323.0f,    3.0f,  213.0f
};

struct UVTableEntry {
  int     c0;
  int     c1;
  float   u;
  float   v;
};
static  float   yTable[9];
static  UVTableEntry    uvTable[43];
static  unsigned char   prgData[0x8801];
static  unsigned char   inputImage[640 * 400 * 3];
static  float   resizedImageY[320 * 400];
static  float   resizedImageU[320 * 400];
static  float   resizedImageV[320 * 400];
static  float   prvLineU[320];
static  float   prvLineV[320];
static  float   prvBitmapU[24];
static  float   prvBitmapV[24];
static  int     ditherTable[256];

static void createDitherTable()
{
  for (int i = 0; i < 256; i++) {
    int     j = ((i & 0x0F) << 4) | ((i & 0xF0) >> 4);
    j = ((j & 0x33) << 2) | ((j & 0xCC) >> 2);
    j = ((j & 0x55) << 1) | ((j & 0xAA) >> 1);
    int     x = 0;
    int     y = 0;
    for (int l = 0; l < 8; l += 2) {
      int     k = j & 3;
      j = j >> 2;
      switch (k) {
      case 0:
        break;
      case 1:
        y += (1 << (l >> 1));
        break;
      case 2:
        x += (1 << (l >> 1));
        y += (1 << (l >> 1));
        break;
      case 3:
        x += (1 << (l >> 1));
        break;
      }
    }
    ditherTable[(y << 4) | x] = i;
  }
}

static void colorToUV(int c, float& u, float& v)
{
  u = 0.0f;
  v = 0.0f;
  c = c & 15;
  if (c < 2)
    return;
  u = float(std::cos(colorPhaseTablePAL[c] * 3.14159265 / 180.0) * 0.19);
  v = float(std::sin(colorPhaseTablePAL[c] * 3.14159265 / 180.0) * 0.19);
}

static void createUVTables()
{
  for (int i = 0; i < 43; i++) {
    int     c0 = i + 1;
    int     c1 = i + 1;
    float   u0 = 0.0f;
    float   v0 = 0.0f;
    float   u1 = 0.0f;
    float   v1 = 0.0f;
    if (i >= 15) {
      c0 = ((i - 15) % 14) + 2;
      c1 = 1;
      colorToUV(c0, u0, v0);
      if (i < 29) {
        float   bestDiff = 1000.0f;
        for (int j = 2; j < 14; j++) {
          colorToUV(j, u1, v1);
          float   d = ((u1 - u0) * (u1 - u0)) + ((v1 - v0) * (v1 - v0));
          if (d < bestDiff && j != c0) {
            c1 = j;
            bestDiff = d;
          }
        }
      }
    }
    uvTable[i].c0 = c0;
    uvTable[i].c1 = c1;
    colorToUV(c0, u0, v0);
    colorToUV(c1, u1, v1);
    uvTable[i].u = (u0 + u1) * 0.5f;
    uvTable[i].v = (v0 + v1) * 0.5f;
  }
}

static void convert8x2Pixels(int x, int y,
                             int ditherXORValue0, int ditherXORValue1)
{
  float   yTbl[16];
  float   *y0Tbl = &(yTbl[0]);
  float   *y1Tbl = &(yTbl[8]);
  float   uTbl_[24];
  float   *uTbl = &(uTbl_[8]);
  float   *um1Tbl = &(uTbl_[0]);
  float   *u0Tbl = &(uTbl_[8]);
  float   *u1Tbl = &(uTbl_[16]);
  float   vTbl_[24];
  float   *vTbl = &(vTbl_[8]);
  float   *vm1Tbl = &(vTbl_[0]);
  float   *v0Tbl = &(vTbl_[8]);
  float   *v1Tbl = &(vTbl_[16]);
  int     offsX = x & (~(int(7)));
  int     offsY0 = y * 320;
  int     offsY1 = (y + 2) * 320;
  bool    oddField = bool(y & 1);
  for (int i = 0; i < 8; i++) {
    y0Tbl[i] = resizedImageY[offsY0 + offsX + i];
    y1Tbl[i] = resizedImageY[offsY1 + offsX + i];
    um1Tbl[i] = prvLineU[offsX + i];
    u0Tbl[i] = resizedImageU[offsY0 + offsX + i];
    u1Tbl[i] = resizedImageU[offsY1 + offsX + i];
    vm1Tbl[i] = prvLineV[offsX + i];
    v0Tbl[i] = resizedImageV[offsY0 + offsX + i];
    v1Tbl[i] = resizedImageV[offsY1 + offsX + i];
  }
  int     l0 = 0;
  int     l1 = 8;
  int     c0 = 0;
  int     c1 = 0;
  // find the best pair of luminance values
  {
    float   tmpBuf[16];
    for (int i = 0; i < 16; i++)
      tmpBuf[i] = yTbl[i];
    if (colorSearchMode == 1) {
      for (int i = 0; i < 15; i++) {
        for (int j = i + 1; j < 16; j++) {
          if (tmpBuf[i] > tmpBuf[j]) {
            float   tmp = tmpBuf[i];
            tmpBuf[i] = tmpBuf[j];
            tmpBuf[j] = tmp;
          }
        }
      }
      for (int i = 1; i < 15; i++)
        tmpBuf[i] = tmpBuf[(i < 8 ? 0 : 15)];
    }
    float   minVal = 0.0f;
    float   maxVal = 1.0f;
    double  minErr = 1000000.0f;
    for (int l0tmp = 0; l0tmp < 8; l0tmp++) {
      for (int l1tmp = l0tmp + 1; l1tmp < 9; l1tmp++) {
        minVal = yTable[l0tmp];
        maxVal = yTable[l1tmp];
        double  err = 0.0;
        for (int i = 0; i < 16; i++) {
          double  err0 = double(tmpBuf[i] - minVal);
          double  err1 = double(tmpBuf[i] - maxVal);
          double  err_ = err0 + err1;
          err0 = err0 * err0;
          err1 = err1 * err1;
          err_ = err_ * err_;
          err = err + (err0 < err1 ? err0 : err1) + (err_ * 0.00001);
        }
        if (err < minErr) {
          minErr = err;
          l0 = l0tmp;
          l1 = l1tmp;
        }
      }
    }
  }
  float   minVal = yTable[l0];
  float   maxVal = yTable[l1];
  // get PRG data pointers
  static const int bitmapOffsetTable[16] = {
    0x2000, 0x4000, 0x2001, 0x4001, 0x2002, 0x4002, 0x2003, 0x4003,
    0x2004, 0x4004, 0x2005, 0x4005, 0x2006, 0x4006, 0x2007, 0x4007
  };
  static const int lumOffsetTable[16] = {
    0x1800, 0x7800, 0x1800, 0x7800, 0x6000, 0x8000, 0x6000, 0x8000,
    0x6800, 0x8800, 0x6800, 0x8800, 0x7000, 0x9000, 0x7000, 0x9000
  };
  static const int clrOffsetTable[16] = {
    0x1C00, 0x7C00, 0x1C00, 0x7C00, 0x6400, 0x8400, 0x6400, 0x8400,
    0x6C00, 0x8C00, 0x6C00, 0x8C00, 0x7400, 0x9400, 0x7400, 0x9400
  };
  unsigned char   *bitmapPtr = &(prgData[((y / 16) * 320)
                                         + ((x / 8) * 8)
                                         + bitmapOffsetTable[y & 15] - 0x0FFF]);
  unsigned char   *lumPtr = &(prgData[((y / 16) * 40)
                                      + (x / 8)
                                      + lumOffsetTable[y & 15] - 0x0FFF]);
  unsigned char   *clrPtr = &(prgData[((y / 16) * 40)
                                      + (x / 8)
                                      + clrOffsetTable[y & 15] - 0x0FFF]);
  // calculate two bitmap bytes with dithering
  for (int i = 0; i < 16; i++) {
    float   c = yTbl[i];
    float   err0 = float(std::fabs(double(c - minVal)));
    float   err1 = float(std::fabs(double(c - maxVal)));
    c = (c > minVal ? (c < maxVal ? c : maxVal) : minVal);
    c = (c - minVal) / (maxVal - minVal);
    float   tmp = 0.5f;
    if (std::fabs(double(err1 - err0)) <= ditherLimit) {
      int     ditherIndex = ((((y + ((i & 8) >> 2)) & 15) << 4)
                            | ((x + (i & 7)) & 15));
      ditherIndex = ditherIndex ^ (i < 8 ? ditherXORValue0 : ditherXORValue1);
      if (oddField && !disableXShift)
        ditherIndex = (ditherIndex & 0xF0) | ((ditherIndex + 4) & 0x0F);
      tmp = (float(ditherTable[ditherIndex]) + 0.5f) / 256.0f;
    }
    if (c >= tmp)
      bitmapPtr[i >> 3] |= (unsigned char) (1 << (7 - (i & 7)));
  }
  // if all bits are 0 or 1, replace with dither pattern
  // to improve color conversion
  if (bitmapPtr[0] == bitmapPtr[1] &&
      (bitmapPtr[0] == 0x00 || bitmapPtr[0] == 0xFF)) {
    if (bitmapPtr[0] == 0x00)
      l1 = l0;
    else
      l0 = l1;
    bitmapPtr[0] = 0x00;
    bitmapPtr[1] = 0x00;
    for (int i = 0; i < 16; i++) {
      int     ditherIndex = ((((y + ((i & 8) >> 2)) & 15) << 4)
                            | ((x + (i & 7)) & 15));
      ditherIndex = ditherIndex ^ (i < 8 ? ditherXORValue0 : ditherXORValue1);
      if (oddField && !disableXShift)
        ditherIndex = (ditherIndex & 0xF0) | ((ditherIndex + 4) & 0x0F);
      float   tmp = (float(ditherTable[ditherIndex]) + 0.5f) / 256.0f;
      if (tmp <= 0.5f)
        bitmapPtr[i >> 3] |= (unsigned char) (1 << (7 - (i & 7)));
    }
  }
  // store luminance code
  (*lumPtr) = (unsigned char) (((l0 > 0 ? (l0 - 1) : l0) << 4)
                               | (l1 > 0 ? (l1 - 1) : l1));
  // find the pair of colors that gives the least amount of error
  float   tmpUTbl_[24];
  float   *tmpUTbl = &(tmpUTbl_[8]);
  float   tmpVTbl_[24];
  float   *tmpVTbl = &(tmpVTbl_[8]);
  for (int i = 0; i < 24; i++) {
    tmpUTbl_[i] = uTbl_[i];
    tmpVTbl_[i] = vTbl_[i];
  }
  bool    useAltColor = oddField;
  if (disableXShift && (x & 8) != 0)
    useAltColor = !useAltColor;
  double  minColorErr = 1000000.0;
  for (int i0 = 0; i0 < (disableXShift ? 43 : 29); i0++) {
    for (int i1 = i0; i1 < (disableXShift ? 43 : 29); i1++) {
      float   u0 = 0.0f;
      float   v0 = 0.0f;
      float   u1 = 0.0f;
      float   v1 = 0.0f;
      int     c0tmp = 0;
      int     c1tmp = 0;
      if (l0 > 0) {
        u0 = uvTable[i0].u;
        v0 = uvTable[i0].v;
        c0tmp = (useAltColor ? uvTable[i0].c1 : uvTable[i0].c0);
      }
      if (l1 > 0) {
        u1 = uvTable[i1].u;
        v1 = uvTable[i1].v;
        c1tmp = (useAltColor ? uvTable[i1].c1 : uvTable[i1].c0);
      }
      double  err = 0.0;
      for (int j = 0; j < 16; j++) {
        float   u_ = u0;
        float   v_ = v0;
        if (int(bitmapPtr[j >> 3]) & (1 << (7 - (j & 7)))) {
          u_ = u1;
          v_ = v1;
        }
        tmpUTbl[j] = u_;
        tmpVTbl[j] = v_;
      }
      for (int j = 0; j < 16; j++) {
        float   u_ = tmpUTbl[j];
        float   v_ = tmpVTbl[j];
        if (!disablePAL) {
          // assume PAL filtering if requested
          u_ += tmpUTbl[j - 8];
          v_ += tmpVTbl[j - 8];
          if ((j & 7) != 7) {
            u_ *= 0.96f;
            v_ *= 0.96f;
            if ((j & 7) != 0) {
              u_ += ((tmpUTbl[j - 1] + tmpUTbl[j - 9]) * 0.52f);
              v_ += ((tmpVTbl[j - 1] + tmpVTbl[j - 9]) * 0.52f);
            }
            else {
              u_ += ((prvBitmapU[j + 15] + prvBitmapU[j + 7]) * 0.52f);
              v_ += ((prvBitmapV[j + 15] + prvBitmapV[j + 7]) * 0.52f);
            }
            u_ += ((tmpUTbl[j + 1] + tmpUTbl[j - 7]) * 0.52f);
            v_ += ((tmpVTbl[j + 1] + tmpVTbl[j - 7]) * 0.52f);
          }
          else {
            u_ += (tmpUTbl[j - 1] + tmpUTbl[j - 9]);
            v_ += (tmpVTbl[j - 1] + tmpVTbl[j - 9]);
          }
          u_ *= 0.25f;
          v_ *= 0.25f;
        }
        double  errU = double(u_) - double(uTbl[j]);
        double  errV = double(v_) - double(vTbl[j]);
        err = err + (errU * errU) + (errV * errV);
      }
      if (err < minColorErr) {
        c0 = c0tmp;
        c1 = c1tmp;
        minColorErr = err;
        // save previous color information for PAL filtering
        for (int j = 0; j < 24; j++) {
          prvBitmapU[j] = tmpUTbl_[j];
          prvBitmapV[j] = tmpVTbl_[j];
        }
        for (int j = 0; j < 8; j++) {
          prvLineU[offsX + j] = tmpUTbl[j + 8];
          prvLineV[offsX + j] = tmpVTbl[j + 8];
        }
      }
    }
  }
  // store color code
  (*clrPtr) = (unsigned char) ((c1 << 4) | c0);
}

int main(int argc, char **argv)
{
  const char  *infileName = (char *) 0;
  const char  *outfileName = (char *) 0;
  bool      invalidOption = false;
  bool      printUsageFlag = false;
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "-ymin") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      yMin = std::atof(argv[i]);
      yMin = (yMin > -0.5 ? (yMin < 1.0 ? yMin : 1.0) : -0.5);
    }
    else if (std::strcmp(argv[i], "-ymax") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      yMax = std::atof(argv[i]);
      yMax = (yMax > 0.0 ? (yMax < 2.0 ? yMax : 2.0) : 0.0);
    }
    else if (std::strcmp(argv[i], "-saturation") == 0) {
      if ((i + 2) >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i]);
        return -1;
      }
      colorSaturationMult = std::atof(argv[i + 1]);
      colorSaturationPow = std::atof(argv[i + 2]);
      i = i + 2;
      colorSaturationMult = (colorSaturationMult > 0.0 ?
                             (colorSaturationMult < 8.0 ?
                              colorSaturationMult : 8.0)
                             : 0.0);
      colorSaturationPow = (colorSaturationPow > 0.1 ?
                            (colorSaturationPow < 2.0 ?
                             colorSaturationPow : 2.0)
                            : 0.1);
    }
    else if (std::strcmp(argv[i], "-mgamma") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      monitorGamma = std::atof(argv[i]);
      monitorGamma = (monitorGamma > 0.25 ?
                      (monitorGamma < 4.0 ? monitorGamma : 4.0) : 0.25);
    }
    else if (std::strcmp(argv[i], "-ditherlimit") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      ditherLimit = std::atof(argv[i]);
      ditherLimit = (ditherLimit > 0.0 ?
                     (ditherLimit < 2.0 ? ditherLimit : 2.0) : 0.0);
    }
    else if (std::strcmp(argv[i], "-pal") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      disablePAL = !(std::atoi(argv[i]));
    }
    else if (std::strcmp(argv[i], "-noxshift") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      disableXShift = bool(std::atoi(argv[i]));
    }
    else if (std::strcmp(argv[i], "-searchmode") == 0) {
      if (++i >= argc) {
        std::fprintf(stderr, " *** missing argument for '%s'\n", argv[i - 1]);
        return -1;
      }
      colorSearchMode = std::atoi(argv[i]);
      colorSearchMode = (colorSearchMode > 0 ?
                         (colorSearchMode < 1 ? colorSearchMode : 1) : 0);
    }
    else if (std::strcmp(argv[i], "-h") == 0 ||
             std::strcmp(argv[i], "-help") == 0 ||
             std::strcmp(argv[i], "--help") == 0) {
      printUsageFlag = true;
    }
    else if (argv[i][0] == '-' || outfileName != (char *) 0) {
      std::fprintf(stderr, " *** invalid option '%s'\n", argv[i]);
      invalidOption = true;
      break;
    }
    else {
      if (!infileName)
        infileName = argv[i];
      else
        outfileName = argv[i];
    }
  }
  if (invalidOption || printUsageFlag || outfileName == (char *) 0) {
    std::fprintf(stderr, "Usage: %s [OPTIONS...] infile.rgb outfile.prg\n",
                         argv[0]);
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "    -ymin <MIN>         (default: 0.0)\n");
    std::fprintf(stderr, "    -ymax <MAX>         (default: 1.0)\n");
    std::fprintf(stderr, "        scale luminance range from 0..1 to "
                         "MIN..MAX\n");
    std::fprintf(stderr, "    -saturation <M> <P> (defaults: 1.0, 0.5)\n");
    std::fprintf(stderr, "    -mgamma <N>         (default: 1.33)\n");
    std::fprintf(stderr, "        assume monitor gamma N\n");
    std::fprintf(stderr, "    -ditherlimit <N>    (default: 0.1)\n");
    std::fprintf(stderr, "    -pal <N>            (0 or 1, default: 1)\n");
    std::fprintf(stderr, "    -noxshift <N>       (0 or 1, default: 0)\n");
    std::fprintf(stderr, "    -searchmode <N>     (0 or 1, default: 1)\n");
    return (printUsageFlag ? 0 : -1);
  }
  createDitherTable();
  createUVTables();
  for (size_t i = 0x0000; i < 0x0401; i++)
    prgData[i] = prgHeader[i];
  if (disableXShift)
    prgData[0x01DB] = 0x00;
  for (size_t i = 0x0401; i < 0x8801; i++)
    prgData[i] = 0x00;
  std::FILE *f = std::fopen(infileName, "rb");
  if (!f) {
    std::fprintf(stderr, " *** error opening input file '%s'\n", infileName);
    return -1;
  }
  for (size_t i = 0; i < (640 * 400 * 3); i++) {
    int     c = std::fgetc(f);
    if (c == EOF) {
      std::fprintf(stderr, " *** unexpected end of input data\n");
      std::fclose(f);
      return -1;
    }
    inputImage[i] = (unsigned char) (c & 0xFF);
  }
  if (std::fgetc(f) != EOF) {
    std::fprintf(stderr, " *** more input data than expected\n");
    std::fclose(f);
    return -1;
  }
  std::fclose(f);
  f = (std::FILE *) 0;
  for (int y = 0; y < 400; y++) {
    double  tmp = 0.0;
    double  tmpU = 0.0;
    double  tmpV = 0.0;
    for (int x = 0; x < 640; x++) {
      double  r = 0.0;
      double  g = 0.0;
      double  b = 0.0;
      if (((y & 1) == 0 && x >= 8 && x < 632) ||
          ((y & 1) == 1 && x >= 0 && x < 624) ||
          disableXShift) {
        int     offs =
            ((y * 640) + (disableXShift || (y & 1) == 0 ? x : (x + 8))) * 3;
        r = (double(inputImage[offs + 0]) * (yMax - yMin) / 255.0) + yMin;
        g = (double(inputImage[offs + 1]) * (yMax - yMin) / 255.0) + yMin;
        b = (double(inputImage[offs + 2]) * (yMax - yMin) / 255.0) + yMin;
      }
      double  y_ = (r * 0.299) + (g * 0.587) + (b * 0.114);
      double  u_ = (b - y_) * 0.492 * colorSaturationMult;
      double  v_ = (r - y_) * 0.877 * colorSaturationMult;
      double  c_ = std::sqrt((u_ * u_) + (v_ * v_));
      if (c_ > 0.000001) {
        // reduce saturation range
        c_ = std::pow((c_ / 0.19), colorSaturationPow) * (0.19 / c_);
        u_ = u_ * c_;
        v_ = v_ * c_;
      }
      tmp += y_;
      tmpU += u_;
      tmpV += v_;
      if ((x & 1) == 1) {
        tmp = tmp * 0.5;
        tmp = (tmp > 0.0 ? (tmp < 1.0 ? tmp : 1.0) : 0.0);
        tmp = std::pow(tmp, monitorGamma);
        resizedImageY[(y * 320) + (x / 2)] = float(tmp);
        resizedImageU[(y * 320) + (x / 2)] = float(tmpU * 0.5);
        resizedImageV[(y * 320) + (x / 2)] = float(tmpV * 0.5);
        tmp = 0.0;
        tmpU = 0.0;
        tmpV = 0.0;
      }
    }
  }
  for (int y = 0; y < 400; y += 2) {
    for (int x = 0; x < 320; x++) {
      float   u0 = resizedImageU[((y + 0) * 320) + x];
      float   u1 = resizedImageU[((y + 1) * 320) + x];
      float   v0 = resizedImageV[((y + 0) * 320) + x];
      float   v1 = resizedImageV[((y + 1) * 320) + x];
      resizedImageU[((y + 0) * 320) + x] = (u0 + u1) * 0.5f;
      resizedImageU[((y + 1) * 320) + x] = (u0 + u1) * 0.5f;
      resizedImageV[((y + 0) * 320) + x] = (v0 + v1) * 0.5f;
      resizedImageV[((y + 1) * 320) + x] = (v0 + v1) * 0.5f;
    }
  }
  for (int i = 0; i < 9; i++) {
    double  tmp = yTableSrc[i];
    tmp = (tmp - yTableSrc[0]) / (yTableSrc[8] - yTableSrc[0]);
    tmp = (tmp > 0.0 ? (tmp < 1.0 ? tmp : 1.0) : 0.0);
    tmp = std::pow(tmp, monitorGamma);
    yTable[i] = float(tmp);
  }
  for (int i = 0; i < 320; i++) {
    prvLineU[i] = 0.0f;
    prvLineV[i] = 0.0f;
  }
  for (int y = 0; y < 400; y += 4) {
    std::fprintf(stderr, "\r  %3d%%  ", y * 50 / 400);
    int     xorValue0 = int((std::rand() & 0x4000) >> 14);
    int     xorValue1 = int((std::rand() & 0x4000) >> 14);
    for (int i = 0; i < 24; i++) {
      prvBitmapU[i] = 0.0f;
      prvBitmapV[i] = 0.0f;
    }
    for (int x = 0; x < 320; x += 8)
      convert8x2Pixels(x, y, xorValue0, xorValue1);
  }
  for (int i = 0; i < 320; i++) {
    prvLineU[i] = 0.0f;
    prvLineV[i] = 0.0f;
  }
  for (int y = 1; y < 400; y += 4) {
    std::fprintf(stderr, "\r  %3d%%  ", (y + 400) * 50 / 400);
    int     xorValue0 = int((std::rand() & 0x4000) >> 14);
    int     xorValue1 = int((std::rand() & 0x4000) >> 14);
    for (int i = 0; i < 24; i++) {
      prvBitmapU[i] = 0.0f;
      prvBitmapV[i] = 0.0f;
    }
    for (int x = 0; x < 320; x += 8)
      convert8x2Pixels(x, y, xorValue0, xorValue1);
  }
  std::fprintf(stderr, "\r  100%%  \n");
  f = std::fopen(outfileName, "wb");
  if (!f) {
    std::fprintf(stderr, " *** error opening output file '%s'\n", outfileName);
    return -1;
  }
  for (int i = 0; i < 0x8801; i++) {
    if (std::fputc(int(prgData[i]), f) == EOF) {
      std::fprintf(stderr, " *** error writing output file\n");
      std::fclose(f);
      return -1;
    }
  }
  if (std::fflush(f) != 0) {
    std::fprintf(stderr, " *** error writing output file\n");
    std::fclose(f);
    return -1;
  }
  std::fclose(f);
  return 0;
}

