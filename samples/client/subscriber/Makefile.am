prgbindir=$(prefix)/bin/samples/savan
prgbin_PROGRAMS = subscriber 
samplesdir=$(prefix)/samples/client/subscriber
subscriber_SOURCES = subscriber.c

subscriber_LDADD   = $(LDFLAGS) \
                    -L$(AXIS2C_HOME)/lib \
					-L../../../src/core \
                    $(AXIS2C_HOME)/lib/libaxutil.la \
                    $(AXIS2C_HOME)/lib/libaxis2_axiom.la \
                    $(AXIS2C_HOME)/lib/libaxis2_engine.la \
                    $(AXIS2C_HOME)/lib/libaxis2_http_sender.la \
                    $(AXIS2C_HOME)/lib/libaxis2_http_receiver.la \
                    -lpthread \
                    $(GUTHTHILA_LIBS) \
                    $(LIBXML2_LIBS)\
					-lmod_savan

INCLUDES = -I$(AXIS2C_HOME)/include \
			@AXIOMINC@ \
			@UTILINC@ \
			-I ../../../../axis2c/include \
			-I ../../../../axis2c/axiom/include \
			-I ../../../../axis2c/util/include \
			-I ../../../../axis2c/neethi/include \
			-I../../../include
