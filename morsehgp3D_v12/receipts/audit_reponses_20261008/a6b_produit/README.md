# A6b : raccord de publication, priorité des aides et admission

Lecture du **prototype non commis** capturé le 8 octobre à 15:25:23 UTC sur HEAD `150392f99`,
inchangé à 15:29:12 sur HEAD `dbdc71753`. Les dix-neuf fichiers utiles sont épinglés dans `capture.json` ;
aucun moteur, compilation, GPU, donnée externe ni porte native rejoué. **Pas de régression d'objet trouvée dans
le raccord examiné.** Cette lecture ne qualifie ni la campagne à venir ni tous les entrelacements natifs.

## Publication I et durée de vie

Le noyau diffère de celui d'A6 `30a69104a` par le pont proposé dans
[a6_indices_concurrence](../a6_indices_concurrence/README.md) : lecture de la feuille en **acquire**, écriture de
l'indice par l'aide en **release** ; `up` reste atomique relaxed. Si H lit un parent, S publie cet indice et L le
consomme, toute écriture ultérieure W du noyau vérifie `H SB S SW L SB W`, donc `H HB W`. La cohérence atomique
interdit à H de lire cette écriture future. Avec l'unique écrivain du noyau et ses reprises ordonnées, l'indice
appartient au préfixe courant. Le lemme d'ancêtre strict de la forêt historique borne aussi la remontée des lectures
mêlées. Le `find` du noyau atteint ainsi la même composante : événements, attaches et cellules sont conservés ;
les demi-compressions internes ne sont pas promises identiques à l'octet.

Les autres publications restent présentes : initialisation avant `kKernelOpen`, feuilles avant le drapeau de
tranche release acquis par l'aide, `kernel_slice` release acquis avant lecture des seuls `element[t]` déjà écrits.
L'aide annonce `hint_active++` puis lit `hint_closed`, tous deux SC. La fermeture écrit `closed=true`, attend
`active==0`, puis rend `cells`, `element` et `leaves`. Une aide réclamée mais retardée jusqu'après fermeture voit
fermé et n'accède pas aux tampons. Les échecs de noyau gardent les tampons jusqu'au retour de toute la région.
Aucun conflit ordinaire ni libération prématurée trouvé sur ces chemins. Le commentaire général de
`forest_kernel.cpp` disant encore « accès relâchés […] aux feuilles » est à mettre à jour.

## N, H et R1

`forest_births.cpp` et `forest_build.cpp` sont identiques à A6 `30a69104a`. La preuve de propriété précédente
s'applique : seul le morceau contenant le début d'une cohorte initialise **et trie** cette cohorte entière ;
les suivants sautent sa continuation. La clôture attend tous les morceaux, puis écrit les clés et la permutation
inverse sur des indices distincts. L'ouverture des feuilles/noyau attend cette clôture. H lit les attaches après
fin du noyau ; le contrôle des événements précède la CSR, et R ne libère les événements qu'après histoire et M.
La profondeur ne lit pas ces événements libérés. `registry_branches.cpp` est identique à R1 `47feedc96` :
le critère `q=d+1` et sa distinction lignes directes/générales ne sont pas changés ici.

L'admission N couvre au plus un espace temporaire par fil par `W × L × (sizeof(Sphere)+4)`, L étant la plus
large cohorte des ordres ≥2. Elle compte aussi dans `kernel_bytes` un espace séquentiel par ordre. Cette borne
conservative et son remplacement ciblé sont **déjà décrits** dans
[a6_admission_n](../a6_admission_n/README.md), sans nouvelle proposition d'algorithme ici.

Le raccord de la borne par tâche reste valide dans A6b : `forest_births`, `forest_build`, `pipeline_steps`, Pool
et Buffer sont identiques à leurs corps `30a69104a` ; la région conserve un appel synchrone par participant et
`kNumberItems=16384`. Si a_j est la plus large cohorte de taille ≥2 appartenant au morceau j (zéro à k=1), la somme
des W plus grands a_j borne donc toujours les deux buffers logiques vivants. Retirer le doublon uniquement dans
`region_bytes`, conserver `kernel_bytes` pour la voie séquentielle, admettre aussi la préparation et préserver
les marges physiques du cache : les obligations du reçu précédent restent entières. Le surcompte peut encore
refuser un budget où les buffers réellement nécessaires tiendraient ; ce n'est pas un dépassement mémoire.
Aucun patch, CTest, gain mesuré, qualification u32 ou qualification universelle de l'admission n'est transféré.

## Priorité : toutes les tranches réclamées n'est pas tous les calculs terminés

A6b préfère noyau, étapes, G, puis aides. `g_next` ne décroît pas : lorsque la recherche de G constate son
épuisement, l'aide ne devance aucune tranche **restant à réclamer**. Le nouveau compteur `hint_jobs_during_g`
teste précisément `g_next < g_total`. Il ne teste ni `g_computed` ni `g_end_ns` : des tranches déjà réclamées
peuvent toujours s'exécuter, et partager les ressources avec une aide. Une nouvelle étape peut aussi devenir
prête après son examen. Cette priorité ne prouve donc pas à elle seule l'absence de pénalité sur G.

Conseil immédiat : remplacer le message de porte « toutes après G » par « après réclamation de toutes les tranches
G » et conserver la même définition dans l'analyse. La nouvelle campagne mesurera si ce choix évite réellement
la régression des petites trames ; aucun gain ni fermeture du contrat FULL 100 ms n'est inféré ici.

La porte `levers.priorite` construit neuf Sessions à 2/3/8 fils et contrôle ce compteur. Elle n'exige pas un nombre
positif d'aides exécutées et ne force pas l'entrelacement aide/fermeture. `levers.indices` joue les aides entre les
avances du noyau, séquentiellement ; `levers.numerotation` mélange les morceaux sans concurrence. Ces portes sont
utiles pour la sémantique et la priorité, mais ne remplacent pas une porte ciblée d'aide retardée/fermeture, ni une
preuve du modèle mémoire. Le mutant `aide_avant_g` est présent ; aucun résultat de sa compilation/exécution n'a
été contre-qualifié ici. L'état de CST-0242 relève du raccord livré et de ces preuves, pas de ce fragment seul.

```sh
python -B check.py ARBRE_CAPTURE/morsehgp3D_v12
python -B -O check.py ARBRE_CAPTURE/morsehgp3D_v12
```

Le lecteur vérifie uniquement les empreintes ; sorties identiques à `results.json`. Les sources intégrales restent
dans le snapshot externe, pas dans ce reçu. Le pilote d'admission fait l'objet d'une contrelecture distincte.

Contrelecture mathématique indépendante favorable sur le pont de publication, la garde SC de durée de vie et la
distinction réclamation/achèvement de G ; aucun modèle ni natif rejoué par cette contrelecture.
