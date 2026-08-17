# Lecture v3 — état courant, pistes fermées, cycle documentaire, recommandations V4

Sources lues intégralement, dans l'ordre :

1. `/home/user/E-HGP/morsehgp3D_v3/README.md` (1082 lignes)
2. `/home/user/E-HGP/morsehgp3D_v3/audits/README.md` (409 lignes)
3. `/home/user/E-HGP/morsehgp3D_v3/audits/AUDIT_ETAT_COURANT.md` (1707 lignes, daté 15 août 2026 UTC — « unique verdict mutable »)
4. `/home/user/E-HGP/morsehgp3D_v3/audits/PISTES_FERMEES.md` (100 lignes)

Note de portée : `PROPOSITION.md` (proposition consolidée) et les audits individuels ne faisaient pas partie de la mission ; plusieurs détails (sampler combinadique u128, § 6bis.6, préfiltre bilinéaire 36 valeurs) n'y sont connus que par citation.

---

## 1. État courant déclaré de la v3

### 1.1 Cadre et non-claims

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_bounded_oracles_and_g4_diagnostic
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

- Rien n'est promu au registre officiel ; aucun statut public, aucune qualification GPU, aucune exactitude déclarée sur un domaine public.
- Pins de référence : `HEAD` live du préfiltre `66b4f0c` ; pin logiciel stable relu `6e815d28b3e229a0161eb00d6fa0c9a272efac5d` ; pin noyau `Lane4` `a369452` ; brut G4 récupéré committé à `c03c0ee` (pin de production `11130cb`).
- Le contrat SLO est **ouvert** : à `n=50000`, `K_max=10`, aucun échantillon ne qualifie le payload complet sous `p95 warm_e2e<1 s` sur G4. Aucun kernel GPU bout-en-bout reçu. Le seul reçu G4 « chaîne complète » est `GPU_RUN=NO`, `78,8 s` CPU sur `uniform,50000`, sans BallKey, census, fold ni payload.
- Le profil est strictement `quantized_u16_input_only`. Le contrat binary64 est un profil **distinct et ouvert** : un arrondi ponctuel fixe non injectif ne préserve pas le HGP (deux points de part et d'autre d'une frontière InSphere peuvent fusionner). Q15–Q17 tranchées en ce sens.
- Q14 fermée **contractuellement** : aucune structure de Delaunay d'aucun ordre (même ordre un) n'est autorisée comme source, squelette, filtre ou census.

### 1.2 La définition de la source (le consensus mathématique reçu)

La source HGP n'énumère jamais une famille continue de sphères. Trois contrats de miniboule, un par arité :

- **q2** : une boule **diamétrale** par paire de positions distinctes (`D=||b-a||^2>0`) ; centre = milieu exact ; `(z-a) dot (b-z)>0` décide l'intérieur strict, égalité = shell. Survit avec au plus **9** intérieurs (`I<=9`), mort à **10**.
- **q3** : une circum-boule par triangle **strictement aigu** ; centre intrinsèque au plan, mais boule et census **ambiants** en 3D. `I<=8`, mort à **9**. Acuité recertifiée par `E+X-D=-2H`, face aiguë ssi `H<0` (le signe avait été inversé à `3703097`, réparé à `a73161c`).
- **q4** : une circumsphère par tétraèdre **bien centré** — l'autorité est la stricte positivité des quatre poids barycentriques du circumcentre (`c8::bien_centre` par Cramer + signes de faces, équivalence prouvée sous entrée u16), pas « quatre faces aiguës ». `I<=7`, mort à **8**.

Seuils de mort `10/9/8` sous `smax=11` ; le préfiltre les exprime par `h_q = s_max - q + 1`. Chaque support est possédé par son **arête maximale canonique** (owner par vrais `PointId`, jamais rang Morton). Les domaines de centres attachés à une ancre partielle (Jung, spindle, etc.) sont **exclusivement des accélérateurs de prune** — leur échec n'ajoute aucun événement, aucune cascade de rang. Contrat sandwich sur un domaine de centres contenant le centre canonique : `U<=D<=C` (`U` témoins universels singleton, `D` profondeur collective, `C` profondeur au centre canonique) ; `C<h` saute, `U>=h` ferme, `U<h<=C` exige `tau(F)`, sweep ou split.

Dégénérescences : Carathéodory donne la supersource finie (base de taille ≤ 4 ssi `c in conv(U_B)`), mais publier exige `c in relint(conv(U_B))` + rang fermé + disposition sur le shell complet. Sous `RelevantGP` (`|I_B|+|S|<=smax`), la positivité suffit ; hors domaine, `unsupported_degeneracy/plateau_pending`, jamais un jitter. Positions dupliquées : seule une paire endpoint `D=0` est filtrée ; tous les `PointId` et leur multiplicité restent dans pools, produits et census (règle de supersession globale — toute ancienne phrase de rejet global est périmée).

### 1.3 La voie active : fork de trois producteurs autonomes

```text
NeutralPairPartition
  |- Lane2(Pair2 -> Q2MidballDepth10)
  |- Lane3(PairAnchor3 x Third3 -> Q3MiniballDepth9)
  `- Lane4(PairAnchor4 x Q4Seed3 x Fourth4 -> Q4SeedAxisTopR4 -> Positive4)
