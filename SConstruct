#SConstruct

# needed imports
import sys
import os
import SCons.Util

PROGRAM_NAME = 'D2X-Rebirth'
target = 'd2x-rebirth'

# version number
D2XMAJOR = 0
D2XMINOR = 57
D2XMICRO = 3
VERSION_STRING = ' v' + str(D2XMAJOR) + '.' + str(D2XMINOR) + '.' + str(D2XMICRO)

# installation path
PREFIX = str(ARGUMENTS.get('prefix', '/usr/local'))
BIN_SUBDIR = '/bin'
DATA_SUBDIR = '/share/games/d2x-rebirth'
BIN_DIR = PREFIX + BIN_SUBDIR
DATA_DIR = PREFIX + DATA_SUBDIR

# command-line parms
sharepath = str(ARGUMENTS.get('sharepath', DATA_DIR))
debug = int(ARGUMENTS.get('debug', 0))
profiler = int(ARGUMENTS.get('profiler', 0))
opengl = int(ARGUMENTS.get('opengl', 1))
opengles = int(ARGUMENTS.get('opengles', 0))
asm = int(ARGUMENTS.get('asm', 0))
editor = int(ARGUMENTS.get('editor', 0))
extra_version = str(ARGUMENTS.get('extra_version', ''))
extra_version_build_time = str(ARGUMENTS.get('extra_version_build_time', ''))
sdlmixer = int(ARGUMENTS.get('sdlmixer', 1))
ipv6 = int(ARGUMENTS.get('ipv6', 0))
use_mudflap = int(ARGUMENTS.get('use_mudflap', 0))
use_python = str(ARGUMENTS.get('use_python', ''))
use_udp = int(ARGUMENTS.get('use_udp', 1))
use_tracker = int(ARGUMENTS.get('use_tracker', 1))
verbosebuild = int(ARGUMENTS.get('verbosebuild', 0))
builddir = str(ARGUMENTS.get('builddir', ''))
target_platform = str(ARGUMENTS.get('target_platform', sys.platform))

# endianess-checker
def checkEndian():
    import struct
    array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
    i = struct.unpack('i', array)
    if i == struct.unpack('<i', array):
        return "little"
    elif i == struct.unpack('>i', array):
        return "big"
    return "unknown"


print '\n===== ' + PROGRAM_NAME + VERSION_STRING + ' =====\n'

# general source files
common_sources = [
'2d/2dsline.cpp',
'2d/bitblt.cpp',
'2d/bitmap.cpp',
'2d/box.cpp',
'2d/canvas.cpp',
'2d/circle.cpp',
'2d/disc.cpp',
'2d/font.cpp',
'2d/gpixel.cpp',
'2d/line.cpp',
'2d/palette.cpp',
'2d/pcx.cpp',
'2d/pixel.cpp',
'2d/poly.cpp',
'2d/rect.cpp',
'2d/rle.cpp',
'2d/scalec.cpp',
'3d/clipper.cpp',
'3d/draw.cpp',
'3d/globvars.cpp',
'3d/instance.cpp',
'3d/interp.cpp',
'3d/matrix.cpp',
'3d/points.cpp',
'3d/rod.cpp',
'3d/setup.cpp',
'arch/sdl/event.cpp',
'arch/sdl/init.cpp',
'arch/sdl/joy.cpp',
'arch/sdl/key.cpp',
'arch/sdl/mouse.cpp',
'arch/sdl/rbaudio.cpp',
'arch/sdl/timer.cpp',
'arch/sdl/window.cpp',
'arch/sdl/digi.cpp',
'arch/sdl/digi_audio.cpp',
'iff/iff.cpp',
'libmve/decoder8.cpp',
'libmve/decoder16.cpp',
'libmve/mve_audio.cpp',
'libmve/mvelib.cpp',
'libmve/mveplay.cpp',
'main/ai.cpp',
'main/ai2.cpp',
'main/aipath.cpp',
'main/automap.cpp',
'main/bm.cpp',
'main/cntrlcen.cpp',
'main/collide.cpp',
'main/config.cpp',
'main/console.cpp',
'main/cxxconsole.cpp',
'main/pybinding.cpp',
'main/controls.cpp',
'main/credits.cpp',
'main/digiobj.cpp',
'main/dumpmine.cpp',
'main/effects.cpp',
'main/endlevel.cpp',
'main/escort.cpp',
'main/fireball.cpp',
'main/fuelcen.cpp',
'main/fvi.cpp',
'main/game.cpp',
'main/gamecntl.cpp',
'main/gamefont.cpp',
'main/gamemine.cpp',
'main/gamepal.cpp',
'main/gamerend.cpp',
'main/gamesave.cpp',
'main/gameseg.cpp',
'main/gameseq.cpp',
'main/gauges.cpp',
'main/hostage.cpp',
'main/hud.cpp',
'main/inferno.cpp',
'main/kconfig.cpp',
'main/kmatrix.cpp',
'main/laser.cpp',
'main/lighting.cpp',
'main/menu.cpp',
'main/mglobal.cpp',
'main/mission.cpp',
'main/morph.cpp',
'main/movie.cpp',
'main/multi.cpp',
'main/multibot.cpp',
'main/newdemo.cpp',
'main/newmenu.cpp',
'main/object.cpp',
'main/paging.cpp',
'main/physics.cpp',
'main/piggy.cpp',
'main/player.cpp',
'main/playsave.cpp',
'main/polyobj.cpp',
'main/powerup.cpp',
'main/render.cpp',
'main/robot.cpp',
'main/scores.cpp',
'main/segment.cpp',
'main/slew.cpp',
'main/songs.cpp',
'main/state.cpp',
'main/switch.cpp',
'main/terrain.cpp',
'main/texmerge.cpp',
'main/text.cpp',
'main/titles.cpp',
'main/vclip.cpp',
'main/wall.cpp',
'main/weapon.cpp',
'maths/fixc.cpp',
'maths/rand.cpp',
'maths/tables.cpp',
'maths/vecmat.cpp',
'mem/mem.cpp',
'misc/args.cpp',
'misc/dl_list.cpp',
'misc/error.cpp',
'misc/hash.cpp',
'misc/hmp.cpp',
'misc/ignorecase.cpp',
'misc/physfsrwops.cpp',
'misc/physfsx.cpp',
'misc/strio.cpp',
'misc/strutil.cpp',
'texmap/ntmap.cpp',
'texmap/scanline.cpp'
]

