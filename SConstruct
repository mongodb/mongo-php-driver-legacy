# to compile:
#       sudo scons install
#

import os
import types 
import tempfile
import SCons.Util
from subprocess import Popen, PIPE

# -----------
# OPTIONS
# -----------

AddOption('--mongodb',
          dest='mongodb',
          type='string',
          default="/opt/mongo/",
          nargs=1,
          action='store',
          metavar='DIR',
          help='mongodb installation directory')

AddOption('--64',
        dest='force64',
        type='string',
        nargs=0,
        action="store",
        help="whether to force 64 bit" )

AddOption('--mac-boost',
        dest='boostlib',
        type='string',
        default="/opt/local",
        nargs=1,
        action="store",
        help="location of boost libraries for mac installs" )

def phpConfig( name ):
    x = Popen( "php -i | grep extension_dir", stdout=PIPE, shell=True ).communicate()[ 0 ].strip()
    arr = x.split( " => " )
    return arr[1]

extensionDir = phpConfig( "extension_dir" )

AddOption('--extension-dir',
          dest='extensionDir',
          default=extensionDir,
          nargs=1,
          action='store',
          help="path to store php extension")

# -----------
# GLOBAL SETUP
# -----------


env = Environment()

env.Append( CFLAGS=" -O0 -DPHP_ATOM_INC " )
env.Append( CPPPATH=[ "." ] )

boostLibs = [ "thread" , "filesystem" , "program_options" ]
nix = False

extensionDir = GetOption( "extensionDir" )
mongoHome = GetOption( "mongodb" )
boostLib = GetOption( "boostlib" )
force64 = not GetOption( "force64" ) is None

# -----------
# PLATFORM CONFIG
# -----------

if force64:
    env.Append( CPPFLAGS=" -m64 " )
    env.Append( LINKFLAGS=" -m64 " )

if "darwin" == os.sys.platform:
    env.Append( CPPPATH=[ "/sw/include" , boostLib + "/include"] );
    env.Append( LIBPATH=["/sw/lib/" , boostLib + "/lib/" ] )

    env.Append( LINKFLAGS=" -Wl,-flat_namespace -Wl,-undefined -Wl,suppress " )

    env.Append( CPPFLAGS=" -mmacosx-version-min=10.4 " )

    env["SHLINKFLAGS"] = SCons.Util.CLVar('$LINKFLAGS -bundle')

    nix = True

elif "linux2" == os.sys.platform:

    nix = True

if nix:
    env.Append( CPPFLAGS="-fPIC -fno-strict-aliasing -ggdb -pthread -O3 -Wall -Wsign-compare -Wno-non-virtual-dtor -DHAVE_CONFIG_H -g -O0 -DPHP_ATOM_INC" )
    env.Append( CPPFLAGS=" " + phpConfig( "includes" ) )

env.Append( CPPPATH=[ mongoHome + "/include"] )
env.Append( LIBPATH=[ mongoHome + "/lib" ] )


# -----------
# SYSTEM CHECK
# -----------

conf = Configure( env )

if not conf.CheckHeader( "php.h" ):
    print( "can't find php.h" )
    Exit(1)

def myCheckLib( poss ):

    if type( poss ) != types.ListType :
        poss = [poss]
        
    allPlaces = [];
    if nix:
        allPlaces += env["LIBPATH"]
        if not force64:
            allPlaces += [ "/usr/lib" , "/usr/local/lib" ]
            
        for p in poss:
            for loc in allPlaces:
                fullPath = loc + "/lib" + p + ".a"
                if os.path.exists( fullPath ):
                    env['_LIBFLAGS']='${_stripixes(LIBLINKPREFIX, LIBS, LIBLINKSUFFIX, LIBPREFIXES, LIBSUFFIXES, __env__)} $SLIBS'
                    env.Append( SLIBS=" " + fullPath + " " )
                    return True

    res = conf.CheckLib( poss )
    if res:
        return True

    return False

for b in boostLibs:
    l = "boost_" + b
    if not myCheckLib( [ l + "-mt" , l ] ):
        print( "ERROR: can't find boost library: " + b )
        Exit(1)

myCheckLib( "boost_system-mt" )

if not conf.CheckLib( "mongoclient" ):
    print( "ERROR: can't find mongo client " )
    Exit(1)

env = conf.Finish()

# -----------
# TARGETS
# -----------

lib = env.SharedLibrary( "mongo" , Glob( "src/*.cpp" ) );
env.Install( extensionDir , str( lib[0] ) )
env.Alias( "install" , extensionDir )
