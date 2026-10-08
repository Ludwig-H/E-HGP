# T massif : réutiliser les rangs des sites déjà calculés par C

**Proposition, aucun changement moteur ni gain mesuré.** Lecture des sources livrées
ea62 et des métadonnées [L1r admises](../session_l1r_admission/README.md). T représente
59,34 à 84,03 % de TMVR sur les neuf derniers succès GPU K5 (sept chauds, deux froids).
Meadow porte T=20,000 s sur TMVR=23,802 s. Ces prises ne ventilent pas T par ordre ou
entre numérotation, feuilles, noyau et histoire : elles n'identifient donc pas à elles
seules le coût dominant interne. Les compteurs `ForestWork.cohorts/max_cohort` et
`ForestPhysical.births_ns` existent, mais ne figurent pas dans ces JSON FULL. Les
anciens profils ng locaux ne leur sont pas transférés.

## Un calcul exactement répété

`catalogue/finish_driver.hpp:91–100` trie les sites par (x,y,z), puis
`LexRankKernel` écrit `lexrank[SiteIdx]=rang+1`. `SiteKeyKernel` encode exactement
`x` en mot haut et `(y<<32)|z` en mot bas, pour tous les profils B≤32. Les sites sont
distincts. Ce calcul sert à la canonicalité des supports du catalogue.

Pour K1, `tower/stage.cpp:144–147` publie toutes les naissances `birth_key[s]=s` et
`birth_rank[s]=0`. Pourtant `forest_births.cpp:38–51` les retrie par `std::sort` XYZ.
Le rang du centre d'une naissance K1 est précisément celui de son site : l'ordre
déjà calculé dans C est donc l'oracle exact de cette numérotation.

**Raccord proposé :** publier un instantané possédé des rangs C, lié au Cloud de la
passe, puis pour chaque SiteIdx s écrire `birth_node[s]=lexrank[s]−1` et
`birth_order[birth_node[s]]=s`. La copie actuelle des clés donne les mêmes
`birth_key` canoniques. Toutes les valeurs sont identiques à la voie existante ; le
noyau reçoit les mêmes IDs, conserve ses départages par taille, événements,
historiques et sorties M/V/R. `cohorts/max_cohort` comptent toujours la même cohorte,
même si son tri est évité. Les comparaisons exactes des centres K≥2 restent hors de
ce raccourci ; aucune approximation rationnelle.

Preuve : les deux tris définissent le même ordre total strict XYZ sur les mêmes
sites. Son unique rang 0..N−1 est `lexrank−1`. L'inversion de cette permutation est
donc exactement celle que `number_births` calcule après son tri. Le modèle
[model.py](model.py) confronte encodage entier C et oracle par tuples : les 255
sous-ensembles non vides du cube binaire et trois nuages aux bits hauts u21/u24/u32.
Il refuse six tables corrompues ou périmées ; un sous-ensemble de naissances ou des
rangs non tous nuls restent dans la voie générique. Aucun natif exécuté.

## Coût et garanties à conserver

On supprime **un tri comparatif de N sites dans T**, remplacé par O(N) lectures et
écritures ; cela ne borne pas T tout entier. Le calcul de C existe déjà, mais son
export n'est pas gratuit : instantané hôte **4N octets**, soit 24 724 364 octets pour
Meadow. Sur appareil, ajouter et comptabiliser le transfert D2H de 4N, la copie et
la synchronisation nécessaires ; sur CPU `PoolExecutor::take` peut déplacer si sa
taille est exacte, sinon il copie. Ne pas emprunter `a.lexrank` du contexte CUDA :
c'est un scratch réécrit sur la trame suivante. Le propriétaire du rang doit vivre
jusqu'à la fin de sa consommation et son admission respecter la coexistence C/G/T.
Les tableaux de sortie T usuels restent nécessaires ; aucune mémoire actuelle
n'est réputée économisée avant un raccord explicite.

La voie générale de `build_forests` ne promet pas toutes les naissances K1 du Cloud.
Activer ce raccourci seulement si N=nombre de sites, clés exactement 0..N−1, rangs
tous nuls et table valide pour ce Cloud. La taille seule ne suffit pas. Sans
préparation liée au même propriétaire, conserver la voie actuelle. Au raccord,
domaine 1..N, unicité de rang et ordre XYZ strict de la permutation se vérifient en
O(N), en réutilisant les sorties préallouées ; cela détecte une table périmée si
elle impose un autre ordre. Deux nuages peuvent partager une permutation : ce test
ne remplace pas l’identité du propriétaire. Refus transactionnel avant publication si une
préparation prétendue valide est incohérente.

## Priorité et chantier existant

A/w902 recouvre déjà G/T/M/V/R, mais conserve ce `sort_sites` ; C prépare la fin
d'étage et garde `lexrank` interne, sans le publier dans `FinishOutput`. Le corps
livré le détruit avec la finition CPU, ou le garde comme scratch du contexte GPU.
La lecture des rapports/corps actifs épinglés ne trouve pas ce raccord en chantier.
Ce levier est distinct du registre R/classe unique et du recouvrement A. Il exige
néanmoins une coordination A/C avant modification de leurs interfaces.

**Recommandation au développeur :** relever d'abord `births_ns` de K1 et sa position
sur le chemin critique, avec les compteurs déjà disponibles. Si ce coût est matériel,
jouer un bras unique « rangs C réutilisés par K1 », validation et transferts inclus.
Comparer mur FULL, pic mémoire, mêmes cohortes complètes, empreinte FULL et CSR R ;
ne pas adopter sur la seule baisse de T. Sur le pipeline A, accélérer une tâche déjà
entièrement masquée peut ne rien gagner au mur. Aucune prévision de millisecondes.

```sh
python3 -B model.py --repo /workspaces/E-HGP
python3 -B -O model.py --repo /workspaces/E-HGP
```

Sources et corps prototypes observés, sans transfert de leurs qualifications :
[capture.json](capture.json). Aucun payload de scène lu ou copié.
