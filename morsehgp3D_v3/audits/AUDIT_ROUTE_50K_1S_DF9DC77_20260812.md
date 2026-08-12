# Audit épinglé de la route exacte 50 k sous une seconde

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot source suivi : `df9dc7768156cfb24cf8e011f55f215115b22ca1`.
Le prototype concurrent non suivi de localité certifiée est hors de ce
pincement; son dernier hash et sa réfutation sont tenus dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Verdict

Aucun chemin courant ne peut mesurer, et encore moins qualifier, le contrat
50 k sous une seconde. Le harness horizontal ne calcule qu'un LBVH CPU, un
EMST brut et des comptes q2. Le contrat secondaire porte pourtant sur le même
`BenchmarkOutputContract-v1` que la cible principale : transfert, index,
source exacte, census, q3/q4, resolver, fold, dix forêts, verticales, lots,
certificat minimal et retour hôte sont tous dans le chronomètre.

Le frein q2 observé n'est plus la taille de la sortie. La traversée duale
ramène les survivantes et le census près d'un régime linéaire, mais réévalue
une frontière témoin ambiguë `656 millions--1,313 milliard` de fois à 50 k.
Le premier P1a q4 présente ensuite un autre rescan racine, presque quadratique
dès 8 k. Porter littéralement ces deux ordonnances sur CUDA ne peut pas établir
la seconde.

La route d'implémentation la plus justifiée est une cascade exacte : banques
Yao, certificat affine direct sur les banques déjà chaudes, dual-tree seulement
sur les plages non résolues, puis classifieur/census terminal. Elle doit en même
temps produire le transcript Yao-1 de `k=1`. Ce document prouve les décisions
locales de cette cascade; il ne prédit ni sa couverture ni sa latence.

## Ce qui est réellement mesuré

| tranche | observation reçue | conséquence |
| --- | --- | --- |
| cellules G4 mass-only | q2 : 465 M--2,86 G; q3 : 14,7 G--131,8 G; q4 : 330 G--9,97 T après prune; aucun tuple formé | l'ordonnance de cellules mesurée est refusée |
| Yao48 CPU initial, q2 seul | environ 0,99--1,56 G tests de boîtes et 0,63--0,79 G tests ponctuels à 50 k | le port direct ne réduit pas assez le travail |
| dual persistant, q2 seul | 2,44--5,79 M survivantes, 1,03--1,23 M entrées de census, mais 656 M--1,313 G visites témoins | la sortie est compacte; l'ordonnance de recherche est rouge |
| P1a q4 mass-only | à 8 k terrain : 181 460 408 visites, 5 017 937 282 tests point--patch, 20 267 313 188 coins, 273,673 s | le rescan par bloc est refusé avant G4 |
| Yao48 GPU enregistré | frontière rang 11 : 2,434 s; recertification hôte : 8,628 s; aucun q3/q4/fold | le prior art est mécanique, pas une qualification v3 |

Les temps CPU ont été produits sous charge partagée et ne sont pas des
benchmarks SLO. Les compteurs déterministes suffisent néanmoins à falsifier les
ordonnances. Les sources sont les reçus indexés dans
[`README.md`](README.md), notamment les audits dual, P1a et G4 mass-only.

## Trous à fermer avant toute comparaison de performance

1. `work_done()` omet les compteurs duals; la fusion et l'égalité shardées
   omettent neuf champs, dont les tests ponctuels. Une accélération peut donc
   seulement déplacer du travail invisible.
2. Aucun `DualReceipt` ne lie ancre, epoch LBVH, plage cible, sous-ensembles
   témoins, masses et bornes. Les sorts globaux peuvent être exacts sans que
   le transcript annoncé soit rejouable.
3. `BankTableEntry.engaged` est mutable et partagé. La voie radiale ne
   sérialise pas son masque. Un reçu tardif peut donc changer le sens d'un reçu
   antérieur.
4. Une feuille partiellement créditée est retirée entière. Ses points encore
   ambigus peuvent devenir témoins après le split de la cible : la sortie reste
   fail-open, mais un prune descendant est perdu.
