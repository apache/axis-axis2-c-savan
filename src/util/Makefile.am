datadir=$(prefix)/modules/savan
noinst_LTLIBRARIES = libsavan_util.la

libsavan_util_la_SOURCES = savan_util.c sqlite3.c db_mgr.c

data_DATA=template.xsl
INCLUDES = -I$(top_builddir)/include \
		@AXIOMINC@ \
		@AXIS2INC@ \
		@UTILINC@ \
		@NEETHIINC@

libsavan_util_la_LIBADD = \
                         -lxml2

EXTRA_DIST=template.xsl
