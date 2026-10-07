# Heredite des supports dans le catalogue v10 (dump : rang q p u flags | S* | I | U).
# Pour chaque boule q3 (resp. q4) emise : ses aretes sont-elles des boules q2 du MEME catalogue (p <= K - 1) ?
# ses faces sont-elles des boules q3 du catalogue ? Mesure la completude d'une generation "depuis les q2 voisines".
import sys, itertools
K = int(sys.argv[2])
q2 = set(); q3 = set(); balls3 = []; balls4 = []
for line in open(sys.argv[1], 'rb'):
    head, sup, rest = line.split(b'|', 2)
    f = head.split()
    q, fl = int(f[1]), int(f[4])
    s = tuple(sorted(sup.split()))
    if q == 2: q2.add(s)
    elif q == 3:
        q3.add(s); balls3.append(s)
    else:
        balls4.append(s)
def stats(balls, name):
    n = len(balls); all_e = 0; miss_e = 0; some = 0; all_f = 0
    for s in balls:
        m = sum(1 for e in itertools.combinations(s, 2) if e not in q2)
        miss_e += m
        if m == 0: all_e += 1
        if len(s) == 4:
            if all(t in q3 for t in itertools.combinations(s, 3)): all_f += 1
    ne = 3 if len(balls[0]) == 3 else 6
    print("K=%d %s : %d boules ; toutes les aretes au catalogue q2 : %.1f %% ; aretes absentes : %.1f %% des aretes" % (K, name, n, 100.0 * all_e / n, 100.0 * miss_e / (ne * n)), end='')
    if len(balls[0]) == 4: print(" ; quatre faces au catalogue q3 : %.1f %%" % (100.0 * all_f / n))
    else: print()
stats(balls3, 'q3'); stats(balls4, 'q4')
