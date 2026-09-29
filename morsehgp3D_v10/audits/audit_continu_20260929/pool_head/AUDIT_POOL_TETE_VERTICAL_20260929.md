# Audit indépendant : pool, tête et verticales

Source auditée : `6206d1d11`, snapshot obtenu par `git archive`, 29 septembre 2026.
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_read_only`, `public_status=not_claimed`.
Moteur inchangé. GCP non utilisé. Petits tests locaux uniquement.

## Résultat

La correction de la course ordinaire du pool est cohérente : capture du descripteur sous
verrou, fermeture de la capture avant l'attente des utilisateurs. Le nouveau build
`mhgp10_unit` passe, stress des travaux courts compris (`unit.stdout`). Le tri de la tête
par rangs est justifié lorsque les rangs du catalogue sont exacts et strictement ordonnés.
Cet audit n'a pas trouvé de contre-exemple géométrique à ces deux changements.

Quatre résultats distincts appellent néanmoins une action :

| Priorité | Constat | Nature |
| --- | --- | --- |
| P1 | Une exception dans le callback du fil appelant détruit fonction/descripteur avant la fin des ouvriers | Défaut mémoire reproduit sous ASan |
| P1 | `mhgp10_tower --no-points --dump=...` segfaulte sur trois points | Défaut CLI reproduit sur build neuf |
| P1 preuve | Une image verticale fausse avant l'entrée des points échappe au juge publié | Mutant de sortie survivant, pas une erreur de la tour réelle |
| P2 performance | La tête remonte séparément les ancêtres pour chaque cluster et point | Carré évitable sur un dendrogramme en peigne, pas un carré LiDAR démontré |

Les commandes, codes, hashes de binaires et hashes des sources de compilation sont dans
[`evidence.json`](../../../receipts/audit_continu_20260929/pool_head/evidence.json). Les sources des tests et leurs sorties sont conservées
dans ce dossier. Les captures complètes avant/après sont également sous
`/tmp/mhgp10-audit-pool-head.X45WLs` (complément temporaire, pas prérequis de lecture).

## 1. Exception dans le pool

`src/sched/pool.cpp:80` appelle `run_chunks` sans garde de sortie ; une exception saute
`current_ = nullptr` et l'attente des utilisateurs, lignes 81–83. Le callback et le `Job`
empruntés disparaissent alors que le second fil travaille encore. `run_chunks` ne restaure
pas non plus le drapeau thread-local lors de cette sortie exceptionnelle. Une exception
échappant au callback d'un ouvrier provoque de son côté `std::terminate`.

[`pool_throw.cpp`](../../../receipts/audit_continu_20260929/pool_head/pool_throw.cpp) synchronise deux fils : l'ouvrier est dans le callback,
l'appelant lève une exception, le `catch` appelant libère l'ouvrier. Résultat : code 1 et
**ASan stack-use-after-scope**, [`pool.stderr`](../../../receipts/audit_continu_20260929/pool_head/pool.stderr). Aucun sabotage du moteur.
Le contrat du header n'interdit pas les exceptions ; les allocations de vecteurs du
catalogue et de Kruskal dans les tâches peuvent notamment en lever.

Correctif proposé : sauvegarde RAII du drapeau, capture de la première exception dans
le travail, fermeture/annulation ordonnée puis attente de tous les utilisateurs avant
destruction et relance dans l'appelant. Tester exception appelant, exception ouvrier,
et réutilisation après exception. Ce n'est pas une réfutation de la correction de la
course normale apportée par `8e3b76245`.

## 2. Export sans attaches

Commande sur les points `(0,0,0), (2,0,0), (5,0,0)` :

```text
mhgp10_tower points.u32le --k=2 --threads=1 --no-points --dump=tower.txt
```

Le processus termine par **SIGSEGV, code -11**. `cli/mhgp10_tower.cpp:186–194`
parcourt tous les sites et indexe `point_node`/`point_level` alors que `--no-points`
les laisse vides. Protéger l'export des attaches par leur présence ; conserver l'export
des nœuds/parents/verticales. La tour calculée avant cet export n'est pas mise en défaut.

## 3. Angle mort de la vérification verticale

`tests/oracle/test_tower_oracle.py:90–114` ne contrôle une image que si un point de
données est déjà entré dans la composante supérieure. Il ne distingue donc pas toutes
les composantes spatiales vides de points, pourtant présentes dans FULL.

Sur les trois points précédents, à l'ordre 2, la paire `{0,2}` naît au rayon carré 1.
Son image correcte à l'ordre 1 est la composante `{0,2}`. Remplacer cette image par le
singleton `{5}` est faux à ce niveau. Pourtant les premiers points concernés n'entrent
en mode `core` qu'au niveau 4 ; entre-temps les trois points ont fusionné à l'ordre 1
au niveau 9/4. Le juge accepte donc **original et mutant : 7 contrôles chacun**.

[`tower.txt`](../../../receipts/audit_continu_20260929/pool_head/tower.txt) est la vraie sortie correcte. [`probe_vertical.py`](../../../receipts/audit_continu_20260929/pool_head/probe_vertical.py)
ne change que le champ vertical du nœud d'ordre 2 né au niveau 1 et démontre le mutant
survivant. Il ne change ni géométrie, ni forêt, ni attaches. Les autres contrôles de
la porte ignorent ce champ. La tour réelle n'est pas réfutée.

Renforcer le juge avec des représentants de composantes de Gamma, ou des centres
témoins rationnels, dès chaque naissance/fusion, y compris quand C∩X est vide. Ne pas
promouvoir la seule inclusion des points en preuve complète des verticales.

## 4. Carré évitable dans la tête

`src/head/head.cpp:135–141`, `155–160` et `162–167` remontent les chaînes d'ancêtres
pour chaque objet. Sur un arbre en peigne valide, la seconde boucle seule traverse
`(n−2)(n−1)/2` arêtes d'ancêtres internes. [`head_ladder.cpp`](../../../receipts/audit_continu_20260929/pool_head/head_ladder.cpp),
`selection=leaf`, `min_cluster_size=1`, confirme la validité du dendrogramme et les
étiquettes ; la formule donne 1 997 001, 7 994 001 et 31 988 001 traversées pour
2 000, 4 000 et 8 000 points.

[`head_ladder.stdout`](../../../receipts/audit_continu_20260929/pool_head/head_ladder.stdout) conserve un passage de trois répétitions
par taille : minima 8,744 / 32,298 / 64,553 ms. La machine partagée est variable ; un
premier passage avait donné 5,107 / 31,290 / 233,373 ms. La conclusion quadratique
vient du travail de boucle, pas de ces chronos. Cette fixture API n'établit pas que
le même peigne soit produit sur un LiDAR ou une gaussienne.

Les parents du condensé précèdent leurs enfants : propager en une passe le premier
ancêtre sélectionné donne `cluster_label`, puis une lecture donne chaque étiquette.
Propager un état d'ancêtre sélectionné remplace aussi la désactivation EOM imbriquée.
Ce levier est petit, exact et testable avant tout nouveau grand chantier.

## 5. Portée des preuves publiées inspectées

Les listes SHA256 des reçus `pool_race_fix_20260929` et
`head_point_dendrogram_20260929` passent intégralement. Elles protègent leurs documents,
scripts et résumés ; elles n'épinglent pas les binaires et sources de chaque exécution.

Le différentiel tête compare réellement labels et arbre exporté dans son script, mais
le reçu ne conserve que 18 lignes avec hashes de sortie tronqués à 16 caractères :
pas les deux dumps, ni les hashes des binaires. Il est un témoignage utile, pas une
relecture autonome de l'égalité. Il ne compare pas les cartes verticales exactes.

Le reçu pool conserve un extrait TSan **avant** correction et décrit les succès après
correction sans logs bruts. Le reçu J2c ajoute `tsan_j2c.txt`, trois lignes récapitulatives
de deux quarts LiDAR et de l'oracle catalogue. Aucun de ces fichiers inspectés ne ferme
commande + source + binaire + stderr brut + code pour une nouvelle relecture TSan.
Cela n'affirme ni absence de tests ni présence d'une nouvelle course ordinaire.

Enfin, `point_dendrogram` coalesce volontairement les niveaux rationnels distincts
dont les doubles se confondent. Son tri reste exact avant cette projection, mais
la tête n'est pas une représentation sans perte des dates de FULL. La porte de
collision vérifie succès et doubles croissants, pas conservation de tous les plateaux.

## Rejouer sans modifier le moteur

Compiler le snapshot `6206d1d11` dans un répertoire neuf, cibles `mhgp10_tower` et
`mhgp10_unit`. Exécuter `probe_vertical.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY`.
Compiler `pool_throw.cpp` avec `src/sched/pool.cpp`, C++20, pthread, ASan/UBSan et
`ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1` ; code 1 attendu sur la
version auditée. Compiler `head_ladder.cpp` contre le core du même build ; code 0
attendu. Les sources épinglées sont dans `evidence.json`. Aucune mesure G4 n'est revendiquée.
