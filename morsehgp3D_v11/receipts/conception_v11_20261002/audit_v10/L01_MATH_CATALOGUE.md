# L01 — Mathématiques du catalogue critique de morsehgp3D_v10

Audit pour la conception de `morsehgp3D_v11`. Rédigé le 2 octobre 2026 entre 06:53 et 07:12 UTC (heures lues par `date -u` au début et à la fin de la rédaction) ; exécutions de 05:33 à 07:10 UTC.

```text
phase=audit_v10_pour_v11 (hors registre)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_math
public_status=not_claimed
GCP non utilisé
```

Sujet : `morsehgp3D_v10` lu dans `/workspaces/E-HGP/build/v11-worktree`. La tâche est ancrée à `afb081774` ; pendant l'audit le worktree est passé à `52687f8e5` (commits d'audit et d'ouverture de la v11). Les dossiers `src/`, `reference/`, `tests/` et `cli/` de la v10 n'ont pas changé : `diff -rq` vide entre le worktree et ma copie de travail prise à 05:33 UTC. Empreintes sha256 : `generator.cpp` `d5996feaf0df…`, `catalogue.hpp` `2f47beb9dfc1…`, `support.hpp` `7cc62930f623…`, `hgp10_ref.py` `2cb84ad549b1…`, `GEN_v2.md` `71edbd8507b7…`, `SPEC_V10.md` `ca5f5785b9a6…` ; binaire de l'audit `mhgp10_catalogue` `a3bbad50b81b…` (Release, g++ 13.3, construit sous `/tmp/v11-audit/l01_math_catalogue/build`).

Rien n'a été modifié dans le dépôt ni dans les dossiers privés. Aucune commande git mutante, aucune commande GCP. Les coordonnées KITTI lues pour les contrôles restent sous `/tmp` ; le dossier de preuves ne contient que des scripts, des comptes et des empreintes.

## 0. Verdict

1. **Le générateur du catalogue est la partie la plus solide de la v10.** Je n'ai trouvé aucune faute mathématique ni aucun écart d'exécution. Chaque lemme invoqué (D, D-loc, G inclus dans D, L, C, ajustement J2c, K, Z, M, S, U, W, F) a une preuve complète, courte, sans position générale, et le code respecte ses hypothèses.
2. **Exactitude confirmée par des juges écrits pour cet audit, sans code commun avec le dépôt.** Oracle brut indépendant : 220 nuages (110 géométries dégénérées ou génériques, chacune avec et sans doublons), 3 960 exécutions, 432 015 enregistrements comparés textuellement (rang, $q_{\min}$, poids, drapeaux, $S^{*}$, I, U), 0 écart. À l'échelle du contrat (trames 08/000000, 000100, 000200 sans sol) : identité d'Euler exactement égale à 1 aux ordres 1 à 10, restriction $\mathrm{cat}(K)=\mathrm{restrict}(\mathrm{cat}(K+2))$ égale sur 6 comparaisons, recensement brut de 2 281 boules tirées au sort sans écart.
3. **Ce qui manque n'est pas une preuve, c'est son rangement et sa garde.** La preuve qui correspond au code n'existe dans aucun document unique : le document normatif (`GEN_v2.md`) décrit un autre algorithme, deux pièces ne sont écrites que hors dépôt, le registre des preuves n'a aucune entrée v10 pour le générateur. Aucun invariant global d'échelle (Euler, restriction, juge des boîtes) n'est dans le dépôt, alors qu'une boule de jonction manquante est invisible à la tour.
4. **Rien n'est prouvé sur le coût.** La linéarité est mesurée. Le coût de l'arbre dépend de façon critique de la marge $M-K$ de la taille de feuille : sur 14 points à K = 10, 1 nœud à M = 24, 3 048 725 nœuds à M = 13, 27 966 119 à M = 12.
5. **La taille de l'objet est le vrai sujet du contrat à 100 ms.** Sur la trame 02 (45 845 points) la tour FULL compte 1 683 088 nœuds à K = 5 et 7 468 379 à K = 10 (982 670 et 4 466 884 naissances), pour 1 407 885 et 5 483 320 boules. Le catalogue n'est pas surdimensionné : il est proportionnel à la sortie.

## 1. Périmètre lu

