# Contre-audit natif de la v11 pour la v12 — 7 octobre 2026

Instantané lu : `33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae`, dernier moteur annoncé `ac081a06f`.
Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
GCP non utilisé ; aucune compilation propre à cet auditeur ; les sondes exécutées utilisent la construction
Release GCC u21 unique du coordinateur, `/tmp/ehgp-v11-audit-build-20261007`.

Périmètre : noyau géométrique `num`, catalogue CPU et source commune hôte/appareil, index,
MEB/descentes/cellules/forêts/verticales, Pool ; changements natifs postérieurs à `28d70f8ab`.
Le cache de `core`, l'API et les écrivains sont relus séparément par le coordinateur.
Ce rapport ne remplace ni la qualification native complète ni la matrice u18/u21/u24, sanitizer, CUDA.

## 1. Verdict et constats classés

Aucune sortie FULL géométriquement fausse n'a été établie dans ce périmètre. La lecture des voies entières
étroites est favorable sous leurs préconditions effectives. Les nouvelles écritures concurrentes O2/cohortes
paraissent correctement séparées, avec les réserves de qualification précisées ci-dessous.

| ID | Gravité | Constat | Nature de la preuve |
|---|---|---|---|
| N1 | P2, capacité/performance | Le nouveau tri parallèle réserve `W × plus_longue_cohorte` enregistrements, y compris avec une seule cohorte et une seule tâche utile ; refus mémoire induits par W | Source et témoin natif reproduit |
| N2 | P2, contrat d'architecture GPU | Le contexte CUDA est un singleton mutable de processus ; le pool CUDA par défaut est modifié et conservé, hors propriété de `Session` | Source, pas d'exécution CUDA |
| N3 | P3, documentation | `GlobalIndex` est annoncé équilibré dans l'en-tête et la provenance ; sa construction actuelle est radix Morton | Source et contre-exemple natif |
| N4 | P3, contre-audit du diagnostic | L'audit géant décrit une énumération de toutes les boules q2 ; le moteur ne présente qu'une boule de diamètre, après le balayage des distances | Source, compteur `diameter_pairs` distinct |

Une matrice verte sur l'ancien pin ne ferme aucun de ces points. N1 n'est pas une fausse tour : le budget
refuse l'opération entière. N2 concerne la voie de banc CUDA ; l'API publique reste CPU.

### N1 — une cohorte indivisible fait néanmoins réserver W tampons maximaux

`src/tower/forest_build.cpp:441` prend `workers = pool.size()`, puis `:443` le maximum de la plus longue
cohorte sur tous les ordres, et `:447–452` admet/alloue leur produit. La tâche utilise sa tranche par worker
en `:397–400`, même si presque toutes les tâches sont vides.

Témoin analytique : `n` points `(i,0,0)`, `i=0..n−1`, à K2. Chaque paire adjacente a une boule vide de
rayon 1/2, donc les `n−1` naissances de l'ordre 2 constituent une unique cohorte. Toutes les bornes
strictement intérieures de `cohort_bounds` sont avancées à sa fin (`:337–348`). La première tranche
non vide trie toute la cohorte ; aucune autre tâche de tri ne peut l'aider. Le scratch est pourtant
`W(n−1)sizeof(BirthRecord)` au lieu de `(n−1)sizeof(BirthRecord)`. La propriété géométrique du témoin ne
dépend ni de Morton, ni du partitionnement du catalogue, ni de la date d'un benchmark.

Ce risque est nouveau dans `5734ca6e8` et distinct de la séparation des parents DSU O2 (`13a4a0a4c`).
Il importe aussi sur des grilles régulières, des plateaux très dégénérés, et pour W élevé. Le problème n'est
pas un débordement : les produits sont suffisamment larges et l'admission est faite. Il s'agit d'une capacité
nécessaire inutilement multipliée, sans contrepartie de parallélisme.

**Reproduction native close.** `cohort_memory_witness.py` génère 512 points `(i,0,0)`, K2, feuilles de 8,
maximum 32. Même construction Release u21, cache de blocs désactivé, mode ordres concurrents ; seule
la largeur du Pool et/ou le lookup dense varient. Les trois succès à budget large rendent la même
empreinte FULL. Les compteurs confirment 511 naissances à l'ordre 2.

| Voie | Pic à budget large | Issue avec budget 681 054 octets |
|---|---:|---|
| W1, lookup dense | 394 894 | succès |
| W8, lookup dense | 967 214 | `resource_exhausted / memory_budget`, code 2 |
| W8, lookup sparse | 359 342 | succès |

