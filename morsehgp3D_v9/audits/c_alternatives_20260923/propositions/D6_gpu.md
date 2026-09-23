# D6 — Chaîne GPU résidente de bout en bout sur la RTX PRO 6000 (sm_120). Les q3/q4 y sont localisés par tuiles, selon la forme D3 partagée par la longueur de l'arête propriétaire ; census et cibles statiques FULL tournent sur le GPU ; q2 à K10, arêtes longues et calendrier FULL restent sur les 48 fils

**Verdict : a_sonder**

## Principe

### Thèse

D6 n'apporte pas un nouvel algorithme de catalogue. C'est une architecture d'exécution.

- Le nuage (0,5 Mo) est téléversé une seule fois.
- L'index, q3/q4, la fusion, le census et les cibles statiques de FULL vivent sur la RTX PRO 6000 jusqu'au catalogue exact trié.
- Les 48 fils du G4 prennent ce qui est séquentiel, rare ou long : q2 en concurrence à K10, arêtes propriétaires longues, coquilles étendues, calendrier FULL.

D6 multiplie par un facteur de débit la forme algorithmique qu'on lui confie. Elle ne change aucun exposant. Elle ne dispense ni de D5 (calendrier FULL) ni des réductions de travail (certificat de bloc, D3).

### Ce que le dépôt a déjà mesuré sur GPU

Cinq tentatives ont eu lieu. Aucune n'a donné un gain de bout en bout supérieur à environ 10 %.

- **Frontière `prune-only` pilotée par l'hôte** (Phase 15, rapport scellé `c047e2f`) : 110,7 M visites LBVH en 3,229 s à 3 125 points, soit environ 34 M visites/s, avec 27 vagues et 47 synchronisations. Rejetée.
- **Frontière de paires device, 50 k, K5** : environ 1,0 s de lanceur, 74 ns par visite de nœud, 8 blocs sur 188 SM.
- **v5, sessions 7 à 13** : lanes q3/q4 device au mieux à parité avec le CPU.
  - Noyaux : environ 1 % du temps d'exécuteur.
  - Covers copiés à 60 o par site : 40 à 95 Go par lane.
  - Enfilement par 48 fils, 98 exécuteurs éphémères.
  - Seul débit utile : noyau q3, 0,20 s pour 87 M graines.
- **v6 C5** : census device en 154 ms de noyaux pour 21,6 M candidats. L'étage complet prend 7,7 s, dont 2,6 s de sérialisation et 4,1 s de reconstruction hôte mono-fil. Mur −10,4 %.
- **v7** :
  - census K10 en 189 ms de noyaux pour 21,47 M boules, phase complète 4,54 s ;
  - FULL resté sur CPU ;
  - primitives MEB et clé qualifiées en exactitude sur la carte, jamais en débit.

Règles déduites :

- résidence de bout en bout, sorties déjà dans leur disposition finale ;
- aucune vague pilotée par l'hôte ni synchronisation par étape (files device, graphes CUDA) ;
- jamais de cover copié ;
- occupation mesurée (registres des entiers larges) ;
- pas de FP64 sur le chemin chaud.

### La machine et le coût de l'exact

Caractéristiques du GB202 :

- 188 SM, environ 2,5 GHz (120 TFLOPS FP32 annoncés) ;
- 96 Go de GDDR7 à 1,6 To/s ;
- PCIe 5.0 x16 : environ 50 Go/s en mémoire épinglée ; v6 a observé 14 Go/s en H2D non épinglé ;
- par SM : 48 warps, 64 K registres, 100 Kio de mémoire partagée ;
- cœurs FP32/INT32 unifiés : environ 60 T instructions entières simples par seconde en crête, IMAD vraisemblablement au demi-débit ;
- FP64 au 1/64, soit environ 1,9 TFLOPS.

Coût des opérations entières :

| Opération | Instructions |
| --- | ---: |
| add i64 | 2 |
| produit i64 bas | 3 IMAD |
| produit 64×64→128 | 8 à 12 |
| produit i128 | 16 à 20 |
| comparaison de niveaux U192×U128 | environ 40 |

Un x86 fait le produit 64×64→128 en une instruction. Le GPU paie donc 3 à 6 fois plus d'instructions par prédicat large. D'où la décision arithmétique de D6 : réduire les largeurs par repère local, et n'employer le flottant que comme proposition vérifiée.

### Voie par voie

**Préparation et index.** Tri Morton (tri radix CUB), unicité, LBVH de Karras : tout se porte naturellement, en moins de 1 ms à 40 k sites. Aucun enjeu : 10 ms sur CPU aujourd'hui.

**q2.** Front WSPD s = 8, témoins universels, census ; 0,23 à 0,51 s sur CPU W48 à K5, 0,41 à 0,82 s à K10.

- Se porte naturellement : front double-arbre en largeur avec compaction par niveau, tests nœud contre boule diamétrale, census par parcours (noyaux v6/v7).
- Irrégulier : profondeur du front, masses de rectangles très inégales.
- Forme favorable : l'actuelle, en largeur, frontière gardée sur le device.
- Placement : q2 est indépendante de q3/q4. Elle reste sur CPU en concurrence à K10, où elle a le même ordre de grandeur que q3/q4 sur GPU. Elle passe sur GPU à K5.

