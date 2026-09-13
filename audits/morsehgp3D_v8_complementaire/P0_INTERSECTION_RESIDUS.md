# Croiser CreditPlan et AxisQ2Plan par des rangs partagés

13 septembre 2026, après `8e406f9b`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Prototype d'audit, aucun changement du moteur.

## Raccord proposé

Les deux plans q2 peuvent se compléter sans développer leurs paires.
Exiger le **même propriétaire immuable**, la voie q2 et le même besoin
h=seuil−cœur. Le résidu CreditPlan garde exactement les paires vérifiant
c_A(a)+c_B(b)<h. Un bloc axial contient une ancre originale a et une
plage [l,r) dans sa propre permutation B ; cette plage n'est pas une
plage d'IDs originaux ni une plage de l'ordre CreditPlan.

Pour chaque entier t de 1 à h, construire une liste S_t qui conserve,
dans l'ordre axial, les seuls IDs b tels que c_B(b)<t. Construire aussi
r_t(i), leur nombre parmi les i premières positions de cet ordre.
Chaque crédit se lit avec **ID original moins début du facteur B**.

Pour un bloc axial (a,[l,r)), poser t=h−c_A(a). Si c_A(a)≥h, le supprimer.
Sinon son intersection devient la seule plage [r_t(l),r_t(r)) de S_t,
associée à a et t ; ne pas émettre une plage vide. Le nombre de paires
de ce fragment est exactement r_t(r)−r_t(l).

La preuve est une propriété du filtrage stable : les éléments sélectionnés
d'une plage contiguë forment une plage contiguë de la sous-liste filtrée.
Deux plages axiales disjointes pour une même ancre restent disjointes
après cette transformation. Cette ancre utilise toujours le même t ;
les listes de seuils différents ne créent donc aucune paire en double.
Le résultat est **l'intersection exacte des résidus**, et conserve toutes
les paires que leurs deux garanties géométriques imposent de conserver.
Il peut encore contenir des paires que seul le census rejettera.

Aucune addition entre crédits axiaux et crédits CreditPlan n'est faite.
Le cœur est déjà soustrait une fois dans h pour chacun des deux plans ;
leur intersection ne lui attribue aucun crédit supplémentaire. Refuser
deux propriétaires différents même lorsque leurs coordonnées sont égales
évite une équivalence implicite des IDs, seuils et propositions de cœur.
Les accès aux propriétaires rejettent les entrées déplacées avant toute
lecture de tableaux.

## Coûts explicitement payés

Soient b=|B|, D le nombre de fragments axiaux examinés, D' celui de
l'intersection et M' son cardinal. Avec les tableaux de rangs explicites
du prototype :

