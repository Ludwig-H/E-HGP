# Seuils de couverture partagés entre les ordres

6 septembre 2026, suite de d95855a7. `public_status=not_claimed`. Le [journal factorisé](LOCAL_DIAGNOSTICS.md) et le [quotient local](README.md) restent les prérequis. Ce complément calcule toutes les contributions d’une boule à partir de seuils par point ; il ne reconstruit pas les parents à partir de ces seuils.

## Une information scalaire par point de coquille suffit pour D_B

Soit B une miniboule de rayon positif, I son intérieur complet, U sa coquille complète, p=|I| et u=|U|. Une partie A de U est dite stricte lorsque son enveloppe convexe ne contient pas le centre de B. Noter h le plus grand cardinal d’une partie stricte et, pour x∈U, h_x le plus grand cardinal d’une partie stricte contenant x. Chaque singleton est strict ; ces maxima existent. Le plein U contient le centre, donc 1≤h_x≤h≤u−1.

Pour **1≤K≤p+u**, le manque local D_B(K), c’est-à-dire S_B privé de l’union des couvertures des facettes strictes de cardinal K, vérifie :

$$D_B(K)\cap U=\left\lbrace x\in U:K>p+h_x\right\rbrace.$$

Sa partie intérieure est I si K>p+h, l’ensemble vide sinon. Pour K>p+u, il n’y a aucun bloc : **D_B n’est pas une naissance ni S_B**, et aucune contribution n’est émise.

**Preuve pour la coquille.** À K≤p, tout x peut être complété par K−1 intérieurs, formant une facette stricte. À K>p, poser t=K−p. La réduction par absorption des intérieurs montre que x est couvert exactement lorsqu’il existe une partie stricte de U de taille t contenant x. Le caractère strict est héréditaire par passage aux sous-ensembles : une telle partie existe exactement pour t≤h_x. D’où le seuil annoncé.

**Preuve pour les intérieurs.** À K≤p, les K-facettes d’intérieurs couvrent I. À K>p, dès qu’une facette stricte existe, sa classe possède un représentant contenant tout I. Une telle facette existe exactement pour K≤p+h. Le seuil est commun à tous les points intérieurs.

Les D_B(K) croissent ainsi par inclusion avec K **sur les seuls rangs présents**. Cette monotonie concerne les contributions locales de cette boule, pas les deltas globaux disjoints. Les tokens de composantes et les images verticales restent distincts selon K.

Les seuls rangs portant une contribution locale non vide vont de p+min_x(h_x)+1 à p+u, restreints à la tour demandée. Leur nombre est au plus u−min_x(h_x), indépendamment de p. Cet intervalle de contributions est distinct de celui des ancres ; les autres rangs peuvent encore nécessiter des parents ou des ancres.

## Calcul et usage dans le producteur

La table booléenne de 2^u masques est déjà construite une fois par coquille. Parcourir ses masques stricts : leur cardinal met à jour h et, pour chaque bit x présent, h_x. Cela demande au plus u·2^(u−1) visites de bits, en plus du parcours des masques, avec seulement u+1 maxima. Une transformée sur tous les sur-ensembles n’est pas nécessaire si seuls les h_x sont demandés.

Ensuite D_B(K) s’obtient par u comparaisons et le test commun des intérieurs, sans calcul de composantes pour cette opération. Sous u≤12, les seuils h_x tiennent chacun sur quatre bits ; la population et ces seuils peuvent être partagés entre tous les ordres. Un enregistrement de contribution peut référencer B et K, à condition que le lecteur retrouve les mêmes seuils et la même population immuable. Il doit toujours porter son token après lot et respecter la date d’activation du [certificat](LOCAL_DIAGNOSTICS.md).

Ce calcul sert à préparer les contributions ou à les consulter indépendamment du quotient. Si les couvertures sont déjà réunies pendant un DSU nécessaire aux parents, ces seuils **ne prouvent pas un gain CPU** : le coût de préparation et celui des requêtes doivent être comparés. Aucun chronométrage ni diminution du coût global de découverte des boules n’est déduit.

Comme toute partie de taille q_min−1 est stricte, h_x≥q_min−1. Il n’y a donc **aucune contribution au premier ordre d’ancre K=p+q_min−1**, même lorsqu’une fusion y est nécessaire. Cela généralise aux coquilles quelconques le manque vide constaté sur les quatre cas réels.

### Premier rang des coquilles à diamètre : une vraie branche sans DSU

Si q_min=2 et u≥3, le premier ordre utile K=p+1 est **toujours localement inerte**. Les sommets réduits sont les singletons de U. Une paire échoue au test strict exactement lorsqu’elle est diamétrale. Chaque point d’une sphère de rayon positif a au plus un antipode ; les arêtes absentes forment donc un couplage. Le graphe complet sur au moins trois sommets privé d’un couplage reste connexe : les extrémités d’une arête absente se rejoignent par tout troisième sommet.

