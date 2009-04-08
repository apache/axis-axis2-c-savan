lib_LTLIBRARIES = libsavan_storage.la

libsavan_storage_la_SOURCES = storage_mgr.c sqlite3.c

libsavan_storage_la_LIBADD = ../common/libsavan_common_storage.la

INCLUDES = -I$(top_builddir)/include \
			-I$(top_builddir)/include/savan \
			-I. \
			@AXIS2INC@ \
		   	@AXIOMINC@ \
		   	@NEETHIINC@ \
		   	@UTILINC@

EXTRA_DIST=
