# 
# Generate documentation files from the source .texinfo
#

all: FAQ
	rm -f *.aux *.cp *.cps *.fn *.ky *.log *.pg *.toc *.tp *.vr

FAQ: FAQ.pdf FAQ.html FAQ.info

## The basic commands, good on Linux/Mac
## need mingw on Windows
TEXI2PDF    = texi2pdf
TEXI2HTML   = texi2html
MAKEINFO    = makeinfo

.SUFFIXES: .pdf .texinfo .info .html

.texinfo.info:
	${MAKEINFO} $< -o $@

.texinfo.pdf:
	${TEXI2PDF} $< -o $@

.texinfo.html:
	${TEXI2HTML} $< -o $@