```

- Les trois lanes peuvent partager `PointStore`, index Morton et une `NeutralPairPartition` WSPD **immuable** ; mais queues, records, caps, continuations et preuves de complétude sont **disjoints**. Aucun record inter-lanes (`Q4Seed3` appartient à `Lane4`, jamais une sortie q3 ; `PairAnchor3` créé dans `Lane3`, jamais lu par q2). La sonde J0 actuelle viole cela (vecteur `acu` partagé q3/q4) — interdit comme architecture.
- Les sorties rejoignent seulement ensuite `BallKey/RLE` puis census.
- **Déblocage clé de Lane4** — théorème `Q4SeedAxisExtremalCompletion-r4` : pour un `Q4Seed3` aigu exact avec `p` intérieurs permanents, les centres compatibles vivent sur un segment axial où chaque site a une puissance affine `A_z - tau*B_z` ; tout quatrième sommet régulier shallow est parmi les `8-p` premières racines entrantes (`B>0`) ou les `8-p` dernières sortantes (`B<0`) : **au plus seize groupes**, ties complets, reconstruction exacte de `I_B/U_B` par replay (sous `PointId` injectifs et disjoints — précondition **pas encore imposée** par l'API, P0). Pour une arête owner à `m_e` lignes carrier : au plus `8m_e` centres shallow, `16m_e^acute` propositions sur les seules faces aiguës — c'est le remplacement déterministe de `binom(m_e,2)`. `r4=smax-3` paramétrique, capacité de shell prouvée **163**, `MORT_GAP` refusé au replay (`a369452`).
- Jointure q4 : `Acute(x) OR Acute(y)`, **jamais AND** ; si les deux, le plus petit `PointId` aigu est le primaire (exact-once).
- **Déblocage arithmétique** (microkernel Gram unifié) : pour `S={a,a+m_1,...,a+m_k}`, `G=M^T M`, `Delta=det(G)`, `r=adj(G) diag(G)` ; numérateurs des poids = `r_i` et `2 Delta - sum r_i` ; `Phi_S(z)=Delta||z-a||^2-(z-a)^T M r` décide exactement intérieur/shell/extérieur. Redonne q2/q3, et en q4 `Delta=O^2`, `Phi=O*J` sans choix d'orientation. Vérifié algébriquement sur 10 000 fixtures u16 (seed `20260814`) mais **aucun microkernel C++ reçu**. Terminal q4 : `T=det3(v1,v2,v3)`, `Q=sum_i ||v_i||^2 (v_j cross v_k)` ; numérateurs Cramer < 106 bits, BallForm `T/Qbar` < 87 bits sur u16 ; blocs au signe indécis : `Phi` < 137 bits mais enclosure naïve jusqu'à 140 bits signés — i192 sûr, i192 vs i256 à arbitrer au coût.
- **Census partagé** : après RLE, toute `BallForm=(A,B,C)`, `A>0`, est une somme de trois quadratiques convexes séparées par axe → extrema exacts par axe sur AABB u16 ; `active_arity_mask` efface q2/q3/q4 à `10/9/8` intérieurs. Un seul backend de census pour les trois lanes.

### 1.4 La voie du 15 août : « tuer l'ancre avant de l'instruire » (= la V4)

Constat chiffré qui la motive : masse candidate = **6 914×** la population retenue ; la chaîne par ancre matérialise **6,09 milliards** de paires de lentille pour **21,4 millions** de candidats à 50 000 points. « Tant que ce rapport est à quatre chiffres, aucune optimisation constante ne compte. »

**L'idée.** Une ancre `(a,b)` ne porte aucun support d'arité `q` dès que `|P inter W_q(a,b)| >= h_q`, `h_q = s_max - q + 1`. On **minore** ce cardinal par trois comptes par rectangle Callahan–Kosaraju :

- `h_coeur` — témoins universels de tout le rectangle, hors `A` et hors `B` ;
- `h_a` — témoins de `A` universels sur `{a} x B` ;
- `h_b` — témoins de `B` universels sur `A x {b}`.

La somme est licite : les trois ensembles sont deux à deux disjoints, et la disjonction est **automatique** (pour `z` dans `A`, le choix `a=z` donne `H=0` — un point de `A` n'est jamais certifié témoin du cœur). Le minorant est **fail-open**. Une fois les trois comptes connus, la décision se lit sur un **histogramme de `h_b` à onze cases** (`s_max=11`) en `O(|A|+|B|)`, sans matérialiser `A x B`. La formation de `h_a/h_b` coûte encore `O(|A|^2+|B|^2)` (auto-jointures) — c'est le poste ouvert.

**Les quatre optimisations, dans l'ordre de leur gain :**

1. `Xi` est convexe en `a` (`(b-a) x (z-a)` affine en `a`) → maximum à un sommet ; mais `xi_max_over_box` maximise les composantes séparément (majorant sûr, pas le max corrélé) — l'autorité exacte sur l'enveloppe AABB continue au témoin ponctuel est `corner64_all_lane` (et `corner512_all_lane` pour `A x B x C`) ;
2. élaguer la descente sur `max_z min_{a,b} H`, pas sur `max H` ;
3. **une seule descente pour les trois lanes** (fuseaux emboîtés `W_4 < W_3 < W_2`) ;
4. une seule évaluation de `(H, Xi)` par point (indépendante de l'arité).

Ensemble : `n=8000` passe de **5 min 08 à 57 s** à résultat identique.

**Sur une ancre survivante** (PROPOSITION § 6bis.6) : q3 = une requête sur `lentille \ boule diamétrale` où **l'acuité vaut positivité** et ne coûte rien de plus ; q4 = un seed dans la même région puis le théorème axial (au plus seize quatrièmes sommets).

**Verdicts quantitatifs live (avec leurs rétractations) :**

- **Dual-tree rétracté comme gain** : le `2,2–3,0x` annoncé comparait une baseline **non fusionnée** ; face à une baseline elle-même multi-lane, le dual-tree coûte `+0,22` à `+30 %`. La vraie optimisation est la **fusion des trois lanes** (`3,0–4,0x` en évaluations, `1,9–2,4x` en temps de paroi — dont la redondance corner8 réelle est `1+7 P(H>0)`, mesurée `1,245–1,845`, pas huit). Trois pièges gravés : sans masque de lanes le filtre ferme à tort (`212` et `1 525` fausses morts en régime tendu) ; la diagonale lue au niveau nœud annule tout (`ha_somme=0`) ; sans cutoff la méthode coûte 2 à 4× ce qu'elle remplace. L'auto-jointure dual-tree à range-add rend les **mêmes valeurs** et reste une piste active.
- **Séparation** : le facteur `6,4` de `s=8` vs `s=6` est rétracté à `99,052 %` (masse hors `cap-cellule=512`) ; sur la seule masse jugée, `s=8` gagne `17,238 %` à `n=32000` contre `17,503 %` à `n=8000` — effet intrinsèque quasi invariant en `n`. `s=10` retire `9–22 %` du résiduel de `s=8`, gain croissant lentement avec `n` (`9,3/10,2/10,5 %` sur terrain). Contrôle interne décisif : `V4_pair_walive` est **identique** pour `s in {8,10,12}` à `n` fixé — l'ensemble `W`-vivant est une propriété du nuage, pas de la partition.
- **Écart au vrai vivant** : mou `mu` croissant lentement en `n` (`1,245 -> 1,436` sur terrain pour un facteur huit), convergeant vers 1 en `s` (`1,050` à `s=16`) ; fraction retirable `1-1/mu = 22,4 %` (pas 33 %). Exposants du `W`-vivant : `1,068–1,163` — **quasi linéaire**.
- **Borne couplée** (§ 3.3) : naît à `39 %` de la séparation exigée par la décorrélée, `4–21 %` de témoins en plus ; écart en `O(r)`, gain s'estompant au-delà du seuil.
- **Porteur aigu** : `x` porteur de `(a,b)` ssi `x` dans la lentille **et** `H<0` (stricte : `H=0` est l'angle droit, un premier jet non strict faisait 185 écarts sur 331 857 triplets). Corollaire : un témoin q2 est exactement un non-porteur. Deux élagages réfutés (certificat au niveau rectangle : gain `1,005` ; boule seule : `0,33–0,59`, plus lente que le balayage) ; la version à deux disjoints rend `13,29x` sur `two_lines`.
- **Gateway ternaire `A x B x C`** : exact et sûr (`Phi`, `Delta_E`, `Delta_X` séparables par axe, extrema `O(1)` gardant la corrélation par `a_k`) ; `two_lines` meurt avec `pairid_expanded=0` ; mais la mesure de croissance **réfute la sparsité annoncée** : la masse des porteurs croît en `n^3` quand on énumère les triangles aigus **sans** le filtre de vivacité d'ancre. Le juge au niveau nuage y a trouvé quatre fautes de récursion invisibles à l'oracle sur boîtes.
- **Résiduel absolu = l'obstacle restant** : `1 026` ancres q4 par point à `eight_clusters,n=32000,s=8,K=10`, contre `428` supports par point à `n=50000`. Le préfiltre retire une part importante du travail, **pas l'ordre de grandeur**.
- Réparations récentes : double crédit q2 réparé (masque de lanes par frame) mais le reçu q2 historique reste invalide ; primitive cœur-boule ouverte de `6220ea3` reçue comme sous-certificat borné (fast path devant Corner64) ; boule d'apex de `66b4f0c` **bloquée** (oublie le signe de `gamma_q`, faux mort à `separation=1` ; correctif `3W>N^2` q3, `2W>N^2` q4 avant le carré) ; P0 de garde : `--coord` hors u16 accepté et peut déborder.
- Enveloppe de calottes (`95b41b7`) : `88,00 %` certifié sur uniform, `14,33 %` sur terrain — **à recertifier** (le classifieur arrondit `||s||` par `floor(sqrt(ds))+1` et peut omettre une cellule ; correctif exact : propager `T2=max(r*r,||s||^2)` et comparer les carrés, sans sqrt ; fixture : `s=(10,0,0)`, cellule `(2,0,-6),(3,0,-5),(2,-1,-5)`, `3600>3400` exact mais le code teste `3600>4114`). Version sûre : marquer fail-open les directions `2(s.u)>max(r,||s||)`, puis seuils `10/9/8` indépendants. Le résiduel terrain, q3 et la masse hors cutoff restent à fermer.

Statut : `counter-only`, compte des **ancres**, jamais des supports ; un rectangle non décidé compte toutes ses paires comme survivantes (la mesure majore le résiduel).

### 1.5 État GPU / échelle

- Pivot axial matériel : à `n=6000,smax=6`, même sortie q4 (`89 796`), `830 044` roots au lieu de `48 791 131` couples (~`59x` moins) ; mur restant `25–28 s` car chaque seed rescane son CSR témoin. Jalons ordonnés : brancher `census_replay` (supprime le second scan), factoriser par arête (`Lane4EdgeBatch` : `S_ab` stocké une fois, ~`10,90/11,26/11,46` seeds par arête à `n=1500/3000/6000`), sélection plate cinq passes → deux, puis descente `Q_theta=q*A-p*B` sur Morton BVH (supprime le premier scan).
- Reçu CUDA borné (`c03c0ee`, verdict `AXIS_FLAT_PREFIX_PARITY`) : `ecarts=0` sur `18 617 211` seeds / `5 789 713 735` incidences ; kernel plat `4 999,97–13 979,84 Msites/s`, `55,33 ms` pour `293,6 M` incidences au plus grand lot complet ; `179,4x–292,1x` le même scan host. Seuls trois lots `cap=0` (`uniform,smax=6,n=1500/3000/6000`) ; tous les `smax=11` tronqués (`cap=1`) → **aucun chiffre K=10 reçu** (le `~750 ms` de `RESULTATS.md` n'est pas reçu). Projection diagnostique `0,46–0,48 s` kernel seul à 50k, profil K=5 — la seconde est « non réfutée », pas mesurée. Compilé arch `52`, pas Blackwell natif. Le monolithe extrapolé est impossible : `2,45 G` incidences > offsets CSR `int`, ~`9,8 Go` d'IDs, ~`5,42 Go` de `SeedOut` → produire/sélectionner/compacter **par tuile sur device**.
- Tuilage exact prouvé (borne HWM) : propriétaire = sommet lexicographique minimal, halo entier `ceil(3*dmax/2)` (car `2R<=sqrt(3/2)*D` en q4), recollement des multiensembles à IDs globaux ; ne certifie pas le cutoff `dmax` lui-même. Raccourci `Q.size()<5 => tuile vide` incomplet (planchers `2/3/4` par lane).
- Rampe J0 : verdict `INCOMPLET_OU_TRONQUE` — `uniform,50000` : `21 432 482` candidats `smax=11` en 38 s CPU, `4 004 994` à `smax=6` en 4 s (repli `5,35x`, pas 12) ; `6 091 112 797` paires de lentille (`623,5` par q4 retenu) ; `eight_clusters,12500,smax=11` : `24 135 659 695` paires puis refus. Aucun chiffre amas 25k/50k n'existe.
- Recettes G4 interdites tant que : parser incompatible (sweeps vs exact-once), décision avant rapatriement des réfutations, pas de deadline globale, P0 d'identités ouvert.

### 1.6 P0 ouverts et ordre bloquant

`0A` (BallForm→BallKey→census→BallEvent exact) **ouvert sur u16** : numérateurs q3/q4 de 81/67 bits rabattus en `int64` ; intermédiaires > 128 bits ; UBSan overflow signé à `ball_event.hpp:290` ; juge partageant identités avec le sujet ; ABI (`PointId`, epoch, profil, schéma, lanes, complétude, publication transactionnelle) non reçue ; census payé par support avant RLE. `0B` **n'existe pas** : le fold live est une DSU de `PointId` sur `I_B union U_B` — juge un goulot `K=1`, faux dès `k=2` (deux générateurs partageant un seul point doivent rester séparés) ; manquent générateurs/facettes par ordre, lots de niveaux égaux à racines gelées, naissances/multifusions/coverage, neuf verticales et payload. Le juge Floyd est borné (20 Go / `1,25e14` triplets à 50k).

Ordre bloquant intégral :

```text
réparer 0A u16 et isoler les juges de mutants
  -> raccorder BallEvent aux autorités Gamma par ordres/lots/verticales
  -> recevoir 0B et le payload borné
  -> recevoir manifeste NoDelaunay + NeutralPairPartition exacte et immuable
  -> FORK Lane2 : Pair2 -> MidballDepth10 -> BallKey/RLE
  -> FORK Lane3 : PairAnchor3 x Third3 -> Q3MiniballDepth9 -> BallKey/RLE
  -> FORK Lane4 : PairAnchor4 x Q4Seed3 x Fourth4
       -> Q4SeedAxisTopR4 -> owner6/primary/Positive4 -> BallKey/RLE
  -> prouver séparément couverture, caps, pending=0 et payload de chaque fork
  -> mesurer leurs tâches, racines, sorties, octets, census, H et HWM
  -> seulement alors portage device et campagne G4 50k
