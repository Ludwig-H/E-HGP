# CONCEPTION_TOUR — tour FULL, index des sites et attaches de points de la v11

2 octobre 2026. Conception des modules `index`, `tower` et `points` de `morsehgp3D_v11`. Ce document conçoit ; il n'écrit pas le produit. Heure de fin de rédaction relevée par `date -u` : voir la dernière ligne.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=conception_moteur (lecture seule hors de build/v11-persist/conception/)
public_status=not_claimed
GCP non utilisé
```

**Vocabulaire.** *Prouvé* : argument écrit et relu, ici ou dans un rapport cité. *Mesuré* : nombre produit par une exécution, avec son lieu et son auteur (reçu G4 de la session 4 de la v10 ; rapports L01 à L06 de l'audit du 2 octobre ; contrôles de cette conception, dossier `preuves_tour/`). *Estimé* : produit d'une mesure par un rapport ou par un coût unitaire supposé ; le fondement est dit. *Conjecturé* : sans fondement mesuré. Les temps locaux (codespace de 8 cœurs, charge 19 à 25 pendant ce travail) ne sont jamais lus en valeur absolue : seuls les compteurs déterministes et les rapports pris dans une même passe comptent.

**Sources lues en entier.** `audit_v10/L01_MATH_CATALOGUE.md`, `L02_MATH_TOUR.md`, `L04_CODE_FONDATIONS.md`, `L05_CODE_CATALOGUE.md`, `L06_CODE_TOUR.md` ; `L03_MATH_POINTS.md` (§ 0, 3, 4, 7.04, 9 à 12) ; `morsehgp3D_v10/src/tower/tower.cpp` et `tower.hpp`, `src/cloud/site_tree.*`, `src/catalogue/catalogue.hpp`, `cli/mhgp10_tower.cpp` ; `docs/conception/TOWER_v2.md` (entier), `GEN_v2.md` (§ 3.9, 8, 9) ; `build/v10-perf/PLAN_PERF.md` et le prototype privé `tour/out/p2c_vs_base.tower.cpp.diff` ; `morsehgp3D_v11/docs/ARCHITECTURE.md` (état de 08:06 UTC, règles F1 à F6), `README.md`, `reference/README.md`, `reference/hgp11_ref/model.py` et `dumps.py`, `src/core/` ; les cinq notes de `morsehgp3D_v11/audits/` dont `AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` et la note `A_LIRE_AUDITEUR_VERROUS_20261002.md`. Arrivés pendant la rédaction et intégrés : `audit_v10/L08_TESTS_PORTES.md` (§ 0 et § 11, version de 08:33 UTC), `preuves_l09_perf_lidar/TABLE_VERITE_G4.md`, et les conceptions voisines `conception/PISTES_DE_RUPTURE.md` (§ 0, 1, 3, 4, 6, 7) et `conception/CONCEPTION_GENERATEUR.md` (§ 0, § 2), lues entre 08:36 et 08:41 UTC dans l'état où elles étaient. Absents à l'heure de la clôture : les rapports `L09` et `L15`, les vérifications `V_L*`, `docs/MATHEMATIQUES.md` de la v11.

**Contrôles exécutés pour cette conception** (annexe B, tous légers, un processus à la fois) : le noyau de forêt sans lots sur les entrées réelles du Kruskal de la v10 (six ordres de la trame 00 : forêts et numérotations identiques, rapport de temps 2,5 à 2,9) ; le quotient local polynomial des coquilles étendues contre la définition brute (5 937 contrôles, 0 écart ; 24 points cosphériques en 0,05 s de Python) ; la contraction des plateaux, l'historique d'attache et l'image des naissances sur 6 000 hypergraphes à égalités (0 écart) ; l'entrée `cover` lue sur les seules naissances contre l'étage de définition de la référence (1 317 entrées, 0 écart).

---

## 0. Résumé

1. **L'objet ne change pas.** Pour $k = 1, \ldots, K$ : arbre de fusion de $\pi_0(L_k(a))$, naissances, multifusions N-aires à plateaux atomiques, niveaux rationnels exacts, verticales $k \to k-1$ à la coupe fermée ; puis attaches `core` et `cover`. Les théorèmes A à J de L02 § 4 et les corrections de l'auditeur indépendant (surjection des morceaux, raffinement exhaustif, classe du terminal seule unique) sont le socle ; rien ici ne les affaiblit.
2. **La tour devient cinq étages purs**, tous parallèles sauf un noyau par ordre : P (préparation : listes par classe $p + q$, index des supports, semis), G (résolution des représentants, fonction pure à politique fixée, sans mémo partagé ni atomique), T (noyau union-find sans lots, un fil par ordre, recouvert par G), M (matérialisation parallèle : contraction des plateaux, numérotation canonique), V (verticales : $O(1)$ par naissance, une requête par fusion).
3. **Trois décisions portent le gain.** (a) Forêt sans lots : noyau mesuré 2,5 à 2,9 fois plus rapide que le Kruskal par lots sur les entrées réelles, forêt et numérotation identiques à l'octet près ; plus aucun passage séquentiel hors de ce noyau. (b) Certificat combinatoire de plus petite boule : pour 76 % des boules minimales (celles du catalogue), l'égalité $S^{*} \subseteq F \subseteq P_b$ remplace toute géométrie exacte. (c) Recensement borné aux $k$ plus proches sur un index implicite de 15 octets par site, à la place d'une boule fermée entière (jusqu'à la moitié du nuage en v10).
4. **Coquilles étendues : quotient polynomial unique** par ensembles séparables maximaux, en $O(m^{3})$ prédicats, pour les coquilles circulaires comme pour les coquilles 3D ; refus typé décidé avant tout calcul au-delà de 64 sites. Les 24 points cosphériques qui coûtent 49 à 57 s à la v10 se traitent en 13 000 prédicats.
5. **Index des sites : pas l'arbre du générateur.** Hiérarchie implicite de boîtes sur l'ordre de Morton, construite en $O(n)$ sans tri. L'arbre de boîtes ajustées ne couvre pas les centres dont la boule a au moins $K$ sites intérieurs, pèse 60 à 100 fois plus, et ferait dépendre le recensement de la tour des listes qu'il doit pouvoir contredire.
6. **Entrée `cover` : l'objet publié est l'ensemble des composantes de première couverture**, lu sans aucune descente sur les naissances de l'ordre ; la projection par premier ancêtre commun et la convention de la v10 sont publiées à part et nommées.
7. **Budget estimé de la tour FULL à 48 fils sur G4** : 22 à 32 ms à $K = 5$ (v10 mesurée : 67 à 89 ms), 125 à 180 ms à $K = 10$ (v10 : 334 à 472 ms). La cible de 30 ms à $K = 5$ est atteignable si l'étage G tient l'estimation (10 à 16 ms, coûts unitaires non mesurés). À $K = 10$, la tour seule dépasse 100 ms en CPU : le contrat de 100 ms à $K = 10$ n'est pas atteignable par cette conception (le nombre de descentes est déjà à 8 % de son minimum ; il ne reste que le coût du pas).

### 0.1 Table des décisions

| Id | Décision | Fondement | Gain attendu | Risque |
| --- | --- | --- | --- | --- |
| D-C1 | La tour lit une vue compacte du catalogue (enregistrement de 12 octets par boule et CSR $I$ puis $U$) ; elle est exacte relativement au catalogue ; elle refuse sans coût une boule de fenêtre absente, une racine multiple et un recensement contradictoire | prouvé (théorème E de L02) ; mesuré (catalogues amputés, L02 § 7.5) | statut honnête ; deux refus gratuits | la zone aveugle des jonctions manquantes reste (9,6 % des retraits en petit) : c'est le théorème du générateur qui la ferme |
| D-C2 | Euler et la restriction sont des portes d'échelle et un diagnostic nommé du catalogue, jamais une certification ni le chemin produit | prouvé (témoin à cinq points de l'auditeur : deux omissions qui se compensent) | aucun coût produit | aucun |
| D-C3 | Limite de coquille commune au catalogue et à la tour : $m \leq 64$ ; refus décidé sur le champ $m$ avant tout calcul | estimé (masque sur un mot ; coût $m^{3}$) | plus de coût explosif | à accorder avec la conception du catalogue |
| D-A1 | Atlas implicite : une liste de boules par classe $c = p + q$ ; la cellule d'une boule régulière est analytique ; aucun tableau par cellule | prouvé (théorème B, corollaire) | étages `t_local` et `t_seeds` de la v10 divisés par deux environ | faible |
| D-A2 | Quotient local polynomial par ensembles séparables maximaux, un seul algorithme pour le cercle et le 3D | prouvé ici (théorème T7) ; mesuré (5 937 contrôles, 0 écart) | 49 s ramenées à quelques millisecondes sur 24 points cosphériques | prédicats larges aux profils 21 et 24 bits |
| D-G1 | Résolution par une fonction pure à politique fixée, arrêt à la première cellule de fenêtre, pointeurs suivis après coup ; ni mémo partagé ni atomique | prouvé (théorème D ; lemme T3) | compteurs déterministes à tout nombre de fils ; chaînes dupliquées supprimées | faible |
| D-G2 | Semis par table plate à empreinte additive, vérification exacte ; toute naissance de population $k$ y entre | mesuré (86 % des descentes finissent sur un semis, L06 ; 70 à 72 % des représentants en sont un, conception voisine) ; estimé pour le coût | sonde de 180 à 100 cycles environ | disposition à trancher sur G4 |
| D-G3 | Plus petite boule : proposition flottante non décidante, puis certificat combinatoire par le catalogue ; certificat géométrique exact seulement hors catalogue ; repli Welzl exact | prouvé (lemme T1) ; part de 76 % mesurée (L06) | 2 000 cycles par pas ramenés à 700 environ | la proposition doit viser $S^{*}$ ; sinon chemin exact, toujours juste |
| D-G4 | Recensement hors catalogue par une requête bornée aux $k$ plus proches ; saut aux $k$ plus proches ; premier représentant pris dans le support | prouvé (théorème D ; lemme 3 de L02) ; mesuré (boule fermée de 1 258 sites, moitié du nuage en contraste) | coût par pas borné ; 4 600 cycles ramenés à 1 500 environ | coût de la requête non mesuré |
| D-G5 | Sortent du chemin produit : juge 1/32, seconde recherche de support, garde exacte de décroissance. Restent : garde gratuite par rangs, plafond de pas | mesuré (9 à 10 % des requêtes d'arbre ; 62 056 et 397 450 recherches perdues) | 3 à 5 % de G | une faute de descente n'est plus vue que par les portes |
| D-I1 | Index = hiérarchie implicite de boîtes entières sur l'ordre de Morton, blocs de 8 sites, construction séquentielle en $O(n)$ | estimé ; non-totalité de l'arbre ajusté prouvée (§ 3.6) | construction du k-d (6,7 à 8,0 ms en local, L04 C10) remplacée par une passe estimée sous 0,3 ms ; requêtes en cache L2 | qualité des boîtes de Morton à mesurer |
| D-I2 | Deux familles de requêtes : centre entier tout en entiers ; centre rationnel avec élagage par borne flottante certifiée à sens unique et décisions par clés exactes | obligation de preuve PO-I1 | aucune marge fixe, valable à tout profil $B$ | preuve à écrire dans `num` |
| D-F1 | Forêt sans lots : noyau union-find par taille, événements, contraction parallèle des plateaux, numérotation (rang, plus petite naissance) | prouvé (théorème J de L02 ; théorème T4) ; mesuré ici (six ordres réels identiques, rapport 2,5 à 2,9) | noyau de l'ordre maximal : 8 à 12 ms au lieu de 21 à 32 ($K = 5$) | interférence mémoire avec G à mesurer |
| D-F2 | Un fil propriétaire par ordre, les autres résolvent ; ordres par $k$ décroissant ; flux par morceaux en option mesurée | mesuré en local sur le prototype privé P2 (mur de la tour ×0,71 à ×0,78) ; estimé sur G4 | noyau recouvert par G | repli séquentiel à un fil indispensable |
| D-F3 | Requêtes d'ancêtre par l'historique d'attache (profondeur au plus $\log_2$ du nombre de naissances), sans pointeurs de saut | prouvé (lemme T5) ; mesuré ici (1,3 à 1,7 saut, 4,5 à 12,8 sondes) | plus aucune passe séquentielle après le noyau | coût par requête à mesurer |
| D-V1 | Verticales : image d'une naissance en $O(1)$ depuis le sommet laissé par la jonction de la même boule à l'ordre $k - 1$ ; image d'une fusion par une requête indépendante ; naturalité dans les portes | prouvé (théorème F ; lemme T6) ; mesuré ici (0 écart sur 2,4 millions de jonctions réelles) | étage V de 15 à 22 ms ramené à 2 à 4 ms | faible |
| D-P1 | `core` : $k$ plus proches en entiers, résolution, coupe fermée ; rang par dichotomie à clé flottante certifiée | prouvé (C1 de L03) | 2 à 4 ms à $K = 5$ | faible |
| D-P2 | `cover` : objet = ensemble $E_k(x)$ lu sur les naissances de l'ordre ; projection par ancêtre commun et convention v10 à part | prouvé (proposition T8) ; mesuré (19 à 151 sites à égalités par ordre, L06) | exactitude ; 20 ms de la v10 ramenées à 1 ms environ | aucun singleton équivariant n'existe : le dire |
| D-P3 | Relation boule couvrante → nœud : implicite pour les naissances, liste explicite pour les cellules étendues de fenêtre | prouvé (proposition G de L02) | K-polyèdres lisibles sans stockage | faible |
| D-M1 | Tout tableau en `Buffer`, tailles connues après une passe de comptage, admission par formule avant d'allouer | — | plafond mémoire vrai | arènes réutilisées exigées du socle |
| D-M2 | Export binaire versionné ; empreinte de Merkle indépendante de la numérotation (définition de L06, ancres v10 disponibles) ; dump texte au format v10 écrit par `io` | mesuré (ancres L06) | campagne appariée possible dès le premier jour | l'écriture non réduite des niveaux est une convention à porter à part |
| D-Q1 | Portes : oracle $\Gamma_k$ jusqu'à $K = 10$ à planchers par régime, juge qui relit les champs, EMST à l'ordre 1, isométries, neutralité, mutants | mesuré (cinq mutants sur six traversent la porte v10) | — | coût de la suite complète sur G4 |

---

## 1. Contrat avec le catalogue

### 1.1 Ce que la tour lit

La tour ne lit que quatre choses ; elle ne lit jamais l'arbre de boîtes du générateur (§ 3.6).

```cpp
struct BallRec {     // 12 octets par boule, dans l'ordre canonique (niveau exact, S*)
  LevelRank rank;    // rang dense du niveau exact parmi les niveaux du catalogue
  u32 pop_off;       // debut de I dans pop ; I trie par SiteIdx, puis U trie par SiteIdx
  u8 p;              // |I|, au plus kcat - 1
  u8 q;              // q_min : 2, 3 ou 4
  u8 m;              // |U|, au plus 64 (au-dela : refus avant calcul, D-C3)
  u8 flags;          // bit 0 : coquille etendue (m > q)
};
```

- `pop` : identifiants de sites, 4 octets pièce. Pour une coquille régulière, $S^{*} = U$ : aucun champ de support. Pour une coquille étendue, une table à part (boules étendues triées, quatre identifiants) porte $S^{*}$.
- Convention de rang : pour un nœud, le rang 0 est le niveau nul, celui des sites à l'ordre 1 (`types.hpp`) ; un nœud né ou créé par une boule porte le rang de cette boule plus un, comme en v10, si le catalogue numérote ses niveaux à partir de 0 (c'est le choix de `CONCEPTION_GENERATEUR.md` § 2.5).
- `rank_first[r]` : première boule du rang $r$ ; le niveau exact se recalcule depuis son $S^{*}$, il n'est stocké nulle part. `level_key[r]` : la clé binaire64 du niveau qui a servi au tri (règle F3), pour les dichotomies de rang (§ 6.1).
- Les sites viennent de `cloud` : coordonnées en ordre de Morton, poids. Une entrée dont un poids dépasse 1 est refusée (`multiplicity_unsupported`), comme l'exige `ARCHITECTURE.md` § 7.3.

Volume (calculé sur les comptes mesurés par L05 § 6.5, trame 02 : 4,63 et 8,10 identifiants par boule) : 30,5 octets par boule à $K = 5$ (43 Mo), 44,4 à $K = 10$ (243 Mo), contre 104 et 125 octets en v10. L'enregistrement est un tableau de structures parce que la tour y accède au hasard pendant l'étage G : une ligne de cache par boule consultée, contre quatre avec des tableaux séparés. `CONCEPTION_GENERATEUR.md` § 2.5 (lu à 08:39 UTC) publie les mêmes informations en tableaux séparés (`rank`, `ids_off`, `n_int`, `n_shell`, `qflags`, `ids`, `level_rep`, `ext`, 11 octets fixes par boule) : la tour construit alors cette vue dans son étage P (une passe en flux, estimée à 1 ms à $K = 5$), ou le catalogue l'écrit directement lors de son unique passe de remplissage ; à trancher à la synthèse.

**Deux pistes voisines, et ce qu'elles changent ici** (`PISTES_DE_RUPTURE.md`, lu à 08:37 UTC). (a) *Le générateur fournit la plus petite boule de chaque morceau* (R2.3) : **écartée sur mesure** par la conception voisine (la liste de la feuille ne certifie le recensement de la boule du morceau que pour 12,7 % des premiers pas hors semis à $K = 5$ et 34,6 % à $K = 10$, et jamais aux deux ordres les plus hauts). Le catalogue ne publie donc ni successeur ni plus petite boule de morceau, et la tour n'en attend pas. (b) *Catalogue stocké dans l'ordre d'émission, ordre canonique en permutation* (R2.2, à prototyper) : la tour s'y adapte sans changer d'algorithme. L'étage G peut parcourir les jonctions dans n'importe quel ordre, puisque chaque cible est rangée à l'indice canonique de son représentant ; seuls le noyau (jonctions par rang croissant) et la numérotation lisent l'ordre canonique, par la permutation. La décision se prend sur la mesure M3 (§ 11).

### 1.2 Ce qu'elle exige

- **(H1)** Sites distincts, coordonnées de $B$ bits.
- **(H2)** Le catalogue contient toute sphère critique avec $p + q_{\min} \leq K + 1$, chacune une fois, avec $I$ et $U$ exacts, dans l'ordre (niveau exact, $S^{*}$), les rangs étant denses et égaux si et seulement si les niveaux exacts le sont.
- **(H3)** Les niveaux se comparent par leurs rangs ; aucune comparaison de niveaux n'a lieu dans la construction des forêts.
- **(H4)** Chaque cellule de fenêtre non naissance fournit un représentant de $V_{<}(b, k)$ par morceau, pour une partition **exhaustive** des morceaux (correction de l'auditeur : un sous-échantillon de représentants ne suffit pas). C'est le cas ici : un représentant par morceau, ni plus ni moins (§ 2).

Sous ces hypothèses, le théorème E de L02 § 4.7 donne l'arbre de fusion exact. La tour est donc **exacte relativement au catalogue**, et le dit dans son statut. Deux moitiés ne se confondent pas : la complétude (H2) est le théorème du générateur ; l'exactitude de la forêt sachant (H2) est celui de la tour.

### 1.3 Ce qu'elle refuse d'elle-même, sans coût

| Refus | Où il se voit | Coût |
| --- | --- | --- |
| structure du catalogue (rangs non décroissants, $I$ et $U$ strictement croissants, $2 \leq q \leq 4$, $m \geq q$, $p + q \leq k_{\mathrm{cat}} + 1$, identifiants bornés, drapeau cohérent) | passe de comptage de l'étage P | lecture déjà faite |
| boule de fenêtre absente : une plus petite boule rencontrée, non saturée, dont l'ordre courant est dans la fenêtre, et que le catalogue ne contient pas | `resolve1`, branche hors catalogue | nul : $p$, $q$, $m$ y sont déjà calculés |
| recensement contradictoire : la même sphère, recensée par l'index, ne donne pas $(p, q, m)$ du catalogue | même branche, quand la coquille est étendue | nul |
| racine multiple à un ordre | fin du noyau : le nombre d'événements doit valoir le nombre de naissances moins un | nul |
| coquille de plus de 64 sites, ou budget de quotient dépassé | passe de comptage | nul |

Aucun de ces refus ne certifie la complétude : une jonction manquante dont l'absence ne casse ni une descente ni la racine reste invisible (L02 § 7.5 : 295 forêts fausses sur 3 062 retraits, en petite taille). La tour ne re-vérifie aucun théorème du générateur : le juge de recensement 1/32 de la v10 sort du chemin produit.

### 1.4 Euler et restriction : des portes, pas un mode

- **Portes d'échelle** (labels `scale8000`, `scale16000`, `scale32000`, `lidar`) : identité d'Euler $\chi_k = 1$ pour $k \leq K$ sur un catalogue construit à $K + 2$, restriction $\mathrm{cat}(K) = \mathrm{restrict}(\mathrm{cat}(K + 2))$, juge d'échantillon du recensement par force brute, ordre 1 contre l'arbre couvrant minimal. Elles jugent le générateur ; elles vivent dans les portes du catalogue.
- **Diagnostic nommé**, hors chronomètre : une sous-commande qui construit le catalogue à $K + 2$ et publie `euler_diagnostic=pass|fail` par ordre. Le coefficient d'une coquille étendue se calcule par énumération des parties jusqu'à 20 sites ; au-delà le diagnostic se déclare inapplicable pour cette boule.
- **Jamais « catalogue certifié ».** Le témoin de l'auditeur (cinq points du plan, $K = 1$ : un triangle qui remplit un trou et une paire qui fusionne au même niveau ; les omettre tous deux conserve la courbe d'Euler et retarde une fusion) devient une fixture qui interdit cette lecture.

### 1.5 Statut publié

`status=ok` signifie : forêts exactes relativement au catalogue reçu. Le résultat porte `completeness=relative_to_catalogue`, $k_{\mathrm{cat}}$, le nombre de boules étendues et la plus grande coquille, et les compteurs déterministes du § 3.8. Raisons nouvelles à demander au socle, chacune avec son émission et sa porte : `multiplicity_unsupported`, `shell_too_wide` (statut `unsupported_degeneracy`) ; `shell_quotient_budget`, `descent_step_budget` (`resource_exhausted`) ; `catalogue_structure`, `catalogue_missing_ball`, `census_mismatch`, `root_count`, `descent_not_decreasing`, `meb_not_certified` (`invariant_violated`). Les paramètres hors domaine (ordre demandé supérieur à $k_{\mathrm{cat}}$, plage d'ordres vide) rendent `parameter_out_of_range`, jamais `ok` sur une demande non servie. Le chemin « plage d'ordres $[k_{\min}, k_{\max}]$ » de la conception voisine (R3.1 : un seul ordre ne consulte que les boules dont la fenêtre le contient) ne demande aucun changement ici, les ordres étant indépendants à catalogue donné ; les verticales ne sont alors calculées qu'entre ordres construits. Il n'entre au produit qu'avec sa porte et son ablation (règle 6 de l'architecture).

---

## 2. Atlas des cellules et classification

### 2.1 Coquilles régulières : rien à stocker

Fenêtre d'une boule (théorème B de L02, corollaire) : $p + q - 1 \leq k \leq \min(K, p + m)$. Pour une coquille régulière ($m = q$) il y a au plus deux cellules, toutes deux analytiques :

- ordre $p + q - 1$ : **jonction** à $q$ morceaux, de représentants $F_u = I \cup U \setminus \lbrace u \rbrace$, $u \in U$ ;
- ordre $p + q$ (s'il est au plus $K$) : **naissance**, de population $P_b = I \cup U$.

On appelle **classe** d'une boule régulière le nombre $c = p + q$, entre 2 et $K + 1$. Les boules de classe $c$, dans l'ordre du catalogue, sont à la fois les jonctions de l'ordre $c - 1$ et les naissances de l'ordre $c$ (L01 § 6.3 : le catalogue à $K$ est la liste des naissances des ordres 2 à $K + 1$). L'atlas se réduit donc à :

- `class_ball[c][i]` : boules régulières de classe $c$, 4 octets par boule ;
- `cls_idx[b]` : position de la boule dans sa classe, 4 octets par boule ;
- `rep_off[k][i]` : préfixes des $q$ pour les représentants de l'ordre $k$ (4 octets par jonction).

Il n'y a ni tableau de genre ni tableau de mémo par cellule : la v10 en écrivait 16 octets par boule (12 d'atlas, 4 pour les verticales) et 5 par cellule (mesuré, L06 : 26,5 Mo d'atlas et 5,2 Mo à $K = 5$ sur la trame 00). La passe de comptage (parallèle, par morceaux fixes de boules, positions écrites par préfixes) produit en même temps les contrôles de structure du § 1.3 et les tailles de tous les tableaux des étages suivants : l'admission mémoire par formule (§ 7.1) se fait avant toute allocation.

Numérotation des naissances d'un ordre : à $k = 1$ les sites, dans l'ordre de `SiteIdx` ; sinon les cellules de naissance dans l'ordre des boules. Les naissances étendues (rares) s'intercalent à leur rang de boule : l'identifiant d'une naissance régulière est sa position de classe plus le nombre de naissances étendues qui la précèdent, lu par dichotomie dans une liste de quelques centaines d'entrées au plus sur LiDAR (227 à 444 boules étendues par trame, L06).

### 2.2 Coquilles étendues : quotient polynomial, ou refus avant calcul

**Rappel de l'objet** (L02 § 4.4, L01 proposition 2). Pour une boule de centre $c$ et $t = k - p$ avec $1 \leq t \leq m$ : les $t$-parties **séparables** de $U$ (celles dont l'enveloppe convexe fermée ne contient pas $c$) sont les sommets ; deux sommets sont reliés quand leur union est séparable ; les **morceaux** sont les composantes. Aucun sommet : naissance. Un morceau : cellule inerte. Au moins deux : jonction.

**La v10** énumère les $\binom{m}{t}$ parties puis les paires de parties séparables : 49 à 57 s pour 24 points cosphériques à $K = 5$, refus après 82 s à $K = 10$ (mesuré, L06 constat 06).

**Ensembles séparables maximaux.** Pour une direction $v$, soit $H(v) = \lbrace x \in U : \langle v, x - c \rangle > 0 \rbrace$. Une partie est séparable si et seulement si elle est contenue dans un $H(v)$ (Gordan). Les $H(v)$ maximaux sont ceux des cellules ouvertes de l'arrangement des $m$ grands cercles $\langle v, x - c \rangle = 0$ ; il y en a $O(m^{2})$. On les obtient sans construire l'arrangement, par ses sommets :

```text
separables_maximaux(U, c):                       # m <= 64 : un ensemble = un masque de 64 bits
  S <- {} ; fait <- {}                           # fait : paires deja couvertes par un plan traite
  pour chaque paire i < j hors de fait, de vecteurs u_i = x_i - c, u_j = x_j - c non paralleles :
    pour s dans {+1, -1} :                       # les deux sommets v0 = s (u_i x u_j) de ce plan
      P <- { x : s det(u_i, u_j, u_x) > 0 }      # strictement du cote de v0
      Z <- { x : det(u_i, u_j, u_x) = 0 }        # sur le plan de u_i et u_j (contient i et j)
      marquer dans fait toutes les paires de Z
      pour chaque l dans Z :                     # fenetres semi-ouvertes dans le plan, orientees par v0
        W_l <- { l } u { x dans Z : s (u_l x u_x) . (u_i x u_j) > 0 }
        ajouter P u W_l a S
  si S est vide (m = 2, paire antipodale) : S <- les deux singletons
  rendre S sans doublons
