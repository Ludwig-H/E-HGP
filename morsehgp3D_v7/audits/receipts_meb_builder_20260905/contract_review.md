# Cas de budget pour le Builder privé

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Lecture et petites dérivations rationnelles ; aucun C++, moteur, coût, produit, Git ou GCP exécuté ou modifié.

Le [catalogue de cas](budget_cases.json) donne 23 séquences locales, trois injections exceptionnelles locales, dix cas de complétion publique et quatre injections exceptionnelles publiques. Ce sont des **attendus dérivés**, pas des résultats compilés. Le bridge du root doit comparer le vrai Builder privé à F, avec un même Builder pendant toute séquence ; recréer un Builder à chaque appel empêcherait de qualifier sa propriété de Work. Les caps MAX sont des injections scalaires pour juger les incréments, sans leur attribuer un travail historique observé.

Le raccord statique est cohérent : Work est membre du Builder, importé une seule fois depuis les nouveaux champs de Stats. Le miroir externe publie ses diagnostics sur chaque sortie ; le miroir entourant le repli ajoute à A seulement la variation du compteur legacy F. Le corps entre accolades de `miniball_reference` est identique octet par octet au corps `miniball` de F épinglé ; son hash et tous les hashes sources sont dans le JSON. Il n'y a ni nouveau Builder au repli ni rappel récursif de l'aiguillage. La preuve A≤legacy est conservée : avec A0≤c0 et c1≥c0, A1=A0+c1−c0≤c1, y compris à MAX et lors d'une sortie exceptionnelle après charge.

Le contrôle P0 est explicite : il appelle le corps F, sans recherche de paire ni charge P. A compte alors les candidats F réellement chargés. Le diagnostic `fallback` compte une décision avec marge legacy initiale, pas toutes les entrées F : à P0 et c≥L, F est appelé, `meb_calls` augmente, mais A et `fallback` restent inchangés. Les cinq nouveaux champs doivent être jugés séparément de la projection des 13 champs legacy.

## Noyau local indépendant

Le triangle T=(0,0,0),(2,2,0),(2,0,2) a trois distances carrées égales à 8. Le troisième point a puissance 4 dans chaque boule diamétrale ; F essaie donc ses trois paires avant le triangle positif : rang R=4, centre (4/3,2/3,2/3), rayon carré 8/3, clé primitive `(3,[-8,-4,-4],0)`. La proposition paie une paire initiale puis les trois paires du pivot à trois points et son triangle : cinq charges P et un pivot. Ces valeurs proviennent de cette géométrie et du catalogue local de candidats, sans support fourni par F au proposeur.

Pour un même Builder frais, L12/P7 et quatre appels T donnent successivement :

| Appel | Legacy | P | A | Pivots | Certificats | Replis | Résultat |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 4 | 5 | 0 | 1 | 1 | 0 | succès |
| 2 | 8 | 7 | 4 | 2 | 1 | 1 | succès |
| 3 | 12 | 7 | 8 | 2 | 1 | 2 | succès |
| 4 | 12 | 7 | 8 | 2 | 1 | 2 | refus legacy |

`meb_calls` augmente exactement une fois par retour normal, refus compris. A n'est ni `legacy−P` ni le total legacy : le premier appel a legacy4/P5/A0. Un certificat avec un cap legacy insuffisant garde A0 et la boule sentinelle, tout en augmentant `certified`. Le carré ajoute la distinction indispensable : le refus de coquille **après** repli laisse une boule matérialisée, tandis qu'un cap legacy trop court conserve la sentinelle. Le tétraèdre régulier complète les attendus avec R11, P16 et deux pivots ; le niveau q4 brut doit rester comparé littéralement à F, en plus de sa valeur rationnelle 3.

Les `sites[]` doivent être résolus depuis les PointId déclarés : Morton ne conserve pas nécessairement leur ordre. Les supports actifs sont donnés en PointId ; les positions inutilisées du tableau brut restent des zéros d'indice, et doivent aussi être comparées. Sur succès local, F ne réinitialise pas le statut ou motif entrant ; le JSON conserve explicitement cette règle.

## Passage par le vrai wrapper de complétion

Sur les trois points T, l'unique coface Gabriel K2 est T, q3, masque7, niveau8/3. Son cœur comporte trois arêtes, chacune avec une MEB q2 de rang1 et aucun intrus. `build_silent_cofaces` avec L3/P1 doit donc rendre `complete`, trois appels, legacy3/P1/A2, un certificat et deux replis. Ce cas tue une remise à zéro de Work à chaque arête en traversant réellement `Builder::run`. Deux invocations indépendantes du wrapper doivent chacune obtenir ces mêmes compteurs frais. Un tétraèdre K3 donne quatre facettes régulières de rang4 et vérifie séparément la persistance P7 à travers les appels publics.

