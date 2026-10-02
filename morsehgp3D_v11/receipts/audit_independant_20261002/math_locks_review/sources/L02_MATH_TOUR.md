# L02 — Mathématiques de la tour FULL : audit de `morsehgp3D_v10` pour la conception de la v11

2 octobre 2026. Lentille L02 de l'audit à fond de la v10. Heure relevée par `date -u` à la fin de la rédaction : 07:06 UTC.

```text
phase=audit_v10_pour_v11 (lecture seule)
sujet=morsehgp3D_v10 au HEAD afb081774 (arbre de lecture build/v11-worktree)
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé
```

Vocabulaire des modes de vérification : **lu** (lecture du code ou du document, ligne citée), **exécuté** (commande rejouée ici, sortie conservée), **mesuré** (compteur ou invariant calculé ici sur une entrée nommée), **prouvé ici** (preuve complète rédigée au § 4, à contre-lire par un tiers), **conjecturé**. Les temps locaux (hôte de 8 cœurs partagé, charge 10 à 19 pendant tout l'audit) ne sont jamais interprétés.

## 0. Résumé

1. **L'objet calculé est juste, et c'est établi ici plus fortement que par les portes du dépôt.** Un juge indépendant écrit pour cet audit reconstruit l'arbre de fusion exact de $\Gamma_k$ en `Fraction` et le compare, nœud par nœud, à la forêt C++ : naissances identifiées par leur population, multifusions N-aires avec leurs enfants exacts, image verticale de **chaque** nœud à la coupe fermée, attache de chaque site. Résultat : 640 nuages de 7 à 13 points dans huit familles (génériques, grilles, plans, amas, cosphériques, cocycliques, alignés, cubes), ordres jusqu'à 10, plus 15 fixtures gravées : 62 471 naissances, 34 785 fusions dont 15 521 à trois parents ou plus, 86 198 verticales, 49 466 attaches, **0 écart**.
2. **Le théorème derrière « minima fixes, morceaux locaux (Gordan), descente » est vrai sans position générale, mais il n'est écrit nulle part dans la v10.** Le dépôt n'en porte que l'énoncé (SPEC § 4, en-tête de `tower.hpp`) et des esquisses d'une ligne dans une conception privée ; le registre des preuves n'a aucune ligne v10. Le § 4 de ce rapport en donne une rédaction complète (classification des événements d'un niveau, fenêtre de rang, descente, plateaux, verticales, Euler), avec statuts et obligations restantes.
3. **La porte `mhgp10_tower_oracle` n'établit que le nombre de composantes et la partition des points entrés.** Cinq mutants sur six la traversent (multifusion binarisée, plateau non atomique, image verticale à la coupe ouverte, image de fusion sans remontée, niveau d'attache strict). Le juge de cet audit les tue tous les six ; trois invariants linéaires tuent aussi, à l'échelle, les cinq qui survivent.
4. **Aucun invariant global d'échelle n'est une porte en v10, alors qu'ils tiennent et ne coûtent presque rien.** Mesuré ici sur les trois trames du contrat (35 551, 39 885 et 45 845 sites) et sur 15 entrées synthétiques de 8 000, 16 000 et 32 000 points : Euler $\chi_K = 1$ pour $K = 1, \ldots, 5$ partout (0,05 à 0,7 s de calcul une fois le catalogue construit à $K + 2$) ; forêt d'ordre 1 égale au dendrogramme N-aire de l'arbre couvrant minimal euclidien de scikit-learn, poids et structure ; attaches et images verticales vivantes à la coupe fermée ; plateaux atomiques ; sortie identique à l'octet près pour 1, 2 et 4 fils et pour deux règles de descente différentes mais valides.
5. **L'exactitude est relative au catalogue, et la tour seule ne voit pas tout catalogue incomplet.** Expérience des catalogues amputés (64 nuages, 3 062 retraits d'une boule admissible) : la tour refuse dans 80,9 % des cas, publie une forêt inchangée dans 9,5 % et publie **une forêt fausse sans refus dans 9,6 %** (295 cas) ; Euler détecte ces 295 cas, sans exception.
6. **La conception normative `TOWER_v2` n'a pas été implémentée.** Le code livré est la tour d'ouverture (descente avec mémo, Kruskal par lots), accélérée ; l'index $(\pi, J)$, la requête `WA`, les ancres, les contributions datées, Euler, le refus d'une sphère absente, les juges d'échelle, les mutants, les fixtures épinglées et le différentiel v9 de la conception n'existent pas. Les « continuations datées » annoncées dans l'objet (README) ne sont pas calculées.
7. **Le Théorème 5 du manuscrit (K-arbre couvrant minimal du K-graphe de Gabriel) est faux en général**, même en position générale : contre-exemple exact à cinq points du plan, $K = 2$, où le graphe de Gabriel reste à jamais en deux composantes. La v10 n'en dépend pas (c'est précisément ce que la descente répare) ; la v11 ne doit pas y revenir.

Aucun constat bloquant dans cette lentille : rien de ce qui a été jugé n'est faux. Les constats majeurs portent sur ce qui garantit l'exactitude (preuves écrites, portes, invariants), pas sur l'exactitude constatée.

## 1. Périmètre lu