**q3/q4.** 69 à 74 % du mur à K5, 55 à 64 % à K10 (R7b).

Se porte naturellement tout ce qui est « formes affines d'un cover » : 3,8 à 6,1 G tests uniformes du certificat de voie morte à K10, tests ponctuels, chargement des formes.

- Un compte saturant « au moins T intérieurs stricts » est un `popc` de ballot.
- Une frontière est un masque de bits aligné sur les warps : 374 sites en moyenne tiennent en 12 mots.
- L'arrêt anticipé se reproduit exactement, au mot près.

Irrégulier :

- (a) **Parcours d'index par requête.** À K10/000100 : témoins de paire 0,65 G visites, noyau 0,78 G, cover 0,24 G. Divergence par fil et piles en mémoire locale : c'est la cause du rendement d'environ 1 % de la crête des ports v6/v7.
- (b) **Arbre de l'atlas à saturation.** 1,59 G visites, 1,31 G tests ponctuels, 0,85 G IDs copiés ; récursion à arrêt anticipé, profondeur au plus 20.
- (c) **Covers** de 2 à plusieurs milliers de sites.
- (d) **Émissions** de taille variable.

Réponses :

- un warp par arête, avec pile explicite en mémoire partagée (un parcours en largeur perdrait l'arrêt anticipé et paierait plus de tests) ;
- trois classes d'arêtes : S (cover d'au plus 1 024 sites), M (au plus 16 k, arène globale résidente), L (au-delà, sur CPU) ;
- émission dans une arène atomique puis tri : l'ordre d'émission n'est pas contractuel, le catalogue trié l'est.

**Forme la plus favorable : D3 composée avec les certificats actuels, partagée par la longueur de l'arête propriétaire.**

Principe de localité. Soit une boule minimale dont l'arête propriétaire est ab, la plus longue arête du support, de longueur l = |ab|. Tout site intérieur ou de coquille z vérifie |z − m_ab| ≤ 0,966 l :

- q4 : borne (√3+1)/(2√2) de l'audit Q4 ;
- q3 aigu : √3/2 ;
- q2 : 1/2.

Les décisions exactes d'une arête ne lisent donc que P ∩ B̄(m_ab, l). Les rejets restent sûrs sur tout sur-ensemble : un sous-ensemble de témoins ne peut qu'ôter des crédits, lemme déjà employé par le noyau diamétral.

Partage :

- On fixe une longueur L0 de 1 à 2 m.
- Chaque arête de longueur au plus L0 appartient à la tuile semi-ouverte de côté T qui contient son milieu (test entier floor((a+b)/2T)). Elle est traitée contre les seuls sites de la tuile élargie de L0.
- Les arêtes plus longues vont à une passe globale sur CPU : algorithme actuel, s = 8, filtre exact |ab|² > L0².
- Chaque présentation garde son unique arête propriétaire : une seule passe l'émet.

Mesure locale faite sur les trois trames entières :

| Tuile T, halo | Sites de la tuile élargie : p99 | Maximum |
| --- | ---: | ---: |
| T = 1 m, L0 = 1 m | 866 à 1 453 | 2 049 à 2 909 |
| T = L0 = 2 m | 1 899 à 3 811 | 3 466 à 5 486 |

En int16 local (étendue inférieure à 2^13 mm), le pire cas occupe 33 Kio : chaque tuile tient en mémoire partagée. Conséquences :

- les requêtes d'index deviennent des balayages cohérents de blocs Morton de 32 sites munis d'une boîte ;
- les produits deviennent étroits : 32×32→64 en une IMAD.WIDE pour les formes du certificat.

On compte 1 516 à 4 771 tuiles (cœurs p50 de 3 à 6 sites, maximum 389 à 2 846). Elles sont ordonnées par travail estimé, plus longue d'abord, sur une file device ; les petites sont groupées par warp.

**Fusion du catalogue.** Se porte naturellement.

- Lemme du support unique : si la coquille d'une boule a exactement q_min sites, tous ses supports minimaux égalent cette coquille. Le couple (arité, IDs triés) identifie alors la boule.
- Dédoublonnage par tri radix sur 128 bits : 4 à 6 ms pour 5,5 M présentations.
- Clé canonique par PGCD 128 bits seulement pour les 135 à 280 boules à coquille étendue.
- Garder le PGCD sur 4,4 à 5,5 M présentations coûterait 22 à 27 G instructions (15 à 70 ms), autant que census et FULL statique réunis.

**Census.** Se porte naturellement ; noyaux v6/v7 : 154 à 189 ms pour 21,5 M boules.

- 392 à 535 M visites à K10 ; un warp par boule, en balayage de tuile.
- Coquille de plus de 12 sites : refus.
- q_min des coquilles étendues : sur CPU.

**FULL.**

