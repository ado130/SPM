#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QtCore>
#include <QtNetwork>

QT_BEGIN_NAMESPACE
class QSslError;
QT_END_NAMESPACE


class DownloadManager : public QObject
{
    Q_OBJECT
    QNetworkAccessManager manager;
    QVector<QNetworkReply *> currentDownloads;

public:
    explicit DownloadManager(QObject *parent = nullptr);

    void execute(QString urlPath);

private:
    void doDownload(const QUrl &url);
    static QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);
    static QString isHttpRedirect(QNetworkReply *reply);

signals:
    void sendData(QByteArray data, QString statusCode);

public Q_SLOTS:
    void downloadFinished(QNetworkReply *reply);
    void sslErrors(const QList<QSslError> &errors);

};

#endif // DOWNLOADMANAGER_H
