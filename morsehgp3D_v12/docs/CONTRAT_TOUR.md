# Contrat de la tranche T2 : la tour

7 octobre 2026. Proposition du développeur, **à contre-lire par les auditeurs avant tout code de produit**
([`PLAN.md`](PLAN.md) § 0 : contrat, puis témoins et oracle, puis le natif). Cadre : `phase=exploration_v12_hors_registre`,
`backend=cpu_reference ; cuda_g4 pour le catalogue`, `objet=full_pi0`, `quantification=quantized_u21_input_only`,
`public_status=not_claimed`. Sources : conception de la tour du 2 octobre
([`CONCEPTION_TOUR.md`](../../morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md), § 3 à 5 et
annexe A) ; lemmes `LEM-T1`, `LEM-T3` à `LEM-T7`, contre-lus et inscrits au registre des preuves (section V12) ;
mesures de T0 ([session B](../receipts/g4_t0b_20261007/README.md), [session D](../receipts/g4_t0d_20261007/README.md)).
Ce document est aussi le **contrat des compteurs** exigé à l'entrée de T2 (§ 8).

## 1. L'objet

La tour FULL : pour chaque ordre $k=1..K$, l'arbre de fusion $T_k$ de $\pi_0$ de la filtration d'ordre $k$, avec ses
naissances (les sites à $k=1$ ; les cellules de naissance $(b,k)$ à $k\geq 2$), ses multifusions $N$-aires datées par
le rang de leur niveau, sa racine unique, et les **verticales** $T_k\to T_{k-1}$ (pour chaque nœud, le nœud de l'ordre
précédent vivant à la coupe fermée de son niveau qui contient ses représentants). S'y ajoute le **registre
d'événements** (étage R, [`ARCHITECTURE.md`](ARCHITECTURE.md) § 4.4), dont les sorties sont des vues (tranche T3).

**Même objet que la v11 gelée, à l'octet.** La v12 écrit, hors du chemin chronométré, le vidage `MHGP11FUL1` de la
sonde de la v11 : sites dans l'ordre de Morton exact avec leur `PointId`, puis par ordre les nœuds (parent, enfants en
CSR, niveau en rationnel exact), les naissances par (niveau, centre exact) avec leur centre, les fusions par (niveau,
plus petite naissance), les verticales. Aucun indice de boule n'y figure : le départage de $S^{*}$ par positions, seul
écart déclaré du catalogue ([`CONTRAT_CATALOGUE.md`](CONTRAT_CATALOGUE.md) § 1), ne le touche pas. Au profil 21, les
octets doivent être **identiques** aux empreintes de [`MESURE.md`](MESURE.md) § 4 ; aux profils 24 et 32, sur les mêmes
coordonnées absolues, l'empreinte sémantique du lecteur strict doit l'être (§ 9).

## 2. Entrée

