####################################################
# Makefile for savan c				   #
# you can do 					   #
# nmake dist     - distribution  (dist / samples ) #
# nmake clean  	 - clean			   #
# nmake samples  - samples 			   #
####################################################

AUTOCONF = configure.in
!include $(AUTOCONF)

#define folders 

SAVANSRC = ..\..
SAVANDISTDIR = ..\savanc
SAVANINTDIR = .\int.msvc
SAVANDLL = mod_savan
SAVAN_CLIENT = savan_client

SAVAN_CODE = $(SAVANSRC)\src\core\*.c \
		 $(SAVANSRC)\src\client\*.c \
		 $(SAVANSRC)\src\handlers\*.c \
		 $(SAVANSRC)\src\util\*.c \
		 $(SAVANSRC)\src\subscribers\*.c \
		 $(SAVANSRC)\src\msgreceivers\*.c \
		 $(SAVANSRC)\src\storage\common\*.c \
		 $(SAVANSRC)\src\storage\inmemory\*.c 
		
#compiler options
CC = @cl.exe

CFLAGS = /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "AXIS2_DECLARE_EXPORT" /D_WIN32_WINNT=0x0400  /w /nologo \
	/I$(AXIS2_BIN_DIR)\include /I$(SAVANSRC)\include $(CRUNTIME) 

#linker options 
LD = @link.exe
LDFLAGS = /nologo /LIBPATH:$(AXIS2_BIN_DIR)\lib /LIBPATH:$(SAVANDISTDIR)\modules\savan /LIBPATH:$(SAVANDISTDIR)\lib
LIBS = axutil.lib axiom.lib axis2_parser.lib axis2_engine.lib axis2_http_receiver.lib axis2_http_sender.lib

#debug symbols
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS) /D "_DEBUG" /Od /Z7
LDFLAGS = $(LDFLAGS) /DEBUG
!else
CFLAGS = $(CFLAGS) /D "NDEBUG" /O2 
LDFLAGS = $(LDFLAGS)
!endif

##############
MT=mt.exe
MT="$(MT)"

!if "$(EMBED_MANIFEST)" == "0"
_VC_MANIFEST_EMBED_EXE=
_VC_MANIFEST_EMBED_DLL=
!else
_VC_MANIFEST_EMBED_EXE= if exist $@.manifest $(MT) -nologo -manifest $@.manifest -outputresource:$@;1
_VC_MANIFEST_EMBED_DLL= if exist $@.manifest $(MT) -nologo -manifest $@.manifest -outputresource:$@;2
!endif
###############

#create the directory structure

distdir:
	if not exist $(SAVANDISTDIR) mkdir $(SAVANDISTDIR)
	if not exist $(SAVANDISTDIR)\modules\savan mkdir $(SAVANDISTDIR)\modules\savan
	if not exist $(SAVANDISTDIR)\lib mkdir $(SAVANDISTDIR)\lib
	if not exist $(SAVANDISTDIR)\include mkdir $(SAVANDISTDIR)\include
	if not exist $(SAVANDISTDIR)\samples mkdir $(SAVANDISTDIR)\samples
	if not exist $(SAVANDISTDIR)\samples\bin mkdir $(SAVANDISTDIR)\samples\bin
	if not exist $(SAVANDISTDIR)\samples\bin\savanc mkdir $(SAVANDISTDIR)\samples\bin\savanc
	if not exist $(SAVANDISTDIR)\samples\src mkdir $(SAVANDISTDIR)\samples\src
	if not exist $(SAVANDISTDIR)\samples\src\savanc mkdir $(SAVANDISTDIR)\samples\src\savanc
	if not exist $(SAVANDISTDIR)\services\listener mkdir $(SAVANDISTDIR)\services\listener
	if not exist $(SAVANDISTDIR)\services\weather mkdir $(SAVANDISTDIR)\services\weather
	if not exist $(SAVANDISTDIR)\services\subscription mkdir $(SAVANDISTDIR)\services\subscription


intdir:
	if not exist $(SAVANINTDIR) mkdir $(SAVANINTDIR)
	if not exist $(SAVANINTDIR)\client mkdir $(SAVANINTDIR)\client
	if not exist $(SAVANINTDIR)\samples mkdir $(SAVANINTDIR)\samples
	if not exist $(SAVANINTDIR)\samples\subscriber mkdir $(SAVANINTDIR)\samples\subscriber
	if not exist $(SAVANINTDIR)\samples\listener mkdir $(SAVANINTDIR)\samples\listener
	if not exist $(SAVANINTDIR)\samples\weather mkdir $(SAVANINTDIR)\samples\weather
	if not exist $(SAVANINTDIR)\samples\subscription mkdir $(SAVANINTDIR)\samples\subscription
	
clean :
	@if exist $(SAVANDISTDIR) rmdir /S /Q $(SAVANDISTDIR)
	@if exist int.msvc rmdir /S /Q int.msvc

copy_extra:
	@if exist $(SAVANSRC)\README copy $(SAVANSRC)\README $(SAVANDISTDIR)
	@if exist $(SAVANSRC)\NEWS copy $(SAVANSRC)\NEWS $(SAVANDISTDIR)
	@if exist $(SAVANSRC)\COPYING copy $(SAVANSRC)\COPYING $(SAVANDISTDIR)
	@if exist $(SAVANSRC)\LICENSE copy $(SAVANSRC)\LICENSE $(SAVANDISTDIR)
	@if exist $(SAVANSRC)\INSTALL copy $(SAVANSRC)\INSTALL $(SAVANDISTDIR)
	@if exist  $(SAVANSRC)\AUTHORS copy $(SAVANSRC)\AUTHORS $(SAVANDISTDIR)
	
