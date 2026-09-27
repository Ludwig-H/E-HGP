# FULL : contre-audit de l'encodage par lots

27 septembre 2026. Lecture indépendante du produit `24308be81`, en particulier
`src/tower/forest/full_coverage_certificate.hpp`, lignes 283–429.
Cadre `exploration_v9_hors_registre`, `quantized_u18_input_only`,
`backend=cpu_reference`, `public_status=not_claimed`.
Pas de modification du moteur ni d'utilisation GCP dans cette revue.

## Critère exact remplaçant le tableau chronologique `live`

Une action sans parent crée une naissance ; avec un parent, elle conserve
l'identité du segment ; avec au moins deux parents, elle crée une fusion.
Le préfixe du nombre d'actions dont l'arité parentale diffère de un fixe
donc les identifiants de nœuds, dans l'ordre original des lots et actions.
Le nombre `prior_count[b]` exclut **toutes** les créations du lot b.

Pour chaque identifiant parent p, ordonner ses incidences par
`(batch, action locale, position dans la liste de parents)`. Les conditions
suivantes, avec les vérifications locales habituelles, sont équivalentes
à l'admission chronologique des parents :

1. Chaque p est strictement inférieur à `prior_count` du lot qui l'utilise.
2. Un même p n'a pas deux incidences dans le même lot, même si l'une est
   une continuation : le natif consomme tous les parents avant de restaurer
   les continuations à la fin du lot.
3. Avant chaque incidence, aucune incidence antérieure de p n'était une
   fusion. De manière équivalente, les incidences sont une suite de
   continuations suivie éventuellement d'une **unique fusion terminale**.

Preuve par induction sur les lots : un nœud est vivant immédiatement après
sa création. À chaque nouveau lot, une continuation unique le consomme
puis le restaure, une fusion le consomme définitivement, et une absence
d'incidence ne change pas son état. Aucun nouveau nœud du lot ne peut être
utilisé pendant sa première passe. Les trois conditions décrivent donc
exactement les transitions de `live`, sans DSU ni compression de chemins.

Attention au motif « fusion terminale » : si une fusion est suivie d'un
usage futur, **la fusion elle-même reste admise** ; l'erreur de parent mort
appartient au premier usage suivant. Un test global « cette fusion n'est
pas dernière » ne suffit pas à reproduire la première erreur native.
Le bon test local à l'incidence i est un OU segmenté **exclusif** des bits
« fusion » des incidences antérieures. Les doublons dans un lot s'attribuent
de même à la deuxième incidence, pas à la première.

La liste des parents de chaque action doit rester strictement croissante.
Cette règle locale ne se déduit pas du regroupement global par parent.
Sur un préfixe contenant déjà une action invalide, les résultats ultérieurs
peuvent devenir artificiels ; ils sont acceptables comme candidats d'erreur
uniquement si une réduction chronologique conserve toujours l'erreur
antérieure. Ils ne doivent jamais déclencher un accès hors domaine.

## Première erreur : reproduire l'ordre des instructions, pas des familles

Le contrat sémantique natif est, dans cet ordre :

1. Toute la forme CSR, puis le domaine (ordre, banque, cardinal, lots).
2. Pour le lot courant : dénominateur positif, niveau strictement croissant,
   lot non vide, positivité K>1, en-tête des racines K1.
3. Pour chaque action : parents, un par un ; continuation vide ; références,
   une par une ; naissance ; racine K1 de cette naissance.
4. Après validation du lot entier seulement : production de ses nœuds,
   successeurs et contributions dans leur ordre d'origine.

Un ordre de réduction sûr des erreurs sémantiques est
`(batch, header_ou_action, action, étape, position, sous_instruction)`.
Pour une référence, l'étape « références » est commune, la position de la
référence précède le choix « population puis masque ». Sinon le masque
invalide de ref0 peut être masqué par la population invalide de ref1,
contrairement au natif. Une erreur d'action au lot b prime tout en-tête
invalide au lot b+1 : vérifier tous les en-têtes d'abord, puis s'arrêter au
premier en-tête invalide, serait faux.

