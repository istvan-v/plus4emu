
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/plus4emu/
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
#  include <wingdi.h>
#  if defined(_MSC_VER) && !defined(__GNUC__)
typedef void (APIENTRY *PFNGLBLENDCOLORPROC)(GLclampf, GLclampf, GLclampf,
                                             GLclampf);
#  endif
#endif

#ifndef WIN32
#  define   glBlendColor_         glBlendColor
#  ifdef ENABLE_GL_SHADERS
#    define glAttachShader_       glAttachShader
#    define glCompileShader_      glCompileShader
#    define glCreateProgram_      glCreateProgram
#    define glCreateShader_       glCreateShader
#    define glDeleteProgram_      glDeleteProgram
#    define glDeleteShader_       glDeleteShader
#    define glDetachShader_       glDetachShader
#    define glGetShaderiv_        glGetShaderiv
#    define glGetUniformLocation_ glGetUniformLocation
#    define glLinkProgram_        glLinkProgram
#    define glShaderSource_       glShaderSource
#    define glUniform1f_          glUniform1f
#    define glUniform1i_          glUniform1i
#    define glUseProgram_         glUseProgram
#  endif
#else
static PFNGLBLENDCOLORPROC      glBlendColor__ = (PFNGLBLENDCOLORPROC) 0;
static inline void glBlendColor_(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
  if (glBlendColor__)
    glBlendColor__(r, g, b, a);
  else
    glDisable(GL_BLEND);
}
#  ifdef ENABLE_GL_SHADERS
static volatile bool  haveGLShaderFuncs = false;
static PFNGLATTACHSHADERPROC    glAttachShader_ = (PFNGLATTACHSHADERPROC) 0;
static PFNGLCOMPILESHADERPROC   glCompileShader_ = (PFNGLCOMPILESHADERPROC) 0;
static PFNGLCREATEPROGRAMPROC   glCreateProgram_ = (PFNGLCREATEPROGRAMPROC) 0;
static PFNGLCREATESHADERPROC    glCreateShader_ = (PFNGLCREATESHADERPROC) 0;
static PFNGLDELETEPROGRAMPROC   glDeleteProgram_ = (PFNGLDELETEPROGRAMPROC) 0;
static PFNGLDELETESHADERPROC    glDeleteShader_ = (PFNGLDELETESHADERPROC) 0;
static PFNGLDETACHSHADERPROC    glDetachShader_ = (PFNGLDETACHSHADERPROC) 0;
static PFNGLGETSHADERIVPROC     glGetShaderiv_ = (PFNGLGETSHADERIVPROC) 0;
static PFNGLGETUNIFORMLOCATIONPROC  glGetUniformLocation_ =
                                        (PFNGLGETUNIFORMLOCATIONPROC) 0;