copy_samples:
	@xcopy /E $(SAVANSRC)\samples $(SAVANDISTDIR)\samples\src\savanc
	@del /s /q $(SAVANDISTDIR)\samples\src\savanc\*.am
	@del /q $(SAVANDISTDIR)\samples\src\savanc\*.*
	@copy $(SAVANSRC)\src\data\axis2.xml $(SAVANDISTDIR)\samples\src\savanc\server

copy_include:
	@xcopy /E $(SAVANSRC)\include $(SAVANDISTDIR)\include

$(SAVANDISTDIR)\modules\savan\$(SAVANDLL).dll:
	$(CC) $(CFLAGS) $(SAVAN_CODE) /Fo$(SAVANINTDIR)\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANINTDIR)\*.obj /DLL \
	/OUT:$(SAVANDISTDIR)\modules\savan\$(SAVANDLL).dll /IMPLIB:$(SAVANDISTDIR)\modules\savan\$(SAVANDLL).lib
	-@$(_VC_MANIFEST_EMBED_DLL)
	@copy $(SAVANSRC)\src\data\module.xml $(SAVANDISTDIR)\modules\savan\module.xml

savan: $(SAVANDISTDIR)\modules\savan\$(SAVANDLL).dll

##### savan client
SAVAN_CLIENT_SRC = $(SAVANSRC)\src\client\*.c \
		   $(SAVANSRC)\src\util\*.c \
		   $(SAVANSRC)\src\subscribers\*.c

$(SAVANDISTDIR)\lib\$(SAVAN_CLIENT).dll :
	if not exist $(SAVANINTDIR)\client mkdir $(SAVANINTDIR)\client
	$(CC) $(CFLAGS) $(SAVAN_CLIENT_SRC) /Fo$(SAVANINTDIR)\client\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANDLL).lib $(SAVANINTDIR)\client\*.obj /DLL \
	/OUT:$(SAVANDISTDIR)\lib\$(SAVAN_CLIENT).dll /IMPLIB:$(SAVANDISTDIR)\lib\$(SAVAN_CLIENT).lib
	-@$(_VC_MANIFEST_EMBED_DLL)

savan_client: $(SAVANDISTDIR)\lib\$(SAVAN_CLIENT).dll


###samples

$(SAVANDISTDIR)\services\subscription\subscription.dll :
	$(CC) $(CFLAGS) $(SAVANSRC)\src\subs_mgr\*.c /Fo$(SAVANINTDIR)\samples\subscription\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANDLL).lib $(SAVANINTDIR)\samples\subscription\*.obj  $(SAVAN_CLIENT).lib /DLL \
	/OUT:$(SAVANDISTDIR)\services\subscription\subscription.dll
	-@$(_VC_MANIFEST_EMBED_DLL)
	@copy $(SAVANSRC)\src\subs_mgr\services.xml $(SAVANDISTDIR)\services\subscription\

subscription: $(SAVANDISTDIR)\services\subscription\subscription.dll

$(SAVANDISTDIR)\services\weather\weather.dll :
	$(CC) $(CFLAGS) $(SAVANSRC)\samples\server\weather\*.c /Fo$(SAVANINTDIR)\samples\weather\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANINTDIR)\samples\weather\*.obj  $(SAVAN_CLIENT).lib /DLL \
	/OUT:$(SAVANDISTDIR)\services\weather\weather.dll
	-@$(_VC_MANIFEST_EMBED_DLL)
	@copy $(SAVANSRC)\samples\server\weather\services.xml $(SAVANDISTDIR)\services\weather\

weather: $(SAVANDISTDIR)\services\weather\weather.dll

$(SAVANDISTDIR)\services\listener\listener.dll :
	$(CC) $(CFLAGS) $(SAVANSRC)\samples\server\listener\*.c /Fo$(SAVANINTDIR)\samples\listener\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANINTDIR)\samples\listener\*.obj  $(SAVAN_CLIENT).lib /DLL \
	/OUT:$(SAVANDISTDIR)\services\listener\listener.dll
	-@$(_VC_MANIFEST_EMBED_DLL)
	@copy $(SAVANSRC)\samples\server\listener\services.xml $(SAVANDISTDIR)\services\listener\

listener: $(SAVANDISTDIR)\services\listener\listener.dll
		
$(SAVANDISTDIR)\samples\bin\savanc\subscriber.exe :
	$(CC) $(CFLAGS) $(SAVANSRC)\samples\client\subscriber\*.c /Fo$(SAVANINTDIR)\samples\subscriber\ /c
	$(LD) $(LDFLAGS) $(LIBS) $(SAVANINTDIR)\samples\subscriber\*.obj $(SAVAN_CLIENT).lib \
	/OUT:$(SAVANDISTDIR)\samples\bin\savanc\subscriber.exe
	-@$(_VC_MANIFEST_EMBED_EXE)

subscriber: $(SAVANDISTDIR)\samples\bin\savanc\subscriber.exe
	
dist: clean distdir intdir savan savan_client listener weather subscription subscriber copy_include copy_samples copy_extra


	 


	






	



