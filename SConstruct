# vim: syntax=python

import sys, os

win64CrossCompile = ARGUMENTS.get('win64', 0)
mingwCrossCompile = win64CrossCompile or ARGUMENTS.get('win32', 0)
linux32CrossCompile = 0
disableSDL = 0          # set this to 1 on Linux with SDL version 1.2.10
disableLua = 0
enableGLShaders = 1
enableDebug = 0
buildRelease = 1
useLuaJIT = 0           # for mingwCrossCompile, use LuaJIT instead of Lua 5.3

compilerFlags = ''
if buildRelease:
    if linux32CrossCompile or (mingwCrossCompile and not win64CrossCompile):
        compilerFlags = ' -march=pentium2 -mtune=generic '
if enableDebug and not buildRelease:
    compilerFlags = ' -Wno-long-long -Wshadow -g -O2 ' + compilerFlags
    compilerFlags = ' -Wall -W -ansi -pedantic ' + compilerFlags
else:
    compilerFlags = ' -Wall -O3 ' + compilerFlags
    compilerFlags = compilerFlags + ' -fno-inline-functions '
    compilerFlags = compilerFlags + ' -fomit-frame-pointer -ffast-math '

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

programNamePrefix = ""
buildingLinuxPackage = 0
if not mingwCrossCompile:
    if sys.platform[:5] == 'linux':
        try:
            instPrefix = os.environ["UB_INSTALLDIR"]
            if instPrefix:
                instPrefix += "/usr"
                buildingLinuxPackage = 1
        except:
            pass
    if not buildingLinuxPackage:
        instPrefix = os.environ["HOME"]
        instShareDir = instPrefix + "/.local/share"
    else:
        instShareDir = instPrefix + "/share"
    instBinDir = instPrefix + "/bin"
    instDataDir = instShareDir + "/plus4emu"
    instPixmapDir = instShareDir + "/pixmaps"
    instDesktopDir = instShareDir + "/applications"
    instROMDir = instDataDir + "/roms"
    instConfDir = instDataDir + "/config"
    instDiskDir = instDataDir + "/disk"
    programNamePrefix = "p4"

plus4emuLibEnvironment = Environment(ENV = { 'PATH' : os.environ['PATH'],
                                             'HOME' : os.environ['HOME'] })
if linux32CrossCompile:
    compilerFlags = ' -m32 ' + compilerFlags
plus4emuLibEnvironment.Append(CCFLAGS = Split(compilerFlags))
plus4emuLibEnvironment.Append(CPPPATH = ['.', './src'])
if not mingwCrossCompile:
    plus4emuLibEnvironment.Append(CPPPATH = ['/usr/local/include'])
if sys.platform[:6] == 'darwin':
    plus4emuLibEnvironment.Append(CPPPATH = ['/usr/X11R6/include'])
if not linux32CrossCompile:
    linkFlags = ' -L. '
else:
    linkFlags = ' -m32 -L. -L/usr/X11R6/lib '
plus4emuLibEnvironment.Append(LINKFLAGS = Split(linkFlags))
if mingwCrossCompile:
    if not win64CrossCompile:
        mingwPrefix = 'C:/mingw32'
        plus4emuLibEnvironment.Prepend(CCFLAGS = ['-m32'])
    else:
        mingwPrefix = 'C:/mingw64'
        plus4emuLibEnvironment.Prepend(CCFLAGS = ['-m64'])
    plus4emuLibEnvironment.Append(CPPPATH = [mingwPrefix + '/include'])
    if sys.platform[:3] == 'win':
        toolNamePrefix = ''
    elif win64CrossCompile:
        toolNamePrefix = 'x86_64-w64-mingw32-'
    else:
        toolNamePrefix = 'i686-w64-mingw32-'
    plus4emuLibEnvironment['AR'] = toolNamePrefix + 'ar'
    plus4emuLibEnvironment['CC'] = toolNamePrefix + 'gcc'
    plus4emuLibEnvironment['CPP'] = toolNamePrefix + 'cpp'
    plus4emuLibEnvironment['CXX'] = toolNamePrefix + 'g++'
    plus4emuLibEnvironment['LINK'] = toolNamePrefix + 'g++'
    plus4emuLibEnvironment['RANLIB'] = toolNamePrefix + 'ranlib'
    if not disableLua:
        if useLuaJIT:
            plus4emuLibEnvironment.Append(
                CPPPATH = [mingwPrefix + '/include/lua5.1'])
        else:
            plus4emuLibEnvironment.Append(
                CPPPATH = [mingwPrefix + '/include/lua5.3'])
    plus4emuLibEnvironment.Append(LIBS = ['comdlg32', 'comctl32', 'ole32',
                                          'uuid', 'ws2_32', 'winmm', 'gdi32',
                                          'user32', 'kernel32'])
    plus4emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    plus4emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])

