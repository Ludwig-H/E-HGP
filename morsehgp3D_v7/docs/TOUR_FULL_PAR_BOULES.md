# Tour FULL par ancres de boule

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le nouveau `src/forest/full_ball_tower.hpp` raccorde les quotients locaux,
les parents globaux, le journal daté v2 et les cartes verticales. Il ne
remplace pas silencieusement la route F ni les anciennes sondes régulières.
Son autorité est **relative à un catalogue de census complet et exact fourni**.
Les tests bornés ci-dessous ne certifient pas toute la génération WSPD.

## Objet calculé

`build_full_ball_tower(index, balls, kmax)` conserve simultanément les
forêts des ordres 1 à min(kmax,n), avec kmax de 1 à 10. Chaque ordre porte
un `FullCoverageCertificate` et, pour chaque nœud hors K1, l'identité de
son image inférieure à son niveau de création fermé. Une lecture verticale
normalise cette image à la coupe demandée, pas à la racine finale.

Les minima réguliers de cardinal K sont les feuilles ; les véritables
multifusions ont leurs parents pré-lot. Les composantes ne sont jamais
identifiées par leur seule couverture de points. Hors régularité, les
naissances peuvent couvrir plus de K points et les continuations peuvent
ajouter une contribution datée sans créer de nœud. Les portails inertes
ne sont pas des nœuds exportés, mais conservent leurs ancres de calcul.
Le singleton et le terminal K=n sont distincts ; ni poids du manuscrit,
ni archive industrielle ou politique de coupe applicative ne sont ajoutés.

Le census utilise des indices géométriques internes ; la sortie utilise
les identités externes, y compris clairsemées. Une banque I/U immuable
est partagée par tous les ordres. Ses lignes ne sont créées que pour les
naissances et contributions, pas pour chaque connexion sans couverture.
Les masques ne sont pas des deltas globaux disjoints : leur union datée
reconstruit la couverture, comme dans le [contrat v2](CONTRAT_COUVERTURES_DATEES.md).

Le format de census actuel contient au plus neuf sites intérieurs et
douze sites de coquille ; un dépassement est refusé, jamais tronqué.
La borne intérieure découle de p+q_min≤11 avec q_min≥2 ; la borne de
coquille est une limite du format/local-helper, **pas un théorème limitant
les plateaux 3D**. Les plateaux plus grands restent à prendre en charge.
Les identifiants de candidats sont encore u32 et les census sont globaux :
le passage massif exige aussi streaming/partitionnement et reprise.

## Calendrier exact

Pour une boule de paramètres p, u, q_min, seules les ancres des ordres
p+q_min−1 à min(kmax,p+u) sont programmées. L'admission amont reste
p+q_min≤min(kmax+1,n), jamais p+u≤kmax+1 pour une extra-shell.
Les boules régulières ont seulement deux rangs possibles et au plus quatre
représentants ; aucune table exponentielle n'y est construite.
Les autres utilisent le [quotient local qualifié](PLATEAUX_FULL_ET_ANCRES.md).

Chaque représentant strict est résolu avant la fermeture de son lot.
Une MEB locale positive contenant les K sites suffit ; sa coquille
sélectionnée peut dépasser le support choisi. La recherche d'une ancre
fermée par BallKey précède la recherche spatiale d'un intrus strict.
Sur un échec d'ancre, échanger un site du support contre un intrus fait
décroître lexicographiquement le rayon puis le nombre de sites sélectionnés
sur la même coquille. Imposer une décroissance strictement en rayon est faux.
La fixture à huit sites de la gate exerce effectivement ce second cas.

Toutes les boules de même niveau sont regroupées par leurs racines
pré-lot communes, jamais par recouvrement de points. Les ancres nouvelles
sont installées seulement après cette fermeture. Une naissance d'ordre K
lit l'ancre de la même boule à l'ordre K−1 au niveau fermé. Pour une fusion,
toutes les images des parents doivent coïncider dans l'ordre inférieur.
Une contradiction invalide la tour entière ; aucune sortie partielle
n'est retournée comme une tour complète.

## Optimisations mono-thread qualifiées

La MEB libre essaie les supports q2/q3/q4 lexicographiquement et ne
matérialise que le premier support positif contenant tous les sites.
Pour dix sites, cela représente au plus 375 supports et 3 750 puissances,
en espace constant. Ce n'est pas une borne du nombre global de résolutions.

La validation d'un support régulier déclaré vérifie directement positivité,
clé et niveau, sans énumérer ses sous-supports. Les extra-shells gardent
leur calcul q_min indépendant. Les compteurs distinguent ces deux travaux.
Un lot d'une seule boule évite DSU locale, tableau de propriétaires,
groupes et cibles, sans omettre ses représentants ni son ancre inerte.

