AC_INIT(libgdiplus, [5.6.1],
	[https://bugzilla.xamarin.com/enter_bug.cgi?product=Class%20Libraries&component=libgdiplus])
	AC_CONFIG_SRCDIR([README])

AC_CANONICAL_SYSTEM
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AM_PROG_LIBTOOL

AC_C_BIGENDIAN

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi
GLIB_REQUIRED_VERSION="2.2.3"
PKG_CHECK_MODULES(BASE_DEPENDENCIES, glib-2.0 >= $GLIB_REQUIRED_VERSION)

GDIPLUS_LIBS="`$PKG_CONFIG --libs glib-2.0 `"
GDIPLUS_CFLAGS="`$PKG_CONFIG --cflags glib-2.0 `"

CAIRO_REQUIRED_VERSION="1.6.4"
PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_REQUIRED_VERSION)

# Optional use (experimental and unsupported) of Pango's text rendering on top of Cairo
AC_ARG_WITH(pango, [  --with-pango],[text_v=pango],[text_v=cairo])

CAIRO_DIR=""
CAIRO_LIBS="`pkg-config --libs cairo `"
CAIRO_CFLAGS="`pkg-config --cflags cairo `"
cairo_info="`pkg-config --modversion cairo ` (system)"
GDIPLUS_PKG_REQ="cairo"

if test $text_v = "pango"; then
	PANGO_REQUIRED_VERSION="1.10"
	PKG_CHECK_MODULES(PANGO, pango >= $PANGO_REQUIRED_VERSION)
	PANGO_LIBS="`pkg-config --libs pangocairo `"
	PANGO_CFLAGS="`pkg-config --cflags pangocairo `"
	AC_DEFINE(USE_PANGO_RENDERING,1,[Use Pango to measure and draw text (INCOMPLETE and UNSUPPORTED)])
	GDIPLUS_PKG_REQ="$GDIPLUS_PKG_REQ pangocairo-1.0"
fi

GDIPLUS_LIBS="$GDIPLUS_LIBS $CAIRO_LIBS $PANGO_LIBS"
GDIPLUS_CFLAGS="$GDIPLUS_CFLAGS $CAIRO_CFLAGS $PANGO_CFLAGS"

# Fall back to using fontconfig/freetype-config if their .pc is not found
PKG_CHECK_MODULES(FONTCONFIG, fontconfig,
	[fontconfig_pkgconfig=yes], [fontconfig_pkgconfig=no])
if test "x$FONTCONFIG" = "xno"; then
	AC_PATH_PROG(FONTCONFIG_CONFIG, fontconfig-config, no)
	if test "x$FONTCONFIG_CONFIG" = "xyes"; then
		FONTCONFIG_CFLAGS="`fontconfig-config --cflags`"
		FONTCONFIG_LIBS="`fontconfig-config --libs`"
	fi
fi
AC_CHECK_LIB(fontconfig, FcInit)
AC_CHECK_FUNCS(FcFini)

PKG_CHECK_MODULES(FREETYPE2, freetype2,
	[freetype2_pkgconfig=yes], freetype2_pkgconfig=no])
if test "x$FREETYPE2" = "xno"; then
	AC_PATH_PROG(FREETYPE_CONFIG, freetype-config, no)
	if test "x$FREETYPE_CONFIG" = "xyes"; then
		FREETYPE2_CFLAGS="`freetype-config --cflags`"
		FREETYPE2_LIBS="`freetype-config --libs`"
	fi
fi

GDIPLUS_LIBS="$GDIPLUS_LIBS $FONTCONFIG_LIBS $FREETYPE2_LIBS"
GDIPLUS_CFLAGS="$GDIPLUS_CFLAGS $FONTCONFIG_CFLAGS $FREETYPE2_CFLAGS"

AC_CHECK_HEADERS(byteswap.h)

