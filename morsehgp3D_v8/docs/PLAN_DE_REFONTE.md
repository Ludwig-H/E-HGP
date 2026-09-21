# Plan de refonte : moins de travail, puis davantage de parallélisme

13 septembre 2026. Plan issu de l'audit ; une première brique P0 est
maintenant implémentée et testée, pas la chaîne complète. Le
[rapport de synthèse](AUDIT_V7_SYNTHESE.md) situe les mesures historiques ;
les [mesures P0](../receipts/p0_local_credits_20260913/README.md) sont propres à la v8.
L'ordre demandé reste mono-thread, multi-CPU local, puis GPU G4 SPOT.

Contrat actif confirmé le21 septembre : **toute la tour d'une trame
SemanticKITTI entière**, sur plusieurs scènes, K1..10 puis repli1..5,
moins d'une seconde puis100ms sur G4. La référence principale n'est plus
50k points ; voir le [périmètre précis](CONTRAT_TRAMES_SEMANTICKITTI_20260921.md).
Les moitiés/quarts mesurent la croissance, pas la réussite de ce contrat.
Les références50k ci-dessous sont les objectifs et observations historiques.

Décision complémentaire du21 septembre : prioriser également le
[LiDAR sans sol](LIDAR_SANS_SOL_PROTOCOLE_20260921.md), avec segmentation
géométrique rapide de la trame entière puis coupes spatiales à masque
figé. Garder une campagne brute comparable ; publier le coût du retrait,
le temps HGP seul et le total. Ne pas importer une performance d'article
comme une mesure locale, ni utiliser les labels comme filtre de production.

Précision précisée ensuite le21 septembre : float32 original par défaut,
grille optionnelle paramétrable à1mm par défaut. Voir le
[port numérique](PRECISION_FLOAT32_ET_GRILLE_20260921.md). Préparation et
prédicat q2 exact sont séparés du moteur u16 : auditer/raccorder bornes,
clés, comparateurs et stockage q3/q4 sans supposer qu'un type plus large
suffit. Les performances20mm ne se transfèrent pas au nouveau profil.
Conserver le partage par blocs et la future distribution fine pendant
ce port ; ne pas en faire une nouvelle campagne de micro-optimisations q2.