Pour les cartes verticales, un normaliseur temporaire active les arêtes
inférieures en ordre chronologique. Union par rang, compression et identité
historique séparée évitent les reparcours quadratiques. L'histoire livrée
n'est jamais compressée. Avec N nœuds inférieurs et Q demandes, ce travail
est O((N+Q)α(N)), mémoire supplémentaire de quatre tableaux u64 et un
tableau u8 par nœud. Chaque arête n'est activée qu'une fois.

| Peigne de fusions | Anciennes traversées répétées | Arêtes activées | Pas find |
| ---: | ---: | ---: | ---: |
| 256 | 32 896 | 512 | 1 022 |
| 512 | 131 328 | 1 024 | 2 046 |
| 1 024 | 524 800 | 2 048 | 4 094 |

Ce gain algorithmique local ne prouve pas le contrat de tour. Le nombre
de boules, résolutions et nœuds reste à mesurer ; la [borne de sortie](CROISSANCE_ET_BORNE_DE_SORTIE.md)
interdit une promesse FULL explicite sous-quadratique universelle en n.
Les brouillons de journaux sont encore retenus avant encodage : leur
résidence et les allocations par représentant restent des coûts à réduire.

Le [delta cache/résidence](OPTIMISATIONS_CACHE_ET_GPU_20260910.md) mémorise
les résolutions par clé entière, normalise tout token à la coupe pré-lot et
sème I union U seulement après fermeture à K=p+u. Collisions et allocation
impossible n'élident aucun travail requis. Il libère aussi les états morts
avant la banque finale et réserve les arènes exactes du journal v2.
Cela ne supprime pas encore les brouillons globaux du journal.

## Qualification et instrument

Le [paquet initial de preuves locales](../receipts/ball_tower_20260910/README.md)
conserve les sources dédupliquées, commandes, échecs et limites de provenance.
La gate indépendante Gram/Gamma juge 24 variantes, 100 ordres, 2 136
coupes ouvertes/fermées et 35 462 images verticales : 130 734 contrôles
identiques en O2 et ASan/UBSan avec fuites actives. Quatre mutants causaux
sont rejetés : croissance omise, ancre inerte omise, image future et
descente strictement en rayon. Le normaliseur temporel conserve ces
résultats et rejette aussi l'activation anticipée de toutes les fusions.
La MEB libre a sa qualification séparée : 605 cas dont 197 extra-shells,
300 permutations, trois mutants. Aucune de ces gates n'est le chemin produit.

Le [paquet courant](../receipts/ball_resolver_residence_20260910/README.md)
porte le moteur avec cache : 28 nuages, 112 ordres, 2 508 coupes,
45 948 verticales et 170 320 contrôles O2/SAN. Six mutants causaux sont
tués, notamment croissance et ancre inerte dans le chemin des lots groupés.
Le défaut `new(nothrow)` de l'injecteur ASan est corrigé dans le test,
avec conservation de son échec antérieur. Vingt CTests CPU passent sur
sources stables dans le [reçu d'échelle](../receipts/full_ball_scale_gpu_20260910/README.md).

`bench/full_ball_tower_probe.cpp` mesure entrée, index, génération WSPD,
tri/RLE, prefilter, census, tour retenue et empreinte du payload. Elle garde
tous les ordres et leurs verticales jusqu'à la sortie. Son digest dense est
déterministe, pas un oracle géométrique canonique ni un format d'archive.
Les anciennes durées de sondes horizontales libérant chaque ordre ne sont
donc pas des comparaisons appariées de cet instrument.

`bench/full_ball_tower_probe.cu` utilise la nouvelle route CUDA uniquement
pour prefilter/census. Le contexte froid, ses allocations, transferts et
libérations sont inclus dans le temps. WSPD, tri et constructeur FULL
restent CPU. La taille du lot device borne une résidence, pas le nombre
de points ni le nombre total de candidats.
La gate de cette route passe 16 627 contrôles dans le stub hôte et sur le
vrai device G4 SM12.0, dont 4 116 boules et 17 rejets. Le build strict NVCC
utilise l'adaptateur de phases qualifié séparément. Les [tours 50k](RESULTATS_TOUR_CACHE_G4_20260910.md)
coïncident CPU/hybride : environ 419 s K1..10 et 33,6 s K1..5. Les deux
générations SPOT utilisées sont arrêtées et certifiées `TERMINATED`.

Les contrats 50k/1s, 100ms et plusieurs dizaines de millions de points
ne sont pas acquis. Le coût FULL dominant motive d'abord la
[séparation statique prouvée sous census complet](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md)
des MEB et du calcul temporel des parents ; cette variante n'est pas encore
le moteur produit. Un autre port GPU utile vise les requêtes de témoins
universels par rectangle WSPD, dont le reçu historique 50k compte près de
cinq milliards de visites d'index. Un kernel de census dans une boule fixe
ne remplace pas ce prédicat universel sur A×B ; son port doit conserver
exclusions A∪B, masques de lanes, seuils stricts et grand-livre des paires.
