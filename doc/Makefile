
CONTENT=$(wildcard *.content)
HTML=$(subst .content,.html,$(CONTENT))
TARBALLS=$(wildcard gc_*.tgz)
OTHER=logo.png sample.gp sample.png cpint.gp cpint.png gui-preview.png

all: $(HTML)
.PHONY: all clean install

clean:
	rm -f $(HTML)

install:
	rsync -avz -e ssh $(HTML) $(TARBALLS) $(OTHER) \
		srhea.net:public_html/goldencheetah/

#	ftp -u ftp://srhea@ftp.goldencheetah.org/ $(HTML) $(TARBALLS) $(OTHER)

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

search.html: search.content genpage.pl
	./genpage.pl "Search" $< > $@

users-guide.html: users-guide.content genpage.pl
	./genpage.pl "User's Guide" $< > $@

