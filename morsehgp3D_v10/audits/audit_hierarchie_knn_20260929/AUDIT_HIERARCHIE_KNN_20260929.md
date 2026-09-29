# Audit : la hiérarchie de la tour FULL au regard des niveaux de densité K-NN (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
code audité : 8b8d66f6e (tour, tête, banc) ; GCP non utilisé pour l'audit ; graines dev seulement
```

## Question

Demande de l'utilisateur : « audite à fond si la hiérarchie produite à partir de la tour FULL est pertinente vis-à-vis
du framework statistique des niveaux de densité K-NN ». Elle est accompagnée de deux consignes : « compare avec le code
de HGP-old (qui est peut-être mauvais, mais donnait de bons résultats empiriques) », et « la tour est censée identifier
exactement les niveaux de densité K-NN ; si le mélange gaussien est séparable, on devrait les identifier ». Aucune
source n'est prise pour oracle : ni HGP-old, ni la thèse, ni Γ_K.

## Méthode

- **Trois lectures indépendantes** (annexes) :
  - [littérature statistique](annexes/LECTURE_LITTERATURE.md), avec références vérifiées ;
  - [code v10](annexes/LECTURE_CODE_V10.md) ;
  - [HGP-old et thèse](annexes/LECTURE_HGP_OLD_THESE.md), HGP-old lu comme une spécification, jamais exécuté ni recopié.
- **Quatre audits ciblés** (objet ; entrée et sélection ; écart HGP-old ; implémentation), chacun suivi d'une
  **contre-vérification adverse**. Leurs constats et verdicts sont en JSON dans `annexes/`.
- **Bilan : 52 constats**, dont 21 confirmés, 31 requalifiés (précisés ou nuancés) et aucun réfuté.
- **Expériences dev** de l'audit, toutes rejouables : reçus `bench_dev_z_samehead_20260929` (exposant z, même tête sur
  MR₁ et MR₂) et `bench_dev_objet_20260929` (même entrée et même tête ; plafond de Bayes des familles gaussiennes).

## 1. Réponses

### 1.1 L'objet est le bon, exactement calculé

- L_K(r) = {y : |B̄(y, r) ∩ X| ≥ K} = {d_K ≤ r} = {f̂_K ≥ K / (n v_d r^d)}. La tranche d'ordre K de la tour est donc
  exactement l'arbre plug-in de l'estimateur K-NN f̂_K.
- Ses ensembles et ses inclusions ne dépendent d'aucune convention de dimension ; seul le choix de la tête (z) agit
  sur les hauteurs.
- Identité vérifiée à K = 1 sur les 256 scènes dev : la tour, MR₁ et MR₂ (hiérarchies d'HDBSCAN à α = 1 et 2) donnent
  les mêmes partitions pour toutes les têtes. À K = 1, les trois sont la liaison simple.
- **Consistance.** La consistance de Hartigan découle de résultats publiés sous les conditions suffisantes
  K/ln n → ∞, K/n → 0 et f uniformément continue (Devroye–Wagner, lemme A.1 de Chaudhuri et al. 2014, Eldridge et al.
  2015). Ce corollaire est une dérivation, pas un théorème publié tel quel. Aucune borne à n fini n'est informative à
  nos K ≤ 10, qui sont inférieurs à 3 ln n ≈ 23–27.

### 1.2 Les modes d'un mélange séparable sont identifiés

Réponse à l'objection de l'utilisateur, mesurée par le plafond de Bayes (partition MAP du mélange) sur 128 scènes
gaussiennes dev (reçu `bench_dev_objet_20260929`) :

- **Aux niveaux faciles, la meilleure coupe de la hiérarchie atteint le plafond** : 0,982 contre 0,982 sur
  `spherical` à K = 10. La hiérarchie contient alors la partition optimale.
- **Aux niveaux difficiles, les modes restent des branches** de l'arbre, et l'EOM les retient. L'écart au plafond
  vient de la notion d'ensemble de niveau, pas de l'objet.
  - Pour séparer deux modes, il faut couper au-dessus du col, qui est à 14 % du pic sur `spherical` hard.
  - Une gaussienne 3D a environ 27 % de sa masse sous ce seuil. La partition de Bayes affecte ces points ; aucun
    ensemble de niveau ne les contient.
  - Il faut donc une règle d'affectation, et le remplissage borné en est une. Il ramène `spherical` et `anisotropic`
    à 0,02–0,05 du plafond jusqu'au niveau hard.
- « Exact » veut dire exact pour f̂_K, pas pour la vraie densité f.

### 1.3 L'exactitude n'a pas de valeur statistique mesurable ; la connexité, si

- **Exactitude.** Aucun avantage statistique n'est démontré sur les niveaux : le bruit d'échantillonnage (environ
  0,1 en ln r entre graines) écrase la précision double.
  - L'exactitude garde une valeur de définition, de reproductibilité et de correction combinatoire : pas d'erreur de
    structure silencieuse (séparabilité, centres, plateaux).
  - Une approximation certifiée (1 + ε) qui garde la connexité de Čech serait probablement admissible ; c'est à
    mesurer.
- **Connexité (loi de la demi-lacune).** À travers un vide de largeur g, la tour ne fusionne pas avant g/2, et MR_α
  pas avant g/α ; ce sont des bornes exactes.
  - En régime de lacune, la tour fusionne à 0,53–0,59 fois le niveau de MR₁ : elle se comporte comme MR₂.
  - D'où l'échec de l'EOM à petit z sur les coquilles.
- **À entrée cœur et tête EOM égales, MR₁ (HDBSCAN par défaut) bat la tour** : −0,01 à −0,12 selon K et z.

### 1.4 L'entrée des points

- **L'entrée `cover` (première couverture) est la sémantique des amas discrets du théorème 2.** C'est l'analogue, sur
  la multicouverture, de la règle des points-bord de DBSCAN :
  - {α_K ≤ r} = X ∩ (L_K(r) ⊕ B̄(r)) ;
  - f̂_K ≤ f_α ≤ 2^d f̂_K.
- **La tête ne lit jamais α_K(x).** En position générale, chaque point est porté par une naissance (sa première
  boule couvrante, au plus K sites, donc sous mcs), et il sort au λ de la fusion qui absorbe cette naissance.
  - Le choix de la composante couvrante est sans effet : |Δ| ≤ 0,003.
  - Le gain de `cover` sur `core` vient de la profondeur à laquelle la masse entre dans l'arbre : +0,03 à +0,08 avec
    b(1,5), +0,07 à +0,13 sans remplissage. Il décroît quand z croît.
- **À K fixe, `cover` et `core` visent deux objets différents** (une dilatation, contre l'ensemble de niveau aux
  données). Préférer `cover` est un choix de cible.
- **La même entrée se donne à la hiérarchie d'HDBSCAN** (MR-bord, `mhgp10_mreach_cluster --entry=border`).

### 1.5 À même entrée et même tête, la tour égale la hiérarchie d'HDBSCAN

- **Dev, 128 scènes de 8 000 points.** La tour et MR-bord (α = 1 ou 2) sont à 0,01 près dès z ≥ 4. Aux têtes du
  lot C, l'écart tour − MR₂-bord vaut de +0,001 à +0,008.
- **Audit, 32 scènes difficiles de 2 000 points.** À z = 3, la tour et MR₂-bord sont indiscernables en moyenne, sans
  qu'une équivalence soit établie. L'ordre dépend de z :
  - à ẑ et K = 10, la tour gagne de +0,095 ;
  - à z = 1 et K = 10, MR-bord gagne de +0,06 à +0,10.
- **L'ordre dépend aussi de la famille**, avec des écarts de 0,1 à 0,23 dans les deux sens. La tour évite les
  effondrements de MR₂-bord sur `shells` à K = 10.
- **Conséquence.** L'avantage mesuré de la tour sur HDBSCAN vient de l'entrée des amas discrets et de la tête, pas de
  l'objet exact. À K = 1, il vient entièrement de la tête. La famille « objet » du lot C le met à l'épreuve sur 960
  scènes de test, de 8 000 à 32 000 points.

### 1.6 La sélection est le point faible et le vrai levier

- **L'EOM n'est pas invariant par reparamétrisation de λ.** Aucun article lu ne fonde l'exposant de la stabilité.
  Avec λ = r^(−z), z est un cadran de granularité : quand z → 0, on obtient la persistance en log r ; quand z → ∞, les
  feuilles.
- **ẑ (Levina–Bickel global) n'est pas fondé comme échelle d'EOM.** Il cause seul la chute à K = 10, sur 16 scènes
  `shells` à 8 000 points, où l'EOM retient 3 parents au lieu de 8 coquilles.
- **z = 1 (l'échelle d'HDBSCAN) est le pire choix à tout K.**
- **L'optimum de z dépend de n** : z ≈ 3–4 à 2 000 points, z ≈ 6 à 8 000, surtout via `filaments`. Aucun z fixe ne
  vaut pour tous les K et toutes les tailles. Le lot C choisit donc z sur les scènes dev de 8 000 points.
- **mcs = √n est un a priori de taille** de type runt, sans justification statistique ; dans le plateau de z, il pèse
  plus que z.
- **Les fonctionnelles fondées ne battent pas l'EOM à z bien choisi** sur ce banc : EOM de Lebesgue, limite log, EOM
  bornée de Moulavi, contenu de probabilité. Un seuil de persistance calé sur le bruit ponctuel (~1/√K) sur-élague aux
  petits K.
- **Le remplissage b(ρ) est une allocation 1-NN bornée en densité, extérieure au modèle.**
  - Il fait l'essentiel du score de sklearn en feuilles α = 2 (+0,19 à K = 10), mais peu pour la tour (+0,017).
  - Sans remplissage, la meilleure tête de la tour bat la meilleure tête sklearn de +0,04 (K = 1) à +0,115 (K = 10).
  - Son k = max(K, 5) contredit EVAL_v2 § 2.6 (core_K) : écart déclaré.

### 1.7 HGP-old

- **Même objet.** C'est un indice sur 43 petits nuages : HGP-old calcule Γ_K sur un catalogue d'ordre-Voronoï qui
  donne les mêmes composantes non triviales que la tour FULL, et il répare le contre-exemple E5. Il retarde en
  revanche certaines attaches de facettes.
- **Ce qui explique ses bons résultats :**
  - l'appartenance par facettes avec un vote généreux, qui est une sémantique d'amas discrets ;
  - hors SIPU, un z ≥ 2 (huiles d'olive : z = 8, avec un λ saturé par l'EPS de 1e−12) ;
  - des conventions d'évaluation : ARI qui compte le bruit comme une classe dans ANS 2026, adversaire à
    `min_samples` = K + 1 ou K + 2, une exécution par jeu.
- **Ce qui ne l'explique pas, ou nuit sur dev :**
  - les masses fractionnaires du § 9.1 (−0,017 à −0,051) ;
  - la racine sélectionnable (effondrements en un seul amas à z ≤ 2) ;
  - l'entrée à α_{K+1} (neutre dès z ≥ 3).
- **Reproductibilité.** Les carnets de HGP-old n'ont aucune sortie sauvegardée, et le snapshot ne reproduit pas les
  résultats publiés.

### 1.8 Ce que seule la tour contient n'est pas exploité

- **La tête ne voit qu'une tranche en K.** Les verticales et les régions ne sont ni calculées sur le chemin du banc
  ni lues par la tête. L'instabilité de cette tranche en K est mesurée (coquilles à K = 8 contre K = 10), comme le
  prévoient Rolle et Scoccola.
- **L'axe multi-K existe aussi pour les hiérarchies de cœurs** (degree-Rips, core bifiltration).
- **Restent propres à la tour** : la stabilité forte, en distance de Prohorov, de la multicouverture
  (Blumberg–Lesnick), et les régions continues.
- **C'est la voie pour un avantage structurel sur HDBSCAN** : une tête multi-K, par exemple sur une tranche oblique
  (K décroissant avec r) construite par les verticales.

## 2. Conséquences

- **Tête v10-b.** Entrée `cover`, EOM, z choisi sur dev à la taille des tests, remplissage déclaré. ẑ est abandonné.
  Les scores sans remplissage sont toujours publiés à côté.
- **Lot A** (tête C∩X du 28 septembre, test préenregistré) : la tour bat HDBSCAN à K = 1, 2 et 3. À K = 1, l'écart
  est un effet de tête, comme l'annonçait l'attribution écrite avant le test.
- **Lot C** (préenregistrement `bc413ff56`, exécution en cours) : famille principale contre sklearn, lot B, famille
  « objet » contre MR₂-bord à même entrée et même tête, paires sans remplissage.
- **« HDBSCAN ne peut pas battre la tour »** est vrai en pratique contre sklearn tel qu'on l'utilise : test du lot A,
  dev à z ≥ 3. Mais l'avantage appartient à l'entrée et à la tête, qu'on peut aussi donner à la hiérarchie
  d'HDBSCAN. Un avantage structurel de l'objet reste à montrer, par l'axe K.
- **Documents à corriger :**
  - `CLUSTERING_DEPUIS_LA_TOUR_20260929.md` § 2 (« t^(−p) moins bon que ẑ ») et § 4 (l'écart à K = 1 est un effet de
    tête) ;
  - `src/tower/tower.hpp` et le reçu `bench_dev_cover_20260929` (« x entre à α_K(x)² » : la tête ne lit pas ce
    niveau) ;
  - `CLUSTER_v2` (« jamais comme hiérarchie ») ;
  - la contradiction EVAL_v2 / CLUSTER_v2 sur le k du remplissage ;
  - le README (multiplicités) ;
  - l'audit v9, constat A9-67, à inverser ;
  - dans la thèse, la phrase « la seule différence est le type de connexité », qui doit mentionner aussi
    l'appartenance.
- **Code :**
  - refuser α ≠ 1 hors de kd_tree dans `methods.hdbscan_labels` (dans sklearn 1.9.1, α n'agit pas en brute ni en
    précalculé) ;
  - corriger la raison du refus des multiplicités ;
  - écrire les échecs dev à ARI 0 (règle D8).
- **Recherche :**
  - tête multi-K (tranches obliques, verticales) ;
  - sélection qui ne dépende pas de la taille (z local, bootstrap de persistance) ;
  - catalogue approché certifié pour la performance.