plus4emuGUIEnvironment = plus4emuLibEnvironment.Clone()
if mingwCrossCompile:
    plus4emuGUIEnvironment.Prepend(LIBS = ['fltk'])
else:
    try:
        if not plus4emuGUIEnvironment.ParseConfig(
            '%s --use-images --cxxflags --ldflags' % fltkConfig):
            raise Exception()
    except:
        print 'WARNING: could not run fltk-config'
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_images', 'fltk'])
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png'])
        plus4emuGUIEnvironment.Append(LIBS = ['fltk_z', 'X11'])

plus4emuGLGUIEnvironment = plus4emuLibEnvironment.Clone()
if mingwCrossCompile:
    plus4emuGLGUIEnvironment.Prepend(LIBS = ['fltk_gl', 'fltk',
                                             'glu32', 'opengl32'])
else:
    try:
        if not plus4emuGLGUIEnvironment.ParseConfig(
            '%s --use-gl --use-images --cxxflags --ldflags' % fltkConfig):
            raise Exception()
        if not "GL" in plus4emuGLGUIEnvironment["LIBS"]:
            plus4emuGLGUIEnvironment.Append(LIBS = ['GL'])
    except:
        print 'WARNING: could not run fltk-config'
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk_images', 'fltk_gl'])
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk', 'fltk_jpeg'])
        plus4emuGLGUIEnvironment.Append(LIBS = ['fltk_png', 'fltk_z', 'GL'])
    if sys.platform[:5] == 'linux':
        plus4emuGLGUIEnvironment.Append(LIBS = ['X11', 'GL'])

plus4emuLibEnvironment['CPPPATH'] = plus4emuGLGUIEnvironment['CPPPATH']

imageLibTestProgram = '''
    #include <FL/Fl.H>
    #include <FL/Fl_Shared_Image.H>
    #include <FL/Fl_PNG_Image.H>
    int main()
    {
      Fl_Shared_Image *tmp = Fl_Shared_Image::get("foo");
      tmp->release();
      Fl_PNG_Image  *tmp2 = new Fl_PNG_Image("foo");
      delete tmp2;
      return 0;
    }
'''

portAudioLibTestProgram = '''
    #include <stdio.h>
    #include <portaudio.h>
    int main()
    {
      (void) Pa_Initialize();
      (void) Pa_GetDeviceInfo(0);
      (void) Pa_Terminate();
      return 0;
    }
'''