# for editor
editor_sources = [
'editor/centers.cpp',
'editor/curves.cpp',
'editor/autosave.cpp',
'editor/eglobal.cpp',
'editor/ehostage.cpp',
'editor/elight.cpp',
'editor/eobject.cpp',
'editor/eswitch.cpp',
'editor/func.cpp',
'editor/group.cpp',
'editor/info.cpp',
'editor/kbuild.cpp',
'editor/kcurve.cpp',
'editor/kfuncs.cpp',
'editor/kgame.cpp',
'editor/khelp.cpp',
'editor/kmine.cpp',
'editor/ksegmove.cpp',
'editor/ksegsel.cpp',
'editor/ksegsize.cpp',
'editor/ktmap.cpp',
'editor/kview.cpp',
'editor/med.cpp',
'editor/meddraw.cpp',
'editor/medmisc.cpp',
'editor/medrobot.cpp',
'editor/medsel.cpp',
'editor/medwall.cpp',
'editor/mine.cpp',
'editor/objpage.cpp',
'editor/segment.cpp',
'editor/seguvs.cpp',
'editor/texpage.cpp',
'editor/texture.cpp',
'main/bmread.cpp',
'ui/button.cpp',
'ui/checkbox.cpp',
'ui/dialog.cpp',
'ui/file.cpp',
'ui/gadget.cpp',
'ui/icon.cpp',
'ui/inputbox.cpp',
'ui/keypad.cpp',
'ui/keypress.cpp',
'ui/keytrap.cpp',
'ui/listbox.cpp',
'ui/menu.cpp',
'ui/menubar.cpp',
'ui/message.cpp',
'ui/popup.cpp',
'ui/radio.cpp',
'ui/scroll.cpp',
'ui/ui.cpp',
'ui/uidraw.cpp',
'ui/userbox.cpp'
]

# SDL_mixer sound implementation
arch_sdlmixer = [
'arch/sdl/digi_mixer.cpp',
'arch/sdl/digi_mixer_music.cpp',
'arch/sdl/jukebox.cpp'
]       

# for opengl
arch_ogl_sources = [
'arch/ogl/gr.cpp',
'arch/ogl/ogl.cpp',
]

# for non-ogl
arch_sdl_sources = [
'arch/sdl/gr.cpp',
'texmap/tmapflat.cpp'
]

# assembler related
asm_sources = [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]

# Acquire environment object...
env = Environment(ENV = os.environ)
if (builddir != '' and builddir[-1:] != '/'):
	builddir += '/'
if builddir != '':
	env.VariantDir(builddir, '.', duplicate=0)
 
# Prettier build messages......
if (verbosebuild == 0):
	env["CCCOMSTR"]     = "Compiling $SOURCE ..."
	env["CXXCOMSTR"]    = "Compiling $SOURCE ..."
	env["LINKCOMSTR"]   = "Linking $TARGET ..."
	env["ARCOMSTR"]     = "Archiving $TARGET ..."
	env["RANLIBCOMSTR"] = "Indexing $TARGET ..."