L'écart W8−W1 vaut exactement `7 × 511 × 160 = 572 320` octets, soit les sept tampons inutiles
supplémentaires. `COHORT_MEMORY_RELEASE_U21.json` garde les six commandes, événements complets,
empreintes du binaire, de l'entrée synthétique et des dumps. Le script régénère les entrées dans un dossier
temporaire supprimé au retour. Ces mesures sont des comptes mémoire du moteur, pas des chronos de performance.

Pour la v12 : rendre explicite une allocation par tâche utile, ou limiter les workers logiques du tri avec
des bornes propres aux cohortes. Une simple substitution `min(W, nombre_tranches)` ne règle pas le cas d'une
seule cohorte ; il faut compter les tranches non vides. Garder un témoin de budget serré avec W1/W8/W48,
une cohorte géante, plusieurs cohortes de tailles très différentes, et mêmes octets de forêt sur tous les
succès. Aucun mécanisme nouveau proposé ici n'est qualifié.

### N2 — propriété CUDA différente du contrat CPU

`src/catalogue/leaf_batch_cuda_context.cu:33–35` porte un `static Prefetch state`, avec mutex, thread,
booléen et compteur modifiables. `:42–57` prend le pool par défaut du périphérique, lui fixe un seuil de
libération maximal et remet ses pics à zéro ; `:62–66` lit ces pics globaux. Cela déroge au §1.3 de
`docs/ARCHITECTURE.md`, « aucun état global modifiable », et au modèle d'un contexte détenu par la Session.

La synchronisation du `Prefetch` ne présente pas de course évidente : l'accès à `thread`/`started` est sous
mutex, l'écriture de `ns` précède le retour de `join`. Le problème établi est le périmètre de propriété,
pas une course prouvée. Deux appels utilisant le même pool CUDA partagent la politique de rétention et les
compteurs de pics : ces compteurs ne sont plus attribuables à un lot isolé sans contrat d'exclusivité.
La compilation CPU seule ne peut pas qualifier ce comportement.

Pour la v12 : choisir et déclarer le propriétaire du device, du contexte et du pool, sa durée de vie et le
droit d'exécuter plusieurs appels. Si l'on garde le singleton du banc, l'annoncer comme tel et ne pas lui
transférer automatiquement les garanties mémoire/concurrence de l'API CPU. Tester fermeture, refus et
appels simultanés sur le vrai backend avant adoption.

### N3 — l'index radix n'est pas un arbre équilibré en cardinalité

`src/index/index.hpp:1` et `docs/PROVENANCE.md:130` parlent d'arbre équilibré. Or
`src/index/build.cpp:22–37` coupe au plus haut bit de Morton différent, et `:40–48` justifie une
profondeur de `kMortonBits+1`. Ce n'est plus la profondeur d'un découpage médian.

Exemple : sites sur x aux abscisses `0,1,2,4,8,...,2^20` en u21, y=z=0, feuilles de taille 1.
Chaque bit supérieur sépare un singleton du préfixe restant ; profondeur 22 pour 22 sites, contre
une hauteur logarithmique attendue d'un arbre médian. La récursion reste bornée par 64 en u21, 73 en u24 ;
aucun dépassement de pile ou défaut de census n'en est déduit.

`INDEX_RADIX_RELEASE_U21.json` reproduit le cas sur les trois axes avec `mhgp11_index_probe` : trois
succès, 22 sites, 43 nœuds, profondeur 22, coquille du singleton origine correcte. Le reçu conserve
l'entrée texte intégrale et l'empreinte du binaire partagé.

Porter le vrai invariant radix et ses coûts, pas l'ancien mot « équilibré ». Tester une échelle dyadique,
des axes permutés, la borne u24 à 72 bits Morton, et la couverture exacte de chaque plage/escape.

### N4 — correction limitée mais nécessaire du récit des MEB

`docs/AUDIT_GEANT_V11.md:950–952` dit « toutes les paires, puis tous les triplets, puis tous les
quadruplets ». `src/tower/meb.cpp:54–69` calcule toutes les distances de paires, mais `:162–172`
ne présente que la paire de diamètre lexicographiquement première ; q3 puis q4 restent exhaustifs
jusqu'au premier support strict contenant la partie. Ne pas confondre `diameter_pairs` et
`presentations` (`src/tower/meb.hpp:14–18`).

La critique principale reste valable : à 12 sites, jusqu'à `1+C(12,3)+C(12,4)=716` présentations,
plus 66 distances auxiliaires, sont possibles. Une proposition de MEB suivie d'un certificat peut
réduire les présentations ; la mesure du gain doit payer proposition, certificat, repli et canonicalisation.
La stabilité des niveaux exacts ne suffit pas : le support canonique et les sorties doivent aussi être
comparés. Ne pas présenter le port de Welzl comme déjà exécuté ou qualifié en v11.

