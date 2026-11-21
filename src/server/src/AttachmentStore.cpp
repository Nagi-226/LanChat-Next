#include "lanchat/server/AttachmentStore.h"
#include "sha256_hex.hpp"
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <algorithm>

namespace lanchat::server {
using namespace protocol_json;
namespace {
constexpr size_t chunkBytes = 48 * 1024;
const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string encode(const std::string& data) {
    std::string out; unsigned value = 0; int bits = -6;
    for (unsigned char c : data) { value = (value << 8) | c; bits += 8;
        while (bits >= 0) { out += alphabet[(value >> bits) & 63]; bits -= 6; } }
    if (bits > -6) out += alphabet[((value << 8) >> (bits + 8)) & 63];
    while (out.size() % 4) out += '=';
    return out;
}
std::string decode(const std::string& data) {
    if (data.size() > chunkBytes * 4 / 3 || data.size() % 4) throw std::runtime_error("invalid chunk length");
    std::string out; unsigned value = 0; int bits = -8;
    bool padding = false;
    for (char c : data) {
        if (c == '=') { padding = true; continue; }
        auto pos = alphabet.find(c);
        if (padding || pos == std::string::npos) throw std::runtime_error("invalid base64");
        value = (value << 6) | unsigned(pos); bits += 6;
        if (bits >= 0) { out += char((value >> bits) & 255); bits -= 8; }
    }
    if (encode(out) != data) throw std::runtime_error("noncanonical base64");
    return out;
}
// Streaming SHA-256 shares the compression implementation covered by NIST tests.
std::string hashFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read attachment");
    std::array<std::uint32_t,8> h = {0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::uint8_t block[64]{}; std::uint64_t length = 0; std::streamsize n;
    while (f.read(reinterpret_cast<char*>(block),64) || f.gcount()) {
        n = f.gcount(); length += n;
        if (n == 64) { vendor::sha256::detail::compress(h,block); continue; }
        block[n] = 0x80; std::fill(block+n+1,block+64,0);
        if (n >= 56) { vendor::sha256::detail::compress(h,block); std::fill(block,block+64,0); }
        goto finish;
    }
    std::fill(block,block+64,0); block[0]=0x80;
finish:
    for(int i=0;i<8;++i) block[63-i]=std::uint8_t((length*8)>>(i*8));
    vendor::sha256::detail::compress(h,block);
    const char* hex="0123456789abcdef"; std::string out;
    for(auto word:h) for(int shift=28;shift>=0;shift-=4) out+=hex[(word>>shift)&15];
    return out;
}
std::int64_t setting(const char* name, std::int64_t fallback) {
    const char* s=std::getenv(name); if(!s) return fallback;
    try { auto n=std::stoll(s); return n>0 && n<=102400 ? n*1024*1024 : fallback; } catch(...) { return fallback; }
}
}
AttachmentStore::AttachmentStore(db::Database& db, db::ChannelRepository& channels)
    : db_(db), channels_(channels), root_(std::filesystem::u8path(db.dataDir())/"attachments") {
    db_.ensureTable("attachments"); std::filesystem::create_directories(root_);
    maxBytes_=setting("LANCHAT_ATTACHMENT_MAX_MB",maxBytes_);
    quotaBytes_=setting("LANCHAT_ATTACHMENT_QUOTA_MB",quotaBytes_);
    // Only unfinished transfers expire. Published files require explicit retention policy.
    auto now=std::int64_t(std::time(nullptr));
    db_.remove("attachments",[&](const db::Row& r){
        bool expired=!intField(r,"complete") && now-int64Field(r,"created")>7*86400;
        if(expired) { std::error_code ec; std::filesystem::remove(root_/stringField(r,"attachment_id"),ec); }
        return expired;
    });
}
bool AttachmentStore::canRead(int uid,const Object& row) {
    int gid=intField(row,"groupId");
    return gid>0 ? channels_.isMember(uid,gid) : uid==intField(row,"fromId") || uid==intField(row,"toId");
}
Object AttachmentStore::process(int uid,const Object& req) {
    Object reply{{"type",Value(71)},{"op",Value(stringField(req,"op"))},
        {"request_id",Value(stringField(req,"request_id"))},{"status",Value("ok")}};
    try {
        if(uid<=0) throw std::runtime_error("login required");
        const auto op=stringField(req,"op");
        std::string id=stringField(req,"attachment_id");
        if(op=="begin") {
            auto key=stringField(req,"upload_id");
            if(key.empty()||key.size()>64||!std::all_of(key.begin(),key.end(),[](unsigned char c){return std::isalnum(c)||c=='-';}))
                throw std::runtime_error("invalid upload id");
            id=std::to_string(uid)+"-"+key;
        }
        if(id.empty()||id.size()>80||!std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='-';}))
            throw std::runtime_error("invalid attachment id");
        reply["attachment_id"]=Value(id);
        auto rows=db_.query("attachments",[&](const db::Row& r){return stringField(r,"attachment_id")==id;});
        Object row=rows.empty()?Object{}:rows[0]; auto path=root_/id;
        if(op=="begin") {
            int to=intField(req,"toId"),gid=intField(req,"groupId");
            auto size=int64Field(req,"file_size"); auto name=stringField(req,"file_name"); auto hash=stringField(req,"sha256");
            if((to>0)==(gid>0)||size<0||size>maxBytes_||name.empty()||name.size()>512||hash.size()!=64 ||
                !std::all_of(hash.begin(),hash.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))
                throw std::runtime_error("invalid attachment or size limit exceeded");
            if(gid>0&&!channels_.isMember(uid,gid)) throw std::runtime_error("group membership required");
            auto kind=stringField(req,"content_type","file");
            if(kind!="file"&&kind!="image"&&kind!="sticker") throw std::runtime_error("invalid attachment kind");
            if(row.empty()) {
                std::int64_t total=0; for(const auto& r:db_.query("attachments")) total+=int64Field(r,"file_size");
                if(total+size>quotaBytes_ || std::filesystem::space(root_).available<std::uint64_t(size)+64*1024*1024)
                    throw std::runtime_error("attachment storage quota exceeded");
                row={{"attachment_id",Value(id)},{"fromId",Value(uid)},{"toId",Value(to)},{"groupId",Value(gid)},
                    {"file_size",Value(size)},{"file_name",Value(name)},{"sha256",Value(hash)},{"content_type",Value(kind)},
                    {"complete",Value(0)},{"created",Value(std::int64_t(std::time(nullptr)))}};
                std::ofstream f(path,std::ios::binary); if(!f) throw std::runtime_error("cannot create attachment");
                db_.insert("attachments",row);
            } else if(intField(row,"fromId")!=uid||intField(row,"toId")!=to||intField(row,"groupId")!=gid||
                      int64Field(row,"file_size")!=size||stringField(row,"sha256")!=hash)
                throw std::runtime_error("upload metadata conflict");
        } else if(row.empty()) throw std::runtime_error("attachment not found");
        if(!canRead(uid,row)) throw std::runtime_error("attachment access denied");
        auto size=int64Field(row,"file_size");
        if(op=="upload"||op=="finish"||op=="cancel") {
            if(intField(row,"fromId")!=uid) throw std::runtime_error("uploader required");
        }
        if(op=="cancel") {
            if(intField(row,"complete")) throw std::runtime_error("published attachment cannot be cancelled");
            std::filesystem::remove(path); db_.remove("attachments",[&](const db::Row& r){return stringField(r,"attachment_id")==id;});
            return reply;
        }
        auto offset=std::int64_t(std::filesystem::file_size(path));
        if(op=="upload") {
            if(intField(row,"complete")) throw std::runtime_error("attachment already complete");
            if(int64Field(req,"offset",-1)!=offset) throw std::runtime_error("upload offset mismatch");
            auto data=decode(stringField(req,"data"));
            if(data.empty()||offset+std::int64_t(data.size())>size) throw std::runtime_error("invalid chunk");
            std::ofstream f(path,std::ios::binary|std::ios::app); f.write(data.data(),data.size()); f.flush();
            if(!f) throw std::runtime_error("attachment write failed");
            offset+=data.size();
        } else if(op=="finish" && !intField(row,"complete")) {
            if(offset!=size||hashFile(path)!=stringField(row,"sha256")) throw std::runtime_error("attachment SHA-256 mismatch");
            db_.update("attachments",[&](const db::Row& r){return stringField(r,"attachment_id")==id;},[](db::Row& r){r["complete"]=Value(1);});
            row["complete"]=Value(1);
        } else if(op=="download") {
            if(!intField(row,"complete")) throw std::runtime_error("attachment not complete");
            auto start=int64Field(req,"offset",-1); if(start<0||start>size) throw std::runtime_error("invalid download offset");
            std::ifstream f(path,std::ios::binary); f.seekg(start);
            std::string data(size_t(std::min<std::int64_t>(chunkBytes,size-start)),'\0');
            f.read(data.data(),data.size()); if(f.gcount()!=std::streamsize(data.size())) throw std::runtime_error("attachment read failed");
            reply["data"]=Value(encode(data)); reply["offset"]=Value(start); reply["eof"]=Value(start+std::int64_t(data.size())==size);
        } else if(op!="begin" && op!="finish" && op!="upload") throw std::runtime_error("unknown attachment operation");
        reply["next_offset"]=Value(offset); reply["attachment"]=Value(row); reply["max_bytes"]=Value(maxBytes_);
    } catch(const std::exception& e) { reply["status"]=Value("error"); reply["msg"]=Value(e.what()); }
    return reply;
}
void AttachmentStore::markPublished(const std::string& id,std::int64_t msgId) {
    db_.update("attachments",[&](const db::Row& r){return stringField(r,"attachment_id")==id;},[&](db::Row& r){r["msg_id"]=Value(msgId);});
}
}
