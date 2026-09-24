# Catalogue scellé : ce que la passe 1 de la validation FULL protège encore

24 septembre 2026, auditeur C. Réponse à la question du développeur
(canal, 15 h 05). Base `48791e721` (R20). Cadre :
`phase=exploration_v9_hors_registre`, `public_status=not_claimed`. GCP
non utilisé.

## Réponse courte

**Oui, je fais la proposition, sous une condition.** Le contrôle de
**positivité** des supports réguliers doit d'abord devenir un invariant de
la chaîne : c'est une vérification en O(1) par clé, sur le support du
représentant. Il n'existe aujourd'hui que dans la passe 1 de la tour. Une
fois ce contrôle dans la chaîne, la tour scellée peut sauter **toute** la
passe 1, avec un échantillon à pas fixe. Sans ce déplacement, je
recommande de garder le sceau hors chemin. Sauter la passe 1 retirerait
alors le seul contrôle d'exécution de la couche q3 du haut, dans l'angle
mort d'Euler. Et sauter seulement les parties redondantes rapporterait
peu : le support déclaré pèse environ la moitié de la passe 1.

## Ce que la passe 1 vérifie, et ce que la chaîne établit déjà

`check_ball_locally` (`src/tower/forest/full_ball_tower.hpp`, l.
1162–1188) coûte sur G4, en R19 et R20 (48 fils), **15 à 26 ms à K5 et
60 à 82 ms à K10**. Elle se trouve sur le chemin critique de la tour.

| contrôle de la passe 1 | établi par la chaîne ? | preuve |
| --- | --- | --- |
| forme : arité 2 à 4, coquille ≥ arité et ≤ 12, intérieurs ≤ 9, dénominateur > 0 | oui | refus au-delà de 12 sites et garde des 9 intérieurs (`tower_chain.cpp` l. 1284–1288, 1341–1343) ; arité des fabriques du générateur ; q_min = arité pour les coquilles étendues ; dénominateurs positifs par les formules. **Réserve** : aucun refus explicite d'une arité > 4 |
| indices de sites, aucun site répété | oui | le recensement renvoie des feuilles distinctes du même `ix` (l. 1231–1239, 1276, 1371) |
| domaine des clés | oui | entrée u18 contrôlée (`prepared_cloud.cpp` l. 53–58), bornes analytiques q2/q3/q4, réduction par pgcd qui conserve les bornes |
| puissances : intérieurs < 0, coquille = 0 | oui | même prédicat `key.power` que `ball_census` (`census.hpp` l. 159 et 194), même index |
| **support déclaré** des boules régulières (forme MEB, clé, niveau) | **non** | pour une coquille régulière, la chaîne reprend l'arité présentée (l. 1289–1297) |

La forme MEB teste bien la positivité (`anchor_meb.hpp`, l. 85–105) :
- q3 : triangle strictement aigu ;
- q4 : `det > 0` et centre strictement intérieur ;
- q2 : toujours positive, puisque le milieu est intérieur au segment.

Sur le **chemin moteur**, la positivité est garantie à l'émission par le
type `ExactBall` (`make_q3` exige l'acuité stricte, `make_q4` des poids
barycentriques strictement positifs), puis par l'égalité des clés. Sur le
**chemin des voies GPU** (S4a/S4b), la chaîne reprend la clé brute de
l'enregistrement (`tower_chain.cpp` l. 981). `check_lanes_batch` ne
contrôle que la forme, et le juge des voies est désactivé sur LiDAR. La
positivité n'y repose donc que sur les filtres du code des voies et sur la
passe 1 de la tour.

**Euler** ne compense qu'en partie :
- une q4 régulière parasite isolée est toujours vue ;
- une q3 est vue si p ≤ Kmax−3.

L'angle mort est exactement **q3 à p = Kmax−2**, ainsi que toute
substitution d'une boule par une autre de même signature (profondeur,
arité).

