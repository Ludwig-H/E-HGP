# Quart LiDAR brut chaud : sensibilité des pentes de densité au tirage

23 septembre 2026. Le [premier reçu K5](../lidar_raw_physical_scaling_20260923/README.md)
et la [matrice K10](../lidar_raw_k10_sectors_20260923/README.md) signalaient
une pente finie proche ou au-dessus de 2 pour les formes du cœur dans
le quart physique `x≥0,y<0` de SemanticKITTI 08/000000. Ce reçu ajoute
**deux autres graines distinctes de décimation globale**, à K5 et K10,
sur ce même quart. La coupe reste définie par les signes float32
originaux du capteur ; les calculs HGP reprennent les octets u18/1 mm
de la trame entière, sans recadrage du morceau ni hypothèse
d'alignement entre nuages.

Pour chaque graine, `splitmix64(ID_brut XOR graine)` classe les **123 389
retours de la scène entière**. Les premiers `floor(n/4)` puis
`floor(n/2)` IDs forment deux sous-nuages emboîtés, ensuite
intersectés avec le quart physique. Les effectifs du quart changent
donc légèrement avec la graine. Le script vérifie par SHA les points,
IDs et secteur sources, puis reproduit **octet pour octet** les deux
entrées de la graine historique avant d'accepter les nouvelles.
Les huit nouvelles sondes utilisent le même binaire v12 épinglé
`e1ba126f…ea80`, K5/K10, s8/W8/static8, six leviers ON, `nice 19`
sur CPU local partagé. Les six sorties historiques du même quart sont
relues avec leurs SHA, leur FNV d'entrée, statut, options, dix/cinq
ordres et identité des formes. Pour chaque graine et densité, les cinq
agrégats publiés par ordre (`nodes`, `births`, `merges`, `parents`,
`contributions`) coïncident entre K5 et les ordres K1..5 de K10 ; ce
contrôle ne compare pas leurs clés ni leur topologie. Aucune sortie n'est FULL
certifiée : statut `complete_relative`.

`p` vaut `log(W_b/W_a)/log(n_b/n_a)` avec **les effectifs réels du quart**.
`core_sites` est le nombre de formes calculées et écrites au chargement
du cœur, **extrémités comprises** ; `dead_core_form_sites` est le
sous-total hors extrémités. La colonne `p_core` porte sur la première
quantité, le coût physique pertinent ici.

| K | graine | sites 1/4 → 1/2 → plein | formes du cœur (M) | `p_core` 1/4→1/2 / 1/2→plein |
| ---: | --- | ---: | ---: | ---: |
| 5 | historique `d1da…` | 7 692 → 15 619 → 31 391 | 3,549 → 14,655 → 52,302 | **2,002** / 1,823 |
| 5 | `s1` | 7 755 → 15 612 → 31 391 | 4,058 → 15,961 → 52,302 | 1,957 / 1,699 |
| 5 | `s2` | 7 755 → 15 580 → 31 391 | 3,990 → 14,600 → 52,302 | 1,860 / 1,821 |
| 10 | historique `d1da…` | 7 692 → 15 619 → 31 391 | 9,618 → 35,141 → 144,877 | 1,829 / **2,029** |
| 10 | `s1` | 7 755 → 15 612 → 31 391 | 9,607 → 36,578 → 144,877 | 1,911 / 1,971 |
| 10 | `s2` | 7 755 → 15 580 → 31 391 | 9,331 → 33,391 → 144,877 | 1,827 / **2,095** |

Le seuil 2 **n'est pas stable à la graine** : le premier lien K5 le
franchit de peu dans la graine historique seulement ; le dernier lien
K10 le franchit dans deux graines sur trois. En revanche, les formes
du K10 restent très sensibles à la population géométrique retenue au
demi-niveau : 33,391–36,578 M pour des tailles proches de 15,6 k,
face aux mêmes 144,877 M du quart plein. Le sous-total sans extrémités
fait franchir 2 au premier lien K5 de `s1` (2,007) alors que le total
calculé reste à 1,957 : le choix du compteur change la conclusion
locale. Les paires développées gardent des pentes 1,372–1,724 sur les
douze liens, et les CPU·s de chaîne 1,171–1,341 ; ces durées sur hôte
partagé ne bornent pas le travail des formes ni le temps G4.

Trois graines, **un seul quart d'une seule trame** et deux doublements
finis ne permettent ni test statistique de la classe LiDAR, ni borne
asymptotique, ni transfert à la trame entière. La décimation par hash
ne simule pas un autre capteur ou des passages superposés. Le constat
utile est plus précis : le risque de masse par cœur survit à un
changement de graine K10, mais l'exposant local varie assez pour
interdire un verdict fondé sur une seule réalisation. Répéter sur
plusieurs trames/séquences et sur le sans-sol, puis mesurer le coût
complet d'un certificat avant `load` avec un repli exact.

Le [manifeste](MANIFEST.json), les [huit cas](CASES.jsonl), leurs
stdout/stderr, les [pentes calculées](SUMMARY.json) et les
[empreintes](SHA256SUMS) forment le reçu. Chaque cas vérifie binaire
et entrée avant/après processus ; le lecteur reconstruit les entrées
et refuse les divergences de statut, FNV, options, cohérence interne des
comptes du catalogue, ordres,
comptes et SHA. Le temps mur des deux premières sondes nouvelles est
fortement perturbé par la contention locale (par exemple 13,77 s pour
7,63 CPU·s sur `s1_quarter_k5`) ; ne pas l'utiliser comme pente.

Depuis la racine du dépôt, régénérer d'abord les entrées sources depuis
les reçus v8 versionnés, puis relire sans relancer le calcul HGP :

```sh
python3 -B morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py \
  --repo "$PWD" --out /tmp/mhgp9-hot-multiseed-base
python3 -B morsehgp3D_v9/audits/lidar_raw_hot_quarter_multiseed_20260923/run_and_check.py \
  verify --base /tmp/mhgp9-hot-multiseed-base \
  --inputs-out /tmp/mhgp9-hot-multiseed-inputs
(cd morsehgp3D_v9/audits/lidar_raw_hot_quarter_multiseed_20260923 && sha256sum -c SHA256SUMS)
```

Le mode `run` avec les mêmes arguments rejoue les huit sondes absentes
seulement ; il exige le binaire épinglé. Le mode `verify` fonctionne
aussi sous `python3 -O` car ses refus n'utilisent pas `assert`.
