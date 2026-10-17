#ifndef GC_NetworkReply_h
#define GC_NetworkReply_h

// Don't remove the #include <optional>, although this is not directly needed in this file.
// It prevents compile issues with QNetworkReply, because
// it needs qhttpheaders, which uses optional, but doesn't #include <optional>.
// It has to be included before QNetworkReply
#include <optional>

#include <QObject>
#include <QByteArray>
#include <QNetworkRequest>
#include <QNetworkReply>

// This is used to return in readData the data it has received in the constructor.
// It simulates a network response, to be able to do unit testing without
// the actual API.
class MockReply : public QNetworkReply
{
    Q_OBJECT

    public:
        MockReply(const QByteArray &body, const QVariant &httpStatusCode, const QNetworkRequest &request, QObject *parent = nullptr)
            : QNetworkReply(parent), body_(body)
        {
            setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatusCode);
            setRequest(request);
            setError(QNetworkReply::NoError, QString());

            setFinished(true);
            open(ReadOnly | Unbuffered);
        }

        qint64 readData(char *data, qint64 maxSize) override
        {
            if (offset_ >= body_.size()) return -1; // EOF

            qint64 count = qMin(maxSize, (qint64)(body_.size() - offset_));
            memcpy(data, body_.constData() + offset_, count);
            offset_ += count;
            return count;
        }

        void abort() override { }

        qint64 bytesAvailable() const override
        {
            return body_.size() - offset_ + QNetworkReply::bytesAvailable();
        }

    private:
        QByteArray body_;
        qint64 offset_ = 0;
};

#endif // GC_NetworkReply_h