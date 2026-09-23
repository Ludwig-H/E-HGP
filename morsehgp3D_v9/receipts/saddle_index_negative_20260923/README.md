# Reçu négatif : index des selles dans la phase 0 de la tour (lemme A du D5)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

## Ce qui a été construit (hors produit, `saddle_index.patch`)

Le lemme A de l'auditeur C (D5) porte sur une boule régulière C du catalogue :
sa coquille est son unique support minimal S_C, donc C est le MEB de S_C.
Pour z dans I_C, la facette F = S_C ∪ I_C ∖ {z} a donc MEB(F) = C.

Le patch s'appuie sur ce lemme :
- **Index par ordre K** : il indexe ces facettes pour les boules du programme
  K avec p + q − 1 = K, par empreinte XOR triée, avec vérification exacte des
  K sites.
- **Consultation** : dans la phase 0, une facette qui n'est pas une graine y
  est cherchée avant `static_terminal`. La cible est celle du produit, dont le
  premier MEB rendrait exactement C avec un coup d'ancre à la profondeur 0.
- **Juge** : les builds de test comparent chaque coup à `static_terminal`.

## Mesure

Sous-nuage emboîté 16 000 de 08/000200 sans sol, K10, W8, tour statique 8,
binaire construit depuis `c19e4b49` avec le patch ; hôte partagé.

| index | condensé | tour (ms) | phase 0 (ms) | MEB | entrées | coups | intrus |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ON | `0c44eba9` | 4 029 | 2 752 | 1 688 262 | 10 188 238 | 1 011 979 | 1 337 551 |
| OFF | `0c44eba9` | 3 990 | 2 582 | 2 700 241 | 0 | 0 | 1 337 551 |

## Lecture

- L'exactitude est confirmée : condensé identique, et recherches d'intrus
  inchangées (même cible à la profondeur 0).
- **Pas de gain** : 1,01 M MEB évités, mais la construction et le tri de
  10,2 M entrées sur les dix ordres coûtent autant ou plus. La phase 0 ne
  baisse pas.
- C'est conforme à la réserve de l'auditeur C : l'index et sa jointure
  valent 0,79 à 1,02 s à 8k/K10. Le lemme A n'est rentable qu'avec un index
  beaucoup moins cher, ou associé au saut au centre qui réduit les chaînes
  d'intrus (lemme B), le vrai coût de la phase 0.
- Code retiré du produit.

## Contenu

`saddle_index.patch`, `out/sad_1.json` et `out/sad_0.json` (sorties de la sonde
v16 locale), `SHA256SUMS`.