Le [propriétaire/index float32](INDEX_FLOAT32_ET_SUITE_Q34_20260921.md) est
maintenant implémenté et qualifié : préparation O(n log n), boîtes exactes,
IDs/rangs et stockage contigu partageable. Les mesures8k/16k/32k et les
découpes de la trame0 sont closes ; cela ne transfère aucune propriété
au census. Priorité suivante : filtres/replis exacts q3/q4 et clés globales
construites à la demande, en conservant les comparateurs réduits et le
partage entre graines. Les18limbes du prédicat q2 ne suffisent pas aux
degrés5/6. Le [port ponctuel q3/q4](BOULES_FLOAT32_Q3_Q4_20260921.md) est
maintenant qualifié : supports128octets, coefficients filtrés réutilisés,
repli exact1728bits et oracles indépendants. Le port suivant ajoute
[clés globales à l'émission et comparateur réduit q4](IDENTITE_FLOAT32_ET_EVENEMENTS_Q4_20260921.md).
Il reste à porter les bornes de blocs et leur raccord ; ne pas confondre
ces briques avec le générateur ou son coût global. Une identité de boule
ne suffit pas pour trier les niveaux de rayon ou reconstruire FULL.

Le [census q3 natif par préfixe](CENSUS_Q3_FLOAT32_PARTAGE_20260921.md)
porte maintenant les bornes continues et le partage des graines d'UNE
arête.612comparaisons Fraction et36mesures synthétiques sont closes.
L'étape suivante doit raccorder les arêtes/graines natives et mesurer leur
travail global, puis transporter les tickets possédés entre workers.
Ne pas multiplier un chrono favorable par un nombre supposé d'arêtes ;
ne pas utiliser l'acceptation q3 pour restreindre q4. Catalogue, q4 natif
complet, FULL et régimes LiDAR brut/sans sol restent à développer/mesurer.

Le [plan de raccord natif global](RACCORD_NATIF_GLOBAL_PLAN_20260921.md)
précise maintenant quatre ports conjoints : front en flux, filtres avant
graines, propriété intégrée à la descente X, puis sorties q3 globales.
Ne pas appeler le census racine pour toute arête puis filtrer ses sorties.
Le pilote [sans sol](PILOTE_LIDAR_SANS_SOL_20260921.md) prépare les entrées
appariées ; son transport exact ne qualifie ni les classes physiques ni
la chaîne HGP. L'évaluation par labels est un diagnostic ultérieur séparé,
pas un préalable aux mesures du calcul.

Mise à jour utilisateur du21 septembre : les prochaines études LiDAR
emploient le [découpage spatial capteur](PROTOCOLE_LIDAR_SPATIAL_20260921.md),
pas une densité variable à8k/16k/32k. Mesurer scène, deux moitiés, quatre
quarts et six relations parent/enfant, sans forcer leurs cardinalités.
Le [raccord et les sept mesures scan0](Q34_MESURES_SPATIALES_20260921.md)
sont clos. Les rapports défavorables trame→moitié positive persistent
dans les bornes q3 et la construction d'atlas q4 ; priorité au partage
du census entre graines, puis à cette construction. Les résultats anciens
ci-dessous restent bornés à leurs entrées sous-échantillonnées.

État du21 septembre après34 : le [raccord graines/cellules q4](Q4_GRAINES_ET_CELLULES_20260921.md)
réduit fortement la navigation sur les trois scans mesurés. Le contrôle
LiveOnly fournit l'essentiel du gain ; ne pas prolonger les variantes de
cache Joined pour quelques pourcents sans hypothèse de réduction du coût.
Les [mesures34](../receipts/q4_seed_cells_20260921/ANALYSE_CROISSANCE.md)
incluent réellement8k/16k/32k, K5/10 et s8/10/12 dans leurs périmètres.
Atlas et census q3 dominent maintenant : à32k/K10,3,082Md tests de
partition et2,127Md bornes de comptage q3, avant le reste de la chaîne.
Scan200 conserve censusq3×4,317 et bornes d'atlas×4,355 au premier doublement.
Priorité suivante : centres conditionnels préparés par bloc de graines q3
(proposition A `f45e27c0`), comparaison au F direct et état compte/curseur
cohérent ; puis réduction de la construction d'atlas. Conserver des parents
immuables possédés pour les futures tâches de graines/paires redistribuables,
sans recopier les préparations. Aucun contrat FULL/G4 n'est acquis.

Réorientation explicite du21 septembre : les limites quadratiques restent
publiées, mais ne bloquent plus le raccord et les essais sur les régimes
visés. Priorité aux scans SemanticKITTI8k/16k/32k/50k et au coût complet
des candidats. Le [raccord global31](Q34_GLOBAL_ET_LIDAR_20260921.md)
mono/multi-CPU est maintenant écrit : mesurer front, expansion des arêtes,
covers, seeds, census et sorties avant de choisir l'optimisation suivante.
Les couches indexées ne sont plus un préalable imposé. La méthode q4
par blocs28 reste disponible et choisie par défaut ;30 est une option.
Cette réorientation ne relâche ni l'exactitude ni le contrat FULL/G4.
Le premier global8k/K5 donne maintenant la priorité concrète :361,201Md
tests ponctuels q3,99,1% extérieurs,780,662M seeds. Rejet précoce de
rectangles/blocs de seeds et census exact par boîtes avant micro-tris ou
autre file q2. La campagne locale arrêtée pendant16k ne donne aucun
ratio8→16→32 ; reprendre ces tailles après réduction du travail.
La série G4 CPU1k/2k/4k/8k confirme ensuite le défaut : census q3>×10
par doublement et8k614,744s avec48workers, mais3,23CPU moyens réellement
utilisés. Prévoir aussi le partage de produits, plages de paires et blocs
de seeds d'une arête, sans recopier leurs préparations. Pas de nouvelle
hausse de budget pour simplement prolonger ce même chemin jusqu'à50k.
Les priorités datées ci-dessous expliquent les décisions historiques.

Priorité de reprise du20 septembre, après3e94c868 : les options q2
fenêtre2K/héritage sont qualifiées et conservées. Cesser les micro-variantes
de constante ; construire les pièces manquantes q3/q4, catalogue et FULL.
Lire [l'audit de reprise](REPRISE_DEVELOPPEMENT_20260920.md) et la
[famille q4 par événements](Q4_FAMILLE_ET_BALAYAGE_20260920.md).
Le coût O(n log n) d'une famille ne borne ni les seeds ni le générateur.
Le [raccord des candidats positifs](Q3_Q4_CANDIDATS_PAR_SEED_20260920.md)
ajoute les clés communes et les décisions de support ; il ne déplace pas
la priorité suivante : génération des seeds par blocs et covers payés,
puis regroupement global avant collecte des intérieurs et reconstruction.

Après785d0589, les [covers partagés et l'accès par arête](Q3_Q4_COVERS_PARTAGES_20260920.md)
évitent une préparation par face. Ils gardent toutefois S·m lectures pour
S faces dans un cover de m sites. La priorité devient le rejet et le
comptage **collectifs entre familles**, pas la parallélisation de ce carré,
avant généralisation au front WSPD q3/q4. La campagne adverse conserve
ce défaut visible ; ne pas déduire une borne globale du seul fond lointain.

Après77f659e4, le [rejet universel de famille](Q3_Q4_REJET_FAMILIAL_20260920.md)
ajoute un petit pool spatial partagé par arête. Il supprime réellement des
familles avant leurs événements, avec masques q3/q4 indépendants et repli
à compte zéro. C'est une référence utile, **pas la fermeture du carré**.
Priorité suivante issue du dialogue A : calculer le minimum de profondeur
du même pool sur la corde q4 fermée ; les témoins peuvent changer le long
de cette corde. Compter tris, extrémités, groupes égaux et résidu aval.
La borne resserrée de corde est une proposition mathématique distincte,
dont l'arithmétique doit être prouvée avant port. Ne pas déplacer le carré
vers un prétraitement exhaustif de témoins ni vers des milliers de copies.

Après8d0a0f0f, la [corde resserrée et le minimum collectif](Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md)
sont portés avec quatre options appariées. Le gain de rejet est réel,
mais le dense32k/C64 laisse185,344M/358,240M lectures minimales futures
K5/10, avant tris : le terme faces survivantes × cover reste prioritaire.
Prochaine tranche : carte de minorants partagée dans le plan des centres,
pas d'arrangement complet, pas de nouveau réglage de constante du pool.
Étudier domaine, cellules, témoins hérités distincts et requêtes de faces
ensemble ; toute zone indécise garde le calcul exact. Le domaine des
complétions inclut les faces obtuses et ne restreint pas les témoins du
census. Mesurer préparation, fragmentation, UNKNOWN et coût aval total.
Le modèle indépendant A reste une proposition, pas un moteur qualifié.

Aprèsddaafdc0/A2920b8b5, la [carte partagée27](Q4_CARTE_CENTRES_PARTAGEE_20260920.md)
est qualifiée, mais sa composition après26 avec le même C64 ajoute trop
peu de rejets. Le domaine positif se prépare bien par blocs ; le scan
du résidu, lui, n'est pas réduit assez fortement. Aucun gain stable,
aucune borne sous-quadratique ; ne pas augmenter simplement profondeur
ou pool. Prochaine priorité : combiner blocs témoins et balayages exacts
locaux du résidu. Partition de tous les témoins en intérieurs certains,
extérieurs stricts et actives, contacts conservés ; comptes exacts non
saturés, IDs disjoints et frontières possédées. Les listes du filtre27
retirant min≥0 et ses crédits comprimés ne satisfont pas ce contrat.
Payer préparation, réplication des actives, requêtes, tri et coquilles,
avec étude8k/16k/32k du total. Les propositions de A restent des objets
d'audit jusqu'au port et à leurs propres preuves constructeur.

Après66b1551f/A5ff70645, les [fragments exacts et balayages locaux28](Q4_FRAGMENTS_ET_BALAYAGES_LOCAUX_20260920.md)
remplacent les scans complets du cover par une partition de blocs partagée
et un résidu local réellement exécuté jusqu'à32k. Finir chaque partition
terminale une fois évite de repayer ses blocs inconnus à toutes les faces.
Le clipping conserve les contributions constantes et les coquilles.
Mais W dense permuté fait×3,922/×4,184 et le tri×4,564/×4,857 : le carré
reste dans les feuilles, pas dans leur seul stockage. Suite structurelle :
partager aussi le traitement des faces ou traiter les événements collectifs
de faible profondeur, en payant les listes de conflits et leur duplication.
Une hausse uniforme de profondeur/pool ou un simple port GPU du double
parcours ne résoudrait pas ce verrou. L'atlas immuable et les buffers privés
sont une base de distribution, pas encore une qualification parallèle.
Conserver la référence locale exacte, puis raccorder q3 séparément et
le générateur global ; ne pas confondre une arête fournie avec toute la tour.

Aprèsc051bdb0, les [couches convexes duales29](Q4_COUCHES_DUALES_20260920.md)
retirent simultanément des seeds et des témoins avant les événements.
Leur certificat exact conserve profondeur faible et coquille complète,
sans hypothèse de position générale ; préparation O(m log(1+m)+Km).
Le dense permutéK10 n'a plus que165648/499848/1575024 lectures à8k/16k/32k,
mais l'adversaireK10 reste quadratique et régresse contre28. Le tri des
événements profonds entre survivants est maintenant explicite : prochaine
question à l'auditeur, extraction des événements peu profonds et articulation
avec les blocs28. Une sélection sous-quadratique à K fixé ne suffit pas,
et son scan commun par arête peut dominer lorsque S est petit. Garder28
comme référence, ne pas ajouter une autre variante de budget ; mesurer
la somme sur toutes les arêtes au futur raccord WSPD q3/q4.

Après31b0243a, la [fenêtre exacte30](Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md)
remplace le tri général des survivants29 par deux rangs certifiés :
H-ième sortie décroissante et H-ième entrée croissante, H=K−2−c.
L'intervalle est fermé, y compris lorsqu'il se réduit à un point ;
les constantes et toutes les égalités de coquille restent exactes.
Au plus2H−2 IDs strictement intérieurs sont triés, mais les deux scans
des r témoins par face restent payés. Le premier garde un carré mesuré
sur l'adversaireK10 : **prochaine priorité, supprimer S×r**, pas réduire
encore les petits tris. Conserver les couches comme un index de chaînes
convexes orientées, avec multiplicités et groupes plats, pour rechercher
les rangs et contacts sans scan complet. Cette recherche exacte n'est
pas implémentée ; constantes nulles, tangences, frontières de dimension
zéro et restitution des IDs doivent être jugées indépendamment.
La composition avec28 exige des populations emboîtées : l'intersection
de deux noyaux certifiés indépendamment n'est pas un certificat.
Ne pas oublier ensuite q3, WSPD multivoie, catalogue et FULL ; aucun
temps par arête ne vaut contrat50k/G4 ni borne sur leur somme.

Suivi du 14 septembre : le [raccord direct front → census q2](P0_FRONT_ET_CENSUS_Q2.md)
est implémenté. Il retire factories, copies et arbres B par rectangle,
sans filtre axial obligatoire. Il reste à réduire le coût du front et
celui des recherches par ancre, puis à fournir des continuations
distribuables et un consommateur q3/q4 canonique. Aucune garantie globale
sous-quadratique ou de tour ne découle de ce raccord. Les mesures portent
sur les supports avec collecte, pas seulement sur les produits compacts.

Neuvième tranche : [certificat autonome du frère](P0_CERTIFICAT_FRERE_Q2.md),
un test constant après chaque division B, sans nouvelle recherche ni
crédit partiel. Le point décisif reste la croissance de toutes les
visites, propositions et tâches, pas seulement le nombre de paires
éliminées par ce certificat. Le partage entre ancres et le choix des
blocs témoins restent à traiter si cette option ne réduit qu'une constante.

Dixième tranche : [ordre complément/B original](P0_ORDRE_TEMOINS_Q2.md),
avec exclusion de l'ancre du seul comptage. Mesurer le coût structurel
en plus des tests géométriques ; moins de bornes ne signifie pas moins
de travail total. La prochaine expérience garde A et B comme groupes
dans le census, puis reprend le chemin à ancre fixe depuis le même
préfixe quand A devient singleton. Aucun redémarrage ni tableau de
paires. Cette piste reste à implémenter et à comparer, sans preuve de
gain global ; elle ne dispense pas de traiter l'ordre des témoins.

Onzième tranche : le [census conjoint A×B](P0_CENSUS_CONJOINT_Q2.md)
ouvre ce raccord avec reprise singleton du même préfixe. Les coûts
conjoints, ceux des ancres restantes et la collecte sont distingués.
Les bornes à 96 octets ne constituent pas une optimisation gagnante
par leur seul format : comparer le travail total sur les quatre régimes.

Suite prioritaire après cette comparaison : [Pool terminal raccordé au
census global](P0_POOL_TERMINAL_RACCORD.md). Le constat B à e6388e55
montre que 28 gros produits dominent le résidu des amas ; il ne mesure
pas encore leur census. Préparer un plan par parent sur les rangs du
même index, ne développer que ses survivantes et repartir de zéro dans
le census global. Mesurer F=Σ tailles de facteurs, pas seulement M.
Ce n'est ni un retour aux histogrammes carrés ni une qualification
héritée du harnais d'audit ; le raccord et l'oracle restent à construire.

## Priorité P0 — supprimer la préparation quadratique des témoins locaux

Tranche17 : le [raccord à une équipe persistante](P0_EQUIPE_PERSISTANTE_Q2.md)
est implémenté pour les seeds Coarse et les continuations q2 hors Pool.
Les rectangles Pool restent synchrones, même lorsqu'ils ne retirent aucune
paire ; `pool_min_factor <= min_b_size` positif rend cette nouvelle voie
inactive. Mesurer son travail et son coût réel avant de changer le défaut.
Restent à posséder/partager le plan Pool parent et les curseurs de bandes,
puis à combiner les continuations de front et de census dans une même
fermeture. Aucun bénéfice global de complexité ou de tour n'en découle.

Seizième tranche : [frères B détachables et répartiteur intérieur](P0_DETACHEMENT_CENSUS_Q2.md)
implémentés pour une ancre. Les branches partagent le contexte original,
pas l'historique déjà payé ; leur somme garde le travail mono. Raccorder
ensuite aux workers persistants du front et aux plans Pool parentaux
possédés : ne pas créer une équipe par ancre, ni convertir les bandes
de permutation Pool en nœuds B globaux. Aucun changement du chemin
q2 complet ni réduction de complexité générale par ce seul détachement.

Quinzième tranche : [census d'une ancre reprenable](P0_CENSUS_REPRENABLE_Q2.md)
qualifié, sans modification du pipeline existant. Extraire ensuite les
frères B en attente pour les exécuter indépendamment ; leur simple migration
dans la pile entière ne suffit pas. Garder les plans parentaux possédés,
pas de préparation par job. La [stratégie q3/q4](Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md)
fixe séparément les formes, blocs de complétions et événements à considérer ;
les voies acceptées ne sont pas des filtres d'accès successifs.

Quatorzième tranche : [redistribution des produits pendants](P0_REDISTRIBUTION_FRONT_Q2.md)
implémentée et qualifiée ;192 mesures propres closes. Le gain de temps
n'est pas général, Coarse reste le défaut.
Le coût des dons/refus et la charge discrète doivent décider du bénéfice ;
le temps de présence inclut désormais les attentes. La file bornée ne
tronque rien et les callbacks déjà commencés restent indivisibles.

Treizième tranche : [jobs du front et workers q2](P0_FRONT_WORKERS_Q2.md)
implémentés sans dupliquer les tests parentaux ni préparer les facteurs
par morceau. Tous les comptes géométriques restent ceux du mono.
Cette première distribution ne partage pas une pile DFS après le départ
du job : mesurer sa queue de durée, pas seulement la moyenne. L'audit A
329e5b86 montre sur son prototype LiDAR50k qu'un seul sous-arbre garde
environ30 % des descentes même avec256 jobs. Redistribuer les produits
encore pendants est une suite ciblée ; les gros callbacks indivisibles
restent à traiter séparément. Aucune borne globale ni tour G4 acquise.

Suivi douzième tranche : le [raccord Pool terminal](P0_POOL_TERMINAL_Q2.md)
est maintenant implémenté sur les nœuds globaux, avec au plus K bandes et
census des survivantes à compte zéro. Le prototype A fbbecc01 a mesuré
le coût q2 complet ; le port a désormais ses 49 CTests Release/Clang et
80 mesures propres. À s8/K10, amas32k passe de184,306 à19,180 s,
avec visites ×2,958/×2,701 aux doublements8k/16k/32k au lieu de
×4,106/×4,229. Ce constat expérimental ne borne pas toutes les familles.
Le repli
sans réduction conserve le partagé sur les rangées, tout en comptant le
plan tenté. Après le filtre, front et petits rectangles sont prioritaires.
La taille du résidu partiellement réduit doit encore orienter le choix
du consommateur ; pas de promotion universelle de Pairwise ou du seuil64.

**Décision explicite de l'utilisateur, 13 septembre 2026 : ce changement
radical passe au premier rang de la refonte.** Après l'échec des témoins
universels, la v8 ne doit plus imposer les parcours exhaustifs A×A et B×B
pour pouvoir sélectionner les paires de A×B. Le coût systématique
O(|A|²+|B|²) des histogrammes v7 est un défaut d'architecture à supprimer,
pas un passage obligatoire à accélérer sur GPU.

L'objet requis pour le préfiltrage est un **minorant certifié** du nombre
de témoins utiles, pas nécessairement l'histogramme exact. Une paire peut
être rejetée dès que h_cœur+minorant_a+minorant_b atteint h_q, avec les
populations disjointes et les prédicats stricts requis. Un minorant
insuffisant signifie « indécis » : il ne prouve ni vacuité ni validité.
Le census et la chaîne finale gardent leurs obligations d'exactitude
et de complétude. Davantage de candidates intermédiaires est admissible
mathématiquement, mais peut rendre une variante économiquement mauvaise.

**Le petit ensemble de témoins est une piste, pas la solution retenue
par avance.** Comparer les familles suivantes, ainsi que toute meilleure
proposition de l'auditeur ; les combinaisons restent possibles :

| Famille à étudier | Économie recherchée | Obligation encore ouverte |
| --- | --- | --- |
| Petits ensembles de témoins proposés puis certifiés | O(h) sites de chaque côté, testés sur toutes les extrémités : préparation O(h(|A|+|B|)) si leur sélection est payée dans cette borne | Choisir des témoins efficaces ; un ensemble fixe peut manquer de nombreuses configurations locales |
| Parcours conjoints de blocs d'ancres et de témoins | Partager certificats positifs/négatifs et crédits entre plusieurs lignes, raffiner seulement les cas indécis | Borner les blocs réellement visités, garder les identités disjointes et ne pas recréer le carré dans les descentes |
| Résumés directionnels, rangs ou enveloppes géométriques | Réutiliser une préparation pour proposer ou certifier beaucoup de crédits | Justifier chaque rejet, payer construction et requêtes ; un rang projeté seul ne prouve pas l'intérieur en 3D |
| Sélection/réduction directe de sous-rectangles sans histogrammes complets | Éviter de compter tous les témoins d'une ligne avant de traiter les produits utiles | Préserver la couverture et la propriété canonique ; éviter que les raffinements ou la validation tardive explosent |

Pour la première piste, la préparation puis la sélection par classes
peuvent coûter O(h(|A|+|B|)+M), où M compte les paires **encore candidates**.
Ce n'est ni le nombre de boules pertinentes ni la taille de sortie FULL.
Cette borne locale ne prouve donc pas une génération globalement
sous-quadratique. Elle ne borne pas non plus à elle seule la somme du
travail sur tous les rectangles de la WSPD.

Critères de choix et de clôture de P0 :

1. Prouver la sûreté des rejets et la complétude du chemin résiduel, avec
   fixtures de crédits partiels, témoins manqués, double comptage et
   frontières. Ne pas exiger des candidates intermédiaires identiques
   à celles de la v7 ; comparer les objets finaux requis.
2. Mesurer en mono n=8 000/16 000/32 000, s=8/10/12, sur gros facteurs
   équilibrés et déséquilibrés, amas, uniforme et géométries adverses :
   coût de sélection des témoins, tests/visites, blocs, paires résiduelles,
   coût q3/q4/census et mémoire. La préparation moins chère ne suffit pas
   si elle augmente davantage le travail aval.
3. Retenir la meilleure architecture sur le travail total observé et
   justifié ; annoncer les régimes non résolus. Un simple gain constant,
   une accélération GPU du même carré ou son déplacement vers les
   candidates ne clôt pas cette priorité. Aucun quota de troncature
   n'est un substitut à une sortie complète.

Le scalaire sur petits sous-blocs peut rester une feuille d'exécution,
et l'exhaustif borné un juge différentiel ; ni l'un ni l'autre ne doit
réintroduire un histogramme quadratique systématique sur les gros facteurs.
Le cadrage de l'API et les petites fixtures FULL ci-dessous soutiennent
ce chantier ; ils ne le repoussent pas derrière un port général de la v7.
P0 reste **ouverte, sans solution finale choisie**. Première tranche
réalisée : [Pool/DualBlocks/Tubes](P0_CREDITS_LOCAUX.md), oracles C++
indépendants, huit CTests Release/sanitizers et mesures mono 8k/16k/32k.
La préparation des tubes évite le carré local obligatoire ; les résidus
et le coût aval ne sont pas résolus. Priorité suivante : partager la
préparation entre voies et rectangles, puis traiter le résidu des nappes
par une couverture raffinée, sans recopier les tableaux à chaque enfant.
Le census exact minimal doit juger la suite, pas seulement compter M.

Deuxième tranche : [préparation partagée et filtre axial q2](P0_PARTAGE_ET_FILTRE_AXIAL.md).
Le tri/cellules Tubes est commun aux trois voies ; un filtre par colonnes
exactes et index B émet des plages sans développer A×B. Les nappes
complètes ont un résidu borné par m(2h+1)², mais le cas général reste
ouvert. Les prochaines variantes précises sont l'addition des colonnes
exactes disjointes, les queues/fenêtres A/B proposées par les auditeurs,
puis le consommateur q2. Ne pas remplacer cette comparaison par un port
GPU prématuré d'un résidu inutile. La validation partagée sur toute la
WSPD et la mémoire de pointe restent à traiter.

Deux nouveaux résultats de l'auditeur précisent ce prochain choix :
sur ses [rangées transverses](../../audits/morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md),
les crédits universels parfaits sont tous nuls mais seules 11m−30 paires
q2 sur m² restent sous le seuil 10, pour m>5. Le raffinement doit donc
agir sur les produits ou la profondeur, pas seulement rechercher davantage
de témoins universels. Sur sa
[tige avec témoins en bout](../../audits/morsehgp3D_v8_complementaire/P0_ORDRE_TEMOINS.md),
changer l'ordre de visite réduit fortement les tâches Dual à comptes
inchangés. Une priorité par projection vers le facteur opposé mérite un
différentiel tous axes et tous régimes ; cette optimisation n'est pas
encore intégrée. Ne pas confondre l'ordonnancement des témoins avec le
préchargement de leurs crédits, qui exigerait une disjonction.

Le second auditeur propose désormais des
[sous-rectangles couvrants et des groupes de témoins](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md).
Son prototype q2 réduit le résidu transverse n512 de 65 536 à 4 032
candidates, avant les 2 786 paires réellement sous seuil ; ce n'est pas
encore une intégration au produit. Préfixes/suffixes donnent les boîtes
des queues sans rescanner leurs points. Pour q3/q4, son certificat collectif
garantit au moins un intérieur par groupe, sans exiger le même témoin
dans toutes les boules. Les groupes doivent avoir des IDs disjoints pour
additionner leurs crédits. Comparer ces certificats après partage des
préparations ; ne pas transférer les résultats du prototype au moteur v8.

Après la qualification de l'addition et de l'intersection q2, le
[census partagé](P0_CENSUS_Q2_PARTAGE.md) est maintenant implémenté et
comparé à la recherche par paire sur les mêmes plages résiduelles. Son
état `(ancre, groupe B, compte, curseur non consommé)` ne copie aucune
liste de continuation. Les mesures paient l'index de tous les sites, les
subdivisions et la collecte des IDs d'intérieur/coquille. Après intersection,
moins de visites ne suffit pas à compenser le prix des tests de groupes.
Cinquième tranche : les [constantes sont préparées par tâche](P0_BORNES_PREPAREES_ET_PARALLELISATION.md),
comparées à coût complet, avec la référence individuelle conservée.
La baisse locale mesurée est modeste et ne change aucune visite.
Le flux conserve les supports et les clés exactes ; la comparaison à Pool
seul, la déduplication globale, la vraie WSPD et la tranche FULL minimale
restent les raccords suivants, sans nouvelle préparation quadratique.

Sixième tranche : [nuage/index global partagé](P0_NUAGE_ET_INDEX_PARTAGES.md),
rectangles sans copie ni validation de n sites, et boîtes de plages
en O(log n). Le contexte rectangle/seuil, la permutation B et l'identité
de l'index Z restent distincts de la seule appartenance au nuage.
**Priorité suivante : pilote de front réel et coût cumulé des facteurs.**
Le terme Ω(R|B|) de Pool/Axis subsiste sur A_i×B ; les tests doivent faire
croître R aussi, et pas uniquement n à R fixé. Ne pas paralléliser ce
carré sans le traiter. Le nouvel audit du front pur montre des millions
de petits rectangles : chemin sans préparation pour petits facteurs,
permutation spatiale globale et rejets certifiés avant séparation complète
sont à examiner ensemble. Fixer la convention s, différente de la v4,
avant de comparer les WSPD s8/10/12. Le raccord Pool par préfixes et la
suspension du parcours doivent partir des objets communs et de leur ordre
exact. Tester mono les reprises, puis plusieurs CPU, puis G4.

Septième tranche : [premier front réel](P0_FRONT_REEL.md), par handles de
nœuds et masques de voies, sans ces plans ni parcours de facteurs par
produit. La convention est `box_gap_diameter_v1`. Les propositions
ponctuelles sont bornées par O(D+K) par produit ; les masques de rejets
sont transmis, pas les comptes partiels. La couverture est jugée par
énumération indépendante sur petits nuages ; les grandes mesures
comptent aussi les masses résiduelles par voie, sans les développer.
Le contre-exemple des deux rangées montre qu'un proposeur ponctuel
parfait laisserait encore m² candidates q3/q4. **Priorité suivante :
raccord direct au census q2 global, puis certificats collectifs q3/q4**,
sans recréer Pool/Axis ou recopier B par descripteur. Le test de lentille
et la transmission de certificats sont des optimisations à juger sur
ce chemin consommé ; le front seul ne clôt ni P0 ni la complexité aval.

L'auditeur A fournit en §9.5 de
[son étude des sous-rectangles](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md)
une autre composition sûre : distribuer les rangs d'un plan parent
déjà préparé, en conservant ses crédits et son ordre B. Ce n'est pas
reconstruire chaque enfant comme dans le témoin de coût Ω(R|B|).
Cette proposition n'est pas encore une API de jobs du moteur.

Les [consignes au futur développeur](VERROUS_ARCHITECTURE.md) détaillent
les cinq verrous suivants : recherches de témoins répétées, interactions
triangles–voisinages q3/q4, rattachements MEB, histoire/export centraux,
puis mémoire et transport. B1/B2 doivent être mesurés dès le travail P0
pour détecter un déplacement du coût ; B3–B5 guident les objets aval.
Ces repères ne changent pas le premier rang de P0. Leur statut reste
ouvert et leurs critères ne constituent pas des gains acquis.

## 1. Une seule chaîne et un seul contrat de sortie

Tout livrable de code devra respecter une API FULL explicite : entrée,
métrique, Kmax, convention de coupe, nœuds, parents, contributions,
verticales, statut de complétion. L'archive F et les probes privés ne
seront pas réétiquetés en produit FULL.

Réutiliser explicitement les primitives v7 sélectionnées et leurs
contre-fixtures, avec sources épinglées et requalification v8. Les anciens
producteurs restent des différentiels externes. Ne pas maintenir plusieurs
copies concurrentes du moteur dans `build/` et des archives opaques.

Une préparation géométrique immuable possédera l'entrée, l'index, le
catalogue certifié, les clés, les permutations, les rangs de niveaux et
les masques. Les vues ne devront ni survivre à leur propriétaire ni se
faire passer pour celles d'un autre nuage. Son constructeur public devra
produire ou vérifier réellement ses certificats, pas accepter un faux
certificat bien formé. Une validation relative à un producteur donné sera
annoncée comme telle, jamais transformée en preuve de complétude universelle.

## 2. Objets proposés pour les étapes coûteuses

| Étape | Objet à conserver | Grain de tâche parallèle | Travail à éviter |
| --- | --- | --- | --- |
| Front WSPD | Descripteurs plats de rectangles, masques q, témoins identifiés | Rectangle ou sous-tâche spatiale selon coût | Redémarrer tous les parcours et recréer une équipe à chaque vague |
| Témoins locaux, P0 | Minorants certifiés, classes de crédits et preuves de blocs | Selon l'architecture comparée, sans histogrammes complets imposés | Préparation systématique A×A et B×B |
| q2 | Ancre, groupe B, compte uniforme et curseur de témoins restant | Groupes puis paires indécises | Repartir de l'index racine après crédit ou copier de longues frontières |
| q3 | Seed canonique et plage de cover | Seed×plage de sites | Recherche complète séquentielle par seed |
| q4 | Intervalles/racines exacts, segments identifiés | Calculs de racines puis tri/scan segmentés | Balayage mono de tous les événements d'une seed |
| Catalogue/census | Une boule canonique, intérieur/coquille, rang partagé | Boules ou blocs de parcours | Clés, tris et validations identiques répétés |
| Rattachements | Facettes entières uniques, terminales certifiées | Requêtes géométriques indépendantes | Descentes répétées et essai lexicographique de tous les supports |
| Histoire par K | Graphe daté sur naissances, puis forêt minimale | Arêtes puis contractions de sous-arbres | Calendrier global qui déclenche de la géométrie |
| Contributions/verticales | Marques datées, index historiques partagés | Requêtes indépendantes et préfixes d'écriture | Reconstruire les index voisins ou les mêmes requêtes |

Un rectangle est une unité logique de preuve et de couverture ; il n'est
pas nécessairement une unité physique de lancement GPU. Découper le travail
en tuiles ne doit pas émettre plusieurs fois une paire ni perdre un crédit.

Invariant précisé par le [nouvel auditeur B](../audits/DIALOGUE_AUDITEUR_B.md) :
l'élimination est propre à une voie q, tandis que la rétention du catalogue
porte sur la boule canonique. Une clé est conservée dès qu'une voie la
conserve ; un rejet q3 ne doit pas effacer une boule admissible produite
par q2. Les supports q3/q4 doivent être engendrés depuis leur **plus longue
arête**, avec départage déterministe des égalités, car les bornes de fuseau
emploient cette longueur. Ces invariants devront recevoir leurs fixtures
lors du raccord du catalogue et des producteurs, encore absents.

## 3. Éliminer tôt les rectangles, sans payer plus que ce que l'on épargne

Les détails et les bornes entières sont dans
[WSPD q2/q3/q4](../audits/WSPD_Q2_Q3_Q4.md). Les pistes ci-dessous sont
subordonnées au choix ouvert de P0 ; elles n'imposent pas de reconstruire
les histogrammes v7. Priorités :

1. Proposer rapidement quelques témoins probables ; vérifier exactement
   qu'ils sont intérieurs pour **tout** le rectangle. Le proposeur peut
   échouer : seul le certificat autorise l'élimination, avec repli complet.
2. Transporter au plus h_q−1 **identifiants** déjà certifiés du parent vers
   ses enfants : la propriété universelle se conserve par restriction.
   Exclure ces identifiants des nouveaux crédits. Ce transport est une
   proposition nouvelle à contre-auditer ; copier seulement le compteur
   h peut compter deux fois les mêmes sites.
3. Utiliser h extérieur à A∪B, puis des minorants de h_a dans A privé de
   a et de h_b dans B privé de b. La disjonction est une partie de la
   preuve, pas un détail d'implémentation. Saturer lorsque le seuil utile
   est acquis ; il n'est pas obligatoire de connaître les autres comptes.
4. Choisir les blocs positifs **et négatifs** en fonction du coût réel ;
   un petit facteur peut coûter moins cher en scalaire. Réutiliser la
   piste v7 négative/saturation seulement après raccord et requalification.
5. Si les certificats ne suffisent pas, raffiner les classes de crédit
   ou les tâches de paires ; compter les visites et les crédits effectivement
   payés. Ne pas remplacer un plafond arbitraire par une boucle quadratique
   cachée sous le mot « adaptatif ».

Le seuil vaut Kmax/Kmax−1/Kmax−2 pour q2/q3/q4. Le plus petit fuseau q4
est une condition suffisante d'intérieur commun, pas toute la boule
circonscrite de chaque tétraèdre. Un certificat indécis ne justifie aucune
suppression. Le cover q4 doit rester assez large pour inclure tous les
points qui peuvent contribuer à sa profondeur.

Comparer s8/10/12 en séparant temps du front, histogrammes et chaque voie.
Ne pas choisir s sur le seul nombre de candidats finals : le coût de
recherche des témoins peut croître alors que ce nombre baisse.

## 4. Réduire le coût d'une terminale avant de multiplier les threads

La cible est le coût des milliards d'essais de supports, non le coût d'une
opération arithmétique isolée. Comparer, sur les mêmes requêtes :

- Support proposé à partir de la boule source et de l'échange précédent.
- Solveur de petit ensemble qui propose directement une MEB candidate.
- Certification exacte : support positif, confinement, coquille et clé.
- Repli complet actuel si la proposition n'est pas certifiée.

Mesurer taux de succès, essais évités, nouvelles recherches d'intrus et
coût des cas difficiles. Une proposition flottante ne devient jamais
une décision géométrique. Une terminale doit être comparée **avant** sa
normalisation historique : une mauvaise terminale peut rejoindre par
hasard la même racine finale et tromper un juge trop faible.

Regrouper les demandes identiques par leur facette entière ; conserver
les destinations originales pour disperser les réponses. Chercher le
compromis entre réemploi global et mémoire des fenêtres. Un cache sans
bornage de résidence ni propriétaire stable n'est pas une solution massive.

## 5. Paralléliser la vraie histoire, pas seulement son tri

Une fois les rattachements connus, chaque ordre dispose d'un graphe daté
sur ses naissances. En extraire une forêt couvrante minimale conserve
les composantes de toutes les coupes. Construire ensuite l'histoire de
cette forêt, puis regrouper les événements de même niveau par vraie
connexité, sans mélanger les plateaux disjoints.

La littérature fournit des constructions parallèles de dendrogrammes à
partir d'un arbre pondéré. RCTT utilise une contraction d'arbre et obtient
une profondeur polylogarithmique ; sa variante ascendante dépend de la
hauteur du résultat. C'est une raison de tester aussi des histoires en
peigne, pas seulement des arbres équilibrés.
[Source primaire, Dhulipala et al., SPAA 2024, v2](https://arxiv.org/abs/2404.19019v2).

PANDORA propose une contraction récursive destinée aussi aux GPU, avec
une implémentation Kokkos CPU/GPU. Elle fournit une seconde piste pour
éviter le calendrier séquentiel.
[Source primaire, Sao et al., version 1](https://arxiv.org/abs/2401.06089v1).

Ces sources sont examinées pour leur algorithme, pas pour transférer leurs
temps à HGP. Elles partent d'un MST/arbre pondéré **déjà disponible** :
ni génération Gabriel, ni rattachements, ni contributions, ni verticales
HGP ne sont payés par leur seule reconstruction de dendrogramme.
L'adaptation aux dates exactes égales et aux multifusions doit être prouvée.

## 6. Faire payer une seule fois les objets communs

Remplacer les préparations répétées Builder→Atlas→resolver par un
propriétaire certifié partagé. Construire chaque index historique une
seule fois, soit dix préparations au lieu de dix-neuf ; ne conserver
que le précédent et le courant lorsque leur usage le permet.
Conserver les marques calculées pendant le premier parcours et prouver
que leur histoire/propriétaire est le bon avant leur réemploi aval.

Pour l'export, compter les tailles puis attribuer les destinations par
sommes préfixes. Éviter petits vecteurs, fragments temporaires et copies
de populations complètes. Les références de populations peuvent être
partagées sans confondre les composantes qui recouvrent les mêmes points.

La sérialisation FULL industrielle doit avoir un manifeste distinct de
l'archive F, des offsets suffisamment larges, des lots identifiables et
un journal de reprise. Le coût de l'archive durable doit être séparé du
contrat de livraison en mémoire, jamais retranché sans déclarer le périmètre.

## 7. CPU et GPU doivent exécuter le même plan de travail

Après le chemin mono, une équipe CPU persistante consommera les tâches
plates. À quatre CPU, vérifier objets identiques et destinations fixes,
puis mesurer le gain apparié ; éviter de relancer des pools imbriqués.
Exiger un travail identique quand seul l'ordonnancement change ; compter
les opérations spéculatives supplémentaires lorsqu'un découpage plus fin
modifie le travail payé avant les arrêts anticipés.

Sur GPU, conserver catalogue, rangs, tâches, réponses et preuves compactes
sur carte autant que possible. Préfixes, compactions, tris segmentés et
réductions de statuts remplacent les reconstructions hôte ligne par ligne.
Le repli CPU rare doit être compté avec ses transferts. Chaque kernel
devra être exécuté sur carte : compilation et émulation ne suffisent pas.

La géométrie exacte peut entraîner des registres ou des piles trop lourds.
Choisir le grain de tâche à partir du travail/résidence observés, pas du
simple nombre de rectangles. Une voie GPU donnant un grand débit de
primitives sans réduire le temps de tour ne franchit aucun contrat.

## 8. Portes de preuve avant mesures coûteuses

| Décision | Tests positifs et contre-fixtures à porter explicitement |
| --- | --- |
| Objet FULL | K1=single-linkage, minima isolés, recouvrement sans fusion, K=n |
| Réduction Gabriel | E5, graphe induit à quatre points, attache silencieuse avec bonne date |
| Plateaux | AB/ABC même boule, composantes disjointes de même date, plateau coupé entre lots, grande arité |
| Témoins | h/h_a/h_b disjoints, double crédit parent/enfant, marge nulle, succès/non-vacuité et absence de témoin |
| q3/q4 | Supports non positifs, dégénérescences, cover trop étroit, racines égales, profondeur qui redescend |
| Resolver | Terminale fausse mais racine finale correcte, semis rejeté puis repli exact, intrus choisi différemment |
| Propriété | Certificat forgé, mélange de nuages, vue périmée, mutation d'un propriétaire emprunté |
| Tour/export | Contributions datées, verticale fermée à la naissance, naturalité, refus de publication partielle |
| Massive | Gros facteurs, grandes coquilles, débordement d'identifiants d'objets, allocation refusée, reprise |

L'oracle exhaustif reste borné et indépendant. Il ne devient pas le
producteur général sous un autre nom. Réutiliser une fixture ne signifie
pas hériter de son résultat ancien ; enregistrer les nouvelles commandes.

## 9. Mesurer la croissance et les contrats sans dépenser à l'aveugle

Localement, mêmes sources et mêmes entrées, n=8 000/16 000/32 000,
s=8/10/12, mono puis quatre CPU. Familles : uniforme, terrain/filiforme,
amas séparés, arcs adverses ; ajouter les cas de dégénérescence au
protocole fonctionnel. Mesurer par étape travail total, pire tâche,
répartition, mémoire simultanée et volume de sortie.

Le temps par unité de sortie distingue amplification algorithmique et
sortie réellement grande. La borne quadratique FULL interdit une promesse
universelle d'énumération sous-quadratique ; elle n'autorise pas du travail
quadratique inutile lorsque la sortie est petite. Ni le choix de s ni le
passage GPU ne remplacent cette analyse.

Une fois la chaîne complète raccordée, ouvrir une courte session G4 SPOT
avec les scripts gardés du dépôt. Qualifier toute la tour 1..10 à 50k,
puis le repli 1..5 si nécessaire, avec deux échauffements, dix nuages frais
par famille et p95. Si une seconde est atteinte, viser 100 ms au même
périmètre. Aucun seuil par composant n'est un contrat accompli.

Le contrat multi-millions est séparé : commencer par la résidence et les
formats, puis une complétion vérifiable aux paliers prévus, dont plusieurs
dizaines de millions. Ne pas extrapoler un pic 50k en succès massif.
N'acheter du temps GPU que pour une expérience qui tranche une question
ouverte ; arrêter la cible exacte dès le résultat utile acquis.

## 10. Premier chantier après cet audit

Suivi tranche18 : le [partage d'ancres et de plans Pool possédés](P0_PLAGES_ANCRES_Q2.md)
est implémenté et qualifié :72 CTests Release/Clang ASan/UBSan, gate
TSan et174 mesures closes, lecteurs normal/−O concordants. Il évite l'allocation de pile
par petite racine et conserve le Shared des replis sans rejet ; il ne
réduit pas le nombre de tests géométriques. Mesurer la gestion par plage,
la durée de vie simultanée des parents, F, le résidu et les sorties avant
de choisir cette entrée. Aucun gain stable sous charge, Coarse reste le
défaut. Priorité alors annoncée : les millions de petits census en lots
compacts et la réduction de leur travail, pas une autre file. FULL/G4 reste ouvert.

Suivi tranche19 : les [lots de singletons entrelacés](P0_LOTS_SINGLETON_Q2.md)
sont implémentés, qualifiés (75 CTests Release/Clang ASan/UBSan, porte
TSan, 172 mesures closes, lecteurs normal/−O concordants) et **fermés comme
résultat négatif** : lots / Coarse ×1,005 à ×1,225 sur 54 comparaisons
n8k/16k/32k, entrelacement sans effet, comptes géométriques inchangés.
Coarse reste le défaut. Le format des petites requêtes n'est pas le
levier ; la priorité passe à leur nombre : rejeter plus de produits au
front par la [surproposition de témoins](P0_SURPROPOSITION_TEMOINS_Q2.md).
FULL/G4 reste ouvert.

Suivi tranche20 : la [fenêtre de propositions élargie](P0_SURPROPOSITION_TEMOINS_Q2.md)
est portée comme option explicite de la voie q2, défaut identique au moteur
précédent (différentiel contre un build épinglé), et qualifiée : 78 CTests
Release/Clang ASan/UBSan, portes TSan, rejeu indépendant du front q2 avec dix
mutants causaux, 684 mesures closes. Fenêtre 2K petits facteurs à n8k/16k/32k :
temps q2 complet ×0,42 à ×0,54 (uniforme), ×0,51 à ×0,62 (amas), ×0,64 à
×0,71 (terrain), ×1,01 à ×1,18 (rangées, régression publiée). C'est une
constante divisée, pas un exposant : la croissance du travail restant est
celle de la référence. P0 global et borne sous-quadratique restent ouverts ;
suite à mesurer avant tout port : descente exacte plafonnée à K ou
transmission des témoins certifiés aux enfants, puis q3/q4 et FULL.

Suivi tranche21 : les [témoins certifiés hérités](P0_TEMOINS_HERITES_Q2.md)
du § 3, point 2, sont portés comme option explicite de la voie q2 et
qualifiés : 81 CTests Release/Clang ASan/UBSan, portes TSan, rejeu indépendant
et modèle Python indépendant du front q2, oracle de sûreté, dix mutants causaux
dont quatre non sûrs, 854 mesures closes. Face à la même fenêtre, n8k/16k/32k :
×0,86 à ×0,93 (uniforme), ×0,88 à ×0,96 (amas), ×0,89 à ×0,96 (terrain) à 2K,
×0,99 à ×1,03 sur les rangées. Le transport contre-audité est celui des
identifiants : le compte seul perd un support sur une fixture de cinq points.
« Redémarrer tous les parcours » (§ 2) a été instruit pour la descente du
proposeur : sa reprise exacte ne rend que ×0,94 à ×0,98 et n'est pas portée.
C'est encore une constante. Le front q2 n'a plus de levier mesuré au-dessus
de 10 % ; la suite est le port aux voies q3/q4 avec leur juge, les
certificats collectifs des rangées, puis q3/q4 et FULL.

Commencer par P0 : comparer les architectures de rejet qui évitent les
histogrammes quadratiques systématiques, d'abord en mono avec petits
juges indépendants. Préparer en soutien le contrat et la tranche FULL
minimale permettant de vérifier les sorties et de mesurer le coût aval.
Le port général et la parallélisation ne doivent pas figer l'ancien
passage A×A/B×B. Raccorder ensuite les améliorations amont et aval une
par une. Aucune réécriture géante sans différentiel, aucune accumulation
de variantes non intégrées, aucun claim industriel anticipé.