# flags and stuff for all platforms
env.Append(CPPFLAGS = ['-Wall', '-Wno-sign-compare'])
env.Append(CFLAGS = ['-std=gnu99'])
env.Append(CCFLAGS = ['-Wall', '-Werror=implicit-int', '-Werror=implicit-function-declaration', '-Werror=format-security', '-Werror=array-bounds', '-Werror=missing-declarations', '-Wno-unused-parameter', '-funsigned-char'])
env.Append(CPPDEFINES = [('PROGRAM_NAME', '\\"' + str(PROGRAM_NAME) + '\\"'), ('D2XMAJORi', str(D2XMAJOR)), ('D2XMINORi', str(D2XMINOR)), ('D2XMICROi', str(D2XMICRO))])
env.Append(CPPDEFINES = ['NETWORK', '_REENTRANT'])
env.Append(CPPPATH = ['include', 'main', 'arch/include'])
if use_python != '':
	env.Append(CPPDEFINES = [ ('USE_PYTHON', use_python) ])
	env.Append(CPPPATH = ['/usr/include/python%s' % use_python])
	env.Append(LIBS = ['-lboost_python-%s' % use_python, '-lpython%s' % use_python])
	common_sources.extend(['python/input.cpp',
		'python/object.cpp',
		'python/player.cpp',
		'python/powerup.cpp',
		'python/reactor.cpp',
		'python/robot.cpp',
		'python/segment.cpp',
		'python/traceback.cpp',
		'python/weapon.cpp',
		])
libs = ['physfs', 'm']
env['CXXFLAGS'] += ['-Wextra', '-std=gnu++0x']

# Get traditional compiler environment variables
if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
	env['CFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CPPFLAGS'):
	env['CPPFLAGS'] += SCons.Util.CLVar(os.environ['CPPFLAGS'])
if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
if os.environ.has_key('CXXFLAGS'):
	env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

# windows or *nix?
if target_platform == 'win32':
	print "compiling on Windows"
	osdef = '_WIN32'
	osasmdef = 'win32'
	sharepath = ''
	env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/win32/include'])
	ogldefines = ['OGL']
	common_sources += ['arch/win32/messagebox.cpp']
	ogllibs = ''
	libs += ['glu32', 'wsock32', 'ws2_32', 'winmm', 'mingw32', 'SDLmain', 'SDL']
	lflags = os.environ["LDFLAGS"] if os.environ.has_key('LDFLAGS') else ''
	lflags += ' -mwindows arch/win32/d2xr.res'
elif target_platform == 'darwin':
	print "compiling on Mac OS X"
	osdef = '__APPLE__'
	sharepath = ''
	env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
	asm = 0
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	common_sources += ['arch/cocoa/SDLMain.m', 'arch/carbon/messagebox.cpp']
	ogllibs = ''
	libs = ''
	# Ugly way of linking to frameworks, but kreator has seen uglier
	lflags = '-framework ApplicationServices -framework Carbon -framework Cocoa -framework SDL'
	libs += '../physfs/build/Debug/libphysfs.dylib'
	if (sdl_only == 0):
		lflags += ' -framework OpenGL'
	if (sdlmixer == 1):
		print "including SDL_mixer"
		lflags += ' -framework SDL_mixer'
	sys.path += ['./arch/cocoa']
	VERSION = str(D2XMAJOR) + '.' + str(D2XMINOR)
	if (D2XMICRO):
		VERSION += '.' + str(D2XMICRO)
	env['VERSION_NUM'] = VERSION
	env['VERSION_NAME'] = PROGRAM_NAME + ' v' + VERSION
	import tool_bundle
