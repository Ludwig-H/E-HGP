# Couture batch géométrique — preuves portables privées

11 septembre 2026. Prototype CPU u16 hors registre, `public_status=not_claimed`.
Base scalaire 6763 ; candidat final 83f1. Aucun actif, CUDA ou GCP utilisé.

Un callback synchrone par K reçoit les U−S clés uniques non semées, les semis
complets et une vue immuable index/catalogue. Il rend les BallId/ordinals et
le travail payé. Le cœur valide le lot avant scatter et conserve ensuite le
calendrier exact, les ancres pré-lot, les verticales et le cache nominal.

Trois captures sont conservées, **33 commandes moteur/compilation au total** :
O2r1/8725 avant durcissement des compteurs ; O2r2 et SAN ROOT/83f1 après celui-ci.
Chaque bras batch1/4 compare directement 118 terminales dans 56 lots au chemin
scalaire, avec 34 nuages, 150 ordres et 87 230 contrôles verticaux. Les journaux
physiques complets et le travail R/U/S/Q/H/T coïncident avec les bras scalaires.
Sept sorties physiques O2r2/SAN sont aussi byte-identiques.

Le premier harnais passe 55 rejets ; le final en passe 65, dont 32 après un
préfixe et 44 avec travail connu non nul. Les fautes de compteurs/capacités
sont des états synthétiques déclarés. Une mauvaise BallId pourtant admissible
est rejetée par le juge direct, pas prétendument par le validateur structurel.
Ce sont des injections C++ contrôlées ; ce paquet ne prétend pas compiler
65 algorithmes mutants distincts.

Le défaut corrigé était un risque de faux `work_known` sur agrégation interrompue.
Le travail est maintenant publié atomiquement par le backend et fusionné avant
les capacités ; une fusion globale partielle est marquée inconnue. Les deux
versions et leurs sources restent séparées, sans promotion rétroactive de r1.

Exécuter les lecteurs depuis cette destination, sans source extérieure :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

`storage_map.json` relie chaque chemin historique exact à son objet SHA-256
dans `objects/`. Le lecteur vérifie les octets, restaure temporairement les
sources/captures et exécute les lecteurs conservés : aucune compilation ni
exécution du moteur. La déduplication est réversible ; les Markdown historiques
restent des objets bruts, pas des liens relatifs rendus comme actifs.
Le patch exact est au chemin logique `closed_o2_san_r1/integration.patch`.

Les ELF sont omis, avec leurs empreintes conservées. Aucun SDK/vendor n'est
copié ; les dépendances natives sont attestées avant/après, pas fournies comme
environnement hermétique. Aucun grand nuage, preuve de trajectoire device,
temps GPU, archive industrielle, complétude WSPD ou contrat 50k n'est acquis.
