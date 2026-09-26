# E-HGP — la tour HGP en grande dimension, par régularisation entropique

> [!IMPORTANT]
> Cadre à annoncer au début de toute tâche E-HGP :
>
> ```text
> phase=exploration_ehgp_hors_registre
> backend=python_reference
> profile=any_dimension_rational_exact
> mode=audit_independant_math_and_architecture
> public_status=not_claimed
> ```
>
> Ce chantier ne touche ni le registre `docs/implementation_status.toml`, ni le
> contrat public `schemas/morsehgp3d-contract-v2.schema.json` (verrouillé à
> $d=3$), ni la ligne enregistrée `morsehgp3d/`. Aucun statut public n'est
> revendiqué. GCP non utilisé.

## Ce que ce chantier cherche

MorseHGP3D (v2 à v9) calcule la tour ordre–échelle HGP pour des nuages de
$\mathbb{R}^3$. E-HGP pose la question du même objet **en grande dimension**
($d$ de 10 à 1000), avec un budget de calcul volontairement relâché —
quadratique, cubique toléré — et une piste précise : la **régularisation
entropique** et les **$f$-divergences**, en particulier les idées du billet de
Francis Bach sur l'estimation spectrale de la log-densité.

La réponse obtenue tient en trois phrases, et les deux premières sont
négatives :

1. **l'objet est libre en dimension, le chemin de calcul ne l'est pas** — le
   théorème 2 du manuscrit (les composantes de la région multicouverte sont
   celles du graphe $\Gamma_k$) n'utilise que la convexité des intersections
   de boules, donc vaut dans tout espace normé ;
2. **la tour FULL ne se transporte pas** : sa taille de sortie passe de
   $O(n)$ en dimension 2 à exactement $\binom{n}{k}$ en dimension 100, et en
   métrique euclidienne ambiante ses niveaux se concentrent au point de ne
   plus porter de signal de densité. Ce n'est pas un problème d'algorithme ;
3. **ce qui se transporte, c'est la méthode** : remplacer l'énumération
   combinatoire des sphères critiques par une **descente** (dont les points
   fixes sont exactement les points critiques de la spécification), et
   certifier chaque décision en rationnels exacts le long de segments. Cette
   voie reproduit la projection exacte de la tour sur les observations dans
   $99{,}5$ pour cent des paires mesurées, en toute dimension testée, et
   jamais en dessous.

## Ordre de lecture

1. [`docs/OBJET_ET_DIMENSION.md`](docs/OBJET_ET_DIMENSION.md) — l'objet, son
   audit énoncé par énoncé, le fait de segment, les trois objets que le
   chantier calcule et qu'il ne faut pas confondre.
2. [`docs/OBSTRUCTION_GRANDE_DIMENSION.md`](docs/OBSTRUCTION_GRANDE_DIMENSION.md)
   — l'obstruction de taille et l'obstruction de signal, mesurées par trois
   implémentations indépendantes, avec les deux fixtures permanentes qui
   corrigent le critère de naissance.
3. [`docs/REGULARISATION_ENTROPIQUE.md`](docs/REGULARISATION_ENTROPIQUE.md) —
   le niveau de Fermi, la correspondance $f$-divergence / forme de noyau,
   l'encadrement exact du noyau rampe, le pont avec la criticité de
   MorseHGP3D, et les trois énoncés réfutés en chemin.
4. [`docs/MOTEUR_ET_COUTS.md`](docs/MOTEUR_ET_COUTS.md) — l'architecture,
   les coûts, la couche statistique et sa limite mesurée.
5. [`docs/MESURES_CONCENTRATION_20260925.md`](docs/MESURES_CONCENTRATION_20260925.md)
   — la question décisive pour un usage réel : dimension intrinsèque contre
   dimension ambiante, seuil de bruit, et les trois prétraitements qu'il faut
   s'interdire.
6. [`docs/JOURNAL_20260925.md`](docs/JOURNAL_20260925.md) — le journal de la
   première journée, avec les commandes qui reproduisent chaque chiffre.
7. [`audits/QUESTION_AUDIT_OUVERTURE_20260925.md`](audits/QUESTION_AUDIT_OUVERTURE_20260925.md)
   — les sept verrous soumis à contradiction.

## Carte du code

```text
E-HGP/src/ehgp/
  exact/meb.py          boule englobante minimale exacte, dimension quelconque
  exact/tower.py        tour FULL exacte (oracle borné), digest canonique
  exact/projection.py   ultramétrique exacte projetée sur les observations
  exact/grid_judge.py   juge par grille de pi_0(L_k(a)), borné à d <= 3
  judge/gamma_bfs.py    seconde implémentation de pi_0(Gamma_k), arithmétique autre
  engine/segment.py     maximum exact de a_k sur un segment
  engine/critical.py    descente MEB-Lloyd : le catalogue critique sans énumération
  engine/point_tower.py tour projetée, segments entre observations
  engine/witness_tower.py  tour projetée avec témoins critiques
  soft/fermi.py         niveau de Fermi, familles logistique et rampe
  soft/families.py      correspondance entropie / noyau, familles f_rho
  soft/ramp_exact.py    comptage doux rampe en rationnels exacts
  spectral/             couche statistique (estimateur de log-densité)
E-HGP/bench/            sondes et portes à code de sortie exact
E-HGP/tests/            portes unittest (jamais pytest, jamais le mot-clé assert)
E-HGP/receipts/         reçus immuables, ancrés au commit
```

## Commandes