def imageLibTest(env):
    usingJPEGLib = 'jpeg' in env['LIBS']
    usingPNGLib = 'png' in env['LIBS']
    usingZLib = 'z' in env['LIBS']
    if usingJPEGLib or usingPNGLib or usingZLib:
        tmpEnv = env.Clone()
        if usingJPEGLib:
            tmpEnv['LIBS'].remove('jpeg')
        if usingPNGLib:
            tmpEnv['LIBS'].remove('png')
        if usingZLib:
            tmpEnv['LIBS'].remove('z')
        tmpConfig = tmpEnv.Configure()
        if tmpConfig.TryLink(imageLibTestProgram, '.cpp'):
            tmpConfig.Finish()
            if usingJPEGLib:
                env['LIBS'].remove('jpeg')
            if usingPNGLib:
                env['LIBS'].remove('png')
            if usingZLib:
                env['LIBS'].remove('z')
        else:
            if (usingJPEGLib
                and not tmpConfig.CheckLib('jpeg', None, None, 'C++', 0)):
                env['LIBS'].remove('jpeg')
            if (usingPNGLib
                and not tmpConfig.CheckLib('png', None, None, 'C++', 0)):
                env['LIBS'].remove('png')
            if (usingZLib
                and not tmpConfig.CheckLib('z', None, None, 'C++', 0)):
                env['LIBS'].remove('z')
            tmpConfig.Finish()
            tmpConfig2 = env.Configure()
            if not tmpConfig2.TryLink(imageLibTestProgram, '.cpp'):
                print ' *** error: libjpeg, libpng, or zlib is not found'
                Exit(-1)
            tmpConfig2.Finish()

def portAudioLibTest(env, libNames):
    tmpEnv = env.Clone()
    if libNames.__len__() > 0:
        tmpEnv.Append(LIBS = libNames)
    tmpEnv.Append(LIBS = ['pthread'])
    if sys.platform[:5] == 'linux':
        tmpEnv.Append(LIBS = ['rt'])
    tmpConfig = tmpEnv.Configure()
    retval = tmpConfig.TryLink(portAudioLibTestProgram, '.c')
    tmpConfig.Finish()
    return retval

def checkPortAudioLib(env):
    alsaLibNeeded = 0
    jackLibNeeded = 0
    if not portAudioLibTest(env, []):
        if portAudioLibTest(env, ['asound']):
            alsaLibNeeded = 1
        elif portAudioLibTest(env, ['jack']):
            jackLibNeeded = 1
        elif portAudioLibTest(env, ['jack', 'asound']):
            alsaLibNeeded = 1
            jackLibNeeded = 1
        else:
            print ' *** error: PortAudio library is not found'
            Exit(-1)
    if jackLibNeeded:
        env.Append(LIBS = ['jack'])
    if alsaLibNeeded:
        env.Append(LIBS = ['asound'])

imageLibTest(plus4emuGUIEnvironment)
imageLibTest(plus4emuGLGUIEnvironment)

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
fltkVersion13 = 0
if configure.CheckCXXHeader('FL/Fl_Cairo.H'):
    fltkVersion13 = 1
    if sys.platform[:5] == 'linux' and not mingwCrossCompile:
        plus4emuGUIEnvironment.Append(LIBS = ['Xinerama', 'Xft'])
        plus4emuGLGUIEnvironment.Append(LIBS = ['Xinerama', 'Xft'])
    # print 'WARNING: using FLTK 1.3.x - this may not work reliably yet'
else:
    plus4emuLibEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
    plus4emuGUIEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
    plus4emuGLGUIEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
if not configure.CheckCHeader('GL/gl.h'):
    print ' *** error: OpenGL is not found'
    Exit(-1)
if enableGLShaders:
    if not configure.CheckType('PFNGLCOMPILESHADERPROC',
                               '#include <GL/gl.h>\n#include <GL/glext.h>'):
        enableGLShaders = 0
        print 'WARNING: disabling GL shader support'
if configure.CheckCHeader('stdint.h'):
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_STDINT_H'])
if not disableSDL:
    haveSDL = configure.CheckCHeader('SDL/SDL.h')
else:
    haveSDL = 0
luaPkgName = ''
if not disableLua:
    haveLua = configure.CheckCHeader('lua.h')
    haveLua = haveLua and configure.CheckCHeader('lauxlib.h')
    haveLua = haveLua and configure.CheckCHeader('lualib.h')
    if not haveLua and sys.platform[:5] == 'linux' and not mingwCrossCompile:
        for pkgName in ['lua-5.1', 'lua51', 'lua-5.3', 'lua53',
                        'lua-5.2', 'lua52', 'lua']:
            print 'Checking for Lua package ' + pkgName + '...'
            try:
                if not plus4emuLibEnvironment.ParseConfig(
                           'pkg-config --silence-errors --cflags ' + pkgName):
                    raise Exception()
            except:
                continue
            luaPkgName = pkgName
            haveLua = 1
            break