## 2. Noyau numérique : ce que la lecture confirme

La séparation entre données certifiées et tuples arbitraires est une des bases solides à préserver.
`Point::make` borne les coordonnées ; `Sphere`, `Q3Candidate` et `Q4Candidate` construisent des centres
par fabriques, puis exposent des vues constantes. `predicates.cpp:13–46` n'accepte pas un concept C++
forgeable de « centre » : `CenterView` a précisément les trois propriétaires autorisés.

Les types découlent de `Budgets<B>` : coordonnées B∈{18,21,24}, différences <2^B, centres q3 au degré 5,
puissance au degré 6, orientation d'un centre au degré 7. Les produits exacts de niveaux sont distincts
des produits qui forment les niveaux. Le q3 matérialise le niveau par `|u|²|v|²|b−c|²/(4|u×v|²)` ;
il évite le degré 10 qu'aurait le carré du numérateur du centre. Les candidats différés ne changent
ni niveau brut, ni support, ni signe.

Les trois chemins de puissance sont correctement ordonnés dans `src/num/predicates.cpp:45–110` :

1. natif i128 si son budget global ou son certificat le permet ;
2. essai i128 vérifié par `__builtin_*_overflow`, sans lire un résultat débordé ;
3. repli `Wide` depuis les coefficients initiaux, jamais depuis une valeur tronquée.

La voie q4 native utilise une borne propre à trois points du même cube, pas la borne de deux vecteurs
arbitraires : chaque composante du cross est <M². Elle permet une somme absolue <72M⁵, donc <2^127
en u24. La preuve du q3 certifié majore tous les produits et toutes les sommes partielles par
15×2^123 <2^127. La voie orientation exige un certificat différent : ne pas réutiliser le booléen
de puissance (`power_certificate.hpp` / `orientation_certificate.hpp`).

Pour q4, `q4_weights.hpp:24–32` calcule les poids depuis le numérateur BRUT avant normalisation du signe
du déterminant. La voie i128 n'est utilisée que si les quatre points ont une étendue par axe ≤2^20 ;
117×(2^20)^6 <2^127 borne chaque intermédiaire. L'autre voie est `Wide`. Le certificat de positivité
appartient au quadruplet de présentation ; la canonicalisation d'un autre quadruplet emploie le prédicat
générique d'orientation du centre. Cette distinction évite une erreur historique et doit rester visible.

Les clés flottantes `catalogue/sort_level_key.hpp` ne décident que lorsqu'une marge F4 sépare les clés :
les tête de mots tronquées, conversions et quotient ont E=6 ; zéro est traité séparément ; dans la zone
d'incertitude, le comparateur exact tranche. Rien ici n'autorise une décision géométrique en binary64,
ni une preuve F3 obtenue en comptant seulement les instructions.

Les bornes de `LatticeSphere` gardent un plancher exact C/D même pour C négatif, saturent les propositions
avant leur utilisation et évaluent finalement des points entiers du domaine. `lower>0` exclut un bloc ;
`upper<0` certifie ses intérieurs ; les zéros sont conservés. Cette lecture est favorable. Les gates
existantes de monotonie, cas limites et Fraction restent nécessaires après tout port.

## 3. Catalogue : complétude et limites effectives

La chaîne est distincte d'un générateur d'arêtes WSPD : boîtes de centres demi-ouvertes, listes de sites
K-certifiées par domination uniforme, supports q2/q3/q4, population locale complète, émission du seul S*.

Dans `boxes.cpp`, le réservoir n'est qu'un ensemble de témoins proposé. Le rejet G1 exige K dominateurs
stricts sur toute la fermeture de la boîte ; l'égalité ne rejette pas. Le rétrécissement à l'enveloppe des
sites restants est compatible avec un centre de support positif. Les boîtes filles partitionnent les
propriétaires par `[lo,hi)` ; l'enveloppe racine inclut la coordonnée maximale via `hi=max+1`.

Dans `leaf.cpp`, G3 compte une UNION de dominateurs distincts. Les lignes vivantes de paires préfiltrent
la même décision ; le ledger conserve un compte logique de préfixes, pas un compte exact d'instructions.
J2 ne teste aucune acuité : un triangle obtus peut encore être le préfixe d'un q4 positif. Le test d'acuité
dans `q3_of` ne commande jamais la récursion q4. M3/E4 bornent des centres de supports positifs ; ils ne
constituent pas des certificats de naissance ou de fusion.

