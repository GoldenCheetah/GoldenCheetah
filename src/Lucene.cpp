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
#include "Context.h"
#include "Athlete.h"

// stdc strings
using namespace std;
// here we go
using namespace lucene::analysis;
using namespace lucene::index;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
using namespace lucene::store;

Lucene::Lucene(QObject *parent, Context *context) : QObject(parent), context(context)
{
    // create the directory if needed
    context->athlete->home.mkdir("index");

    // make index directory if needed
    dir = QDir(context->athlete->home.canonicalPath() + "/index");

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

    } catch (CLuceneError &e) {

        //qDebug()<<"clucene error!"<<e.what();
    }
}

Lucene::~Lucene()
{
}

bool Lucene::importRide(SummaryMetrics *, RideFile *ride, QColor , unsigned long, bool)
{
    // create a document
    Document doc;

    // add Filename special field (unique)
    std::wstring cname = ride->getTag("Filename","").toStdWString();
    Field *fadd = new Field(_T("Filename"), cname.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED);
    doc.add( *fadd );

    QString alltexts;

    // And all the metadata texts individually
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (!context->specialFields.isMetric(field.name) && (field.type < 3 || field.type == 7)) {

            std::wstring name = context->specialFields.makeTechName(field.name).toStdWString();
            std::wstring value = ride->getTag(field.name,"").toStdWString();

            alltexts += ride->getTag(field.name,"") + " ";
            Field *add = new Field(name.c_str(), value.c_str(), Field::STORE_YES | Field::INDEX_TOKENIZED);
            doc.add( *add );
        }
    }

    // add a catchall text which is concat of all text fields
    std::wstring value = alltexts.toStdWString();
    Field *cadd = new Field(_T("contents"), value.c_str(), Field::STORE_YES | Field::INDEX_TOKENIZED);
    doc.add( *cadd );
    
    // delete if already in the index
    deleteRide(ride->getTag("Filename", ""));

    // now add to index
    try { 

        // now lets open using a mnodifier since the API is much simpler
        IndexWriter *writer = new IndexWriter(dir.canonicalPath().toLocal8Bit().data(), &analyzer, false); // for updates
        writer->addDocument(&doc); 
        writer->close();
        delete writer;

    } catch (CLuceneError &e) {
        qDebug()<<"add document clucene error!"<<e.what();
    }

    doc.clear();

    return true;
}

bool Lucene::deleteRide(QString name)
{
    std::wstring cname = name.toStdWString();

    try {

        IndexReader *reader = IndexReader::open(dir.canonicalPath().toLocal8Bit().data());
        Term del = Term(_T("Filename"), cname.c_str());
        reader->deleteDocuments(&del);
        reader->close();
        delete reader;

    } catch (CLuceneError &e) {
        qDebug()<<"deleteDocuments clucene error!"<<e.what();
    }
    return true;
}

void Lucene::optimise()
{
    try {

        IndexWriter *writer = new IndexWriter(dir.canonicalPath().toLocal8Bit().data(), &analyzer, false); // for updates
        writer->optimize();
        writer->close();
        delete writer;

    } catch(CLuceneError &e) {
        qDebug()<<"optimise clucene error!"<<e.what();
    }
}

int Lucene::search(QString query)
{

    try {
        // parse query
        QueryParser parser(_T("contents"), &analyzer);
        parser.setPhraseSlop(4);

        std::wstring querystring = query.toStdWString();
        Query* lquery = parser.parse(querystring.c_str());

        if (lquery == NULL) return 0;

        IndexReader *reader = IndexReader::open(dir.canonicalPath().toLocal8Bit().data());
        IndexSearcher *searcher = new IndexSearcher(reader);           // to perform searches

        // go find hits
        hits = searcher->search(lquery);
        filenames.clear();

        for (size_t i=0; i< hits->length(); i++) {
            Document *d = &hits->doc(i);
            filenames << QString::fromWCharArray(d->get(_T("Filename")));
        }

        searcher->close();
        reader->close();

        delete hits;
        delete lquery;
        delete searcher;
        delete reader;

    } catch (CLuceneError &e) {

        //qDebug()<<"clucene error:"<<e.what();
        return 0;
    }

    emit results(filenames);

    return filenames.count();
}