Se portent les cibles statiques, dont les primitives v7 sont déjà qualifiées sur la carte :

- 8,6 à 11,3 M appels MEB à K10, soit 97 M tests de puissance et 257 M distances de paires à 000100 ;
- 282 à 403 M visites d'intrus ;
- recherches de clés par table de hachage.

Ne se porte pas tel quel : le calendrier (lots par niveau exact, union-find à compression, IDs canoniques, contributions, verticales, banque). Mesures hors dépôt :

- l'ordre K = 10 de 08/000000 coûte 4,65 Gcycles sur un fil, soit 2 840 cycles par nœud, dont 390 par défaut de cache sur les racines ;
- seuls 1,72 % des blocs K10 sont dans des lots multiples ;
- 159 086 à 290 466 lots groupés : un noyau par niveau est exclu.

Le calendrier reste donc sur CPU (ordres K en parallèle). Il fixe le plancher de D6 seule : environ 0,29 à 0,55 s à K5, 0,76 à 1,47 s à K10.

Le franchir exige la reformulation D5 : forêt couvrante minimale du graphe temporel, fermeture des plateaux, IDs canoniques (proposition PHASE_A_GRAPHE_TEMPOREL, non prouvée). La fermeture v6 F0 rappelle qu'un calcul parallèle des composantes connexes ne préserve pas à lui seul les racines canoniques.

### Organisation d'exécution

- Un contexte CUDA chaud ; sa création est publiée à part.
- Une arène préallouée de moins de 10 Go à 40 k : paires 0,26 Go, catalogue SoA environ 1,1 Go, cibles FULL 0,3 Go.
- Un seul flux, et une quarantaine de noyaux enchaînés par un graphe CUDA.
- Files de travail device (compteur atomique, tuiles triées par coût) ; jamais de boucle hôte par tuile ou par arête.
- Statut par élément, puis réduction au plus petit indice global, qui entraîne le refus du run entier.
- Sorties préremplies de sentinelles et validées (contrat v6).
- Retour vers l'hôte :
  - catalogue compact, environ 64 o par boule (0,07 à 0,35 Go), si le calendrier reste sur CPU ;
  - la tour (0,8 à 1 Go à K10) si D5 est porté.
- Portes :
  - égalité des condensés catalogue et tour entre CPU et GPU ;
  - T2 par la route GPU ;
  - invariant d'Euler par K sur les 18 coupes ;
  - mutants device.

### Classement des formes pour le GPU

1. **D3 composée avec les certificats actuels.** Unités bornées et résidentes, balayages cohérents, entiers étroits, aucune structure globale.
2. **Forme actuelle globale.** Portable, mais parcours d'index divergents et covers non bornés. Elle convient à la passe globale des arêtes longues.
3. **D4 (inversion par site).** Grain régulier, mais les ≤k-niveaux 3D se construisent de façon incrémentale et très branchue, et l'ensemble candidat n'est pas borné sans D3. La brute force en O(m³) par site est intenable : aux densités mesurées, m va de quelques centaines à environ 2 700 sites dans un voisinage de 1 m.
4. **D2 (délétion).** Delaunay incrémental par ensemble intérieur, perturbation symbolique, réparation d'étoiles. C'est la forme la plus branchue, et elle est voisine des lignes PDEL/Geogram archivées.

## Complétude

**Nature du résultat.** D6 ne crée pas de complétude : elle la transfère. La route CPU n'est elle-même que `complete_relative` : oracles exhaustifs T2 sur petits nuages, invariant d'Euler publié à l'échelle. D6 en hérite, sans plus.

**T1 — Équivalence de route (esquisse).** On compare les catalogues après dédoublonnage et census exact. Celui de la route D6 est égal à celui de la route CPU sous quatre conditions :

- (i) chaque paire candidate est couverte, soit par le front WSPD s = 8 de la passe globale, soit par l'énumération locale des paires de milieu dans la tuile et de longueur au plus L0 ;
- (ii) seuls sont appliqués les certificats déjà prouvés de la chaîne : témoins universels stricts de rectangle ou de paire, voie morte sur le noyau ou le cover, saturation d'atlas, census de feuille exacte ;
- (iii) toute voie non rejetée atteint les mêmes prédicats exacts d'émission ;
- (iv) chaque émission est ensuite recensée exactement.

L'ordre d'application des certificats et le découpage des cellules de centres n'interviennent pas. Chaque certificat est sûr indépendamment de l'ordre ; le catalogue trié ne dépend pas de l'ordre d'émission.

**L1 — Localité de l'arête propriétaire (prouvé, élémentaire).** Soit une boule minimale de rayon R, de centre c et d'arête propriétaire ab, avec l = |ab| et m le milieu de ab.

- q2 : |z − m| ≤ l/2.
- q3 aigu : R ≤ l/√3 et |c − m|² = R² − l²/4 ≤ l²/12, donc |z − m| ≤ R + |c − m| ≤ (√3/2) l.
- q4 strictement positif : R² ≤ (3/8) l² par l'identité de variance, et |c − m|² ≤ l²/8, donc |z − m| ≤ ((√3+1)/(2√2)) l < 0,966 l (audit Q4_STRUCTURE_ET_BORNES).

