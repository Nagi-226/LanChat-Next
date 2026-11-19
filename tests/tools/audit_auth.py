# LanChat-Next 注册/登录独立审计客户端。
#  purpose: speak the wire protocol ([u32 big-endian length] + JSON) directly,
#  without importing any LanChat code, so the server is tested against an
#  outside observer rather than against itself.
# 用法: py audit_auth.py [--port 12346] [--phase 1|2]
#   phase 1 = 注册/登录/负向/ spoof 全套（服务端首次启动后跑）
#   phase 2 = 重启服务端后复登（验证凭据持久化）
import argparse
import json
import socket
import struct
import time

ap = argparse.ArgumentParser()
ap.add_argument("--host", default="127.0.0.1")
ap.add_argument("--port", type=int, default=12346)
ap.add_argument("--phase", type=int, default=1)
args = ap.parse_args()

RESULTS = []


def check(name, ok, evidence):
    RESULTS.append((name, ok))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}: {evidence}")


class Client:
    """One TCP connection; frames are [4-byte big-endian length][JSON bytes]."""

    def __init__(self):
        self.sock = socket.create_connection((args.host, args.port), timeout=15)
        self.buf = b""

    def send(self, obj):
        payload = json.dumps(obj).encode("utf-8")
        self.sock.sendall(struct.pack(">I", len(payload)) + payload)

    def recv(self):
        # independent framing: read until one whole frame is present
        while True:
            if len(self.buf) >= 4:
                n = struct.unpack(">I", self.buf[:4])[0]
                if len(self.buf) >= 4 + n:
                    payload = self.buf[4:4 + n]
                    self.buf = self.buf[4 + n:]
                    return json.loads(payload)
            chunk = self.sock.recv(65536)
            if not chunk:
                raise ConnectionError("server closed connection")
            self.buf += chunk

    def recv_expect(self, types):
        # 跳过不请自到的在线/离线广播（type 7/8/42），只取期待类型的响应。
        # 服务端断线广播发给所有会话（TcpServer::markOfflineAndBroadcast），
        # 不跳过会把广播误读成响应（phase 1 的 C11/C14 曾因此读串）。
        while True:
            r = self.recv()
            if r.get("type") in (7, 8, 42):
                continue
            return r

    def close(self):
        self.sock.close()


def has_password_key(node):
    """Recursively look for credential material leaking into any response."""
    if isinstance(node, dict):
        for k, v in node.items():
            if "password" in k.lower():
                return True
            if has_password_key(v):
                return True
    elif isinstance(node, list):
        return any(has_password_key(v) for v in node)
    return False


ALICE_PW = "correct-horse-42"
BOB_PW = "bob-password-7"

