# MorseHGP3D v5 — Dossier mathématique

Date d'ouverture : 27 août 2026.

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Hérité de `morsehgp3D_v4/docs/MATHEMATIQUES.md` au pin `main@d4f3ce59` ;
**chaque statut a été re-déclaré**. Ce document est la source unique de
l'aspect mathématique de la v5 : l'objet, la réduction q2/q3/q4, les énoncés
avec leur statut, leurs largeurs arithmétiques sous u16 et les fixtures
gravées. Il ne décrit pas l'implémentation (voir
[`ARCHITECTURE.md`](ARCHITECTURE.md)) ni sa provenance (voir
[`PROVENANCE.md`](PROVENANCE.md)) ; les références de code ci-dessous
pointent uniquement vers des fichiers v5 existants, à titre de localisation.

## 0. Légende des statuts

Chaque énoncé porte un statut. **Aucun statut n'est une promotion** : en cas
de doute, le plus faible a été retenu, et la raison est dite.

- `theoreme_manuscrit` — énoncé et prouvé dans le manuscrit de thèse
  (référence exacte ; pages PDF = pages imprimées + 26) ; il est *invoqué*,
  jamais re-vérifié exhaustivement (`docs/TEST_PLAN_MORSEHGP3D.md` § 3.2).
- `theoreme_externe` — théorème de la littérature (Jung, Carathéodory,
  Callahan–Kosaraju), invoqué avec sa référence. Hors légende v4 ; ajouté
  parce que confondre un théorème externe avec un énoncé du manuscrit ou
  un dérivé local serait une promotion déguisée dans un sens ou dans l'autre.
- `recu_auditeur_v4` — énoncé dont la preuve a été déroulée et **reçue par
  l'auditeur v4** ; l'audit ou le reçu est cité (nom de fichier de
  `morsehgp3D_v4/audits/` ou `morsehgp3D_v4/receipts/`, ou hash court du
  commit d'audit). Un reçu v4 n'est pas un reçu v5 : le contrat v5 le
  requalifie par ses propres portes (`PROVENANCE.md`), mais l'énoncé
  mathématique, lui, n'a pas à être re-prouvé.
- `derive_v4_non_recu` — dérivé pendant la v4 (note, reçu d'implémenteur ou
  commentaire de code), **jamais reçu formellement** par un auditeur ; à faire
  valider par l'auditeur v5 avant toute invocation sans garde. Le statut v4
  `theoreme_v3` disparaît : chaque énoncé qui le portait devient
  `recu_auditeur_v4` s'il a été reçu depuis (l'audit est cité), sinon
  `derive_v4_non_recu`.
- `derive_v5` — dérivé ici même ; à faire valider par l'auditeur v5.
- `mesure` — fait chiffré (v3, v4 ou v5, daté), jamais une garantie.
- `ouvert` — question posée, non résolue (§ 9).

Les quatre verrous d'ouverture V1–V4 (§ 9) ont été **arbitrés** par
l'auditeur v5 le 27 août 2026
(`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`, pin
jugé `87e915bd`) ; chaque passage concerné cite « arbitrage V<n> ». Un
arbitrage ferme une question de conception, il ne promeut aucun statut : les
énoncés gardent le leur, les quatre verrous restent fermés pour tout claim et
`public_status` reste `not_claimed`.

Conventions : $D^2 = \left\Vert b-a \right\Vert^2$ pour une ancre $(a,b)$ ;
$m = (a+b)/2$ ; le **niveau est un rayon au carré** exact ; « intérieur »
signifie intérieur strict, « coquille » signifie sur la sphère.

---

## 1. L'objet normatif (manuscrit, Parties I–II)

### 1.1 La hiérarchie HGP

- **Complexe de Čech** (Déf. 20, p. PDF 83) :
  $\check{C}(X,r) = \left\lbrace \sigma \subseteq X : \bigcap_{x \in \sigma} \bar{B}(x,r) \neq \emptyset \right\rbrace$.
- **K-polyèdres** (Déf. 21, p. PDF 84) : composantes connexes du graphe
  $\Gamma_K(X,r)$ dont les sommets sont les $(K-1)$-simplexes ($K$ points)
  de $\check{C}(X,r)$, adjacents quand leur union est un simplexe de
  $\check{C}(X,r)$.
- **HGP-Clusterer** (Déf. 22, p. PDF 84) : $\theta_K^{HGP}(r)$ = les
  K-polyèdres de $\check{C}(X,r)$. Pour $K=1$, c'est exactement le
  Single-Linkage (Partie I). Les K-polyèdres ne font que croître et fusionner
  quand $r$ augmente.
- **Théorème 2** (p. PDF 86) : les K-polyèdres coïncident avec les amas
  discrets de forte densité K-NN, $\theta_K^{HGP}(r) = H^{discrets}_{\hat{f}_K}(r)$.
- Pour $K \geq 2$, l'objet est un **recouvrement** des points, mais une
  **partition des $(K-1)$-simplexes** ; la partition stricte des points est un
  post-traitement (§ 9.1 du manuscrit, vote pondéré, Prop. 7).

Statut : `theoreme_manuscrit`. Convention capitale : **le niveau est un
rayon** ($r = d/2$ pour une arête), jamais un diamètre.

### 1.2 La réduction : rayon de naissance, Gabriel, K-MST

- **Rayon de naissance** (Déf. 25, p. PDF 110) :
  $\rho(\sigma) = \inf_y \max_{x \in \sigma} \left\Vert y - x \right\Vert$,
  rayon de la **miniboule** $B_\sigma = \bar{B}(c_\sigma, \rho(\sigma))$.
  C'est le niveau exact d'apparition de $\sigma$ dans la filtration. Jamais
  le circumradius (les deux divergent dès qu'un simplexe est obtus —
  Remarque p. PDF 114).
- **Position générale pour la filtration de Čech** (Déf. 26, p. PDF 110) :
  aucun point de $X \setminus \sigma$ sur $\partial B_\sigma$, pour tout
  $\sigma$. Sur grille u16 elle est **fréquemment violée** (§ 7.5) : la sortie
  honnête hors du régime traité est `unsupported_degeneracy` ou
  `resource_exhausted`, jamais un jitter.
- **Fait 12** (p. PDF 111) : la miniboule est unique ; son **support**
  $S(\sigma) = \sigma \cap \partial B_\sigma$ a au plus $p+1 = 4$ points en
  3D, $c_\sigma$ est intérieur (relatif) à $\mathrm{conv}(S)$, et retirer un
  point du support fait strictement décroître $\rho$. $\left\vert S \right\vert \geq 2$ toujours.
- **Adjacences élémentaires** (Prop. 5, p. PDF 112) : restreindre les arêtes
  de $\Gamma_K$ aux seules unions de taille $K+1$ (les K-simplexes) ne change
  aucune composante. L'événement atomique est donc toujours « un
  $(K+1)$-uplet naît ».
- **Simplexe K-séparant** (Déf. 27, p. PDF 112–113) : $\sigma$ ($K+1$
  points) est K-séparant si deux de ses **facettes actives**
  ($\rho(\tau) < \rho(\sigma)$) sont dans deux composantes distinctes de
  $\Gamma_K(X)_{< \rho(\sigma)}$. Pour $K=1$ : les arêtes du MST.