else:
    haveLua = 0
haveZLib = configure.CheckCHeader('zlib.h')
configure.Finish()

if not havePortAudioV19:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveSDL:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_SDL_H'])
if haveLua:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_LUA_H'])
if enableGLShaders:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DENABLE_GL_SHADERS'])
if not fltkVersion13:
    plus4emuLibEnvironment.Append(CCFLAGS = ['-DFLTK1'])

plus4emuGUIEnvironment['CCFLAGS'] = plus4emuLibEnvironment['CCFLAGS']
plus4emuGUIEnvironment['CPPPATH'] = plus4emuLibEnvironment['CPPPATH']
plus4emuGUIEnvironment['CXXFLAGS'] = plus4emuLibEnvironment['CXXFLAGS']
plus4emuGLGUIEnvironment['CCFLAGS'] = plus4emuLibEnvironment['CCFLAGS']
plus4emuGLGUIEnvironment['CPPPATH'] = plus4emuLibEnvironment['CPPPATH']
plus4emuGLGUIEnvironment['CXXFLAGS'] = plus4emuLibEnvironment['CXXFLAGS']

if buildRelease:
    plus4emuGUIEnvironment.Append(LINKFLAGS = ['-s'])
    plus4emuGLGUIEnvironment.Append(LINKFLAGS = ['-s'])

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

plus4emuLibSources = Split('''
    src/cpu.cpp
    src/cpuoptbl.cpp
    src/memory.cpp
    src/render.cpp
    src/ted_api.cpp
    src/ted_init.cpp
    src/ted_main.cpp
    src/ted_read.cpp
    src/ted_snd.cpp
    src/ted_write.cpp
    src/vc1541.cpp
    src/vc1551.cpp
    src/vc1581.cpp
    src/via6522.cpp
    src/plus4vm.cpp
    src/vm.cpp
    src/acia6551.cpp
    src/bplist.cpp
    src/cia8520.cpp
    src/d64image.cpp
    src/disasm.cpp
    src/display.cpp
    src/dotconf.c
    src/fileio.cpp
    src/iecdrive.cpp
    src/mps801.cpp
    src/riot6532.cpp
    src/snd_conv.cpp
    src/soundio.cpp
    src/system.cpp
    src/tape.cpp
    src/vc1526.cpp
    src/videorec.cpp
    src/wd177x.cpp
''')

plus4emuLibSources2 = []
if not fltkVersion13:
    plus4emuLibSources2 += ['Fl_Native_File_Chooser/Fl_Native_File_Chooser.cxx']
plus4emuLibSources2 += Split('''
    src/cfg_db.cpp
    src/emucfg.cpp
    src/fldisp.cpp
    src/gldisp.cpp
    src/guicolor.cpp
    src/joystick.cpp
    src/script.cpp
    src/sndio_pa.cpp
    src/vmthread.cpp
''')
plus4emuLib = plus4emuLibEnvironment.StaticLibrary(
    'plus4emu', plus4emuLibSources + plus4emuLibSources2)

# -----------------------------------------------------------------------------

residLibEnvironment = plus4emuLibEnvironment.Clone()
residLibEnvironment.Append(CPPPATH = ['./resid'])

residLibSources = Split('''
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
''')

residLib = residLibEnvironment.StaticLibrary('resid', residLibSources)

# -----------------------------------------------------------------------------

def fixDefFile(env, target, source):
    f = open('plus4lib/plus4emu.def', 'rb')
    s = []
    for tmp in f:
        s = s + [tmp]
    f.close()
    if s.__len__() > 0:
        if s[0][:7] != 'LIBRARY':
            f = open('plus4lib/plus4emu.def', 'wb')
            f.write('LIBRARY plus4emu.dll\r\n')
            for tmp in s:
                f.write(tmp)
            f.close()

