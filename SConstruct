# vim: syntax=python

import sys

win32CrossCompile = 0
linux32CrossCompile = 0
disableSDL = 0          # set this to 1 on Linux with SDL version >= 1.2.10
disableLua = 0
enableGLShaders = 1
enableDebug = 0
buildRelease = 1

compilerFlags = ''
if buildRelease:
    if linux32CrossCompile:
        compilerFlags = ' -march=pentium2 -mtune=generic '
    elif win32CrossCompile:
        compilerFlags = ' -march=pentium2 -mtune=athlon-xp '
if enableDebug and not buildRelease:
    compilerFlags = ' -Wno-long-long -Wshadow -g -O2 ' + compilerFlags
    compilerFlags = ' -Wall -W -ansi -pedantic ' + compilerFlags
else:
    compilerFlags = ' -Wall -O3 ' + compilerFlags
    compilerFlags = compilerFlags + ' -fno-inline-functions '
    compilerFlags = compilerFlags + ' -fomit-frame-pointer -ffast-math '

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

plus4emuLibEnvironment = Environment()
if linux32CrossCompile:
    compilerFlags = ' -m32 ' + compilerFlags
plus4emuLibEnvironment.Append(CCFLAGS = Split(compilerFlags))
plus4emuLibEnvironment.Append(CPPPATH = ['.', './src'])
plus4emuLibEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
plus4emuLibEnvironment.Append(CPPPATH = ['/usr/local/include'])
if sys.platform[:6] == 'darwin':
    plus4emuLibEnvironment.Append(CPPPATH = ['/usr/X11R6/include'])
if not linux32CrossCompile:
    linkFlags = ' -L. '
else:
    linkFlags = ' -m32 -L. -L/usr/X11R6/lib '
plus4emuLibEnvironment.Append(LINKFLAGS = Split(linkFlags))
if win32CrossCompile:
    plus4emuLibEnvironment['AR'] = 'wine C:/MinGW/bin/ar.exe'
    plus4emuLibEnvironment['CC'] = 'wine C:/MinGW/bin/gcc-sjlj.exe'
    plus4emuLibEnvironment['CPP'] = 'wine C:/MinGW/bin/cpp-sjlj.exe'
    plus4emuLibEnvironment['CXX'] = 'wine C:/MinGW/bin/g++-sjlj.exe'
    plus4emuLibEnvironment['LINK'] = 'wine C:/MinGW/bin/g++-sjlj.exe'
    plus4emuLibEnvironment['RANLIB'] = 'wine C:/MinGW/bin/ranlib.exe'
    plus4emuLibEnvironment.Append(LIBS = ['comdlg32', 'ole32', 'uuid',
                                          'ws2_32', 'gdi32', 'user32',
                                          'kernel32'])
    plus4emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    plus4emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])

plus4emuGUIEnvironment = plus4emuLibEnvironment.Copy()
if win32CrossCompile:
    plus4emuGUIEnvironment.Prepend(LIBS = ['fltk'])
else:
    try:
        if not plus4emuGUIEnvironment.ParseConfig(
            '%s --cxxflags --ldflags' % fltkConfig):
            raise Exception()
    except:
        print 'WARNING: could not run fltk-config'
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_images', 'fltk'])
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png'])
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_z'])

plus4emuGLGUIEnvironment = plus4emuLibEnvironment.Copy()
if win32CrossCompile:
    plus4emuGLGUIEnvironment.Prepend(LIBS = ['fltk_gl', 'fltk',
                                             'glu32', 'opengl32'])
else:
    try:
        if not plus4emuGLGUIEnvironment.ParseConfig(
            '%s --use-gl --cxxflags --ldflags' % fltkConfig):
            raise Exception()
    except:
        print 'WARNING: could not run fltk-config'
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk_images', 'fltk_gl'])
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk', 'fltk_jpeg'])
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk_png', 'fltk_z', 'GL'])

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
if enableGLShaders:
    if not configure.CheckType('PFNGLCOMPILESHADERPROC',
                               '#include <GL/gl.h>\n#include <GL/glext.h>'):
        enableGLShaders = 0
        print 'WARNING: disabling GL shader support'