- **Simplexe de Gabriel** (Déf. 28, p. PDF 113) :
  $\mathring{B}_\sigma \cap (X \setminus \sigma) = \emptyset$ — la miniboule
  **ouverte** est vide de tout point extérieur. Un point extérieur SUR la
  sphère est permis (c'est ce qui rend les plateaux du § 7.5 inévitables).
- **Théorème 4** (p. PDF 114–115) : K-séparant ⟹ Gabriel. Sa preuve donne
  plus : un $\sigma$ avec intrus $z \in \mathring{B}_\sigma$ a toutes ses
  facettes actives déjà connectées strictement avant $\rho(\sigma)$ (chemin
  $\tau_s \leftrightarrow \eta_{s,t}^z \leftrightarrow \tau_t$ par les
  simplexes $\sigma_s^z = (\sigma \setminus \lbrace s \rbrace) \cup \lbrace z \rbrace$
  de rayon $< \rho(\sigma)$). **L'élimination par témoin intérieur est donc
  sans perte pour la forêt.**
- **K-graphe de Gabriel** (Déf. 29, p. PDF 115–116) : sommets = facettes
  d'au moins un K-simplexe de Gabriel ; pour chaque K-simplexe de Gabriel
  $\sigma$, une clique sur ses facettes au poids $\rho(\sigma)$. **Prop. 6**
  (p. PDF 116) : ses composantes non triviales élaguées à $r$ = les
  K-polyèdres non triviaux. Relier *deux* facettes actives (un chemin) suffit
  pour la connectivité — la clique est un confort (Alg. 1 étape 4,
  p. PDF 126) ; elle redevient nécessaire pour le rendu (§ 7.7).
- **K-MST** (Déf. 30) et **Théorème 5** (p. PDF 117) : un arbre couvrant
  minimum du K-graphe de Gabriel, élagué à $r$, redonne exactement les
  K-polyèdres non triviaux, pour tout $r$. C'est l'objet que la v5 rend :
  **la forêt HGP = ce K-MST par K**, avec les niveaux $\rho^2$.
- **Théorèmes 6–7** (p. PDF 118, 125) : tout K-simplexe de Gabriel est porté
  par la mosaïque de Delaunay d'ordre K (resp. se lit dans $\mathrm{Del}_{K-1}$).
  La v5 les utilise comme **caractérisations et oracles**, jamais comme chemin
  de calcul (interdit d'architecture : aucune mosaïque d'ordre supérieur
  matérialisée).
- **Prop. 8** (p. PDF 124) et Fig. 8.1 (p. PDF 111) : un triangle obtus ou
  rectangle n'est jamais support ; son support retombe à l'arité 2 de son
  côté le plus long. **Fait 13** (p. PDF 130) : constante $\alpha_3 = \sqrt{6}/2$.

Statut : `theoreme_manuscrit`.

### 1.3 Le rendu § 9.1 (partition stricte, poids)

Sur les facettes $F_K$ : score
$S_\tau = \sum_{\sigma \supset \tau, \left\vert \sigma \right\vert = K+1} \psi(\rho(\sigma))$
avec $\psi(t) = 1/t^p$ (toute $\psi$ décroissante admissible ; le choix $1/r$
contre $1/r^2$ change le comportement filiforme — fixture `birch2`,
p. PDF 128–129) ; normalisation $T_x = \sum_{\tau \ni x} S_\tau$ ; masse
$m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$ (celle que consomme
`min_cluster_size`) ; vote $V_x(c) = \sum_{\tau \ni x, \ell(\tau) = c} S_\tau / T_x$,
étiquette argmax avec départage déterministe (Prop. 7). La partition de
l'unité $w_{x\tau} = S_\tau / T_x$ vérifie $\sum_{\tau \ni x} w_{x\tau} = 1$.

Statut : `theoreme_manuscrit`, avec une convention opérationnelle que le
manuscrit ne fixe pas — quelles facettes entrent dans $F_K$ — gravée au § 2
et discutée au § 9 (Q4).

---

## 2. Contrats gravés

Contrats issus des audits v4 (`ETAT_COURANT.md` v4 du 17 août, commit
`acd60d29`, et du 22 août, commit `bab37b97`), re-déclarés comme
**contrats v5** :

- **Sites distincts — sémantique normative : le REFUS.** Le manuscrit et la
  spécification d'autorité travaillent sur un ensemble de sites distincts
  $X \subset \mathbb{R}^3$. Les positions dupliquées sont **refusées**
  (`unsupported_degeneracy`) tant qu'un HGP pondéré n'est pas défini et
  prouvé (blocage motivé : deux identités au même site portent plusieurs
  sommets sur le même point de frontière, ce que la taxonomie « support de
  2 à 4 points + intérieurs stricts » ne code pas — `ETAT_COURANT.md` v4 du
  17 août § 1). **Incohérence v4 corrigée ici** : la v4 disait « refusées »
  dans sa doctrine et « bucketisées avec multiplicité » dans son
  architecture, et l'audit du 22 août (§ 3.6) a relevé que le refus n'était
  porté que par l'exécutable, pas par la frontière. En v5 : l'index spatial
  peut bucketiser (commodité d'index, `src/tree/cloud_index.hpp`), mais le
  pipeline **refuse** avant toute géométrie exacte ; les comptes de
  multiplicité de l'index n'ont **aucune** sémantique HGP. **Arbitrage V1**
  (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : le refus normatif est **conservé**. Le HGP pondéré n'est
  ni défini ni prouvé ; étendre la profondeur à un multi-ensemble
  modifierait les seuils, les supports minimaux, la notion d'arité, les
  événements et potentiellement les théorèmes invoqués ; la bucketisation de
  l'index est une capacité de représentation, pas une autorisation
  sémantique. Le refus doit être cohérent à toutes les frontières :
  `run_pipeline` refuse les doublons avant génération
  (`unsupported_degeneracy`) ; toute API basse acceptant un `CloudIndex`
  déclare et vérifie la précondition « positions distinctes », ou devient
  explicitement pondérée ; une fixture permanente vérifie le code de sortie
  exact, l'absence de callback et l'absence de payload partiel ; la présence
  de `range_weight()` ne doit pas laisser croire qu'un census pondéré est
  livré. Une voie pondérée future serait une **phase distincte** (définition
  de $\rho$, profondeur, supports de diamètre nul, oracles et reçus propres),
  jamais une optimisation silencieuse du profil actuel. Statut :
  `recu_auditeur_v4` (le refus), confirmé par l'arbitrage V1 ; l'extension
  pondérée n'est plus une question posée au profil courant — elle est hors
  profil.
- **Seuils effectifs** : $K_{eff} = \min(K_{max}, n)$,
  $s_{max} = \min(K_{eff}+1, n)$ ; la constante 11 vaut pour la cible
  $n \geq 11$, $K_{max} = 10$, jamais comme vérité générale (les petits
  oracles ne reçoivent pas 11 par défaut). Statut : `recu_auditeur_v4`
  (`ETAT_COURANT.md` v4 du 17 août, verdict, point 4).
- **Niveau public = rayon AU CARRÉ exact** (q2 : $D^2/4$ ; q3/q4 : fraction
  rationnelle). Une variable interne peut rester un rayon, elle ne contamine
  jamais `ExactLevel` (`src/lanes/level.hpp`). Statut : `recu_auditeur_v4`
  (idem, § 4).
- **Un événement critique est une hyperarête/multifusion** sur tous ses
  bras : un chemin couvrant compresse la connectivité, il n'autorise pas à
  binariser la chronologie d'un plateau. Deux `BallKey` distinctes de même
  niveau se traitent simultanément (contraction des durées nulles).
  Statut : `recu_auditeur_v4` (idem, § 4).
- **La sortie complète conserve les applications verticales entre ordres** :
  dix forêts indépendantes ne représentent pas seules la tour ordre-échelle.
  La v5 ne les construit pas encore (`../audits/ETAT_COURANT.md`).
  **Arbitrage V3**
  (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : le terme « forêt complète K = 1..10 » est retiré ou borné
  tant que la tour et la publication transactionnelle ne sont pas livrées ;
  le payload livré se déclare pour ce qu'il est — dix forêts horizontales
  indépendantes, **pas la tour** (`ARCHITECTURE.md` § 7 ; contrat de la
  future livraison des applications verticales : § 7.1). Statut du
  contrat : `recu_auditeur_v4` ; sa réalisation est un chantier non livré,
  ce n'est plus une question.
- **$F_K$ du rendu § 9.1 = TOUTES les facettes distinctes des événements**
  ($F_K^{render}$), pas seulement les actives ($F_K^{conn}$, suffisantes pour
  la connectivité seule) ; une variante active-only est une heuristique
  nommée, jamais « le rendu exact ». Statut : `recu_auditeur_v4` (idem, § 4 ;
  `AUDIT_CIBLE_5A08AB6_NAISSANCES_CROISSANCE_ET_RENDU_20260817.md` § 4).
- **Statuts transactionnels v5** : toute exécution termine dans
  `complete_regular | unsupported_degeneracy | resource_exhausted | invalid_input | invariant_violated` ;
  aucun préfixe de payload n'est publié sur un refus. Le statut v4
  `complete_regular_only` est remplacé par `complete_regular`, qui signifie
  la même chose : **le régime régulier plus les plateaux sous plafond**, pas
  « exact » (§ 7.5).

---

## 3. La réduction événements-boules et la taxonomie q2/q3/q4

### 3.1 Boule-événement ≡ simplexe de Gabriel

**Énoncé.** Sous sites distincts et position générale (Déf. 26),
l'application $\sigma \mapsto (S(\sigma), B_\sigma)$ est une bijection
entre :

- les K-simplexes de Gabriel $\sigma$ de $X$ ($K+1$ points), et
- les couples $(S, B)$ où $S \subseteq X$ est le support d'une miniboule $B$
  ($\left\vert S \right\vert \in \lbrace 2,3,4 \rbrace$ en 3D, $c_B$
  intérieur relatif à $\mathrm{conv}(S)$, $S \subset \partial B$) dont
  l'intérieur ouvert contient exactement $K+1-\left\vert S \right\vert$
  points de $X$.

*Preuve.* Direct : la position générale interdit tout point de
$X \setminus \sigma$ sur $\partial B_\sigma$, Gabriel en interdit tout
point dans l'intérieur ; donc $\sigma = S \cup (X \cap \mathring{B}_\sigma)$
et la profondeur vaut $K+1-\left\vert S \right\vert$. Réciproque : pour un
tel couple $(S,B)$, $\sigma = S \cup (X \cap \mathring{B})$ vérifie
$S \subseteq \sigma \subseteq B$, donc $\rho(\sigma) = \rho(S)$ et sa
miniboule est $B$ (ajouter des points intérieurs ne change pas la plus petite
boule englobante) ; aucun point extérieur à $\sigma$ n'est intérieur à $B$,
donc $\sigma$ est de Gabriel, et $\left\vert \sigma \right\vert = K+1$. La
miniboule étant unique (Fait 12), la correspondance est bijective.

Statut : `recu_auditeur_v4` — preuve déroulée par l'auditeur
(`ETAT_COURANT.md` v4 du 17 août § 1 « Réponse Q1 »), confirmée par
`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md` § 11 et par l'audit du
22 août § 3.1. **Hors** position générale, l'énoncé est remplacé par le
théorème du plateau (§ 7.5) ; **hors** sites distincts il n'a pas de sens —
d'où le refus normatif des positions dupliquées (§ 2, arbitrage V1).

### 3.2 Conséquences : arités, profondeurs, seuils de mort

Énumérer les événements de TOUTES les forêts $K = 1..K_{max}$ revient à
énumérer les couples (support, boule) **peu profonds** :

- arité $q = \left\vert S \right\vert \in \lbrace 2,3,4 \rbrace$ — la
  taxonomie q2/q3/q4 ;
- profondeur $d = \left\vert X \cap \mathring{B} \right\vert \leq K_{max}+1-q$ ;
  au-delà, la boule ne sert aucun $K \leq K_{max}$. Avec
  $s_{max} = K_{max}+1$, le seuil de mort d'une ancre d'arité $q$ est
  $h_q = s_{max} - q + 1$ témoins intérieurs : **10/9/8** à $K_{max} = 10$,
  **5/4/3** à $K_{max} = 5$ ;
- une boule de profondeur $d$ et d'arité $q$ est UN événement de la forêt
  $K = q+d-1$, de niveau $\rho(B)^2$, qui relie les facettes actives
  $\sigma \setminus \lbrace s \rbrace$, $s \in S$ (Théorème 4/Prop. 6).

Statut : `recu_auditeur_v4` (même audit, § 1).

**Lemme de complétude sous les seuils.** Si un plateau (§ 7.5) contient un
simplexe d'au plus $K_{max}+1$ points et si sa miniboule a un support minimal
d'arité $q$, alors $\left\vert I_B \right\vert \leq K_{max}+1-q = h_q - 1$.
Les témoins universels du fuseau de l'ancre (§ 4) sont intérieurs à cette
boule ; l'ancre du support minimal ne peut donc pas être tuée par le compte
de fuseau. Statut : `recu_auditeur_v4`
(`AUDIT_BLOQUANT_E7E4D5E_POINTID_DANS_LE_FOLD_20260817.md`, verdict).

**Règle de mort exacte par arité minimale.** Après déduplication des boules,
soit $q_{min}$ la plus petite arité d'un générateur de la boule (c'est la
cardinalité minimale d'un support de sa miniboule). Tout événement
$\sigma = I_B \cup T$ vérifie $\left\vert T \right\vert \geq q_{min}$, donc
il est utile à un $K \leq K_{max}$ seulement si
$\left\vert I_B \right\vert \leq K_{max}+1-q_{min} = h_{q_{min}} - 1$ :
$\left\vert I_B \right\vert \geq h_{q_{min}}$ ⟹ aucun événement (mort à
10/9/8 intérieurs au profil). Statut : `recu_auditeur_v4`
(`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md` § 1 ; le premier
candidat d'un groupe RLE porte $q_{min}$ par l'ordre canonique de
`src/pipeline/candidates.hpp` ; passe count-only dans
`src/pipeline/expand.hpp` / `census.hpp`).

### 3.3 Les trois arités en 3D

- **q2** : toute paire $\lbrace a,b \rbrace$ est son propre support ; la
  miniboule est la boule diamétrale $B(m, D/2)$. Niveau $D^2/4$. Forme
  primitive $\left\Vert 2z-(a+b) \right\Vert^2 - D^2 = 4(\left\Vert z \right\Vert^2 - (a+b) \cdot z + a \cdot b)$,
  donc `BallKey` $(A, B, C) = (1, -(a+b), a \cdot b)$ sans pgcd
  (`src/lanes/q2.hpp`). Statut : `theoreme_manuscrit` (miniboule d'une
  paire) ; forme : `derive_v4_non_recu` (calcul direct, gravé par
  `mhgp5_q2_fixture`).
- **q3** : $S = \lbrace a,b,x \rbrace$ est un support ssi le triangle est
  **strictement aigu** ; la miniboule est alors la boule (3D) centrée au
  circumcentre du plan du triangle. Obtus ou rectangle : le support retombe à
  l'arité 2 du côté le plus long (Prop. 8). L'arête maximale $(a,b)$ étant
  fixée, l'acuité du triangle $abx$ équivaut à l'acuité du seul angle en $x$,
  qui se lit sur $V^2 > D^2$ avec $V = \left\Vert 2x-a-b \right\Vert$ (les
  deux angles adjacents à l'arête maximale sont automatiquement non obtus ;
  l'égalité $V^2 = D^2$ est le triangle rectangle) — le porteur vit dans
  $\mathrm{lentille}(ab) \setminus \bar{B}(m, D/2)$. Statut :
  `recu_auditeur_v4` (`AUDIT_ORACLE_RATIONNEL_Q3_EBC8236_20260817.md`
  § 1.4 : confrontation à l'oracle des trois produits scalaires).
- **q4** : $S = \lbrace a,b,x,y \rbrace$ est un support ssi $c_\sigma$ est
  strictement intérieur à $\mathrm{conv}(S)$ (poids barycentriques de Cramer
  strictement positifs). « Tétraèdre bien centré » et « à faces aiguës »
  sont deux notions **distinctes**, chacune réfutant l'autre (fixtures v3
  gravées, reprises dans `PLAN_DE_TESTS.md`). Statut : `theoreme_manuscrit`
  (Fait 12 : centre intérieur relatif au support) ; distinction :
  `recu_auditeur_v4` (fixtures reprises, lemme du préfixe § 6.4).

### 3.4 L'ancre : arête maximale canonique

Tout support $S$ est possédé par son **arête maximale canonique** $(a,b)$ :
la plus longue arête, départagée par la plus petite
`EdgeKey` $= (\min \mathrm{PointId}, \max \mathrm{PointId})$
(`src/lanes/keys.hpp`) ; la fixture « tétraèdre aux six arêtes égales » grave
le départage. Propriétés :

- **Domaine (lentille).** Tout sommet du support est dans
  $\mathrm{lentille}(a,b) = \bar{B}(a,D) \cap \bar{B}(b,D)$ ; tout point
  intérieur strict de la miniboule est à distance $< 0{,}966\,D$ du milieu
  $m$ (§ 6.1, cover) : une seule requête $B(m,D)$ couvre sommets et
  intérieurs.
- **Jung.** $\rho(S) \leq \kappa'_q \cdot D$ avec, pour un ensemble de
  diamètre $D$ en dimension 3, $\rho/D \leq \sqrt{3/8} \approx 0{,}6124$
  (atteint par le tétraèdre régulier), $\rho/D \leq 1/\sqrt{3}$ pour un
  triangle (équilatéral), $\rho/D = 1/2$ pour une paire.

Statut : ancre et départage `recu_auditeur_v4` (owner canonique reçu,
`AUDIT_Q4_LEMME_PREFIXE_ET_NIVEAU_20260817.md` verdict ; audit du 22 août
§ 3.1) ; lentille `recu_auditeur_v4` (cover, § 6.1) ; Jung :
`theoreme_externe` (Jung 1901, dimension 3).

---

## 4. Fuseaux de mort et élagage par témoins

Code : `src/spindle/spindle.hpp` (formes, bornes, boule-cœur, autorités aux
coins), `src/spindle/witness_count.hpp` (descente fusionnée).

### 4.1 Le fuseau $W_q(a,b)$ et ses formes closes

$W_q(a,b)$ = intersection des **intérieurs** de toutes les miniboules
admissibles d'arité $q$ d'ancre owner $(a,b)$. Implication utile :
$z \in W_q(a,b)$ ⟹ $z$ tue tout support d'arité $q$ d'ancre $(a,b)$. La
réciproque est **fausse** en q3/q4 ($W$ est une sous-région de la zone de
mort réelle d'un support donné : c'est un minorant de témoins, fail-open).

Avec $w = z-a$, $d = b-a$, $H = d \cdot w - \left\Vert w \right\Vert^2 = (z-a) \cdot (b-z)$,
$\Xi = \left\Vert d \times w \right\Vert^2$ :

- $W_2$ : $H > 0$ ; $W_3$ : $H > 0$ et $3H^2 > \Xi$ ; $W_4$ : $H > 0$ et $2H^2 > \Xi$ ;
- emboîtement $W_4 \subset W_3 \subset W_2$ ; $(H, \Xi)$ ne dépendent pas de
  l'arité : une évaluation sert les trois lanes.

*Preuve (auditeur).* Toute sphère par $a,b$ a un centre $m+t$, $t \perp d$,
et un rayon $R(t) = \sqrt{p^2 + \left\Vert t \right\Vert^2}$ avec $p = D/2$.
Pour une ancre owner maximale, la fermeture du domaine des centres
admissibles est le disque $\left\Vert t \right\Vert \leq p/\sqrt{3}$ en q3 et
$\left\Vert t \right\Vert \leq p/\sqrt{2}$ en q4 (bornes de Jung, **atteintes** :
en q3 par un troisième sommet dans le plan de $d,t$ ; en q4, pour $t \neq 0$
et $e \perp d,t$ unitaire, par $x = m+2t+pe$, $y = m+2t-pe$, tétraèdre bien
centré de centre $m+t$ dont toutes les arêtes sont au plus $D$ dès que
$\left\Vert t \right\Vert \leq p/\sqrt{2}$). Avec $s = z-m$, la condition
$z \in \mathring{B}(m+t, R(t))$ s'écrit $p^2 - \left\Vert s \right\Vert^2 + 2 s \cdot t > 0$ ;
en minimisant sur le disque, avec $r$ la norme de la projection de $s$ sur
$d^\perp$ : $H - 2T_q r > 0$, $T_3 = p/\sqrt{3}$, $T_4 = p/\sqrt{2}$. Comme
$\Xi = D^2 r^2 = 4p^2 r^2$, on obtient exactement les trois formes.

Statut : `recu_auditeur_v4` (`ETAT_COURANT.md` v4 du 17 août § 6
« Réponse Q6 »). Largeurs u16 : $H$ en i64 ; $\Xi < 2^{72}$, $3H^2 < 2^{74}$ —
i128.

### 4.2 q2 est exact ; le rang de voisinage n'est pas borné

$W_2(a,b) = \mathring{B}(m, D/2)$ (boule diamétrale ouverte) et le
certificat de mort est une équivalence : $(a,b)$ q2-morte ⟺
$r_{h_2}(m) \leq D/2$ ($h_2$-ième plus proche voisin du milieu). Statut :
`recu_auditeur_v4` (idem § 6 ; « la lane q2 est correcte dans le régime
régulier », `AUDIT_CIBLE_1310B21_FACETTES_NEES_DANS_LE_LOT_20260817.md`).

Le rang de voisinage d'une arête vivante n'est PAS borné : contre-exemple v3
d'une arête vivante joignant $a$ à son 1001-ième voisin (fixture 50 000
points, `morsehgp3D_v3` PROPOSITION § 10) — aucune source kNN de petit
préfixe n'est complète. Statut : `mesure` (contre-exemple constructif,
gravé).

### 4.3 Boule-cœur ponctuelle : rayons $\kappa_q$ et formes entières

La plus grande boule centrée en $m$ incluse dans le fuseau ouvert d'une ancre
ponctuelle est $\mathring{B}(m, \kappa_q D)$ avec
$\kappa_q = \min_{\left\Vert t \right\Vert \leq T_q} (R(t) - \left\Vert t \right\Vert) / D$
(minimum au bord du disque) :
$\kappa_2 = 1/2$, $\kappa_3 = 1/(2\sqrt{3}) = 1/\sqrt{12}$,
$\kappa_4 = (\sqrt{3}-1)/(2\sqrt{2}) = \sin 15^\circ$.

Formes entières (avec $u = 2z-a-b$, $U = u \cdot u$, $L = D^2$) : q2 :
$U < L$ ; q3 : $3U < L$ ; q4 : poser $Y = 2L - U$, tester $Y > 0$ et
$Y^2 > 3L^2$ (car $\sin^2 15^\circ = (2-\sqrt{3})/4$). $U = L$ est la coquille
q2. Le test $15U < 4L$ (rayon $D/\sqrt{15}$) est une **sous-approximation
sûre mais stricte** du cœur q4 exact, car $4/15 < 2-\sqrt{3}$ ; « $D/\sqrt{12}$ »
et « $D/(2\sqrt{3})$ » sont LE MÊME rayon q3 (rectificatif reçu). Fixture
discriminante gravée : $a = (10000,10000,0)$, $b = (20000,10000,0)$,
$z = (15000,12585,0)$ — dans le cœur exact q4 ($Y^2 > 3L^2$), hors de
$15U < 4L$. Aucun décimal ne décide ; seules ces formes entières sont
l'autorité.

Statut : `recu_auditeur_v4` (idem § 6, « Correction obligatoire de
`1f1ae0c` »). En v5, `spindle.hpp` utilise des constantes point-fixe
**sous-approchées** de $2\kappa_q$ (échelle $2^{30}$) à preuves compilables
(`static_assert`) : l'arrondi est du côté fail-open par construction.

### 4.4 Boules-cœur de bloc : $R_{dec}$ et $R_{coup}$

Pour des extrémités dans deux boules (centres $c_A, c_B$ distants de $d$,
rayons $r_A, r_B$, $r = r_A + r_B$), au milieu nominal $c = (c_A+c_B)/2$ :

$$R_{dec,q} = \kappa_q (d - r) - r/2, \qquad R_{coup,q} = \kappa_q d - \sqrt{(4\kappa_q^2+1)(r_A^2+r_B^2)/2}.$$

Les deux boules $\mathring{B}(c, R)$ sont incluses dans $W_q(a,b)$ pour tout
$a \in B(c_A, r_A)$, $b \in B(c_B, r_B)$ ; prendre $\max(0, R_{dec}, R_{coup})$
est sûr. *Preuve (auditeur)* : $a = c_A+u$, $b = c_B+v$, $p' = (u+v)/2$
(déplacement du milieu), $w' = (v-u)/2$ (erreur de demi-arête) ;
$\left\Vert b-a \right\Vert \geq d - 2\left\Vert w' \right\Vert$ ; une boule
centrée en $c$ reste incluse dès que
$R \leq \kappa_q d - (2\kappa_q \left\Vert w' \right\Vert + \left\Vert p' \right\Vert)$ ;
l'identité du parallélogramme donne
$\left\Vert p' \right\Vert^2 + \left\Vert w' \right\Vert^2 \leq (r_A^2+r_B^2)/2$,
puis Cauchy–Schwarz donne la borne couplée. Arithmétique dirigée
obligatoire : distance des centres **minorée**, rayons **majorés**, terme
soustrait arrondi vers le haut, racine par `ceil_sqrt` exact, comparaison
stricte ; tout arrondi vers le haut du rayon crée de faux crédits (mutant v5
`core-ball-ceil-distance`).

Statut : `recu_auditeur_v4` (`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md`
§ 1 « Réception du rayon couplé » ; `ETAT_COURANT.md` v4 du 17 août § 6).
Le gain « +71 % de rayon à $s = 6$ en q4 » reste une `mesure` v3.

### 4.5 Bornes de bloc : $H_{min}$ exact, $H_{max}$, autorité aux coins

Sur $\mathrm{Box}(A) \times \mathrm{Box}(B) \times \mathrm{Box}(Z)$ :

- le minimum de $H$ est **séparable par axe** (bilinéaire en $(a_i,b_i)$ :
  minimum à un coin ; concave en $z_i$ : minimum à un bout) — minimum exact
  sur le produit continu des boîtes ; $H_{min} > 0$ ⟹ tout point de $Z$ est
  témoin universel du rectangle (crédit de sous-arbre q2). Statut :
  `recu_auditeur_v4` (`ETAT_COURANT.md` v4 du 17 août § 9 : « $H_{min}$ est
  exact ») ;
- le majorant minimax $H_{max}$ ($\min_{a,b} \max_z H$, échelle quatre en
  v5) : $H_{max} \leq 0$ élague le sous-arbre pour les trois lanes (car
  $W_q \subseteq W_2$), jamais utilisé pour créditer. Statut :
  `recu_auditeur_v4` (idem : « la borne minimax $H_{max}$ est sûre pour
  élaguer ») ;
- **autorité aux 64 coins** : avec $u = z-a$, $v = b-z$,
  $H = u \cdot v$ et $\Xi = \left\Vert u \times v \right\Vert^2$, les trois
  lanes sont des contraintes angulaires : $\angle(u,v) < 90^\circ$ (q2),
  $< 60^\circ$ (q3), $< \arccos(1/\sqrt{3})$ (q4). À $v$ fixé, l'ensemble
  des $u$ admissibles est un cône circulaire ouvert **convexe**, et la
  relation est symétrique. Si les $8 \times 8$ couples de coins distincts de
  $z-A$ et $B-z$ satisfont la lane, la convexité en $u$ puis en $v$ donne
  toute la boîte : `ALL` aux coins est exact pour l'enveloppe AABB continue,
  boîtes plates comprises après suppression des coins dupliqués. Sur les
  `PointId` réellement stockés, ce certificat reste **suffisant**, jamais
  nécessaire. Statut : `recu_auditeur_v4` (idem § 7 « preuve de l'autorité
  64 coins »). Mise en garde héritée : le majorant de $\Xi$ par composantes
  est sûr mais **pas** serré, et une combinaison séparée de $H_{min}$ et
  $\Xi_{max}$ peut prendre ses extrema en des coins incompatibles — la preuve
  par cônes est la bonne.

### 4.6 Autorité $h_a$ : coins de $\mathrm{Box}(T)$

Pour une extrémité ponctuelle $s$ et un partenaire dans $\mathrm{Box}(T)$ :
$z$ est témoin universel de $\lbrace s \rbrace \times \mathrm{Box}(T)$ ssi
$z \in W_q(s,t)$ pour les 8 coins distincts $t$ de $\mathrm{Box}(T)$ — à
$s,z$ fixés, le lieu des partenaires admissibles est un cône convexe ouvert.
Statut : `recu_auditeur_v4`
(`AUDIT_CONSTRUCTIF_APRES_A047460_COVER_Q3_20260817.md` § 0, point 1). Les
formes v3 en racines (« $J > r\sqrt{E}$ », etc.) ne sont **pas** reprises :
la v5 évalue $(H, \Xi)$ aux coins, sans racine (`spindle.hpp`).

### 4.7 Théorème de disjonction et corollaire fail-open

Par rectangle WSPD $A \times B$ : $h_{coeur}$ (témoins hors $A \cup B$
universels sur tout le rectangle), $h_a(a)$ (témoins de $A$ universels sur
$\lbrace a \rbrace \times B$), $h_b(b)$ (témoins de $B$ universels sur
$A \times \lbrace b \rbrace$).

- **Disjonction** : les trois ensembles sont deux à deux disjoints par
  identité — automatique, sans hypothèse géométrique (pour $z \in A$, le
  choix $a = z$ donne $H = 0$ : $z$ n'est jamais témoin universel du
  rectangle ; idem pour $B$).
- **Corollaire (fail-open)** :
  $\left\vert X \cap W_q(a,b) \right\vert \geq h_{coeur} + h_a(a) + h_b(b)$ ;
  l'ancre meurt dès que la somme atteint $h_q$. Le filtre ne ferme jamais à
  tort.
- Avec $r_q = h_q - h_{coeur,q}$ et $h_b$ saturé à $r_q$, l'histogramme
  cumulatif $C(t) = \#\lbrace b \in B : h_b(b) < t \rbrace$ donne pour chaque
  $a$ le nombre de partenaires survivants **sans former $A \times B$**
  (survie ⟺ $h_b(b) < r_q - h_a(a)$, inégalité stricte).

Statut : `recu_auditeur_v4` (`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md`
§ 0 point 2 et § 2 ; `ETAT_COURANT.md` v4 du 17 août § 9), sous le contrat
de sites distincts.

### 4.8 Masques de lanes

Les fuseaux étant emboîtés, un sous-arbre crédité pour la lane $q$ et
redescendu pour les autres doit masquer $q$ ; sans le masque, doubles crédits
et fausses morts (mutant v3 `dual-sans-masque` : 212 et 1525 fausses morts
mesurées ; mutant v5 `witness-no-lane-mask`). La descente fusionnée
(une pile, un masque par sous-arbre, trois compteurs écrêtés à $h_q$,
$(H,\Xi)$ une fois par coin) n'a ni double comptage ni fermeture injustifiée.
Statut : `recu_auditeur_v4` (même audit, § 3 « Descente q2/q3/q4
fusionnée » ; `ETAT_COURANT.md` v4 du 22 août § 3.3).

### 4.9 Qualité du minorant de témoins

- `mesure` (v3, 36 configurations, `morsehgp3D_v4/receipts/wspd_front_20260817/`) :
  fermeture des ancres q4 de 76,5 % à 99,3 % selon famille/$s$/$n$ ;
  $s = 8$ domine $s = 6$ partout ; le « mou » (survivantes/vivantes réelles)
  vaut 1,10 (`uniform`), 1,31 (`terrain`), 2,12 (`eight_clusters`). Le vrai
  vivant est un invariant du nuage, PAS de la partition — contrôle
  obligatoire par juge d'échantillon.
- **Poisson homogène, ancre ponctuelle fixée** : avec
  $F_h(\mu) = e^{-\mu} \sum_{j=0}^{h-1} \mu^j/j!$, $\mu_W = \lambda \left\vert W_q \right\vert$
  et $\mu_L = \lambda \left\vert L \right\vert$ pour une région certifiée
  $L \subseteq W_q$ : survie au filtre $F_h(\mu_L)$, vie réelle $F_h(\mu_W)$,
  mou $F_h(\mu_L)/F_h(\mu_W)$, aucune fausse mort. Globalement, si
  $\left\vert W_q(a,b) \right\vert = v_q \left\Vert a-b \right\Vert^3$ hors
  bord, le nombre de paires réellement vivantes vérifie
  $E[V_h]/E[n] \to 2\pi h/(3 v_q)$ : environ 40, 123,8 et 139,1 par point
  pour $h = 10/9/8$ ; le mou du cœur ponctuel seul vaut $v_q/c_q \approx 1$,
  1,511, 1,659 (fractions volumiques 1, 0,662, 0,603). Statut :
  `recu_auditeur_v4` (`ETAT_COURANT.md` v4 du 17 août § 2) **pour une région
  déterministe** ; les constantes sont de volume infini et un reçu cubique
  exige le terme de bord (`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md`
  § 9).
- `ouvert` (Q2) : passer du bloc fixé à la WSPD aléatoire (corrélée au
  nuage) exige un argument de stabilisation ou un arbre pilote
  indépendant ; le cas `eight_clusters` (paires inter-amas au milieu vide :
  le cœur ne peut rien, seuls $h_a/h_b$ mordent).

---

## 5. La source : WSPD de Callahan–Kosaraju

Code : `src/wspd/wavefront.hpp`.

- **Théorie.** Sur un fair split tree, la WSPD à séparation $s$ produit
  $O(s^3 n)$ rectangles en 3D et partitionne exactement les paires de
  positions distinctes ; coût $O(n \log n + s^d n)$. Statut :
  `theoreme_externe` (Callahan–Kosaraju 1995).
- **Partition exacte des paires (arbre radix).** Chaque paire non ordonnée
  de positions distinctes a un unique plus petit ancêtre commun, donc
  apparaît dans une unique graine $(\mathrm{left}(v), \mathrm{right}(v))$ ;
  chaque scission partitionne exactement le rectangle parent. Le ledger
  $\sum \left\vert A \right\vert \left\vert B \right\vert = \binom{n}{2} - \sum_u \binom{m_u}{2}$
  n'est pas la preuve, c'est une porte contre les fautes d'implémentation.
  Statut : `recu_auditeur_v4` (`ETAT_COURANT.md` v4 du 17 août § 3).
- **Les deux interdits** (post-mortem v3, arbitrage du 16 août 2026) :
  terminal ⟺ séparé (jamais de cap dans le critère : un cap de masse $C$
  force $\#\mathrm{rect} \geq \binom{n}{2}/C^2$, quadratique par
  construction) ; scission du facteur de plus grand **diamètre géométrique**,
  jamais du plus peuplé (invariant de l'argument d'empilement ; −14,7 % de
  rectangles mesuré). Statut : `recu_auditeur_v4` (idem § 9, affirmations
  confirmées) ; mutants v5 `wspd-cap-terminal`, `wspd-split-heaviest`.
- **Prédicat de séparation entier** : avec $D2$ le carré de la distance des
  centres **doublés** et $W2$ le carré du diamètre de boîte,
  $q^2 \cdot D2 \geq (p+2q)^2 \cdot \max(W2_A, W2_B)$ implique
  $d - r_A - r_B \geq s \cdot \max(r_A, r_B)$ pour $s = p/q$. Il peut manquer
  une séparation (le front grossit), jamais en inventer. i64 sous u16.
  Statut : `recu_auditeur_v4` comme condition **suffisante**
  (`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md` § 10).
- **Mort dans la descente.** Une paire de nœuds dont le cœur compte déjà
  $h_q$ témoins universels meurt SANS descente : tout point de $W_q(a,b)$
  est intérieur à toute miniboule admissible de l'ancre, donc une paire de
  nœuds créditée de $h_q$ témoins ne contient l'owner d'aucun événement
  utile ; la masse d'un rectangle tué est comptabilisée morte, le ledger
  total est inchangé. Statut : `recu_auditeur_v4` (`ETAT_COURANT.md` v4 du
  17 août § 3 « Ce qui est reçu » ; verdict « préfiltre fail-open reçu »).
- **Borne $O(s^3 n)$ pour la variante implémentée** (arbre radix sur clés
  de Morton, boîtes serrées, scission par diamètre de boîte) : **non
  établie**. Le ledger prouve une partition sans perte, pas une borne de
  taille ; l'argument « aspect borné, donc facteur $8^d$ » ne suffit pas car
  le choix de la branche scindée est lui aussi modifié. Route reçue de
  l'auditeur : scission pilotée par la cellule de préfixe exacte (aspect 1,
  2 ou 4), terminal si `SepCell` OU `SepTight`, la récursion « ombre » à
  `SepCell` seule étant la WSPD de packing standard et le front réel un
  coarsening de son antichaîne. Statut : `ouvert` (audit du 22 août § 3.3 ;
  `AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md` § 10). **Abaissé** par
  rapport à la v4, qui présentait la borne comme « constat de conformité
  v3 » ; tant que ce raccord n'est pas écrit, la complexité est
  conditionnelle aux compteurs mesurés.
- `mesure` (v3) : régime linéaire non atteint aux tailles d'intérêt sauf
  `scanline` (exposant local 1,31 → 1,22 → 1,18 jusqu'à $n = 64\,000$,
  `terrain` plafonne à ~1,27) ; 236–887 rectangles/point selon famille et
  $s$ ; toute pente sous $n \approx 8\,000$ est fausse (plafond
  $\binom{n}{2}$) ; ~$10^7$ rectangles pour $6{,}6 \cdot 10^5$ arêtes q2
  vivantes à $n = 32\,000$.

---

## 6. Identification exacte des événements

Pour chaque ancre survivante $(a,b)$, par arité (`src/pipeline/generate.hpp`).

### 6.1 Le cover d'ancre

- **Coefficient 3 (q3 : porteurs, intérieurs, coquille ; q4 : sommets).**
  Soit $C$ l'angle opposé à l'arête maximale $ab$ d'un triangle aigu
  possédé : $\pi/3 \leq C < \pi/2$, $R = D/(2 \sin C)$,
  $\delta = D/(2 \tan C)$ (distance du circumcentre à $m$), donc tout point
  de la circum-boule fermée vérifie
  $\left\Vert z-m \right\Vert \leq R + \delta = (D/2) \cot(C/2) \leq (\sqrt{3}/2) D$,
  égalité pour $C = \pi/3$ (constante **sharp**). En entiers :
  $\left\Vert 2z-a-b \right\Vert^2 \leq 3D^2$. Corollaire :
  $\left\Vert z-a \right\Vert \leq (\sqrt{3}+1)D/2$, soit
  $\left\Vert z-a \right\Vert^2 < 2D^2$ (car $((\sqrt{3}+1)/2)^2 < 2$) —
  le majorant $S_{max} = 2D^2$ de l'étage flottant (§ 6.7). Statut :
  `recu_auditeur_v4` (`AUDIT_CONSTRUCTIF_APRES_A047460_COVER_Q3_20260817.md`
  § 4.1 ; corollaire : `CONTRE_AUDIT_879B37_BORNE_FLOTTANTE_DYNAMIQUE_20260818.md`
  § 1).
- **Coefficient 4 (q4 : intérieurs et coquille).** Le support étant d'arité
  4, la circum-boule EST la miniboule du tétraèdre, donc
  $R \leq \sqrt{3/8} D$ (Jung, diamètre $\leq D$ car $ab$ est maximale) ;
  le centre est sur le plan médiateur de $ab$ :
  $\left\Vert c-m \right\Vert^2 = R^2 - D^2/4$ ; tout point intérieur ou de
  coquille vérifie
  $\left\Vert z-m \right\Vert \leq R + \sqrt{R^2 - D^2/4} \leq (\sqrt{3/8} + \sqrt{1/8}) D \approx 0{,}966\,D < D$,
  soit $\left\Vert 2z-(a+b) \right\Vert^2 \leq 4D^2$. Statut :
  `recu_auditeur_v4` (`AUDIT_Q4_LEMME_PREFIXE_ET_NIVEAU_20260817.md`,
  verdict : « le cover de coefficient 4 »). Code : `src/lanes/edge_cover.hpp`
  (coefficient 1/3/4).
- **Early-exit à $h_q$** : une boule ayant déjà $h_q$ intérieurs ne sert
  aucune forêt demandée, donc sa coquille n'a pas à être certifiée par le
  pipeline peu profond. Statut : `recu_auditeur_v4` (même audit A047460,
  § 0 point 4).

### 6.2 q2

L'événement existe ssi $\mathrm{depth}(B(m, D/2)) < h_2$ ; la boule
diamétrale est toujours émise, la profondeur se décide au census (§ 6.8).
Décision par comparaisons i64. Statut : `recu_auditeur_v4` (§ 4.2).

### 6.3 q3 : la forme de Gram

Code : `src/lanes/q3.hpp`. Avec $d = b-a$, $u = x-a$, $D = d \cdot d$,
$E = u \cdot u$, $F = d \cdot u$, $G = DE - F^2 > 0$ :

- **Circumcentre** : $c - a = \alpha d + \beta u$ avec
  $\alpha = E(D-F)/(2G)$, $\beta = D(E-F)/(2G)$ ; en posant
  $W = E(D-F)\,d + D(E-F)\,u$, $W = 2G(c-a)$.
- **Puissance sans centre ni division** : pour $v = z-a$,
  $P(z) = G \left\Vert v \right\Vert^2 - v \cdot W = G(\left\Vert z-c \right\Vert^2 - \left\Vert a-c \right\Vert^2)$ ;
  $P < 0$ intérieur strict, $= 0$ coquille, $> 0$ extérieur.
- **Niveau** : $R^2 = D \cdot E \cdot X / (4G)$ avec
  $X = \left\Vert b-x \right\Vert^2 = D+E-2F$ (identité croisée vérifiée par
  l'oracle : $\left\Vert a\,\det - N \right\Vert^2 \cdot 4G = DEX \det^2$).
- **Descente par axe** : $P$ est séparable et convexe par coordonnée ; le
  minimum de réseau sur un intervalle entier est atteint à l'un des deux
  entiers voisins du sommet $w_i/(2G)$ (clippés), le maximum à une
  extrémité ; élagage STRICT $mn > 0$ (un nœud à $mn = 0$ peut porter une
  coquille — mutant `q3-prune-ge`).
- **Owner** : $(a,b)$ doit rester l'arête maximale de $\lbrace a,b,x \rbrace$
  avec départage `EdgeKey`.
- **`BallKey`** : $(A, B, C) = (G, -(2Ga + W), G \left\Vert a \right\Vert^2 + W \cdot a)$,
  réduite pgcd/signe.
- **Largeurs u16** : $D, E < 3 \cdot 2^{32}$ ; $G < 9 \cdot 2^{64}$ ;
  $\left\vert W_i \right\vert < 9 \cdot 2^{82}$ ;
  $G \left\Vert v \right\Vert^2 < 2^{103}$, $\left\vert v \cdot W \right\vert < 2^{105}$
  — i128 ; $\left\vert B_i \right\vert < 2^{86}$, $\left\vert C \right\vert < 2^{104}$ ;
  $DEX < 2^{104}$ ; produits croisés q3/q3 $< 2^{171}$ (U192,
  `src/lanes/level.hpp`).

Statut : forme, signe, descente et niveau `recu_auditeur_v4`
(`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md` § 4 ;
`AUDIT_ORACLE_RATIONNEL_Q3_EBC8236_20260817.md` § 1.1–1.3) ; largeurs
`recu_auditeur_v4` (même audit oracle, § 3 : « les largeurs sont sûres ») ;
le cast du sommet rationnel en i64 est sûr **sous** $G > 0$ (centre dans
l'intérieur relatif du triangle aigu) — précondition à garder explicite.

**Filtre de profondeur à la génération** : $\#\lbrace z \in \mathrm{cover} : P(z) < 0 \rbrace$
minore $\left\vert I_B \right\vert$ (le cover contient tous les intérieurs,
§ 6.1) ; atteindre $h_3$ tue avant l'émission. Fail-open : omettre un site
ne peut qu'affaiblir le compte. Statut : `recu_auditeur_v4` (A047460 § 0,
points 3–4).

### 6.4 q4 : Cramer relatif, arité stricte, owner, lemme du préfixe

Code : `src/lanes/q4.hpp`.

**Circumcentre (Cramer 3×3 relatif).** Lignes $M = (2(b-a), 2(x-a), 2(y-a))$,
second membre $r = (\left\Vert b-a \right\Vert^2, \left\Vert x-a \right\Vert^2, \left\Vert y-a \right\Vert^2)$ ;
$M(c-a) = r$, $c - a = N'/\det$ avec $N' = \mathrm{adj}(M)\,r$.
**Canonisation d'orientation** : si $\det < 0$, négation simultanée
$(\det, N') \to (-\det, -N')$ — le centre est inchangé, $\det > 0$ gravé.
$\det = 0$ (quatre points coplanaires) : jamais un support q4, le candidat
est ignoré.

**Puissance affine** (le carré est évité) : avec $dz = z-a$,
$P_4(z) = \det \left\Vert dz \right\Vert^2 - 2 N' \cdot dz = \det (\left\Vert z-c \right\Vert^2 - R^2)$ ;
sous $\det > 0$ : $P_4 < 0$ intérieur strict, $= 0$ coquille.

**Arité 4 stricte.** Pour chaque face $(p,q,r)$ de sommet opposé $s$, le
signe de $\det_3(q-p, r-p, N' - \det (p-a))$ doit être NON NUL et égal à
celui de $\det_3(q-p, r-p, s-p)$ (l'égalité $c-p = (N' - \det(p-a))/\det$
et $\det > 0$ rendent le test homogène). Un zéro ⟹ centre sur une face ⟹ le
support retombe à l'arité $\leq 3$ : le candidat q4 est ignoré (il appartient
à une autre lane). Par multilinéarité du déterminant, les quatre
$\det_3(q-p, r-p, s-p)$ valent $\pm V$ pour le même volume orienté
$V = \det_3(b-a, x-a, y-a)$ (signes alternés selon le sommet opposé) : un
seul volume à calculer. Statut de cette dernière remarque : `derive_v5`.

**Owner (6 arêtes)** : $ab$ maximale parmi les six longueurs carrées, tout
ex æquo départagé par la plus petite `EdgeKey` (cinq comparaisons).

**Lemme du préfixe ternaire (exact-once du seed).** Tout q4 bien centré
d'arête maximale $ab$ a AU MOINS UNE face $abv$ ($v \in \lbrace x,y \rbrace$)
strictement aiguë, jamais nécessairement deux (contre-fixture gravée).
*Preuve (auditeur).* Centre à l'origine : $u = a-c$, $v = b-c$, $p = x-c$,
$q = y-c$, tous de norme $R$. Centre strictement intérieur ⟹ il existe
$\alpha, \beta, \gamma, \delta > 0$ de somme 1 avec
$\alpha u + \beta v + \gamma p + \delta q = 0$. Si les deux faces sont non
aiguës, l'angle fautif est opposé à $ab$ (arête maximale), donc
$(u-p) \cdot (v-p) \leq 0$ et $(u-q) \cdot (v-q) \leq 0$. Avec $s = u+v$ et
$\tau = R^2 + u \cdot v = \left\Vert s \right\Vert^2/2 \geq 0$, ces
inégalités donnent $p \cdot s \geq \tau$ et $q \cdot s \geq \tau$, tandis
que $u \cdot s = v \cdot s = \tau$. Le produit scalaire de la relation
barycentrique par $s$ donne $0 \geq \tau$, donc $\tau = 0$, $s = 0$,
$v = -u$ : le centre serait le milieu de $ab$, sur le bord du tétraèdre —
contradiction. L'angle opposé à $ab$ d'une des deux faces est donc
strictement aigu, et $ab$ y étant maximale, la face entière l'est. CQFD.
La source par seeds aigus est donc COMPLÈTE sans aucun héritage q3
(fixture 13 points, renforcée à 22 : ancre q3-morte / q4-vivante avec
tétraèdre de profondeur 0, `AUDIT_CIBLE_F6B29E1_FIXTURE_Q4_VRAIMENT_INDEPENDANTE_20260817.md`).
Le seed canonique est le carrier de plus petit `PointId` parmi les faces
incidentes aiguës du tétraèdre FORMÉ ; une complétion n'émet que si son seed
est ce minimum — vérification en temps constant.

**`BallKey` q4** : $(A, B, C) = (\det, -2(\det a + N'), \det \left\Vert a \right\Vert^2 + 2 N' \cdot a)$,
$A > 0$, réduite pgcd/signe — le MÊME gabarit que la clé q3 (« une boule est
une boule », `src/lanes/keys.hpp`).

**Niveau q4 — la largeur dépasse i128.** $R^2 = \left\Vert N' \right\Vert^2 / \det^2$
avec $\left\Vert N' \right\Vert^2 < 3 \cdot 2^{144} < 2^{146}$ : numérateur
en U192, $\det^2 < 2^{114}$ en i128, **non réduits** ; comparaison croisée
q4/q4 $< 2^{260}$ → U320 ; q3/q4 mixte $< 2^{218}$. Contrat indispensable :
l'égalité de représentation n'est pas l'égalité sémantique ; la seule
égalité autorisée pour les macro-lots est `same_exact_level` (produit croisé
nul), jamais `==` ni un hachage du couple brut. La canonisation pgcd 192 bits
n'est requise qu'à une éventuelle sérialisation canonique des seuls
survivants.

**Largeurs u16** : $\left\vert \det \right\vert < 6 \cdot 2^{54} < 2^{57}$ ;
$\left\vert N'_i \right\vert < 2^{72}$ ; $\det \left\Vert dz \right\Vert^2 < 2^{93}$,
$2 \left\vert N' \cdot dz \right\vert < 2^{92}$ → i128 ;
$\left\vert N' - \det (p-a) \right\vert < 2^{75}$,
$\det_3(2^{17}, 2^{17}, 2^{75}) < 2^{112}$ → i128 ;
$\left\vert B_i \right\vert < 2^{74}$, $\left\vert C \right\vert < 2^{90}$.

Statuts : Cramer relatif, orientation, puissance affine, arité stricte,
owner, exact-once, lemme du préfixe (avec sa preuve), représentant de
niveau non réduit et contrat d'égalité sémantique (Q12) —
`recu_auditeur_v4` (`AUDIT_Q4_LEMME_PREFIXE_ET_NIVEAU_20260817.md` verdict,
§ 1, § 2 ; audit du 22 août § 3.1). Largeurs q4 — `derive_v4_non_recu` :
relues par l'auditeur (« les limites de largeur sont documentées et les
oracles extrêmes u16 exercent les carries », 22 août § 3.1) sans dérivation
formelle publiée ; **abaissé** par prudence, à re-dériver dans le reçu
arithmétique v5 (`mhgp5_level_cmp`, oracle 384 bits).

La **sélection axiale** v3/v4 (faisceau de sphères par seed, « seize
groupes ») n'existe plus en v5 : voir `PISTES_FERMEES.md`.

### 6.5 Le cœur universel du seed (Jung)

Seed aigu $(a,b,x)$ fixé, forme q3 $P$ et forme du plan $B(z) = n \cdot (z-a)$
avec $n = (b-a) \times (x-a)$. Les sphères par le cercle circonscrit de la
face forment le faisceau $\Phi_\mu(z) = P(z) - \mu B(z)$, de centre
$a + (W + \mu n)/(2G)$ et de rayon carré $R_\mu^2 = R_3^2 + \mu^2/(4G)$,
$R_3^2 = DEX/(4G)$. Toute complétion q4 acceptée est un tétraèdre bien centré
d'arête owner de longueur carrée $D$ ; sa circumboule est sa miniboule, donc
(Jung) $R_\mu^2 \leq 3D/8$ (ici $D$ note la longueur **carrée**), d'où
$2\mu^2 \leq J$ avec $J = D(3G - 2EX)$. Par conséquent un site $z$ avec

$$P(z) < 0 \quad \text{et} \quad 2P(z)^2 > J \cdot B(z)^2$$

est strictement intérieur à **toute** sphère q4 admissible du seed (car
$\Phi_\mu(z) \leq P(z) + \sqrt{J/2} \left\vert B(z) \right\vert < 0$) : compté
comme témoin seed-universel jusqu'à $h_4$, le seed entier meurt. Fail-open :
omettre les témoins hors cover ne peut que sous-compter. Corollaire
(`derive_v5`) : $J < 0$ ⟹ aucune sphère du faisceau ne satisfait Jung ⟹ le
seed n'a aucune complétion q4 admissible. Comparaison exacte $2P^2$ contre
$JB^2$ en U320 (produits ~212 bits). Statut : `recu_auditeur_v4` (dérivé
par l'auditeur, `AUDIT_CIBLE_A524020_AXIAL_ARBRE_ET_COEUR_DE_SEED_20260817.md`
§ 1 ; « le cœur de seed de Jung reste exact »,
`CONTRE_AUDIT_879B37_BORNE_FLOTTANTE_DYNAMIQUE_20260818.md` verdict) ;
mutant v5 `q4-seed-core-nonstrict`.

### 6.6 Préfiltres exacts du bien-centrage

Conditions **nécessaires** du bien-centrage strict, en seules longueurs
carrées $l_{ij}$ ; un rejet est définitif, une survie ne prouve rien ; la
frontière (égalité) est rejetée par le contrat strict.

**Étage i64.** Soit $c$ le centre strictement intérieur, $u_i = p_i - c$,
$\left\Vert u_i \right\Vert = R$, $\sum_i \lambda_i u_i = 0$ avec
$\lambda_i > 0$, et $D^2 = l_{ab}$ l'arête maximale (une corde : $D \leq 2R$).

- *Sommet* : en multipliant la relation par $u_i$, tous les $u_i \cdot u_j$
  ($j \neq i$) ne peuvent être $\geq 0$ ; il existe $j$ avec
  $u_i \cdot u_j < 0$, donc $l_{ij} = 2R^2 - 2 u_i \cdot u_j > 2R^2 \geq D^2/2$.
  Pour $a, b$ c'est $ab$ ; pour $x$ c'est impliqué par l'acuité du seed ;
  la seule vérification nouvelle : $2 \max(l_{ay}, l_{by}, l_{xy}) > D^2$.
- *Paire* : $x, y$ ne sont pas antipodaux (sinon $c$ serait le milieu de
  $xy$, sur le bord) ; avec $w = u_x + u_y$,
  $w \cdot u_x = w \cdot u_y = 2R^2 - l_{xy}/2 > 0$ ; comme
  $\sum_i \lambda_i (w \cdot u_i) = 0$, l'un de $a, b$ vérifie
  $w \cdot u_z < 0$, soit $l_{xz} + l_{yz} > 4R^2 \geq D^2$ :
  $\max(l_{ax} + l_{ay}, l_{bx} + l_{by}) > D^2$.

Statut : `recu_auditeur_v4` (dérivés par l'auditeur,
`ADDENDUM_742035_Q4_DEUX_FILTRES_I64_AVANT_Q3_POWER_20260819.md` § 1–2 ;
audit du 22 août § 3.2 : « conditions nécessaires sûres », aucune fausse
mort).

**Puissance de face (lemme équatorial).** Un tétraèdre non dégénéré est
strictement bien centré **ssi** chacun de ses sommets est strictement
extérieur à la boule équatoriale de la face opposée (la boule dont le grand
cercle est le cercle circonscrit de la face). Chaque face seule est donc une
condition nécessaire. *Preuve.* Face $F$ de centre circonscrit $o_F$ (dans
son plan), rayon $R_F$, normale unitaire $n$ ; sommet opposé
$d = d_0 + h\,n$ ($h \neq 0$) ; centre du tétraèdre $o = o_F + t\,n$.
L'équidistance $\left\Vert o-d \right\Vert^2 = \left\Vert o-a \right\Vert^2 = R_F^2 + t^2$
donne
$\left\Vert d - o_F \right\Vert^2 - R_F^2 = 2th = \mathrm{Pow}_F(d)$ ; « $o$
du même côté que $d$ » ⟺ $th > 0$ ⟺ $\mathrm{Pow}_F(d) > 0$. Mieux : la
coordonnée barycentrique de $o$ associée à $d$ est $\lambda_d = t/h$
(projection sur $n$ de $\sum_i \lambda_i p_i = o$, les trois autres sommets
ayant une coordonnée normale nulle), donc
$\mathrm{Pow}_F(d) = 2 \lambda_d h^2$ : le signe de la puissance **est**
l'un des quatre signes barycentriques du test d'arité stricte, et la
frontière $\mathrm{Pow}_F = 0$ ⟺ $\lambda_d = 0$ ⟺ centre dans le plan de
la face. Pour la face seed $abx$ et le site $y$, cette puissance est, à un
facteur $G > 0$ près, la forme de Gram du seed évaluée en $y$ : aucune
primitive nouvelle (`q4_face_power_prefilter` dans `q4.hpp` appelle
`q3_power`). La formule « en six longueurs » de la v4 est un oracle croisé,
jamais un chemin de production. Statut : `recu_auditeur_v4`
(`CONTRE_AUDIT_5B89BC_Q4_REUTILISER_Q3_POWER_20260819.md` verdict et § 1 ;
`REPONSE_CLAUDE_5B89BC6_PREFILTRE_EQUATORIAL_20260819.md` § 1 pour la
re-dérivation ; audit du 22 août § 1 : « reçu mathématiquement et par
tests »).

`mesure` (v4, `uniform n=8000`, `ADDENDUM_FRONTIERE_STRICTE_ET_ETAGE_I64_20260819.md`) :
sur 87 499 759 paires entrantes, l'étage i64 en retire 15,3 % avant tout
i128, la puissance de face 43 921 721 de plus ; 80,65 % des rejets du
centre, zéro faux rejet ; la frontière n'apparaît que 17 fois sur les
emprises par défaut (d'où un nuage de porte à 200 points dans $14^3$ sites,
où elle apparaît 1 524 fois). Deux compteurs obligatoires : `faux_rejets`
(garde trop agressive) ET `frontieres_manquees` (garde trop permissive).
Gain de temps : ×1,055 (cascade contre rien) ; l'étage i64 seul n'a
**aucun gain de temps mesurable** (médiane appariée 1,0021, 8/20) — il est
conservé sur un argument compté (~40 M de multiplications i128 en moins),
pas chronométré.

### 6.7 L'étage flottant certifié

Code : `src/pipeline/float_filter.hpp`. Le flottant n'existe que comme
**filtre à repli exact** : une décision est prise seulement quand une borne
d'erreur prouvée certifie le signe ; sinon l'entier tranche. La correction
ne dépend jamais du filtre.

- **Programme sur la forme de Gram (reçu).** Pour le programme FMA figé
  évaluant $P = G \left\Vert v \right\Vert^2 - W \cdot v$ (une
  multiplication, trois FMA : quatre arrondis), en binaire64 au plus proche,
  sans `fast-math`, l'erreur totale est $< 6u\,M$ avec $u = 2^{-53}$ et $M$
  la somme des modules des termes, tandis que le seuil dynamique
  $E_f = 2^{-48} (G_d S_{max} + \left\Vert W_d \right\Vert_1 v_{max})$,
  $S_{max} = 2D^2$ (§ 6.1), $v_{max} = \sqrt{S_{max}} + 1$, reste
  $> 31u\,M$ : marge ×5, arrondis de $E_f$ absorbés. Décision :
  $\hat{P} < -E_f$ certifie $P < 0$, $\hat{P} > E_f$ certifie $P > 0$, sinon
  repli exact i128. Statut : `recu_auditeur_v4`
  (`CONTRE_AUDIT_879B37_BORNE_FLOTTANTE_DYNAMIQUE_20260818.md` § 1–4 et
  `CONTRE_AUDIT_879B37_BORNE_FLOTTANTE_DYNAMIQUE_ET_GARDE_FOLD_20260818.md`
  § 1–3 : « le coefficient $2^{-48}$ peut être reçu tel quel »).
- **Forme affine par ancre.** Par ancre : $u_z = 2z-a-b$,
  $q_z = \left\Vert u_z \right\Vert^2 - D^2$, entiers $< 2^{36}$, exacts en
  binaire64 ; par seed : $N = W - G\,d$ ($\left\vert N \right\vert < 2^{87}$).
  Identité $L(z) = G q_z - 2 u_z \cdot N = 4P(z)$ (preuve : $P(a) = P(b) = 0$
  ⟹ $d \cdot N = 0$ ; développer), donc $P = L/4$ exactement (divisibilité).
  Séquence figée `affine_l_hat` (une multiplication, trois FMA, un doublement
  exact) ; seuil par seed $E = 2^{-48} (G q_{max} + 2 \left\vert N \right\vert_1 u_{max})$ ;
  repli affine exact i128 ($\left\vert L \right\vert < 2^{105}$). Statut :
  identité `derive_v4_non_recu` gravée par porte (`mhgp5_q3_affine` :
  identité + divisibilité, témoin de forte annulation
  $G = 2^{67} - 12345$, $L = +216577 / -45565$) ; borne d'erreur de cette
  variante `derive_v4_non_recu` (`ADDENDUM_KERNEL_AFFINE_20260818.md`,
  reçu d'implémenteur : la structure d'erreur reçue pour le programme de
  Gram est transposée, pas re-reçue). **Abaissé** par rapport à la
  passation v4, qui présentait la borne comme « reçue par les deux
  contre-audits 879B37 » alors que ceux-ci jugent le programme de Gram.
- **Intervalles de Jung.** Pour un site certifié $P < 0$
  ($P \in [(\hat{L}-E)/4, (\hat{L}+E)/4]$) et $J \geq 0$ encadré à
  $\pm 2^{-40}$, si les intervalles de $2P^2$ et de $J B^2$ se séparent, la
  décision (témoin / non-témoin) est certifiée ; sinon repli exact U320.
  Les égalités tombent TOUJOURS dans le repli. Statut : `derive_v4_non_recu`
  (`ADDENDUM_INTERVALLES_JUNG_20260818.md` ; l'auditeur a **prescrit** la
  forme — « Jung doit utiliser un intervalle, puis propager les bornes »,
  879B37 § 6 — sans recevoir la borne $2^{-40}$) ; mutant `jung-swap-bounds`
  tué par un témoin gravé à cheval sur la fenêtre. `mesure` v4 : ~1,35 G de
  comparaisons U320 tombent à 80 (`eight_clusters`) et 145 (`uniform`)
  replis.
- **Conditions d'exécution** (reçues) : binaire64, arrondi au plus proche,
  aucune réassociation ; filtre coupé à la compilation sous `__FAST_MATH__`
  et à l'exécution si `fegetround() != FE_TONEAREST` (borne $= +\infty$ ⟹
  repli intégral). Statut : `recu_auditeur_v4` (879B37 second contre-audit,
  § 4) ; porte `mhgp5_float_rounding`.
- Ce que le filtre ne certifie PAS : les quotients de niveaux et toute
  comparaison de niveau ; « réutiliser une constante qui a bien marché une
  fois n'est pas un théorème d'analyse numérique » (879B37 § 6).

### 6.8 Le poste cher : RLE puis census

Ordre obligatoire : formes positives → `BallKey` + représentant de niveau
→ tri/RLE par clé (arité minimale, puis plus petite représentation :
`src/pipeline/candidates.hpp`) → **passe count-only** par clé unique
(`ball_depth_at_least(h_{q_{min}})`, § 3.2 : $mn \geq 0$ élague — coquille
incluse ; $mx < 0$ ajoute tout le nœud en $O(1)$ — STRICT, à $mx = 0$ une
coquille serait comptée) → census complet $I_B / U_B$ par clé survivante
(coquille complète, supports inclus ; plafonds explicites, jamais une
troncature) → jonction. `census_calls = unique_BallKeys`, jamais un census
par support. Statut : `recu_auditeur_v4`
(`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md` § 1–3 ; audit
du 22 août § 6.1) ; mutants v5 `range-add-max-le-zero`,
`depth-threshold-minus-one`, `skip-full-census`, `shell-cap-before-depth`
(`src/pipeline/census.hpp`, `expand.hpp`).

---

## 7. Reconstruction de la forêt

### 7.1 L'objet et les clés

Pour chaque $K = 1..K_{max}$ : les sommets du K-graphe de Gabriel (Déf. 29)
sont les **facettes** (K-uplets triés de `PointId` — la `FacetKey`) des
K-simplexes de Gabriel ; chaque événement $\sigma = S \cup I$ (support
d'arité $q$, $\left\vert I \right\vert = d = K+1-q$ intérieurs, niveau exact
$\rho(\sigma)^2$) y met une clique au poids $\rho(\sigma)$. La forêt HGP =
le K-MST élagué (Théorème 5), rendue comme suite de (multi)fusions de
composantes avec niveaux. $K_{max} \leq 10$ borne $d$ par lane exactement
comme les seuils $h_q$ (q2 : $K = d+1$, q3 : $d+2$, q4 : $d+3$). Statut :
`theoreme_manuscrit` (squelette) ; code `src/forest/fold.hpp`.

### 7.2 Rôles des facettes : actives contre attachements

Une facette de $\sigma$ est **active** ssi son rayon de naissance est
STRICTEMENT inférieur à $\rho(\sigma)$. Règle exacte (plateaux compris) :
pour $\sigma = I \cup T$, retirer un intérieur $z \in I$ garde la même
miniboule — la facette naît AU niveau (**attachement**) ; retirer $v \in T$
est actif ssi $c \notin \mathrm{conv}(T \setminus \lbrace v \rbrace)$
(sinon la boule est conservée et la facette naît au niveau aussi). Sous
position générale ($T = S$ support minimal) : les $q$ retraits de support
sont actifs (Fait 12), les $d$ retraits d'intérieur des attachements.

**Chronologie du dendrogramme** : les enfants d'un nœud de fusion sont les
composantes présentes JUSTE AVANT le niveau — donc les racines pré-lot des
facettes actives (ou préexistantes) seulement. Une facette née dans le lot
reste dans la fermeture union-find (elle appartient à la composante) mais
n'est JAMAIS un enfant absorbé : compter autrement gonfle les arités et
fabrique des nœuds fantômes alors que les partitions restent justes.
Contre-fixture `q2_one_interior_attachment` :
$\lbrace (0,0,0), (4,0,0), (2,1,0) \rbrace$, $K = 2$, nœud correct à 2
enfants, jamais 3. La v5 unionne les $K+1$ facettes (équivalent clique,
Déf. 29) et MESURE deux invariants (toujours 0) : un attachement déjà vu dans
un lot antérieur, une facette à la fois active et attachement au même
niveau. Statut : `recu_auditeur_v4`
(`AUDIT_CIBLE_1310B21_FACETTES_NEES_DANS_LE_LOT_20260817.md` ;
`AUDIT_CIBLE_5A08AB6_NAISSANCES_ET_CROISSANCES_DE_COMPOSANTES_20260817.md`
verdict) ; mutant v5 `attach-prebatch`.

### 7.3 Le payload hiérarchique : `ComponentDelta`

Une vue merge-only ne suffit pas : un K-polyèdre est une composante **comme
ensemble de K-facettes**, et ajouter une facette sans fusionner modifie le
polyèdre. Pour un niveau exact $\lambda$ et une composante finale $C$
touchée par le lot, avec $P(C)$ les composantes distinctes présentes juste
avant $\lambda$ aboutissant dans $C$ et $N(C)$ les facettes nées exactement
à $\lambda$ dans $C$, trois transitions existent :

```text
|P(C)| = 0, N(C) non vide  -> NAISSANCE (la composante apparaît entière) ;
|P(C)| = 1, N(C) non vide  -> CROISSANCE (le polyèdre absorbe des facettes) ;
|P(C)| >= 2                -> (MULTI)FUSION, éventuellement avec facettes nées.
```

Un delta est émis pour chaque racine post-lot touchée dès que
$\left\vert P(C) \right\vert \neq 1$ ou $N(C) \neq \emptyset$ : (lot, niveau
exact, identifiant canonique post-lot, parents = canoniques pré-lot, nés =
attachements ni préexistants ni actifs). L'identifiant canonique d'une
composante est **la plus petite `FacetKey`** de la composante (en v5 : le
plus petit `fid`, les `fid` étant attribués en ordre de `FacetKey`
croissante, ce qui rend les deux définitions égales), maintenue à travers
les unions — déterministe, mais équivariante par BLOCS sous un relabeling
des `PointId`, pas point à point (un minimum ne commute pas avec une
bijection non monotone). Les nœuds de fusion sont une VUE DÉRIVÉE des deltas
à $\geq 2$ parents ; les facettes nées sont membres pleins de
$F_K^{render}$. Fixtures gravées : le carré cocyclique en $K = 3$ est une
NAISSANCE (0 parent, 4 nées) ; `q2_one_interior_attachment` une fusion à 2
parents + 1 née ; la croissance unaire
$\lbrace (8,10,10), (12,10,10), (10,11,10), (10,13,10) \rbrace$ donne au
niveau 4 un delta à 1 parent + 1 née ($\lbrace a,b \rbrace$) sans aucun nœud
de fusion. Le mutant `drop-nonmerge` laisse les partitions justes et meurt
sur les fixtures de naissance et de croissance. Statut : `recu_auditeur_v4`
(`AUDIT_CIBLE_5A08AB6_NAISSANCES_CROISSANCE_ET_RENDU_20260817.md` § 1–3 ;
`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md` verdict ;
canonique min-`fid` :
`CONTRE_AUDIT_879B37_BORNE_FLOTTANTE_DYNAMIQUE_ET_GARDE_FOLD_20260818.md`
§ 6 ; audit du 22 août § 3.4).

### 7.4 Macro-lots

Les événements sont triés par niveau EXACT (`compare_exact_level` U320
après promotion — jamais l'égalité de représentation). Un **macro-lot** = un
groupe maximal de niveaux sémantiquement égaux : racines gelées avant le lot,
toutes les unions du lot appliquées ensemble, puis UN nœud de dendrogramme
par racine finale ayant absorbé plusieurs composantes pré-lot — aucune
chronologie binaire artificielle. La partition résultante est indépendante
de l'ordre interne (clôture d'union-find) ; le nombre de nœuds par lot
aussi. Statut : `recu_auditeur_v4`
(`AUDIT_Q4_LEMME_PREFIXE_ET_NIVEAU_20260817.md` § 2 ; audit du 22 août
§ 3.4) ; mutants v5 `repr-ties`, `binary-ties`.

### 7.5 Plateaux sphériques hors position générale

**Le refus des coquilles ne suffit pas.** Supprimer les événements à
coquille puis construire la forêt sur le sous-flux régulier NE rend PAS la
hiérarchie exacte : contre-fixture du carré cocyclique
$(110,100,100), (100,110,100), (90,100,100), (100,90,100)$ (sphère de centre
$(100,100,100)$, $R^2 = 100$). Les quatre triangles rectangles y sont de
Gabriel (Déf. 28 : la boule OUVERTE est vide — un point externe SUR la
sphère est permis) et fusionnent les quatre côtés de $\Gamma_2$ au niveau
100 ; le sous-flux régulier n'émet rien (supports q2 à coquille refusés,
aucun triangle aigu) et laisse quatre composantes. Le défaut change les
composantes HGP, pas une convention de rendu. Statut : `recu_auditeur_v4`
(`AUDIT_BLOQUANT_2AA0C3A_COQUILLES_U16_AVANT_FORET_20260817.md` § 1).

