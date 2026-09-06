# Diagnostic rationnel local des coquilles supplémentaires

Préparation privée, `diagnostic_only=true`, `public_status=not_claimed`.
Aucun code produit ou oracle importé ; aucun moteur C++, GCP ou Git exécuté.
Les traces réelles de 50 000 points restent à lire : les succès ci-dessous ne les qualifient pas.

`read_extra_shell.py` lit exactement le schéma `mhgp7-extra-shell-diagnostic-v1`.
Il rejette clés JSON dupliquées, nombres flottants/non finis, inventaires incorrects,
identités ou coordonnées incohérentes, clés non primitives, rayons incorrects et
listes I/U incomplètes. La clé est le polynôme A|x|²+B·x+C avec A>0 et PGCD=1 ;
le rayon comparé rationnellement vaut (B·B−4AC)/(4A²). Le format de niveau peut
être non réduit. La reconstruction Gram rationnelle énumère tous les supports
affinement indépendants à poids strictement positifs, de cardinal 2 à 4 ;
leur cardinal minimal doit égaler `minimal_arity`.

L'entrée complète est reconstruite indépendamment : MT19937 graine 3,
coordonnées u16 obtenues par multiply-high de la distribution libstdc++ de la
campagne, dédoublonnage et identités dans l'ordre d'acceptation. Le digest FULL
de toute l'entrée doit être identique ; le rang géométrique est recalculé par
entrelacement Morton. Chaque clé fournie est ensuite confrontée, en entiers
Python, à **tous** les points. Aucun catalogue exhaustif de boules n'est produit.
La concordance de recette est déjà testée sur les digests historiques n=8 et
n=8 000 ; la liaison au digest de n=50 000 sera exigée lors de la lecture réelle.

Pour une coquille d'au plus 12 points, la table entière des sous-ensembles dit
si le centre appartient à leur enveloppe convexe. Pour chaque K demandé, le
lecteur donne les composantes strictes locales, leurs représentants et leurs
couvertures, puis le bloc fermé et les facettes nouvellement présentes.
La réduction absorbe tous les points strictement intérieurs lorsque K>|I| ;
pour K≤|I| le graphe strict local est connexe. Une coquille plus large est
refusée explicitement, jamais tronquée. Le critère d'inertie suffisant est
K≤|I|+q_min−2. Les points absents des couvertures strictes locales ne sont pas
annoncés comme des points nouveaux globalement : des chemins extérieurs
peuvent déjà relier ces composantes. **Aucun parent global n'est reconstruit.**

La réduction mathématique est celle décrite dans
`morsehgp3D_v7/audits/receipts_plateaux_full_20260906/README.md`, sans importation
de son code. Le lecteur ne certifie ni que toutes les boules pertinentes ont
été fournies ni que leur traitement suffit à produire toute la forêt FULL.

```bash
python3 -B build/v7_extra_shell_20260906/selftest_reader.py
python3 -B -O build/v7_extra_shell_20260906/selftest_reader.py
python3 -B build/v7_extra_shell_20260906/read_extra_shell.py TRACE.jsonl --expected-n 50000
```

`--mixed-stderr` autorise explicitement l'omission des lignes non JSON d'un
stderr mixte. Les lignes JSON restent strictes ; utiliser un fichier par
commande, car les indices de boules doivent être uniques dans le fichier.

Les deux selftests purs finaux ont rendu 0 et des sorties identiques : quatre
géométries (q_min=2,3,4), 80 masques confrontés à une faisabilité affine
rectangulaire indépendante, 165 facettes strictes en graphes locaux exhaustifs,
34 mutants rejetés, un rayon non réduit accepté. `model_checks.json` conserve
les commandes et résultats. Le premier passage avait un décompte annoncé de
34 mutants pour 33 effectivement présents ; cet échec est conservé dans
`development_count_failure.json`. Un rejet explicite de PointId booléen a été
ajouté avant les deux passages finaux. Aucun résultat réel n'a été réécrit.
