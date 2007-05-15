# vim: syntax=python

import sys

win32CrossCompile = 0
disableSDL = 0          # set this to 1 on Linux with SDL version >= 1.2.10

compilerFlags = Split('''
    -Wall -W -ansi -pedantic -Wno-long-long -O2
''')

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

plus4emuLibEnvironment = Environment()
plus4emuLibEnvironment.Append(CCFLAGS = compilerFlags)
plus4emuLibEnvironment.Append(CPPPATH = ['.', './src', '/usr/local/include'])
plus4emuLibEnvironment.Append(LINKFLAGS = ['-L.'])
if win32CrossCompile:
    plus4emuLibEnvironment['AR'] = 'wine D:/MinGW/bin/ar.exe'
    plus4emuLibEnvironment['CC'] = 'wine D:/MinGW/bin/gcc.exe'
    plus4emuLibEnvironment['CPP'] = 'wine D:/MinGW/bin/cpp.exe'
    plus4emuLibEnvironment['CXX'] = 'wine D:/MinGW/bin/g++.exe'
    plus4emuLibEnvironment['LINK'] = 'wine D:/MinGW/bin/g++.exe'
    plus4emuLibEnvironment['RANLIB'] = 'wine D:/MinGW/bin/ranlib.exe'
    plus4emuLibEnvironment.Append(LIBS = ['ole32', 'uuid', 'ws2_32',
                                          'gdi32', 'user32', 'kernel32'])
    plus4emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    plus4emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])

plus4emuGUIEnvironment = plus4emuLibEnvironment.Copy()
if win32CrossCompile:
    plus4emuGUIEnvironment.Prepend(LIBS = ['fltk'])