**Théorème du plateau.** Pour une boule $B$ de centre $c$,
$I_B = X \cap \mathrm{int}(B)$, $U_B = X \cap \partial B$ : les K-simplexes
de Gabriel de miniboule EXACTEMENT $B$ sont les $\sigma = I_B \cup T$ avec
$T \subseteq U_B$, $\left\vert T \right\vert = K+1-\left\vert I_B \right\vert$
et $c \in \mathrm{conv}(T)$ (fermé). *Preuve (auditeur).* Gabriel force
$I_B \subseteq \sigma$ ; les autres sommets sont sur la coquille ; une boule
est la miniboule de son ensemble ssi son centre est dans l'enveloppe convexe
de ses points de bord ; réciproque directe (les points omis sont extérieurs
ou sur la coquille). Sous position générale $U_B = S$ et la règle
$K = \left\vert I_B \right\vert + q - 1$ des lanes est retrouvée ; sans
elle, des points supplémentaires de $U_B$ produisent des simplexes d'ordres
plus élevés au même niveau. Par Carathéodory (dimension 3, `theoreme_externe`),
$c \in \mathrm{conv}(T)$ ⟺ $T$ contient un support minimal de cardinal 2,
3 ou 4 (paire diamétrale, triangle fermé, tétraèdre fermé contenant $c$) :
**les lanes q2/q3/q4 restent les générateurs locaux**, mais publient un
QUOTIENT commun par boule, jamais des événements isolés qui ignorent le
reste de la coquille. Statut : `recu_auditeur_v4` (même audit, § 2 ;
`AUDIT_CIBLE_5A08AB6_NAISSANCES_CROISSANCE_ET_RENDU_20260817.md` verdict).
Code : `src/forest/plateau.hpp` (arithmétique de production i128, entier
signé 192 bits pour les barycentriques du triangle, produits $< 2^{140}$ —
largeur `derive_v4_non_recu`).

