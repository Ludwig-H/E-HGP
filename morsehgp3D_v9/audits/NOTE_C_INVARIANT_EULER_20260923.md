# Note C — invariant d'Euler par ordre K : un juge global du catalogue à l'échelle

23 septembre 2026, auditeur C. Cadre : `phase=exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Sources de la chaîne : instantané `93066733` (arbres `src` et `bench` identiques
à ceux de `4530644b`, sonde v12, et du HEAD `1f73b40d`). GCP non utilisé.
Mesures locales à priorité basse, deux fils, hôte partagé : **aucun chrono
n'est revendiqué**, seuls les comptes font foi.

## Le trou que cet invariant ferme en partie

`complete_relative` certifie la tour **relativement** au catalogue recoupé :
chaque clé émise est recensée exactement, mais une clé que le générateur
n'émet jamais n'est vue par aucun contrôle à grande taille
([état courant](ETAT_COURANT.md), [clé q4 rembourrée](COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md)).
Les oracles T2 bornés jugent l'inventaire complet jusqu'à n ≤ 14 ; au-delà, il
n'existait aucun invariant global. En voici un, gratuit, exact et déjà vérifié
sur les coupes LiDAR 8k, 16k et 32k.

## Énoncé

Soit $P$ un nuage fini de $n$ sites distincts et $d_K(x)$ la distance de $x$
à son $K$-ième site le plus proche. Le sous-niveau
$\lbrace x : d_K(x)\leq r\rbrace=\lbrace x : \lvert P\cap B(x,r)\rvert\geq K\rbrace$
est exactement le niveau supérieur $L_K(r)$ du manuscrit (texte p. 60,
définition 22 et théorème 2).

Pour une boule $B$ de centre $c$, avec $p$ sites strictement intérieurs et une
coquille $U$ de $u$ sites, posons $m=K-p$. Si $1\leq m\leq u$, la contribution
de $B$ à l'ordre $K$ vaut $e_K(B)=1-\chi(\Lambda_m)$, où $\Lambda_m$ est
l'ouvert des directions $v$ de la sphère unité telles qu'au moins $m$ sites
$x$ de $U$ vérifient $\langle v,x-c\rangle>0$ ; sinon $e_K(B)=0$. Au premier
ordre, $d_K$ décroît dans la direction $v$ si et seulement si $v$ est dans
$\Lambda_m$ : c'est le lien inférieur de $c$.

Dans le cas générique (coquille égale au support, $u=q$, centre intérieur à
l'enveloppe du support), l'indice est $\mu=p+q-K$ et
$e_K(B)=(-1)^{\mu}\binom{q-1}{\mu}$ : c'est exactement l'indice et la
multiplicité locale de Reani–Bobrowski déjà inscrits comme `theorem_external`
dans le [registre des preuves](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md)
(section 2).

| support | $m=1$ | $m=2$ | $m=3$ | $m=4$ |
| --- | ---: | ---: | ---: | ---: |
| q2 | −1 | +1 | | |
| q3 | +1 | −2 | +1 | |
| q4 | −1 | +3 | −3 | +1 |

Lecture : à $m=q$ la boule est une naissance ($+1$) ; à $m=q-1$ elle est une
**multifusion** de $q$ régions en une ($-(q-1)$) ; les $m$ plus petits
créent ou tuent des cycles et des cavités, qui ne changent pas $H_0$ mais
comptent dans $\chi$. Un triangle aigu fusionne trois lentilles disjointes à
son rayon circonscrit : sa contribution est $-2$, pas $-1$.

**Proposition (Morse–Euler par ordre).** Pour tout $1\leq K\leq n$ :
$n\cdot[K=1]+\sum_{B}e_K(B)=1$, la somme portant sur les boules minimales
(centre dans l'enveloppe convexe de leur coquille). Pour $K\geq2$, le
sous-niveau est vide aux petits rayons et contractile aux grands ; pour
$K=1$, il vaut $P$ au rayon nul, d'où le terme $n$.

**Conséquence pour le catalogue v9.** Si $e_K(B)\neq0$, le centre est dans
l'enveloppe convexe de $U$ (sinon une direction $w$ rend tous les produits
positifs et $\Lambda_m$ est étoilé autour de $w$, donc contractile) ; par
Carathéodory, $B$ est la boule minimale d'un support $S\subseteq U$ avec
$q_{\min}\leq4$, et $p\leq K-1$. Pour $K\leq K_{\max}-2$, on a donc
$p+q_{\min}\leq K_{\max}+1$ : **toute boule qui compte à ces ordres est
admissible dans le catalogue**. D'où l'invariant, nécessaire à la complétude :

$E_K=n\cdot[K=1]+\sum_{B\in\mathrm{catalogue}}e_K(B)=1$ pour $K=1,\dots,K_{\max}-2$.

Statut proposé : `proved_here` dans le cas générique (Morse–Euler appliqué
aux deux entrées Reani–Bobrowski du registre), et `conditional_theorem` pour
la formule du lien inférieur des coquilles dégénérées (preuve esquissée
ci-dessus, validée par l'oracle ci-dessous). **Cette note n'écrit pas dans le
registre** ; l'inscription revient au développeur.

## Validation indépendante

L'[oracle exhaustif](c_euler_20260923/euler_oracle.py) n'utilise aucun code v9 :
toutes les boules minimales des sous-ensembles de 2 à 4 sites en rationnels
exacts, contributions génériques par la formule, dégénérées par
l'arrangement exact de grands cercles
([`euler_degenerate.py`](c_euler_20260923/euler_degenerate.py)), puis la somme pour
**tous** les ordres $K=1..n$. Sur **1 600 nuages** (n = 4 à 10, grilles
de pas 2, 3 et 4 pour forcer les cosphéricités, et pas 50 et 1 000) :
**0 échec**, **9 924 boules dégénérées** rencontrées. Les autotests du
module figent les cas q2, q3, q4 génériques et la coquille q2 + 1 point
(contributions $0,-1,+1$).

## Application au catalogue v9 sur coupes LiDAR sans sol

Harnais [`euler_check.cpp`](c_euler_20260923/euler_check.cpp) lié à la chaîne
publiée (`run_tower_chain`, `run_tower=false`, `keep_catalogue=true`, leviers
par défaut, s = 8, deux fils). Chaque boule dégénérée est recomptée par
balayage exact du nuage (intérieurs et coquille), puis évaluée en Python
exact. Entrées : les sous-nuages emboîtés du runner v2 du développeur
(reçu `lidar_scaling_local_20260923`), SHA-256 dans le
[README du dossier](c_euler_20260923/README.md).

| trame | sites | Kmax | boules | dégénérées | écarts de recomptage | ordres vérifiés | $E_K$ |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 08/000000 | 8 000 | 5 | 342 181 | 54 | 0 | K=1..3 | toutes = 1 |
| 08/000100 | 8 000 | 5 | 257 607 | 66 | 0 | K=1..3 | toutes = 1 |
| 08/000200 | 8 000 | 5 | 278 809 | 111 | 0 | K=1..3 | toutes = 1 |
| 08/000000 | 8 000 | 10 | 1 567 942 | 104 | 0 | K=1..8 | toutes = 1 |
| 08/000100 | 8 000 | 10 | 987 076 | 155 | 0 | K=1..8 | toutes = 1 |
| 08/000200 | 8 000 | 10 | 1 083 173 | 230 | 0 | K=1..8 | toutes = 1 |
| 08/000000 | 16 000 | 5 | 639 102 | 143 | 0 | K=1..3 | toutes = 1 |
| 08/000100 | 16 000 | 5 | 511 801 | 121 | 0 | K=1..3 | toutes = 1 |
| 08/000200 | 16 000 | 5 | 534 376 | 170 | 0 | K=1..3 | toutes = 1 |
| 08/000000 | 16 000 | 10 | 2 830 107 | 304 | 0 | K=1..8 | toutes = 1 |
| 08/000100 | 16 000 | 10 | 1 957 458 | 261 | 0 | K=1..8 | toutes = 1 |
| 08/000200 | 16 000 | 10 | 2 054 138 | 385 | 0 | K=1..8 | toutes = 1 |
| 08/000000 | 32 000 | 5 | 1 114 470 | 222 | 0 | K=1..3 | toutes = 1 |
| 08/000100 | 32 000 | 5 | 998 005 | 135 | 0 | K=1..3 | toutes = 1 |
| 08/000200 | 32 000 | 5 | 999 378 | 569 | 0 | K=1..3 | toutes = 1 |
| 08/000000 | 32 000 | 10 | 4 769 579 | 438 | 0 | K=1..8 | toutes = 1 |
| 08/000100 | 32 000 | 10 | 3 990 717 | 280 | 0 | K=1..8 | toutes = 1 |
| 08/000200 | 32 000 | 10 | 3 813 607 | 1291 | 0 | K=1..8 | toutes = 1 |

Aucun désaccord entre le recomptage par balayage et les champs `n_interior`
et `n_shell` du catalogue (`recount_mismatch = 0` partout). Les ordres
$K_{\max}-1$ et $K_{\max}$ ne sont pas vérifiables par construction (les
sommes y valent des centaines de milliers : les boules de rang supérieur ne
sont pas dans le catalogue).

## Ce que l'invariant détecte : mutants du générateur

Campagne en cours au moment de la publication : chacun des 35 mutants compilés
de `tests/gen/mutants.json` est lié à la chaîne (08/000000 8k, K5) et classé
`killed_euler`, `killed_chain` ou `survived` par
[`run_euler_mutants.py`](c_euler_20260923/run_euler_mutants.py). Les résultats
seront ajoutés ici dans une mise à jour datée ; aucun n'est revendiqué avant.

## Limites

- **Nécessaire, pas suffisant** : deux omissions de contributions opposées se
  compensent ; une boule dont toutes les contributions vérifiables sont nulles
  échappe. Les boules avec $p\geq K_{\max}-2$ ne comptent qu'aux deux ordres
  non vérifiables : sur la coupe 8k de 08/000000, 112 258 des 342 181 boules
  à K5 (33 %) et 191 398 des 1 567 942 à K10 (12 %).
- Pour couvrir **tous** les ordres du contrat, exécuter le générateur à
  $K_{\max}+2$ (K7 pour le contrat K5), vérifier $E_K$ jusqu'à $K_{\max}$,
  puis exiger l'égalité du catalogue $K_{\max}$ avec la restriction
  $p+q_{\min}\leq K_{\max}+1$ du catalogue $K_{\max}+2$. Le contrat K10
  demanderait un générateur à K12, hors du domaine actuel ($K\leq10$).
- L'invariant juge le **catalogue**, pas la tour FULL. Il ne remplace ni T2
  ni un juge de tour d'échantillon.
- Les coquilles de plus de 12 sites sont déjà refusées par la chaîne ; le
  calcul du lien inférieur n'a pas de plafond propre (arrangement exact de
  $u$ grands cercles, coût $O(u^3)$ en entiers).

## Proposition au développeur (priorité haute, coût faible)

1. Calculer $E_K$ dans la chaîne après la fusion du catalogue, **hors chrono**,
   et publier dans la sonde : `euler_by_k`, `euler_checkable_k`,
   `euler_degenerate_balls`. Le cas générique est un compteur par boule ; le
   cas dégénéré est un arrangement exact de grands cercles sur au plus 12
   directions (directions entières $2a\,x+b$, produits vectoriels jusqu'à
   environ $2^{195}$ : prendre les entiers larges déjà présents dans la tour,
   ou refuser explicitement).
2. Lecteur G4 : refuser `complete_relative` si un $E_K$ vérifiable diffère
   de 1.
3. Porte CTest (labels `scale8000` et suivants) sur les trois coupes 8k
   emboîtées à K5 et K10, avec au moins un mutant d'omission tué causalement
   par l'invariant (voir la section précédente).
4. Campagne « $K_{\max}+2$ » sur les trames entières pour le contrat K5.
5. Inscrire l'énoncé au registre avant toute utilisation comme preuve.

Reproduction : [README du dossier](c_euler_20260923/README.md).