else:
	print "compiling on *NIX"
	osdef = '__LINUX__'
	osasmdef = 'elf'
	sharepath += '/'
	env.ParseConfig('sdl-config --cflags')
	env.ParseConfig('sdl-config --libs')
	env.Append(CPPDEFINES = ['__LINUX__', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	libs += env['LIBS']
	if (opengles == 1):
		ogllibs = ['GLES_CM', 'EGL']
	else:
		ogllibs = ['GL', 'GLU']
	lflags = os.environ["LDFLAGS"] if os.environ.has_key('LDFLAGS') else ''
	if use_mudflap:
		env['CFLAGS'][0:0] += ['-fmudflapth']
		env['CXXFLAGS'][0:0] += ['-fmudflapth']
		lflags += ' -fmudflapth'
		libs[0:0] += ['mudflapth']

# set endianess
if (checkEndian() == "big"):
	print "BigEndian machine detected"
	asm = 0
	env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
elif (checkEndian() == "little"):
	print "LittleEndian machine detected"

# opengl or software renderer?
if (opengl == 1) or (opengles == 1):
	if (opengles == 1):
		print "building with OpenGL ES"
		env.Append(CPPDEFINES = ['OGLES'])
	else:
		print "building with OpenGL"
	env.Append(CPPDEFINES = ogldefines)
	common_sources += arch_ogl_sources
	libs += ogllibs
else:
	print "building with Software Renderer"
	common_sources += arch_sdl_sources

# assembler code?
if (asm == 1) and (opengl == 0):
	print "including: ASSEMBLER"
	env.Replace(AS = 'nasm')
	env.Append(ASCOM = ' -f ' + str(osasmdef) + ' -d' + str(osdef) + ' -Itexmap/ ')
	common_sources += asm_sources
else:
	env.Append(CPPDEFINES = ['NO_ASM'])

# SDL_mixer support?
if (sdlmixer == 1):
	print "including SDL_mixer"
	env.Append(CPPDEFINES = ['USE_SDLMIXER'])
	common_sources += arch_sdlmixer
	if (target_platform != 'darwin'):
		libs += ['SDL_mixer']

# debug?
if (debug == 1):
	print "including: DEBUG"
	env.Append(CPPFLAGS = ['-g'])
else:
	env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
	env.Append(CPPFLAGS = ['-O2'])

# profiler?
if (profiler == 1):
	env.Append(CPPFLAGS = ['-pg'])
	lflags += ' -pg'

#editor build?
if (editor == 1):
	env.Append(CPPDEFINES = ['EDITOR'])
	env.Append(CPPPATH = ['include/editor'])
	common_sources += editor_sources

# IPv6 compability?
if (ipv6 == 1):
	env.Append(CPPDEFINES = ['IPv6'])

# UDP support?
if (use_udp == 1):
	env.Append(CPPDEFINES = ['USE_UDP'])
	common_sources += ['main/net_udp.cpp']
	
	# Tracker support?  (Relies on UDP)
	if( use_tracker == 1 ):
		env.Append( CPPDEFINES = [ 'USE_TRACKER' ] )

print '\n'

env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(sharepath) + '\\"')])
versid_cppdefines=env['CPPDEFINES'][:]
versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % extra_version))
versid_cppdefines.append(('DESCENT_VERSION_BUILD_TIME', '\\"%s\\"' % extra_version_build_time))
env.Object(source = ['%s%s' % (builddir, 'main/vers_id.cpp')], CPPDEFINES=versid_cppdefines)
common_sources += ['main/vers_id.o']
# finally building program...
env.Program(target='%s%s' % (builddir, str(target)), source = [('%s%s' % (builddir, s)) for s in common_sources], LIBS = libs, LINKFLAGS = str(lflags))
if (sys.platform != 'darwin'):
	env.Install(BIN_DIR, str(target))
	env.Alias('install', BIN_DIR)
else:
	tool_bundle.TOOL_BUNDLE(env)
	env.MakeBundle(PROGRAM_NAME + '.app', target,
                       'free.d2x-rebirth', 'd2xgl-Info.plist',
                       typecode='APPL', creator='DCT2',
                       icon_file='arch/cocoa/d2x-rebirth.icns',
                       subst_dict={'d2xgl' : target},	# This is required; manually update version for Xcode compatibility
                       resources=[['English.lproj/InfoPlist.strings', 'English.lproj/InfoPlist.strings']])

# show some help when running scons -h
Help(PROGRAM_NAME + ', SConstruct file help:' +
	"""

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	
	'sharepath=[DIR]' (non-Mac OS *NIX only) use [DIR] for shared game data. [default: /usr/local/share/games/d2x-rebirth]
	'opengl=[0/1]'        build with OpenGL support [default: 1]
	'opengles=[0/1]'      build with OpenGL ES support [default: 0]
	'sdlmixer=[0/1]'      build with SDL_Mixer support for sound and music (includes external music support) [default: 1]
	'asm=[0/1]'           build with ASSEMBLER code (only with opengl=0, requires NASM and x86) [default: 0]
	'debug=[0/1]'         build DEBUG binary which includes asserts, debugging output, cheats and more output [default: 0]
	'profiler=[0/1]'      profiler build [default: 0]
	'editor=[0/1]'        include editor into build (!EXPERIMENTAL!) [default: 0]
	'ipv6=[0/1]'          enable IPv6 compability [default: 0]
	'use_udp=[0/1]'       enable UDP support [default: 1]
	'use_tracker=[0/1]'   enable Tracker support (requires udp) [default :1]
	'verbosebuild=[0/1]'  print out all compiler/linker messages during building [default: 0]
	
	Default values:
	""" + ' sharepath = ' + DATA_DIR + """

	Some influential environment variables:
	  CC          C compiler command
	  CFLAGS      C compiler flags
	  LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a
                      nonstandard directory <lib dir>
                      <include dir>
	  CXX         C++ compiler command
	  CXXFLAGS    C++ compiler flags
        """)
#EOF