- Dans `build/v11-worktree/morsehgp3D_v10/` : `docs/SPEC_V10.md` (entier), `docs/conception/TOWER_v2.md` (entier, 1 636 lignes), `docs/conception/CONCEPTION_V10.md` (§ 0 à 2, 4.3, 6.4, 9.4, 10, 11.3), `docs/conception/README.md`, `README.md`, `PASSATION.md`, `src/tower/tower.hpp`, `src/tower/tower.cpp` (entier, 1 867 lignes), `src/tower/rank_search.hpp`, `src/catalogue/support.hpp`, `src/catalogue/catalogue.hpp`, `src/arith/geometry.hpp` et `.cpp`, `src/core/reasons.def`, `src/core/status.hpp`, `src/core/types.hpp`, `src/cloud/cloud.hpp`, `src/cloud/site_tree.hpp`, `cli/mhgp10_tower.cpp`, `cli/mhgp10_catalogue.cpp`, `reference/hgp10_ref.py`, `reference/test_ref.py`, `tests/oracle/test_tower_oracle.py`, `tests/oracle/test_catalogue_oracle.py`, `tests/regression/test_batch_equivalence.py`, `CMakeLists.txt`, `cmake/gates.cmake`, `cmake/run_expect.cmake`.
- Registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (recherche des lignes v10 et des lignes FULL v7 et v9), historique `git log` de `src/tower/` et de `tests/oracle/`.
- Thèse, pages PDF 35 à 134 (`build/v9-open-worktree/docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, texte extrait par `pdftotext`) : Déf. 6 à 8, 20 à 22, 25 à 31, Th. 2, 4, 5, 6, 7, Prop. 5 à 9, § 9.1, Algorithme 1.
- Pistes, jamais preuves : audit géant (`receipts/audit_geant_developpeur_20260930/RAPPORT_AUDIT_GEANT.md`, `TRACKER.md` lignes RG1, AT1, AT2, JG1 à JG3), `build/v10-persist/design/TOWER_v1.md`, `build/v10-verify-tower/verify.sh`, `build/v10-persist/g4/verify_tower.log`, `build/v10-r2-backup-20260930/oracles/`, notes v7 `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md` et `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md` (dans `build/v9-open-worktree`), rapport voisin `build/v11-persist/audit_v10/L06_CODE_TOUR.md` (constats 01 et 03, pour vérifier la cohérence des nombres).

## 2. Méthode

- **Build.** Copie des sources de la v10 (hors reçus et audits) sous `/tmp/v11-audit/l02_math_tour/v10src`, build Release (GCC 13.3, `-Werror`), `-j3`. Rien n'est écrit dans l'arbre de lecture.
- **Porte du dépôt rejouée.** `tests/oracle/test_tower_oracle.py` au HEAD : `tower_oracle_checks 73 fails 0 cuts 23444`, code 0.
- **Sonde `l02_dump.cpp`** (API publique de `mhgp10_core` seulement) : catalogue à `kcat`, tour à `K`, dump enrichi (population $I \cup U$ de chaque naissance), invariant d'Euler calculé sur le catalogue, invariants linéaires de cohérence.
- **Juge `l02_judge.py`** (Python, `Fraction`, aucune dépendance au code ni à la référence de la v10) : plus petite boule englobante exacte (lemme de Welzl, recoupée contre la force brute sur 10 392 parties, 0 écart), arbre de fusion de $\Gamma_k$ étiqueté par les naissances, comparaison B, M, V, P décrite au § 6.2.
- **Six mutants et deux variantes neutres** de `tower.cpp`, en copies sous `/tmp` (script `make_mutants.py`, chaque édition exige une occurrence unique). Chaque mutant est soumis à la porte enregistrée du dépôt (24 nuages, 73 exécutions), puis au juge de cet audit, puis aux invariants d'échelle.
- **Catalogues amputés** (`l02_amputation.py`) : la sonde retire une boule du catalogue avant la tour ; le juge classe la sortie (refus, forêt inchangée, forêt fausse publiée) et Euler est calculé sur le même catalogue.
- **Invariants globaux à l'échelle** : Euler, ordre 1 contre l'arbre couvrant minimal euclidien de `sklearn.cluster.HDBSCAN(min_samples=1)` (scikit-learn 1.9.1, jamais réimplémenté), cohérences linéaires, neutralité des variantes, invariance au nombre de fils. Entrées : les trois trames du contrat (`build/v10-g4-data-s1/lidar0{0,1,2}_full.u32le`, lues en place, aucun octet copié sous `/workspaces`) et les 15 entrées synthétiques existantes de `build/v10-scale-inputs` (régime spatial, 8 000, 16 000 et 32 000 points, cinq familles).
- **Règle d'échelle respectée.** Les oracles exhaustifs restent bornés à 13 points (ils *établissent* la vérité). À l'échelle, seuls des invariants globaux et des juges linéaires sont utilisés ; aucun juge cubique, aucune vérification exhaustive.
- **Thèse.** Le Théorème 5 est confronté par un script exact (`l02_these_th5.py`) qui construit le K-graphe de Gabriel de la Déf. 29 et le compare aux K-polyèdres de Čech.

Reproduction : annexe A. Pièces déposées : annexe B.

## 3. L'objet : la tour FULL telle que la v10 la calcule

### 3.1 Définition effective (lue dans le code, vérifiée par le juge)

Entrée : $n$ sites distincts à coordonnées entières de 18 bits. Une entrée à positions répétées est refusée (`multiplicity_unsupported`, `src/tower/tower.cpp:1157-1158`). Le catalogue fournit toute sphère critique $b$ (centre $c$, niveau $\lambda_b$ rationnel exact, intérieur strict $I$ de cardinal $p$, coquille complète $U$ de cardinal $m$, plus petit cardinal de support $q$) telle que $p + q \leq K + 1$, dans l'ordre (niveau exact, support canonique), avec un rang dense par niveau exact distinct.

Pour chaque ordre $k \leq \min(K, n)$, la sortie `OrderForest` (`src/tower/tower.hpp:100-117`) contient :

| Champ | Contenu exact |
| --- | --- |
| nœuds | les naissances d'abord (à $k = 1$ : les $n$ sites, rang 0 ; sinon dans l'ordre des boules), puis les fusions dans l'ordre de création (rang croissant) |
| `rank` | 0 pour le niveau nul, sinon rang du catalogue + 1 ; le niveau exact est `cat.level[rank - 1]` |
| `parent`, `child_off`, `child_val` | forêt à fusions N-aires ; une seule racine (sinon refus `root_count`) |
| `birth` | boule de naissance (site à $k = 1$), `kNone` pour une fusion |
| `lower` | pour $k \geq 2$, nœud de l'ordre $k - 1$ **vivant à la coupe fermée** du niveau de création du nœud (vérifié ici pour tous les nœuds) |
| `point_node`, `point_level` | entrée `core` : $D_k(x)$ entier exact et nœud vivant à la coupe fermée $D_k(x)$ ; entrée `cover` : première boule couvrante |

Règles de construction (`tower.cpp`, étages L, H, G, T, P, V) :

1. **Cellules.** Une boule $b$ a une cellule $(b, k)$ pour $\max(1, p + q - 1) \leq k \leq \min(K, p + m)$ (l. 1229-1244). On pose $t = k - p$.
2. **Structure locale** (`local_structure`, l. 556-632). Coquille régulière ($m = q$) : naissance à $k = p + m$ ; jonction à $k = p + m - 1$, de représentants $I \cup U \setminus \lbrace u \rbrace$, $u \in U$. Coquille étendue : énumération des parties de $U$ de taille $t$ *séparables* (le centre hors de leur enveloppe convexe fermée, `center_in_closed_hull`, `src/catalogue/support.hpp:16-51`) ; aucune : naissance ; sinon morceaux = classes de la relation « $A \cup A'$ séparable », un représentant $I \cup A$ par morceau ; au moins deux morceaux : jonction ; **un seul morceau : cellule `inert`, rien n'est enregistré** (l. 631).
3. **Descente** (`resolve`, l. 799-993). Une $k$-partie $F$ : semis si $F$ est la population d'une naissance régulière de $k$ sites ; sinon plus petite boule englobante exacte, puis (i) au moins $k$ sites strictement intérieurs : saut aux $k$ plus proches du centre ; (ii) sphère du catalogue dont la fenêtre contient $k$ : nœud de naissance, ou mémo de la cellule ; (iii) sinon premier représentant de la structure locale. Le niveau décroît strictement à chaque pas (garde exacte, l. 836-844).
4. **Forêt** (`kruskal`, l. 1041-1116). Les jonctions de même rang exact forment un lot ; les racines pré-lot de tous les représentants sont lues avant toute union ; chaque groupe d'au moins deux racines pré-lot devient **une** fusion dont les enfants sont exactement ces racines.
5. **Verticales** (l. 1658-1747). Naissance de $b$ à l'ordre $k$ : descente à l'ordre $k - 1$ d'une $(k-1)$-partie de $I \cup U$, puis ancêtre vivant au rang de $b$. Fusion : ancêtre, à son rang, de l'image d'un enfant ; tous les enfants doivent donner le même nœud (refus `vertical_naturality`).
6. **Ancêtres** (`Jumps`, l. 717-739 ; `ancestor`, l. 1119-1126) : pointeurs de saut de Myers, résultat égal à la remontée parent par parent.

### 3.2 Ce que la sortie ne contient pas

- **Aucune contribution datée, aucune couverture.** Le mot « contribution » n'apparaît dans `src/` que dans un commentaire sans rapport (l. 1301). Une cellule à un seul morceau ne laisse aucune trace. La forêt est donc l'arbre de fusion *abstrait* de $\pi_0(L_k)$ ; les ensembles de points des composantes (les K-polyèdres de la thèse) n'en sont pas lisibles en présence de coquilles étendues (§ 4.9). `README.md:21-23` annonce pourtant « continuations datées » dans l'objet.
- **Aucune identité canonique publiée** : ni digest, ni numérotation indépendante de l'ordre des boules, ni population des naissances dans le dump du CLI (le dump de `mhgp10_tower` ne donne que parent, niveau, image, et les attaches).
- **Aucune multiplicité** : entrée pondérée refusée.
- **Coquilles étendues au-delà d'un budget** : refus `shell_quotient_budget` si $m > 24$ ou si une cellule a plus de 20 000 parties séparables (l. 22-23, 582, 594) ; ces bornes sont des budgets d'énumération, pas des bornes mathématiques.

### 3.3 Confrontation à l'objet de la thèse (Parties I et II)

| Thèse | v10 | Écart |
| --- | --- | --- |
| Niveau = rayon $r$, boules fermées (Déf. 7, 20) | niveau = $r^{2}$ rationnel exact, coupes fermée et ouverte | changement de variable monotone, aucun écart d'objet |
| $L_K(r)$, amas continus $\pi_0(L_K(r))$ (Déf. 6, 7) | même objet, pour tous les $K \leq K_{\max}$ | aucun |
| $\Gamma_K$ : sommets = $K$-parties du complexe de Čech, adjacence « $\sigma \cup \tau$ est un simplexe » (Déf. 21) ; adjacences élémentaires suffisantes (Prop. 5) | $\Gamma_k$ à adjacences élémentaires | aucun ; `SPEC_V10.md:29-31` attribue au Th. 2 seul ce qui relève du Th. 2 **et** de la Prop. 5 |
| K-polyèdres = amas discrets = **ensembles de points**, recouvrants pour $K \geq 2$ (Déf. 8, 21, 22, Th. 2, remarque 3 de la Déf. 8) | forêt abstraite + attaches de points ; couvertures non publiées | la v10 ne produit pas les K-polyèdres ; l'entrée `core` ($C \cap X$) est ce que la remarque 1 de la Déf. 8 appelle « une erreur », l'entrée `cover` suit la Déf. 8 (divergence déclarée, `CONCEPTION_V10.md` § 2.1) |
| Position générale exigée pour les Th. 4 à 7 (Déf. 26 : aucun point hors de $\sigma$ sur le bord de sa boule minimale) | aucune hypothèse : coquilles étendues traitées par le quotient local | la v10 est plus générale |
| Réduction : K-graphe de Gabriel (Déf. 29), K-arbre couvrant minimal (Déf. 30, Th. 5), composantes non triviales seulement | catalogue de sphères critiques à fenêtre de rang + descente ; toutes les composantes, isolées comprises (FULL) | **le Th. 5 est faux en général** (§ 4.11) ; la v10 ne l'utilise pas |
| Un seul $K$ à la fois ; ordre $K \leq n - 1$ au chapitre 8 | tous les ordres, $K = n$ compris, et applications verticales $K \to K - 1$ | la « tour » (bifiltration de multicouverture restreinte à $\pi_0$) est un ajout du projet, absent de la thèse |
| Partition stricte par vote pondéré (§ 9.1) | hors de cette lentille | — |

L'écart de fond est donc double : la v10 calcule **plus** que la thèse (tous les ordres, les composantes isolées, les verticales, sans position générale) et publie **moins** (pas les ensembles de points des composantes).

## 4. Reformulation propre : définitions, théorèmes, preuves, statuts

Cette section est écrite pour être portée telle quelle dans la note mathématique de la v11. Tout y est énoncé pour des **sites distincts** dans $\mathbb{R}^{d}$ ; rien ne dépend de $d$ sauf la borne $d + 1$ sur le cardinal d'un support ($4$ en dimension $3$). **Aucune hypothèse de position générale.**

### 4.1 Définitions

- $X \subset \mathbb{R}^{d}$ fini, $n = \lvert X \rvert$. Pour $F \subseteq X$ non vide, $B(F)$ est sa plus petite boule fermée englobante (unique), $c_F$ son centre, $\beta(F)$ son rayon carré.
- $D_k(y)$ est la $k$-ième plus petite distance carrée de $y$ aux sites ; $L_k(a) = \lbrace y : D_k(y) \leq a \rbrace$ et $L_k^{<}(a) = \lbrace y : D_k(y) < a \rbrace$.
- $\Gamma_k(a)$ : les sommets sont les $k$-parties $F$ avec $\beta(F) \leq a$ ; chaque $(k+1)$-partie $G$ avec $\beta(G) \leq a$ relie toutes ses faces $G \setminus \lbrace g \rbrace$. $\Gamma_k^{<}(a)$ : même définition avec des inégalités strictes.
- **Sphère critique** $b$ : centre $c$, niveau $\lambda > 0$, $I$ = sites strictement intérieurs, $U$ = sites sur la sphère, avec $c \in \mathrm{conv}(U)$. On note $p = \lvert I \rvert$, $m = \lvert U \rvert$, $P_b = I \cup U$, et $q$ le plus petit cardinal d'un **support**, c'est-à-dire d'une partie affinement indépendante $S \subseteq U$ avec $c \in \mathrm{relint}\,\mathrm{conv}(S)$. La coquille est **régulière** si $m = q$ (alors $U$ est l'unique support), **étendue** sinon.
- Une partie $A \subseteq U$ est **séparable** si $c \notin \mathrm{conv}(A)$, enveloppe fermée ; par le théorème de Gordan, c'est équivalent à l'existence de $v$ avec $\langle v, x - c \rangle > 0$ pour tout $x \in A$. Toute partie d'une partie séparable est séparable.
- **Arbre de fusion** $T_k$ : pour chaque niveau critique $\lambda$ et chaque composante $C$ de $\Gamma_k(\lambda)$, soit $o(C)$ le nombre de composantes de $\Gamma_k^{<}(\lambda)$ contenues dans $C$. Si $o(C) = 0$, $C$ est une **naissance** (feuille de niveau $\lambda$) ; si $o(C) \geq 2$, $C$ est une **multifusion** (nœud de niveau $\lambda$ dont les enfants sont les nœuds courants de ces $o(C)$ composantes) ; si $o(C) = 1$, c'est une **continuation** : aucun nœud.
- **Application verticale** : pour $k \geq 2$, $\phi_k^{a}$ envoie la composante de $F$ dans $\Gamma_k(a)$ sur celle de $F \setminus \lbrace x \rbrace$ dans $\Gamma_{k-1}(a)$.

### 4.2 Théorème A (nerf) — `theorem_external`, complété

**Énoncé.** Pour tout $k \leq n$ et tout $a \geq 0$, $F \mapsto W_F(a) = \bigcap_{x \in F} \bar{B}(x, \sqrt{a})$ induit une bijection entre les composantes de $\Gamma_k(a)$ et celles de $L_k(a)$, naturelle en $a$ et compatible avec les inclusions $L_k(a) \subseteq L_{k-1}(a)$ ; de même entre $\Gamma_k^{<}(a)$ et $L_k^{<}(a)$.

**Preuve.** C'est le Théorème 2 du manuscrit (p. 60, numérotation imprimée) composé avec sa Proposition 5 (p. 86, adjacences élémentaires) : $L_k(a)$ est l'union des convexes compacts $W_F(a)$, non vides si et seulement si $\beta(F) \leq a$ ; $W_F(a) \cap W_{F'}(a) \neq \emptyset$ si et seulement si $\beta(F \cup F') \leq a$, et alors les $k$-parties de $F \cup F'$ sont reliées de proche en proche par ses $(k+1)$-parties. La version ouverte s'obtient avec les boules ouvertes. La compatibilité verticale vient de $W_F(a) \subseteq W_{F \setminus \lbrace x \rbrace}(a)$ ; $\phi_k^{a}$ est bien définie parce que les $(k-1)$-parties de $F$ sont reliées par $F$ elle-même, et que deux faces d'une même $(k+1)$-partie partagent une $(k-1)$-partie. $\square$

Aucune position générale n'est utilisée. Statut : la preuve de la thèse est complète ; la version ouverte et la naturalité verticale, absentes de la thèse, sont immédiates.

### 4.3 Lemmes

**Lemme 1 (boule minimale dans une boule critique).** Soit $b$ une sphère critique et $F \subseteq P_b$ non vide. Alors $B(F) = b$ si et seulement si $F \cap U$ n'est pas séparable ; sinon $\beta(F) < \lambda$.

*Preuve.* Si $c \in \mathrm{conv}(F \cap U)$ et $c' \neq c$, il existe $x \in F \cap U$ avec $\langle x - c, c' - c \rangle \leq 0$, donc $\lVert x - c' \rVert^{2} \geq \lambda + \lVert c - c' \rVert^{2} > \lambda$ : aucune boule de rayon carré $\leq \lambda$ autre que $b$ ne contient $F$. Sinon, Gordan donne $v$ avec $\langle v, x - c \rangle > 0$ sur $F \cap U$ ; pour $\varepsilon > 0$ assez petit, le centre $c + \varepsilon v$ est strictement plus proche de chaque point de $F \cap U$ et les points de $F \cap I$ gardent leur marge : $\beta(F) < \lambda$. $\square$

**Lemme 2 (Johnson).** Si $Q \subseteq X$ et $\lvert Q \rvert \geq k + 1$, les $k$-parties de $Q$ sont dans une même composante de $\Gamma_k(\beta(Q))$. Si de plus toute $(k+1)$-partie de $Q$ vérifie $\beta < a$, elles sont dans une même composante de $\Gamma_k^{<}(a)$.

*Preuve.* Deux $k$-parties de $Q$ se rejoignent en échangeant un élément à la fois ; chaque échange passe par une $(k+1)$-partie de $Q$. $\square$

**Lemme 3 (taille d'une partie non séparable).** Toute partie non séparable de $U$ contient un support ; elle a donc au moins $q$ éléments.

*Preuve.* Une partie minimale de $A$ dont l'enveloppe contient $c$ est affinement indépendante et contient $c$ dans son intérieur relatif (Carathéodory). $\square$

**Lemme 4 (structure stricte locale).** Soit $b$ une sphère critique, $k \leq p + m$, $t = k - p$, et $V_{<}(b, k)$ l'ensemble des $k$-parties $F \subseteq P_b$ dont la trace $F \cap U$ est séparable (par le lemme 1, ce sont les $k$-parties de $P_b$ de niveau $< \lambda$).

- (a) Si $p \geq k$ : $V_{<}(b, k)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(\lambda)$.
- (b) Si $p < k$ : toute $F \in V_{<}(b, k)$ est reliée dans $\Gamma_k^{<}(\lambda)$ à une partie $I \cup A$ avec $A \subseteq F \cap U$, $\lvert A \rvert = t$.
- (c) Si $p < k$ et si $A$, $A'$ sont deux parties de $U$ de taille $t$ telles que $A \cup A'$ soit séparable : $I \cup A$ et $I \cup A'$ sont dans la même composante de $\Gamma_k^{<}(\lambda)$.
- (d) Si $p < k \leq p + q - 2$ : $V_{<}(b, k)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(\lambda)$.

*Preuve.* (a) Soit $N \subseteq I$ de cardinal $k$. Pour $F \in V_{<}$, tant que $F \neq N$ : ajouter $x \in N \setminus F$ (la $(k+1)$-partie a la même trace sur $U$, donc un niveau $< \lambda$), puis retirer $y \in F \setminus N$ (la trace ne peut que diminuer). (b) Même échange : ajouter $x \in I \setminus F$, retirer un point de $F \cap U$, qui en compte au moins $t + 1$ tant que $F \not\supseteq I$. (c) Toutes les $k$-parties et $(k+1)$-parties de $I \cup A \cup A'$ ont une trace séparable ; lemme 2. (d) Par le lemme 3, toute partie de $U$ de taille $\leq q - 1$ est séparable ; les parties de taille $t$ et $t + 1 \leq q - 1$ le sont, et deux parties de taille $t$ se rejoignent par échanges ; conclure par (b) et (c). $\square$

### 4.4 Théorème B (classification des événements d'un niveau) — prouvé ici

**Énoncé.** Fixons $k$ et un niveau $\lambda > 0$. Les sommets et arêtes de $\Gamma_k(\lambda)$ absents de $\Gamma_k^{<}(\lambda)$ se répartissent selon leur boule minimale, qui est une sphère critique de niveau $\lambda$ ; ceux de la sphère $b$ sont contenus dans $P_b$. L'effet de $b$ sur les composantes est le suivant.

1. $p + m < k$ : aucun.
2. $p + m = k$ : **naissance** d'une composante réduite au sommet $P_b$, isolée dans $\Gamma_k(\lambda)$.
3. $p + m \geq k + 1$ et $V_{<}(b, k) = \emptyset$ : **naissance** d'une composante formée de toutes les $k$-parties de $P_b$. Ce cas se produit si et seulement si $p < k$ et aucune partie de $U$ de taille $t$ n'est séparable (ce qui force $t \geq q$).
4. $p + m \geq k + 1$ et $V_{<}(b, k) \neq \emptyset$ : toutes les composantes de $\Gamma_k^{<}(\lambda)$ qui rencontrent $V_{<}(b, k)$ sont réunies en une seule, à laquelle s'attachent les sommets nouveaux de $b$. De plus : si $p \geq k$ ou si $k \leq p + q - 2$, une seule composante est rencontrée (boule **inerte**) ; si $p + q - 1 \leq k \leq p + m - 1$, les composantes rencontrées sont exactement celles des représentants $I \cup A_j$, un par **morceau**, les morceaux étant les classes des parties séparables de taille $t$ pour la clôture transitive de « $A \cup A'$ est séparable ».

Deux sphères distinctes de même niveau n'ont ni sommet nouveau ni arête nouvelle en commun. Au niveau $0$, seul l'ordre $1$ a des sommets (les sites, tous naissances).

**Preuve.** Un simplexe $\sigma$ de niveau $\lambda$ a $B(\sigma) = b$ pour une sphère critique $b$ (son centre est dans l'enveloppe de ses points de bord), et $\sigma \subseteq P_b$ ; l'unicité de la boule minimale donne la répartition et la disjonction entre sphères. Les faces d'une arête nouvelle de $b$ sont des $k$-parties de $P_b$ : $b$ ne relie donc que des $k$-parties de $P_b$, et par le lemme 2 il les relie toutes dès que $p + m \geq k + 1$. Cas 2 : $P_b$ est de niveau $\lambda$ par le lemme 1 ; une $(k+1)$-partie de niveau $\leq \lambda$ qui la contient aurait $b$ pour boule minimale, donc serait dans $P_b$, de cardinal $k$ : impossible. Cas 3 : si toutes les $k$-parties de $P_b$ sont nouvelles, elles forment une composante que rien d'autre ne touche au niveau $\lambda$ (même argument) ; $V_{<} = \emptyset$ exclut $p \geq k$ (une partie de $I$ a une trace vide, séparable) et équivaut, en prenant $F \supseteq I$, à l'absence de partie séparable de taille $t$. Cas 4 : première phrase par le lemme 2 ; le reste par le lemme 4. $\square$

**Corollaires.**

- *Fenêtre de rang.* Une sphère n'est un événement de l'ordre $k$ que si $p + q - 1 \leq k \leq p + m$. C'est exactement la fenêtre du code (`tower.cpp:1231-1232`) ; l'admission $p + q \leq K + 1$ du catalogue est donc nécessaire et suffisante pour les ordres $\leq K$.
- *Coquille régulière.* Les seules parties non séparables de $U$ sont $U$ : naissance à $k = p + m$, jonction à $m$ morceaux $U \setminus \lbrace u \rbrace$ à $k = p + m - 1$, inertie ailleurs. C'est la règle analytique du code (l. 567-580, 1374).
- *Isolement d'une naissance.* Une naissance de niveau $\lambda$ n'est jamais enfant d'une fusion de niveau $\lambda$. C'est l'invariant de plateau (D) du § 7.
- *Le critère par paires du code est exact.* « $A \cup A'$ séparable » et « $A$, $A'$ faces d'une même partie séparable de taille $t + 1$ » ont la même clôture transitive : une union séparable contient un chemin d'échanges. Mieux, tout découpage **plus fin** que les morceaux reste correct : le théorème C ne demande à chaque représentant que d'appartenir à $V_{<}(b, k)$.

### 4.5 Théorème C (forêt à plateaux) — prouvé ici

**Énoncé.** Au niveau $\lambda$, soit $H$ le graphe biparti dont les sommets sont les composantes de $\Gamma_k^{<}(\lambda)$ et les sphères de niveau $\lambda$ du cas 4, une sphère étant reliée aux composantes qu'elle rencontre. Les multifusions de $T_k$ au niveau $\lambda$ sont exactement les composantes connexes de $H$ qui contiennent au moins deux composantes de $\Gamma_k^{<}(\lambda)$, avec ces composantes pour enfants ; les naissances sont les sphères des cas 2 et 3.

**Preuve.** Conséquence directe du théorème B : les sphères d'un même niveau n'interagissent que par les composantes anciennes qu'elles partagent. $\square$

C'est la sémantique « racines pré-lot lues avant toute union, un nœud par groupe » de `kruskal` (l. 1071-1111). Traiter les jonctions d'un même niveau une à une produit une chaîne de fusions de même niveau : faux, et le mutant `m_seq` le montre.

### 4.6 Théorème D (descente) — prouvé ici

**Énoncé.** Soit $F$ une $k$-partie, $k \geq 2$, et $b = B(F)$. Définissons un **pas valide** $F \to F'$ :

- si $p \geq k$ : $F'$ est une $k$-partie quelconque de $I$ (le code prend les $k$ plus proches du centre) ;
- sinon, si $V_{<}(b, k) = \emptyset$ : arrêt, $F$ est un sommet de la naissance $(b, k)$ ;
- sinon : $F' = I \cup A$ pour une partie séparable $A \subseteq U$ de taille $t$ quelconque.

Alors $\beta(F') < \beta(F)$, et $F$, $F'$ sont dans la même composante de $\Gamma_k(\beta(F))$. Toute suite de pas valides termine donc sur une naissance $N(F)$ de niveau $\leq \beta(F)$, et $F$ appartient à la composante de $N(F)$ dans $\Gamma_k(a)$ pour tout $a \geq \beta(F)$. Si $\beta(F) < \lambda$, c'est déjà vrai dans $\Gamma_k^{<}(\lambda)$.

**Preuve.** Décroissance : une partie de $I$ tient dans une boule concentrique strictement plus petite ; pour $I \cup A$, lemme 1. Liaison : $F \neq F'$ sont deux $k$-parties de $P_b$, donc $\lvert P_b \rvert \geq k + 1$ et le lemme 2 s'applique au niveau $\beta(P_b) = \lambda_b = \beta(F)$. Terminaison : les niveaux sont en nombre fini. $\square$

**Corollaire (indépendance des choix).** La composante de $N(F)$ à tout niveau $\geq \beta(F)$ ne dépend ni de la règle du saut, ni du représentant choisi, ni d'un éventuel raccourci (semis, mémo). C'est ce qui rend la descente « pure » et le mémo par cellule sûr (`tower.cpp:1448-1450`). Vérifié à l'échelle au § 7 (variantes `v_noseed` et `v_alt`).

Le nombre de pas n'a **aucune borne prouvée** autre que le nombre de niveaux ; mesuré ici : 3,6 millions de pas pour 3,0 millions de résolutions sur la trame 01 à $K = 5$.

### 4.7 Théorème E (exactitude de la construction, relativement au catalogue) — prouvé ici

**Hypothèses.** (H1) sites distincts ; (H2) le catalogue contient toute sphère critique avec $p + q \leq K + 1$, avec $I$ et $U$ exacts ; (H3) les niveaux sont comparés exactement ; (H4) les représentants d'une cellule en fenêtre sont des éléments de $V_{<}(b, k)$, au moins un par composante de $\Gamma_k^{<}(\lambda_b)$ rencontrée par $V_{<}(b, k)$ (c'est le cas des morceaux, théorème B).

**Énoncé.** Pour $k \leq K$, la forêt obtenue en (i) créant une feuille par cellule de naissance et, à l'ordre $1$, par site, (ii) envoyant chaque représentant sur la naissance où aboutit une descente valide, (iii) appliquant la règle des plateaux du théorème C aux hyperarêtes ainsi formées, est l'arbre de fusion $T_k$.

**Preuve.** Récurrence sur les niveaux critiques. Invariant : après le niveau $a$, deux naissances sont dans la même classe si et seulement si elles sont dans la même composante de $\Gamma_k(a)$, et toute composante contient une naissance (théorème D appliqué à l'un de ses sommets). Au niveau $\lambda$ : par le théorème B, les composantes anciennes réunies par $b$ sont celles de ses représentants ; par le théorème D, chacune contient la naissance où aboutit la descente de ce représentant, dans $\Gamma_k^{<}(\lambda)$, donc dans la classe pré-lot correspondante par l'invariant. Le théorème C conclut. $\square$

**Portée exacte.** La tour est exacte **relativement** à (H2). Rien dans le produit ne témoigne de (H2) : voir le § 7.5 et le constat 08.

### 4.8 Théorème F (verticales) — prouvé ici

**Énoncé.** (i) Si $v$ est une naissance de l'ordre $k \geq 2$ portée par $b$, son image par $\phi_k^{\lambda_b}$ est la composante de $\Gamma_{k-1}(\lambda_b)$ qui contient **toutes** les $(k-1)$-parties de $P_b$. (ii) Si $v$ est une fusion de niveau $a$, son image est l'ancêtre vivant à la coupe fermée $a$ de l'image de n'importe lequel de ses enfants. (iii) Pour un nœud $v$ vivant à la coupe $a$, $\phi_k^{a}(v)$ est l'ancêtre vivant à la coupe fermée $a$ de l'image de $v$ à son niveau de création.

**Preuve.** (i) $\lvert P_b \rvert \geq k = (k - 1) + 1$ : lemme 2 à l'ordre $k - 1$. (ii) et (iii) : fonctorialité du théorème A. $\square$

Le code calcule (i) par descente d'une $(k-1)$-partie de $P_b$ puis remontée au rang de $b$ (l. 1689-1702), avec le raccourci régulier « dernier représentant de la jonction de la même boule à l'ordre $k - 1$ » (l. 1678-1687), et vérifie (ii) pour tous les enfants. Jugé ici pour tous les nœuds (§ 6.2, contrôle V).

### 4.9 Proposition G (couvertures et continuations) — prouvé ici, mesuré

La **couverture** d'une composante est l'union de ses sommets ; c'est le K-polyèdre de la thèse.

- Si toutes les sphères en fenêtre sont régulières, la couverture d'un nœud est l'union des populations $P_b$ des naissances de son sous-arbre : aucune continuation ni aucune fusion n'apporte de point nouveau. *Preuve.* Aux boules inertes, tout point de $P_b$ appartient déjà à une $k$-partie de trace séparable de la composante (lemme 4) ; à une jonction régulière, chaque point de $P_b$ est dans un représentant.
- Sinon c'est faux : une sphère à coquille étendue peut ajouter un point à une composante sans fusion. **Fixture** : les quatre points $(8, 9, 0)$, $(5, 10, 0)$, $(2, 9, 0)$, $(5, 0, 0)$ sur le cercle de centre $(5, 5, 0)$ et de rayon $5$. À l'ordre $3$, la seule partie séparable de taille $3$ est formée des trois premiers ; au niveau $25$ la composante gagne le quatrième point, sans fusion ni naissance. Le juge compte ce cas (« gain de couverture sans fusion ») : 1 sur cette fixture, 281 dans les campagnes ; la forêt C++ est conforme, mais l'événement n'y laisse aucune trace.

Conséquence : pour publier les K-polyèdres exacts hors régularité, la tour doit enregistrer une contribution datée par cellule à morceaux, ou la relation « boule couvrante $\to$ nœud » (que le code sait produire en entrée `cover`, `ball_node`).

### 4.10 Théorème H (Euler) — prouvé (TOWER_v2 § 9.3, vérifié ici), mesuré

**Énoncé.** Pour $1 \leq K \leq n$, avec $[K = 1]$ valant $1$ si $K = 1$ et $0$ sinon : $$n \cdot [K = 1] + \sum_{b} e_K(b) = 1, \qquad \sum_{K \geq 1} e_K(b)\, y^{K-1} = y^{p} \sum_{T \subseteq U,\ c \in \mathrm{conv}(T)} (y - 1)^{\lvert T \rvert - 1}.$$

La somme porte sur toutes les sphères critiques. Pour une coquille régulière, $e_K(b) = (-1)^{m - 1 - j} \binom{m - 1}{j}$ avec $j = K - 1 - p$, nul hors de $0 \leq j \leq m - 1$.

**Preuve.** Pour une famille finie de boules fermées, $\mathbf{1}[n(y) \geq K] = \sum_{\lvert A \rvert \geq K} (-1)^{\lvert A \rvert - K} \binom{\lvert A \rvert - 1}{K - 1} \mathbf{1}[y \in \bigcap A]$ (identité binomiale en $n(y)$, le nombre de boules contenant $y$) ; la caractéristique d'Euler est additive sur les unions finies de convexes compacts et vaut $1$ sur une intersection non vide, donc $\chi(L_K(a)) = \sum_{\lvert A \rvert \geq K,\ \beta(A) \leq a} (-1)^{\lvert A \rvert - K} \binom{\lvert A \rvert - 1}{K - 1}$. On regroupe les parties $A$ par leur boule minimale (lemme 1 : $A = I' \cup T$ avec $I' \subseteq I$ et $c \in \mathrm{conv}(T)$), d'où la série génératrice en $y$ après la substitution $x^{j} \mapsto (y - 1)^{j - 1}$. Enfin, pour $a \geq \beta(X)$, tous les $W_F(a)$ contiennent $c_X$ : $L_K(a)$ est étoilé et $\chi = 1$. $\square$

**Portée.** Une sphère contribue aux ordres $p + 1 \leq K \leq p + m$ ; l'identité à l'ordre $K$ exige donc **toutes les sphères avec $p \leq K - 1$**, quel que soit $q$. Le catalogue du produit ($p + q \leq K + 1$) ne suffit pas ; un catalogue à $K + 2$ suffit (il contient ces sphères puisque $q \leq 4$), et l'admission $p \leq K - 1$ suffirait à moindre coût (constat 05). Une sphère régulière manquante change la somme d'un terme $\pm 1$ dès l'ordre $p + 1$ (pour une coquille étendue, au premier ordre où son coefficient est non nul) : l'identité est un témoin de complétude sensible, mais pas une preuve (des erreurs peuvent se compenser).

### 4.11 Contre-exemple I : le Théorème 5 du manuscrit est faux en général — exécuté

**Énoncé de la thèse** (Déf. 29 et 30, Prop. 6, Th. 5, p. 89 à 91, numérotation imprimée) : en position générale (Déf. 26), les ensembles de points des composantes non triviales du K-graphe de Gabriel élagué à $r$ sont les K-polyèdres non triviaux de Čech.

**Contre-exemple exact** ($K = 2$, cinq points du plan, en position générale au sens de la Déf. 26, vérifié en `Fraction`) : $A = (-100, 0)$, $C = (100, 0)$, $z = (1, -90)$, $y = (30, -85)$, $w = (3, 300)$. La paire $AC$ a deux intrus $z$ et $y$ dans sa boule diamétrale : les triangles $ACz$ et $ACy$ ne sont pas de Gabriel, et $AC$ rejoint silencieusement la composante de $Az$, $Cz$ au niveau $r^{2} = 10000$. Les seuls triangles de Gabriel sont $Azy$, $Czy$ et $ACw$. Le graphe de Gabriel a donc deux composantes à jamais, de points $\lbrace A, C, z, y \rbrace$ et $\lbrace A, C, w \rbrace$, alors que $\Gamma_2$ n'en a plus qu'une, $\lbrace A, C, z, y, w \rbrace$, dès $r^{2} = 10001440081/360000$. Le « K-arbre couvrant minimal » n'existe même pas comme arbre. La fixture E5 du dépôt donne un désaccord de même nature sur un intervalle de niveaux ($r^{2} = 83886/3563$ : Čech $\lbrace A, B, C, D, E \rbrace$, Gabriel $\lbrace A, B, C \rbrace$ et $\lbrace A, C, D, E \rbrace$). Sur des nuages aléatoires en position générale, 0 à 4 nuages sur 300 sont en désaccord à au moins un niveau selon la configuration (cinq configurations, $n$ de 5 à 7, $K$ de 2 à 3 ; 11 désaccords sur 1 500 nuages).

**Où la preuve de la Prop. 6 échoue.** L'invariant de récurrence porte sur les ensembles de points ; il ne retient pas qu'une facette née dans une coface non-Gabriel appartient désormais à une composante. Quand cette facette devient plus tard la facette active d'un simplexe de Gabriel, le graphe de Gabriel la traite comme un sommet neuf.

**Ce que la v10 fait à la place.** Sur ce nuage, le représentant $\lbrace A, C \rbrace$ de la jonction $ACw$ descend : sa boule minimale contient deux sites strictement intérieurs, saut vers $\lbrace z, y \rbrace$, qui est une naissance. La fusion est alors correcte ; le juge de cet audit le confirme (fixture `contre_exemple_th5_plan`, conforme aux cinq ordres). Le registre racine connaît le phénomène (« le flot des seules cofaces Gabriel […] reconstruit FULL » : `false_in_general`, E5) ; il n'y est pas dit que l'énoncé de la thèse lui-même tombe, ni donné de contre-exemple permanent.

### 4.12 Théorème J (règle cartésienne de TOWER_v2, non implémentée) — prouvé dans TOWER_v2 § 9.2, recoupé ici

**Énoncé (PO-T18).** Si l'on traite les hyperarêtes par rang croissant, dans un ordre quelconque à rang égal, par union-find et concaténation de listes, et si $J[i]$ est le rang de l'arête qui a collé les positions $i$ et $i + 1$ de l'ordre final, alors les multifusions sont exactement les intervalles maximaux de jonctions $\leq t$ dont le maximum vaut $t$, et leurs enfants sont les morceaux séparés par les jonctions égales à $t$.

La preuve écrite dans `TOWER_v2.md:836-870` est correcte (elle n'utilise que la contiguïté des composantes dans l'ordre de concaténation). Recoupement indépendant : 2 000 hypergraphes aléatoires à poids très répétés, 5 003 nœuds dont 3 839 à trois enfants ou plus, 0 écart avec le balayage par lots atomiques (`l02_cartesian_rule.py`). La règle est donc disponible pour la v11 si le noyau séquentiel par lots devient le plafond.

### 4.13 Tableau des statuts et obligations restantes

| Id | Énoncé | Statut après cet audit | Trace écrite en v10 | Contrôle |
| --- | --- | --- | --- | --- |
| A | nerf, coupes fermée et ouverte, naturalité verticale | `theorem_external` (thèse Th. 2 + Prop. 5) complété | thèse ; `SPEC_V10.md:25-31` | — |
| B | classification des événements, fenêtre, quotient local | prouvé ici, à contre-lire | énoncé seul (`SPEC_V10.md:49-56`, `tower.hpp:3-9`) ; esquisses hors dépôt (`build/v10-persist/design/TOWER_v1.md:632-648`) ; antécédents v7, dans un autre vocabulaire et pour un autre résolveur : bloc inerte (`docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md`, th. 4.2), fenêtre et ancres (`morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md`, hors de l'arbre de lecture) | juge L02, toutes familles |
| C | plateaux atomiques | prouvé ici | énoncé seul | juge L02 ; mutants `m_bin`, `m_seq` |
| D | descente, indépendance des choix | prouvé ici | énoncé seul ; `TOWER_v2.md:809-834` prouve un autre résolveur (saut depuis le centre dans tous les cas) | juge L02 ; variantes neutres à l'échelle |
| E | exactitude relative au catalogue | prouvé ici | absent | juge L02 |
| F | verticales à la coupe fermée | prouvé ici | énoncé en commentaire (`tower.cpp:1658-1662`) | juge L02 (tous les nœuds) ; mutants `m_vopen`, `m_vnoclimb` |
| G | couvertures : union des feuilles en régulier seulement | prouvé ici ; fixture du gain de couverture | absent | juge L02 (compteur) |
| H | Euler, sites distincts | prouvé (`TOWER_v2.md:872-893`), vérifié ici | conception seulement ; **aucun code** | petits nuages et échelle (§ 7) |
| I | Th. 5 de la thèse | `false_in_general`, contre-exemple gravé ici | registre racine : formulation voisine, sans contre-exemple à l'énoncé | `l02_these_th5.py` |
| J | règle cartésienne | prouvé (conception), recoupé ici | conception seulement ; non implémenté | `l02_cartesian_rule.py` |
| — | première boule couvrante dans le catalogue (entrée `cover`) | prouvé ici en passant : si $F \ni x$ minimise $\beta$, sa boule vérifie $p + q \leq K + 1$ | commentaire (`tower.cpp:1531-1534`) | lentille L03 |

**Obligations restantes pour la v11** (rien de ce qui suit n'est prouvé ni jugé en v10) :

1. **Complétude du catalogue** (H2) : théorème du générateur, hors de cette lentille ; seul témoin indépendant disponible à l'échelle : Euler.
2. **Multiplicités.** L'extension aux multiensembles (fenêtre en poids, Euler pondéré, quotient pondéré) n'existe que dans `TOWER_v2` § 3.9, 9.3 à 9.5 ; le code refuse. À décider avant tout : une trame avec doublons est aujourd'hui refusée.
3. **Coquilles étendues.** L'énumération est exponentielle en $m$ et bornée par des budgets ; aucune borne de $m$ n'est prouvée pour les entrées du contrat (mesuré : $m \leq 5$ sur les trois trames). Les quotients circulaire et pondéré de la conception ne sont ni codés ni jugés.
4. **Longueur des descentes** : aucune borne.
5. **Arithmétique** : la garde de décroissance et les sélections en double reposent sur une marge (`kApproxMargin`, `tower.cpp:27`) justifiée par un commentaire ; hors de cette lentille.
6. **Inscription au registre** : aucune de ces lignes n'y figure.

## 5. « Tous ordres ensemble par étages, verticales à pointeurs de saut » : énoncés, preuves, hypothèses

Cette construction (commit `025fb7782`, 29 septembre) est une réorganisation de la tour d'ouverture (`6b4b4ccee`, `9af04985c`, 28 septembre) ; elle ne change pas l'objet. Les énoncés qui la fondent, et ce qui en est écrit :

| Énoncé | Où il est écrit | Preuve écrite | Jugé |
| --- | --- | --- | --- |
| Les ordres sont indépendants à catalogue donné ; les étages (atlas, semis, descentes, Kruskal, attaches, verticales) peuvent être menés pour tous les ordres à la fois | `SPEC_V10.md:62-63`, `tower.hpp:16-29` | immédiat (théorème E par ordre ; seules les verticales couplent $k$ et $k - 1$, après les deux forêts) | préfixe : ordres 1 à 5 identiques avec le catalogue à 7 et à 12 (trame 01, comptes par ordre) |
| La descente est une fonction pure de $(F, k)$ ; le mémo par cellule n'est qu'un cache | `tower.hpp:22-23`, `tower.cpp:1448-1450` | absente du dépôt ; ici, corollaire du théorème D (la valeur mémorisée est l'aboutissement de la descente du premier représentant de la cellule, toujours le même) | dumps identiques à 1, 2 et 4 fils (trame 01, $K = 5$) |
| Le semis $H_k$ (population d'une naissance régulière de $k$ sites) donne le même terminal que le chemin général | `tower.hpp:21` | absente ; immédiate : la boule minimale de $I \cup U$ est $b$ (lemme 1), cellule de naissance | variante `v_noseed` : dump identique |
| Recensement $(I, U)$ lu au catalogue quand le support certifié de la boule minimale est le support canonique d'une boule | `tower.cpp:848-858` | repose sur l'exactitude du recensement du générateur (hors lentille) ; juge d'échantillon 1 boule sur 32 (l. 884-894) | 0 refus `census_mismatch` sur toutes les entrées |
| Kruskal par plateaux : racines pré-lot figées, un nœud par groupe | `tower.cpp:1038-1040` | absente ; ici théorème C | juge L02 ; invariant (D) à l'échelle |
| Pointeurs de saut de Myers : `ancestor(v, r)` est l'ancêtre le plus haut de rang $\leq r$, égal à la remontée parent par parent | `tower.cpp:717-720` | classique ; hypothèse : rangs croissants vers la racine (strictement, par l'isolement des naissances) | dumps identiques au binaire figé d'avant les pointeurs (journal privé `build/v10-persist/g4/verify_tower.log`, 8 entrées) ; empreinte de la trame 01 à $K = 5$ retrouvée ici (`6ebb1eb4255a0903…`) |
| Image verticale d'une naissance régulière = ancêtre du dernier représentant de la jonction de la même boule à l'ordre $k - 1$ | `tower.hpp:28-29`, `tower.cpp:1678-1687` | absente ; ici théorème F (toute $(k-1)$-partie de $P_b$ convient) | juge L02 (tous les nœuds) ; invariant (C) à l'échelle |

**Hypothèses réelles de l'ensemble** : sites distincts ; catalogue complet et exact pour $p + q \leq K + 1$ ; coordonnées de 18 bits (marges des filtres flottants) ; coquilles étendues de 24 sites au plus et de 20 000 parties séparables au plus par cellule.

**Cas dégénérés.** Égalités de niveaux : lots par rang exact, théorème C ; sur la trame 00 à l'ordre 1, 7 350 rangs portent au moins deux fusions. Coquilles cosphériques ou cocycliques : quotient local par énumération, théorème B ; jugé jusqu'à $m = 12$ (campagnes : 8 656 boules étendues, histogramme au § 6.2). Sites alignés ou coplanaires : supports de 2 ou 3 sites, rien de particulier ; jugés (familles `line`, `plane`, `circle`). Sites répétés : refus.

## 6. Ce que les oracles établissent, et ce qu'ils n'établissent pas

### 6.1 Les quatre « preuves par oracle » de la v10

| Oracle ou contrôle | Ce qu'il compare | Ce qu'il établit | Ce qu'il n'établit pas |
| --- | --- | --- | --- |
| `reference/test_ref.py` (référence Python contre $\Gamma_k$) | à chaque niveau critique, coupes ouverte et fermée : la liste des **couvertures** des composantes ; la partition $C \cap X$ | que la théorie (catalogue brut, morceaux, descente) reproduit les couvertures de $\Gamma_k$ sur 25 nuages génériques et 40 grilles $\lbrace 0, 1, 2 \rbrace^{3}$ de 4 à 8 points, le carré, le cube et E5, $K \leq 4$ (rejoué ici au HEAD : 4 tests, OK) | rien sur le C++ ; rien sur les verticales (la référence n'en a pas) ; rien au-delà de 8 points et de $K = 4$ ; non enregistré dans CTest |
| `mhgp10_tower_oracle` (C++ contre $\Gamma_k$) | à chaque niveau critique, coupes ouverte et fermée : le **nombre** de nœuds vivants ; la partition des sites entrés (`test_tower_oracle.py:136-146`) ; pour $k \geq 2$, l'image de la composante d'un site entré, **après remontée** (l. 83-87, 90-114) | le nombre de composantes à toutes les coupes et la partition $C \cap X$, sur 24 nuages de 12 points au plus, $K \in \lbrace 1, 3, 5 \rbrace$, et E5 à $K = 4$ : 73 exécutions, 23 444 coupes, 0 écart (rejoué ici) | quelles composantes fusionnent quand aucun site n'y est entré ; l'arité des fusions et l'atomicité des plateaux ; que `lower` est le nœud vivant à la coupe fermée ; que le nœud d'attache est vivant à la coupe fermée ; les ordres 6 à 10 ; un seul plancher (500 coupes) ; les contrôles verticaux ne comptent dans aucun plancher (l. 153) |
| « dumps identiques au binaire figé `4a3d09d8a` sur 8 entrées » (`PASSATION.md:88`) | sorties de deux versions du même algorithme | la neutralité d'une optimisation, le 29 septembre | l'exactitude ; ce n'est pas une porte : la trace est un journal privé (`build/v10-persist/g4/verify_tower.log`), sans reçu |
| « 1 fil = 4 fils » | étiquettes de `mhgp10_cluster` sur deux scènes de 2 000 points (`tests/regression/test_batch_equivalence.py`) ; dumps de la tour dans le même journal privé (2 entrées) | l'invariance des étiquettes au nombre de fils à 2 000 points | l'invariance de la tour elle-même aux tailles d'intérêt (rejouée ici : identique à 1, 2 et 4 fils sur la trame 01) |

`SPEC_V10.md:80` décrit la porte C++ par « forêts et partitions C∩X égales à l'oracle $\Gamma_k$ à tous les niveaux critiques » : c'est plus que ce qu'elle compare.

### 6.2 Le juge de cet audit

`l02_judge.py` construit, pour chaque ordre, l'arbre de fusion $T_k$ de la définition du § 4.1 par un balayage exhaustif de $\Gamma_k$ en `Fraction`, puis exige du dump C++ :

- **B** : bijection des naissances par (niveau exact, couverture $= I \cup U$ de la boule de naissance) ;
- **M** : pour chaque fusion, niveau exact, ensemble des feuilles et ensembles de feuilles de chaque enfant identiques ;
- **V** : pour **chaque** nœud d'ordre $k \geq 2$, `lower` est le nœud de l'ordre $k - 1$ dont l'ensemble de feuilles est celui de la composante de $\Gamma_{k-1}$, à la coupe fermée du niveau du nœud, d'une $(k-1)$-partie d'un sommet du nœud ;
- **P** : pour chaque site, niveau d'entrée $D_k(x)$ et nœud vivant à la coupe fermée de ce niveau ;
- **Euler** : $\chi_{K'} = 1$ pour tout $K' \leq \min(n, K)$, sur le catalogue construit à $K + 2$.

Résultats sur le build de référence (HEAD) :

| Campagne | Nuages | Ordres | Naissances | Fusions (dont $\geq 3$ parents) | Verticales | Attaches | Euler | Écarts |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| graine 11 : $n$ de 7 à 11, $K = 5$ (80 %) ou 7 | 240 | 1 308 ordres jugés | 14 885 | 8 613 (3 590) | 19 849 | 11 830 | 1 308 | 0 |
| graine 21 : $n$ de 10 à 12, $K = 8$ (80 %) ou 10 | 200 | 1 666 ordres jugés | 23 499 | 12 937 (5 831) | 32 804 | 18 476 | 1 666 | 0 |
| graine 22 : $n$ de 10 à 12, $K = 8$ (80 %) ou 10 | 200 | 1 680 ordres jugés | 23 582 | 13 105 (6 018) | 33 045 | 18 478 | 1 680 | 0 |
| 15 fixtures gravées (E5, carré, rectangle et centre, cube, cube et centre, octaèdre, octaèdre et centre, cinq points de `BALL_ANCHORS`, gain de couverture, cercle de 12 points, cercle et centre, triangle rectangle, cinq alignés, contre-exemple du Th. 5, coins u18), tous les ordres $\leq \min(n, 10)$ | 15 | — | 505 | 130 (82) | 500 | 682 | 92 | 0 |

Couverture de ces campagnes (mesurée par `l02_coverage.py` sur les mêmes graines) : 61 400 boules de catalogue, dont 8 656 à coquille étendue ($m = 3$ : 5 637 ; 4 : 1 916 ; 5 : 733 ; 6 : 197 ; 7 : 92 ; 8 : 35 ; 9 : 19 ; 10 : 16 ; 11 : 10 ; 12 : 1) ; 587 naissances de population supérieure à $k$ ; arité maximale 16 ; 1 304 rangs portant au moins deux fusions ; ordres maximaux : $K = 5$ pour 186 nuages, 7 pour 54, 8 pour 327, 10 pour 73.

**Limite.** Les descentes y sont courtes : 1 103 sauts K-NN pour 153 112 résolutions de représentants. Le régime long du LiDAR (environ 205 000 sauts pour 3,0 millions de résolutions de représentants sur la trame 08/000100 à $K = 5$ ; environ 1,49 million de sauts à $K = 10$) n'est jugé par aucun oracle borné ; il l'est à l'échelle par la neutralité des variantes (§ 7), qui montre que deux descentes différentes aboutissent aux mêmes composantes, pas que ces composantes sont justes.

### 6.3 Six mutants

Chaque mutant est une copie de `tower.cpp` modifiée en un point (`make_mutants.py`). « Porte du dépôt » : `tests/oracle/test_tower_oracle.py` complet (73 exécutions). « Juge L02 » : 64 nuages de 6 à 9 points. « Invariant d'échelle » : trame 01, $K = 5$.

| Mutant | Faute simulée | Porte du dépôt | Juge L02 | Invariant d'échelle qui le tue |
| --- | --- | --- | --- | --- |
| `m_bin` | multifusion remplacée par une chaîne de fusions binaires de même rang | **survit** (0 écart, 23 444 coupes) | tué, 64 nuages sur 64 | (D) plateaux : 216 726 violations ; structure N-aire de l'ordre 1 contre l'EMST |
| `m_seq` | chaque jonction est son propre lot (plateau non atomique) | **survit** | tué, 45 sur 64 | (D) : 278 violations ; structure N-aire de l'ordre 1 |
| `m_vopen` | image verticale d'une naissance prise à la coupe ouverte | **survit** | tué, 64 sur 64 | (C) image vivante : 350 834 violations |
| `m_vnoclimb` | image d'une fusion = image brute du premier enfant, naturalité non vérifiée | **survit** | tué, 64 sur 64 | (C) : 395 796 violations |
| `m_strict` | `niveau <= e` remplacé par `<` dans le rang d'entrée `core` (constat AT1 de l'audit géant) | **survit** | tué, 29 sur 64 | (A) attache vivante : 133 violations |
| `m_onepiece` | coquille étendue jamais jointe | tué (35 exécutions en écart sur 73, dont des refus `root_count`) | tué sur 50 nuages : 44 refus `root_count` du produit, **6 forêts fausses publiées** avec une seule racine | non mesuré |

Cinq fautes sur six traversent la porte enregistrée. Deux d'entre elles (`m_bin`, `m_seq`) contredisent une propriété que tous les documents annoncent (« multifusions N-aires jamais binarisées », « plateaux atomiques ») : cette propriété n'a aujourd'hui aucune porte. Remarque : l'égalité des **poids** de l'ordre 1 avec ceux de l'arbre couvrant minimal ne tue pas ces deux mutants (les multiensembles restent égaux) ; c'est la comparaison de la **structure N-aire** qui les tue.

## 7. Invariants globaux : lesquels existent, lesquels sont des portes

### 7.1 Tableau

| Invariant | Coût | En v10 | Mesuré ici |
| --- | --- | --- | --- |
| Une racine par ordre | $O(\text{nœuds})$ | invariant de produit (`tower.cpp:1112-1114`, raison `root_count`) | 0 refus sur toutes les exécutions d'échelle de cet audit |
| Niveau strictement décroissant à chaque pas de descente | par pas | invariant de produit (l. 836-844) | 0 refus |
| Naturalité des verticales (tous les enfants de toutes les fusions) | $O(\text{nœuds})$ | invariant de produit (l. 1719-1732) | 0 refus |
| Recensement d'une boule sur 32 parmi celles qu'une descente atteint | par boule | invariant de produit (l. 884-894) | 0 refus |
| **Euler** $\chi_K = 1$ | $O(\text{boules})$ une fois le catalogue construit à $K + 2$ | **absent** : aucun code, aucune porte (`grep -ri euler src cli tests` : un commentaire de `types.hpp`) | $\chi_K = 1$ partout, tableaux ci-dessous |
| **Ordre 1 = arbre couvrant minimal euclidien** | juge indépendant (scikit-learn, Prim en $O(n^{2})$ : 13 à 29 s à ces tailles) | **absent** | égal, poids et structure N-aire |
| (A) attache `core` vivante à la coupe fermée : $\text{niveau}(v) \leq D_k(x) < \text{niveau}(\text{parent}(v))$ | $O(nK \log)$ | absent de la tour et de sa porte ; côté tête, `validate()` n'en contrôle qu'une version large qui admet l'égalité avec le niveau du parent (`src/points/dendrogram.cpp:27-28`) | 0 violation |
| (B) cohérence points–verticales à la coupe $D_k(x)$ | $O(nK \cdot \text{profondeur})$ | porte bornée seulement (petits nuages) | 0 violation ; ne tue aucun des cinq mutants survivants |
| (C) `lower[v]` vivant à la coupe fermée du niveau de $v$ | $O(\text{nœuds})$ | absent | 0 violation |
| (D) plateaux atomiques : aucun nœud n'a un parent de même rang ; toute fusion a au moins deux enfants | $O(\text{nœuds})$ | absent ; `validate()` admet un enfant de même rang que son parent (`src/points/dendrogram.cpp:18`) | 0 violation |
| Neutralité des règles de descente valides | deux builds | absent | dumps identiques à l'octet près |
| Invariance au nombre de fils | deux exécutions | étiquettes à 2 000 points seulement | dumps identiques à 1, 2 et 4 fils |
| Nombre de naissances | lu dans le JSON | publié, aucune porte | cohérent : identité de forêt (naissances $- 1 = \sum (\text{arité} - 1)$) ; 361 211 naissances régulières d'ordre 5 sur 08/000200, égal au décompte du prototype de la critique de conception (`TOWER_v2.md:1038`) |
| Différentiel v9 | — | comptes du catalogue par $(q, p)$ comparés une fois (`SPEC_V10.md:84-85`) | 1 407 885 boules à $K = 5$ sur 08/000200 et 1 306 696 sur 08/000000, retrouvés |

Aucun des invariants en gras n'est une porte de la v10 ; `PASSATION.md:222` les range dans les chantiers ouverts.

### 7.2 Trois trames du contrat (sans sol, grille de 1 mm)

Euler avec le catalogue à $K + 2 = 7$ ; ordre 1 contre scikit-learn ; invariants linéaires à $K = 5$.

| Trame | Sites | Boules à 7 | Boules étendues ($m$ max) | $\chi_K$, $K = 1 \ldots 5$ | $\chi_6$, $\chi_7$ (hors portée) | Arêtes EMST = liens de l'ordre 1 | Nœuds N-aires de l'ordre 1, identiques | (A) attaches | (C) images | (D) nœuds |
| --- | ---: | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | 39 885 | 2 565 656 | 320 (5) | 1, 1, 1, 1, 1 | 304 161 ; −958 732 | 39 884 | 39 796 | 199 425 | 1 462 069 | 1 541 750 |
| 08/000100 | 35 551 | 2 104 698 | 204 (5) | 1, 1, 1, 1, 1 | 223 844 ; −741 137 | 35 550 | 35 461 | 177 755 | 1 235 709 | 1 306 721 |
| 08/000200 | 45 845 | 2 675 990 | 865 (5) | 1, 1, 1, 1, 1 | 262 257 ; −909 878 | 45 844 | 45 563 | 229 225 | 1 591 680 | 1 683 088 |

Les colonnes (A), (C), (D) donnent le nombre de contrôles ; toutes à 0 violation. Les valeurs hors portée montrent que l'identité n'est pas triviale : dès qu'il manque des sphères, la somme s'écarte de $1$ par centaines de milliers.

Trame 08/000100 à $K = 10$, catalogue à 12 (6 492 748 boules, 2,0 Gio de mémoire résidente) : $\chi_K = 1$ pour $K = 1, \ldots, 10$ ($\chi_{11} = 612\,326$, $\chi_{12} = -1\,695\,373$ hors portée) ; une racine par ordre ; (A) 355 510, (B) 319 959, (C) 5 883 033, (D) 5 954 045 contrôles, 0 violation ; ordres 1 à 5 identiques (nœuds, naissances, fusions, fusions à trois parents ou plus) à ceux du calcul à $K = 5$. Le rapport voisin `build/v11-persist/audit_v10/L01_MATH_CATALOGUE.md` (juge d'Euler écrit séparément, catalogue à 12 sur les trois trames) obtient le même nombre de boules pour cette trame et la même identité aux ordres 1 à 10 : deux calculs indépendants concordent.

Structure de la tour de 08/000200 à $K = 5$ (pour dimensionner les planchers de la v11) :

| Ordre | Nœuds | Naissances | Fusions | dont $\geq 3$ parents | Rangs portant $\geq 2$ fusions | Naissances à coquille étendue |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 91 408 | 45 845 | 45 563 | 272 | 7 069 | 0 |
| 2 | 207 884 | 118 797 | 89 087 | 29 536 | 4 012 | 0 |
| 3 | 314 861 | 182 823 | 132 038 | 46 059 | 3 714 | 93 |
| 4 | 459 559 | 273 879 | 185 680 | 77 692 | 1 153 | 125 |
| 5 | 609 376 | 361 326 | 248 050 | 95 623 | 1 855 | 115 |

Les multifusions à trois parents ou plus sont un tiers à 40 % des fusions dès l'ordre 2 ; les égalités exactes de niveaux se comptent par milliers. Ce sont les deux propriétés que la porte du dépôt ne voit pas.

**Neutralité et fils** (trame 08/000100, $K = 5$, dump complet de 68 400 175 octets, forêts, verticales et attaches) : `sha256 = 6ebb1eb4255a0903a581d304c7d7194a7ef0c0cafae8bac0ad8701032f1bf0cd` pour le HEAD à 1, 2 et 4 fils, pour `v_noseed` (sans semis : 3 437 356 boules minimales calculées au lieu de 841 427) et pour `v_alt` (saut vers les $k$ sites intérieurs les plus **éloignés** du centre, autres représentants : 348 302 sauts au lieu de 205 437). Les seize premiers chiffres sont ceux du journal privé du 29 septembre pour la même entrée.

### 7.3 Tailles d'intérêt : 8 000, 16 000, 32 000

Entrées existantes de `build/v10-scale-inputs` (régime spatial). Euler avec le catalogue à 7 ; ordre 1 contre scikit-learn.

| Famille | $n$ | Boules à 7 | $\chi_K = 1$, $K = 1 \ldots 5$ | Ordre 1 = EMST (poids, structure) | Fusions des ordres 1 à 5 (dont $\geq 3$ parents) | Sauts K-NN |
| --- | ---: | ---: | --- | --- | ---: | ---: |
| uniforme | 8 000 | 1 297 692 | oui | oui, oui | 257 569 (94 158) | 64 878 |
| uniforme | 16 000 | 2 691 255 | oui | oui, oui | 529 837 (194 623) | 131 868 |
| uniforme | 32 000 | 5 572 910 | oui | oui, oui | 1 086 367 (398 138) | 272 794 |
| amas | 8 000 | 1 174 907 | oui | oui, oui | 245 497 (88 148) | 76 513 |
| amas | 16 000 | 2 350 279 | oui | oui, oui | 490 663 (175 657) | 159 618 |
| amas | 32 000 | 4 722 146 | oui | oui, oui | 985 134 (351 863) | 324 420 |
| coquilles | 8 000 | 321 256 | oui | oui, oui | 96 691 (35 535) | 17 674 |
| coquilles | 16 000 | 648 694 | oui | oui, oui | 194 221 (71 658) | 37 891 |
| coquilles | 32 000 | 1 302 244 | oui | oui, oui | 389 440 (143 142) | 78 558 |
| filaments | 8 000 | 808 833 | oui | oui, oui | 189 309 (68 572) | 52 616 |
| filaments | 16 000 | 1 619 081 | oui | oui, oui | 379 494 (137 075) | 108 775 |
| filaments | 32 000 | 3 242 848 | oui | oui, oui | 759 605 (274 428) | 214 775 |
| terrain | 8 000 | 277 070 | oui | oui, oui | 89 145 (30 197) | 13 943 |
| terrain | 16 000 | 561 059 | oui | oui, oui | 179 079 (60 718) | 28 555 |
| terrain | 32 000 | 1 132 287 | oui | oui, oui | 361 233 (122 260) | 58 006 |

Quinze entrées sur quinze : Euler vrai aux cinq ordres, ordre 1 identique, aucune racine multiple, aucun refus.

### 7.4 Ce que coûterait un catalogue « Euler-complet »

Sur 08/000200, par $(q, p)$ (comptes du CLI `mhgp10_catalogue`) : le catalogue du produit à $K = 5$ a 1 407 885 boules ; l'ensemble nécessaire à Euler jusqu'à l'ordre 5 (toutes les sphères avec $p \leq 4$) en a 2 089 870, soit $\times 1{,}48$ ; le catalogue à $K + 2 = 7$ en a 2 675 990, soit $\times 1{,}90$. Le calcul de l'identité elle-même est négligeable (0,05 à 0,7 s ici, un fil, non optimisé ; 3,2 s pour 6,5 millions de boules).

### 7.5 Catalogues amputés : ce que la tour voit seule, ce qu'Euler voit

Expérience prévue par `TOWER_v2` § 13.2 (« amputated ») et jamais menée en v10. Sur 64 nuages de 8 à 10 points (huit familles), $K = 4$, catalogue construit à 6 : chaque boule admissible du produit ($p + q \leq 5$) est retirée tour à tour, soit 3 062 retraits ; la tour est rejugée par l'arbre étiqueté, Euler est recalculé.

| Issue | Retraits | Part |
| --- | ---: | ---: |
| la tour refuse (`census_mismatch` : 1 848 ; `root_count` : 629) | 2 477 | 80,9 % |
| la tour publie la forêt exacte (boule sans effet sur $\pi_0$) | 290 | 9,5 % |
| **la tour publie une forêt fausse, sans refus** | **295** | **9,6 %** |
| Euler détecte le retrait ($\chi_{K'} \neq 1$ pour un $K' \leq 4$) | 3 008 | 98,2 % |
| forêt fausse publiée **et** Euler muet | 0 | 0 % |

Les 54 retraits qu'Euler ne voit pas laissent tous la forêt exacte. Par type de boule, les forêts fausses publiées viennent surtout des sphères régulières à support de trois sites (157 sur 953 retraits) et de deux sites (61 sur 1 534), puis des coquilles étendues (57 sur 513). Le premier ordre en écart est l'ordre maximal $K$ dans 293 cas sur 295, et 238 de ces boules vérifient $p + q = K + 1$ : elles n'ont qu'une cellule de jonction, à l'ordre $K$, et aucune cellule de naissance construite dont l'absence ferait échouer une descente. C'est la zone aveugle annoncée par `TOWER_v2.md:760-775` ; elle est donc réelle, et large à l'ordre le plus élevé.

## 8. Constats

Gravité : **bloquant** = rend faux ou invalide un résultat ou un contrat ; **majeur** = à traiter dans la conception de la v11 ; **mineur** ; **info**. Aucun bloquant dans cette lentille.

### L02_MATH_TOUR-01 — majeur — Le théorème de la tour est vrai mais n'est écrit nulle part ; le registre des preuves n'a aucune ligne v10

- **Fait.** L'énoncé « naissance ou jonction de morceaux locaux, représentants rattachés par descente » figure dans `SPEC_V10.md:49-63`, `tower.hpp:3-14` et la docstring de `reference/hgp10_ref.py:11-18`, sans preuve. Les preuves sont des esquisses d'une ligne dans une conception **hors dépôt** (`build/v10-persist/design/TOWER_v1.md:632-648`), reprises par renvoi dans `TOWER_v2.md:779-780` (« à rédiger en note avant le code correspondant »). `CONCEPTION_V10.md:807-809` exige l'inscription au registre « avant le code qui en dépend » (lignes V10-R01 à V10-R39) ; `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (1 320 lignes) ne contient aucune occurrence de « v10 ». La preuve de `TOWER_v2` § 9.1 porte sur un résolveur (saut depuis le centre dans tous les cas) qui n'est pas celui du code.
- **Preuve.** `grep -n "v10\|V10" docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` : aucune ligne ; lignes citées. L'audit géant l'avait relevé (RG1) ; contrôlé ici indépendamment.
- **Vérification.** Lu.
- **Conséquence v11.** Écrire la note mathématique avant le code : le § 4 de ce rapport en est une rédaction complète (théorèmes A à J, obligations restantes), à faire contre-lire par un tiers, puis à inscrire au registre avec ses fixtures.

### L02_MATH_TOUR-02 — majeur — La conception normative `TOWER_v2` n'a pas été implémentée ; aucun document ne dit ce qui en reste normatif

- **Fait.** `CONCEPTION_V10.md:61` déclare la conception « normative pour la suite » et prévoit (l. 104, § 9.4) la « réécriture de la tour selon TOWER_v2 ». L'historique de `src/tower/` ne contient que la tour d'ouverture (`6b4b4ccee`, `9af04985c`) et son accélération (`025fb7782`). Sont absents du code : l'index $(\pi, J)$ et la requête `WA`, la règle cartésienne, les ancres par cellule, les contributions, la numérotation canonique et les digests, Euler (I2), le refus d'une sphère admissible absente (I5), les quotients circulaire et pondéré, les multiplicités, les modes `full` et `sampled`, les juges J-CENSUS, J-DESC, J-MF, les 24 mutants, les fixtures épinglées, le différentiel v9. Des huit portes prévues pour la tour (`CONCEPTION_V10.md:888-899`), une seule existe, à 73 exécutions au lieu de 5 000 nuages.
- **Preuve.** `git log -- morsehgp3D_v10/src/tower/` (13 commits) ; `grep -rn "jblock\|leaf_lo\|tower_digest\|merkle" src/tower` : rien ; `CMakeLists.txt:66-135` (13 portes, dont une pour la forêt).
- **Vérification.** Lu.
- **Nuance.** `SPEC_V10.md` et `PASSATION.md` décrivent fidèlement le code livré ; c'est la conception qui est restée en arrière.
- **Conséquence v11.** Un seul document d'objet, tenu égal au code. Reprendre de `TOWER_v2` ce qui est prouvé et utile (règle cartésienne, Euler, juges, mutants par copies) et l'écrire comme obligations de la v11, pas comme héritage implicite.

### L02_MATH_TOUR-03 — majeur — La porte `mhgp10_tower_oracle` n'établit que le nombre de composantes et la partition $C \cap X$ : cinq mutants sur six la traversent

- **Fait.** § 6.1 et 6.3. La porte ne compare ni les parents, ni l'arité, ni les populations des naissances ; elle relève elle-même les nœuds publiés avant de comparer (`top()`), ce qui masque toute erreur de hauteur.
- **Preuve.** `tests/oracle/test_tower_oracle.py:83-87, 136-146, 153` ; journaux `mutants_porte_du_depot/*.log` (cinq fois `tower_oracle_checks 73 fails 0 cuts 23444`, code 0) ; journaux du juge L02 sur les mêmes mutants.
- **Vérification.** Lu, exécuté. Recoupe le constat JG1 de l'audit géant (non contre-vérifié à l'époque) et le constat 03 du rapport L06, établis séparément.
- **Conséquence v11.** La porte d'oracle de la v11 doit comparer l'arbre étiqueté (contrôles B, M, V, P du § 6.2), avec des planchers par propriété (fusions à trois parents ou plus, niveaux à plusieurs événements, coquilles étendues par taille, ordres jusqu'à $K_{\max}$) et des mutants tués par construction.

### L02_MATH_TOUR-04 — info — La forêt C++ est l'arbre de fusion exact de $\Gamma_k$ sur toutes les entrées jugées, verticales comprises

- **Fait.** 640 nuages et 15 fixtures, 0 écart sur 62 471 naissances, 34 785 fusions, 86 198 verticales, 49 466 attaches, 4 746 identités d'Euler (§ 6.2). Coquilles étendues jusqu'à 12 sites, ordres jusqu'à 10, fixtures dégénérées comprises.
- **Preuve.** `campagnes/*.log`, `fixtures.log`.
- **Vérification.** Exécuté (oracle borné, donc établissant la vérité sur ces entrées).
- **Limite.** Descentes courtes à ces tailles (§ 6.2).
- **Conséquence v11.** La sémantique de la v10 se porte telle quelle ; le juge L02 peut servir de point de départ à la porte T2 de la v11 et d'ancre pour la campagne appariée v10/v11.

### L02_MATH_TOUR-05 — majeur — Aucun invariant global d'échelle n'est une porte, alors qu'Euler et l'ordre 1 contre l'EMST tiennent partout où ils ont été mesurés

- **Fait.** § 7. Euler vrai aux ordres 1 à 5 sur 18 entrées (trois trames, quinze synthétiques de 8 000 à 32 000 points) et aux ordres 1 à 10 sur une trame ; ordre 1 égal à l'arbre couvrant minimal de scikit-learn, structure N-aire comprise, sur les 18 entrées ; invariants linéaires (A), (C), (D) à 0 violation ; ils tuent les cinq mutants survivants.
- **Preuve.** `lidar/*.json`, `echelle/*.json`, `neutralite/*`.
- **Vérification.** Mesuré.
- **Conséquence v11.** Faire d'Euler (catalogue à admission $p \leq K - 1$, $\times 1{,}48$ en boules sur 08/000200), de l'ordre 1 contre l'EMST (structure, pas seulement les poids) et de (A), (C), (D) des portes d'échelle aux tailles 8 000, 16 000, 32 000 et sur les trames ; (C) et (D) coûtent un parcours des nœuds et peuvent être des invariants de produit.

### L02_MATH_TOUR-06 — majeur — « Continuations datées » et K-polyèdres annoncés, non calculés

- **Fait.** `README.md:21-23` et `CONCEPTION_V10.md` § 2.1 placent les « continuations datées » dans l'objet. Le code classe `inert` toute cellule à un seul morceau (`tower.cpp:631`) et ne publie ni contribution ni couverture. Hors régularité, une composante peut gagner un point sans fusion (proposition G, fixture à quatre points) : les K-polyèdres de la thèse ne sont pas reconstructibles depuis la seule forêt et les populations des naissances.
- **Preuve.** `grep -rn -i "contrib\|continuation" src cli` : un commentaire sans rapport ; fixture `continuation_avec_gain_de_couverture` (ordre 3, niveau 25) ; compteur « gain de couverture sans fusion » du juge : 281 occurrences dans les campagnes.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Décider de l'objet avant d'écrire : soit la tour est l'arbre abstrait de $\pi_0$ plus la relation « boule couvrante $\to$ nœud » (ce que fait la v10, à dire ainsi), soit elle porte les contributions datées. Ne plus annoncer ce qui n'est pas produit.

### L02_MATH_TOUR-07 — majeur — Le Théorème 5 du manuscrit est faux en général ; la v11 ne doit pas se rabattre sur le K-graphe de Gabriel

- **Fait.** § 4.11 : contre-exemple exact à cinq points du plan en position générale, $K = 2$ ; désaccord aussi sur E5.
- **Preuve.** `l02_these_th5.py` et `these_th5.log`.
- **Vérification.** Exécuté (arithmétique exacte), relu à la main pour l'exemple plan.
- **Conséquence v11.** La réduction correcte est celle de la v10 (toutes les sphères de la fenêtre de rang, rattachement de chaque représentant par descente). Graver le contre-exemple en fixture permanente et inscrire la ligne au registre ; le signaler à l'auteur de la thèse (énoncé à corriger : il faut rattacher les facettes nées dans une coface non-Gabriel). Toute implémentation de l'Algorithme 1 en hérite : aucune ne peut servir d'oracle de hiérarchie (HGP-old n'a pas été contrôlé ici).

