# Projection D6 (secondes) par trame et K, trois bandes. Sources : modele d'instructions (model.py) sur les
# compteurs deterministes des sondes v12 ; temps CPU G4 W48 de R7b ; phase A FULL 4,65 Gcyc pour 1 638 573
# noeuds de l'ordre 10 (08/000000, harnais hors depot) => 0,6-0,9 us/noeud sur G4 (hypothese).
frames = {  # q34 instr (G), census instr (G), FULL statique instr (G), q2 CPU W48 (s), q34 CPU W48 (s), noeuds ordre Kmax, tour CPU (s)
 ('000000',5): (204, 6.1, 2.4, 0.452, 4.11, 576371, 0.90),
 ('000100',5): (158, 4.8, 1.8, 0.235, 2.52, 478265, 0.73),
 ('000200',5): (237, 6.2, 1.8, 0.506, 4.85, 609376, 0.97),
 ('000000',10): (650, 32.8, 34.3, 0.747, 8.17, 1638573, 4.06),
 ('000100',10): (497, 24.0, 24.6, 0.413, 5.23, 1265065, 3.20),
 ('000200',10): (752, 30.0, 25.2, 0.822, 9.84, 1555780, 3.82),
}
bands = {'pessimiste': (0.4e12, 2.0, 0.9e-6), 'central': (1.5e12, 1.5, 0.75e-6), 'optimiste': (4e12, 1.2, 0.6e-6)}
for (f, K), (qi, ci, fi, q2c, q34c, nodes, tour) in frames.items():
    out = []
    for b, (thr, ovh, us) in bands.items():
        q34 = qi * 1e9 / thr * ovh
        cpu_ratio = (qi * 1e9 / q34c)  # instr-modele/s du CPU W48
        q2 = q2c * cpu_ratio / thr * ovh          # q2 porte au meme rapport que q3/q4
        geo = (ci + fi) * 1e9 / thr * ovh
        merge = 0.005 if K == 5 else 0.012
        io = 0.01 if K == 5 else 0.04             # D2H/ecritures sorties, epingle
        orch = 0.01
        cal_noD5 = nodes * us
        cal_D5 = 0.03 if K == 5 else 0.08
        gpu_part = q2 + q34 + merge + geo + orch
        noD5 = gpu_part + cal_noD5 + io
        withD5 = gpu_part + cal_D5 + io
        out.append(f'{b}: q2 {q2:.3f} q34 {q34:.3f} geo {geo:.3f} cal {cal_noD5:.2f} | sansD5 {noD5:.2f} avecD5 {withD5:.2f}')
    print(f, 'K', K, 'CPU R7b: q34 %.2f tour %.2f' % (q34c, tour))
    for o in out: print('   ', o)
