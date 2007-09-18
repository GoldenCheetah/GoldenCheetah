
CONTENT=$(wildcard *.content)
HTML=$(subst .content,.html,$(CONTENT))
TARBALLS=$(wildcard gc_*.tgz)
OTHER=logo.jpg sample.gp sample.png cpint.gp cpint.png \
	screenshot-summary.png screenshot-plot.png \
	screenshot-cpint.png screenshot-phist.png \
	screenshot-download.png screenshot-weekly.png
DISTRIB=GoldenCheetah_2006-12-25_Darwin_powerpc.dmg \
	GoldenCheetah_2006-09-06_Darwin_powerpc.dmg \
	GoldenCheetah_2006-09-07_Darwin_powerpc.dmg \
	GoldenCheetah_2006-09-19_Darwin_powerpc.dmg \
	GoldenCheetah_2007-01-06_Linux_x86.tgz \
	GoldenCheetah_2007-01-30_Darwin_powerpc.dmg \
	GoldenCheetah_2007-01-30_Linux_x86.tgz \
	GoldenCheetah_2007-02-12_Darwin_powerpc.dmg \
	GoldenCheetah_2007-02-12_Linux_x86.tgz \
	GoldenCheetah_2007-02-22_Darwin_powerpc.dmg \
	GoldenCheetah_2007-02-22_Linux_x86.tgz \
	GoldenCheetah_2007-04-01_Darwin_powerpc.dmg \
	GoldenCheetah_2007-04-01_Linux_x86.tgz \
	GoldenCheetah_2007-04-26_Darwin_powerpc.dmg \
	GoldenCheetah_2007-04-26_Linux_x86.tgz \
	GoldenCheetah_2007-08-07_Darwin_powerpc.dmg \
	GoldenCheetah_2007-08-07_Linux_x86.tgz \
	GoldenCheetah_2007-09-18_Darwin_powerpc.dmg \
	GoldenCheetah_2007-09-18_Linux_x86.tgz

all: $(HTML)
.PHONY: all clean install

clean:
	rm -f $(HTML)

install:
	rsync -avz -e ssh $(HTML) $(TARBALLS) $(OTHER) $(DISTRIB) \
		srhea.net:wwwroot/goldencheetah.org/

contact.html: contact.content genpage.pl
	./genpage.pl "Contact Us" $< > $@

contrib.html: contrib.content genpage.pl
	./genpage.pl "Contributors" $< > $@

download.html: download.content genpage.pl
	./genpage.pl "Download" $< > $@

faq.html: faq.content genpage.pl
	./genpage.pl "Frequently Asked Questions" $< > $@

index.html: index.content genpage.pl
	./genpage.pl "Introduction" $< > $@

license.html: license.content genpage.pl
	./genpage.pl "License" $< > $@

screenshots.html: screenshots.content genpage.pl
	./genpage.pl "Screenshots" $< > $@

search.html: search.content genpage.pl
	./genpage.pl "Search" $< > $@

users-guide.html: users-guide.content genpage.pl
	./genpage.pl "User's Guide" $< > $@

zones.html: zones.content genpage.pl
	./genpage.pl "Power Zones File Guide" $< > $@

