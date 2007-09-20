datadir=$(prefix)
SUBDIRS = src
docsdir=$(prefix)/docs
samplesdir=$(prefix)/samples/savanc
includedir=$(prefix)/include/savan-0.90
sysconfdir=$(prefix)
include_HEADERS=$(top_builddir)/include/*.h

sysconf_DATA=src/data/axis2.xml
install-data-hook:
	cp -rf xdocs/docs/* $(docsdir)

dist-hook:
	sh dist_hook.sh
EXTRA_DIST = build.sh autogen.sh build xdocs samples


