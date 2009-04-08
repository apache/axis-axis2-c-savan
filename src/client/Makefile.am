prglibdir=$(prefix)/lib
prglib_LTLIBRARIES = libsavan_client.la

libsavan_client_la_SOURCES = savan_client.c
libsavan_client_la_LIBADD = $(top_builddir)/src/util/libsavan_util.la \
							../subs_mgrs/@SAVAN_SUBS_MGR@/libsavan_subs_mgr.la \
							../filters/@SAVAN_FILTER_LIB@ 

INCLUDES = -I$(top_builddir)/include \
		@AXIS2INC@ \
		@UTILINC@ \
		@AXIOMINC@ \
		@NEETHIINC@ 
