# Phase A de FULL : graphe temporel exact et racines à coût borné

23 septembre 2026. Lecture du produit `ec6d1b74`,
`src/tower/forest/full_ball_tower.hpp` SHA-256
`77588b5d1880c60aa881fdc6ffb1e847494f9e8a4a194b12a0885ec311ebf865`.
**Proposition mathématique et architecturale, non implémentée dans le produit.**
Elle vise la partie séquentielle de chaque ordre K, après résolution exacte
des facettes. Elle ne réduit pas à elle seule les candidats q3/q4, les
recherches MEB ou la preuve des clés entièrement omises.
La [contrelecture B du parallélisme au sein d'un
lot](PHASE_A_FULL_LOTS_ET_PARALLELISME_20260923.md) décrit l'interface
thread-safe immédiate ; les mesures ci-dessous quantifient sa portée sur
une trame et motivent une formulation entre niveaux.

## Un graphe dont les coupes donnent exactement les lots

Fixer un ordre K. Créer un sommet pour chaque bloc `(K, BallId)` de
`programs[K]` et, pour K=1, un sommet initial pour chaque site du domaine.
Chaque facette qu'énumère `visit_block_at` donne une arête non orientée du
bloc vers son représentant : le site initial pour K=1, le BallId cible de
`static_targets` pour K>1, **après** la résolution géométrique terminale
de cette cible. Son poids est le **niveau exact du bloc source**.
La même `BallData` peut être bloc dans plusieurs ordres : ses sommets
`(K, BallId)` restent alors distincts, même si les coordonnées coïncident.
Comme `programs[K]` est déjà trié, un rang de plateau exact peut encoder
ce poids pour l'algorithme de graphe, sans comparaison flottante ni copie
du niveau rationnel dans chaque arête.
Le contrôle produit exige que la cible soit de niveau strictement inférieur ;
le graphe est donc activable par niveaux. Un bloc sans facette est un sommet
isolé à sa naissance. Les sommets de K différents ne sont jamais mêlés.

Juste avant un niveau λ, contracter les composantes des arêtes de poids
strictement inférieur à λ. Les blocs nouveaux de niveau λ et ces anciennes
composantes forment alors un graphe biparti. Deux blocs du même lot se
retrouvent ensemble exactement lorsqu'ils partagent directement ou par
transitivité une racine antérieure : c'est la DSU de `order_lot`. Par
induction sur les niveaux, les composantes des arêtes de poids ≤λ sont donc
les composantes vivantes après le lot λ. La fermeture du plateau entier est
obligatoire : une union arête par arête à poids égal fabriquerait des fusions
intermédiaires que FULL n'émet jamais.

Cette égalité des partitions ne suffit pas à elle seule à reconstruire FULL.
Pour chaque composante du plateau qui contient un bloc nouveau, recueillir
les **racines distinctes d'avant λ**, trier leurs IDs et recueillir les
contributions de tous ses blocs dans l'ordre de `programs[K]`. Zéro parent
donne une naissance valide seulement sous la garde actuelle, un parent une
continuation, plusieurs parents une unique fusion. Les groupes sont publiés
par premier bloc du plateau, ce qui retrouve l'ordre des actions, les IDs
canoniques, `current.next` et les ancres historiques. Les actions à un seul
parent mais avec contribution restent émises ; les blocs inertes restent
comptés. Les validations `target_not_strict`, `anchor_missing`, naissance
distincte et dépassement u32 ne doivent pas disparaître.

Une forêt couvrante **minimale** du graphe pondéré conserve, à chaque seuil
λ, les mêmes composantes que toutes les arêtes de poids ≤λ : sinon une arête
du préfixe joindrait deux composantes de la forêt et remplacerait une arête
plus lourde sur leur chemin. Elle peut réduire la reconstruction des
composantes à O(sommets) arêtes, sous réserve de construire cette forêt
efficacement. Ses arêtes du plateau suffisent aussi à retrouver les parents :
chaque ancienne composante touchée doit être raccordée par au moins une
arête retenue au nouveau groupe. Il faut mapper la cible à sa racine au
**seuil ouvert** `λ−`, pas à la racine déjà fusionnée à λ. Les égalités de
poids n'autorisent toujours aucune fusion
intermédiaire publiée ; les contributions de tous les blocs doivent rester.
Un arbre couvrant quelconque ne suffit pas : avec deux sites initiaux A/B,
un bloc X au niveau 1 relié aux deux et un bloc Y au niveau 2 relié aux
deux, l'arbre `{X–A,Y–A,Y–B}` reporte à tort la fusion de 1 à 2.
La résolution des facettes, la construction de la forêt et la restitution
des actions canoniques ont chacune un coût réel. Un algorithme parallèle qui
rescannerait toutes les arêtes à chaque niveau ne fermerait pas le verrou.
La voie statique possède déjà `static_targets` par ordinal de facette ;
elle pourrait réutiliser ce tableau comme extrémités d'arêtes. Il lui
faudrait néanmoins des offsets source par bloc, les niveaux et des espaces
de travail de composantes. L'indexation u32 locale du reçu ne justifie pas
un u32 silencieux aux dizaines de millions de points : les refus de
représentation et les tailles doivent rester explicites.

