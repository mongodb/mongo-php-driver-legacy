PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_ARG_ENABLE(64, whether to compile for 64-bit architecture,
[  --enable-64   Compile for 64-bit architecture], no, no)

PHP_ARG_WITH(mongodb, MongoDB install directory,
[  --with-mongodb[=DIR]       Set path to mongodb install])

PHP_ARG_WITH(boost, Boost path,
[  --with-boost[=DIR]       Set path to Boost])

PHP_ARG_WITH(pcre, PCRE path,
[  --with-pcre[=DIR]       Set path to PCRE])

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, mongo.c mongo_types.c gridfs.cpp bson.c, $ext_shared)

  AC_MSG_CHECKING(for MongoDB install)
  for dir in $PHP_MONGODB /usr /usr/local /opt/mongo ~/mongo .; do
    if test -e $dir && test -e $dir/lib/mongo && test -e $dir/include/mongo; then
      mongo=$dir
      break
    fi
  done
  if test -z "$mongo"; then
    AC_MSG_ERROR([MongoDB install not found.])
  fi
  AC_MSG_RESULT($mongo)

  AC_MSG_CHECKING(for Boost)
  for dir in $PHP_BOOST /opt/boost /opt/local /usr /usr/local; do
    if test -e $dir && test -e $dir/lib/libboost_thread-mt.a; then
      boost=$dir
      break
    fi
  done
  if test -z "$boost"; then
    AC_MSG_ERROR([Boost not found.])
  fi
  AC_MSG_RESULT($boost)

  AC_MSG_CHECKING(for PCRE)
  for dir in $PHP_PCRE /usr/64 /opt/local /usr /usr/local; do
    if test -e $dir/include/pcrecpp.h; then
      pcre=$dir
      break
    fi
  done
  if test -z "$pcre"; then
    AC_MSG_ERROR([PCRE not found.])
  fi
  AC_MSG_RESULT($pcre)


  LDFLAGS="$LDFLAGS -L$mongo/lib -L$boost/lib -L$pcre/lib -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_program_options-mt"
  dnl boost >= 1.35 needs boost_system-mt
  if test -e $boost/lib/libboost_system-mt.a; then
    LDFLAGS="$LDFLAGS -lboost_system-mt"
  fi

  INCLUDES="$INCLUDES -I$mongo/include -I$boost/include -I$pcre/include"

  CXX=g++
  CC=g++

  if test "$PHP_64" != "no"; then
    CPPFLAGS="$CPPFLAGS -m64"
    LDFLAGS="$LDFLAGS -m64"
  fi
fi

