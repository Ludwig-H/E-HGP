# Objets de la tour pour un parallélisme massif

11 septembre 2026. Cible : remplacer les dépendances de programmation du
constructeur `83f1c78e…`, pas changer l'objet HGP FULL. Cadre :
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Les prototypes de cette note ne remplacent pas encore le moteur actif.
GCP non utilisé pour ce jalon.

## 1. Décision d'architecture

**L'objet central n'est ni Gamma exhaustif, ni un arbre sur les points :
c'est une famille de graphes datés sur les naissances, accompagnée de marques.**
La géométrie est partagée par toute la tour. Les horizontales de chaque K
peuvent être construites indépendamment. Les verticales sont ensuite des
requêtes sur les arbres horizontaux achevés, sans nouvelle MEB verticale.
L'auditeur a [confirmé cette décomposition et ses prémisses](../audits/receipts_parallel_objects_20260911/README.md).

```text
points → index → fronts WSPD → candidats → census géométrique partagé
                                                    ↓
                              atlas clairsemé de blocs (K, boule)
                                                    ↓
                            représentants → terminales géométriques
                                                    ↓
                     pivots → graphes datés sur les naissances, par K
                                                    ↓
                   forêts couvrantes minimales → arbres de multifusions
                                                    ↓
               requêtes historiques : contributions, ancres, verticales
                                                    ↓
                             numérotation canonique et export
```

Chaque ligne doit distribuer des milliers de blocs, requêtes ou arêtes.
Exécuter dix copies du Builder en parallèle n'est pas cette architecture :
cela multiplierait ses grosses allocations et conserverait ses calendriers
séquentiels à l'intérieur de chaque K.

## 2. Catalogue partagé et atlas compact

Pour C boules, garder une seule géométrie, clé, population et valeur exacte
du niveau par BallId. Pour chaque boule B, l'intervalle admis est :

$$\ell_B=\max(1,p_B+q_{\min,B}-1),\qquad h_B=\min(K_{\max},n,p_B+u_B).$$

Un préfixe des largeurs donne `base[B]`, puis le bloc se retrouve par
`cell(B,K)=base[B]+K−lo[B]`. Il n'y a pas de matrice dense Kmax×C.
Dans le régime régulier, la largeur est au plus deux ; les coquilles
supplémentaires gardent leur vraie largeur et leur quotient local.
Ajouter séparément les n naissances-points de K1.

Trier les niveaux exactement une fois et attribuer un rang entier commun
permet les tris et comparaisons ultérieurs sur entiers. **Ce rang ne fusionne
aucun niveau distinct et ne départage aucun plateau géométrique.** Garder
également BallId et sa représentation rationnelle d'origine : deux fractions
équivalentes peuvent avoir des octets différents, observables dans l'export
historique. Le tri exact commun existe déjà dans Builder ; le delta est son
exposition comme atlas et son utilisation par toutes les phases.

Le [raccord de trois gardes par rangs](GARDES_RANGS_CERTIFIES_20260911.md)
est maintenant qualifié O2/SAN sur la voie CPU fenêtrée : semis initial,
ordre des consommateurs et terminale strictement antérieure. Il réutilise
les rangs déjà certifiés, sans retirer leur validation exacte ni changer
les comparaisons des MEB intermédiaires. K1 conserve le domaine des points.
Ce changement ne partage pas encore la préparation répétée du catalogue.

Dans cette fenêtre, chaque représentant strict comprend **tout I(B)** et
un masque de U(B). Conserver une fois les masques, offsets et contribution
du quotient évite les deux appels actuels à `visit_block`. Déclarer l'ordre
de chaque masque : le prototype développe ses représentants dans l'ordre
BallData d'origine, tandis que les contributions publiques utilisent la
coquille triée par PointId. Leur conversion doit être explicite ; jamais
une liste census réordonnée implicitement. Une naissance est un bloc sans représentant
strict ; avec extra-shell, elle n'est pas forcément le semis complet I∪U.