Ainsi P ∩ B̄(c, R) ⊂ B(m, l), qui est contenu dans le cube de la tuile élargi de L0 lorsque l ≤ L0.

**L2 — Sûreté par restriction (esquisse).**

- Tout certificat de rejet reste sûr quand ses témoins sont pris dans un sous-ensemble de P : un sous-ensemble ne peut qu'ôter des crédits. Le code l'utilise déjà pour le noyau diamétral.
- Les décisions d'émission (profondeur, coquille) des arêtes de longueur au plus L0 sont exactes sur la tuile élargie, par L1.

Il reste à vérifier cela chemin de code par chemin de code : cache de témoins par extrémité, census q3 `GlobalBoxes`, feuilles d'atlas, balayages q4.

**L3 — Partage exact.**

- Une présentation a une arête propriétaire unique. Sa longueur est comparée à L0 en entier (|ab|² contre L0²).
- Son milieu appartient à une tuile unique : a + b est entier, donc le test floor((a+b)/2T) est exact.
- Chaque présentation est donc émise une et une seule fois.

Statut : prouvé, si la règle de propriété actuelle est une fonction de la présentation. Les deux présentations surnuméraires de K10/000100 (4 383 304 présentations pour 4 383 302 clés) viennent de supports distincts de boules à coquille étendue, pas d'une double émission ; c'est à vérifier.

**L4 — Support unique (prouvé, élémentaire).**

- Soit U la coquille. Si |U| = q_min, tout support minimal S ⊆ U vérifie |S| ≥ q_min = |U|, donc S = U.
- Un support affinement indépendant, avec un centre dans l'intérieur relatif de son enveloppe convexe, détermine une unique boule minimale.
- Donc le couple (arité, IDs triés) est une clé exacte hors coquille étendue. Le census détecte le cas coquille > arité ; on calcule alors la clé canonique exacte.

**Pièges de dégénérescence.**

- Milieux sur une frontière de tuile : réglé par le floor entier.
- Égalité |ab|² = L0² : affectée aux tuiles.
- Coquilles étendues : q_min est recalculé sur CPU, comme aujourd'hui.
- Niveaux égaux dans FULL : tri par clé approchée certifiée, puis regroupement exact par comparaisons U320.
- Classe L découpée en sous-cellules de centres : la propriété des centres doit être semi-ouverte, sans quoi une même présentation serait émise deux fois. Il ne faut surtout pas assouplir la garde `chain_duplicate_presentation`.
- Dépassement des bornes du repère local : l'arête est routée vers la passe globale, jamais tronquée.
- Supports plats (det = 0) : refus, comme aujourd'hui.
- Compteurs de politique (cache, arrêts anticipés) : ils peuvent différer entre routes ; l'objet, non.

**Statut global.** Esquisse, jugée par portes :

- condensés du catalogue et de la tour identiques entre CPU et GPU sur les trois trames et les 18 coupes ;
- T2 exhaustif rejoué par la route GPU ;
- invariant d'Euler par K ;
- fixture de boule longue manquée si la passe globale est retirée (mutant).

Aucun statut public n'est revendiqué.

## Exactitude

**Bornes actuelles** (coordonnées sur 18 bits, M = 262 143) :

- formes du certificat de voie morte : constante mise à l'échelle inférieure à 2^60 (i64), x et y inférieurs à 2^39 ;
- `outside()` : calcul en i128, valeurs inférieures à 2^84 ;
- clés q3/q4 : i128 ;
- niveaux : U192/U320.

**Coût sur sm_120.** i64 et i128 y sont émulés : de 2 à 20 instructions selon l'opération ; U192/U320 entre 40 et 80.

**Décisions de D6.**

**(1) Repère local par tuile.**

- Coordonnées relatives au coin de la tuile élargie. Étendue au plus T + 2 L0 ≤ 6 m, soit moins de 2^13 mm : int16 en mémoire partagée.
- w = 2z − (a + b) < 2^14 et |w|² < 2^29,6.
- Formes du certificat avec une échelle de 2^20 : constante inférieure à 2^50, x et y inférieurs à 2^28.
- Coins de cellules : au plus 2^21, donc produits x·coin inférieurs à 2^49.

Le calcul reste en i64, mais chaque produit devient un s32×s32→s64, soit une seule IMAD.WIDE, si l'on borne x et le coin sous 2^31. Le test uniforme passe d'environ 30 à environ 8–12 instructions.

Les tables de bornes de la chaîne, écrites pour M, doivent être redémontrées paramétriquement en fonction de l'étendue locale E, avec `static_assert`, et avec refus ou routage vers la passe globale si E dépasse la borne. La v7 rappelle le risque : son test de plateau en i128 rendait de vraies réponses fausses à 18 bits.

**(2) Centres et puissances q3/q4.** i128 conservé là où les bornes l'exigent (clés, centres rationnels). En repère local, beaucoup de ces calculs passent en i64, à prouver cas par cas.