if mingwCrossCompile or sys.platform[:5] == 'linux':
    plus4emuDLLEnvironment = plus4emuLibEnvironment.Clone()
    plus4emuDLLEnvironment.Append(CPPPATH = ['./plus4lib'])
    plus4emuDLLEnvironment.Append(LIBS = ['sndfile'])
    if not mingwCrossCompile:
        plus4emuDLLEnvironment.Append(CCFLAGS = ['-fvisibility=hidden'])
        plus4emuDLLEnvironment.Append(LIBS = ['pthread'])
        plus4emuDLL = plus4emuDLLEnvironment.SharedLibrary(
            'plus4lib/plus4emu',
            plus4emuLibSources + residLibSources + ['plus4lib/plus4api.cpp'])
    else:
        plus4emuDLLEnvironment.Prepend(
            LINKFLAGS = ['-mwindows', '-Wl,--output-def,plus4lib/plus4emu.def'])
        plus4emuDLLEnvironment.Prepend(LIBS = ['plus4emu', 'resid'])
        plus4emuDLLEnvironment['SHLIBPREFIX'] = ''
        plus4emuDLLEnvironment['SHLIBSUFFIX'] = ''
        plus4emuDLL = plus4emuDLLEnvironment.SharedLibrary(
            'plus4lib/plus4emu.dll', ['plus4lib/plus4api.cpp'])
        # add LIBRARY line to the .def file if needed
        plus4emuDLLEnvironment.Command('__fix_plus4emu_def_file__', plus4emuDLL,
                                       fixDefFile)
    Depends(plus4emuDLL, plus4emuLib)
    Depends(plus4emuDLL, residLib)

# -----------------------------------------------------------------------------

plus4emuEnvironment = plus4emuGLGUIEnvironment.Clone()
plus4emuEnvironment.Append(CPPPATH = ['./gui'])
if luaPkgName:
    # using pkg-config
    if not plus4emuEnvironment.ParseConfig('pkg-config --libs ' + luaPkgName):
        print ' *** error: Lua library is not found'
        Exit(-1)
elif haveLua:
    if not mingwCrossCompile:
        plus4emuEnvironment.Append(LIBS = ['lua'])
    elif not useLuaJIT:
        plus4emuEnvironment.Append(LIBS = ['lua53'])
    else:
        plus4emuEnvironment.Append(LIBS = ['lua51'])
if haveSDL:
    plus4emuEnvironment.Append(LIBS = ['SDL'])
plus4emuEnvironment.Append(LIBS = ['portaudio', 'sndfile'])
if not mingwCrossCompile:
    checkPortAudioLib(plus4emuEnvironment)
    plus4emuEnvironment.Append(LIBS = ['pthread'])
    if sys.platform[:5] == 'linux':
        plus4emuEnvironment.Append(LIBS = ['rt'])
else:
    plus4emuEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
plus4emuEnvironment.Prepend(LIBS = ['plus4emu', 'resid'])

plus4emuSources = ['gui/gui.cpp']
plus4emuSources += fluidCompile(['gui/gui.fl', 'gui/disk_cfg.fl',
                                 'gui/disp_cfg.fl', 'gui/kbd_cfg.fl',
                                 'gui/snd_cfg.fl', 'gui/vm_cfg.fl',
                                 'gui/debug.fl', 'gui/printer.fl',
                                 'gui/about.fl'])
plus4emuSources += ['gui/debugger.cpp', 'gui/monitor.cpp', 'gui/main.cpp',
                    'src/pngwrite.cpp']
if mingwCrossCompile:
    plus4emuResourceObject = plus4emuEnvironment.Command(
        'resource/resource.o',
        ['resource/plus4emu.rc', 'resource/Cbm4.ico', 'resource/1551.ico',
         'resource/Plus4Mon4.ico', 'resource/Plus4i.ico',
         'resource/CbmFile.ico'],
        toolNamePrefix + 'windres -v --use-temp-file '
        + '--preprocessor="gcc -E -xc -DRC_INVOKED" '
        + '-o $TARGET resource/plus4emu.rc')
    plus4emuSources += [plus4emuResourceObject]
