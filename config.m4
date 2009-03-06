PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_ARG_ENABLE(64, whether to compile for 64-bit architecture,
[  --enable-64   Compile for 64-bit architecture], no, no)

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, mongo.cpp mongo_id.cpp mongo_regex.cpp mongo_date.cpp mongo_bindata.cpp gridfs.cpp bson.cpp, $ext_shared)

  for dir in /usr /usr/local /opt/mongo ~/mongo .; do
    if test -e $dir && test -e $dir/lib/mongo && test -e $dir/include/mongo; then
      mongo=$dir
      break
    fi
  done

  for dir in /usr/lib /usr/local/lib; do
    if test -e $dir/lib/libboost_thread-mt.a; then
      boost=$dir
      break
    fi
  done

  LDFLAGS="$LDFLAGS -L$mongo/lib -L$boost/lib -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_program_options-mt"
  INCLUDES="$INCLUDES -I$mongo/include"

  CXX=g++
  CC=g++

  if test "$PHP_64" != "no"; then
    CPPFLAGS="$CPPFLAGS -m64"
    LDFLAGS="$LDFLAGS -m64"
  fi
fi

