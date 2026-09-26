import sys, io, contextlib; sys.path.insert(0,'.')
with contextlib.redirect_stdout(io.StringIO()):
    from tables import avg, CASES
ROWS=[('chain','chain_total'),('q2 own (hidden, q2_wait=0)','q2_own'),('q2-side census incl. index (bg)','q2_census(bg)'),('· of which tower index (bg)','q2_census_index(bg)'),('q34 total','q34'),('· front (CPU)','front'),('· filter call / host','filter'),('· certificates call','cert'),('· lanes call','lanes'),('·· lanes kernel','lanes_kern'),('·· lanes transfer','lanes_xfer'),('·· lanes host','lanes_host'),('· host in the 3 calls','host_in_calls'),('· untimed glue','glue_untimed'),('· q34 tail','tail'),('device kern / xfer total','kern_total'),('merge','merge'),('tower_index (post-merge)','tower_index'),('census (post-merge)','census'),('tower','tower'),('· validate','tower.validate'),('· phase 0 total Σstatic','tower.static'),('· static(Kmax)','static(K)'),('· A(Kmax)','A(K)'),('· window static+lots','tower.window'),('· tail pop+img+bank+enc','tower.tail'),('chain residual','chain_residual')]
for K in (5,10):
    fr=['00','01','02','b00','b01','b02']
    data={f:(avg('r21',CASES[(f,K)]['r21']),avg('r22',CASES[(f,K)]['r22'])) for f in fr}
    print(f'\nK{K}: R22 (Δ vs R21), ms')
    print('| phase | '+' | '.join(fr)+' |'); print('|'+'---|'*(len(fr)+1))
    for lab,k in ROWS:
        cells=[]
        for f in fr:
            a,b=data[f]
            if k=='kern_total': cells.append(f'{b["kern_total"]:.0f}/{b["xfer_total"]:.0f} ({b["kern_total"]-a["kern_total"]:+.0f}/{b["xfer_total"]-a["xfer_total"]:+.0f})')
            elif k=='filter': cells.append(f'{b["filter"]:.0f}/{b["filter_host"]:.0f} ({b["filter"]-a["filter"]:+.0f})')
            else: cells.append(f'{b[k]:.1f} ({b[k]-a[k]:+.1f})')
        print(f'| {lab} | '+' | '.join(cells)+' |')