```

**Théorème T7** (prouvé à l'annexe A.7, contrôlé contre la définition brute). (i) Tout élément de $\mathcal{S}$ est séparable, et toute partie séparable de $U$ est contenue dans un élément de $\mathcal{S}$. (ii) Pour $t$ donné, soit $\mathcal{S}_t = \lbrace M \in \mathcal{S} : \lvert M \rvert \geq t \rbrace$, et relions $M$ et $M'$ quand $\lvert M \cap M' \rvert \geq t$. Les morceaux de la cellule sont en bijection avec les composantes de ce graphe : le morceau d'une composante est l'ensemble des $t$-parties contenues dans l'un de ses éléments. (iii) Il y a naissance si et seulement si $\mathcal{S}_t = \emptyset$. (iv) Les sites que la cellule apporte à la couverture sans appartenir à aucun sommet sont $U \setminus \bigcup \mathcal{S}_t$ (contribution du § 6.4).

- **Représentant d'un morceau** : $I$ et les $t$ plus petits `SiteIdx` de l'élément de la composante qui minimise ce $t$-uplet dans l'ordre lexicographique. Un par morceau, tous les morceaux : (H4) tient. Le choix n'influence aucune sortie (théorème D) ; il est fixé pour que les compteurs soient reproductibles.
- **Cas circulaire.** Si $U$ est coplanaire avec $c$, il n'y a qu'un plan : $P = \emptyset$, $Z = U$, et $\mathcal{S}$ est la famille des $m$ fenêtres angulaires semi-ouvertes $[\theta_l, \theta_l + \pi)$. C'est le quotient par fenêtres de `TOWER_v2` § 9.4, retrouvé comme cas particulier ; son coût est $O(m^{2})$ prédicats dans cette forme simple.
- **Prédicats exacts.** Deux signes seulement, tous deux linéaires dans le centre $c = a + N / D$ : $D \det(x_i - a, x_j - a, x_x - a) - N \cdot w$, où $w$ est la somme des trois produits vectoriels (135 bits au profil 18 bits : c'est `orient_center_wide` de la v10) ; et une composante de $D (x_l - a) \times (x_x - a) - N \times (x_x - x_l)$ (115 bits). Le second remplace le produit scalaire de deux produits vectoriels : les deux vecteurs sont parallèles, il suffit du signe d'une composante non nulle de $u_i \times u_j$ et de la même composante de $u_l \times u_x$. Budgets calculés par `num::Int<bits>` ; filtre F6 par palier d'étendue ; repli exact à l'égalité, qui est ici le cas fréquent.
- **Coût.** Au plus $m^{2}(m + 1)$ prédicats (chaque plan par le centre n'est traité qu'une fois), et $\lvert \mathcal{S} \rvert = O(m^{2})$ masques ; puis, par ordre de la fenêtre, $O(\lvert \mathcal{S}_t \rvert^{2})$ intersections de masques. Mesuré par le prototype Python (`preuves_tour/quotient_check2.py`) : 24 points cosphériques, 116 ensembles, 0,054 s de Python pour les 24 ordres ; 30 points cosphériques, 176 ensembles, 0,16 s ; cercles de 12, 16 et 24 points : 12, 16 et 24 ensembles, moins de 5 ms.
- **Refus a priori.** $m > 64$ : `shell_too_wide`, lu sur le champ `m` pendant la passe de comptage. Somme des $m^{3} + K m^{4} / 2$ sur les boules étendues au-delà d'un budget publié : `shell_quotient_budget`. Les deux se décident avant le premier prédicat. Sur les trames du contrat le chemin est marginal ($m \leq 5$, 204 à 865 boules étendues à $K + 2$, L02 § 7.2).
- **Rangement.** Une table par boule étendue : par ordre de la fenêtre, genre (naissance, jonction, inerte), représentants, masque de contribution. Une cellule étendue de fenêtre non naissance, **même inerte**, entre dans la liste des jonctions de son ordre avec ses représentants (un seul si elle est inerte) : elle reçoit ainsi une cible et un sommet comme toute jonction, ce qui sert aux descentes qui tombent sur elle, aux verticales et à la relation de couverture, sans cas particulier dans le noyau.

---

## 3. Résolution des représentants (étage G)

### 3.1 Les faits qui dimensionnent l'étage

Compteurs déterministes de la v10, trame 00, un fil (`preuves_l06_code_tour/mesures/run_l00_k*_t1_nopoints.json`, relus ici) :

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| représentants de jonction | 3 621 560 | 17 388 593 |
| dont ordre maximal | 1 350 122 (37 %) | 3 831 401 (22 %) |
| descentes qui finissent sur un semis | 3 111 203 (85,9 %) | 15 046 831 (86,5 %) |
| arrêts sur une cellule mémorisée ; sur une cellule de naissance | 307 738 ; 441 | 2 138 194 ; 1 390 |
| plus petites boules calculées | 1 084 548 | 7 656 889 |
| … dont la sphère est au catalogue | 825 366 (76 %) | 6 012 454 (79 %) |
| … hors catalogue (boules fermées par l'arbre, hors juge 1/32 ; compteur `L06_CB` de la copie instrumentée) | 260 759 | 1 653 027 |
| boules fermées par l'arbre à l'ordre maximal seul, juge compris, sur les plus petites boules de cet ordre | 202 682 sur 516 352 | 947 043 sur 1 965 797 |
| sauts | 275 230 | 2 049 772 |
| chaîne la plus longue (pas) | 8 | 12 |

Cycles de `resolve` en v10 (sonde `rdtsc` de L06, un fil, machine locale ; les parts seules sont fiables) : 5,0 milliards à $K = 5$ et 35,7 à $K = 10$, soit par unité : 180 et 150 cycles par sonde de semis, 1 310 et 1 750 par plus petite boule, 710 et 600 par pas pour la recherche de boule et le recensement lu, 4 600 et 4 700 par boule fermée, 680 et 960 par sélection de saut. Sur G4 l'étage prend 22,2 à 28,6 ms à $K = 5$ et 179 à 243 ms à $K = 10$ à 48 fils, et 47,2 ms à 24 fils pour la trame 02 à $K = 5$ (reçu de la session 4) : le second fil d'un cœur rapporte ×1,74, l'étage est borné par la latence mémoire.

Deux précisions mesurées par la conception voisine sur un quart de trame (`PISTES_DE_RUPTURE.md` § 3.1). (a) Le compteur de semis compte les descentes qui **finissent** sur un semis ; la part des représentants qui **sont** un semis est de 72 % à $K = 5$ et 70 % à $K = 10$ : un représentant sur quatre demande au moins une plus petite boule. Premier pas d'un représentant hors semis : cellule de jonction du catalogue au même ordre dans 67,5 % et 61,4 % des cas (c'est le cas de l'arrêt sur cellule du § 3.2), saut dans 23,9 % et 23,4 %, boule inerte dans 8,5 % et 15,1 %. (b) La descente de la v10 calcule 8 % de plus petites boules de plus que le minimum de sa règle ; l'écart est celui des cellules traversées avant d'être mémorisées, que le § 3.2 supprime. **Le gain de l'étage est donc dans le coût du pas, pas dans le nombre de pas.**

Les sphères hors catalogue se concentrent aux deux derniers ordres (boules fermées par l'arbre à $K = 10$ : 947 043 à l'ordre 10, 429 733 à l'ordre 9, moins de 220 000 à chaque ordre inférieur ; à l'ordre 5, 202 682 quand le catalogue est bâti à $K = 5$ et 34 528 quand il l'est à $K = 10$). C'est donc la requête d'index qui pèse à grand $K$, et le certificat par le catalogue qui pèse partout ailleurs.

### 3.2 `resolve1` : fonction pure, politique fixée, arrêt à la première cellule

```text
resolve1(k, F) -> cible            # F : k sites tries. cible : naissance (NodeIdx) ou cellule (jonction j de l'ordre k)
  si k = 1 : rendre le site
  repeter au plus kStepCap fois :
    v <- semis[k].find(F) ; si v existe : rendre naissance v
    S <- proposition_meb(F)                              # flottant, ne decide rien (3.4)
    b <- supports.find(S)                                # cle verifiee sur S* de la boule
    si b existe et F est inclus dans P_b :               # certificat combinatoire (lemme T1)
        (p, q, m) <- BallRec(b)
        si la boule precedente de la chaine est au catalogue : exiger rank(b) < son rang
    sinon :
        (c, S) <- meb_exacte(F, S)                       # certificat geometrique exact, sinon Welzl exact
        R <- index.census_upto(c, k)                     # 3.5
        si R sature (au moins k sites strictement interieurs) : F <- R.plus_proches ; continuer   # saut
        (p, I, U) <- R ; m <- |U|
        (q, S*) <- (U = S) ? (|S|, S) : support_canonique(U, c)
        b <- (U = S) ? absent : supports.find(S*)
        si b existe : exiger (p, q, m) = BallRec(b), sinon refus census_mismatch
    si p >= k : F <- k sites de I ; continuer                              # saut depuis le catalogue
    si k <= p + q - 2 : F <- I u (les k - p plus petits sites de U) ; continuer   # boule inerte, sous la fenetre
    si b absent : refus catalogue_missing_ball                             # en fenetre : (H2) viole
    rendre naissance(b, k) si la cellule est une naissance, sinon cellule(b, k)
  refus descent_step_budget
