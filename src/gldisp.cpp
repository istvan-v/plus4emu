
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
#include "system.hpp"

#include <cstring>
#include <typeinfo>

#define GL_GLEXT_PROTOTYPES 1

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glext.h>

#include "fldisp.hpp"
#include "gldisp.hpp"

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#ifdef WIN32
#  include <wingdi.h>
#  if defined(_MSC_VER) && !defined(__GNUC__)
typedef void (APIENTRY *PFNGLBLENDCOLORPROC)(GLclampf, GLclampf, GLclampf,
                                             GLclampf);
#  endif
#endif

#ifdef ENABLE_GL_SHADERS

static const char *shaderSourcePAL[1] = {
  "uniform sampler2D textureHandle;\n"
  "uniform float blendScale1;\n"
  "const mat4 yuv2rgbMatrix = mat4( 0.5000,  0.0000,  0.2337,  0.0000,\n"
  "                                 0.5000, -0.0574, -0.1190,  0.0000,\n"
  "                                 0.5000,  0.2953,  0.0000,  0.0000,\n"
  "                                 0.0000,  0.0000,  0.0000,  1.0000);\n"
  "void main()\n"
  "{\n"
  "  vec2 c0 = vec2(gl_TexCoord[0][0], gl_TexCoord[0][1]);\n"
  "  vec4 p00 = texture2D(textureHandle, c0 + vec2(0.0032, 0.015625));\n"
  "  vec4 p01 = texture2D(textureHandle, c0 + vec2(0.0009, 0.015625));\n"
  "  vec4 p02 = texture2D(textureHandle, c0 + vec2(-0.0009, 0.015625));\n"
  "  vec4 p03 = texture2D(textureHandle, c0 + vec2(-0.0032, 0.015625));\n"
  "  vec4 p10 = texture2D(textureHandle, c0 + vec2(0.0032, -0.046875));\n"
  "  vec4 p11 = texture2D(textureHandle, c0 + vec2(0.0009, -0.046875));\n"
  "  vec4 p12 = texture2D(textureHandle, c0 + vec2(-0.0009, -0.046875));\n"
  "  vec4 p13 = texture2D(textureHandle, c0 + vec2(-0.0032, -0.046875));\n"
  "  p01 = p01 + p02;\n"
  "  vec4 tmp = ((p00 + p03 + p10 + p13) * 0.5) + (p01 + p11 + p12);\n"
  "  float y = p01[0];\n"
  "  float u = tmp[1] - 3.0;\n"
  "  float v = tmp[2] - 3.0;\n"
  "  float f = mix(sin(c0[1] * 100.531) * 0.5 + 0.5, 1.0, blendScale1);\n"
  "  gl_FragColor = (vec4(y, u, v, 0.0) * yuv2rgbMatrix) * f;\n"
  "}\n"
};

static const char *shaderSourceNTSC[1] = {
  "uniform sampler2D textureHandle;\n"
  "uniform float blendScale1;\n"
  "const mat4 yuv2rgbMatrix = mat4( 0.5000,  0.0000,  0.4673,  0.0000,\n"
  "                                 0.5000, -0.1147, -0.2380,  0.0000,\n"
  "                                 0.5000,  0.5907,  0.0000,  0.0000,\n"
  "                                 0.0000,  0.0000,  0.0000,  1.0000);\n"
  "void main()\n"
  "{\n"
  "  vec2 c0 = vec2(gl_TexCoord[0][0], gl_TexCoord[0][1]);\n"
  "  vec4 p00 = texture2D(textureHandle, c0 + vec2(0.0032, 0.015625));\n"
  "  vec4 p01 = texture2D(textureHandle, c0 + vec2(0.0009, 0.015625));\n"
  "  vec4 p02 = texture2D(textureHandle, c0 + vec2(-0.0009, 0.015625));\n"
  "  vec4 p03 = texture2D(textureHandle, c0 + vec2(-0.0032, 0.015625));\n"
  "  p01 = p01 + p02;\n"
  "  vec4 tmp = ((p00 + p03) * 0.5) + p01;\n"
  "  float y = p01[0];\n"
  "  float u = tmp[1] - 1.5;\n"
  "  float v = tmp[2] - 1.5;\n"
  "  float f = mix(sin(c0[1] * 100.531) * 0.5 + 0.5, 1.0, blendScale1);\n"
  "  gl_FragColor = (vec4(y, u, v, 0.0) * yuv2rgbMatrix) * f;\n"
  "}\n"
};

#endif  // ENABLE_GL_SHADERS

static void setTextureParameters(int displayQuality)
{
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  if (displayQuality > 0) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glPixelTransferf(GL_RED_SCALE, GLfloat(1));
  glPixelTransferf(GL_RED_BIAS, GLfloat(0));
  glPixelTransferf(GL_GREEN_SCALE, GLfloat(1));
  glPixelTransferf(GL_GREEN_BIAS, GLfloat(0));
  glPixelTransferf(GL_BLUE_SCALE, GLfloat(1));
  glPixelTransferf(GL_BLUE_BIAS, GLfloat(0));
  glPixelTransferf(GL_ALPHA_SCALE, GLfloat(1));
  glPixelTransferf(GL_ALPHA_BIAS, GLfloat(0));
}

