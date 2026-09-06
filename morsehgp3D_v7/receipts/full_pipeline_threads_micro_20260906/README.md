# Pipeline multi-CPU : micros — 6 septembre 2026

Cinq appels n=8, s=8, Kmax=10, lazy C=1 000 000, P=unlimited emploient
le même ELF `4f5ba475…` : sans option threads (v5), puis threads=1/2/4/8
(v6 explicite). Chacun termine code 0, huit ordres K=1..8. L'entrée,
les huit digests d'ordre et le digest final sont identiques ; tous les champs
non mesurés des ordres sont identiques. FULL et la boucle K restent séquentiels.

Sur ces petites fixtures, les workers observés rects/préfiltre/census/expand
atteignent 2/4/8 ; WSPD et tri restent à 1. Ce ne sont pas des chronométrages
de performance, ni des preuves de complétude géométrique.

Neuf rejets de la sonde rendent 2 avec terminal invalid_input ; sept rejets
du runner rendent 2 avant création du répertoire demandé. La porte de schéma
pure du lecteur mono inchangé admet v5 et refuse les quatre v6 avec la cause
exacte `schema` ; ce contrôle n'est pas une exécution du juge mono complet.
La comparaison Python normal/-O rend deux fois 0 avec des stdout identiques.
Les 23 commandes et leurs groupes sont clos ; leurs bruts sont conservés.

Les limites locales 60 s murales / 45 s CPU du recorder sont les paramètres
réellement exécutés pour ces seuls micros ; elles ne sont pas appliquées aux
mesures directes de taille utile. Aucune capture n'est réécrite après clôture.

Les 42 dépendances projet du depfile sont copiées une seule fois sous sources/,
avec le runner et le lecteur mono (44 fichiers au total). Ce snapshot est
post-compilation, avant les micros : aucune provenance pré-build ou hermétique
n'est revendiquée. build_observation.json distingue la compilation rapportée
par ROOT des commandes réellement capturées. L'ELF n'est pas distribué ; son
identité avant/après est conservée. public_status=not_claimed, CPU/u16.

Lecture portable, sans moteur, depuis ce dossier :

```bash
sha256sum -c SHA256SUMS
python3 -B record.py --check .
python3 -B -O record.py --check .
```

Aucun résultat n=8000 n'est inclus ici. GCP non utilisé.