5. Le cap de frontière ne borne pas l'arène append-only, n'a pas de tie-break
   canonique, manque à la provenance et n'a pas de fixture dédiée.
6. Le LBVH dit « device » utilise encore `std::sort` et une construction
   récursive hôte avec rescans AABB. Aucun coût de transfert ou de résidence
   device n'est reçu.
7. Le diagnostic horizontal fusionne des ledgers scalaires, pas les sorts et
   reçus complets; cinq répétitions sans warmup et un percentile interpolé ne
   suivent pas le protocole officiel à trente répétitions chaudes.

Ces corrections ne sont pas du travail administratif : sans elles, les deux
pentes et la mémoire ne sont pas comparables entre deux implémentations.

## Cascade q2 exacte proposée

### 1. Banque Yao immuable

Une banque top-nearest certifiée conserve exactement dix candidats. Un
réservoir arbitraire peut conserver onze candidats pour exclure une cible
ponctuelle, puis engager dix identifiants distincts. Pour une plage cible `Q`,
un même masque de dix n'est valable que si la banque rencontre `Q` en au plus
un `PointId`; sinon il faut scinder `Q`, choisir une banque disjointe ou échouer
ouvert. Chaque reçu porte `(bank_version,engagement_mask)` et le juge recalcule
la disjonction.

La coupe Yao et son enveloppe radiale sont des préfiltres exacts. Une égalité,
une chambre sous-pleine ou une identité incertaine descend au stade suivant.

### 2. Certificat affine direct

Pour une ancre `p` et un témoin ponctuel immuable `w`, poser
`h_w(q)=(q-p) dot (w-p)-||w-p||^2`. Cette forme est affine en `q`. Son minimum
exact sur une boîte entière choisit, sur chaque axe, la borne basse si le
coefficient `w-p` est positif et la borne haute s'il est négatif.

Dix minima strictement positifs certifient dix témoins intérieurs pour toute
cible réelle de la boîte. Pour la même banque engagée, ce test domine la coupe
Yao : toute réussite Yao implique les dix inégalités affines, tandis que la
réciproque n'est pas requise. Les dix identifiants doivent être distincts,
différents de `p` et disjoints de tous les identifiants de la boîte cible.

Essayer naïvement les 48 banques coûte jusqu'à 480 tests affines par boîte.
L'implémentation doit donc propager le masque de chambres, ordonner les banques,
court-circuiter au premier échec et envoyer immédiatement le résidu au dual.
Une table globale `50 000*48*11` d'identifiants u32 coûte 105 600 000 octets,
soit 100,71 Mio avant masques et provenance; une banque par tuile est le point
de départ raisonnable, non une obligation mathématique.

### 3. Majorant dual entier exact

Le majorant de rejet courant remplace le maximum entier de `u*v-v^2` par un
arrondi continu. Pour chaque axe, le maximum exact sur les intervalles entiers
est :

$$M=\max_{u\in\left\lbrace u_{\min},u_{\max}\right\rbrace,\ v\in V(u)}\left(uv-v^{2}\right),\qquad V(u)=\left\lbrace v_{\min},v_{\max},\mathrm{clip}(\lfloor u/2\rfloor),\mathrm{clip}(\lceil u/2\rceil)\right\rbrace.$$

Pour `v` fixé, la fonction est affine en `u`, donc son maximum est à une
extrémité. Pour `u` fixé, elle est concave en `v`, donc son maximum entier est
aux voisins bornés de `u/2`. La somme des trois `M` est le maximum exact AABB.
Le `ceil(u^2/4)` courant est trop grand d'une unité pour `u` impair; le
remplacement peut transformer des ambiguïtés en rejets `U<=0` sans faux prune.

### 4. Feuilles résiduelles et partage structurel

Chaque feuille témoin porte trois masques disjoints : accepté (`L>0`), rejeté
(`U<=0`) et ambigu. Seul le masque ambigu est raffiné. Les crédits positifs
sont hérités; les points du sibling de la cible redeviennent admissibles après
le split et doivent être présents exactement une fois. Une feuille ne disparaît
pas parce qu'un seul de ses points a été crédité.

