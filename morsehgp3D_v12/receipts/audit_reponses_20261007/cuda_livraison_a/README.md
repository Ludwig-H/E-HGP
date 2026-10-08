# Livraison CUDA A'' : sources et portée des preuves locales

Relecture close le 8 octobre 2026, CPU local, hors registre, `public_status=not_claimed`.
Aucun moteur, build, CTest, CUDA ou GCP lancé. Aucun état de constat modifié.

Au commit livré **8ba7d728711995e73913487adb46dea9e7dcc2a6**, les 43 fichiers
présents touchés par la livraison sont identiques à A''
**66c41ede461dcc7a235a2414079c916841c4fb46**. Les deux anciens fichiers de tri
sériel sont absents des deux arbres ; le seul ajout documentaire distinct est le
RAPPORT archivé. Les modules entiers `src/core` (9 fichiers) et `src/catalogue`
(33 fichiers) ont également les mêmes objets Git. **B'' n'est pas dans ce commit.**
La modification ultérieure af0c2ecd7 porte sur le juge G et ses portes seulement.

`BudgetReservation::swap` (`src/core/buffer.hpp:169`) échange ensemble le compte
partagé et les octets. Le multiensemble des couples (compte, octets) reste donc
identique : rien n'est réservé ou rendu par l'échange, et chaque destruction
restitue ensuite au bon compte le bon montant. Cela couvre aussi statiquement
deux comptes différents, une réservation vide ou l'auto-échange. La porte native
ajoutée teste deux réservations de 400 000 et 100 000 octets du **même budget avec
cache actif**, vérifie `used`/`held`, puis les restitutions après échange.
`CudaExecutor::grow` réserve le nouveau tableau pendant que l'ancien reste
compté ; l'échange transmet la nouvelle réservation au tableau et rend l'ancienne
par le destructeur local. Aucun changement supplémentaire lors de l'intégration.

Les 29 anciennes raisons et leur ordre sont conservés. Les nouvelles raisons
sont ajoutées aux indices 29 (`device_unavailable`, `resource_exhausted`) et 30
(`device_fault`, `invariant_violated`), après celles de la tour. Pas de
renumérotation des anciennes priorités de refus. La table explicite de la porte
`reasons` est mise à jour et cette porte figure parmi les réussites locales.

La trace conservée `build_v12_u21.ctest_gpuA.log`, close le 7 octobre à 23:55:27,
établit **686 tests exécutés réussis et 1 sentinelle LiDAR sautée**, parmi 687
sélectionnés. La formule « 687/687 » du commit masque ce saut. Configuration
Release/u21, tous modules, CUDA désactivé : le journal de construction compile
`device_stub.cpp`, sans avertissement. Les portes `block_cache_limit`, `reasons`,
`pipeline_witnesses` et `device_open` passent ; cette dernière ne joue pas CUDA.
717 portes étaient enregistrées, les 30 autres sont hors de cette sélection.
Les portes différentielles v11 de catalogue et de tour n'étaient pas configurées.
Le build a ensuite été réutilisé : son `LastTest.log` et ses binaires courants ne
sont pas épinglés comme témoins de cette ancienne exécution.

Les rapports **A''** `mutA3/report.json` et `report_core.json` ont un témoin vert
et le code 0. Leur hash de 352 fichiers est recalculé depuis le commit A'' :
`abbe985c9cca0f8a838280ba2581538085d6a561de5932f63400da1383de386b`.
Catalogue : 15 tués, dont 14 par code et `finition_prefixe_inclusif` par signal.
Socle : le seul mutant ciblé `echange_sans_les_octets` est tué par code dans
`block_cache_limit` ; ce reçu ne prétend pas rejuger les 96 mutants du socle.
Les sources concernées sont identiques dans la livraison ; aucune campagne de
mutants nouvellement exécutée sur main n'est déduite de cette identité.

Le RAPPORT A/B distingue CUDA **A'' au profil 21** et **B'' aux profils 21/24/32**
(table et entrées 22:27, 22:43, 23:42). Les anciens builds A/CUDA24 et CUDA32
précèdent les corrections finales. La mention globale « CUDA 21/24/32 » du commit
ne qualifie donc pas automatiquement A'' livrée aux trois profils. Les preuves
ASan/TSan B'' déjà relues restent dans leur reçu propre. Aucun calcul sur GPU ni
chrono G4 n'est attesté ici.

Rejeu de lecture (normal/−O identiques ; résultats intégrés à `capture.json`) :

```sh
python3 check.py --main /chemin/E-HGP --scratch /chemin/scratchpad
python3 -O check.py --main /chemin/E-HGP --scratch /chemin/scratchpad
```

Le script compare les commits, recalcule l'arbre A'' et vérifie les sept hashes
de journaux/rapports avant et après lecture. Aucun brut ni binaire n'est copié.

## Ajout distinct : livraison B'' du 8 octobre à 00:02

Au commit **c903774b1d16c3b18c15b86fa2a55d11d44b8bc8**, les quatre fichiers livrés
(`leaf_census.hpp`, `leaf_common.hpp`, `leaf_j3.hpp`, `simt.hpp`) sont identiques
à B'' **3b445b763057adc405141a6650efa14bfd4e276a**. Les modules finaux `core`
(9 fichiers) et `catalogue` (33 fichiers) sont également identiques à B''.
Le lecteur ajoute uniquement ces comparaisons de métadonnées et de sources.
La section A'' ci-dessus garde son pin et sa portée ; ses 686 réussites ne sont
pas réattribuées à B''. Aucune campagne B3 n'est reprise ici, aucun nouveau temps
n'est qualifié, aucun transfert A21→B32 ni aucune preuve FULL n'est déduit.
