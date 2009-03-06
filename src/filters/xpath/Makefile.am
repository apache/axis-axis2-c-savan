noinst_LTLIBRARIES = libsavan_filter.la

libsavan_filter_la_SOURCES = filter.c
libsavan_filter_la_LIBADD = ../common/libsavan_common_filter.la \
							 -lxml2

INCLUDES = -I$(top_builddir)/include \
			-I$(top_builddir)/include/savan \
			-I. \
			@AXIS2INC@ \
		   	@AXIOMINC@ \
		   	@NEETHIINC@ \
			@XPATHINC@ \
		   	@UTILINC@

EXTRA_DIST=
