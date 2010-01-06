
CONTENT=$(wildcard *.content)
HTML=$(subst .content,.html,$(CONTENT))
TARBALLS=$(wildcard gc_*.tgz)
OTHER=logo.jpg sample.gp sample.png cpint.gp cpint.png \
	critical-power-plot.png histogram-analysis.png pf-pv-plot.png \
	ride-plot.png ride-summary.png weekly-summary.png \
	choose-a-cyclist.png main-window.png critical-power.png \
	power.zones


all: $(HTML)
.PHONY: all clean install

clean:
	rm -f $(HTML)

install:
	rsync -avz -e ssh $(HTML) $(TARBALLS) $(OTHER) \
		srhea.net:wwwroot/goldencheetah.org/

bug-tracker.html: bug-tracker.content genpage.pl
	./genpage.pl "Bug Tracker" $< > $@

command-line.html: command-line.content genpage.pl
	./genpage.pl "Legacy Command-Line Tools" $< > $@

contact.html: contact.content genpage.pl
	./genpage.pl "Contact Us" $< > $@

contrib.html: contrib.content genpage.pl
	./genpage.pl "Contributors" $< > $@

developers-guide.html: developers-guide.content genpage.pl
	./genpage.pl "Developer's Guide" $< > $@

download.html: download.content genpage.pl
	./genpage.pl "Download" $< > $@

faq.html: faq.content genpage.pl
	./genpage.pl "Frequently Asked Questions" $< > $@

index.html: index.content genpage.pl
	./genpage.pl "Introduction" $< > $@

license.html: license.content genpage.pl
	./genpage.pl "License" $< > $@

mailing-list.html: mailing-list.content genpage.pl
	./genpage.pl "Mailing Lists" $< > $@

screenshots.html: screenshots.content genpage.pl
	./genpage.pl "Screenshots" $< > $@

search.html: search.content genpage.pl
	./genpage.pl "Search" $< > $@

users-guide.html: users-guide.content genpage.pl
	./genpage.pl "User's Guide" $< > $@

wishlist.html: wishlist.content genpage.pl
	./genpage.pl "Wish List" $< > $@

zones.html: zones.content genpage.pl
	./genpage.pl "Power Zones File Guide" $< > $@

