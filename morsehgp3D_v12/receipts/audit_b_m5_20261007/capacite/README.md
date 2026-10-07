# Capacité M5 : compte de tâches avant le port

7 octobre 2026, pin `e30000dec1027c5f0ade3093a94563ee412409d3`. **CST-0222**,
défaut de borne, gravité mineure avant port. Aucune entrée des campagnes LiDAR
n'est concernée par le témoin ; il ne contient que sept petits enregistrements.

`EmitKernel`, `include/mhgp12/traversal/bfs.hpp`, calcule
`q.tasks = (o.count + kChunk - 1) / kChunk` avec `o.count` et `kChunk` en u32.
Pour `count` allant de **2^32−255 à 2^32−2**, domaine de compte autorisé par le
lecteur (`n_sites < 0xffffffff`), l'addition déborde modulo 2^32 et écrit zéro.
La préparation `child_fields` élargit déjà en u64 avant son addition et écrit
correctement 33 554 432 tâches pour les deux enfants ; les deux étapes divergent.

Le vrai noyau commun est compilé en Release, sans modification, puis appelé sur
un parent et un enfant synthétiques. Aucun tableau de milliards de sites n'est
alloué ; le noyau d'émission ne lit pas les coordonnées.

| Sites retenus | Tâches exactes par enfant | Tâches émises |
| ---: | ---: | ---: |
| 1 / 256 / 257 | 1 / 1 / 2 | 1 / 1 / 2 |
| 4 294 967 039 / 4 294 967 040 | 16 777 215 | 16 777 215 |
| 4 294 967 041 / 4 294 967 294 | 16 777 216 | **0** |

`locate` divise ensuite par `Parent.tasks`. Le témoin ne lance pas cette division
ni un parcours géant : il établit la mauvaise valeur émise, pas un crash observé
sur un nuage complet. Les allocations physiques pourraient refuser beaucoup plus
tôt ; elles ne constituent pas une borne déclarée de ce champ.

Correction : élargir **avant** l'addition, ou employer `count/256 + (count%256 != 0)`,
puis contrôler la conversion. Le même calcul doit servir au scan et à l'émission.
Les sept cas donnent une porte bornée, indépendante de la mémoire disponible.

Observation de lecture liée au futur budget (`CST-0211`) : `driver.hpp` teste
`t.f[2] > UINT32_MAX` et `t.f[0] > INT32_MAX` après réservation des tableaux du
niveau suivant et après `Scatter/Emit`. Le port doit déplacer l'admission avant
ces actions et les conversions ; aucun second défaut d'exécution n'est reproduit
ici à partir de ces totaux gigantesques. Le microbanc déclare son budget Session
encore à faire, et cette observation ne prétend pas l'avoir qualifié.

```sh
python3 -B morsehgp3D_v12/receipts/audit_b_m5_20261007/capacite/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_b_m5_20261007/capacite/check.py
```

`result.json` conserve les résultats, le compilateur et le hash du binaire temporaire.
Le lecteur exige sept sources au pin et les contrôle avant/après avec ses propres
témoins. Les sorties normal/`-O` sont identiques. Aucun GCP, GPU, sanitizer ou build
de moteur ; le binaire temporaire est supprimé à la fin.