```

Une piste parallèle `counter-only` sur petit `n` contre l'oracle exhaustif est autorisée (morts `T2/permanents/gaps`, groupes de racines, splits, comparaisons larges, octets, HWM) — elle ne ferme ni 0A ni 0B.

### 1.7 Ce qui est mort ou rétrogradé (hors PISTES_FERMEES)

- **`CKPairTape -> OwnedCK-WST3 -> OwnedCK-WST4`** (produit de cellules) : rétrogradé en **diagnostic de masse historique**, jamais l'ordonnance P0 ; statut réel `CandidateCover` (couverture adressée par owner choisi a posteriori, pas exact-once physique : ancres non-owner, diagonales, tie Morton, positions dupliquées rejetées à tort). Masse WST4 ~`n^4`, `187 M` blocs à `n=8000` ; le compteur `Sym2` récursif atteint `459 477 476` nœuds à `n=4000` contre `141 468` couples plats.
- **SOC64 / BlockJungDual64 / HCBlockDepth / Midball raccordé** : primitives et théorèmes reçus (SOC64 : q3 `H>0 && 4H^2>EX`, q4 `H>0 && 3H^2>EX`, 64 coins = `ALL` exact sur l'enveloppe ; BJD : `A0>0 && 2A0^2>||C0||^2` q4, `3` pour q3, 64 coins exacts, i128 sous `1<=W<=65535`) mais **no-go comme hot path** : « le certificat évalué à chaque nœud visité ne peut pas économiser plus de visites qu'il n'en coûte ». BJD à `n=1500` : lectures strictement identiques, CPU médian `+5,47/+8,15 %`, gain de masse `12,55 %/0,87 %`.
- **`--borne-sup` multivue** : réfuté (perd des fermetures avec vue combinée et BJD) ; l'invariant mono-vue `P=cred+sum(pop)` reste exact.
- **Sampler `--masse`** : non uniforme dans les quadruplets, positivité forcée `centre=true`, cumul `double`, accepté sans `--ordre=4` — réfuté.
- **`--supports-retenus`** : mesure `Positive4 intersect {I<=7}` des `SupportKey` (`61–73 n` uniform, `30–35 n` amas), pas `H4_rank` ; « localité de rayon » = arête/espacement, proxy à facteur deux après positivité.
- **Sonde J0 (`acd792d`)** : faux vert `two_lines` (20 q2 contre 45 au brute), owner q3 dupliquant un isocèle à deux arêtes maximales, vecteur `acu` partagé — les 11 CTests verts n'exercent aucun de ces P0.
- **Réouvert** : `directional_dominance` (retiré par erreur de classement avec les cellules de centres ; jamais réfuté ; rejeu counter-only : fermeture q4 `3,13 % -> 41,6 %` uniform, `0,60 % -> 27,1 %` amas entre `n=2000` et `n=8000`).
- Contre-familles normatives : `two_lines` (masse quadratique, zéro carrier aigu, zéro q4 — la source doit rendre zéro sans développer de `PairId`) ; fixture 50k à `12 499` distracteurs réfutant toute source ancrée `k<=12499` (aucun cutoff kNN universel) ; fixture de bloc `4096` supports (`A=(20000,20000,20000)+{0,1}^3`, etc.) prouvant une masse q4 positive quartique **avant rang**.

---

## 2. Pistes fermées — liste complète (PISTES_FERMEES.md)

Rappel du statut : mémo, jamais une autorité ; textes intégraux supprimés le 15 août 2026, détail dans l'historique Git.

### Front, localité, chambres

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 1 | Front de Jung coalescé par dual-tree d'ancres | `4,85 -> 23,84` M de visites q3 entre `n=500` et `1000` ; pentes `2,30/2,33` contre une porte à `1,35` | Front coalescé à `141,18 n` |
| 2 | Banque directionnelle de chambres Yao-48 | La chambre fait `54,74°` quand un témoin de Jung exige `<35,26°` (q3) et `<31,13°` (q4) ; la condition q4 délimite un **anneau**, pas un préfixe radial | `3 D_i^2 < D_j^2` place `b_i` dans la boule diamétrale, exact en `i64` |
| 3 | Génération locale exacte par cône (`certified_locality_probe`) | Faux vert : `681/795/174` contre `681/884/202` au juge exhaustif ; `4,65 s` contre `1,39 s` pour le scan remplacé | Contre-fixture extra-shell / support non unique |
| 4 | Cône cible par endpoint alimenté par banque k-NN | Aucune série ne ferme deux pentes `<=1,35` ; `39,2` M de tests témoin-nœud à `n=2000` | **Le noyau ponctuel `H>0, 4H^2>E_2X_2` / `3H^2>E_2X_2` et la porte `ALL` par huit coins** — repris tel quel |

### Cellules de centres

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 5 | Source S par listes imbriquées de cellules de centres | Snapshot non transférable au source live ; supersédée par `CKPairTape` | **Le lemme profondeur–cellule** `beta <= R_p(C)` |
| 6 | Juge rationnel indépendant des cellules | Porte vacueuse : driver sans `--judge`, refus code 2 converti en vert par `WILL_FAIL` ; six vérités manquantes une fois le flux muté soumis | Positivité barycentrique stricte = centre dans `relint conv(U)` |
| 7 | Sentinelle top-`(12-q)` hors support, parallélisée | Ne réduit ni les cellules ni les `839 582 666` occurrences, **et casse la télémétrie** : `7 012` occurrences contre `22 543` lifts, code retour zéro | Le théorème de la sentinelle et sa minimalité |
| 8 | Pentes vertes de `uniform` comme propriété du générateur | Binaire non gelé, `wall_s` relevés sous charge concurrente | `coord = sqrt(25 n)` fait croître la boîte de `terrain` en `n^{1,5}` |

### Source par ancre, lentille aiguë

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 9 | Coupure de lentille aiguë fermant des chambres de paires | Carrier aigu sur **`300/300`** paires échantillonnées à `eight_clusters,n=50000` | **Le théorème de face adjacente aiguë**, encore porté par la lane q4 |
| 10 | Couple de carriers dans la lentille ⟹ ancre diamétrale | Fixture à cinq points : `||x-y||^2 = 144 > D^2 = 100`, le centre sort de l'ellipse, rang `4` au lieu de `5` | La correction `||x-y||^2 <= D^2`, posée avant le produit q4 |

### Ledger, owner, GPU

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 11 | Premier ledger des causes de lifts | Ne ferme pas : `130 033` occurrences sans attribution ; quotients divisant trois populations par les seules acceptations | Rejets owner `96,1 / 91,7 / 92,1 %`, qui motivent le groupement avant lift |
| 12 | Histogramme de multiplicité `SupportKey` | Ses trois issues sont un **stade maximal**, pas des propriétés orthogonales | La fermeture à écart nul par arité |
| 13 | Réemploi du prune Yao48 `P1a` de la ligne enregistrée | **Aucun prune ne survit au portage** : `dist2 >= 3D` est faux comme preuve de dix intérieurs stricts — un témoin tombe sur la coquille | Deux fixtures q2 gravées |
| 14 | Déduplication `SupportKey` avant géométrie | Diagnostic devenu la route : `39,24` géométries par support et `81,6 %` de rejets owner à `n=50000` | Le théorème du minimum auto-centré, qui fonde le « q3 par droite » |
| 15 | Certificat de Helly sur le disque de Jung | Reste **ponctuel** ; `F_k` autour de `180` bits sous u16, hors `i128` | Le sous-certificat de taille au plus trois |
| 16 | Cœur universel de Jung sur arête maximale | Ne borne ni le nombre d'ancres ni le coût ; pire cas quadratique | **Les relaxations `3||U||^2 < D^2` et `15||U||^2 <= 4D^2`** — les lanes q3/q4 actuelles |
| 17 | Gate à trois voies comparant trois certificats | Juxtapose `n=12500`, `150`, `600` avec des ELF et univers différents, sans union commune ni pente | La récursion `A x A` en trois cas, avec la porte `paires_couvertes == C(n,2)` |

### Ordre k, Gabriel, front inverse

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 18 | K-graphe de Gabriel brut | Fixture `E5` : deux non-Gabriel rattachent une facette sans nouveau `PointId` ; deux composantes subsistent jusqu'à `24` | L'étoile silencieuse : `<= k-1` attaches au lieu d'une clique |
| 19 | Route sparse « directes + gateways » | Pivot dans l'union des supports **faux sur un carré cosphérique** ; les `68,07` records par point sont des supports proposés, pas des cofaces reçues | La clé de niveau `beta = N/(4D)` et ses bornes u16 |
| 20 | Borne de degré q2 par chambre canonique | **Treize** partenaires q2 dans une seule chambre, sans plateau ni cosphère — tout cap 12 est réfuté | La contre-fixture, et le feu vert au filtre flottant certifié |
| 21 | Source S comme front inverse par transitions | Le graphe **n'est pas connexe** : deux q4 de niveau zéro ne partagent qu'une arête, jamais une facette | Quatre contre-fixtures, dont `plateau_carre_multifusion` |
| 22 | Fenêtre top-`M` par ancre | Zéros limités aux `SupportKey` et deux cardinalités ; `1 277` supports jamais proposés sur `eight_clusters` | **La fixture d'égalité `delta_out^2 = 100 = 4R^2`**, qui impose l'inégalité stricte |
| 23 | Parcours de l'arrangement relevé (BFS puis GPU) | Ses trois énoncés fondateurs sont faux hors position simple ; volume quadratique là où la sortie est linéaire ; `1 270` sommets par point à `n=800` pour `300` sphères | `order_k_flats.hpp`, qui le remplace |

### Boule d'apex unique pour `h_a` (15 août 2026, entrée longue)

| # | Idée | Cause d'abandon | Ce qui survit |
|---|---|---|---|
| 24 | Boule inscrite dans le cône suffisant d'apex `a`, demi-ouverture `gamma_q = theta'_q - arcsin((r_B+2r_A)/D)`, pour remplacer les auto-jointures `O(|A|^2+|B|^2)` (Q23) | Sur les trois mesures `n=4000,s=6` : plus lente et perd jusqu'à `11` points de fermeture q4 ; P0 hors des portes : le carré de `sin(gamma_q)` oublie le signe, fixture u16 à `separation=1` donne `oracle_faux_morts=1` | Le théorème du cône une fois la garde `3W>N^2` (q3) / `2W>N^2` (q4) ajoutée, son test `ALL` par huit coins, le compteur `travail_ha` ; **l'auto-jointure dual-tree à range-add reste une piste active** (non implémentée) et ne doit pas être enterrée avec la boule unique. La mesure ne prouve pas que l'auto-jointure vaut `O(|A| h_q)` : l'arrêt après `h_q` succès laisse un pire cas quadratique sur les échecs |

