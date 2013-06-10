
CONTENT=$(wildcard *.content)
HTML=$(subst .content,.html,$(CONTENT))
TARBALLS=$(wildcard gc_*.tgz)
OTHER= 3d.png choose-a-cyclist.png cpint.gp cpint.png critical-power-plot.png critical-power.png \
       cyclist-info.png editor.png gui-preview.png histogram-analysis.png logo.jpg logo.png \
       main-window.png map.png metrics-power.png metrics-timedist.png metrics-tiz.png pf-pv-plot.png \
       pm.png power.zones realtime.png ride-plot.png ride-plot2.png ride-summary.png sample.gp \
       sample.png weekly-summary.png google-earth.png aerolab.png

BIN=    GoldenCheetah_3.0.0_Mac_32bit.dmg \
        GoldenCheetah_3.0.0_Mac_64bit.dmg \
        GoldenCheetah_3.0.0_Windows_Installer.exe

all: $(HTML)
.PHONY: all clean install

clean:
	rm -f $(HTML)

install:
	rsync -avz -e ssh $(HTML) $(TARBALLS) $(OTHER) \
		goldencheetah.org:/home/srhea/wwwroot/goldencheetah.org/

install-bin:
	rsync -avz -e ssh $(BIN) \
		goldencheetah.org:/home/srhea/wwwroot/goldencheetah.org/

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

older-releases.html: older-releases.content genpage.pl
	./genpage.pl "Older Releases" $< > $@

download.html: download.content genpage.pl
	./genpage.pl "Download" $< > $@

release-notes.html: release-notes.content genpage.pl
	./genpage.pl "Release Notes" $< > $@

release-notes_2.1.0.html: release-notes_2.1.0.content genpage.pl
	./genpage.pl "Release Notes 2.1.0" $< > $@

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

wiki.html: wiki.content genpage.pl
	./genpage.pl "Wiki" $< > $@

wishlist.html: wishlist.content genpage.pl
	./genpage.pl "Wish List" $< > $@

zones.html: zones.content genpage.pl
	./genpage.pl "Power Zones File Guide" $< > $@

