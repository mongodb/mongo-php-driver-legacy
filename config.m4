PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_ARG_ENABLE(64, whether to compile for 64-bit architecture,
[  --enable-64   Compile for 64-bit architecture], no, no)

PHP_ARG_WITH(mongodb, location of the mongo database client,
[  --with-mongodb=path   Path to mongo client /usr/local], no, no)

PHP_ARG_WITH(boost, location of boost libraries,
[  --with-boost=path   Path to boost libraries /usr/lib], no, no)

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, src/mongo.cpp src/auth.cpp src/mongo_id.cpp src/mongo_regex.cpp src/mongo_date.cpp src/mongo_bindata.cpp src/gridfs.cpp src/bson.cpp, $ext_shared)

  if test "$PHP_MONGODB" != "no"; then
    mongolib=$PHP_MONGODB
  else
    mongolib=/usr/local
  fi

  if test "$PHP_BOOST" != "no"; then
    boostlib=$PHP_BOOST
  else
    boostlib=/usr/lib
  fi

  LDFLAGS="$LDFLAGS -L$mongolib/lib -L$boostlib -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_program_options-mt"
  INCLUDES="$INCLUDES -I$mongolib/include"

  CXX=g++
  CC=g++

  if test "$PHP_64" != "no"; then
    CPPFLAGS="$CPPFLAGS -m64"
    LDFLAGS="$LDFLAGS -m64"
  fi
fi

