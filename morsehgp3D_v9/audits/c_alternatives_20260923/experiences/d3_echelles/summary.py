import json,sys
E=[50,100,200,400,800,1600,3200,6400,12800]
for f in sys.argv[1:]:
    try: d=json.load(open(f))
    except Exception: continue
    ce=d['cpu_ns']; tot=sum(ce)+sum(d['rect_cpu_ns'])
    j=5  # > 1600 mm
    long_edges=sum(d['edges'][j+1:]); rej=sum(d['witness_rejected'][j+1:])
    em=sum(d['q3_emitted'])+sum(d['q4_emitted']); em_long=sum(d['q3_emitted'][j+1:])+sum(d['q4_emitted'][j+1:])
    small=4  # <= 800 mm
    s=lambda k: sum(d[k][:small+1])
    print(f"{f}: n={d['n']} K={d['kmax']} balls={d['balls']} q34 edge+rect CPU={tot/1e9:.1f}s ; aretes>1,6m: {long_edges} ({100*long_edges/sum(d['edges']):.1f}% des paires developpees), rejet temoins {100*rej/max(1,long_edges):.1f}%, CPU {100*(sum(ce[j+1:])+sum(d['rect_cpu_ns'][j+1:]))/tot:.1f}%, emissions {100*em_long/em:.2f}% ; aretes<=0,8m: {s('edges')} paires, {s('cover_builds')} covers, {s('cover_sites')} sites de cover, {s('q3_seeds')} graines q3, {s('q4_seeds')} graines q4, CPU {100*(sum(ce[:small+1]))/tot:.1f}%, us/arete {sum(ce[:small+1])/1e3/max(1,s('edges')):.1f} vs longues {sum(ce[j+1:])/1e3/max(1,long_edges):.1f}")
