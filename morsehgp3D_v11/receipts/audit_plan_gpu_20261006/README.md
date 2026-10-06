# Aide au plan GPU/CPU du 6 octobre

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Référence publiée `cf5da0e91` ;
[plan local relu](PLAN_GPU_FINAL.snapshot.md), empreinte dans
[source_pins.json](source_pins.json). Aucun build, test natif ni GCP lancé
par l’audit. Les modèles Python ci-dessous sont bornés et indépendants du moteur.

## Décision immédiate : poursuivre N1 avec une référence inchangée

L’arène peut supprimer les allocations par nœud sans changer les décisions
G1, les listes ni le catalogue. Le WIP observé choisit toutefois une autre
architecture que le plan : **1 Mio fixe par Workspace, avec repli exact en
Buffer si la place manque**, plutôt qu’un bloc dimensionné sur un majorant
qui exclurait tout repli. Il faut mesurer et qualifier cette version réelle.
La [coupe WIP et son modèle borné](arena/README.md) épinglent les cinq
fichiers observés ; leur relecture est favorable sur la propriété des listes.

- Garder les `ReadyNode` de frontière **possédés**. Leur stockage survit aux
  rondes, au plan figé et à sa relecture ; il ne doit pas devenir une vue sur
  le scratch d’un ouvrier. Seul le suffixe DFS synchrone emprunte l’arène.
- La liste du parent reste vivante jusqu’au retour des deux enfants.
  Chaque sortie, succès ou refus, restaure la marque d’entrée ; une tâche
  entière rend le curseur initial. Les queues GPU doivent garder leurs
  copies de SiteIdx, comme `TaskLeafQueue::push` aujourd’hui.
- Rendre la voie de référence explicitement sélectionnable pour l’A/B :
  capacité d’arène nulle ou option inactive ; candidat avec 2^18 SiteIdx.
  Réserver réellement les blocs avant le dispatch. Le repli reste budgété
  et les sorties coexistent avec frontières, scratchs et blocs.
- Publier au minimum pic utile de l’arène, nombre de replis et allocations
  de listes. Le nombre d’octets réservés doit rester distinct du pic utile.
  Un repli fréquent peut annuler le gain sans changer un seul octet de sortie.

**Dimensionner aussi les petits appels.** Le témoin existant `ghost()`
contient neuf sites et prépare 37 tâches finales pour K1/leaf4. En lui
appliquant W16 et un budget de 16 Mio, les 16 arènes fixes prendraient déjà
16 Mio, avant les scratchs et la frontière : l’admission serait impossible.
Ce constat vient du code, pas d’un test natif exécuté ; la porte existante
utilise W4 sans limite mémoire. Les tâches sont pourtant déjà des feuilles,
sans suffixe DFS consommateur d’arène. Prévoir une capacité adaptée au
travail restant, en particulier zéro si `suffix_memory_bound=0`, et une
voie explicite sans arène. Un surcoût fixe par ouvrier ne doit pas devenir
un minimum de mémoire imposé aux petits nuages.

**Deux mutants du plan sont à remplacer.** Avec une racine déjà possédée à
profondeur d, le suffixe a une borne `count × (3B − d)` SiteIdx ; `3B+2`
n’est pas un minimum. Passer à `3B+1` peut rester sûr. De même, oublier le
rewind peut seulement provoquer davantage de replis dans le WIP fixe : le
dump reste juste et aucun refus n’est garanti. Tester la restauration du
curseur et le pic sur une petite pile dont l’occupation exacte est connue,
puis une capacité qui force le repli ; vérifier résultat identique et
comptabilité du repli. Garder les portes de panne mémoire et d’absence de
publication partielle.

## Mesurer le mécanisme, puis le gain

Le quotient historique CPU/nœud entre feuilles de 16 et de 24 n’isole pas
les allocations : ces deux exécutions retirent simultanément **510 822
nœuds et 131 150 311 tests G1**, et le CPU enregistré couvre index,
domaine et forêts. Cela justifie une expérience N1 ; cela ne démontre
pas sa cause ni son gain attendu.

Pour rendre T1 décisif :

1. A/B à taille de feuille fixe, mêmes entrées et mêmes compteurs logiques.
2. CPU du fil au début et à la fin de chaque tâche de génération ; CPU des
   feuilles mesuré séparément. `walk_thread_ns = CPU tâche − CPU feuilles`,
   avec préfixe de frontière publié séparément. Des lectures aux seules
   bornes de feuilles omettent les extrémités de la tâche.
3. Activation spécifique des microhorloges. La sonde passe **toujours**
   `&timings` à `prepare_full_domain` : conditionner l’instrumentation au seul
   pointeur `timings` l’allumerait aussi dans les bras censés mesurer le mur
   sans cette instrumentation.
