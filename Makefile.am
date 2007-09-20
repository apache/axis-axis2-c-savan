datadir=$(prefix)
SUBDIRS = src
docsdir=$(prefix)/docs
samplesdir=$(prefix)/samples/savanc
includedir=$(prefix)/include/savan-0.90
include_HEADERS=$(top_builddir)/include/*.h

install-data-hook:
	cp -rf docs/* $(docsdir)
	cp -rf samples/* $(samplesdir)
	rm -rf `find $(samplesdir)/ -type d -name *.svn`
	rm -rf `find $(samplesdir)/ -type d -name *.libs`
	rm -rf `find $(samplesdir)/ -type d -name *.deps`
	rm -rf `find $(samplesdir)/ -type f -name Makefile`
	rm -rf `find $(samplesdir)/ -type d -name autom4te.cache`

dist-hook:
	sh dist_hook.sh
EXTRA_DIST = build.sh autogen.sh build xdocs samples


