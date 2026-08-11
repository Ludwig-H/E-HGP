# Réponse à Claude — passer à une architecture G4 compatible avec 50 k

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=candidat_gpu_g4`,
`profile=terrain_u16_K10`, `mode=presumed_exact_structural_candidate`,
`public_status=not_claimed`.

Snapshot de lecture : HEAD `8df7ac8`; `parallel_catalogue.hpp`
`ef620e57372c4219`; `order_k_flats.hpp` `7da730fe50cf7c61`. Le worktree est
actif : ces empreintes bornent les constats sur le prototype, pas les
recommandations d'architecture.

## Verdict utile

Le catalogue 48 threads est un bon échafaudage pour qualifier la partition de
la reverse-search et mesurer le terrain. Ce n'est pas le chemin du SLO : même
la division irréaliste de `236,5 s` par 48 donne `4,93 s` à seulement 2 400
points, avant fold. Porter tel quel le catalogue CPU sur GPU ne suffit pas non
plus : le scénario publié de 55 millions de sommets consommerait déjà autour de
1,5 seconde pour la seule navigation au débit optimiste cité, avant `next`,
harvest, tri et réduction.

La transition industrielle doit supprimer quatre masses, pas seulement ajouter
des threads :

1. aucun `Vertex` global ni double tableau récolté;
2. aucune incidence de $k$-faces `I_k` dans le chemin produit;
3. aucun flux exhaustif d'arêtes ramené sur l'hôte;
4. aucune union DSU hôte entre les étages device.

La chaîne cible est :

~~~text
points u16 + index exact immuable, résidents
    -> tâches de source transactionnelles sur GPU
    -> GeneratorTable compacte + niveaux exacts triés
    -> attaches principales (<= 4) + fallback préfixe--préfixe rare
    -> graphe sparse exact par ordre
    -> MSF temporelles GPU + rejeu atomique des lots
    -> K forêts et sidecars compacts + reçu
~~~

Cette forme est compatible en mémoire avec 50 k parce que son coût porte sur
les générateurs et les attaches utiles, et non sur les dizaines de milliards de
cooccurrences. Elle ne prouve pas encore la seconde : chaque masse et chaque
débit ci-dessous devient une autorité d'admission mesurée.

## 1. Réponse sur les régimes surfaciques

Il existe un résultat positif, mais sous une hypothèse plus forte que « proche
du LiDAR ». Pour des points **exactement planaires**, en position générale, la
complexité au pire du diagramme de Voronoï d'ordre $k$, et donc de la mosaïque
de Delaunay duale, est $O(k(n-k))$; cette borne est asymptotiquement atteinte
par certaines familles. Pour $K$ fixé, la somme des ordres jusqu'à $K$ est
$O(nK^2)$. Si la source produit un nombre borné de records canoniques par
événement et si le rang/coquille est borné, le nombre de générateurs et sa
masse logique sont alors linéaires en $n$ pour $K=10$. Cette asymptotique
planaire est cohérente avec les algorithmes optimaux récents de diagrammes
d'ordre supérieur; elle ne se transfère pas automatiquement au catalogue
affine 3D actuel.

Une surface 3D avec relief, même lisse et bien échantillonnée, n'offre pas cette
borne par sa seule dimension intrinsèque. Des échantillons uniformes de
surfaces lisses peuvent avoir une Delaunay 3D superlinéaire, et certaines
familles s'approchent du quadratique. Les hypothèses qui rendent les bornes
favorables contrôlent explicitement la séparation, le spread, la géométrie de
la surface ou les contacts de l'axe médian. Voir
[Chan--Cheng--Zheng](https://arxiv.org/abs/2310.15363),
[Erickson](https://arxiv.org/abs/cs/0103017) et
[Amenta--Attali--Devillers](https://web.cs.ucdavis.edu/~amenta/pubs/HiDeeDel.pdf).

Le jitter fin ne constitue pas un tel certificat. Les coplanarités et
cosphéricités quantifiées peuvent donner une coquille ou un lot de niveau exact
de taille $\Theta(n)$. La profondeur 18 observée n'est pas une borne. La borne
384 bits déjà prouvée pour comparer les niveaux u16, elle, ne dépend pas de la
position générale : la dégénérescence augmente la multiplicité, pas la largeur
arithmétique.

Deux profils séparés sont donc utiles :

- un backend exactement planaire 2D, avec sa propre preuve et ses propres
  prédicats;
- le terrain 3D, admis par des compteurs hostiles et rendu
  `slo_not_admitted` si son enveloppe déborde, jamais réputé linéaire par
  analogie.

Avant tout 50 k, mesurer par famille, graine et ordre : `V/n`, `G/n`, membres
`L/n`, flats et enfants par sommet, points touchés par `next`
`p50/p95/p99/max`, coquilles, tailles des lots de niveau égal, largeur de
l'antichaîne, travail par sous-arbre, principal/fallback, hits du fallback,
arêtes sparse et octets réels. Le premier sweep décisionnel est
`2400 -> 6250 -> 12500`; `25000 -> 50000` ne vient que si l'admission calculée
depuis le débit p10, avec 30 % de marge, reste sous l'enveloppe.

## 2. Source GPU résidente

### 2.1 Réutiliser le bon théorème du prototype

La couronne à profondeur fixe est une antichaîne de l'arbre de parent unique.
Ses sous-arbres sont donc indépendants et couvrent chaque sommet une fois. Le
prototype parallèle sert à recevoir cette propriété; le kernel produit peut
porter une tâche compacte `(root,v,cursor,epoch,segment)` et parcourir chaque
sous-arbre sans `seen` global.

Avant d'utiliser ses chronos, deux réparations locales sont nécessaires dans le
prochain travail de Claude :

- séparer la topologie KD immuable des métriques et scratch par worker. Le
  snapshot épinglé partage `CertifiedIndex` alors que ses méthodes `const`
  modifient des compteurs `mutable`; ThreadSanitizer a confirmé une data race
  dans `sign_disagreement`, sortie 66;
- comparer à la vérité séquentielle le multiensemble complet
  `(shell,interior,level)`, l'identité couronne+sous-arbres et la multiplicité
  un, pas seulement le catalogue dédupliqué. Le mutant historique qui perdait
  un sous-arbre montre pourquoi la déduplication finale n'est pas ce certificat.

Le tableau `seen_candidate(n)` remis à zéro dans chaque `neighbour_along` est un
verrou de débit indépendant. Sur CPU, employer une époque par worker et une
liste `touched`; sur GPU, produire les candidats dans un scratch borné puis
sort/unique local. Il ne doit exister aucun tableau `tache x n`.

La couronne fixe devient adaptative : viser au moins 8 à 16 fois le nombre de
CTA résidentes, publier `p50/p95/max` du travail par racine et rescinder une
tâche lourde par continuation authentifiée. Un `fetch_add` répartit les tâches,
mais ne raccourcit pas le dernier sous-arbre dominant.

### 2.2 Transaction de reverse-search

Les points u16 et l'index exact LBVH/grille sont transférés une fois, scellés
par `(epoch,digest)`. Un persistent kernel consomme les racines de l'antichaîne.
Chaque tâche écrit dans un segment privé :

- `commit` si navigation, voisin terminal, census, owner et capacité sont tous
  reçus;
- `split/replay` sur une continuation plus petite si le segment est plein;
- `wide-shell` vers une file device paginée, jamais vers un drain CPU dans le
  chrono chaud;
- `rollback` complet sur faute, sans préfixe public.

Un split remplace le domaine logique du parent par une partition disjointe et
complète de domaines enfants; aucune sortie de la tentative parent n'est alors
publiée. Un rollback compte une tentative, pas une tâche logique achevée : les
reçus séparent `logical_tasks_completed`, `attempts_committed`,
`attempts_rolled_back` et `children_created`, au lieu de l'identité fausse
`started=commit+rollback` en présence de replay.

Le scan `next` en deux passes sur les $n$ points reste l'oracle. Le produit exige
un branch-and-bound terminal exact : une boîte n'est coupée que si elle prouve
que tous ses points sont après le meilleur candidat, et toutes les égalités du
minimum sont fermées ensemble. Les compteurs `points_touched/next` décident si
la source reverse passe le budget. Si le scénario de 55 millions de sommets ne
tient pas dans la tranche source mesurée, la route devient explicitement
`reverse_no_go` et la source directe `center-cover + degree` devient le palier
obligatoire; aucun débit du microkernel parent ne masque cette décision.

### 2.3 Récolter sans stocker les sommets

Un sommet non terminal ne vit que dans l'état de sa tâche. Dès qu'une boule
critique est certifiée, émettre directement un record compact device : niveau
exact, membres triés, support canonique de taille au plus quatre, rang,
`q_min`, bits de provenance, tâche source et certificat principal. Owner puis
sort/dedup construisent une `GeneratorTable`; aucun `vector<Vertex>` global ne
survit.

Une table minimale indicative commence autour de `32*G + 4*L` octets avant
index et alignement, où $G$ est le nombre de générateurs et $L$ le nombre total
de membres. Elle n'inclut pas les clefs exactes, supports, provenances, ledger,
hash, second buffer de tri ni workspace CUB. Tous ces champs et leurs capacités
réelles sont calculés avant allocation; cette formule n'est pas un high-water.

Pour les niveaux, `double` ne sert qu'au diagnostic. Employer une clef grossière
monotone avec arrondi certifié, grouper tout intervalle qui se chevauche, puis
trier chaque groupe avec le comparateur rationnel multi-limb exact. Une égalité
est décidée par la représentation rationnelle, jamais par la clef grossière.
Cette hiérarchie conserve l'étage 64 bits pour le parent, 128 bits pour
`next`/owner/census et 384 bits pour l'ordre des rayons sans payer le comparateur
le plus large sur tous les couples.

## 3. Fold sans incidences de faces

### 3.1 Chemin principal

Classifier `principal_support` pendant la source. Pour chaque $u$ du support
$U$, la miniboule exacte de $M\setminus\lbrace u\rbrace$ doit être strictement
plus petite; le certificat positif contient au plus quatre vérifications et
leurs témoins, une par $u$. Sous le
contrat de complétude du sidecar hybride, le théorème des $q$ attaches demande
alors au plus $q\leq4$ lookups de vrais carriers, au lieu de
$\binom{\lvert M\rvert}{k}$ signatures.

Le dictionnaire device peut être adressé par le support canonique puis vérifier
`BallKey` et saturé à la construction. Une collision, un second handle pour la
même boule ou une provenance incomplète choisit le fallback ou refuse; il ne
retourne jamais le premier match.

### 3.2 Fallback préfixe--préfixe

La nouvelle note
[`NOTE_SOLUTION_GPU_INDEX_PREFIX_PREFIX_20260810.md`](NOTE_SOLUTION_GPU_INDEX_PREFIX_PREFIX_20260810.md)
renforce le cover $t=1$ existant. Dans un ordre global commun, si deux
générateurs partagent au moins $k$ points, leurs préfixes de longueurs
$\lvert M\rvert-k+1$ et $\lvert N\rvert-k+1$ partagent un point. Il suffit
donc :

1. d'indexer chaque candidat $N$ seulement sur ce préfixe;
2. d'interroger le préfixe de la requête $M$;
3. de dédupliquer les `GeneratorId`;
4. de recertifier directement $\lvert M\cap N\rvert\geq k$ dans un warp.

Pour un rang 11 aux dix ordres, l'index persistant représente 65 identifiants
par générateur, contre 110 postings complets et 2 046 signatures non triviales.
En pratique, traiter un ordre à la fois réduit encore la résidence. Le nombre
de hits peut rester grand : le chemin est réservé au `query_mask` non principal
et son admission porte sur `prefix_hits`, `unique_candidates` et
`recertified_true`.

Cette admission est calculable avant le join. Si $d_{x,k}$ est le degré du
posting préfixé candidat et $q_{x,k}$ le nombre de requêtes fallback dont le
préfixe contient $x$, le nombre exact de hits bruts vaut
$\sum_x q_{x,k}d_{x,k}$; en tout-requête il vaut $\sum_x d_{x,k}^2$. La sonde
peut donc publier/refuser cette masse par une simple réduction de degrés avant
toute concaténation, puis réserver le join réel à la mesure des candidats
uniques et faux positifs.

Le lot ex aequo entier est stagé avant les requêtes. Une paire entre deux
fallback interrogés est possédée par l'`ActivationId` canonique le plus grand;
une paire entre un fallback interrogé et un fast est gardée par le fallback,
même si ce fast est postérieur dans le lot. Une paire sans extrémité dans le
masque courant — dont ancien-fallback--nouveau-fast — reste une obligation du
sidecar principal. Le couple réel et son témoin restent présents jusqu'à la
recertification; compresser les postings par racine DSU avant le seuil
inventerait des intersections.

### 3.3 Le graphe sparse est l'objet à certifier

Pour chaque ordre $k$, le chemin principal et le fallback émettent un graphe
$H_k$ d'arêtes réelles. Son contrat n'est pas « beaucoup moins d'arêtes », mais :

$$\pi_0(H_k^{<a})=\pi_0(G_k^{<a})\quad\text{et}\quad\pi_0(H_k^{\leq a})=\pi_0(G_k^{\leq a})$$

pour tout niveau exact $a$, où $G_k$ est le graphe d'intersections complet
relatif à la source admise. C'est la porte différentielle du sidecar hybride.
Elle est plus importante que le choix du réducteur suivant.

## 4. Retirer le DSU hôte avec des MSF temporelles

Soit $b(v)$ le niveau exact d'activation d'un générateur, $c(e)$ le plus grand
ordre admis par une arête réelle et $\theta(e)=\max(b(u),b(v))$ son niveau
d'apparition. Pour chaque $k$, filtrer $c(e)\geq k$ et calculer une forêt
couvrante minimale $F_k$ dont le poids primaire est $\theta$. Par convention,
$F_k^{<a}$ contient **tous** les sommets de rang au moins $k$ avec
$b(v)<a$, y compris les isolés, et les arêtes de poids $<a$; la version fermée
remplace les deux signes par $\leq$. La même convention vaut pour $H_k$.

**Théorème.** Si $H_k$ possède les mêmes composantes que la vérité à chaque
préfixe, alors toute MSF de $H_k$ préserve simultanément toutes les coupes
strictes et fermées :

$$\pi_0(F_k^{<a})=\pi_0(H_k^{<a})\quad\text{et}\quad\pi_0(F_k^{\leq a})=\pi_0(H_k^{\leq a}).$$

Après que Kruskal a traité tous les poids strictement inférieurs à $a$, puis
inférieurs ou égaux à $a$, sa forêt est maximale dans chacun de ces
sous-graphes. C'est aussi la propriété minimax des chemins d'une MSF. Les
partitions avant et après chaque lot sont donc conservées.

Il faut **une MSF par ordre**. Une forêt commune à tous les $k$ est impossible
en général : trois arêtes peuvent forcer une arête de capacité un à une coupe
précoce puis les deux autres, de capacité deux, à la coupe suivante; leur union
est un cycle. Un lancement GPU peut traiter les $K$ couches segmentées, mais la
structure mathématique reste $K$ forêts. La sortie sélectionnée vérifie :

$$\sum_{k=1}^{K}\lvert F_k\rvert\leq K(G-1).$$

La MSF ne remplace pas le ledger et son théorème ne certifie que $\pi_0$. Une
obligation indépendante doit recevoir que le ledger contient tous les
événements, marqueurs et contributions logiques. Chaque champ public est alors
défini comme une réduction déterministe, associative et canonique du ledger sur
les composantes strictes et fermées; cette propriété, plus la complétude du
ledger, est le lemme qui relève l'égalité de partitions vers l'égalité des
records. Au rejeu d'un niveau $a$ :

1. figer les racines strictes;
2. activer **tous** les générateurs $b(v)=a$, même isolés dans la MSF;
3. contracter ensemble toutes les arêtes $\theta=a$;
4. réduire sur chaque composante fermée les témoins stricts, le témoin fermé,
   `q_min`, marqueurs, contributions et provenances de tous les événements;
5. seulement alors publier naissance, continuation ou multifusion.

Une continuation silencieuse, un générateur utile seulement à une attache
future et une `DirectHyperedge` redondante restent donc dans le ledger même si
la MSF élimine toutes leurs arêtes. Les records sont canonisés par minimum de
clef et listes triées, jamais par « première arête choisie » ni par numéro de
racine DSU.

Le candidat device est un Borůvka ou filter-Kruskal segmenté par $k$, suivi
d'une contraction de merge-tree; les poids égaux sont contractés en un lot,
pas en un kernel par niveau. Une clef secondaire totale rend la sélection
reproductible, mais $\theta$ reste le poids primaire. Si les attaches arrivent
déjà triées par lot et sont très peu nombreuses, un DSU device direct peut être
plus rapide : benchmarker les deux. Le gain fondamental est $H_k$ sparse; la
MSF est son compresseur, son certificat de préfixe et un bon support de
dendrogramme.

La porte locale la plus forte vérifie, pour chaque arête de $H_k$ non retenue,
que ses extrémités sont reliées dans $F_k$ par un chemin dont le poids maximal
est au plus $\theta(e)$. Sur les petites tailles, comparer ensuite toutes les
partitions strictes/fermées, records, marqueurs et forêts à `G2`, au fold
hybride et à `face-owner`.

## 5. Mémoire et budget d'une seconde

L'arène doit être calculée depuis `cudaMemGetInfo` et plafonnée, par exemple à
70 % de la mémoire libre après contexte; ne pas coder « 96 Go » comme contrat.
Les buffers permanents sont points/index, `GeneratorTable`, membres, dictionnaire
de supports et ledger compact. Les temporaires sont réutilisés ordre par ordre :
index préfixé, arêtes sparse, MSF et workspaces de tri/contraction.

Exemple de stress, pas forecast : avec `G=50 M`, `L=450 M` et `E_k=200 M`, des
headers de 32 octets coûtent 1,6 Go, les membres u32 1,8 Go, une construction
CSR large quelques gigaoctets, des arêtes double-buffer de 16 octets 6,4 Go et
la MSF d'un ordre au plus `G-1` arêtes. Conserver simultanément les dix MSF
peut atteindre environ 500 M arêtes, soit 8 Go à 16 octets par arête, avant
ledger, hash et workspace; le traitement ordre par ordre ou une forme de
merge-tree compacte est donc préférable. Cette architecture peut entrer dans
une arène de plusieurs dizaines de gigaoctets sous ces hypothèses; les `5e10`
incidences du forecast, elles, ne le peuvent pas. Chaque taille, capacité et
workspace reste dans le manifeste réel.

Budget de conception proposé, **non mesuré et servant uniquement de seuil
d'admission** :

| étage warm end-to-end | cible d'allocation |
| --- | ---: |
| source + tri exact + `GeneratorTable` | 0,48 s |
| attaches principales + fallback | 0,22 s |
| MSF + rejeu/merge-trees | 0,18 s |
| finalisation, reçus et copie compacte | 0,07 s |
| marge | 0,05 s |

`warm_e2e` inclut copie des points, remise à zéro logique des arènes, index,
source, tri, fold, synchronisations et copie du résultat compact. Publier aussi
`cold_e2e` avec contexte et allocations. Un temps kernel-only ou qui commence
après upload du catalogue ne remplit pas ce budget.

Le scénario actuel de 55 millions de sommets est déjà évalué autour de 1,5 s
pour la seule navigation : il échoue donc au seuil source de 0,48 s tant qu'une
mesure device complète ne réfute pas ce scénario. Les nombres de la table ne
sont ni un forecast favorable, ni une manière de déclarer la reverse-search
admissible.

## 6. Jalons qui apprennent vite

### M1 — fold sparse sur catalogue reçu

Construire d'abord la `GeneratorTable` device par upload du catalogue CPU.
Implémenter support principal, préfixe--préfixe, graphe sparse et MSF. Différentiel
champ par champ jusqu'à $n\leq400$, puis exécuter le catalogue $n=2400$ déjà
disponible. Le résultat décisif est que temps et mémoire suivent
`A_fast + prefix_hits + E_k`, et non `I_k`.

### M2 — source device reçue

Porter `next` terminal, census, owner, harvest direct et transactions
d'antichaîne. Comparer sommets, enfants, générateurs et digests jusqu'à 2 400.
Les coquilles larges sont réellement rejouées sur device; un compteur de refus
sans payload ne passe pas.

### M3 — composition résidente $K=10$

Composer source, tri, attaches, MSF, records et forêts sans D2H intermédiaire.
Mesurer `6250/12500/25000`, plusieurs graines uniformes et terrain. Le 50 k est
lancé seulement si le manifeste de masse et la projection p10+30 % passent le
budget et l'arène; sinon la commande rend `slo_not_admitted` avant allocation.

### M4 — sidecars de hiérarchie ponctuelle

Réserver dès M1 les identifiants et digests de
`CoverageContribution/VerticalAssignment`. Les contributions silencieuses sont
impossibles à reconstruire depuis les seules forêts. Leur émission complète a
cependant son propre budget `I_cov`; si elle ne tient pas, la sortie SLO reste
explicitement « K forêts sans projection ponctuelle exacte », au lieu de
charger silencieusement le fold chaud.

Ce séquencement remplace la session G4 proposée « catalogue CPU 48 cœurs jusqu'à
50 k » par deux expériences moins coûteuses et plus décisionnelles : fold sparse
device sur un catalogue existant, puis source device seule. Le CPU parallèle
reste utile pour produire les vérités intermédiaires et mesurer le déséquilibre.

## 7. Statut et garde-fous demandés par Louis

`presumed_exact_structural` est défendable comme statut public distinct si son
nom complet porte aussi `partial_refinement`, le rang admis et $K=10$. Il
signifie : prédicats et fold exacts relativement à la `GeneratorTable`,
différentiels bornés permanents, source tronquée déclarée, reçus de transactions
et contrôles structurels à l'échelle. Il ne doit jamais être sérialisé sous le
booléen `exact`.

Les deux garde-fous proposés sont bons et restent hors chrono produit :

- $k=1$ : comparer à chaque lot les partitions de `PointId` de la forêt avec
  l'EMST exact, pas seulement les niveaux ou la composante finale;
- $k=2$ : énumérer les triangles de deux arêtes Delaunay incidentes, même si
  ces arêtes ne sont qu'un diagnostic. Chaque triplet distinct donne une vraie
  deadline de Čech à sa miniboule exacte; une LCA des trois carriers dans la
  forêt $k=2$ classe `connected_before`, `connected_at`, `late` ou `never`.

Une arête Delaunay fausse ne crée pas de fausse obligation, puisque tout triplet
est un simplexe de Čech à son propre niveau. Une arête manquante réduit seulement
la couverture du garde-fou. Les résultats sont stratifiés par point, tuile,
degré et niveau pour empêcher les hubs de masquer une zone oubliée. La note
[`REPONSE_CLAUDE_STRUCTURE_K2_DELAUNAY_20260810.md`](REPONSE_CLAUDE_STRUCTURE_K2_DELAUNAY_20260810.md)
donne le reçu et les mutants.

## 8. Portes minimales avant la prochaine G4

1. ThreadSanitizer vert sur le catalogue parallèle, métriques par worker et
   ledger exactly-once contre le séquentiel.
2. Préfixe--préfixe exhaustif sur petits set-systems; mutants longueur moins un,
   ordre par requête, future visible et projection DSU précoce tués.
3. Graphe hybride $H_k$ égal à la vérité sur toutes les coupes strictes et
   fermées; source/sidecar authentifiés.
4. MSF temporelles comparées à `G2` sur partitions, records, marqueurs et
   forêts; mutants forêt unique, poids capacité, lot égal scindé, événement
   silencieux et ledger non-MSF supprimés.
5. `GeneratorTable` et dictionnaire exacts bit à bit CPU/device, y compris
   niveaux distincts ayant le même `double` et rationnels égaux encodés depuis
   des supports différents.
6. Admission chiffrée depuis `V,G,L,A_fast,prefix_hits,E_k,max_batch`, capacités
   CUB réelles et débit p10; aucune allocation avant passage.
7. `warm_e2e` complet $K=10$, reçu brut, digests d'entrée/binaire/sortie et
   rollback injecté; EMST et wedges exécutés séparément comme garde-fous.

## Réponse courte aux six questions

1. Terrain : linéaire conditionnel pour le planaire exact; aucune borne issue du
   seul mot « surface ». Mesurer puis admettre le terrain 3D.
2. Architecture : les verrous mathématiques sont parent unique, `next`
   terminal, lots exacts, complétude du graphe sparse et ledger; LBVH, queues,
   arènes, tri et MSF sont de l'ingénierie qualifiable.
3. Statut : `presumed_exact_structural + partial_refinement`, oui; `exact`, non.
4. $k=2$ : les wedges à deux arêtes sont le bon garde-fou praticable; ne pas en
   faire un backend Gamma$_2$.
5. Terrain quantifié : préflighter coquilles, lots égaux, largeur de tâche,
   fallback et replay; aucune borne petite implicite.
6. Dendrogramme : réserver maintenant le ledger et les hooks compacts, construire
   le routeur après fermeture du SLO, et déclarer séparément son budget.

GCP non utilisé.
