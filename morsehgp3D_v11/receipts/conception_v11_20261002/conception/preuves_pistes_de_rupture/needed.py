# Compte, dans un dump de catalogue (rang q p u flags | S* | I | U), les boules dont la fenetre contient K :
# p + q - 1 <= K <= p + u (u = taille de la coquille). Empreinte de l'ensemble de ces lignes (ordre du dump).
import sys, hashlib
K = int(sys.argv[2])
n = 0; tot = 0; h = hashlib.sha256()
for line in open(sys.argv[1], 'rb'):
    tot += 1
    f = line.split(None, 5)
    q, p, u = int(f[1]), int(f[2]), int(f[3])
    if p + q - 1 <= K <= p + u:
        n += 1
        h.update(line.split(b'|', 1)[1])   # sans le rang (qui depend de l'ensemble emis)
print(sys.argv[1], 'boules', tot, 'necessaires a l ordre', K, ':', n, 'empreinte', h.hexdigest()[:16])
