PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[	--enable-mongo					Enable the MongoDB extension])

PHP_MONGO_CFLAGS="-I@ext_builddir@/util"

if test "$PHP_MONGO" != "no"; then
	AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
	PHP_NEW_EXTENSION(mongo, php_mongo.c mongo.c mongoclient.c mongo_types.c bson.c cursor.c collection.c db.c gridfs.c gridfs_stream.c util/hash.c util/log.c util/pool.c mcon/bson_helpers.c mcon/collection.c mcon/connections.c mcon/io.c mcon/manager.c mcon/mini_bson.c mcon/parse.c mcon/read_preference.c mcon/str.c mcon/utils.c, $ext_shared,, $PHP_MONGO_CFLAGS)

	PHP_ADD_BUILD_DIR([$ext_builddir/util], 1)
	PHP_ADD_INCLUDE([$ext_builddir/util])
	PHP_ADD_INCLUDE([$ext_srcdir/util])
	PHP_ADD_BUILD_DIR([$ext_builddir/mcon], 1)
	PHP_ADD_INCLUDE([$ext_builddir/mcon])
	PHP_ADD_INCLUDE([$ext_srcdir/mcon])

	dnl call acinclude func to check endian-ness
	PHP_C_BIGENDIAN
	if test "$ac_cv_c_bigendian_php" = "yes"; then
		 CFLAGS="$CFLAGS -DPHP_C_BIGENDIAN"
	fi
	dnl default to little-endian

	case $build_os in
	darwin1*.*.*)
		AC_MSG_CHECKING([whether to compile for recent osx architectures])
		CFLAGS="$CFLAGS -arch i386 -arch x86_64 -mmacosx-version-min=10.5"
		AC_MSG_RESULT([yes])
		;;
	darwin*)
		AC_MSG_CHECKING([whether to compile for every osx architecture ever])
		CFLAGS="$CFLAGS -arch i386 -arch x86_64 -arch ppc -arch ppc64"
		AC_MSG_RESULT([yes])
		;;
	esac

fi

PHP_ARG_ENABLE(coverage,	whether to include code coverage symbols,
[	--enable-coverage				 Mongo: Enable code coverage symbols, maintainers only!], no, no)

if test "$PHP_COVERAGE" = "yes"; then

	if test "$GCC" != "yes"; then
		AC_MSG_ERROR([GCC is required for --enable-coverage])
	fi

	dnl Check if ccache is being used
	case `$php_shtool path $CC` in
		*ccache*[)] gcc_ccache=yes;;
		*[)] gcc_ccache=no;;
	esac

	if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
		AC_MSG_ERROR([ccache must be disabled when --enable-coverage option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
	fi

	lcov_version_list="1.5 1.6 1.7 1.9 1.10"

	AC_CHECK_PROG(LCOV, lcov, lcov)
	AC_CHECK_PROG(GENHTML, genhtml, genhtml)
	PHP_SUBST(LCOV)
	PHP_SUBST(GENHTML)

	if test "$LCOV"; then
		AC_CACHE_CHECK([for lcov version], php_cv_lcov_version, [
			php_cv_lcov_version=invalid
			lcov_version=`$LCOV -v 2>/dev/null | $SED -e 's/^.* //'` #'
			for lcov_check_version in $lcov_version_list; do
				if test "$lcov_version" = "$lcov_check_version"; then
					php_cv_lcov_version="$lcov_check_version (ok)"
				fi
			done
		])
	else
		lcov_msg="To enable code coverage reporting you must have one of the following LCOV versions installed: $lcov_version_list"
		AC_MSG_ERROR([$lcov_msg])
	fi

	case $php_cv_lcov_version in
		""|invalid[)]
			lcov_msg="You must have one of the following versions of LCOV: $lcov_version_list (found: $lcov_version)."
			AC_MSG_ERROR([$lcov_msg])
			LCOV="exit 0;"
			;;
	esac

	if test -z "$GENHTML"; then
		AC_MSG_ERROR([Could not find genhtml from the LCOV package])
	fi

	PHP_ADD_MAKEFILE_FRAGMENT

	dnl Remove all optimization flags from CFLAGS
	changequote({,})
	CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
	CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
	changequote([,])

	dnl Add the special gcc flags
	CFLAGS="$CFLAGS -O0 -fprofile-arcs -ftest-coverage"
	CXXFLAGS="$CXXFLAGS -O0 -fprofile-arcs -ftest-coverage"
fi