```

**Politique fixée** (elle ne change aucune sortie, seulement le travail) : saut hors catalogue vers les $k$ plus proches du centre (clé exacte, puis `SiteIdx`) ; saut depuis le catalogue vers les $k$ plus proches parmi $I$ (variante à mesurer : les $k$ plus petits `SiteIdx`, sans aucune géométrie) ; sous la fenêtre, les $t$ plus petits `SiteIdx` de $U$ ; sur une cellule de fenêtre non naissance, **arrêt**.

**Pourquoi c'est juste.** Chaque pas est un pas valide du théorème D de L02 : une $k$-partie de $I$ quand $p \geq k$ ; $I \cup A$ avec $A$ séparable sinon, et toute partie de $U$ de moins de $q_{\min}$ sites est séparable (lemme 3 de L02), ce qui couvre le cas sous la fenêtre où $t \leq q - 2$. Le niveau décroît strictement à chaque pas. L'arrêt sur une cellule $(b', k)$ non naissance rend une cible dont la valeur finale est celle du premier représentant de cette cellule : c'est le pas « $I' \cup A$, $A$ dans le premier morceau », simplement différé (lemme T3, annexe A.3).

**Phase des pointeurs.** Soit $T[r]$ la cible du représentant $r$. Si $T[r]$ est une cellule $(b', k)$, sa valeur finale est $T$ du premier représentant de la jonction de $b'$, et ainsi de suite ; la chaîne descend strictement en rang, donc elle est acyclique, et elle ne lit que des jonctions de rang inférieur, déjà résolues quand le noyau en a besoin (§ 4.4). Le suivi lit le tableau $T$ sans l'écrire.

**Ce que cela remplace.** La v10 mémorise par cellule, dans un tableau partagé écrit par des atomiques, la naissance trouvée par la première descente qui la traverse ; le travail dépend de l'ordre d'arrivée des fils, et la descente de la cellule est faite deux fois (une fois par le premier visiteur, une fois comme représentant de sa propre jonction : c'est l'écart de 8 % du § 3.1). Ici chaque cellule est résolue une fois, par son propre représentant ; il n'y a plus ni écriture partagée ni compteur dépendant des fils.

**Date d'usage d'une cible** (exigence de l'auditeur). La classe du terminal n'est unique qu'aux coupes $a \geq \beta(F)$ ; sur $\lbrace 0, 2, 4 \rbrace$ à $K = 2$, la paire extrême peut descendre vers l'une ou l'autre naissance. Une cible de représentant n'est donc lue que : par le noyau, à la coupe ouverte du niveau de sa jonction, strictement supérieur à $\beta(F)$ ; par les verticales et les attaches, à une coupe fermée de niveau au moins $\beta(F)$. Le pointeur d'une cellule $(b', k)$ ne vaut qu'à partir du niveau de $b'$ ; il n'identifie jamais lequel de ses morceaux est concerné, et aucun usage ne le lui demande.

### 3.3 Semis

Pour chaque ordre $k \geq 2$, une table plate (adressage ouvert) : population triée d'une naissance de $k$ sites → nœud de naissance. Y entrent toutes les naissances régulières (leur population a toujours $k$ sites) et les naissances étendues de population $k$ (théorème B, cas 2 : la v10 les en excluait sans raison).

- **Empreinte additive** : $h(F) = \mathrm{mix}(\sum_{x \in F} g(x) \bmod 2^{64})$ avec $g$ un mélange du `SiteIdx`. L'empreinte d'un représentant $P_b \setminus \lbrace u \rbrace$ s'obtient de celle de $P_b$ par une soustraction. L'empreinte n'adresse que ; **toute égalité est vérifiée sur les identifiants**.
- **Disposition de base** (celle de la v10, éprouvée) : entrée de 8 octets (étiquette de 32 bits, nœud), populations recopiées par nœud ($4k$ octets) ; insertion concurrente sans verrou dont le résultat ne dépend pas de l'ordre d'insertion. **Variante à mesurer** : population dans l'entrée (un défaut de cache par arrêt au lieu de deux, mémoire ×1,17 à ×1,34).
- Les sondes sont préchargées par lots de 32 représentants, comme en v10.

« Ou mieux » : j'ai cherché une structure sans table. Aucune ne tient : reconnaître qu'une $k$-partie est la population d'une naissance demande soit sa plus petite boule (c'est le chemin lent qu'on veut éviter), soit une égalité d'ensembles, donc une table. Une jointure triée sur l'empreinte coûte autant de défauts de cache à la vérification. Le semis reste donc une table ; son poids dans G passe de 15 % à environ 20 % d'un étage 2,5 fois plus court (estimé).

### 3.4 Plus petite boule : proposer en flottant, certifier par le catalogue

**Lemme T1 (certificat combinatoire).** Soit $b$ une boule du catalogue, $S^{*}$ son support canonique, $P_b = I \cup U$. Si $S^{*} \subseteq F \subseteq P_b$, alors $b$ est la plus petite boule englobante de $F$. *Preuve.* $c \in \mathrm{relint}\,\mathrm{conv}(S^{*})$ et $S^{*}$ est sur la sphère, donc $c \in \mathrm{conv}(F \cap \partial b)$ ; une boule fermée qui contient $F$ et dont le centre est dans l'enveloppe des points de $F$ sur son bord est sa plus petite boule (lemme 1 de L02). $\square$

La décision « la plus petite boule de $F$ est $b$ » se réduit donc à deux égalités d'entiers : la clé $S$ proposée est le $S^{*}$ d'une boule (recherche vérifiée), et $F \subseteq P_b$ (fusion de deux listes triées d'au plus 12 et 16 identifiants). Aucune arithmétique exacte, aucun centre, aucun niveau. L'exactitude de $I$ et $U$ est celle du catalogue, que la tour invoque déjà. La v10 calculait pour ces mêmes pas un centre exact, des orientations et des côtés de sphère en 128 bits, puis relisait le recensement au catalogue ; ici, 76 à 79 % des plus petites boules n'ont plus de géométrie exacte (parts mesurées du § 3.1).

- **Proposition** (`proposition_meb`) : heuristique flottante, sans rôle dans aucune décision. Deux candidates, à départager sur G4 (§ 11, M4) : le port de l'heuristique de la v10 (paire éloignée, puis ajout du pire point ; 0 repli sur 7,66 millions de boules, L06) ; une marche depuis une sphère de départ connue (boule mère privée d'un point de support pour un représentant de jonction, sphère précédente pour un premier représentant : le centre glisse vers le centre circonscrit des points de bord restants jusqu'au premier point qui touche, à la manière de Fischer, Gärtner et Kutz). La seconde part de l'information que le texte de la tâche suggère ; son gain n'est pas mesuré.
- **Certificat géométrique exact**, seulement quand la recherche échoue ou que l'inclusion est fausse : centre exact depuis $S$ (forme typée à 2, 3 ou 4 sites), barycentriques de signe positif au sens large, côtés de sphère des autres sites de $F$ négatifs ou nuls. S'il échoue : Welzl exact (jamais atteint sur LiDAR en v10 ; une porte l'atteint par harnais et par fixtures adverses).
- **Coquille étendue à plusieurs supports minimaux** : si la proposition rend un support valide autre que $S^{*}$, la recherche échoue, le chemin exact recense la sphère, calcule $S^{*}$ et retrouve la boule. Rare (0,02 à 0,04 % des boules), toujours juste.

### 3.5 Recensement d'une sphère hors catalogue : requête bornée

`index.census_upto(sphère, k)` rend l'un de deux résultats, tous deux exacts :

- **saturé** : au moins $k$ sites sont strictement intérieurs ; la requête rend les $k$ plus proches du centre (clé exacte, puis `SiteIdx`) et rien d'autre ;
- **complet** : moins de $k$ sites sont strictement intérieurs ; la requête rend tout $I$ et toute la coquille $U$.

La requête parcourt l'index avec un rayon qui part de celui de la sphère et se resserre dès que $k$ candidats intérieurs sont connus. Sa sortie est bornée par $k$ et par la taille de la coquille, qui est intrinsèque. La v10 recensait et triait la boule fermée entière : 10 et 16 sites en moyenne, mais 1 201 et 1 258 au plus sur la trame 00, et la moitié du nuage sur une famille à contraste (L06 constat 05). Ce qu'il faut ensuite :

- saturé : saut, sans autre calcul (la sélection est faite) ;
- complet et coquille égale au support : $q = \lvert S \rvert$, aucune seconde recherche (la v10 en faisait 62 056 à $K = 5$ et 397 450 à $K = 10$, pour 1 et 37 succès) ;
- complet et coquille étendue : support canonique, seconde recherche, contrôle croisé du recensement.

### 3.6 Index des sites : arbre k-d, ou arbre de boîtes du générateur ?

**Décision : ni l'un ni l'autre tel quel.** Une hiérarchie implicite de boîtes entières sur l'ordre de Morton des sites, indépendante du catalogue.

*Pourquoi pas l'arbre de boîtes du générateur.* L'idée est séduisante : la liste d'une feuille est $K$-certifiée, donc contient $N_K(c)$ pour tout centre $c$ de sa boîte, et la requête devient un recensement local de 16 à 24 sites. Quatre faits s'y opposent.

1. **Il n'est pas total** (prouvé). Depuis J2c, chaque nœud réduit sa boîte à l'enveloppe de sa liste filtrée. Un centre $c$ est conservé à tous les niveaux si sa sphère a $p \leq K - 1$ (les sites de la coquille ont alors moins de $K$ dominateurs et restent dans les listes, donc $c \in \mathrm{conv}(U)$ reste dans l'enveloppe : c'est le lemme d'ajustement de L01). Mais un centre dont la boule a $p \geq K$ peut tomber hors de toutes les feuilles : exemple sur une droite, deux sites de support en $\pm 10$ et $K$ sites entre 1 et 2, centre 0 hors de l'enveloppe $[1, 2]$ de ses $K$ plus proches. Or ce sont précisément les sphères d'où l'on saute, et à l'ordre maximal tous les sauts sont dans ce cas. Il faudrait donc un second index pour elles.
2. **Il faut localiser le centre, et il est rarement dans la feuille de la boule mère** (mesuré, L05 `feuilles_histogrammes_et_duplication.txt`, trame 02 à $K = 5$) : le plus long côté d'une boîte de feuille est sous 128 mm pour 53 % des feuilles, alors que l'étendue de sa liste dépasse 512 mm pour 60 % d'entre elles. Le centre de la plus petite boule d'un morceau, qui s'écarte du centre de la boule mère d'une fraction du rayon, tombe dans une autre feuille : il faut descendre l'arbre depuis la racine.
3. **Cet arbre est gros** (mesuré, L05 § 6.1) : 637 000 à 782 000 nœuds et 284 000 à 353 000 feuilles à $K = 5$ pour 35 000 à 46 000 sites, 4,4 millions d'identifiants de listes (trame 02) ; environ 40 Mo à conserver à $K = 5$ et 75 Mo à $K = 10$, contre 0,7 Mo pour l'index proposé, qui tient dans le cache L2 d'un cœur. Une descente de 20 à 30 niveaux dans 40 Mo coûte plus de défauts de cache qu'elle n'économise de calcul.
4. **Il lie la tour aux listes qu'elle doit pouvoir contredire.** Un site omis par le filtre du générateur serait omis de la même façon par le recensement de la tour (critique M2 de `TOWER_v2`). Avec un index indépendant, le contrôle croisé du § 1.3 garde un sens.

Le recensement local reste disponible comme **variante à mesurer** pour les seules sphères avec $p \leq K - 1$, si le catalogue garde ses feuilles pour une autre raison (§ 11, M5).

*Pourquoi pas le k-d de la v10.* Sa construction est séquentielle avec sélection de médiane (6,7 à 8,0 ms en local, 6,5 à 8,3 ms de préparation sur G4), ses requêtes reposent sur une marge absolue fixe valable à 18 bits seulement et dans la boîte des sites (L04 C04), et elles coûtent 4 600 cycles en moyenne.

**Structure.** Les sites sont déjà triés par clé de Morton. Niveau 0 : blocs de 8 sites consécutifs, boîte entière serrée par bloc. Niveaux supérieurs : groupes de 8 nœuds consécutifs, jusqu'à la racine. Tableaux par axe (bornes basses, bornes hautes), aucune table de permutation, aucun pointeur : 3,4 octets par site pour les boîtes, 12 pour les coordonnées. Construction : une passe séquentielle, $O(n)$, sans tri (estimée sous 0,3 ms pour 46 000 sites) ; elle ne demande pas `sched`, ce qui respecte la table des modules.

**Requêtes.**

- *Centre entier* (un site : attaches `core`) : distances carrées et distances aux boîtes en entiers exacts de 64 bits ; aucun flottant.
- *Centre rationnel* $c = a + N / D$ (formes typées à 2, 3 ou 4 sites) : l'**élagage** d'une boîte emploie une borne inférieure flottante **certifiée à sens unique** de la distance du centre à la boîte, comparée à une borne supérieure certifiée du rayon courant ; une boîte non élaguée est simplement visitée, donc l'élagage ne décide rien et n'a pas de repli. Les **décisions** sur les sites (intérieur, coquille, ordre des plus proches) se prennent sur la clé exacte $D \lVert z - a \rVert^{2} - 2 N \cdot (z - a)$, précédée du filtre de signe F6 quand il conclut. Aucune marge fixe : la borne se calcule depuis $B$ et l'erreur certifiée du centre approché (PO-I1).

Coût attendu d'une requête (estimé : 10 à 15 tests de nœud vectorisables sur 8 enfants, 16 à 24 sites examinés, clé exacte à 30 cycles pièce d'après L05 § 6.2) : 1 000 à 1 500 cycles, contre 4 600 mesurés en v10. À mesurer sur les sphères réelles vidées (§ 11, M5).

### 3.7 Ce qui sort du chemin produit, et ce qui y reste

| Contrôle de la v10 | Sort | Raison | Remplacé par |
| --- | --- | --- | --- |
| juge de recensement 1 boule sur 32 | oui | re-vérifie le théorème C du générateur ; 9 à 10 % des requêtes d'arbre | juge d'échantillon dans les portes du catalogue |
| seconde recherche du même support | oui | 62 056 et 397 450 recherches perdues | le cas « coquille égale au support » ne cherche plus |
| garde exacte de décroissance à chaque pas | oui | théorème D | garde gratuite : rangs décroissants entre deux boules du catalogue consécutives ; plafond de pas `kStepCap` (refus typé) ; décroissance complète jugée dans les portes |
| naturalité complète des verticales | oui | théorème F | porte (§ 8) |
| une racine par ordre | reste | détecte un catalogue incomplet | — |
| boule de fenêtre absente | **ajouté** | gratuit, demandé par l'auditeur | — |

### 3.8 Compteurs publiés

Par ordre, tous déterministes et identiques à 1 et à 48 fils (c'est une porte) : représentants ; arrêts au semis ; propositions de plus petite boule ; certificats combinatoires, géométriques, replis ; recherches de support (succès, échecs) ; requêtes d'index (saturées, complètes), sites examinés (moyenne, maximum) ; sauts (catalogue, hors catalogue) ; premiers représentants sous la fenêtre ; arrêts sur cellule ; histogramme des longueurs de chaîne ; longueur des suivis de pointeurs. Ils sont dans un grand livre à part, jamais dans l'objet publié.

---

## 4. Forêt d'un ordre, sans lots (étages T et M)

### 4.1 Noyau : la seule partie séquentielle

Entrée d'un ordre $k$ : ses naissances (feuilles), ses jonctions dans l'ordre du catalogue (donc par rang croissant), et pour chaque représentant sa naissance `rep_node` (cible suivie jusqu'au bout, § 3.2).

```text
noyau(k):                                  # un fil ; aucune allocation ; aucun lot
  cellule[l] = { up = l, taille = 1, dernier = aucun, minfeuille = l }       # 16 octets par naissance
  pour chaque jonction j, dans l'ordre :   # prechargement des cellules des representants 16 positions plus loin
    x <- find(rep_node[premier representant de j])                           # demi-compression
    pour chaque autre representant r de j :
      y <- find(rep_node[r]) ; si x = y : continuer                          # racines dedupliquees d'elles-memes
      e <- nouvel evenement { rang(j), sommet(x), sommet(y), min(minfeuille[x], minfeuille[y]), survivant }
      survivant s = la plus grande des deux composantes, perdant l = l'autre  # union par TAILLE
      up[l] <- s ; attache[l] <- (s, rang(j)) ; taille, minfeuille, dernier[s] <- e ; x <- s
    jtop[j] <- sommet(x)                   # sommet de la composante juste apres la jonction
  exiger : nombre d'evenements = nombre de naissances - 1, sinon refus root_count
