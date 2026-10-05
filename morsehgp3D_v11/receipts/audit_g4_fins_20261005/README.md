# Relecture des onze lots sanitizer fins clos — 5 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Les lots ASan/UBSan s’exécutent en u24, les lots TSan en u21.

Session `v11.20261005.claudefins`, source publiée
`38b76701b9b0198fc1c37afe16e1480e638e513c`. DONE=3,
`failed_remote`, worker=1 ; arrêt ciblé certifié. Le reçu, le plan, le paquet
et l’archive sont hachés et vérifiés. Une archive Git sélective confirme
l’égalité exacte des 635 fichiers utiles du paquet avec la source figée.
Les données sont vérifiées côté worker ; aucune donnée n’est recopiée ici.

| Profil | Lots terminés | Sélection | PASS | Échecs explicites | Sans résultat |
|---|---:|---:|---:|---:|---:|
| ASan/UBSan u24 | 4 | 80 | 74 | 6 | 0 |
| TSan u21 | 7 | 80 | 80 | 0 | 0 |

Les inventaires sont uniques, les lots sont deux à deux disjoints dans chaque
profil, et les deux unions contiennent exactement les mêmes 80 noms.
Chaque nom possède un verdict CTest terminé. Aucun lot n’est coupé par le
budget. Le résumé conserve tous les noms sélectionnés, PASS, Failed et
missing, ainsi que les empreintes des inventaires, résultats et journaux.

Les six échecs ASan/UBSan portent uniquement sur `api_supports_route` :
8k/16k/32k et les trois trames sans sol ng00/ng01/ng02. La [contrelecture native des six journaux](supports_route/README.md)
établit des sondes conformes suivies du seul refus de leurs hashes u24
par les attendus u21. Aucun diagnostic sanitizer n’est trouvé dans
les 45 journaux runtime archivés examinés. Les statuts CTest restent
inchangés ; ces causes ne reclassent pas les six Failed en PASS. Aucun de ces échecs n’est un résultat
manquant ou une coupure de budget.

PASS dans les deux profils : les six portes plat S10, les six portes points
S9 et les six `num_roots_cost`, aux trois tailles et aux trois trames.
Les noms exacts sont conservés dans le résumé. Ces résultats qualifient leur
portée native d’échelle/LiDAR à la source figée ; ils ne remplacent pas les
différentiels Python longs dédiés.

Les jumelles −O ne sont pas sélectionnées par S. `head_unit_huge`,
`points_unit_sort_refusal` et `num_roots` courts sont également hors de ces
onze lots. Le complément B court est explicitement prévu après S dans la
chaîne finale ; aucun résultat B n’est transféré à cette capture.
La qualification globale reste ouverte et ASan/UBSan est non conforme sur
ses six portes échouées. Aucun contrat de temps de tour ou résultat GPU.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script contrôle fermeture, hashes, identité des sources, inventaires
uniques/disjoints et tous les verdicts terminés, puis compare le résultat au
résumé figé. Aucun build, test natif, réseau ou accès cloud. Il dépend des
archives locales de fins et du commit Git ; cette capsule n’est pas un reçu
autonome.

Aucune sortie binaire native, identité de compte ou octet LiDAR copié.
La capsule globale ne garde que les empreintes des journaux ; le complément
`supports_route` conserve six extraits de métadonnées, comptes et verdicts.
