# Audit ciblé MES-M2 — lecteurs et limites de feuille

**CST-0215 confirmé : le lecteur de vidages ne certifie pas le domaine qu'attendent les noyaux.** Six vidages invalides, de checksum FNV valide, sont acceptés. Trois d'entre eux sont ensuite déclarés conformes par le vrai exécutable d'identité hôte. Aucun défaut géométrique n'a été trouvé sur les petites portes exécutées ; cela ne qualifie pas l'exécution GPU.

Pin : **95247cf4baf2ebd0856c1ac75670f643d24daa6e**, worktree isolé et détaché. MES-M2 est issu de `a0091e2b7195b68df6f707074b5ad59fb7ea98bb`. Cadre : exploration v12 hors registre, référence CPU et warp simulé, profil u21, `public_status=not_claimed`. Aucun GPU/G4, LiDAR, sanitizer ou grosse matrice. Aucun code produit ni fichier `audits/` modifié, aucun commit. Les sources relues et les quatre artefacts du reçu sont hachés dans MANIFEST.json.

Les chemins ci-dessous sont relatifs à `morsehgp3D_v12/microbancs/mes_m2_feuille/` sauf mention contraire.

## CST-0215 — admission incomplète des données, gravité majeure

`include/mhgp12/leaf/dump_format.hpp:185–238` contrôle la magie, la version, le nombre de compteurs, le checksum, quelques monotonies d'offsets et la non-vacuité des boîtes. Il ne contrôle pas le domaine des coordonnées, la concordance du profil avec le binaire, K, les indices de sites dans le nuage ni les populations de référence. Le test `job.begin + job.m > n_leaf_sites` (**223**) peut lui-même boucler modulo 2^64.

Le fichier d'entrée est fabriqué avec le **Writer original**, relu avec le **Reader original** : checksum correct, aucune mutation non resignée. Le témoin nominal a un site `(1,1,1)`, une feuille, un préfixe, aucune émission. Les six variantes ci-dessous sont acceptées par `read()` :

| Variante | Invariant violé | Résultat exécuté |
| --- | --- | --- |
| `site_out_of_cloud` | un seul site dans le nuage, liste contenant l'indice 1 | lecteur : accepté |
| `wrapped_job_begin` | `begin=2^64−1`, `m=1`, donc `begin+m` devient 0 | lecteur : accepté |
| `zero_k` | K=0 | lecteur accepté ; identité hôte code 0, `identity=true`, **0 résolue / 1 non résolue**, pour J3 et cohérente |
| `profile_mismatch` | en-tête u24, binaire compilé u21 | lecteur accepté ; identité hôte code 0, `identity=true`, **1 résolue**, pour les deux formes |
| `coordinate_out_of_profile` | x=`UINT32_MAX` déclaré u21, boîte locale `[UINT32_MAX,UINT32_MAX+1)` | lecteur accepté ; identité hôte code 0, `identity=true`, **1 résolue**, pour les deux formes |
| `short_record_population` | l'enregistrement exige deux incidences, le tableau n'en contient qu'une | lecteur : accepté |

**Périmètre de la démonstration.** Les trois cas d'indices/offsets/population susceptibles de provoquer un accès hors allocation n'ont **pas** été transmis aux noyaux ou au comparateur : seul leur lecteur a été exécuté. Les trois cas passés à `leaf_identity` ont un stockage valide et n'exercent aucun accès dangereux. Le cas de coordonnée hors profil n'a qu'un site : il démontre l'admission erronée, sans rechercher un overflow. Aucun crash, sanitizer ou résultat GPU n'est revendiqué.

La suite des accès explique le risque concret :

- `leaf_common.hpp:108–113` déréférence `x[sites[l]]`, `y[...]`, `z[...]` sans validation supplémentaire.
- `LeafDump::leaf_sites`, `dump_format.hpp:99`, forme `sites.data()+job.begin` ; le dépassement de l'addition de contrôle n'est pas réparé.
- `compare.hpp:24–31` et `host/leaf_identity.cpp:157–167` construisent des plages de populations de longueur `p+m`, en faisant confiance au vidage.
- `host/leaf_identity.cpp:124` convertit K de u64 en int sans validation. Le noyau refuse K=0 comme non résolu ; le vérificateur saute alors la feuille (**132–134**) et son critère final (**229**) permet `identity=true`. Le présent constat vise l'admission des données ; les fausses adoptions du juge externe font l'objet de l'audit distinct de CST-0018.

