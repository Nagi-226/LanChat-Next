"""Real TCP server; isolated users/database; PC attachment bytes never touch a board."""
import argparse,base64,hashlib,json,os,pathlib,socket,struct,subprocess,tempfile,time

class Peer:
    def __init__(self,port):
        self.socket=socket.create_connection(('127.0.0.1',port));self.socket.settimeout(15);self.inbox=[]
    def send(self,**body):
        data=json.dumps(body,ensure_ascii=False,separators=(',',':')).encode();self.socket.sendall(struct.pack('>I',len(data))+data)
    def exact(self,n):
        data=b''
        while len(data)<n:
            b=self.socket.recv(n-len(data))
            if not b:raise EOFError('server disconnected')
            data+=b
        return data
    def wait(self,type,**fields):
        deadline=time.monotonic()+15
        def matches(a):return a.get('type')==type and all(a.get(k)==v for k,v in fields.items())
        while time.monotonic()<deadline:
            for i,a in enumerate(self.inbox):
                if matches(a):return self.inbox.pop(i)
            size=struct.unpack('>I',self.exact(4))[0];assert 0<size<=256*1024
            a=json.loads(self.exact(size))
            if matches(a):return a
            self.inbox.append(a)
        raise AssertionError((type,fields))
    def call(self,op,**fields):
        rid=str(time.monotonic_ns());self.send(type=70,op=op,request_id=rid,**fields)
        return self.wait(71,request_id=rid)
    def login(self,nick):
        self.send(type=2,id=nick,password='isolated-test-password');return self.wait(3)
    def close(self):self.socket.close()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--server',required=True);args=parser.parse_args()
    exe=str(pathlib.Path(args.server).resolve())
    with tempfile.TemporaryDirectory(prefix='lanchat-lan-') as folder:
        folder=pathlib.Path(folder);probe=socket.socket();probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close()
        env=os.environ.copy();env['LANCHAT_ATTACHMENT_MAX_MB']='100';env['LANCHAT_ATTACHMENT_QUOTA_MB']='512'
        def start():
            process=subprocess.Popen([exe,'--port',str(port),'--data',str(folder/'nested/data')],cwd=folder,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            for _ in range(100):
                try:p=Peer(port);p.close();return process
                except OSError:time.sleep(.05)
            raise AssertionError('server failed to start')
        process=start();peers=[]
        try:
            for nick in ('Alice','Bob','Outside'):
                p=Peer(port);peers.append(p);p.send(type=0,nickname=nick,password='isolated-test-password');assert p.wait(1)['status']=='ok';p.uid=p.login(nick)['id']
            a,b,c=peers
            anonymous=Peer(port);anonymous.send(type=24,toId=a.uid);assert anonymous.wait(33)['code']==401;anonymous.close()
            a.send(type=9,name='公司项目组');gid=a.wait(10)['groupId']
            b.send(type=13,groupId=gid);joined=b.wait(14);assert joined['name']=='公司项目组' and len(joined['users'])==2
            b.send(type=60,groupId=gid);assert {u['id'] for u in b.wait(61,groupId=gid)['users']}=={a.uid,b.uid}
            text='中文消息 😀 👍 🎉 👨‍👩‍👧‍👦 café <img src="fake">'
            a.send(type=16,groupId=gid,msg=text,fromId=c.uid)
            echo=a.wait(17,msg=text);assert echo['fromId']==a.uid and b.wait(17,msg=text)['msg_id']==echo['msg_id']
            c.send(type=16,groupId=gid,msg='unauthorized');assert c.wait(33)['code']==403
            c.send(type=24,groupId=gid);assert c.wait(25)['code']==403
            c.send(type=60,groupId=gid);assert c.wait(61,status='error')['code']==403
            c.send(type=47,fromId=a.uid,groupId=gid,msg_id=echo['msg_id'],reaction='x');assert c.wait(33)['code']==403
            c.send(type=29,ai_type='search',msg=text,request_id='private-search');assert not json.loads(c.wait(30)['msg'])['messages']
            a.send(type=5,toId=b.uid,msg='Direct 😀');assert a.wait(6,msg='Direct 😀')['fromId']==a.uid;b.wait(6,msg='Direct 😀')
            print('PASS registration, group join/name/members, Unicode, echo, authorization')
            # More than the frame limit, non-aligned SHA block count, interruption and restart.
            payload=os.urandom(2*1024*1024+61);sha=hashlib.sha256(payload).hexdigest()
            meta=dict(upload_id='resume-large-test',groupId=gid,file_name='中文-文件.bin',file_size=len(payload),sha256=sha,content_type='file')
            begun=a.call('begin',**meta);assert begun['status']=='ok';aid=begun['attachment_id']
            offset=0
            for _ in range(3):
                data=payload[offset:offset+49152];r=a.call('upload',attachment_id=aid,offset=offset,data=base64.b64encode(data).decode());assert r['status']=='ok';offset=r['next_offset']
            for p in peers:p.close()
            process.terminate();process.wait(timeout=10);process=start();peers=[Peer(port) for _ in range(3)];a,b,c=peers
            for p,n in zip(peers,('Alice','Bob','Outside')):p.uid=p.login(n)['id']
            r=a.call('begin',**meta);assert r['next_offset']==offset
            while offset<len(payload):
                data=payload[offset:offset+49152];r=a.call('upload',attachment_id=aid,offset=offset,data=base64.b64encode(data).decode());assert r['status']=='ok',r;offset=r['next_offset']
            assert a.call('finish',attachment_id=aid)['status']=='ok'
            message=b.wait(17,content_type='file');assert message['attachment']['attachment_id']==aid
            assert c.call('download',attachment_id=aid,offset=0)['status']=='error'
            assert b.call('upload',attachment_id=aid,offset=0,data='QQ==')['status']=='error'
            got=bytearray();offset=0
            while offset<len(payload):
                r=b.call('download',attachment_id=aid,offset=offset);assert r['status']=='ok';data=base64.b64decode(r['data']);got.extend(data);offset+=len(data)
            assert got==payload and hashlib.sha256(got).hexdigest()==sha
            before=message['msg_id'];b.send(type=24,groupId=gid,limit=1);history=b.wait(25);assert history['has_more'] and history['messages'][0]['content_type']=='file'
            b.send(type=24,groupId=gid,before=str(before),limit=1);assert b.wait(25)['messages'][0]['msg']==text
            # Repeated finish must not publish another message.
            assert a.call('finish',attachment_id=aid)['status']=='ok'
            b.send(type=24,groupId=gid,limit=50);assert len(b.wait(25)['messages'])==2
            print('PASS real bytes >256 KiB, streaming SHA-256, restart/resume, persisted typed history, download access')
            bad=dict(meta,upload_id='cancel-test',file_size=4,sha256=hashlib.sha256(b'abcd').hexdigest())
            r=a.call('begin',**bad);bid=r['attachment_id'];a.call('upload',attachment_id=bid,offset=0,data='YWJjZQ==');assert a.call('finish',attachment_id=bid)['status']=='error'
            assert a.call('cancel',attachment_id=bid)['status']=='ok';assert a.call('download',attachment_id=bid,offset=0)['status']=='error'
            assert a.call('begin',**dict(meta,upload_id='oversized',file_size=100*1024*1024+1))['status']=='error'
            b.send(type=15,groupId=gid);assert b.wait(19,id=b.uid)['status']=='ok';assert b.call('download',attachment_id=aid,offset=0)['status']=='error'
            print('PASS cancellation, checksum failure, size limit, leave revokes attachment access')
            # Full permitted-size upload exercises bounded memory and near-limit throughput.
            chunk=b'Z'*49152;size=100*1024*1024;h=hashlib.sha256();remaining=size
            while remaining:n=min(remaining,len(chunk));h.update(chunk[:n]);remaining-=n
            r=a.call('begin',upload_id='100mb-boundary',toId=c.uid,file_name='100MB.bin',file_size=size,sha256=h.hexdigest(),content_type='file');assert r['status']=='ok',r;aid=r['attachment_id'];offset=0
            start=time.monotonic()
            while offset<size:
                data=chunk[:min(len(chunk),size-offset)];r=a.call('upload',attachment_id=aid,offset=offset,data=base64.b64encode(data).decode());assert r['status']=='ok',r;offset=r['next_offset'];time.sleep(.001)
            assert a.call('finish',attachment_id=aid)['status']=='ok'
            print(f'PASS 100 MB boundary upload and server SHA-256 ({time.monotonic()-start:.1f}s)')
            if os.name=='nt':
                import ctypes,ctypes.wintypes as w
                class Counters(ctypes.Structure):
                    _fields_=[('cb',w.DWORD),('PageFaultCount',w.DWORD)]+[(name,ctypes.c_size_t) for name in ('PeakWorkingSetSize','WorkingSetSize','QuotaPeakPagedPoolUsage','QuotaPagedPoolUsage','QuotaPeakNonPagedPoolUsage','QuotaNonPagedPoolUsage','PagefileUsage','PeakPagefileUsage')]
                counters=Counters();counters.cb=ctypes.sizeof(counters)
                api=ctypes.WinDLL('psapi').GetProcessMemoryInfo;api.argtypes=[w.HANDLE,ctypes.POINTER(Counters),w.DWORD]
                assert api(w.HANDLE(int(process._handle)),ctypes.byref(counters),counters.cb)
                assert counters.PeakWorkingSetSize<96*1024*1024
                print(f'PASS bounded server peak working set: {counters.PeakWorkingSetSize/1024/1024:.1f} MiB during 100 MiB upload')
        finally:
            for p in peers:p.close()
            process.terminate();process.wait(timeout=10)
if __name__=='__main__':main()
