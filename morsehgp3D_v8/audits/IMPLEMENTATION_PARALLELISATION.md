# Audit d'implémentation et de parallélisation de la v7

13 septembre 2026. Contrelecture d'architecture pour l'ouverture v8, sans
compilation, benchmark ni accès GCP. Cadre v8 : `backend=none`,
`phase=exploration_v8_hors_registre`, `profile=quantized_u16_input_only`,
`mode=audit_v7_math_and_architecture`, `public_status=not_claimed`.

Base publiée examinée : `dc57ffd5fec5b73aff9bc7f79f280fb8bc92a6d1`.
Les fichiers d'entrée v7 localement modifiés et la préparation du parcours
fusionné sont distingués des sources publiées. Les numéros de ligne
ci-dessous concernent les fichiers effectivement lus, pas une version
future. Cette revue suit les chemins de calcul, leurs interfaces, leurs
allocations et leurs preuves ; elle n'est pas une nouvelle certification
exécutable de chaque ligne de la v7.

## 1. Le diagnostic principal

La v7 ne passe pas son temps à faire seulement un tri puis Kruskal.
Elle paie trois problèmes très différents :

1. Trouver et certifier les bonnes boules, sans énumérer toutes les paires,
   tous les triangles et tous les tétraèdres.
2. Trouver à quelle composante antérieure chaque facette utile se rattache.
   Le chemin FULL actuel répète pour cela des calculs de plus petite boule
   et des recherches spatiales.
3. Construire, numéroter et conserver toutes les histoires, contributions
   et images entre ordres, avec beaucoup de données intermédiaires.

Les GPU ont effectivement été utilisés, mais **le constructeur FULL mesuré
à 50k restait sur CPU**. Il ne s'agit donc pas d'une démonstration que FULL
ne serait pas parallélisable : c'est surtout un raccord GPU incomplet.
La reformulation en graphes datés offre une séparation mathématique plus
adaptée, mais son implémentation demeure un prototype CPU privé.

À 50k/K1..10, la sonde hybride du 10 septembre a payé 418,921 s, dont
390,481 s dans le constructeur FULL, avec 41 986 201 calculs MEB et
3 898 856 828 supports exacts testés. Les kernels de census totalisaient
189,346 ms seulement. Cette différence de périmètre explique l'essentiel
du paradoxe apparent. [Mesure v7, lignes 19–37](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md)

## 2. Il existe plusieurs « v7 », pas un seul moteur raccordé

| Chemin | Ce qui est réellement disponible | Ce qu'il ne faut pas lui attribuer |
| --- | --- | --- |
| Exécutable `mhgp7` | Pipeline historique réduit, options d'incidences silencieuses, archive transactionnelle F | Le payload FULL de la sonde ; des verticales FULL ; un succès de `--require-exact` |
| Sonde `mhgp7_full_ball_tower_probe` | Vrai générateur/census puis Builder FULL, dix ordres et verticales retenus | Une archive industrielle ni une qualification universelle du générateur |
| Option `--static-threads` de cette sonde | Développement, tri-unique et résolution parallèle des facettes ; restitution au calendrier CPU | Un constructeur d'histoires parallèle ; un backend GPU automatiquement sélectionné |
| Callback par lots du Builder | Interface transactionnelle de résolution statique, tests CPU1/4 permanents | L'exécution device du terminal composé |
| Route hybride CUDA de la sonde | Préfiltre et census sur carte ; résultats remontés au CPU | La génération WSPD, les résolutions FULL, les fusions et les verticales sur GPU |
| Atlas, graphes, streaming et chaînes historiques privés | Raccords CPU qualifiés séparément, objets FULL comparés sur des corpus bornés | Leur présence dans le binaire produit ou dans les cibles CMake actives |
| Parcours fusionné des marques en préparation locale | Qualification propre documentée, sources privées | Un commit publié, une intégration active ni un nouveau temps 50k |

