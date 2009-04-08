lib_LTLIBRARIES = libsavan_subs_mgr.la

libsavan_subs_mgr_la_SOURCES = storage_mgr.c
libsavan_subs_mgr_la_LIBADD = ../common/libsavan_common_subs_mgr.la \
							 -lregistry_client

INCLUDES = -I$(top_builddir)/include \
			-I$(top_builddir)/include/savan \
			-I. \
			@AXIS2INC@ \
		   	@AXIOMINC@ \
		   	@NEETHIINC@ \
			@REGISTRYINC@ \
		   	@UTILINC@

EXTRA_DIST=
