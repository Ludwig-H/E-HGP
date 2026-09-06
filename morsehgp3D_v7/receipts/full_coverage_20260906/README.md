# Qualification du journal FULL daté v2

6 septembre 2026, après main `22003315`, worktree déclaré dans les captures.
`public_status=not_claimed`, autorité `structural_only`, GCP non utilisé.
Le contrat actif est [CONTRAT_COUVERTURES_DATEES.md](../../docs/CONTRAT_COUVERTURES_DATEES.md).

## Résultats

16 commandes directes closes : contexte, compilations O2/SAN, portes
codes 0/2 et trois mutants O2 réfutés avec leur cause attendue. O2 et
ASan/UBSan donnent les mêmes 710 contrôles, 30 rejets, 30 coupes de rejeu,
34 coupes Gram/Gamma et 34 refus d'allocation injectés. Aucun échec
préliminaire sur ce journal ; la correction d'immutabilité a précédé
ces compilations. Détection des fuites conservée, aucune option pour
la désactiver. Les ELF restent dans build et seuls leurs hashes sont ici.

Les mutants n'altèrent pas le produit : copies privées isolées,
remplacements uniques conservés par le recorder et les sources mutées.

| Mutation | Code | Cause |
| --- | ---: | --- |
| Omettre les contributions de continuation | 1 | `growth.no_fake_node` |
| Ignorer la date d'une contribution | 1 | `growth.no_future_leak` |
| Suivre les successeurs futurs | 1 | `replay.live_identity` |

Le dossier `cmake/` conserve une seconde qualification Release fraîche :
quatre commandes closes, trois seules cibles construites et six CTests
réussis (journal v2, quotient local et certificat structurel v1). Les
codes 0/2 sont vérifiés par le wrapper, pas par une règle WILL_FAIL.
Le [delta du quotient](../local_plateau_diameter_20260906/README.md)
garde son propre paquet, y compris son ancien échec SAN ptrace et sa
reprise réussie. Il n'est pas requalifié par les seuls tests du journal.

## Sources et reproduction

`source_refs.json` relie les 17 sources projet de la qualification CMake
à des octets archivés : trois nouvelles captures copiées, autres sources
référencées par chemin et SHA sans duplication. Les sources nouvelles
ont été épinglées avant/après la qualification directe. Ses autres
dépendances `.d` sont observées après compilation ; le second témoin
CMake relie aussi les dépendances projet réellement compilées à un
inventaire préalable puis au contrôle après tests. Les dépendances
externes/système CMake sont seulement observées après compilation,
sans prétendre qu'elles avaient été figées auparavant.

Les commandes, flags, sources, logs et scripts sont conservés ; aucun
ELF ni header système copié. `record.py` et `cmake/capture.py` sont les
enregistreurs de l'environnement original, pas des rebuilds portables
prétendument autonomes. Pour une nouvelle compilation, utiliser CMake
sur ces sources dans un répertoire neuf avec Boost, puis les six tests
ciblés listés dans `cmake/run_r1/inventory.stdout`.

```bash
python3 -B morsehgp3D_v7/receipts/full_coverage_20260906/verify.py
python3 -B -O morsehgp3D_v7/receipts/full_coverage_20260906/verify.py
```

Ce lecteur contrôle l'intégrité, les sources liées, les codes, comptes,
causes des mutants et six CTests sans relancer C++. Son succès n'est ni
un chronométrage, ni une certification géométrique du fournisseur, ni
un résultat de tour FULL, d'archive ou de GPU. Les contrats restent ouverts.