Points de contrôle :

- [CLI](../../morsehgp3D_v7/cli/mhgp7.cpp), lignes 109–112 :
  `--require-exact` refuse ; lignes 215–220 : l'archive utilise `ForestResult`
  puis le pilote appelle `run_pipeline`.
- [CMake](../../morsehgp3D_v7/CMakeLists.txt), lignes 50, 265–273 et
  1142–1144 : produit, sonde CPU et sonde hybride sont trois cibles distinctes.
- [Sonde FULL](../../morsehgp3D_v7/bench/full_ball_tower_probe.cpp), lignes
  102–128 : seule la branche préfiltre/census dépend de CUDA ; l'appel FULL
  reste `build_full_ball_tower`.
- [Reçu actif du callback](../../morsehgp3D_v7/receipts/full_ball_batch_active_cmake_20260911/README.md) :
  Builder `83f1c78e…`, 40 CTests sélectionnés, pas la suite complète de
  449 tests dénombrée dans cette capture historique.
- [Reçu des graphes FULL](../../morsehgp3D_v7/receipts/atlas_graph_full_20260911/README.md)
  et [parent streaming publié](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/README.md) :
  sources privées conservées dans les paquets, pas sous `src/` actif.

Conséquence v8 : un seul chemin de référence bout en bout doit être nommé
explicitement. Les autres deviennent des différentiels ou des backends
interchangeables de ce chemin, avec un statut vérifiable. Copier tous les
chemins historiques de la v7 reproduirait sa difficulté de lecture.

## 3. Amont : déjà parallèle, mais pas assez fin ni assez compact

### 3.1 La tâche « un rectangle » existe déjà

Le [générateur actif](../../morsehgp3D_v7/src/pipeline/generate.hpp) traite
les rectangles par `parallel_items` à la ligne 1289. L'intuition utilisateur
est donc juste sur l'indépendance de nombreux travaux, et déjà partiellement
présente dans le code. Le problème est le contenu de cette tâche :

- Les trois lanes sont parcourues à la suite, lignes 1297–1300.
- Les histogrammes d'extrémités utilisent les doubles boucles internes aux
  facteurs, lignes 491–510 : travail en taille de A au carré plus taille de
  B au carré. Ce helper calcule des comptes complets ; il ne s'arrête pas
  dès le nombre de succès nécessaire atteint.
- Les survivants sont parcourus par les boucles imbriquées sur a et b,
  lignes 1332–1338. Un gros rectangle n'est pas subdivisé entre workers
  dans ce corps.
- q3/q4 développent ensuite les covers, seeds et balayages à l'intérieur
  de cette même tâche, lignes 1378–1388. Une ancre coûteuse peut donc
  monopoliser un worker après l'épuisement des autres rectangles.

Ce n'est pas un verrou mathématique. Il faut pouvoir distribuer aussi les
grands histogrammes, les tuiles de paires survivantes et les seeds, sans
matérialiser le produit A×B pour obtenir ces tâches. La certification des
témoins, l'addition de crédits disjoints et les limites de la saturation
sont étudiées dans l'audit WSPD séparé ; leur correction ne doit pas être
remplacée par une simple limite du nombre de positions examinées.

### 3.2 Le front fonctionne encore par vagues et copies

Toujours dans `generate.hpp`, lignes 349–463 : chaque vague possède ses
shards temporaires ; une équipe est lancée puis jointe ; les shards sont
concaténés séquentiellement ; la vague suivante commence ensuite. Tous les
rectangles terminaux sont conservés dans `alive` avant le début du corps,
lignes 1253–1269.

[Le pool CPU](../../morsehgp3D_v7/src/parallel/pool.hpp), lignes 55–86,
crée effectivement de nouveaux `std::thread` pour chaque appel parallèle.
Son verrou n'est pas un verrou de données géométriques dans le chemin
nominal : la mutex des lignes 89–104 sert à recueillir les exceptions.
Il serait faux d'expliquer toutes les secondes par une grosse serrure
commune. Les coûts observables sont plutôt créations/jonctions, barrières
de phase, tâches déséquilibrées, allocations et copies.

