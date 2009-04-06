noinst_LTLIBRARIES = libsavan_publisher_mod.la
libsavan_publisher_mod_la_SOURCES = publisher_mod.c
data_DATA=
libsavan_publisher_mod_la_LIBADD = $(top_builddir)/src/util/libsavan_util.la \
							../../filters/@SAVAN_FILTER_LIB@ \
							../common/libsavan_common_publisher_mod.la

INCLUDES = -I$(top_builddir)/include \
			-I$(top_builddir)/include/savan \
			-I. \
			@AXIS2INC@ \
		   	@AXIOMINC@ \
		   	@NEETHIINC@ \
			@XPATHINC@ \
		   	@UTILINC@

EXTRA_DIST=