| Opération ou stockage supplémentaire | Coût |
| --- | --- |
| Lire les crédits B dans l'ordre axial pour les h seuils | hb tests |
| Construire les préfixes | h(b+1) entrées de rang |
| Stocker les sous-listes d'IDs | somme sur b de max(0,h−c_B(b)), au plus hb |
| Transformer les fragments | O(D), avec D'≤D |
| Compter le résultat | O(D), sans développement |
| Développer ensuite les paires retenues | O(M') |

La construction ajoute O(hb+D) travail et O(hb+D') mémoire. Les tableaux
de rangs sont temporaires ; seules les sous-listes, les fragments et le
propriétaire doivent survivre dans un éventuel objet produit. h≤10 dans
ce contrat q2. Aucun tri, ordre inverse d'IDs, tableau A×B ni produit
des deux listes de fragments n'est nécessaire. Le coût de construction
des deux plans d'entrée demeure intégralement à payer en amont.
Les plages de S_t portent des IDs ; leur nouvelle numérotation ne permet
pas de réutiliser comme telles les plages ou les boîtes d'un index B antérieur.

Le coût hb ne disparaît pas parce que h est borné. On peut préparer
seulement les seuils réellement utilisés, au prix d'un passage initial
sur les fragments ; ce raffinement n'est pas implémenté ici. Le prototype
sort immédiatement si h=0 ou si un des deux plans est vide. Il conserve
les entrées vivantes pendant l'expérience ; une API retournant un plan
durable doit retenir son RectanglePtr et définir ses garanties d'affectation.

Une simple annotation « filtrer c_B<t plus tard » aurait un autre coût :
son expansion relirait les paires du résidu axial. Les sous-listes S_t
paient les IDs une fois et permettent de ne visiter que les M' paires
effectivement retenues à la consommation.

## Contre-vérification bornée et sources publiées

Le [juge C++](residual_intersection_probe.cpp) compare physiquement
l'expansion du prototype à `CreditPlan.keeps && AxisQ2Plan.keeps` sur
48 plans : trois stratégies, h1/2/5/10, cœur présent/absent, facteurs
inversés et ordres originaux volontairement permutés. Il vérifie les IDs,
la disjonction, les comptes et les plans vides. La géométrie des plans
d'entrée est celle déjà auditée ; ce test de composition n'est pas un
nouveau census géométrique.

Résultats par mode : 172 800 paires confrontées ; 24 568 paires développées
uniquement pour cette vérification bornée ; 18 plans réduisent strictement
les deux résidus. La composition paie 11 520 tests de rang et stocke
4 984 IDs ; 5 448 fragments examinés donnent 2 328 fragments de sortie.
Six plans sont vides. Cinq mauvais raccords sont refusés : propriétaires
différents, propriétaire déclaré incohérent, mauvaise voie et chacune
des deux entrées déplacées.

Deux mutations de copies temporaires sont réfutées par la divergence
exacte de l'ensemble des paires : remplacer `<t` par `≤t`, ou lire le
crédit B au rang de la permutation au lieu de son ID original. Les deux
sortent avec le code 1 ; la version positive sort avec le code 0.

Le [runner](residual_intersection_checks.py) compile dans un répertoire
neuf avec C++20 strict et UBSan. Le [reçu](RESIDUAL_INTERSECTION_CHECKS.json)
épingle `8e406f9b`, embarque les sept sources produit et le juge, et
conserve commandes, hashes et sorties. Rejeu autonome :

```bash
python3 audits/morsehgp3D_v8_complementaire/residual_intersection_checks.py --replay audits/morsehgp3D_v8_complementaire/RESIDUAL_INTERSECTION_CHECKS.json --output /tmp/intersection_replay.json
```

Les cinq sources produit du reçu BATCH_EXCEPTION sont identiques aux
octets publiés à `8e406f9b`. Les deux sources axiales sont identiques au
snapshot AXIS_ROTATION embarqué. Le correctif copie-puis-échange, les
états déplacés et les conventions du cœur n'ont donc aucun delta final
à requalifier. L'économie des tableaux initialisés puis remplacés reste
une proposition de l'autre auditeur ; elle n'est pas incluse dans ce commit.

Pendant la fermeture du reçu, le constructeur a commencé de nouveaux
changements dans local_credits.cpp et axis_q2.hpp/.cpp. Le reçu signale
leurs hashes distincts ; les essais ci-dessus consomment exclusivement
le snapshot publié et ne qualifient pas ces changements concurrents.

Un [contrôle séparé](ALLOCATION_DELTA_CHECKS.json) du nouveau cpp confirme
ensuite les états des tableaux après l'initialisation conditionnelle :
batch passe 10 872 contrôles et 585 comparaisons de voies ; affectation
passe 1 110 contrôles, dont 20 pannes d'allocation avec conservation de
la cible. Sept sources sont stables avant/après capture. Options reprises
du précédent audit : GCC 13.3, C++20 strict, -O1 et UBSan. Une tentative
supplémentaire -O2+UBSan est conservée comme refus de compilation : GCC
diagnostique une indexation hors limites dans le test de voie invalide,
dont le header et la gate sont inchangés. Ce diagnostic n'est pas une
observation d'exécution ni une qualification -O2 du nouveau code.
Cette fermeture bornée ne couvre pas les nouveaux changements axiaux.

Aucun temps, grand census, gain global P0 ou résultat de tour n'est déduit
de cette composition. GCP non utilisé.