Les émissions finales passent encore de vecteurs par worker au vecteur
global, lignes 1445–1453. La réserve évite certaines réallocations mais
pas la copie de tous les candidats. Le commentaire distingue correctement
tailles logiques, capacités et pic RAM ; la v8 doit conserver cette rigueur.

Proposition v8 : décision par rectangle, compte des enfants et terminaux,
sommes préfixes, écriture directe aux offsets obtenus ; équipe persistante
CPU, tableaux plats et lots résidents GPU. Cette proposition n'est pas un
débit mesuré. Le front de témoins optionnel v7 fournit un différentiel
utile, mais le générateur nominal ne l'appelle pas encore.

### 3.3 q3/q4 ne se résument pas au même rejet q2 répété

Leur indépendance peut être exploitée, mais leur travail interne est plus
riche. Pour q4, le code trie des racines rationnelles par seed puis traite
les groupes égaux, lignes 1100–1178 de `generate.hpp`. Les entrées et sorties
au point exact du groupe doivent être exclues du compte intérieur. Une
somme préfixe/suffixe segmentée est une unité de calcul parallèle naturelle,
pas une suppression des témoins constants ni un arrêt définitif après un
groupe profond. Il faut mesurer séparément visits, seeds, racines et
candidats acceptés : une faible sortie peut masquer un gros scan rejeté.

## 4. Tri, Kruskal et arbre de multifusions : trois calculs

Le tri donne un ordre. Kruskal choisit les arêtes qui relient des composantes.
La reconstruction fabrique ensuite les événements de naissance et les
vraies multifusions, avec leurs parents au bon niveau. Ils ne sont pas
interchangeables.

Le [tri parallèle v7](../../morsehgp3D_v7/src/parallel/sort.hpp) trie déjà
des indices pour éviter un second gros tableau de candidats. Il utilise
des fusions parallèles, mais l'application de la permutation est un parcours
séquentiel de cycles, explicitement décrit lignes 28–31 et 93–101. Ce
compromis réduit la mémoire ; il ne représente pas le meilleur débit GPU.

Le tri de dates exactes n'est pas initialement un tri de clés u32. Les
[niveaux](../../morsehgp3D_v7/src/lanes/level.hpp), lignes 36–58, ont un
numérateur 192 bits et un dénominateur 128 bits ; leur comparaison utilise
des produits croisés 320 bits. Il est pertinent de les classer une seule
fois puis d'employer des rangs entiers. C'est précisément la fonction
de l'Atlas privé : il ne remplace pas cette première certification.

Même un tri entier très rapide n'est pas « gratuit » : lire puis écrire
100 millions d'enregistrements de 16 octets représente déjà 3,2 milliards
d'octets transférés, avant les passes supplémentaires, le scratch et les
autres phases. Ce calcul de volume n'est ni une estimation de débit G4,
ni une preuve d'impossibilité du contrat 1 s.

L'approche graphe daté permet de choisir une forêt couvrante minimale puis
de reconstruire les multifusions par contraction parallèle. Mais il faut
regrouper les fusions de même niveau géométrique et ne pas réunir des
composantes disjointes simplement parce que leurs dates sont égales.
Une forêt arbitraire, un MST sur les points pour K≥2, ou un arbre binaire
avec égalités départagées définitivement ne sont pas le résultat demandé.
Les pistes RCTT/PANDORA doivent donc être jugées sur l'objet HGP propre,
pas promues depuis les temps de leurs auteurs.

## 5. Le constructeur FULL actif impose des dépendances évitables

