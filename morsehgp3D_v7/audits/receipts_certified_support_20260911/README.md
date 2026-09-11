# Canoniser une MEB certifiée avec au plus quatre témoins

11 septembre 2026. Proposition constructive indépendante, CPU u16 hors registre,
`public_status=not_claimed`. Écritures dans `audits/` uniquement ; GCP non utilisé.

**Le support positif de la MEB suffit à juger les candidats canoniques. Quand
la coquille est exactement ce support, toute la canonisation peut être omise.**
Cette réduction complète le proposeur rapide et son repli déjà étudiés ; elle
n’exige pas de changer de support canonique, de trajectoire ou de BallId.

## Certificat d’entrée et preuve

Soient F les K sites distincts, B leur MEB certifiée, U sa coquille exacte et S
un support positif de B. Le centre c de B appartient à l’intérieur relatif du
simplexe S ; son cardinal q est compris entre deux et quatre. Le certificat
comprend **la positivité de S et le confinement de tout F**, pas seulement une
boule passant par quelques sites. La voie singleton existante reste séparée.

Pour un candidat positif T pris dans U, sa boule C est MEB(T). Notons d son
centre, t son rayon et r celui de B. Puisque T est contenu dans B, t≤r. Si C
contient seulement S, les coefficients positifs du certificat donnent :

$$t^2\geq\sum_{s\in S}\lambda_s\lVert s-d\rVert^2=r^2+\lVert c-d\rVert^2.$$

Ainsi t=r et d=c : C=B. Le confinement des autres sites et la taille de coquille
sont déjà connus. Chaque candidat peut donc être testé sur **S seul**, au plus
quatre points. Les membres de S déjà dans T ont une puissance nulle par la
formation exacte du candidat ; leurs évaluations peuvent aussi être omises.

L’ordre canonique est préservé : tout support admis par l’énumération complète
appartient à la coquille de l’unique MEB. Garder les indices initiaux triés,
puis les mêmes boucles q2, q3 et q4, restitue le premier support admissible.
`selected_shell_count` vaut alors le cardinal de U déjà calculé.

**Cas direct U=S.** Le support certifiant est affinement indépendant et
strictement positif. Aucun sous-ensemble propre ne porte le centre ; puisque
aucun autre site n’est sur la coquille, S trié est l’unique support canonique.
Il suffit de restituer ses champs, sans candidat ni test de puissance de
canonisation supplémentaire. Cela inclut les coquilles régulières q2/q3/q4 ;
cette conclusion est locale et n’exige aucune régularité globale de F.
Aucune fréquence de ce cas dans les vraies descentes n’est supposée ici.

## Raccord concret au proposeur

1. Former ou vérifier le support positif S de la proposition, avec les gardes
   exactes q2/q3/q4 existantes. Réutiliser la forme déjà calculée lorsque possible.
2. Lors du passage complet de confinement, extraire simultanément U. Ce passage
   est déjà nécessaire au proposeur ; conserver son coût.
3. Si U=S, restituer directement S trié. Sinon, énumérer les supports canoniques
   dans U et ne tester que les sites de S qui ne sont pas dans le candidat.
4. Si le certificat manque ou échoue, garder la canonisation complète ou le
   repli exact existant. Le coût de la proposition et de sa certification reste
   payé même lorsqu’un repli est nécessaire.

`boundary_ball` du prototype réparé n’impose pas la positivité. Pour utiliser
ce raccourci, il faut donc un certificat positif du même B, par exemple la
validation de son support par `form`. Un échec de cette validation ne prouve
pas que B est incorrecte : un autre support positif peut exister ; la voie
complète reste disponible. Ne pas remplacer q_min du census par cette arité
locale, ni changer implicitement le choix du support.

## Pourquoi le certificat positif est nécessaire

Considérer F=`{(0,5,0),(2,9,0),(5,10,0),(5,1,0)}`. La circumboule des trois
premiers points, centre `(5,5,0)`, rayon carré 25, contient F. Sa coquille U
comprend ces trois points. La boule diamétrale du premier et du troisième
point contient U, mais laisse le quatrième point dehors : distance carrée
97/2 contre un rayon carré 25/2. Elle ne garde que deux points sur sa coquille.