- `morsehgp3D_v10/docs/SPEC_V10.md` (entier), `docs/conception/GEN_v2.md` (entier), `docs/conception/CONCEPTION_V10.md` (§ 0 à 4, § 10 et décisions D-V10-02 à 07), `docs/conception/TOWER_v2.md` (§ 3, § 6.1, § 8, § 9).
- `src/catalogue/catalogue.hpp`, `support.hpp`, `generator.cpp` (les 876 lignes), `src/arith/geometry.hpp` et `geometry.cpp`, `src/core/types.hpp`, `reasons.def`, `src/cloud/cloud.hpp` et `cloud.cpp`, `cli/mhgp10_catalogue.cpp`, `src/tower/tower.hpp` et les passages de `tower.cpp` qui lisent le catalogue (lignes 23, 840 à 990, 1158).
- `reference/hgp10_ref.py`, `reference/test_ref.py`, `tests/oracle/test_catalogue_oracle.py`, `tests/oracle/test_tower_oracle.py`, `CMakeLists.txt`.
- Reçus `receipts/catalogue_filter_j2_20260929` (README et `RECU_AGENT_J2.md`), `catalogue_fitted_split_j2c_20260929` (README et `RECU_AGENT_J2C.md`), `g4_session4_j2c_20260929`, `g4_session5_scale_20260929`, `audit_v9_20260928/CONNAISSANCES_V10_20260928.md` (§ L13), `audit_geant_developpeur_20260930/RAPPORT_AUDIT_GEANT.md` (passages catalogue).
- Audits `audits/audit_continu_20260929/catalogue/` (les six notes), `audits/audit_independant_20260929/CONTRE_AUDIT_GEOMETRIE.md`.
- Hors dépôt, en lecture seule : `build/v10-persist/design/GEN_v1.md` et `TOWER_v1.md`, `build/v10-persist/design/gen_v2_probe/out/l13_pins.txt`, `build/v10-integration-r2/src/morsehgp3D_v10/src/catalogue/generator.cpp` (raccord R2 non poussé), `build/v10-fixes/faits_math`.
- Registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` et `docs/math/CATALOGUE_CRITIQUE_3D.md`.
- Ouverture de la v11 : `morsehgp3D_v11/README.md` et `docs/ARCHITECTURE.md` (lus pour ne pas contredire le cadre).

Les audits existants ont servi de pistes. Aucune de leurs affirmations n'est reprise ici sans contrôle dans le code ou par exécution ; quand je ne fais que citer, je le dis.

## 2. Méthode et exécutions

Lecture ligne à ligne du générateur ; chaque preuve a été refaite, puis confrontée à celle du dépôt ; puis exécutions. Tout a tourné sous `/tmp/v11-audit/l01_math_catalogue/`, à 3 fils au plus, sur une machine chargée (charge 7 à 20) : les temps cités ici ne valent rien, les compteurs et les égalités valent tout.

| Id | Exécution | Résultat |
| --- | --- | --- |
| E1 | porte du dépôt `test_catalogue_oracle.py` sur mon build | 161 contrôles, 0 écart, 14 132 boules |
| E2 | oracle brut indépendant `oracle_indep.py` (Gram par Cramer en `Fraction`, poids, $S^{*}$ minimal en ordre de Morton, rangs denses), 11 familles (les sphères entières en six rayons), K dans {1, 2, 3, 5, 10, 12}, trois réglages de feuille et de fils | 220 nuages, 3 960 exécutions, 432 015 enregistrements dont 158 121 à coquille pondérée et 75 666 à coquille étendue, 0 écart, 0 refus |
| E3 | catalogue des trois trames du contrat à K = 2, 3, 5, 7, 10, 12 | comptes ci-dessous ; 08/000200 : 1 407 885 boules à K5 et 5 483 320 à K10, répartition par $q_{\min}$ égale aux épingles |
| E4 | juge d'Euler indépendant `euler_juge.py` : validation de la formule sur petits nuages (267 contrôles, 5 133 boules dont 696 étendues), puis les trois trames avec $k_{\mathrm{cat}}=12$ | somme exactement égale à 1 aux ordres 1 à 10 sur les trois trames (8 025 829, 8 314 472 et 6 492 748 boules) ; deux mutants de comptes tués |
| E5 | restriction `jkm2.sh` : $\mathrm{cat}(K)$ contre $\mathrm{restrict}(\mathrm{cat}(K+2))$, flux canoniques hachés en sha256, trois trames, K = 5 et 10 | 6 égalités sur 6 |
| E6 | juge d'échantillon `j1_juge.py` : 2 281 boules tirées du dump K10 de la trame 02, recensement brut sur les 45 845 sites | 0 écart (I, U, p, u, drapeaux, $q_{\min}$, $S^{*}$, admission) |
| E7 | tour contre $\Gamma_k$ à K = 10 (`tower_oracle_k10.py`, nuages de 11 à 13 points, cinq genres) | 20 nuages, 19 320 coupes aux ordres 1 à 10, 0 écart |
| E8 | taille de feuille : 14 points, K = 10, `--leaf` = défaut, 13, 12, 11 | 1 nœud ; 3 048 725 ; 27 966 119 ; délai dépassé |
| E9 | amas denses très éloignés, feuille par défaut, 40 à 8 000 sites, séparations 512 à 262 144 | 16 à 26 nœuds par site, indépendants de la séparation |
| E10 | entrées dégénérées : sphères entières de 144, 168 et 312 points, cercle de 108 points, grille plane, grille $12^{3}$, droite | catalogue exact ou refus `wide_leaf` ; la tour refuse dès 25 sites de coquille |
| E11 | doublons à 1 mm sur 5 trames brutes du cache des démos et les 3 trames du contrat | 0 doublon sur les 8 nuages |
| E12 | tour FULL de la trame 02 à K = 5 et 10 (comptes de nœuds seulement) | 1 683 088 et 7 468 379 nœuds |

Scripts, journaux et empreintes : `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l01_math_catalogue/` (annexe C).

## 3. L'objet « catalogue critique », tel qu'il est implémenté

### 3.1 Définition

Entrée : positions distinctes $X\subset\mathbb{Z}^{3}$, $0\le x<2^{18}$ par axe, poids $w(x)\ge1$ (multiplicité), indice de site = rang de Morton (`cloud.cpp:38-72`).

Pour une sphère de centre $c$ et de rayon carré $a>0$ : $I=X\cap B^{\circ}(c,\sqrt{a})$, $U=X\cap\partial B(c,\sqrt{a})$, $p=w(I)$, $u=w(U)$, $m=\lvert U\rvert$.

- La sphère est **critique** si $c\in\mathrm{conv}(U)$. C'est équivalent à l'existence d'un **support** : $T\subseteq U$ de 2 à 4 sites affinement indépendants avec $c\in\mathrm{relint}\,\mathrm{conv}(T)$ (Carathéodory dans la face minimale).
- $q_{\min}$ est le plus petit cardinal d'un support, compté en positions ; $S^{*}$ est le plus petit support de cardinal $q_{\min}$ dans l'ordre lexicographique des indices de sites. $S^{*}$ détermine la boule ; $U$ aussi.
- Coquille **régulière** : $U=S^{*}$. Coquille **étendue** : $m>q_{\min}$ (0,02 à 0,04 % des boules sur les trames).

Le catalogue d'ordre $K$ est l'ensemble des boules critiques **admises** :

$$\mathrm{adm}_{K}(b)\iff p+q_{\min}\le K+1\ \text{(aucun site de poids}>1\text{ dans }U),\qquad p\le K-1\ \text{(sinon)}.$$

C'est la règle du code (`generator.cpp:262`) et de `SPEC_V10.md:38-39`. Les boules de rayon nul (un site, naissances de l'ordre 1) ne sont pas dans le catalogue : la tour les lit dans la table des sites. $K\le12$ (`types.hpp:33`).

### 3.2 Données par boule (`catalogue.hpp:66-91`)

| Champ | Contenu | Remarque |
| --- | --- | --- |
| `support[4]` | $S^{*}$ trié, complété par `kNone` | identité de la boule ; clé des consultations de la tour |
| `qmin` | 2, 3 ou 4 | en positions |
| `p`, `u` | poids de I et de U | égaux aux cardinaux sans doublon |
| `flags` | bit 0 coquille étendue, bit 1 coquille pondérée | |
| `pop_off`, `pop`, `n_interior` | CSR : I trié puis U trié (indices de sites) | 4,6 identifiants par boule à K5, 8,1 à K10 (`GEN_v2.md:392`) |
| `rank` | rang dense du niveau exact, à partir de 0 | la tour décale de 1 (rang 0 = niveau nul) |
| `level[rank]` | rayon carré exact `num/den`, non réduit (`I192` / `I128w`) | représentation fixée par `emitted_level` (constat C9) |

Le centre n'est pas stocké : `ball_center` le recalcule depuis $S^{*}$, relatif au premier site du support, sous la forme $c=a+N/D$, $D>0$ (`geometry.hpp:42-88`). L'ordre publié est (niveau exact, $S^{*}$) ; l'unicité de $S^{*}$ et l'ordre des niveaux sont contrôlés en exact sur tous les voisins avant publication (`generator.cpp:806-836`), sinon refus `rank_order` ou `census_mismatch`.

### 3.3 Ordres servis par une boule

Pour un ordre $k$ avec $p<k\le p+u$, posons $t=k-p$. La boule est le lieu d'un événement de $\pi_0(L_k)$ seulement si $t\ge q_{\min}-1$ (lemme W, § 4). La fenêtre lue par la tour est $[\max(1,p+q_{\min}-1),\min(K,p+u)]$. Pour une coquille régulière de $q$ sites :

- ordre $p+q-1$ : **jonction** de $q$ morceaux locaux (une fusion peut donc être ternaire ou quaternaire) ;
- ordre $p+q$ : **naissance**.

L'admission $p+q_{\min}\le K+1$ dit exactement : la jonction de la boule tombe à un ordre au plus $K$.

### 3.4 Le théorème de complétude dont la tour dépend

Il a deux moitiés, que les documents de la v10 ne séparent pas.

**Moitié générateur (théorème G, annexe A.3).** `build_catalogue` rend exactement l'ensemble des boules critiques admises, chacune une fois, avec I et U exacts, ou refuse (`wide_leaf`). Preuve : chaîne des lemmes du § 4. Statut : complète, je l'ai revérifiée pièce par pièce ; elle n'est écrite d'un seul tenant nulle part.

**Moitié tour (théorème T, annexe A.4).** Pour tout $k\le K$, l'arbre de fusion de $\pi_0(L_k(a))$ est déterminé par les sites et par les boules critiques telles que $p+q_{\min}\le K+1$ : naissances aux boules sans sous-ensemble séparable, jonctions des morceaux locaux aux autres, rattachement des morceaux par descente. Une boule hors admission n'a qu'un morceau à tout ordre au plus $K$ (lemme W) : elle ne change pas $\pi_0$. Statut : sous position générale, c'est le théorème de Reani et Bobrowski (indice $\mu=p+u-k$, seuls $\mu=0$ et $\mu=1$ touchent $H_0$), inscrit `theorem_external` au registre racine (`STATUT_PREUVES_ET_HEURISTIQUES.md:42-44`). Pour les coquilles étendues, le registre racine disait encore « il reste à prouver » et prévoyait un refus (`CATALOGUE_CRITIQUE_3D.md` § 14.3) ; la v10 l'implémente (quotient de Gordan) et le valide par oracle, mais la preuve n'est écrite qu'en esquisses (`TOWER_v2.md:784-807`, renvois à `TOWER_v1.md` hors dépôt). Une preuve courte figure en annexe A.4.

Conséquence pratique, lue dans `tower.cpp:935-990` : pendant une descente, si la sphère rencontrée n'est pas au catalogue, la tour prend « le premier morceau » et continue. Elle ne détecte une absence que si la sphère est une naissance (`census_mismatch`, ligne 985) ou si le nombre de racines finales est faux. **Une boule de jonction manquante déplace une fusion sans aucun signal.** Tout repose donc sur la complétude du générateur.

## 4. Les lemmes, un par un

Notations de `GEN_v2.md` § 3.1 : $d_k(c)$ distance au $k$-ième voisin pondéré, $N_k(c)=X\cap\bar{B}(c,d_k(c))$ boule k-NN fermée, $L$ est **K-certifiée pour Q** si $N_K(c)\subseteq L$ pour tout $c\in Q$.

| Lemme | Énoncé | Où est la preuve | Complète | Hypothèses | Dans le code |
| --- | --- | --- | --- | --- | --- |
| G (gardes) | si $w(S_0)\ge K$, tout $x\in N_K(c)$ a un $y\in S_0$ pas strictement plus proche | **seulement** `GEN_v1.md` § 2.2, hors dépôt | oui (3 lignes) | aucune | n'est plus évalué : G est inclus dans D |
| D (dominateurs) | si des $y$ strictement plus proches que $x$ partout dans $Q$ pèsent au moins $K$, alors $x\notin N_K(c)$ pour tout $c\in Q$ | `GEN_v2.md:116-118` | oui | aucune ; poids admis ; $Y$ quelconque | `generator.cpp:496-559` |
| D-loc | forme entière de « $y$ domine $x$ sur le pavé » : $A(x)-A(y)>\sum_i\max(0,2h_i(x'_i-y'_i))$ | cube : `GEN_v2.md:120-124` ; pavé à un côté par axe : `RECU_AGENT_J2C.md` § 3.2 | oui (maximum d'une forme affine sur un pavé) | bornes i64 : `static_assert` ligne 33 | lignes 536-556 ; même entier que la forme par coins de la feuille (lignes 359-374) |
| G inclus dans D | retirer la garde ne change aucune liste | `GEN_v2.md:128`, `RECU_AGENT_J2.md` lemme 2 | oui | $S_0$ préfixe de $Y$ | comptage par tranches, lignes 548-553 |
| L (induction) | la liste filtrée d'une sous-boîte reste K-certifiée | `GEN_v2.md:130-132` | oui (contraposée de D) | racine : $L=X$ | `process`, lignes 567-571 |
| C (recensement local) | si $c\in Q$ : $p^{*}\le K-1$ entraîne $\hat{p}=p^{*}$ et $\hat{U}=U^{*}$ ; $p^{*}\ge K$ entraîne $\hat{p}\ge K$ | `GEN_v2.md:134-138` ; détail de (b) dans `GEN_v1.md` § 2.4 | oui | $W\ge K$ pour que $d_K$ existe ; sinon aucune exclusion n'a lieu et $L=X$ | `judge`, lignes 227-281 |
| Ajustement (J2c) | tout centre admis de $Q$ est dans $S=Q\cap[\mathrm{env.lo},\mathrm{env.hi}+1)$ | `RECU_AGENT_J2C.md` § 3.1 ; commentaire lignes 561-566 | oui | $p\le K-1$, vrai dans les deux règles d'admission | lignes 577-596 ; le `+ 1` est nécessaire (centres exactement sur `env.hi`) |
| K (enveloppe) | $S$ vide : aucun centre admis dans $Q$ | cas particulier du précédent | oui | — | lignes 573-594 (3 directions ; les 13 directions de la conception ne sont pas implémentées) |
| Partition | chaque centre admis est dans exactement une feuille demi-ouverte | `RECU_AGENT_J2C.md` § 3.3 | oui | racine = cube dyadique minimal qui contient les sites, lignes 659-674 | coupe au milieu du plus long côté, lignes 600-607 |
| Z (droite des centres) | la droite des points équidistants de trois sites non alignés rencontre le pavé fermé si et seulement si $0$ est dans un zonogone ; test par les normales des générateurs | `GEN_v2.md:151-152` (cube) ; pavé : commentaire lignes 112-121 | oui ; le zonogone est de dimension 2 parce que $u\times v\ne0$ | sites non alignés (cas rendu `-1`) | `center_line_meets`, lignes 122-144, i128 |
| M (union des dominés) | les dominateurs d'un site du support sont strictement intérieurs à la boule : $w(\bigcup\mathrm{Dom})\le p$ | `GEN_v2.md:153` | oui | $c$ dans le pavé fermé | masques, lignes 359-374, 383-385, 410-412, 451-453 |
| S (survie de $S^{*}$) | $S^{*}$ d'une boule admise passe tous les filtres ; une présentation non canonique peut être écartée | `GEN_v2.md:156` pour la règle positionnelle ; **la variante du code** (seuils $K-1$, $K-2$, $K-3$ sur feuille sans poids, $K-1$ partout sinon) n'est écrite que dans `GEN_v1.md` § 2.6 et dans le commentaire lignes 294-317 | oui | seuils et règle d'admission doivent changer ensemble | lignes 341, 386-388, 418-422 |
| U (identité) | une boule critique est déterminée par $U$, et par $S^{*}$ | `GEN_v2.md:158-164` | oui | — | le mémo est sur $S^{*}$ (lignes 253-255), pas sur les masques de $U$ de la conception : les deux sont licites |
| W (admission) | pour $1\le j\le q_{\min}-2$, l'ensemble des directions qui rapprochent un poids au moins $j$ de la coquille est non vide et connexe ; donc aucun ordre $p+j$ n'est critique | `GEN_v2.md:166-176` | oui (Gordan, graphe de Johnson, poids) | — | justifie l'admission et la fenêtre de la tour |
| F (listes larges) | $\lvert L(Q)\rvert\ge\lvert N_K(c)\rvert$ pour tout $c\in\bar{Q}$ | `GEN_v2.md:191` | oui | — | explique les feuilles bloquées et le refus `wide_leaf` |

Points que j'ai contrôlés au-delà de la relecture.

1. **Signe de $D$.** Tous les tests rationnels (`side`, `center_in_box`, `orient_center`) supposent $D>0$. `center2` rend $D=2$, `center3` rend $D=2\lVert u\times v\rVert^{2}$, `center4` normalise le signe (`geometry.hpp:70-75`). Respecté.
2. **Bornes u18.** À la main, avec des différences de coordonnées de valeur absolue $<2^{18}$ : centre q3 $\lvert N\rvert<2^{94,6}$, $D<2^{77,6}$ ; côté q3 $<2^{115,2}$ ; pont « centre dans la boîte » $<2^{102,7}$ ; orientation du centre q4 $<2^{115,3}$ ; niveau q4, numérateur $<2^{154}$, dénominateur $<2^{115,2}$ ; produits croisés $<2^{269}$. Tout tient. Mais une seule borne est gardée par `static_assert` (le filtre, ligne 33) ; les autres sont des commentaires (constat C11).
3. **Listes croissantes.** Le départage du réservoir par rang suppose que toute liste est une sous-suite croissante des indices ; la compaction de `filter_node` la conserve.
4. **Mémo.** Une coquille régulière n'a qu'une présentation (le centre a des barycentriques uniques et strictement positives dans $S^{*}$). Une coquille étendue passe toujours par le mémo, dont la clé $S^{*}$ est calculée sur la coquille complète ; la recherche restreinte par l'arité de la présentation (`canonical_support`, lignes 158 et 171) est correcte parce que $q_{\min}\le\lvert T\rvert$.
5. **Recensement interrompu.** `judge` s'arrête dès que $p$ dépasse le seuil de la présentation. Pour $S^{*}$ d'une boule admise, le seuil n'est jamais dépassé ; une autre présentation de la même boule qui termine son recensement trouve les mêmes I, U et $S^{*}$, donc la même décision.
6. **Stagnation.** La règle (`kStagnationLimit = 9`, lignes 29 et 598-599) ne décide que de l'endroit où l'arbre s'arrête ; une feuille bloquée est énumérée exactement ou refusée. La justification « neuf coupes binaires valent trois niveaux d'octree » (commentaire lignes 23-28) est trop forte, comme l'a relevé l'audit continu : sans effet sur l'exactitude.

Aucun de ces lemmes ne suppose la position générale, l'absence d'égalités, de cosphéricité, de colinéarité ni de doublons.

## 5. Cas dégénérés : traitement et argument

| Cas | Traitement | Argument | Contrôle L01 |
| --- | --- | --- | --- |
| Doublons | un site de poids $w$ ; $p$ et $u$ sont des poids ; coquille pondérée admise par $p\le K-1$ | D, C et M comptent des poids ; lemme W pour la borne basse | E2 : 158 121 enregistrements pondérés sans écart. **La tour refuse tout poids** (`tower.cpp:1158`) |
| Plus de 4 sites cosphériques, centre dans leur enveloppe | une seule boule, $U$ complet, $q_{\min}$ et $S^{*}$ par énumération lexicographique, mémo par feuille | lemme U ; théorème C | E2 : 75 666 enregistrements étendus ; grilles, sphères entières, cube et ses centres |
| Triplet aligné, quadruplet coplanaire | pas un support (produit vectoriel ou déterminant nul) ; les sous-supports sont énumérés à part | définition du support | E2 : familles `colineaire_plus`, `coplanaire`, `rectangles` |
| Triangle rectangle ou obtus, centre sur une face du tétraèdre | pas un support de cette taille ; la boule est émise par son support plus petit, avec coquille étendue si le troisième site est sur la sphère | barycentriques strictement positives | idem |
| Centre exactement sur une face de boîte ou sur `env.hi` | boîtes demi-ouvertes, test rationnel exact, borne `env.hi + 1` | partition ; lemme d'ajustement | grilles de E2 ; mutant M1 du reçu J2c (cité, non rejoué) |
| Niveaux égaux | rang dense par comparaison exacte en 320 bits ; ordre (niveau, $S^{*}$) | le tri flottant n'est qu'une proposition : bandes réparées, puis tous les voisins comparés en exact | E2 compare les rangs ; trame 02 à K5 : 1 099 581 niveaux distincts pour 1 407 885 boules |
| Grande coquille ou grande liste | la liste d'une boîte ne descend pas sous $\lvert N_K(c)\rvert$ (lemme F) : feuille bloquée, énumérée si elle a au plus 256 sites, sinon refus `resource_exhausted/wide_leaf` | exactitude inchangée, coût en $m^{4}$ | E10 : sphère de 144 points acceptée en 12 s, 168 points en 25 s (temps locaux, indicatifs), 312 points refusée |
| $W<K$, $n<2$ | aucune exclusion ; catalogue vide si $n<2$ | D ne peut pas réunir un poids $K$ | E2 : petits nuages à K = 10 et 12 |

Deux incohérences, toutes deux reproduites (E10) :

- **Le catalogue accepte ce que la tour refuse.** `max_leaf = 256` (`catalogue.hpp:98`) contre `kMaxShellEnum = 24` (`tower.cpp:23`). Sur la sphère de 144 points entiers ($x^{2}+y^{2}+z^{2}=89$), le catalogue travaille 12 s (8 feuilles bloquées de 144 sites, 2 169 boules à K5), puis la tour rend `unsupported_degeneracy/shell_quotient_budget`. Même chose pour un cercle de 108 points. `GEN_v2.md:363` prévoyait $M_{\mathrm{hard}}=128$ et un refus de cette sphère (fixture F11) : ce n'est plus le comportement.
- **Les multiplicités sont natives dans le catalogue et refusées par la tour** (`multiplicity_unsupported`). Tout l'appareil pondéré du générateur (règle d'admission à part, seuils « feuille serrée ou non », drapeau) est donc du code sans consommateur. Fait utile : aucun doublon à 1 mm sur les 3 trames du contrat ni sur 5 trames brutes avec sol du cache des démos (109 273 à 126 267 points).

## 6. Complexité et nombre de boules

### 6.1 Ce qui est prouvé

Rien, et la v10 le dit (`SPEC_V10.md:89`, `GEN_v2.md:196`). La sortie peut être $\Omega(n^{2})$ à $K$ fixé. Aucune borne sur le nombre de nœuds, de feuilles bloquées, ni sur $\sum m^{3}$.

### 6.2 Ce qui est mesuré

Catalogue des trames du contrat (E3, mon build, comptes déterministes) :

| Trame | Sites | K = 2 | K = 3 | K = 5 | K = 7 | K = 10 | K = 12 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | 39 885 | 267 355 | 516 844 | 1 306 696 | 2 565 656 | 5 512 670 | 8 314 472 |
| 08/000100 | 35 551 | 231 624 | 443 000 | 1 095 926 | 2 104 698 | 4 383 302 | 6 492 748 |
| 08/000200 | 45 845 | 301 745 | 575 614 | 1 407 885 | 2 675 990 | 5 483 320 | 8 025 829 |
| boules par site | | 6,5 à 6,7 | 12,5 à 13,0 | 30,7 à 32,8 | 58,4 à 64,3 | 119,6 à 138,2 | 175,1 à 208,5 |

- Croissance en $K$ : exposant 1,96 de K5 à K10 et 2,09 de K10 à K12 sur la trame 02. Les boules q4 croissent comme $K^{3}$ (143 105 à K5, 1 486 593 à K10, 2 571 699 à K12), les q3 comme $K^{2}$, les q2 comme $K$. C'est ce que donne un processus de Poisson sur une surface pour q2 et q3 et en volume pour q4 ; ce modèle est une **conjecture d'ordre de grandeur**, pas un théorème sur le LiDAR.
- Arbre, trame 02 : 16,0 nœuds par site et 2,35 recensements par boule à K5 ; 23,2 nœuds par site et 1,97 recensements par boule à K10 ; liste moyenne de 14,0 puis 21,6 sites ; aucune feuille bloquée.
- Reçu G4 cité, non rejoué (`receipts/g4_session5_scale_20260929`) : exposant 1,00 en régime spatial jusqu'à ×32 ; 1,01 à 1,07 en régime de densité pour `uniform` et `clusters` ; jusqu'à 1,3 à 1,4 pour `shells` et `terrain` à K10 parce que le nombre de boules par site monte de la valeur de surface vers la valeur de volume (≈ 460 à K10).

### 6.3 La taille de la sortie elle-même

Tour FULL de la trame 02 (E12, comptes rendus par `mhgp10_tower`) :

| Ordre | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| naissances | 45 845 | 118 797 | 182 823 | 273 879 | 361 326 | 470 936 | 574 604 | 693 464 | 807 798 | 937 412 |
| nœuds de fusion | 45 563 | 89 087 | 132 038 | 185 680 | 248 050 | 313 600 | 381 144 | 455 775 | 532 190 | 618 368 |
| cellules de jonction | 118 797 | 182 823 | 273 879 | 361 329 | 470 939 | 574 606 | 693 466 | 807 800 | 937 412 | 1 062 138 |

- Total : 982 670 naissances et 1 683 088 nœuds à K5 ; 4 466 884 naissances et 7 468 379 nœuds à K10. Soit 37 nœuds par point à K5 et 163 à K10.
- Les naissances de l'ordre $k$ sont les boules avec $p+q_{\min}=k$, c'est-à-dire les cellules de jonction de l'ordre $k-1$ : la ligne 3 décalée d'un cran redonne la ligne 1, à trois unités près (coquilles étendues). **Le catalogue à $K$ est donc la liste des naissances des ordres 2 à $K+1$** ; il n'a pas de gras. À K10, 81 % de ses boules sont des feuilles de la tour.
- Le nombre de naissances par point croît un peu plus vite que l'ordre (1,0 ; 2,6 ; 4,0 ; 6,0 ; 7,9 aux ordres 1 à 5, puis 20,4 à l'ordre 10) : la sortie FULL croît à peu près comme $K^{2{,}2}n$ sur cette trame (21,4 naissances par point à K5, 97,4 à K10).
- Conséquence pour le contrat : à K10, 100 ms pour 5,5 M boules laissent 0,9 µs de CPU par boule sur 48 fils parfaitement occupés. Le reçu G4 cité (`g4_session4_j2c_20260929`) donne 0,628 s pour le catalogue seul à K10 et 0,171 s à K5 sur 48 fils.

### 6.4 La marge $M-K$ (E8, E9)

| Entrée | K | Feuille M | Nœuds | Feuilles | Bloquées | Boules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 14 points, deux amas de 7 aux coins opposés du domaine | 10 | 24 (défaut) | 1 | 1 | 0 | 112 |
| idem | 10 | 13 ($K+3$) | 3 048 725 | 1 524 359 | 0 | 112 |
| idem | 10 | 12 | 27 966 119 | 13 638 737 | 39 710 | 112 |
| idem | 10 | 11 | délai de 60 s dépassé (13 minutes sans fin lors de la campagne E2 avant correction) | | | |

- `--leaf` n'a aucune garde à HEAD (`generator.cpp:638-641`). Le raccord R2, non poussé, ajoute la règle $M\ge K+3$ (`build/v10-integration-r2`, `generator.cpp:654-664`). **Cette règle n'est pas une garantie de coût** : à $M=K+3$, 14 points coûtent 3 millions de nœuds. L'argument de R2 (une liste tend vers $N_K(c)$, de $K+3$ sites au plus en position générale) dit seulement que la descente finit par s'arrêter, pas quand.
- Avec les tailles par défaut (12, 16, 24, 28 selon $K$), des amas denses très éloignés ne posent pas de problème : 16 à 26 nœuds par site pour 2 ou 8 amas de 1 000 à 4 000 points, à séparation 512 comme 262 144 (E9). La famille est donc mesurée linéaire au défaut ; la pathologie est propre aux petites marges.
- Lecture : le coût de l'arbre est gouverné par la marge entre la taille de feuille et $K$, pas seulement par la sortie. Cette dépendance n'a ni énoncé ni borne.

## 7. Ce que l'oracle $\Gamma_k$ établit

**Objet.** $\Gamma_k(a)$ a pour sommets les $k$-parties $F$ de rayon minimal $\beta(F)\le a$ et pour arêtes les $(k+1)$-parties de rayon minimal $\le a$. La v10 s'appuie sur le théorème 2 du manuscrit pour identifier ses composantes à celles de $L_k(a)$. Cette identification se prouve en quelques lignes, sans la thèse et sans position générale (annexe A.4, théorème 1) : $L_k(a)$ est une union finie de convexes compacts, deux d'entre eux se rencontrent si et seulement si l'union de leurs parties a un rayon minimal $\le a$, et deux telles parties se relient par des échanges d'un point à l'intérieur d'une même boule témoin. L'oracle est donc bien la vérité de l'objet.

**Ce que les portes du dépôt comparent.**

| Porte | Nuages | Tailles | Ordres | Ce qui est comparé |
| --- | --- | --- | --- | --- |
| `reference/test_ref.py` | E5, 25 génériques, 40 grilles $\lbrace0,1,2\rbrace^{3}$, carré, cube | $n\le8$ | $k\le4$ (3 pour le carré et le cube) | couvertures des composantes aux coupes ouvertes et fermées ; partitions $C\cap X$ (référence Python contre $\Gamma_k$) |
| `tests/oracle/test_tower_oracle.py` | 24 nuages tronqués à 12 points, plus E5 | $n\le12$ | K dans {1, 3, 5}, E5 à K = 4 | **nombre** de composantes et partition $C\cap X$ à chaque niveau critique, coupes ouvertes et fermées ; verticales sur les points entrés |
| `tests/oracle/test_catalogue_oracle.py` | 40 nuages, 4 genres, plus une translation de 3 000 points | $5\le n\le22$ | K dans {1, 2, 3, 5} | ensemble des ($q_{\min}$, p, I, U) ; $S^{*}$ est **un** support valide ; cohérence des rangs |

Ce que ces portes ne voient pas : les ordres 6 à 10 ; les poids (aucun doublon dans les nuages) ; la minimalité lexicographique de $S^{*}$ ; la valeur des niveaux (seulement l'ordre) ; l'atomicité des multifusions (l'audit continu a montré qu'une fusion ternaire binarisée au même niveau passe le juge ; cité, non rejoué) ; les couvertures côté C++. La translation compare le binaire à lui-même.

**Extension L01 (E7).** Même juge, K = 10, nuages de 11 à 13 points de cinq genres (générique, grilles $\lbrace0..2\rbrace^{3}$ et $\lbrace0..3\rbrace^{3}$, plan, motif (0, 2, 4) + (0, 1)) : 20 nuages, 4 par genre, 19 320 coupes (ordre, niveau, ouverte ou fermée) comparées, 0 écart, aucun refus. Le juge du dépôt n'a pas été modifié ; seuls la liste des nuages et K changent. La sémantique de la tour (naissances, morceaux, descente, fenêtre $p+q_{\min}-1$) tient donc aussi aux ordres 6 à 10 sur ces petits nuages dégénérés.

**À l'échelle**, $\Gamma_k$ ne dit rien : c'est le rôle des invariants E4 à E6, qui n'existent pas dans le dépôt.

## 8. Constats

Gravité : bloquant (rend faux ou invalide un résultat ou un contrat), majeur (à traiter dans la conception v11), mineur, info. Vérification : lu, exécuté, mesuré.

**C1 — info — Le générateur est exact contre un oracle sans code commun avec le dépôt.**
Fait : 110 géométries de 11 familles (génériques, extrêmes u18, grilles, plans, alignements, sphères entières, cube et centres, rectangles, cellule unité, amas aux coins), chacune avec et sans doublons, soit 220 nuages, K dans {1, 2, 3, 5, 10, 12}, trois réglages (défaut ; un fil et feuille $\max(8,K+4)$ ; trois fils et feuille 256). Le dump natif est comparé ligne à ligne au dump reconstruit par l'oracle, donc $S^{*}$ minimal, rang dense, poids et drapeaux compris.
Preuve : `preuves_l01_math_catalogue/oracle_indep.py`, `campagne_20261002_per10.log` : `clouds=220 runs=3960 records=432015 weighted_records=158121 extended_records=75666 fails=0 refus=0 delais=0`. Exécuté.
Conséquence v11 : le générateur de la v10 est une référence différentielle fiable sur petits nuages, poids compris ; l'oracle est réutilisable tel quel comme porte.

**C2 — info — Les invariants d'échelle que j'ai ajoutés sont conformes sur les trois trames du contrat.**
Fait : (a) identité d'Euler (annexe A.5), catalogue à $k_{\mathrm{cat}}=12$ : somme égale à 1 pour chaque ordre de 1 à 10, sur 8 025 829, 8 314 472 et 6 492 748 boules, coquilles étendues traitées en exact (1 559, 529 et 341) ; (b) $\mathrm{cat}(K)=\mathrm{restrict}(\mathrm{cat}(K+2))$ à K = 5 et 10, flux canoniques de 1,1 à 5,5 millions de lignes, sha256 égaux ; (c) 2 281 boules tirées dans le dump K10 de la trame 02, recensement brut sur les 45 845 sites : I, U, $q_{\min}$, $S^{*}$ identiques.
Preuve : `euler_lidar0{0,1,2}.txt`, `mutants_euler.txt` (deux mutants de comptes rendent une somme différente de 1), `jkm2_lidar.txt`, `j1_lidar02_k10.txt`. Exécuté.
Conséquence v11 : la complétude du catalogue v10 sur ces trames ne repose plus seulement sur la preuve et sur des comptes ; ces trois juges coûtent O(sortie) et se portent en portes `lidar`.

**C3 — majeur — Le dépôt n'a aucun invariant global de complétude du catalogue, et la tour ne peut pas voir une jonction manquante.**
Fait : `grep -i euler` sur `src`, `cli`, `tests`, `bench` ne rend qu'un commentaire (`types.hpp:33`). Ni Euler (invariant I2 de `TOWER_v2.md:726`, « toujours »), ni la restriction J-KM2, ni le juge des boîtes, ni J-CENSUS de la conception ne sont implémentés. La tour contrôle 1 recensement sur 32 parmi les boules qu'une descente consulte (`tower.cpp:851-893`), l'absence d'une naissance (ligne 985) et l'unicité de la racine ; une sphère absente dans la fenêtre d'une jonction est traitée comme régulière (lignes 951-990).
Preuve : lecture ; grep exécuté. Corroboration citée, non rejouée ici : la lentille L02 a retiré une à une 3 062 boules admissibles sur 64 petits nuages ; la tour publie une forêt fausse sans refus dans 295 cas, qu'Euler détecte tous (`L02_MATH_TOUR.md` § 7.5, même dossier).
Conséquence v11 : porter Euler à $k_{\mathrm{cat}}=K+2$ (preuve combinatoire en annexe A.5) et la restriction comme portes d'échelle dès le premier catalogue ; écrire dans le contrat de la tour qu'elle est exacte **relativement** au catalogue.

**C4 — majeur — La preuve qui correspond au code n'est écrite d'un seul tenant nulle part ; le document normatif décrit un autre algorithme.**
Fait : `CONCEPTION_V10.md:26` dit que `GEN_v2.md` prévaut pour l'algorithme. Écarts entre `GEN_v2.md` et `generator.cpp` :

| Sujet | `GEN_v2.md` / `CONCEPTION_V10.md` | Code à HEAD |
| --- | --- | --- |
| arbre | octree, racine fixe $[0,2^{18})^{3}$ (ligne 281) | arbre binaire de pavés ajustés, racine = cube dyadique minimal (lignes 561-607, 659-674) |
| stagnation | « aucune règle » (ligne 190 ; D-V10-02) | 9 niveaux sans décroissance sous 1 mm (lignes 29, 598-599) |
| admission | unique et positionnelle (lignes 96-98 ; D-V10-03) | clause pondérée $p\le K-1$ (ligne 262) |
| seuils de feuille | positionnels (ligne 156) | $K-1$, $K-2$, $K-3$ sur feuille sans poids, $K-1$ sinon (ligne 341) |
| mémo | masques de $U$ ; « mémo sur $S^{*}$ » y est le mutant M15 | mémo sur $S^{*}$ (lignes 253-255) |
| tailles | M = 16 / 24 / 32, $M_{\mathrm{hard}}=128$ | 12 / 16 / 24 / 28, `max_leaf = 256` |
| enveloppe | 13 directions | boîte englobante |
| lemme Z | forme fermée i64 | i128, un côté par axe |
| oracle de feuilles (lemme O, D-V10-07) | fourni à la tour | absent ; la tour interroge un arbre k-d |
| bornes | un `static_assert` par prédicat | un seul (ligne 33) |

Le lemme G et la variante pondérée du lemme S ne sont prouvés que dans `GEN_v1.md` (hors dépôt, cité par `generator.cpp:294`). D-loc par axe, l'ajustement et la partition binaire ne sont prouvés que dans deux reçus d'agent. Le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne contient aucune entrée v10 (grep vide à HEAD) alors que `CONCEPTION_V10.md:815-822` en prévoyait huit ; le raccord R2 non poussé n'y ajoute que la projection des points.
Preuve : lecture croisée ; grep exécuté.
Conséquence v11 : un seul `MATHEMATIQUES.md`, écrit pour le code tel qu'il est, avec un énoncé par lemme, sa preuve, sa fixture d'égalité et son mutant ; l'annexe A en est une base.

**C5 — majeur — La sortie FULL croît comme $K^{2}n$ sur LiDAR ; le catalogue lui est proportionnel.**
Fait : trame 02, 45 845 points : 982 670 naissances et 1 683 088 nœuds à K5, 4 466 884 naissances et 7 468 379 nœuds à K10 ; catalogue de 1 407 885 et 5 483 320 boules ; 30,7 à 32,8 boules par site à K5 et 119,6 à 138,2 à K10 sur les trois trames.
Preuve : `catalogue_lidar_comptes.txt`, `tour_lidar02_comptes.txt`. Mesuré. Aucune borne prouvée ; pire cas $\Omega(n^{2})$.
Conséquence v11 : le contrat « 100 ms à K10 » demande de produire 55 millions de boules par seconde et 75 millions de nœuds par seconde ; ce n'est pas un problème de générateur mal fait mais de taille de l'objet. À décider avant de coder : FULL à K10 dans le contrat de temps, ou seulement à K5.

**C6 — majeur — Le coût de l'arbre n'a pas de borne et dépend de la marge $M-K$.**
Fait : 14 points, K = 10 : 1 nœud à M = 24, 3 048 725 à M = 13, 27 966 119 à M = 12, délai dépassé à M = 11. `--leaf` et `--max-leaf` sont exposés sans garde. Aux tailles par défaut, la famille adverse « amas denses très éloignés » reste à 16 à 26 nœuds par site.
Preuve : `petite_feuille_rejeu.log`, `amas_rejeu.log`. Exécuté.
Conséquence v11 : M(K) est une constante interne, pas un paramètre ; un budget de nœuds convertit toute dérive en refus `resource_exhausted` ; la linéarité reste un `experimental_target` jugé aux tailles 8 000 à 32 000 et sur les trames.

**C7 — majeur — Politique de dégénérescence incohérente entre catalogue et tour.**
Fait : `max_leaf = 256` contre `kMaxShellEnum = 24` ; 144 points cosphériques : 12 s de catalogue (énumération en $m^{4}$ de 8 feuilles de 144 sites) puis refus de la tour ; multiplicités acceptées par le catalogue et refusées par la tour.
Preuve : `degeneres.txt`. Exécuté.
Conséquence v11 : une seule limite de coquille, déclarée, appliquée au plus tôt ; les doublons sont soit servis de bout en bout, soit refusés à l'entrée. Sur les 8 nuages KITTI contrôlés il n'y a aucun doublon à 1 mm.

**C8 — mineur — Deux règles d'admission pondérée coexistent.**
Fait : le code et `SPEC_V10.md` appliquent $p\le K-1$ aux coquilles pondérées ; `GEN_v2.md` et D-V10-03 décident la règle positionnelle unique, que le lemme W justifie. Les deux sont des sur-ensembles sûrs ; les seuils de feuille doivent suivre la règle retenue.
Preuve : `generator.cpp:262`, `:341` ; `GEN_v2.md:96-98`. Lu ; E2 confirme que le binaire suit la règle du code.
Conséquence v11 : une seule règle, $p+q_{\min}\le K+1$ en positions, et les seuils $\theta_q=K+1-q$ ; plus de notion de « feuille serrée ».

**C9 — mineur — La représentation publiée d'un niveau est un artefact d'une feuille retirée.**
Fait : `emitted_level` (`generator.cpp:187-221`) rend le niveau « dans la représentation de la présentation qui l'émettait en premier dans l'ordre d'énumération de la feuille v1 » ; pour une coquille étendue à $q_{\min}=3$, ce peut être celle d'un tétraèdre de la coquille et non celle de $S^{*}$. S'y ajoute que le couple publié pour un rang est celui de sa première boule dans l'ordre canonique, et que les trois formules (q2, q3, q4) donnent des couples différents pour un même niveau. Le dump de la tour imprime ce couple non réduit (`cli/mhgp10_tower.cpp:177-184`).
Preuve : lecture, puis exécution : sur 264 coquilles étendues de 5 sites à $q_{\min}=3$ (sphère de rayon carré 9, intérieur vide), le niveau imprimé par `mhgp10_tower --dump` pour la naissance d'ordre 5 est le couple de `level3`$(S^{*})$ dans 206 cas et un autre couple dans 58 cas (par exemple 186624/20736 au lieu de 18432/2048, tous deux égaux à 9) ; `c9_niveau.py`, `c9_niveau.log`.
Conséquence v11 : une conformité « octet pour octet » sur les dumps de tour (`morsehgp3D_v11/docs/ARCHITECTURE.md` § 6) obligerait à porter cette règle et ces formules. Comparer des niveaux réduits, ou définir la représentation depuis $S^{*}$ seul et normaliser le dump v10 avant comparaison.

**C10 — mineur — L'oracle $\Gamma_k$ du dépôt s'arrête à K = 5 et à 12 points, et ne compare ni poids ni structure N-aire.**
Fait : tableau du § 7.
Preuve : `test_tower_oracle.py:164-165`, `test_ref.py:36-63`, `test_catalogue_oracle.py:114-141`. Lu ; extension E7 exécutée à K = 10.
Conséquence v11 : l'oracle borné doit couvrir tous les ordres servis (K = 10 tient en quelques minutes par nuage en Python), les couvertures, et la forme canonique des multifusions.

**C11 — mineur — Les bornes arithmétiques ne sont pas gardées.**
Fait : un seul `static_assert` (`generator.cpp:33`) ; les bornes de `geometry.hpp:1-8` sont des commentaires, calculées pour des différences $<2^{19}$. Je les ai revérifiées à la main pour u18 : elles tiennent.
Preuve : lecture ; calcul au § 4.
Conséquence v11 : déjà décidé dans `morsehgp3D_v11/docs/ARCHITECTURE.md` § 3 (entiers à budget de bits) ; à appliquer à chaque prédicat du générateur.

**C12 — mineur — Le théorème de la tour n'est écrit qu'en esquisses.**
Fait : `TOWER_v2.md:784-807` donne des énoncés et un statut `proved_here` pour PO-T1 à T6, T12, T13, avec une ligne d'argument chacun, ou un renvoi à `TOWER_v1.md` hors dépôt ; le registre racine considérait le cas des coquilles de plus de quatre points comme non prouvé.
Preuve : lecture.
Conséquence v11 : l'annexe A.4 donne des preuves rédigées (théorème 1, propositions 2 et 3, théorème T). Elles sont de moi et n'ont été relues par personne : à faire relire par un tiers (lentille tour) avant inscription au registre.

**C13 — mineur — Les portes permanentes du catalogue ont des angles morts.**
Fait : pas de poids, pas de minimalité de $S^{*}$, pas de valeur de niveau, aucune des fixtures F1 à F16 de `GEN_v2.md:548`, aucun mutant enregistré dans CMake (les mutants des reçus J2 et J2c ont été joués à la main), aucun test du refus `wide_leaf`.
Preuve : `CMakeLists.txt:87-134`, `tests/oracle/test_catalogue_oracle.py`. Lu ; grep exécuté. Déjà relevé par l'audit continu (P2).
Conséquence v11 : `oracle_indep.py` comble les trois premiers ; graver les fixtures d'égalité de l'annexe B.

**C14 — info — « Égal à la v9 » veut dire « mêmes comptes ».**
Fait : l'égalité porte sur le total, la répartition par $q_{\min}$ et le nombre de coquilles étendues (`l13_pins.txt`, 12 épingles), pas sur les enregistrements ; la base de connaissances le dit (`CONNAISSANCES_V10_20260928.md:434`). L'outil `v9_catalogue_dump` de `GEN_v2.md:553` n'existe pas.
Preuve : lecture ; E3 retrouve les comptes de 08/000200.
Conséquence v11 : la référence différentielle de la v11 est la v10, enregistrement par enregistrement ; la v9 n'apporte plus rien.

## 9. Ce qui est solide et mérite un port explicite

1. **Le principe des boîtes de centres** : listes K-certifiées par dominance (lemmes D, L), recensement local exact (théorème C), une feuille demi-ouverte par centre. Exact sans position générale, poids compris.
2. **Le pavé ajusté** (lemme d'ajustement, borne `env.hi + 1`) et la coupe binaire du plus long côté.
3. **La forme D-loc entière** et le comptage par tranches sans branchement.
4. **La feuille par masques de dominance** : bissectrice lue dans la matrice de dominance, lemme M, lemme Z par zonogone, triplets vivants sans exigence d'acuité, seuils $\theta_q$.
5. **L'identité par $S^{*}$**, l'ordre canonique (niveau exact, $S^{*}$), le contrôle exact de tous les voisins après le tri flottant, le refus sur doublon de $S^{*}$.
6. **Les deux repères arithmétiques** : sites non mis à l'échelle pour centres et recensements, boîte en unités $2^{-6}$ pour la dominance ; aucun flottant dans une décision.
7. **Le lemme W** et l'admission positionnelle $p+q_{\min}\le K+1$.
8. **L'identité d'Euler** à $k_{\mathrm{cat}}=K+2$ et la restriction J-KM2 comme juges d'échelle ; le juge d'échantillon par recensement brut.
9. **La référence Python** `hgp10_ref.py` (catalogue brut, structure locale, descente, $\Gamma_k$) comme oracle borné, à étendre aux ordres 6 à 10 et aux poids.
10. **Les refus transactionnels** (`wide_leaf`, ordre des rangs, doublon) plutôt qu'une sortie partielle.

## 10. Ce qu'il ne faut pas refaire

1. Un document normatif qui décrit un algorithme différent du code, et des preuves dans des reçus ou hors dépôt.
2. Deux règles d'admission, des seuils qui dépendent d'un état de feuille, un drapeau pondéré sans consommateur.
3. Un catalogue qui accepte des entrées que la tour refuse (coquilles de 25 à 256 sites, multiplicités).
4. Une taille de feuille et un plafond de feuille exposés en ligne de commande sans garde.
5. Une représentation de niveau qui dépend d'un ordre d'énumération.
6. Des invariants d'échelle annoncés « toujours actifs » dans la conception et jamais codés.
7. Des mutants joués une fois par un agent et non enregistrés comme portes.
8. Une justification géométrique approximative de la règle de stagnation ; la dire pour ce qu'elle est : un choix de coût sans effet sur l'exactitude.
9. Une « égalité à la version précédente » fondée sur des comptes.

## 11. Questions ouvertes

1. **Contrat.** La tour FULL à K10 a 163 nœuds par point. Le contrat de 100 ms porte-t-il sur FULL à K10, ou sur FULL à K5 et une hiérarchie de points à K10 ?
2. **Faut-il toutes les naissances pour la hiérarchie de points ?** La hiérarchie $C\cap X$ n'a que $n$ feuilles par ordre. Existe-t-il un théorème de complétude pour un sous-catalogue qui suffise à $C\cap X$ (ou à l'entrée `cover`) sans construire FULL ? Aucun élément dans la v10 ; c'est le seul levier d'ordre de grandeur sur le volume.
3. **Borne de coût.** Peut-on prouver, pour une famille déclarée (densité bornée, pas de cosphéricité au-delà de $M-K$), que le nombre de nœuds est $O(n+\text{boules})$ à $M-K$ fixé ? Quelle est la bonne loi en $M-K$ ?
4. **Grandes coquilles.** Faut-il un quotient local polynomial (cas circulaire de `TOWER_v2.md` § 9.4, cas général par arrangement de grands cercles) ou un refus dès 25 sites ?
5. **Multiplicités.** Servies de bout en bout (lemme W, Euler pondéré de `TOWER_v2.md` § 9.3, boules de rayon nul aux ordres 1 à $w$) ou refusées à l'entrée ? Les trames KITTI n'en ont pas à 1 mm ; un banc synthétique quantifié peut en avoir.
6. **Loi du nombre de boules.** Le modèle de Poisson (q2 en $K$, q3 en $K^{2}$ sur une surface, q4 en $K^{3}$ en volume) est-il démontrable pour un modèle de trame ? Il fixerait le budget mémoire par point.
7. **Coquilles étendues à l'échelle.** Elles sont 0,02 à 0,04 % des boules mais portent tout le code de quotient local ; le coût de leur traitement exact dans la tour est-il mesuré séparément ?

## 12. Recommandations pour la v11

1. Écrire `MATHEMATIQUES.md` à partir de l'annexe A : objet, théorème G (générateur), théorème T (tour), identité d'Euler ; un lemme = un énoncé, une preuve, une fixture d'égalité, un mutant. L'inscrire au registre avant le premier code du moteur.
2. Admission unique $p+q_{\min}\le K+1$ ; seuils $\theta_q=K+1-q$ ; sites de rayon nul traités comme des boules à $q=1$ dans la même table (la référence Python le fait déjà), ce qui unifie K = 1 et les poids.
3. Décider la politique de dégénérescence une fois : limite de coquille commune au catalogue et à la tour, refus au plus tôt, statut `unsupported_degeneracy` ; doublons servis ou refusés à l'entrée.
4. Portes d'échelle dès le premier catalogue : Euler à $K+2$, restriction, juge d'échantillon ; portes d'oracle : `oracle_indep.py` (poids, $S^{*}$, rangs) et $\Gamma_k$ jusqu'à K = 10.
5. M(K) interne, budget de nœuds, compteurs déterministes du grand livre comme porte de coût (nœuds par site, recensements par boule) aux tailles 8 000 à 32 000 et sur les trames.
6. Niveau canonique défini par $S^{*}$ ; conformité v10 sur niveaux réduits.
7. Avant toute optimisation, trancher la question ouverte 1 : elle décide si le générateur doit produire 5,5 millions de boules en 100 ms.

## Annexe A. Énoncés reformulés (base du document mathématique de la v11)

Tout ce qui suit vaut sans position générale. Les preuves sont celles que j'ai refaites ; elles sont courtes exprès. Statut proposé entre crochets.

### A.1 Cadre

$X\subset\mathbb{Z}^{3}$ est un ensemble fini de **sites** (positions distinctes), $w:X\to\mathbb{Z}_{\ge1}$ les multiplicités, $W=w(X)$. Un point est une copie d'un site ; une $k$-partie est un ensemble de $k$ copies. Pour $y\in\mathbb{R}^{3}$ et $1\le k\le W$ :

$$d_k(y)=\min\lbrace\rho\ge0:w(X\cap\bar{B}(y,\rho))\ge k\rbrace,\qquad D_k=d_k^{2},\qquad N_k(y)=X\cap\bar{B}(y,d_k(y)).$$

$$L_k(a)=\lbrace y:D_k(y)\le a\rbrace,\qquad L_k^{<}(a)=\lbrace y:D_k(y)<a\rbrace.$$

Pour une partie $G$ : $\beta(G)$ est le rayon carré de la plus petite boule fermée qui contient $G$, et $Q_G(a)=\bigcap_{x\in G}\bar{B}(x,\sqrt{a})$, convexe compact, non vide si et seulement si $\beta(G)\le a$. Ainsi $L_k(a)=\bigcup_{\lvert F\rvert=k}Q_F(a)$.

**Fait M (plus petite boule).** Une boule fermée $(c,a)$ qui contient $G$ est sa plus petite boule englobante si et seulement si $c\in\mathrm{conv}(G\cap\partial B(c,\sqrt{a}))$. [classique]

### A.2 Boules critiques et catalogue

**Définition 1.** Une boule $b=(c,a)$, $a>0$, est **critique** si $c\in\mathrm{conv}(U)$, où $U=X\cap\partial B(c,\sqrt{a})$. On note $I=X\cap B^{\circ}(c,\sqrt{a})$, $p=w(I)$, $u=w(U)$, $m=\lvert U\rvert$. Un **support** est une partie $T\subseteq U$ affinement indépendante avec $c\in\mathrm{relint}\,\mathrm{conv}(T)$ ; $q_{\min}$ est le plus petit cardinal d'un support, $S^{*}$ le plus petit support de cardinal $q_{\min}$ pour l'ordre lexicographique des indices de sites.

**Lemme 1 (supports).** (i) Une boule est critique si et seulement si elle a un support, et tout support a 2, 3 ou 4 sites. (ii) Si $T$ est un support de $b$ et $T\subseteq G\subseteq I\cup U$, alors $b$ est la plus petite boule englobante de $G$ ; réciproquement la plus petite boule englobante d'une partie d'au moins deux sites est critique. (iii) $S^{*}$ détermine $b$ ; $U$ détermine $b$. (iv) Si $T$ est un support, aucune partie stricte de $T$ ne contient $c$ dans son enveloppe convexe. [proved_here]

*Preuve.* (i) Carathéodory dans la face minimale de $\mathrm{conv}(U)$ qui contient $c$ ; un seul site est exclu par $a>0$. (ii) Fait M. (iii) $c$ est le seul point de $\mathrm{aff}(T)$, et de $\mathrm{aff}(U)$, équidistant de ses sites. (iv) Les coordonnées barycentriques de $c$ dans $T$ sont uniques et strictement positives. ∎

**Définition 2.** $\mathrm{Cat}_K(X)=\lbrace b\text{ critique}:p+q_{\min}\le K+1\rbrace$, chaque boule portant $(S^{*},q_{\min},p,u,I,U)$ et le rang dense de $a$ parmi les niveaux exacts. Un site de poids $w$ est en plus une boule de rayon nul, de fenêtre $[1,\min(K,w)]$.

### A.3 Le générateur

Une boîte est un pavé $Q=\prod_i[l_i,h_i)$ ; $\bar{Q}$ est son adhérence. $L\subseteq X$ est **K-certifiée pour Q** si $N_K(y)\subseteq L$ pour tout $y\in\bar{Q}$. On dit que $z$ **domine** $x$ sur $Q$ si $\lVert z-y\rVert<\lVert x-y\rVert$ pour tout $y\in\bar{Q}$.

**Lemme D.** Si des sites qui dominent $x$ sur $Q$ pèsent au moins $K$, alors $x\notin N_K(y)$ pour tout $y\in\bar{Q}$. [proved_here]

*Preuve.* En $y$, ces sites sont strictement plus proches que $x$ et pèsent $K$ : $d_K(y)<\lVert x-y\rVert$. ∎

**Lemme D-loc.** Avec $x'=x-l$, $z'=z-l$ et $s_i=h_i-l_i$ : $z$ domine $x$ sur $Q$ si et seulement si $\lVert x'\rVert^{2}-\lVert z'\rVert^{2}>\sum_i\max(0,2s_i(x'_i-z'_i))$. [proved_here]

*Preuve.* $\lVert z-y\rVert^{2}-\lVert x-y\rVert^{2}=\lVert z'\rVert^{2}-\lVert x'\rVert^{2}+2\,y'\cdot(x'-z')$ est affine en $y'=y-l\in\prod_i[0,s_i]$ ; son maximum se prend coordonnée par coordonnée. ∎

**Proposition L.** $X$ est K-certifiée pour toute boîte. Si $L$ l'est pour $Q$ et $Q'\subseteq Q$, alors $L'=\lbrace x\in L:w(\lbrace z\in Y:z\text{ domine }x\text{ sur }Q'\rbrace)<K\rbrace$ l'est pour $Q'$, quel que soit $Y\subseteq X$. [proved_here ; contraposée de D]

**Théorème C (recensement local).** Soient $L$ K-certifiée pour $Q$ et $(c,a)$ une sphère avec $c\in\bar{Q}$. Notons $p^{*}$ et $U^{*}$ le poids intérieur et la coquille dans $X$, $\hat{p}$ et $\hat{U}$ les mêmes dans $L$. Si $p^{*}\le K-1$, alors $\hat{p}=p^{*}$ et $\hat{U}=U^{*}$, et tout l'intérieur est dans $L$. Si $p^{*}\ge K$, alors $\hat{p}\ge K$. [proved_here]

*Preuve.* Si $p^{*}<K$, toute boule fermée de rayon $<\sqrt{a}$ pèse moins de $K$, donc $d_K(c)\ge\sqrt{a}$ et $X\cap\bar{B}(c,\sqrt{a})\subseteq N_K(c)\subseteq L$. Sinon $d_K(c)<\sqrt{a}$ et $N_K(c)$, de poids au moins $K$, est dans $L\cap B^{\circ}$. Si $W<K$, aucune exclusion n'a lieu et $L=X$. ∎

**Lemme A (ajustement).** Soient $L$ K-certifiée pour $Q$, $e^{-}\le e^{+}$ les coins de la boîte englobante de $L$, et $b$ critique avec $p\le K-1$ et $c\in Q$. Alors $c\in Q\cap\prod_i[e^{-}_i,e^{+}_i+\varepsilon)$ pour tout $\varepsilon>0$. En particulier, si la boîte englobante de $L$ ne rencontre pas $\bar{Q}$, aucune telle boule n'a son centre dans $Q$. [proved_here]

*Preuve.* Par C, $S^{*}\subseteq U\subseteq L$ ; $c\in\mathrm{conv}(S^{*})$ est dans la boîte englobante fermée de $L$. ∎

**Lemme P (partition).** Partant d'un pavé demi-ouvert qui contient $\mathrm{conv}(X)$, si chaque nœud remplace sa boîte par l'intersection du lemme A puis la coupe en deux moitiés demi-ouvertes, alors toute boule critique avec $p\le K-1$ a son centre dans exactement une feuille, dont la liste est K-certifiée. [proved_here ; récurrence sur A et L]

**Lemme M.** Si $s\in U(b)$, $c\in\bar{Q}$ et $z$ domine $s$ sur $Q$, alors $z\in I(b)$. Donc pour $T\subseteq U$, le poids de l'union des dominateurs des sites de $T$ est au plus $p$. [proved_here]

**Lemme Z.** Pour $a_1,a_2,a_3$ non alignés, posons $u=a_1-a_2$, $v=a_1-a_3$, $F(y)=(\lVert a_1-y\rVert^{2}-\lVert a_2-y\rVert^{2},\ \lVert a_1-y\rVert^{2}-\lVert a_3-y\rVert^{2})$, $y_0$ le centre de $\bar{Q}$, $g_i=s_i(u_i,v_i)$. La droite des points équidistants des trois sites rencontre $\bar{Q}$ si et seulement si, pour chaque $i$ avec $g_i\ne0$ et $n_i=(v_i,-u_i)$ : $\lvert n_i\cdot F(y_0)\rvert\le\sum_j\lvert n_i\cdot g_j\rvert$. [proved_here]

*Preuve.* $F$ est affine, $F(\bar{Q})=F(y_0)+\sum_i[-1,1]\,g_i$ est un zonogone, de dimension 2 car $u\times v\ne0$ ; un point est dans un polygone convexe à symétrie centrale si et seulement s'il satisfait les bandes de ses côtés, qui sont parallèles aux $g_i$. ∎

**Lemme S (survie du support canonique).** Posons $\theta_q=K+1-q$. Soit $b\in\mathrm{Cat}_K(X)$ de centre $c\in Q$, la liste de $Q$ étant K-certifiée. Alors, pour $S^{*}$ et pour chacune de ses parties $T'$ : aucun site de $T'$ n'en domine un autre ; le poids de l'union des dominateurs des sites de $T'$ est au plus $p\le\theta_{q_{\min}}\le\theta_{\lvert T'\rvert}$ ; pour un triplet $T'$ (non aligné par le lemme 1), la droite des équidistants rencontre $\bar{Q}$ ; et le recensement de la présentation $S^{*}$, interrompu seulement quand le poids intérieur dépasse $\theta_{q_{\min}}$, va à son terme. Une énumération qui ne garde que les paires, triplets et quadruplets passant ces tests présente donc $S^{*}$. [proved_here ; M, Z et C]

**Lemme E (émission unique).** Si chaque présentation $T$ jugée calcule le recensement complet, puis $S^{*}$ par énumération lexicographique des parties de $U$ de taille 2, 3 puis 4 (au plus $\lvert T\rvert$), une coquille régulière n'a qu'une présentation, et une table des $S^{*}$ déjà émis dans la feuille suffit à émettre une fois chaque coquille étendue. [proved_here ; lemme 1 (iii) et (iv)]

**Théorème G (générateur).** L'arbre des lemmes L, A, P, dont chaque feuille énumère selon S et E et recense selon C, rend exactement $\mathrm{Cat}_K(X)$, chaque boule une fois, avec $I$ et $U$ exacts. Ni la taille de feuille, ni le choix de $Y$, ni la règle d'arrêt de la descente n'interviennent dans l'exactitude. [proved_here ; validé : E1, E2, E4, E5, E6]

**Lemme F (listes inhérentes).** Toute liste K-certifiée pour $Q$ contient $N_K(y)$ pour chaque $y\in\bar{Q}$ : une configuration de $m$ sites cosphériques d'intérieur de poids $<K$ impose des listes d'au moins $m$ sites autour de son centre, à toute profondeur. [proved_here]

### A.4 La tour

**Théorème 1 (discrétisation).** Soit $\Gamma_k(a)$ le graphe dont les sommets sont les $k$-parties $F$ avec $\beta(F)\le a$ et les arêtes les couples $F,F'$ avec $\lvert F\cup F'\rvert=k+1$ et $\beta(F\cup F')\le a$. L'application qui envoie $F$ sur la composante de $L_k(a)$ contenant $Q_F(a)$ induit une bijection de $\pi_0(\Gamma_k(a))$ sur $\pi_0(L_k(a))$. De même pour $\Gamma_k^{<}(a)$ et $L_k^{<}(a)$, avec des inégalités strictes et des boules ouvertes. [proved_here ; remplace l'appel au théorème 2 du manuscrit]

*Preuve.* $L_k(a)$ est une union finie de convexes compacts $Q_F(a)$ ; les composantes d'une telle union sont celles du graphe d'intersection. $Q_F\cap Q_{F'}\ne\emptyset$ si et seulement si $\beta(F\cup F')\le a$. Si $y$ est dans cette intersection, $F$ et $F'$ sont deux $k$-parties de $N=X\cap\bar{B}(y,\sqrt{a})$ ; on passe de l'une à l'autre en échangeant un point à la fois, et chaque union de deux parties consécutives, de cardinal $k+1$, est dans $N$, donc de rayon minimal $\le a$. Le cas ouvert est identique. ∎

**Proposition 2 (structure locale).** Soient $b=(c,a)$ critique, $p<k\le p+u$, $t=k-p$. Une partie $A$ de copies de $U$ est **séparable** si $c\notin\mathrm{conv}(A)$, c'est-à-dire s'il existe $v$ avec $v\cdot(x-c)>0$ pour tout $x\in A$ (Gordan). Posons $C_A=\bigcap_{x\in A}B^{\circ}(x,\sqrt{a})$. Alors il existe $\delta>0$ tel que

$$L_k^{<}(a)\cap B(c,\delta)=\bigcup_{\lvert A\rvert=t,\ A\text{ séparable}}C_A\cap B(c,\delta).$$

Chaque $C_A$ non vide est convexe, adhérent à $c$, et $C_A\cap C_{A'}=C_{A\cup A'}$ est non vide si et seulement si $A\cup A'$ est séparable. Les **morceaux** de $b$ à l'ordre $k$ sont donc les composantes du graphe des $t$-parties séparables, reliées quand leur union est séparable. Il n'y a aucun morceau si et seulement si $c$ est un minimum local strict de $D_k$. [proved_here ; lève l'obligation du § 14.3 de `CATALOGUE_CRITIQUE_3D.md` pour $H_0$]

*Preuve.* Pour $y$ assez près de $c$, les sites de $I$ restent à distance $<\sqrt{a}$ et ceux hors de la boule fermée à distance $>\sqrt{a}$ ; donc $D_k(y)<a$ si et seulement si au moins $t$ copies de $U$ sont à distance $<\sqrt{a}$ de $y$. Pour $x\in U$, $\lVert y-x\rVert^{2}<a$ équivaut à $2(x-c)\cdot(y-c)>\lVert y-c\rVert^{2}$ : si $C_A$ contient un point $y$, alors $v=y-c$ sépare $A$, et réciproquement $c+\varepsilon v\in C_A$ pour $\varepsilon$ petit. Un convexe ouvert adhérent à $c$ rencontre toute boule centrée en $c$ selon un convexe ; une union finie de convexes ouverts a pour composantes celles de son graphe d'intersection. ∎

**Lemme W (ordres réguliers).** Si $t\le q_{\min}-2$, la boule a exactement un morceau à l'ordre $p+t$. [proved_here ; `GEN_v2.md` § 3.8]

*Preuve.* Toute partie de moins de $q_{\min}$ sites de $U$ est séparable (sinon elle contiendrait un support). Deux $t$-parties qui diffèrent d'un site ont une union de $t+1<q_{\min}$ sites, séparable ; le graphe de Johnson est connexe. Avec poids, on raisonne sur les copies : une partie de $t$ copies est portée par au plus $t$ sites, une union de deux parties qui diffèrent d'une copie par au plus $t+1<q_{\min}$ sites, et l'argument est le même. ∎

**Proposition 3 (représentants).** Dans le cadre de la proposition 2, sans poids. (a) Pour $A$ séparable, $\beta(I\cup A)<a$. (b) Deux représentants $I\cup A$, $I\cup A'$ d'un même morceau sont dans une même composante de $\Gamma_k^{<}(a)$. (c) Toute $k$-partie $F\subseteq I\cup U$ avec $\beta(F)<a$ est dans la composante de $\Gamma_k^{<}(a)$ d'un représentant $I\cup A$ avec $A\subseteq F\cap U$. (d) Toutes les $k$-parties de $I\cup U$ sont dans une même composante de $\Gamma_k(a)$. [proved_here]

*Preuve.* (a) et (b) : le point $c+\varepsilon v$, $v$ séparant $A$ (ou $A\cup A'$), est à distance $<\sqrt{a}$ de tout $I\cup A$ (ou $I\cup A\cup A'$) ; on conclut comme au théorème 1. (c) Sur le segment de $c$ au centre $c_F$, par convexité de $y\mapsto\lVert x-y\rVert^{2}$, tout point autre que $c$ est à distance $<\sqrt{a}$ de chaque $x\in F$ ; près de $c$ il l'est aussi de $I$. Donc $F\cap U$, qui a au moins $t$ sites, est séparable, et $F$ comme $I\cup A$ sont des $k$-parties d'un ensemble dont toutes les parties ont un rayon minimal $<a$. (d) Témoin $c$. ∎

**Théorème T (les événements du catalogue engendrent $\Gamma_k$).** Sans poids, $1\le k\le\min(K,n)$. Pour tout niveau $a$, la relation « même composante de $\Gamma_k(a)$ » sur les $k$-parties de rayon minimal $\le a$ est engendrée par : (i) « même composante de $\Gamma_k^{<}(a)$ » ; (ii) pour chaque boule critique $b$ de niveau $a$ avec $p<k\le p+m$, l'identification de toutes les $k$-parties de $I\cup U$ ; (iii) pour chaque $k$-partie $F$ de rayon minimal $a$ dont la boule a $p\ge k$, l'identification de $F$ aux $k$-parties de $I$, qui sont dans une même composante de $\Gamma_k^{<}(a)$. De plus : une composante de $\Gamma_k(a)$ ne contient aucune partie de rayon minimal $<a$ si et seulement si elle est l'ensemble des $k$-parties de $I\cup U$ d'une boule de niveau $a$ sans partie séparable (**naissance**) ; et si $k\le p+q_{\min}-2$, (ii) ne relie que des composantes déjà égales de $\Gamma_k^{<}(a)$. Les naissances et les fusions de l'ordre $k$ sont donc portées par les seules boules avec $p+q_{\min}\le k+1$, toutes dans $\mathrm{Cat}_K(X)$. [proved_here, à relire par un tiers ; validé par oracle jusqu'à $n=13$, $k=10$]

*Preuve.* Correction : (ii) et (iii) relient des parties d'une même composante de $\Gamma_k(a)$ par la proposition 3 (d) et par le témoin $c$. Complétude : un sommet $F$ de niveau $a$ est dans $I\cup U$ de sa boule ; il relève de (ii) si $p<k$ (alors $k\le p+m$), de (iii) sinon. Une arête $G$ de niveau $a$ a toutes ses facettes dans $I\cup U$ de sa boule. Si $p<k$, alors $k+1\le p+m$ et (ii) les identifie. Si $p\ge k$, une facette de niveau $a$ relève de (iii), une facette de niveau $<a$ est reliée dans $\Gamma_k^{<}(a)$ aux $k$-parties de $I$ par l'argument du segment. Naissance : si une composante n'a que des sommets de niveau $a$, aucun ne relève de (iii), et la boule $b$ de l'un d'eux n'a pas de partie séparable (proposition 3 (a)) ; toute arête de niveau $a$ qui contient une $k$-partie de $I\cup U$ de niveau $a$ a la même plus petite boule, donc reste dans $I\cup U$. La réciproque suit de la proposition 3 (c). Inertie : lemme W et proposition 3 (b), (c). ∎

**Corollaire (ce que la tour lit).** La forêt de l'ordre $k$ a pour feuilles les boules sans partie séparable (et les sites si $k=1$) ; à chaque niveau, ses multifusions sont les classes, parmi les composantes de $\Gamma_k^{<}(a)$, de la relation engendrée par « porter un morceau d'une même boule de niveau $a$ ». Une $k$-partie se rattache à sa composante par descente : plus petite boule englobante, puis saut aux $k$ plus proches du centre si $p\ge k$, sinon représentant d'un morceau ; $\beta$ décroît strictement (proposition 3 (a)) jusqu'à une naissance.

**Avec poids.** Le théorème 1, la proposition 2 et le lemme W sont énoncés pour des copies et restent vrais tels quels. La proposition 3 et le théorème T se réécrivent avec des multiensembles ; c'est une obligation de rédaction, pas une difficulté (annexe B, O2).

### A.5 Identité d'Euler

**Proposition 5.** Sans poids. Pour une boule critique $b$ et $k\ge1$, posons $t=k-p$ et, si $1\le t\le m$,

$$e_k(b)=\sum_{B\subseteq U,\ c\in\mathrm{conv}(B),\ \lvert B\rvert\ge t}(-1)^{\lvert B\rvert-t}\binom{\lvert B\rvert-1}{t-1},$$

et $e_k(b)=0$ sinon. Alors, pour tout $1\le k\le n$ : $n\,[k=1]+\sum_{b}e_k(b)=1$. Pour une coquille régulière de $q$ sites, $e_k(b)=(-1)^{q-t}\binom{q-1}{t-1}$. Seules les boules avec $p\le k-1$ contribuent : le catalogue à $K+2$ suffit pour les ordres $\le K$. [proved_here ; même identité que `TOWER_v2.md` § 9.3, preuve sans valuation]

*Preuve.* Le complexe de toutes les familles non vides de $k$-parties est un simplexe : $\sum_{\mathcal{C}\ne\emptyset}(-1)^{\lvert\mathcal{C}\rvert-1}=1$. On regroupe les familles par leur union $G$. Si $f(G)$ est la somme sur les familles d'union exactement $G$, alors $\sum_{G'\subseteq G}f(G')=1$ dès que $\lvert G\rvert\ge k$, et l'inversion de Möbius donne $f(G)=\sum_{s=k}^{\lvert G\rvert}(-1)^{\lvert G\rvert-s}\binom{\lvert G\rvert}{s}=(-1)^{\lvert G\rvert-k}\binom{\lvert G\rvert-1}{k-1}$. Donc $\sum_{\lvert G\rvert\ge k}(-1)^{\lvert G\rvert-k}\binom{\lvert G\rvert-1}{k-1}=1$. On regroupe maintenant les $G$ par leur plus petite boule : par le fait M, ce sont les $G=J\cup B$ avec $J\subseteq I$, $B\subseteq U$, $c\in\mathrm{conv}(B)$ (un site seul pour le rayon nul). La somme sur $J$ est une différence finie : $\sum_{j}(-1)^{j}\binom{p}{j}\binom{j+s-1}{k-1}=(-1)^{p}\binom{s-1}{k-1-p}$. ∎

L'identité est purement combinatoire : elle ne suppose ni théorie de Morse ni position générale. Vérifiée ici sur 267 couples (nuage, ordre) dégénérés et sur les trois trames.

## Annexe B. Obligations de preuve restantes et fixtures d'égalité

| Id | Obligation | État | Ce qui manque |
| --- | --- | --- | --- |
| O1 | Théorème T rédigé et relu, plateaux et multifusions N-aires compris (une multifusion = une classe de la relation engendrée au niveau $a$) | preuve en annexe A.4, non relue ; esquisses PO-T1 à T6 dans la v10 | relecture par la lentille tour ; fixture du plateau ternaire |
| O2 | Multiplicités : proposition 3 et théorème T pour des multiensembles ; boules de rayon nul aux ordres 1 à $w$ ; Euler pondéré | lemme W prouvé ; Euler pondéré écrit dans `TOWER_v2.md` § 9.3 ; rien d'implémenté côté tour | décider servi ou refusé ; si servi, oracle $\Gamma_k$ sur copies |
| O3 | Borne de coût de l'arbre en fonction de $M-K$ sur une famille déclarée | aucune ; mesures seulement | énoncé, ou statut `experimental_target` assumé avec budget de nœuds |
| O4 | Loi du nombre de boules par site | mesurée ; modèle de Poisson conjecturé | preuve ou borne mémoire déclarée par point |
| O5 | Budgets de bits de chaque prédicat du générateur gardés à la compilation | un seul `static_assert` en v10 | porter les calculs du § 4 en `constexpr` |
| O6 | Clé flottante des niveaux : borne d'erreur de la conversion | affirmée ($<2^{-50}$), bande à $2^{-40}$ ; filet exact sur tous les voisins | la preuve n'est nécessaire que si le filet exact est retiré |
| O7 | Quotient local des grandes coquilles (plus de 24 sites) | refus | algorithme polynomial ou refus assumé |
| O8 | Inscription au registre des lemmes D, D-loc, L, C, A, P, M, Z, S, E, W, F, des théorèmes G, 1, T et de la proposition 5 | absente | une ligne par énoncé, avec fixture et mutant |

Fixtures d'égalité à graver (une par inégalité qui peut basculer) :

- D : un site dont le dernier dominateur est à égalité de distance en un coin de la boîte (il doit rester).
- A et P : centre exactement sur `env.hi` ; centre exactement sur le plan de coupe ; centre sur une face de la racine.
- C : $p=K-1$ avec un site exactement sur la sphère ; $p=K$.
- S : $p=\theta_q$ pour $q=2,3,4$ ; triplet d'un quadruplet support qui n'est pas aigu.
- Z : droite des équidistants passant par une arête ou un sommet de la boîte.
- Admission : $p+q_{\min}=K+1$ et $K+2$.
- Coquilles étendues : carré, triangle rectangle, coins du cube, octaèdre, tétraèdre et son centre ; coquille de 24 et de 25 sites.
- Niveaux : deux boules de niveaux égaux à représentations différentes (une q2 et une q3) ; deux niveaux voisins à $2^{-40}$ près.
- Coût : les 14 points du § 6.4 (un nœud attendu au défaut).
- Poids, si servis : paire (3, 1) aux ordres critiques {1, 4} ; triangle aigu (3, 1, 1) aux ordres {2, 4, 5}.

## Annexe C. Reçus de l'audit

Dossier : `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l01_math_catalogue/` (moins de 200 ko ; scripts, journaux, comptes, empreintes ; aucune coordonnée KITTI). Les scripts gardent les chemins `/tmp/v11-audit/l01_math_catalogue/` où ils ont tourné ; le binaire est celui du build Release de la copie des sources (`mhgp10_catalogue` sha256 `a3bbad50b81b…`).

| Exécution | Fichiers | Ligne de verdict |
| --- | --- | --- |
| E1 | `e1_test_catalogue_oracle.log` | `catalogue_oracle_checks 161 fails 0 balls 14132` |
| E2 | `oracle_indep.py`, `campagne_20261002_per10.log` | `clouds=220 runs=3960 records=432015 weighted_records=158121 extended_records=75666 fails=0 refus=0 delais=0` |
| E3 | `run_lidar.sh`, `catalogue_lidar_comptes.txt` | comptes par trame et par K ; table `by_q_p` de la trame 02 à K = 12 |
| E4 | `euler_juge.py`, `euler_lidar00.txt`, `euler_lidar01.txt`, `euler_lidar02.txt`, `mutants_euler.txt` | `ordres 1..10 : CONFORME` trois fois ; deux mutants `ECART` ; validation sur petits nuages : `euler_petit checks 267 fails 0 boules 5133 etendues 696` (sortie de session, graine 7, 2 par famille) |
| E5 | `jkm2.sh`, `jkm2_lidar.txt` | six lignes `EGAL` |
| E6 | `j1_echantillon.sh`, `j1_juge.py`, `j1_lidar02_k10.txt` | `j1 boules 2281 ecarts 0 q2=408 q3=1247 q4=626` |
| E7 | `tower_oracle_k10.py`, `tower_oracle_k10.log` | 20 nuages (4 par genre), 19 320 coupes aux ordres 1 à 10, 0 écart ; campagne arrêtée à 20 nuages sur 30 demandés |
| E8 | `amas_coins_14.u32le`, `petite_feuille_rejeu.sh`, `petite_feuille_rejeu.log` | nœuds : 1, 3 048 725, 27 966 119, délai |
| E9 | `gen_amas.py`, `amas_rejeu.sh`, `amas_rejeu.log` | 16 lignes, 3,8 à 25,6 nœuds par site |
| E10 | `degeneres_rejeu.sh`, `degeneres.txt` | refus `wide_leaf` à 312 points ; refus `shell_quotient_budget` de la tour à 144 et 108 points |
| E11 | `doublons_kitti.txt` | 0 doublon sur 8 nuages |
| E12 | `tour_lidar02_comptes.txt` | 1 683 088 nœuds à K5, 7 468 379 à K10 |
| C9 | `c9_niveau.py`, `c9_niveau.log` | 206 représentations égales à celle de $S^{*}$, 58 différentes, sur 264 |

Fichiers laissés sous `/tmp` parce qu'ils contiennent des coordonnées de trames : les lignes de dump des coquilles étendues à K = 12 (529, 341 et 1 559 lignes) et l'échantillon de 2 281 boules ; leurs empreintes sont dans `empreintes_fichiers_restes_sous_tmp.txt`.

Limites de cet audit.

- Les temps mesurés ici sont inutilisables (machine à 8 cœurs, charge 7 à 20). Je n'ai rejoué aucune session G4 ; les deux chiffres G4 cités viennent des reçus du dépôt.
- La référence Python `reference/test_ref.py` n'a pas été menée à son terme de mon côté (deux tests sur quatre verts avant arrêt, doublon de la lentille L02).
- Le théorème T de l'annexe A.4 est une preuve que j'ai écrite pendant l'audit ; elle n'a été relue par personne. Elle est cohérente avec l'oracle jusqu'à $n=13$ et $k=10$, ce qui ne remplace pas une relecture.
- Je n'ai pas vérifié le coût ni l'exactitude de la tour elle-même, ni le traitement des coquilles étendues dans `tower.cpp` au-delà de ce que l'oracle $\Gamma_k$ en voit.
- Les mutants des reçus J2 et J2c et la mutation de dump de l'audit continu sont cités, non rejoués.