### L02_MATH_TOUR-08 — majeur — L'exactitude est relative au catalogue, et rien dans le produit ne témoigne de sa complétude

- **Fait.** Quand la boule minimale d'une partie est une sphère en fenêtre **absente** du catalogue, la descente continue avec un représentant calculé à la volée (`tower.cpp:951`, puis 978-989) ; seul le cas « naissance absente » est refusé (`census_mismatch`). Le contrôle I5 de la conception n'existe pas, Euler non plus, et il n'y a pas de statut `catalogue_incomplete`. Une sphère de jonction manquante retarde ou supprime une fusion sans refus tant qu'il reste une racine.
- **Mesure.** Catalogues amputés (§ 7.5) : sur 3 062 retraits d'une boule admissible, 295 forêts fausses publiées sans refus (9,6 %) ; Euler les détecte toutes. Mutant `m_onepiece` : sur 64 nuages, 44 refus `root_count` et 6 forêts fausses publiées avec une seule racine.
- **Preuve.** Lignes citées ; `src/core/reasons.def` (aucune raison de complétude) ; `amputation.log` ; `mutants_juge_l02.log`.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Publier « exact relativement au catalogue » tant qu'un témoin indépendant n'a pas tourné ; prévoir un mode certifié (Euler) et le refus d'une sphère en fenêtre absente. Sur les 18 entrées d'échelle de cet audit, Euler est vrai : rien n'indique un catalogue incomplet aujourd'hui.