static PFNGLLINKPROGRAMPROC     glLinkProgram_ = (PFNGLLINKPROGRAMPROC) 0;
static PFNGLSHADERSOURCEPROC    glShaderSource_ = (PFNGLSHADERSOURCEPROC) 0;
static PFNGLUNIFORM1FPROC       glUniform1f_ = (PFNGLUNIFORM1FPROC) 0;
static PFNGLUNIFORM1IPROC       glUniform1i_ = (PFNGLUNIFORM1IPROC) 0;
static PFNGLUSEPROGRAMPROC      glUseProgram_ = (PFNGLUSEPROGRAMPROC) 0;
static bool queryGLShaderFunctions()
{
  if (!haveGLShaderFuncs) {
    glAttachShader_ =
        (PFNGLATTACHSHADERPROC) wglGetProcAddress("glAttachShader");
    if (!glAttachShader_)
      return false;
    glCompileShader_ =
        (PFNGLCOMPILESHADERPROC) wglGetProcAddress("glCompileShader");
    if (!glCompileShader_)
      return false;
    glCreateProgram_ =
        (PFNGLCREATEPROGRAMPROC) wglGetProcAddress("glCreateProgram");
    if (!glCreateProgram_)
      return false;
    glCreateShader_ =
        (PFNGLCREATESHADERPROC) wglGetProcAddress("glCreateShader");
    if (!glCreateShader_)
      return false;
    glDeleteProgram_ =
        (PFNGLDELETEPROGRAMPROC) wglGetProcAddress("glDeleteProgram");
    if (!glDeleteProgram_)
      return false;
    glDeleteShader_ =
        (PFNGLDELETESHADERPROC) wglGetProcAddress("glDeleteShader");
    if (!glDeleteShader_)
      return false;
    glDetachShader_ =
        (PFNGLDETACHSHADERPROC) wglGetProcAddress("glDetachShader");
    if (!glDetachShader_)
      return false;
    glGetShaderiv_ =
        (PFNGLGETSHADERIVPROC) wglGetProcAddress("glGetShaderiv");
    if (!glGetShaderiv_)
      return false;
    glGetUniformLocation_ =
        (PFNGLGETUNIFORMLOCATIONPROC) wglGetProcAddress("glGetUniformLocation");
    if (!glGetUniformLocation_)
      return false;
    glLinkProgram_ =
        (PFNGLLINKPROGRAMPROC) wglGetProcAddress("glLinkProgram");
    if (!glLinkProgram_)
      return false;
    glShaderSource_ =
        (PFNGLSHADERSOURCEPROC) wglGetProcAddress("glShaderSource");
    if (!glShaderSource_)
      return false;
    glUniform1f_ =
        (PFNGLUNIFORM1FPROC) wglGetProcAddress("glUniform1f");
    if (!glUniform1f_)
      return false;
    glUniform1i_ =
        (PFNGLUNIFORM1IPROC) wglGetProcAddress("glUniform1i");
    if (!glUniform1i_)
      return false;
    glUseProgram_ =
        (PFNGLUSEPROGRAMPROC) wglGetProcAddress("glUseProgram");
    if (!glUseProgram_)
      return false;
    haveGLShaderFuncs = true;
  }
  return haveGLShaderFuncs;
}
#  endif
#endif

#ifdef ENABLE_GL_SHADERS

static const char *shaderSourcePAL[1] = {
  "uniform sampler2D textureHandle;\n"
  "uniform float lineShade;\n"
  "const mat4 yuv2rgbMatrix = mat4( 0.50000,  0.00000,  0.18236, -0.70100,\n"
  "                                 0.50000, -0.04476, -0.09289,  0.52914,\n"
  "                                 0.50000,  0.23049,  0.00000, -0.88600,\n"
  "                                 0.00000,  0.00000,  0.00000,  0.00000);\n"
  "void main()\n"
  "{\n"
  "  float txc = gl_TexCoord[0][0];\n"
  "  float tyc = gl_TexCoord[0][1];\n"
  "  float txcm1 = txc - 0.00296;\n"
  "  float txcm0 = txc - 0.00085;\n"
  "  float txcp0 = txc + 0.00085;\n"
  "  float txcp1 = txc + 0.00296;\n"
  "  float tyc0 = tyc + 0.015625;\n"
  "  float tyc1 = tyc - 0.046875;\n"
  "  vec4 p00 =   texture2D(textureHandle, vec2(txcm0, tyc0))\n"
  "             + texture2D(textureHandle, vec2(txcp0, tyc0));\n"
  "  vec4 p10 =   texture2D(textureHandle, vec2(txcm0, tyc1))\n"
  "             + texture2D(textureHandle, vec2(txcp0, tyc1));\n"
  "  vec4 p01 =   texture2D(textureHandle, vec2(txcm1, tyc0))\n"
  "             + texture2D(textureHandle, vec2(txcp1, tyc0))\n"
  "             + texture2D(textureHandle, vec2(txcm1, tyc1))\n"
  "             + texture2D(textureHandle, vec2(txcp1, tyc1));\n"
  "  float f = mix(sin(tyc * 100.531) * 0.5 + 0.5, 1.0, lineShade);\n"
  "  vec4 tmp = (p00 + p10) + (p01 * 0.922);\n"
  "  gl_FragColor = (vec4(p00[0], tmp[1], tmp[2], 1.0) * yuv2rgbMatrix) * f;\n"
  "}\n"
};

