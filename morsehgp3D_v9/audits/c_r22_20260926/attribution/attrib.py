import sys; sys.path.insert(0,'.')
import io, contextlib
with contextlib.redirect_stdout(io.StringIO()):
    from tables import avg, CASES
print('| frame K | chain R21 | chain R22 | Δ chain | pinned (Δ lanes xfer + host) | seal (Δ validate) | early (Δ tower_index + Δ census) | front (drift) | tower tail | A(K)/window | other | lever sum |')
print('|---|---|---|---|---|---|---|---|---|---|---|---|')
for K in (5,10):
    for f in ['00','01','02','b00','b01','b02']:
        a=avg('r21',CASES[(f,K)]['r21']); b=avg('r22',CASES[(f,K)]['r22'])
        D=lambda k: b[k]-a[k]
        pin=D('lanes_xfer')+D('lanes_host'); seal=D('tower.validate'); early=D('tower_index')+D('census')
        front=D('front'); tail=D('tower.tail'); win=D('tower.window')
        tot=D('chain_total'); other=tot-pin-seal-early-front-tail-win
        print(f'| {f} K{K} | {a["chain_total"]:.1f} | {b["chain_total"]:.1f} | {tot:+.1f} | {pin:+.1f} ({D("lanes_xfer"):+.1f} {D("lanes_host"):+.1f}) | {seal:+.1f} | {early:+.1f} ({D("tower_index"):+.1f} {D("census"):+.1f}) | {front:+.1f} | {tail:+.1f} | {win:+.1f} | {other:+.1f} | {pin+seal+early:+.1f} |')
