"""Sous-processus factices : conserver le premier diagnostic sans executer de natif."""
import contextlib
import hashlib
import io
import json
import subprocess
from unittest.mock import patch
import q4_presentation_oracle as oracle


def selftest():
    checks=0;scenarios=0
    def need(condition,message):
        nonlocal checks
        oracle.require(condition,message);checks+=1
    executable='/fake/q4_probe';rows=oracle.cases(21)[:2]
    complete='bits 21\n'+''.join(oracle.line_of(row,21)+'\n' for row in rows)
    partial='bits 21\n'+oracle.line_of(rows[0],21)+'\nok candidate'
    # Les cinq bases : code3, signal, stderr avec code0, timeout en octets, succes silencieux.
    bads=((3,partial.encode(),b'native failure\n'),(-11,b'',b'\xff'+b'x'*12000),
          (0,complete.encode(),b'unexpected stderr\n'))
    for code,stdout,stderr in bads:
        stream=io.StringIO();child=subprocess.CompletedProcess([executable],code,stdout,stderr)
        with patch.object(oracle.subprocess,'run',return_value=child),contextlib.redirect_stderr(stream):
            try:oracle.run_child(executable,'payload',7,'native',rows=rows,bits=21)
            except ValueError:pass
            else:raise ValueError('child failure accepted')
        result=json.loads(stream.getvalue());scenarios+=1
        need(result['argv']==[executable] and result['phase']=='native','argv/phase')
        need(result['returncode']==code and result['timeout'] is None,'code preserved')
        need(result['stdout']==oracle.bounded(stdout),'stdout preserved/bounded')
        need(result['stderr']==oracle.bounded(stderr),'stderr preserved/bounded')
        need(result['stderr_sha256']==hashlib.sha256(stderr).hexdigest(),'full stderr hash')
        need(result['stderr_truncated']==(len(oracle.decoded(stderr))>4096),'truncation explicit')
        need(len(result['stderr'])<4200,'bounded diagnostic')
        need(result.get('first_unverified_request_index')==(1 if code==3 else None),'request location')
        if code==3:need(result['request']==json.loads(json.dumps(rows[1])),'actual request retained')
    stream=io.StringIO();error=subprocess.TimeoutExpired([executable],7,output=partial.encode(),stderr=b'timeout\xff')
    with patch.object(oracle.subprocess,'run',side_effect=error),contextlib.redirect_stderr(stream):
        try:oracle.run_child(executable,'payload',7,'native',rows=rows,bits=21)
        except subprocess.TimeoutExpired as caught:need(caught is error,'original timeout propagated')
        else:raise ValueError('timeout accepted')
    result=json.loads(stream.getvalue());scenarios+=1
    need(result['returncode'] is None and result['timeout']==7,'timeout/code')
    need(result['stdout']==partial and result['stderr']=='timeout\\xff','timeout bytes retained')
    need(result['argv']==[executable] and result['first_unverified_request_index']==1,'timeout location')
    good=subprocess.CompletedProcess([executable],0,b'bits 21\n',b'');stream=io.StringIO()
    with patch.object(oracle.subprocess,'run',return_value=good) as call,contextlib.redirect_stderr(stream):
        result=oracle.run_child(executable,'payload',7,'header')
    scenarios+=1;need(result.stdout=='bits 21\n' and result.returncode==0,'successful child intact')
    need(not stream.getvalue(),'success no diagnostic')
    need(call.call_args.kwargs['input']==b'payload' and call.call_args.kwargs['timeout']==7,'real arguments')
    # Code0 geometriquement faux, sortie tronquee et en-tete incorrect : diagnostics aussi.
    for text,index in ((complete.replace('candidate','sphere',1),0),(partial,1),('bad header\n',None)):
        stream=io.StringIO();child=subprocess.CompletedProcess([executable],0,text,'')
        with contextlib.redirect_stderr(stream):
            try:oracle.check_output(executable,child,rows,21)
            except ValueError:pass
            else:raise ValueError('bad protocol accepted')
        result=json.loads(stream.getvalue());scenarios+=1
        need(result['phase']=='judge' and result['returncode']==0,'judge phase')
        need(result['stdout']==text and result['stderr']=='','judge outputs')
        need(result.get('first_unverified_request_index')==index,'judge request location')
        need(bool(result.get('reason')),'judge rejection reason')
    # Refus de syntaxe attendu2 : ni un autre code ni stderr ne deviennent un succes.
    for code,stderr,accepted in ((2,b'',True),(0,b'',False),(2,b'bad',False)):
        stream=io.StringIO();child=subprocess.CompletedProcess([executable],code,b'bits 21\n',stderr)
        with patch.object(oracle.subprocess,'run',return_value=child),contextlib.redirect_stderr(stream):
            try:oracle.run_child(executable,'bad\n',7,'malformed',expected=2)
            except ValueError:need(not accepted,'unexpected refusal')
            else:need(accepted,'invalid code accepted')
        scenarios+=1;need(bool(stream.getvalue())!=accepted,'only failure diagnostic')
    # Raccord run reel : retablir les anciens require muets doit faire echouer cette porte.
    for code,stdout,phase,index in ((3,partial,'native',1),
                                   (0,complete.replace('candidate','sphere',1),'judge',0)):
        stream=io.StringIO();responses=[good,subprocess.CompletedProcess([executable],code,stdout.encode(),b'')]
        with patch.object(oracle,'cases',return_value=rows),patch.object(oracle.subprocess,'run',side_effect=responses) as call:
            with contextlib.redirect_stderr(stream):
                try:oracle.run(executable)
                except ValueError:pass
                else:raise ValueError('run accepted failure')
        result=json.loads(stream.getvalue());scenarios+=1
        need(result['phase']==phase and result['returncode']==code,'run diagnostic phase/code')
        need(result['stdout']==stdout,'run diagnostic output')
        need(result['first_unverified_request_index']==index,'run diagnostic request')
        need(call.call_count==2,'run stopped at first failed process')
    print(f'q4_presentation_process_verdict conforme scenarios{scenarios} checks{checks} native0')


if __name__=='__main__':selftest()
