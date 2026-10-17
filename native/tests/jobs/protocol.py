import json,subprocess,sys,threading,queue
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
        self.p.stdin.close()
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