### L02_MATH_TOUR-09 — mineur — Extension non régulière par énumération à budgets ; multiplicités refusées ; quotients de la conception absents

- **Fait.** Coquille étendue : énumération de toutes les parties de taille $t$, puis test par paires en $O(s^{2})$ sur les $s$ parties séparables (`tower.cpp:581-631`) ; refus au-delà de 24 sites ou de 20 000 parties séparables. Ce sont des budgets, pas des bornes de l'objet. Sur les trois trames, $m \leq 5$ et 204 à 865 boules étendues sur 2,1 à 2,7 millions : aucun effet mesuré. Une entrée à sites répétés est refusée.
- **Preuve.** Lignes citées ; `lidar/*_k5_kcat7.json` (`ext_m_hist`).
- **Vérification.** Lu, mesuré.
- **Conséquence v11.** Garder l'énumération tant que $m$ reste petit sur les entrées du contrat, mais publier $m$ maximal et refuser par boule ; décider du sort des doublons avant le contrat « trames entières » (question ouverte 1).

### L02_MATH_TOUR-10 — mineur — « Dumps identiques au binaire figé » et « 1 fil = 4 fils » : journal privé, pas des portes ; rejoués et confirmés

- **Fait.** § 6.1. Rejoué ici : même empreinte que le journal du 29 septembre pour la trame 01 à $K = 5$ ; identique à 1, 2 et 4 fils ; identique pour deux variantes neutres de la descente.
- **Preuve.** `neutralite/lidar01_k5_hashes.txt` ; `build/v10-persist/g4/verify_tower.log` (piste).
- **Vérification.** Exécuté.
- **Conséquence v11.** Porte de déterminisme sur le dump de la tour aux tailles d'intérêt ; porte de neutralité (deux règles de descente valides), qui est le seul contrôle à l'échelle du régime des descentes longues.