**Q5 tranchée — option A** : l'objet normatif EST le nuage u16 (le profil
`quantized_u16_input_only` l'impose ; l'option B — coordonnées de vérité
plus fines, u16 réduit à l'index — serait un AUTRE profil à nommer et
re-dimensionner). Conséquences :

- l'ABI commune est le plateau : `BallKey`, niveau exact, $I_B$ complet,
  $U_B$ COMPLET (supports inclus), supports minimaux ; une seule passe de
  census par `BallKey`, collectant $I_B$ ET $U_B$ — plus jamais un booléen
  « coquille > 0 » ;
- régime régulier ($\left\vert U_B \right\vert = q$) : exactement UN
  $\sigma$ par $K$ ;
- régime dégénéré : le plateau est traité SIMULTANÉMENT pour chaque $K$ par
  la formule ci-dessus — voie **oracle bornée** (énumération des
  $T \subseteq U_B$, plafond de coquille explicite, `resource_exhausted`
  au-delà, jamais une troncature ni un décalage hors largeur : l'UB v4 à
  $\left\vert U_B \right\vert = 32$ relevé par l'audit du 22 août § 3.5 est
  un refus en v5). **Arbitrage V2**
  (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : la **compression par supports minimaux n'est pas
  approuvée** — l'énoncé « les $T$ admissibles sont les sur-ensembles de
  supports minimaux dans $U_B$, et les rôles actifs se lisent sur le support
  minimal contenu » n'est pas prouvé, et aucune porte indépendante n'établit
  qu'une compression conserve simultanément l'ensemble des événements, les
  rôles actifs, les naissances, les attachements et les niveaux ; une
  compression de représentation n'est pas une réduction de l'objet. Le seul
  comportement autorisé est le census complet de la coquille, un plafond
  explicite, puis `resource_exhausted` sans troncature ; **augmenter le
  plafond n'est pas une solution de complexité, et réduire la coquille est
  une perte silencieuse**. Aucune borne sur $\left\vert U_B \right\vert$ ne
  rend le régime « exact plutôt que refusé » : le plafond est un profil, pas
  un théorème. Ouvrir la compression exigerait, dans cet ordre : un énoncé
  précisant l'objet reconstructible et les rôles conservés ; un oracle
  exhaustif borné comparant l'expansion brute et la compression sur toutes
  les sous-familles d'une petite coquille ; des fixtures de plateaux
  cocycliques/cosphériques, d'attachement et de croissance unaire ; un
  mutant qui perd un support minimal et un mutant qui attribue un mauvais
  rôle actif ; un reçu de mémoire séparant sortie compressée, index de
  reconstruction et temporaires. Statut de la compression : `ouvert` comme
  chantier de preuve, **fermé** comme option d'implémentation à ce pin ;
- le fold gèle les composantes avant le niveau puis applique ENSEMBLE tous
  les simplexes du plateau (le macro-lot § 7.4 : même boule ⟹ même niveau
  exact) ; un ordre binaire entre cosphériques serait aussi faux que leur
  suppression.

`mesure` (v4) : 837 coquilles q2 dès `uniform n=400` ; 345 806 points de
coquille pour 104 802 événements sur ce même nuage (v5) — la position
générale n'est pas une précondition pratique du profil u16.

### 7.6 La frontière d'identité : `PointId` contre rang géométrique

Contrat : `PointId` ≠ index dense ≠ rang de Morton. Le census, l'arbre radix
et l'expansion des plateaux vivent en indices de positions uniques ; la forêt
combinatoire vit en `PointId` EXTERNES fournis par l'appelant. La conversion
a lieu UNE fois, à la frontière événement → forêt, via le représentant du
bucket — jamais par un cast du rang (`src/pipeline/expand.hpp`, mutant
`dense-pointid`). La sortie publique est **équivariante à un relabeling**
$\pi$ des ids à positions fixes (les `BallKey` primitives sont aveugles aux
ids ; chaque clé publique devient $\pi(\mathrm{cle})$) et **invariante à une
permutation physique** des couples (id, position). La propriété décisive
est le relabeling, pas la seule permutation : sous positions distinctes une
permutation physique peut conserver le même ordre dense et masquer le
défaut. Porte à écrire en v5 : ids brouillés non monotones dépassant le
bit 31, bijection non monotone, permutation physique, et AUCUNE clé publique
hors de l'ensemble d'ids fourni. Statut : `recu_auditeur_v4`
(`AUDIT_BLOQUANT_E7E4D5E_POINTID_DANS_LE_FOLD_20260817.md` ;
`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md` verdict : « la
frontière est au bon endroit et la porte de relabeling est substantielle »).

### 7.7 Le rendu § 9.1 : $F_K^{render}$, multiplicités, naissances de facettes

Contrat gravé (§ 2) : $F_K^{render}$ = TOUTES les facettes distinctes de tous
les événements — les attachements nés au lot en sont membres pleins (le
carré $K = 3$ : quatre triangles, tous attachements ; un rendu active-only
serait VIDE alors qu'un K-polyèdre vient de naître) ; $F_K^{conn}$ = la
compression suffisante pour la connectivité. Connectivité et rendu sont deux
consommateurs distincts du même flux d'événements, aux macro-lots identiques.

**Multiplicités.** $S_\tau$ somme la contribution de CHAQUE K-simplexe
incident : pour une boule $B$, $\mathrm{mult}_B(\tau)$ est le nombre de
$T \subseteq U_B$ avec $\left\vert T \right\vert = K+1-\left\vert I_B \right\vert$,
$c \in \mathrm{conv}(T)$ et $\tau$ facette de $I_B \cup T$ ; la contribution
du plateau est $\Delta S_\tau = \mathrm{mult}_B(\tau) \cdot \psi(r_B)$. Une
compression par arbre couvrant est exacte pour $F_K^{conn}$ mais FAUSSE pour
le § 9.1 (mutant v4 `render-collapse-mult`, à reprendre en v5). Le rendu conserve l'objet
symbolique facette → (lot, multiplicité) dont tout $\psi$ décroissant se
déduit en aval ($S_\tau$, $T_x$, $m_\tau$, votes — Prop. 7). Fixture gravée
(v4) `plateau_render_multiplicity` : sur le carré $K = 2$, les quatre triangles
rectangles donnent EXACTEMENT 2 incidences à chaque côté et chaque diagonale
(6 facettes, 12 incidences, un lot).

**Naissances de facettes.** Le niveau de naissance d'une facette ne se
résume PAS au bit actif ni au niveau de sa première incidence : table
`FacetKey` → $\rho(\mathrm{facette})^2$ par MINIBOULE EXACTE de ses
$\leq 10$ points — ne jamais supposer qu'une facette est elle-même un
événement de Gabriel de l'ordre inférieur. Théorème utilisé : la miniboule a
un support de 2 à 4 points dont elle est la boule circonscrite, et toute
candidate CONTENANTE a un niveau supérieur ou égal — le minimum sur les
candidates contenantes suffit, sans test de convexité. Les candidates
énumérées la couvrent toujours : paires (diamétrales, toutes) ; triplets
STRICTEMENT aigus seulement (un support rectangle a sa circonscrite égale à
la diamétrale de l'hypoténuse, couverte par la paire ; un obtus n'est
jamais support) ; quadruplets non coplanaires (un support coplanaire
cocirculaire est couvert par un triplet aigu ou une paire). Fixture gravée :
côté du carré à $\rho^2 = 50$, diagonale à $100$ — le mutant
v4 `birth-from-events` (niveau de la première incidence : 100 pour les deux)
meurt ; à reprendre en v5.

Statut : `recu_auditeur_v4` (multiplicités :
`AUDIT_CIBLE_5A08AB6_NAISSANCES_CROISSANCE_ET_RENDU_20260817.md` § 4 ;
naissances : `AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md`
verdict, « je n'ai pas trouvé de contre-exemple » à la règle du minimum des
candidates contenantes ;
audit du 22 août § 3.4). Réserve de l'audit du 22 août : le rendu v4 n'était
appelé que dans le chemin de jugement et ne publiait pas $S_\tau$, $T_x$,
$m_\tau$, votes ni verticales dans un objet final — en v5 le rendu n'est
**pas encore livré** (`../audits/ETAT_COURANT.md`).

### 7.8 Poisson q2 et taille de la sortie

Sous un processus de Poisson homogène sans bord, pour une paire
$\lbrace a,b \rbrace$ avec $r = \left\Vert a-b \right\Vert$ et
$v_2 = \pi/6$ (volume de la boule diamétrale $/r^3$), le nombre $N_j$ de
paires dont la boule diamétrale ouverte contient exactement $j$ sites
vérifie, par Campbell–Mecke et le changement $t = \lambda v_2 r^3$,
$E[N_j]/E[n] \to 2\pi/(3v_2) = 4$ pour **chaque** $j \geq 0$. Chaque telle
paire est un événement q2 distinct de $K = j+1$ (§ 3.1). À $K_{max} = 10$ :
40 événements q2 par point ; les retraits d'intérieurs injectent
$4 \sum_{j=1}^{9} j = 180$ facettes nées distinctes par point, portant
$4 \sum_{j=1}^{9} j(j+1) = 1320$ identités `PointId` par point. À 30 millions
de points : 1,2 milliard d'événements q2, 5,4 milliards de facettes nées,
39,6 milliards d'identités u32, soit **158,4 Go** de seuls ids — minorant
théorique de la trace exhaustive avant q3, q4, niveaux, deltas et
verticales. Statut : théorème `recu_auditeur_v4` (dérivé par l'auditeur,
`CONTRE_AUDIT_0328_BORNE_POISSON_SORTIE_Q2_30M_20260817.md` § 1–3 ; audit du
22 août § 5) ; constantes de volume infini, terme de bord requis pour un
reçu cubique (`AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md` § 9).
Conséquence sur le contrat de sortie — **arbitrage V3**
(`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
27 août 2026) : aucun objet partiel (hiérarchie de connectivité seule par
$K$, partition finale à une coupe $r$, requêtes ciblées) ne s'appelle à lui
seul « hiérarchie HGP calculée » ; le flux par $K$ obéit au contrat de
payload versionné de `ARCHITECTURE.md` § 7, une coupe ciblée est un
**autre** payload versionné, et un flux physiquement émis avant le statut
terminal n'est recevable que marqué `provisional` et invalidable
atomiquement. `mesure` (v4, `uniform n=8000`) : ~391 événements tous ordres
par point.

---

## 8. Le juge indépendant

Spécification du juge (`oracle/`, **à écrire en v5** — `../audits/ETAT_COURANT.md` ;
jusque-là la seule autorité de correction v5 est la conformité au digest
v4, qui hérite des limites de la v4). Sur petits nuages ($n \leq 14$,
coordonnées bornées documentées — régime oracle T2) : énumération de TOUS les
sous-ensembles $\sigma$, miniboule par recherche de support propre
(paires/triplets/quadruplets, centre dans l'enveloppe, plus petite boule
contenante — arithmétique rationnelle à représentation volontairement autre
que la production, jugée par un selftest `mhgp5_obig_selftest`), Gabriel =
boule ouverte vide, coquille externe traitée par le théorème du plateau ;
puis le K-graphe du manuscrit (cliques COMPLÈTES) et un Kruskal propre à
lots. Comparaison : ensembles de sommets, partition après CHAQUE lot, nombre
de nœuds par lot, égalité des niveaux de lot en arithmétique croisée du
juge, `ComponentDelta` propres (rôles par rayons de naissance indépendants,
canon par minimum, unions propres) comparés SANS le champ de niveau (le
représentant n'est pas re-dérivable indépendamment sur un plateau), table
position → id reconstruite depuis les enregistrements d'entrée sans appeler
la conversion du sujet. Le juge n'est jamais un backend ni un chemin
produit. Statut : spécification `recu_auditeur_v4` (audit du 22 août § 3.4 :
« le juge de forêt réénumère indépendamment les miniboules en grands
entiers ») ; réalisation v5 : non livrée.

**À l'échelle ($n = 8000/16000/32000$ et au-delà) — arbitrage V4**
(`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
27 août 2026) : il n'existe pas d'invariant unique qui transformerait une
régression en preuve d'exactitude, et aucun des contrôles proposés (niveaux
de naissance recalculés par miniboule sur un échantillon, monotonie des
partitions entre $K$ et $K+1$ par le Théorème 2) ne suffit seul. La porte
d'échelle combine des autorités causalement distinctes :

1. conformité différentielle v4/v5 sur mêmes entrées, digests et codes ;
2. $K = 1$ contre un calcul indépendant de single-linkage/MST ;
3. rejeu intégral des deltas et vérification de la partition finale pour
   chaque $K$ ;
4. échantillon déterministe de boules et d'événements, rejugés par
   miniboule, census et niveau indépendants ;
5. invariants verticaux entre $K$ et $K+1$ dès que les applications
   verticales existent ;
6. planchers de non-vacuité pour chaque famille, arité, type de rôle et
   chemin de rejet ;
7. reçu complet : pin, worktree, toolchain, commande, hashes d'entrée et de
   sortie, codes, compteurs, RSS et statut terminal.

La conformité v4 est une porte de **divergence**, le juge échantillonné une
porte de **falsification**, le rejeu structurel une porte de **cohérence** :
leur conjonction augmente la confiance, mais seuls l'oracle borné ci-dessus
et les preuves contractuelles autorisent le vocabulaire d'exactitude. La
question « quel invariant ? » est close ; la matrice reste à réaliser
(`PLAN_DE_TESTS.md`, `../audits/ETAT_COURANT.md`).

---

## 9. Questions ouvertes

Héritées de la v4 avec leur état :

- **Q1 (bijection événements-boules).** *Tranchée* sous sites distincts et
  position générale (§ 3.1, `recu_auditeur_v4`) ; le bord cosphérique est
  couvert par le théorème du plateau (§ 7.5). Les multiplicités de
  positions sont **hors profil** par l'arbitrage V1 (§ 2) : refus normatif ;
  toute extension pondérée serait une phase distincte.
- **Q2 (qualité du minorant de témoins).** `ouvert`. Ce qui est reçu : la
  loi conditionnelle et la constante $2\pi h/(3v_q)$ pour une région
  déterministe (§ 4.9). Ce qui manque : le passage à la WSPD aléatoire
  (stabilisation ou arbre pilote), la dépendance en $s$, le cas
  `eight_clusters`.
- **Q3 (complétude WSPD fail-open de bout en bout).** Reçus : la partition
  exacte des paires, la mort fail-open dans la descente, le lemme de
  complétude sous $h_q$ (§ 3.2, § 5), les instructions q3/q4 sur le domaine
  jugé (audit du 22 août § 3.3 : « complétude fonctionnelle crédible »).
  `ouvert` : le théorème écrit d'un seul tenant — « toute ancre d'événement
  peu profond est l'arête maximale d'un support dont les deux extrémités
  tombent dans exactement un rectangle vivant, et aucune étape d'élagage ni
  d'identification ne perd un support » — et la borne $O(s^3 n)$ de la
  variante implémentée (§ 5).
- **Q4 (convention $F_K$ du rendu).** *Tranchée* : $F_K^{render}$ = toutes
  les facettes distinctes (§ 2, § 7.7, `recu_auditeur_v4`). La proposition
  v4 « facettes actives seulement » est **retirée** — l'audit du 22 août
  (§ 7.2) a relevé qu'elle contredisait la règle reçue. Reste à déclarer,
  jamais à supposer : la convention accompagne tout rendu publié, et le
  rendu v5 n'est pas livré.
- Q5 (ex æquo) et Q12 (forme du niveau q4) restent tranchées (§ 7.5, § 6.4).

Verrous posés dans la question historique publiée au pin `f9b4d7b6` et
**arbitrés** le 27 août 2026
(`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`, pin
jugé `87e915bd`). Les quatre restent fermés pour tout claim ; aucun
arbitrage n'est une promotion :

- **V1 — positions dupliquées** : *tranché*. Le refus reste normatif (§ 2) ;
  le HGP pondéré n'est ni défini ni prouvé et ne pourrait entrer que comme
  phase distincte (définition de $\rho(\sigma)$ et de
  $\left\vert I_B \right\vert$ pour un multi-ensemble, supports de diamètre
  nul, oracles et reçus propres), jamais comme optimisation silencieuse du
  profil. Reste à livrer, pas à trancher : la cohérence du refus à toutes
  les frontières (§ 2).
- **V2 — plateaux sphériques** : *chantier de preuve, fermé comme option
  d'implémentation*. La compression par supports minimaux n'est pas
  approuvée ; l'énumération des $T \subseteq U_B$ sous plafond explicite
  puis `resource_exhausted` sans troncature est le seul comportement
  autorisé, et aucune borne sur $\left\vert U_B \right\vert$ ne rend le
  régime exact plutôt que refusé (§ 7.5, avec les cinq portes préalables à
  toute compression).
- **V3 — contrat de sortie à 30 M de points** : *tranché par un contrat,
  non par un objet*. Le flux symbolique complet reste impossible (§ 7.8) ;
  aucun des trois objets proposés (hiérarchie de connectivité par $K$,
  partition à une coupe, requêtes) ne s'appelle seul « hiérarchie HGP
  calculée ». Le flux par $K$ déclare version de représentation, niveaux de
  lots, deltas (parents, naissances, représentant de sortie), partition
  finale ou certificat de reconstruction, politique de rétention des
  facettes, applications verticales si l'objet revendiqué est la tour, et
  statut terminal global avant toute publication ; une coupe ciblée est un
  **autre** payload versionné ; une émission avant le statut terminal exige
  un marquage `provisional` invalidable atomiquement, que l'API actuelle ne
  porte pas encore (`ARCHITECTURE.md` § 0 et § 7 ; § 2 ci-dessus).
- **V4 — le juge indépendant à l'échelle** : *tranché* : pas d'invariant
  unique en $O(n \log n)$ qui ferait d'une régression une preuve, mais la
  matrice d'autorités causalement distinctes du § 8 (conformité v4/v5,
  $K = 1$ contre MST indépendant, rejeu des deltas et partition finale par
  $K$, échantillon rejugé, invariants verticaux quand ils existent,
  planchers de non-vacuité, reçu complet). L'égalité au digest v4 prouve
  « même objet que la v4 », pas « objet exact » : c'est une porte de
  divergence.

