#include "downloadmanager.h"


DownloadManager::DownloadManager(QObject *parent) : QObject(parent)
{
    connect(&manager, SIGNAL(finished(QNetworkReply*)),
                SLOT(downloadFinished(QNetworkReply*)));
}

void DownloadManager::doDownload(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

#if QT_CONFIG(ssl)
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
            SLOT(sslErrors(QList<QSslError>)));
#endif

    currentDownloads.push_back(reply);
}

QString DownloadManager::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
    {
        basename = "download";
    }

    if (QFile::exists(basename))
    {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';

        while (QFile::exists(basename + QString::number(i)))
        {
            ++i;
        }

        basename += QString::number(i);
    }

    return basename;
}

bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << QString("Could not open %1 for writing: %2\n").arg(filename)
                                                                  .arg(file.errorString());
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

QString DownloadManager::isHttpRedirect(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString status = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    QString ret = QString("Status code: %1 %2").arg(statusCode).arg(status);

    qDebug() << "Status" << ret;

    return ret;

    /*return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;*/
}

void DownloadManager::execute(QString urlPath)
{
    QUrl url = QUrl::fromEncoded(urlPath.toUtf8(), QUrl::TolerantMode);
    doDownload(url);
}

void DownloadManager::sslErrors(const QList<QSslError> &sslErrors)
{
#if QT_CONFIG(ssl)
    for (const QSslError &error : sslErrors)
    {
        qDebug() << QString("SSL error: %1\n").arg(error.errorString());
    }
#else
    Q_UNUSED(sslErrors);
#endif
}

void DownloadManager::downloadFinished(QNetworkReply *reply)
{
    QUrl url = reply->url();

    if (reply->error())
    {
        qDebug() << QString("Download of %1 failed: %2 \n").arg(url.toEncoded().constData())
                                                          .arg(reply->errorString());

        emit sendData(reply->readAll(), reply->errorString());
    }
    else
    {
        QString statusCode = isHttpRedirect(reply);

        if (!statusCode.contains("200")) /*== 301 || statusCode == 302 || statusCode == 303 || statusCode == 305 || statusCode == 307 || statusCode == 308*/
        {
            qDebug() << "Request was redirected. \n";

            emit sendData(reply->readAll(), statusCode);
        }
        else
        {
            /*QString filename = saveFileName(url);

            if (saveToDisk(filename, reply))
            {
                qDebug() << QString("Download of %1 succeeded (saved to %2) \n").arg(url.toEncoded().constData())
                                                                               .arg(filename);
            }*/

            emit sendData(reply->readAll(), statusCode);
        }
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();

    if (currentDownloads.isEmpty())
    {
        // all downloads finished
        //QCoreApplication::instance()->quit();
    }
}
