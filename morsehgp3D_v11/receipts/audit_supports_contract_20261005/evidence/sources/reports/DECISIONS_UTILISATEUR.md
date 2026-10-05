# Décisions de l'utilisateur et plan retenu — sortie paramétrée et hiérarchie des supports

4 octobre 2026, environ 20 h UTC. Ce document **prime** sur `SPECIFICATION_FINALE.md` et sur
`CRITIQUE_ET_PLAN_REVISE.md` partout où ils diffèrent. Il fixe les réponses de l'utilisateur et ce qu'elles changent.

## 1. Les réponses

Premier tour (questions de conception) :

1. **Tout natif** : façade C++ `api` et exécutable `mhgp11 --sortie=full|supports|points|plat`. État final tout
   natif ; livraison par étapes (voir § 3), une sortie non encore livrée est refusée `parameter_out_of_range`.
2. **Tous les supports positifs minimaux $\mathcal{Q}_b$** de chaque boule, énumérés sur **toute** la coquille $U_b$
   (jamais seulement jusqu'à $q_{\min}$) ; plafond de coquille 24 avec refus explicite au-delà.
3. **Toutes les boules d'événement d'ordre K**, $W_K=\lbrace b\in\mathrm{Cat}_K : p+q_{\min}-1\leq K\leq p+m\rbrace$
   (naissances, fusions, liaisons internes) ; chaque nœud publie ses boules propres ; $P_v$ est l'union sur le
   sous-arbre.

Message ultérieur : « Mes réponses n'étaient pas intangibles, si le workflow va contre moi, étudie avec esprit
critique les différentes possibilités. » L'examen critique (`CRITIQUE_ET_PLAN_REVISE.md`) a confirmé 1 à 3, et
l'utilisateur a répondu au second tour :

4. **Populations : NON.** La sortie `supports` publie le **squelette $\mathcal{Q}_b$ seul**, sans $P_b=I_b\cup U_b$
   (ni section `POP`). Conséquence : les sites de chaque support sont publiés **explicitement** (une coquille
   régulière ne se déduit plus de $U_b$).
5. **« Liaisons » = K-parties reliées.** Le compte mis en avant s'appelle `kparties_reliees` ; il **remplace le nom**
   `k_parts` de la spécification (§ 2.6, lemme G), sans changer sa valeur :
   $\mathrm{kparties\_reliees}(b)=\binom{p+m}{K}$, le nombre de $K$-parties de $P_b$. Sens : ce sont les $K$-parties
   dont les cellules du recouvrement d'ordre $K$ contiennent toutes le centre $c_b$ au niveau $\lambda_b$, toutes
   reliées entre elles à la coupe fermée (lemme A, par T3). Il vaut $K+1$ à une jonction régulière, 1 à une naissance
   régulière (la $K$-partie qui naît) et davantage sur les coquilles cosphériques (carré à K3 : 4). Il ne dépend que de
   $(p,m,K)$, **pas de $\mathcal{Q}_b$** : il échappe donc à l'instabilité des supports (cercle à quatre points).
   La décision 11 de la spécification (« nombre de liaisons par défaut : `cofaces` ») est remplacée par celle-ci ;
   `cofaces` (les liaisons de la thèse, Prop. 5) reste dérivé sous son nom, avec `strict_traces`, `compressed_parts`,
   `components` (`strict_global_components`) et les comptes de Gabriel, noms de l'audit `1bf4be68f`. Aucun compte
   n'est stocké dans le fichier (§ 2) : l'API C++ et le lecteur les calculent, le manifeste publie des agrégats.
6. **Ordre de livraison : supports, puis points, puis plat** ; `plat` reportable par décision écrite.

## 2. Format `MHGP11SP` version 1 (normatif, remplace § 6.3 de la spec et § 2.1 de la critique)

Principe de sobriété : ne stocker que ce qui ne se dérive pas. Petit-boutiste, colonnes séparées (structure de
tableaux), chaque colonne alignée sur 8 octets, bourrage nul, comme § 6.1 de la spec. Ordres canoniques du § 6.1 de
la spec. Pas de population (décision 4).

| Section | Colonnes stockées |
| --- | --- |
| En-tête | magie `MHGP11SP`, puis des `u64` : `version=1`, `coord_bits`, `k`, `n` (sites), `N` (nœuds), `root`, `B` (boules), `S` (supports), `Z` (somme des arités), `A` (branches publiées), décalages des sections, taille totale |
| `SITES` | `x u32[n]`, `y u32[n]`, `z u32[n]`, `point_id u32[n]`, dans l'ordre des `SiteIdx` (Morton) |
| `NODES` (numérotation canonique de la forêt d'ordre K de FULL) | `parent u32[N]` (`kNone` à la racine), `rank u32[N]`, `kind u8[N]` (0 feuille-site à K = 1, 1 naissance de boule, 2 fusion), `ball_count u32[N]` (boules propres) |
| `BALLS` (`B`, triées par postordre du nœud de rattachement, puis rang, puis $S^*$ lexicographique) | `rank u32[B]` ($\lambda_b$), `prior_count u32[B]` (taille de $\mathrm{ant}(b)$ publiée : rôle fusion seulement, 0 sinon), `support_count u16[B]` ($\lvert\mathcal{Q}_b\rvert\leq 12\,926$), `role u8[B]` (0 naissance, 1 fusion, 2 interne), `p u8[B]`, `m u8[B]` |
| `SUPPORTS` (`S`, par boule dans l'ordre (arité, lexicographique des `SiteIdx`), donc $S^*$ en tête) | `arity u8[S]`, puis `sites u32[Z]` (lignes de `SITES`, croissantes dans chaque support) |
| `PRIOR` (`A`) | `node u32[A]` : $\mathrm{ant}(b)$ en numérotation canonique, croissant, pour le rôle fusion seulement |

**Dérivé, jamais stocké** (API C++ et lecteur en bibliothèque standard) : enfants (par `NodeIdx` croissant), postordre,
taille de sous-arbre, décalages (sommes préfixes de `ball_count`, `support_count`, `arity`, `prior_count`), $q_{\min}$
(arité du premier support), le nœud de rattachement de chaque boule, la ligne de site des feuilles à K = 1 (la feuille
$i$ est le site de rang $i$ dans l'ordre lexicographique $(x,y,z)$, à vérifier sur `forest_build.cpp:59-69`), les
niveaux exacts (rayon carré circonscrit du premier support ; niveau d'un nœud = celui de sa première boule propre ;
feuille de K = 1 : 0), `kparties_reliees`, `cofaces` par boule et par support, `strict_traces`, `compressed_parts`,
`components`, comptes de Gabriel, `has_support_geometry` (`kind != 0`). Le lecteur contrôle : arbre bien formé,
rangs croissants vers la racine, boules triées, première boule propre au rang du nœud, supports positifs de la
sphère de $S^*$ (centre dans l'intérieur relatif, prédicats exacts en `Fraction`), ordre des supports, rôle cohérent
avec les rangs (lemme B) et avec $\mathrm{strict\_traces}=0$ pour une naissance. Il ne peut pas vérifier la
complétude de $\mathcal{Q}_b$ (coquille non publiée) : elle relève des juges bornés et natifs.

Le manifeste publie, en plus du § 6.6 de la spec : nombre de supports par arité, coquilles étendues, boules à
plusieurs supports, somme et maximum de `kparties_reliees` et de `cofaces`.

Déclarations obligatoires (contrat S0 et `docs/SORTIES.md`) : la réalisation par supports **n'est pas stable** aux
cosphéricités (contre-exemple du cercle à quatre points de l'audit `1bf4be68f`) ; elle omet les sites intérieurs et,
sur une coquille étendue, les sites de coquille hors de tout support ; elle n'est pas le $K$-polyèdre de la thèse
(ensemble de points, Déf. 21), qui reste reconstructible hors format à partir du catalogue (lemme H, P3).

## 3. Plan retenu

- **L0** (sans G4) : S0, contrat mathématique et documents (lemmes A–G, et H comme énoncé ; `docs/SORTIES.md` ; table
  des modules avec `supports` ; `ARCHITECTURE.md` ; registre des preuves ; règle de décision de L2 écrite d'avance ;
  clause de report de `plat`), puis S1, oracle borné des supports (Python, bibliothèque standard, n ≤ 12–14 :
  arbre d'ordre K, rattachements, rôles, $\mathcal{Q}_b$, K-parties reliées, branches), fixtures gravées, mutants
  de l'oracle.
- **L1** (session G4 n° 1) : S2 (**livré**, `257aabb92`), S3 (arbre d'ordre K par `build_order`, journal des
  graines, `WindowAttachment`, juge E2), S6 (module `supports` : $\mathcal{Q}_b$, comptes, assemblage).
- **L2** (session G4 n° 2) : S4 (**livré**, `f98aeed67`, avec `retract()`), S5 (`api`, `Session`, manifeste JSON
  déterministe, CLI `--sortie=full` identique à l'octet au banc), S7 (`--sortie=supports`, écrivain et lecteur
  `MHGP11SP`, mesure appariée sous la règle écrite en L0).
- **L3** : S8 (`num` : entiers à longueur utile, racines, sommes de radicaux) et S9 (`--sortie=points` natif).
- **L4** : S10 (`--sortie=plat` natif), reportable.
- **L5** : S11 (pipeline à un ordre), facultative, chantier 100 ms.