---

## Annexe — fixtures gravées (coordonnées exactes)

Reprises de la v4 (chacune requalifiée par une porte v5, `PLAN_DE_TESTS.md`
§ 4) :

- carré cocyclique $(110,100,100), (100,110,100), (90,100,100), (100,90,100)$ —
  plateau $K = 2$ (4 triangles rectangles, 2 incidences par côté et par
  diagonale), naissance $K = 3$ ; naissances de facettes : côté
  $\rho^2 = 50$, diagonale $100$ ;
- `q2_one_interior_attachment` $\lbrace (0,0,0), (4,0,0), (2,1,0) \rbrace$ —
  $K = 2$, fusion à 2 parents + 1 née ;
- croissance unaire $\lbrace (8,10,10), (12,10,10), (10,11,10), (10,13,10) \rbrace$ —
  niveau 4, 1 parent + 1 née ;
- cœur q4 exact contre sous-approximation : $a = (10000,10000,0)$,
  $b = (20000,10000,0)$, $z = (15000,12585,0)$ ;
- « dix témoins q2 dans la boule diamétrale qui ne ferment pas q4 » :
  $a = (100,100,100)$, $b = (200,100,100)$, $x = (150,30,120)$,
  $y = (150,30,80)$, $z_i = (150+i, 140, 100)$ ;
