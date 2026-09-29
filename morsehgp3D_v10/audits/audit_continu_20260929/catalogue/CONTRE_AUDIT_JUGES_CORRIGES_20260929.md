# Contre-audit des juges v10 corrigés — 29 septembre 2026

Les corrections ferment les anciennes failles sur les supports non canoniques,
les niveaux exacts, les rangs denses et les images verticales des branches sans
point entré. Elles ne ferment pas encore le contrat d'export du catalogue ni
l'atomicité des multifusions. Une nouvelle mutation transforme une fusion
ternaire en deux fusions binaires au même niveau : le juge corrigé l'accepte.
Ces mutations de dumps démontrent des angles morts des juges, **pas un bug du
moteur ni un catalogue ou une tour incorrects sur les entrées exécutées**.

Cadre : `exploration_v10_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `correction_de_juges`, `not_claimed`. Aucun GCP,
aucun fichier de moteur modifié. Nous avons lu les copies corrigées
`build/v10-fixes/oracles/src` et `oracles-verif/src`, dont les deux juges ont
les mêmes empreintes. Le worktree courant à cette lecture était `12aa92110` ;
ses juges suivis étaient encore les versions anciennes. La capture porte
donc explicitement sur les copies corrigées, pas sur un déploiement implicite.

Preuves nouvelles :
[`receipts/audit_continu_20260929/oracles_corrected/`](../../../receipts/audit_continu_20260929/oracles_corrected/).
Le dossier contient les scripts, les dumps, les JSON normal/−O et les copies
des juges, de la référence exacte et des contrats. `SHA256SUMS` ferme les
fichiers. Le rejeu sur ces copies est autonome : aucun binaire n'est requis.

## 1. Preuve nouvelle : fusion N-aire binarisée au même niveau

La fixture `TRIANGLE` du juge corrigé contient six sites :
`(1,0,0), (0,1,0), (0,0,1), (2,2,0), (50,50,50), (51,50,50)`.
Une unique exécution courte à K3, répétée seulement pour le mode Python −O,
conserve son dump brut avec témoins. Le dump intact est accepté.

À l'ordre 2, le nœud 7 d'origine est une fusion des trois naissances
1, 2 et 3 au niveau exact `2/3`. La mutation ajoute une fusion intermédiaire :

| Dump intact | Dump mutant |
| --- | --- |
| un nœud de fusion 7, trois enfants 1/2/3 | nœud 7 à `2/3`, enfants 1/2 ; son parent 8, aussi à `2/3`, enfants 7/3 |
| 11 nœuds à l'ordre 2 | 12 nœuds à l'ordre 2 |
| une multifusion ternaire atomique | deux fusions binaires dans le même plateau |

Les IDs, parents, attaches et images depuis l'ordre 3 sont remis en
correspondance ; la nouvelle fusion reçoit la même image verticale correcte.
Le fichier mutant est reparsé par `parse()` avant `judge()`. Dans les deux
modes Python, le verdict vaut `None` pour l'intact **et** le mutant, alors
que `SPEC_V10.md §2` exige « multifusions N-aires jamais binarisées,
plateaux atomiques par niveau exact ».

Cause : `structure()` n'interdit qu'un parent de niveau inférieur
(`test_tower_oracle.py:241`) et vérifie seulement au moins deux enfants par
fusion (`:258`). `sweep()` consomme toutes les fusions du plateau avant de
juger les nœuds vivants (`:336-354`). Le nœud artificiel naît et disparaît
dans ce plateau ; il n'ajoute aucune composante aux coupes ouvertes ou
fermées. Ses témoins de descendants et sa verticale ont la bonne identité.
Les bijections aux coupes ne jugent donc pas à elles seules l'arbre N-aire
canonique exigé par le contrat.

Gate proposée : ajouter un mutant gravé `binarized_same_level` sur cette
fixture, exiger d'abord l'acceptation du dump intact puis le rejet du mutant,
et contrôler le type d'événement dans les liens **horizontaux** parent/enfant.
Une fusion ne doit pas être l'enfant d'une autre fusion au même niveau :
c'est précisément la binarisation d'un plateau. Une naissance a zéro
enfant et porte un témoin ; une fusion a au moins deux enfants et ne porte
pas de témoin. Conserver ces contrôles déjà présents, notamment celui des
fusions unaires. À terme, comparer chaque multifusion au regroupement
indépendant de tout le plateau permettrait de juger aussi son ensemble
complet d'enfants.

Ce contrôle ne doit **pas** interdire naïvement toute égalité de niveaux :
le contrat de hiérarchie de points autorise une naissance absorbée au même
plateau et une durée de vie nulle. Il ne concerne pas non plus les liens
verticaux entre ordres k+1 et k, dont l'image vivante peut être créée au même
niveau fermé. Le mutant contient deux événements de type fusion ; cette
distinction suffit à qualifier le défaut prouvé ici.

## 2. Catalogue : doublons internes et ordre d'export encore invisibles

Nous mutons une copie du petit dump déjà existant
`build/v10-fixes/oracles/scratch/c_lv.txt`, carré plus `(7,7,1)`, K3,
9 boules et 5 niveaux. Le dump intact est accepté. Les cinq mutants suivants
sont acceptés en normal et −O :

| Mutation | Conséquence non jugée |
| --- | --- |
| répéter un site de I | I n'est plus une liste de sites distincts |
| répéter un site de U déjà étendue | U n'est plus une coquille de sites distincts ; le bit0 reste vrai |
| renverser toutes les lignes | les niveaux de l'export ne sont plus croissants |
| échanger deux boules d'un même niveau | l'ordre canonique par S* dans le plateau est perdu |
| renverser U | la population n'est plus dans l'ordre de Morton |

Les contrôles positifs sont bien rejetés : décaler les rangs donne
« rang 1, rang dense attendu 0 » ; remplacer le support du carré par l'autre
diagonale donne « support non canonique ».

Deux causes distinctes :

- `key()` utilise `frozenset(I)` et `frozenset(U)` (`test_catalogue_oracle.py:111`).
  Les doublons et permutations sont effacés avant la comparaison ; p et u
  sont ensuite comparés aux poids de la référence, pas recalculés sur les
  listes publiées. Dans une U déjà étendue, le test `len(U)>q` ne révèle pas
  le doublon ajouté.
- La boucle de contrôle trie `got` par rang (`:130`). Elle répare l'ordre
  avant de vérifier sa croissance et ne vérifie aucun départage par S*
  entre records d'un même rang. Un rang dense correct ne prouve pas que
  l'export lui-même est ordonné.

Gate proposée : juger **avant toute normalisation** que I et U sont
strictement croissants par indice de site/Morton ; cela impose simultanément
unicité et ordre. Vérifier leurs poids sur les positions uniques annoncées,
leur disjonction et la cohérence du drapeau d'extension avec le vrai cardinal.
Parcourir les lignes dans l'ordre reçu et comparer `(niveau exact, S* Morton)`
à la clé précédente. Ajouter les cinq mutants ci-dessus aux mutations
systématiques, distinctement de celles qui prouvent déjà S* minimal et rang
dense. La tour vérifie elle-même les populations strictement croissantes
(`src/tower/tower.cpp`, garde `pop_sorted`) : ces contraintes ne sont pas de
simples préférences de présentation.

## 3. Ce que notre preuve confirme et ce qu'elle laisse ouvert

Sur le dump corrigé archivé à trois sites, l'image fausse de la naissance
d'ordre 2 à `a=1`, alors qu'aucun point n'est entré, est rejetée :
« verticale k=2 noeud 0 a=1 : image 2 fausse ». La nouvelle vérification
par témoins traite donc effectivement cette ancienne faille. La lecture du
balayage confirme qu'elle parcourt les naissances **et** les fusions de
l'ordre supérieur, en vérifiant existence, naissance antérieure ou égale,
vivacité fermée et identité de composante ; ce contrôle n'est pas réduit aux
branches ayant déjà des points.

Un constat adjacent reste reproductible : à l'ordre 2, attacher `(5,0,0)`
au nœud 1, né à `9/4` mais absorbé à `25/4`, avec entrée `D2=9`, reste
accepté. Le juge remonte l'ancêtre à l'entrée, et reconnaît ainsi la bonne
composante malgré une attache hors de l'intervalle de vie du nœud annoncé.
Une garde explicite de cet intervalle est distincte du contrôle des
verticales. Respecter dans son écriture le cas d'égalité prévu par le
contrat `src/points/dendrogram.hpp`, puis préciser si le dump de tour exige
en outre directement la composante vivante à la coupe fermée.

## 4. Résultats développeur réellement terminés, seulement observés

Nous n'avons pas rejoué de longue campagne. Les captures développeur que
nous avons lues ont des fins explicites :

- `oracles/apres/ctest_gate_final.txt` : 24/24, `code=0` ;
- `oracles-verif/apres/ctest_gate.txt` : 24/24, `rc=0` ;
- appels directs oracles-verif : tour normal et −O, catalogue normal et
  −O à 12 nuages, chacun terminé avec `code 0` ; le fichier
  `oracles_direct.done` était présent lors de la clôture de notre lecture.

Les deux tours rapportent 3 978 verticales, dont 2 905 sans point entré,
et 218/218 mutants tués ; les catalogues rapportent 571/571 mutants tués.
Ces réussites concernent les classes de mutation prévues par leurs scripts.
Nos mutations nouvelles révèlent les classes absentes. Les preuves ASan,
TSan et différentiels supplémentaires annoncées dans `RECU_oracles.md`
n'ont pas été rejouées par ce contre-audit et ne sont pas transformées en
qualification nouvelle. Aucun contrat FULL de temps, GPU ou G4 n'est acquis
par cette capture.
