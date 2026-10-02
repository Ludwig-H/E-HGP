# Cellules strictes et localisation d'une MEB

Tranche du 2 octobre 2026, profil entier u18/u21/u24, poids unitaires.
**Qualification native G4 à venir.** Ces primitives privées de tower ne construisent
encore ni descente complète, ni forêt, plateau, verticale ou hiérarchie de points.
Elles utilisent le même [FullDomain possédé](FULL_DOMAIN.md) et les lemmes
[M1/M2, T2–T6](MATHEMATIQUES.md). Les [sources épinglées](../src/tower/source_pins.json)
séparent les ports, les constructions neuves et les oracles.

## Localisation dans le domaine commun

`locate_part(domain,F,k,budget)` exige `1<=k<=K` et une partie de k SiteIdx distincts.
La MEB possède sa sphère exacte et son support canonique **local**. Un hit exact de ce
support dans le catalogue identifie la même boule : un support strict détermine sa MEB
unique. Il réutilise alors la population complète du catalogue, sans allocation de census.
Ce hit peut avoir p>=k tout en rendant `Complete` : le futur saut lit aussi le cardinal de I.
Un miss ne certifie jamais l'absence de la boule.

Sur miss, le census du même index utilise le seuil k. S'il sature, le résultat possède
exactement k témoins stricts et aucune coquille ; la canonicalisation n'est pas appelée.
Sinon, I et toute U sont disponibles. `canonical.cpp` cherche S* sur U par arité croissante,
puis SiteIdx lexicographiques : milieu, triangle strict aigu dans le plan du centre,
tétraèdre strict intérieur. Un préfixe q3 obtus ne coupe pas q4. Le cas ponctuel q1 a
un support unique et n'appartient pas au catalogue positif.

Une boule positive absente alors que `p+qmin<=K+1` donne `tower_invariant`.
L'absence hors admission est permise : la future descente doit d'abord réaliser un saut
strict valide. Le raccord ne confond pas la présence d'une boule avec la validité de sa
cellule à chaque ordre. L'identité n'utilise ni niveau seul, ni hash seul, ni nouvelle clé PGCD.

`LocatedPart` possède sa MEB. Sur hit il emprunte I/U du domaine, qui doit donc lui survivre ;
sur miss il possède le Census. Les déplacements conservent les adresses des buffers et
vident les vues de la source. Un refus restitue les nouvelles réservations, sans modifier
le domaine ni les résultats déjà retenus. Mémoire propre : zéro octet Buffer sur hit,
4k sur saturation, 4(p+m) sur census complet ; états fixes et temps de canonicalisation
s'ajoutent. La recherche du support global peut visiter O(m^4) tuples, sans borne par K
sur m. Son helper est privé et suppose le Census complet produit par ce raccord ; il ne
constitue aucune API publique de canonicalisation d'une coquille arbitraire.

## Toutes les traces strictes d'une cellule

`build_cell(domain,b,k,budget)` valide BallIdx et la fenêtre fermée
`p+qmin-1<=k<=min(p+m,K)` ; tout autre ordre est refusé. Poser t=k-p, donc `1<=t<=m`
et `t<=12`. Chaque `CellTrace` possède la partie complète I∪A, triée par SiteIdx,
de cardinal k ; ses douze emplacements sont complétés par `kNone`.
Les traces sont ordonnées lexicographiquement selon A dans U.

Si m=qmin, t=m est une naissance et t=m-1 fournit les qmin faces strictes analytiques.
Pour une coquille étendue, t=m reste une naissance analytique. Sinon toutes les
combinaisons A de taille t sont visitées. Si t<qmin elles sont strictes par minimalité
du support global. Pour les autres, le produit teste `beta(A)<lambda_b` par MEB exacte.
T2 donne l'équivalence avec la séparabilité et avec `beta(I∪A)<lambda_b`.
La MEB(A) ne devient jamais la MEB(I∪A) à transmettre à une descente.

Zéro trace signifie naissance ; sinon le résultat est `StrictTraces`.
**Le nombre de traces ne donne ni le nombre de morceaux ni l'arité d'une fusion.**
Le raffinement en toutes les traces couvre chaque morceau local. Leur image dans les
composantes globales est une surjection : des chemins extérieurs peuvent identifier
plusieurs morceaux. Le futur plateau doit résoudre toutes les traces puis dédupliquer
les racines globales. Le quotient local quadratique de la référence constructive n'est
donc pas nécessaire ici ; aucune équivalence octet pour octet de ses représentants n'est annoncée.

## Capacités, travail et transactions

C(m,t) est calculé avant parcours. À chaque étape le produit est élargi en u128 :
un accumulateur u64 multiplié par un facteur u32 reste strictement sous 2^96.
La division est exacte ; un dépassement u64 ou de 2C donne `tower_capacity` avant
énumération. Aucun quota supplémentaire de coquille ou de traces n'est introduit.
Le coût reste combinatoire, sans borne globale favorable.

Les cellules étendues non analytiques font deux passes déterministes, comptage puis
remplissage. Cardinalités et compteurs des deux passes doivent coïncider, sinon
`tower_invariant`. `combinations` vaut C ; `passes`, `trace_tests`, `meb_calls` et les
six compteurs MEB publient leur somme réellement exécutée. Les additions sont contrôlées ;
leur dépassement peut refuser après du travail, toujours sans résultat partiel.
Pour t sites, la MEB actuelle visite au plus `sum(binomial(t,q),q=1..min(4,t))`
présentations, avec au plus t tests de points par présentation.

Une seule allocation contient S traces retenues : `sizeof(CellTrace)*S`, soit 52S
octets sur l'ABI G4 visée. Les temporaires sont de taille fixe, au plus douze sites.
Ce stockage est admis après le comptage, avant l'allocation ; il n'existe pas de tableau
de C candidats ni de DSU local. Pour U réservations préexistantes stables, le pic des
réservations sur succès vaut `max(ancien_pic,U+sizeof(CellTrace)*S)`, hors états fixes/RSS.
Le domaine et les autres cellules restent intacts après tout refus.

## Portes et portée actuelle

Les tests natifs préparés couvrent q2/q3/q4 réguliers, carré avec/sans centre,
qmin4/m5, coquille14 à K1, parties jusqu'à K12, grands bits, tri/padding, fenêtres,
compteurs, allocation unique refusée, cohabitation et lectures concurrentes avec budgets privés.
La localisation couvre support local q3 devenu global q2, saturation, q1 et MEB q3/q4
légitimement absentes de CatK. Les mutants visent séparément ces décisions.

Le juge cellules classe par faisabilité barycentrique fermée Gram/Fraction et Carathéodory,
indépendamment du test MEB(A) du produit. Il réutilise le modèle MEB seulement pour les
compteurs. Son modèle normal et `-O` passe : 51 requêtes, 1 598 cellules et 2 533 traces
par profil ; au total 236 328 contrôles, 48 corruptions rejetées et trois JSON invalides.
La ligne de treize sites fournit deux traces strictes de cardinal douze, sans padding.
Ce sont des tests Python sans natif. Les sources/testeurs et refus restent à qualifier
ensemble sur G4 ; ni ces comptes ni les résultats R2 ne qualifient FULL.