Le lemme R substitue des classifications exactes aux appels de puissance quand la domination le permet.
Le test `inside & outside` protège un invariant. Les supports ne sont pas omis de la coquille ; les autres
contacts sont tous conservés. Une population acceptée a p≤K+1−q ; la canonicalisation cherche cardinal
minimal puis ordre lexicographique dans la coquille entière, et seul le générateur S* émet.

Les chemins `device_leaf`, lot hôte, CUDA et split ne sont pas interchangeables par déclaration : ils
doivent rendre même sortie et même ledger logique. La source hôte/appareil est commune, mais les formules
géométriques de cette source sont une seconde transcription de `num` ; un port v12 doit garder un
différentiel qui les confronte au noyau exact et à un oracle réellement indépendant.

Une feuille non résolue est intégralement rejouée par le CPU. Le comptage avant émission dans la voie
`device_leaf` évite de publier un préfixe avant de découvrir un prédicat non certifié. Dans le lot, les
statuts non résolus ont zéro émission retenue ; le repli reconstruit la feuille complète. Les feuilles
au-delà de 32 sites sortent du backend partagé ; elles ne sont pas tronquées à 32.

Limites à afficher : K≤12, feuille complète ≤1024, quotas mémoire/nœuds/boules, multiplicité refusée.
Un nuage accepté à l'entrée peut encore être refusé lors du catalogue ou de la tour. Aucune borne
subquadratique globale n'est acquise sur le nombre de boîtes, les populations cumulées des feuilles ou les
candidats. L'absence de faute observée sur trois LiDAR n'est pas une preuve de terminaison sous 100 ms.

## 4. MEB, cellules, descentes et forêt

`bounded_meb` trie les IDs locaux, contrôle unicité et cardinal ≤12, balaie le diamètre, puis q3/q4
indépendamment. L'arrêt au premier support strict contenant repose sur la caractérisation M1 de la MEB ;
il ne compare pas arbitrairement deux boules candidates. Le support produit est local à la partie,
distinct de la clé canonique globale d'une boule du catalogue.

`cells_classify.cpp` et `forest_build.cpp:162–184` traitent la fenêtre
`p+qmin−1 ≤ k ≤ p+m`. La coquille régulière m=qmin a deux cas analytiques : k=p+qmin donne naissance,
k=p+qmin−1 donne les faces strictes. Les autres coquilles utilisent le modèle T2 ; les choix t≥qmin
peuvent nécessiter une MEB exacte de sous-partie. Ne pas étendre les raccourcis réguliers à une coquille
dégénérée ou aux seules cofaces de Gabriel.

La descente T3 choisit une partie strictement intérieure si p≥k, sinon la première trace stricte de la
coquille ; chaque nouveau niveau doit être STRICTEMENT inférieur au précédent (`descent.cpp:208–211`).
Les dates initiales sont gardées et comparées au plateau lors du raccord (`forest_plateau.cpp:56–61`).
La seule date terminale ne certifie pas une attache préplateau. Le cache de populations est un terminal
géométrique ; il n'est ni le DSU, ni un cache de composante.

À un plateau, `touch` mémorise les anciennes composantes une seule fois ; `unite_roots` concatène leurs
chaînes ; `close` crée une unique multifusion N-aire par groupe connecté. Il exige des enfants de rang
strictement inférieur, trie les enfants et conserve les anciens tops jusqu'à la clôture. Les parents
DSU O2 sont séparés des états de chaîne mais gardent le même invariant : la racine est la plus petite
naissance canonique, les parents décroissent strictement hors des racines. Ce choix assure une numérotation
déterministe ; il n'est pas l'union par taille. Une accélération v12 devra séparer représentant technique
et identité canonique, puis prouver la même multifusion, notamment sur les plateaux cycliques.

Les verticales utilisent une coupe FERMÉE : toutes les fusions basses de même niveau sont actives avant la
requête. Chaque fusion haute exige que tous ses enfants aient la même image basse après remontée. C'est
plus fort qu'un seul parent choisi au hasard. L'index d'ancêtres de balayage possède son propre union-find
par taille ; il ne lit pas les parents mutables du publieur.

## 5. Concurrence : chaîne de publication relue

Le Pool rejette la réentrance et le second appel actif avant les paramètres, distribue chaque intervalle
une fois, attend même les workers sans tranche et agrège les refus après la jointure. Les callbacks ne
sont pas annulés au premier refus ; c'est le module supérieur qui garde sa sortie transactionnelle.
L'époque booléenne est sûre parce que tous les workers acquittent avant l'appel suivant.

