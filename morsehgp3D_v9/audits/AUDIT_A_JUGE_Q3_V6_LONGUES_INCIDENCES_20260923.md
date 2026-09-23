# Juge q3 v6 : rendre causale la porte des longues incidences

23 septembre 2026. Audit A du commit publié `abf3c3827`, puis du
[reçu v6](c_omission_20260923/README.md) publié par `88f30372` ; GCP
non utilisé. La recette v6 corrige utilement les substitutions `git`
masquées du lanceur v5, exige un répertoire neuf et des marqueurs pour les
mutants. Les deux limites ci-dessous portent sur la **portée des portes**,
pas sur une omission observée du générateur HGP.

Le reçu est positif dans sa portée : `STATUS=0`, tous les 205 182
incidences q2 et 286 706 incidences q3 échantillonnées présentes dans la
campagne v5, puis deux mutants `drop-long` tués avec désaccord causal dans
les portes v6. Les deux observations `obs` ont réellement rendu le code 0 ;
le défaut de traitement du code 2 ci-dessous ne les a donc pas contaminées.
Mais le cas isolé s02 passe `--min-long=50` avec **59** incidences longues
tous rangs confondus et seulement **13** à `p=Kmax−2` (`top_ge1600`) ; s00
en compte **308** et **64**. Les désaccords des mutants ne publient ni
rang ni arité. Ce reçu confirme que la porte actuelle est utile sans
établir une détection causale de la seule famille q3 longue au rang
critique. Le juge reste échantillonné et `run_tower=false`.

## Le plancher long peut manquer le rang critique

Dans [`q3_sample_judge.cpp`](c_omission_20260923/q3_sample_judge.cpp),
`pmax=Kmax−2` (ligne 387),
`by_len[2]` compte toutes les présentations admissibles de diamètre
maximal au moins 1 600 unités de grille (ligne 454), tandis que
`by_len_top[2]` ne compte que celles avec `p=pmax` (ligne 455).
`--min-long` refuse pourtant seulement si `by_len[2]` est trop petit
(ligne 628). La [recette v6](c_omission_20260923/run_judges_v6_gates.sh)
demande 50 incidences
longues dans chacun de ses deux cas isolés, mais peut donc accepter
`len_ge1600≥50` et `top_ge1600=0`. Son mutant `drop-long` retire les
partenaires longs **avant** le census (ligne 398) ; un désaccord à un
rang inférieur suffit à lui donner le code 1 et le marqueur attendus.
Il ne cible même pas exactement la même notion de longueur : le triangle
aigu entier `a=(0,0,0)`, `b=(1500,0,0)`, `c=(100,1500,0)` a `|ab|<1600`
et `|ac|<1600`, mais `|bc|>1600`. Il entre dans `by_len[2]` depuis
l'ancre `a` sans que `drop-long` retire `b` ou `c`.
Cette porte n'établit donc pas qu'elle détecte une omission des boules
q3 longues à `p=Kmax−2`, l'angle mort FULL visé.

Même un plancher sur `by_len_top[2]` serait un premier progrès, mais ce
compteur inclut aussi les coquilles étendues ou des boules de support
minimal q2. Pour viser la famille régulière, compter séparément les
présentations avec `p=pmax`, `shell.size()==3`, sans paire antipodale
(`q_min=3`), et longueur maximale au moins 1 600. Exiger un plancher
positif de cette strate **dans le parcours brut** et un désaccord de
`--compare` dans la même strate sous un mutant ciblé. Le seuil de 50 ne
doit être conservé que si les données archivées le satisfont ; sinon
publier la vraie couverture au lieu de transformer une vacuité en succès.

## `obs` peut absorber un échec du juge

Dans `run_judges_v6_gates.sh`, `run()` cherche seulement la sous-chaîne
` kmax=` puis retourne aussitôt si l'attendu vaut `obs` (lignes 27–28),
sans examiner le code du processus. Or le juge q3 imprime
`kmax=... chain_status=...` puis rend **2** quand la chaîne est incomplète
(`q3_sample_judge.cpp:354–360`). Les deux essais
`q3_lidar_*_overprune_isolated` sont `obs` : ils peuvent donc être
archivés comme observations valides avec `STATUS=0` malgré ce refus.

Reproduction isolée de la fonction `run()` publiée, avec une fausse
commande qui écrit ` kmax=10 chain_status=partial` puis rend 2 :
`RUN_RC=0 FAIL=0 STATUS_LINE="exit=2 expected=obs"`. Cette reproduction
ne lance ni HGP ni CUDA ; elle prouve le chemin du lanceur. Faire refuser
`c≥2` **avant** la branche `obs`, et exiger la vraie ligne de synthèse
` n=... kmax=...` pour les observations. Ajouter au `--selftest` le faux
juge de code 2 qui écrit une ligne `chain_status`, et vérifier un statut
global non nul ainsi que la présence du journal de refus. Les gates
strictes 0/1 et les marqueurs causaux restent utiles tels quels.

La suite est une porte qui affiche par cas les comptes longs de la strate
régulière au rang critique, les désaccords mutants de cette **même** strate,
les codes et les empreintes des sorties. Même réussie, elle ne change pas
le fait que le juge est échantillonné et `run_tower=false` : il ne certifie
ni la tour FULL ni la
complétude globale.