**(3) Niveaux FULL.** Jamais comparés en flottant.

- Tri radix sur une clé de 64 bits certifiée : plancher et plafond d'une approximation calculée en entier.
- Comparaisons U320 exactes seulement entre voisins dont les intervalles se chevauchent (plateaux).

**(4) Flottant.**

- FP64 exclu du chemin chaud : son débit sur GB202 est le 1/64 du FP32.
- FP32 seulement comme proposition vérifiée exactement : Welzl du MEB, soit 6,1 M propositions pour 8,6 M appels à K10/000100, environ 300 flops chacune. C'est moins de 5 ms même en FP64, et `meb_proposal_fallbacks` = 0 dans R7b.
- Pas de filtre flottant devant les tests affines : ils sont déjà moins chers en entier étroit. Le filtre fp64 placé devant un repli entier à largeur fixe a été mesuré neutre (±2 %, GERMINATION_LOCALE_SUPPORTS_3_4 § 8.1).
- Compilation avec `-fmad=false` ou intrinsèques `_rn`/`_ru`/`_rd` explicites, jamais `--use_fast_math`.

**(5) Replis et refus.**

- Toute sortie de domaine est un refus transactionnel : statut par élément, réduction au plus petit indice global, run entier refusé. Jamais une troncature.
- Dégénérescences traitées exactement comme sur CPU.
- Sorties device préremplies de sentinelles et validées avant toute reconstruction (contrat v6).

**(6) Répartition du temps GPU projeté** (modèle, bande centrale, K10) :

- tests entiers exacts étroits (visites, bornes, tests uniformes, tests ponctuels, balayages, copies) : plus de 95 % des instructions, soit environ 0,5 à 0,75 s ;
- propositions et filtres flottants certifiés (Welzl, clés de tri des niveaux) : moins de 10 ms ;
- replis exacts larges : environ 5 à 15 ms. Ils comprennent 9,7 M constructions de boules q3 à environ 300 instructions, 97 M tests de puissance MEB, les égalités de niveaux, et les PGCD limités aux coquilles étendues.

Le flottant n'achète donc presque rien sur ce GPU. Le levier arithmétique est le rétrécissement des entiers.

## Estimation du travail

**Modèle.** Fichier /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/D6_gpu/model.py. Il compte des équivalents d'instructions entières 32 bits par opération élémentaire publiée par la sonde v12. Ces compteurs sont déterministes et égaux à ceux de R7b sur les champs communs.

| Opération élémentaire | Instructions |
| --- | ---: |
| visite de nœud | 60 |
| borne | 40 |
| test uniforme | 30 |
| test ponctuel | 15 |
| site, forme ou ID copié | 10 |
| site actif de balayage | 15 |
| événement de balayage | 150 |
| construction de boule q3 | 300 |

**Totaux sur trames entières.** 000100 est la plus légère, 000200 la plus lourde.

| | Opérations q3/q4 | Instructions q3/q4 | Census | FULL statique |
| --- | ---: | ---: | ---: | ---: |
| K5 | 4,43 à 6,77 G | 158 à 237 G | 4,8 à 6,2 G | 1,8 à 2,4 G |
| K10 | 14,55 à 22,42 G | 497 à 752 G | 24 à 33 G | 24,6 à 34,3 G |

Poids à K10/000100 : visites de nœuds 4,22 G, tests uniformes 3,81 G, copies 3,11 G, tests ponctuels 1,70 G, bornes 1,19 G.

**Calibrage CPU (expérience faite).** Sur les 60 cas du reçu `lidar_scaling_local_20260923` (W8 local), le rapport « instructions-modèle / temps q3/q4 mesuré » vaut 6,4 à 12,3 G/s : médiane 9,5, p10–p90 de 7,5 à 11,4. Le modèle prédit donc le temps CPU à ±30 %. Sur G4 W48 (R7b), il vaut 49 à 63 G/s à K5 et 76 à 95 G/s à K10.

**Débit GPU**, en instructions-modèle par seconde :

- **pessimiste, 0,4 T/s** : débit des meilleurs noyaux déjà mesurés dans le dépôt. Census v7 : 21,47 M boules en 189 ms, soit 0,35 à 0,6 T/s selon 50 à 90 visites par boule ; census v6 : 154 ms pour 21,6 M candidats ;
- **central, 1,5 T/s** : environ 2,5 % de la crête entière (environ 60 T/s), quatre fois le meilleur noyau mesuré ;
- **optimiste, 4 T/s** : environ 7 % de la crête.

On applique des facteurs de restructuration et de déséquilibre : ×2, ×1,5 et ×1,2. Rapport au CPU W48 sur q3/q4 : environ ×3, ×10–13 et ×40.

**Autres postes.**

- Fusion : 4 à 15 ms.
- Transferts : 0,5 Mo montants ; 0,07 à 0,35 Go descendants en format compact.
- Environ 40 lancements de noyaux : moins de 1 ms.
- Calendrier FULL sur CPU : 2 840 cycles par nœud de l'ordre maximal. Source : 4,65 Gcycles pour 1 638 573 nœuds à 08/000000 K10, harnais hors dépôt. Soit 0,6 à 0,9 µs par nœud sur G4 : 0,29 à 0,55 s à K5, 0,76 à 1,47 s à K10.