**Correction requise avant admission au microbanc.** Vérifier les tailles et leurs produits avant allocation d'après la taille réelle du fichier ; profil supporté et égal à celui du binaire ; K du catalogue source (actuellement 1..12) ; coordonnées dans `[0,2^B)` ; boîtes dans le domaine T0 déclaré, dont borne haute `2^B` ; indices de sites bornés, strictement croissants et sans répétition dans une feuille ; plage `begin≤n` puis `m≤n−begin` ; offsets initiaux nuls, monotones et bornés ; enregistrements/supports/populations valides et somme des incidences égale à la plage annoncée. Les statuts et drapeaux de référence doivent aussi être cohérents. Un FNV valide protège l'intégrité du fichier, **pas ces invariants**. Les six variantes doivent rendre un refus propre avant tout noyau ; l'échauffement et le chronométrage restent hors de ce chemin.

## Vérifications positives limitées des noyaux

Le témoin C++ appelle les headers originaux J3 et cohérent, profil u21, et `morsehgp3D_v11/src/catalogue/leaf_device.hpp` comme référence différentielle hôte. Il compare **les quinze compteurs et l'ensemble des émissions**, y compris support, p, m, qmin et masques intérieur/coquille, pour chaque feuille résolue. Ce n'est ni un nouvel oracle indépendant de géométrie, ni l'exécution de `leaf.cpp`.

- **Coquille synthétique de 32 sites** : points choisis par paires opposées sur `x²+y²+z²=50`, translatés dans le domaine positif, K5. Les deux formes sont résolues et produisent **347 émissions identiques à la référence**. J3 visite **4960 triplets, 35960 quadruplets, 82 tranches de file** : le test traverse réellement la limite de 512 cases, plusieurs frontières entre lignes et tranches, les derniers paquets partiels et la voie 31. Il complète les vérifications antérieures des faces/J2 sans les présenter comme une nouvelle preuve de ces lemmes.
- **Fermeture d'étendue exactement 2^20** au bord haut u21 : deux sites diagonaux, une émission identique, deux formes résolues.
- **Même feuille, fermeture agrandie d'une unité** : étendue `2^20+1`, deux formes non résolues, zéro émission, comme la référence. Pas de transfert de cette porte aux profils u24/u32.

Lecture supplémentaire : `leaf_j3.hpp:180–206` distribue des intervalles disjoints issus du scan exclusif et garde un itérateur par ligne entre tranches ; les écritures de file sont suivies de `sync`, et la fin de phase T publie H avant Q (**237**). Aucun écrasement de file ou lecture anticipée de H identifié. Les 496 indices H et les quatre rangs de 5 bits couvrent bien m≤32. La limite héritée `32·3·(32+496+4960+35960)=3979008<2^22` couvre les compteurs u32 de feuille. L'ajout `side_q2_narrow` reste exact sous l'étendue certifiée : norme et produit scalaire sont chacun bornés par `3·2^40`, donc leur différence et tous les intermédiaires rentrent en i64.

**Limite CPU/GPU explicite.** La simulation hôte exécute séquentiellement les voies ; elle ne vérifie ni l'ordonnancement indépendant des fils CUDA, ni les collectives à masque plein, ni les courses réelles de mémoire partagée. La lecture des barrières et les portes ci-dessus ne ferment donc pas `racecheck`/`synccheck`, l'identité appareil ni la performance. Aucune nouvelle campagne GPU n'a été lancée.

## Rejeu et conservation

Depuis le worktree :

```text
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/feuille/run.py
```

Deux compilations hôte ciblées : le témoin et l'exécutable original `host/leaf_identity.cpp`, g++ C++20 `-O2 -Wall -Wextra -Werror`, sans construction du moteur. Toutes les fixtures binaires sont synthétiques, créées temporairement puis supprimées avec les exécutables. RESULT.json conserve uniquement les comptes et verdicts utiles. L'empreinte du prédicat v11 relu est exactement celle annoncée par le port : `c638996b661c6fc690a9dff30d61a0c5294b882001bb806009d72120f6168472`.

Le rejeu vérifie les empreintes de MANIFEST.json **avant et après** les compilations/exécutions, y compris ses propres `run.py` et `probe.cpp`. Une divergence interdit l'émission du résultat épinglé. Les fichiers `.d` produits par **g++ `-MMD`** ferment les dépendances locales effectivement compilées : toute dépendance absente du manifeste est refusée ; leurs chemins figurent dans RESULT.json et MANIFEST.json. Les en-têtes système sont exclus par `-MMD`, le compilateur est identifié séparément. Les lignes de sortie du témoin et les statuts/compteurs de résolution des deux formes sont comparés à des attendus explicites, avec exceptions et sans `assert`. La capture finale du reçu renforcé a été exécutée avec **`python3 -O`** ; toutes ces vérifications restent actives.
