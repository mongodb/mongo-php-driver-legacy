# to compile:
#       sudo scons install
#

import os
import SCons.Util
from subprocess import Popen, PIPE

# -----------
# OPTIONS
# -----------



# -----------
# GLOBAL SETUP
# -----------

def phpConfig( name ):
    return Popen( [ "php-config", "--" + name ], stdout=PIPE ).communicate()[ 0 ].strip()

env = Environment()

env.Append( CFLAGS=" -O0 -DPHP_ATOM_INC " );
env.Append( CPPPATH=[ "." ] )

boostLibs = [ "thread" , "filesystem" , "program_options" ]
nix = False

extensionDir = phpConfig( "extension-dir" )

mongoHome = "/opt/mongo/"

# -----------
# PLATFORM CONFIG
# -----------


if "darwin" == os.sys.platform:
    env.Append( CPPPATH=[ "/sw/include" , "/opt/local/include"] )
    env.Append( LIBPATH=["/sw/lib/" , "/opt/local/lib/" ] )

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

for b in boostLibs:
    if not conf.CheckLib( [ "boost_" + b + "-mt" , "boost_" + b  ] ):
        print( "ERROR: can't find boost library: " + b )
        Exit(1)

conf.CheckLib( "boost_system-mt" )

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
