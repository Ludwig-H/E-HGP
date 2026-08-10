# Réception du diagnostic G4 `face-owner` à `23379d4`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=g4_spot_reported_and_cpu_local`,
`profile=device_faceowner_oracle_k_le_6`, `mode=audit_and_bounded_qualification`,
`public_status=not_claimed`.

Snapshot réaudité : `23379d408bc903c41607334a08bcae56dcd59aa8`, puis documentation
seule à `f6cb562680138ee37a8ef9684453af73e3dad946`. Le code device reste celui de
`7022298` :

- `faceowner_device_kernel.cu=e4c83aa20cec97a15cd5315bf6c8d72694112df44cf17b9a4e75ad07cf3dd028`;
- `faceowner_device_qualification.cpp=03eabfb4233ab5da94b881fdddb58f0cf7625b084112269683a1f2ff90058527`;
- `saturated_fold_faceowner.hpp=436b88ebe0881d024c41222a379e5081640ed668e6fd09db86323a96d79def5f`;
- `CMakeLists.txt=5be3a08e6e0e79ea97463baad40a06b4144572c4305f100814cef8ca3ee65d49`;
- note de session Claude `0c335688e6bb7282356aa698d477deaa5a3929a0d28ea4837d25709b44a5da19`.

## Verdict utile

Le résultat positif à conserver est précis : Claude rapporte quinze accords
arête par arête, cinq ordres sur chacun de trois catalogues, entre le flux GPU
et `collect_edges` CPU, plus la mort du mutant qui retire la dernière arête.
Cela qualifie utilement le calcul device **relativement au même catalogue
borné** et confirme que le tri, la réduction de l'owner et la déduplication
sont une excellente charge GPU.

Ce résultat ne qualifie pas encore un backend produit. Le catalogue est un
`partial_refinement`, le fold hybride n'est pas appelé, le DSU, les records et
les marqueurs restent sur CPU, et le chrono device exclut allocation, H2D,
D2H et rejeu. Surtout, la validation hostile, la borne VRAM et la porte CUDA
permanente restent inchangées depuis l'audit de `7022298`.

Le libellé exact recommandé est donc : **diagnostic G4 positif du flux
`face-owner` sur trois catalogues bornés; qualification produit et mémoire non
reçues**.

## Sécurité GCP reçue en lecture seule