Le [Builder](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp), hash
`83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`,
enchaîne les ordres aux lignes 306–348 puis les plateaux aux lignes 326–336.
Les ancres d'un lot sont installées seulement après le calcul de tous ses
parents. Cette règle est correcte ; la réaliser par un calendrier global
mutable n'est pas la seule architecture correcte.

La voie statique optionnelle constitue un progrès réel : lignes 751–839,
elle développe les demandes, trie les facettes entières, forme des classes,
résout une fois par clé unique et disperse les réponses dans l'ordre
d'origine. Le resolver ne consulte pas les composantes temporelles :
sa séparation est explicite lignes 574–627. C'est une base beaucoup plus
adaptée au GPU que de faire appeler le calendrier par chaque thread device.

Ses coûts restants sont toutefois importants :

- Le développement puis le tri de toutes les demandes d'un ordre et le
  tableau `static_targets` sont globaux à cet ordre, lignes 753–789.
- Le chemin nominal reste le cache temporel si `static_threads=0` ;
  sélectionner beaucoup de threads amont ne sélectionne pas l'option.
- Une descente répète MEB, recherche exacte de clé, recherche d'intrus,
  échange et MEB suivante, lignes 575–627. Le nombre d'étapes varie.
- Les lots, brouillons, populations et historiques sont reconstruits
  dans des structures intermédiaires avant l'encodage final, lignes 303,
  331–335 et 363–379. Les allocations par petit objet demeurent nombreuses.

Le [MEB exact actif](../../morsehgp3D_v7/src/forest/anchor_meb.hpp), lignes
118–170, essaie les paires, puis triples, puis quadruples dans l'ordre
lexicographique, et teste leur confinement de la facette. Pour dix sites,
l'espace maximal des supports candidats contient 45+120+210=375 éléments.
Le premier bon support arrête la recherche, mais répéter ce petit problème
des dizaines de millions de fois explique des milliards d'essais.
« Support de taille au plus quatre » ne signifie donc pas « quatre opérations ».

Un proposeur rapide suivi de certification exacte est une piste légitime :
confinement de tous les sites, support positif, coquille complète et repli
exact si nécessaire. La certification finale ne coûte pas le même prix
qu'une recherche lexicographique de tous les supports. Aucun gain n'est
garanti avant mesures, et les contre-exemples des anciens proposeurs restent
des fixtures obligatoires.

## 6. Le prototype graphe débloque l'architecture, pas encore le débit

Le [paquet publié](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/README.md)
utilise des fenêtres de demandes, une équipe géométrique persistante et une
DSU sur les naissances. Il évite le graphe complet et le grand tableau de
toutes les terminales. Mais il remet les résultats dans l'ordre source puis
les consomme séquentiellement. C'est un bon moteur CPU de référence pour le
nouvel objet, pas encore un algorithme de contraction massive.

Autres répétitions identifiées dans ses sources épinglées :

- La sonde valide d'abord le catalogue via Builder, puis l'Atlas retrie clés
  et niveaux ; le streaming retrie encore les clés pour son resolver.
  Dans les sources logiques du paquet : `bench.cpp` lignes 753–754,
  `rank_atlas.hpp` lignes 162–163, `streaming_graph.hpp` lignes 170–172.
- L'export prépare une banque de populations puis la fait copier dans le
  propriétaire immuable. Il reconstruit aussi des fragments et brouillons
  avant le certificat final.
- L'index de chaînes historiques est construit séquentiellement. Seules
  ses requêtes sont parallèles. Pour la tour 1..10, l'export construit un
  index courant pour chaque ordre puis reconstruit l'index inférieur :
  19 préparations au lieu de 10 si les voisins étaient réutilisés.
- L'export ne consomme pas les marques déjà reconstruites : il réémet des
  requêtes contributives. Le parcours fusionné local supprime un second
  DSU, mais pas ce travail aval.