Dans le pipeline, les taches de résolution précèdent en ordinal les publieurs, qui précèdent les balayages.
Les seuls appels susceptibles d'attendre dépendent de tâches d'indice inférieur : pas de cycle de dépendance
lu dans cette construction. Une résolution en refus continue à marquer tous ses blocs, puis rend le refus.

Les graines sont écrites avant le store release du statut de bloc ; `await_job` les acquiert. O2 précharge
16 jobs plus loin seulement dans un bloc confirmé (`forest_concurrent.cpp:81–87`), donc ne lit pas une graine
en cours d'écriture. `parents` et `states` appartiennent au seul publieur de l'ordre.

Les annonces de forêt publient `nodes` puis `closed` ; un balayage utilise les champs immuables
rang/enfants des nœuds annoncés. Le champ parent est encore modifiable par le publieur, mais le balayage
ne le lit pas. `ProgressView` acquiert `closed`, puis les drapeaux et le nombre de nœuds ; la fin ou l'abandon
publie une valeur terminale et notifie. La garde d'abandon APRÈS `await_lower` est bien présente.

Les bornes des cohortes sont calculées avant tout tri, chaque tranche contient des cohortes entières,
chaque worker dispose de son scratch, chaque tranche de son ledger. La table dense est remplie après
la jointure du tri. Cela ferme les risques de lecture de bornes pendant leur réécriture et de scratch partagé
qui auraient été plausibles dans cette optimisation. Cela ne ferme pas N1, ni une qualification TSan au pin final.

## 6. Exécutions propres à ce sous-audit

`NARROW_RELEASE_U21.json` conserve les commandes, stdout/stderr, codes et SHA-256 du binaire partagé.
Les trois groupes natifs existants ont été exécutés directement :

| Groupe | Contrôles | Échecs |
|---|---:|---:|
| `line_reference` | 200 003 | 0 |
| `acute_and_triple` | 120 000 | 0 |
| `span_refusal` | 35 | 0 |

Ils couvrent l'étendue exactement 2^20, le voisin refusé au-delà, les coordonnées au sommet du cube u21,
les cas dégénérés, droites disjointes et intersectantes, et le produit mixte q4 étroit. Ils confrontent la
transcription étroite à la voie exacte du produit ; ce n'est pas une exécution GPU, ni un oracle indépendant
de bout en bout, ni une matrice sanitizer. Le coordinateur conserve séparément les résultats CTest communs.

`SOURCE_MANIFEST.json` fixe les empreintes de 116 fichiers des cinq modules. Cette couverture d'empreintes
ne signifie pas que chaque ligne a été démontrée formellement ; les conclusions de lecture sont celles
explicitées ici.

## 7. Portes minimales avant adoption en v12

- Pins exacts par mécanisme et réexécution au nouveau code : une ligne d'une ancienne matrice ne couvre pas
  le dernier moteur. Profils u18/u21/u24 séparés, GCC/Clang, ASan/UBSan/TSan ; CUDA réel séparé du source hôte.
- Fractions/Gram indépendants pour centres, puissances, orientations, niveaux ; bornes juste à la limite
  i64/i128 et juste au-delà, géométries quasi dégénérées, modes d'arrondi mixtes sur les clés F3/F4.
- Catalogue contre oracle borné de définition : contacts complets, q4 à face obtuse, coquilles mixtes,
  frontières demi-ouvertes, mêmes émissions sur CPU/source hôte/lot/repli ; mutations d'étendue et de statut.
- MEB proposée puis certifiée : proposition erronée, certificat faux, cas sans repli et avec repli ; support
  canonique, niveau brut, coquille et FULL identiques. Mesurer le coût total, pas le seul nombre de MEB.
- Forêts : plateau triangulaire cyclique, multifusions distinctes simultanées, dates initiales de descentes,
  verticales fermées, refus au milieu d'une résolution ou d'une publication, W varié et budgets serrés.
- Cohortes : nouveau témoin N1 ; séparer capacité prévue, mémoire physique et travail utile. Le parallélisme
  ne doit pas rendre une seule cohorte plus coûteuse en mémoire par simple multiplication par W.
- Hypothèses de coût : nombre de feuilles, tailles cumulées, candidats et replis, étapes T3, présentations MEB,
  lectures d'index, publication, verticales, préparation et sortie. Aucune micro-accélération ne vaut contrat FULL.

Les voies exactes actuelles constituent des références différentielles utiles ; les choix d'algorithme de la
v12 restent à prouver et mesurer sur leur propre implantation.
