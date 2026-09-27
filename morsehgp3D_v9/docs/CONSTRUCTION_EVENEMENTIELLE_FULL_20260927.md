# Reconstruction FULL : entrée native et constructeur événementiel

27 septembre 2026. Grille 1 mm/u18, exploration v9 hors registre,
`public_status=not_claimed`. Aucun changement du moteur ni nouvel appel GCP
dans ce lot. Le contrat reste la tour **explicite** K5 sans sol en 100 ms.

## Le verrou traité

Les ordres K étaient déjà construits et encodés avec recouvrement.
La partie encore séquentielle à l'intérieur de chaque K suivait les contacts
un par un, mettait à jour les racines et ajoutait les actions. Augmenter
le nombre de CPU ne suffisait donc pas à paralléliser cette partie.

Le [diagnostic précédent](../audits/b_full_construction_parallel_20260927/README.md)
identifiait 252–256 ms de tour hors fenêtre d'encodage dans deux passages
G4 historiques. Ce n'est ni un nouveau chrono, ni le gain promis du nouveau
constructeur : validation, géométrie des cibles, populations et verticales
restent aussi à payer.

## Première brique close : savoir exactement ce qu'il faut conserver

Le [manifeste A](../audits/b_full_a_manifest_20260927/README.md) capture
les vrais blocs et cibles du constructeur natif. Il conserve les événements
silencieux, les contributions datées, l'ordre des contacts et les racines
de chaque occurrence. Un simple arbre final ou la liste des seuls événements
visibles n'aurait pas suffi.

La qualification R2 ferme 17 commandes Release et Clang ASan/UBSan/LSan :
22 variantes, 44 captures et 376 rejeux donnent exactement les mêmes tableaux.
Les cas comprennent des identifiants non consécutifs, des contacts égaux,
des contributions tardives et une fusion à 32 parents. Les lectures LIVE
normale et `-O`, ainsi que vingt corruptions du reçu, passent.
La [contrelecture indépendante](../audits/b_full_a_manifest_review_20260927/README.md)
confirme cette portée, sans transférer la preuve à la résolution géométrique
des cibles ou à la complétude des catalogues.

Deux défauts du harnais ont été corrigés avant qualification : une capture
réutilisée après échec ne garde plus son ancienne admission, et un succès
local de toutes les phases A ne suffit pas si une phase suivante échoue.
La première capture R1 reste non qualifiée : son inventaire préalable
d'en-têtes était incomplet malgré des tests exécutés avec succès. R2
conserve les mêmes sources C++, mais refait proprement cette preuve.

## Deuxième brique : rendre les événements calculables indépendamment

Le [prototype C++ événementiel](../audits/b_full_a_events_20260927/README.md)
est implémenté ; sa qualification fraîche R1 est close. Il ne recopie
aucune résolution géométrique. Il utilise les cibles du manifeste pour
construire une forêt auxiliaire qui conserve les composantes à chaque
seuil, puis interroge leur historique.

Il distingue impérativement :

- les parents **avant** le contact, avec une coupe ouverte ;
- le groupe **après** tous les contacts du même niveau, avec une coupe fermée ;
- les événements qui créent un nœud et ceux qui ajoutent seulement une
  contribution à un nœud existant.

Les tableaux finaux retrouvent les IDs historiques, parents et contributions
natifs. Le niveau représenté est celui du premier bloc de tout le plateau,
pas du premier bloc qui émet une contribution.

La porte Release et Clang ASan/UBSan/LSan passe 376 comparaisons issues des catalogues natifs,
avec deux enracinements opposés ; s'y ajoutent deux rejeux du cas limite K1 et
64 rejeux de petits programmes abstraits. Trois erreurs volontairement
introduites sont détectées : mauvaise coupe parent, historiques non
contributifs supprimés, ordre de plateau inversé. La capture ferme
17 commandes et 1 141 dépendances ; lecteurs normal/−O et 25 corruptions
ciblées passent. Les trois mutants sont des branches du même binaire,
pas trois compilations indépendantes. Aucun chrono de gate ne devient
un benchmark LiDAR ou FULL. La
[contrelecture indépendante](../audits/b_full_a_events_review_20260927/README.md)
confirme les mêmes objets, les contrôles de reçus et cette portée limitée.

## Ce qui est — et n'est pas — sous-quadratique

Pour V sommets, E occurrences de représentants, B boules et N sites dans
le domaine, ce prototype a une borne de travail
O(N+B+(V+E)·log(V+E+1)). Sa mémoire comprend notamment E occurrences,
V·log(V+1) entrées d'ancêtres et B ancres de sortie comme le natif.
Le tri global des E arêtes est évité : les sources arrivent déjà par rang
croissant. Les tris locaux de parents restent payés.

Cette borne porte sur la **taille du manifeste**, pas directement sur N.
Elle ne prouve donc pas que tout le générateur LiDAR soit sous-quadratique.
Aucune nouvelle mesure 8k/16k/32k, demi/quart de trame ou multi-scènes
n'est revendiquée par ce lot. Les résidus q3/q4 difficiles restent ouverts.

## Prochaine étape vers les 100 ms

Ce premier constructeur C++ est encore séquentiel. Il sert à fixer et
vérifier l'objet avant de paralléliser la forêt, l'enracinement, les
requêtes, les préfixes et les écritures. Une
[simplification mathématique séparée](../audits/b_full_a_events_review_20260927/NEXT_MIN_LABEL.md)
propose de choisir le plus ancien ordinal de chaque composante : elle
permettrait d'éviter le parcours d'enracinement, une table auxiliaire et
les renvois intermédiaires. Ce n'est pas une capacité du prototype R1
qualifié, ni un gain mesuré. Il faudra mesurer V/E/groupes/
parents/contributions sur les vrais catalogues, ainsi que mémoire et
temps de toutes ces étapes. Les parents d'événements, ceux du draft et
les liens de la forêt finale sont comptés séparément.

Ensuite seulement, réutiliser cet historique pour les verticales et
raccorder la sortie explicite. Les populations, la banque et l'encodage
restent natifs pour l'instant. Aucun gain de tour ni contrat 100 ms
nouvellement acquis ; la priorité est le parallélisme **dans** chaque K.