haveDotconf = configure.CheckCHeader('dotconf.h')
if configure.CheckCHeader('stdint.h'):
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_STDINT_H'])
if not disableSDL:
    haveSDL = configure.CheckCHeader('SDL/SDL.h')
else:
    haveSDL = 0
if not disableLua:
    haveLua = configure.CheckCHeader('lua.h')
    haveLua = haveLua and configure.CheckCHeader('lauxlib.h')
    haveLua = haveLua and configure.CheckCHeader('lualib.h')
else:
    haveLua = 0
haveZLib = configure.CheckCHeader('zlib.h')
configure.Finish()

if not havePortAudioV19:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveDotconf:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_DOTCONF_H'])
if haveSDL:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_SDL_H'])
if haveLua:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_LUA_H'])
if enableGLShaders:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DENABLE_GL_SHADERS'])
plus4emuLibEnvironment.Append(CCFLAGS = ['-DFLTK1'])

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
    Fl_Native_File_Chooser/Fl_Native_File_Chooser.cxx
    src/acia6551.cpp
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
    src/guicolor.cpp
    src/iecdrive.cpp
    src/joystick.cpp
    src/memory.cpp
    src/plus4vm.cpp
    src/render.cpp
    src/riot6532.cpp
    src/script.cpp
    src/snd_conv.cpp
    src/soundio.cpp
    src/system.cpp
    src/tape.cpp
    src/ted_api.cpp
    src/ted_init.cpp
    src/ted_main.cpp
    src/ted_read.cpp
    src/ted_snd.cpp
    src/ted_write.cpp
    src/vc1526.cpp
    src/vc1541.cpp
    src/vc1551.cpp
    src/vc1581.cpp
    src/via6522.cpp
    src/videorec.cpp
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
    if win32CrossCompile:
        # hack to work around binary incompatible dirent functions in
        # libdotconf.a
        plus4emuEnvironment.Append(LIBS = ['mingwex'])
    plus4emuEnvironment.Append(LIBS = ['dotconf'])
if haveLua:
    plus4emuEnvironment.Append(LIBS = ['lua'])
if haveSDL:
    plus4emuEnvironment.Append(LIBS = ['SDL'])
plus4emuEnvironment.Append(LIBS = ['portaudio', 'sndfile'])
if not win32CrossCompile:
    if sys.platform[:5] == 'linux':
        if not buildRelease:
            plus4emuEnvironment.Append(LIBS = ['jack'])
        plus4emuEnvironment.Append(LIBS = ['asound', 'pthread', 'rt'])
    else:
        plus4emuEnvironment.Append(LIBS = ['pthread'])
else:
    plus4emuEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

plus4emu = plus4emuEnvironment.Program('plus4emu',
    ['gui/gui.cpp']
    + fluidCompile(['gui/gui.fl', 'gui/disk_cfg.fl', 'gui/disp_cfg.fl',
                    'gui/kbd_cfg.fl', 'gui/snd_cfg.fl', 'gui/vm_cfg.fl',
                    'gui/debug.fl', 'gui/printer.fl', 'gui/about.fl'])
    + ['gui/debugger.cpp', 'gui/monitor.cpp', 'gui/main.cpp'])
Depends(plus4emu, plus4emuLib)
Depends(plus4emu, residLib)

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/plus4emu', 'plus4emu',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

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
    if win32CrossCompile:
        # hack to work around binary incompatible dirent functions in
        # libdotconf.a
        makecfgEnvironment.Append(LIBS = ['mingwex'])
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

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/makecfg', 'makecfg',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

p4fliconvLibEnvironment = plus4emuGLGUIEnvironment.Copy()
p4fliconvLibEnvironment.Append(CPPPATH = ['./util/p4fliconv'])
if not haveZLib:
    print 'WARNING: zlib is not found, building p4fliconv without P4S support'
    p4fliconvLibEnvironment.Append(CXXFLAGS = ['-DNO_P4S_SUPPORT'])