Un seul représentant I plus le premier point de coquille suffit ; sa classe couvre S_B et ses membres réduits sont les u singletons. On peut produire cette description en O(u), sans DSU de masques pour ce rang. Cela s’applique aux quatre coquilles réelles 50k. À u=2, il reste deux classes : ne pas appliquer le raccourci au cas régulier. **L’ancre fermée du bloc demeure nécessaire**, même lorsque le bloc ne change ni identité ni couverture publique.

## Deux contre-fixtures qui bornent la simplification

Toutes les coordonnées suivantes sont u16. Prendre le centre (5,5,5), rayon carré 25, intérieur vide. La coquille asymétrique contient S=(5,5,0), N=(5,5,10) et (8,5,9),(5,8,9),(2,5,9),(5,2,9). Les cinq points au nord donnent h=5, mais h_S=3 ; les cinq autres h_x valent 5. Un hémisphère contenant S ne peut contenir N ni deux points opposés de l’anneau. Il peut contenir S et deux points adjacents : h_S=3. Ainsi D_B(4)=D_B(5)={S}, tandis que D_B(6) contient toute la coquille. Remplacer h_x par h ou par h−1 serait faux.

Les seuils ne déterminent pas les parents. Le cercle plan (10,5,5),(8,9,5),(2,9,5),(0,5,5),(2,1,5),(8,1,5) et l’octaèdre (10,5,5),(0,5,5),(5,10,5),(5,0,5),(5,5,10),(5,5,0) ont tous deux u=6, q_min=2, h=3 et tous h_x=3. Pourtant, à K3, ils ont respectivement **six et huit facettes strictes isolées**. Aucune coface stricte de cardinal quatre ne les relie. Le même profil de contributions à tous les ordres ne suffit donc pas à reconstruire H0, et encore moins à décider les parents globaux.

## Contrelecture du helper C++ publié

Le [helper publié par 7debdbab](../../receipts/local_plateau_20260906/README.md) a ses propres captures constructeur. La lecture indépendante de `src/forest/local_plateau.hpp` est favorable : supports minimaux, étoiles de cofaces, représentants, couvertures et D_B correspondent aux preuves. Le domaine validé protège aussi les intermédiaires. Les gardes sur A, B et C précèdent la puissance et le calcul du centre. Avec les coordonnées u16, les puissances restent sous 2^106 en valeur absolue ; les déterminants et tests de coplanarité restent sous 2^123, donc dans i128. Une borne conservatrice de la comparaison barycentrique est 2^142, dans S192. Les puissances nulles des points de coquille donnent même d_j=A·E_j dans le triangle. Cette sûreté ne dépend pas du nombre d’intérieurs.

Le test `local_plateau_gate.cpp` vérifie la minimalité des supports retournés. Au moment de la lecture, son contrôle d’exhaustivité était limité aux quatre fixtures réelles à support unique. Une comparaison simple complète le juge : tous les masques vrais de la table Gram indépendante dont chaque face immédiate est fausse doivent former exactement `minimal_supports()`. Le carré détecte ainsi la perte d’un de ses diamètres, même si la table complète et les rangs restent corrects. Aucun défaut du producteur n’est observé ; aucun mutant C++ n’a été exécuté par l’auditeur.

Cette contrelecture est statique et datée par les empreintes de sources. Elle ne qualifie ni le helper exécuté, ni le raccord FULL, ni une archive industrielle. Les qualifications C++ D–O restent inchangées.

## Vérification indépendante bornée

[coverage_threshold_model.py](coverage_threshold_model.py) confronte la table de supports et les seuils à **toutes les facettes MEB rationnelles** de dix nuages de deux à six points. Il ne prend pas les couvertures du DSU comme juge de la formule. Les cas de supports positifs q2/q3/q4, les intérieurs, le carré, la croissance ABCZ et les deux coquilles à six points sont conservés.

Les vérifications portent sur 306 facettes complètes, 274 masques non vides, 45 rangs présents et dix rangs absents, ainsi que les 45 premiers ordres où chaque point manque localement. Six coupes Gamma vérifient le raccourci du premier rang q2 ; la paire diamétrale seule le réfute à u=2. Les autres contre-fixtures réfutent le remplacement de h_x par h ou h−1, la comparaison non stricte au seuil, l’inférence de connexité depuis un manque vide ou depuis tous les seuils, et l’interprétation du rang absent comme une naissance.

Les [sorties normales](coverage_threshold_normal.json) et [optimisées](coverage_threshold_optimized.json) sont identiques, codes 0. Les [sources et la portée de la revue](coverage_threshold_review.json) sont épinglées ; aucune source ou sortie scientifique antérieure n’est modifiée.

```bash
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/coverage_threshold_model.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/coverage_threshold_model.py
```
