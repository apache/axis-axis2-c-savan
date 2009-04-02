noinst_LTLIBRARIES = libsavan_publisher_mod.la

libsavan_publisher_mod_la_SOURCES = publisher_mod.c
libsavan_publisher_mod_la_LIBADD = ../common/libsavan_common_publisher_mod.la -lxml2

data_DATA=

INCLUDES = -I$(top_builddir)/include \
			-I$(top_builddir)/include/savan \
			-I. \
			@AXIS2INC@ \
		   	@AXIOMINC@ \
		   	@NEETHIINC@ \
			@XPATHINC@ \
		   	@UTILINC@

EXTRA_DIST=