Traçabilité de ces derniers constats : sources logiques
`build/v7_graph_full_20260911/graph_full.hpp`, hash `bad5051f…`, lignes
95–126, 135–139, 181–195, 207–209 et 250–284 ;
`build/v7_atlas_graph_20260911/historical_chains.hpp`, hash `686c2137…`,
lignes 21–74 et 111–131. Elles sont publiées dans le
[manifeste reproductible](../../morsehgp3D_v7/receipts/atlas_graph_full_20260911/MANIFEST.json),
pas dépendantes de la conservation d'un `build/` local.

Une préparation immuable partagée doit donc fournir géométrie validée,
permutations, rangs et masques. Les histoires achevées rendent ensuite les
verticales indépendantes les unes des autres. Réutiliser une marque exige
toutefois de certifier son appartenance à la bonne histoire : une valeur
bien formée n'est pas une preuve que son segment est la composante visée.

## 7. GPU : matrice de maturité

| Composant | Test hôte | Compilation CUDA | Exécution device attestée | Tour/contrat |
| --- | --- | --- | --- | --- |
| Préfiltre/census intégré à la sonde hybride | Oui | Oui | Oui, G4 SM120 | Tour hybride 50k terminée ; contrat échoué |
| Sélection MEB primitive | Oui | Oui | Oui, 605 cas | Primitive, pas débit de tour |
| Clé primitive/PGCD/division 128 bits | Oui | Oui | Oui, 13 573 cas | Primitive, pas resolver complet |
| Terminal composé K1..10 et callback résident de la sonde privée | Oui | Oui, strict | Non dans les reçus examinés | Pas de nouveau chrono GPU |
| Génération WSPD/seeds/sweep q4 de la tour | CPU | Briques séparées possibles | Pas de route complète qualifiée ici | Verrou ouvert |
| Forêt minimale, reconstruction, contributions/verticales GPU | Référence CPU privée | Pas de backend complet livré | Non | Verrou ouvert |

Sources : [deux primitives exécutées](../../morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md),
[sonde complète par lots](../../morsehgp3D_v7/receipts/terminal_batch_probe_20260911/README.md),
[trois tentatives G4 sans calcul](../../morsehgp3D_v7/receipts/terminal_batch_g4_20260911/README.md).
Les préemptions et outils invités manquants expliquent l'absence du dernier
test device ; ils ne prouvent ni un échec mathématique ni une accélération.

Le [transport census](../../morsehgp3D_v7/src/gpu/census_route.cuh) est
synchrone : empaquetage CPU, copies des sentinelles et entrées, deux kernels,
`cudaDeviceSynchronize`, readbacks, validation/reconstruction CPU de toutes
les lignes, puis lot suivant ; voir lignes 255–336. Il lance le census sur
le lot avant toute compaction host des survivants. À 50k/K10, la route entière
coûte 4,540 s pour 0,189 s de kernels, dont 2,932 s de reconstruction hôte.
Cette disproportion appelle des destinations fixes, de la compaction
device quand utile et des objets résidents ; elle ne se répare pas en
augmentant le nombre de threads du kernel seul.

Ne pas supprimer les statuts/contrôles de transport pour obtenir un temps :
faire produire leurs preuves compactes et réduire les erreurs en parallèle.
De même, la pile bornée par les bits Morton est une borne démontrée de
représentation, pas un quota arbitraire d'itérations à retirer.

## 8. Mémoire, profils et industrialisation