static void initializeTexture(const Plus4Emu::VideoDisplay::DisplayParameters&
                                  dp,
                              const unsigned char *textureBuffer)
{
  GLsizei txtWidth = 1024;
  GLsizei txtHeight = 16;
  switch (dp.displayQuality) {
  case 0:
    txtWidth = 512;
    txtHeight = 8;
    break;
  case 1:
    txtWidth = 512;
    break;
  }
  if (dp.displayQuality < 3) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, txtWidth, txtHeight, 0,
                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                 (const GLvoid *) textureBuffer);
  }
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, txtWidth, txtHeight, 0,
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 (const GLvoid *) textureBuffer);
  }
}

namespace Plus4Emu {

  bool OpenGLDisplay::compileShader(int shaderMode_)
  {
#ifdef ENABLE_GL_SHADERS
    if (shaderMode_ == shaderMode)
      return true;
    if (shaderMode != 0)
      deleteShader();
    if (shaderMode_ == 0)
      return true;
    shaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    if (!shaderHandle)
      return false;
    programHandle = glCreateProgram();
    if (!programHandle) {
      glDeleteShader(GLuint(shaderHandle));
      shaderHandle = 0UL;
      return false;
    }
    if (shaderMode_ == 1) {
      glShaderSource(GLuint(shaderHandle),
                     GLsizei(1), &(shaderSourcePAL[0]), (GLint *) 0);
    }
    else {
      glShaderSource(GLuint(shaderHandle),
                     GLsizei(1), &(shaderSourceNTSC[0]), (GLint *) 0);
    }
    glAttachShader(GLuint(programHandle), GLuint(shaderHandle));
    shaderMode = shaderMode_;
    glCompileShader(GLuint(shaderHandle));
    GLint   compileStatus = GL_FALSE;
    glGetShaderiv(GLuint(shaderHandle), GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
      deleteShader();
      return false;
    }
    glLinkProgram(GLuint(programHandle));
    shaderMode = shaderMode_;
    return true;
#else
    (void) shaderMode_;
    shaderMode = 0;
    return false;
#endif  // ENABLE_GL_SHADERS
  }

  void OpenGLDisplay::deleteShader()
  {
#ifdef ENABLE_GL_SHADERS
    if (shaderMode == 0)
      return;
    disableShader();
    shaderMode = 0;
    glDetachShader(GLuint(programHandle), GLuint(shaderHandle));
    glDeleteProgram(GLuint(programHandle));
    programHandle = 0UL;
    glDeleteShader(GLuint(shaderHandle));
    shaderHandle = 0UL;
#endif  // ENABLE_GL_SHADERS
  }

  bool OpenGLDisplay::enableShader()
  {
#ifdef ENABLE_GL_SHADERS
    if (shaderMode == 0)
      return false;
    glUseProgram(GLuint(programHandle));
    // FIXME: is it safe to use a constant texture ID of 0 here ?
    glUniform1i(glGetUniformLocation(GLuint(programHandle), "textureHandle"),
                0);
    glUniform1f(glGetUniformLocation(GLuint(programHandle), "blendScale1"),
                float(displayParameters.blendScale1) * 1.996f + 0.001f);
    return true;
#else
    return false;
#endif  // ENABLE_GL_SHADERS
  }

  void OpenGLDisplay::disableShader()
  {
#ifdef ENABLE_GL_SHADERS
    if (shaderMode == 0)
      return;
    glUseProgram(GLuint(0));
#endif  // ENABLE_GL_SHADERS
  }

