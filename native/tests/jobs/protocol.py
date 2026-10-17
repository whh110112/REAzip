import json,subprocess,sys,threading,queue,time
worker,engine,archive=sys.argv[1:]
class Client:
    def __init__(self):
        self.p=subprocess.Popen([worker,'--engine',engine],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,encoding='utf-8')
        self.q=queue.Queue()
        def read():
            for line in self.p.stdout:self.q.put(json.loads(line))
        threading.Thread(target=read,daemon=True).start()
        assert self.get()['type']=='ready'
    def get(self):return self.q.get(timeout=12)
    def send(self,**msg):
        msg.setdefault('protocolVersion',1);msg.setdefault('jobId','test-job')
        self.p.stdin.write(json.dumps(msg,ensure_ascii=False)+'\n');self.p.stdin.flush()
    def close(self):
        try:self.p.stdin.close()
        except BrokenPipeError:pass
        try:assert self.p.wait(timeout=5)==0
        finally:
            if self.p.poll() is None:self.p.kill()
c=Client();c.send(type='submit',operation='list',archive=archive)
assert c.get()['type']=='started'
challenge=c.get();assert challenge['type']=='passwordRequired'
c.send(type='respondChallenge',challengeId=challenge['challengeId'],password='incorrect')
challenge=c.get();assert challenge['type']=='passwordRequired' and challenge['incorrect']
c.send(type='respondChallenge',challengeId=challenge['challengeId'],password='test-only')
terminal=0;entries=[]
while not terminal:
    frame=c.get();assert frame['jobId']=='test-job'
    if frame['type']=='entriesChunk':entries+=frame['entries']
    if frame['type'] in ['completed','failed','cancelled']:
        terminal+=1;assert frame['type']=='completed'
assert len(entries)==1;c.close()
c=Client();c.send(type='submit',operation='list',archive=archive)
assert c.get()['type']=='started';assert c.get()['type']=='passwordRequired'
c.send(type='cancel');assert c.get()['type']=='cancelled';c.close()
c=Client();c.send(type='hello',protocolVersion=99)
assert c.get()['code']=='ProtocolVersionMismatch';c.close()
c=Client();c.p.stdin.write('{not json}\n');c.p.stdin.flush()
assert c.get()['code']=='ProtocolError';c.close()
print('worker: password retry/cancel/protocol failures PASS')
c=Client();frame=json.dumps(dict(protocolVersion=1,jobId='test-job',type='submit',operation='list',archive=archive))+'\n'
for part in [frame[:4],frame[4:23],frame[23:-1],frame[-1:]]:
    c.p.stdin.write(part);c.p.stdin.flush();time.sleep(.01)
assert c.get()['type']=='started';challenge=c.get();assert challenge['type']=='passwordRequired'
c.send(type='cancel',jobId='old-job');c.send(type='respondChallenge',challengeId='old-challenge',password='incorrect')
try:c.q.get(timeout=.2);raise AssertionError('stale controls changed the active task')
except queue.Empty:pass
c.send(type='respondChallenge',challengeId=challenge['challengeId'],password='test-only')
while True:
    message=c.get()
    if message['type'] in ['completed','failed','cancelled']:
        assert message['type']=='completed';break
c.close();assert not any(f['type'] in ['completed','failed','cancelled'] for f in list(c.q.queue))
c=Client();c.send(type='submit',operation='list',archive=archive);assert c.get()['type']=='started';assert c.get()['type']=='passwordRequired';c.p.stdin.close();assert c.get()['type']=='cancelled';c.close()
c=Client()
try:c.p.stdin.write('x'*(16*1024*1024+1)+'\n');c.p.stdin.flush()
except BrokenPipeError:pass
assert c.get()['code']=='ProtocolError';c.close()
print('worker: fragmented frames, stale job/challenge, EOF, oversized frame, unique terminal PASS')