- arrondi plancher du rayon : q3 $a = (0,0,0)$, $b = (14,0,0)$, $z = (7,1,4)$ ;
  q4 $a = (0,0,0)$, $b = (8,0,0)$, $z = (4,1,2)$ ;
- fixture q4 13 points (ancre q3-morte, q4-vivante, tétraèdre de profondeur
  0) et sa version 22 points (deux faces restantes q3-profondes) ;
- témoin de forte annulation de l'identité affine : $G = 2^{67} - 12345$,
  $L = +216577$ et $-45565$ ;
- contre-familles `collinear_seven` (crédit de groupe sans disjonction
  d'identités), `two_lines` (un point colinéaire entre $a$ et $b$ satisfait
  $W_3$ et $W_4$ puisque $\Xi = 0$ et $H > 0$ : porteurs et témoins ne se
  confondent pas) ; « tétraèdre aux six arêtes égales » (départage de
  l'owner par `EdgeKey`).

## 10. Tests d'ancre : géométrie des centres, W_q exact et témoins sectoriels (27 août 2026)

Statut : `derive_claude` — preuves ci-dessous, fixtures gravées, **verrous posés à l'auditeur** (`../audits/QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`) ; rien n'est promu. Analyse complète (quatre rapports, dont un contradicteur) : `analyses/seeds_20260827/`.

**Cadre.** Ancre $(a,b)$, $d = b - a$, $D^2 = \left\vert d \right\vert^2$, $m = (a+b)/2$ ; pour un site $z$, $u = 2z - a - b$ et $q = \left\vert u \right\vert^2 - D^2$ (les « sites affines » de la lane) ; $w = z - m = u/2$.

**Lemme 10.1 (centres).** Toute boule-candidate de l'ancre a son centre $c$ dans le plan bissecteur de $ab$ ; avec $v = c - m \perp d$ : $R^2 = \left\vert v \right\vert^2 + D^2/4$. Pour un seed q3 (triangle $abx$ aigu, $ab$ arête la plus longue), l'angle en $x$ est dans $[60^\circ, 90^\circ)$ et $\left\vert v \right\vert^2 \le D^2/12$ (égalité : équilatéral) ; pour q4 (Jung), $R^2 \le 3D^2/8$ donc $\left\vert v \right\vert^2 \le D^2/8$. En entiers : $N = W - G d$ de la lane vaut $2 G v$, et $\left\vert N \right\vert^2 = D^2 G (EX - G)$.

**Proposition 10.2 (demi-plan).** $z$ est strictement intérieur à la boule de centre $m+v$ ssi $2\, w \cdot v > \left\vert w \right\vert^2 - D^2/4$, soit $4\, u \cdot v > q$ ; le kernel affine de production $L = G q - 2\, u \cdot N$ vaut exactement $-G \cdot (4 u \cdot v - q)$. Pour $z$ fixé, l'ensemble des centres qui le rendent intérieur est un demi-plan ouvert $H_z$ du plan bissecteur : **la mort d'un seed est la profondeur de $v(x)$ dans l'arrangement des $m$ demi-plans** ($\ge h_q$). Corollaire : $\left\vert W_q \cap P \right\vert \le \min_{v} \text{depth}(v) \le \text{depth}(0)$, et $z \in W_3 \iff q < 0 \wedge 3 q^2 > 4 \left\vert d \times u \right\vert^2$ (c'est `in_spindle(kQ3)`).