### L02_MATH_TOUR-11 — info — La règle cartésienne de `TOWER_v2` (PO-T18) est correcte et disponible

- **Fait.** § 4.12.
- **Preuve.** `l02_cartesian_rule.py` : 2 000 essais, 0 écart.
- **Vérification.** Exécuté ; preuve de la conception relue.
- **Conséquence v11.** Option sûre si le Kruskal par lots séquentiel devient le plafond du contrat de temps.

### L02_MATH_TOUR-12 — mineur — La référence Python ne modélise pas les verticales et reste hors CTest

- **Fait.** `reference/hgp10_ref.py` n'a aucune fonction verticale ; `reference/test_ref.py` s'arrête à 8 points et $K = 4$ ; il n'est pas enregistré dans `CMakeLists.txt` (lancé à la main ; « ≈ 6 min » selon `README.md:75`). Rejoué ici au HEAD : 4 tests, OK.
- **Preuve.** Fichiers cités.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Une référence qui porte tout l'objet (verticales, attaches), enregistrée comme porte.

### L02_MATH_TOUR-13 — mineur — `mhgp10_tower --no-points --dump=…` meurt par signal

- **Fait.** Le dump lit `point_node[s]` même quand les attaches sont désactivées (`cli/mhgp10_tower.cpp:186-194`) : SIGSEGV sur toute entrée.
- **Preuve.** `cli_no_points_dump.log` (quatre points, code de retour 139) ; rencontré d'abord en lançant le juge de l'ordre 1 sur les trois trames.
- **Vérification.** Exécuté. Recoupe le constat CL1 de l'audit géant.
- **Conséquence v11.** Les sondes sont des portes : sorties validées, aucun signal.