plus4emu = plus4emuEnvironment.Program('plus4emu', plus4emuSources)
Depends(plus4emu, plus4emuLib)
Depends(plus4emu, residLib)

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/plus4emu', 'plus4emu',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

tapconvEnvironment = plus4emuLibEnvironment.Clone()
tapconvEnvironment.Prepend(LIBS = ['plus4emu'])
tapconvEnvironment.Append(LIBS = ['sndfile'])
if buildRelease:
    tapconvEnvironment.Append(LINKFLAGS = ['-s'])
tapconv = tapconvEnvironment.Program(programNamePrefix + 'tapconv',
                                     ['util/tapconv.cpp'])
Depends(tapconv, plus4emuLib)

# -----------------------------------------------------------------------------

makecfgEnvironment = plus4emuGUIEnvironment.Clone()
makecfgEnvironment.Append(CPPPATH = ['./installer'])
makecfgEnvironment.Prepend(LIBS = ['plus4emu'])
if haveSDL:
    makecfgEnvironment.Append(LIBS = ['SDL'])
makecfgEnvironment.Append(LIBS = ['sndfile'])
if not mingwCrossCompile:
    makecfgEnvironment.Append(LIBS = ['pthread'])
else:
    makecfgEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

makecfg = makecfgEnvironment.Program(programNamePrefix + 'makecfg',
    ['installer/makecfg.cpp'] + fluidCompile(['installer/mkcfg.fl']))
Depends(makecfg, plus4emuLib)

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/' + programNamePrefix + 'makecfg',
            programNamePrefix + 'makecfg',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

compressLibEnvironment = plus4emuLibEnvironment.Clone()
compressLibEnvironment.Append(CPPPATH = ['./util/compress'])
compressLib = compressLibEnvironment.StaticLibrary(
    'compress',
    Split('''
        util/compress/compress0.cpp
        util/compress/compress0.hpp
        util/compress/compress1.cpp
        util/compress/compress1.hpp
        util/compress/compress2.cpp
        util/compress/compress2.hpp
        util/compress/compress.cpp
        util/compress/compress.hpp
        util/compress/decompress0.cpp
        util/compress/decompress0.hpp
        util/compress/decompress1.cpp
        util/compress/decompress1.hpp
        util/compress/decompress2.cpp
        util/compress/decompress2.hpp
    '''))

# -----------------------------------------------------------------------------

p4fliconvLibEnvironment = plus4emuGLGUIEnvironment.Clone()
p4fliconvLibEnvironment.Append(CPPPATH = ['./util/compress',
                                          './util/p4fliconv'])
if not haveZLib:
    print 'WARNING: zlib is not found, building p4fliconv without P4S support'
    p4fliconvLibEnvironment.Append(CXXFLAGS = ['-DNO_P4S_SUPPORT'])
elif not 'fltk_z' in p4fliconvLibEnvironment['LIBS']:
    if not 'z' in p4fliconvLibEnvironment['LIBS']:
        p4fliconvLibEnvironment.Append(LIBS = ['z'])