AC_MSG_CHECKING([host threading settings])
case "$host" in
	*-*-mingw*|*-*-cygwin*)
		AC_DEFINE_UNQUOTED(CYGWIN,1,[Win32])
		AC_CHECK_LIB(intl, gettext)
		;;

	*-*-*netbsd*)
		AC_DEFINE_UNQUOTED(BSDOS,1,[NetBSD])
		;;

	*-*-*freebsd*)
		AC_DEFINE_UNQUOTED(BSDOS,1,[FreeBSD])
		CFLAGS="-D_THREAD_SAFE $CFLAGS"
		;;

	*-*-*openbsd*)
		AC_DEFINE_UNQUOTED(BSDOS,1,[OpenBSD])
		CFLAGS="$CFLAGS"
		LIBS="$LIBS"
		;;

	*-*-linux*)
		AC_DEFINE_UNQUOTED(LINUX,1,[Linux])
		AC_CHECK_LIB(pthread, pthread_create)
		;;

	*-*-solaris*)
		AC_DEFINE_UNQUOTED(SOLARIS,1,[Solaris])
		AC_DEFINE_UNQUOTED(_REENTRANT,1,[For libc reentrancy])
		CFLAGS="-pthreads $CFLAGS"
		;;

	*-*-darwin*)
		AC_DEFINE_UNQUOTED(OSX,1,[OS X])
		CFLAGS="-no-cpp-precomp $CFLAGS"
		LDFLAGS="-flat_namespace -undefined suppress $LDFLAGS"
		no_x=yes
		no_libpng16=yes # workaround until we can upgrade the libpng used by bockbuild
		;;

	*)
		AC_MSG_RESULT(Unrecognized host $host)
		AC_DEFINE_UNQUOTED(OTHEROS,1,[Unknown])
		;;
esac

# Apparently for BSD special handling needs to happen
# See http://archives/neohapsis.com/archives/openbsd/2001-11/2142.html
# by Miod Vallat <miod@online.fr>
AC_CACHE_CHECK(
  [if compiler recognizes -pthread],
   myapp_cv_gcc_pthread,
   ac_save_CFLAGS=$CFLAGS
   CFLAGS="$CFLAGS -pthread"
   AC_LANG_SAVE
   AC_LANG_C
   AC_TRY_LINK([#include <pthread.h>],
               [void *p = pthread_create;],
                myapp_cv_gcc_pthread=yes,
                myapp_cv_gcc_pthread=no)
   AC_LANG_RESTORE
   CFLAGS=$ac_save_CFLAGS
)
if test $myapp_cv_gcc_pthread = yes
then
   myapp_threads=""
   CFLAGS="$CFLAGS -pthread"
else
   AC_CHECK_LIB(pthread,pthread_create,myapp_threads=-lpthread,
                [AC_CHECK_LIB(c_r,pthread_create,myapp_threads=-lc_r)])
fi


# sigsetjmp is a macro on some platforms, so can't use AC_CHECK_FUNCS
AC_MSG_CHECKING(for sigsetjmp)
AC_TRY_LINK([#include <setjmp.h>], [
sigjmp_buf env;
sigsetjmp(env, 0);
], gdip_ok=yes, gdip_ok=no)
AC_MSG_RESULT($gdip_ok)
if test "$gdip_ok" = "yes"; then
  AC_DEFINE(HAVE_SIGSETJMP, 1, sigsetjmp is available)
fi


# when possible hide stuff we don't want to export
AC_MSG_CHECKING(for visibility __attribute__)
AC_TRY_COMPILE([], [
   void __attribute__ ((visibility ("hidden"))) doit (void) {}
   void main () { doit (); }
], [
   have_visibility_hidden=yes
   AC_MSG_RESULT(yes)
], [
   have_visibility_hidden=no
   AC_MSG_RESULT(no)
])

case "$host" in
	i*86-*-*)
		case $host_os in
		  solaris*)
			# On solaris 10 x86, gcc prints a warning saying 'visibility attribute not supported on this configuration; ignored', but linking fails.
			have_visibility_hidden=no
		esac
		;;
esac

if test "x$have_visibility_hidden" = "xyes"; then
   AC_DEFINE(HAVE_VISIBILITY_HIDDEN, 1, [Support for the visibility ("hidden") attribute])
fi


AC_ARG_WITH(libexif,
	[AC_HELP_STRING([--without-libexif], [disable EXIF support])])

