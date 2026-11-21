# PC 附件协议 v1

沿用 4 字节大端长度 + UTF-8 JSON；每帧最大 256 KiB。附件请求 `type=70`，响应 `type=71`，均需登录。身份使用连接会话，忽略伪造的 `fromId`。

## 上传

1. `op=begin`：`request_id`、客户端生成的 `upload_id`、`toId` 或 `groupId`（二选一）、`file_name`、`file_size`、`sha256`、`content_type`（`file/image/sticker`）。服务端生成 `attachment_id`，返回磁盘实际 `next_offset`，相同 upload_id 可续传。
2. `op=upload`：`attachment_id`、`offset`、`data`。data 为最多 48 KiB 二进制的规范 Base64；只有上传者可写，偏移必须等于当前文件大小，收到响应后再发送下一块。
3. `op=finish`：核对完整长度与流式 SHA-256 后发布一次持久化聊天消息。正常重复 finish 不重复发布。
4. `op=cancel`：只允许上传者取消未完成附件，删除部分文件及元数据；已发布文件不能通过此操作删除。

## 下载与消息

`op=download`：`attachment_id` 和 `offset`，返回 data、offset、eof。私聊仅发送者/接收者可下载；群附件每次读取均检查当前群成员身份，退出后失去访问。

聊天通知仍使用 `type=6/17`，带持久 `msg_id`、`content_type` 和 `attachment` 对象；正文 msg 是可显示的文件名。服务器历史/离线消息保留附件引用，客户端按 ID 合并。旧 type26 元数据协议返回升级错误，不把元数据到达宣称为文件传输成功。

type60 请求真实群成员，type61 返回成员；groupId=0 表示同一 LAN 服务的员工目录。身份已登录的客户端可查看目录，注册/上下线更新目录。

客户端最多 4 项活动传输，每项仅一块在途，暂停/续传使用新的 request_id 过滤旧响应。下载 `.part` 校验后改名，缓存按服务器与账号隔离。普通聊天保留 20 帧/秒限额；认证附件请求有独立 64 MiB/秒请求字节上限，服务端待发送队列最大约 8 MiB，超限断开。

默认单文件 100 MiB、服务端总配额 5 GiB；未完成上传计入预留配额，磁盘需保留 64 MiB 余量。设置见 [局域网部署](lan-deployment.md)。完整附件持久化在 attachments 表和磁盘，未完成文件 7 天启动回收。

崩溃、电源断电与大规模压力尚未做故障注入，不能据普通重启用例宣称 exactly-once 或生产 SLA。服务端当前运行于单 Asio 事件循环；磁盘慢、注册 bcrypt 和大文件最终哈希可能短暂阻塞该循环，后续企业压力阶段需要测量并拆出工作线程。

Console 对非 text 类型和任何 attachment 字段分别在 LanChatSink、DisplayBridge 入口拒绝转发。固件不参与该附件协议。
