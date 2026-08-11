# Audit de réemploi — EMST exact par graphe Yao48

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Réponse d'architecture

La lane `k=1` n'a pas besoin de relancer une recherche géométrique pour chaque
point et chaque ronde de Borůvka. La ligne enregistrée contient déjà le lemme
plus fort et ses deux différentiels : les plus proches voisins exacts dans les
48 chambres Yao contiennent l'EMST canonique du graphe complet. Le graphe
dirigé possède au plus `48n` arêtes avant déduplication.

Ce résultat est réutilisable en v3 comme théorème, oracle et blueprint d'un
producteur sparse. Il n'est pas une preuve de débit : son ancien prototype CPU
LBVH a été rejeté. La ligne enregistrée le maintient d'ailleurs comme oracle
hors ligne et réduit directement son propre flux exact de paires pour `k=1`.
La v3 hors registre peut explorer une autre architecture, sans promouvoir ni
modifier ce statut enregistré.

## 1. Théorème exact

Après choix des signes et ordre décroissant des valeurs absolues, chaque
chambre est isométrique au cône `x>=y>=z>=0`. Son diamètre sphérique vaut :

$$\arccos\left(\frac{1}{\sqrt{3}}\right)<\frac{\pi}{3}.$$

Sous le preflight du profil initial qui impose des positions 3D deux à deux
distinctes, pour chaque point `u` et chambre non vide, choisir l'arête `uw`
minimale selon la clé totale
`(distance_squared,min_PointId,max_PointId)`. Soit `G_Y` l'union non orientée
de ces arêtes. L'EMST obtenu par Kruskal canonique sur le graphe complet est
contenu dans `G_Y`.

En effet, si une arête canonique `uv` absente de `G_Y` était choisie par
Kruskal, la chambre de `v` en `u` contiendrait un `w` tel que `uw` précède ou
égale `uv` dans la clé canonique. Comme l'angle `vuw` est strictement inférieur
à 60 degrés et `|uw|<=|uv|`, la loi des cosinus donne `|wv|<|uv|`. Les arêtes
`uw` et `wv` sont donc toutes deux traitées avant `uv` dans le graphe complet;
elles relient déjà `u` à `v`, contradiction.

La stricte inégalité angulaire est essentielle. Les frontières de chambres
peuvent être semi-ouvertes, mais les égalités de distance doivent être fermées
par la clé canonique : un premier point rencontré par hasard ne certifie pas
l'EMST canonique.

Le vecteur nul n'a pas de chambre directionnelle. Le contrat courant rejette
les positions dupliquées; sous une future politique
`duplicate_policy=aggregate`, chaque position devient un site canonique portant
une multiplicité. Yao-1 s'applique alors aux sites distincts et aucune étoile
nulle de `PointId` n'existe dans ce graphe. Les effets des multiplicités sur les
rangs constituent un contrat séparé.

Le lemme suivant concerne seulement une éventuelle sémantique hors contrat qui
conserverait un `PointId` par occurrence. Soient `C` les classes de même
position, `r(C)=min C`, `m` leur nombre et
`kappa=(distance_squared,min_PointId,max_PointId)` la clé canonique. À poids
nul, Kruskal accepte exactement les arêtes `{r(C),v}` pour
`v in C\{r(C)}` : elles ajoutent chacune un sommet, puis toute autre arête
nulle ferme un cycle. Entre deux classes `C,D`, toutes les arêtes ont la même
longueur et `{r(C),r(D)}` est leur minimum unique selon `kappa`; après sa
décision, les autres relient les mêmes composantes quotient et sont
redondantes.

Le suffixe positif est donc exactement le Kruskal canonique sur les
représentants minimaux. Leurs positions sont distinctes, donc Yao-1 contient
cet EMST quotient. L'arbre occurrence-preserving est l'union des étoiles
nulles, publiées dans le lot de niveau zéro et triées par `kappa`, et de l'arbre
quotient. Avec `n` occurrences, il contient au plus
`(n-m)+48m<=48n` candidats avant réduction; le ledger directionnel porte
`48m` slots, pas `48n`. Affecter arbitrairement un vecteur nul à une chambre ne
prouve aucun de ces faits.

## 2. Prior art réellement présent

Le contrat et l'oracle exhaustif borné sont dans
[`yao48_emst.hpp`](../../morsehgp3d/include/morsehgp3d/hierarchy/yao48_emst.hpp).
Ils balaient toutes les paires jusqu'à `n=4096`, publient le transcript complet
chambre--candidat--déduplication--Kruskal et le rejouent sans partager les
prédicats du producteur.