**Constat.** La lane q3 ne comptait pas $\left\vert W_3 \cap \text{cover} \right\vert$ (seul l'histogramme de coin, comme en v4) ; la lane q4 comptait $W_4$. Le compte exact est suffisant et l'objet est inchangé (tout $z \in W_3$ est intérieur à toute boule admissible : chaque seed a $\ge h_3$ intérieurs et est déjà tué à la génération) ; il retire 57 % des ancres et 92,6 % des seeds à `eight_clusters` 8000.

**Théorème 10.3 (témoins sectoriels).** Soit un polygone convexe $\Pi$ à sommets entiers du plan bissecteur, contenant le disque des centres, et $(0, p_k, p_{k+1})$ ses $K$ triangles. Si pour chaque $k$ au moins $h$ sites vérifient $4\left\vert w \right\vert^2 < D^2$, $8\, w \cdot p_k > 4\left\vert w \right\vert^2 - D^2$ et $8\, w \cdot p_{k+1} > 4\left\vert w \right\vert^2 - D^2$, alors toute boule-candidate a $\ge h$ sites strictement intérieurs et l'ancre n'émet rien. *Preuve* : une forme affine atteint son minimum sur un triangle en un sommet ; tout centre est dans un triangle. Polygone employé : parallélogramme $(\pm u, \pm v)$ des deux plus grands produits $d \times e_i$, contenant le disque ssi $\left\vert u \times v \right\vert^2 \ge \rho^2 \left\vert u \mp v \right\vert^2$, puis l'octogone $(\pm u, \pm v, \pm(u+v), \pm(u-v))$ ($K = 8$). **Les deux tests sont incomparables** (fixtures F1/F3) et sont cumulés.

**Fixtures gravées** (`tests/anchor_kill_fixture.cpp`, `tests/sector_kill_fixture.cpp`, ancre $(0,0,0)$–$(2000,0,0)$, $h_3 = 9$) : F1 — 9 + 9 sites $(1000+e, \pm 900, 0)$ : $\left\vert W_3 \right\vert = 0$, tout seed mort, secteurs tuent ; F2 — 8 + 8 sites et le seed $x = (1000, 1200, 0)$ de profondeur 8 : la boule $\lbrace a,b,x \rbrace$ est émise, le mutant `anchor-kill-h-minus-one` la perd ; F3 — 9 sites $(1000+e, 550, 0)$ : $W_3$ tue, les secteurs non ; sphère diamétrale — mutant `sector-kill-nonstrict` tué. Les compteurs (`seeds`, `depth_killed`, `q3_cert`) changent, l'objet post-RLE non (conformités v4 égales).

**Théorème 10.4 (morceaux de corde, seed q4 ; `src/lanes/chord_kill.hpp`).** Pour un seed $(a,b,x)$ de q4, les centres des boules admissibles (tétraèdres $abxy$ bien centrés, $ab$ arête la plus longue, $R^2 \le 3D^2/8$) forment une corde du plan bissecteur : $c_\mu - m = v_3 + \mu\, n/(2G)$, $n = (b-a)\times(x-a)$, $\left\vert \mu \right\vert \le \mu^\ast = \sqrt{J/2}$, $J = D^2(3G - 2 l_{ax} l_{bx}) > 0$ (identité exacte $\left\vert v_3 \right\vert^2 + J/(8G) = D^2/8$). Un site $z$ est strictement intérieur à la boule de centre $c_\mu$ ssi $P(z) - \mu B(z) < 0$, $P = L/4$, $B = n \cdot (z-a)$ — affine en $\mu$. Le cœur de seed de Jung ($P<0$ et $2P^2 > J B^2$) est « témoin de toute la corde ». Avec $\hat\mu = \lfloor \sqrt{J/2} \rfloor + 1 \ge \mu^\ast$ et les sommets $\mu_j = (2j-4)\hat\mu/4$, $j = 0..4$, $z$ est témoin du morceau $i$ ssi $v_i(z) < 0$ et $v_{i+1}(z) < 0$ avec $v_j = L - (2j-4)\hat\mu B$ (forme affine, minimum aux extrémités). Si chaque morceau compte $\ge h_4$ témoins, toute boule admissible du seed a $\ge h_4$ sites strictement intérieurs, donc toute complétion est tuée par le filtre de profondeur : le seed est mort, l'objet inchangé. Le test est cumulé avec le cœur (la corde élargie n'est pas plus forte aux extrémités). Filtre flottant certifié ($L \in [\hat L - E, \hat L + E]$, produit $(2j-4)\hat\mu B$ à garde relative $2^{-40}$) et repli exact `__int128` ($\left\vert v_j \right\vert < 2^{110}$). Mesure (sonde contrefactuelle, `receipts/mesures_secteurs_635951d6_20260827/q4_chord_*`) : `eight_clusters` 2000, K = 4 : 62 % des seeds vivants après le cœur tués, 82,5 % des complétions évitées, 0 faux positif ; fixture F8 (site coplanaire et cosphérique : $v_j = 0$ partout) et mutant `chord-nonstrict` ; oracle ON/OFF et conformités v4 inchangées.

