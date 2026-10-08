# A6 N/I : propriété des cohortes, publication des indices et durée de vie

**Aucun conflit mémoire concret trouvé par cette lecture ; un pont de preuve portable est proposé pour I.**
Prototype local `69dd8e9021fb9b4203e72374629aaf6c4c6610dd`, base produit `72f622a55` et porte native de terminaison.
Douze fichiers capturés à 10:27:44 UTC, inchangés à 10:35:30. La livraison a été réemballée sur `bdfca8fb1` pendant
la lecture : les présentes conclusions restent attachées aux hashes des corps, pas au nom mutable du patch.
Aucun produit modifié, compilation, moteur, données XYZ/IDs ou GCP. Aucune qualification native transférée.

La conception développeur retient **N** (numérotation parallèle), **I** (aides calculant des indices de racines),
**H** (historique parallèle). La proposition MSF de [notre précédent reçu](../a6_noyau_raccord/README.md) est une
alternative distincte, écartée par le développeur pour cette tranche notamment à cause de la perte de recouvrement.

## N : la cohorte entière appartient à un seul morceau

`number_births_range` (`forest_births.cpp`, SHA `0cd7b835…`) commence à la première cohorte dont le début est dans
le morceau, puis initialise **et trie toute cette cohorte**, même au-delà de sa frontière. Le morceau suivant saute
la continuation ; il n'initialise pas aveuglément sa plage nominale. Deux morceaux ne peuvent donc écrire la même
case d'`order_out`. Le soupçon « B écrase la fin triée par A » ne se réalise pas dans ce corps.

Les cohortes couvrent les indices une seule fois. Chaque permutation conserve son domaine ; `kNumberClose` attend
tous les morceaux via le graphe/compteur de fin, puis remplit des places distinctes de `birth_node` et `birth_key`.
`kKernelOpen` attend toute cette clôture. Les sphères et indices temporaires sont locaux à l'appel ; au plus un
appel de numérotation par fil, couvert par le terme `threads × widest × (sizeof(Sphere)+4)` de l'admission région.
Les compteurs sont par ordre/fil, fusionnés après la région. Ce raisonnement ne qualifie pas une allocation réelle
à la borne maximale ni toutes les pannes d'allocation.

## I : accès et durée de vie

Les points de synchronisation du corps capturé sont cohérents :

- Un noyau à la fois par ordre, obtenu par CAS du drapeau `kStepBusy`. Sa restitution publie les écritures pour
  une reprise par un autre fil. Initialisation de l'union-find avant publication de `kKernelOpen`.
- Les écritures ordinaires initiales des feuilles précèdent le drapeau de tranche `kSliceLeaves` en release ;
  l'aide acquiert ce drapeau. Une tranche sans feuilles est résolue par le noyau, et n'est pas admissible aux aides.
  Après publication, feuilles et champ `up` utilisent `atomic_ref<u32>` pour les accès concurrents.
- `element[t]` n'est lu par l'aide que si `t < processed`, obtenu par acquisition de `kernel_slice`. Sa seule
  écriture est déjà terminée ; les cellules suivantes ne réécrivent pas cet élément.
- La fermeture écrit `hint_closed=true`, puis attend `hint_active==0`, les deux en séquentiellement cohérent.
  L'aide annonce `active++` avant de lire `closed`. Si elle voit ouvert, elle reste comptée jusqu'à sa sortie ;
  si sa réclamation précède la fermeture mais son démarrage la suit, elle voit fermé et ne touche aucun tampon.
  `cells`, `element` et `leaves` ne sont rendus qu'après cette garde. Les échecs anticipés gardent les tampons
  jusqu'à la fin de la région et au retour de tous les participants.

Cela ferme les conflits ordinaires et les cas de libération examinés, sans faire des portes existantes une preuve
exhaustive de tous les entrelacements C++.

## Lemme et renforcement proposé : distinguer arbre final et préfixe courant

