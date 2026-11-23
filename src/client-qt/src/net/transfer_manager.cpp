#include "net/transfer_manager.h"
#include "net/session.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QRegularExpression>
#include <QSaveFile>

namespace {
QString digest(const QString& path) {
    QFile f(path); if(!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha256);
    return h.addData(&f)?QString::fromLatin1(h.result().toHex()):QString{};
}
QString safeName(QString name) {
    name=QFileInfo(name.replace('\\','/')).fileName();
    name.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");
    name=name.left(120); return name.isEmpty()?"attachment":name;
}
}
TransferManager::TransferManager(Session* session,QObject* parent):QObject(parent),session_(session) {
    connect(session_,&Session::attachmentResponse,this,&TransferManager::response);
    connect(session_,&Session::stateChanged,this,[this](const QString&){
        if(session_->selfId()>0) return;
        for(auto& task:tasks_) {task.active=false; if(task.state!="已完成") task.state="已暂停，可继续";}
        if(owner_>0) persist(); emit changed();
    });
    connect(session_,&Session::ready,this,[this](int uid,const QString&){
        if(owner_!=uid||endpoint_!=session_->endpoint()) { tasks_.clear();owner_=uid;endpoint_=session_->endpoint();restore(); }
        emit changed();
    });
}
QString TransferManager::scope() const { return QString::fromLatin1(QCryptographicHash::hash((endpoint_+":"+QString::number(owner_)).toUtf8(),QCryptographicHash::Sha256).toHex()); }
QString TransferManager::cacheRoot() const { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/attachments/"+scope(); }
QString TransferManager::cachedPath(const QJsonObject& a) const {
    const auto id=a.value("attachment_id").toString();
    if(!QRegularExpression("^[A-Za-z0-9-]{1,80}$").match(id).hasMatch()) return {};
    QString path=cacheRoot()+"/"+id+"-"+safeName(a.value("file_name").toString());
    return QFileInfo::exists(path)?path:QString{};
}
QString TransferManager::upload(const QString& path,int to,int gid) {
    if(session_->selfId()<=0) {emit error("请先登录");return {};}
    int running=0;for(const auto& t:tasks_) if(t.active) ++running;
    if(running>=4){emit error("最多同时进行 4 项传输");return {};}
    QFileInfo info(path);qint64 maximum=QSettings().value("network/attachment_max_mb",100).toLongLong()*1024*1024;
    if(!info.isFile()||info.size()>maximum){emit error(QString("请选择不超过 %1 MB 的文件").arg(maximum/1024/1024));return {};}
    TransferTask t;t.id=QUuid::createUuid().toString(QUuid::WithoutBraces);t.uploadId=t.id;t.path=info.absoluteFilePath();
    t.toId=to;t.groupId=gid;t.uid=session_->selfId();t.size=info.size();t.hash=digest(t.path);t.upload=true;t.active=true;t.state="准备上传";
    if(t.hash.isEmpty()){emit error("无法读取文件");return {};}
    QString ext=info.suffix().toLower();t.kind=ext=="gif"?"sticker":QStringList{"png","jpg","jpeg","webp","bmp"}.contains(ext)?"image":"file";
    tasks_.insert(t.id,t);persist();emit changed();request(t.id,"begin");return t.id;
}
QString TransferManager::download(const QJsonObject& a) {
    QString cached=cachedPath(a);if(!cached.isEmpty()){emit downloaded(a.value("attachment_id").toString(),exportDownload(cached));return {};}
    const auto aid=a.value("attachment_id").toString();
    if(!QRegularExpression("^[A-Za-z0-9-]{1,80}$").match(aid).hasMatch()) {emit error("附件标识无效");return {};}
    for(const auto& t:tasks_) if(!t.upload&&t.attachmentId==aid){resume(t.id);return t.id;}
    int running=0;for(const auto& t:tasks_) if(t.active)++running;
    if(running>=4){emit error("最多同时进行 4 项传输");return {};}
    TransferTask t;t.id=QUuid::createUuid().toString(QUuid::WithoutBraces);t.uid=session_->selfId();t.attachment=a;
    t.attachmentId=aid;t.size=a.value("file_size").toVariant().toLongLong();t.hash=a.value("sha256").toString();
    if(t.uid<=0||t.size<0||t.size>QSettings().value("network/attachment_max_mb",100).toLongLong()*1024*1024||!QRegularExpression("^[a-f0-9]{64}$").match(t.hash).hasMatch()){emit error("附件大小或校验信息无效");return {};}
    QDir().mkpath(cacheRoot());t.path=cacheRoot()+"/"+aid+"-"+safeName(a.value("file_name").toString());
    t.offset=QFileInfo(t.path+".part").size();if(t.offset<0)t.offset=0;t.active=true;t.state="下载中";
    tasks_.insert(t.id,t);persist();emit changed();request(t.id,"download");return t.id;
}
void TransferManager::request(const QString& id,const QString& op) {
    if(!tasks_.contains(id))return;auto& t=tasks_[id];if(!t.active)return;
    t.pendingRequest=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject obj{{"type",70},{"op",op},{"request_id",t.pendingRequest},{"attachment_id",t.attachmentId},{"offset",t.offset}};
    if(op=="begin") {obj["upload_id"]=t.uploadId;obj["toId"]=t.toId;obj["groupId"]=t.groupId;obj["file_name"]=QFileInfo(t.path).fileName();
        obj["file_size"]=t.size;obj["sha256"]=t.hash;obj["content_type"]=t.kind;}
    if(op=="upload") {
        QFile f(t.path); if(!f.open(QIODevice::ReadOnly)||!f.seek(t.offset)){pause(id);emit error("原文件无法读取");return;}
        auto bytes=f.read(48*1024); if(bytes.isEmpty()){pause(id);emit error("原文件已改变，请重新发送");return;}
        obj["data"]=QString::fromLatin1(bytes.toBase64());
    }
    session_->sendAttachment(obj);
}
void TransferManager::response(const QJsonObject& o) {
    QString id;
    for(const auto& task:tasks_)if(task.pendingRequest==o.value("request_id").toString()){id=task.id;break;}
    if(id.isEmpty())return;
    auto& t=tasks_[id];if(!t.active)return;
    if(o.value("status").toString()!="ok") {t.active=false;t.state="失败："+o.value("msg").toString();persist();emit changed();return;}
    auto op=o.value("op").toString();
    if(op=="begin"||op=="upload") {
        t.attachmentId=o.value("attachment_id").toString();t.offset=o.value("next_offset").toVariant().toLongLong();
        if(t.offset<0||t.offset>t.size){pause(id);emit error("服务端返回的偏移无效");return;}
        t.state="上传中";emit changed();request(id,t.offset==t.size?"finish":"upload");
    } else if(op=="finish") {
        t.active=false;t.state="已完成";
        if(t.path.startsWith(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/clipboard/"))QFile::remove(t.path);
        persist();emit changed();
    }
    else if(op=="download") {
        auto data=QByteArray::fromBase64(o.value("data").toString().toLatin1());
        if(o.value("offset").toVariant().toLongLong()!=t.offset||data.size()>48*1024||t.offset+data.size()>t.size|| (data.isEmpty()&&t.offset<t.size)) {pause(id);emit error("下载分块数据无效");return;}
        QFile f(t.path+".part");if(!f.open(QIODevice::WriteOnly|QIODevice::Append)||f.write(data)!=data.size()){pause(id);emit error("无法保存下载文件");return;}f.close();
        t.offset+=data.size();t.state="下载中";emit changed();
        if(t.offset==t.size) {
            if(digest(t.path+".part")!=t.hash){QFile::remove(t.path+".part");t.offset=0;t.active=false;t.state="校验失败，请重试";}
            else if(!QFile::rename(t.path+".part",t.path)){t.active=false;t.state="无法保存完整附件";}
            else {t.active=false;t.state="已完成";emit downloaded(t.attachmentId,exportDownload(t.path));pruneCache();}
            persist();emit changed();
        } else request(id,"download");
    }
}
void TransferManager::pause(const QString& id) {if(!tasks_.contains(id))return;tasks_[id].active=false;tasks_[id].state="已暂停，可继续";persist();emit changed();}
void TransferManager::resume(const QString& id) {
    if(!tasks_.contains(id)||session_->selfId()!=tasks_[id].uid||tasks_[id].state=="已完成"||tasks_[id].active)return;
    int running=0;for(const auto& t:tasks_)if(t.active)++running;
    if(running>=4){emit error("最多同时进行 4 项传输");return;}
    auto& t=tasks_[id];t.active=true;
    if(t.upload && (QFileInfo(t.path).size()!=t.size||digest(t.path)!=t.hash)){t.active=false;t.state="原文件已改变，请重新发送";emit changed();return;}
    if(!t.upload){t.offset=QFileInfo(t.path+".part").exists()?QFileInfo(t.path+".part").size():0;}
    request(id,t.upload?"begin":"download");emit changed();
}
void TransferManager::cancel(const QString& id) {
    if(!tasks_.contains(id))return;auto t=tasks_.take(id);
    if(t.upload&&!t.attachmentId.isEmpty()&&t.state!="已完成") session_->sendAttachment({{"type",70},{"op","cancel"},{"request_id",id},{"attachment_id",t.attachmentId}});
    if(!t.upload)QFile::remove(t.path+".part");persist();emit changed();
}
void TransferManager::persist() {
    if(owner_<=0)return;QJsonArray array;
    for(const auto& t:tasks_) if(t.state!="已完成") array.append(QJsonObject{{"id",t.id},{"path",t.path},{"hash",t.hash},{"upload_id",t.uploadId},{"attachment_id",t.attachmentId},
        {"kind",t.kind},{"toId",t.toId},{"groupId",t.groupId},{"uid",t.uid},{"size",t.size},{"upload",t.upload},{"attachment",t.attachment}});
    QSettings().setValue("transfers/"+scope(),QJsonDocument(array).toJson(QJsonDocument::Compact));
}
void TransferManager::restore() {
    auto array=QJsonDocument::fromJson(QSettings().value("transfers/"+scope()).toByteArray()).array();
    for(auto value:array){auto a=value.toObject();TransferTask t;t.id=a["id"].toString();t.path=a["path"].toString();t.hash=a["hash"].toString();t.uploadId=a["upload_id"].toString();
        t.attachmentId=a["attachment_id"].toString();t.kind=a["kind"].toString();t.toId=a["toId"].toInt();t.groupId=a["groupId"].toInt();t.uid=a["uid"].toInt();t.size=a["size"].toVariant().toLongLong();
        t.upload=a["upload"].toBool();t.attachment=a["attachment"].toObject();t.state="已暂停，可继续";if(tasks_.size()<32)tasks_.insert(t.id,t);}
}
void TransferManager::pruneCache() {
    auto files=QDir(cacheRoot()).entryInfoList(QDir::Files,QDir::Time|QDir::Reversed);qint64 total=0;
    for(auto f:files) if(!f.fileName().endsWith(".part"))total+=f.size();
    const auto limit=qBound(64,QSettings().value("preferences/cache_mb",512).toInt(),4096)*1024LL*1024;
    for(auto f:files) {if(total<=limit)break;if(f.fileName().endsWith(".part"))continue;
        bool used=false;for(const auto& t:tasks_)if(t.path==f.absoluteFilePath() && t.active)used=true;
        if(!used&&!f.isSymLink()&&QFile::moveToTrash(f.absoluteFilePath()))total-=f.size();}
}
QString TransferManager::downloadDirectory(){return QSettings().value("preferences/download_directory",QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)+"/LanChat").toString();}
QString TransferManager::exportDownload(const QString& path){
    const auto directory=downloadDirectory();if(!QDir().mkpath(directory)){emit error("下载已完成，但无法写入默认下载目录，文件保留在缓存中。");return path;}
    QString target=QDir(directory).absoluteFilePath(QFileInfo(path).fileName());
    if(QFileInfo::exists(target)){if(digest(target)==digest(path))return target;target=QDir(directory).absoluteFilePath(QUuid::createUuid().toString(QUuid::WithoutBraces)+"-"+QFileInfo(path).fileName());}
    QFile source(path);QSaveFile output(target);bool ok=source.open(QIODevice::ReadOnly)&&output.open(QIODevice::WriteOnly);
    while(ok&&!source.atEnd()){const auto bytes=source.read(64*1024);ok=!bytes.isEmpty()&&output.write(bytes)==bytes.size();}
    if(ok&&output.commit())return target;
    emit error("下载已完成，但复制到下载目录失败，文件保留在缓存中。");return path;
}
bool TransferManager::autoDownload(const QJsonObject& a){
    QSettings settings;const auto size=a.value("file_size").toVariant().toLongLong();const auto kind=a.value("content_type").toString();
    if(!settings.value("preferences/auto_download_images",false).toBool()||size<0||size>settings.value("preferences/auto_download_mb",10).toLongLong()*1024*1024||(!QStringList{"image","sticker"}.contains(kind)&&!QStringList{"png","jpg","jpeg","gif","webp","bmp"}.contains(QFileInfo(a.value("file_name").toString()).suffix().toLower())))return false;
    download(a);return true;
}
qint64 TransferManager::cacheBytes()const{qint64 size=0;for(const auto& file:QDir(cacheRoot()).entryInfoList(QDir::Files))if(!file.isSymLink())size+=file.size();return size;}
int TransferManager::clearCache(){int count=0;for(const auto& file:QDir(cacheRoot()).entryInfoList(QDir::Files)){if(file.isSymLink()||file.fileName().endsWith(".part"))continue;bool active=false;for(const auto& t:tasks_)if(t.active&&t.path==file.absoluteFilePath())active=true;if(!active&&QFile::moveToTrash(file.absoluteFilePath()))++count;}emit changed();return count;}