### Les deux motifs d'échec transverses (gravés dans le mémo)

1. **La porte vacueuse.** Verte sans rien prouver : refus converti en succès par `WILL_FAIL`, regex qui ignore le code de retour, quantificateur `{n}` lu littéralement par `cmsys::RegularExpression`. Toute porte doit exhiber son plancher de couverture et son mutant tué.
2. **Le certificat qui coûte plus qu'il ne rapporte.** Évalué à chaque nœud visité, il ne peut pas économiser plus de visites qu'il n'en coûte — sort commun de `SOC64`, `BlockJungDual`, `HCBlockDepth`. Un gain se mesure **apparié**, contre une exécution désarmée.

**Critère de réouverture** : nouveau théorème de complétude, fixture qui falsifie le motif d'abandon sans casser les contre-exemples, architecture sans structure globale interdite, porte de coût distincte. Un bon rappel empirique ne suffit jamais.

---

## 3. Le cycle documentaire des audits (à reconduire en V4)

### 3.1 Rôles des types de documents

- **`AUDIT_ETAT_COURANT.md`** : **unique verdict mutable**. Ancré au pin/`HEAD` exact (hash complet), à l'état du worktree, aux CTests rejoués et aux blocages. Tout le reste est daté et figé. Règle de fraîcheur : « un fichier daté ne devient jamais live ».
- **`NOTE_SOLUTION_*`** : spécification de solution **avant** implémentation (contrat mathématique, invariants, ordre d'implémentation) — ex. `NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE`, `NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES`. Peut être « autorité contractuelle mathématique, sans réception logicielle ».
- Implémentation dans `prototype/` + portes CMake (codes de sortie exacts via `mhgp3v_add_expected_code_test_for` : 1 = désaccords du juge, 2 = campagne refusée avant calcul, 3 = plancher/invariant violé, 4 = mutant tué ; crashs par signal refusés partout).
- **`AUDIT_LIVE_*` / `AUDIT_RECEPTION_*`** : réception, ancrée au hash court du commit audité (le hash figure dans le nom de fichier, ex. `AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md`).
- **`AUDIT_CONTRE_RECEPTION_*` / `AUDIT_CONTRE_SESSION_*` / `AUDIT_REAUDIT_*` / `AUDIT_WORKTREE_*` / `AUDIT_CONSTRUCTIF_*`** : contre-audits indépendants (rejeu, contre-fixtures, rétractations chiffrées). C'est là que vivent les rétractations explicites (« le gain `2,2–3,0x` est rétracté », « cinq rétractations y sont inscrites »).
- **`AUDIT_REQUALIFICATION_*`** : requalification d'un statut antérieur.
- **Dialogue** : `QUESTION[S]_CLAUDE_*` ↔ `REPONSE_*` (questions numérotées Q6–Q30… tranchées une à une) ; notes de travail de Claude : `NOTE_CLAUDE_*` (conservées **avec bannière de rétractation/supersession** quand leurs claims sont dépassés).
- **`PISTES_FERMEES.md`** : mémo condensé (idée / tueur / survivant), jamais une autorité, jamais citée comme réception.
- **`receipts/`** : diagnostics et reçus épinglés (hash ELF, hash source, commit, transcripts) ; leur statut est fixé par l'audit, pas par eux-mêmes.
- Principe cardinal : **les audits motivent les corrections, ils ne certifient rien** ; **les titres de commits restent des claims, jamais des verdicts reçus**.

### 3.2 Index et hygiène (`audits/README.md`)

- L'index n'est **pas un journal chronologique** : trois sections exhaustives — *Autorités actives* (avec résumé d'une phrase du verdict, y compris ses rétractations), *Dépendances historiques gardées* (uniquement parce que CMake/prototype/oracle/reçu les cite ; leur titre n'est pas une réception), *Scripts gardés*.
- **Tout fichier du dossier doit apparaître dans exactement une section. Un fichier déclaré nulle part est un défaut d'index, pas un fichier neutre.** (La v3 a découvert 29 fichiers sur 89 non déclarés alors que l'index affirmait le ménage fait.)
- Critère mécanique et rejouable d'évacuation vers `PISTES_FERMEES.md` : n'est évacué qu'un fichier ni autorité active, ni dépendance gardée, ni cité par `CMakeLists.txt`, un prototype, un oracle, un reçu, `PROPOSITION.md`, `README.md` ou `AUDIT_ETAT_COURANT.md`. Le texte intégral reste dans Git.
- Règles de supersession **globales** en tête d'index (ex. multiplicité des positions dupliquées) qui périment d'office toute phrase historique contraire.

