prglibdir=$(prefix)/modules/savan
prglib_LTLIBRARIES = libmod_savan.la
prglib_DATA= ../data/module.xml

libmod_savan_la_SOURCES = mod_savan.c savan_sub_processor.c

INCLUDES = -I$(top_builddir)/include \
		@AXIOMINC@ \
		@UTILINC@ \
		@AXIS2INC@ \
		@NEETHIINC@

libmod_savan_la_LIBADD = \
                        ../handlers/libsavan_handlers.la\
						../util/libsavan_util.la \
						../storage/@SAVAN_STORAGE@/libsavan_storage.la