Considérer la forêt historique des unions, perdant→survivant, jamais réécrite. Chaque `up` non trivial écrit par
union ou demi-compression désigne un **ancêtre strict** dans cette forêt. Mélanger d'anciennes versions ne peut donc
créer de cycle : `find_read` progresse vers un ancêtre, au plus la profondeur historique (bornée par l'union par
taille). Une ancienne racine est un représentant valide si ses unions sont antérieures à sa consommation.

L'appartenance à l'arbre **final** ne suffit pas, seule, pour l'identité des coupes. Témoin sémantique : avant deux
cellules `t=(0,1)` puis `u=(0,2)`, la composante `{2,3,4}` a racine 2, et 0/1 sont isolées. Si t remplaçait déjà 0
par sa future racine 2, sa coupe serait `{0},{1,2,3,4}` au lieu de `{0,1},{2,3,4}` ; pourtant u écrit `up[0]=2`
dans les deux traces et la partition finale coïncide. **Ce n'est pas une contre-exécution native ni C++ complète
du prototype** : cela isole l'obligation de préfixe dans l'argument de correction.

Avec les accès `relaxed`, cette antériorité est naturelle dans un entrelacement opérationnel, mais le transfert
`up → aide → feuille → noyau` ne crée pas, seul, de relation happens-before C++. Le patch proposé change seulement
deux expressions : publication de l'indice en **release**, `load_leaf` en **acquire** ; les accès `up` restent relaxed.
Si le noyau consomme cet indice, chaque lecture H d'un parent par l'aide précède sa publication S ; S synchronise
avec la consommation L. Pour toute écriture W d'un parent postérieure à L par le noyau, `H HB W`. La cohérence
lecture-écriture interdit alors à H de lire W ou une version ultérieure. L'unique écrivain du noyau et ses reprises
ordonnées rattachent ainsi les parents observés au préfixe courant. Références normatives :
[C++20 N4861, synchronisation et cohérence](https://timsong-cpp.github.io/cppwp/n4861/intro.races#17),
[ordres acquire/release](https://timsong-cpp.github.io/cppwp/n4861/atomics.order#2).

Le `find` du noyau retrouve dès lors la même racine courante depuis l'indice ou la feuille originale ; égalité des
racines, tailles, départage, événements, attaches et sommets restent identiques par induction. Les demi-compressions
internes ne sont pas promises identiques à l'octet. Le patch est une proposition de fermeture de preuve, **pas la
réparation d'un échec natif démontré**. Aucun gain, identité assembleur ni coût nul de ces ordres n'est revendiqué.

## Portes et portée

La porte `levers.indices` compare des âges d'instantanés, mais appelle les aides séquentiellement avant le bloc du
noyau ; elle ne force pas la course aide/fermeture. `levers.numerotation` mélange l'ordre des morceaux sans les
exécuter concurremment. La porte de graphe vérifie les dépendances des étapes, pas le protocole des aides hors Step.
Les portes Pool d'identité/déterminisme comparent aussi forêts, compteurs et cibles : utiles, sans exploration
exhaustive du modèle mémoire. Les annonces TSan de la **porte de terminaison** ne qualifient pas automatiquement I.
Avant adoption native, un entrelacement forcé aide retardée/fermeture et une vérification concurrente des aides
compléteraient directement ces obligations ; rien n'a été exécuté par cet audit.

Contrelecture mathématique de perf_math favorable sur le lemme, la garde de fermeture et le pont release/acquire.
`check.py` vérifie uniquement les douze hashes et l'application exacte du patch en répertoire temporaire :

```sh
python3 -B check.py ARBRE_SOURCES_CAPTURE
python3 -B -O check.py ARBRE_SOURCES_CAPTURE
```

Sorties identiques à `results.json`. L'arbre externe attendu contient `src/tower/` et `tests/tower/` ; aucune copie
des sources entières n'est ajoutée au reçu. Les preuves mathématiques et leurs limites sont le texte ci-dessus.
