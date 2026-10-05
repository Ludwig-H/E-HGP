# Suivi borné S6 et portes à K élevé — 5 octobre 2026

Lecture source, sans défaut produit nouveau établi. Les douze fichiers S6 capturés sont identiques à ceux du reçu précédent ; sa correction du domaine `Shape` reste présente. `BEFORE.json` distingue les acteurs WIP de base f98, les documents Git **9cbf805c6e4c4f4893cb3549d5ab89a5d57e321c** et le rapport vivant. `AFTER.json` consigne leur comparaison après lecture. Aucun moteur compilé ou exécuté, aucune qualification native/G4 ajoutée.

La couverture présente mérite deux petites portes complémentaires. Les nuages bornés de `s3/tests/tower/order_tree_support.hpp:43` montent à K=5 ; une porte LiDAR longue K10 existe dans `s3/tests/tower/tests.cmake:314`. S6 exerce déjà une coquille plane de 24 sites, mais `s6/tests/supports/supports_test.cpp:319` compare explicitement N2, N3 et N24. Au-delà de 16 sites, `sample_judge.py:217` utilise les N_j publiés pour contrôler les formules : il ne fournit donc pas seul un oracle indépendant de chaque indice haut. Ce sont des différences de couverture, pas des échecs observés.

## Nouveau témoin exact à K12

Sphere5 est l'ensemble des 24 triplets entiers de norme carrée 5, translatable de +2 sur chaque axe. Il contient 12 paires antipodales. Ses supports minimaux sont Q2=12, Q3=24, Q4=792 ; le reçu précédent a confronté toutes les 12 926 présentations à un calcul Gram indépendant. Le présent helper recalcule la famille par déterminants entiers, puis complète uniquement les indices hauts.

Toute partie de cardinal au moins 13 contient une paire antipodale, donc le centre : **N_j=C(24,j), j≥13**. Une partie de cardinal 12 sans paire opposée choisit exactement un site dans chacune des 12 paires. Parmi ses **4 096** choix, 116 ne contiennent aucun support minimal. Carathéodory et la minimalité affinement indépendante impliquent alors : **N12=C(24,12)−116=2 704 040**. Un calcul indépendant des chambres des douze plans centraux perpendiculaires aux paires retrouve 116 : lors de l'ajout d'un plan, ses L lignes distinctes d'intersection créent 2L chambres, avec incréments `[1,1,2,3,3,4,4,5,8,9,9,9]`.

Ainsi `make_shape(0,24,2,12)` avec cette `Closure` doit donner :

| Compte | Valeur exacte |
| --- | ---: |
| K-parties reliées / parties comprimées | 2 704 156 |
| Traces strictes | 116 |
| Cofaces distinctes / cofaces Gabriel | 2 496 144 |
| Incidences de supports vers cofaces | 149 954 688 |

La dernière ligne ne doit pas remplacer les cofaces distinctes. Une porte native de primitives peut récupérer cette fermeture via `ball_supports` sur la boule centrale de **Cat1**, puis appeler `make_shape` et `ball_counts` à K12. Cela ne construit ni ne qualifie un arbre FULL12 ; `ball_shape(domain_Cat1,ball,12)` n'est pas nécessaire à cette porte. La boule possède une présentation canonique q2 : cette garde ne remplace pas les portes numériques des fabriques initiales q3/q4 et leurs profils.

## Petit témoin complémentaire à K10

Les quatre coins `(0,0,10),(0,20,10),(20,0,10),(20,20,10)`, plus les huit points intérieurs explicités dans `normal.json`, définissent la boule de centre `(10,10,10)` et de niveau carré 200. Son census est p=8, m=4, qmin=2 ; N2=2, N3=4, N4=1. À K10, les comptes attendus sont **66 parties reliées, 6 comprimées, 4 traces strictes, 12 cofaces distinctes et 4 Gabriel** ; les deux diamètres portent 20 incidences. Ce petit nuage à 12 sites peut compléter une porte native S3/S6 à K10 sans dépendre de la longue trame LiDAR. Le helper vérifie uniquement sa géométrie et ses comptes, pas son arbre complet.

## Admission mémoire du prochain assemblage

S6a exige des buffers fournis par l'appelant ; aucun nouvel assemblage global S6b n'est qualifié ici. Pour compter puis remplir, une réserve cohérente inclut le domaine et l'arbre retenus, les métadonnées par boule, les sorties admises et, **par worker actif**, le scratch de fermeture et la liste temporaire de supports nécessaire à `ball_supports`. À m=24, ce scratch vaut 2 Mio ; sa seule somme à 48 workers vaut 96 Mio, sans représenter le pic total ni le RSS. La capacité locale de liste est au plus C(24,2)+C(24,3)+C(24,4)=12 926 entrées, multipliées par `sizeof(Support)` de l'ABI réellement testée. Les offsets globaux restent en u64 avec sommes vérifiées ; l'identité BallIdx/ordinal doit être stable entre les deux passes. Ce conseil concerne le raccord futur, pas une panne mémoire reproduite.

## Rejeu et fermeture

Depuis ce répertoire : `python3 -B -S check_high_k.py` et `python3 -B -S -O check_high_k.py`. Les sorties `normal.json` et `optimized.json` sont identiques, **4 135 gardes**, code 0, stderr vide. `COMMANDS.json` conserve les commandes, versions et hashes exacts. `SHA256SUMS` inventorie tous les fichiers réguliers sauf lui-même. Les anciens reçus sont restés intacts ; aucune exploration de 2^24 masques, allocation massive, campagne ou mesure de performance.
