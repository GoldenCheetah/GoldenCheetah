
CONTENT=$(wildcard *.content)
HTML=$(subst .content,.html,$(CONTENT))
TARBALLS=$(wildcard gc_*.tgz)
OTHER=logo.jpg sample.gp sample.png cpint.gp cpint.png \
	screenshot-summary.png screenshot-plot.png \
	screenshot-cpint.png screenshot-phist.png \
	screenshot-download.png screenshot-weekly.png

all: $(HTML)
.PHONY: all clean install

clean:
	rm -f $(HTML)

install:
	rsync -avz -e ssh $(HTML) $(TARBALLS) $(OTHER) \
		srhea.net:wwwroot/goldencheetah.org/

command-line.html: command-line.content genpage.pl
	./genpage.pl "Legacy Command-Line Tools" $< > $@

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