### 3.3 Procédure avant toute conclusion (règle de fraîcheur)

1. lire `AUDIT_ETAT_COURANT.md` ;
2. comparer son `HEAD` et son worktree au dépôt ;
3. distinguer sujet, oracle, mutant, provenance et payload ;
4. conserver toute contradiction mathématique comme fixture ;
5. mettre à jour la proposition consolidée, puis **évacuer** la note absorbée en lui écrivant sa ligne dans `PISTES_FERMEES.md` — jamais la laisser traîner sans être déclarée.

---

## 4. Recommandations V4 : les erreurs de méthode v3 à ne pas reproduire

### 4.1 Portes et tests

1. **Jamais de porte à regex seule.** `PASS_REGULAR_EXPRESSION` ignore le code de retour ; `--selftest=1` a imprimé `accord=OUI` **puis** rendu code 3, plusieurs fois. Doubler tout regex par une porte à code de sortie exact (1/2/3/4), refuser les crashs par signal.
2. **Planchers de couverture contre le vert-par-vacuité** (`--min-*`) et **mutants tués** (`--inject`, `--force-*`) dès la première porte. Exemples de vacuité v3 : `--masse` accepté sans `--ordre=4` (ligne q4 factice en mode q3), `--rang` code 0 sans `--porteurs`, `WILL_FAIL` convertissant un refus code 2 en vert, `--fenetre-exhaustive` contournant les gates, `exige-q4-ouvert` seul rendant OK.
3. **Fixtures gravées aux coordonnées exactes** pour chaque contradiction mathématique, plus permutations, orientation opposée, extrêmes u16, hors-profil, mutants de poids nul/négatif — la v3 a systématiquement des fixtures nominales sans ces axes.
4. **Le juge réécrit sa propre arithmétique** (représentation volontairement différente) et un selftest juge le juge ; la v3 a payé plusieurs fois des prédicats recopiés du sujet (`q4_brute_oracle`, Corner8, scan `interieur_strict`) qui ne pouvaient pas voir un défaut commun.
5. Un juge « au niveau nuage » attrape ce que l'oracle sur boîtes ne voit pas (quatre fautes de récursion du gateway ternaire) — prévoir les deux niveaux.

