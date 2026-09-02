# NOTE — livraison : paliers d'échelle P1 à P3 et streaming C6 jalons 1 à 3

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Livré au commit `0422fadd`. `ctest -L gate` : **230/230** ; `scale8000` :
16/16 ; `check_docs` : 258 fichiers. GCP non utilisé ; le profil d'échelle
reste sous votre NO START et je ne demande pas de GO ici. Aucune mesure de
performance n'est revendiquée.

## 1. Ce que P1 ferme, et pourquoi ce n'était pas qu'une couverture

Les 23 conformités se rejouent maintenant à `smax` réduit et comparent
`digest_forest_K1..K(smax−1)` **et** les cardinalités à la référence gravée,
en exigeant que la référence porte strictement plus d'ordres que le préfixe —
sans quoi la porte se validerait elle-même.

Le point intéressant est le mutant `prefix-tamper-event-order`, que vous
aviez raison de compter comme orphelin. Il **ne peut être tué ni par un
digest, ni par un plancher de cardinalité** : `facet_minus` trie la clé par
insertion, donc échanger `interior[0]` et `interior[1]` produit des `FacetKey`
identiques ; le contrôle d'événement ne vérifie que la distinction des
identifiants ; et le census remplit les identifiants intérieurs dans l'ordre
de la pile, non trié — il n'existe donc même pas d'invariant « intérieurs
croissants » à vérifier sur un run isolé. Il est tué par un **témoin de
multiensemble d'événements** (second run à `smax=11` dans le même processus,
multiensembles triés comparés ordre par ordre), et une contre-fixture à
**code 3 gravé** atteste qu'il **survit** sans ce témoin. La porte déclare
ainsi sa propre portée au lieu de la supposer.

## 2. P2 : le verdict ne dépend plus de l'allocateur

Le relevé du pic historique par étage est en place, sur deux lignes propres et
adjacentes — la ligne `rss_mb` gravée dans vos reçus est inchangée au
caractère près. Une contre-lecture interne a montré que le critère initial
pouvait être satisfait par le seul pic résiduel du fold : il est remplacé par
une **sonde auto-portée** (réservation adaptative, pages touchées, restitution)
qui établit la distinction pic/instantané par elle-même, sans allocateur dans
la boucle, et qui tient sous des réglages hostiles de la bibliothèque C. Les
seuils dépendants de l'hôte ont été retirés. La porte est étiquetée
**résidence** dans son en-tête, dans le plan de tests et par son propre label
CTest : elle juge l'instrumentation, jamais l'objet.

## 3. P3 : un résultat négatif, dit comme tel

Les trois libérations par tranche sont en place, mais **le RSS ne sépare pas
les deux variantes** : mesuré, l'incrément de pic du census vaut 201 Mo avec
libération contre 213 sans à 2 000 points, 729 contre 801 à 8 000. La cause
est que la bibliothèque C ne rend une tranche au système que si elle dépassait
son seuil dynamique, seuil qui monte dès les premières libérations. J'ai donc
instrumenté la grandeur **déterministe** (`census_merge_peak_bytes` : octets
déjà copiés plus octets encore détenus par les tranches non consommées,
maximisés sur les pas de la fusion) et je la plafonne, avec son mutant. Le
gain de RSS n'apparaîtra qu'aux tailles où les tranches dépassent ce seuil ;
je ne l'annonce pas.

## 4. C6 : vos trois premiers jalons, sans une ligne de CUDA

**Jalon 2, encodeur pur.** Écriture à offsets fixes absolus, sans allocation
ni `push_back`, appelable par plusieurs fils sur plages disjointes, produits de
tailles **prévalidés** contre le débordement, refus rendus comme valeur et
jamais une écriture partielle. Preuve d'équivalence `pack == append` **octet
pour octet** sur 2 238 014 boules, 15 lots incomplets, 8 bords, 1 à 8 fils,
propre sous ASan, UBSan et TSan ; deux mutants et une contre-fixture de
sélectivité à code 1. `append_ball_in` reste le chemin de production.

**Jalons 1 et 3, contrat des baux et modèle différé.** Trois ressources à
durées de vie séparées, comme vous l'avez corrigé : entrée rendue au transfert
montant, jeu de calcul au descendant, sortie **seulement** à la
reconstruction ; machines à états et époques distinctes ; retraite strictement
ordonnée par `base_global`. Granularité versionnée `lot_ring_granularite_v1`
et règle d'erreur `lot_ring_erreur_v1` (minimum lexicographique sur index
global, rang d'étage, code, message), dont l'indépendance à l'ordre d'arrivée
est prouvée en rejouant la même scène dans les deux sens. Le modèle différé
est **séparé** et `cuda_stub.hpp` reste séquentiel et intact ; il est qualifié
d'auto-test de l'ordonnanceur, et son en-tête dit qu'il ne prouve ni le
matériel, ni l'absence de course, ni aucun temps. 21 scènes, trois tickets
simultanés attestés, bouclage dans les deux variantes de sentinelles, fins
inversées à chaque étage, queues, erreurs précoce et tardive ne laissant
**aucun** enregistrement visible ; cinq mutants tués.

Corrigé avant tout le reste, comme vous le demandiez : les deux constantes du
contrat de transport valaient 12 et 92 contre 9 et 91 réellement transportés,
et sont désormais liées par assertion aux 100 octets par boule.

## 5. En cours

Le jalon 4 sous stub : brancher l'anneau et l'encodeur dans la route hôte
partagée, sentinelles hôte conservées, un seul flux et un seul jeu de mémoire
de calcul, reconstruction séquentielle, la route C5 restant disponible à côté
pour la campagne appariée à trois bras. Et le palier P4 d'échelle : suppression
du double exact du tableau de candidats au tri (permutation appliquée par
cycles, avec son théorème de stabilité) et hissage des deux piles allouées par
boule dans le census.

Je ne demanderai le pin et le GO de session qu'après les cinq paliers en
mémoire, comme vous l'avez ordonné.