## 9. Ce qui est solide et mérite un port explicite en v11

1. **La sémantique des cellules** : fenêtre $[\max(1, p + q - 1), \min(K, p + m)]$, admission $p + q \leq K + 1$, naissance si aucune partie de taille $t$ n'est séparable, jonction des morceaux sinon, règle analytique des coquilles régulières (théorème B). Jugée exacte ici.
2. **La descente comme fonction pure**, avec sa garde de décroissance stricte, et le fait prouvé que la sortie ne dépend d'aucun choix valide (théorème D) : la règle du saut et du représentant peut être choisie pour son coût.
3. **Les plateaux atomiques** (racines pré-lot figées, un nœud par groupe ; théorème C).
4. **Les verticales à la coupe fermée**, par descente d'une $(k-1)$-partie de la boule de naissance, avec contrôle de naturalité sur tous les enfants (théorème F).
5. **Le critère de Gordan exact** `center_in_closed_hull` et le support canonique (`src/catalogue/support.hpp`), sans flottant.
6. **Les invariants de produit existants** : une racine par ordre, décroissance stricte, naturalité, recensement échantillonné.
7. **Les fixtures** : E5, carré, cube, et celles de ce rapport (contre-exemple du Th. 5, gain de couverture, cinq points de `BALL_ANCHORS`, cercle de 12 points, coins u18).
8. **Le juge par arbre étiqueté** et **les invariants d'échelle** de cet audit, comme base des portes.
9. **Les nombres d'ancrage** : 1 407 885 boules à $K = 5$ sur 08/000200 ; naissances par ordre du § 7.2 ; empreinte `6ebb1eb4…` du dump de 08/000100 à $K = 5$.