static const char *shaderSourceNTSC[1] = {
  "uniform sampler2D textureHandle;\n"
  "uniform float lineShade;\n"
  "const mat4 yuv2rgbMatrix = mat4( 0.50000,  0.00000,  0.37387, -0.70100,\n"
  "                                 0.50000, -0.09177, -0.19044,  0.52914,\n"
  "                                 0.50000,  0.47253,  0.00000, -0.88600,\n"
  "                                 0.00000,  0.00000,  0.00000,  0.00000);\n"
  "void main()\n"
  "{\n"
  "  float txc = gl_TexCoord[0][0];\n"
  "  float tyc = gl_TexCoord[0][1];\n"
  "  float txcm1 = txc - 0.00317;\n"
  "  float txcm0 = txc - 0.00095;\n"
  "  float txcp0 = txc + 0.00095;\n"
  "  float txcp1 = txc + 0.00317;\n"
  "  float tyc0 = tyc + 0.015625;\n"
  "  vec4 p00 =   texture2D(textureHandle, vec2(txcm0, tyc0))\n"
  "             + texture2D(textureHandle, vec2(txcp0, tyc0));\n"
  "  vec4 p01 =   texture2D(textureHandle, vec2(txcm1, tyc0))\n"
  "             + texture2D(textureHandle, vec2(txcp1, tyc0));\n"
  "  float f = mix(sin(tyc * 100.531) * 0.5 + 0.5, 1.0, lineShade);\n"
  "  vec4 tmp = p00 + (p01 * 0.875);\n"
  "  gl_FragColor = (vec4(p00[0], tmp[1], tmp[2], 1.0) * yuv2rgbMatrix) * f;\n"
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
  if (dp.displayQuality < 2) {
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
#  ifdef WIN32
    if (!queryGLShaderFunctions())
      return false;
#  endif
    shaderHandle = glCreateShader_(GL_FRAGMENT_SHADER);
    if (!shaderHandle)
      return false;
    programHandle = glCreateProgram_();
    if (!programHandle) {
      glDeleteShader_(GLuint(shaderHandle));
      shaderHandle = 0UL;
      return false;
    }
    if (shaderMode_ == 1) {
      glShaderSource_(GLuint(shaderHandle),
                      GLsizei(1), &(shaderSourcePAL[0]), (GLint *) 0);
    }
    else {
      glShaderSource_(GLuint(shaderHandle),
                      GLsizei(1), &(shaderSourceNTSC[0]), (GLint *) 0);
    }
    glAttachShader_(GLuint(programHandle), GLuint(shaderHandle));
    shaderMode = shaderMode_;
    glCompileShader_(GLuint(shaderHandle));
    GLint   compileStatus = GL_FALSE;
    glGetShaderiv_(GLuint(shaderHandle), GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
      deleteShader();
      return false;
    }
    glLinkProgram_(GLuint(programHandle));
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
#  ifdef WIN32
    if (!queryGLShaderFunctions())
      return;
#  endif
    shaderMode = 0;
    glDetachShader_(GLuint(programHandle), GLuint(shaderHandle));
    glDeleteProgram_(GLuint(programHandle));
    programHandle = 0UL;
    glDeleteShader_(GLuint(shaderHandle));
    shaderHandle = 0UL;
#endif  // ENABLE_GL_SHADERS
  }

  bool OpenGLDisplay::enableShader()
  {
#ifdef ENABLE_GL_SHADERS
    if (shaderMode == 0)
      return false;
#  ifdef WIN32
    if (!queryGLShaderFunctions())
      return false;
#  endif
    glUseProgram_(GLuint(programHandle));
    // FIXME: is it safe to use a constant texture ID of 0 here ?
    glUniform1i_(glGetUniformLocation_(GLuint(programHandle), "textureHandle"),
                 0);
    glUniform1f_(glGetUniformLocation_(GLuint(programHandle), "lineShade"),
                 displayParameters.lineShade * 0.998f + 0.001f);
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
#  ifdef WIN32
    if (!queryGLShaderFunctions())
      return;
#  endif
    glUseProgram_(GLuint(0));
#endif  // ENABLE_GL_SHADERS
  }

  OpenGLDisplay::OpenGLDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl, bool isDoubleBuffered)
    : Fl_Gl_Window(xx, yy, ww, hh, lbl),
      FLTKDisplay_(),
      colormap16(),
      colormap32_0(),
      colormap32_1(),
      linesChanged((bool *) 0),
      textureSpace((unsigned char *) 0),
      textureBuffer16((uint16_t *) 0),
      textureBuffer32((uint32_t *) 0),
      textureID(0UL),
      forceUpdateLineCnt(0),
      forceUpdateLineMask(0),
      redrawFlag(false),
      yuvTextureMode(false),
      prvFrameWasOdd(false),
      lastLineNum(-2),
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

  void OpenGLDisplay::setColormap_quality3(
      const VideoDisplay::DisplayParameters& dp)
  {
    VideoDisplay::DisplayParameters dp0(dp);
    VideoDisplay::DisplayParameters dp1(dp);
    if (yuvTextureMode && !dp.ntscMode) {
      dp0.hueShift -= dp.palPhaseError;
      dp0.hueShift =
          (dp0.hueShift >= -180.0f ?
           (dp0.hueShift < 180.0f ? dp0.hueShift : (dp0.hueShift - 360.0f))
           : (dp0.hueShift + 360.0f));
      dp1.hueShift += dp.palPhaseError;
      dp1.hueShift =
          (dp1.hueShift >= -180.0f ?
           (dp1.hueShift < 180.0f ? dp1.hueShift : (dp1.hueShift - 360.0f))
           : (dp1.hueShift + 360.0f));
    }
    colormap32_0.setDisplayParameters(dp0, yuvTextureMode);
    colormap32_1.setDisplayParameters(dp1, yuvTextureMode);
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
    uint8_t videoFlags = uint8_t(((~lineNum) & 2) | ((l->flags & 0x80) >> 2));
    size_t  pixelSample2 = l->lineLength;
    l->getLineData(bufp, nBytes);
    if (displayParameters.ntscMode)
      videoFlags = videoFlags | 0x10;
    if (pixelSample2 == (displayParameters.ntscMode ? 392 : 490) &&
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
      size_t  pixelSample1 = 490;
      size_t  pixelSampleCnt = 0;
      uint8_t readPos = 4;
      do {
        if (readPos >= 4) {
          readPos = readPos & 3;
          pixelSample1 = ((bufp[bufPos] & 0x01) ? 392 : 490);
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

  void OpenGLDisplay::decodeLine_quality3(
      uint32_t *outBuf, Message_LineData **lineBuffers_, int lineNum,
      const VideoDisplayColormap<uint32_t>& colormap)
  {
    Message_LineData  *l = (Message_LineData *) 0;
    if (lineNum >= 0 && lineNum < 578) {
      if (lineBuffers_[lineNum] != (Message_LineData *) 0)
        l = lineBuffers_[lineNum];
      else
        l = lineBuffers_[lineNum ^ 1];
    }
    if (!l) {
      if (yuvTextureMode) {
        for (size_t xc = 0; xc < 768; xc++)
          outBuf[xc] = 0x00808000U;
      }
      else {
        for (size_t xc = 0; xc < 768; xc++)
          outBuf[xc] = 0x00000000U;
      }
      return;
    }
    const unsigned char *bufp = (unsigned char *) 0;
    size_t  nBytes = 0;
    size_t  bufPos = 0;
    size_t  xc = 0;
    uint8_t videoFlags = uint8_t(((~lineNum) & 2) | ((l->flags & 0x80) >> 2));
    size_t  pixelSample2 = l->lineLength;
    l->getLineData(bufp, nBytes);
    if (displayParameters.ntscMode)
      videoFlags = videoFlags | 0x10;
    if (pixelSample2 == (displayParameters.ntscMode ? 392 : 490) &&
        !(l->flags & 0x01)) {
      do {
        size_t  n = colormap.convertFourToEightPixels(&(outBuf[xc]),
                                                      &(bufp[bufPos]),
                                                      videoFlags);
        bufPos = bufPos + n;
        xc = xc + 8;
      } while (xc < 768);
    }
    else {
      uint32_t  tmpBuf[4];
      size_t  pixelSample1 = 980;
      size_t  pixelSampleCnt = 0;
      uint8_t readPos = 4;
      do {
        if (readPos >= 4) {
          readPos = readPos & 3;
          pixelSample1 = ((bufp[bufPos] & 0x01) ? 784 : 980);
          size_t  n = colormap.convertFourPixels(&(tmpBuf[0]),
                                                 &(bufp[bufPos]), videoFlags);
          bufPos += n;
        }
        outBuf[xc] = tmpBuf[readPos];
        pixelSampleCnt += pixelSample2;
        if (pixelSampleCnt >= pixelSample1) {
          pixelSampleCnt -= pixelSample1;
          readPos++;
        }
      } while (++xc < 768);
    }
  }

  void OpenGLDisplay::drawFrame_quality0(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1, bool oddFrame_)
  {
    // half horizontal resolution, no interlace (384x288)
    // no texture filtering or effects
    for (size_t yc = 0; yc < 288; yc += 8) {
      for (size_t offs = 0; offs < 8; offs++) {
        // decode video data, and build 16-bit texture
        decodeLine_quality0(&(textureBuffer16[offs * 384]), lineBuffers_,
                            ((yc + offs + 1) << 1) + size_t(oddFrame_));
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
                                         double x1, double y1, bool oddFrame_)
  {
    GLfloat txtycf0 = GLfloat(1.0 / 16.0);
    GLfloat txtycf1 = GLfloat(15.0 / 16.0);
    if (oddFrame_) {
      // interlace
      txtycf0 -= GLfloat(0.5 / 16.0);
      txtycf1 -= GLfloat(0.5 / 16.0);
    }
    // half horizontal resolution, no interlace (384x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      if (yc > 0) {
        std::memcpy(&(textureBuffer16[0]), &(textureBuffer16[14 * 384]),
                    sizeof(uint16_t) * size_t(2 * 384));
      }
      // decode video data, and build 16-bit texture
      for (size_t offs = size_t(oddFrame_) + (yc > 0 ? 4 : 0);
           offs < 32;
           offs += 2) {
        decodeLine_quality0(&(textureBuffer16[(offs >> 1) * 384]),
                            lineBuffers_, yc + offs);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 16,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, textureSpace);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0)) * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0)) * (y1 - y0));
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= GLfloat(6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), txtycf0);
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), txtycf0);
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(384.0 / 512.0), txtycf1);
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), txtycf1);
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality2(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1, bool oddFrame_)
  {
    GLfloat txtycf0 = GLfloat(1.0 / 16.0);
    GLfloat txtycf1 = GLfloat(15.0 / 16.0);
    if (oddFrame_) {
      // interlace
      txtycf0 -= GLfloat(0.5 / 16.0);
      txtycf1 -= GLfloat(0.5 / 16.0);
    }
    // full horizontal resolution, no interlace (768x288)
    for (size_t yc = 0; yc < 588; yc += 28) {
      if (yc > 0) {
        std::memcpy(&(textureBuffer32[0]), &(textureBuffer32[14 * 768]),
                    sizeof(uint32_t) * size_t(2 * 768));
      }
      // decode video data, and build 32-bit texture
      for (size_t offs = size_t(oddFrame_) + (yc > 0 ? 4 : 0);
           offs < 32;
           offs += 2) {
        decodeLine_quality3(&(textureBuffer32[(offs >> 1) * 768]),
                            lineBuffers_, yc + offs, colormap32_0);
      }
      // load texture
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 768, 16,
                      GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, textureSpace);
      // update display
      double  ycf0 = y0 + ((double(int(yc)) * (1.0 / 576.0)) * (y1 - y0));
      double  ycf1 = y0 + ((double(int(yc + 28)) * (1.0 / 576.0)) * (y1 - y0));
      if (yc == 560) {
        ycf1 -= ((y1 - y0) * (12.0 / 576.0));
        txtycf1 -= GLfloat(6.0 / 16.0);
      }
      glBegin(GL_QUADS);
      glTexCoord2f(GLfloat(0.0), txtycf0);
      glVertex2f(GLfloat(x0), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), txtycf0);
      glVertex2f(GLfloat(x1), GLfloat(ycf0));
      glTexCoord2f(GLfloat(768.0 / 1024.0), txtycf1);
      glVertex2f(GLfloat(x1), GLfloat(ycf1));
      glTexCoord2f(GLfloat(0.0), txtycf1);
      glVertex2f(GLfloat(x0), GLfloat(ycf1));
      glEnd();
    }
  }

  void OpenGLDisplay::drawFrame_quality3(Message_LineData **lineBuffers_,
                                         double x0, double y0,
                                         double x1, double y1, bool oddFrame_)
  {
    if (shaderMode != (displayParameters.ntscMode ? 2 : 1))
      compileShader(displayParameters.ntscMode ? 2 : 1);
    if (enableShader() != yuvTextureMode) {
      yuvTextureMode = !yuvTextureMode;
      setColormap_quality3(displayParameters);
    }
    double  yOffs = (y1 - y0) * (-2.0 / 576.0);
    // full horizontal resolution, interlace (768x576), TV emulation
    for (int yc = int(oddFrame_) - 4; yc < 594; yc += 26) {
      if (yc > 0) {
        std::memcpy(&(textureBuffer32[0]), &(textureBuffer32[13 * 768]),
                    sizeof(uint32_t) * size_t(3 * 768));
      }
      // decode video data, and build 32-bit texture
      for (int offs = (yc > 0 ? 6 : 0); offs < 32; offs += 2) {
        decodeLine_quality3(&(textureBuffer32[(offs >> 1) * 768]),
                            lineBuffers_, yc + offs,
                            (((yc + offs) & 2) ? colormap32_1 : colormap32_0));
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
      if (yc >= 568) {
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
                             * double(displayParameters.pixelAspectRatio));
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
#ifdef WIN32
    if (!glBlendColor__)
      glBlendColor__ = (PFNGLBLENDCOLORPROC) wglGetProcAddress("glBlendColor");
#endif

    if (displayParameters.displayQuality == 0 &&
        displayParameters.bufferingMode == 0) {
      // half horizontal resolution, no interlace (384x288)
      // no texture filtering or effects
      if (forceUpdateLineMask) {
        // make sure that all lines are updated at a slow rate
        for (size_t yc = 0; yc < 289; yc++) {
          if (forceUpdateLineMask & (uint8_t(1) << uint8_t((yc >> 3) & 7)))
            linesChanged[yc] = true;
        }
        forceUpdateLineMask = 0;
      }
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
          decodeLine_quality0(&(textureBuffer16[offs * 384]), lineBuffers,
                              ((yc + offs + 1) << 1) + size_t(prvFrameWasOdd));
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
    }
    else if (displayParameters.bufferingMode != 2) {
      float   blendScale3 = displayParameters.motionBlur;
      float   blendScale2 = (1.0f - blendScale3) * displayParameters.blendScale;
      if (!(displayParameters.bufferingMode != 0 ||
            (blendScale2 > 0.99f && blendScale2 < 1.01f &&
             blendScale3 < 0.01f))) {
        glEnable(GL_BLEND);
        glBlendColor_(GLclampf(blendScale2),
                      GLclampf(blendScale2),
                      GLclampf(blendScale2),
                      GLclampf(1.0f - blendScale3));
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_ALPHA);
      }
      else
        glDisable(GL_BLEND);
      switch (displayParameters.displayQuality) {
      case 0:
        // half horizontal resolution, no interlace (384x288)
        // no texture filtering or effects
        drawFrame_quality0(lineBuffers, x0, y0, x1, y1, prvFrameWasOdd);
        break;
      case 1:
        // half horizontal resolution, no interlace (384x288)
        drawFrame_quality1(lineBuffers, x0, y0, x1, y1, prvFrameWasOdd);
        break;
      case 2:
        // full horizontal resolution, no interlace (768x288)
        drawFrame_quality2(lineBuffers, x0, y0, x1, y1, prvFrameWasOdd);
        break;
      case 3:
        // full horizontal resolution, interlace (768x576), TV emulation
        drawFrame_quality3(lineBuffers, x0, y0, x1, y1, prvFrameWasOdd);
        break;
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
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
      if (readPosFrac <= 0.998) {
        glDisable(GL_BLEND);
        Message_LineData  **lineBuffers_ = frameRingBuffer[readPosInt & 3];
        bool    oddFrame_ = (lineBuffers_[100] == (Message_LineData *) 0 &&
                             lineBuffers_[101] != (Message_LineData *) 0);
        switch (displayParameters.displayQuality) {
        case 0:
          // half horizontal resolution, no interlace (384x288)
          // no texture filtering or effects
          drawFrame_quality0(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576), TV emulation
          drawFrame_quality3(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        }
      }
      if (readPosFrac >= 0.002) {
        if (readPosFrac <= 0.9975) {
          glEnable(GL_BLEND);
          glBlendColor_(GLclampf(1.0), GLclampf(1.0), GLclampf(1.0),
                        GLclampf(readPosFrac));
          glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
        }
        else
          glDisable(GL_BLEND);
        Message_LineData  **lineBuffers_ =
            frameRingBuffer[(readPosInt + 1) & 3];
        bool    oddFrame_ = (lineBuffers_[100] == (Message_LineData *) 0 &&
                             lineBuffers_[101] != (Message_LineData *) 0);
        switch (displayParameters.displayQuality) {
        case 0:
          // half horizontal resolution, no interlace (384x288)
          // no texture filtering or effects
          drawFrame_quality0(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 1:
          // half horizontal resolution, no interlace (384x288)
          drawFrame_quality1(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 2:
          // full horizontal resolution, no interlace (768x288)
          drawFrame_quality2(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        case 3:
          // full horizontal resolution, interlace (768x576), TV emulation
          drawFrame_quality3(lineBuffers_, x0, y0, x1, y1, oddFrame_);
          break;
        }
      }
      // clean up
      glBindTexture(GL_TEXTURE_2D, GLuint(savedTextureID));
      glPopMatrix();
      glFlush();
    }
  }

  void OpenGLDisplay::initializeGLDisplay()
  {
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

  void OpenGLDisplay::copyFrameToRingBuffer()
  {
    Message_LineData  **lineBuffers_ = frameRingBuffer[ringBufferWritePos];
    try {
      for (size_t yc = 0; yc < 578; yc++) {
        Message_LineData  *m = lineBuffers_[yc];
        lineBuffers_[yc] = (Message_LineData *) 0;
        if ((yc & 1) == size_t(prvFrameWasOdd) &&
            lineBuffers[yc] != (Message_LineData *) 0) {
          if (!m)
            m = allocateMessage< Message_LineData >();
          *m = *(lineBuffers[yc]);
          lineBuffers_[yc] = m;
          m = (Message_LineData *) 0;
        }
        if (m)
          deleteMessage(m);
      }
    }
    catch (std::exception) {
      for (size_t yc = 0; yc < 578; yc++) {
        if (lineBuffers_[yc]) {
          deleteMessage(lineBuffers_[yc]);
          lineBuffers_[yc] = (Message_LineData *) 0;
        }
      }
    }
    ringBufferWritePos = (ringBufferWritePos + 1) & 3;
  }

  void OpenGLDisplay::draw()
  {
    if (!textureID)
      initializeGLDisplay();
    if (this->damage() & FL_DAMAGE_EXPOSE) {
      forceUpdateLineMask = 0xFF;
      forceUpdateLineCnt = 0;
      forceUpdateTimer.reset();
      redrawFlag = true;
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
        Message_LineData  *msg = static_cast<Message_LineData *>(m);
        int     lineNum = msg->lineNum;
        if (lineNum >= lineReload) {
          lastLineNum = lineNum;
          if ((lineNum & 1) == int(prvFrameWasOdd) &&
              lineBuffers[lineNum ^ 1] != (Message_LineData *) 0) {
            // non-interlaced mode: clear any old lines in the other field
            deleteMessage(lineBuffers[lineNum ^ 1]);
            lineBuffers[lineNum ^ 1] = (Message_LineData *) 0;
          }
          if (displayParameters.displayQuality == 0) {
            if (!displayParameters.bufferingMode) {
              // check if this line has changed
              int     lineNum_ = (lineNum & (~(int(1)))) | int(prvFrameWasOdd);
              if (lineBuffers[lineNum_] != (Message_LineData *) 0 &&
                  *(lineBuffers[lineNum_]) == *msg) {
                if (lineNum == lineNum_) {
                  deleteMessage(m);
                  continue;
                }
              }
              else {
                linesChanged[lineNum >> 1] = true;
              }
            }
          }
          if (lineBuffers[lineNum])
            deleteMessage(lineBuffers[lineNum]);
          lineBuffers[lineNum] = msg;
          continue;
        }
      }
      else if (typeid(*m) == typeid(Message_FrameDone)) {
        // need to update display
        messageQueueMutex.lock();
        framesPending = (framesPending > 0 ? (framesPending - 1) : 0);
        framesPendingFlag = (framesPending > 0);
        messageQueueMutex.unlock();
        redrawFlag = true;
        deleteMessage(m);
        int     yc = lastLineNum;
        prvFrameWasOdd = bool(yc & 1);
        lastLineNum = (yc & 1) - 2;
        if (yc < 576) {
          // clear any remaining lines
          yc = yc | 1;
          do {
            yc++;
            if (lineBuffers[yc]) {
              linesChanged[yc >> 1] = true;
              deleteMessage(lineBuffers[yc]);
              lineBuffers[yc] = (Message_LineData *) 0;
            }
          } while (yc < 577);
        }
        noInputTimer.reset();
        if (screenshotCallbackFlag)
          checkScreenshotCallback();
        if (videoResampleEnabled) {
          double  t = inputFrameRateTimer.getRealTime();
          inputFrameRateTimer.reset();
          t = (t > 0.002 ? (t < 0.25 ? t : 0.25) : 0.002);
          inputFrameRate = 1.0 / ((0.97 / inputFrameRate) + (0.03 * t));
          if (ringBufferWritePos != int(ringBufferReadPos)) {
            // if buffer is not already full, copy current frame
            copyFrameToRingBuffer();
            continue;
          }
        }
        break;
      }
      else if (typeid(*m) == typeid(Message_SetParameters)) {
        Message_SetParameters *msg;
        msg = static_cast<Message_SetParameters *>(m);
        if (msg->dp.displayQuality < 2) {
          yuvTextureMode = false;
          colormap16.setDisplayParameters(msg->dp, false);
        }
        else if (msg->dp.displayQuality == 2) {
          yuvTextureMode = false;
          colormap32_0.setDisplayParameters(msg->dp, false);
        }
        else {
          yuvTextureMode = true;
          setColormap_quality3(msg->dp);
        }
        if (displayParameters.displayQuality != msg->dp.displayQuality ||
            displayParameters.bufferingMode != msg->dp.bufferingMode) {
          Fl::remove_idle(&fltkIdleCallback, (void *) this);
          if (displayParameters.bufferingMode != msg->dp.bufferingMode ||
              displayParameters.displayQuality == 3 ||
              msg->dp.displayQuality == 3) {
            // if TV emulation (quality=3) or double buffering mode
            // has changed, also need to generate a new texture ID
            deleteShader();
#ifdef WIN32
            glBlendColor__ = (PFNGLBLENDCOLORPROC) 0;
#  ifdef ENABLE_GL_SHADERS
            haveGLShaderFuncs = false;
#  endif
#endif
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
          if (msg->dp.displayQuality < 2) {
            std::memset(textureBuffer16, 0, sizeof(uint16_t) * 1024 * 16);
          }
          else if (!yuvTextureMode) {
            for (size_t n = 0; n < (1024 * 16); n++)
              textureBuffer32[n] = 0x00000000U;
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
        for (size_t yc = 0; yc < 289; yc++)
          linesChanged[yc] = true;
      }
      deleteMessage(m);
    }
    if (noInputTimer.getRealTime() > 0.5) {
      noInputTimer.reset(0.25);
      if (videoResampleEnabled)
        copyFrameToRingBuffer();
      redrawFlag = true;
      if (screenshotCallbackFlag)
        checkScreenshotCallback();
    }
    if (forceUpdateTimer.getRealTime() >= 0.15) {
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

