# Critique indépendante : géométrie du polyèdre des supports (décisions 2 et 3)

4 octobre 2026, 19 h 25 UTC (heure lue par `date -u`). Rôle : avocat indépendant. Lecture seule : rien construit,
modifié ni commité ; seule écriture, ce fichier. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

Sources : moteur `build/v11-claude-20261003/morsehgp3D_v11` à `57dd21be1` (chemins `src/…`, `bench/…`, `docs/…`
relatifs à ce dossier) ; reçus et Zoltan dans `main` à `1bf4be68f` (chemins depuis la racine du dépôt). Dossier du
workflow : `build/v11-persist/sortie_supports/` (cinq lectures, deux conceptions, `SPECIFICATION_FINALE.md`, citée
« spec »). La thèse est citée par ses pages imprimées.

---

## 0. Verdict

1. **Décision 2 ($\mathcal{Q}_b$) : l'utilisateur a raison**, mais la spec en tire un format trop pauvre. $\mathcal{Q}_b$
   est la seule réalisation par supports qui soit canonique ; elle ne coûte rien. Il faut cependant publier **avec**
   elle la coquille entière $U_b$, en codant $\mathcal{Q}_b$ par des **masques sur les positions de $U_b$**, comme le
   fait déjà `mark_supports` (`bench/catalogue_euler.hpp:137-165`). Sans $U_b$, le site orphelin des coquilles
   $m=3$, $q=2$ disparaît, et l'autre réalisation que demande `JETON.md` (`conv(U_b)`, l. 78-81) n'est plus
   calculable.