## 10. Ce qu'il ne faut pas refaire

1. Juger une forêt par des **comptes** et des partitions relevées par le juge lui-même.
2. Annoncer comme acquis une propriété sans porte (« jamais binarisées », « plateaux atomiques », « image à la coupe fermée »).
3. Laisser une conception « normative » diverger du code sans le dire ; laisser des preuves à l'état d'esquisses « à rédiger avant le code ».
4. Décrire dans l'objet ce qui n'est pas produit (continuations datées).
5. Prendre « sorties identiques à une version antérieure » pour une preuve d'exactitude, ou garder une telle vérification dans un journal privé.
6. Revenir au K-arbre couvrant du graphe de Gabriel (Th. 5), ou à tout repli qui ignore les facettes rattachées silencieusement.
7. Traiter un budget d'énumération comme une frontière de l'objet sans le publier boule par boule.
8. Lancer une réécriture pour la performance avant d'avoir figé les juges qui diront si elle calcule le même objet.

## 11. Questions ouvertes

1. **Doublons.** Les trames du contrat lues ici n'en ont pas ($n$ = nombre de sites) ; qu'en est-il des trames avec sol et des autres séquences ? Sémantique pondérée (multiensembles) ou dédoublonnage déclaré ?
2. **Objet publié.** Arbre abstrait plus relation de couverture, ou contributions datées donnant les K-polyèdres exacts ?
3. **Complétude.** Euler en porte seulement, ou mode certifié du produit (coût : catalogue $\times 1{,}48$ à $K = 5$) ?
4. **Juge indépendant à l'échelle pour $K \geq 2$.** Il n'en existe pas : Euler juge le catalogue, la neutralité juge la cohérence des descentes. Un juge de descente indépendant (échange d'intrus, prévu par `TOWER_v2` § 13.4) ou un différentiel avec une seconde implémentation est-il exigé avant de revendiquer l'exactitude à l'échelle ?
5. **Coquilles étendues.** Quelle borne de $m$ sur les entrées réelles (mesuré : 5) ; faut-il le quotient par arrangement de grands cercles, ou l'énumération suffit-elle avec un refus par boule ?
6. **Longueur des descentes.** Aucune borne prouvée ; faut-il un plafond de pas publié ?
7. **Rangs exacts vers la tête.** `point_dendrogram` publie des niveaux en double et fusionne les rangs de niveaux exacts distincts dont les doubles coïncident (`tower.cpp:1828-1840`) : la v11 transmet-elle les rangs exacts ? (Frontière avec la lentille des points.)
8. **Thèse.** L'auteur souhaite-t-il corriger l'énoncé du Th. 5 (rattachement des facettes nées dans une coface non-Gabriel) ?

## 12. Recommandations pour la v11

1. **Note mathématique d'abord.** Reprendre le § 4 (définitions, lemmes 1 à 4, théorèmes A à J), le faire contre-lire, l'inscrire au registre avec les fixtures de l'annexe B. Statut de départ proposé : A `theorem_external` ; B à F `proved_here` après contre-lecture ; H `proved_here` ; I `false_in_general` ; J `proved_here`.
2. **Objet unique et canonique.** Par ordre : naissances (boule, population), fusions N-aires, niveau exact, image verticale à la coupe fermée ; identité canonique indépendante de l'ordre des boules et du nombre de fils ; digest.
3. **Porte d'oracle T2 par arbre étiqueté**, bornée à 14 points, avec planchers : fusions à trois parents ou plus, niveaux à plusieurs événements, coquilles étendues par taille jusqu'à 12, naissances de population supérieure à $k$, ordres 1 à $K_{\max}$, familles dégénérées, et les six mutants de cet audit tués.
4. **Portes d'échelle** (8 000, 16 000, 32 000 et trames) : Euler ; ordre 1 contre l'EMST, structure N-aire ; (A), (C), (D) ; neutralité de deux règles de descente ; déterminisme du dump en fils ; préfixe ($K = 5$ contre $K = 10$).
5. **Statuts honnêtes.** `exact_relative_to_catalogue` par défaut ; mode certifié nommé quand Euler a tourné ; refus typé d'une sphère en fenêtre absente.
6. **Liberté de conception prouvée.** Le choix du saut, du représentant et du découpage en morceaux (tout découpage plus fin que les morceaux convient) est libre : l'utiliser pour le coût, sous la porte de neutralité.
7. **Ne rien promettre sur les doublons, les couvertures et les coquilles larges avant d'avoir tranché les questions 1, 2 et 5.**

## Annexe A — Reproduction

Dossier de calcul : `/tmp/v11-audit/l02_math_tour/`. Sources de la v10 copiées dans `v10src/` depuis `build/v11-worktree/morsehgp3D_v10` (HEAD `afb081774`).

```text
cmake -S v10src -B build_ref -DCMAKE_BUILD_TYPE=Release && cmake --build build_ref -j3
python3 -B v10src/tests/oracle/test_tower_oracle.py build_ref                 # porte du dépôt : 73 exécutions, 0 écart
g++ -std=c++20 -O2 -I v10src/src tools/l02_dump.cpp build_ref/libmhgp10_core.a -lpthread -o tools/l02_dump
python3 -B tools/l02_judge.py tools/l02_dump 11 240 generic,grid,plane,clusters,sphere,circle,line,cube 7-11 5
python3 -B tools/l02_judge.py tools/l02_dump 21 200 generic,grid,plane,clusters,sphere,circle,line,cube 10-12 8
python3 -B tools/l02_fixtures.py tools/l02_dump
python3 -B tools/l02_amputation.py tools/l02_dump 64 4                      # catalogues amputés
python3 -B tools/make_mutants.py v10src .      # puis cmake de chaque copie, porte du dépôt et juge sur chacune
tools/l02_dump TRAME.u32le --k=5 --kcat=7 --threads=4 --no-points          # Euler + statistiques
tools/l02_dump TRAME.u32le --k=5 --kcat=5 --threads=2 --coherence --no-euler # invariants (A) à (D)
python3 -B tools/l02_emst_k1.py build_ref TRAME.u32le 2                     # ordre 1 contre scikit-learn
build_ref/mhgp10_tower TRAME.u32le --k=5 --threads=W --dump=F ; sha256sum F  # neutralité, fils
python3 -B tools/l02_these_th5.py ; python3 -B tools/l02_cartesian_rule.py 2000
```

## Annexe B — Pièces déposées (`build/v11-persist/audit_v10/preuves_l02_math_tour/`)

- `outils/` : `l02_dump.cpp`, `l02_judge.py`, `l02_fixtures.py`, `l02_coverage.py`, `l02_amputation.py`, `l02_emst_k1.py`, `l02_these_th5.py`, `l02_cartesian_rule.py`, `make_mutants.py`, `lint.py`, scripts de lancement.
- `porte_du_depot_head.log` : la porte `mhgp10_tower_oracle` rejouée au HEAD.
- `campagnes/` : journaux du juge L02 (graine 11 ; graines 21 et 22), couverture.
- `fixtures.log` : les 15 fixtures gravées.
- `mutants_porte_du_depot/` : sortie de la porte du dépôt sur chaque mutant ; `mutants_juge_l02.log` : verdicts du juge ; `mutants_invariants_echelle.txt`.
- `lidar/` : Euler et statistiques ($K = 5$, catalogue à 7 ; $K = 10$, catalogue à 12), invariants (A) à (D), ordre 1 contre l'EMST, comptes du catalogue par $(q, p)$. Aucun octet de nuage.
- `echelle/` : les 15 entrées synthétiques (Euler, ordre 1).
- `neutralite/` : empreintes des dumps (fils, variantes) et grands-livres des trois builds.
- `amputation.log` : catalogues amputés.
- `these_th5.log`, `regle_cartesienne.log`, `test_ref_head.log`, `cli_no_points_dump.log`.
