# Contre-audit B — proposition de portes q2/q3 R-20

23 septembre 2026. Base `b89d0adc0` :
[`judges_product_gates.patch`](c_omission_20260923/judges_product_gates.patch),
[`PROVENANCE.txt`](c_omission_20260923/results/judges_product_v1/PROVENANCE.txt)
et reçus CTest. Lecture du patch, **sans** le porter dans le moteur ni
relancer ses 202 secondes de tests. Aucun GCP utilisé.

## Verdict et ce qui marche

Le patch proposé, non adopté au contrôle, ajoute deux binaires de juge
CPU et 34 CTests. Le reçu local rapporte **34/34** : 21 portes `gate`
(quatre exécutions positives, 17 mutants/arguments invalides) et 13
`scale8000` (12 familles uniform/terrain/amas à K5/K10 et une
comparaison q3 brut/élagué). Les mutations de clé, niveau, coquille,
élagage et index ont des codes et marqueurs causaux : c'est une bonne
**porte de régression échantillonnée**. Le mode `--compare` teste
réellement le désaccord entre le parcours q3 élagué et le brut, mais
les deux partagent construction de sphère et census ; une erreur dans
ces briques communes peut passer.

## Pourquoi cela ne ferme pas la complétude FULL

1. À 8 000 sites, chaque test q2 ne choisit que **50 ancres** et
   chaque test q3 **20** ; les sens direct et inverse inspectent les
   boules incidentes à ces ancres. Une boule régulière omise dont aucun
   site de coquille n'est choisi est invisible aux deux sens. La
   mutation d'anti-vacuité retire une **autre** clé trouvée par le juge ;
   elle ne couvre pas les clés jamais rencontrées. À titre d'échelle,
   pour un tirage uniforme aléatoire de `S` sites parmi `n=8000`, la
   probabilité de toucher la coquille de taille `q` d'une omission fixée
   vaut `1−C(n−q,S)/C(n,S)` : **1,246 %** pour q2 (`S=50,q=2`),
   **0,748 %** pour q3 (`S=20,q=3`). Les graines de ces CTests sont
   déterministes ; ces pourcentages ne sont **pas** des probabilités
   mesurées du pipeline. Pour une coquille q3 étendue, rencontrer un
   site de coquille peut encore ne pas donner un triangle aigu
   représentant la boule depuis cette ancre. La fixture adverse
   `verification_juge_q3.json` du dossier C illustre cette limite.
2. Tous ces appels ont `run_tower=false` : aucun n'exerce la
   reconstruction FULL. Les 13 cas d'échelle sont synthétiques ; aucun
   CTest `file` ne lit une coupe LiDAR ni une trame entière, et les
   familles utilisées ici restent sous 65 536 coordonnées par axe
   (`front_fixtures.hpp`). Le chemin u18 haut, les contacts de la
   grille LiDAR 1 mm et les coquilles étendues ne sont pas qualifiés par
   ces 34 cas. Les 34 sorties CTest archivées donnent succès/durée,
   non les compteurs individuels `top_keys/top_population`, CRL et
   `extended` que le juge pourrait publier. `--min-top=100` et
   `--min-crl=1` prouvent une non-vacuité minimale, pas un taux de
   couverture des strates critiques.
3. `check_index()` valide un **nouvel** `tower::CloudIndex` reconstruit
   dans le juge depuis les points. Les six mutants `INDEX_*` corrompent
   sa copie locale : ils prouvent sa propre garde avant échantillonnage,
   pas la fidélité de l'index privé effectivement consommé par
   `run_tower_chain`. Ce n'est pas un défaut introduit dans le moteur,
   seulement une limite à l'interprétation de cette porte.

**Usage proposé.** Porter ces tests en CI sous le libellé « régression
CPU du catalogue, échantillonnée », si le coût `scale8000` convient ;
conserver leur reçu et les compteurs de chaque exécution. Ajouter au
minimum une fixture u18 haute, une dégénérescence à coquille étendue,
et des coupes 1 mm LiDAR régénérées avec empreintes ; un différentiel
de catalogue GPU/CPU complet sur les chemins ciblés reste distinct et
ne prouve pas à lui seul l'absence d'une omission commune. Aucun
échantillonnage fini de petites ancres ne ferme mathématiquement le
contrat d'exactitude global q2/q3/q4/FULL. Ne pas convertir 34/34 en
qualification du contrat G4 ou de la sous-quadraticité.
