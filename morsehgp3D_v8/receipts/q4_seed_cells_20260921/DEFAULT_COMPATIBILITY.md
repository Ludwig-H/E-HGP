# Compatibilité des anciennes CLI : constructeur33 vers constructeur34 R1

La capture [compatibility_qkwfsev7](compatibility_qkwfsev7/DEFAULT_COMPATIBILITY.json) est close **PASS** : seize commandes natives, huit paires de sorties complètes. Les schémas1/2/3, les records, le front et tous les comptes géométriques comparables sont identiques. Les [quatre lectures](compatibility_qkwfsev7/DEFAULT_COMPATIBILITY_READBACK.json), normales et `-O`, historiques et `--check-live`, passent avec le même résultat, sans réexécution native.

Cette preuve porte sur `build/v8_q4_seed_cells_20260921` (**R1**), pas sur la qualification finale R2. Les défauts ultérieurs de harnais/gate R1 ne sont ni effacés ni qualifiés par cette comparaison. Aucun fichier de moteur, test ou runner des216 sources n'a été modifié pour ce travail.

## Commandes réellement comparées

Même préfixe de64 sites de `single_000000/n8000.u16le`, K5/10, s8, masque6, front `samples`, sortie `records`. Les hashes SHA256 de fichier et FNV des64 sites sont contrôlés indépendamment. Aucune option34 n'est passée ; le mode q4 est donc l'ancien `Individual`.

| Schéma | K | q4 | Workers | Témoins | Census q3 | Bornes33 |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 1 | 5 | 28 | 1 | défaut désactivé | défaut scalaire | défaut Legacy |
| 1 | 10 | 30 | 4 | défaut désactivé | défaut scalaire | défaut Legacy |
| 2 | 5 | 30 | 1 | pair | scalar | défaut Legacy |
| 2 | 10 | 28 | 4 | rectangle-pair | boxes | défaut Legacy |
| 3 | 5 | 28 | 4 | pair | boxes | legacy |
| 3 | 10 | 30 | 1 | rectangle-pair | scalar | exclude |
| 3 | 5 | 30 | 4 | rectangle-pair | boxes | affine |
| 3 | 10 | 28 | 1 | pair | boxes | affine |

Chaque cas K5 émet448 q3 et9 q4 ; chaque cas K10 émet1549 q3 et40 q4. Les records comparés contiennent les coefficients exacts, supports, profondeurs et coquilles complètes, pas seulement un digest. C'est une comparaison bornée de compatibilité, pas une nouvelle preuve exhaustive de complétude ni un contrat de tour.

## Mémoire et exceptions de comparaison

Le seul accroissement structurel observé est celui de l'état worker : **3584→3840 octets par worker**, soit **+256 octets**. Les exécutions W4 passent de14336 à15360 octets. Ce delta est mesuré dans les huit paires, non déduit de la seule taille296 du nouveau ledger ; les alignements et le remplissage des structures participent au résultat.

Les temps sont conservés mais ne servent à aucune revendication de vitesse. La distribution par slot peut changer ; son inventaire de champs et les sommes de jobs, produits, rectangles, paires et émissions sont intégralement vérifiés. Seuls quatre postes de capacité explicitement dépendants de cette distribution peuvent différer : `work.peak_edge_buffer_bytes`, `work.q3.peak_shell_bytes`, `parallel.edge_buffer_bytes_sum`, `memory.worker_record_capacity_bytes_before_merge`. Tous les écarts chiffrés figurent dans `comparisons[].observed_differences`.

Dans cette capture, les quatre paires W1 n'ont aucun écart de ces capacités. Parmi les paires W4, le pic global de buffers varie une fois de5132 à5130 octets ; les sommes de pics et les capacités des records privés varient également. Le pic `q3.peak_shell_bytes` reste identique. Aucun autre champ non temporel, hors distribution par worker et surcoût structurel explicite, n'est exempté.

## Provenance et fermeture

Le [helper autonome](compatibility_tools/check_default_compatibility.py) porte explicitement le mécanisme `DEFAULT_COMPATIBILITY` du constructeur33, étendu aux trois anciens schémas. Son hash lors de la capture est `61094c944b08778c160ed935aad13b4bd0c83f8397e9908b5eb45f635c949e5e`.

L'ancien exécutable, sa bibliothèque et son cache correspondent à l'autorité33 close [smoke_jeqq5w9w](../q34_affine_20260921/candidate/smoke_jeqq5w9w/MANIFEST.json). Côté R1, les25 fichiers de dépendances du compilateur, leurs objets, les flags et commandes de lien, la bibliothèque, l'exécutable et le cache sont épinglés. Le périmètre précis est **609 prérequis source**, **4 entrées**, **60 artefacts** : il inclut524 headers Boost installés en lecture seule sous `build/v7_boost_gate`,64 fichiers `src`, les dépendances de sonde/oracle, CMake et16 modules Python. L'utilisation de ces headers tiers ne transfère aucune qualification v7.

Les hashes avant/après sont égaux pendant les commandes et pendant les quatre lectures. Le périmètre de sources est le graphe de compilation réel lib+probe et les lecteurs utilisés, **pas** l'inventaire complet216 des gates constructeur34. Chaque commande, environnement utile, stdout/stderr bruts, record normalisé, résultat et hash sont conservés. Aucun build n'a été reconstruit, aucune VM démarrée, aucun timing comparatif revendiqué.

Relecture sans exécution native :

```sh
python3 -B morsehgp3D_v8/receipts/q4_seed_cells_20260921/compatibility_tools/check_default_compatibility.py read morsehgp3D_v8/receipts/q4_seed_cells_20260921/compatibility_qkwfsev7 --check-live
```