  OpenGLDisplay::OpenGLDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl, bool isDoubleBuffered)
    : Fl_Gl_Window(xx, yy, ww, hh, lbl),
      FLTKDisplay_(),
      colormap16(),
      colormap32(),
      linesChanged((bool *) 0),
      textureSpace((unsigned char *) 0),
      textureBuffer16((uint16_t *) 0),
      textureBuffer32((uint32_t *) 0),
      textureID(0UL),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false),
      displayFrameRate(60.0),
      inputFrameRate(50.0),
      ringBufferReadPos(0.0),
      ringBufferWritePos(2),
      shaderMode(0),
      shaderHandle(0UL),
      programHandle(0UL)
  {
    try {
      for (size_t n = 0; n < 4; n++)
        frameRingBuffer[n] = (Message_LineData **) 0;
      linesChanged = new bool[289];
      for (size_t n = 0; n < 289; n++)
        linesChanged[n] = false;
      // max. texture size = 1024x16, 32 bits
      textureSpace = new unsigned char[1024 * 16 * 4];
      std::memset(textureSpace, 0, 1024 * 16 * 4);
      textureBuffer16 = reinterpret_cast<uint16_t *>(textureSpace);
      textureBuffer32 = reinterpret_cast<uint32_t *>(textureSpace);
      for (size_t n = 0; n < 4; n++) {
        frameRingBuffer[n] = new Message_LineData*[578];
        for (size_t yc = 0; yc < 578; yc++)
          frameRingBuffer[n][yc] = (Message_LineData *) 0;
      }
    }
    catch (...) {
      if (linesChanged)
        delete[] linesChanged;
      if (textureSpace)
        delete[] textureSpace;
      for (size_t n = 0; n < 4; n++) {
        if (frameRingBuffer[n])
          delete[] frameRingBuffer[n];
      }
      throw;
    }
    displayParameters.bufferingMode = (isDoubleBuffered ? 1 : 0);
    savedDisplayParameters.bufferingMode = (isDoubleBuffered ? 1 : 0);
    this->mode(FL_RGB | (isDoubleBuffered ? FL_DOUBLE : FL_SINGLE));
  }

  OpenGLDisplay::~OpenGLDisplay()
  {
    Fl::remove_idle(&fltkIdleCallback, (void *) this);
    deleteShader();
    if (textureID) {
      GLuint  tmp = GLuint(textureID);
      textureID = 0UL;
      glDeleteTextures(1, &tmp);
    }
    delete[] textureSpace;
    delete[] linesChanged;
    for (size_t n = 0; n < 4; n++) {
      for (size_t yc = 0; yc < 578; yc++) {
        Message *m = frameRingBuffer[n][yc];
        if (m) {
          frameRingBuffer[n][yc] = (Message_LineData *) 0;
          m->~Message();
          std::free(m);
        }
      }
      delete[] frameRingBuffer[n];
    }
  }

  void OpenGLDisplay::decodeLine_quality0(uint16_t *outBuf,
                                          Message_LineData **lineBuffers_,
                                          size_t lineNum)
  {
    Message_LineData  *l = (Message_LineData *) 0;
    if (lineNum < 578) {
      if (lineBuffers_[lineNum] != (Message_LineData *) 0)
        l = lineBuffers_[lineNum];
      else
        l = lineBuffers_[lineNum ^ 1];
    }
    if (!l) {
      for (size_t xc = 0; xc < 384; xc++)
        outBuf[xc] = uint16_t(0);
      return;
    }
    const unsigned char *bufp = (unsigned char *) 0;
    size_t  nBytes = 0;
    size_t  bufPos = 0;
    size_t  xc = 0;
    uint8_t videoFlags = uint8_t((lineNum & 2) | ((l->flags & 0x80) >> 2));
    size_t  pixelSample2 = l->lineLength;
    l->getLineData(bufp, nBytes);
    if (displayParameters.ntscMode)
      videoFlags = videoFlags | 0x10;
    if (pixelSample2 == (displayParameters.ntscMode ? 396 : 495) &&
        !(l->flags & 0x01)) {
      do {
        size_t  n = colormap16.convertFourPixels(&(outBuf[xc]), &(bufp[bufPos]),
                                                 videoFlags);
        bufPos = bufPos + n;
        xc = xc + 4;
      } while (xc < 384);
    }
    else {
      uint16_t  tmpBuf[4];
      size_t  pixelSample1 = 495;
      size_t  pixelSampleCnt = 0;
      uint8_t readPos = 4;
      do {
        if (readPos >= 4) {
          readPos = readPos & 3;
          if (bufPos >= nBytes) {
            for ( ; xc < 384; xc++)
              outBuf[xc] = uint16_t(0);
            break;
          }
          pixelSample1 = ((bufp[bufPos] & 0x01) ? 396 : 495);
          size_t  n = colormap16.convertFourPixels(&(tmpBuf[0]),
                                                   &(bufp[bufPos]), videoFlags);
          bufPos += n;
        }
        outBuf[xc] = tmpBuf[readPos];
        pixelSampleCnt += pixelSample2;
        while (pixelSampleCnt >= pixelSample1) {
          pixelSampleCnt -= pixelSample1;
          readPos++;
        }
        xc++;
      } while (xc < 384);
    }
  }

  void OpenGLDisplay::decodeLine_quality2(uint16_t *outBuf,
                                          Message_LineData **lineBuffers_,
                                          size_t lineNum)
  {
    Message_LineData  *l = (Message_LineData *) 0;
    if (lineNum < 578) {
      if (lineBuffers_[lineNum] != (Message_LineData *) 0)
        l = lineBuffers_[lineNum];
      else
        l = lineBuffers_[lineNum ^ 1];
    }
    if (!l) {
      for (size_t xc = 0; xc < 768; xc++)
        outBuf[xc] = uint16_t(0);
      return;
    }
    const unsigned char *bufp = (unsigned char *) 0;
    size_t  nBytes = 0;
    size_t  bufPos = 0;
    size_t  xc = 0;
    uint8_t videoFlags = uint8_t((lineNum & 2) | ((l->flags & 0x80) >> 2));
    size_t  pixelSample2 = l->lineLength;
    l->getLineData(bufp, nBytes);
    if (displayParameters.ntscMode)
      videoFlags = videoFlags | 0x10;
    if (pixelSample2 == (displayParameters.ntscMode ? 396 : 495) &&
        !(l->flags & 0x01)) {
      do {
        size_t  n = colormap16.convertFourToEightPixels(&(outBuf[xc]),
                                                        &(bufp[bufPos]),
                                                        videoFlags);
        bufPos = bufPos + n;
        xc = xc + 8;
      } while (xc < 768);
    }
    else {
      uint16_t  tmpBuf[4];
      size_t  pixelSample1 = 990;
      size_t  pixelSampleCnt = 0;
      uint8_t readPos = 4;
      do {
        if (readPos >= 4) {
          readPos = readPos & 3;
          if (bufPos >= nBytes) {
            for ( ; xc < 768; xc++)
              outBuf[xc] = uint16_t(0);
            break;
          }
          pixelSample1 = ((bufp[bufPos] & 0x01) ? 792 : 990);
          size_t  n = colormap16.convertFourPixels(&(tmpBuf[0]),
                                                   &(bufp[bufPos]), videoFlags);
          bufPos += n;
        }
        outBuf[xc] = tmpBuf[readPos];
        pixelSampleCnt += pixelSample2;
        if (pixelSampleCnt >= pixelSample1) {
          pixelSampleCnt -= pixelSample1;
          readPos++;
        }
        xc++;
      } while (xc < 768);
    }
  }

  void OpenGLDisplay::decodeLine_quality3(uint32_t *outBuf,
                                          Message_LineData **lineBuffers_,
                                          int lineNum)
  {
    Message_LineData  *l = (Message_LineData *) 0;
    if (lineNum >= 0 && lineNum < 578) {
      if (lineBuffers_[lineNum] != (Message_LineData *) 0)
        l = lineBuffers_[lineNum];
      else
        l = lineBuffers_[lineNum ^ 1];
    }
    if (!l) {
      for (size_t xc = 0; xc < 768; xc++)
        outBuf[xc] = 0x00808000U;
      return;
    }
    const unsigned char *bufp = (unsigned char *) 0;
    size_t  nBytes = 0;
    size_t  bufPos = 0;
    size_t  xc = 0;
    uint8_t videoFlags = uint8_t((lineNum & 2) | ((l->flags & 0x80) >> 2));
    size_t  pixelSample2 = l->lineLength;
    l->getLineData(bufp, nBytes);
    if (displayParameters.ntscMode)
      videoFlags = videoFlags | 0x10;
    if (pixelSample2 == (displayParameters.ntscMode ? 396 : 495) &&
        !(l->flags & 0x01)) {
      do {
        size_t  n = colormap32.convertFourToEightPixels(&(outBuf[xc]),
                                                        &(bufp[bufPos]),
                                                        videoFlags);
        bufPos = bufPos + n;
        xc = xc + 8;
      } while (xc < 768);
    }
    else {
      uint32_t  tmpBuf[4];
      size_t  pixelSample1 = 990;
      size_t  pixelSampleCnt = 0;
      uint8_t readPos = 4;
      do {
        if (readPos >= 4) {
          readPos = readPos & 3;
          if (bufPos >= nBytes) {
            for ( ; xc < 768; xc++)
              outBuf[xc] = 0x00808000U;
            break;
          }
          pixelSample1 = ((bufp[bufPos] & 0x01) ? 792 : 990);
          size_t  n = colormap32.convertFourPixels(&(tmpBuf[0]),
                                                   &(bufp[bufPos]), videoFlags);
          bufPos += n;
        }
        outBuf[xc] = tmpBuf[readPos];
        pixelSampleCnt += pixelSample2;
        if (pixelSampleCnt >= pixelSample1) {
          pixelSampleCnt -= pixelSample1;
          readPos++;
        }
        xc++;
      } while (xc < 768);
    }
  }

  void OpenGLDisplay::drawFrame_quality0(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    // half horizontal resolution, no interlace (384x288)
    // no texture filtering or effects
    for (size_t yc = 0; yc < 288; yc += 8) {
      for (size_t offs = 0; offs < 8; offs++) {
        linesChanged[yc + offs + 1] = false;
        // decode video data, and build 16-bit texture
        decodeLine_quality0(&(textureBuffer16[offs * 384]),
                            lineBuffers_, (yc + offs + 1) << 1);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 8,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, textureSpace);
      // update display
      double  ycf0 = y0 + ((double(int(yc << 1)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc << 1) + 16) * (1.0 / 576.0))
                           * (y1 - y0));
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(0.001 / 8.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(0.001 / 8.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(7.999 / 8.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(7.999 / 8.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality1(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    // half horizontal resolution, no interlace (384x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      for (size_t offs = 0; offs < 32; offs += 2) {
        // decode video data, and build 16-bit texture
        decodeLine_quality0(&(textureBuffer16[(offs >> 1) * 384]),
                            lineBuffers_, yc + offs);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 16,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, textureSpace);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  txtycf1 = 15.0 / 16.0;
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= (6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality2(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    // full horizontal resolution, no interlace (768x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      for (size_t offs = 0; offs < 32; offs += 2) {
        // decode video data, and build 16-bit texture
        decodeLine_quality2(&(textureBuffer16[(offs >> 1) * 768]),
                            lineBuffers_, yc + offs);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 16,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, textureSpace);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0))
                           * (y1 - y0));
      double  txtycf1 = 15.0 / 16.0;
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= (6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(1.0 / 16.0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality3(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1)
  {
    if (shaderMode != (displayParameters.ntscMode ? 2 : 1))
      compileShader(displayParameters.ntscMode ? 2 : 1);
    if (!enableShader()) {
      drawFrame_quality2(lineBuffers_, x0, y0, x1, y1);
      return;
    }
    double  yOffs = (y1 - y0) * (-1.0 / 576.0);
    // interlace
    if (lineBuffers_[100] != (Message_LineData *) 0 ||
        lineBuffers_[101] == (Message_LineData *) 0) {
      yOffs = yOffs * 2.0;
    }
    // full horizontal resolution, interlace (768x576), TV emulation
    for (int yc = -4; yc < 594; yc += 26) {
      for (int offs = 0; offs < 32; offs += 2) {
        // decode video data, and build 32-bit texture
        decodeLine_quality3(&(textureBuffer32[(offs >> 1) * 768]),
                            lineBuffers_, yc + offs);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 16,
                      GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, textureSpace);
      // update display
      double  ycf0 =
          y0 + ((double(yc + 4) * (1.0 / 576.0)) * (y1 - y0)) + yOffs;
      double  ycf1 =
          y0 + ((double(yc + 30) * (1.0 / 576.0)) * (y1 - y0)) + yOffs;
      double  txtycf0 = 2.0 / 16.0;
      double  txtycf1 = 15.0 / 16.0;
      if (ycf0 < y0) {
        txtycf0 -= ((ycf0 - y0) * (288.0 / 16.0) / (y1 - y0));
        ycf0 = y0;
      }
      if (yc == 568) {
        ycf1 -= ((y1 - y0) * (20.0 / 576.0));
        txtycf1 -= (10.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf0));
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf0));
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), GLfloat(txtycf1));
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
    disableShader();
  }

  void OpenGLDisplay::fltkIdleCallback(void *userData_)
  {
    (void) userData_;
    Fl::unlock();
    Timer::wait(0.000001);
    Fl::lock();
  }

  void OpenGLDisplay::displayFrame()
  {
    glViewport(0, 0, GLsizei(this->w()), GLsizei(this->h()));
    glPushMatrix();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);

    double  x0, y0, x1, y1;
    double  aspectScale = (768.0 / 576.0)
                          / ((double(this->w()) / double(this->h()))
                             * displayParameters.pixelAspectRatio);
    x0 = 0.0;
    y0 = 0.0;
    x1 = 1.0;
    y1 = 1.0;
    if (aspectScale > 1.0001) {
      y0 = 0.5 - (0.5 / aspectScale);
      y1 = 1.0 - y0;
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(y0));
      glVertex2f(GLfloat(0.0), GLfloat(y0));
      glVertex2f(GLfloat(0.0), GLfloat(y1));
      glVertex2f(GLfloat(1.0), GLfloat(y1));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glEnd();
    }
    else if (aspectScale < 0.9999) {
      x0 = 0.5 - (0.5 * aspectScale);
      x1 = 1.0 - x0;
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(0.0));
      glVertex2f(GLfloat(x0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glVertex2f(GLfloat(x1), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(x1), GLfloat(1.0));
      glEnd();
    }
    if (x0 >= x1 || y0 >= y1)
      return;

    GLuint  textureID_ = GLuint(textureID);
    GLint   savedTextureID = 0;
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    setTextureParameters(displayParameters.displayQuality);

    if (displayParameters.displayQuality == 0 &&
        displayParameters.bufferingMode == 0) {
      // half horizontal resolution, no interlace (384x288)
      // no texture filtering or effects
      glDisable(GL_BLEND);
      for (size_t yc = 0; yc < 288; yc += 8) {
        size_t  offs;
        // quality=0 with single buffered display is special case: only those
        // lines are updated that have changed since the last frame
        for (offs = 0; offs < 8; offs++) {
          if (linesChanged[yc + offs + 1])
            break;
        }
        if (offs == 8)
          continue;
        for (offs = 0; offs < 8; offs++) {
          linesChanged[yc + offs + 1] = false;
          // decode video data, and build 16-bit texture
          decodeLine_quality0(&(textureBuffer16[offs * 384]),
                              lineBuffers, (yc + offs + 1) << 1);
        }
        // load texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 8,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, textureSpace);
        // update display
        double  ycf0 = y0 + ((double(int(yc << 1)) * (1.0 / 576.0))
                             * (y1 - y0));
        double  ycf1 = y0 + ((double(int(yc << 1) + 16) * (1.0 / 576.0))
                             * (y1 - y0));
        glBegin(GL_QUADS);
        glTexCoord2f(GLfloat(0.0), GLfloat(0.001 / 8.0));
        glVertex2f(GLfloat(x0), GLfloat(ycf0));
        glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(0.001 / 8.0));
        glVertex2f(GLfloat(x1), GLfloat(ycf0));
        glTexCoord2f(GLfloat(384.0 / 512.0), GLfloat(7.999 / 8.0));
        glVertex2f(GLfloat(x1), GLfloat(ycf1));
        glTexCoord2f(GLfloat(0.0), GLfloat(7.999 / 8.0));
        glVertex2f(GLfloat(x0), GLfloat(ycf1));
        glEnd();
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
      // make sure that all lines are updated at a slow rate
      if (forceUpdateLineMask) {
        if (!screenshotCallbackCnt) {
          for (size_t yc = 0; yc < 289; yc++) {
            if (!(forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 3) & 7))))
              continue;
            for (size_t tmp = (yc << 1); tmp < ((yc + 1) << 1); tmp++) {
              if (lineBuffers[tmp] != (Message_LineData *) 0) {
                Message *m = lineBuffers[tmp];
                lineBuffers[tmp] = (Message_LineData *) 0;
                deleteMessage(m);
              }
            }
            linesChanged[yc] = true;
          }
          forceUpdateLineMask = 0;
        }
      }
    }
    else if (displayParameters.bufferingMode != 2) {
      if (!(displayParameters.bufferingMode != 0 ||
            (displayParameters.blendScale2 > 0.99 &&
             displayParameters.blendScale3 < 0.01))) {
        glEnable(GL_BLEND);
#ifndef WIN32
        glBlendColor(GLclampf(displayParameters.blendScale2),
                     GLclampf(displayParameters.blendScale2),
                     GLclampf(displayParameters.blendScale2),
                     GLclampf(1.0 - displayParameters.blendScale3));
#else
        PFNGLBLENDCOLORPROC glBlendColor_ =
            (PFNGLBLENDCOLORPROC) wglGetProcAddress("glBlendColor");
        if (glBlendColor_)
          glBlendColor_(GLclampf(displayParameters.blendScale2),
                        GLclampf(displayParameters.blendScale2),
                        GLclampf(displayParameters.blendScale2),
                        GLclampf(1.0 - displayParameters.blendScale3));
        else
          glDisable(GL_BLEND);
#endif
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_ALPHA);
      }
      else
        glDisable(GL_BLEND);
      switch (displayParameters.displayQuality) {
      case 0:
        // half horizontal resolution, no interlace (384x288)
        // no texture filtering or effects
        drawFrame_quality0(lineBuffers, x0, y0, x1, y1);
        break;
      case 1:
        // half horizontal resolution, no interlace (384x288)
        drawFrame_quality1(lineBuffers, x0, y0, x1, y1);
        break;
      case 2:
        // full horizontal resolution, no interlace (768x288)
        drawFrame_quality2(lineBuffers, x0, y0, x1, y1);
        break;
      case 3:
        // full horizontal resolution, interlace (768x576), TV emulation
        drawFrame_quality3(lineBuffers, x0, y0, x1, y1);
        break;
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
      if (!screenshotCallbackCnt) {
        for (size_t n = 0; n < 578; n++) {
          if (lineBuffers[n] != (Message_LineData *) 0) {
            Message *m = lineBuffers[n];
            lineBuffers[n] = (Message_LineData *) 0;
            deleteMessage(m);
          }
        }
      }
    }
    else {
      // resample video input to monitor refresh rate
      int     readPosInt = int(ringBufferReadPos);
      double  readPosFrac = ringBufferReadPos - double(readPosInt);
      double  d = inputFrameRate / displayFrameRate;
      d = (d > 0.01 ? (d < 1.75 ? d : 1.75) : 0.01);
      switch ((ringBufferWritePos - readPosInt) & 3) {
      case 1:
        d = 0.0;
        readPosFrac = 0.0;
        break;
      case 2:
        d = d * 0.97;
        break;
      case 3:
        d = d * 1.04;
        break;
      case 0:
        d = d * 1.25;
        break;
      }
      ringBufferReadPos = ringBufferReadPos + d;
      if (ringBufferReadPos >= 4.0)
        ringBufferReadPos -= 4.0;
#ifdef WIN32
      PFNGLBLENDCOLORPROC glBlendColor_ =
          (PFNGLBLENDCOLORPROC) wglGetProcAddress("glBlendColor");
#endif
      bool    blendEnabled_ = true;
      if (readPosFrac >= 0.002 && readPosFrac <= 0.998)
        glEnable(GL_BLEND);
      else {
        glDisable(GL_BLEND);
        blendEnabled_ = false;
      }
      if (blendEnabled_) {
#ifndef WIN32
        glBlendColor(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                     GLclampf(1.0 - readPosFrac));
#else
        if (glBlendColor_)
          glBlendColor_(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                        GLclampf(1.0 - readPosFrac));
        else
          glDisable(GL_BLEND);
#endif
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ZERO);
      }
      if (blendEnabled_ || readPosFrac < 0.5) {
        switch (displayParameters.displayQuality) {
        case 0:
          // half horizontal resolution, no interlace (384x288)
          // no texture filtering or effects
          drawFrame_quality0(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576), TV emulation
          drawFrame_quality3(frameRingBuffer[readPosInt & 3], x0, y0, x1, y1);
          break;
        }
      }
      if (blendEnabled_) {
#ifndef WIN32
        glBlendColor(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                     GLclampf(readPosFrac));
#else
        if (glBlendColor_)
          glBlendColor_(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                        GLclampf(readPosFrac));
        else
          glDisable(GL_BLEND);
#endif
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
      }
      if (blendEnabled_ || readPosFrac > 0.5) {
        switch (displayParameters.displayQuality) {
        case 0:
          // half horizontal resolution, no interlace (384x288)
          // no texture filtering or effects
          drawFrame_quality0(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576), TV emulation
          drawFrame_quality3(frameRingBuffer[(readPosInt + 1) & 3],
                             x0, y0, x1, y1);
          break;
        }
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
    }

    messageQueueMutex.lock();
    framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
    messageQueueMutex.unlock();
  }

  void OpenGLDisplay::draw()
  {
    if (!textureID) {
      glViewport(0, 0, GLsizei(this->w()), GLsizei(this->h()));
      glPushMatrix();
      glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
      // on first call: initialize texture
      glEnable(GL_TEXTURE_2D);
      GLuint  tmp = 0;
      glGenTextures(1, &tmp);
      textureID = (unsigned long) tmp;
      GLint   savedTextureID;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
      glBindTexture(GL_TEXTURE_2D, tmp);
      setTextureParameters(displayParameters.displayQuality);
      initializeTexture(displayParameters, textureSpace);
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      // clear display
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glColor4f(GLfloat(0), GLfloat(0), GLfloat(0), GLfloat(1));
      glBegin(GL_QUADS);
      glVertex2f(GLfloat(0.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(0.0));
      glVertex2f(GLfloat(1.0), GLfloat(1.0));
      glVertex2f(GLfloat(0.0), GLfloat(1.0));
      glEnd();
      glPopMatrix();
    }
    if (redrawFlag || videoResampleEnabled) {
      redrawFlag = false;
      displayFrame();
      if (videoResampleEnabled) {
        double  t = displayFrameRateTimer.getRealTime();
        displayFrameRateTimer.reset();
        t = (t > 0.002 ? (t < 0.25 ? t : 0.25) : 0.002);
        displayFrameRate = 1.0 / ((0.97 / displayFrameRate) + (0.03 * t));
      }
    }
  }

  bool OpenGLDisplay::checkEvents()
  {
    threadLock.notify();
    while (true) {
      messageQueueMutex.lock();
      Message *m = messageQueue;
      if (m) {
        messageQueue = m->nxt;
        if (messageQueue) {
          messageQueue->prv = (Message *) 0;
          if (!messageQueue->nxt)
            lastMessage = messageQueue;
        }
        else
          lastMessage = (Message *) 0;
      }
      messageQueueMutex.unlock();
      if (!m)
        break;
      if (typeid(*m) == typeid(Message_LineData)) {
        Message_LineData  *msg;
        msg = static_cast<Message_LineData *>(m);
        if (msg->lineNum >= 0 && msg->lineNum < 578) {
          if (displayParameters.displayQuality == 0) {
            msg->lineNum = msg->lineNum & (~(int(1)));
            if (!displayParameters.bufferingMode) {
              // check if this line has changed
              if (lineBuffers[msg->lineNum] != (Message_LineData *) 0) {
                if (*(lineBuffers[msg->lineNum]) == *msg) {
                  deleteMessage(m);
                  continue;
                }
              }
              linesChanged[msg->lineNum >> 1] = true;
            }
          }
          if (lineBuffers[msg->lineNum] != (Message_LineData *) 0)
            deleteMessage(lineBuffers[msg->lineNum]);
          lineBuffers[msg->lineNum] = msg;
          continue;
        }
      }
      else if (typeid(*m) == typeid(Message_FrameDone)) {
        // need to update display
        if (redrawFlag) {
          // lost a frame
          messageQueueMutex.lock();
          framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
          messageQueueMutex.unlock();
        }
        redrawFlag = true;
        deleteMessage(m);
        noInputTimer.reset();
        if (screenshotCallbackCnt) {
          checkScreenshotCallback();
        }
        else if (videoResampleEnabled) {
          double  t = inputFrameRateTimer.getRealTime();
          inputFrameRateTimer.reset();
          t = (t > 0.002 ? (t < 0.25 ? t : 0.25) : 0.002);
          inputFrameRate = 1.0 / ((0.97 / inputFrameRate) + (0.03 * t));
          if (ringBufferWritePos != int(ringBufferReadPos)) {
            // if buffer is not already full, copy current frame
            for (size_t yc = 0; yc < 578; yc++) {
              if (frameRingBuffer[ringBufferWritePos][yc])
                deleteMessage(frameRingBuffer[ringBufferWritePos][yc]);
              frameRingBuffer[ringBufferWritePos][yc] = lineBuffers[yc];
              lineBuffers[yc] = (Message_LineData *) 0;
            }
            ringBufferWritePos = (ringBufferWritePos + 1) & 3;
            continue;
          }
        }
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = static_cast<Message_SetParameters *>(m);
        if (displayParameters.displayQuality != msg->dp.displayQuality ||
            displayParameters.bufferingMode != msg->dp.bufferingMode) {
          Fl::remove_idle(&fltkIdleCallback, (void *) this);
          if (displayParameters.bufferingMode != msg->dp.bufferingMode ||
              displayParameters.displayQuality == 3 ||
              msg->dp.displayQuality == 3) {
            // if TV emulation (quality=3) or double buffering mode
            // has changed, also need to generate a new texture ID
            deleteShader();
            GLuint  oldTextureID = GLuint(textureID);
            textureID = 0UL;
            if (oldTextureID)
              glDeleteTextures(1, &oldTextureID);
            this->mode(FL_RGB
                       | (msg->dp.bufferingMode != 0 ? FL_DOUBLE : FL_SINGLE));
            if (oldTextureID) {
              oldTextureID = 0U;
              glGenTextures(1, &oldTextureID);
              textureID = (unsigned long) oldTextureID;
            }
            Fl::focus(this);
          }
          if (msg->dp.bufferingMode == 2) {
            videoResampleEnabled = true;
            Fl::add_idle(&fltkIdleCallback, (void *) this);
            displayFrameRateTimer.reset();
            inputFrameRateTimer.reset();
          }
          else {
            videoResampleEnabled = false;
            for (size_t n = 0; n < 4; n++) {
              for (size_t yc = 0; yc < 578; yc++) {
                Message *m_ = frameRingBuffer[n][yc];
                if (m_) {
                  frameRingBuffer[n][yc] = (Message_LineData *) 0;
                  m_->~Message();
                  std::free(m_);
                }
              }
            }
          }
          // reset texture
          if (msg->dp.displayQuality < 3) {
            std::memset(textureBuffer16, 0, sizeof(uint16_t) * 1024 * 16);
          }
          else {
            for (size_t n = 0; n < (1024 * 16); n++)
              textureBuffer32[n] = 0x00808000U;
          }
          glEnable(GL_TEXTURE_2D);
          GLint   savedTextureID;
          glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTextureID);
          glBindTexture(GL_TEXTURE_2D, GLuint(textureID));
          setTextureParameters(msg->dp.displayQuality);
          initializeTexture(msg->dp, textureSpace);
          glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
        }
        displayParameters = msg->dp;
        if (displayParameters.displayQuality < 3)
          colormap16.setDisplayParameters(displayParameters, false);
        else
          colormap32.setDisplayParameters(displayParameters, true);
        for (size_t yc = 0; yc < 289; yc++)
          linesChanged[yc] = true;
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 0.6) {
      noInputTimer.reset(0.4);
      if (redrawFlag) {
        // lost a frame
        messageQueueMutex.lock();
        framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
        messageQueueMutex.unlock();
      }
      if (videoResampleEnabled) {
        for (size_t yc = 0; yc < 578; yc++) {
          Message *m = frameRingBuffer[ringBufferWritePos][yc];
          if (m) {
            frameRingBuffer[ringBufferWritePos][yc] = (Message_LineData *) 0;
            deleteMessage(m);
          }
        }
        ringBufferWritePos = (ringBufferWritePos + 1) & 3;
      }
      redrawFlag = true;
      if (screenshotCallbackCnt)
        checkScreenshotCallback();
    }
    if (this->damage() & FL_DAMAGE_EXPOSE) {
      forceUpdateLineMask = 0xFF;
      forceUpdateLineCnt = 0;
      forceUpdateTimer.reset();
    }
    else if (forceUpdateTimer.getRealTime() >= 0.085) {
      forceUpdateLineMask |= (uint8_t(1) << forceUpdateLineCnt);
      forceUpdateLineCnt++;
      forceUpdateLineCnt &= uint8_t(7);
      forceUpdateTimer.reset();
    }
    return (redrawFlag | videoResampleEnabled);
  }

  int OpenGLDisplay::handle(int event)
  {
    return fltkEventCallback(fltkEventCallbackUserData, event);
  }

#ifndef ENABLE_GL_SHADERS
  void OpenGLDisplay::setDisplayParameters(const DisplayParameters& dp)
  {
    DisplayParameters dp_(dp);
    if (dp_.displayQuality >= 3)
      dp_.displayQuality = 2;
    FLTKDisplay_::setDisplayParameters(dp_);
  }
#endif

}       // namespace Plus4Emu