```

`sommet(x)` est le dernier événement dont la racine $x$ est sortie survivante, ou la feuille $x$ si elle n'a jamais fusionné. Le suivi des pointeurs (§ 3.2) se fait à la lecture de `rep_node` par le noyau : une lecture de plus pour les représentants arrêtés sur une cellule (11 % de ceux de l'ordre 5 de la trame 00 finissaient sur une cellule mémorisée en v10). Le noyau n'écrit **aucun nœud** : il écrit des événements binaires (20 octets : rang, deux opérandes, plus petite feuille, survivant), l'attache de chaque perdant (8 octets) et un sommet par jonction (4 octets). Il ne peut donc pas binariser une multifusion : les nœuds naissent à l'étage M.

L'union par taille borne la profondeur de la forêt d'attache par $\log_2$ du nombre de naissances (§ 4.3). La plus petite feuille de la composante est tenue à part : c'est elle qui donne la clé canonique des nœuds.

### 4.2 Matérialisation parallèle : contraction des plateaux

Les événements sont rangés par rang croissant ; ceux d'un même rang sont contigus. Deux événements de même rang sont **liés** quand l'un a pour opérande le sommet produit par l'autre.

**Théorème T4** (annexe A.4 ; c'est la règle cartésienne du théorème J de L02, dans la forme des événements). Les multifusions de l'ordre au rang $t$ sont exactement les classes d'événements de rang $t$ pour la clôture transitive de « liés » ; les enfants d'une multifusion sont les opérandes de ses événements qui sont des feuilles ou des sommets de rang strictement inférieur. L'ordre des unions à l'intérieur d'un rang n'intervient pas.

Algorithme, en boucles parallèles sur des tranches de la liste d'événements alignées sur les changements de rang (une tranche ne coupe jamais un rang) :

1. par groupe de rang : classes d'événements liés (mesuré sur la trame 00 : au plus 4 événements par groupe aux ordres 4, 5 et 10, 6 à l'ordre 3, 14 à l'ordre 2, 24 à l'ordre 1 ; un groupe géant, sur une grille, est traité par un union-find local dans une seule tranche) ; clé de chaque classe : (rang, plus petite feuille) ; compte des classes par tranche ;
2. préfixes : identifiant du nœud = nombre de naissances + position de la classe dans l'ordre (rang, plus petite feuille) ; `nid[e]` par événement ;
3. `rank[v]`, `minleaf[v]` ; `parent[enfant] = v` (chaque enfant a un seul parent : écritures disjointes) ; CSR des enfants par comptage, préfixes, remplissage, enfants triés par identifiant ;
4. index d'historique (§ 4.3).

La numérotation (naissances dans l'ordre des boules, fusions par rang puis plus petite naissance du sous-arbre, enfants triés) est **celle de la v10** : le dump au format v10 en sort sans renumérotation.

### 4.3 Historique d'attache : les requêtes d'ancêtre sans pointeurs de saut

La seule requête dont la tour et les attaches ont besoin est : *le nœud vivant à la coupe fermée de rang $r$ dont la composante contient la naissance $\ell$*.

```text
component_at(l, r):
  x <- l ; tant que attache[x] existe et attache[x].rang <= r : x <- attache[x].survivant
  e <- dernier evenement de survivant x et de rang <= r      # dichotomie dans la liste des evenements de x
  rendre nid[e], ou la naissance x si e n'existe pas
