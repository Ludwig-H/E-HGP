# T2d-C : prélecture des sorties en flux, 8 octobre 2026

Capture d'un **prototype non livré**, déclaré issu de `8dc5d6b16`. Lecture statique des sources et des traces du
développeur ; aucun moteur, compilateur, microbanc ou GCP exécuté par cet audit. Aucun défaut de valeur ou de durée de
vie identifié dans les chemins examinés. Ce résultat ne certifie ni les erreurs du pilote CUDA ni une accélération.
Les empreintes complètes sont dans `capture.json` ; aucune source complète ni trace contenant un chemin privé copiée.

**Valeurs, copies, budget.** `stream_staged` attend l'événement d'une case avant de la consommer et la réutilise après
le retour du consommateur. Le Pool est synchrone. CUDA enregistre l'événement après la copie ; un refus du flux
attend toutes les copies restantes, et le destructeur attend avant de rendre événements et mémoire épinglée.
Les transferts du rejeu hôte précèdent désormais Copy/Replay : la destruction de sa LeafStage ne laisse pas de source
H2D en vol. Les refus des allocations anticipées libèrent des sorties hôte que ces noyaux n'utilisent pas ; leurs
destinations appartiennent à l'état appareil résident. Les tableaux finaux ont leur taille exacte, les tranches ne
coupent aucun élément ; les niveaux gardent tous les mots de `LevelWords`, passent par la même fabrique exacte et sont
écrits directement aux rangs 1..L−1. Le rang zéro est écrit séparément. Aucune réduction de précision observée.

Les réservations restent comptées avant allocation. Cependant `stage(2 * 8 Mio)` impose **16 Mio de staging** au flux
(jusqu'à 64 Mio déjà conservés), même si le contexte détenait auparavant 1 Mio. « Mémoire déjà réservée » ne décrit
donc pas tous les appels. Les sorties vivent aussi plus tôt, pendant le dernier lot et avant les allocations de
finition : le pic et des refus sous petit budget peuvent changer, sans dépassement du plafond. Les tests de cette
capture ne prouvent pas une équivalence des refus sous budget serré. La fusion devra préserver l'évolution distincte
de main vers budgets hôte/appareil séparés (`device_cuda.cu`, `catalogue.hpp`) ; elle n'est pas couverte ici.

**Chronos et ablation.** Le flux compte `transfert = mur du flux − consommateurs de publication`, puis compte ces
consommateurs à part. C'est une partition du mur, pas une mesure DMA. `outputs_ns` porte allocation/premier toucher ;
sa soustraction aux étapes attribue à ce poste le temps pendant lequel un noyau peut avancer en parallèle. Le net
n'est donc pas la durée propre du noyau. Les copies ordinaires, niveaux, sorties et adoption restent dans le mur du
catalogue. Les étapes CPU réintègrent ces postes dans `assemble_ns`, avec diagnostics appareil nuls.
`kPrefaultOutputs=false` retire seulement le premier toucher, **pas l'allocation anticipée**. La compaction des chaînes
est un **quatrième levier**, à déclarer et isoler en plus des trois annoncés initialement. Déduire une fraction DMA
à partir du débit d'un autre microbanc reste une hypothèse ; ni ces durées ni les essais locaux ne constituent une
mesure causale sur les passes de la session K.

**Complétude de la réparation.** Les positions compactées sont croissantes. Sous les invariants du producteur,
1≤i<n et une paire mal ordonnée appartient à une chaîne de voisins incertains. `chain_bounds` étend les deux côtés
tant que `key_order=0` ; toucher une frontière non globale agrandit la fenêtre, jusqu'au tableau entier au besoin.
Il retrouve donc l'intervalle maximal de l'ancien `chain_around`. `covered` élimine ses autres marqueurs ; les chaînes
restent disjointes. La limite existante n<2³²−1 protège le doublement et les conversions. Le tri exact et la
revérification finale subsistent. Contrelecture math indépendante favorable (sans rejeu natif).
Pour M marqueurs, Q chaînes et R éléments réparés, les lectures directes de `collect_chains` valent
**4M + 8Σ|fenêtre lue| + 12R octets**, auxquels s'ajoutent notamment les lectures de contrôle du scan et les transferts
communs du tri exact. Les fenêtres doublées, halos compris, coûtent O(R+64Q) mots ; le scan ajoute O(n) travail.
Ce n'est pas exactement 12R octets, ni un gain garanti avec beaucoup de petites chaînes. `bounds` réserve 2M entrées,
pas seulement 2Q : mémoire comptée, mais ne pas annoncer toutes les allocations comme minimales.

**Traces effectivement disponibles.** Le journal CTest clos à 03:45 contient 13 Passed : 12 groupes et l'inventaire,
11 734 contrôles, zéro échec. Parmi eux : chaîne longue 25, pages 64, flux simulé 82 contrôles. Ce dernier utilise
1/2/3 cases de 256 octets, copie différée à l'attente et empoisonnement préalable ; il compare les catalogues.
`device_open` joue seulement le refus appareil indisponible. Les caches déclarent Release/u21, CUDA OFF pour ces tests,
CUDA ON/module catalogue pour la bibliothèque dont le journal atteint la fin de construction. Ce sont des traces
lues, sans code externe CTest archivé ni chaîne de compilation indépendante reconstituée. Aucun GPU exécuté, profil
24/32, test d'erreur CUDA ou gain chronométrique nouvellement acquis par ce reçu.

Rejeu des empreintes, gardes textuelles et métadonnées, sans compilation :

```sh
python check.py --prototype /CHEMIN_DU_CHANTIER_T2D_C
python -O check.py --prototype /CHEMIN_DU_CHANTIER_T2D_C
```

Les deux sorties doivent être identiques au champ `result` de `capture.json`. Le lecteur refuse toute dérive de ses
sources ou journaux épinglés. Ses gardes textuelles rendent la lecture localisable ; elles ne remplacent pas une
preuve d'exécution CUDA. Aucun nouvel état du registre d'audit proposé.