4. Juger le gain sur le périmètre complet aussi : allocation/préparation des
   arènes et mémoire réservée incluses. Un parcours encore lent ne suffit pas
   à attribuer tout son temps restant à la latence mémoire.

[Preuve de source et des travaux comparés](measurement/proof.json),
rejeu `python3 -B measurement/replay.py` puis `python3 -B -O measurement/replay.py`.
Le lecteur dépend des reçus S4 versionnés et des objets Git épinglés.

Les builds, ASan/UBSan et chronos W1/W8 prévus « en local » dans T1 sont à
placer sur G4, conformément à la consigne du chantier. Les 4 200 secondes
n’ont pas encore un budget complet de build, portes, mutants et mesures :
prévoir des commandes reprenables ; aucune tenue ni dépassement certain
n’est déduit de cette seule liste.

## T3 : isoler d’abord la répartition des feuilles tardives

`fill_kernel` n’est pas codé avec une grille fixe d’un bloc : la grille vaut
`ceil(fill_jobs/32)`. Un seul bloc observé signifie ici peu de feuilles
résolues qui ont débordé de leur scratch, sélectionnées après le comptage.
Le noyau traite actuellement une feuille entière par fil.

Une première ablation est plus petite qu’un port coopératif J3 : **une
feuille par bloc, un seul fil actif**, même `run_leaf`, mêmes préfixes et
mêmes emplacements d’écriture. Les autres fils sortent avant le travail ;
l’entrée devient `list[blockIdx.x]`, avec garde `blockIdx.x < list_count`.
Conserver l’index actuel `blockIdx.x*blockDim.x+threadIdx.x` avec des blocs
de 32 et le seul fil 0 sauterait 31 feuilles sur 32. Le nombre de blocs
vient du nombre de feuilles sélectionnées, avec la borne de grille vérifiée. Cela répartit les feuilles indépendantes sans prétendre
paralléliser le calcul interne d’une feuille. Comparer explicitement la
liste, les statuts et les sorties, puis mesurer ; aucun gain n’est acquis.

Ne pas agrandir globalement `kThreads` : `count_kernel` réduit un warp et
publie seulement depuis `threadIdx.x == 0`. Des blocs de plusieurs warps
avec ce corps perdraient les autres contributions. La variante fill doit
avoir sa propre géométrie de lancement, sans modifier cette réduction.

Le plan réclame une empreinte **à chaque passe** résidente. La sonde actuelle
rend seulement statut et temps pour les passes intermédiaires, puis sérialise
la dernière. Ajouter un contrôle canonique par passe hors chrono, ou annoncer
explicitement la portée dernière passe ; ce point compte particulièrement
pour l’anneau et ses buffers réutilisés. Ne pas transférer le PASS historique
à cette future vérification des réutilisations.

## O7 : hiérarchie et comptes doivent être préservés séparément

Les détails et témoins de la contrelecture mathématique sont dans le
[complément mathématique](mathematics/REPORT.md). L’arbre de Kruskal, après contraction de ses
chaînes de fusions de même rang, peut reproduire les plateaux atomiques.
Les naissances restent des feuilles ; les niveaux et la numérotation
canonique restent exacts. Cette équivalence n’autorise pas à jeter les
métadonnées des cellules originales.

Un triangle K1 donne un témoin concret : les trois arêtes Gabriel ont des
niveaux distincts ; la dernière est une continuation dans une composante
déjà fusionnée et disparaît de l’arbre couvrant minimal. La hiérarchie est
préservée mais son événement, son plateau et ses comptes de composantes
touchées ne se reconstruisent pas depuis les seules arêtes retenues.

`ancestor_find_steps` dépend du parcours. Les compteurs actuels
`ancestor_activations` et `ancestor_unions` ont au contraire des valeurs
logiques reconstructibles : nombre de fusions basses traitées et somme
de leurs arités jusqu’au dernier niveau demandé. Conserver ces définitions
ou nommer de nouveaux diagnostics physiques, sans changer silencieusement
les champs existants. Aucun changement du registre formel n’est fait ici.

## Autorisations et périmètre pour avancer

T1/T2 et les prototypes bornés n’attendent pas une nouvelle décision de
précision : la v11 active est u21, avec u18/u24 qualifiés séparément. Le
profil u32 évoqué dans le plan n’est pas un prérequis à ces tranches ; B25
est déjà refusé par CMake et le garde commun des types. Une porte B25 juge
ce refus de profil, pas une voie CUDA qui aurait accepté la configuration.

Conserver le périmètre FULL explicite et comparer résident à résident,
puis processus neuf à processus neuf. Aucune de ces expériences ne ferme
les 100 ms par anticipation. Le transfert GPU du catalogue seul laisse
l’étage tree à traiter. Les optimisations des sorties et de l’écriture
restent utiles sous leur propre périmètre, sans remplacer cette priorité.