p4fliconvLibSources = ['util/p4fliconv/compress.cpp',
                       'util/p4fliconv/dither.cpp',
                       'util/p4fliconv/flicfg.cpp',
                       'util/p4fliconv/flidisp.cpp',
                       'util/p4fliconv/hiresfli.cpp',
                       'util/p4fliconv/hiresnofli.cpp',
                       'util/p4fliconv/hrbmifli.cpp',
                       'util/p4fliconv/imageconv.cpp',
                       'util/p4fliconv/imgwrite.cpp',
                       'util/p4fliconv/interlace7.cpp',
                       'util/p4fliconv/mcbmifli.cpp',
                       'util/p4fliconv/mcchar.cpp',
                       'util/p4fliconv/mcfli.cpp',
                       'util/p4fliconv/mcifli.cpp',
                       'util/p4fliconv/mcnofli.cpp',
                       'util/p4fliconv/p4fliconv.cpp',
                       'util/p4fliconv/p4slib.cpp',
                       'util/p4fliconv/prgdata.cpp']
p4fliconvLibSources += fluidCompile(['util/p4fliconv/p4fliconv.fl'])
p4fliconvLib = p4fliconvLibEnvironment.StaticLibrary('p4fliconv',
                                                     p4fliconvLibSources)

p4fliconvEnvironment = p4fliconvLibEnvironment.Copy()
p4fliconvEnvironment.Prepend(LIBS = ['p4fliconv', 'plus4emu', 'fltk_images'])
if win32CrossCompile or buildRelease:
    p4fliconvEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png', 'fltk_z'])
if haveDotconf:
    if win32CrossCompile:
        # hack to work around binary incompatible dirent functions in
        # libdotconf.a
        p4fliconvEnvironment.Append(LIBS = ['mingwex'])
    p4fliconvEnvironment.Append(LIBS = ['dotconf'])
if not win32CrossCompile:
    p4fliconvEnvironment.Append(LIBS = ['pthread'])

p4fliconv = p4fliconvEnvironment.Program(
    'p4fliconv', ['util/p4fliconv/main.cpp'])
Depends(p4fliconv, p4fliconvLib)
Depends(p4fliconv, plus4emuLib)

if win32CrossCompile:
    p4fliconvGUIEnvironment = p4fliconvEnvironment.Copy()
    p4fliconvGUIEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
    p4fliconvGUIMain = p4fliconvGUIEnvironment.Object(
        'guimain', 'util/p4fliconv/main.cpp')
    p4fliconvGUI = p4fliconvGUIEnvironment.Program(
        'p4fliconv_gui', [p4fliconvGUIMain])
    Depends(p4fliconvGUI, p4fliconvLib)
    Depends(p4fliconvGUI, plus4emuLib)

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/p4fliconv', 'p4fliconv',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

p4sconvEnvironment = p4fliconvLibEnvironment.Copy()
p4sconvEnvironment.Prepend(LIBS = ['p4fliconv', 'plus4emu', 'fltk_images'])
if win32CrossCompile or buildRelease:
    p4sconvEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png', 'fltk_z'])
if haveDotconf:
    if win32CrossCompile:
        # hack to work around binary incompatible dirent functions in
        # libdotconf.a
        p4sconvEnvironment.Append(LIBS = ['mingwex'])
    p4sconvEnvironment.Append(LIBS = ['dotconf'])
if win32CrossCompile:
    p4sconvEnvironment.Prepend(LINKFLAGS = ['-mconsole'])
else:
    p4sconvEnvironment.Append(LIBS = ['pthread'])
p4sconv = p4sconvEnvironment.Program('p4sconv', ['util/p4fliconv/p4sconv.cpp'])
Depends(p4sconv, p4fliconvLib)
Depends(p4sconv, plus4emuLib)

# -----------------------------------------------------------------------------

compressEnvironment = plus4emuLibEnvironment.Copy()
compressEnvironment.Prepend(LIBS = ['p4fliconv'])
compress = compressEnvironment.Program('compress', ['util/compress.cpp'])
Depends(compress, p4fliconvLib)