L'arène utilise des watermarks et un rollback DFS. Dès que la masse dix est
atteinte, aucun suffixe de frontière mort n'est copié. Une feuille cible sans
descendant restaure son checkpoint. Un microtile de 32 nœuds cibles contre un
nœud témoin produit trois bitmasks accepté/rejeté/ambigu et ne splitte le nœud
témoin qu'une fois pour toutes les lanes actives.

Cette machine vise directement les retests rouges; une réécriture de layout
seule ne le fait pas.

### 5. Transcript Yao-1 mutualisé

Le parcours de banques possède déjà l'information directionnelle nécessaire à
`k=1`, mais le contrat est plus fort. Chaque couple `(PointId,chambre)` finit
en `exact_first_neighbor` canonique après fermeture de tous les ex æquo, ou en
`certified_empty` après épuisement exact. Un cap ou une interruption donne
`incomplete`, jamais `empty`.

Après déduplication, le graphe contient au plus `48n` arêtes dirigées. Un
Kruskal/Borůvka sparse produit l'EMST, trie les `n-1` arêtes finales par niveau
et groupe les égalités atomiquement. Cela retire les requêtes point--LBVH
répétées du Borůvka CPU sans construire le graphe complet.

## q3/q4 après q2

Le q2 rapide ne suffit pas à la seconde. Avant les 64 patches de P1a, le cœur
universel de Jung doit traiter les blocs d'ancres. Pour une ancre maximale et
un témoin, les prédicats q3/q4 déjà prouvés se bornent sur les huit coins d'une
boîte cible en arithmétique `i128` sous u16. Le résidu seul entre dans un état
persistant `(pair_block,W,patch_mask)` avec masques accepté/rejeté/ambigu par
patch et borne de masse encore atteignable. Si aucun patch ne peut atteindre
le seuil neuf ou huit, le bloc est scindé immédiatement au lieu de rescanner la
racine.

Le probe P1a courant n'émet ni paire, ni ancre, ni support. Les range-reports
q3, les niveaux shallow q4, `BallActivation`, le resolver latent, le fold et le
cas terminal `k=n` restent à construire. Aucun résultat q2 ne qualifie donc le
pipeline complet par lui-même.

Le certificat concurrent par calottes inversées ne remplace pas cette route.
Il est valable localement pour une ancre certifiée, mais toute ancre extrême de
l'enveloppe convexe possède une direction sortante couverte par aucune calotte
stricte. Exiger toutes les ancres certifiées rend donc sa voie globale
impossible sur un nuage fini. Il peut seulement devenir un prune partiel avec
owner certifié et résiduel exact, idéalement limité aux directions de boîtes
cibles effectivement possédées. Sa masse résiduelle doit être mesurée avant
toute extension q3/q4.

## Fixtures et compteurs d'admission

Avant une nouvelle rampe, les portes permanentes minimales sont :

- banque utilisée par deux reçus dans les deux ordres, avec masques différents;
- boîte cible contenant zéro, un puis deux membres d'un réservoir de onze;
- reçu radial antérieur à tout reçu Yao ponctuel;
- feuille partiellement créditée dont un point ambigu devient témoin sur un
  enfant;
- maximum entier avec `u` impair et clips sur chacun des deux bords;
- fusion 1/2/N shards comparant tous les sorts, reçus et compteurs duals;
- cap et effacement qui doivent rendre les mêmes sorties que la voie non capée;
- mutant EMST `level-off-by-one` à 5 000 points, sans dépendre de l'oracle Prim.

Les compteurs d'échelle incluent au minimum banques tentées/réussies, tests
affines, splits d'exclusion, nœuds et points duals, scans/copies, allocations,
octets et high-water, travail du classifieur/census résiduel et candidats
Yao-1. Les quatre familles sont mesurées à `12 500/25 000/50 000` dans un même
ELF auto-authentifié. Deux pentes complètes doivent être admissibles avant le
port device; ce passage n'est pas une preuve du SLO.

La qualification finale emploie trente répétitions chaudes à 50 k et arrête le
chronomètre seulement après le retour hôte du `BenchmarkOutputContract-v1`.

GCP non utilisé pour cet audit.