if args.phase == 1:
    c = Client()
    # C1 注册：应回 type=1 status=ok 并分配数字 id
    c.send({"type": 0, "nickname": "audit_alice", "password": ALICE_PW})
    r = c.recv()
    alice_id = r.get("id")
    check("C1 register ok", r.get("type") == 1 and r.get("status") == "ok"
          and isinstance(alice_id, int) and alice_id >= 1, f"resp={r}")

    # C2 正确密码登录：应回 type=3，且响应里任何位置都不含 password 字段
    t0 = time.time()
    c.send({"type": 2, "id": alice_id, "password": ALICE_PW})
    r = c.recv()
    ms = int((time.time() - t0) * 1000)
    leak = has_password_key(r)
    check("C2 login ok + no credential leak", r.get("type") == 3
          and r.get("id") == alice_id and r.get("nickname") == "audit_alice"
          and not leak, f"type={r.get('type')} login_ms={ms} leak={leak} "
          f"users_in_resp={len(r.get('users', []))}")

    # C3 错密码：应回 type=4（登录失败），不能回 type=3
    c.send({"type": 2, "id": alice_id, "password": "wrong-password"})
    r = c.recv()
    check("C3 wrong password rejected", r.get("type") == 4, f"resp={r}")

    # C4 不存在的 id：应回 type=4
    c.send({"type": 2, "id": 999999, "password": ALICE_PW})
    r = c.recv()
    check("C4 unknown id rejected", r.get("type") == 4, f"resp={r}")

    # C5 重名注册：应回 type=1 且 code=409
    c.send({"type": 0, "nickname": "audit_alice", "password": "other-pw"})
    r = c.recv()
    check("C5 duplicate nickname rejected", r.get("type") == 1 and r.get("code") == 409,
          f"resp={r}")

    # C6 空密码注册：应回 400
    c.send({"type": 0, "nickname": "audit_empty", "password": ""})
    r = c.recv()
    check("C6 empty password rejected", r.get("type") == 1 and r.get("code") == 400,
          f"resp={r}")

    # C7 缺 nickname 注册：应回 400
    c.send({"type": 0, "password": "x"})
    r = c.recv()
    check("C7 missing nickname rejected", r.get("type") == 1 and r.get("code") == 400,
          f"resp={r}")

    # C8 bcrypt 72 字节截断修复验证：注册 80 字符密码（v2 预哈希），
    #    用前 72 字符登录必须被拒（修复前会成功 = 截断等价）。
    long_pw = "A" * 72 + "TAIL-SECRET"
    c.send({"type": 0, "nickname": "audit_long", "password": long_pw})
    r = c.recv()
    long_id = r.get("id")
    c.send({"type": 2, "id": long_id, "password": "A" * 72})
    r = c.recv()
    check("C8 bcrypt truncation defeated", r.get("type") == 4,
          f"login_with_first_72_chars_type={r.get('type')} (4=fixed, 3=still truncated)")
    c.send({"type": 2, "id": long_id, "password": long_pw})
    r = c.recv()
    check("C8b full long password still works", r.get("type") == 3, f"type={r.get('type')}")

    # C9 未登录直接发私聊（冒充他人 fromId）：应被拒（400），不能投递
    c2 = Client()
    c2.send({"type": 5, "fromId": alice_id, "toId": alice_id, "msg": "spoof-unauth"})
    r = c2.recv()
    check("C9 unauthenticated SendMsg rejected", r.get("code") == 400 or r.get("type") == 33,
          f"resp={r}")
    c2.close()

    # C10 登录 bob 后把 fromId 写成 alice：服务端应按会话归属记 bob，不信请求字段
    c.send({"type": 0, "nickname": "audit_bob", "password": BOB_PW})
    r = c.recv()
    bob_id = r.get("id")
    c3 = Client()
    c3.send({"type": 2, "id": bob_id, "password": BOB_PW})
    r = c3.recv()
    assert r.get("type") == 3, r
    c3.send({"type": 5, "fromId": alice_id, "toId": bob_id, "msg": "spoof-auth"})
    r = c3.recv()
    check("C10 fromId spoof ignored (session uid wins)", r.get("fromId") == bob_id,
          f"echo_fromId={r.get('fromId')} bob_id={bob_id} alice_id={alice_id} type={r.get('type')}")
    c3.close()

    # C11 把昵称填进 id 字段：协议要求数字 id，应失败（记录行为供客户端 UX 参考）
    c.send({"type": 2, "id": "audit_alice", "password": ALICE_PW})
    r = c.recv()
    check("C11 nickname-in-id rejected (informational)", r.get("type") == 4,
          f"resp={r}")

    # C14/C15 schema 上限是否被服务端强制：password 256 / nickname 64 是 schema 上限
    c.send({"type": 0, "nickname": "audit_maxpw", "password": "P" * 256})
    r = c.recv()
    ok256 = r.get("type") == 1 and r.get("status") == "ok"
    c.send({"type": 0, "nickname": "audit_overpw", "password": "P" * 257})
    r = c.recv()
    over_pw_accepted = r.get("type") == 1 and r.get("status") == "ok"
    c.send({"type": 0, "nickname": "N" * 65, "password": "x"})
    r = c.recv()
    over_nick_accepted = r.get("type") == 1 and r.get("status") == "ok"
    check("C14/15 schema max enforcement (informational)", True,
          f"pw256_ok={ok256} pw257_accepted={over_pw_accepted} nick65_accepted={over_nick_accepted}")

    c.close()
    print(f"PHASE1_SUMMARY alice_id={alice_id} bob_id={bob_id} "
          f"passed={sum(1 for _, ok in RESULTS if ok)}/{len(RESULTS)}")