Les clés complètes sont développées seulement pour le tri et l'exécution
d'une fenêtre de requêtes. Dédoublonner par `(K,facette entière triée)` : une
égalité de support MEB ne suffit pas, car la recherche d'intrus exclut toute
la facette. Les semis complets et les états après échange restent exacts.
Le nombre de requêtes simultanées est un budget de résidence, pas un plafond
d'itérations ni une raison de déclarer un terminal prématurément.

## 3. Remplacer le calendrier, puis retirer la boucle des ordres

La [réduction aux naissances](GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md#6-retour-de-lauditeur--éliminer-les-hubs-non-natifs)
donne un pivot strictement antérieur par bloc non natif. Le saut de pointeurs
en doubles buffers produit sa naissance représentative φ. Les arêtes émises
entre ces naissances portent **la date du bloc émetteur**, non celle d'une
extrémité. Leurs deux naissances sont strictement antérieures à cette date.
Les doublons gardent le minimum exact ; les boucles disparaissent.

Pour la voie **mono déjà ordonnée**, le [correctif fenêtré](RESOLUTIONS_PAR_FENETRES_20260911.md#correctif-ordonné-qualifié-séparément)
qualifie maintenant un DSU persistant sur les hubs, sans retris de leurs
certificats. Ses pivots sont tous retenus et la projection est forestière.
La spécialisation proposée ensuite avec l'auditeur peut contracter chaque
pivot dès sa consommation : φ porte un indice dense de naissance stable,
pas une racine DSU. Un DSU sur les seules naissances suffit alors aux autres
occurrences. Cette [spécialisation est maintenant qualifiée O2/SAN](CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md),
sur une nouvelle source distincte du brouillon historique non compilé.
Cela ne supprime pas l'obligation de remettre les résultats géométriques
dans l'ordre source avant leur consommation. Les fenêtres indépendantes
arrivant hors ordre gardent le contrat distinct de composition ; ce raccourci
mono ne constitue donc pas, à lui seul, une parallélisation massive.

Une forêt couvrante **minimale pour ces dates** conserve toutes les composantes
aux coupes ouvertes et fermées. Une forêt arbitraire ou le MST euclidien des
points ne suffit pas pour K≥2. Conserver les dates et les identités des
naissances, même lorsque leurs populations de points se recouvrent.

Pour rendre parallèle la transformation de cette forêt en histoire de
fusions, étudier la contraction d'arbres. RCTT et PANDORA fournissent des
algorithmes publiés sur arbres pondérés ; leurs performances ne sont pas
des prévisions HGP. Le ParUF du premier travail garde une profondeur liée
à la hauteur de l'histoire : ce n'est pas le choix robuste par défaut pour
un peigne. [ParSLD/RCTT](https://arxiv.org/abs/2404.19019),
[PANDORA](https://arxiv.org/abs/2401.06089).

Trois structures restent distinctes : forêt minimale comme certificat des
coupes, arbre de contraction comme plan de calcul, arbre FULL comme sortie.
Un raffinement binaire interne peut départager les arêtes égales ; il faut
ensuite contracter ses liens entre fusions de **même rang géométrique**.
Les composantes disjointes de même date restent deux multifusions distinctes.

La correspondance φ reste nécessaire après cette réduction. Une marque de
bloc conserve son admission λ_B, même si φ(B) est beaucoup plus ancienne.
Une contribution conserve sa date, sa population et son masque. ABCZ apporte
Z à 25 sur une composante née à 16 : ni le graphe nu ni une contribution
antidatée à 16 ne reconstruisent cette croissance correctement.

Pour les requêtes historiques, les chaînes lourdes proposées par l'auditeur
gardent un stockage O(N) pour N nœuds et une requête O(log N), sans table
permanente de sauts N log N. Leur construction doit aussi être parallélisée ;
une consultation indépendante ne rend pas son index gratuit.

Pour chaque naissance supérieure, conserver sa référence inférieure de
même boule. Pour chaque nœud supérieur, choisir une naissance descendante
et consulter sa référence inférieure à la date du nœud, **coupe fermée**.
Toutes ces requêtes ne lisent que les histoires horizontales. Vérifier
ensuite que l'image de chaque parent, normalisée à cette coupe, coïncide.
Il n'y a ni dépendance aux verticales déjà calculées de K−1, ni nouvelle MEB.
Une unique naissance par arbre final ne remplace pas une descendante de
chaque nœud ; la racine finale ne remplace pas une image historique.

## 4. Les étapes amont restent dans le contrat

| Phase | Unité plate à distribuer | Travail ou invariant à préserver |
| --- | --- | --- |
| Index | clés Morton/PointId, plages et boîtes | Tri déterministe, index propriétaire commun ; ce poste est déjà petit sur le triplet uniforme. |
| Vague WSPD | une décision par rectangle, puis comptage/préfixe/écriture | Réutiliser `witness_front.hpp`, déjà qualifié CPU en option ; éviter les shards de sorties, conserver ordre et grand-livre par lane. |
| Témoins et ancres | histogrammes par rectangle/lane et tuiles implicites de paires | Cœur hors A∪B, h_a dans A privé de a, h_b dans B privé de b ; ne pas additionner des populations chevauchantes après subdivision. |
| Covers et seeds | segments de handles partagés, tâches référençant leur cover | Ne pas recopier le cover par seed, ni matérialiser tout A×B pour distribuer le calcul. |
| Profondeurs q3 | segments de seeds et de leurs covers | Distribuer les parcours seed×cover et compter leurs visites ; l'atlas ne les supprime pas. |
| Sweep q4 | groupes de racines rationnelles exactement égales, segmentés par seed | Profondeur = c0 + préfixe exclusif des entrées + suffixe exclusif des sorties ; conserver les témoins constants c0 et exclure les incidents du groupe. |
| Candidats | runs triés, indices de permutation, groupes de clés entières | Canonisation et RLE traversent les frontières de runs ; un hash seul n'est pas une identité. |
| Census | une boule par tâche, destination à ordinal fixe | CPU écrit déjà directement les BallData ; C3→compaction→C4 GPU reste un raccord distinct à qualifier. |
| Validation géométrique | une boule et son certificat de support, q_min et quotient extra-shell | Réduire erreurs/comptes après travaux indépendants ; un atlas de rangs ne remplace ni cette validation ni la complétude du census. |
| Export | populations partagées, occurrences, comptes et offsets | Réduire la première occurrence canonique pour numéroter ; pas d'ensemble complet de points par nœud. |

La génération nominale n'appelle pas encore le front batch optionnel. Un
plan de vague peut stocker `FrontDecision[W]`, compacter les seules demandes
de coins encore nécessaires, puis deux préfixes allouent sorties terminales
et enfants à offsets disjoints. Il conserve O(Wmax) de plan ; la consommation
en flux des terminaux est une étape distincte.

Le calcul des histogrammes peut encore être quadratique sur deux amas :
WSPD seule ne borne pas ce coût. Sur l'uniforme, ses petits facteurs ne sont
au contraire pas le poste dominant. Les témoins universels de rectangle et
les covers existentiels sont deux certificats différents. La compaction
C3→C4 GPU ne promet pas un gros gain dans les campagnes où plus de 99 % des
candidats survivent. Le [dossier WSPD](ELIMINATION_BLOCS_WSPD.md) conserve les
contre-exemples et essais négatifs.

## 5. Mémoire, qualification et prochaine intégration

Compter C boules, A blocs admis, R occurrences strictes, Q clés initiales
uniques, D états géométriques découverts, L naissances, E arêtes et N nœuds
FULL. Le travail géométrique adaptatif dépend aussi des visites/supports,
pas seulement du nombre de requêtes. Une forêt FULL sans nœud unaire a au
plus 2L−racines nœuds ; cela ne borne pas L linéairement en points.

À 32k, les 45 208 799 requêtes historiques **K2..10** demanderaient
2 531 692 744 octets (2,532 Go) à 56 octets par requête si elles étaient toutes
présentes à la fois, **en plus** du catalogue et de la sortie. C'est une
résidence hypothétique, pas le pic actuel : le moteur travaille par ordre,
avec 939 524 096 octets de capacité requêtes au pic dans ce
[reçu](../receipts/post_exchange_scale_20260911/README.md).
Les représentants tous ordres sont, eux, 45 453 599.
Un masque compact et des fenêtres peuvent éviter cette allocation multi-K,
mais leur pic réel reste à mesurer. Le format
massif doit aussi traiter les offsets globaux en 64 bits et distinguer les
bornes de représentation des quotas arbitraires de calcul.

Ordre de raccord : atlas/masques et résolutions hors calendrier, graphe et
histoire séquentielle de référence, contributions/verticales offline contre
Builder et T2, puis primitives parallèles remplaçant cette référence. Un
prototype abstrait de graphe ne certifie pas l'extraction du graphe depuis
les vrais census. Les premières briques locales sont décrites dans les
reçus associés au jalon ; aucun backend MSF/RCTT GPU n'est encore livré.

Les mesures existantes restent des points de départ, pas celles de cette
architecture : mono 8k/16k/32k en 141,366/318,968/694,459 s, dont génération
60,192/134,292/289,177 s. [Triplet fermé](../receipts/post_exchange_scale_20260911/README.md).
La comparaison s8/10/12 conserve les mêmes boules et hiérarchies à 8k ; elle
ne choisit pas encore le meilleur temps. [Comparaison fermée](../receipts/static_s_factors_20260911/README.md).

Rejouer ces trois tailles et s=8/10/12 après raccord réel, en séparant coûts
intermédiaires et taille de sortie, puis mesurer toute la tour 50k K1..10,
ou K1..5 en repli. Ni 1 seconde, ni 100 ms, ni plusieurs dizaines de millions
de points sur G4 ne sont acquis. La borne de sortie interdit une promesse
universelle sous-quadratique pour FULL explicite ; l'objectif est de ne pas
rajouter de travail quadratique inutile aux objets effectivement requis.

## 6. Premières briques exécutées, portée bornée

Les trois helpers privés passent C++20 strict en O2 et sous ASan/UBSan/LSan.
Le moteur actif `83f1c78e…` n'est pas remplacé.

Le [raccord de résolution en flux](RESOLUTIONS_PAR_FENETRES_20260911.md)
supprime maintenant la conservation globale des terminales et des arêtes :
une fenêtre géométrique alimente directement les certificats sur hubs,
puis φ et leur projection. Il conserve les masques u16[R] et compte les
résolutions répétées entre fenêtres. O2/SAN compare les terminales et la
tour entière, pas seulement les certificats abstraits. Sa première version
reste mono-thread. Le nouveau raccord dense distribue maintenant les
groupes géométriques entre workers persistants, puis disperse et consomme
dans l'ordre source ; préparations, union-find et reconstruction ne sont
pas encore massivement parallèles. Aucun gain massif n'est acquis.

| Brique | Qualification locale | Ce qui n'est pas encore livré |
| --- | --- | --- |
| [Atlas partagé](../receipts/rank_atlas_20260911/README.md) | 13 vrais census, n≤32, s8/10/12, K jusqu'à 10 ; 30 562 blocs et 52 469 représentants identiques au parcours du Builder ; six mutations d'état réfutées. | Résolution des représentants, extraction du graphe et nouvelle forêt FULL. |
| [Graphe daté → histoire](../receipts/filtered_calendar_20260911/README.md) | 264 graphes abstraits, 6 570 coupes et 474 524 contrôles ; parents des multifusions contre BFS, 17 refus et deux mutations du calendrier. | MSF et construction d'arbre parallèles ; géométrie/couvertures/verticales depuis census. |
| [Consultations historiques](../receipts/filtered_calendar_20260911/README.md) | 307 500 requêtes CPU1/2/4, 922 537 contrôles, peigne de 8 191 nœuds ; huit refus et une mutation sémantique d'admission. | Construction parallèle de l'index, jointure des vraies contributions/verticales et exécution GPU. |

L'atlas confronte son résultat au vrai `validate_catalogue`/`visit_block`
d'une copie du Builder instrumentée par une unique déclaration friend,
sans modification de corps. Ses 22 265 350 contrôles incluent un juge
pair-à-pair des rangs, quadratique sur ces petits corpus seulement. Ce
juge n'est pas une étape du helper ni une stratégie pour 8k/16k/32k.
Il exerce les extras, K9/K10, ABCZ, n=1 et K=n ; 66 paires de niveaux égaux
ont des fractions brutes différentes. Masques et contributions faux mais
bien formés sont refusés sémantiquement par le différentiel.

À ce premier jalon, graphes abstraits et vrais census étaient deux portes
séparées. Leur succès ne constituait donc **pas** celui du chemin complet.
La numérotation déterministe du prototype de graphe n'est pas présentée
comme l'encodage physique du Builder. Le premier échec de compilation du
harnais de consultations est conservé, avec sa correction signée/non signée.
Les reçus n'incluent aucun exécutable ni résultat GCP. Aucune nouvelle mesure
8k/16k/32k ou 50k n'est attribuée à ces prototypes.

## 7. Raccord complet et certificats composables

Le raccord privé suivant joint désormais ces objets : vrai census WSPD,
atlas partagé, résolutions géométriques, pivots, graphe daté, histoire FULL,
contributions et verticales. Il n'utilise ni le catalogue de l'oracle comme
entrée ni les ensembles de points comme identités de composantes. La
qualification et ses sources sont dans le [paquet du raccord](../receipts/atlas_graph_full_20260911/README.md).
Le constructeur actif reste inchangé ; cette référence rend testable son
remplacement par des primitives parallèles.

O2 et ASan/UBSan/LSan donnent les mêmes résultats : 114 census positifs,
546 graphes d'ordres, 29 784 coupes et 15 594 832 vérifications verticales
par l'oracle indépendant sur les petites géométries. Le cas n32 est un
différentiel explicite avec Builder, sans oracle exhaustif à cette taille.
Chaque capture compare 237 840 nœuds et 150 240 contributions entre chemins,
et réfute sept corruptions ciblées avec leur cause attendue. Les largeurs de
fenêtres sont 1, 7 et 31, sans rapport avec K ni avec s WSPD=8/10/12.

Le [nouvel audit de composition](../audits/receipts_composable_msf_20260911/README.md)
permet de remplacer chaque lot d'arêtes par sa forêt minimale, puis de réduire
ces certificats entre eux. Une arête éliminée possède un chemin de remplacement
dont toutes les dates sont au plus la sienne. Toutes les coupes sont conservées,
même si un lot léger arrive tard. Les plateaux sont reconstruits **après**
composition : leurs fusions locales ne se concatènent pas.

Deux routes sont exercées sur les mêmes vrais census :

- Réduire des fenêtres du graphe déjà projeté sur les naissances. Avec une
  même clé totale d'arêtes, le certificat final est identique au MSF direct.
- Réduire d'abord les arêtes entre blocs d'origine, puis projeter les arêtes
  retenues via les pivots. Conserver leur date originale et retirer les
  boucles. Le certificat interne peut changer ; les histoires FULL, leurs
  contributions et leurs verticales doivent rester identiques.

Le second chemin permet conceptuellement de consommer les résolutions par
fenêtres, sans attendre tous les pivots pour commencer à comprimer les
arêtes. Dans le témoin actuel, **seules les fenêtres d'arêtes sont bornées** :
l'extraction possède encore toutes les terminales R et le graphe de contrôle.
Il ne faut donc pas annoncer la disparition effective de ces allocations.
La prochaine modification utile est de raccorder directement le producteur
de terminales à ce consommateur, puis de libérer les clés de chaque fenêtre.

Le helper MSF emprunte les naissances et ne renvoie que des arêtes. Son DSU
ne porte que sur les extrémités réellement présentes dans le lot, pas sur
tout le catalogue. La pile binaire garde un certificat par niveau de réduction.
Avec N sommets, M arêtes et J fenêtres, ses certificats prennent
O(min(M,N(1+log(max(1,J))))) arêtes, plus la fenêtre et les temporaires :
**pas une borne O(N) sur toute la RAM**. Catalogue, atlas, pivots, populations
et sortie restent à compter. Les lots forestiers ne se compriment pas.

La comparaison FULL utilise une bijection explicite par descendants de
naissances (K,B), puis vérifie dates rationnelles, parents, successeurs,
contributions datées avec masques et images inférieures. La banque partagée
et le représentant brut choisi pour un rang peuvent différer du Builder ;
on ne prétend pas conserver ses octets historiques. Dans la nouvelle
convention, CPU1/4 et les deux routes de certificats doivent en revanche
donner les mêmes octets. Des changements d'identités cohérents structurellement
ne deviennent pas pour autant géométriquement corrects.

L'adaptateur suppose ses histoires issues de certificats vérifiés contre
le graphe initial. Son contrôle structurel ne suffit pas à lier une histoire
arbitraire à ce graphe ; les marques externes et leurs dates restent séparées
des arêtes éliminées. Les contributions sont consultées à leur admission,
les verticales à la date du nœud supérieur, jamais à la racine finale.

Restent séquentiels dans cette référence : les résolutions géométriques,
le calcul des pivots, les MSF, la reconstruction et la construction des
index historiques. Seules les consultations historiques sont distribuées
sur CPU1/4. Aucun débit GPU, gain de latence ou contrat 50k ne découle de ce
raccord ; les prochains benchmarks 8k/16k/32k doivent porter sur un moteur
réellement raccordé, avec travail et résidence de toutes les phases.

## 8. Préparations partagées : doublons identifiés, retrait non implémenté

Les sondes ordonnée et dense conservent entièrement `Builder::validate_catalogue`, puis
en détruit les temporaires avant de préparer l'atlas. Ce parcours trie
trois fois les clés de boules : validateur, atlas et index de la géométrie.
Il trie deux fois les niveaux exacts et recrée les programmes par K.
Le prochain partage envisageable est un catalogue immuable préparé et
validé, avec ses permutations réutilisées par les consommateurs. Ce n'est
ni une option pour ignorer la validation d'une entrée extérieure, ni une
preuve de complétude du producteur. L'auditeur a accepté ce partage sous
liaison au même propriétaire immuable ; son raccord reste à implémenter.

Après vérification des niveaux distincts et de chaque correspondance
boule→rang, trois gardes de fenêtre comparent maintenant les rangs entiers,
avec leur [qualification propre](GARDES_RANGS_CERTIFIES_20260911.md).
Le comparateur d'ordre du programme dans l'Atlas reste exact dans ce delta.
Ni préparation partagée ni gain possible non mesuré ne sont soustraits des
chronométrages publiés.

Pour le raccord géométrique parallèle, l'unité indépendante reste le groupe
de facettes entières identiques dans une fenêtre, après les hits du semis
initial. Le résultat terminal est redistribué aux occurrences, puis consommé
dans leur ordre original. Le calcul parallèle ne doit lire ni modifier φ
ou le DSU ; ses scratchs et compteurs sont propres aux workers. Les semis
complets, l'index et le catalogue sont partagés en lecture seule.

Un doublon précis doit être évité lors du réemploi de l'[adaptateur GPU
par lots](../receipts/gpu_terminal_batch_t2_20260911/sources/current/prototype/batch_adapter.hpp) :
`resolve_batch` compare actuellement tous les semis à chaque appel.
L'appeler pour chacune des J fenêtres d'un K répéterait J fois le parcours
des S semis. La liaison complète au résident doit donc être établie une
fois par propriétaire immuable et par K, puis vérifiée par les vues liées,
sans affaiblir l'identité des données ni leur durée de vie. Un même contexte
mutable ne peut pas être appelé concurremment sans emplacements indépendants.
Ce raccord n'est pas encore implémenté ; les transferts réels comprennent
aussi les statuts, la provenance et le travail, pas seulement le BallId utile.

## 9. Histoires : calculer puis réutiliser leurs marques

La suite convenue dans le [dialogue de l'auditeur](../audits/DIALOGUE_COURANT.md)
distingue trois raccords. D'abord, résoudre les marques dans le premier
DSU, après fermeture complète de chaque plateau, pour éviter leur second
DSU/rejeu. Ensuite, consommer ces réponses pour les contributions. Enfin,
conserver les index de chaînes précédent/courant au lieu de reconstruire
l'index inférieur : 19→10 préparations pour K1..10. Les gains de temps de
ces raccords ne sont pas acquis par les mesures de gardes de rang.

Le [prototype de l'auditeur](../audits/receipts_fused_marks_20260911/README.md),
publié dans 2970d679, reste qualifié séparément sur 30 essais structurels.
Le [raccord propre du constructeur](MARQUES_PREMIER_PARCOURS_20260911.md)
passe maintenant O2/SAN sur 114 vrais census, 912 essais et 308208 marques
physiquement comparées, avant l'export inchangé. Sa porte structurelle
ajoute les forêts sans marques et vérifie 11504 coupes BFS sur 38 essais.
Il évite le second DSU/rejeu, mais n'implémente encore ni réemploi
contributif ni partage des index. Ses mesures et celles de l'auditeur
ne sont pas interchangeables.

La réutilisation demande une liaison explicite : MarkId=BlockId,
représentant=φ, admission=rang(B), et segment égal à la composante fermée
à cette admission. Les trois premières identités ne certifient pas la
quatrième. Une marque visant une autre composante vivante peut passer
les validateurs de parents/successeurs et produire une contribution
structurellement admissible mais fausse. Les histoires fraîches doivent
donc être construites et liées dans un propriétaire immuable explicite ;
des histoires extérieures gardent leur vérification indépendante, sans
option implicite pour faire confiance aux marques.

Ce propriétaire doit aussi posséder les données que l'Atlas emprunte :
index et census à adresses stables, sans alias mutable. Une factory publique
qui accepte des certificats arbitraires peut reconstruire fidèlement le
mauvais graphe ; des hashes ou des métadonnées compatibles ne suffisent pas.
Le prochain raccord utilisera une factory intégrée qui exécute elle-même
le producteur qualifié et la reconstruction avant de publier les vues
constantes. Un futur producteur parallèle séparé devra qualifier la même
frontière, pas seulement remplir des champs structurellement plausibles.

Émettre les contributions dans `atlas.program(K)`, à la date d'admission
de B, pas à la naissance du segment ; ne pas parcourir simplement les
marques triées par identifiant. Garder les singletons K1 à part, les marques
silencieuses et les coupes propres aux verticales. Les index adjacents
restent vivants jusqu'à leur dernière consultation, sur des histoires
immuables à adresses stables ; chaque histoire reste validée.

La qualification du premier parcours compare tous les champs physiques
des histoires/marques, y compris les forêts non vides sans marque et les
naissances tardives. Le prochain réemploi ajoutera une marque
forgée vers une autre composante vivante et un mélange de propriétaires.
Compter séparément marques réutilisées, HLD effectivement exécutées,
recherches de marque, préparations d'index et résidence des deux index.
