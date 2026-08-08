# MorseHGP3D v3

État : **M1 seulement** — le juge. Il n'y a pas de v3 ici, et il ne doit pas y en
avoir tant que M2 n'a pas répondu. Voir [`PROPOSITION.md`](PROPOSITION.md) §13.

L'autorité mathématique reste `docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`. Aucun statut public, aucun SLO
n'est ouvert.

## Ce que M1 est

Un oracle **indépendant** du chemin testé, exécuté sur le **domaine déclaré**
$[0,2^{16})^3$ — et non sur les coordonnées $\leq120$ auxquelles la porte de la
v2 se limitait.

| couche | choix, et pourquoi |
| --- | --- |
| arithmétique | rationnels de **précision arbitraire**, entiers signe-magnitude en chiffres de 32 bits — représentation *différente* du complément à deux de largeur fixe de la production, pour qu'un défaut commun ne se compense pas |
| géométrie | la sphère par un sous-ensemble est résolue par **élimination de Gauss**, jamais par les formules de Cramer du chemin testé |
| structure | la forêt de référence est reconstruite **depuis $\Gamma_k$ lui-même** (tous les $k$- et $(k+1)$-sous-ensembles), pas depuis le catalogue testé |

Ni division entière ni PGCD : les rationnels ne sont pas normalisés, la division
est une multiplication croisée et la comparaison un produit croisé. Le juge est
le composant le plus simple du système, à dessein.

## Ce qu'il compare

- **catalogue** : cardinal, absence de doublon de support, rang, membres,
  **tranche triée**, niveau rationnel **exact** ;
- **forêt, par ordre** : genre, niveau exact, **arité**, **racines**,
  **nombre canonique de nœuds**, **généalogie complète** — la clé canonique d'un
  nœud étant l'ensemble trié des minima de son sous-arbre ;
- **invariants** : niveau croissant vers la racine, aucune chaîne de deux
  multifusions de même niveau, tout nœud porte une sphère source.

## Fermeture de campagne

`attempted = decided + rejected_domain`, publiée dans le reçu. Arguments
absurdes refusés (code 2). Planchers durs sur nuages décidés et nœuds comparés :
une campagne vide ne peut pas rendre `OK`. Toute censure inattendue est un
**échec**, pas un succès. Le garde de domaine est **symétrique** : un désaccord
dans l'un ou l'autre sens échoue.

## Résultats du 8 août 2026

```
attempted=40 decided=40 rejected_domain=0 | spheres=1850 forets=82
noeuds=1909 | largeur max=158 bits | grille=[0,65535]
OK : campagne fermee, structure complete comparee sur la grille declaree
```

Reçus dans [`receipts/`](receipts/).

Deux faits établis par cette première exécution :

1. **Un défaut trouvé immédiatement.** Les tranches `I ∪ U` n'étaient pas triées,
   alors que `mhgp.hpp` le contractualise. Aucun test de la v2 ne le voyait — O1
   ne lit jamais `cat.members`, O2 trie avant de comparer. Corrigé dans
   `morsehgp3D_v2/src/catalogue.cpp`.
2. **Les niveaux exacts font jusqu'à 158 bits** sur la grille déclarée, donc au
   delà de `__int128`. C'est la raison pour laquelle la porte précédente décidait
   **zéro nuage sur quarante** ici, en annonçant `OK`.

## Construire et exécuter

```sh
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
cd build/v3 && ctest --output-on-failure     # 6 tests : 2 de v3, 4 de v2

./mhgp3v_arith_selftest 20000                # arithmetique contre __int128 et GMP
./mhgp3v_oracle --clouds 40 --seed 4242 --min-points 8 --max-points 11 \
                --max-order 3 --min-decided 30 --min-nodes 500 \
                --receipt receipts/oracle_campaign.json
```

GMP n'est **pas** une dépendance de l'oracle : il n'intervient que comme second
témoin de la validation arithmétique, et seulement s'il est présent.

## Census

`census_tukey_shallow.py` produit un reçu complet (provenance, digests, jeu de
directions, convention de demi-espace, identité de campagne). Il mesure un
**minorant** de l'ensemble où la borne tangente *non contrainte* de la v2 vaut
$+\infty$ — et **rien d'autre** : l'ensemble où la borne à centre convexe échoue
est vide, puisque $R\leq\mathrm{diam}(X)$.

Nuages : Stanford bunny, reconstruction fusionnée et **dix captations brutes
recalées** — le cas multi-captation que la proposition doit traiter. Les données
ne sont pas versionnées ; le reçu porte leur origine et leur digest.