else:
    if args.phase == 3:
        # 复测 phase 1 被广播帧污染的用例：全部用新连接 + recv_expect
        c = Client()
        # C11 昵称填进 id 字段：协议要求数字 id，应回 type=4
        c.send({"type": 2, "id": "audit_alice", "password": ALICE_PW})
        r = c.recv_expect({4})
        check("C11 nickname-in-id rejected", r.get("type") == 4, f"resp={r}")
        c.close()

        c = Client()
        # 昵称带 _p3 后缀：phase 1 的同名注册服务端已实际执行，重名会 409 污染读数
        c.send({"type": 0, "nickname": "audit_maxpw_p3", "password": "P" * 256})
        r = c.recv_expect({1})
        ok256 = r.get("status") == "ok"
        c.send({"type": 0, "nickname": "audit_overpw_p3", "password": "P" * 257})
        r = c.recv_expect({1})
        over_pw_rejected = r.get("code") == 400
        c.send({"type": 0, "nickname": "M" * 65, "password": "x"})
        r = c.recv_expect({1})
        over_nick_rejected = r.get("code") == 400
        c.send({"type": 0, "nickname": "K" * 64, "password": "x"})
        r = c.recv_expect({1})
        nick64_ok = r.get("status") == "ok"
        check("C14/15 schema max enforced", ok256 and over_pw_rejected
              and over_nick_rejected and nick64_ok,
              f"pw256_ok={ok256} pw257_rejected={over_pw_rejected} "
              f"nick65_rejected={over_nick_rejected} nick64_ok={nick64_ok}")
        # C16 多用户后复登：users 列表变大后再扫一次凭据泄露 + 量响应体积
        c.send({"type": 2, "id": 1000, "password": ALICE_PW})
        r = c.recv_expect({3, 4})
        raw_len = len(json.dumps(r))
        check("C16 relogin no leak with many users", r.get("type") == 3
              and not has_password_key(r),
              f"type={r.get('type')} users={len(r.get('users', []))} resp_bytes={raw_len}")

        # C17 非好友不应收到离线广播：bob 上线又断线，alice（非好友）应什么都收不到
        c.send({"type": 0, "nickname": "audit_bob_p3", "password": BOB_PW})
        r = c.recv_expect({1})
        bob_id = r.get("id")
        cb = Client()
        cb.send({"type": 2, "id": bob_id, "password": BOB_PW})
        rb = cb.recv_expect({3, 4})
        assert rb.get("type") == 3, rb
        cb.close()  # bob 断线 → 服务端发 UserOffline；修复后只发好友
        c.sock.settimeout(1.5)
        noise = None
        try:
            noise = c.recv()
        except socket.timeout:
            pass
        except ConnectionError:
            pass
        c.sock.settimeout(15)
        check("C17 non-friend gets no offline noise", noise is None,
              f"unexpected_frame={noise}")
        c.close()
    else:
        if args.phase == 4:
            # admin 权限用例：种子账户登录 / announce 全员送达 / 非 admin 403 / 未登录 401
            import os
            admin_id = int(os.environ.get("AUDIT_ADMIN_ID", "1000"))
            admin_pw = os.environ.get("AUDIT_ADMIN_PASS", "")
            ca = Client()
            ca.send({"type": 2, "id": admin_id, "password": admin_pw})
            r = ca.recv_expect({3, 4})
            check("A1 admin login", r.get("type") == 3 and r.get("nickname") == "Nagi_226",
                  f"type={r.get('type')} nickname={r.get('nickname')}")
            # 旁观者：普通注册用户，应能收到 admin 的 announce
            cw = Client()
            # 昵称带时间戳：phase 4 可重复跑，重名会 409 拿不到 id
            cw.send({"type": 0, "nickname": f"audit_watcher_p4_{int(time.time())}",
                     "password": "watch-pw"})
            r = cw.recv_expect({1})
            wid = r.get("id")
            cw.send({"type": 2, "id": wid, "password": "watch-pw"})
            r = cw.recv_expect({3, 4})
            assert r.get("type") == 3, r
            # admin 发 announce
            ca.send({"type": 33, "event": "announce", "fromId": admin_id,
                     "msg": "admin announce test"})
            r = ca.recv_expect({33})
            ok_ack = r.get("status") == "ok"
            got = cw.recv_expect({33})
            check("A2 admin announce reaches all", ok_ack
                  and got.get("event") == "announce" and got.get("msg") == "admin announce test",
                  f"ack={r.get('status')} watcher_got={got.get('event')}")
            # 非 admin 发 announce → 403
            cw.send({"type": 33, "event": "announce", "fromId": wid, "msg": "intruder"})
            r = cw.recv_expect({33})
            check("A3 non-admin announce 403", r.get("code") == 403, f"resp={r}")
            # 未登录发 announce → 401
            cu = Client()
            cu.send({"type": 33, "event": "announce", "fromId": admin_id, "msg": "anon"})
            r = cu.recv_expect({33})
            check("A4 unauth announce 401", r.get("code") == 401, f"resp={r}")
            cu.close(); cw.close(); ca.close()
        elif args.phase == 5:
            # 用户名登录：string id 按 nickname 解析；数字 id 不回归。
            # 密码从环境变量读（不落盘）。
            import os
            pw = os.environ.get("AUDIT_ADMIN_PASS", "")
            c = Client()
            c.send({"type": 2, "id": "Nagi_226", "password": pw})
            r = c.recv_expect({3, 4})
            check("U1 username login", r.get("type") == 3 and r.get("nickname") == "Nagi_226",
                  f"type={r.get('type')} nickname={r.get('nickname')}")
            c.send({"type": 2, "id": "Nagi_226", "password": "wrong"})
            r = c.recv_expect({3, 4})
            check("U2 username wrong pw 401", r.get("code") == 401, f"resp={r}")
            c.send({"type": 2, "id": "no_such_user_xyz", "password": pw})
            r = c.recv_expect({3, 4})
            check("U3 unknown username 401", r.get("code") == 401, f"resp={r}")
            c.send({"type": 2, "id": 1000, "password": pw})
            r = c.recv_expect({3, 4})
            check("U4 numeric id still works", r.get("type") == 3, f"type={r.get('type')}")
            c.close()
        else:
            # phase 2：服务端重启后复登，验证凭据持久化（id 由环境变量传入）。
            # 若数据库里是修复前的 v1 老哈希，第一次登录走 v1 路径并自动升 v2；
            # 第二次登录走 v2 路径——两次都成功才证明升级链与持久化都通。
            import os
            alice_id = int(os.environ.get("AUDIT_ALICE_ID", "1000"))
            c = Client()
            c.send({"type": 2, "id": alice_id, "password": ALICE_PW})
            r = c.recv_expect({3, 4})
            check("C12 credentials survive server restart", r.get("type") == 3,
                  f"id={alice_id} resp_type={r.get('type')} nickname={r.get('nickname')}")
            c.send({"type": 2, "id": alice_id, "password": ALICE_PW})
            r = c.recv_expect({3, 4})
            check("C12b second login (post-upgrade v2 path)", r.get("type") == 3,
                  f"resp_type={r.get('type')}")
            c.close()

fails = [n for n, ok in RESULTS if not ok]
print("AUDIT_EXIT", 1 if fails else 0, "fails=", fails)