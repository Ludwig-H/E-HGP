# Revue du transport GPU — snapshot du 4 octobre 2026

Source : snapshot développeur du 4 octobre à 14:50:18 UTC, base Git
`66372e621dcee58daaa7d7309875ab157894acf4`. Les fichiers WIP copiés ne sont
ni une publication Git de la voie CUDA ni une qualification native.
`SOURCE_BEFORE.json` rattache chaque copie au manifeste du snapshot parent.
`SOURCE_AFTER.json` rehache cette même source figée ; il ne décrit pas un
nouvel état LIVE.

**P1 : les payloads GPU échappent au plafond mémoire.**
`sources/src/catalogue/leaf_batch_cuda.cu:92–95` ajoute `count * sizeof(T)` à
un compteur puis appelle `cudaMalloc`, sans réservation ni plafond device.
Les allocations d'entrée, des préfixes, du temporaire CUB et des sorties
empruntent toutes cette voie (:132–136, :157–165, :179–182). Le
`MemoryBudget` reçu ne protège ici que les `Buffer` hôtes (:121–124,
:190–195), alors que l'architecture exige l'admission des tableaux variables
avant allocation (`sources/docs/ARCHITECTURE.md:161–172`). Un OOM CUDA rend
un refus ; il ne remplace pas la limite de mémoire déclarée par l'appelant.

Correction conseillée : réserver effectivement ces payloads dans le budget
commun, ou définir un plafond device explicite avec son compte de réservation
et sa libération RAII. Garder produits et sommes avant `cudaMalloc`, et
maintenir les réservations jusqu'à la libération des tableaux. Pour les
préfixes et sorties, protéger également les totaux et le domaine admis par
CUB/la grille. Aucune entrée publique causant un débordement ou une sortie
FULL erronée n'est démontrée dans cette revue.

`device_bytes` est la somme des tailles de ces tableaux au succès, pas un
pic global hôte + device. Le contexte/runtime CUDA, la pile device demandée
à 16 KiB et un éventuel staging implicite n'y figurent pas. Il faut annoncer
ces exclusions et mesurer séparément la mémoire GPU libre/occupée et ses
pics ; leur coût opaque n'a pas à être compté exactement dans le budget des
payloads. Aucun tampon pinned explicite n'est créé dans cette source.

Les autres contrôles sont favorables par lecture : la mémoire hôte
simultanée passe par les `Buffer`/pages budgétés ; une feuille non résolue
contribue zéro sortie et zéro ledger GPU, puis est entièrement reprise sur
CPU ; count et fill ont les mêmes entrées immuables ; les sorties restent
privées jusqu'au succès de l'assemblage. Les gardes publiques imposent des
multiplicités unitaires et K dans 1..12 ; les vues de transport internes
supposent les listes de sites et boîtes déjà certifiées.

Un diagnostic doit être clarifié : `single_pass.cpp:228` publie
`geometry_passes=1`, mais les feuilles résolues sont examinées dans count
**et** fill (`leaf_batch_cuda.cu:56,82`), avec reprise CPU pour les feuilles
non résolues. Conserver le ledger logique compté une seule fois ; distinguer
le front unique des examens physiques. Cela n'est pas un défaut géométrique.

`MATRIX.json` couvre neuf contrats. `review_transport.py` rejoue seulement
les vérifications de provenance, de présence des ancrages et de cohérence
de cette matrice, en Python standard normal et optimisé. Il n'exécute ni C++,
ni CUDA, ni prédicats, ni FULL. Aucune compilation, VM, mesure ou
qualification de temps n'a été faite.

```sh
python3 -B -S review_transport.py
python3 -B -O -S review_transport.py
sha256sum -c SHA256SUMS
```

Le ledger inventorie tous les payloads hors lui-même et `SHA256SUMS` racine.
`SHA256SUMS` inclut le ledger et exclut uniquement son propre chemin racine.
