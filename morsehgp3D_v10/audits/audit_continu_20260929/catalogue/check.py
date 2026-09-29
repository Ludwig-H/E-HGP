"""R2 uses small but not pathological leaf=8; every native call has a 5 second observation cap."""
import hashlib,json,subprocess
import check_r1_interrupted as R1

real_run=subprocess.run
events=[]
def observed_run(cmd,**kw):
    cmd=list(cmd)
    if cmd[3]=='2': cmd[3]=str(max(8,int(cmd[2])+1))
    event={'argv':cmd,'status':'running'}
    events.append(event)
    try:
        value=real_run(cmd,timeout=5,**kw)
        event.update(status='completed',returncode=value.returncode)
        return value
    except BaseException as exc:
        event.update(status='failed',error=repr(exc))
        raise
    finally:
        (R1.HERE/'commands_r2.json').write_text(json.dumps(events,indent=2)+'\n')

subprocess.run=observed_run
try:
    R1.main()
finally:
    rp=R1.HERE/'receipt.json'
    if rp.exists():
        receipt=json.loads(rp.read_text())
        receipt['checks'][-1]='leaf-size 0/max(8,K+1)/256; workers 1/4'
        receipt['first_attempt']='Interrupted (130) during cube_max in leaf_size=2 stress; kept in check_r1_interrupted.py'
        for name in ('check.py','check_r1_interrupted.py','commands_r2.json'):
            path=R1.HERE/name
            receipt['hashes'][str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
        rp.write_text(json.dumps(receipt,indent=2)+'\n')
