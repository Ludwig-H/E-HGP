# Reçu : assemblage du catalogue sans remplissage en série (suite de J2, 29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. GCP non utilisé pour ce reçu : les mesures sont locales, et
celle de G4 à 48 fils reste à faire.

## Objet

Suite de J2 (`receipts/catalogue_filter_j2_20260929`). Les grands tableaux du catalogue et les tableaux de travail de
l'ordre étaient mis à zéro en série par `resize`, et leurs pages touchées par un seul fil. Ils passent sur un
allocateur sans initialisation par valeur, `UninitAlloc`, compatible avec `std::vector`. Chaque case est écrite par
la boucle parallèle qui remplit déjà le tableau. Le reçu complet de l'agent est
[`RECU_AGENT_J2_ASSEMBLAGE.md`](RECU_AGENT_J2_ASSEMBLAGE.md). Trois fichiers sont modifiés : `catalogue.hpp`,
`generator.cpp` et `sched/sort.hpp`.

## Relecture et intégration

- **Écriture avant lecture**, vérifiée dans le code :
  - `cmp[0]` n'est jamais lu : toute lecture est gardée par `b > 0` ou court-circuitée par `b == 0` ;
  - `rank`, `pop_off`, `support`, `qmin`, `p`, `u`, `flags` et `n_interior` sont affectés pour chaque indice ;
  - chaque rang de `level` est écrit à la première boule de ce rang ;
  - les plages de `pop` pavent tout le tableau.

  Aucun autre code ne construit de `Catalogue`, et aucun ne modifie ces tableaux par `|=`.
- **Différentiels à l'intégration**, sur le worktree avec J2 et l'assemblage, contre le binaire `568d45297` d'avant
  J2 (`build/v10-j2/build-base`, `433b4b97…`), sur les 10 entrées, à 1 et 4 fils :
  - build normal : 10 sur 10 identiques ([`differentiel_integration.txt`](differentiel_integration.txt)) ;
  - build empoisonné `MHGP10_POISON` (octets 0xA5 à l'allocation) : 10 sur 10 identiques
    ([`differentiel_integration_poison.txt`](differentiel_integration_poison.txt)). Une case lue avant d'être écrite
    changerait la sortie.
- **Portes** : 9 sur 9 vertes ([`ctest_gate_integration.txt`](ctest_gate_integration.txt)).

## Mesures de l'agent (codespace, `t_order` + `t_assemble`)

| Trame | K | Fils | Avant (s) | Après (s) | Écart |
| --- | ---: | ---: | ---: | ---: | ---: |
| 02 | 5 | 4 | 0,327 | 0,232 | −29 % |
| 02 | 10 | 4 | 1,350 | 0,941 | −30 % |
| 00 | 5 | 4 | 0,307 | 0,211 | −31 % |
| 00 | 10 | 4 | 1,366 | 0,923 | −32 % |
| 02 | 5 | 1 | 0,662 | 0,657 | neutre |

Le catalogue entier gagne 3 à 5,5 % à 4 fils. Le gain devrait être plus grand sur G4 à 48 fils, où le remplissage en
série ne se parallélisait pas : il est à mesurer.

## Limite

Ces tableaux ne sont toujours pas comptés au budget mémoire, comme avant. Le passage à `Buffer<T>` relève d'une
décision séparée.