dnl If the user doesn't force a --prefix= then assume the OS default for resolcing libtiff etc
case $prefix in
NONE) prefix=$ac_default_prefix ;;
esac

dnl Test for libjpeg (this needs to be checked before libtiff, since libtiff depends on libjpeg)
AC_ARG_WITH([libjpeg],
	AS_HELP_STRING([--with-libjpeg=PREFIX],
	[Prefix where libjpeg is installed, or 'no' to disable]),
	[libjpeg_prefix="$withval"], [libjpeg_prefix="${prefix}"])

  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    JPEG_CFLAGS="-I${libjpeg_prefix}/include"
    JPEG_LIBS="-L${libjpeg_prefix}/lib"
    save_cflags=$CFLAGS; CFLAGS=$JPEG_CFLAGS
    save_libs=$LIBS; LIBS=$JPEG_LIBS
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      jpeg_ok=yes,
      jpeg_ok=no
      AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***))
    if test "$jpeg_ok" = yes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
        jpeg_ok=yes,
        jpeg_ok=no)
      AC_MSG_RESULT($jpeg_ok)
      if test "$jpeg_ok" = yes; then
        JPEG='jpeg'; LIBJPEG="-ljpeg"

        if test "$libjpeg_prefix" != "NONE"; then
          LIBJPEG="$LIBJPEG -L$libjpeg_prefix"
        fi

      else
        AC_MSG_WARN(*** JPEG loader will not be built (JPEG header file not found) ***)
      fi
    fi
    CFLAGS=$save_cflags
    LIBS=$save_libs
  fi

if test x$JPEG != x; then
  GDIPLUS_LIBS="$GDIPLUS_LIBS $LIBJPEG"
  AC_DEFINE(HAVE_LIBJPEG, 1, Define if jpeg support is available)
else
  jpeg_ok="no (Get it from http://freshmeat.net/projects/libjpeg)"
fi