La validation CSR globale a une priorité particulière : un dernier offset
corrompu est rejeté avant le domaine, même si `order=0`. En revanche la
« forme » locale d'une naissance ou d'une continuation n'est pas un
préflight global ; sa priorité reste celle de l'action.

Les opérations d'allocation ne sont pas équivalentes. Le natif compte et
réserve ses arènes avant les tests sémantiques. Un encodeur avec un autre
ensemble d'arènes et de tris ne peut promettre le même point d'échec de
la n-ième allocation. Limiter l'égalité de la première erreur aux entrées
pour lesquelles les allocations réussissent, en conservant une sortie
transactionnelle et les statuts explicites de manque de ressources.

## Fixtures prioritaires transmises au développeur

Base K2 : domaine `{0,1,2,3}`, populations de coquilles `{0,1}` et `{2,3}`,
intérieurs vides ; premier lot au niveau 1, deux naissances valides créant
0 et 1. Une contribution de continuation valide est `(population=0,
shell_mask=1, include_interior=false)`.

| Modification de cette base | Raison attendue |
| --- | --- |
| Lot2 : fusion `{0,1}` ; lot3 : continuation de 0 | `coverage_parent_not_unique_prebatch_root` au lot3 |
| Même motif, fusion du lot2 portant une population invalide | `coverage_population_reference` au lot2, pas une mort rétroactive |
| Même lot : continuation 0 valide puis fusion `{0,1}` | Parent non unique à la seconde action |
| Même motif, première continuation sans contribution | `coverage_empty_continuation` à la première action |
| Même lot : naissance créant 2, puis continuation de 2 | Parent non unique : 2 n'existait pas avant le lot |
| Lot2 parent 999 ; lot3 dénominateur nul | Erreur de parent au lot2, pas d'en-tête au lot3 |
| Lot2 reprend le niveau 1 et porte parent 999 | `coverage_nonincreasing_batch` avant l'action |
| Action avec ref0 masque nul et ref1 population 999 | `coverage_empty_or_invalid_mask` de ref0 |
| Naissance avec deux refs valides ; variante ref1 invalide | `coverage_birth_population` ; puis `coverage_population_reference` dans la variante |
| Dernier offset CSR corrompu et ordre nul | `coverage_flat_draft_shape` avant le domaine |

Ajouter les cas positifs naissance → plusieurs continuations → fusion,
continuation après fusion **sur le nouveau segment**, deux composantes
indépendantes, fusion sans contribution, K1 avec toutes ses racines et
masque complet de 16 bits. Conserver les mots exacts du niveau d'entrée :
deux fractions égales servent au refus d'un plateau séparé, mais les
fractions acceptées ne sont pas à normaliser dans la sortie.

## Limites de la brique

Ce lemme ne certifie ni les événements géométriques ni les liens verticaux.
Il suppose une banque réellement immuable pendant tout l'appel ; le défaut
d'alias du constructeur public par déplacement reste un sujet distinct.
Un tri des incidences est O(P log P), non une preuve de gain face à
l'encodeur linéaire actuel. Préfixes, sommes, tri et dispersion sont
parallélisables, mais une simulation scalaire des calendriers ne qualifie
ni les courses CPU ni le GPU. La sortie explicite et ses octets restent
à payer ; aucune économie n'est acquise avant un raccord chronométré.

## Revue finale du gate et de la capture r1

Le [gate complet](../b_full_batch_encoder_20260927/gate.cpp),
l'[encodeur](../b_full_batch_encoder_20260927/encode.hpp) et son
[lecteur](../b_full_batch_encoder_20260927/run.py) ont été relus après
gel. Aucun défaut sémantique nouveau n'a été identifié dans cette revue.
La [capture r1](../../receipts/full_batch_encoder_20260927/README.md)
a été relue normalement puis sous `python3 -O` : les deux lectures passent.
Elles n'ont recompilé ni relancé les exécutables, et n'ont modifié aucun
reçu ou build épinglé.

Les contre-fixtures prioritaires sont bien présentes, avec une raison
native attendue explicite avant la comparaison différentielle :