### 4.2 Mesure et coût

6. **Tout gain se mesure apparié, contre une exécution désarmée, à binaire gelé.** Rétractations v3 : dual-tree `2,2–3,0x` (baseline non fusionnée), facteur `6,4` de `s=8` (`99,052 %` d'artefact de cap), `1,62` vs `1,57` (unités différentes), pentes `uniform` sous charge concurrente.
7. **Un certificat évalué à chaque nœud ne peut pas économiser plus qu'il ne coûte** — fermer les preuves **avant** la descente (proof-tile), pas pendant. Sort commun de SOC64/BJD/HCBlockDepth.
8. **Contrôle interne obligatoire** : une quantité indépendante du paramètre balayé (le `W`-vivant identique pour `s in {8,10,12}`) — c'est ce qui manquait aux campagnes antérieures.
9. **Petites tailles = correction uniquement ; pentes et coût à `n=8000/16000/32000`** ; trois points ne font pas une pente (les séries `terrain` déclinent à 32k alors que huit séries sur douze croissent). Jamais de juge `O(n^3)` ni de tableau indexé par paire à l'échelle : invariants globaux (masse de partition `= C(n,2)`, `pending=0`, aucun cap dépassé) + juge d'échantillon `O(Kn)` déterministe.
10. **Aucune rampe 50k avant les portes petites tailles** (`1500/3000/6000` avec `pending=0`, coût, HWM) ; aucune session G4 avant les gates CPU. Chaque campagne publie le bloc « porte de coût » complet : masses exclusives `CLOSED/OPEN/PENDING`, compteurs par étage, octets/HWM/temps par phase, commandes/seeds/commit/binaire/codes de sortie.
11. **Ledgers, pas vérités partielles** : un `--dmax` observé ne prouve pas la complétude ; une masse sautée porte un certificat local exact ou reste une continuation ; `DEBORDEMENT` continue ou refuse, jamais compté comme mort ; séparer `AttemptStats` de `TerminalLedger` (les compteurs de tentatives v3 dépassaient `380 %`), `M4_raw` de `residual_output` ; compter des **ancres** n'est pas compter des **supports**. Sceller/hasher le brut **pendant** le calcul, pas après récupération.

### 4.3 Architecture et contrats

12. **Lanes réellement autonomes** : aucun record matériel partagé (le vecteur `acu` de la sonde J0), seuls un `PointStore`, un index Morton et une partition neutre immuable sont mutualisables ; masque de lanes par tâche (le caller bisigne abandonnait un nœud dès qu'**un** bit concluait ; sans masque, `212` et `1 525` fausses morts au préfiltre).
13. **Owner sur vrais `PointId`** (jamais rang Morton), tie total par `EdgeKey`, et attention au tie q3 sur triangle isocèle à arêtes maximales égales (dupliqué par J0). Un juge qui choisit l'owner a posteriori et ne regarde que ce rectangle prouve une couverture adressée, pas l'exact-once de la relation émise.
14. **Conventions versionnées sans ambiguïté** : le paramètre `--echelle=num/den` a vécu avec code, commentaire et CMake en sens opposés ; imposer deux portes réciproques (`4/1` et `1/4`) qui distinguent les deux sens de toute convention.
15. **ABI typée** : jamais un booléen fusionnant `{positive, nonpositive, degenerate, invalid, numeric_failure}` ; jamais `kLaneNone` pour une invalidité (verdicts `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`) ; huit coins extérieurs ne prouvent **jamais** `NONE` ; tri-state `ALL/NONE/MIXED`, égalité ou cap = `PENDING`, jamais un verdict inventé.
16. **Arithmétique prouvée avant code** : préflighter le domaine u16 avant toute soustraction signée ; widening avant sommes/produits/normes ; pas de `sqrt` — comparer les carrés (`T2=max(r*r,ds)`) ; largeurs par étage (i128 hot path, i192/i256 résiduel, GMP replay) ; garde de signe avant tout carré (le `sin^2(gamma_q)` sans signe a produit une fausse fermeture).
17. **Fail-open partout** dans les préfiltres ; les erreurs v3 qui ont fermé à tort sont exactement des violations de ce principe (arrondi `+1` du rayon, calottes « admissibles », diagonale au niveau nœud).
18. **Positions dupliquées** : seule la paire endpoint `D=0` est impropre ; multiplicité et `PointId` conservés partout (plusieurs probes v3 rejetaient globalement, contraire au profil).
19. **Pas de watermark monotone par ancre** : runs scellés, triés, mergés par niveau exact avant le premier commit d'un lot ; « streamé » = mémoire résidente bornée, jamais fold en ligne sur source non scellée.
20. **Interdits structurels** à maintenir : aucune Delaunay d'aucun ordre, aucun catalogue `∝ C(n,k)`, aucun cutoff kNN universel (fixture des `12 499` distracteurs), `two_lines` doit mourir sans expansion de `PairId`.

### 4.4 Méthode documentaire

21. Tenir l'équivalent des quatre documents dès le premier jour (ÉTAT_COURANT mutable unique + index à trois sections exhaustives + PISTES_FERMEES + proposition consolidée), avec le critère mécanique d'évacuation — la v3 a laissé 29 fichiers non déclarés en croyant le ménage fait.
22. Bannières de rétractation sur toute note dépassée ; rétractations chiffrées explicites (la v3 en inscrit cinq dans une seule note) ; jamais promouvoir un titre de commit.
23. Ne pas re-fermer ce qui est fermé, ne pas enterrer ce qui ne l'est pas : `directional_dominance` retiré par erreur de classement ; l'auto-jointure dual-tree à range-add explicitement protégée de l'enterrement de la boule d'apex.

### 4.5 Acquis v3 directement réutilisables par la V4 (à ne pas repayer)

- Le préfiltre `h_coeur + h_a + h_b` avec disjonction automatique, fuseaux emboîtés `W_4 < W_3 < W_2`, histogramme `h_b`, une évaluation `(H,Xi)` par point, élagage sur `max_z min_{a,b} H` — et ses trois pièges gravés (masque de lanes, diagonale, cutoff).
- Le noyau ponctuel `H>0`, `4H^2>EX` (q3) / `3H^2>EX` (q4) et les autorités exactes `corner64/corner512` ; la borne couplée ; la garde de signe `3W>N^2`/`2W>N^2`.
- Le porteur aigu (`lentille et H<0`, strict), l'équivalence témoin q2 = non-porteur, la correction `||x-y||^2 <= D^2`.
- Le théorème axial q4 (16 groupes, `16m_e^acute`), la factorisation `S_ab` par arête (~11 seeds/arête), le tuilage owner lexicographique + halo `ceil(3*dmax/2)`.
- Le microkernel Gram/`BallForm` (`Delta`, `Phi`, `T/Qbar`, largeurs 87/106/137 bits), le census range-count partagé par arité, `active_arity_mask` à `10/9/8`.
- Les bornes de payload : Helly avec tolérance (`eta(3,h)<(h+1)^2` : ≤ **80 IDs q4**, **99 q3**), noyau d'axe **16 IDs** (`eta(2,8)=16`, i256 ~207 bits), `Depth=tau(E)` sur hypergraphe de rang trois.
- Les chiffres de dimensionnement : `W`-vivant quasi linéaire (`1,068–1,163`), mou `22,4 %`, `s=8 > s=6`, `s=10` retire `9–22 %` de plus, résiduel `1 026` ancres q4/point sur amas 32k vs `428` supports/point à 50k.
- Les contre-familles et fixtures normatives (two_lines, 12 499 distracteurs, bloc 4096, isocèle q3, `2×2` Jung non uniforme, `delta_out^2=4R^2`).

---

## Questions ouvertes / ambiguïtés

1. **Relation `smax` / `K`** : les textes emploient `smax=11` pour le régime K=10 et `smax=6` pour K=5 (donc apparemment `smax=K+1`), et `h_q=s_max-q+1` donne bien `10/9/8`. Mais aucune des quatre sources lues ne définit formellement `smax` (probablement `|I_B|+|S|<=smax` via `RelevantGP`). À vérifier dans `PROPOSITION.md` ou la spécification avant de figer les seuils V4.
2. **Le `57 s` à `n=8000`** du préfiltre ne précise ni la famille de nuage ni la machine ; il est cité sans reçu épinglé. De même le « facteur 6 914 » (masse candidate/retenue) est un chiffre de cadrage sans provenance détaillée dans les documents lus.
3. **`--dmax` / cutoff** : le tuilage et la rampe J0 opèrent sous un `dmax` jamais certifié (« ce lemme ne certifie pas le cutoff lui-même »). La V4, qui cherche l'arête maximale via WSPD, devra dire explicitement comment elle borne le diamètre des supports sans sentinelle réfutée — la v3 n'a pas de solution reçue.
4. **`h_a/h_b` sub-quadratique** : la roadmap V4 prévoit une structure dual-tree ; la v3 conclut que le dual-tree rend les mêmes valeurs mais coûte plus cher que la baseline fusionnée, tandis que « l'auto-jointure dual-tree à range-add » (non implémentée) reste la piste active. L'écart entre ces deux variantes (dual-tree mesuré vs range-add non implémenté) n'est pas détaillé dans les documents lus — voir `NOTE_CLAUDE_DUAL_TREE_Q23_20260815.md`.
5. **q3 dans la V4** : la roadmap dit « troisième témoin x formant un triangle aigu » ; la v3 précise que la boule q3 est **ambiante** (centre intrinsèque au plan, census 3D) et que l'acuité vaut positivité **sur `lentille \ boule diamétrale`**. Le détail opératoire est en § 6bis.6 de `PROPOSITION.md`, non lu ici.
6. **Séparation WSPD** : la V4 annonce `s=6/8/10` ; la v3 mesure `s=8` dominant `s=6` et `s=10` marginalement meilleur, mais ne publie **aucun verdict de temps** pour `s=10/12` (durées polluées par `--vrai-vivant`). L'arbitrage temps/sélectivité à `s=10` reste ouvert.
7. **GPU K=10 <100 ms** : le contrat V4 est plus dur que tout ce que la v3 a approché — le seul chiffre device reçu est `55,33 ms` pour un seul étage (sélection plate q4, K=5, `n=6000`, préfixes tronqués partout en `smax=11`). Aucune donnée v3 ne borne le census, le fold, ni H2D/D2H. La projection `~0,46–0,48 s` kernel seul à 50k/K=5 est explicitement non-reçue.
8. **« Dizaines de millions de points »** : la v3 n'a aucun chiffre au-delà de 50 000 ; la seule note de route (`NOTE_CLAUDE_PLAN_50K_PUIS_TRENTE_MILLIONS_20260814.md`) est « fraîche et non reçue ». Les bornes ABI (offsets CSR `int`, `INT_MAX`) sont le seul enseignement transférable.
9. **Auteur/provenance des audits** : l'état courant note qu'« aucun artefact durable ne permet d'identifier un second auteur ou modèle : Git attribue les deux flux documentaires au même auteur » — le dialogue auditeur/Claude est une convention documentaire, pas une indépendance organisationnelle. La V4 devrait décider si elle veut une indépendance réelle (deux environnements, deux implémentations) ou assumer la convention.
10. **Fixtures d'égalité q2 des relaxations** (`3||U||^2<D^2`, `15||U||^2<=4D^2`, piste fermée n° 16) : le mémo dit qu'elles « fondent les lanes q3/q4 actuelles » sans redonner leur dérivation ; à retrouver dans `PROPOSITION.md` avant réutilisation V4.
11. **Régimes de test V4** : la roadmap demande uniforme/terrain/clusters ; la v3 utilise `uniform`, `terrain`, `eight_clusters`, `two_lines` (+ `clusters` ponctuellement). `terrain` est systématiquement la famille qui résiste (calottes `14,33 %`, déclin à 32k, continuations q3/q4 à 25k/50k) — la V4 ne doit pas l'omettre, et `two_lines` est indispensable comme contre-famille de sparsité.