2. **Plus fort : publier la population entière $P_b=I_b\cup U_b$.** Je démontre (lemme H, § 3) qu'avec le
   rattachement de la spec, l'ensemble des points du $K$-polyèdre de la Déf. 21 de la thèse (p. 58), daté, est
   exactement $\bigcup\lbrace P_b : b\in W_K,\ \mathrm{att}(b)\preceq v,\ \lambda_b\leq a\rbrace$.
   - Aucune des quatre sorties prévues ne rend aujourd'hui cet objet, qui est celui de la thèse (HGP-Clusterer,
     Déf. 22).
   - Le tokenizer le réclame : `SPECIFICATION.md:321-323` (populations en CSR), `:44-56` (masses du § 9.1, qui
     exigent l'identité des cofaces de Gabriel, donc $I_b$), `JETON.md:57-59` (le « socle » des retours observés).
3. **Décision 3 (toutes les boules de $W_K$) : l'utilisateur a raison, et les faits le confirment fortement.**
   - Les boules de rôle interne font 27 % de $W_5$ sur ng00 (mesuré, § 1).
   - À $K=1$, elles sont le graphe de Gabriel hors arbre couvrant minimal, soit environ 60 % des boules.
   - « Événements seuls » donnerait un squelette **arborescent**, sans cycle, et perdrait la croissance de couverture
     de P3 (`docs/MATHEMATIQUES.md:277-278`).
4. **Le périmètre $W_K$ est le bon, mais la spec le justifie mal.** Une boule hors fenêtre change $\Gamma_K$ : elle
   ajoute des sommets et des liaisons non-Gabriel. Elle ne change ni $\pi_0$ ni, par le lemme H, les ensembles de
   points. Erratum de la spec, § 2.2, l. 217.
5. **Supports propres par nœud, avec postordre (spec)** : bon choix, je n'ai rien de mieux. Le polyèdre cumulé stocké
   coûterait $O(B\cdot\text{profondeur})$.
6. **« Un même support pour de nombreuses liaisons » : les faits vont contre cette prémisse, à $K$ fixé.**
   - Une liaison est une $(K+1)$-partie : `MATHEMATIQUES.md` § 4 et Prop. 5, p. 86.
   - Un support d'une boule régulière de $W_K$ en porte **exactement 1** (jonction) ou **0** (naissance). Les
     exceptions sont les coquilles cosphériques : 49 à 239 boules par trame à $K=5$.
   - Le « nombreuses » est vrai **à travers les ordres** : l'intervalle $[Q,P_b]$ compte $2^{p+m-\lvert Q\rvert}$
     simplexes de même boule minimale. Il est vrai aussi pour les boules non-Gabriel, exclues du produit.
7. **Taille.**
   - Format v1 de la spec : environ 84 Mo par trame à $K=5$ et 220 Mo à $K=10$ (ng00 ; $K=5$ à partir de comptes
     mesurés, $K=10$ extrapolé).
   - Format sobre avec populations complètes : environ 39 Mo et 145 Mo, **plus petit tout en contenant
     strictement plus**. Avec $U_b$ seul : environ 31 Mo et 84 Mo.
   - L'écart vient du stockage de champs dérivables : binômes, `components`, postordre, enfants, et sites de
     supports complétés à quatre.

---

## 1. Faits mesurés et vérifiés

| Fait | Valeur | Source |
| --- | --- | --- |
| $\lvert W_5\rvert$ (`work.cells` à l'ordre 5) | 789 886 / 652 958 / 832 386 (ng00/01/02) | `morsehgp3D_v11/receipts/audit_deep_20261004/performance/selected/c40_paired/results/cmd/002_paired_full/files/full_paired.json` |
| Naissances, nœuds à $K=5$ (ng00) | 341 081 naissances ; 576 371 nœuds ; fusions 235 290 | idem (`semantic.orders`) |
| Continuations à $K=5$ (ng00) | 212 858 | idem ; compteur `forest_plateau.cpp:78` |
| Rôles dans $W_5$ (ng00) | naissance 341 081 (43 %) ; fusion ≥ 235 290 ; interne ≥ 212 858 ; somme des minorants 448 148 sur 448 805 non-naissances | arithmétique sur les lignes ci-dessus |
| $K=1$ (ng00) | 101 138 boules ; 39 796 fusions ; 60 062 continuations | idem |
| Coquilles étendues dans $W_5$ | 107 / 49 / 239 (`parallel.extended_cells`) | idem |
| Supports en sus de $S^*$ ($\mathrm{Cat}_7$) | 6 / 2 / 13 boules en tout (326−320, 206−204, 878−865) | `build/v11-local/euler_tmp/lidar_ng0*.k5.json` (local, non reçu) |
| Répartition par $m$ des étendues ($\mathrm{Cat}_7$, ng00) | 313 / 2 / 5 pour $m$ = 3 / 4 / 5 | idem |
| $q_{\min}$ moyen | 2,96 ($\mathrm{Cat}_7$) ; 3,25 ($\mathrm{Cat}_{12}$) | idem, `by_qmin` |
| Nœuds $K=5\to10$ (c08_000038) | 479 374 → 1 265 007 (×2,64) | lecture `points`, l. 131-133 (session F) |

Code vérifié :
- fenêtre : `src/tower/forest_build.cpp:150` ;
- population « I croissant puis U croissante » : `src/catalogue/catalogue.hpp:161-167`. Elle est disponible sans
  calcul ;
- `birth_key` seulement : `src/tower/forest.hpp:12` ;
- prédicats : `src/num/predicates.cpp:216-240,305-309` ;
- masques de supports : `bench/catalogue_euler.hpp:110-165` ;
- FULL ne publie aucune population : `bench/full_probe.cpp:40-75`.

---

## 2. Quelle géométrie pour une boule : $S^*$, $\mathcal{Q}_b$, `conv(U_b)`, sommets ajoutés

### 2.1 $S^*$ seul : à écarter

- $S^*$ est le premier support pour l'ordre des `SiteIdx`, c'est-à-dire des rangs de Morton (`MATHEMATIQUES.md:24-28`,
  `src/cloud/cloud.hpp:6`). Il est donc invariant par réétiquetage des `PointId` et par permutation de l'entrée :
  l'objection de `JETON.md:49-53` (« selon les IDs ») ne tient plus en v11.
- Mais l'ordre de Morton n'est **pas invariant par translation entière** ni par les symétries de la grille.
  - Exemple en 2D, bit $x$ de poids faible : $(1,0)\prec(0,1)$ ; après translation de $(1,0)$, on obtient
    $(2,0)\succ(1,1)$.
  - Dans le carré, la diagonale retenue bascule donc sous une translation, alors que FULL ne change pas.
- Or le tokenizer recalcule la tour par vue (`SPECIFICATION.md:308-317`). $S^*$ y introduirait une dépendance
  artificielle à la position absolue.

### 2.2 $\mathcal{Q}_b$ : la bonne réalisation par supports

- C'est un ensemble géométrique, invariant par translation, par symétries de la grille et par réétiquetage.
- Son coût est nul en pratique : 21 boules par trame ont plus d'un support.
- Son seul défaut est d'être **discontinu** : saut de Hausdorff d'au moins $1/4$ dans
  `morsehgp3D_v11/receipts/audit_supports_20261004/carrier/README.md`.

### 2.3 `conv(U_b)` : plus stable que $\mathcal{Q}_b$, et pourtant écarté

- Dans le contre-exemple du *carrier*, $U_b=\lbrace A,B_t,C,D\rbrace$ reste le même ensemble et bouge continûment.
  Le quadrilatère $AB_tCD$ varie donc **continûment**, alors que $\mathcal{Q}_b$ saute de $\lbrace AC,BD\rbrace$ à
  $\lbrace AC,B_tCD\rbrace$.
- `conv(U_b)` est donc, sur ce témoin, **plus robuste** que la réalisation choisie. Il est discontinu seulement quand
  $U_b$ lui-même change, et alors la boule et FULL changent aussi.
- Sur les coquilles régulières (toutes sauf 49 à 239 boules par trame), $\mathrm{conv}(U_b)=\mathrm{conv}(S^*)$ : les
  deux réalisations coïncident.

### 2.4 Le site orphelin n'est pas un détail

Soit une coquille $m=3$, $q=2$ : $U=\lbrace a,b,o\rbrace$, $ab$ diamétral, et $o$ sur la sphère (angle droit en $o$).
C'est le cas de 313 étendues sur 320.
- À l'ordre $K=p+2$, les deux traces strictes sont $I\cup\lbrace a,o\rbrace$ et $I\cup\lbrace b,o\rbrace$ : elles
  **contiennent toutes deux $o$**.
- L'unique liaison est $P_b=I\cup\lbrace a,b,o\rbrace$, et son support est $ab$.
- Le squelette dessine $ab$ et omet le seul site que partagent les deux sommets reliés.

C'est la même chose à plus grande échelle pour les intérieurs, qui sont la masse $K$-NN de la boule
(`JETON.md:82`).

### 2.5 Recommandation

- **Garder $\mathcal{Q}_b$ comme réalisation déclarée.** C'est le choix de l'auteur, dessiné dans
  `Zoltan/PolyhedralEncoding/Presentation_2026-09-16_Inria_SZTE/README.md` (« colorer les supports, pas l'enveloppe
  convexe »).
- **Publier $U_b$ entier**, et coder $\mathcal{Q}_b$ par des masques `u32` sur les positions de $U_b$. Pour une
  coquille régulière, le masque est implicite (« tout $U_b$ »), donc aucune table.
- Le lecteur dérive alors, sans arithmétique exacte, les réalisations « supports », `conv(U_b)` et « supports plus
  sommets de coquille ». Chacune reçoit son identifiant de construction, comme le demande `JETON.md:80-88`, et
  l'ablation de `JETON.md:203-209` tranche.
- C'est le moteur qui doit décider l'exact ($\mathcal{Q}_b$). Ce n'est pas à lui de **choisir** la réalisation.

---

## 3. Lemme H : la population des boules de $W_K$ donne exactement les $K$-polyèdres de la thèse

**Énoncé.** Soient $K\geq2$, un nœud $v$ de $T_K$ et une coupe fermée $a\in[a_v,a_{\mathrm{parent}(v)})$. On note
$C$ la composante de $\Gamma_K(a)$ représentée par $v$, et $\mathrm{pts}(C)$ les sites qui apparaissent dans un sommet
de $C$, c'est-à-dire le $K$-polyèdre de la Déf. 21 (p. 58). Alors

$$\mathrm{pts}(C)=\bigcup\lbrace P_b : b\in W_K,\ \mathrm{att}(b)\preceq v,\ \lambda_b\leq a\rbrace,$$

et l'union peut se restreindre aux boules **fortes** ($p+q\leq K\leq p+m$). À $K=1$, $\mathrm{pts}(C)$ est l'ensemble
des feuilles-sites du sous-arbre.

**Preuve.**
- *Inclusion $\supseteq$.* Soit $x\in P_b$. Comme $\lvert P_b\rvert\geq K$, $x$ est dans une $K$-partie de $P_b$. Celle-ci
  est un sommet de la composante de $\mathrm{att}(b)$ à $\lambda_b$ (lemme A de la spec), donc de $C$ à $a$.
- *Inclusion $\subseteq$.* Soit $x\in\mathrm{pts}(C)$. On choisit $F'\ni x$, sommet de $C$, de niveau $\beta$ minimal,
  et l'on pose $b'=B(F')$.
  - Supposons $p'+q'\geq K+1$. On forme $F''\ni x$ avec au plus $q'-1$ sites de coquille, complétée par des
    intérieurs. C'est possible :
    - si $x\in I'$, il faut $p'+q'-1\geq K$ ;
    - si $x\in U'$, on prend $x$, $q'-2$ autres sites de coquille et $K-q'+1\leq p'$ intérieurs.
  - Sa trace est séparable, puisque toute partie non séparable a au moins $q'$ sites. Par T2, $\beta(F'')<\lambda_{b'}$.
  - Comme $\lvert P_{b'}\rvert\geq K+1$, T3 relie $F''$ à $F'$ à la coupe fermée $\lambda_{b'}\leq a$, donc
    $F''\in C$. C'est une contradiction avec la minimalité.
  - Donc $p'+q'\leq K\leq p'+m'$ : $b'$ est forte, et $x\in P_{b'}$. Enfin $\mathrm{att}(b')$ est le nœud de $F'$ à
    $\lambda_{b'}$, dont l'ancêtre fermé à $a$ est $v$.

C'est l'argument de P3 (`MATHEMATIQUES.md:263-278`), appliqué aux sommets au lieu de la couverture. ∎

**Conséquences.**
1. Avec $P_b$ publié, la sortie « supports » rend **l'objet de la thèse** : les $K$-polyèdres datés de la Déf. 22,
   recouvrements entre branches compris. Ni FULL (`bench/full_probe.cpp:40-75`, aucune population), ni « points »
   (projection laminaire), ni « plat » ne le rendent.
2. Le lemme corrige la justification du périmètre (§ 4.2) : une boule hors de $W_K$ n'ajoute **aucun point** à un
   $K$-polyèdre.
3. **Tokenizer.**
   - `SPECIFICATION.md:321-323` demande « les populations en CSR de sites uniques ».
   - Les masses du § 9.1 (`SPECIFICATION.md:44-56`) sont $S_\tau=\sum_{\sigma\supset\tau}\psi(\rho(\sigma))$ sur les
     cofaces de Gabriel. Une coface de Gabriel est $\sigma=I_b\cup A$, avec $A\subseteq U_b$ qui contient un support.
     Il faut donc l'**identité** de $I_b$ et de $U_b$ ; le seul compte $p$ ne suffit pas. Le texte le dit lui-même :
     « Les seules populations FULL ne suffisent pas » (l. 46).
   - Le canal « retours observés », le socle de `JETON.md:57-59`, est $\mathrm{pts}(C)$.
4. **Coût de calcul nul.** Le catalogue tient déjà les populations en CSR (`catalogue.hpp:161-167`). On les recopie.

---

## 4. Toutes les boules de $W_K$, contre les seuls événements

### 4.1 Pour $W_K$ entier (décision 3)

- **Mesure** (ng00, $K=5$). Les boules internes sont au moins 212 858, soit 27 % de $W_5$ et 47 % des non-naissances.
  À $K=1$, il y a 60 062 continuations sur 101 138 boules : le moteur y publie le **graphe de Gabriel** (boules
  $p=0$, $q=2$), rangé par le dendrogramme du simple lien. Les fusions en sont l'arbre couvrant minimal, et les
  internes sont les arêtes qui ferment des cycles.
- **Avec les seuls événements**, $P_v$ serait la réunion d'une naissance et de supports de pont. C'est un objet
  arborescent : il ne peut jamais fermer une face, ni une boucle de surface.
- P3 : « Les continuations étendues peuvent augmenter la couverture sans créer de nœud » (`MATHEMATIQUES.md:277-278`).
  La fixture `growth_ABCZ` (spec, § 2.9, n° 3) perdrait Z.
- Coût des internes : $+27\,\%$ de boules, sans descente, puisque le journal les voit (spec, § 2.4).

### 4.2 Au-delà de $W_K$ : ne pas y aller en v1, et corriger la justification

- **Boules non-Gabriel** ($p+q\geq K+2$, hors de $\mathrm{Cat}_K$).
  - Elles ajoutent des sommets et des liaisons à $\Gamma_K$ entre des parties déjà connexes. Elles ne changent ni
    $\pi_0$ (T3) ni $\mathrm{pts}(C)$ (lemme H).
  - Géométriquement, ce sont des cordes qui traversent la masse intérieure : les omettre est souhaitable, pas
    seulement forcé.
  - La phrase de la spec (l. 217), « Une boule hors de $W_K$ ne change pas $\Gamma_K$ », est fausse telle quelle. Il
    faut écrire : « ne change ni les composantes de $\Gamma_K$ ni les ensembles de points des $K$-polyèdres ».
- **Événements de dimension supérieure.**
  - Par J3 (`MATHEMATIQUES.md:350-358`), une boule régulière de $p+q=K+2$ ou $K+3$ a
    $e_K(b)=(-1)^{q-t}\binom{q-1}{t-1}\neq0$, avec $t=K-p\geq1$. C'est pour cela que le juge d'Euler lit
    $\mathrm{Cat}_{K+2}$ (`docs/CATALOGUE.md:190-196`).
  - Si cette somme datée est bien la caractéristique d'Euler de $L_K(a)$, comme le juge le suppose, ces boules créent
    ou referment des boucles et des cavités de $L_K$ sans toucher $\pi_0$.
  - Elles n'entrent pas dans la liste de l'utilisateur (naissance, fusion, interne). Les ajouter coûterait
    $\mathrm{Cat}_{K+2}$ (×1,96 boules à $K=5$, ×1,51 à $K=10$ sur ng00) et des descentes, car le journal ne les voit
    pas.
  - C'est une extension à documenter, **pas** pour v1.

---

## 5. Supports propres par nœud, contre polyèdre cumulé

- La spec publie les boules propres, triées par postordre de leur nœud (§ 6.3, l. 776-786). $P_v$ et chaque instantané
  daté y sont des **tranches contiguës**.
- C'est strictement mieux que stocker le cumul, qui coûterait $\sum_v\lvert\text{sous-arbre}\rvert$.
- Je confirme les deux points délicats :
  - les boules du sous-arbre strict ont un rang $<a_v$ (une interne d'un enfant $w$ vérifie $\lambda_b<a_{\mathrm{parent}(w)}=a_v$) ;
  - une boule **passagère** (rôle fusion, une seule branche) appartient au parent, ce qui est cohérent avec la coupe
    fermée.
- Une remarque pour le tokenizer : avec $P_b$ publié, $\mathrm{pts}(C)$ est l'union **dédupliquée** d'une tranche.
  Cette déduplication est du ressort du lecteur.

---

## 6. Les liaisons : définition et comptage

**Définition normative.** Les sommets de $\Gamma_K$ sont les $K$-parties ; chaque $(K+1)$-partie $G$ de niveau
$\leq a$ relie ses faces (`MATHEMATIQUES.md` § 4, l. 138-140 ; Prop. 5, p. 86). Une **liaison est une $(K+1)$-partie**,
pas une $K$-partie. La lecture « $K$-parties contenant le support » compte des sommets reliés, pas des liaisons.

**Ce que valent les comptes sur $W_K$, à $K$ fixé.**

| Boule | `cofaces` par support | $K$-parties reliées $\binom{p+m}{K}$ | Couples de sommets reliés (Déf. 21 non élaguée) |
| --- | --- | --- | --- |
| jonction régulière $p+q=K+1$ | **1** (la seule liaison est $P_b$, de Gabriel) | $K+1$ | $\binom{K+1}{2}$ |
| naissance régulière $p+q=K$ | **0** | 1 | 0 |
| coquille étendue (49 à 239 par trame, $K=5$) | $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$, éventuellement $>1$ | $\binom{p+m}{K}$ | — |

- **Les faits vont contre la prémisse** « un même support peut correspondre à de nombreuses liaisons ». À $K$ fixé et
  sur les boules qui comptent, c'est 0 ou 1, sauf pour quelques centaines de coquilles cosphériques.
- La prémisse est vraie dans trois cas :
  1. **à travers les ordres** : toutes les parties $G$ telles que $Q\subseteq G\subseteq P_b$, soit
     $2^{p+m-\lvert Q\rvert}$ simplexes, ont la même boule minimale $b$ (M2) ; c'est l'intervalle de Morse de $b$ ;
  2. **hors fenêtre** : une boule non-Gabriel de $p+q\geq K+2$ porte $\binom{p}{K+1-q}$ liaisons par support ;
  3. au sens des **sommets reliés** : $K+1$ à chaque jonction.
- Si l'utilisateur pensait à « beaucoup », c'est le sens 1 ou le sens 3 qu'il faut nommer.
- **Recouvrement quand $\lvert\mathcal{Q}_b\rvert>1$.** La somme par support compte des incidences $(Q,G)$, et
  `cofaces(b)` compte les liaisons distinctes ; la spec le dit (§ 2.6). Cela concerne 21 boules par trame au total.
- **Recommandation.**
  - Ne **stocker aucun** de ces comptes : `k_parts`, `cofaces` par support, `cofaces(b)`, `strict_traces`,
    `components`. Tous sont des fonctions de $(p,m,\lvert Q\rvert,K)$, des masques ou de la longueur de `PRIOR`.
  - Le lecteur les recalcule par une table de binômes et une fermeture zêta sur au plus $2^{m}$ masques, triviale
    pour $m\leq5$.
  - Le produit garde ses contrôles internes I5 et I6 (journal contre $\binom{m}{t}-N_t$), mais ne les sérialise pas.
- **Ce que le tokenizer consomme** : des cofaces **de Gabriel** avec leur identité et leur niveau (§ 9.1). Ce ne
  sont pas des comptes, et cela renvoie au § 3.

---

## 7. Taille de sortie

Bases : ng00 (39 885 sites). À $K=5$ : $\lvert W_5\rvert$ = 789 886 et $N$ = 576 371, mesurés. À $K=10$ :
- $N\approx1{,}52$ M, soit ×2,64 comme sur c08_000038 ;
- $\lvert W_{10}\rvert\approx2{,}08$ M, soit le rapport $\lvert W\rvert/N=1{,}37$ de $K=5$. C'est cohérent avec
  $n_{10}+n_{11}$ estimé par différences de catalogues, entre 2,0 et 2,1 M ;
- environ 0,98 M naissances.

**Ce sont des estimations, pas des mesures.**

| Format | Octets | $K=5$ | $K=10$ |
| --- | --- | ---: | ---: |
| spec v1 (`MHGP11SP`, l. 776-778, 797) | 41/nœud + 52/boule + 21/support + 4/branche | ≈ 84 Mo | ≈ 220 Mo |
| sobre, $U_b$ seul | 17/nœud + 12/boule + $4m$ + 4/branche + masques des étendues | ≈ 31 Mo | ≈ 84 Mo |
| **sobre, $P_b$ entier** | 17/nœud + 12/boule + $4(p+m)$ + 4/branche + masques | **≈ 39 Mo** | **≈ 145 Mo** |
| entrée, pour mémoire | 16 octets par point | 0,64 Mo | 0,64 Mo |

**Champs redondants de la v1.**
- `child_off` et `children` se déduisent de `parent`.
- `post` et `size` s'obtiennent en $O(N)$.
- `node` par boule est implicite dans `ball_off`.
- `key` (`BallIdx`) est propre à l'appel et sans sens hors de lui.
- Les quatre comptes `u32` sont dérivables (§ 6).
- `cofaces` par support est dérivable.
- `sites u32[4]` est complété à quatre même pour une paire.
- `support_off` ne sert à rien pour 99,97 % des boules, dont l'unique support est $U_b$.

**Ordre de grandeur du corpus.** Les 19 130 trames d'entraînement de SemanticKITTI représentent environ 1,6 To (v1,
$K=5$) ou 4,2 To (v1, $K=10$) **par vue**, contre 0,75 To et 2,8 To pour le format sobre avec populations. La spec
recalcule la tour par vue (`SPECIFICATION.md:308-317`). Le tokenizer consommera donc l'API en mémoire
(`SupportHierarchy`) plutôt que des fichiers, et le format sert surtout aux reçus et aux lecteurs. Raison de plus pour
le garder sobre **dès la v1** plutôt que de prévoir une « version 2 compacte » (spec, l. 1204). Par défaut, le
tokenizer prend $K\in\lbrace1,2,3,5\rbrace$ ; c'est la mesure qui décidera si les ordres 7 à 10 servent
(`SPECIFICATION.md:300-302`). C'est donc $K=5$ qui dimensionne le premier pilote, et $K=10$ reste l'extension.

---

## 8. Format proposé (différence avec `MHGP11SP` v1)

| Section | Contenu |
| --- | --- |
| `SITES` | inchangé (`x`, `y`, `z`, `point_id`) |
| `NODES` | `parent u32`, `rank u32`, `kind u8`, `ball_off u64[N+1]` (postordre). Le lecteur reconstruit enfants, `post` et `size`. |
| `BALLS` | ordre (postordre du nœud, rang, clé canonique) : `rank u32`, `role u8`, `p u8`, `m u8`, `qmin u8`, `prior_count u32`. Les décalages de population et de `PRIOR` sont des sommes préfixes de $p+m$ et de `prior_count`, publiables si l'accès direct l'exige. |
| `POP` | $\sum(p+m)$ lignes de `SITES` : $I_b$ croissant puis $U_b$ croissante, recopié de `catalogue.hpp:161-167`. Le premier support lexicographique de $U_b$ est $S^*$ et redonne le niveau exact (spec, l. 788-795). |
| `SUPPORTS` | **pour les seules coquilles étendues** : (ordinal de boule, nombre, masques `u32` sur les positions de $U_b$). Une coquille régulière a implicitement $\mathcal{Q}_b=\lbrace U_b\rbrace$. |
| `PRIOR` | inchangé |

- Aucune option : la section `POP` est toujours présente, ce qui respecte la règle 6 (`docs/ARCHITECTURE.md:21-22`).
- Les invariants I1 à I11 de la spec s'appliquent tels quels. S'y ajoute un juge d'échantillon du lemme H : sur
  quelques nœuds, comparer $\mathrm{pts}(C)$ à l'oracle borné S1, puis, à l'échelle, contrôler l'égalité avec
  l'union des incidences fortes de `MHGP11PH`.

---

## 9. Ce qui reste ouvert

- **Stabilité.** Aucune réalisation n'est stable en général. `conv(U_b)` et $\mathrm{pts}$ résistent au témoin du
  *carrier* ; $\mathcal{Q}_b$ n'y résiste pas. Le format proposé laisse le choix à l'ablation au lieu de le figer.
- **Coût d'écriture.** 39 à 145 Mo par trame, soit plusieurs dizaines de millisecondes d'écriture. Hors contrat de
  100 ms ; à mesurer en S7.
- **H1/H2** (§ 4.2) : extension possible, sur $\mathrm{Cat}_{K+2}$ et avec des descentes, non demandée.
- Les comptes du § 1 viennent d'un reçu à u21 et, pour la répartition par $m$, de sorties locales non reçues. Les
  valeurs à $K=10$ sont extrapolées.