```bash
cd /workspaces/E-HGP/E-HGP
python3 -m unittest discover -s tests -p 'test_*.py'            # toutes les portes
python3 -O -m unittest discover -s tests -p 'test_*.py'         # doit tenir sous -O
python3 bench/births_vs_dimension.py --n 8 --k-max 4 --dims 2,3,5,10,20,50,100 --seeds 5,17,31
python3 bench/empty_ball_fraction.py --n 11 --ks 2,3,4,5 --dims 2,3,5,10,20,50,100 --seed 5
python3 bench/scale_space.py --n 11 --k 3 --dims 2,3,20 --seed 31
python3 bench/witness_campaign.py --ns 8 --dims 2,3,5,10,20,50 --k-max 3 --seeds 17,23,31 \
    --families uniform,clusters --triples --min-cases 30 --min-pairs 2000 \
    --out receipts/relecture/campagne.json      # 2856 paires sur 2856, 0 violation
python3 bench/gate_engine.py --n 7 --d 3 --k 2 --seed 3 --min-cases 5   # porte, code 0
python3 bench/gate_engine.py --n 7 --d 3 --k 2 --seed 3 --min-cases 5 --inject extremites  # code 4
```

Codes de sortie des portes, comme dans le reste du dépôt : `0` conforme,
`1` désaccord d'un juge, `2` refus avant calcul, `3` plancher ou invariant
violé, `4` mutant tué.

## Règles propres au chantier

* **Exactitude.** Toute décision publiée est rationnelle exacte
  (`fractions.Fraction`). Le flottant est un **proposeur** : il sert à la
  descente et aux recherches, jamais à une décision. Aucune borne, aucun
  accord numérique ne promeut un statut.
* **Trois objets, trois noms.** Tour FULL exacte (oracle borné, $n\leq14$),
  projection certifiée sur les observations (majorant, égalité mesurée), tour
  régularisée d'un modèle. Ne jamais présenter l'une pour l'autre.
* **Une contradiction devient une fixture.** Les fixtures F1 (boule fermée
  contre intérieur strict) et F2 (cosphéricité) sont permanentes ; toute
  nouvelle contradiction s'ajoute à
  [`docs/OBSTRUCTION_GRANDE_DIMENSION.md`](docs/OBSTRUCTION_GRANDE_DIMENSION.md)
  avant de continuer.
* **Planchers de couverture.** Chaque porte échoue si elle n'a pas
  effectivement comparé, visité ou tué un minimum explicite : le vert par
  vacuité est un échec.
* **Pas de branche.** Commits sur `main`, comme partout dans ce dépôt.
* **Licences.** `HGP-old/` (licence non commerciale) n'est jamais importé, ni
  copié, ni adapté ligne à ligne : E-HGP n'en reprend que des idées
  publiquement décrites dans le manuscrit.

## État au 25 septembre 2026

Acquis, avec la commande qui le reproduit :

| fait | statut |
| --- | --- |
| tour d'ordre 1 = dendrogramme de l'arbre couvrant minimal euclidien, exact, $d$ jusqu'à 20 | **mesuré** |
| tour exacte confrontée à un juge indépendant : 0 désaccord sur 4650 comparaisons, $d\in\lbrace2,3,5,20\rbrace$ | **mesuré** |
| machinerie `reference/morsehgp3d_oracle/` confrontée en dimension $d$ après neutralisation locale de ses deux verrous : 0 désaccord | **mesuré** |
| naissances topologiques : $O(n)$ en $d=2$, exactement $\binom{n}{k}$ en $d=100$ | **mesuré** |
| encadrement du noyau rampe $L_k(a-\varepsilon)\subseteq L_k^{\varepsilon}(a)\subseteq L_k(a)$, décalage optimal | **démontré** |
| points fixes de la descente MEB-Lloyd = sphères critiques, zéro faux positif | **démontré et mesuré** |
| tour à témoins = projection exacte sur **2856 paires sur 2856** (départs par triplets), 0 violation, $d$ de 2 à 50 | **mesuré** |
| couverture complète du catalogue critique par descente en grande dimension | **non acquis** |
| certificat de séparation exact (hyperplan et sphère), zéro faux | **démontré et mesuré** |
| niveau de fusion **certifié exact** par le moteur seul : 21 paires sur 21 à l'ordre 1 en $d=20$ | **démontré** |
| certificat de séparation efficace aux ordres $k\geq2$ | **non acquis** |
| obstruction gouvernée par la dimension **intrinsèque**, pas l'ambiante ($\varphi_5$ constante de $d=2$ à $200$ à rang 2 fixé) | **mesuré** |
| bruit ambiant sur une variété de rang 2 : exposant du compte de naissances $1{,}96$ (quadratique) | **mesuré** |
| blanchiment interdit : en rang $d\geq n-1$ la tour devient celle d'un simplexe régulier, indépendante des données | **démontré** |
| tour régularisée contre tour empirique sous bruit ambiant : ARI $0{,}994$ contre $0{,}000$ à $(d=50,k=5)$ | **mesuré** |
| la voie régularisée est moins bonne en $d=2$ ($-0{,}33$ d'ARI) : c'est un régime, pas un remplacement | **mesuré** |
| couche statistique utile à $d=200$ avec des descripteurs FIXES | **non acquis** ($0{,}362$ d'ARI) |
| l'axe d'ordre $k$ apporte quelque chose comme clusterer sur des distances euclidiennes brutes | **réfuté** : $-0{,}06$ à $-0{,}21$ d'ARI contre $k=1$, et la famille densitaire est dominée par Ward et $k$-moyennes |
