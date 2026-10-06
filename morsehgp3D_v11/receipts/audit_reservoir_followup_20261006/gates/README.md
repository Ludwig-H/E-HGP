# Raccord du réservoir aux portes de banc

Pin produit : `59509bbc816646f7bda0c77a3b948f57c79a8b9c`. Lecture de sources et rejeu Python stdlib seulement ; aucune compilation, exécution native, CUDA ou action cloud.

Le parseur de `bench/full_probe.cpp:359` accepte désormais les masques jusqu’à 262143. Le bit 131072 sélectionne le replay des débordements, sans effet hors des lots (`src/catalogue/catalogue.hpp:43–45`). Le témoin `tests/tower/full_bench_io.py:110` conserve pourtant 131072 comme option trop grande. Toutes les préconditions du parseur admettent ce bit seul. L’appel atteint le calcul et la ligne `phase=exit` inconditionnelle (`full_probe.cpp:393–396`), alors que cette porte exige un refus d’usage sans événement (`full_bench_io.py:112–114`). La contradiction est établie dans la source ; aucun verdict CTest exécuté n’est revendiqué.

Le collecteur `bench/full_campaign.py:46` conserve également la borne 131071 et rejette donc les modes replay valides. La proposition textuelle modifie seulement deux lignes : borne et message de `optimization()` à 262143, témoin `opt_large` à 262144. Les autres préconditions et les planchers de la porte restent inchangés. La recherche épinglée dans tests, API, CLI et bench ne trouve aucun autre témoin de refus périmé dans ces sources.

`replay.py` charge les sources par `git show` au pin, vérifie leurs SHA-256, exécute les fonctions Python réelles `need` et `optimization` extraites par AST, avant et après la proposition. Il compare quatre modes (131072, 180219, 212987, 262144) au modèle borné des cinq préconditions C++ dont le texte exact est vérifié. Il vérifie aussi que `proposal.patch` correspond exactement aux deux substitutions. Son défaut compare `proof.json` sans l’écraser ; `--emit` sert seulement à produire un JSON sur stdout. Ce modèle ne simule pas le moteur.

La lecture du réservoir CPU/CUDA est favorable à ce périmètre : réservations distinctes, curseurs bornés, chaînes copiées après la fin du comptage, repli conservé et compteurs logiques inchangés. Les empreintes et le raisonnement borné sont dans `native_review.json`. Cet avis source ne qualifie ni le GPU, ni les performances, ni l’épuisement du réservoir sur une entrée native. Après intégration, rejouer sur G4 les portes existantes `mhgp11_tower_full_bench_io` normal/−O et `mhgp11_tower_full_leaf_lanes`, et le collecteur stdlib normal/−O ; aucun nouveau lot de mesure n’est demandé ici.

Rejeu depuis ce dossier dans un checkout contenant le pin :

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Sortie des deux replays : `reservoir_gate_source_verdict conforme modes4 changed_lines2 native0 cloud0`.
