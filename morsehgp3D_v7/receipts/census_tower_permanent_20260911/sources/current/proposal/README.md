# Candidat privé : porte permanente census → tour

Cadre v7 hors registre, backend CPU de référence, entrée quantifiée u16, audit mathématique/architecture, statut public non revendiqué. Préparation à la demande du second auditeur `b8336ad3`. Aucun code actif, audit, index Git ou GCP n’est modifié. La base active est épinglée `6763a877f43d79a45532bce4426feca645b6ee97f18c9a4ee1fea4c47cd408a5`. ROOT a ensuite demandé une nouvelle qualification sur le **futur header de couture CPU `83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`**, copié par overlay explicite. Aucun nouveau helper ni callback n’est fourni à cette gate : elle teste la voie CPU0/1/4 par défaut, pas le futur backend GPU ni le terminal privé c03.

## Périmètre proposé

Deux nouveaux fichiers seulement à promouvoir, après qualification : `tests/census_tower_gate.cpp` et `tests/census_tower_oracle.hpp`. `CMake.diff` ajoute une seule cible C++ de test et onze CTests. Les helpers génériques du juge FULL existant sont inclus avec son `main` renommé, suivant le modèle de `full_ball_work_gate.cpp` et `facet_resolver_cache_gate.cpp` ; aucun moteur ni oracle n’est copié dans `src/`.

Le code dérive du juge T2 renforcé `13f06875da99ae8413b0e64115964979bc5afe9f0c6f7224a7cd4efe773883dc` et de l’oracle `57b615240896f4631434421fa2360d8e5b87db41b0cfc74a28afa0bd0727939c`. Les changements sont la relocalisation des includes/namespaces, le nom de `main` incorporé et l’émission d’une ligne `cause=…` sur stdout pour que le wrapper CTest vérifie **à la fois** le code 1 et la cause attendue des mutants. Les originaux restent épinglés, aucun résultat historique n’est hérité.

Les trois cas ligne12/coquille12+centre+extérieur14/spatial12 exercent chacun deux variantes d’identités, s WSPD=8/10/12, et la construction de tour avec threads0/1/4 : **54 tours K1..10**. La génération, le préfiltre et le census sont réels. Le catalogue Gram indépendant est comparé exhaustivement au census avant de donner **uniquement le vrai census** au producteur. Les coupes ouvertes/fermées et verticales de chaque ordre sont comparées à Gamma ; les autres variantes doivent conserver les mêmes objets physiques. Les deux gardes renforcées `forest.order()==K` et `lower_nodes.size()==nodes.size()` restent antérieures aux coupes.

Le plancher conserve K9/K10 non vacuants, les coquilles de douze points et les supports q3/q4. Le juge historique compare ses MEB et composantes au modèle antérieur. Neuf rejets de l’oracle et quatre mutants (assignment, coupe ouverte, adjacence, omission de census) gardent leurs causes exactes. Arguments inconnus ou absents doivent sortir 2. La cible n’active pas `MHGP7_TESTING` : les mutations restent exclusivement dans le juge, pas le moteur.

La borne n≤14 limite uniquement l’oracle exponentiel. Les TIMEOUT CTest sont des limites d’exécution de tests ; aucune borne de travail, de descente ou de nombre de candidats n’est introduite dans le produit. Cela ne prouve ni une complexité sous-quadratique universelle ni le contrat temporel 50k.

## Coût prévisible, non résultat de ce candidat

Les reçus T2 metadata antérieurs donnent environ 19,70 s pour compiler O2, puis 0,17/1,21/1,49 s pour ligne/coquille/spatial ; SAN compilait en 42,71 s et exécutait ces cas en 1,44/10,35/12,42 s. Le contrôle historique supplémentaire coûtait environ 0,37 s O2 et 3,63 s SAN dans l’ancien reçu T2 final. Ces observations servent uniquement à dimensionner une porte ordinaire ; le nouveau snapshot doit être qualifié séparément et ne reçoit aucun résultat par transfert.

`record.py` prépare une copie composée du code actif épinglé et du candidat, configure CMake, construit uniquement cette cible avec un job, puis exécute les onze CTests séquentiellement. Il conserve le journal CTest complet, les commandes, sources réellement compilées, dépendances, pin ELF et fermetures ; les captures sont create-only. Aucune compilation ne doit être lancée sans le créneau accordé par ROOT. SAN est exécuté par ROOT avec ASan/UBSan et LSan activés.

```bash
python3 -B build/v7_census_tower_permanent_20260911/record.py --out o2_r1
python3 -B build/v7_census_tower_permanent_20260911/record.py --out san_root_r1 --san
```

Statut courant de cette note : **préparé, pas encore compilé**. `origin.json` et `baseline/` épinglent le contexte lu ; `candidate/` et `CMake.diff` n’ont pas été appliqués au moteur actif.
