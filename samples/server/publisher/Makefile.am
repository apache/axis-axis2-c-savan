prglibdir=$(prefix)/services/publisher
samplesdir=$(prefix)/samples/server/publisher
prglib_LTLIBRARIES = libpublisher.la
prglib_DATA= services.xml
noinst_HEADERS = publisher.h
SUBDIRS =

libpublisher_la_SOURCES = publisher.c publisher_skeleton.c

libpublisher_la_LIBADD  = -L$(AXIS2C_HOME)/lib -lsavan_client

INCLUDES =  -I@AXIS2INC@ \
		    	@AXIS2INC@ \
				@SAVANINC@

EXTRA_DIST=services.xml