Le contrôle indépendant de l'API GCE après la session donne pour
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` :

- `g4-standard-48`, `SPOT`, label `project=e-hgp`;
- `instanceTerminationAction=STOP`, `maxRunDuration=7200 s`;
- `automaticRestart=false`, maintenance `TERMINATE`;
- démarrage `2026-08-10T14:34:45.764Z`;
- arrêt `2026-08-10T14:39:53.775Z`;
- état final certifié `TERMINATED`.

La génération visible a donc duré environ 5 min 08 s, et non « environ quinze
minutes ». La clé publique de session n'apparaît plus dans OS Login : sa
révocation distante est reçue. Les fichiers privé et public subsistent en
revanche dans le scratchpad local; Claude doit les supprimer sans toucher aux
autres clés. La cible initiale de l'auditeur,
`europe-west4-a/ehgp-blackwell-spot`, est elle aussi `TERMINATED`; sa tentative
avait été refusée avant démarrage par le quota consommé par la session
concurrente. Aucune cible n'a été démarrée ou arrêtée pendant ce réaudit en
lecture seule. La clé OS Login de cette tentative avortée de l'auditeur a été
révoquée explicitement et ses deux fichiers temporaires supprimés; les deux
cibles labellisées sont `TERMINATED`. Le coupe-circuit invité n'est pas prouvé
par les champs GCE
consultés; conserver le reçu brut du script est sa bonne preuve. L'archive
source de la session existe encore localement : 1 027 904 octets, SHA-256
`0717c3948ce6e99e399e6a78b4740cf9034243aea417d078c1d522195a6ced60`;
les huit fichiers pertinents correspondent aux empreintes de `7022298`. Ce
digest doit rejoindre le reçu versionné.

## Masses CPU recertifiées

Un build Release CPU frais de `f6cb562`, CUDA désactivé, produit le binaire
`c33bd671758c4686664c9a3155cc522cc4c9ede6b63d95e592f00dd0d6ceba4b`.
Le build des cibles pipeline, gate et qualification réussit; les sept mutants
`face-owner`, le refus d'option inconnue et le refus sans CUDA passent 9/9.
Les commandes `saturated_pipeline --join faceowner --compare-joins 0`, avec
`smax=11`, `K=5` et la graine `20260810`, donnent :

| n | coord | générateurs | incidences | arêtes dédupliquées | identités |
| ---: | ---: | ---: | ---: | ---: | --- |
| 64 | 40 | 7 873 | 3 030 554 | 1 319 076 | respectées |
| 200 | 58 | 40 007 | 17 282 892 | 7 385 988 | respectées |
| 400 | 73 | 99 942 | 44 258 951 | 19 073 174 | respectées |

Les deux premières lignes reproduisent exactement la note G4. La troisième ne
reproduit pas les `44,5 M / 13,9 M` publiés. `coord=73` est la valeur par défaut
effective du harnais à `n=400`; des contrôles supplémentaires à `coord=74` et
`coord=40` donnent respectivement `44 138 271 / 18 994 918` et
`43 778 320 / 18 685 370`. Aucun de ces cas ne produit `13,9 M` arêtes.

La conclusion device peut rester positive si le log brut montre une autre
commande, mais la ligne `n=400` doit être corrigée ou accompagnée de sa
provenance exacte. Il faut versionner les sorties par ordre, pas seulement des
totaux arrondis.

## Le chrono est intéressant, mais le rapport de vitesse ne l'est pas encore

Les événements CUDA encadrent bien l'émission, le tri puis la réduction et les
étoiles. Ils excluent volontairement les constructions de vecteurs device,
les transferts d'entrée, la copie des arêtes vers l'hôte et tout le replay. Le
temps CPU publié englobe au contraire le fold complet et la copie de toutes les
arêtes dans le reçu. `49,25 ms` contre `8,67 s` compare donc deux périmètres
différents.

Le résultat constructif est que 44 millions d'incidences constituent une
charge device très favorable. Pour pouvoir écrire « le mur du join est
effacé », séparer et publier sur les deux chemins : préparation, H2D, émission,
tri/groupage, émission-déduplication, D2H, rejeu DSU, records et mur total. Le
rapport produit est le mur total GPU sur mur total CPU, tandis que les temps de
phase expliquent ce rapport.

## La VRAM annoncée n'est pas observée ni bornée

Le `~1,07 Go device` est calculé par `result->device_bytes`; aucun high-water
device n'est échantillonné. Le harnais appelle `cudaMemGetInfo` une seule fois
avant la boucle des ordres, puis teste pour chaque ordre la formule `56*I` avec
cette même valeur. Les buffers explicitement simultanés atteignent pourtant
au moins les entrées plus `76*I + 16*S`, jusqu'à environ `92*I` si chaque
signature est unique, avant les workspaces Thrust.

Le seuil de 70 % est donc un avertissement heuristique, pas une admission. La
réparation la plus courte est toujours de trier directement les incidences par
`(signature,activation_rank,generator)`. Le premier élément de chaque groupe
est alors l'owner; une transformation compacte produit directement les arêtes.
Cela retire `candidates`, `reduce_by_key`, ses deux sorties, `raw_edges` et
`keep`. Ensuite seulement, requêter les workspaces CUB et allouer tous les
buffers dans une arène plafonnée rend le budget prouvable.

## Portes immédiatement rentables

1. Sous `MHGP3V_ENABLE_CUDA=OFF`, garder seulement le rejet « kernel absent ».
   Sous CUDA, enregistrer un nominal à masses planchées et le mutant
   `drop-edge` attendu en code 1. Le CTest actuel est inconditionnel et échoue
   justement quand le kernel est présent. Les options CUDA disciplinaires sont
   actuellement appliquées seulement à `device_wavefront`; les appliquer aussi
   à la cible `faceowner` ou documenter pourquoi ce kernel entier n'en dépend
   pas.
2. Valider avant `incidence_offsets.back()` toutes tailles, offsets, rangs,
   binomiales, membres denses, rangs d'activation, lots, conversions et grille;
   la masse nulle ne contourne pas ces gardes.
3. Rendre les événements CUDA RAII, vérifier chaque statut et le cardinal de
   réduction, et publier seulement après la dernière synchronisation réussie.
4. Versionner un reçu par run : SHA commit/source/binaire, commande complète,
   digests nuage/catalogue, versions CUDA/driver, identité GPU, masses et digest
   d'arêtes CPU/device par ordre, timings par phase, mémoire réellement
   plafonnée, codes mutants, génération GCE et reçus start/stop.
5. Rejouer les arêtes device dans une seconde instance du fold et comparer
   partitions, records et marqueurs, sans réutiliser les arêtes CPU comme
   entrée de ce rejeu.

Le candidat device distinct est un compteur exact des recouvrements de postings
pour un `query_mask` reçu : il rend de vrais `GeneratorId` incidents et laisse
à l'hôte le DSU, le pruning dynamique et le commit atomique du lot.
Il ne constitue pas encore un backend d'échelle général : sur ces mêmes trois
catalogues, le mode tout-requête émet 74,9 à 129,9 fois plus de hits que
`face-owner`. Le dispatcher doit donc calculer `H_query(mode)` exactement, en
distinguant count/cover et dirigé/canonique, et ne
choisir cette voie que sous admission; sinon l'owner demand-driven, le fallback
CPU ou une autre forme exacte reste nécessaire. Le filtre `cover` dépend du
masque; l'owner sparse exige un certificat batch ou le carrier strict dérivé de
la source complète. Contrats :
[`NOTE_SOLUTION_GPU_FALLBACK_POSTINGS_COUNT_20260810.md`](NOTE_SOLUTION_GPU_FALLBACK_POSTINGS_COUNT_20260810.md) et
[`NOTE_SOLUTION_GPU_OWNER_DEMAND_DRIVEN_20260810.md`](NOTE_SOLUTION_GPU_OWNER_DEMAND_DRIVEN_20260810.md).
