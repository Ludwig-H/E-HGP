# Contrelecture B — G4 R10 et validation parallèle publiée après le reçu

23 septembre 2026. Reçu
[`g4_tower_r10_20260923`](../receipts/g4_tower_r10_20260923/README.md),
publié par `308ca2a1`. Audit indépendant, sans nouveau GCP. Cadre du
reçu : exploration v9 hors registre, `reference_cpu`, entrée entière
1 mm u18 **sans sol**, `not_claimed`.

## Portée et chaîne de preuve

La VM G4 SPOT a exécuté le paquet source **`33d51efd`**, pas le moteur
modifié par `308ca2a1` après la capture. Trois trames 08/000000,
000100, 000200, toutes de la **même séquence SemanticKITTI**, 39 885,
35 551 et 45 845 sites retenus ; séparation s8, 48 workers q3/q4 et
48 fils FULL. K1..5 et K1..10, deux répétitions entrelacées. Seul
`tower_overlap_static` varie ON/OFF ; tous les autres leviers sont ON.
Le GPU de la G4 n'a pas été utilisé, pas plus que les profils float32,
trame brute avec sol, s10/s12 ou croissance 8k/16k/32k dans ce reçu.

Hôte et VM déclarent `completed`, 24/24 cas `complete_relative`, Euler
`holds`, 18 comparaisons logiques égales et arrêt ciblé certifié. Les
**388/388** fichiers de `SHA256SUMS` passent. Empreintes de clôture :
`SUMMARY.json` `c191e0d56809a7e2069873654737e0d0b37fd70a34279fd814f0fffe49fac617`,
`SHA256SUMS` `d41d38a643a6d09fb2c6d492298a9db88790cd94f88c31759b3ff1ca0086420a`.
Le relecteur de réception épinglé à `33d51efd` et le lecteur corrigé
`c19e4b49` rendent tous deux `completed`, chacun en Python normal et
`-O`, à partir des sorties brutes. Ce sont des juges du même protocole,
pas des oracles indépendants de toutes les clés possibles. Les 18
comparaisons portent sur la projection `logical_result` (entrée,
cardinalités du catalogue, Euler, ordres, digest FNV64), **pas** sur
une comparaison champ par champ du payload complet ni de toutes les
`BallKey`.

## Résultats appariés

Les deux chiffres par case sont les répétitions 0/1 de `chain_s` avec
recouvrement ON. La preuve causale du levier est la paire ON/OFF
**dans R10**, pas la différence R9→R10 entre campagnes.

| Trame 08/ | K1..5 ON (s) | K1..10 ON (s) | Gain tour ON/OFF K10 (s) |
| --- | ---: | ---: | ---: |
| 000100 | 2,753 / **2,736** | 8,185 / **8,066** | 0,461 / 0,461 |
| 000000 | 3,652 / **3,640** | **11,134** / 11,300 | 0,761 / 0,620 |
| 000200 | 3,963 / **3,940** | **11,360** / 11,496 | 0,726 / 0,592 |

La tour K5 gagne 0,049–0,151 s selon les cas ; le README du reçu dit
0,05–0,12 s et manque donc le cas 000200/répétition 1
(0,948→0,797 s). La tour K10 gagne 0,461–0,761 s. Le champ `lots`
ON mesure la **queue non recouverte** de la fenêtre des lots après la
phase statique, pas une diminution de leur travail total. La phase
statique peut ralentir pendant ce recouvrement : le maximum observé
est **9,94 %** (000100/K5/répétition 1, 200,392→220,305 ms), et non
« au plus 8 % » comme l'indique le README du reçu.

Meilleur K5, 000100/répétition 1 : `chain_s=2,736`, q3/q4 `1,653`,
tour `0,658`, q2 `0,231`, census `0,084` s. Le résidu
`chain_s−q34_s=1,083 s` : sur **ce chemin mesuré**, accélérer seulement
q3/q4 ne suffit pas à passer sous une seconde, même en l'annulant
idéalement. q3/q4 consomme 77,592 CPU·s sur 1,653 s de mur, soit
environ **98 %** des 48 CPU disponibles : le gros déséquilibre de R8
est déjà traité par R9, l'ordonnancement seul a peu de marge ici.
Meilleur K10 : q3/q4 `4,492`, tour `2,455`, chaîne `8,066` s. Le mur
externe est respectivement **2,971 s** et **9,049 s** pour ces meilleurs
cas ; il inclut lecture et digest séparé (0,163 et 0,800 s), à ne pas
confondre avec `chain_s`. Ce bilan interdit de déclarer le contrat
1 s ou 100 ms acquis ; il ne borne pas une future architecture GPU.

Euler ne juge que K1..3 quand Kmax=5, K1..8 quand Kmax=10. La tour
reste exacte **relativement aux clés émises**, sans preuve absolue de
catalogue complet sur ces trames. Ni croissance sous-quadratique du
pipeline ni qualification multi-séquence/brute ne découlent de R10.

## Lecteur K1 et code postérieur à la mesure

Le lecteur `33d51efd` pouvait faussement refuser un lot K1 qui recouvre
toutes les phases statiques ; ses 24 sorties R10 ne déclenchent pas ce
cas. `c19e4b49` juge K1 par `validate + max(static, lots_K1) + aval`,
et K≥2 par les phases 0 antérieures à leur lot. Une mutation du vrai
probe R10 avec `lots_K1=static` est désormais acceptée ; l'ancien
lecteur la refusait, et le mutant impossible K5 reste refusé. La porte
positive est dans `tests/chain/probe_worker_contract.py`. Il reste à
qualifier le recouvrement par comparaison **directe** de payload ON/OFF,
stress TSan et injection propre à `runners.emplace_back` ; aucun défaut
de sortie ni race n'est ici démontré.

Le même commit `308ca2a1` parallélise ensuite la passe 2 de validation
du catalogue et la construction des programmes
([code](../src/tower/forest/full_ball_tower.hpp), vers 1113–1220).
Sur succès, les préfixes par segments conservent l'ordre logique de
`by_level` et écrivent dans des plages disjointes ; aucune course ni
erreur géométrique n'a été trouvée par lecture. Mais cette passe ne
figure **pas** dans le paquet R10 : lui attribuer une part du gain est
interdit, et son coût propre reste à mesurer. Elle change en outre la
comptabilité des refus : `st.records` n'est ajouté qu'après succès de
toute la passe 2 (vers 1162), alors qu'il comptait auparavant chaque
boule validée. Un refus tardif publie donc `records=0` malgré les
enregistrements antérieurs et le travail payé. Les blocs parallèles
continuent de parcourir/allouer après un échec local déjà repéré ; une
exception de ressource ultérieure peut masquer le refus sémantique.
Les compteurs `u8`, le plafond K≤10 et le test de rang empêchent les
indices hors bornes envisagés pour une arité malformée ; aucun crash
n'est revendiqué. Ajouter une porte de refus/ledger et ignorer les
indices ≥ au premier échec connu avant de qualifier ce port.