**Expérience D3 faite** (tiles.cpp, même dossier), sur les trames entières :

| Tuile T, halo h | Tuiles | Cœur p50 | Cœur p99 | Cœur max | Élargie p99 | Élargie max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| T = 1 m, h = 1 m | 3 766 à 4 771 | 3 | 89 à 195 | 389 à 954 | 866 à 1 453 | 2 049 à 2 909 |
| T = h = 2 m | 1 516 à 2 054 | 4 à 6 | 213 à 482 | 682 à 2 846 | 1 899 à 3 811 | 3 466 à 5 486 |
| T = 0,5 m, h = 0,5 m | — | — | — | — | — | 797 à 2 753 |

Aucune tuile élargie ne dépasse 8 192 sites.

**Non mesuré** (hôte chargé, charge 6,5 à 15) : la part du travail CPU actuel portée par les arêtes propriétaires plus longues que L0. C'est elle qui décide si la passe globale sur CPU reste masquée par le GPU. Le modèle suppose qu'elle ne dépasse pas 10 %.

**Étage cible de l'expérience de falsification** (« noyau diamétral + certificat sur noyau »), en instructions-modèle :

| Trame | K5 | K10 |
| --- | ---: | ---: |
| 000100 | 45,9 G | 119,5 G |
| 000000 | 62,6 G | 173,2 G |
| 000200 | 73,1 G | 209,8 G |

À K10, cela donne 80 à 140 ms à 1,5 T/s, et 300 à 525 ms à 0,4 T/s.

## Projection G4

**Répartition.**

- GPU : index ; q3/q4 des arêtes d'au plus L0, par tuiles ; q2 à K5 ; fusion ; census ; cibles statiques FULL.
- CPU, 48 fils :
  - q2 à K10 (0,41 à 0,82 s), en concurrence avec q3/q4 sur GPU ;
  - passe globale des arêtes plus longues que L0 ;
  - coquilles étendues (135 à 280 boules) ;
  - calendrier FULL, ordres K en parallèle ;
  - validations.
- Mémoire device : moins de 10 Go sur 96. Une seule vague suffit à 40 k. RSS hôte réduit, puisque plus aucune présentation ne vit sur l'hôte.
- Transferts :
  - montant : 0,5 Mo ;
  - descendant : 0,07 à 0,35 Go en format compact, soit 1,5 à 7 ms épinglé ;
  - ou 0,8 à 1 Go pour la tour K10 si D5 est porté sur GPU, soit 16 à 20 ms épinglé, ou jusqu'à 70 ms au débit observé en v6.

**Temps projetés** (s), trames de 35,5 à 45,8 k sites, de 000100 à 000200/000000. Fichier projection.py.

| | D6 seule, sans D5 | Avec D5 | Dont q3/q4 GPU | Dont calendrier CPU |
| --- | ---: | ---: | ---: | ---: |
| K5 pessimiste | 1,35 à 1,92 | 0,95 à 1,40 | 0,79 à 1,19 | 0,29 à 0,55 |
| K5 central | 0,56 à 0,75 | 0,23 à 0,32 | 0,16 à 0,24 | 0,29 à 0,55 |
| K5 optimiste | 0,37 à 0,47 | 0,11 à 0,14 | 0,05 à 0,07 | 0,29 à 0,55 |
| K10 pessimiste | 4,1 à 5,8 | 3,1 à 4,5 | 2,5 à 3,8 | 0,76 à 1,47 |
| K10 central | 1,60 à 2,10 | 0,73 à 1,01 | 0,50 à 0,75 | 0,76 à 1,47 |
| K10 optimiste | 1,00 à 1,28 | 0,32 à 0,40 | 0,15 à 0,23 | 0,76 à 1,47 |

La colonne « Avec D5 » suppose le calendrier ramené à 0,03 s à K5 et 0,08 s à K10.

Référence CPU R7b à 48 fils : chaîne de 3,67 à 6,57 s à K5 et de 9,58 à 15,30 s à K10.

**Lecture.**

- **1 s à K5** : atteint en bande centrale par D6 seule. Confiance faible à moyenne.
- **1 s à K10** : atteint seulement avec D5 en plus, et au bord (0,73 à 1,01 s en central).
- **100 ms** : atteint dans aucune bande.
  - K5 optimiste avec D5 donne 0,11 à 0,14 s ; il faudrait encore réduire le travail d'un facteur 1,5 à 2 avec D3 ou le certificat de bloc.
  - À K10, la seule sortie explicite de 0,8 à 1 Go coûte 16 à 70 ms de transfert ou d'écriture.
- **Trame de 60 k** : multiplier q3/q4 par 1,2 à 2,2. Les pentes du modèle sur les coupes emboîtées valent de 0,4 à 1,9 par doublement ; `core_sites` atteint p = 2,5 à 3,05.

