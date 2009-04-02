prglibdir=$(prefix)/lib
prglib_LTLIBRARIES = libsavan_client.la

libsavan_client_la_SOURCES = savan_client.c
libsavan_client_la_LIBADD = $(top_builddir)/src/subscribers/libsavan_subscribers.la \
							$(top_builddir)/src/util/libsavan_util.la \
							../storage/@SAVAN_STORAGE@/libsavan_storage.la \
							../filters/@SAVAN_FILTER_LIB@ 

INCLUDES = -I$(top_builddir)/include \
		@AXIS2INC@ \
		@UTILINC@ \
		@AXIOMINC@ \
		@NEETHIINC@ 