- Le domaine des sites : `Cloud` dans l'ordre de Morton exact (clé sur les coordonnées absolues), index `GlobalIndex`.
- Le catalogue $\mathrm{Cat}_K$ de T1 : pour chaque boule, rang dense de son niveau exact, $p$, $m$, $q_{\min}$,
  $S^{*}$, populations $I$ puis $U$ en CSR ; table $S^{*}\to$ boule ; classement des cellules de fenêtre (naissance ou
  jonction) ; **table de populations** (`LEM-POP` : population triée d'une naissance $\to$ nœud), construite par la
  fin d'étage du catalogue et gardée par la Session (en v11 : reconstruite à chaque appel, 44 Mo à K5, 360 Mo à K10).
- **Pendant le développement**, avant que le catalogue de T1 ne tourne, la tour reçoit le catalogue de la v11 par ses
  vidages `MHGP12DP` de genre « catalogue » (`cat.bin` de l'outil des microbancs), lus par un **adaptateur de test** qui
  construit le même objet `Catalogue` que T1 produira. Ce n'est pas un second chemin produit : la tour n'a qu'un type
  d'entrée, et la sortie de T2 exige le catalogue de T1 dans la chaîne mesurée.

## 3. L'algorithme : changements déclarés d'avance

| Étage | v11 gelée | v12 | Preuve d'égalité |
| --- | --- | --- | --- |
| G, plus petite boule | `bounded_meb` exhaustive (74 présentations par boule à K10) | proposée en flottant (ne décide rien), certifiée par `LEM-T1` (deux inclusions sur les identifiants), sinon certificat exact du support proposé puis canonisation sur $F$ ; repli exact | `MES-M3` **adopté** : identité sur toutes les parties de ng00–02 à K5 et K10 |
| G, arrêt | chaque trace descend jusqu'à une naissance | arrêt à la **première cellule de fenêtre non naissance** (`LEM-T3`) | `LEM-T3` (A.3) ; seule compte la composante à la coupe ouverte de la jonction (`LEM-T4`) |
| G, sauts | $k$ plus petits `SiteIdx` de $I$ (catalogue ou census saturé) | politique choisie par mesure parmi des pas valides (§ 4.3) | théorème D : toute $k$-partie de $I$ est un pas valide ; la forêt ne dépend pas des graines |
| G, census | deux passes ou espace de travail, centre absolu | census **gardé** d'une boule certifiée, repère local, une passe, seuil $k$ (socle, `CST-0108`, `CST-0109`) | même certificat `complete` ou `saturated` ; portes du socle |
| G, populations | table reconstruite à chaque appel | produite par la fin d'étage, résidente | `LEM-POP` |
| T, noyau | Kruskal par lots dans le publieur (81 à 102 ms à l'ordre 5) | union-find par taille sur des événements binaires de 20 octets, un propriétaire par ordre, recouvert par G | `LEM-T4` ; `MES-M4` : forêts identiques sur ng00–02 à K5 et K10 |
| M, contraction | collée au publieur | classes d'événements liés par rang, parallèle par tranches alignées sur les rangs | `LEM-T4` ; `MES-M4` |
| V, verticales | balayage d'ancêtres (`forest_vertical*`) | naissances par `LEM-T6` en $O(1)$ ; fusions par `LEM-T5` (historique d'attache) | `MES-M4` juge `LEM-T6` sur toutes les naissances ; naturalité au lecteur strict |
| R, registre | `attach` sériel a posteriori (75 ms) | produit au fil du calcul par T, M et V | égalité des vues (T3) |

Tout le reste est porté depuis la v11 gelée (`ac081a06f`), épinglé dans [`PORTS.md`](PORTS.md) et requalifié.

## 4. Étage G : la résolution

### 4.1 La fonction

`resolve1(k, F)` est une fonction pure du domaine immuable (catalogue, index, table de populations) ; elle ne lit
jamais la structure d'union, n'écrit rien de partagé, et rend une **cible** : une naissance, ou une cellule $(b',k)$.

```text
resolve1(k, F) -> cible                 # F : k sites tries, representant d'une jonction de l'ordre k
  si k = 1 : rendre la naissance du site
  boucle :
    v <- populations[k].trouver(F) ; si v existe : rendre v                  # LEM-POP, egalite verifiee sur les identifiants
    S <- proposition(F)                                                      # flottant, ne decide rien
    si S ⊆ F, b <- supports.trouver(S) existe et F ⊆ P_b :                   # LEM-T1
      (p, q, m, I, U) <- catalogue(b)
      exiger rang(b) < rang precedent (ou niveau exact precedent) ; precedent <- b
    sinon :
      boule <- certificat exact de S sur F, sinon repli exact ; S' <- support canonique parmi F ∩ sphere
      exiger niveau(boule) < niveau precedent ; precedent <- boule           # AVANT toute sortie par saut
      si b <- supports.trouver(S') existe et F ⊆ P_b : (p, q, m, I, U) <- catalogue(b)
      sinon :
        R <- census_garde(boule, k)
        si R sature : F <- saut(boule, k) ; continuer                          # p >= k ; la sphere peut etre au catalogue
        (I, U) <- R ; b <- supports.trouver(S*(U)) ; si b existe : exiger (p, q, m) = catalogue(b), sinon census_mismatch
    si p >= k : F <- saut(I, k) ; continuer
    si k <= p + q - 2 : F <- I ∪ (les k - p plus petits SiteIdx de U) ; continuer   # boule inerte, sous la fenetre
    si b absent : refus catalogue_missing_ball                                     # (H2) violee
    si (b, k) est une naissance : rendre la naissance (b, k)
    rendre la cellule (b, k)                                                       # LEM-T3 : arret
```

**Pourquoi c'est juste.** Chaque pas est un pas valide du théorème D (L02) : une $k$-partie de $I$ quand $p\geq k$ ;
$I\cup A$ avec $A$ séparable sinon, toute partie de $U$ de moins de $q_{\min}$ sites l'étant (lemme 3 de L02). Le niveau
décroît strictement à chaque pas, contrôlé par les rangs entre deux boules du catalogue et par le niveau exact sinon
(il est matérialisé pour le census) ; le contrôle et la mise à jour de la boule précédente précèdent **toute** sortie
par saut, census saturé compris (contre-lecture Codex) : la boucle termine sans plafond arbitraire. Un census saturé
prouve seulement $p\geq k$ : la sphère peut être au catalogue quand son $S^{*}$ n'est pas dans $F$ (fait gravé
`fact_saturated_in_catalogue` : carré, deux sites intérieurs, $K=5$, $F$ la diagonale qui n'est pas $S^{*}$). L'arrêt sur une cellule $(b',k)$ non
naissance rend une cible dont la valeur est celle de la jonction de $b'$, de rang strictement inférieur (`LEM-T3`).

**Lecture des cibles.** Le noyau traite les jonctions par rang croissant ; une cible « cellule $(b',k)$ » se lit comme
l'élément d'union de la jonction de $b'$, **déjà traitée** : après elle, tous ses représentants sont dans une même
composante. Aucun suivi de pointeurs n'est donc nécessaire pour la forêt ; la chaîne des pointeurs ne sert qu'aux usages à
coupe fermée (attaches de `points`, tranche T3). **Date d'usage** : une cible n'est lue qu'à la coupe ouverte du niveau de
sa jonction, strictement supérieur à $\beta(F_0)$, ou à une coupe fermée de niveau au moins $\beta(F_0)$ ; le pointeur
d'une cellule ne vaut qu'à partir du niveau de $b'$ ; **aucune** garde $\beta(F_0)\leq\ell(r_b-1)$ (`CST-0104`).

### 4.2 Un fait qui dimensionne le census

**`LEM-HORS-CAT`.** Si la plus petite boule $b$ d'une $k$-partie de sites distincts, $2\leq k\leq K$, n'est pas dans
$\mathrm{Cat}_K$, alors $p\geq K-2$ ; si de plus $p<k$, alors $k\geq K-1$, et $p=K-2$ exige $q_{\min}=4$. *Preuve.* Dans
$\mathbb{R}^{3}$, le centre d'une boule critique est dans l'enveloppe convexe d'au plus quatre sites de sa coquille
(Carathéodory), donc $q_{\min}\leq 4$ ; hors de $\mathrm{Cat}_K$ signifie $p+q_{\min}\geq K+2$, d'où $p\geq K-2$, et
$p<k\leq K$ donne $k\geq p+1\geq K-1$. $\square$ Aux ordres $k\leq K-3$, toute sphère hors du catalogue est donc
saturée. **Il faut $k\geq 2$** (constat `CST-0229` de Codex) : $\mathrm{Cat}_K$ ne contient que des boules positives, et la
boule d'un singleton est le site, de rayon nul, hors du catalogue avec $p=0$ (trois sites alignés, $K=3$ : $p=0<K-2$ ;
fait gravé `fact_lemma_needs_two_sites`) ; le résolveur traite $k=1$ à part.

**Portée** (correction du 7 octobre, données de `MES-G1`) : le lemme ne parle que des sphères **hors** du catalogue. Une
sphère **du** catalogue dont $S^{*}$ n'est pas dans la partie (coquille à plusieurs supports minimaux, partie qui en
contient un autre) n'est pas trouvée par le support local et demande elle aussi un census, complet si $p<k$, **à tout
ordre** : la glose écrite d'abord, « un census complet n'arrive qu'aux ordres $K-1$ et $K$ », était fausse en général.
Témoin gravé (`fact_complete_census_in_catalogue` de `reference/test_resolution_v12.py`) : carré
$A,B,C,D$ et un point lointain, $K=5$, $F=\lbrace B,D\rbrace$, sphère du carré ($S^{*}=\lbrace A,C\rbrace$), census
complet à l'ordre $2=K-3$. Ces cas sont rares : 3 parties sur ng00 à K5 ; 16 parties aux ordres 5 à 8 sur ng00 à K10.
Mesuré (session D, ng00 K5, toutes causes) : censuses complets 0, 0, 435 et 66 906 aux ordres 2, 3, 4 et 5 ; saturés
2 515, 14 000, 57 239 et 150 419.

### 4.3 Où passe le temps, et les leviers

Session D, ng00 K5, résolution à un fil sur G4 : **1,376 s** avec la plus petite boule certifiée (réplique de la v11 :
1,477 s ; v11 elle-même : 2,146 s), dont 0,741 s à l'ordre 5. À l'ordre 5 : 1 350 288 représentants, dont 963 244
(71 %) s'arrêtent à la première sonde de la table de populations ; 568 919 plus petites boules, dont 351 594 certifiées
par `LEM-T1` (62 %) et 217 325 par un census ; chaîne la plus longue : 11 pas. Un ajustement linéaire sur les ordres 2 à
5 donnait environ 140 ns par représentant et 1,6 µs par census, soit près de la moitié de l'ordre 5 pour le census ;
**le profil mesuré le corrige** (`MES-M7` en local, compteur de cycles, [reçu](../receipts/mes_g1_m7_local_20261007/README.md)) :
sur les ordres 2 à 5 de ng00 à K5, **sondes de la table de populations 41,6 %** (285 ns par sonde à l'ordre 5),
proposition et `LEM-T1` 23,3 %, census saturé 14,4 % et complet 6,5 % (2,0 et 2,8 µs à l'ordre 5), certificat
2,3 %, reste 10,4 % ; à K10, sondes 32,9 %, proposition 31,3 %, census 22,9 %. **Confirmé sur G4** (session E) : sondes
38,6 à 42,9 %, proposition 25,5 à 26,0 %, census 13,9 à 18,6 % à K5 sur ng00–02 ; 148 ns par sonde et 394 ns par
proposition à l'ordre 5 de ng00. Au facteur de
parallélisme de la v11 (sa propre résolution, 2,146 s à un fil sur ng00, prenait 63 à 83 ms sur ng00–02 avec 39
résolveurs à 48 fils, soit ×26 à ×34 ; `AUDIT_GEANT_V11.md` § 7.4), 1,376 s donnerait 40 à 53 ms : il faut **diviser le
coût à un fil par 1,5 à 2**, ou mieux paralléliser, pour tenir 25 à 30 ms.

| Levier | Idée | Mesure et règle (écrites d'avance) | État |
| --- | --- | --- | --- |
| `G-L1` | plus petite boule certifiée (`LEM-T1`) | `MES-M3` | **adopté** |
| `G-L2` | mémo de cellule (`LEM-T3`) | structurel ; parties et censuses évités publiés (`MES-M7`) | déclaré |
| `G-L3` | **saut certifié sans census** : tester exactement des candidats locaux contre la boule certifiée ; $k$ sites strictement intérieurs exhibés prouvent $p\geq k$, et les $k$ plus petits `SiteIdx` d'entre eux font le saut (pas valide) ; sinon census | `MES-G1` : part des censuses saturés évités, hors ligne sur les vidages (deux ensembles de candidats : $K$ plus proches voisins des sites de $F$, calculés en P et utiles aussi à `points` ; fenêtre de Morton autour de $F$) ; adopté si au moins la moitié des censuses saturés disparaissent à K5 sur ng00–02 **et** si le temps de G à un fil baisse (borne haute de l'IC 95 % du rapport sous 1) | **rejeté sur G4** (7 octobre, [session E](../receipts/g4_t2e_20261007/README.md)) : 81 à 83 % des censuses saturés évités, forêt identique, mais résolution 3 à 4 % plus lente (bornes hautes 1,036 à 1,043) ; le census gardé reste la voie produit |
| `G-L4` | catalogue **hors fenêtre** : par `LEM-HORS-CAT`, les censuses complets des sphères hors du catalogue ne visent que des boules de $\mathrm{Cat}_{K+2}$ à $p\in\lbrace K-2,K-1\rbrace$ ; le catalogue les émettrait marquées, sans cellule, et `LEM-T1` les trouverait | `MES-G2` : nombre de boules distinctes concernées, puis coût du catalogue élargi contre census évités, sur G4 ; adopté si le gain net est positif | à mesurer |
| `G-L5` | **premières sondes en masse** : 71 % des représentants s'arrêtent à la première sonde ; au lieu d'une sonde aléatoire par représentant (deux défauts de cache, 285 ns à l'ordre 5 en local), une **jointure triée** des premières traces et des populations de naissance par leur empreinte additive (tri par base, fusion, égalité vérifiée sur les identifiants), sur l'appareil où la table est construite, ou sur l'hôte par tri parallèle | `MES-G3` : passe de sondes sur le GPU et rapatriement des échecs contre sondes sur l'hôte, de bout en bout ; adopté si G baisse (IC) | à mesurer |
| `G-L6` | politique de saut : $k$ plus petits `SiteIdx` (v11) ou $k$ plus proches du centre (comparateur exact) | `MES-G4` : temps de G à un fil et histogramme des chaînes ; le moins cher est retenu | à mesurer |
| `G-L7` | localité : travail trié par clé de Morton de la partie, sondes préchargées par lots de 32 (v10), census partant de la feuille du premier site | publié avec `MES-M7` | à mesurer |

Chaque levier change le travail, jamais la sortie : la porte `MES-M0` (§ 9) le vérifie à chaque adoption. **Ordre
de travail déduit du profil** : les sondes (`G-L5`, `G-L7` : disposition de la table, population dans l'entrée,
préchargement par lots) puis la proposition, avant le census (`G-L3`, `G-L4`).

### 4.4 Parallélisme

G se joue en parallèle sur les jonctions, par tranches de rangs croissants ; le propriétaire de chaque ordre consomme
les tranches dans l'ordre des rangs et résout lui-même quand il attend (`D-F2`). Aucun atomique partagé : chaque
représentant a sa case de cible, écrite par un seul fil. Les compteurs logiques ne dépendent ni du découpage ni du
nombre de fils (§ 8).

## 5. Étages T, M, V et R

- **T** : port des prototypes de `MES-M4` (noyau union-find par taille, événements binaires de 20 octets, attache de
  chaque perdant avec son rang), un propriétaire par ordre, ordres indépendants. Racine unique exigée ; le nombre
  d'événements vaut le nombre de naissances moins un.
- **M** : contraction par classes d'événements liés de même rang (`LEM-T4`) ; numérotation canonique des naissances
  par (rang, centre exact, comparaison en deux temps) et des fusions par (rang, plus petite naissance) (`CST-0107`). À
  K10 la contraction parallèle reste au-dessus de 3 ms (3,2 à 4,3 ms, `MES-M4`) : elle se recouvre avec les autres
  ordres, l'écart est publié.
- **V** : image d'une naissance par `LEM-T6` (sommet laissé par la jonction de la même boule à l'ordre $k-1$, remonté
  d'un cran si le parent a le rang de la boule) ; image d'une fusion par `LEM-T5` (requête d'ancêtre sur l'historique
  d'attache de l'ordre $k-1$, **sous l'hypothèse** $\mathrm{rang}(\ell)\leq r$, refus typé hors domaine, `CST-0105`).
- **R** : nœuds (rang, parent, genre, boule de naissance), hyperarêtes de fusion retenues par Kruskal avec leurs
  branches, verticales ; écrits par T, M et V au fil du calcul, sans balayage a posteriori.

## 6. Numérique

Tout le [contrat numérique](CONTRAT_NUMERIQUE.md) s'applique. Certificat de plus petite boule dans le repère local de
la partie (supports mesurés à $s\leq 15$) ; census gardé au domaine $t=s+2$ du certificat ; tests des candidats de
`G-L3` : **d'abord le rejet sans arithmétique de `NUM-GARDE`** (un voisin ou un site de fenêtre peut sortir du pavé de la
boule certifiée : il est extérieur, jamais évalué), puis le prédicat mixte au budget de la boule ($6s+11$) ; décroissance par rangs entre boules du catalogue, par niveaux exacts
(jusqu'à 512 bits) sinon ; centres des naissances comparés en deux temps. Aucune décision en flottant : la proposition
de plus petite boule et les clés de tri ne décident rien.

## 7. Capacité, mémoire, refus

- **Domaines** : au plus $2^{31}-1$ naissances par ordre (opérandes à 31 bits utiles, `CST-0212`), refus avant
  allocation ; représentants, événements et nœuds en `u32` avec refus à la vraie limite ; décalages et compteurs en `u64`.
- **Cibles de 4 octets** : bit 31 = genre (0 naissance, 1 cellule), 31 bits d'indice dans l'espace de son genre (nœud de
  naissance de l'ordre, ou indice de cellule de fenêtre de l'ordre) ; au plus $2^{31}-1$ naissances **et** au plus
  $2^{31}-1$ cellules par ordre, refus `tower_capacity` avant allocation ; `0xFFFFFFFF` réservé (aucune cible) et
  exclu des deux domaines.
- **Mémoire** : cibles (4 octets par représentant : 3,4 millions sur ng00 à K5, 17,4 millions à K10 dans la v10),
  événements (20 octets), historique d'attache, table de populations compacte (cases à étiquette vérifiées contre la
  CSR). Comptage puis réservation, publication transactionnelle ; jamais un préfixe publié.
- **Refus** : `catalogue_missing_ball` ((H2) violée), `census_mismatch` (contrôle croisé du census et du catalogue),
  `tower_capacity`, `resource_exhausted`, coquille étendue au-delà du plafond déclaré (`WIT-SPHERE50`), et tout invariant
  violé (`tower_invariant`). Pas de plafond de pas : la décroissance contrôlée garantit la terminaison.

## 8. Compteurs (contrat)

Trois classes (correction du 7 octobre, constat `CST-0228` de Codex : la profondeur d'attache et les sites examinés par
un census arrêté au $k$-ième témoin dépendent de l'ordre de visite à objet égal).

**De l'objet** : fonctions de la tour seule, indépendantes de la politique, de l'ordre de visite, du découpage, du
nombre de fils et de la voie ; elles entrent dans l'empreinte du grand livre. Par ordre : naissances, fusions, histogramme
des arités, verticales, représentants (traces strictes des cellules de fenêtre), cellules inertes.

**Du travail** : fonctions de la politique déclarée (règle de saut, leviers adoptés) **et** de l'ordre canonique fixé
(jonctions par rang puis indice de cellule, sondes et parcours de l'index dans l'ordre préfixe) ; identiques à 1 et à
48 fils et entre l'hôte et l'appareil pour une même politique (porte W1 contre W48), mais **hors** de l'empreinte de
l'objet, publiées avec le nom de la politique. Par ordre : arrêts à la première sonde ; sondes après des pas ; plus
petites boules par route (`t1`, certificat puis table, certificat puis census, repli) ; censuses saturés et complets,
sites examinés (somme, maximum) ; sauts (catalogue, census, candidats de `G-L3`) ; pas inertes ; arrêts sur cellule ;
naissances atteintes ; histogramme des longueurs de chaîne. Noyau : événements, attaches, profondeur d'attache maximale
(au plus $\log_2$ du nombre de naissances, `LEM-T5`). Contraction : classes. Verticales : naissances par `LEM-T6`,
fusions par `LEM-T5`, profondeurs des requêtes d'ancêtre.

**Physiques** : temps par étage, fils, tranches, attentes du propriétaire, préchargements, passes sur l'appareil,
tentatives et replis ; publiés à part, jamais dans une empreinte. Un compteur de l'objet ou du travail qui varie avec
le nombre de fils est un défaut (porte W1 contre W48).

## 9. Portes

1. **Différentiel `MES-M0`** : vidages `MHGP11FUL1` de la v12 identiques à l'octet à ceux de la v11 au profil 21
   (ng00–02 à K5 et K10, uniformes de 8 000, 16 000 et 32 000 sites à K5) ; aux profils 24 et 32, empreinte sémantique
   identique sur les mêmes coordonnées, par le lecteur strict de la v11 adapté (profils 21, 24 et 32 ; ordre de Morton
   exact). L'invariance par translation reste un **contrat distinct**, jugé par translation explicite des sites et des
   centres, jamais par cette empreinte (note de mesure de l'auditeur, `CST-0207`).
2. **Oracle borné** (`reference/`, $n\leq 14$) : forêt, coupes ouvertes et fermées, verticales, sur la suite rapide.
   La règle du § 4.1 y est gravée depuis le 7 octobre (porte `mhgp12_reference_resolution_v12`) : arrêt sur la
   première cellule, cibles « cellule » lues sur la cellule déjà traitée, cellules inertes comprises, trois
   politiques de saut ; résultat identique à l'étage B (donc à la définition) sur les 342 nuages et 1 362 ordres de
   la suite rapide, 168 cibles sur des cellules inertes ; quatre mutants de la règle tués ; repli de `G-L3` sur un
   fait gravé.
3. **Témoins** : `WIT-D2` et `WIT-MEMO` (`LEM-T3`, `CST-0104`) ; `WIT-T1-CARRE` côté tour (route exacte) ; `WIT-SIX`
   (fusion ternaire simultanée) ; `WIT-TRI-EQ` (plateau) ; carré K1..4 (verticales, `CST-0214`) ; `WIT-E5` (fenêtre) ;
   refus hors domaine de `LEM-T5` (`CST-0105`) ; `WIT-T7-CERCLE25` si les coquilles étendues passent par `LEM-T7`
   (`CST-0106`) ; `WIT-SPHERE50` (refus) ; un témoin de `G-L3` où les candidats ne suffisent pas (repli sur le census).
4. **`JUG-EMST`** à l'échelle : la forêt d'ordre 1 est l'arbre de fusion du lien simple sur l'arbre couvrant euclidien
   minimal exact (niveaux $d^{2}/4$), indépendant du catalogue, sur 8 000, 16 000 et 32 000 sites et sur les trames.
   **Livré le 7 octobre** ([`juges/emst/`](../juges/emst/README.md)) : ordre 1 identique à celui de la v11 gelée sur
   ses 9 vidages réels et à l'oracle sur 308 nuages ; 1 million de sites en 5 s à un fil.
5. **Invariants globaux et juge d'échantillon** à l'échelle : une racine par ordre, événements égaux aux naissances
   moins un, verticales vivantes et naturelles, comptes par ordre égaux à ceux des cellules du catalogue ; échantillon
   de représentants résolus par la descente exacte de la v11 et comparés à la coupe de leur jonction. Jamais un juge
   exhaustif à l'échelle.
6. **Déterminisme** : vidage et compteurs logiques identiques à 1 et à 48 fils, et entre l'hôte et l'appareil.
7. **Mutants causaux** (copies, jamais une branche du produit) : `LEM-T1` sans $S\subseteq F$ ; date terminale au lieu
   de la date initiale ; contraction de toute l'étoile au lieu du lien choisi (`REG:238`) ; plateau sans contraction ;
   union par indice au lieu de la taille (profondeur de `LEM-T5`) ; `LEM-T6` sans la remontée d'un cran ; candidat de
   `G-L3` admis sur la sphère (côté nul) ; census sans seuil.
8. **Échelle** : 8 000, 16 000 et 32 000 sites, trames LiDAR de plusieurs séquences, découpes de 1 à 8 millions de
   sites (`MES-E`), petits nuages (`MES-P`).

## 10. Budget et règle de sortie

À K5, trame de type ng00, G4, à chaud ([`ARCHITECTURE.md`](ARCHITECTURE.md) § 3) : G **25 à 30 ms** ; T recouvert
(8 à 12 ms) ; M et V **5 ms** ; R mesuré. À K10, objectif publié : résolution 150 à 250 ms. Sortie de T2 : portes
vertes, `MES-M0` identique avec le catalogue de T1 dans la chaîne, budgets atteints ou écart publié avec sa cause.

## 11. Mesures de la tranche

- **`MES-M7`** (précisée) : profil de la résolution par composante, à un fil, compteur de cycles, sur la réplique v12
  de la descente (sonde, plus petite boule par route, census saturé et complet, saut, construction de la partie
  suivante), K5 et K10, ng00–02 et une trame d'une autre séquence ; publié. La comparaison à la v10 R2 prévue au plan
  est abandonnée : la réplique de la v11, jugée trace par trace, est la référence.
- **`MES-G1`** à **`MES-G4`** : règles du tableau du § 4.3.
- Toute mesure de vitesse se juge d'abord sur des trames LiDAR réelles ; les uniformes complètent.

## 12. Ordre de travail

1. **T2-a** : ce contrat contre-lu ; témoins de la tour gravés dans `reference/` ; `MES-M7` et `MES-G1` (hors ligne,
   comptes déterministes en local, temps sur G4).
2. **T2-b** : port de la voie CPU de référence (G, T, M, V, R) nourrie par l'adaptateur de test du catalogue de la v11,
   puis par le catalogue de T1 ; `MES-M0` à K5 sur les uniformes et une trame en local, K10 sur G4.
3. **T2-c** : G parallèle et leviers mesurés sur G4 ; budgets.

## 13. Questions pour les auditeurs

1. La lecture des cibles « cellule » comme élément de la jonction déjà traitée (§ 4.1) remplace la phase de pointeurs
   pour la forêt : voyez-vous un cas où la composante de la jonction de $b'$ à la coupe ouverte de la jonction courante
   diffère de celle de la cible du premier représentant de $b'$ ?
2. `G-L3` : $k$ sites strictement intérieurs exhibés par des tests exacts prouvent $p\geq k$ sans census ; le saut vers
   les $k$ plus petits `SiteIdx` d'entre eux est-il, pour vous, un pas valide du théorème D dans tous les cas ?
3. `LEM-HORS-CAT` repose sur $q_{\min}\leq 4$ dans $\mathbb{R}^{3}$ pour une coquille quelconque (cosphérique comprise) :
   l'énoncé vous paraît-il complet ?
4. L'adaptateur de test du catalogue de la v11 pendant le développement respecte-t-il la règle « un seul chemin
   produit » telle que vous la lisez ?
