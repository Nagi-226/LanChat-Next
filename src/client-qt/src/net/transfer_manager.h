#pragma once
#include <QObject>
#include <QHash>
#include <QJsonObject>
class Session;
struct TransferTask {
    QString id, path, hash, uploadId, attachmentId, kind, state, pendingRequest;
    int toId=0, groupId=0, uid=0;
    qint64 size=0, offset=0;
    bool upload=false, active=false;
    QJsonObject attachment;
};
class TransferManager : public QObject {
    Q_OBJECT
public:
    explicit TransferManager(Session* session,QObject* parent=nullptr);
    QString upload(const QString& path,int toId,int groupId);
    QString download(const QJsonObject& attachment);
    void pause(const QString& id);
    void resume(const QString& id);
    void cancel(const QString& id);
    QString cachedPath(const QJsonObject& attachment) const;
    QList<TransferTask> tasks() const { return tasks_.values(); }
    bool autoDownload(const QJsonObject& attachment);
    qint64 cacheBytes() const;
    int clearCache();
    static QString downloadDirectory();
signals:
    void changed();
    void error(const QString& message);
    void downloaded(const QString& attachmentId,const QString& path);
private:
    void response(const QJsonObject& object);
    void request(const QString& id,const QString& op);
    void persist();
    void restore();
    void pruneCache();
    QString cacheRoot() const;
    QString scope() const;
    QString exportDownload(const QString& path);
    Session* session_;
    QHash<QString,TransferTask> tasks_;
    int owner_=0;
    QString endpoint_;
};
