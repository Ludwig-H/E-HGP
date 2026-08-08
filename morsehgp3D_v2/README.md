# MorseHGP3D v2

Réimplémentation de la hiérarchie HGP en dimension trois à partir d'un **autre
objet de calcul** : la théorie de Morse de la fonction $d_K$ (distance au
$K$-ième plus proche voisin), au lieu du graphe des facettes de ČECH.

- Le modèle, ses preuves et ses limites : [`DESIGN.md`](DESIGN.md).
- L'audit de complétude qui a corrigé la première rédaction :
  [`WARNING_AUDIT_COMPLETUDE.md`](WARNING_AUDIT_COMPLETUDE.md).
- Les mesures : [`RESULTATS.md`](RESULTATS.md).

## En une phrase

$L_K(r)=\left\lbrace y:d_K(y)\leq r\right\rbrace$ est un sous-niveau ; la forêt
demandée est donc le *merge tree* de $d_K$, et il se lit sur les seuls points
critiques d'indice 0 et 1 — les **sphères critiques** de rang fermé $K$ et $K+1$.
Il n'y a jamais de facette de cardinal $K$ dans le calcul.

## Construire et tester

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/mhgp_tests     # arithmetique exacte, spheres, miniball
./build/mhgp_oracle       # P0 + P2 contre l'oracle exhaustif
./build/mhgp_regressions  # fixtures permanentes des deux audits
./build/mhgp_cli --family uniform --n 1000 --k 10 --single-order
```

`mhgp_oracle` compare, sur des nuages tirés au hasard :

- **P0** — le catalogue produit doit être **identique** à l'énumération
  exhaustive de tous les supports de taille 1 à 4 ;
- **P2** — pour chaque ordre $k$ et chaque niveau testé, le nombre de composantes
  de $\Gamma_k(a)$ (définition normative de `docs/math/DEFINITION_HGP_3D.md` §1,
  calculée par force brute sur tous les $\binom{n}{k}$ sous-ensembles) doit
  **égaler** celui lu sur la forêt.

Les deux comparaisons sont exactes ; aucun accord moyen n'est accepté.

`mhgp_regressions` rejoue les contre-exemples des deux audits :

| fixture | ce qu'elle piège |
| --- | --- |
| R1 | croissance du voisinage qui s'arrête avant une paire de GABRIEL lointaine |
| R2 | support non minimal accepté (coefficient barycentrique de $p$ oublié) |
| R3 | tétraèdre bien centré de rang 4 dont les quatre faces sont de rang $\geq12$ |
| R4 | `cone_directions` dégénéré certifiant à tort |
| R5 | bon centrage dépendant de la permutation des sommets |
| R6 | chaîne de fusions binaires de même niveau au lieu d'une multifusion |

## État

| élément | état |
| --- | --- |
| arithmétique exacte (`i128`, `BigInt<N>`) | fait, testée |
| sphères par support, `sphere_side`, bon centrage | fait, testé |
| catalogue des sphères critiques, croissance certifiée | fait, conforme à l'oracle |
| forêt de fusion multifurquée par ordre | fait, lots contractés par composante |
| bras de descente non résolus | comptés (`unresolved_arms`), forêt non fiable si > 0 |
| flèches verticales | **non émises** — obligation de preuve V.1 |
| `coverage_log` | **non émis** — obligation de preuve C.1 |
| coquilles cosphériques (> 4 points) | **rejetées et comptées**, non traitées |
| certification du recouvrement sphérique | **heuristique** — obligation ouverte |
| oracle indépendant des primitives testées | **non** — obligation ouverte |
| peeling local incrémental (performance) | **non fait** — voir `RESULTATS.md` |
| portage CUDA | **non fait** |

## Ce qui n'est pas revendiqué

Aucun statut public, aucun budget. En particulier, **le contrat 50 000 points
n'est pas atteint par cette version** : l'énumération locale est quadratique en
$\lvert W_p\rvert$ et $\lvert W_p\rvert$ n'est pas prouvé petit. Les mesures et
le chemin identifié pour y remédier sont dans `RESULTATS.md`.
