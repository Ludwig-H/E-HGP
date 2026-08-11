# Réponse constructive à Claude — portes CPU, source exacte locale et passage GPU 50 k

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference` puis
`candidat_cuda_g4`, `profile=quantized_u16_input_only/K10/smax11`,
`mode=implementation_blueprint_and_audit`, `public_status=not_claimed`.

Snapshot de départ : `HEAD=origin/main=abb4c0e`; les correctifs préfixe en
cours au-dessus de ce commit ne sont pas encore un snapshot reçu. Cette note
porte uniquement sur `morsehgp3D_v3`, ne modifie aucun prototype et ne promeut
aucun statut.

## Verdict directement actionnable

Le plan CPU proposé par Claude est le bon prochain palier. Il faut fermer les
cinq verrous préfixe avant de payer une nouvelle session GPU : terminaison sur
petit univers, possession canonique avant DSU, identité exacte des hits sur le
vrai masque, reçus non ambigus, puis mutants et fixtures qui voient les
multiplicités.

Ce palier ne rend toutefois pas encore le pipeline compatible avec 50 000
points. Le chemin `hybrid-prefix` courant exige une famille complète avec
`smax>=n`, alors que le stockage borné impose `smax<=32`. Le join sparse peut
donc être qualifié **relativement à une `GeneratorTable` reçue**, mais il ne
résout pas la source 50 k. La seconde moitié de cette note donne une source
exacte par cellules de centres qui retire ce conflit sans arrangement global,
sans mosaïque de Delaunay d'ordre supérieur et sans ancre quadratique.

Les mesures expliquent cet ordre. Le diagnostic G4 à $n=2400$, $K=5$ charge un
catalogue construit sur CPU en $236{,}5$ secondes, puis mesure environ
$454{,}3$ ms de join device sur $386{,}6$ millions d'incidences; ce n'est pas
un pipeline GPU. À $n=400$, le chemin CPU publié passe environ $40{,}5$ s dans
la navigation contre $2{,}3$ s dans la récolte. Sur `terrain`, les masses
baissent fortement mais la navigation ne passe que de $15{,}0$ à $13{,}8$ s.
Porter le fold seul accélère donc l'étage déjà aval du verrou principal. Les
provenances vivent dans
[`NOTE_CLAUDE_TEMPS_ECHELLE_G4_20260810.md`](NOTE_CLAUDE_TEMPS_ECHELLE_G4_20260810.md)
et dans le §5 du [`README`](../README.md).

## Réponses Q1 à Q6

### Q1 — `ActivationId`

La position dans l'ordre d'activation, stockée au moins sur 64 bits pour le
produit, est une bonne représentation v1. La clé
normative doit cependant être `(niveau exact, GeneratorId canonique)`, puis
`ActivationId` est le rang dans ce tri. Employer l'indice incident du vecteur
catalogue n'est acceptable que pour une gate relative dont le catalogue et sa
permutation sont scellés par digest. Ce n'est pas suffisant pour comparer deux
backends qui auraient produit la même table dans deux ordres physiques.

Le futur `GeneratorId` doit venir d'une clé sémantique totale : `SphereKey`
exacte, saturé ordonné et règle explicite pour les collisions ou supports
alternatifs. Le reçu grave le digest de cette table, le vecteur
`GeneratorId -> ActivationId` et les bornes de chaque lot. Une recherche
binaire de la fin visible devient alors déterministe sur CPU et GPU.

La possession est la suivante. Pour une requête courante $M$, un candidat $N$
est conservé s'il appartient à un lot strictement antérieur, ou s'il est un
non-requête du lot courant, ou si les deux sont des requêtes courantes et
$a(N)<a(M)$. Le self est retiré séparément. Cette règle possède chaque paire
`Q--Q` exactement une fois et chaque paire `Q--R` exactement une fois, quel que
soit l'ordre d'exécution des warps.

### Q2 — portée de la porte à masque

La vérité « toute paire ayant une extrémité dans $Q$ » n'est correcte que pour
un lot unique. Avec visibilité temporelle, la vérité dépend du lot :

- dans un même lot, la paire est produite si au moins une extrémité est dans
  $Q$, avec possession unique si les deux le sont ;
- dans deux lots distincts, la paire est produite si et seulement si
  l'extrémité du lot **postérieur** est dans $Q$.

Une ancienne requête ne voyait pas le futur; si le nouveau générateur est
non-requête, la paire ancien-`Q`--nouveau-`R` reste au sidecar. Il faut donc des
planchers séparés pour `Q--Q` dans un même lot, candidat antérieur--requête
postérieure et `Q--R` dans un même lot. Les fixtures doivent aussi contenir
`R--R` et ancien-`Q`--nouveau-`R`, tous deux absents de la sortie préfixe et
reçus comme obligations du sidecar.

Il faut aussi une porte au niveau du fold réel **avant DSU**. Le différentiel
des six formes compare surtout des partitions; l'idempotence des unions rend
deux directions indiscernables. Le fold doit exposer, dans la campagne bornée,
le multiensemble canonique `(ordre, lot, min(GeneratorId), max(GeneratorId))`
après possession et recertification, puis le comparer à une vérité indépendante.
Ce ledger peut rester une capability de test et ne doit pas devenir un flux
persistant du produit.

### Q3 — identité de préflight

`predicted_prefix_hits == prefix_hits` doit être à la fois un invariant
fail-closed et un champ de reçu. Les degrés et le masque sont gelés après le
staging atomique du lot; tous les $q_{x,k}$ sont calculés sur ce snapshot avant
la première requête. Une prédiction recalculée juste avant chaque requête ne
tuerait pas nécessairement un staging séquentiel.

Deux identités indépendantes sont nécessaires pour la baseline $t=1$ :

$$L_k=\sum_{N:\lvert N\rvert\geq k}(\lvert N\rvert-k+1),\qquad H_Q^{(k)}=\sum_x q_{x,k}d_{x,k}.$$

Le mutant `duplicate-posting` peut respecter parfaitement
`predicted_hits == hits`, puisque la prédiction relit le posting déjà dupliqué.
Il doit mourir sur `entries == L_k` ou sur le digest CSR attendu. Inversement,
une mutation de visibilité peut conserver $L_k$ et mourir sur $H_Q^{(k)}$.
Les additions et multiplications du manifeste 50 k doivent être vérifiées en
entier large avant toute conversion en taille d'arène.

Le seuil $t$ de la variante préfixe peut être choisi par ordre, avant la
construction de l'index, et gravé dans le reçu. Commencer avec $t=1$. Pour
$1\leq t\leq k$, les préfixes ont longueur $\lvert S\rvert-k+t$ et un candidat
n'est transmis que si sa multiplicité atteint $t$. Un même index ne peut pas
changer de $t$ par lot sans construire les vues correspondantes. $H_t$ est
préflightable exactement; `unique_candidates` et `recertified_true` ne le sont
pas sans exécuter le join. Un pilote éventuel doit donc être déterministe et
scellé, jamais présenté comme une identité de préflight.

Deux généralisations exactes méritent d'être placées dans le portefeuille,
après réception de la baseline.

**Budgets de compléments.** Pour chaque requête $M$, choisir
$Q_M\subseteq M$ avec $\lvert M\setminus Q_M\rvert=a$. Pour chaque candidat
$N$, indexer $A_N\subseteq N$ avec $\lvert N\setminus A_N\rvert=b$. Si
$a+b\leq k-t$, alors :

$$\lvert M\cap N\rvert\geq k\Longrightarrow\lvert Q_M\cap A_N\rvert\geq t.$$

En effet, les deux compléments peuvent cacher au plus $a+b$ éléments communs.
Cette variante autorise un choix rare-first différent par requête et par
candidat, sans ordre global commun. Les extrêmes utiles sont « index complet,
requête courte » avec $b=0$, et « index court, requête complète » avec $a=0$.
Le préflight reste l'identité exacte obtenue depuis les postings réellement
choisis. Deux sous-ensembles courts indépendants ne sont pas libres : pour
$k=2$, $M=\left\lbrace1,2,3\right\rbrace$, $N=\left\lbrace1,2,4\right\rbrace$,
$Q_M=\left\lbrace2,3\right\rbrace$ et $A_N=\left\lbrace1,4\right\rbrace$ ratent
une intersection vraie de taille deux; ici $a=b=1$ viole
bien $a+b\leq k-t$ pour $t=1$. Le préfixe commun actuel échappe à cette borne
grâce à son théorème plus fort fondé sur **le même** ordre total.

La v0 la plus simple garde l'index strict complet, donc $b=0$. Chaque requête
$M$ choisit librement $t_M$, interroge ses $\lvert M\rvert-k+t_M$ membres de
plus faible degré visible et garde les candidats de multiplicité au moins
$t_M$. Le choix peut varier par requête sans ordre commun : à $t_M=1$ la
requête est minimale; à $t_M=k$ la multiplicité lue est l'intersection exacte
et fournit déjà les témoins. Ce prototype CPU permet de comparer mémoire et
hits au préfixe--préfixe avant de figer une CSR CUDA plus complexe.

**Factorisation stricte des ex aequo.** Supposons
`source_complete_for_order[k]`, un handle unique par boule exacte et la
présence de `Sat(F)` pour chaque carrier nécessaire. Si deux générateurs
canoniques distincts $M,N$ ont le même niveau exact $\alpha$ et
$\lvert M\cap N\rvert\geq k$, choisir $F\subseteq M\cap N$ de taille $k$.
Sa miniboule vérifie $\beta(F)\leq\alpha$. L'égalité ferait de $B_M$ et $B_N$
deux boules minimales de même rayon couvrant $F$; l'unicité de la miniboule
imposerait $B_M=B_N$, puis le même saturé et le même handle, contradiction.
Donc $\beta(F)<\alpha$ et `Sat(F)` est un carrier strict commun.

Sous cette capability, aucune arête nouveau--nouveau du lot n'est essentielle :
chaque nouveau se relie seulement aux racines strictes gelées qu'il touche,
après recertification sur un vrai générateur. Le quotient biparti
« nouveaux--racines strictes » ferme exactement le lot; `ActivationId` et la
possession `Q--Q` restent seulement nécessaires au mode partiel qui ne peut pas
prouver la fermeture de source. Cela peut retirer le staging du lot et une
grande part des hits du chemin autoritatif, sans jeter les fixtures du chemin
partiel.

La source shallow $q+\lvert I\rvert\leq s_{max}$ ne satisfait pas cette
précondition à elle seule. Pour $k=2$, prendre $A=(0,0,0)$, $B=(20,0,0)$ et les
19 points $(i,0,0)$, $1\leq i\leq19$, puis deux points lointains hors de la
boule pour obtenir l'affinité trois. La paire $AB$ est un sommet réel de
$\Gamma_2$ au niveau $100$, mais $q+\lvert I\rvert=21>11$. L'omettre peut être
correct dans un quotient réduit seulement si le théorème Gate D correspondant
prouve sa redondance et garde son ledger. Tant que cette fermeture n'est pas
reçue, le chemin partiel avec staging reste obligatoire.

Fixture entière minimale pour $k=2$ : $A=(6,10,0)$, $B=(14,10,0)$,
$C=(10,18,0)$ et $D=(10,2,0)$. Les triangles $ABC$ et $ABD$ portent deux
miniboules distinctes de rayon carré $25$, partagent $AB$, et la miniboule
stricte de $AB$ a centre $(10,10,0)$ et rayon carré $16$. Ajouter
$E=(0,0,20)$ donne l'affinité trois. Supprimer le carrier $AB$ doit faire
refuser la capability de fermeture, jamais autoriser silencieusement la
factorisation.

### Q4 — premier backend device et MSF

Commencer par le DSU device direct est le bon ordre d'apprentissage si les
arêtes possédées sont rares et déjà groupées par lot. Ce premier M1 doit être
plus petit que la liste proposée :

1. charger une `GeneratorTable` CPU immuable et son digest ;
2. construire ou charger la CSR préfixe ;
3. produire le ledger d'arêtes possédées et recertifiées ;
4. comparer ce ledger au CPU **avant** toute union ;
5. rejouer lot par lot dans un DSU device, avec publication atomique après la
   fermeture du lot.

Quand `source_complete_for_order[k]` et la fermeture carrier sont reçues, le
ledger chaud peut être plus petit : records
`(nouveau, racine_stricte_figee, carrier_reel)`, jamais toutes les paires.
Après recertification, projeter les incidents vers les racines strictes, trier
par racine et fermer le graphe biparti nouveaux--racines. Sa masse est la somme
du nombre de racines strictes distinctes réellement touchées par chaque
nouveau. Le ledger de chaque nouveau reste publié même si toutes ses incidences
sont redondantes. Le ledger de paires demeure le juge et le chemin partiel.

Le DSU trié par niveau réalise alors un Kruskal temporel. Borůvka segmenté ne
devient prioritaire que si les mesures montrent de la contention, trop de
passes ou un ledger intermédiaire trop grand. Les ex aequo exigent un snapshot
strict puis une fermeture atomique; l'ordre interne du lot ne doit pas modifier
le transcript sémantique.

Une MSF ne rend rien exact par elle-même. Pour chaque ordre, il faut d'abord
prouver que le graphe sparse $H_k$ a les mêmes composantes que le graphe complet
des générateurs à chaque coupe exacte stricte et fermée. La MSF conserve ensuite
les composantes de **ce** graphe. Le raccourci par graphe de Gabriel/MST brut est
réfuté dans le registre du dépôt; les générateurs silencieux et leur ledger
restent présents même s'ils n'ajoutent aucune arête de forêt.

Le filtre d'ordre ne se réduit pas à $\lvert M_u\cap M_v\rvert\geq k$. Avec
$q_v=q_{min}(v)$, le sommet $v$ n'est actif que pour
$k\in[\max(1,q_v-1),\min(K,\lvert M_v\rvert)]$. Une arête $uv$ n'est active que
pour :

$$k\in[\max(1,q_u-1,q_v-1),\min(K,\lvert M_u\cap M_v\rvert,\lvert M_u\rvert,\lvert M_v\rvert)].$$

Sans cette fenêtre, un générateur de support trop grand devient un faux sommet
ou marqueur. Les sommets isolés sont activés séparément du flux d'arêtes.

### Q5 — première famille proche LiDAR

La famille `terrain` seule peut qualifier `terrain_synthetic_v1`, pas un régime
LiDAR général. La première famille supplémentaire doit être une nappe à
**stries de balayage et recouvrements multi-échos** : espacement anisotrope le
long et entre lignes, bandes de densité variable, trous, bords francs, plusieurs
retours verticaux quantifiés et deux passages faiblement décalés. Elle exerce à
la fois les postings lourds, les ex aequo u16 et le régime multi-captation déjà
observé comme plus difficile.

Le reçu doit séparer au minimum `terrain`, `scanline_single_pass` et
`scanline_overlap_multiecho`, avec plusieurs graines et p50/p95/p99/max des
masses par lot. Le taux de fallback mesuré autour de deux pour cent sur les
petits nuages complets $n=11$ ne doit pas être transporté à 50 k. Le masque peut
être corrélé aux postings lourds, donc multiplier $H_{all}$ par ce taux n'est
pas une borne.

### Q6 — séquencement des sessions G4

M1 et un prototype M2 peuvent partager une même session G4 gardée, sans pause
d'audit humaine, si un checkpoint automatique interdit M2 dès qu'une identité
M1 est rouge. Les artefacts, timers, pics et digests des deux étages restent
séparés. Il n'est pas utile d'occuper une seconde session pour constater un
échec que le ledger CPU pouvait déjà voir.

En revanche, le M2 actuel ne doit pas être présenté comme la source 50 k : le
port du parcours visite encore des dizaines de millions de sommets dans le
scénario publié et le chemin CPU alloue/zéro encore un tableau de taille $n$
dans chaque requête de voisinage. La première exécution M2 utile est soit une
qualification bornée du mécanisme transactionnel, soit la sonde de masse de la
source locale ci-dessous. La composition bout en bout vient seulement après la
réception séparée de leurs payloads.

## Verrou local fermé : source par cellule de centre, portée globale à recevoir

Le cover courant possède déjà le bon certificat, mais l'utilise après avoir
formé les tuples dans un voisinage d'ancre construit avec le pire $Q$ global.
On peut inverser l'énumération : la cellule de **centre** devient propriétaire
du support et donne, avant tout tuple, l'ensemble complet de points qui peuvent
participer à une sortie pertinente.

Prendre comme racine $\prod_i[\min_i,\max_i+1)$, puis la partitionner en
cellules half-open dont les fermetures sont des boîtes à bornes entières. Les
coupures restent entières, même si les deux enfants n'ont pas exactement le
même côté. L'ownership d'un centre rationnel est décidé en entier exact; aucun
clamp n'est une décision admissible. Pour l'arité
$q\in\left\lbrace2,3,4\right\rbrace$, poser $t_q=s_{max}-q+1$. Pour chaque
cellule $C$, choisir **n'importe quels** $t_q$ `PointId` distincts $W_{q,C}$ et
poser :

$$Q_{q,C}=1+\max_{w\in W_{q,C}}\max_{y\in\mathrm{coins}(C)}\lVert w-y\rVert^2.$$

Définir ensuite la dilation exacte de la cellule :

$$A_{q,C}=\left\lbrace x\in X:\mathrm{dist}^{2}(x,\overline{C})<Q_{q,C}\right\rbrace.$$

**Théorème cellule--candidat.** Soit une miniboule propre de support $U$, de
centre $c$ possédé par $C$, de rayon carré $\beta$ et d'intérieur strict $I$.
Si $\lvert U\rvert=q$ et $q+\lvert I\rvert\leq s_{max}$, alors
$\beta<Q_{q,C}$ et le support, l'intérieur et toute la coquille appartiennent à
$A_{q,C}$.

**Preuve.** Tout témoin est à distance carrée strictement inférieure à
$Q_{q,C}$ de tout point de $C$. Si $\beta\geq Q_{q,C}$, les $t_q$ témoins sont
strictement intérieurs à la boule, donc
$q+\lvert I\rvert\geq q+t_q=s_{max}+1$, contradiction. Ainsi
$\beta<Q_{q,C}$. Pour tout point fermé $x$ de la boule,
$\mathrm{dist}^{2}(x,\overline{C})\leq\lVert x-c\rVert^2\leq\beta<Q_{q,C}$,
donc $x\in A_{q,C}$. Réciproquement, si $x\notin A_{q,C}$, alors
$\lVert x-c\rVert^2\geq Q_{q,C}>\beta$ : aucun point extérieur à la liste
locale ne peut appartenir au census fermé.

Cette preuve ferme aussi le cercle logique. Pour un tuple local, on calcule la
sphère et son centre exactement. S'il n'est pas possédé par $C$, on le rejette
dans cette tâche. Si $\beta\geq Q_{q,C}$, la banque prouve directement
`AboveInteriorWindow`, sans census. Si $\beta<Q_{q,C}$, le census dans
$A_{q,C}$ est globalement complet. Une coquille cosphérique arbitrairement
grande est incluse; elle augmente la sortie et le temps, mais ne crée aucune
omission.

Le cas $n<t_q$ reprend le fallback racine exact déjà utilisé par le prototype.
L'arité un publie directement les singletons. Les centres de supports propres
sont dans leur enveloppe convexe, donc dans la boîte du nuage : aucune région
non bornée n'est à parcourir.

La portée doit rester explicite. Ce théorème rend complète l'énumération des
miniboules propres satisfaisant la fenêtre ouverte $q+\lvert I\rvert\leq s_{max}$;
il rend donc aussi complète la sous-famille de rang fermé au plus `smax`. Il ne
prouve pas, à lui seul, que cette famille suffit à toute la tour $\Gamma_k$ en
présence de coquilles multiples. Cette dernière implication doit venir du
contrat de source/quotient Gate D et de son oracle. Tant qu'elle manque, le
statut est « exact relatif à la `GeneratorTable` et à la fenêtre déclarée », pas
« MorseHGP3D exact public ».

Pour le catalogue fermé borné actuel, on obtient néanmoins une vraie preuve
source. Tout générateur $B$ de saturé $M$ avec $\lvert M\rvert\leq s_{max}$
possède un support canonique propre $U$ de taille $q\leq4$; le théorème place
$U$ et tout $M$ dans la liste de sa cellule owner, donc le tuple est énuméré et
le census le retrouve. Réciproquement, un tuple propre accepté fixe le rayon de
sa boule; après census complet, test $\lvert M\rvert\leq s_{max}$ et support
canonique, il appartient au même catalogue. Les singletons sont ajoutés
séparément. L'oracle borné doit encore recevoir cette implémentation sans
partager son énumérateur.

La même banque $t_q=s_{max}-q+1$ suffit au catalogue fermé : tout record fermé
conservé vérifie $q+\lvert I\rvert\leq\lvert M\rvert\leq s_{max}$, puis le
census local retrouve la coquille entière et le filtre $\lvert M\rvert\leq s_{max}$.
Une grosse extra-coquille peut néanmoins coûter cher avant ce rejet.
Une banque alternative de $s_{max}+1$ témoins fermés est un filtre précoce
valide, mais son $Q$ plus grand peut augmenter fortement $A_C$ et les tuples;
elle n'est ni nécessaire à l'exactitude ni un autre contrat scientifique.

## Pourquoi ce résultat change l'implémentation

Le prototype courant construit un voisinage d'ancre au seuil
$4\max_C Q_{q,C}$, puis énumère les combinaisons avant de connaître la cellule
du centre. La variante ci-dessus fait une requête boîte--boîte au seuil local
$Q_{q,C}$ et forme seulement les tuples de $A_{q,C}$. Elle gagne donc à la fois
le maximum global et le facteur quatre du rayon carré de l'inégalité par ancre.

Surtout, la construction des témoins n'exige pas une requête top-$t_q$ exacte
pour être **correcte**. N'importe quels témoins distincts sont valides. Une
petite fenêtre déterministe autour du code Morton de la cellule fournit une
borne initiale; si elle contient moins de $t_q$ points, un réservoir global
déterministe la complète. Ne pas avoir trouvé les témoins globalement les plus
proches peut seulement agrandir $Q$ et le travail, jamais supprimer une sortie.

Pour une cellule lourde, le vrai top-$t_q$ donne néanmoins le meilleur $Q$
possible. Poser :

$$g_C(w)=\sum_i\max\bigl((w_i-C_i^-)^2,(w_i-C_i^+)^2\bigr).$$

Sur un nœud AABB de la LBVH, une borne inférieure exacte est séparable par axe :
projeter le milieu de $[C_i^-,C_i^+]$ sur l'intervalle du nœud, puis évaluer le
maximum des deux carrés. En doublant les coordonnées, les milieux
demi-entiers restent des entiers. Un branch-and-bound avec un heap de
$t_q\leq10$ témoins termine lorsque la borne scalaire de tout nœud restant
atteint le $t_q$-ième score $g_C$. Le score terminal est le minimum possible de
$Q_{q,C}$ parmi toutes les banques de cette taille; les ex aequo peuvent fournir
n'importe quels témoins distincts. Si le reçu exige la liste canonique par
`PointId`, il faut aussi visiter les nœuds encore égaux au score terminal ou
porter leur plus petit identifiant dans la borne. Si même cette banque laisse
une cellule trop lourde, le diagnostic est un vrai no-go local, pas la faute
d'une proposition Morton médiocre. Le pire cas peut toujours visiter tous les
points; le reçu publie les nœuds visités.

La partition peut être adaptative. Pour une cellule active, compter
$m_{q,C}=\lvert A_{q,C}\rvert$ et la masse exacte
$\binom{m_{q,C}}{q}$. Si la cellule est trop lourde et peut être divisée, ne rien
émettre au parent, compter d'abord ses enfants et comparer le coût pondéré
parent/enfants. Les enfants partitionnent tous les centres du parent, donc le
split est sémantiquement exact, mais il n'est pas forcément moins cher : si
tous les $A_{child}$ sont identiques, il multiplie les tuples par huit. Une
cellule unitaire encore lourde passe à un chemin exact hors SLO ou produit un
statut `slo_not_admitted`; elle n'est jamais tronquée et son replay CPU ne doit
pas être caché dans le chrono de la seconde.

Hériter d'abord les témoins du parent donne
$Q_{q,C'}\leq Q_{q,C}$ et $A_{q,C'}\subseteq A_{q,C}$ pour un enfant $C'$; une
nouvelle banque n'est adoptée que si elle abaisse encore $Q$. Les listes
enfants peuvent donc être construites en filtrant le CSR parent. Cette
monotonie réduit la construction, pas la somme des tuples entre enfants.

Les cellules pavent l'espace des **centres**, pas les seules cellules occupées
par des points. Un centre critique peut se trouver dans une cellule Morton
vide. Sous un modèle stationnaire où le volume de la boîte croît comme $n$ et
où le pas optimal reste borné, un nombre linéaire de cellules et des listes
locales bornées sont plausibles. C'est une hypothèse de performance à mesurer,
pas une conséquence du lemme. Les duplications, alignements, grandes coquilles
et recouvrements de scans peuvent rendre une tâche dense; le reçu les rend
visibles et le fallback reste exact.

Une réduction sûre existe avant les banques : le centre d'une miniboule propre
appartient à $\mathrm{conv}(X)$. Toute cellule dont la fermeture est certifiée
disjointe de la coque convexe ordinaire peut être supprimée. Les cellules qui
touchent la frontière restent. Cette coque n'est ni une mosaïque d'ordre
supérieur ni une promesse de gros gain sur un terrain à coque volumique.

## Blueprint CUDA minimal de la source

1. Trier les points par Morton et construire une LBVH résidente une seule fois.
2. Émettre les cellules actives par arité; une fenêtre Morton fournit l'upper
   bound, puis les cellules lourdes lancent le top-$t_q$ LBVH exact.
3. Faire un `count` LBVH avec la distance AABB--cellule exacte, puis un scan
   exclusif. Aucun buffer `cellule*n` n'est alloué.
4. Comparer en count-only parent et enfants, sceller la frontière retenue, puis
   remplir seulement les listes $A_{q,C}$ de ses feuilles.
5. Énumérer séparément les paires, triples et quadruples par intervalles
   combinadiques streamés, sans tableau de tuples. Les prédicats rapides donnent
   seulement des filtres sûrs; toute décision indécidable monte vers les
   largeurs exactes déjà prévues.
6. Vérifier sphère propre, bon centrage, ownership half-open, puis appliquer le
   test $\beta\geq Q_{q,C}$ ou le census local complet.
7. Émettre des records compacts, trier/RLE par `SphereKey`, réunir tous les
   supports alternatifs, certifier $q_{min}$ comme le minimum des supports
   propres trouvés et conserver le saturé fermé complet à longueur variable.
8. Trier la `GeneratorTable` par niveau exact et clé canonique, puis seulement
   construire les sidecars du fold préfixe.

Cette voie ne matérialise ni sommets globaux de l'arrangement, ni cellules ou
cofaces de Delaunay d'ordre supérieur, ni incidences de toutes les $k$-faces.
Elle ne conserve pas non plus un `seen_candidate[n]` par tâche. Sur CPU, un
scratch `epoch[PointId]` réutilisé par worker suffit; sur GPU, les listes sont
produites par count/scan ou sort/unique borné.

## Préflight et critère de décision à 50 k

Pour la frontière adaptative finale, publier par arité :

$$B_q=\lvert\mathcal{C}_q\rvert,\qquad L_{A,q}=\sum_{C\in\mathcal{C}_q}m_{q,C},\qquad R_q=\sum_{C\in\mathcal{C}_q}\binom{m_{q,C}}{q},\qquad T_q^{max}=\sum_{C\in\mathcal{C}_q}m_{q,C}\binom{m_{q,C}}{q}.$$

Ajouter les visites LBVH, la distribution de $m_{q,C}$, la profondeur de
split, le coût parent/enfants, les cellules unitaires lourdes, les candidats
non-owner, propres, refusés par banque et réellement censés, le travail réel
$T_q^{actual}=\sum_{(C,U)\in\mathcal{J}_q}m_{q,C}$, les tailles de coquille,
les octets de chaque arène et leur high-water. Ces compteurs doivent être
calculés en largeur vérifiée avant allocation.

Avec $s_{max}=11$, toute cellule de la banque ouverte contient déjà au moins
$\binom{10}{2}=45$, $\binom{9}{3}=84$ et $\binom{8}{4}=70$ tuples dans les
lanes deux, trois et quatre. Le nombre de cellules est donc lui-même un poste
d'admission, même lorsque chaque liste locale est idéale.

La première expérience 50 k utile est une **sonde de masse de cette source** :
LBVH, cellules, témoins, $A_{q,C}$ et $R_q$, sans énumération de tuples. Si
cette sonde est rouge, un kernel aveugle sur les tuples ne sauvera pas le
budget : il faut une condition nécessaire supplémentaire ou une autre
partition, pas un cap. Si elle est verte, M2 exécute les tuples sur exactement
le même manifeste et vérifie les identités `count/fill/commit`.

Le dispatcher peut rester exact par lane : comparer le préflight de la route
globale actuelle, $\sum_p\binom{d_p^+}{q-1}$, à $R_q$ pour la route
cellule--centre, puis prendre la route de plus faible masse admise. Elles ont la
même vérité sous leur contrat; aucune ne domine l'autre sur tous les nuages.

Une seconde universelle n'est pas démontrable sur tout nuage u16 : une coquille
de taille linéaire impose déjà une sortie linéaire, et le nombre de générateurs
peut dépasser tout budget fixe. Le contrat industriel honnête est : exactitude
inchangée pour toute entrée acceptée par le domaine, SLO reçu sur des familles
nommées et digérées, chemin lourd exact pour les entrées hors enveloppe de coût.
`slo_not_admitted` est un résultat de planification/performance, jamais un rejet
scientifique de l'entrée. Aucun cap de degré, de coquille ou de candidats ne
doit changer silencieusement la réponse.

## Portes permanentes de la nouvelle source

1. Différentiel exhaustif sur petits nuages contre `flat_catalogue`, comparant
   sphère, niveau, support canonique, intérieur, coquille et multiplicité.
2. Centre rationnel sur chaque face, arête et sommet de cellule; une seule
   cellule émet, les voisines possèdent zéro fois.
3. Témoin exactement au score de coin; le `+1` et les inégalités strictes
   doivent être mutation-résistants. Pour tuer $t_q-1$, prendre $q=2$,
   $s_{max}=3$, $A=(0,0,0)$, $B=(4,0,0)$ et $z=(2,1,0)$ : la boule diamètre
   $AB$ a exactement un intérieur et reste admissible; un seul témoin choisi en
   $z$ la rejetterait à tort.
4. Support dont le centre et le plus petit `PointId` sont dans des cellules
   différentes, pour tuer l'ownership par ancre.
5. Coquille cosphérique de taille supérieure à `smax` : le profil ouvert
   transporte le saturé variable; le profil fermé concorde avec
   `flat_catalogue` et refuse le record après census. Les deux statuts ne sont
   jamais confondus.
6. Mutation `omit-child`, `emit-parent-and-child`, `Q-without-plus-one`,
   `distance-to-cell-center`, `closed-range-as-open` et `cap-heavy-cell`.
7. Invariance sous permutation des `PointId`, du catalogue physique, des blocs
   CUDA et des frontières de chunks.
8. Identités exactes de count/fill, commit ou replay intégral après overflow,
   et comparaison du ledger source avant le fold.
9. La paire de grand intérieur donnée plus haut doit rester absente du
   catalogue shallow mais présente dans l'oracle $\Gamma_2$ complet; seule une
   capability de quotient reçue a le droit de l'éliminer du produit réduit.

## Réception live intermédiaire du 11 août

Cette réception reste attachée à des empreintes de fichiers, car le lot de
Claude n'est pas encore committé au-dessus de `abb4c0e`. Aucun de ces résultats
ne vaut réception d'un futur snapshot différent.

- La porte abstraite préfixe termine désormais sur l'univers hostile de taille
  huit. Sur la campagne `400 x 24`, elle possède 84 367 paires : 16 804
  `Q--Q` de même lot, 50 750 candidat antérieur--requête postérieure et 16 813
  `Q--R` de même lot; 48 539 obligations restent au sidecar. La masse CSR
  242 459 coïncide avec l'identité indépendante. Les huit mutants de longueur,
  omission, staging, duplication, double possession, ordre, futur visible et
  staging séquentiel meurent tous.
- Dix-neuf CTests ciblés du préfixe, du ledger du fold réel, des cinq mutants
  du fold, du masque hybride, des familles scanline et du catalogue parallèle
  ont passé sur le sous-snapshot testé. La porte structurelle `k=1` renforcée a
  aussi tué séparément les décalages de niveau et une fusion trop précoce. La
  campagne complète lancée concurremment par Claude n'est pas couverte par ce
  verdict ciblé.
- La sonde `terrain`, 200 points, 10 682 générateurs, donne exactement
  4 481 465, 3 614 547 et 2 866 416 hits fallback aux ordres un à trois. Sur
  l'hôte chargé de l'audit, le catalogue a pris 53,193 s, contre 0,654 s au
  join de l'ordre un. Les valeurs absolues ne sont pas un benchmark G4, mais
  le rapport confirme que la source, non le fold sparse, est le verrou local.
- Le nouveau mode partiel `prefix-all` a été reçu manuellement avec les
  empreintes SHA-256 `saturated_pipeline.cpp=882e43507e9d...` et
  `saturated_fold_hybrid.hpp=68d8059dbf8d...`. La commande
  `--points 32 --smax 11 --max-order 3 --seed 20260810 --join prefix-all
  --compare-joins 1` termine avec le code zéro en 4,51 s : digests
  13583866067985804659 identiques, 16 326 606 hits prédits et lus,
  5 129 573 paires recertifiées et 850 990 faux candidats. C'est une réception
  positive de l'exactitude **relative à la table tronquée**, pas de la
  complétude de cette table pour la tour.
- La porte CMake `mhgp3v_saturated_pipeline_prefix_all_partial` courante ne
  passe toutefois pas `--compare-joins 1`; elle vérifie seulement que le mode
  s'exécute et imprime sa provenance. Ajouter le différentiel à cette porte,
  ou créer une porte distincte sur la même famille partielle, est le dernier
  verrou permanent précis observé pour cette livraison `prefix-all`.

Les empreintes complètes, sorties brutes et tests permanents devront être
rejoués après stabilisation du lot. Cette réception n'autorise toujours aucune
session G4 : la prochaine expérience qui discrimine le plan 50 k reste la
sonde `mass-only` de la source par cellules.

## Ordre recommandé après le lot CPU live

1. Graver `--compare-joins 1` dans la porte partielle `prefix-all`, rejouer les
   portes permanentes après stabilisation et publier leurs empreintes. Le mode
   relatif est alors reçu indépendamment de la source.
2. Construire la factory `ValidatedHybridSidecar` : handle unique par boule,
   saturé fermé, `q_min`, ordre exact et capability explicite de fermeture des
   carriers. La fixture des deux triangles de rayon carré 25 reçoit le chemin
   strict; la même fixture privée de $AB$ doit refuser la capability.
3. Comparer sur CPU le préfixe commun au prototype « index complet, requête
   rare-first » avec le préflight exact de chacun. Garder le moins massif par
   ordre ou par requête seulement sous le théorème de budgets correspondant.
4. Construire une sonde **mass-only** de la source par cellules sur `terrain`,
   `scanline_single_pass` et `scanline_overlap_multiecho`, d'abord à
   400/1 600/2 400 points contre la route globale, puis à 50 k sans tuples.
5. Si les arènes, $R_q$ et $T_q^{actual}$ sont admis, écrire le kernel source
   et son différentiel borné; sinon modifier la partition ou le filtre exact,
   jamais l'exactitude.
6. Qualifier le fold sparse device sur un catalogue reçu, puis composer source,
   tri exact, lots, fold et réduction forestière dans un même chrono
   `warm_e2e`. La seconde n'est revendiquée que sur les profils nommés dont le
   manifeste a franchi l'admission.

Le résultat attendu n'est pas « un CUDA plus rapide du prototype actuel ».
C'est un changement de variable : énumérer les petites listes certifiées par la
position du centre, puis laisser le GPU traiter des tâches indépendantes et
rejouables.

GCP non utilisé.
