# 
# Generate documentation files from the source .texinfo
#

all: FAQ RELEASE MANUAL
	rm -f *.aux *.cp *.cps *.fn *.ky *.log *.pg *.toc *.tp *.vr

clean:
	rm -f GC3-Manual.pdf GC3-FAQ.pdf GC3-FAQ.html \
    GC3-FAQ.info GC3-Release.pdf GC3-Release.html GC3-Release.info 
	rm -f *.aux *.cp *.cps *.fn *.ky *.log *.pg *.toc *.tp *.vr

MANUAL: GC3-Manual.pdf 
FAQ: GC3-FAQ.pdf GC3-FAQ.info
RELEASE: GC3-Release.pdf GC3-Release.info GC31-Release.pdf GC31-Release.info

## The basic commands, good on Linux/Mac
## need mingw on Windows
TEXI2PDF    = texi2pdf
TEXI2HTML   = texi2html
MAKEINFO    = makeinfo

.SUFFIXES: .pdf .texinfo .info .html

.texinfo.info:
	${MAKEINFO} $< -o $@ >/dev/null 2>&1

.texinfo.pdf:
	${TEXI2PDF} -I ../contrib $< -o $@  >/dev/null 2>&1

.texinfo.html:
	${TEXI2HTML} $< -o $@ >/dev/null 2>&1
