# Pont natif isolé des deux extrêmes Euler

30 septembre 2026. Complément au [contre-audit de l'antichaîne](../../../audit_continu_20260929/antichain_counterreview_20260930/README.md).
Aucun produit, build existant, note active ou Git modifié. Trois binaires
isolés ne comprennent que `extrema.hpp` et `probe.cpp`, sans source ni
bibliothèque Morse HGP. Aucun nouveau calcul de géométrie/FULL ou campagne.

## Mécanisme et contrat

Pour les nœuds sélectionnés d'un point, garder deux entrées : le minimum de
`(tout croissant, tin décroissant)` et le maximum de tin. Ils sont les extrêmes
de l'antichaîne minimale ; leur LCA vaut celui de toute l'antichaîne. Le
minimum ne peut être un ancêtre redondant : son descendant termine plus tôt,
ou partage sa fin tout mais commence plus tard. Les minima incomparables
suivent le même ordre pour leurs débuts et leurs fins.

`Summary::push/merge` utilise des comparaisons explicites **u32**, sans
négatif, conversion i32, somme, produit ou clé entière empaquetée. Le vide
est `has=false`, distinct de tout identifiant. Des clés totalement égales
sont départagées par identifiant pour rendre la réduction déterministe,
sans effet sous le contrat d'un tin unique par nœud.

Les résumés sont des couples de réductions min/max, donc associatifs,
commutatifs et idempotents, avec le résumé vide pour neutre. Leur combinaison
permet de distribuer des incidences entre buffers privés de workers. Le
probe exerce des **partitions logiques**, pas des threads concurrents ni
leur synchronisation. Tous les résumés doivent appartenir au même contexte
Euler immuable ; le helper ne certifie pas la table ni son propriétaire.

Le contrat Euler est un préordre qui incrémente son compteur **une fois par
nœud** : `0 ≤ tin < tout ≤ N`, `N ≤ UINT32_MAX`. Si N vaut `UINT32_MAX`,
la fin exclusive tout peut valoir **kNone** ; ce n'est pas un nœud absent.
Les indices de nœuds restent `< kNone` et `< N`. Le helper accepte cette
fin et refuse cet identifiant. Un Euler incrémenté à l'entrée **et** à la
sortie n'hérite pas de cette borne : adapter son contrat ou son type.
Le calcul des compteurs Euler à cardinalité extrême n'est pas implémenté
ici ; seuls le domaine des comparaisons et sa preuve sont contrôlés.

Après préparation Euler O(V), le balayage est O(D), sans tri ni antichaîne
stockée, puis au plus une LCA par point. L'index LCA, sa mémoire et son coût
restent distincts. La combinaison parallèle coûte aussi O(P) pour P résumés
partiels émis ; allouer une table dense par worker ajouterait une mémoire
en n×W. Ce reçu ne borne ni D, P, FULL, ni le coût massif réel.

Les mêmes obligations géométriques restent nécessaires : toutes les
incidences fortes I/U propres à K, seuil carré exact, témoins vivants après
le plateau et fermeture complète de la bande avant décision. Le calcul
des extrêmes n'avance pas la disponibilité algorithmique des témoins.
Les garanties de couverture et d'ancrage proviennent de la preuve de
l'antichaîne à K fixé ; ce helper ne requalifie pas cette chaîne native.
Si les extrêmes appartiennent à des racines distinctes, leur LCA n'est pas
fini : refus ou politique +∞ explicite, jamais une fusion fabriquée.

## Contrôles nouveaux

Normal et `-O` : **588 sélections**, dont 24 variantes virtuelles sans
allocation d'un arbre géant. Les formes comprennent chaînes unary,
embranchements non binaires, arbre binaire et forêt déconnectée. Oracles
Python entiers exacts : antichaîne pairwise par ascendance parentale et
LCA de tous ses minima, séparés du code C++. Les intervalles virtuels sont
laminaires et sont jugés par inclusion exacte.

Release et Clang UBSan, six variantes de résumé par sélection : balayage
direct, fusion avant/arrière/rotation, fusion équilibrée, idempotence.
**7 056 comparaisons**, ordre inversé, doublons, workers vides et identifiant
zéro réel. Les 21 sélections traversant des racines distinctes retrouvent
des extrêmes sans LCA fini dans l'oracle. Ce panel ne rejoue pas les 6 227
sélections du contre-audit Python précédent.

Cas virtuels : franchissement de 2³¹, fin exclusive et cardinalité
`2³²−1`, dernier indice `2³²−2`, chaînes partageant cette fin et branches
de part et d'autre de la limite signée. Aucun tableau de N nœuds n'est
alloué. Huit entrées mal formées sont refusées sous UBSan, notamment
identifiant kNone, intervalle vide, fin au-delà de N et entier hors u32.

Le mutant **compilé** omet uniquement le départage par tin décroissant.
Sur une chaîne sélectionnée ancêtre d'abord, il retourne code **0**, mais
produit `(gauche=2,droite=0)` au lieu de `(0,0)`. Le LCA induit est la
racine 2 au lieu du dernier descendant 0. Le défaut est donc causalement
numérique, sans crash ou échec de compilation interprété comme preuve.

## Captures et limites

`record.py` crée un build neuf hors du moteur. Les 254 dépendances de
compilation, compilateur et runtimes sont hachées avant/après ; les vrais
fichiers `.d` sont inclus dans la fermeture initiale. Binaires, dépendances
dynamiques et sources restent identiques pendant les deux lecteurs.
`capture_receipt.json`, `build_receipt.json` et les reçus normal/optimized
conservent les arguments, sorties, pins et compteurs.

Deux préflights en échec sont conservés : résoudre `clang++` en `clang`
changeait la liaison C++ ; préparer les dépendances sans les flags -O2
omettait deux headers conditionnels de la bibliothèque standard. Le
lanceur corrigé conserve le frontend C++ et prépare chaque variante avec
ses flags exacts. Aucun défaut du reducer n'en est déduit.

`verify.py` et `python3 -B -O verify.py` vérifient les hashes locaux puis
recalculent les oracles sur les sorties natives archivées, sans compilation
ni nouvel appel natif. Un replay vivant nécessite les binaires/dépendances
externes explicitement capturés et des dossiers de sortie neufs. Aucun
index LCA, concurrence réelle, branche FULL, statistique, qualité,
performance LiDAR/G4 ou nouveau contrat de tour n'est qualifié.