p4fliconvLibSources = ['util/p4fliconv/dither.cpp',
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

p4fliconvEnvironment = p4fliconvLibEnvironment.Clone()
if mingwCrossCompile:
    p4fliconvEnvironment.Prepend(LIBS = ['fltk_images'])
    p4fliconvEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png'])
p4fliconvEnvironment.Prepend(LIBS = ['p4fliconv', 'compress', 'plus4emu'])
if not mingwCrossCompile:
    p4fliconvEnvironment.Append(LIBS = ['pthread'])

p4fliconv = p4fliconvEnvironment.Program(
    'p4fliconv', ['util/p4fliconv/main.cpp'])
Depends(p4fliconv, p4fliconvLib)
Depends(p4fliconv, compressLib)
Depends(p4fliconv, plus4emuLib)

if mingwCrossCompile:
    p4fliconvGUIEnvironment = p4fliconvEnvironment.Clone()
    p4fliconvGUIEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
    p4fliconvGUIMain = p4fliconvGUIEnvironment.Object(
        'guimain', 'util/p4fliconv/main.cpp')
    p4fliconvGUI = p4fliconvGUIEnvironment.Program(
        'p4fliconv_gui', [p4fliconvGUIMain])
    Depends(p4fliconvGUI, p4fliconvLib)
    Depends(p4fliconvGUI, compressLib)
    Depends(p4fliconvGUI, plus4emuLib)

if sys.platform[:6] == 'darwin':
    Command('plus4emu.app/Contents/MacOS/p4fliconv', 'p4fliconv',
            'mkdir -p plus4emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

p4sconvEnvironment = p4fliconvLibEnvironment.Clone()
if mingwCrossCompile:
    p4sconvEnvironment.Prepend(LIBS = ['fltk_images'])
    p4sconvEnvironment.Append(LIBS = ['fltk_jpeg', 'fltk_png'])
p4sconvEnvironment.Prepend(LIBS = ['p4fliconv', 'compress', 'plus4emu'])
if mingwCrossCompile:
    p4sconvEnvironment.Prepend(LINKFLAGS = ['-mconsole'])
else:
    p4sconvEnvironment.Append(LIBS = ['pthread'])
p4sconv = p4sconvEnvironment.Program('p4sconv', ['util/p4fliconv/p4sconv.cpp'])
Depends(p4sconv, p4fliconvLib)
Depends(p4sconv, compressLib)
Depends(p4sconv, plus4emuLib)

# -----------------------------------------------------------------------------

compressEnvironment = compressLibEnvironment.Clone()
compressEnvironment.Prepend(LIBS = ['compress'])
if buildRelease:
    compressEnvironment.Append(LINKFLAGS = ['-s'])
compress = compressEnvironment.Program(programNamePrefix + 'compress',
                                       ['util/compress/main.cpp'])
Depends(compress, compressLib)

# -----------------------------------------------------------------------------

if not mingwCrossCompile:
    if buildingLinuxPackage:
        makecfgEnvironment.InstallAs([instBinDir + "/plus4emu.bin",
                                      instBinDir + "/plus4emu"],
                                     [plus4emu, "installer/plus4emu"])
    else:
        makecfgEnvironment.Install(instBinDir, plus4emu)
    makecfgEnvironment.Install(instBinDir,
                               [tapconv, makecfg, p4fliconv, p4sconv, compress])
    makecfgEnvironment.Install(instPixmapDir, ["resource/Cbm4.png"])
    makecfgEnvironment.Install(instDesktopDir, ["resource/plus4emu.desktop"])
    if not buildingLinuxPackage:
        makecfgEnvironment.Command(
            instConfDir + "/P4_64k_PAL_3PLUS1.cfg", makecfg,
            ['./' + programNamePrefix + 'makecfg -f "' + instDataDir + '"'])
    makecfgEnvironment.Install(instROMDir,
                               ["roms/1526_07c.rom", "roms/3plus1.rom",
                                "roms/dos15412.rom", "roms/dos1541.rom",
                                "roms/dos1551.rom", "roms/dos1581.rom",
                                "roms/mps801.rom", "roms/p4_basic.rom",
                                "roms/p4fileio.rom", "roms/p4kernal.rom",
                                "roms/p4_ntsc.rom"])
    makecfgEnvironment.Install(instConfDir,
                               ["config/clearkbd.cfg", "config/p4_keys.cfg",
                                "config/p4keyshu.cfg"])
    makecfgEnvironment.Install(instDiskDir, ["disk/disk.zip"])
    makecfgEnvironment.Alias("install",
                             [instBinDir, instPixmapDir, instDesktopDir,
                              instDataDir, instROMDir, instConfDir,
                              instDiskDir])