**Confiance.**

- Bande pessimiste : ancrée sur des débits mesurés dans le dépôt. Confiance moyenne.
- Bande centrale : suppose quatre fois le meilleur noyau jamais mesuré ici. Confiance faible.
- Plancher du calendrier : tiré d'un harnais hors dépôt. Confiance moyenne.

Aucun de ces temps n'est une mesure GPU du code v9.

## Doctrine

**Invariants du dépôt conservés.**

- Exactitude entière : aucune décision flottante, aucun jitter.
- Refus transactionnels, jamais de préfixe publié.
- Aucune mosaïque de Delaunay d'ordre supérieur, ni catalogue global de cellules ou de cofaces : les tuiles sont des lots de calcul, pas une structure combinatoire, et rien n'est proportionnel à C(n, k).
- s ≥ 8 conservé pour q2 et pour la passe globale. Les tuiles emploient un WSPD local s = 8 ou le filtre de rectangles déjà prouvé.

**Pistes archivées touchées.**

- **« Fenêtre Morton fixe ou préfixe fini comme autorité exhaustive ».** La tuile n'est pas une autorité : la complétude vient de L1 et L3 (partage par la longueur de l'arête propriétaire, plus la passe globale). La règle de réouverture est satisfaite ainsi :
  - un lemme nouveau et prouvé (L1, L3, L4), à inscrire dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant tout usage ;
  - une fixture qui falsifie le motif d'abandon : une boule de catalogue dont le support traverse plusieurs tuiles (deux murs à environ 2,5 m, arête propriétaire plus longue que L0), trouvée par la passe globale et manquée par un mutant qui la retire ;
  - aucune structure globale interdite ;
  - une porte de performance distincte.
- **« Subdivision prune-only pilotée par l'hôte »** et **« parcours stackless relancé par paire »** : exclus par construction, puisque frontières et files restent sur le device, sans boucle hôte.
- **« PDEL/Geogram binary64 »** : non utilisés ; D2 est écartée pour le GPU.
- **« Grille comme définition exacte »** : la grille ne définit rien, elle ne fait que ranger.
- **v6 F0** (le reduce du fold ne se porte pas) : respecté. Le calendrier reste sur CPU tant que D5 n'est pas prouvée, avec un théorème d'équivalence et une fixture.

**Doctrine GPU v6 reprise.**

- Option CUDA OFF par défaut ; la CI ne la construit jamais.
- Aucun noyau au build produit sans reçu de gain G4.
- Le stub hôte ne prouve que la logique.
- Architecture `CMAKE_CUDA_ARCHITECTURES=120` contractuelle.
- Sessions SPOT par les seuls scripts gardés, `TERMINATED` certifié.

**Cadre.** `exploration_v9_hors_registre`, `public_status=not_claimed`. Aucune phase formelle touchée.

## Risques