La fixture CHAIN5 prouve aussi une purge non vide. Ses points sont A0=(0,4,0), B1=(8,4,0), C2=(4,10,0), D3=(3,1,0), E4=(5,1,0). Le JSON contient les certificats rationnels des 14 supports positifs issus des dix paires et dix triples ; aucun n'a d'extra-shell, et la coplanarité exclut un support q4 essentiel. Le filtre `q+nombre_interieurs=3` donne exactement ce catalogue Gabriel K2 :

| Support | Intérieur | Rayon carré |
| --- | --- | --- |
| A,E | D | 17/2 |
| B,D | E | 17/2 |
| C,D | A | 41/2 |
| C,E | B | 41/2 |
| A,B,C | vide | 169/9 |

Le cœur trié est AB, AC, AD, BC, BE, DE. La boule AB contient strictement D et E, et D précède E en Morton. La chaîne ajoute ABD, support AB et niveau16, puis remplace l'essentiel A par E : BDE est direct, au niveau17/2. Les huit appels sont donc AB, ABD, BDE, AC, AD, BC, BE, DE ; tous ont q2 et rang1. Avec L8/P2, le résultat est une coface ajoutée, legacy8/P2/A6, huit appels, deux certificats et six replis. Une coface est déjà présente **avant le troisième MEB**. Le cap L2 refuse ce troisième appel et doit purger cette coface ; la non-vacuité de `events` avant refus fait partie du contrôle, en plus de `added_cofaces=1`.

## Exceptions et limites du raccord

Les helpers arithmétiques actuels n'allouent pas. Les exceptions après charge P ou après charge F sont des injections dans des copies d'audit ; elles ne simulent pas un calendrier naturel d'allocations identique dans F et dual. Ne pas lancer une exception depuis le `NoObserver` nominal déclaré `noexcept`, ni depuis un destructeur de miroir. La couture doit être exactement après la charge acquittée et avant la suite du candidat ; un hook générique dans `charge` doit distinguer le compteur MEB des budgets de cœur ou de chaîne.

Après la première charge P locale sur T, une exception doit laisser P1, legacy0/A0, aucun certificat/repli et aucun nouveau `meb_calls` : ce dernier est normalement incrémenté après proposition ou à l'entrée F. Après une charge F dans le repli de T/P1, elle doit laisser legacy1/P1/A1, un pivot, un repli et un appel. Ces attentes vérifient les miroirs et la conservation des charges acquittées, sans inventer un appel normal terminé.

Sur CHAIN5, l'injection après la troisième charge P voit une coface déjà ajoutée : après déroulement de pile, legacy2/P3/A0, deux appels et deux certificats. L'injection à la première charge F avec P2 atteint le troisième MEB : legacy3/P2/A1, trois appels, deux certificats et un repli. `std::bad_alloc` doit être convertie par le wrapper en `resource_exhausted/silent_allocation_failure`, avec `events` purgé et diagnostics préservés. `std::runtime_error` doit se propager ; le wrapper ne rend alors aucun résultat. Pour lire les diagnostics après cette dernière exception, employer aussi `Builder::run` avec résultat détenu extérieurement, ou une couture de déroulement explicitement qualifiée, sans prétendre que le wrapper a retourné des statistiques.

Les entrées hostiles de la primitive interne (n hors2..11, indices invalides ou doublonnés, Candidate hostile) restent hors précondition : ne pas créer d'UB pour obtenir un rejet. Les rejets publics proposés portent sur l'index invalide, un masque non régulier et le catalogue dupliqué, tous avant MEB ; le catalogue vide et K1 sont des contrôles sans MEB. A/P et leur somme doivent être traités en entiers exacts, notamment la fixture où P atteint MAX puis A augmente à 1.

Le consommateur produit relu reste `src/pipeline/run.hpp`, qui inclut le header F de `src/forest/` et appelle son wrapper pour K≥2. Il copie `added.stats` avant de traiter un refus. Aucun nouveau consommateur des champs P/A n'a été trouvé dans `src/` ou `cli/` ; le header privé ne raccorde pas à lui seul l'option, l'identifiant de comptabilité, l'archive ou le refus P>0 sans complétion. Le bridge peut qualifier l'API publique **de l'overlay** sans attribuer ce raccord au pipeline produit. Cette note ne demande aucune campagne de coût ni reprise du corpus géométrique déjà fermé.