elif not plus4emuGUIEnvironment.ParseConfig(
        '%s --cxxflags --ldflags --libs' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    plus4emuGUIEnvironment.Append(LIBS = ['fltk'])

plus4emuGLGUIEnvironment = plus4emuLibEnvironment.Copy()
if win32CrossCompile:
    plus4emuGLGUIEnvironment.Prepend(LIBS = ['fltk_gl', 'fltk',
                                             'glu32', 'opengl32'])
elif not plus4emuGLGUIEnvironment.ParseConfig(
        '%s --use-gl --cxxflags --ldflags --libs' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    plus4emuGLGUIEnvironment.Append(LIBS = ['fltk_gl', 'GL'])

plus4emuLibEnvironment['CPPPATH'] = plus4emuGLGUIEnvironment['CPPPATH']

configure = plus4emuLibEnvironment.Configure()
if not configure.CheckCHeader('sndfile.h'):
    print ' *** error: libsndfile 1.0 is not found'
    Exit(-1)
if not configure.CheckCHeader('portaudio.h'):
    print ' *** error: PortAudio is not found'
    Exit(-1)
elif configure.CheckType('PaStreamCallbackTimeInfo', '#include <portaudio.h>'):
    havePortAudioV19 = 1
else:
    havePortAudioV19 = 0
    print 'WARNING: using old v18 PortAudio interface'
if not configure.CheckCXXHeader('FL/Fl.H'):
    if configure.CheckCXXHeader('/usr/include/fltk-1.1/FL/Fl.H'):
        plus4emuLibEnvironment.Append(CPPPATH = ['/usr/include/fltk-1.1'])
    else:
        print ' *** error: FLTK 1.1 is not found'
    Exit(-1)
if not configure.CheckCHeader('GL/gl.h'):
    print ' *** error: OpenGL is not found'
    Exit(-1)
haveDotconf = configure.CheckCHeader('dotconf.h')
if configure.CheckCHeader('stdint.h'):
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_STDINT_H'])
if not disableSDL:
    haveSDL = configure.CheckCHeader('SDL/SDL.h')
else:
    haveSDL = 0
configure.Finish()

if not havePortAudioV19:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveDotconf:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_DOTCONF_H'])
if haveSDL:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_SDL_H'])

plus4emuGUIEnvironment['CCFLAGS'] = plus4emuLibEnvironment['CCFLAGS']
plus4emuGUIEnvironment['CXXFLAGS'] = plus4emuLibEnvironment['CXXFLAGS']
plus4emuGLGUIEnvironment['CCFLAGS'] = plus4emuLibEnvironment['CCFLAGS']
plus4emuGLGUIEnvironment['CXXFLAGS'] = plus4emuLibEnvironment['CXXFLAGS']

def fluidCompile(flNames):
    cppNames = []
    for flName in flNames:
        if flName.endswith('.fl'):
            cppName = flName[:-3] + '_fl.cpp'
            hppName = flName[:-3] + '_fl.hpp'
            Command([cppName, hppName], flName,
                    'fluid -c -o %s -h %s $SOURCES' % (cppName, hppName))
            cppNames += [cppName]
    return cppNames

plus4emuLib = plus4emuLibEnvironment.StaticLibrary('plus4emu', Split('''
    src/bplist.cpp
    src/cfg_db.cpp
    src/cia8520.cpp
    src/cpu.cpp
    src/cpuoptbl.cpp
    src/disasm.cpp
    src/display.cpp
    src/emucfg.cpp
    src/fileio.cpp
    src/fldisp.cpp
    src/gldisp.cpp
    src/joystick.cpp
    src/memory.cpp
    src/plus4vm.cpp
    src/render.cpp
    src/snd_conv.cpp
    src/soundio.cpp
    src/system.cpp
    src/tape.cpp
    src/ted_api.cpp
    src/ted_init.cpp
    src/ted_main.cpp
    src/ted_read.cpp
    src/ted_write.cpp
    src/vc1541.cpp
    src/vc1551.cpp
    src/vc1581.cpp
    src/via6522.cpp
    src/vm.cpp
    src/vmthread.cpp
    src/wd177x.cpp
'''))

# -----------------------------------------------------------------------------

residLibEnvironment = plus4emuLibEnvironment.Copy()
residLibEnvironment.Append(CPPPATH = ['./resid'])

residLib = residLibEnvironment.StaticLibrary('resid', Split('''
    resid/envelope.cpp
    resid/extfilt.cpp
    resid/filter.cpp
    resid/pot.cpp
    resid/sid.cpp
    resid/version.cpp
    resid/voice.cpp
    resid/wave6581_PS_.cpp
    resid/wave6581_PST.cpp
    resid/wave6581_P_T.cpp
    resid/wave6581__ST.cpp
    resid/wave8580_PS_.cpp
    resid/wave8580_PST.cpp
    resid/wave8580_P_T.cpp
    resid/wave8580__ST.cpp
    resid/wave.cpp
'''))

# -----------------------------------------------------------------------------

plus4emuEnvironment = plus4emuGLGUIEnvironment.Copy()
plus4emuEnvironment.Append(CPPPATH = ['./gui'])
plus4emuEnvironment.Prepend(LIBS = ['plus4emu', 'resid'])
if haveDotconf:
    plus4emuEnvironment.Append(LIBS = ['dotconf'])
if haveSDL:
    plus4emuEnvironment.Append(LIBS = ['SDL'])
plus4emuEnvironment.Append(LIBS = ['portaudio', 'sndfile'])
if not win32CrossCompile:
    if sys.platform[:5] == 'linux':
        plus4emuEnvironment.Append(LIBS = ['jack', 'asound', 'pthread', 'rt'])
    else:
        plus4emuEnvironment.Append(LIBS = ['pthread'])
else:
    plus4emuEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

plus4emu = plus4emuEnvironment.Program('plus4emu',
    ['gui/gui.cpp']
    + fluidCompile(['gui/gui.fl', 'gui/disk_cfg.fl', 'gui/disp_cfg.fl',
                    'gui/kbd_cfg.fl', 'gui/snd_cfg.fl', 'gui/vm_cfg.fl',
                    'gui/debug.fl', 'gui/about.fl'])
    + ['gui/main.cpp'])
Depends(plus4emu, plus4emuLib)
Depends(plus4emu, residLib)

# -----------------------------------------------------------------------------

tapconvEnvironment = plus4emuLibEnvironment.Copy()
tapconvEnvironment.Prepend(LIBS = ['plus4emu'])
tapconvEnvironment.Append(LIBS = ['sndfile'])
tapconv = tapconvEnvironment.Program('tapconv', ['util/tapconv.cpp'])
Depends(tapconv, plus4emuLib)

# -----------------------------------------------------------------------------

makecfgEnvironment = plus4emuGUIEnvironment.Copy()
makecfgEnvironment.Append(CPPPATH = ['./installer'])
makecfgEnvironment.Prepend(LIBS = ['plus4emu'])
if haveDotconf:
    makecfgEnvironment.Append(LIBS = ['dotconf'])
if haveSDL:
    makecfgEnvironment.Append(LIBS = ['SDL'])
makecfgEnvironment.Append(LIBS = ['sndfile'])
if not win32CrossCompile:
    makecfgEnvironment.Append(LIBS = ['pthread'])
else:
    makecfgEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

makecfg = makecfgEnvironment.Program('makecfg',
    ['installer/makecfg.cpp'] + fluidCompile(['installer/mkcfg.fl']))
Depends(makecfg, plus4emuLib)

