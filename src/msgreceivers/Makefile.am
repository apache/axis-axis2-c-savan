lib_LTLIBRARIES = libsavan_msgreceivers.la

libsavan_msgreceivers_la_SOURCES = savan_msg_recv.c

libsavan_msgreceivers_la_LIBADD = ../publisher/@SAVAN_PUBLISHER_LIB@ \
								  	../storage/@SAVAN_STORAGE@/libsavan_storage.la \
									../util/libsavan_util.la


INCLUDES = -I$(top_builddir)/include \
		@AXIS2INC@ \
		@AXIOMINC@ \
		@UTILINC@ 