## Variante plus proche du code : tête physique et jeton historique

Le code actuel cherche la racine de `anchors[target]` dans `compressed`
avec compression de chemin. Ce tableau est mutable, donc une simple mise en
parallèle de `order_block` créerait une course. Une variante **séquentielle
par lots**, à mesurer avant tout port, garde chaque `anchors[ball]` comme
jeton historique et chaque `current.next` inchangé. Quatre tableaux u32 par
nœud historique suffisent : `head[node]`, `next_member[node]`,
`size[head]`, `owner[head]`. Les listes des têtes partitionnent les nœuds,
et `owner[head]` est l'ID canonique vivant. Une recherche lit
`owner[head[anchors[target]]]`, sans poursuivre une chaîne.
Cette séparation est nécessaire à la phase C : si l'ancre d'une boule
fermée au niveau 1 devient la racine de sa fusion au niveau 2, une image
verticale demandée à la coupure 1 part d'un nœud futur et viole la
précondition de `root_at`. Même une continuation conserve l'ID du nœud
canonique **au niveau du bloc** ; seule la tête physique peut évoluer.

À une multifusion, choisir la plus grande liste physique, réétiqueter tous
les nœuds des petites listes vers sa tête, les concaténer, y ajouter le
nouveau nœud canonique et changer `owner`. Les lectures du plateau sont
figées **avant** ces écritures. Un nœud réétiqueté depuis une liste de taille
s rejoint une liste d'au moins 2s ; il subit au plus ⌊log₂ N⌋ changements.
Le coût de cette **gestion des racines** est O(R+N log N), R étant le nombre
de recherches et N celui des nœuds historiques ; les lots, facettes et
sorties restent à payer. C'est une borne de travail, non une preuve de
gain : deux lectures indirectes, des réétiquetages et leurs défauts de cache
peuvent coûter plus que les chemins compressés actuels. Cette variante ne
parallélise pas les niveaux ; le graphe temporel ouvre l'étude d'une
construction de composantes par coupes sur CPU/GPU.

## Échelle observée et porte de décision

Le [reçu R7b](../receipts/g4_tower_r7b_20260923/README.md), 08/000000
sans sol, K10/s8/W48, paquet `8e8b83a3`, compte 5 512 670 boules,
17 389 031 représentants, 237 727 lots groupés **agrégés sur les dix
ordres**, et 4,062 s de tour CPU G4 dans `vm/probe_4.stdout`. Il compte
1 638 573 nœuds à K10 et 7 426 215 nœuds sur K1..10. Quatre u32 par
nœud représentent respectivement 26 217 168 et 118 819 440 octets
logiques, contre 13 108 584 et 59 409 720 octets pour `compressed` u64 ;
le surcoût logique est 12,5 et 56,7 Mio, hors capacités et co-résidence.
Le reçu ne publie ni tailles de plateaux, ni sauts de racine, ni temps A
par K. La [coordination du développeur](../../audits/COORDINATION_MORSEHGP3D_V9.md)
annonce une sonde locale K10 : 2,85 Gcycles de blocs, dont 1,49 Gcycles
pour 3,8 M recherches de racine, et 1,80 Gcycles de lots. Même annuler
entièrement les recherches ne retirerait que 32,0 % de ces 4,65 Gcycles
de phase A ; ce calcul local n'est pas un chrono G4 ni une borne d'un
nouvel algorithme.

Une instrumentation **hors produit** du même header sur l'entrée locale
de SHA-256 `0baa4de1…`, correspondant au reçu R7b, observe pour K10
**2 117 675 blocs** et **3 831 431 facettes/racines**. Les
**2 081 320 lots singletons** dominent ; les **16 336 lots groupés**
contiennent seulement **36 355 blocs, soit 1,72 %** des blocs, et leur
taille maximale est huit. Paralléliser seulement les blocs *d'un même
lot* ne viserait donc qu'une petite fraction de ce K10 LiDAR. Les
recherches actuelles traversent **11 865 867** liens de `compressed`
(3,10 par requête) et écrivent **9 029 520** raccourcis. La variante à
têtes physiques, simulée en observation sur ces mêmes événements,
réétiquetterait **3 940 560** nœuds, au plus dix fois un même nœud.
Ses **3 831 431** réponses de racine concordent en ligne avec le code
actuel ; statut, digest et résumés de tour coïncident, sans comparaison
octet à octet des forêts ni chrono causal. La source instrumentée est
`ec6d1b74`, tandis que R7b exécute `8e8b83a3` : cette égalité de résumés
ne transfère pas le reçu G4 au sidecar. Cette première mesure rend
la réduction des sauts plausible, mais ne départage pas encore le coût
des lectures indirectes et des réétiquetages.