Cette fixture logique impose le refus du raccourci sans support positif ;
elle n’est pas présentée comme une sortie effective du Welzl réparé. Les
plateaux, coquilles supplémentaires et supports d’arités différentes restent
acceptables lorsque le certificat complet est satisfait.

## Qualification bornée

Le [helper C++](certified_support.hpp) reçoit les indices d’un support proposé,
reconstruit lui-même son certificat positif et vérifie le confinement complet.
Il ne consomme pas un simple booléen fourni par le proposeur. Le domaine du
témoin est K=2..10, sites distincts u16 ; singleton et politique d’admission
produit restent séparés. Le helper est un témoin local, sans raccord au moteur.

Le [probe](probe.cpp) confronte la réduction à l’énumération sur U vérifiant
chaque candidat sur tout F, puis à `anchor_meb` : statut, clé, niveau exact,
arité, quatre slots canoniques et cardinal de coquille coïncident. Il couvre
diamètre, triangle aigu, cube avec deux points intérieurs et fixture K7,
chacun sous trois permutations ; le cube reçoit quatre supports positifs
alternatifs. Au total **21 cas par build**, dont douze changements de support,
six passages q4→q2 et neuf retours directs U=S (q2, q3 et q4).

| Travail sur ces 21 cas | Contrôle sur F | Réduction proposée |
| --- | ---: | ---: |
| Puissances du certificat d’entrée | 171 | 171 |
| Puissances de canonisation | 460 | 123 |
| Total des deux phases | 631 | 294 |

Formations tentées et positives sont aussi rapportées séparément pour ces deux
phases. Les neuf retours directs ne tentent aucune formation de canonisation ;
les douze autres cas conservent exactement l’ordre des tentatives du témoin
complet. Ce tableau ne comprend aucun coût de génération de la proposition.

Deux rejets distincts sont bloquants dans chaque build : la circumboule
englobante sans support positif ci-dessus, puis un support positif K7 dont
la boule laisse un site dehors. Le premier retrouve une sortie valide par
`anchor_meb`, avec les 18 puissances du repli comptées séparément ; le second
refuse après sept puissances du confinement, avant toute canonisation.

Les commandes C++20 utilisent `-Wall -Wextra -Wpedantic -Werror`, en O2 puis
ASan/UBSan ; codes 0, stderr vide et sorties identiques. Le détail des commandes,
sources avant/après et sorties complètes figure dans [commands.json](commands.json).
Une première capture et ses deux sources `initial_*` sont conservées avant
normalisation des slots inactifs. La capture finale exerce des suffixes 255
sur les mêmes cas q2/q3 ; les quatre slots de sortie restent canoniques.
Les huit commandes initiales/finales réussissent, avec sorties identiques.
La [source épinglée](source_pins.json) réutilise l’archive à 57 fichiers du
[reçu précédent](../receipts_meb_boundary_20260911/README.md), sans duplication.
Le header MEB consommé est `386072c8`, identique au header actif lors de cette
passe ; le header de tour de l’archive est antérieur et n’est pas qualifié ici.

## Lecture et reproduction

Le [lecteur](verify.py) vérifie les empreintes, les résultats et la non-vacuité,
sans compiler ni exécuter de géométrie ; ses portes restent effectives avec `-O`.
Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/audits/receipts_certified_support_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_certified_support_20260911/verify.py
python3 -B morsehgp3D_v7/audits/receipts_certified_support_20260911/reproduce.py --work morsehgp3D_v7/audits/receipts_certified_support_20260911/.work_replay
```

La reproduction exige un sous-dossier neuf de ce paquet, restaure les sources
scellées et conserve ses quatre commandes dans ce sous-dossier. Les refus et
échecs éventuels y restent lisibles. Les sources actives ne sont pas modifiées.
Aucun temps de noyau, gain de tour ou résultat GPU n’est déduit de ces bornes.