**Politique « prétests avant le cover » (sans effet sur l'objet).** Les deux tests d'ancre ne lisent que la boule diamétrale ouverte ; sur un rectangle dense (≥ `kPretestQueryMinPoints` = 512 points de handles), ils sont exécutés sur une requête d'arbre de coefficient 1 non triée, et le cover complet (coef 3, trié en 32 classes) n'est construit que pour les ancres survivantes. Les prétests lisent les **candidats diamétraux du rectangle** (`rect_diametral_candidates` : une traversée par rectangle des points $z$ avec $\text{dist}(2z, \text{Box}(A)+\text{Box}(B))^2 \le D_{\max}^2$, sur-ensemble de la boule diamétrale de chaque ancre du rectangle). Profil q4 `eight_clusters` 4000 (1 fil, ratios) : covers 21,1 s → candidats 4,3 s + covers 1,3 s ; total lane 31,0 s → 14,1 s ; `uniform` 7,5 → 7,1 s ; verdicts identiques quelle que soit la politique (conformités v4, portes de lane, oracle). À 8000, huit fils (ratio) : génération `eight_clusters` 71,8 s en début de journée → 16,6 s.

**Ce qui est inhérent.** Le scan par seed est au plancher (≈ 9,6 évaluations par seed mort pour $h_3 = 9$) ; après les tests d'ancre et de seed, les postes dominants sont la construction du cover par ancre (`anchor_cover_from_handles`) et, en q4, le scan complet du cover pour les seeds qui survivent — le levier suivant est au niveau du rectangle, pas du seed. Lemme q4 reçu du contradicteur : $J = D^2(3G - 2 l_{ax} l_{bx}) = G (D^2 - 8 \left\vert v_3 \right\vert^2) \ge G D^2/3 > 0$ pour tout seed aigu — la branche $J < 0$ de la lane est inatteignable (garde).

**Mesure G4 (session 9, pin `5c777be3`, reçu `receipts/campagne_g4_v5_20260827_corde_pretests`, 48 fils, comparaison appariée avec la session 8).** `eight_clusters` : 50 k 162 → 82 s, 100 k 440 → 185 s, 200 k 1457 → 443 s (lane q4 758 → 84 s, q3 383 → 45 s) ; `uniform` et `terrain` inchangés (leur mur est le fold) ; `scanline` 100 k 89 → 57 s, 200 k 503 → 499 s. Digests `digest_balls` et `digest_all` identiques CPU / `--gpu` sur les quatre familles : l'objet est inchangé aux tailles de contrat. **Limite constatée (`scanline`).** Sur une ancre $(a,b)$ prise entre deux lignes de balayage (pas $8$, $D \approx 8$–$16$), la boule diamétrale ouverte est vide de points (les lignes sont à distance $\ge D/2$ de $m$) ; or chaque secteur du théorème 10.3 contient l'apex $v = 0$, dont la condition $4\left\vert w \right\vert^2 < D^2$ n'a alors aucun témoin : l'ancre survit à tous les tests d'ancre bien que chacun de ses seeds meure (cœur ou corde) après un balayage de 83 à 218 sites. À l'échelle, ces ancres croissent en $n^{1{,}86}$ et le balayage cœur/corde en $n^{2{,}9}$ (10 G → 431 G itérations de 50 k à 200 k) alors que les candidats émis sont linéaires. Piste ouverte (à prouver avant d'être codée) : polygones sans apex — disque intérieur $\left\vert v \right\vert \le \rho_0$ et secteurs annulaires à quatre sommets entiers — dont la condition aux sommets reste affine en $v$ ; et, pour les seeds, un index par ancre des sites dans le dual $(\theta_z, t_z)$ du plan bissecteur ($z$ tue la boule de centre $v$ ssi $v \cdot \hat w_\perp > t_z$), qui rend les témoins d'un seed en temps proportionnel à leur nombre plutôt qu'au cover.

**Théorème 10.5 (grille de cellules sans apex ; mesure `bench/rect_probe.cpp`, palmarès).** La sonde des rectangles sur `scanline_single_pass` 16 000 (pin `d86b4ec7`) désigne le gaspillage : en q3, 1 % des rectangles portent 80 % des seeds et 0,2 % des survivants — la production ($W_3$ exact + secteurs) y tue toutes les ancres ; en q4, 1 % des rectangles portent 74 % des seeds, **95,7 % des seeds sont dans des rectangles sans aucun survivant**, et dans les plus lourds ($D_{\max} \approx 260$–$300$, ~200 ancres vivantes après $W_4$, ~1000 seeds par ancre, covers de 400 k à 900 k sites) les secteurs ne tuent presque rien (1 ancre sur 236, 0 sur 169, 0 sur 129) parce que chaque secteur contient l'apex $v = 0$ et que la boule diamétrale est vide. Énoncé : soit $P_1, P_2$ deux vecteurs entiers du plan bissecteur (ceux du théorème 10.3) et $G \ge 1$ ; la cellule $C_{ij}$, $-G \le i, j < G$, est le parallélogramme de sommets $p = (i' P_1 + j' P_2)/G$, $i' \in \lbrace i, i+1 \rbrace$, $j' \in \lbrace j, j+1 \rbrace$ ; l'union des cellules contient le disque des centres (rayon $\rho_q$, car $\left\vert P_k \right\vert \ge \rho_q$). Un site $z$, $w = z - m$, est *témoin de la cellule* ssi $8 G\, w \cdot (i' P_1 + j' P_2) > G (4 \left\vert w \right\vert^2 - D^2)$ aux quatre sommets (entier, exact : la condition « $z$ strictement intérieur à la boule de centre $m + v$ » est affine en $v$, donc vraie sur le parallélogramme ssi vraie aux sommets). Une cellule est *morte* ssi elle a $\ge h_q$ témoins. **Toute boule admissible dont le centre est dans une cellule morte a $\ge h_q$ sites strictement intérieurs**, donc est tuée par le filtre de profondeur : (q3) un seed $x$ dont le centre $c_0 = m + v_3$ est dans une cellule morte est mort ; (q4) un seed dont la corde (théorème 10.4) ne rencontre que des cellules mortes est mort — un sur-ensemble conservatif des cellules rencontrées (boîte des extrémités en coordonnées de cellule, arrondie vers l'extérieur) suffit. Les cellules vivantes (typiquement autour de l'apex) laissent leurs seeds au test par cœur et corde : la sortie est inchangée quelle que soit la grille. Coût par ancre : $(2G+1)^2$ produits scalaires exacts par site (une passe sur le cover, $G = 8$ : 289) puis, par seed, une localisation rationnelle sans balayage, au lieu de ~200–1000 sites par seed. **Implémentation (`src/lanes/cell_grid.hpp`, 28 août 2026).** Base entière $(u, v)$ partagée avec les secteurs (`bisector_basis`), $G = 8$ (256 cellules, 289 sommets), cellules nécessaires = celles dont la boîte rencontre le losange $\left\vert \alpha \right\vert + \left\vert \beta \right\vert \le 1$ (172 sur 256) ; un site compte pour une cellule ssi ses quatre sommets vérifient $4 w' \cdot (i' u + j' v) > G(\left\vert w' \right\vert^2 - D^2)$, $w' = 2z - (a+b)$ (entier, chemin i64 sous $2^{62}$ sinon i128) ; comme le membre gauche est monotone en $i'$ et le membre droit en $j'$, les sommets témoins d'une ligne forment un intervalle dont la borne se déplace de façon monotone d'une ligne à l'autre (deux pointeurs, ~50 comparaisons par site au lieu de 289) et les cellules témoignées d'une ligne forment un intervalle incrémenté par tableau de différences : **58 ns par site** (banc, 2000 sites ; 264 ns pour l'évaluation directe des 289 sommets). Localisation des centres par la matrice de Gram en binaire64 avec borne d'erreur absolue $\varepsilon = 2^{-46}\, G\, (\left\vert t_1 \right\vert + \left\vert t_2 \right\vert + \left\vert s_1 \right\vert + \left\vert s_2 \right\vert)/(\text{den} \cdot \det) + 2^{-40}$ : toutes les cellules nécessaires de $[\alpha G - \varepsilon, \alpha G + \varepsilon] \times [\beta G - \varepsilon, \beta G + \varepsilon]$ (q3 : $v_3 = N/(2G_3)$, $N = W - G_3 d$), respectivement de la boîte des deux extrémités $(N \pm \hat\mu n)/(2G_3)$ (q4), doivent être mortes ; une cellule hors grille ou hors losange ne contient aucun centre admissible. Politique (sans effet sur l'objet, identique sur tous les chemins) : grille ssi le cover atteint `cell_min_sites` = 256 (0 dans l'oracle : toute ancre), les seeds aigus atteignent cover/8 (q4) resp. cover/2 (q3 : un seed q3 coûte ~0,8 µs, la grille ne s'y justifie que sur les ancres très riches) et moins de $h$ sites sont à moins de $0{,}30\,D$ de $m$ (sinon les témoins de cœur sont trouvés dès les premières classes radiales) ; mesure locale 8 fils (ratios) : `scanline` 16 000 lane q4 4,86 → 4,41 s (86 k grilles, 3,1 M seeds tués sans balayage), `eight_clusters` 8000 neutre ; reçu G4 n° 11 (`receipts/campagne_g4_v5_20260828_grille`, 48 fils, comparaison appariée avec la session 10) : `scanline` 200 k **502 → 268 s** (lane q4 438 → 215 s, 990 888 grilles, 687 851 ancres et 277,9 M seeds tués sans balayage, itérations du cœur 431 G → 195 G), 100 k 54 → 40 s ; `uniform` et `eight_clusters` inchangés à ±3 % ; digests CPU / `--gpu` identiques ; corps de production q3/q4 et lanes par lots (les seeds tués ne sont jamais matérialisés), compteurs `ancres_cellules` / `seeds_cellules` comparés par les portes de lane. Fixture **F9** (`tests/anchor_kill_fixture.cpp`) : ancre `a=(800,0,0)`, `b=(2800,0,0)` au-dessus d'une vallée à fond plat ($h = -600$, parois de pente 6, remontée au-delà de $a$ et $b$) — 1111 sites dans la boule diamétrale, aucun à moins de $0{,}30\,D$ de $m$, $W_4$ et secteurs q4 impuissants, **172/172 cellules mortes**, ancre tuée sans énumérer un seed, contrefactuel : 298 seeds aigus tous morts, aucune boule. Oracle ON/OFF sur toutes les paires de cinq petits nuages avec seuil 0 (non-vacuité : seeds tués par cellules en q3 et en q4) ; mutant `cell-kill-h-minus-one` tué par l'oracle (code 4) ; mutant `cell-kill-nonstrict` tué par la fixture **F10** (13 sites entiers de la sphère diamétrale, $s^2 + t^2 = 10^6$, exactement sur la frontière $4 w' \cdot p = G(\left\vert w' \right\vert^2 - D^2) = 0$ des sommets $i' = 0$ : témoins de toute la colonne $i = 0$ pour le mutant, d'aucune cellule en strict) ; conformités v4 inchangées.