| Point relu | Emplacement dans `gate.cpp` |
| --- | --- |
| Fusion puis réemploi ; erreur de population sur la fusion prime le réemploi ultérieur | `fixtures()`, lignes 83–85 |
| Deux usages du même parent dans le lot ; continuation vide antérieure prioritaire | lignes 86–88 |
| Nouveau nœud du lot interdit comme parent de ce même lot | lignes 89–90 |
| Erreur d'action antérieure à un en-tête futur invalide ; en-tête du même lot prioritaire | lignes 91–93 |
| Masque invalide de ref0 avant population invalide de ref1 | lignes 94–95 |
| Références examinées avant cardinalité d'une naissance | lignes 96–98 |
| Mauvaise forme CSR globale avant ordre nul et banque absente | ligne 99, puis neuf corruptions lignes 124–137 |
| Deux continuations puis fusion silencieuse ; multifusion à 32 parents | lignes 113–114 et 120–122 |
| Trois mots du niveau préservés ; égalité rationnelle refusée entre deux lots | lignes 117–118 |

La non-vacuité n'est pas déduite du seul succès du programme : le gate
exige au moins 400 entrées acceptées, 6 000 refusées, 10 000 nœuds et
680 cas pour chacun des ordres 1 à 10. La capture contient effectivement
6 838 entrées, dont 404 acceptées et 6 434 refusées, chacune comparée dans
deux calendriers ; 13 780 nœuds, 13 131 parents et 16 500 contributions
sont examinés sur les entrées acceptées, comptés une fois par entrée.

La comparaison porte sur statut, motif, ordre, identité de la banque,
tous les nœuds/parents/successeurs/contributions dans leur ordre, avant
le calcul du digest. `ExactLevel::operator==` compare bien les trois
mots et le dénominateur, et non seulement la valeur rationnelle : le
mutant de normalisation est donc une vraie différence d'objet. Les refus
sont comparés à une sortie native transactionnelle vide.

Les trois mutants `ignore-dead`, `first-visited` et `normalize-level`
sont tués par divergence sémantique dans chaque build, code 1 et stderr
vide ; ce ne sont ni des crashes ni des compilations mutantes séparées.
Le lecteur recalcule sa recette, contrôle les sources avant/après,
chaque dépendance non système déclarée par `-MM`, les hashes des binaires
et sorties, et l'égalité Release/sanitizer. Les 14 commandes et le digest
`2613309417252955929` sont cohérents avec les comptes annoncés.

Limites résiduelles de preuve : les histoires pseudo-aléatoires sont
déterministes et modestes, pas une énumération exhaustive des drafts ; les
cas particuliers « continuation du segment créé par fusion » et « coquille
de 16 sites » sont dans ce générateur, mais n'ont pas de compteur de
couverture individuel dans le reçu. Le juge réutilise les types et le
comparateur numérique natifs ; ce n'est pas un second oracle arithmétique.
La vérification LIVE des hashes ne réexécute pas le binaire et n'est pas
une archive autonome hors de ces sources/builds. Aucun test de course,
d'injection de panne mémoire ou de performance n'est ajouté par cette
contrelecture. La conclusion reste : **prototype structurel compatible
sur le domaine testé, candidat au raccord parallèle, pas FULL GPU qualifié**.

## Piste supplémentaire : voie linéaire sans continuations

Contre-analyse demandée après la capture de vrais drafts FULL. Cette
section est un argument de conception, pas une qualification d'un nouvel
encodeur ni une mesure de gain. Les captures et sources r1 ci-dessus ne
sont pas modifiées.

La condition exacte est une réduction globale : **aucune action n'a
exactement un parent**. Une action a donc zéro parent (naissance) ou au
moins deux parents (fusion). Si une continuation existe, utiliser la voie
générale ; sa présence n'est pas une erreur de draft et ne justifie pas
un refus public.

Dans ce domaine, chaque action crée exactement un nœud : `V=A`, son ID
est son ordinal d'action global, et le nombre de nœuds pré-lot est
`batch_begin[b]`. Les offsets de parents de la sortie sont directement
ceux du draft : aucun parent de continuation n'est à retirer.