| Sujet | Acquis v7 | Lacune v8 à traiter explicitement |
| --- | --- | --- |
| Identité des points | PointId distinct de l'ordre Morton | Pas de confusion avec facettes, boules, blocs ou nœuds |
| Exactitude numérique | Profil u16, décisions entières et arithmétique large | Pas d'extension automatique aux flottants ou pondérés |
| Census | Tableaux inline, scratch de parcours réutilisé, écriture CPU à ordinal fixe | Catalogue global et payloads lourds ; externalisation/résidence GPU non qualifiées |
| Identifiants de sortie | FullNodeId u64 | BallId u32, positions i32 et divers offsets/permutations u32 restent bornés |
| Coquilles | Extra-shells pris en charge sur domaine admis | Coquille active limitée à 12, masque de contribution u16 ; pas « tout nuage u16 » sans refus |
| Échecs mémoire | Refus vides, compteurs contrôlés, tests de pannes | Budgets partiels de buffers, pas garantie globale de RAM/VRAM |
| Publication | Archive F create-only et succès terminal | Pas d'archive FULL intégrée, pas de reprise de calcul |
| Échelle | Diagnostics 8k/16k/32k et tours 50k | Aucun contrat plusieurs dizaines de millions ; formats et simultanéité à mesurer |

Références de code : [types](../../morsehgp3D_v7/src/core/types.hpp), ligne 30 ;
[Builder](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp), lignes 67,
109 et 444–446 ; [census](../../morsehgp3D_v7/src/pipeline/expand.hpp),
lignes 46–57 et 190–221 ;
[journal](../../morsehgp3D_v7/src/forest/full_coverage_certificate.hpp),
lignes 55–69 et 83–100 ; [caps](../../morsehgp3D_v7/src/core/caps.hpp),
lignes 13–27 et 34–50.

Les 32 bits d'un PointId peuvent couvrir des dizaines de millions de points ;
ils ne suffisent pas forcément à couvrir toutes les boules ou incidences
produites. À l'inverse, passer ces champs en 64 bits ne réduit pas un seul
octet des grands tableaux et ne résout pas leur résidence.