Le même comptage montre le rétrécissement des plateaux aux ordres élevés :

| Ordre | Blocs | Blocs de lots groupés | Part | Sauts moyens par recherche |
| ---: | ---: | ---: | ---: | ---: |
| K1 | 101 138 | 61 680 | 60,99 % | 1,88 |
| K5 | 789 886 | 64 286 | 8,14 % | 2,74 |
| K10 | 2 117 675 | 36 355 | 1,72 % | 3,10 |

Ces fractions portent sur un seul nuage et une seule séquence ; elles ne
prouvent aucune loi de croissance. Elles rendent néanmoins la porte de
parallélisme au sein d'un plateau peu prometteuse **pour ce K10**, tandis
que les sauts de racines augmentent avec K.

La [provenance compacte du
sidecar](phase_a_20260923/MANIFEST.md) conserve les patchs applicables
au pin, le harnais, les dix comptages et les six résumés.
Le catalogue exact figé de **1 141 122 698 octets** a le SHA-256
`c9829439ca819b48f5f0dc46cb7f5cdf7b4b4cae876c7881397e4c77fb461563` ;
il n'est pas joint au dépôt. Les sorties canoniques portent toutes le
SHA-256 `ea3c642bbcfe22ec28b8b7ce0474f85b3296eb19574612b05f91622fb7a0e31a`.

Une ablation locale appariée sur le **même catalogue figé** construit la
tour K10 avec le header de référence A et la variante minimale à têtes
physiques B, en ordre A/B, B/A, A/B. Les six constructions rendent
`complete_relative` et les mêmes compteurs ; chaque B est **identique
octet par octet** à A sur une sérialisation de **1 056 931 646 octets**
comprenant domaine et lignes de la banque, tous les nœuds, niveaux,
parents, successeurs, contributions et images verticales des dix ordres.
Les temps du seul `build_full_ball_tower` mesurés par ce harnais local sont
A **13,074 / 13,159 / 13,542 s**, B **13,195 / 13,080 / 13,817 s** ;
le RSS maximal de B est supérieur de **54–159 Mio** selon la paire.
L'hôte est partagé, ces temps ne sont ni G4 ni une ablation de chaîne,
et trois paires ne démontrent aucune distribution. Elles ne montrent
**aucun gain stable** de la tête physique sur ce LiDAR ; ne pas la porter
sur cette seule intuition. L'égalité des sorties soutient plutôt l'étude
du graphe temporel, où le but est de dépasser les niveaux séquentiels.
Une sonde supplémentaire avec le **même chronomètre CPU par ordre** dans
les deux headers mesure K10 phase A à **2 179,7 ms** pour A contre
**2 290,3 ms** pour B, toujours avec sortie octet à octet identique.
Cette paire locale isolée renforce l'absence de signal favorable ; elle
ne constitue pas un profil G4 ni une borne pour d'autres nuages.

Avant un port dans le produit, publier par K le nombre et la taille des
plateaux, les recherches/sauts/lectures de racine, les réétiquetages et
la taille maximale d'une composante, les cycles blocs/racines/lots, les
capacités simultanées et le RSS. Répéter sur **les mêmes catalogues**
ancienne DSU, têtes physiques et éventuelle voie graphe, avec les actions,
IDs, parents, contributions, ancres, images verticales et banque FULL
sérialisés entièrement, statuts compris ; un seul digest FNV ne suffit
pas. Les fixtures doivent inclure les trois sites collinéaires équidistants
(un seul plateau de fusion à trois parents), une même boule utilisée comme
bloc à K1 et K2, une facette dont la cible immédiate est échangée pour sa
cible terminale, un bloc sans facette avec contribution, une continuation
et un contact de niveau. Tuer des mutations « union en ligne dans le
plateau », cible immédiate et réétiquetage omis par les actions ou facettes
ultérieures
de la branche déplacée. Refaire Release, ASan/UBSan et les échecs
d'allocation des nouveaux tableaux avant de comparer K5/K10 sur trames
entières, puis sur plusieurs séquences et trames brutes avec sol. Aucun
gain sous-quadratique, GPU ou contrat 1 s n'est acquis ici.
