dnl run autogen.sh to generate the configure script.

AC_PREREQ(2.59)

AC_INIT(savanc-src, 1.0)
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([tar-ustar])
AC_PREFIX_DEFAULT(/usr/local/savanc)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Checks for libraries.
AC_CHECK_LIB(dl, dlopen)

if test -d $srcdir/samples; then
    AC_CONFIG_SUBDIRS(samples)
fi

CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE"
if test "$GCC" = "yes"; then
    CFLAGS="$CFLAGS -ansi -Wall -Wno-implicit-function-declaration"
    #CFLAGS="$CFLAGS -ansi -Wall -Wno-implicit-function-declaration"
fi
LDFLAGS="-lpthread"

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h string.h])

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_C_CONST

dnl Checks for library functions.
dnl AC_FUNC_MALLOC
dnl AC_FUNC_REALLOC
#AC_CHECK_FUNCS([memmove])
abs_top_builddir=`pwd`
AC_SUBST(abs_top_builddir)

AC_MSG_CHECKING(path to use Axis2C . This is a compulsory to build Savan/C)
AC_ARG_WITH(axis2,
[  --with-axis2[=PATH]      use axis2c.],
[ case "$withval" in
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT($withval)
    dnl Find axis2 include dir in the path
    if test -d $withval; then
        AXIS2INC="-I$withval"
    dnl else find the axis2inc include dir in $(AXIS2C_HOME)/include
    elif [ -d "$abs_top_builddir" ]; then
            AXIS2INC="-I$abs_top_builddir/../axis2c/include"
    else
        AC_MSG_ERROR(could not find axis2inc. stop)
    fi
    ;;
  esac ],
  AXIS2INC="-I$abs_top_builddir/../axis2c/include"
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING(whether to enable savan filtering)
AC_ARG_ENABLE(filtering, [  --enable-filtering
                          enable savan filtering(default=no)],
[ case "${enableval}" in
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT(yes)
    PKG_CHECK_MODULES(XSLT, libxslt)
    CFLAGS="$CFLAGS -DSAVAN_FILTERING $XSLT_CFLAGS"
    LDFLAGS="$LDFLAGS -lxslt"

    ;;
  esac ],
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING(whether to build sqlite based storage)
AC_ARG_ENABLE(sqlite, [  --enable-sqlite
                          build Sqlite based storage (default=yes)],
[ case "${enableval}" in
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT(yes)
    SAVAN_STORAGE="sqlite"

    ;;
  esac ],
  AC_MSG_RESULT(yes)
  SAVAN_STORAGE="sqlite"
)


AC_MSG_CHECKING(whether to build service based storage)
AC_ARG_ENABLE(service, [  --enable-service    
                          build service based storage (default=no)],
[ case "${enableval}" in
  no)
    AC_MSG_RESULT(no)
    SAVAN_STORAGE=""
    SVC_DIR=""
    ;;
  *)
    AC_MSG_RESULT(yes)
    SAVAN_STORAGE="service"
    SVC_DIR="subs_mgr"
    ;;
  esac ],
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING(To Use Registry based storage manager . This is an optional module to build Savan C)
AC_ARG_WITH(registry,
[  --with-registry[=PATH]     Find the REGISTRY header files in 'PATH'.
    'PATH' should point to REGISTRY include files location.
    If you omit the '=PATH' part completely, the configure script will search
    '$(AXIS2C_HOME)/include/registry-0.1.0' for REGISTRY headers.],
[ case "$withval" in
  no)
    AC_MSG_RESULT(no)
    SAVAN_STORAGE=""
    ;;
  *)
    AC_MSG_RESULT(yes)
    SAVAN_STORAGE="registry"
    dnl Find registry include dir in the path
    if test -d $withval; then
        registryinc="-I$withval"
    dnl else find the registry include dir in $(AXIS2C_HOME)/include
    elif test -d '$(AXIS2C_HOME)/include'; then
        registryinc="-I$(AXIS2C_HOME)/include/registry-0.1.0"
    else
        AC_MSG_ERROR(could not find registry. stop)
    fi
    ;;
  esac ],
  AC_MSG_RESULT(no)
)


AC_MSG_CHECKING(To Use Xml Schema.)
AXIOMINC="-I$abs_top_builddir/../axis2c/axiom/include"
UTILINC="-I$abs_top_builddir/../axis2c/util/include"
NEETHIINC="-I$abs_top_builddir/../axis2c/neethi/include"
CFLAGS="$CFLAGS $LIBXML2_CFLAGS"

REGISTRYINC=$registryinc

AC_SUBST(REGISTRYINC)
AC_SUBST(AXIS2INC)
AC_SUBST(AXIOMINC)
AC_SUBST(UTILINC)
AC_SUBST(NEETHIINC)
AC_SUBST(XMLSCHEMAINC)
AC_SUBST(SAVAN_STORAGE)
AC_SUBST(SVC_DIR)

export SAVAN_STORAGE
export SVC_DIR

AC_CONFIG_FILES([Makefile \
    src/Makefile \
    src/client/Makefile\
    src/core/Makefile\
    src/handlers/Makefile \
    src/msgreceivers/Makefile\
    src/subscribers/Makefile\
    src/storage/Makefile\
    src/storage/common/Makefile\
    src/storage/sqlite/Makefile\
    src/storage/registry/Makefile\
    src/storage/registry/test/Makefile\
    src/storage/service/Makefile\
    src/subs_mgr/Makefile\
    src/data/Makefile\
    src/util/Makefile
    ])
    
AC_OUTPUT