dnl Test for libtiff
AC_ARG_WITH([libtiff],
	AS_HELP_STRING([--with-libtiff=PREFIX],
	[Prefix where libtiff is installed, or 'no' to disable]),
	[libtiff_prefix="$withval"], [libtiff_prefix="${prefix}"])

  if test x$with_libtiff != xno && test -z "$LIBTIFF"; then
    TIFF_CFLAGS="-I${libjpeg_prefix}/include -I${libtiff_prefix}/include"
    TIFF_LIBS="-L${libjpeg_prefix}/lib -L${libtiff_prefix}/lib"
    save_cflags=$CFLAGS; CFLAGS=$TIFF_CFLAGS
    save_libs=$LIBS; LIBS=$TIFF_LIBS
    AC_CHECK_LIB(tiff, TIFFReadScanline,
      [AC_CHECK_HEADER(tiffio.h,
        TIFF='tiff'; LIBTIFF='-ltiff'; tiff_ok=yes,
        AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
      [AC_CHECK_LIB(tiff, TIFFWriteScanline,
        [AC_CHECK_HEADER(tiffio.h,
          TIFF='tiff'; LIBTIFF='-ltiff -ljpeg -lz -llzma'; tiff_ok=yes,
          AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
        [AC_CHECK_LIB(tiff34, TIFFFlushData,
          [AC_CHECK_HEADER(tiffio.h,
            TIFF='tiff'; LIBTIFF='-ltiff34 -ljpeg -lz -llzma'; tiff_ok=yes,
            AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
        AC_MSG_WARN(*** TIFF plug-in will not be built (TIFF library not found) ***),  -lz -lm)], -ljpeg -lz -lm)], -lm)
    CFLAGS=$save_cflags
    LIBS=$save_libs
  fi

if test x$TIFF != x; then
  GDIPLUS_LIBS="$GDIPLUS_LIBS $LIBTIFF"
  AC_DEFINE(HAVE_LIBTIFF, 1, Define if tiff support is available)
else
  tiff_ok="no (Get it from http://www.libtiff.org/)"
fi

dnl Test for libgif or libungif
AC_ARG_WITH([libgif],
	AS_HELP_STRING([--with-libgif=PREFIX],
	[Prefix where libgif is installed, or 'no' to disable]),
	[libgif_prefix="$withval"], [libgif_prefix="${prefix}"])

  if test x$with_libgif != xno && test -z "$LIBGIF"; then
    GIF_CFLAGS="-I${libgif_prefix}/include"
    GIF_LIBS="-L${libgif_prefix}/lib"
    save_cflags=$CFLAGS; CFLAGS=$GIF_CFLAGS
    save_libs=$LIBS; LIBS=$GIF_LIBS
    AC_CHECK_LIB(gif, DGifOpenFileName,
      [AC_CHECK_HEADER(gif_lib.h,
        GIF='gif'; LIBGIF='-lgif'; gif_ok=yes,
	AC_MSG_WARN(*** GIF loader will not be built (giflibrary not found) ***))],
      AC_MSG_WARN(*** GIF loader will not be built (giflibrary not found) ***))

    AC_CHECK_LIB(ungif, DGifOpenFileName,
      [AC_CHECK_HEADER(gif_lib.h,
	GIF='ungif'; LIBGIF='-lungif'; gif_ok=yes,
	AC_MSG_WARN(*** GIF loader will not be built (ungiflibrary not found) ***))],
      AC_MSG_WARN(*** GIF loader will not be built (ungiflibrary not found) ***))
    CFLAGS=$save_cflags
    LIBS=$save_libs
  fi

if test x$GIF != x; then
  GDIPLUS_LIBS="$GDIPLUS_LIBS $LIBGIF"
  AC_DEFINE(HAVE_LIBGIF, 1, Define if gif support is available)
else
  gif_ok="no (See http://sourceforge.net/projects/libgif)"
fi

dnl Test for libpng
    libpng_found=no

    AC_MSG_CHECKING(for libpng16)
    if test "x$no_libpng16" != "x"; then
      AC_MSG_RESULT(skipped)
    elif $PKG_CONFIG --exists libpng16 ; then
      AC_MSG_RESULT(yes)
      PNG='png'
      PNG_DEP_CFLAGS_PACKAGES=libpng16
      LIBPNG=`$PKG_CONFIG --libs libpng16`
      libpng_found=yes
    else
      AC_MSG_RESULT(no)
    fi

    if test "x$libpng_found" != "xyes"; then
      AC_MSG_CHECKING(for libpng14)
      if $PKG_CONFIG --exists libpng14 ; then
        AC_MSG_RESULT(yes)
        PNG='png'
        PNG_DEP_CFLAGS_PACKAGES=libpng14
        LIBPNG=`$PKG_CONFIG --libs libpng14`
        libpng_found=yes
      else
        AC_MSG_RESULT(no)
      fi
    fi

    if test "x$libpng_found" != "xyes"; then
      AC_MSG_CHECKING(for libpng12)
      if $PKG_CONFIG --exists libpng12 ; then
        AC_MSG_RESULT(yes)
        PNG='png'
        PNG_DEP_CFLAGS_PACKAGES=libpng12
        LIBPNG=`$PKG_CONFIG --libs libpng12`
        libpng_found=yes
      else
        AC_MSG_RESULT(no)
      fi
    fi

    if test "x$libpng_found" != "xyes"; then
      AC_MSG_CHECKING(for png)
      AC_CHECK_LIB(png, png_read_info,
        [AC_CHECK_HEADER(png.h, png_ok=yes, png_ok=no)],
        AC_MSG_ERROR(*** libpng12 not found. See http://www.libpng.org/pub/png/libpng.html.), -lz -lm)
      if test "$png_ok" = yes; then
        AC_MSG_CHECKING([for png_structp in png.h])
        AC_TRY_COMPILE([#include <png.h>],
          [png_structp pp; png_infop info; png_colorp cmap; png_create_read_struct;],
          png_ok=yes,
          png_ok=no)
        AC_MSG_RESULT($png_ok)
        if test "$png_ok" = yes; then
          PNG='png'; LIBPNG='-lpng -lz'
        else
          AC_MSG_ERROR(*** libpng12 found, but it is too old. See http://www.libpng.org/pub/png/libpng.html.)
        fi
      else
          AC_MSG_ERROR(*** libpng12 not found. See http://www.libpng.org/pub/png/libpng.html.)
      fi
    fi


GDIPLUS_LIBS="$GDIPLUS_LIBS $LIBPNG"
AC_DEFINE(HAVE_LIBPNG, 1, Define if png support is available. Always defined.)

dnl
dnl Test for X11. Allow compiling without x11 support using the without-x11
dnl flag
dnl

AC_ARG_WITH(x11,
	[AC_HELP_STRING([--without-x11], [Disable X11 support])],
	[disable_x11=yes])

if test x$disable_x11 != xyes; then
  AC_MSG_CHECKING(X11 support)
  AC_TRY_COMPILE(
    [#include <X11/Xlib.h>],
    [],
    [x11_available=yes],
    [x11_available=no])

  if test "x$x11_available" != "xyes"; then
    AC_MSG_ERROR( [ Failed to compile with X11/Xlib.h include. You must fix your compiler paths ])
  else
    AC_MSG_RESULT(yes)
  fi
else
  x11_available=no
fi

AS_IF(
	[test x$x11_available = xyes],
	[AC_DEFINE([HAS_X11], [1], [Define X11 support])],
	[AC_DEFINE([HAS_X11], [0], [Skip X11 support])])
AM_CONDITIONAL([HAS_X11], [test x$x11_available = xyes])

# Add xrender here so that we don't fail to find glib if we won't have xrender.pc
if test "x$no_x" != "xyes" && "x$x11_available" = "xyes"; then
   GDIPLUS_LIBS="$GDIPLUS_LIBS `$PKG_CONFIG --libs xrender `"
   GDIPLUS_CFLAGS="$GDIPLUS_CFLAGS `$PKG_CONFIG --cflags xrender `"
fi

dnl Test for libexif
if test x$with_libexif != xno && test -z "$LIBEXIF"; then
  PKG_CHECK_MODULES(LIBEXIF, libexif, [libexif_pkgconfig=yes], libexif_pkgconfig=no])
  if test x$libexif_pkgconfig = xyes ; then
	GDIPLUS_CFLAGS="$GDIPLUS_CFLAGS $LIBEXIF_CFLAGS"
	GDIPLUS_LIBS="$GDIPLUS_LIBS $LIBEXIF_LIBS"
	AC_DEFINE(HAVE_LIBEXIF, 1, Define if libexif is available.)
  else
	libexif_pkgconfig="No. Get it from http://libexif.sourceforge.net/"
  fi
else
	libexif_pkgconfig="No. You disabled it."
fi
AM_CONDITIONAL(HAVE_LIBEXIF, test x$libexif_pkgconfig = xyes)

GDIPLUS_PKG_LIBS="$GDIPLUS_LIBS"
GDIPLUS_PKG_CFLAGS="$GDIPLUS_CFLAGS"

AC_SUBST(GDIPLUS_LIBS)
AC_SUBST(GDIPLUS_CFLAGS)
AC_SUBST(GDIPLUS_PKG_LIBS)
AC_SUBST(GDIPLUS_PKG_CFLAGS)
AC_SUBST(GDIPLUS_PKG_REQ)
AC_SUBST(CAIRO_DIR)

AC_OUTPUT([
Makefile
libgdiplus.pc
libgdiplus0.spec
src/Makefile
tests/Makefile])

echo "---"
echo "Configuration summary"
echo ""
echo "   * Installation prefix = $prefix"
echo "   * Cairo = $cairo_info"
echo "   * Text = $text_v"
echo "   * EXIF tags = $libexif_pkgconfig"
echo "   * X11 = $x11_available"
echo "   * Codecs supported:"
echo ""
echo "      - TIFF: $tiff_ok"
echo "      - JPEG: $jpeg_ok"
echo "      - GIF: $gif_ok"
echo "      - PNG: yes"
echo ""
echo "      NOTE: if any of the above say 'no' you may install the"
echo "            corresponding development packages for them, rerun"
echo "            autogen.sh to include them in the build."
echo ""
echo "---"