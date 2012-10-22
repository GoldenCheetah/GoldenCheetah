/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Lucene.h"
#include "MainWindow.h"

// stdc strings
using namespace std;
// here we go
using namespace lucene::analysis;
using namespace lucene::index;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
using namespace lucene::store;

Lucene::Lucene(MainWindow *parent) : QObject(parent), main(parent)
{
    // create the directory if needed
    main->home.mkdir("index");

    // make index directory if needed
    QDir dir(main->home.canonicalPath() + "/index");

    try {

        bool indexExists = IndexReader::indexExists(dir.canonicalPath().toLocal8Bit().data());

        // clear any locks
        if (indexExists && IndexReader::isLocked(dir.canonicalPath().toLocal8Bit().data()))
            IndexReader::unlock(dir.canonicalPath().toLocal8Bit().data());

        if (!indexExists) {

            IndexWriter *create = new IndexWriter(dir.canonicalPath().toLocal8Bit().data(), &analyzer, true);

            // lets flush to disk and reopen
            create->close();
            delete create;
        }

        // now lets open using a mnodifier since the API is much simpler
        writer = new IndexWriter(dir.canonicalPath().toLocal8Bit().data(), &analyzer, false); // for updates

    } catch (CLuceneError &e) {

        qDebug()<<"clucene error!"<<e.what();
    }
}

Lucene::~Lucene()
{
    try {

        writer->flush();
        writer->close();
        //XXXdelete writer; Causes a SEGV !?
    } catch(CLuceneError &e) {}
}

bool Lucene::importRide(SummaryMetrics *, RideFile *ride, QColor , unsigned long, bool)
{
    // create a document
    Document doc;

    // add Filename special field (unique)
    std::wstring cname = ride->getTag("Filename","").toStdWString();
    doc.add( *_CLNEW Field(_T("Filename"), cname.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED));

    // And all the metadata texts
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {

        if (!main->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {

            std::wstring name = main->specialFields.makeTechName(field.name).toStdWString();
            std::wstring value = ride->getTag(field.name,"").toStdWString();

            doc.add( *_CLNEW Field(name.c_str(), value.c_str(), Field::STORE_YES | Field::INDEX_TOKENIZED));
        }
    }

    
    // delete if already in the index
    deleteRide(ride->getTag("Filename", ""));

    // now add to index
    try { writer->addDocument(&doc); } catch (CLuceneError &e) {}

    doc.clear();

    return true;
}

bool Lucene::deleteRide(QString name)
{
    std::wstring cname = name.toStdWString();

    try { writer->deleteDocuments(new Term(_T("Filename"), cname.c_str())); } catch (CLuceneError &e) {}
    return true;
}

void Lucene::optimise()
{
    try {
        writer->flush();
        writer->optimize();
    } catch(CLuceneError &e) {}
}

int Lucene::search(QString query)
{

    try {
        // parse query
        QueryParser parser(_T("Notes"), &analyzer);
        parser.setPhraseSlop(4);

        std::wstring querystring = query.toStdWString();
        Query* lquery = parser.parse(querystring.c_str());

        if (lquery == NULL) return 0;

        reader = IndexReader::open(writer->getDirectory());                // for querying against
        searcher = new IndexSearcher(reader);           // to perform searches

        // go find hits
        hits = searcher->search(lquery);
        filenames.clear();

        for (unsigned int i=0; i< hits->length(); i++) {
            Document *d = &hits->doc(i);
            filenames << QString::fromWCharArray(d->get(_T("Filename")));
        }

        delete hits;
        delete lquery;
        delete searcher;
        delete reader;

    } catch (CLuceneError &e) {

        qDebug()<<"clucene error:"<<e.what();
        return 0;
    }

    return filenames.count();
}