- Antécédents : cinq ports GPU du dépôt (Phase 15 prune-only, frontière de paires Phase 15, lanes v5, census v6, census et primitives v7), aucun gain de bout en bout supérieur à environ 10 %. La bande centrale suppose un débit quatre fois supérieur au meilleur noyau jamais mesuré ici.
- Plancher du calendrier FULL : 0,76 à 1,47 s à K10 et 0,29 à 0,55 s à K5 sur un fil (4,65 Gcyc pour l'ordre 10 de 000000, harnais hors dépôt). Sans D5, 1 s à K10 est hors d'atteinte quelle que soit la vitesse du GPU.
- Orchestration : toute vague pilotée par l'hôte retombe à environ 34 M visites/s (prune-only, 47 synchronisations), soit 100 fois sous la bande pessimiste. La v6 a montré 88 % de l'étage en code hôte mono-fil.
- Occupation : les entiers larges demandent 96 à 128 registres par fil, donc au plus 16 à 21 warps par SM ; les piles de parcours finissent en mémoire locale. Tant que les registres, l'occupation effective et les débordements ne sont pas publiés, rien n'est prouvé.
- Déséquilibre : tuiles denses près du capteur (jusqu'à 2 049 à 5 486 sites en tuile élargie) et arêtes de classe L. Une tuile mal découpée borne le mur.
- Passe globale CPU des arêtes plus longues que L0 : sa part du travail n'est pas mesurée. Si elle dépasse 10 à 20 %, elle devient le chemin critique à K5.
- Exposants inchangés : pentes du modèle de 0,4 à 1,9 par doublement, `core_sites` à p = 2,5 à 3,05. À 60 k, q3/q4 est multiplié par 1,2 à 2,2 et K10 central sort de 1 s même avec D5.
- Bornes entières du repère local à redémontrer : une borne fausse donne une réponse fausse silencieuse (précédent v7 : test de plateau en i128 faux à 18 bits).
- Double route CPU/GPU à maintenir en permanence. Ni GPU ni nvcc dans le conteneur : chaque itération de noyau coûte une session SPOT, exposée à la préemption (R4) et à la rupture de stock (R7).
- Déterminisme : les compteurs de politique (cache de témoins, arrêts anticipés, atomiques) varient. Seules les portes d'objet (condensés, T2, Euler) jugent ; les identités de compteurs actuelles ne se transposent pas telles quelles.
- Remplacer la clé canonique par le support (fusion et recherche de clé après MEB) change l'interface FULL : risque de régression sur les coquilles étendues et les plateaux.
- 100 ms : la sortie explicite K10 (0,8 à 1 Go) et le catalogue (environ 1 Go) coûtent à eux seuls 16 à 70 ms de transfert ou d'écriture. Contrat hors de portée sans format compact (D5).
- FP64 au 1/64 sur GB202 : reprendre tel quel le Welzl en double ou un filtre fp64 du CPU sur le chemin chaud ferait perdre un ordre de grandeur.

## Expérience de falsification

**E0 — sans GPU ni session, moins d'une journée.** Chronométrer dans la sonde CPU le chemin critique séquentiel du calendrier FULL (`order_block` et `order_lot` par ordre K, un fil), sur les trois trames à K5 et K10 : en local d'abord, puis sur G4 à la prochaine session CPU déjà prévue.

- Critère : si l'ordre Kmax dépasse 0,6 s sur G4 à K10, D6 ne peut pas donner 1 s à K10 sans D5. D6 est alors déclarée subordonnée à D5 pour K10, sans être arrêtée pour K5.

**E1 — une session G4, au plus 1 h de GPU.** Écrire le noyau device « noyau diamétral + certificat de voie morte » :

- un warp par arête ;
- frontières en masques de bits, comptes saturants par `popc` de ballot ;
- pile explicite en mémoire partagée ;
- arithmétique de production (i64 et i128), résident.

Protocole :

1. Téléverser le nuage (0,5 Mo) et la liste des arêtes qui atteignent le noyau, produite sur la VM par la route CPU : 3 673 260 arêtes à 000100/K10, 1 732 176 à K5.
2. Comparer arête par arête les masques prouvés et ouverts (noyau, puis cover), les voies et le nombre de voies closes. Zéro désaccord est exigé ; sinon, fixture et correction avant toute lecture de temps.
3. Chronométrer par événements les noyaux seuls, et le mur de l'étage avec transferts. Mesurer le même étage sur CPU W48 dans la même session, sur les mêmes arêtes.
4. Publier registres, occupation, octets H2D et D2H, nombre de lancements, zéro boucle hôte par arête.

Cibles du modèle : 119,5 G instructions-modèle à 000100/K10, 173,2 G à 000000, 209,8 G à 000200.

**Critères chiffrés.**

- **Arrêt.** L'une de ces conditions suffit :
  - noyaux au-delà de 300 ms à 000100/K10 (moins de 0,4 T instructions-modèle/s, soit environ ×3 seulement sur le CPU W48) ;
  - mur de l'étage supérieur au double des noyaux (orchestration) ;
  - un désaccord inexpliqué.

  D6 se limite alors au census et au FULL statique.
- **Confirmation** : au plus 80 ms (1,5 T/s ou plus) et transferts inférieurs à 10 % de l'étage. Le port q3/q4 complet est alors autorisé.
- **Entre les deux** : reprendre seulement après avoir mesuré la variante D3 du même étage, en repère local int16 avec IMAD.WIDE sur les mêmes arêtes. Critère : gain au moins ×2, sinon arrêt.

**Complément facultatif dans la même session.** Noyau census sur le catalogue v9 de 000100/K10 : 4,38 M boules, 392 M visites ; attendu 20 à 60 ms. Il recoupe la bande pessimiste sur LiDAR v9.

## Coût

**Expériences préalables.**

- E0 : moins d'une journée, sans GPU.
- E1 : 1 à 2 semaines-développeur, pour le noyau, son stub hôte au bit près, la porte d'égalité par arête, l'instrumentation par événements et le runner de session ; plus une session G4 d'environ 1 h.

**Port complet**, seulement si E1 confirme :

| Chantier | Durée |
| --- | ---: |
| Fondations : arène préallouée, graphe CUDA, files device, statuts et refus, sentinelles, stub hôte | 2 semaines |
| Tuiles, passe globale L0, preuve des bornes locales et inscription au registre des preuves | 2 à 3 semaines |
| q3/q4 device : filtre de paires, noyau, cover, certificat, atlas, graines q3, census de feuille, balayages q4, émission, classes S/M/L | 5 à 8 semaines |
| Fusion par support, census, FULL statique (réemploi des noyaux v6/v7) | 2 à 3 semaines |
| Portes : égalité catalogue et tour, T2 par la route GPU, Euler par K sur les 18 coupes, mutants device, travail égal W1/W48 | 2 semaines |

Total : environ 13 à 18 semaines-développeur et 10 à 20 sessions G4. La v5 en a consommé 13 pour deux lanes, sans gain net.

**À ajouter séparément** : D5 (calendrier FULL), indispensable pour K10.

**Coût permanent** : double route CPU/GPU à maintenir, et pas de GPU local.