Pour chaque parent `p<V`, construire
`first[p] = min { j : draft.parent[j] == p }`, initialisé à un absent
distinct de tous les ordinaux valides. Au slot global `j`, le contrôle
des parents devient : ordre strict intra-action, `p<batch_begin[b]`, puis
`j==first[p]`. L'évaluation de cette condition s'inscrit au même stade et
au même slot dans la réduction d'erreur canonique déjà décrite. Les
parents `p>=V` ne doivent jamais indexer la table ; leur erreur locale
est tout de même enregistrée.

Preuve : un premier usage accepté consomme définitivement son parent,
car c'est nécessairement une fusion. Ce parent n'est donc vivant à un
usage ultérieur ni dans le même lot ni dans un lot futur. Réciproquement,
un parent pré-lot sans usage antérieur est vivant. Les deux contrôles
produisent ainsi les mêmes décisions sur tout préfixe natif valide.
Au premier refus natif, tous les événements antérieurs sont valides ;
le contrôle local ou `first` reproduit ce refus. Les erreurs hypothétiques
détectées après ce point ne changent pas le minimum chronologique.

On peut inclure dans `first` une référence `p<V` pourtant interdite
par le contrôle pré-lot : son refus local est antérieur à tout refus
supplémentaire qu'elle induit. À l'inverse, **marquer le premier worker
arrivé** au lieu du minimum d'ordinal n'est pas correct : cela peut
déplacer l'erreur parent vers une action antérieure et lui faire dépasser
une erreur de population native. `atomicMin` est une implémentation
possible du minimum, pas une autorisation de changer l'ordre des motifs.

Après admission seulement, les successeurs peuvent être écrits sans
conflit, car chaque parent est utilisé au plus une fois. Les nœuds,
parents et contributions ont des plages de sortie disjointes. Conserver
exactement les niveaux et les références ; ne pas fusionner de naissances
et ne pas réduire les niveaux rationnels.

Travail abstrait : `O(B+A+P+C+V)` avec B lots, A actions, P références
parents, C contributions et V nœuds, en comptant mode-check, validation,
initialisation de la table et sortie. La table ajoute `V*sizeof(ordinal)`
octets ; le tri et le tableau d'incidences disparaissent de cette voie.
Cela ne mesure pas le coût des atomiques GPU : un draft invalide qui
répète le même parent peut les sérialiser. Un draft accepté a au plus
un usage par entrée. Les limites d'ordinal, les tailles d'allocation et
la priorité des erreurs de forme/domaine restent à contrôler. Comme
pour r1, l'équivalence sémantique ne garantit pas la même chronologie
en présence d'épuisement mémoire.

Fixtures prioritaires du futur gate :

- naissances puis fusions indépendantes et fusion de leurs nouveaux
  nœuds dans un lot ultérieur ; sorties champ à champ dans deux calendriers ;
- fusion et réemploi dans le même lot puis dans un lot ultérieur ;
  permutation des calendriers avec les mêmes motifs ;
- population invalide sur la première fusion avant un réemploi futur :
  la population doit gagner, ce qui tue un « premier worker » arbitraire ;
- parent `p<V` créé dans le même lot, et parent créé dans un lot futur,
  puis réemploi après sa création : le premier refus pré-lot doit gagner ;
- parent `p>=V`, y compris l'absent maximal, sans accès à la table ;
- parent localement non croissant et doublon intra-action ;
- en-tête invalide du même lot contre erreur parent, puis erreur d'action
  avant en-tête invalide d'un lot futur ;
- une seule continuation insérée : sélection obligatoire de la voie
  générale, avec continuation valide, vide et continuation après fusion ;
- forme CSR invalide contre domaine invalide, et erreur de masque ref0
  contre population invalide ref1, comme dans la qualification générale.

Conclusion de la contre-analyse : **voie exacte et pertinente à prototyper**
pour les drafts sans continuations, sans conclusion de performance avant
capture appariée. Elle simplifie aussi les préfixes de sortie ; elle ne
remplace pas l'encodeur général lorsque les continuations sont présentes.
