# Contrelecture : encodeur FULL sans continuations

27 septembre 2026. Audit indépendant en lecture seule de
[first.hpp](../b_full_first_parent_20260927/first.hpp) et du
[gate](../b_full_first_parent_20260927/gate.cpp), après ajout des cas
pré-lot futur et des quatre mutations. Cette note ne rapporte pas encore
la clôture d'une capture. Aucun moteur, source testée, reçu ni build n'est
modifié par cette contrelecture ; GCP non utilisé.

## Conclusion de lecture

Aucun défaut sémantique évident trouvé. Le raccourci applique correctement
le [lemme de première occurrence](../b_full_batch_review_20260927/README.md#piste-supplémentaire--voie-linéaire-sans-continuations)
au domaine où aucune action n'a exactement un parent. Il ne remplace pas
le cas général et ne change pas les conditions d'admission des drafts.

La vérification de forme CSR précède toute lecture par offset, puis celle
du domaine. La recherche d'une continuation porte sur toutes les actions
avant de fabriquer les objets de la voie rapide. À la première trouvée,
le code appelle l'encodeur général et ne publie aucun objet du préfixe.

Dans la voie rapide, `a_count` est bien le nombre de nœuds ; les nœuds
pré-lot sont exactement `batch_begin[b]`. `first[p]` conserve le minimum
de l'ordinal global des parents, indépendamment du calendrier. Le test
`p<a_count` protège tous ses accès. Une référence pré-lot invalide mais
dans ce domaine numérique peut entrer dans ce minimum : sa propre erreur
antérieure empêche qu'un refus induit plus tard change le premier motif.
L'absent maximal et les références hors tableau sont rejetés sans accès.

L'ordre strict intra-action, l'interdiction d'un parent né dans le même
lot et l'absence d'un usage antérieur sont tous contrôlés au stade parent.
Les erreurs de références restent ordonnées d'abord par ordinal, puis par
instruction ; un masque invalide de ref0 prime une population invalide de
ref1. Les en-têtes, même visités après les actions, retrouvent leur priorité
native par réduction du tuple. Une erreur d'action antérieure prime un
en-tête futur. Les sorties ne sont allouées/remplies qu'après admission.

Le scatter correspond aux objets natifs : `nodeID=a`, offsets de parents
égaux au CSR d'entrée, niveaux copiés sans normalisation, contributions
à leurs offsets initiaux et segment `a`. L'unicité globale des parents
admis garantit un seul écrivain par successeur. Aucun live chronologique
ni tri des incidences n'est reconstitué dans cette voie.

## Gate : portée et non-vacuité

Le gate réutilise explicitement les fixtures et le comparateur champ à
champ du prototype général ; ses appels passent par le nouvel encodeur.
Il ajoute 400 histoires K1 à K10 dont les continuations sont retirées,
puis six corruptions de chaque histoire. Ce retrait ne renumérote aucun
nœud puisque les continuations n'en créent pas ; les lots devenus vides
sont retirés. Chaque histoire rapide est exigée acceptée, pas seulement
égale à une autre implémentation qui la refuserait.

Les deux calendriers visitent actions, parents et en-têtes en sens direct
ou inverse. Le gate exige plus de 5 000 appels rapides, plus de 5 000
appels de repli et au moins 800 entrées acceptées. Il ne se limite donc
pas aux drafts invalides ni au fallback qui masquerait le nouveau code.

Les ajouts ciblés couvrent désormais dans la voie rapide :

- fusion puis réemploi, avec erreur de population antérieure qui doit
  primer le futur réemploi ;
- référence à un nœud né dans le même lot ; référence à un nœud d'un lot
  futur, puis réemploi après sa naissance ;
- masque invalide ref0 avant population invalide ref1 ;
- erreur parent d'une action avant en-tête invalide d'un lot futur ;
- parent absent maximal sans indexation de la table ;
- continuation tardive valide puis vide, qui doit déclencher le repli.

Les mutants sont causalement distincts : ignorer les répétitions, accepter
un nœud du même lot, garder la première erreur visitée au lieu du minimum,
et garder la dernière occurrence parent au lieu de la première. Le dernier
cas utilise une population invalide sur la première fusion : le mauvais
minimum déplace abusivement une erreur parent avant cette population.
Il ne dépend donc pas d'un crash pour détecter le défaut. Ces observations
portent sur la construction du gate ; le résultat exécuté appartient au
reçu du développeur, pas à cette note.

## Coûts et limites à conserver

Le code relu est scalaire. Le minimum est une boucle et non un `atomicMin`
GPU ; deux calendriers ne constituent pas une qualification de threads.
Le scatter est parallélisable après admission, mais ce parallélisme n'est
pas exécuté ici. La réduction d'erreur canonique et l'initialisation de la
table restent à payer dans un port parallèle.

Scratch principal de la voie rapide : `batch[A]` de `size_t` et `first[V]`
de u64, avec V=A. Les buffers des objets retournés et la copie complète
du CSR parents ne disparaissent pas. Le travail total inclut lots, actions,
parents, contributions et initialisation V. Le repli repaie les validations
de forme/domaine après la détection de continuation ; ne pas annoncer
son coût égal au seul appel général.

L'équivalence concerne les décisions sémantiques lorsque tailles et
allocations réussissent ; elle ne garantit pas le même premier échec
d'allocation qu'un autre ordonnancement. Le juge partage les types et le
comparateur numérique exact avec le moteur. Pas de qualification GPU,
d'injection de panne mémoire ou de borne globale de la tour dans cette
contrelecture.

Enfin, l'absence de continuations observée sur un draft LiDAR ne constitue
pas un invariant du producteur : `full_ball_tower.hpp::order_lot` publie
explicitement les actions à un parent lorsque leur contribution n'est
pas vide. Le repli général reste nécessaire même si la voie rapide est
fréquente sur les cas mesurés.