Le prototype accéléré est dans
[`exact_lbvh_yao48_emst.hpp`](../../morsehgp3d/include/morsehgp3d/hierarchy/exact_lbvh_yao48_emst.hpp)
et son implémentation CPU. Il effectue un parcours LBVH par source, partage les
48 chambres par masque, ne prune que sur une borne strictement supérieure,
descend sur égalité, déduplique les arêtes puis applique Kruskal. Sa
qualification bornée compare le transcript à l'oracle Yao et l'arbre à un
Borůvka LBVH indépendant.

Le registre donne le verdict exact suivant : Release et AddressSanitizer hôte
passent, mais le chemin CPU produit est rejeté. Sur `uniform_latin`, `n=1000`
et fenêtre Morton 32, la recherche seule vaut 31,129001876 s; à 50 k elle
expire proprement après 60 s sans résultat. Ce code reste un blueprint GPU et
un oracle, jamais un reçu 50 k ni une preuve de SLO. Les faits sont scellés
dans
[`implementation_status.toml`](../../docs/implementation_status.toml) sous
`phase15_exact_lbvh_yao48_*`.

## 3. Condition de mutualisation avec q2

Une banque q2 de dix témoins arbitraires ou une antichaîne de masse ne fournit
pas un plus proche voisin Yao. Même une banque remplie par best-first ne suffit
pas si elle s'arrête sur un budget avant de fermer les égalités.

Pour réutiliser le travail q2, chaque couple `(point,chambre)` doit publier
exactement l'un des deux reçus suivants :

- `(target_PointId,distance_squared)`, avec preuve qu'aucun nœud compatible
  non visité n'a une borne inférieure plus petite, ni la même distance avec une
  clé d'arête canonique antérieure;
- `empty`, seulement après épuisement certifié de tous les nœuds compatibles.

Un budget interrompu, une patience épuisée ou une chambre q2 sous-pleine ne
prouve jamais `empty`. Cette obligation est indépendante du cutoff strict
q2 : les mêmes tuiles, masques de chambres et parcours peuvent être partagés,
mais les reçus et les décisions restent séparés.

Le ledger ne se réduit pas à une somme globale. Sur le profil courant distinct,
il exige exactement un statut par clé `(source_PointId,chambre)`, sans doublon
ni omission, puis ferme `nonempty_slots+empty_slots=48n` avec `incomplete=0`.
Pour un candidat, le minimum de toute frontière compatible restante doit être
strictement supérieur à sa distance; l'égalité descend pour fermer le
tie-break `PointId`. Le reçu authentifie source, cible, chambre, distance et clé
d'arête; `empty` exige l'épuisement de toute la frontière compatible. Après
déduplication, `unique_edges<=nonempty_slots`, puis l'arbre possède exactement
`n-1` arêtes, est acyclique et connexe. Un tas qui départage les ex æquo par
position Morton ne fournit donc pas encore le transcript canonique.

## 4. Route v3 proposée

1. Extraire en parallèle le transcript Yao-1 exact pendant le parcours tuilé
   des banques q2, avec fermeture canonique des ex æquo.
2. Émettre par `count--scan--fill` au plus `48n` candidats dirigés et des
   offsets 64 bits; dédupliquer les arêtes non orientées par la clé
   `(distance_squared,min_PointId,max_PointId)`.
3. Réduire ce graphe sparse par Kruskal canonique ou Borůvka sur arêtes. Une
   recherche point--LBVH répétée à chaque ronde n'est plus admise dans la
   route industrielle.
4. Trier les `n-1` arêtes finales par niveau puis `PointId`, et appliquer chaque
   lot de même niveau atomiquement. Une suite dans l'ordre des rondes de
   Borůvka n'est pas un payload de niveaux valide.
5. Rejouer à petit `n` le transcript Yao complet, l'arbre, puis les partitions
   strictes et fermées à chaque niveau contre l'oracle quadratique. Les mutants
   minimaux omettent une chambre, tronquent une égalité, déclarent une chambre
   vide après budget, changent le tie-break, perdent une arête ou cassent un
   lot d'égalité.

Cette route conserve `O(n)` nœuds LBVH et `O(48n)` candidats, sans Delaunay,
matrice de paires, cellule d'ordre supérieur, coface ou incidence globale. Elle
ne devient intéressante pour la seconde G4 que si l'extraction Yao-1 est
mutualisée avec q2 et si ses compteurs ferment une gate dédiée; le port littéral
du prototype CPU enregistré est déjà réfuté.

Le statut du prototype v3 courant, de ses tests et de son harnais appartient
exclusivement à [l'audit live](AUDIT_ETAT_COURANT.md).

GCP non utilisé pour cet audit.
