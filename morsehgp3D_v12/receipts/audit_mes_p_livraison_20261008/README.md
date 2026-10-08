# MES-P : admission livrée — 8 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`public_status=not_claimed`. Livraison `5e5d5eb9a` : pilote `a5ca62fc…`,
analyseur `7514ace7…`, porte officielle inchangée `c8e5c4bc…`.
Les deux fichiers produit sont **identiques octet pour octet** à la proposition
publiée `../audit_reponses_20261007/mes_p_admission/`.

Les 28 contre-témoins de ce reçu, réexécutés sur ces mêmes octets, retrouvent
exactement leurs résultats : deux positifs admis, 26 négatifs refusés. La
porte officielle rend cinq cas et zéro écart, dont l'expiration à une seconde
du double Python/shell et l'arrêt de son groupe de processus. Le contre-témoin
de l'analyseur `code=0`, `chaud=null`, admission invalide donne bien **une prise,
zéro rendue, un échec**, avec le motif de protocole malgré un `exit ok/none`.
La cohorte commune à 1/4/48 fils est conservée. Rejeux normal et `-O` identiques.

La session G publie 54 JSONL et 318 agrégats. L'annonce de livraison ne nomme
pas ses vingt fichiers : nous déclarons donc notre sélection, les vingt
premiers noms de JSONL dans l'ordre lexicographique, soit `clusters8` et
`lattice`, cinq tailles chacune, K5/K10 à 48 fils, quatre passes demandées.
Les noms et hashes sont dans `capture.json` ; aucune nouvelle fixture n'est copiée.

Ces octets historiques, restitués par un exécutable Python, donnent **19
admissions**, avec les mêmes valeurs chaudes que l'archive, et **un refus** :
`synth_lattice_n10000_k10_f48.jsonl`, historiquement expiré, ne contient que
`cloud` et deux passes. Le double sort volontairement avec code 0 : le flux
reste refusé par le protocole, sans valeur chaude. L'agrégat historique avait
conservé à tort une valeur chaude malgré son code `expire` ; il reste une
archive inchangée. Aucun nouveau délai natif ou chrono HGP n'est simulé.

La correction d'admission MES-P de CST-0018 est donc livrée et contre-éprouvée
dans cette portée. Elle ne ferme pas les autres juges regroupés sous ce
constat, ne requalifie pas les 298 autres prises G ni les bruts absents de H,
et ne constitue pas une preuve géométrique, de latence v12 ou GPU.

```sh
python3 -B -S check.py --repo DEPOT
python3 -O -B -S check.py --repo DEPOT
```

Le lecteur extrait les objets Git épinglés, réutilise la contre-preuve publiée
sans la modifier, puis rejoue les vingt flux. Les sorties égalent le champ
`result` de `capture.json`. Aucune compilation, exécution HGP ou action GCP.