[L'archive](../../morsehgp3D_v7/src/io/archive.hpp) annonce dès les lignes
1–3 qu'elle n'assure ni checkpoint/reprise ni garantie de coupure électrique.
Son manifeste, ligne 303, dit `vertical_maps=none`. Ne pas la présenter comme
le format industriel FULL. La v8 doit définir fichiers/lots immuables,
offsets globaux, horizon de succès et journal de reprise, avant de lancer
la cible multi-millions en supposant que la RAM suffira.

Les références héritées `docs/ECHELLE.md` dans certains commentaires source
ne sont plus des fichiers navigables de la v7. Les dénominations « v6 » en
tête de plusieurs headers sont des traces du port, pas une version fiable
du comportement courant. Une petite carte d'architecture unique est plus
utile que d'accumuler des annonces successives dispersées.

## 9. Preuves et tests : solides localement, pas un certificat global

Points positifs à conserver :

- Oracles Gram/Gamma séparés des producteurs et bornés à de petites tailles.
- Comparaisons physiques des parents, successeurs, dates, contributions et
  verticales ; les digests ne constituent pas le seul juge.
- Mutants ciblés sur strict/fermé, arité, identité propriétaire, ordre source,
  corruption de transport et publication partielle.
- Tests de panne d'allocation/création de threads, invariants et codes exacts.
- Sources, commandes, compilateurs, dépendances et résultats rattachés à
  chaque reçu ; conservation des échecs et absence de transfert implicite.

La v7 possède désormais une
[CI CPU dédiée](../../.github/workflows/morsehgp3d-v7.yml), lignes 44–58 :
construction CMake v7 puis CTest filtré par label `gate`. Dire « aucune CI v7 »
serait faux. Ce fichier ne prouve pas que son dernier run distant a réussi ;
aucun run GitHub n'a été consulté dans cette contrelecture. Il ne comprend
pas de mesure GPU ou de qualification 50k. CUDA est désactivé en CI et le
[CMake GPU](../../morsehgp3D_v7/CMakeLists.txt), lignes 1099–1119, est borné
à SM120 ; les autres GPU ne sont pas qualifiés par cette configuration.

Lacunes de preuve restantes :

- Un ledger WSPD fermé compte les paires émises/tuées ; sa clôture seule ne
  prouve pas que toutes les décisions de mort étaient géométriquement justes.
- Un catalogue validé contient de bonnes entrées ; cela ne prouve pas qu'il
  contient toutes les entrées nécessaires. Les census→FULL bornés réfutent
  des omissions, pas toutes les omissions possibles.
- Le lecteur structurel du journal dit précisément `structural_only`.
  Une forêt cohérente peut avoir les mauvais parents ou une mauvaise origine.
- Les macros de tests, les builds O2/SAN, les callbacks hôte et les kernels
  réels doivent garder des résultats séparés, même si le même `.cu` est utilisé.
- Les comparaisons sur hôte partagé, souvent uniques, ne qualifient pas un
  speedup reproductible ; la préparation du parcours fusionné ne fournit
  aucun nouveau résultat TSan. ASan/UBSan ne démontrent pas l'absence de race.
- Le coût p95, les familles variées et le succès de bout en bout du contrat
  50k ne sont pas validés par les tests fonctionnels ni par une micro-gate.

Exemples de juges antérieurs incomplets déjà corrigés, à garder en v8 :
parent→0 échappait au lecteur fondé sur les successeurs ; une mauvaise
terminale pouvait aboutir à la bonne composante finale ; les marques ne
sont pas vérifiées par l'export qui les ignore. Ces leçons sont détaillées
dans [les décisions écartées v7](../../morsehgp3D_v7/docs/FAUSSES_PISTES.md).

## 10. Priorités de refonte et preuve minimale attendue

| Priorité | Décision v8 proposée | Preuve/mesure avant promotion |
| --- | --- | --- |
| P0 : objet et API | Un seul contrat FULL daté, avec identités et verticales ; séparer géométrie et structure | Round-trip complet, minima isolés, E5, coquilles, K=n, coupes ouvertes/fermées, populations recouvrantes |
| P0 : travail géométrique | Facettes entières uniques, resolver statique, semis certifiés et proposition MEB rapide avec repli | Bonne terminale exacte avant normalisation, travail total/supports/intrus payé, aucun quota de fin prématurée |
| P0 : tâches amont | Front plat, histogrammes adaptatifs, tuiles de paires/seeds, scans segmentés q4 | Ledger + oracle géométrique, gros facteurs adverses, suivi du travail rejeté, s8/10/12 |
| P0 : histoire | Graphes datés indépendants par K, forêt minimale puis contraction | Toutes les coupes, plateaux divisés entre lots, peignes, égalités disjointes et multifusions de grande arité |
| P1 : préparation | Un propriétaire immuable pour catalogue/rangs/masques/permutations | Refus des vues d'un autre propriétaire, égalités rationnelles et ordre de shell explicitement transportés |
| P1 : aval | Marques réutilisées, index adjacents réutilisés, numérotation et export par préfixes | Marque forgée vers autre composante vivante, dates d'admission, verticales/naturalité, compte des préparations |
| P1 : backends | Même plan de travail CPU persistant puis GPU résident | Sorties identiques, activité réelle de chaque backend, transferts froids et reconstruction inclus |
| P1 : mémoire | Tableaux plats, références de populations, buffers bornés en résidence, offsets 64 bits | Pic simultané mesuré, allocation échouée, reprise après interruption, aucune amplification cachée par lot |
| P2 : performance | Une sonde unique du coût complet et du débit de chaque phase | 8k/16k/32k sur plusieurs régimes puis porte 50k répétée ; ne pas faire payer le diagnostic au produit sans le nommer |

La priorité n'est pas de supprimer les vérifications pour gagner le facteur
manquant. Elle est de définir des objets compacts dont la certification est
effectuée une fois, dont les nombreuses requêtes sont indépendantes et dont
les résultats s'écrivent directement à leur destination. Le tri devient
alors une brique bien délimitée, et non le nom donné à toute la construction.

Aucun code moteur v8 produit par cet audit. GCP non utilisé.