```

**Lemme T5** (annexe A.5). Les rangs croissent le long d'un chemin d'attache ; la boucle s'arrête donc sur la racine de la composante de $\ell$ dans l'état « tous les événements de rang au plus $r$ traités », en au plus $\log_2$ (nombre de naissances) pas ; le dernier événement de cette racine de rang au plus $r$ appartient au nœud vivant à la coupe fermée.

L'index se compose de `attache` (8 octets par naissance) et de la liste des événements par survivant (CSR construit par un tri par comptage parallèle : 4 octets par naissance, 8 par événement). Il n'est pas canonique et n'entre dans aucune empreinte. Il remplace les pointeurs de saut de la v10, dont la construction est une passe séquentielle par ordre sur tous les nœuds, placée sur le chemin critique.

### 4.4 Ordonnancement : un propriétaire par ordre, recouvert par G

Une seule région parallèle porte les étages G et T :

- les $K$ premières tâches sont les **propriétaires** : le propriétaire de l'ordre $k$ exécute `noyau(k)` ;
- les autres fils sont des **aides** : ils prennent des morceaux de représentants (32 à 64 jonctions) dans une file ordonnée par $k$ décroissant, calculent les cibles $T$, marquent le morceau fait ;
- un propriétaire qui attend ses représentants **aide** lui aussi ; à un seul fil, la région dégénère en « tout G, puis les noyaux », sans surcoût (le prototype privé P2 de la v10 mesure qu'un flux sans ce repli coûte ×2,1 à ×2,5 au noyau à un fil).

Deux régimes, le second étant un incrément mesuré du premier :

- **O1, par ordre** : le noyau de l'ordre $k$ démarre quand tous les représentants de l'ordre $k$ sont résolus. Avec l'ordre maximal en tête de file, son noyau tourne pendant que les aides résolvent les ordres inférieurs (63 % des représentants à $K = 5$, 78 % à $K = 10$).
- **O2, en flux** : le noyau consomme les morceaux de son ordre à mesure qu'ils sont faits. Il lit un représentant quand tous les morceaux de rang inférieur ou égal de son ordre sont faits, ce qui garantit aussi que les cibles suivies (qui pointent vers des jonctions de rang strictement inférieur) sont prêtes. Sans lots, il n'y a pas de plateau à cheval sur le filigrane : c'est le défaut que le prototype P2 avait dû corriger.

**Ce que cela exige de l'ordonnanceur** : une boucle parallèle à réclamation dynamique dans l'ordre des indices (les propriétaires d'abord), une attente courte par relâchement du fil, la capture déterministe du premier refus par ordinal, aucun parallélisme imbriqué. Aucune primitive de tâche générale n'est nécessaire. Les boucles parallèles de l'étage M et des verticales sont lancées par l'orchestrateur après la région, jamais depuis un propriétaire.

**Déterminisme.** Les cibles sont des fonctions pures ; chaque morceau écrit ses propres cases ; le noyau suit l'ordre des jonctions ; la matérialisation écrit à des positions fixées par préfixes. Aucune sortie ni aucun compteur ne dépend de l'entrelacement.

### 4.5 Mesures

Prototype `preuves_tour/noyau_v11.cpp` (cette conception), sur les entrées réelles du Kruskal de la v10 vidées par l'audit L06 (trame 00 ; ordres 1 à 5 du calcul à $K = 5$, ordre 10 du calcul à $K = 10$). Minimum de 7 passes entrelacées, machine à charge 19 : **les rapports seuls sont à lire**.

| Ordre | Naissances | Représentants | Fusions | Forêt et numérotation égales à la v10 | Kruskal par lots (ms) | Noyau v11 complet (ms) | Rapport | Sauts d'attache (moyenne ; max) | Sondes de dichotomie (moyenne) |
| ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- | ---: |
| 1 | 39 885 | 202 178 | 39 796 | oui | 6,8 | 2,5 | 2,76 | 1,73 ; 8 | 4,5 |
| 2 | 101 089 | 410 825 | 77 038 | oui | 15,1 | 6,1 | 2,49 | 1,58 ; 8 | 7,2 |
| 3 | 166 228 | 673 223 | 119 682 | oui | 29,4 | 10,1 | 2,90 | 1,49 ; 8 | 8,5 |
| 4 | 249 493 | 985 212 | 172 168 | oui | 35,7 | 13,4 | 2,66 | 1,47 ; 8 | 9,5 |
| 5 | 341 081 | 1 350 122 | 235 290 | oui | 47,6 | 18,2 | 2,61 | 1,44 ; 8 | 10,2 |
| 10 | 979 350 | 3 831 401 | 659 223 | oui | 140,9 | 54,2 | 2,60 | 1,33 ; 8 | 12,8 |

« Noyau v11 complet » comprend tout ce que le § 4.1 écrit (événements, attaches, sommets par jonction, préchargement). « Égales » : tableaux `rank` et `parent` identiques élément par élément à ceux du Kruskal par lots. Les images de toutes les jonctions calculées de deux façons (§ 5) concordent : 2 444 942 contrôles, 0 écart. Les rapports recoupent ceux du banc de L06 (2,6 à 3,2 pour un noyau nu).

Transposition à G4 (estimée) : le reçu de la session 4 donne 20,8 à 31,6 ms pour le Kruskal par lots de l'ordre 5 (pointeurs de saut compris) et 52,3 à 90,3 ms pour l'ordre 10 ; divisés par 2,6, cela fait **8 à 12 ms** et **20 à 35 ms** de noyau séquentiel pour l'ordre maximal. La matérialisation séquentielle naïve du prototype coûte 34 ms en local pour l'ordre 5 et 93 ms pour les cinq ordres ; en boucles parallèles elle est estimée à 1,5 à 3 ms à $K = 5$ sur G4.

---

## 5. Verticales (étage V)

Objet (théorème F de L02) : pour chaque nœud $v$ d'ordre $k \geq 2$, `lower[v]` est le nœud de l'ordre $k - 1$ vivant à la coupe **fermée** du niveau de $v$ dont la composante contient celle de $v$.

**Naissances : $O(1)$, sans descente ni requête.** Une naissance régulière de l'ordre $k$ est portée par une boule de classe $k$, qui est aussi une jonction de l'ordre $k - 1$ (§ 2.1). Toute $(k-1)$-partie de $P_b$ convient (théorème F (i)), en particulier un représentant de cette jonction. Le noyau de l'ordre $k - 1$ a laissé `jtop[j]`, le sommet de la composante juste après la jonction.

**Lemme T6** (annexe A.6). Soit $w$ le nœud de `jtop[j]` et $r$ le rang de la boule. Le nœud vivant à la coupe fermée $r$ est le parent de $w$ si ce parent a le rang $r$, et $w$ sinon. Une seule remontée conditionnelle suffit : après la jonction, tout événement a un rang au moins $r$, et deux nœuds emboîtés ont des rangs strictement croissants.

Les naissances de l'ordre $k$ et les jonctions de l'ordre $k - 1$ sont la même liste de boules, dans le même ordre : la passe est une lecture en flux de deux tableaux, parallèle. Une naissance étendue lit de même la cellule de sa boule à l'ordre $k - 1$, qui est dans la fenêtre (une naissance à $k$ exige $k - p \geq q_{\min}$) : nœud de naissance si cette cellule est une naissance, `jtop` sinon.

**Fusions : une requête indépendante par nœud.** `lower[v] = component_at(L(v), rang(v))` dans l'ordre $k - 1$, où $L(v)$ est une naissance de l'ordre $k - 1$ située sous l'image de la plus petite naissance de $v$ : le `rep_node` du premier représentant de la jonction de sa boule. Toutes les requêtes sont indépendantes : une boucle parallèle à plat sur les fusions de tous les ordres. La v10 traitait les fusions dans l'ordre de création, un ordre par tâche (12,5 à 18,7 ms à $K = 5$ pour l'ordre maximal).

**Naturalité** (tous les enfants d'une fusion ont la même image remontée) : c'est le théorème F (ii). Elle sort du chemin produit et devient un juge de porte qui relit `lower` sur tous les nœuds, avec les mutants `m_vopen` et `m_vnoclimb` de L02.

Coût estimé à 48 fils sur G4 : 2 à 4 ms à $K = 5$ (trame 02, ordres 2 à 5 : 0,94 million de naissances en lecture en flux ; 0,65 million de requêtes, supposées à 100 à 150 ns d'après les 1,3 à 1,7 saut et les 4,5 à 12,8 sondes mesurés au § 4.5), 10 à 20 ms à $K = 10$ (4,42 millions de naissances et 2,96 millions de fusions).

---

## 6. Attaches des points (module `points`)

Le module lit la tour, l'index et le catalogue. Il publie trois choses, séparées : l'entrée `core`, l'entrée `cover` **comme ensemble**, et des conventions nommées.

### 6.1 `core`

Pour chaque site $x$ et chaque ordre $k$ : $D_k(x)$, carré de la distance de $x$ à son $k$-ième plus proche site, $x$ compris (entier exact), et le nœud vivant à la coupe fermée $D_k(x)$ dont la composante contient $x$.

- Une requête entière `nearest(x, K)` par site donne les $K$ plus proches dans l'ordre (distance, `SiteIdx`) ; le préfixe de longueur $k$ sert à l'ordre $k$. La composante ne dépend pas du départage des ex æquo (énoncé C1 de L03 : toutes les $k$-parties de la boule fermée ont $x$ pour témoin).
- Nœud : `component_at(Min(resolve1(k, N_k(x))), pos)`, où `pos` est le nombre de niveaux du catalogue inférieurs ou égaux à $D_k(x)$.
- `pos` par dichotomie sur `level_key` : la comparaison d'une clé à l'entier $D_k(x)$ suit la règle F4 ; quand elle ne conclut pas, la comparaison exacte du niveau (recalculé depuis $S^{*}$ de `rank_first`) décide. L'ordre exact des rangs rend la dichotomie valide même si deux clés voisines sont inversées.
- Champs : `core_level` (u64), `core_pos` (`LevelRank`), `core_eq` (1 si $D_k(x)$ est exactement un niveau du catalogue), `core_node`.

### 6.2 `cover` : l'objet est un ensemble

$A_k(x) = \alpha_k(x)^{2}$ est le plus petit rayon carré d'une boule fermée contenant $x$ et au moins $k$ sites ; $E_k(x)$ est l'ensemble des nœuds de l'ordre $k$ vivants à la coupe fermée $A_k(x)$ dont la composante couvre $x$ à ce niveau.

**Proposition T8** (annexe A.8 ; c'est P1 de L03 § 12.2 et son lemme de couverture). Une boule du catalogue est un **témoin** de l'ordre $k$ si $p + q_{\min} \leq k \leq p + m$. Alors $A_k(x)$ est le plus petit niveau d'un témoin dont la population contient $x$, et $E_k(x)$ est l'ensemble des nœuds, à la coupe fermée, des témoins de ce niveau qui contiennent $x$. Pour une coquille régulière, les témoins de l'ordre $k$ sont exactement les boules de classe $k$, c'est-à-dire **les naissances de l'ordre $k$**, et le nœud d'un témoin est son nœud de naissance (une naissance n'est jamais absorbée à son propre niveau).

Le calcul ne demande donc **aucune descente** pour les coquilles régulières :

1. par ordre, une passe parallèle sur les naissances : pour chaque site de $P_b$, minimum atomique du rang de la boule (le résultat ne dépend pas de l'ordre des fils) ;
2. une seconde passe : chaque couple (site, naissance) dont le rang égale ce minimum entre dans $E_k(x)$ ; compte, préfixes, remplissage ;
3. les cellules étendues de fenêtre avec $p + q_{\min} \leq k$ s'ajoutent avec le nœud de leur cellule à la coupe fermée (naissance, ou `jtop` remonté d'un cran comme au § 5).

Volume : 3,7 millions d'incidences à $K = 5$ et 33 millions à $K = 10$ sur la trame 02 (somme des $k$ fois le nombre de naissances de l'ordre $k$, calculée sur la table de L01 § 6.3) ; estimé à 1 ms et 5 ms à 48 fils. La v10 y passait 19 à 24 ms à $K = 5$ (passes en série sur toutes les boules, tri, résolutions).

Champs : `cover_rank` (rang du niveau $A_k(x)$, un niveau du catalogue) ; `cover_nodes` (CSR : $E_k(x)$, nœuds triés) ; `cover_witness` (même forme : plus petite boule témoin de chaque nœud). À $k = 1$, $A_1(x) = 0$ et $E_1(x)$ est le site.

### 6.3 Conventions publiées à part, et nommées

L'auditeur indépendant l'a établi : **aucun choix d'un seul nœud n'est équivariant sous toutes les isométries**. Sur $\lbrace 0, 2, 4 \rbrace$ à $K = 2$, la réflexion fixe le point du milieu et échange ses deux composantes de première couverture. La v10 choisit la boule de plus petit indice, donc selon le rang de Morton : 47 sites de la trame 00 changent de classe sous un échange d'axes (L06 constat 02). La v11 ne publie donc aucun départage comme étant l'objet.

- **Projection conservatrice par premier ancêtre commun** (`cover_lca`, `cover_lca_rank`) : nœud $o(x)$, premier ancêtre commun de $E_k(x)$, et sa date, le rang de $o(x)$ si $E_k(x)$ a plusieurs éléments, `cover_rank` sinon. Elle est équivariante par isométrie et par renumérotation, laminaire (règle ancrée), et distincte de l'entrée `cover` : sa date est celle de l'ancêtre. Elle n'atteint pas la cible des deux triangles ; c'est écrit avec elle.
- **Convention de la v10** (`cover_v10`), pour le seul différentiel : le nœud du témoin de plus petit `BallIdx`. Elle dépend du rang de Morton ; elle n'est écrite que par le dump au format v10.

Sur les trames, 19 à 151 sites par ordre ont plusieurs composantes de première couverture (L06) : l'ensemble est presque toujours un singleton, et la CSR ne coûte rien.

### 6.4 Relation boule couvrante → nœud (amas discrets)

La couverture d'un nœud à une coupe (le K-polyèdre de la thèse) se lit ainsi, sans stockage par nœud :

- **naissances** : implicite. Le nœud de naissance $i$ porte la population de sa boule, lue dans la CSR du catalogue. Quand toutes les sphères de fenêtre sont régulières, la couverture d'un nœud est l'union des populations des naissances de son sous-arbre (proposition G de L02).
- **cellules étendues de fenêtre non naissance** : liste explicite `ext_cover` par ordre (boule, nœud à la coupe fermée, masque des sites apportés $U \setminus \bigcup \mathcal{S}_t$, théorème T7 (iv)). La fixture des quatre points cocycliques de L02 § 4.9 y laisse sa trace (le quatrième point, au niveau 25, sans fusion).

La tour publie donc l'arbre abstrait de $\pi_0$ **et** cette relation ; elle n'annonce pas de « continuations datées » au-delà.

### 6.5 Ce que le module ne fait pas

Hiérarchie de points réduite aux événements, condensation, sélection : étages suivants (L03 § 12.1), hors de cette conception. Les votes, ER0h, l'existence mûre restent hors du moteur (réponse Q5 de l'auditeur).

---

## 7. Mémoire et formats de sortie

### 7.1 Mémoire

Tous les tableaux ci-dessous sont des `Buffer` ; leurs tailles sont connues après la passe de comptage de l'étage P (nombre de boules par classe, de représentants par ordre, de boules étendues) ; l'étage appelle `MemoryBudget::admit` avec la somme avant d'allouer. Le nombre d'événements d'un ordre est exactement le nombre de naissances moins un, le nombre de fusions est au plus ce nombre : les bornes sont a priori.

| Tableau | Octets | Durée de vie |
| --- | --- | --- |
| index des supports (adressage ouvert, charge au plus 0,67) | 12 par boule | étages P et G |
| `cls_idx` | 4 par boule | P à V |
| listes de classes, préfixes des représentants | 8 par boule environ | P à V |
| semis : entrées et populations | 13 et $4k$ par naissance | P et G |
| cibles `T`, puis `rep_node` | 4 par représentant | G et T |
| cellules du noyau | 16 par naissance | T |
| événements | 20 par naissance | T et M |
| `jtop` | 4 par jonction | T et V |
| forêt : `rank`, `parent`, `lower`, CSR des enfants, boule de naissance, `minleaf` | 22 par nœud en moyenne | publié |
| historique : attache, événements par survivant | 20 par naissance | publié (index, hors empreinte) |
| index des sites | 15,4 par site | session |

Totaux calculés sur les comptes de la trame 02 (L01 § 6.3) : forêts 37 Mo à $K = 5$ et 164 Mo à $K = 10$ ; historique 20 et 89 Mo ; pointe transitoire de la tour 95 à 120 octets par boule (environ 135 Mo et 650 Mo), en plus du catalogue (43 et 243 Mo dans le format du § 1.1). La v10 mesurait 109 Mo et 562 Mo de tableaux de travail plus 51 et 242 Mo de forêts et de pointeurs (L06 constat 07), mais sur un catalogue de 147 et 683 Mo et hors de tout budget.

Deux exigences envers le socle en découlent : des **arènes de session réutilisées** d'une construction à la suivante (L04 C11 : 145 000 défauts de page par construction coûtent 0,09 s de noyau à un fil et 0,31 s à 48 fils), et la possibilité de grandes pages. Sans elles, l'étage P ne tient pas son estimation.

### 7.2 Formats

- **Objet en mémoire** : par ordre, les tableaux « publiés » ci-dessus, identifiants forts (`NodeIdx`, `BallIdx`, `LevelRank`, `SiteIdx`).
- **Export binaire versionné** (`io`) : en-tête (magie, version de format, $B$, $K$, pas et origine de la grille, nombres de sites, de boules, de nœuds par ordre, empreintes de l'entrée et du catalogue) ; table des sections avec longueur et somme de contrôle ; sections petit-boutistes par ordre (`rank`, `parent`, enfants, boule de naissance, `lower`) ; table des niveaux exacts (par rang : le support canonique de la première boule, en coordonnées, d'où le niveau se recalcule ; aucun flottant) ; sections optionnelles des attaches. Écriture transactionnelle : fichier temporaire puis renommage.
- **Empreinte canonique**, indépendante de la numérotation des nœuds et des sites. Deux empreintes par ordre, toutes deux en SHA-256 :
  - `merkle_iso` : la définition exécutable de L06 (`isometrie/tower_merkle.py`) reprise mot pour mot : $h$(naissance) du niveau réduit, $h$(fusion) du niveau réduit et des empreintes triées des enfants, empreinte de la racine ; verticales : liste triée des couples ($h_k(v)$, $h_{k-1}$(image)) ; attaches : liste triée (niveau d'entrée, $h$(nœud)). Invariante par isométrie de la grille. Les ancres de la v10 existent déjà pour les trois trames à $K = 5$ et la trame 00 à $K = 10$ (`anchors.jsonl`) : la comparaison v10 contre v11 ne demande aucun binaire.
  - `merkle_pop` : la même avec, dans $h$(naissance), la population triée en coordonnées. Elle distingue deux naissances de même niveau, ce que la première ne fait pas ; elle n'est pas invariante par isométrie.
- **Dump texte au format v10**, pour le différentiel octet pour octet contre le binaire figé : lignes `order`, `node`, `point`, écrites par `io` depuis l'objet. Trois conventions de la v10 y sont reproduites et déclarées comme telles (elles sont déjà isolées dans `reference/hgp11_ref/dumps.py`) : la numérotation (c'est aussi celle de la v11) ; l'écriture non réduite `num den` d'un niveau (règle `emitted_level`, portée dans `io` comme convention d'écriture, jamais lue par le moteur) ; le choix `cover_v10`.

---

## 8. Portes

Codes exacts de l'architecture (0 conforme, 1 désaccord d'un juge, 2 refus avant calcul, 3 plancher ou invariant, 4 mutant tué). Aucune porte ne re-parcourt ce qu'un théorème garantit : les oracles bornés établissent la vérité, les juges d'échelle cherchent des fautes d'implémentation par invariants globaux, échantillons et mutants.

### 8.1 Oracle borné : la référence `hgp11_ref`

La porte compare le moteur à l'étage A de `reference/` (définition par $\Gamma_k$, 14 points au plus, $K \leq 10$), **champ par champ sur l'arbre étiqueté** : naissances (niveau exact, population), fusions (niveau exact, enfants exacts), `lower` de chaque nœud, entrée `core` de chaque site (niveau et nœud), entrée `cover` de chaque site (niveau et **ensemble** des nœuds), projection par ancêtre commun. Le juge lit les champs publiés tels quels ; il ne relève jamais un nœud lui-même (c'est par là que cinq fautes sur six traversaient la porte de la v10).

Planchers par régime, comptés par le juge, la porte échouant au code 3 si l'un manque :

| Régime | Plancher (suite complète ; suite rapide entre parenthèses) |
| --- | --- |
| ordres jugés de 6 à 10 | 2 000 (100) |
| fusions d'au moins trois enfants | 10 000 (500) |
| niveaux portant au moins deux fusions | 500 (50) |
| coquilles étendues, par taille 3, 4, 5 à 6, 7 à 9, 10 à 12 | 500, 200, 100, 30, 10 (un dixième) |
| cellules étendues inertes à gain de couverture | 50 (5) |
| naissances de population supérieure à $k$ | 100 (10) |
| représentants à au moins 1 saut ; à au moins 2 ; dont aux ordres 6 à 10 | 5 000 ; 500 ; 250 (un dixième) |
| sphères hors catalogue recensées par l'index (saturées ; complètes) | 2 000 ; 200 |
| arrêts sur une cellule de fenêtre (pointeurs suivis) | 1 000 |
| entrées `core` exactement à un niveau de nœud | 200 |
| entrées `cover` à plusieurs composantes | 300 (30) |

Familles : celles de `hgp11_ref/families.py`, plus « noyau serré et halo » de L06 (`oracle_sauts.py` : 459 à 599 tirages sur 600 sautent aux ordres élevés à 14 points), sans laquelle les ordres 6 à 10 n'exercent pas la descente.

Fixtures gravées (coordonnées exactes) : celles de L02 annexe B et de L03 § 12.4 pour la tour et les deux entrées ; les deux témoins de l'auditeur ($\lbrace (0,0), (2,0), (4,0), (2,3) \rbrace$ à $K = 2$ : deux morceaux locaux déjà reliés à l'extérieur ; $\lbrace 0, 2, 4 \rbrace$ à $K = 2$ : terminal dépendant du choix, entrée `cover` à deux composantes) ; les deux triangles ; le contre-exemple au théorème 5 du manuscrit ; les 24 points cosphériques et les 24 points cocycliques avec centre (quotient polynomial, coût borné) ; 65 points cosphériques (refus `shell_too_wide`, code 2, avant calcul).

### 8.2 Portes unitaires par pièce

Chaque pièce a sa porte et son juge indépendant : plus petite boule (proposition, certificat combinatoire, certificat exact, repli Welzl atteint par harnais) contre la force brute en fractions ; quotient local contre la définition brute (c'est `quotient_check.py`, porté) jusqu'à 13 sites, puis coût seul jusqu'à 64 ; requêtes d'index contre la force brute en entiers, centres rationnels lointains compris (fixture `far_center` de L04) et aux extrêmes du domaine ; noyau et matérialisation contre le Kruskal par lots gardé dans `tests/` (c'est `foret_check.py` et `noyau_v11.cpp`, portés) ; `component_at` et l'image des naissances contre la remontée parent par parent.

### 8.3 Invariants globaux d'échelle (8 000, 16 000, 32 000 points et trames)

Tous en $O$(nœuds) ou $O(nK \log)$, calculés par un juge qui relit l'export binaire :

- (A) attache `core` vivante à la coupe fermée : niveau$(v) \leq D_k(x) <$ niveau(parent$(v)$) ; de même pour chaque nœud de $E_k(x)$ au niveau `cover` ;
- (C) `lower[v]` vivant à la coupe fermée du niveau de $v$ ;
- (D) plateaux atomiques : aucun nœud n'a un parent de même rang ; toute fusion a au moins deux enfants ; identité de forêt (naissances moins un égale la somme des arités moins un) ;
- naturalité des verticales sur tous les nœuds ;
- ordre 1 contre l'arbre couvrant minimal euclidien : **structure N-aire**, pas seulement les poids (les poids ne tuent ni `m_bin` ni `m_seq`) ; juge indépendant (scikit-learn ou scipy, jamais réimplémenté) ;
- Euler et restriction sur catalogue à $K + 2$ : portes du catalogue, rappelées ici parce que la tour en dépend ;
- préfixe : ordres 1 à 5 identiques, par empreinte, avec le catalogue à $K = 5$ et à $K = 10$ ;
- neutralité : deux politiques de descente valides (sauts vers les plus proches ou vers les plus petits indices ; premier représentant par le premier ou le dernier morceau ; avec et sans semis) donnent le même export à l'octet près. C'est le seul contrôle à l'échelle du régime des descentes longues ; il montre la cohérence, pas la justesse ;
- déterminisme : export et compteurs identiques à 1, 2, 8 et 48 fils ; entrée permutée et `PointId` renommés : empreintes égales ;
- isométries de la grille : les 48 permutations et miroirs d'axes (après translation dans le domaine) laissent invariantes `merkle_iso` de la forêt, des verticales, de `core`, de l'ensemble `cover` et de la projection par ancêtre commun. La convention `cover_v10` est exclue de cette porte, et une porte à part vérifie qu'elle **change** sur la trame 00 (47 sites), pour graver qu'elle n'est pas l'objet ;
- compteurs par boule (pas de descente, propositions, requêtes d'index, sites examinés, événements) : rapport par doublement de 8 000 à 16 000 à 32 000 au plus 1,15 ; maxima publiés ;
- **certificats par échantillon** (L08 § 11.2, porte `mhgp11_tower_certificates` ; `TOWER_v2` § 13.4) : c'est le juge indépendant de la justesse à l'échelle pour $k \geq 2$, qui manquait à la v10. L'API publique `tower::trace(k, F)` rend la chaîne des parties visitées par `resolve1` et par le suivi des pointeurs (c'est le localisateur de facettes, utile aux consommateurs ; ce n'est pas un crochet de test). Un juge en fractions exactes, sans index ni code commun, vérifie pour chaque pas que la partie suivante est dans la boule fermée de la plus petite boule de la précédente et de niveau strictement inférieur (lemme 2 de L02 : les deux parties sont alors dans la même composante), puis que la dernière partie est un sommet d'une naissance (recensement par force brute sur tous les sites). Pour une fusion tirée, il vérifie que la boule de chaque jonction du plateau a le niveau du nœud, que chacun de ses représentants aboutit sous un enfant de la fusion, que tous les enfants sont atteints et que les jonctions relient ces enfants. Planchers : au moins 1 000 descentes et 1 000 fusions par ordre et par entrée, stratifiées par longueur de chaîne (1, 2, 3 pas et plus) et par arité ;
- les empreintes canoniques du § 7.2 sont publiées dans **tout** reçu, y compris en mode chronométré sans attaches (règle R2 de L08 : la v10 ne savait pas exporter ce mode) ; épingles des trames du contrat par leur sha256 (`0baa4de1…`, `ba15adc6…`, `a4bbc86d…`), jamais par leurs octets.

### 8.4 Mutants (copies mutées, jamais de branche dans le produit)

Les six de L02 et de L06 dont cinq traversaient la porte de la v10 : multifusion binarisée (`m_bin` : ici, « chaque événement ouvre son nœud »), plateau non atomique (`m_seq`), image verticale à la coupe ouverte (`m_vopen`), image de fusion sans remontée (`m_vnoclimb`), attache stricte (`m_strict`), coquille étendue jamais jointe (`m_onepiece`). S'y ajoutent les fautes propres à cette conception :

| Mutant | Faute | Tué par |
| --- | --- | --- |
| `cert_sans_inclusion` | boule du catalogue acceptée sans contrôler $F \subseteq P_b$ | oracle (plancher des sauts) |
| `cible_sans_suivi` | cellule prise pour une naissance, pointeur non suivi | oracle ; invariant (D) |
| `saut_non_strict` | saut vers $k$ sites de la boule fermée, coquille comprise | oracle ; plafond de pas |
| `census_ouvert` | requête d'index qui perd les sites de coquille | oracle (coquilles étendues) |
| `fenetre_basse` | fenêtre basse à $p + q$ | racine multiple ; oracle |
| `manquante_muette` | refus `catalogue_missing_ball` désactivé | catalogues amputés |
| `union_min_sans_feuille` | clé canonique prise sur la racine d'union et non sur la plus petite feuille | noyau contre lots ; campagne v10 |
| `lien_plateau_perdu` | deux événements de même rang liés non contractés | (D) ; oracle |
| `jtop_sans_remontee` | image d'une naissance sans la remontée conditionnelle | (C) |
| `historique_strict` | `component_at` avec rang strictement inférieur | (A), (C) |
| `fenetre_angulaire_fermee` | antipode inclus dans une fenêtre | quotient contre brut |
| `seuil_de_lien` | lien entre ensembles à $t + 1$ au lieu de $t$ | quotient contre brut |
| `cover_premier_seul` | $E_k(x)$ tronqué à son premier élément | oracle (plancher des égalités) |
| `cover_temoin_large` | témoins pris sans la condition $p + q_{\min} \leq k$ | sans effet attendu : mutant équivalent, à déclarer tel (annexe A.8) |
| `semis_sans_verification` | égalité de semis sur l'empreinte seule, empreinte affaiblie dans la copie | oracle |

### 8.5 Campagne appariée contre la v10 figée

- Petits nuages : dumps au format v10 identiques à l'octet près au binaire figé, entrées `core` et `cover` (la référence le fait déjà depuis ses deux étages ; le moteur s'y ajoute).
- Trames du contrat et entrées de 8 000 à 32 000 points : dumps identiques ; à défaut de binaire sur la machine, empreintes `merkle_iso` égales aux ancres de L06 (trames 00, 01 et 02 à $K = 5$, trame 00 à $K = 10$) et empreinte du dump de la trame 01 à $K = 5$ (`6ebb1eb4…`, retrouvée par L02 et L06).
- Toute différence devient une fixture minimale et une ligne du registre avant de continuer.

---

## 9. Budget de temps

Machine : G4, EPYC 9B45, 24 cœurs, 48 fils. Colonnes « v10 mesuré » : reçu `g4_session4_j2c_20260929`, dernière passe chaude, trames 01, 00 et 02 dans cet ordre de charge, extrait par L06 (`preuves_l06_code_tour/g4/extraction_recu_g4_session4.txt`). Colonnes « v11 » : **estimations**, avec leur fondement ; aucune n'est une mesure sur G4.

| Étage | v10 mesuré, $K = 5$ (ms) | v11 estimé, $K = 5$ (ms) | v10 mesuré, $K = 10$ (ms) | v11 estimé, $K = 10$ (ms) | Fondement de l'estimation |
| --- | ---: | ---: | ---: | ---: | --- |
| P : index des supports, listes de classes, semis, quotients | 9,3 à 11,5 (`t_prepare` + `t_local` + `t_seeds`) | 4 à 6 | 48 à 62 | 20 à 30 | plus de tableaux par cellule ni de copie par boule ; empreinte additive ; arènes déjà touchées. Fondement faible : le coût de premier contact des pages n'est pas mesuré séparément |
| index des sites (hors tour en v10 : `prepare_s` 6,5 à 8,3, nuage et pool compris, dont 3,4 à 4,6 pour le k-d) | — | moins de 0,3 | — | moins de 0,3 | une passe sur 46 000 sites, sans tri ; non mesuré |
| G : résolution | 22,2 à 28,6 | 10 à 16 | 179 à 243 | 75 à 130 | profil `rdtsc` de L06 par poste, multiplié par des coûts unitaires supposés (sonde 100 cycles, proposition 350 à 450, certificat combinatoire 100, recherche 300, requête d'index 1 400 à 1 600) : 5,0 milliards de cycles ramenés à 1,8 ($K = 5$), 35,7 à 13,7 ($K = 10$), soit ÷2,6 à ÷2,7 ; fourchette élargie à ÷1,8 parce qu'aucun de ces coûts n'est mesuré |
| T : noyau de l'ordre maximal (séquentiel) | 20,8 à 31,6 (lots et pointeurs de saut) | 8 à 12, dont visible 0 à 6 | 52,3 à 90,3 | 20 à 35, dont visible 0 | rapport 2,5 à 2,9 **mesuré** ici sur les entrées réelles (§ 4.5), appliqué au temps G4 mesuré ; recouvrement par G estimé |
| M : matérialisation (parallèle) | compris dans T | 1,5 à 3 | compris dans T | 6 à 12 | 93 ms de CPU local en séquentiel naïf pour les cinq ordres, à répartir sur 48 fils |
| V : verticales | 14,8 à 21,5 | 2 à 4 | 47,5 à 64,8 | 10 à 20 | naissances en $O(1)$ ; une requête par fusion (§ 5) |
| **Tour FULL** | **67,3 à 89,3** | **22 à 32** | **334 à 472** | **125 à 180** | chemin critique : P, puis le plus long de G et du noyau recouvert, puis M, puis V |
| attaches `core` (hors tour FULL) | non mesuré seul sur G4 | 2 à 4 | — | 5 à 10 | $nK$ résolutions ; une requête entière par site |
| attaches `cover` | 18,7 à 24,3 (`t_points`) | 0,5 à 1 | non mesuré | 4 à 6 | 3,7 et 33 millions d'incidences, deux passes |

Rendement parallèle mesuré de la v10 (table de vérité de l'audit de performance, `preuves_l09_perf_lidar/TABLE_VERITE_G4.md` § 6, trame 02, $K = 5$, session 1 à un fil et session 4 à 48 fils) : descentes 852 ms à un fil et 27,1 ms à 48 (×31) ; index, atlas et semis 121 ms et 11,5 ms (×10,5 seulement : ces étages sont bornés par la mémoire, ce qui fonde le risque R4) ; Kruskal 77,6 ms à un fil pour les cinq ordres et 76,7 ms en somme à 48 fils (aucune interférence entre noyaux d'ordres différents) ; verticales 103 ms à un fil.

Lecture.

- **$K = 5$** : la cible de 30 ms est dans la fourchette. Elle repose pour moitié sur une mesure (le noyau) et pour moitié sur l'estimation de G. Si G ne gagnait rien (22 à 29 ms), la tour resterait à 33 à 42 ms : le noyau sans lots, les verticales et la préparation rendent à eux seuls 35 à 50 ms sur 67 à 89.
- **Somme avec le catalogue** : L05 § 6.9 estime le catalogue à 82 à 93 ms à $K = 5$ en CPU seul avec les seuls leviers fondés. Avec une tour à 22 à 32 ms, la chaîne ferait 105 à 125 ms : **le contrat de 100 ms à $K = 5$ se joue dans le générateur**, pas dans la tour. Les conceptions voisines visent, sans mesure encore, 18 à 30 ms (`CONCEPTION_GENERATEUR.md` § 0) ou 40 à 54 ms (`PISTES_DE_RUPTURE.md` § 7) pour le catalogue, soit un moteur à 65 à 91 ms dans la table de synthèse de cette dernière.
- Les 8 à 15 ms de préparation en série relevées par L04 (nuage, k-d, mise en place) ne sont pas dans ce tableau ; l'index proposé en retire la construction du k-d, soit 3,4 à 4,6 ms sur G4 (différence des préparations des deux sondes de la v10, L04 C10).

---

## 10. Ce qui est atteignable à $K = 10$

Honnêtement : **pas 100 ms en CPU seul**, ni pour la tour ni, a fortiori, pour la chaîne.

- La tour à $K = 10$ est estimée à 125 à 180 ms. Son étage G seul fait 75 à 130 ms : 17,4 millions de représentants demandent au moins une sonde chacun, et 7,7 millions de plus petites boules restent à proposer et à certifier, dont 1,65 million par une requête d'index.
- Le catalogue à $K = 10$ est estimé par L05 à 297 à 346 ms en CPU seul. La chaîne ferait donc 420 à 530 ms, contre 860 à 1 125 ms mesurés en v10 (catalogue 528 à 653, tour 334 à 472, par trame).
- L'objet lui-même borne le débit : 7,47 millions de nœuds et 5,48 millions de boules sur la trame 02 (L01 § 6.3) ; 100 ms laissent 0,9 µs de CPU par boule à 48 fils parfaitement occupés.
- Ce qui reste, hors de cette conception. Le nombre de pas ne se réduit plus : la conception voisine le mesure à 8 % de son minimum et écarte les liens de morceaux fournis par le générateur (`PISTES_DE_RUPTURE.md` § 3.1 et § 3.4). Restent : le coût du pas (l'étage G est un lot d'évaluations pures et indépendantes, sans mémo partagé ici, donc portable sur un accélérateur ; la même conception voisine classe le GPU hors du premier moteur) ; et un contrat à $K = 10$ sur **un seul ordre** (R3.1 : 21 à 26 % des représentants et des pas de la tour FULL, aucune verticale), question 1 de L01 § 11.
- À $K = 10$ le noyau séquentiel n'est pas le plafond : 20 à 35 ms recouverts par un étage G trois fois plus long. Il le redeviendrait si G passait sur GPU ; l'option de coupure par rangs (`TOWER_v2` § 6.5, prototype privé P3) serait alors à mesurer.

---

## 11. Mesures décisives à faire sur G4

Toutes par l'agent de mesure, en processus résident, passes chaudes alternées (au moins 5), 1, 24 et 48 fils, avec le jeu d'instructions de référence de la v11. Entrées : les trois trames du contrat, et pour les prototypes les vidages produits par une copie instrumentée de la v10 (représentants, parties à résoudre, sphères hors catalogue, entrées du Kruskal).

| Id | Prototype | Entrée | Variantes | Critère de décision |
| --- | --- | --- | --- | --- |
| M1 | `preuves_tour/noyau_v11.cpp` | vidages du Kruskal des trois trames, $K = 5$ et $10$, tous les ordres | préchargement à 0, 8, 16, 32 ; union par taille ou par plus petit indice ; un fil seul, puis un fil pendant que 47 autres chargent la mémoire | noyau de l'ordre maximal au plus 12 ms ($K = 5$) et 35 ms ($K = 10$) ; sinon mesurer l'option de coupure par rangs |
| M2 | région G + T (propriétaires et aides) | trames, $K = 5$ et $10$ | régime O1 (par ordre) contre O2 (flux par morceaux) ; taille de morceau 32, 64, 256 jonctions | O2 adopté s'il rend au moins 3 ms sur le mur de la tour à $K = 5$ ; sinon O1, plus simple |
| M3 | semis seul | représentants vidés | entrée de 8 octets et populations à part ; population dans l'entrée ; empreinte additive ou hachage de la suite ; lots de préchargement 16, 32, 64 | cycles par sonde à 48 fils ; mémoire ; garder la disposition de base si l'écart est sous 15 % |
| M4 | plus petite boule seule | parties non semées vidées (1,08 et 7,66 millions sur la trame 00) | proposition de la v10 portée ; marche depuis la sphère de départ ; part des certificats combinatoires ; coût du certificat géométrique | cycles par boule ; 0 repli ; adopter la marche si elle gagne au moins 20 % |
| M5 | `census_upto` | sphères hors catalogue vidées (0,26 et 1,65 million) | index implicite (blocs de 8, de 16) ; k-d de la v10 ; recensement par feuille du générateur si la conception du catalogue garde ses feuilles | cycles et sites examinés par requête ; index retenu si au moins 2 fois moins cher que le k-d ; la variante par feuille n'entre que si elle fait encore 2 fois mieux et couvre le cas $p \geq K$ |
| M6 | politique de saut | trames | saut depuis le catalogue vers les $k$ plus proches ou vers les $k$ plus petits indices | compteurs déterministes : retenir les plus petits indices si le nombre de plus petites boules monte de moins de 5 % |
| M7 | étage P | trames | arènes neuves ou réutilisées ; grandes pages ou non | P au plus 6 ms à $K = 5$ et 30 ms à $K = 10$ en passe chaude ; publier aussi la passe froide |
| M8 | verticales | forêts des trois trames | historique d'attache ; pointeurs de saut de la v10 construits à part | étage V au plus 4 ms à $K = 5$ ; distribution des sauts et des sondes |
| M9 | matérialisation | événements vidés par M1 | 1, 24, 48 fils ; tranches de 4 096 à 65 536 événements | au plus 3 ms à $K = 5$ à 48 fils |
| M10 | quotient local | 24, 48, 64 points cosphériques ; cercles de 24 à 64 points ; grilles $6^{3}$ à $16^{3}$ | filtre F6 actif ou non | coût par boule ; nombre de replis exacts ; budget de quotient à publier |
| M11 | tour entière | trois trames ; trame avec sol (123 389 sites) ; 8 000, 16 000, 32 000 points des cinq familles | 1, 24, 48 fils ; avec et sans attaches | $K = 5$ : au plus 30 ms sur chaque trame du contrat ; compteurs par boule stables par doublement (rapport au plus 1,15) |
| M12 | réveil d'une région | région vide, puis 48 tranches courtes | sommeil ou attente active bornée | coût d'une région ; la tour en ouvre une douzaine (51 comptées en v10 par L04) |

---

## 12. Obligations de preuve

Énoncés que cette conception exige et qui ne sont pas dans L01 à L03. « Preuve ici » renvoie à l'annexe A ; toutes sont à contre-lire par un tiers et à inscrire au registre avec leur fixture avant le code correspondant.

| Id | Énoncé | État |
| --- | --- | --- |
| PO-T1 | Certificat combinatoire : $S^{*} \subseteq F \subseteq P_b$ entraîne $B(F) = b$ | preuve ici (A.1) ; relatif à (H2) |
| PO-T2 | Chaque pas de `resolve1` est un pas valide du théorème D, cas sous la fenêtre compris ($t \leq q_{\min} - 2$ : toute partie de $U$ convient) | preuve ici (A.2) |
| PO-T3 | Arrêt sur une cellule et suivi des pointeurs : la naissance obtenue est dans la composante du représentant dans $\Gamma_k^{<}(\lambda)$ ; chaîne acyclique ; date d'usage | preuve ici (A.3) |
| PO-T4 | Noyau sans lots et contraction des événements liés de même rang : égal à l'arbre de fusion à plateaux atomiques, pour tout ordre des unions dans un rang et toute règle d'union | preuve ici (A.4, par le théorème J) ; contrôlé (6 ordres réels, 6 000 hypergraphes) |
| PO-T5 | Historique d'attache : profondeur au plus $\log_2$ du nombre de naissances ; `component_at` rend le nœud vivant à la coupe fermée | preuve ici (A.5) ; contrôlé (1,59 million de requêtes) |
| PO-T6 | Image d'une naissance par `jtop` et une remontée conditionnelle | preuve ici (A.6) ; contrôlé (2,44 millions de jonctions réelles) |
| PO-T7 | Quotient local par ensembles séparables maximaux (théorème T7) | preuve ici (A.7) ; contrôlé (5 937 contrôles contre la définition brute, coquilles de 3 à 13 sites) |
| PO-T8 | Témoins de `cover` : $A_k(x)$ et $E_k(x)$ lus sur les boules avec $p + q_{\min} \leq k \leq p + m$ ; cas régulier = naissances de l'ordre | preuve ici (A.8), d'après P1 de L03 ; contrôlé contre l'étage de définition de la référence (1 317 entrées, dont 216 à plusieurs composantes, 0 écart) |
| PO-T9 | Refus `catalogue_missing_ball` : une sphère non saturée, en fenêtre à l'ordre $k \leq K$, vérifie $p + q_{\min} \leq K + 1$, donc doit être au catalogue | immédiat ($k \geq p + q_{\min} - 1$) |
| PO-I1 | Borne inférieure flottante certifiée de la distance d'un centre rationnel à une boîte entière, et borne supérieure certifiée d'un rayon, sous tout mode d'arrondi et avec contraction ; erreur absolue certifiée du centre approché en fonction de $B$ | à écrire dans `num` (une soustraction entre un entier exact et une approximation : hors F3, à borner par l'erreur absolue du centre ; l'élagage ne décide rien) |
| PO-I2 | Filtre F6 des deux prédicats de coquille et du côté de sphère, par palier d'étendue certifiée opérande par opérande | à écrire dans `num` (règle F6 de l'architecture) |
| PO-B | Budgets de bits, en `constexpr` depuis $B$ : clé de sphère par forme de centre ; prédicat $D \det(\cdot) - N \cdot w$ (135 bits à 18 bits) ; composante $D (x_l - a) \times (x_x - a) - N \times (x_x - x_l)$ (115 bits) ; niveau contre entier ; support canonique | à écrire dans `num` ; tables de L04 C05 comme attendu |
| PO-M | Multiplicités : rien n'est énoncé ici pour des sites pondérés ; la tour refuse | assumé (`ARCHITECTURE.md` § 7.3) |

---

## 13. Exigences envers les fondations et le catalogue

**`core`.** Arènes de session réutilisées entre constructions, grandes pages possibles, admission par formule (déjà là) ; `Csr` ; grand livre de compteurs par étage et par ordre, hors objet publié ; raisons du § 1.5 ajoutées en fin de table avec leurs portes ; un tampon de travail par fil à capacité bornée par une constante ($k_{\mathrm{cat}}$, 64 sites de coquille, profondeur de pile de l'index).

**`num`.** Formes de centre typées (2, 3, 4 sites) portant leur budget ; clé de sphère exacte par forme ; signes barycentriques ; support canonique d'une coquille (fonction unique, partagée avec le catalogue) ; les deux prédicats de coquille du § 2.2 ; niveau exact depuis un support, comparaison d'un niveau à un entier, clé binaire64 F3 d'un niveau ; centre approché avec erreur absolue certifiée (PO-I1) ; filtres F6 à paliers (PO-I2). Juge indépendant aux extrêmes de chaque profil.

**`sched`.** Boucle parallèle à réclamation dynamique dans l'ordre des indices, grain 1 possible ; primitive d'attente courte ; réduction déterministe des refus par ordinal ; indice de fil stable pour les tampons de travail ; aucun fil créé ailleurs ; coût d'une région mesuré (M12) ; tri par comptage et préfixes parallèles.

**`cloud`.** Sites en ordre de Morton, coordonnées en tableaux par axe d'entiers de 32 bits ; poids (la tour refuse tout poids supérieur à 1) ; table site → `PointId` ; pas et origine de la grille ; une fixture qui grave la convention de l'ordre de Morton (le mutant M14 de L04 survit à tout en v10).

**`io`.** Export binaire versionné et son lecteur ; empreintes `merkle_iso` et `merkle_pop` ; dump au format v10 avec la règle d'écriture `emitted_level` et le choix `cover_v10`, isolés comme conventions ; sorties transactionnelles ; sous-commande de dump canonique lisible par le juge de `reference/`.

**`catalogue`** (conception voisine). Enregistrements du § 1.1 ou tableaux équivalents ; ordre canonique et rangs denses ; `rank_first` et `level_key` ; table des supports des coquilles étendues ; limite de coquille commune (64) ; la fonction de support canonique partagée ; diagnostic d'Euler et restriction comme portes ; si les feuilles sont conservées pour une autre raison, leur accès en lecture pour la mesure M5.

**`reference`.** Déjà conforme au modèle visé (ensemble `cover`, arbre étiqueté). À ajouter : la projection par ancêtre commun, la relation de couverture des cellules étendues, et la famille « noyau serré et halo ».

---

## 14. Risques et questions ouvertes

| Id | Risque ou question | Parade |
| --- | --- | --- |
| R1 | L'estimation de G (÷1,8 à ÷2,7) repose sur des coûts unitaires non mesurés | M3, M4, M5 avant le code produit ; la tour tient 33 à 42 ms à $K = 5$ même sans ce gain |
| R2 | Le noyau, borné par la latence mémoire, ralentit quand 47 fils chargent le cache L3 | M1 (variante sous charge) ; flux O2 ; préchargement |
| R3 | Coût réel d'une requête d'historique (dichotomie dans 40 000 à 170 000 événements d'une même racine) | M8 ; repli possible sur des pointeurs de saut construits hors chemin critique |
| R4 | Premier contact des pages dans l'étage P | arènes réutilisées ; M7 |
| R5 | Le format du catalogue de la conception voisine (tableaux séparés, rangs à partir de 0, éventuel stockage dans l'ordre d'émission) diffère du § 1.1 | la vue se construit dans P en une passe ; l'étage G ne dépend pas de l'ordre de stockage |
| R6 | Prédicats de coquille larges aux profils 21 et 24 bits | chemin rare ; budgets par `num` ; profils non qualifiés tant qu'ils n'ont pas leurs portes |
| R7 | Longueur des descentes : aucune borne prouvée (12 pas mesurés) | plafond publié `kStepCap`, refus typé ; histogramme dans les compteurs |
| R8 | Juge indépendant de la justesse à l'échelle pour $K \geq 2$ (L02 question 4, L06 question 3) | certificats par échantillon du § 8.3 (chaîne de descente rejugée en fractions, fusions par leurs jonctions) ; la neutralité juge en plus la cohérence. Ce juge reste un échantillon : il ne voit pas une jonction absente du catalogue |
| Q1 | Les verticales entrent-elles dans le chronomètre du contrat ? Elles pèsent 2 à 4 ms ici | à trancher par l'utilisateur ; le budget les compte |
| Q2 | Le contrat part-il du fichier, du nuage préparé ou du catalogue ? | L04 question 1 ; ce document compte la tour depuis le catalogue et l'index |
| Q3 | Les cellules étendues inertes laissent une trace (`ext_cover`) : est-ce l'objet voulu, ou faut-il des contributions datées complètes ? | L02 question 2 ; la trace proposée suffit aux K-polyèdres (théorème T7 (iv)) |

---

## 15. Découpage (règles de propreté de l'architecture)

Un fichier par contrat, chacun sous 500 lignes, chaque fonction sous 100, chacun avec sa porte unitaire et ses mutants ; aucun état global ; les tampons de travail par fil appartiennent à la `Session`.

| Module | Fichiers internes | Contrat |
| --- | --- | --- |
| `index` | `site_index` (construction), `query_int`, `query_rat` | § 3.6 |
| `tower` | `view` (vue du catalogue, classes), `prepare` (comptage, contrôles de structure, admission mémoire), `support_index`, `seeds`, `shell_quotient`, `meb` (proposition, certificats, repli), `resolve` (`resolve1`, cibles, `trace`), `kernel`, `materialize`, `history` (`component_at`), `verticals`, `build` (orchestration, statut, grand livre) | § 1 à 5 |
| `points` | `core`, `cover`, `conventions` | § 6 |

Dans `tests/` et jamais dans le produit : le Kruskal par lots de référence, la définition brute du quotient local, la force brute des requêtes d'index, la descente alternative de la porte de neutralité, les copies mutées.

---

## Annexe A — Preuves des énoncés nouveaux

Notations de L02 § 4.1 : $B(F)$ plus petite boule de $F$, $\beta(F)$ son rayon carré, $P_b = I \cup U$, $t = k - p$, $V_{<}(b, k)$ l'ensemble des $k$-parties de $P_b$ de trace séparable sur $U$. Sites distincts, aucune position générale. Ces preuves sont de l'auteur de cette conception ; elles n'ont été relues par personne.

### A.1 Lemme T1 (certificat combinatoire)

*Énoncé.* Si $b$ est une sphère critique, $S$ un de ses supports, et $S \subseteq F \subseteq P_b$, alors $B(F) = b$.

*Preuve.* $F$ est dans la boule fermée de $b$. $S$ est sur la sphère et $c \in \mathrm{relint}\,\mathrm{conv}(S) \subseteq \mathrm{conv}(F \cap U)$ : la trace $F \cap U$ n'est pas séparable, et le lemme 1 de L02 donne $B(F) = b$. $\square$

Le moteur l'applique avec $S = S^{*}$ lu au catalogue ; l'hypothèse $F \subseteq P_b$ se contrôle sur les identifiants. L'exactitude de $I$ et de $U$ est (H2).

### A.2 Pas valides de `resolve1`

Soit $b = B(F)$, $\lvert F \rvert = k$. (a) $p \geq k$ : toute $k$-partie de $I$ (théorème D). (b) $p < k \leq p + q_{\min} - 2$ : $t \leq q_{\min} - 2$, donc toute partie $A$ de $U$ de $t$ sites est séparable (lemme 3 de L02 : une partie non séparable contient un support, donc au moins $q_{\min}$ sites), et $I \cup A$ est un pas valide. (c) $p + q_{\min} - 1 \leq k \leq p + m$ et la cellule n'est pas une naissance : $I \cup A$ pour $A$ dans le premier morceau. Dans les trois cas $\beta$ décroît strictement et la partie d'arrivée est dans la composante de $F$ dans $\Gamma_k(\beta(F))$ (théorème D). Si la cellule est une naissance, $F$ en est un sommet : arrêt.

### A.3 Lemme T3 (arrêt sur une cellule, pointeurs)

*Énoncé.* Soit $r$ un représentant d'une jonction de niveau $\lambda$ à l'ordre $k$, $F_0 = r, F_1, \ldots, F_s$ les parties visitées par `resolve1`, et supposons l'arrêt sur une cellule non naissance $(b', k)$ avec $b' = B(F_s)$. Soit $r'$ le premier représentant de cette cellule et $N$ la naissance obtenue en suivant la cible de $r'$. Alors le rang de $b'$ est strictement inférieur à celui de la jonction, et $N$ est dans la composante de $r$ dans $\Gamma_k^{<}(\lambda)$.

*Preuve.* $\beta(F_0) < \lambda$ (lemme 1 : la trace d'un représentant est séparable) et les niveaux décroissent le long de la chaîne, donc le niveau de $b'$ est $\beta(F_s) \leq \beta(F_0) < \lambda$. $F_s$ et $r'$ sont deux $k$-parties de $P_{b'}$ avec $\lvert P_{b'} \rvert \geq k + 1$, donc dans la même composante de $\Gamma_k(\lambda_{b'})$ (lemme 2), et $r' \to \cdots \to N$ est une descente valide. La composition est une suite de pas valides de $F_0$ à $N$ : par le théorème D, $N$ est dans la composante de $F_0$ à tout niveau au moins $\beta(F_0)$, en particulier dans $\Gamma_k^{<}(\lambda)$. La récurrence sur le rang de la cellule d'arrêt termine : chaque pointeur descend strictement. $\square$

*Date d'usage.* L'énoncé ne dit rien en dessous de $\beta(F_0)$. Le noyau lit la cible de $r$ à la coupe ouverte $\lambda$ ; les verticales et les attaches la lisent à une coupe fermée de niveau au moins $\beta(F_0)$.

### A.4 Théorème T4 (noyau sans lots, contraction)

*Énoncé.* Soit $G_t$ l'hypergraphe des naissances muni des jonctions de rang au plus $t$. Le noyau traite les jonctions par rang croissant, dans un ordre quelconque à rang égal, avec une règle d'union quelconque. Alors, pour chaque rang $t$ : les classes des événements de rang $t$ pour la clôture de « l'un a pour opérande le sommet produit par l'autre » sont en bijection avec les composantes de $G_t$ qui réunissent au moins deux composantes de $G_{t-1}$ ; les opérandes de rang inférieur (ou les feuilles) d'une classe sont les sommets de ces composantes de $G_{t-1}$, chacun une fois.

*Preuve.* Par récurrence sur $t$, supposons qu'après le rang $t - 1$ les classes de l'union-find soient les composantes de $G_{t-1}$, chacune avec son sommet (son nœud, ou sa feuille). Un événement de rang $t$ réunit deux classes courantes $X$ et $Y$, distinctes, que relie une jonction de rang $t$ : elles sont dans une même composante $C$ de $G_t$. Chaque opérande est ou bien le sommet d'une composante de $G_{t-1}$ non encore touchée au rang $t$, ou bien le sommet produit par un événement antérieur de rang $t$, auquel cas les deux événements sont liés. Les événements de rang $t$ dont les classes sont dans $C$ forment donc un arbre binaire dont les feuilles sont les sommets des composantes de $G_{t-1}$ contenues dans $C$ et dont les nœuds internes sont liés de proche en proche ; deux événements de composantes différentes ne partagent aucun opérande. En fin de rang, les classes sont les composantes de $G_t$ (toute jonction de rang $t$ a été traitée), et une composante de $G_t$ sans événement est une composante de $G_{t-1}$ inchangée. Par le théorème C de L02, ces composantes à au moins deux enfants sont les multifusions de niveau $t$ avec exactement ces enfants. L'argument n'emploie ni l'ordre dans le rang ni la règle d'union. $\square$

La clé (rang, plus petite naissance de la composante) est unique : deux nœuds de même rang ont des sous-arbres disjoints. Qu'une naissance ne soit jamais enfant d'une fusion de son propre niveau vient de ce que tout représentant descend strictement (corollaire du théorème B).

### A.5 Lemme T5 (historique d'attache)

*Énoncé.* Avec l'union par taille : (i) le chemin d'attache d'une naissance a au plus $\lfloor \log_2 n_b \rfloor$ arêtes, $n_b$ étant le nombre de naissances ; (ii) les rangs y sont croissants au sens large ; (iii) `component_at`$(\ell, r)$ rend le nœud vivant à la coupe fermée de rang $r$ dont la composante contient $\ell$.

*Preuve.* (i) Quand une racine perd, sa composante est au plus aussi grande que celle du survivant : la taille de la composante d'une naissance double au moins à chaque arête de son chemin. (ii) Une racine perd après toutes les unions où elle a survécu, et les événements sont traités par rang croissant. (iii) Par (ii), la boucle franchit exactement les attaches faites par des événements de rang au plus $r$ : elle s'arrête sur la racine $x$ de la classe de $\ell$ dans l'état où tous les événements de rang au plus $r$ sont traités, c'est-à-dire sur la composante $C$ de $G_r$. Si $C$ est réduite à une naissance, c'est le nœud. Sinon le dernier événement à l'intérieur de $C$ a $x$ pour survivant, et c'est le dernier événement de survivant $x$ de rang au plus $r$ (un événement ultérieur de survivant $x$ a un rang supérieur à $r$). Son nœud est le sommet de $C$ : s'il est de rang $r$, c'est la classe de plateau du théorème T4, qui contient tous les événements de rang $r$ de $C$. $\square$

### A.6 Lemme T6 (image d'une naissance)

*Énoncé.* Soit $j$ une jonction de rang $r$, $w_0$ le sommet de sa classe juste après son traitement, $w$ le nœud de $w_0$. Le nœud vivant à la coupe fermée $r$ qui contient les représentants de $j$ est le parent de $w$ si ce parent a le rang $r$, et $w$ sinon.

*Preuve.* Tout événement postérieur a un rang au moins $r$. Si $w_0$ est un événement de rang $r$, $w$ est sa classe de plateau, qui contient tout événement de rang $r$ lié, antérieur ou postérieur : c'est le nœud cherché, et son parent a un rang supérieur. Sinon $w$ a un rang inférieur à $r$ (ou c'est une feuille) ; le premier événement postérieur qui touche la classe a $w_0$ pour opérande : s'il est de rang $r$, sa classe de plateau est le parent de $w$ et c'est le nœud cherché ; s'il est de rang supérieur, ou s'il n'existe pas, la composante n'a pas changé à la coupe fermée $r$ et le nœud est $w$. $\square$

### A.7 Théorème T7 (quotient local par ensembles séparables maximaux)

Soit $u_x = x - c$ pour $x \in U$ ; les $u_x$ ont la même norme et sont deux à deux distincts. $A \subseteq U$ est séparable si et seulement s'il existe $v$ avec $\langle v, u_x \rangle > 0$ sur $A$ (Gordan), c'est-à-dire $A \subseteq H(v)$.

*(i) Les éléments de $\mathcal{S}$ sont séparables.* Soit $v_0 = \pm (u_i \times u_j) \neq 0$, $P$ et $Z$ comme au § 2.2. Les $u_x$, $x \in Z$, sont dans le plan $\Pi = v_0^{\perp}$, deux à deux non colinéaires de même sens. La fenêtre $W_l$ est l'ensemble des points de $Z$ d'angle dans $[\theta_l, \theta_l + \pi)$ pour l'orientation de $\Pi$ donnée par $v_0$ ; comme $Z$ est fini, il existe un vecteur $w \in \Pi$ avec $\langle w, u_x \rangle > 0$ exactement sur $W_l$. Pour $\varepsilon > 0$ assez petit, $v = v_0 + \varepsilon w$ est strictement positif sur $P$ (par continuité) et sur $W_l$ (où $\langle v_0, u_x \rangle = 0$) : $P \cup W_l \subseteq H(v)$.

*Toute partie séparable est dans un élément de $\mathcal{S}$.* Soit $A \subseteq H(v)$. Si tous les $u_x$ sont colinéaires, $m = 2$ et la clause finale donne les singletons. Sinon l'arrangement des grands cercles $\langle \cdot, u_x \rangle = 0$ a au moins deux cercles distincts, et toute cellule ouverte a un sommet $v_0 = \pm (u_i \times u_j)$ dans son adhérence. En déplaçant $v$ dans une cellule ouverte adjacente (les inégalités strictes sur $A$ se conservent), puis en lisant cette cellule depuis l'un de ses sommets : $H = P \cup \lbrace x \in Z : \langle w, u_x \rangle > 0 \rbrace$ pour une direction tangente $w$ générique, et un demi-plan ouvert de $Z$ est contenu dans la fenêtre semi-ouverte qui commence à son premier point (ou est vide). Donc $A \subseteq P \cup W_l$. Ne traiter chaque plan par le centre qu'une fois ne perd rien : deux paires du même plan donnent les mêmes sommets, donc les mêmes ensembles.

*(ii) Morceaux.* Les sommets sont les $t$-parties contenues dans un élément de $\mathcal{S}_t$. Deux $t$-parties d'un même $M$ sont reliées : on passe de l'une à l'autre par échanges d'un site, chaque union de $t + 1$ sites restant dans $M$, donc séparable. Si $\lvert M \cap M' \rvert \geq t$, une $t$-partie commune relie les sommets de $M$ à ceux de $M'$. Réciproquement, si $A \subseteq M$ et $A' \subseteq M'$ ont une union séparable, elle est dans un $M'' \in \mathcal{S}_t$, et $\lvert M \cap M'' \rvert \geq t$, $\lvert M' \cap M'' \rvert \geq t$. Les composantes du graphe des sommets sont donc celles du graphe sur $\mathcal{S}_t$.

*(iii)* Aucun sommet si et seulement si $\mathcal{S}_t = \emptyset$. *(iv)* Un site appartient à un sommet si et seulement s'il est dans un élément de $\mathcal{S}_t$. $\square$

Pour une coquille régulière on retrouve le corollaire du théorème B : $\mathcal{S}$ est la famille des $q$ parties $U \setminus \lbrace u \rbrace$.

### A.8 Proposition T8 (témoins de `cover`)

*Énoncé.* Soit $x$ un site et $k \geq 2$. (a) $A_k(x)$ est le plus petit niveau d'une boule critique $b$ avec $x \in P_b$ et $\lvert P_b \rvert \geq k$. (b) Toute boule de ce niveau qui contient $x$ et au moins $k$ sites vérifie $p + q_{\min} \leq k$ ; elle est donc au catalogue. (c) $E_k(x)$ est l'ensemble des nœuds, à la coupe fermée, des composantes des centres de ces boules. (d) Si la coquille est régulière, une telle boule a $p + q = k$ : c'est une naissance de l'ordre $k$, et son nœud est son nœud de naissance.

*Preuve.* (a) Une $k$-partie $F \ni x$ de rayon minimal a une plus petite boule critique qui la contient ; réciproquement toute boule fermée contenant $x$ et $k$ sites a un rayon au moins $\alpha_k(x)$. (b) Supposons $k \leq p + q_{\min} - 1$ pour une telle boule de niveau $A_k(x)$. Si $p \geq k$, on forme une $k$-partie contenant $x$ avec des sites de $I$ et au plus le site $x$ sur la coquille : sa trace a au plus un site, elle est séparable. Sinon $t = k - p \leq q_{\min} - 1$ et $I \cup A$, avec $A \ni x$ si $x \in U$, a une trace de moins de $q_{\min}$ sites, séparable. Dans les deux cas cette partie contient $x$ et a un rayon carré strictement inférieur à $A_k(x)$ (lemme 1) : contradiction. (c) Si une composante $C$ de $L_k(A)$ couvre $x$ au niveau $A = A_k(x)$, il existe $y \in C$ à distance au plus $\sqrt{A}$ de $x$ ; $x$ et $k - 1$ autres sites de la boule fermée de centre $y$ forment une $k$-partie $F$ dont la région témoin contient $y$, donc un sommet de $C$ ; $\beta(F) \leq A$, et $\beta(F) \geq A$ par définition de $A_k(x)$. Sa plus petite boule est un témoin de niveau $A$ contenant $x$, dont le centre est dans $C$. Réciproquement le centre d'un témoin est dans $L_k(A)$ à distance $\sqrt{A}$ au plus de $x$. (d) $p + q \leq k \leq p + m = p + q$ ; la cellule $(b, k)$ est la naissance, isolée à son niveau (théorème B). $\square$

---

## Annexe B — Contrôles exécutés pour cette conception

Dossier `/workspaces/E-HGP/build/v11-persist/conception/preuves_tour/`. Exécutions du 2 octobre 2026 entre 08:00 et 08:45 UTC, codespace de 8 cœurs à charge 19 à 25, un processus à la fois, quelques secondes chacune.

| Pièce | Ce qu'elle fait | Résultat |
| --- | --- | --- |
| `noyau_v11.cpp`, `noyau_v11.log` | noyau du § 4.1 (événements, attaches, `jtop`, préchargement), matérialisation, historique, images, contre une copie du Kruskal par lots de la v10 ; entrées : `/tmp/v11-audit/l06_code_tour/krdump/kruskal_k{1..5}.bin` et `krdump10/kruskal_k10.bin` (trame 00, vidages de l'audit L06 ; sha256 de `k5` : `d313127a…`, de `k10` : `362b4416…`) | six ordres : tableaux `rank` et `parent` identiques ; fusions 39 796, 77 038, 119 682, 172 168, 235 290, 659 223 ; événements = naissances − 1 ; 2 444 942 images de jonction concordantes ; rapport de temps 2,49 à 2,90 ; 0,74 à 0,78 pas de compression par `find` |
| `foret_check.py` | même chaîne en Python sur 6 000 hypergraphes aléatoires à rangs très répétés (2 à 40 naissances) | 0 écart sur 152 415 nœuds dont 15 658 fusions d'au moins trois enfants, 1 588 454 requêtes `component_at`, 194 481 images |
| `quotient_check.py` | quotient par ensembles séparables maximaux contre la définition brute ; coquilles de 3 à 9 sites tirées de sphères et de cercles entiers, du cube, de l'octaèdre, et mixtes | 5 700 contrôles (coquille, $t$), 0 écart ; 1 369 naissances, 1 415 jonctions, 2 916 cellules à un morceau |
| `quotient_check2.py` | le même sur des coquilles de 10 à 13 sites ; puis coût seul sur 24 et 30 points cosphériques et sur des cercles de 12, 16, 24 points | 237 contrôles, 0 écart ; 116 et 176 ensembles ; 0,054 s et 0,16 s de Python |
| `cover_temoins_check.py` | proposition T8 : entrée `cover` (niveau et ensemble des nœuds) lue sur les seuls témoins, nœud de naissance pour un témoin régulier, contre l'étage de définition de `morsehgp3D_v11/reference` ; 60 nuages de 6 à 9 points (génériques, grilles, plans, quasi-alignés), ordres 2 à 4 | 1 317 entrées dont 216 à plusieurs composantes, 0 écart ; 1 175 témoins réguliers, tous des naissances de l'ordre ; 678 témoins étendus |
| `controles_python.log` | sorties des scripts Python | — |

Reproduction :

```text
g++ -std=c++20 -O2 noyau_v11.cpp -o noyau_v11 && ./noyau_v11 kruskal_k5.bin 7      # code 0 si forets et images egales
python3 -B quotient_check.py ; python3 -B quotient_check2.py ; python3 -B foret_check.py 5 3000
```

Ce que ces contrôles n'établissent pas : aucun temps sur G4 ; rien sur l'étage G de la v11 (ses coûts unitaires sont supposés) ; rien sur les trames 01 et 02 pour le noyau (les vidages n'existent que pour la trame 00) ; le quotient local n'est jugé contre la définition que jusqu'à 13 sites.


Fin de rédaction : 2026-10-02 08:39:48 UTC (`date -u`). Note de contre-lecture pour les auditeurs : `A_CONTRE_LIRE_TOUR_20261002.md`, même dossier.