Trois sceptiques ont vérifié ces affirmations, en lecture seule, et
aucune n'a été réfutée. Ils ont aussi relevé un point absent du dépôt :
**aucune porte de refus** ne donne à la tour une boule régulière non
positive (triangle obtus, tétraèdre dont le centre est hors de
l'enveloppe). La branche de positivité de la passe 1 n'a donc pas de
porte propre.

## Mesure locale : part de chaque contrôle

Quatre variantes compilées du code réel
([`pass1_variants.patch`](c_catalogue_scelle_20260924/pass1_variants.patch)) :
passe 1 complète, sans puissances, sans support déclaré, sans les deux.
Mesure sur 08/000000, W8, deux répétitions entrelacées, sur un hôte
**chargé** (les temps varient fortement). Durées de la passe 1 en ms,
validation `validate_parts[3]` :

| K | complète | sans puissances | sans support | sans les deux |
| ---: | --- | --- | --- | --- |
| 5 | 203 / 314 | 153 / 152 | 80 / 36 | 64 / 18 |
| 10 | 971 / 621 | 479 / 498 | 430 / 367 | 253 / 192 |

Les condensés de tour et de catalogue sont identiques dans les quatre
variantes : ces contrôles ne changent pas l'objet. À titre indicatif, le
support déclaré pèse environ la moitié de la passe 1 à K10 et plus de la
moitié à K5. Les puissances en pèsent moins, et les bornes, indices et tris
environ un quart. Sorties :
[`c_catalogue_scelle_20260924/`](c_catalogue_scelle_20260924/).

## Proposition

1. **Positivité dans la chaîne** (utile avec ou sans sceau). Dans la
   boucle de recensement, pour chaque clé distincte à coquille régulière :
   - arité 3 : triangle strictement aigu, trois produits scalaires sur des
     entiers u18 qui tiennent en i64 ;
   - arité 4 : `det > 0` et centre strictement intérieur, tests déjà
     disponibles (`q4_center_strictly_inside`) ;
   - `require(rep.arity <= 4)`.

   Refus typé `chain_nonpositive_regular_support`. Cela ferme le trou du
   puits des voies GPU pour tous les chemins, pour un coût en O(1) par
   clé.
2. **Portes de refus manquantes** : triangle obtus et tétraèdre à centre
   extérieur, d'une part donnés directement à la tour (chemin non scellé,
   `full_ball_census_geometry`), d'autre part injectés dans la chaîne par
   un mutant du puits des voies (`chain_nonpositive_regular_support`).
3. **Levier `tower_sealed_catalogue`**, désactivé par défaut, seulement sur
   l'appel interne chaîne → tour :
   - un `SealedCatalogue` n'est constructible que par `run_tower_chain`
     après son recensement. Il porte le catalogue (const, déplacé) et
     l'instance d'index, que la tour exige identique ;
   - jamais un drapeau de donnée, ni un catalogue réinjecté par
     `keep_catalogue` ; aucun condensé, puisque celui du catalogue coûte
     de 0,4 à 2,6 s sur un fil ;
   - l'API publique, l'oracle T2 et les portes gardent la validation
     complète.
4. **Sous le sceau**, la passe 1 est sautée, sauf sur un **échantillon à
   pas fixe** (une boule sur 64) à chaque exécution. L'échantillon attrape
   une faute systématique de plomberie ; une faute isolée est un résidu
   déclaré. La passe 2 est inchangée : fenêtres de rang, tables de plateau
   et témoins MEB des coquilles étendues.
5. **Portes du levier** :
   - scellé / non scellé : condensés identiques sur les familles à K3, K5
     et K10 et sur les fixtures T2 ;
   - mutant systématique de plomberie (un intérieur de chaque boule
     remplacé) : refusé sous le sceau par l'échantillon ;
   - mutant d'une seule boule : refusé sans le sceau, non garanti sous le
     sceau (documenté) ;
   - mutant « support régulier non positif » : refusé par la chaîne, avec
     ou sans le sceau.
6. **Statut** : `complete_relative` inchangé. La raison publiée devient
   `…_sealed_in_process_census`, avec une section PROVENANCE. Aucun statut
   public ne change.

**Gain attendu** : à peu près toute la passe 1, soit 15 à 26 ms à K5 et 60
à 82 ms à K10 sur G4 d'après R19 et R20, moins le coût de la positivité
dans la chaîne (quelques ms au plus). C'est une projection. Seule une
paire sur la même VM tranche.
