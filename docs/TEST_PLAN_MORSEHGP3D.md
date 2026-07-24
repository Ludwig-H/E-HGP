# Plan de validation de MorseHGP3D

> [!IMPORTANT]
> Ce document est un **contrat de validation pour une implémentation future**. Les seuils de temps sont des objectifs réfutables sur une configuration G4 donnée, pas des garanties de complexité ni des performances déjà mesurées.

## 1. Objet du plan

MorseHGP3D doit construire, pour un nuage fini $X\subset\mathbb{R}^{3}$ et tous les ordres $1\leq k\leq K_{\max}\leq10$, une hiérarchie HGP en échelle. Le plan vérifie séparément :

1. la validité de chaque prédicat géométrique ;
2. la complétude du catalogue critique lorsqu'elle est annoncée ;
3. l'exactitude des forêts horizontales et des applications verticales ;
4. l'absence de faux certificat dans les modes incomplets ;
5. la robustesse numérique et le traitement explicite des dégénérescences ;
6. la reproductibilité, le débit, la mémoire et la reprise sur une VM G4 Spot.

Le plan vérifie en plus l'objectif architectural premier : le chemin produit doit être beaucoup plus léger que `HGP-old` et ne doit jamais matérialiser la mosaïque de Delaunay d'ordre $K$, tous les parents top-$m$ ou Gamma global. Les supports et Gamma exhaustifs restent autorisés dans l'oracle borné $n\leq14$; l'atlas cellulaire top-$m$ est séparément gelé à $n\leq8$.

Le résultat mathématique testé est prioritaire sur toute mesure de qualité par rapport à des étiquettes « terrain ». Un bon score de clustering ne compense jamais une erreur de hiérarchie.

Chaque campagne valide aussi [`implementation_status.toml`](implementation_status.toml) : une phase ne peut être marquée fermée sans porte satisfaite et preuves référencées.

## 2. Contrat mathématique observable

### 2.1 Filtration

Pour $t\geq0$, on note

$$L_k(t)=\left\lbrace y\in\mathbb{R}^{3}:\#\bigl(X\cap\overline{B}(y,\sqrt{t})\bigr)\geq k\right\rbrace.$$

La sortie observable doit représenter les composantes de cette filtration, ou le périmètre réduit explicitement défini par la spécification active. Les niveaux sont toujours stockés et comparés comme des distances au carré.

Pour $n\geq1$, les tests calculent $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Si $s_{\max}\geq2$, la dernière profondeur générique attendue est $m_{\star}=s_{\max}-2$; pour $s_{\max}=1$, seuls le minimum de rang un et la forêt triviale sont attendus. Les campagnes couvrent explicitement $K_{\max}=1$, $n<K_{\max}$, $n=K_{\max}$ et $n\geq K_{\max}+1$; en particulier, $n=K_{\max}=10$ donne $s_{\max}=10$ et $m_{\star}=8$, jamais un événement de rang onze.

Pour une sphère candidate $(c,r)$, on pose

$$I=X\cap B^{\circ}(c,r),\qquad U=X\cap\partial B(c,r),\qquad s=\lvert I\rvert+\lvert U\rvert.$$

Sous les hypothèses de position générale du mode certifié, $U$ est affine indépendant, $1\leq\lvert U\rvert\leq4$ et $c\in\mathrm{relint}\,\mathrm{conv}(U)$. Le catalogue doit enregistrer le support, le rang fermé $s$, le niveau $r^2$ et les ordres auxquels l'événement intervient. En particulier, un événement de rang $s$ porte la naissance d'indice $0$ à l'ordre $s$ et la selle d'indice $1$ à l'ordre $s-1$, lorsque ces ordres appartiennent au périmètre demandé.

### 2.2 Deux périmètres de sortie à ne pas confondre

Chaque exécution indique un `profile` :

| valeur | objet comparé | exigence |
|---|---|---|
| `hgp_reduced` | toutes les composantes à $k=1$, puis les K-polyèdres non triviaux, leurs unions de points et leurs fusions pour $k\geq2$ | conformité à l'EMST à $k=1$, puis réduction des composantes de Gamma exhaustif |
| `full_pi0` | toutes les composantes de $L_k(t)$, y compris les naissances et la généalogie triviales | comparaison complète à l'oracle topologique ; statut certifié seulement si la spécification le permet |

Un test ne doit jamais accepter une sortie `hgp_reduced` comme preuve implicite de `full_pi0`. À l'ordre $k\geq2$, les unions de points associées aux composantes forment en général une **couverture** et non une partition ; leur recouvrement est donc autorisé et doit être testé.

Le contrat actif est v2. `hgp_reduced` exact exige `reconstruction_contract_id=hgp-reduced-v2`, `proof_basis=gamma_exhaustive_reference` et `effective_backend=reference_cpu`. Une sortie issue du flot Gabriel brut exige `proof_basis=gabriel_positive_connectivity`, `forest_semantics=partial_refinement`, `require_exact=false` et un statut non exact. Les tests T0 rejettent toute autre combinaison, notamment la base v1 `reduced_manuscript_theorem_5` et la base future non activée `incidence_complete_reduction_proved`.

### 2.3 Modes et statuts d'exécution

Le mode demandé vaut `certified` ou `budgeted`; `forest_semantics` vaut `exact`, `partial_refinement` ou reste absent si aucune forêt n'est publiée. Le statut public obtenu reste distinct :

| statut public | signification testable | assertions autorisées |
|---|---|---|
| `exact` | catalogue complet sur le périmètre annoncé, tous les signes décisifs certifiés, lots de niveaux égaux fermés, aucune limite atteinte | égalité avec l'oracle et assertions d'absence |
| `conditional` | chaque événement émis est certifié, mais la complétude du catalogue ou des attaches n'est pas prouvée | validité des événements présents seulement |
| `budget_exhausted` | un budget explicite a interrompu une fermeture; le résultat éventuel est conditionnel | validité des événements présents et raison d'arrêt |
| `unsupported_degeneracy` | l'entrée sort du domaine exact déclaré, par exemple plateau non pris en charge | diagnostic déterministe, aucune hiérarchie présentée comme exacte |
| `numeric_failure` | un prédicat n'a pas pu être décidé ou rejoué | aucune sortie scientifique exploitable |

Un mode `budgeted` obtient `public_status=conditional` ou `public_status=budget_exhausted`, même si son rappel mesuré vaut $100\%$ sur tous les exemples connus. Avec `forest_semantics=partial_refinement`, il rend un `PartialScope` contenant profondeurs et ordres fermés, cellules canoniques closes et loci non résolus; il ne permet aucune assertion d'absence et son statut ne vaut jamais `exact`. Une réponse d'événements seuls omet `forest_semantics`. Si `require_exact=true`, toute impossibilité de produire `exact` doit provoquer un échec explicite, jamais une rétrogradation silencieuse; `forest_semantics=partial_refinement` est refusé à la validation.

## 3. Architecture générale des tests

Les tests sont répartis en sept niveaux. Un niveau ne devient bloquant qu'après validation de tous les niveaux qui le précèdent.

| niveau | objet | oracle principal | fréquence minimale |
|---:|---|---|---|
| T0 | schémas, déterminisme des générateurs, documentation et formats | validation statique | chaque changement |
| T1 | prédicats, miniballs, rangs et égalités de niveaux | arithmétique exacte indépendante | chaque changement |
| T2 | catalogue et hyperarêtes | oracle exhaustif $n\leq12$, étendu à $n\leq14$ | PR puis nocturne |
| T3 | forêts, couvertures et flèches verticales | graphe exhaustif à chaque seuil | PR puis nocturne |
| T4 | implémentations CPU/GPU et variantes de noyaux | tests différentiels et métamorphiques | nocturne G4 |
| T5 | robustesse, dégénérescences et charges adversariales | `exact`, `unsupported_degeneracy` ou statut budgétaire attendu | nocturne et release |
| T6 | performance, mémoire, streaming et reprise Spot | budgets et artefacts de mesure | jalon G4 et release |

Une optimisation GPU ne peut être fusionnée si elle modifie une sortie canonique T1–T4 sans que le changement soit justifié dans la spécification mathématique.

## 4. Oracle exhaustif indépendant

### 4.1 Périmètre

L'oracle exhaustif est obligatoire pour $n\leq12$ dans la suite de PR et pour $n\leq14$ dans la suite nocturne ou de release. Il couvre tous les ordres $1\leq k\leq\min(K_{\max},n)$, et pas seulement $k=10$.

L'oracle doit être développé séparément du backend :

- aucune réutilisation des noyaux CUDA de propositions, de rang ou de miniball ;
- aucun partage du code de réduction hyper-Kruskal, hormis le schéma sérialisé ;
- décisions par rationnels, expansions exactes ou multiprécision avec borne de signe ;
- entrées entières ou rationnelles pour les fixtures exactes ;
- double implémentation recommandée pour les cas délicats : bibliothèque géométrique exacte et référence multiprécision directe.

Une comparaison CPU/GPU n'est pas un oracle indépendant si les deux chemins utilisent le même prédicat fautif.

### 4.2 Énumération du catalogue critique

Pour chaque sous-ensemble $U\subseteq X$ de taille $1$ à $4$, l'oracle :

1. vérifie son rang affine ;
2. construit exactement le centre équidistant dans $\mathrm{aff}(U)$ ;
3. teste $c\in\mathrm{relint}\,\mathrm{conv}(U)$ par signes barycentriques exacts ;
4. classe tous les points de $X$ comme intérieurs, frontaliers ou extérieurs ;
5. rejette le candidat si le shell réel n'est pas exactement celui représenté, sauf traitement explicite d'un plateau ;
6. calcule $s=\lvert I\rvert+\lvert U\rvert$ et conserve les événements pertinents jusqu'au rang $s_{\max}$ ;
7. déduplique par la clé canonique `(interior_ids, boundary_ids, level_exact)`.

L'oracle compare au backend :

- la présence de chaque événement ;
- l'absence de chaque faux événement en mode `certified` lorsque le statut public vaut `exact` ;
- $I$, $U$, $s$, $r^2$ et la classe de dégénérescence ;
- les ordres de naissance et de selle ;
- l'égalité exacte ou l'ordre strict entre niveaux.

### 4.3 Flux direct de supports sensible à $H_0$

Le chemin produit est testé indépendamment de l'oracle combinatoire. Pour les paires de nœuds LBVH, le test recalcule exactement la borne $\max\phi$ avec $\phi(x,u,v)=(x-u)\mathbin{\cdot}(x-v)$ sur les extrémités des trois AABB. Un prune de rang n'est valide que si au moins $s_{\max}-1$ identifiants distincts sont strictement intérieurs à toutes les boules du produit. Les nœuds témoins doivent former une antichaîne canonique de plages Morton deux à deux disjointes et disjointes des plages supports; seule la cardinalité de leur union est comptée.

Les fixtures couvrent un produit entièrement prunable, une borne témoin nulle qui ne doit jamais être acceptée, une égalité shell exclue par une paire-ancre réelle, des plages supports partiellement superposées, une paire longue absente des listes locales, un prune devenu faux après mutation d'une seule borne et une exécution sans aucun prune qui retombe sur toutes les paires feuilles. Faute de certificat ancre indépendant, une borne témoin nulle ou positive descend. Deux témoins identiques, un ancêtre avec son descendant et un témoin recouvrant une plage support doivent chacun invalider le certificat. Pour chaque $n\leq14$, les supports feuilles acceptés et les diagnostics doivent coïncider avec l'énumération exhaustive, indépendamment du chunking et de l'ordre de parcours.

Le premier batch CUDA P1 teste une borne dirigée négative, une borne non négative, la capacité fixe, l'ordre canonique des requêtes, une permutation complète du transcript, deux epochs successives et une queue sentinelle intacte. Les falsifications modifient séparément l'identité de $C$, l'index de requête, le bit d'upper, l'epoch, la queue et une descente transformée en faux `strict_interior`; ce dernier cas doit échouer lors du recalcul rationnel CPU puis empoisonner uniquement le contexte concerné. P1 ne possède encore ni corde `escape`, ni agrégation au seuil $T$, ni décision de prune global : ces tests appartiennent à P2.

La façade terminale commune rejoue fraîchement les voies paire et supérieure, exige leurs frontières vides et conserve leurs digests de nuage et de LBVH séparément, car leurs domaines SHA-256 diffèrent volontairement. Sur le tétraèdre régulier elle normalise six paires, quatre triangles et un tétraèdre; une source budgétaire ne publie aucun payload. Un flux synthétique par chunks d'un record doit dépasser sa capacité de sortie résidente cumulée sans conserver l'historique des chunks.

Le lot court `10.2-RCPU` vérifie le journal factorisé des bras sans lancer l'oracle Gamma. Le tétraèdre régulier doit produire 11 familles et 28 graines, avec histogrammes par arité `6/4/1` et `12/12/4`; sa selle tétraédrique reconstruit les quatre faces opposées. Son rejeu source compte exactement deux passes de digest, 15 projections, 26 rôles et sept lots, sans seconde arène. La fixture obtuse exige $F_u=(I\cup U)\setminus\lbrace u\rbrace$ et interdit la substitution fautive $U\setminus\lbrace u\rbrace$. Les deux selles miroir au niveau $169/36$ conservent six graines et deux provenances distinctes de la facette partagée. Un cap de graines inférieur d'une unité échoue sans payload avant le rejeu source; une identité retirée ou une jointure de rôle mutée échoue au rejeu. Une façade de même cardinalité issue d'un nuage translaté doit être rejetée par les digests avant reconstruction; une mutation locale de support sous un certificat inchangé doit ensuite être rejetée par le digest d'événement. Le contrôle statique et symbolique interdit le vérificateur allouant une seconde arène, miniball, `critical_arm`, Gamma, cofaces et l'archive historique dans le target produit. Aucun benchmark long n'est requis pour ce jalon linéaire.

Le lot court `10.3-RCPU` vérifie la forêt directe exacte limitée à $k=1$. Deux points à distance deux doivent lier leurs bras aux deux feuilles du snapshot strict au niveau un, puis créer un nœud binaire. Les trois points collinéaires `(-2,0,0)`, `(0,0,0)`, `(2,0,0)` produisent deux selles simultanées au niveau un : les deux occurrences du singleton central gardent leur provenance, mais le quotient crée une unique multifusion ternaire, jamais une chaîne binaire. Pour `(-1,0,0)`, `(0,3/2,0)`, `(1,0,0)`, les deux arêtes courtes fusionnent au niveau $13/16$ et l'arête longue devient une continuation $q=1$ au niveau un, avec deux liaisons vers la racine antérieure, zéro enfant et zéro nouveau nœud. Les rejeux streaming 10.1, 10.2 et 10.3 doivent tous réussir sans seconde sortie persistante. Une cible remplacée par le parent créé dans le même lot, un nœud inventé sur $q=1$, un journal 10.2 falsifié et un budget de liaison inférieur d'une unité doivent être rejetés; ce dernier échoue avant rejeu source et sans payload, et son vérificateur ne certifie aucune arène non visitée. Le contrôle statique et symbolique impose le target isolé, deux DSU distincts, l'ordre snapshot--quotient--commit, les bornes $2n+5J_1-2$ et $O(n+B_{\max})$, et l'absence du chemin géométrique historique. Aucun benchmark long n'est requis.

Le lot court `10.4-RCPU` vérifie la partition exacte des $k+1$ facettes de chaque selle directe. La fixture obtuse de rang trois doit réutiliser deux bras stricts et ajouter une unique suppression intérieure de niveau égal. La frontière collinéaire $x_i=(i-5,0,0)$ pour $0\leq i\leq10$ doit produire, à l'ordre dix et au niveau 25, le support $\lbrace0,10\rbrace$, les intérieurs $\lbrace1,\ldots,9\rbrace$, deux bras stricts et neuf graines égales distinctes; le rejeu streaming complet doit réussir. Un cap égal inférieur d'une unité échoue avant rejeu et allocation, et une identité intérieure falsifiée est rejetée. Le contrôle statique et symbolique impose les bornes $J+P\leq10E$ et $3n+20E$, la réutilisation sans copie des bras 10.2, le scratch fixe de dix identifiants et l'absence de Gamma, miniball, cellules, cofaces globales et mosaïque. Le noyau root-only séparé couvre lot vide, $q=1$ avec doublons, chaîne d'hyperarêtes, groupes aux identifiants entrelacés, permutation intra-arête, CSR mal formé, cinq caps juste insuffisants et mutations du résultat; il ne doit jamais annoncer une action HGP ni accepter l'autorité externe des racines. Aucun benchmark long ni GCP n'est requis.

Le lot court `10.5a-RCPU` vérifie le locator positif sparse indépendamment de toute géométrie. Deux clés distinctes de dix `PointId` sous un masque d'empreinte nul doivent rester distinctes après comparaison complète. Une clé insérée dans le lot courant reste `unresolved` pour les requêtes de ce lot puis devient `positive` au suivant. Une union modifie la racine observée au lot suivant sans réécrire le slot ni l'arène de clé. Une liaison exacte incompatible après les unions explicites rejette atomiquement aussi une union sans rapport; une union explicite qui rend deux handles compatibles accepte le doublon sans dupliquer sa clé. Le test couvre également clé non canonique, token nul, capacité durable juste insuffisante et plafonds d'initialisation. Le digest initial doit être non nul; chaque commit accepté, même vide ou composé seulement d'un doublon compatible, doit changer le stamp, tandis qu'un rejet le conserve. Deux locators ayant les mêmes autorité et compteurs mais des clés, unions ou témoins committés différents doivent avoir des digests distincts. Le vérificateur structurel doit rejouer le DSU et la chaîne SHA-256, recalculer chaque empreinte et retrouver chaque clé, puis rejouer le premier slot libre de chaque insertion dans l'ordre committé. Une permutation physique de deux slots en collision qui conserve clés, digest, compteurs, DSU et retrouvabilité finale doit échouer uniquement sur ce fait chronologique. Le fingerprint public doit rester identique sur toute clé canonique et borner ses lectures à dix identifiants même si `point_count` est invalide. Les mutations de digest, slot, parent ou compteur et tout span surdimensionné sont rejetés avant promotion; l'autorité externe reste explicitement non rejouée. Le contrôle statique et symbolique impose le target isolé, la table plate, l'ordre requêtes--unions candidates--compatibilité--commit, le miss `unresolved` et l'absence de map, Gamma, miniball, cellule, coface ou Delaunay. Aucun benchmark long ni GCP n'est requis.

La suite courte `10.5b-RCPU` valide séparément la sonde, le top-k borné, la miniball isolée et le pas composé avant la promotion à `validated_host_software`. La sonde conserve `query_key`; son rejeu frais lie locator, clé, témoin et budget, puis rejette séparément une clé canonique substituée, un audit modifié, un handle positif modifié et tout payload caché après épuisement. Les relations exactes exigent `full=equal+1` sur hit, `full=equal` et au moins une visite de slot sur miss complet, et zéro handle ou témoin de liaison sur les deux épuisements. Chaque miniball de facette couvre singleton, dégénérescences, supports multiples et cardinal dix; construction et vérification rejouent chacune jusqu'à 385 supports, soit au plus quatre passes et 1540 examens de supports candidats pour les deux facettes d'un pas.

Le test top-k doit comparer les résultats complets aux deux ordres `near_first` et `far_first`, puis diminuer d'une unité chacun des sept plafonds : visites de nœuds, expansions internes, évaluations exactes de bornes AABB, évaluations exactes de distances, frontière, meilleurs voisins et shell. Chaque arrêt doit conserver son audit opérationnel mais ne rendre aucune `TopKPartition`; le shell provisoire qui déborde puis disparaît après baisse du cutoff est distingué du shell final trop grand. Le test composé couvre hit source, hit cible strict, miss cible, choix canonique identique, successeur non strict, contradiction et épuisements des deux sondes ou du top-k, toujours sans mutation, insertion, singleton ni attache. Un ordre de parcours LBVH invalide doit être rejeté avant toute branche courte, y compris lorsqu'un hit source évite entièrement la requête top-k.

Deux fixtures permanentes ferment le théorème de segment décrit dans [DESCENTE_FACETTE_SPARSE_PHASE10.md](math/DESCENTE_FACETTE_SPARSE_PHASE10.md). `AC` vers `DE` exige $\beta(F)=a=33/2$, $d=31/2<a$ et un segment fermé strict; `LR` vers `LP` exige $\beta(F)=d=a=1$, $\beta(G)=5/16$, un segment source-ouvert strict et un booléen de segment fermé faux. Le rejeu doit établir exactement $\beta(G)<\beta(F)\leq a$ et que le segment fermé est strict sous $a$ si et seulement si $d<a$. La suite reste courte et CPU : elle ne valide ni checkpoint, ni reprise, ni annulation coopérative, ni SLO 50 k sous la seconde, ni profil 10 M+, et elle n'appelle pas Gamma.

La suite courte `10.5c-RCPU` fixe d'abord une fermeture exacte sur six points. Elle impose $F_0=\lbrace0,1,2,4\rbrace$ au niveau 52, $F_1=\lbrace1,2,3,5\rbrace$ au niveau $85/4$ et $F_2=\lbrace1,2,3,4\rbrace$ au niveau $325/16$. Le pas $F_0\to F_1$ doit être `complete_unresolved_strict_successor_not_bound`, puis $F_1\to F_2$ doit être `complete_relative_strict_successor_positive_hit`. Avec les graines $F_0$ et $F_1$, le suffixe $F_1\to F_2$ est partagé et $F_2$ est un terminal positif non réévalué; les comptes exacts sont $V=3$, $E=2$, $T=1$, $B=2$ et $Q=2$. La source construit une miniball puis en réutilise une, les successeurs en construisent deux sans réutilisation, et les identités $E=V-T$ et $E\leq Q\leq B\leq V$ doivent être recertifiées.

La même fixture est rejouée après permutation complète des enregistrements de graines, puis avec les parcours LBVH `near_first` et `far_first`. La permutation conserve le résultat complet grâce aux indices durables; les deux parcours conservent le graphe, les projections et les comptes scientifiques canoniques. L'ajout de références de graines dupliquées conserve le graphe interné et le travail local, tandis que les projections et compteurs de doublons reflètent exactement leur multiplicité. Un masque d'empreinte nul force les collisions du mémo de nœuds et du cache auxiliaire : chaque candidat de slot doit encore être distingué par sa clé complète. Une source déjà positive exige exactement une sonde, zéro miniball, zéro top-k et zéro arête. Un plateau fixe $G=F$ reste terminal non strict. Deux sources non strictes distinctes qui produisent le même successeur canonique non interné doivent construire deux miniballs source, construire une seule fois celle du successeur, la réutiliser une fois, compter trois miniballs distinctes en cache et ne publier aucune arête.

Les préflights diminuent séparément d'une unité le plafond de références de graines, le nombre de slots du mémo et la capacité de nœuds nécessaire pour préinterner toutes les racines distinctes; aucun de ces trois échecs ne lance de géométrie. Les arrêts d'exécution couvrent ensuite la capacité d'un nouveau nœud strict, le nombre d'appels 10.5b et le budget local de sonde du successeur. Le témoin strict diagnostique peut subsister après saturation des nœuds, mais ni arête ni référence sortante ne doit être publiée : toute demi-arête est interdite. Un ordre de parcours invalide reste rejeté avant la fermeture.

Le vérificateur frais rejoue le snapshot commun et rejette une mutation du locator commise après le build, ainsi que les mutations séparées d'une cible d'arête remplacée par une boucle, d'un niveau strict, d'un terminal, d'une projection de graine, des compteurs locaux ou agrégés et du drapeau de publication d'attache. Des tableaux observés hostiles dépassant les plafonds de nœuds, d'arêtes ou de projections de graines sont refusés avant leur scan, avant le contrôle du snapshot et avant tout rejeu. Une fixture à $K=10$ impose une clé canonique de dix `PointId`, un seul scratch local et aucun développement de l'univers global. Le contrôle statique et symbolique impose le target isolé, les deux tables plates, les comparaisons de clés complètes, les symboles 10.5b/miniball/LBVH attendus, l'absence de Gamma, Delaunay, catalogue global, cellule, coface, singleton et `GatewayAttach`, ainsi que le maintien de `partial_refinement` et `not_claimed`. Aucun benchmark long, SLO produit ou profil 10 M+ n'est validé par ce lot.

La suite courte `10.6-RCPU` valide la première incidence d'une unique facette fournie. Elle couvre $X=F$ et `complete_no_coface`, un minimiseur extérieur unique, un support de coface qui utilise un ancien point intérieur absent du support source, puis plusieurs co-minimiseurs exacts. Les parcours `near_first` et `far_first` conservent le même niveau et le même vecteur canonique. Une feuille dont la borne radiale est égale à l'incumbent est visitée, tandis qu'un prune publié repose sur une inégalité rationnelle strictement positive.

La frontière $K=10$, $n=11$ exige deux passes source de 385 supports, soit 770 examens, puis exactement 176 supports contenant le point extérieur ajouté. La coface logique de onze points est classifiée sans créer de clé persistante de largeur onze; seul un support local de cardinal au plus quatre est matérialisé. Les neuf plafonds sont rejoués à leur valeur exacte puis diminués séparément d'une unité : support source, visites, expansions, bornes AABB, points, supports de cofaces, classifications, frontière et co-minimiseurs. Tout épuisement doit supprimer le niveau et tous les co-minimiseurs, y compris lorsqu'un shell d'égalité a déjà été observé. La fixture permanente `AC` impose le niveau $33/2$ et exactement les deux co-minimiseurs $D,E$; un point sur la frontière source, un overflow provisoire effacé par un meilleur incumbent, deux co-minimiseurs extérieurs à $K=10$ et un différentiel sur toutes les facettes propres d'un nuage de six points complètent la falsification courte.

Le vérificateur frais doit refuser avant publication un vecteur de co-minimiseurs surdimensionné, un niveau, un minimiseur, un support, une borne ou un compteur falsifié, ainsi que tout drapeau prétendant avoir matérialisé une structure interdite. Le contrôle statique et symbolique impose le target isolé et l'absence de catalogue global de facettes ou cofaces, Gamma, cellule top-$m$, mosaïque de Delaunay d'ordre supérieur, `GatewayAttach` et mutation de forêt. Le lot reste CPU, court et sans benchmark long; il ne valide ni le SLO 50 k, ni 10 M+, ni la complétude globale des facettes-portes et ne change aucun statut public.

La suite courte `10.7-RCPU` doit d'abord rejouer 10.2 et 10.4, reconstruire les $R=\sum_e\lvert S_e\rvert$ suppressions de toutes les selles directes, vérifier $R\leq11J$, puis dédupliquer les facettes par clé complète en conservant chaque provenance. Une même facette issue de plusieurs événements ne déclenche qu'un seul appel 10.6. L'ABI amont imposant un ordre canonique indexé, la permutation métamorphique porte sur les points bruts et doit reproduire le même flux d'événements certifié; une permutation directe du tableau d'événements est au contraire une falsification d'autorité et doit être rejetée. Sous les parcours LBVH `near_first` et `far_first`, les clés, niveaux, co-minimiseurs, lots `(cardinal, niveau exact)` et projections temporelles restent identiques.

Le différentiel indépendant énumère, pour $n\leq14$, toutes les cofaces à un point des seules clés directes proposées et compare exactement $\lambda(F)$ ainsi que la famille complète $M(F)$. Il couvre au moins une provenance avec $\lambda(F)<a_e$, une avec $\lambda(F)=a_e$, une facette partagée par plusieurs provenances, le résultat sans selle et la fixture permanente à cinq points. Dans cette dernière, la suppression de $B$ dans la selle future $ABC$ propose $AC$, puis le journal conserve exactement les tokens factorisés ajoutant $D$ et $E$ au niveau $33/2$. La frontière $K=10$ vérifie qu'une coface logique de onze points reste `(facet_key, added_point_id)` et qu'aucune clé persistante de onze `PointId` n'existe.

Chaque plafond global — familles sources parcourues, références, clés distinctes, `PointId` des clés, candidats, lots, références de lots et stockage logique — est testé à sa valeur exacte puis diminué d'une unité. Le budget 10.6 commun à chaque clé est exercé séparément sur ses neuf plafonds locaux. Le compteur déclaré vaut $D$ et le garde statique impose l'unique site d'appel 10.6 dans la boucle des $D$ clés; l'ABI ne possède pas de compteur d'injection séparé. Une autorité divergente, une relation falsifiée $\lambda(F)>a_e$, une projection, un lot, un support ou un compteur muté, ainsi qu'un tableau observé hors budget doivent échouer sans aucune des cinq arènes scientifiques. Sous une source 10.4 certifiée, `complete_no_coface` est mathématiquement inaccessible puisque le point retiré fournit déjà une coface; sa branche défensive reste imposée par le garde, tandis qu'une source falsifiée est rejetée avant de pouvoir l'atteindre. Le contrôle statique et symbolique interdit l'univers $\binom{n}{k}$, les cofaces globales, Gamma, cellules, mosaïque de Delaunay d'ordre supérieur, locator, quotient, racine, union, forêt, `GatewayAttach`, résultat enfant 10.6 persistant et clé de largeur onze.

Le dernier rejeu GCC Release valide 11 CTests en 3,84 secondes. Il couvre le différentiel explicite pour $n=6\leq14$, la fixture sans selle, la canonisation sous permutation brute, les deux ordres LBVH, la facette partagée, les relations stricte et égale, `AC` vers $D,E$, $K=10$, les huit plafonds globaux et les neuf plafonds 10.6 exacts puis moins-un, les autorités étrangères, les compteurs logiques sur- et sous-déclarés, les mutations croisées et l'isolation symbolique. Aucun benchmark long ou GCP n'est requis; le suivi est `validated_host_software`, sans promotion de la Phase 10 ni du statut public.

La suite `10.8-ORACLE` est indépendante du binaire produit. Pour chaque cas `RelevantGP` avec $3\leq n\leq14$ et $2\leq k\leq\min(10,n-1)$, elle compare aux coupes ouvertes puis fermées : la partition Gamma restreinte à l'alphabet $V_k$ des naissances et facettes directes, puis, sur ce même $V_k$, l'hypergraphe des seules selles directes et cofaces minimisantes 10.7 avec toutes leurs suppressions comme relais. Les composantes de facettes, les points couverts par les composantes complètes et la généalogie sur toute la suite des coupes sont comparés séparément; aucun test n'exige la présence de toute facette Gamma hors de $V_k$. Le premier écart doit conserver un chemin Gamma canonique et la première facette ou coface absente. E5 passe sans ablation; le retrait de `ACD` et `ACE` doit isoler `AC` à la coupe fermée $33/2$ puis conserver la divergence généalogique jusqu'à `ABC` et après une éventuelle reconvergence. Les bornes de domaine, les dégénérescences, la permutation des cofaces égales, les jointures catalogue--Gamma et les mutations de témoin échouent fermé. La campagne déterministe validée couvre huit cas $k=2$ et quatre cas $k=3$ pour $5\leq n\leq8$, puis $n=11$, $k=10$; tout nouvel échec générique fait échouer la suite avec ses coordonnées et son témoin afin de devenir une fixture minimale permanente. Les 15 tests dédiés passent en 7,35 secondes et aucun des treize cas non ablatés ne montre d'écart. Aucun succès ne promeut une hypothèse au-delà de `open_bounded_evidence_only`.

La suite courte `10.9-RCPU` rejoue d'abord 10.7 depuis toutes ses autorités fiables, puis vérifie les deux arènes de localisation contre un locator 10.5a vivant et gelé. Sur E5, `AC={1,3}` est partagé par les candidats `ACD` et `ACE`; ses deux occurrences doivent pointer vers un unique token et déclencher une seule sonde. Un masque d'empreinte nul force les collisions : seuls les hits à clé complète conservent le handle racine et le témoin de liaison, tandis que les autres tokens restent `latent_unresolved` sans handle, singleton ou isolation.

À la frontière $K=10$, les onze candidats factorisant la même coface logique de onze points produisent 121 projections mais seulement onze tokens distincts, tous de cardinal dix. Chaque projection est comparée au reconstructeur public 10.7; aucune arène, structure ou symbole 10.9 ne peut contenir une clé de onze `PointId`. Les invariants obligatoires sont $P\leq11C$, $D\leq P$, une sonde par clé distincte et un stockage logique $O(P+KD)$.

Chaque plafond global — candidats parcourus, projections, clés distinctes, points de clés, visites de slots cumulées, sauts de parents cumulés et stockage logique — est testé à sa valeur exacte puis diminué d'une unité. Les deux plafonds de la sonde individuelle sont exercés séparément. Toute saturation produit une décision budgétaire et deux arènes vides; elle ne peut pas être reclassée en miss. Un commit vide après construction doit rendre le stamp observé périmé; une clé latente liée ensuite devient positive seulement dans une nouvelle construction, ce qui interdit définitivement l'interprétation d'un miss comme absence.

Le succès vide $C=P=D=0$ exige deux arènes vides, aucune sonde et exactement deux contrôles de stamp. La porte d'entrée rejette séparément autorité nulle ou divergente, token de rejeu nul, locator non certifié, LBVH étranger et ordre invalide; le cap de candidats doit échouer avant le rejeu 10.7. Le vérificateur borne les tailles observées et les points de clés avant tout rejeu 10.7, puis rejette les mutations d'indices, positions supprimées, lots sources, liens vers tokens, clés, dispositions, handles, témoins, compteurs, stamp et drapeaux hors portée. Le garde statique et `nm` imposent les deux dépendances produit directes, un unique site de rejeu 10.7, de reconstruction et de sonde, le contrôle du stamp dans la boucle et les stamps `const`; ils interdisent `apply_batch`, tableau de onze points, map globale, Gamma, Delaunay, cellule, catalogue, quotient, DSU, racine, forêt et `GatewayAttach`. Le test confirme enfin que `locator_snapshot_batch_level_alignment_claimed` reste faux : le snapshot courant n'est pas une preuve d'état pré-lot.

La suite courte `10.10-RCPU` construit un locator à collisions forcées en six commits : insertion de `A` sur le handle 5, lot vide, union `5--4` avec insertion de `B` sur 2, union `4--1` avec doublon compatible de `A`, union redondante `5--1` avec insertion de `C` sur 3, puis union `2--1`. Les requêtes monotones couvrent tous les préfixes de zéro à six. Elles doivent observer successivement `A` sur les racines 5, 4 et 1 en conservant son premier témoin, `B` sur 2 puis 1, et `C` sur 3; une clé encore future arrête la recherche comme vide logique, alors qu'une clé réellement absente atteint le vide physique.

Le vérificateur 10.5a doit accepter l'état authentique puis rejeter une permutation physique de deux slots en collision qui conserve pourtant clés, indices, digest, retrouvabilité finale, DSU et compteurs. Ce cas fixe le fait `committed_slot_insertion_chronology_freshly_replayed`. Le sweep doit laisser inchangés toutes les valeurs, adresses et capacités des arènes du locator. Le succès vide initialise seulement le scratch dense et effectue deux contrôles de stamp; la frontière $K=10$ masque une clé au préfixe zéro puis la résout après son commit.

Les onze plafonds du sweep — requêtes, points de clés, handles scratch, exactement $2B$ scans de lots pour préflight puis balayage, unions, sauts de parents des unions, slots cumulés, sauts de parents des requêtes, sortie, slots locaux et parents locaux — passent à leur valeur exacte puis échouent séparément à moins un avec zéro résolution publiée. Le budget séparé du vérificateur structurel couvre avant allocation slots, liaisons, arène de points, handles, unions et lots, puis borne les visites cumulées de lookup sous collisions, de chronologie et du DSU; chaque coût variable passe à sa valeur exacte et échoue à moins un sans lancer la reconstruction du sweep. La suite rejette indices non denses, préfixes décroissants ou hors historique, clés non canoniques, locator ou témoin invalides, stamp périmé, racine ou témoin positif hors forme auto-contenue, compteurs de dispositions, préfixe falsifié et alignement 10.7 inventé. Le checker impose le target isolé, le fingerprint public, le rejeu structurel et chronologique borné, le DSU identitaire, le terminateur futur et les reliquats globaux; il interdit `apply_batch`, sonde courante, copie des parents finaux, Gamma, cofaces globales, Delaunay, quotient, forêt, `GatewayAttach` et toute revendication `source_batch_alignment_claimed=true`. Aucun benchmark long, SLO 50 k ou profil 10 M+ n'est validé par ce lot.

La suite courte `10.11-PSTAMP` réutilise le locator à six commits de 10.10. Elle capture le stamp vivant après chaque commit, notamment avant et après le commit vide, puis demande les préfixes zéro à six avec au moins un préfixe répété. Chaque sortie doit égaler le stamp capturé correspondant, rester dans l'ordre des requêtes et conserver la répétition; le préfixe final doit égaler le stamp vivant. Une clé insérée, une union, un doublon compatible et le lot vide doivent chacun modifier exactement les champs engagés par la transition canonique 10.5a. Le préfixe zéro est aussi comparé à un vecteur initial indépendant de l'exécution du sweep.

Les huit caps PSTAMP — requêtes, exactement $2B$ scans de records, $M$ scans de slots si $I_B>0$, $I_B$ entrées d'index, $U_B$ records d'union, $I_B$ liaisons, $P_B$ points et $I_B\,\mathrm{sizeof}(\texttt{size\_t})$ octets scratch — passent à leur valeur exacte puis échouent séparément à moins un avec zéro stamp publié. Un cas sans liaison active doit certifier zéro scan de table et zéro scratch. La suite rejette les requêtes décroissantes ou hors histoire, un index de lot non dense, un record incohérent et un locator non certifié; elle vérifie qu'un débordement ou un budget insuffisant reste distinct d'une histoire invalide.

Le vérificateur public reçoit les requêtes, le locator et le budget fiables, borne le stockage observé avant le rejeu, reconstruit fraîchement les stamps et rejette toute mutation du stamp vivant, d'un stamp historique, d'un compteur, d'un besoin, d'un fait, de la décision ou de la portée. Avant et après chaque appel, le test compare toutes les valeurs, adresses et capacités des arènes du locator. Le checker impose un builder canonique unique partagé par le commit et le rejeu, puis un helper chronologique unique partagé par la vérification structurelle 10.5a et PSTAMP; il interdit toute seconde transcription de la chaîne, `apply_batch`, DSU, copie des parents, Gamma, coface globale, Delaunay, quotient, forêt, `GatewayAttach`, ainsi que tout champ ou fait prétendant aligner 10.7 sur le locator. Aucun benchmark long ni GCP n'est requis par ce lot.

La suite courte `10.11-CLOCK` doit d'abord figer des vecteurs canoniques indépendants pour `compute_exact_direct_sparse_gateway_candidate_scientific_identity`. Chaque champ du schéma, de l'ordre LBVH, des compteurs et des quatre identités amont, puis chaque champ des cinq arènes 10.7, audits complets 10.6 compris, doit influencer le digest. Pour $R$ projections, $D$ tokens, $C$ candidats, $S$ lots, $I$ références de tokens et $E$ octets décimaux de niveaux exacts, le test doit recompter le payload hors domaine comme $194+75R+313D+66C+48S+8I+E$. Il doit aussi vérifier les tags d'arène et d'énumération, les booléens sur un octet, les longueurs big-endian et les numérateurs et dénominateurs décimaux canoniques. Les cinq caps de populations, le cap binaire d'un entier exact, le cumul décimal et le cap du payload passent chacun exactement à la frontière puis échouent à moins un; un `BigInt` hostile doit être rejeté par son nombre de bits avant toute conversion décimale démesurée.

La seconde famille de vecteurs cible `compute_exact_direct_sparse_gateway_clock_certificate_digest`. Pour $S$ frontières, la taille hors domaine doit être exactement $136+92S$ octets. Les mutations indépendantes du schéma, de l'autorité, du token de rejeu, de l'identité source, de chaque champ du stamp final, des indices, préfixes et stamps historiques doivent changer le digest; `certificate_digest` et `digest_present`, exclus du payload, sont contrôlés séparément par le vérificateur. Le digest tout-zéro avec présence vraie doit être traité comme une donnée, et le même digest avec présence fausse doit être rejeté. Le cap de frontières et celui des octets passent à leur valeur exacte puis échouent à moins un.

La vérification intégrée doit construire une frontière dense pour chaque lot 10.7, autoriser des préfixes non monotones et répétés, trier déterministiquement les couples `(préfixe, source)`, appeler PSTAMP exactement une fois puis remapper chaque stamp vers l'ordre source. Elle doit rejouer fraîchement 10.7, exécuter le vérificateur structurel borné complet du locator et contrôler le stamp final à l'entrée et à la sortie sous gel intégral des deux sources. Les tests hostiles couvrent frontière omise, dupliquée ou hors histoire, indices sources permutés, tri juste au cap puis moins-un, budgets de scans et scratches exacts puis moins-un, commit vide, stamp final périmé après commit vide et suffixe structurel corrompu après le plus grand préfixe demandé. Ce dernier cas doit échouer lors du rejeu complet du locator même si PSTAMP n'aurait pas à avancer jusque-là.

Les attaques d'autorité doivent distinguer le payload et l'ancre externe : rehasher un certificat muté sous l'ancre inchangée échoue; fournir aussi une nouvelle ancre cohérente ne démontre pas son histoire et conserve `external_clock_authority_replayed=false` avec `conditional_on_caller_clock_authority_replay=true`. Les schémas étrangers, autorités ou tokens nuls ou divergents, digests attendus falsifiés, source 10.7 étrangère, locator étranger, payload rehashé, mutation entre capture et retour et toute revendication de quotient, forêt, attache, statut public ou qualification 50 k/10 M+ doivent échouer ou rester hors portée selon le contrat. Le checker doit imposer l'unique rejeu 10.7, le vérificateur structurel complet, l'unique appel PSTAMP et l'absence de Gamma, coface globale, cellule, mosaïque de Delaunay d'ordre supérieur, `apply_batch`, quotient, racine, union, forêt et `GatewayAttach`. Ce lot ne doit publier ni résultat chronométrique ni promotion avant sa recertification.

La suite courte `10.12-AUTH` réemploie une seule fixture CLOCK et évite une nouvelle campagne combinatoire. Elle ouvre l'autorité sur l'identité 10.7 et le stamp zéro du locator, capture les indices sources dans l'ordre réel `1,0,2,...`, insère un commit vide entre les deux premières frontières et conserve deux captures au même préfixe. Elle exige l'échec atomique d'un doublon, d'un locator revenu à un préfixe antérieur, d'une capture après scellement et d'un cap de scan diminué d'une unité. Le scellement doit remapper exactement la chronologie vers l'ordre source, rester unique et produire un certificat que le rejeu AUTH compose avec CLOCK pour obtenir `external_clock_authority_replayed=true`, `conditional_on_caller_clock_authority_replay=false`, `in_memory_replay_only=true` et `crash_durable=false`. Un contrôle de compilation interdit la copie du journal et exige un move sans exception. La source 10.7 et le locator restent identiques pendant le rejeu; aucun test de course C++ n'est autorisé, car le verrou commun appartient à la précondition.

La suite courte `10.13-TRES` ajoute une seule fixture au binaire CLOCK/AUTH. Tous les lots sont capturés au préfixe zéro, puis une facette choisie parmi les tokens 10.9 est liée dans un commit ultérieur avant scellement. La localisation finale doit la voir positive, tandis que chaque résolution historique correspondante reste `latent_unresolved` sans payload, isolation ni singleton. Le test recompte indépendamment les couples distincts `(préfixe, token)`, exige exactement deux arènes, puis appelle une fois le fresh verifier qui compose 10.9, AUTH et 10.10. Il vérifie explicitement que l'autorité temporelle est rejouée mais que l'autorité scientifique externe des témoins, le gel et la discipline pré-lot restent des prémisses. Aucun nouveau produit cartésien de caps, oracle exhaustif, benchmark long, sanitizer ou GPU n'est requis pour ce jalon.

La suite courte `10.14-QPROP` prolonge cette même fixture, sans créer de matrice supplémentaire. Elle exige les cinq arènes plates, les ranges de suppressions référencés dans 10.9, l'ordre canonique `(source_batch_index, representative_gateway_candidate_index)` et des tranches de racines et d'indices latents strictement croissantes. Deux candidats du même lot qui n'ont aucune projection positive mais partagent un même `temporal_resolution_index` latent doivent appartenir au même composant `latent_only_unresolved`; des candidats de deux lots distincts doivent rester dans des composants distincts. Ce cas exerce le pont latent avant la projection des racines, les doublons idempotents et l'identité publiée de `L(resolution)`, sans jamais créer de racine synthétique.

Le test vérifie aussi $P\leq11C$, les besoins linéaires des cinq arènes, le scratch de tri $\max(P,C)$, ainsi que la présence de plafonds séparés pour les comparaisons des trois heapsorts, les entrées DSU et les sauts de parents DSU, compression comprise. Ces plafonds sont contrôlés par les préflights et compteurs de l'unique fixture; ils ne sont pas déclinés en une campagne exact-puis-moins-un. Un seul appel du fresh verifier doit rejouer 10.13, reconstruire la clôture batch-locale et conserver l'autorité de liaison, le gel et la discipline pré-lot comme prémisses. Une mutation de `historical_component_index` dans `candidate_to_component` doit être rejetée. Le résultat doit garder `latent_facet_means_isolated=false`, `root_birth_continuation_or_multifusion_claimed=false`, `locator_root_union_or_forest_mutated=false` et `gateway_attach_published=false`. Aucun oracle Gamma, benchmark long, sanitizer, GPU ou GCP n'est requis pour 10.14.

Le jalon `10.15-LIFE` réemploie exclusivement cette fixture 10.13--10.14. Elle doit produire les mêmes deux arènes temporelles, le même mapping dense, les mêmes préfixes, tokens, multiplicités, dispositions, racines, témoins, compteurs, besoins, décisions et faits de portée. La revue statique du diff confirme l'ordre de durée de vie : préconstruction des $Q$ records depuis les groupes triés, libération réelle de `sorted` par `swap` avant l'appel 10.10, puis enrichissement en place sans accès ultérieur aux triplets. Sous LP64, l'audit fixe les tailles 24/16/104/48/72/8 et recalcule les deux formules de pic; sur une autre ABI, ces nombres doivent être recalculés plutôt que supposés.

Aucune matrice de plafonds, nouvelle fixture géométrique ou mesure RSS à dix millions n'est ajoutée. Le cas analytique $P=Q=H=10\,000\,000$ sert seulement à vérifier le gain de 160 Mo; il ne qualifie pas un nuage de dix millions, puisque $P=n$ n'est pas établi et que seule $P\leq11C$ est prouvée. La conservation de `BuildArtifacts` par le fresh verifier reste explicitement hors de ce jalon. Aucun benchmark long, sanitizer, GPU ou GCP n'est requis pour 10.15.

Le jalon normatif de fermeture utilise un seul exécutable court et trois fixtures déjà permanentes, sans nouvelle matrice. Le triangle aigu discrimine d'abord `full_pi0` et `hgp_reduced` : ses minima d'arêtes reçoivent des carriers mais aucun nœud, puis le triangle forme un groupe $q_R=0$ et crée une unique naissance réduite sans enfant. Toute multifusion ternaire à ce niveau est une régression bloquante. Le miroir 6.18 impose ensuite que deux selles simultanées partageant seulement un carrier latent $L(h)$ appartiennent à une unique composante $q_R=0$; supprimer les latents avant la fermeture ou traiter les selles séquentiellement produirait deux fausses naissances.

Sur la vraie fixture E5 tridimensionnelle, le même exécutable reconstruit 10.1 et 10.2, garde les minima d'ordre au moins deux latents jusqu'à leur absorption et exige que chaque bras strict termine positivement sous le snapshot pré-lot. Il vérifie que `AC`, bras de `ABC`, atteint la composante gelée contenant le minimum `DE`, sans catalogue de première incidence ni gateway persistante, puis que le groupe au niveau exact $83886/3563$ est une continuation $q_R=1$ sans nouveau nœud. Toutes les hyperarêtes d'un niveau sont projetées ensemble sur les carriers; le groupe compte seulement les racines réduites antérieures, puis engage leurs unions et les minima courants. Le contrôle recertifie le résultat complet. Il n'exécute ni matrice de caps, ni oracle Gamma supplémentaire, ni sanitizer, ni benchmark, ni GPU. Le succès valide cette composition locale discriminante; il ne démontre ni la surjectivité globale des carriers, ni M.1, ne publie aucun statut `exact` et ne qualifie ni 50 k sous la seconde, ni 10 M+.

Le lot court `14C-RCPU` planifie les descentes initiales sans les exécuter. Un singleton sans selle produit d'abord un plan complet sans lane. Sur le tétraèdre régulier, il exige exactement trois lanes de supports deux, trois et quatre, respectivement 6, 4 et 1 familles, 12, 12 et 4 bras, puis 7 lancements avec une tuile de 5 et une borne totale de 304 examens locaux avant fermeture partagée. Les caps de chunks, lanes, lancements et examens sont chacun diminués d'une unité et doivent rejeter avant publication; le dépassement du cap de chunks ne doit conserver aucun chunk source partiel et une tuile nulle est invalide. Le profil streaming découpe plusieurs chunks mais aucune lane ne traverse un lot exact, et chaque lot conserve un snapshot locator commun ainsi qu'une seule fermeture et une seule mémoïsation. Une fixture à deux amas place dans le même lot exact une lane de support deux et une lane de support trois; elle impose la même identité de batch, le même chunk et la même barrière partagée.

La frontière $K=10$ utilise onze points collinéaires et recherche la lane de facettes de cardinal dix issue d'un support de cardinal deux et de neuf points intérieurs. Elle contient une famille, deux bras, un lancement et la borne initiale $2\times4\times385=3080$ examens de supports. Chaque résultat accepté est reconstruit fraîchement; une redistribution coordonnée des lanes n'est pas couverte par le seul prédicat de forme. Le test ne borne ni le travail de la fermeture partagée, ni les difficultés LBVH ou rationnelles, ne lance aucune descente GPU et ne revendique aucune latence ou capacité.

Le lot court `14D-RCPU` utilise le tétraèdre régulier à l'ordre un et un locator dont les quatre singletons sont positifs. Une même session traverse les lots sans reconstruire le plan 14C : le compteur d'ancrage reste exactement un, chaque tentative d'un lot non vide construit une seule fermeture commune, les douze bras de support deux se dédupliquent en quatre clés, et le graphe transitoire est nul dans l'audit entre deux lots. Un cap de bras fixé à onze ou un cap de clés résolues fixé à trois doit produire un diagnostic sans delta et sans avancer le curseur; dans le second cas, aucune fermeture ne commence. Le retry aux caps exacts doit réussir. Un budget 10.5c hors plafond est rejeté avant même de synthétiser le lot vide. Un commit locator vide entre préparation et vérification rend le stamp périmé, refuse l'avancement, puis une nouvelle préparation sous le nouveau snapshot réussit. Un plan 14C falsifié est rejeté dès l'ouverture de session.

Une configuration streaming limitée à un lot par chunk traverse aussi deux chunks, vérifie l'identité propriétaire de chaque lot et termine avec le même ancrage unique et zéro nœud retenu. Le delta survivant contient seulement les clés complètes distinctes, carriers, témoins et jointures par `arm_seed_index`, jamais les nœuds, arêtes, projections ou indices de terminaux de 10.5c; une mutation de l'indice dense d'une clé ou du compteur de doublons invalide son prédicat autonome. Le test n'engage ni quotient, ni transaction locator scientifique, ni réduction de hiérarchie. Aucun benchmark, sanitizer, GPU, oracle combinatoire, test massif ou GCP n'est requis pour 14D.

Le lot court `14E-RCPU` compare le résultat complet de l'API générale 10.5c et de la vue canonique distincte sur une fermeture possédant au moins une arête stricte. Il exerce les deux ordres LBVH et les collisions de fingerprints déjà disponibles, puis rejette avant fermeture une vue inversée, dupliquée, de cardinal mixte ou contenant une clé invalide. Dans l'exécuteur, la fixture tridimensionnelle E5 omet volontairement `AC` du locator, lie `DE` et exige une arête stricte non nulle de `AC` vers le carrier de `DE`, suivie de la destruction du graphe avant publication.

Le même lot teste séparément la primitive top-$K$ avec aucun incumbent, les vrais voisins et des voisins adversariaux. Les trois partitions scientifiques doivent être égales. Toutes les distances incumbentes sont comptées dans le cap exact; un cap juste inférieur échoue avant toute évaluation partielle. Trop de graines, un doublon, un identifiant hors domaine ou exclu sont rejetés. Une coquille de six points exactement équidistants vérifie qu'une borne égale n'est jamais élaguée et que tous les co-minimiseurs survivent. Aucun transcript de lot, kernel GPU, benchmark, test massif ou GCP n'est requis pour 14E.

Le lot court `14F-RCPU` valide d'abord le transcript isolé : vide, sparse non vide, zéro à $K$ candidats, ordre canonique libre des candidats, cinq caps exacts puis insuffisants, mauvaise forme de stamp, clé non canonique, cardinalités mixtes, ordre décroissant, doublon de clé ou de candidat et tails non nuls. Un identifiant hors domaine reste une proposition structurellement licite à ce premier étage, mais doit être rejeté par l'enveloppe de consommation avant toute fermeture.

La primitive spatiale compare ensuite $F$ seul, $F$ avec une bonne proposition et $F$ avec une proposition adversariale à l'oracle top-$K$ exact. Le recouvrement $F/P$ est évalué une seule fois; un cap de distances égal à $\lvert F\cup P\rvert-1$ échoue avant toute évaluation; les doublons internes et les tailles incorrectes sont rejetés. Six points équidistants répartis entre $F$ et $P$ doivent tous rester dans la coquille alors que la heap ne dépasse jamais $K$. Un épuisement après élagage doit conserver les comptes de sous-arbres et de points élagués dans l'audit même si aucune partition scientifique n'est publiée.

Enfin, la fermeture 10.5c consommatrice compare sa forêt scientifique à la voie canonique sans transcript sur une chaîne stricte et sur `AC` vers `DE`. Les propositions bonne, vide et adversariale doivent conserver clés, arêtes, témoins et carrier; une racine couverte et un successeur dynamique doivent apparaître dans les classes d'audit distinctes. Si ce successeur est aussi une seed dotée d'un record, son record ne doit pas être consommé après qu'il a été atteint dynamiquement. Un lot, niveau ou stamp périmé, une clé de record hors du lot, un candidat hors domaine ou un transcript muté rejettent avant `ClosureBuilder` avec `scientific_closure=nullopt`. Aucun executor 14D, kernel GPU, benchmark, test massif ou GCP n'est requis pour ce premier raccord 14F.

Le lot court `14G-RCPU` réutilise l'exécuteur 14D et les fixtures existantes, sans nouvelle géométrie. Sur le lot vide du tétraèdre, un transcript vide valide doit être effectivement revalidé, produire un audit de fermeture vide, puis laisser le delta scientifique strictement égal au raccourci historique dont le compteur de fermeture vaut zéro. Sur le lot non vide, un cap de clés résolues juste insuffisant doit rejeter avant toute consommation du transcript, sans audit propositionnel, delta ou avancement.

Sur la fixture tridimensionnelle E5, le lot contenant `AC` est préparé plusieurs fois au même curseur avec un transcript vide, la proposition utile `DE` et un candidat adversarial. Les trois deltas `ExactDirectSparseFacetDescentBatchExecutionResult` doivent être égaux champ par champ à la préparation historique, tandis que les audits distinguent respectivement le fallback, le pool de quatre points et le pool de trois points. Un transcript portant un mauvais indice de lot doit construire zéro fermeture et publier zéro delta. Après destruction de tous les transcripts et enveloppes intermédiaires, le seul delta utile est transmis à `commit_prepared`; sous les caps généreux de la fixture, le rejeu historique non amorcé doit l'accepter et avancer exactement une fois. Le test vérifie aussi que la session ne retient aucun record, audit ou graphe entre les appels. Il ne certifie pas qu'une préparation amorcée implique la vivacité d'un rejeu non amorcé sous tout budget. Aucun benchmark, sanitizer, GPU, test massif, oracle combinatoire ou GCP n'est requis pour 14G.

Le lot court `14H-RCPU` conserve le même exécuteur et ajoute deux contrôles concentrés. Sur le tétraèdre, les traits de type imposent un ticket non constructible et non copiable dont le déplacement est sans exception. Un transcript rejeté ne scelle aucune capacité. Une mutation du stamp, une session étrangère, un ticket déplacé, réutilisé ou frère devenu périmé doivent tous être consommés sans avancement. Un commit scellé accepté avance exactement une fois; un commit historique accepté change aussi l'epoch et périme les tickets spéculatifs antérieurs.

La fixture discriminante prolonge E5 par quatre points de coquille et choisit une descente stricte exacte dont le successeur est lié positivement. Sous caps généreux, les préparations vide et utile produisent le même delta. Le test sélectionne ensuite une dimension de travail LBVH strictement réduite par l'incumbent exact et fixe son cap à la consommation utile observée. La préparation utile doit encore produire un ticket et le même delta sous ce cap, tandis que la préparation non amorcée et le commit historique doivent s'épuiser sans avancer. Après destruction du transcript et extraction puis modification de l'audit séparé, le commit scellé doit avancer, restituer le delta inchangé et laisser constants les compteurs de rejeu frais, de fermeture propositionnelle et de graphe transitoire.

Ce test prouve seulement la fermeture de la dette de vivacité en mémoire dans la portée 14H. Il ne qualifie ni CUDA, ni crash-recovery, ni plafond de tickets détenus par l'appelant, ni quotient, locator ou commit de hiérarchie, ni latence 50 k ou capacité 10 M+; aucun oracle combinatoire, sanitizer, test massif ou benchmark long n'est requis.

Le lot court `14I-GPU-PROP` doit valider le producteur `cuda_g4 / hgp_reduced / proposal_only` sans campagne longue. Le contrôle hôte vérifie la capacité paresseuse du snapshot device à exactement $32n$ octets, le staging propre au producteur à $32n+n\cdot\mathrm{sizeof}(\mathrm{size\_t})$ octets, les buffers device à $208C+144C=352C$ octets, la copie hôte de records à $144C$ octets, la correspondance des positions copiées dans les requêtes et le rejet avant launch d'un rayon nul, d'une capacité dépassée, de clés non canoniques ou de cardinalités mélangées. Il couvre aussi les lots vide, entièrement hors plage binary64 et mixte, les epochs strictement consécutifs, la queue sentinelle, les corruptions de permutation, clé, candidat et partition des compteurs, et borne les inspections par $2kW$ avec au plus $k$ candidats distincts hors facette source. Le transcript interne conserve volontairement `candidate_exclusions_validated=false` : les exclusions du point d'usage appartiennent toujours à 14F.

Le replay matériel requis compile le vrai code AOT `sm_120` et exécute sur G4 deux fixtures déterministes courtes. La première, à $k=2$, vérifie les identifiants choisis par la sélection flottante indépendante, un centre exact non dyadique $1/3$, deux epochs et l'égalité de la partition CPU exacte avec et sans amorce. La seconde est réellement tridimensionnelle, atteint effectivement $k=10$, couvre toute sa petite permutation Morton, exige dix identifiants attendus et rend chacun des trois axes discriminant pour cette sélection avant de rejouer la même égalité exacte. Ces fixtures distinguent le centre exact d'entrée de son projeté opérationnel sans attribuer au kernel les jusqu'à $\sum_{j=1}^{4}\binom{k}{j}\leq385$ examens de supports nécessaires à une construction amont; 10.5c peut payer de nouveau ce coût.

La non-autorité scientifique est ensuite une composition des contrats déjà testés séparément : 14F revalide domaine et exclusions, recalcule les distances au centre exact, conserve $F$ comme baseline et descend les égalités LBVH; 14H ignore transcript, digest et audit après scellement. Le lot 14I ne répète donc pas toute la matrice adversariale 14F--14H et ne revendique pas un raccord exécuteur industriel tant qu'un worker GCP reproductible propre à 14I n'existe pas. Le rejeu ciblé hôte et le smoke G4 réel passent au SHA optimisé `3aeb62019252c785d94cfb91de331bb74b6572e2`; le digest et toutes les sorties restent identiques tandis que la pile ptxas passe de 672 à 160 octets par thread. Même ce succès ne certifie ni rappel des fenêtres Morton, ni accélération, ni SLO 50 k, ni capacité 10 M+. Le coût actuel d'environ 189 comparaisons rationnelles par projection de centre et le trafic de sortie en $C$ devront être réduits ou profilés avant le SLO; aucun benchmark long, oracle combinatoire ou test massif n'est demandé ici.

Le lot court `14J-ACTIVE-IO` remplace uniquement la matrice de trafic 14I. Avec $C=6$ et $D=2$, puis avec un lot mixte où $D=1$ et un lot vide où $D=0$, il exige des capacités persistantes inchangées de $208C+144C$ octets et des compteurs actifs exactement égaux à $208D$ octets H2D, $144D$ octets initialisés et $144D$ octets D2H. Le vecteur hôte contient exactement $D$ records, jamais $C$.

Le contrôle valide la sentinelle de la queue de candidats de chaque record actif et injecte une valeur résiduelle au premier indice inutilisé pour imposer un rejet. Il falsifie aussi un extent D2H rapporté par le launcher afin de fermer le couplage entre les tailles réellement passées aux opérations CUDA et l'audit hôte. Il ne lit pas la queue device $[D,C)$, qui n'a aucune autorité et sera réinitialisée si elle devient active. Un CTest hôte ciblé et un unique smoke G4 AOT avec memcheck suffisent; ce smoke réutilise un même buffer de capacité $C=6$ pour la transition $D=4$, puis $D=1$, puis $D=5$. Aucun benchmark long ni test massif n'est demandé. La sortie scientifique et le digest actifs doivent rester ceux de 14I, sans revendication de SLO 50 k ou de capacité 10 M+.

Le lot court `14K-DIRECT-PROJ` compare le projecteur entier au projecteur par encadrement 14J conservé seulement dans le test. Il exige les mêmes décisions de plage et les mêmes bits sur zéro, $1/3$, le minimum subnormal et son midpoint avec zéro, la frontière normal--subnormal, des midpoints de mots adjacents autour de 1, des changements de binade, le maximum fini et une valeur juste hors plage, avec les deux signes. Soixante-quatre rationnels déterministes supplémentaires suffisent à distinguer les formules sans devenir une campagne longue.

L'audit doit compter exactement trois axes par requête et au plus trois divisions entières. Le source produit ne doit plus inclure l'encadrement rationnel ni appeler `ExactCenter3::coordinate`; il lit directement les trois numérateurs et le dénominateur commun. La qualification G4 courte rejoue $1/3$, $K=10$, la transition $D=4$, $D=1$, $D=5$, le digest et memcheck. Elle ne mesure pas un débit et ne qualifie ni 50 k, ni 10 M+.

Au SHA `5e7e8449d7f4de2875ad0d9db8674d7664a30e4d`, cette qualification passe 2/2 en 0,29 seconde sur G4 `SPOT`; six axes sont fermés comme une division, cinq zéros et zéro hors plage, avec digest inchangé, code AOT `sm_120` sans PTX et memcheck nul.

Les tailles de support trois et quatre obéissent au même contrat : une proposition GPU ambiguë descend, et tout prune exact est rejoué par déterminants et comptage global indépendants. La complétude est suivie séparément pour chaque taille. Un run ne peut annoncer un catalogue complet tant que l'une des trois frontières reste non vide.

Le lot court `9.2a-RCPU` vérifie séparément la primitive de produit et le flux. Pour la primitive : triangle aigu, obtus et collinéaire; tétraèdre régulier et centre extérieur; requête strictement intérieure et égalité shell; puis la famille $p_0(t)=(t,2,0)$, $p_1=(-1,0,0)$, $p_2=(1,0,0)$. Sur $t\in[-2,2]$, les deux extrémités non aiguës ne doivent pas masquer le triangle aigu intérieur. Sur $t\in[-1/2,1/2]$ et $x=(0,33/16,0)$, les puissances négatives aux extrémités ne doivent pas masquer la puissance positive au centre. L'oracle Python rationnel recalcule ces valeurs indépendamment.

Pour le flux, des nuages de quelques points seulement suffisent : le total exact des univers doit valoir $\binom{n}{3}+\binom{n}{4}$ en `BigInt`; chaque expansion doit conserver la somme des cardinalités de ses enfants; le tétraèdre régulier et une fixture avec extra-shell exercent événements et diagnostic sparse. Le lot 9.2a exerce le cap de travail à zéro et la capacité initiale juste sous les deux racines. Une capacité incapable de contenir les racines initiales attendues échoue comme configuration invalide; les insuffisances ultérieures retournent le statut budgétaire et le parent intact. Le vérificateur frais recalcule la partition, chaque intervalle et chaque boule depuis le nuage, le LBVH, $K_{\max}$ et le budget; les mutations du groupe, de la multiplicité, du cardinal `BigInt`, d'une borne, d'un reçu, d'un compte de shell ou du statut terminal doivent échouer. Une exécution `budget_exhausted` doit conserver exactement sa couverture résiduelle et ne possède aucune assertion d'absence.

Le lot court 9.2b découpe le tétraèdre régulier en chunks d'une unité de travail. Chaque candidat est préparé depuis le checkpoint exact possédé par une session ancrée, rejoué avant commit et doit atteindre le même audit terminal que l'exécution monolithique sans double charge. Un second petit nuage doit interrompre une recherche de rang avec pile DFS non vide, puis recertifier ses reçus après reprise. Les tests vérifient le cache unique du manifeste, l'ordre commun des trois types de records, la chaîne terminale indépendante du découpage, le singleton terminal et le rejet de tout chunk no-op après terminaison.

Les mutations permanentes couvrent une plage Morton, le compte de partition `BigInt`, la chaîne de sortie au record zéro, le checkpoint successeur, un reçu actif et un token source périmé, chaque fois avec checksum recalculé. Une fixture d'autorité remplace les deux racines initiales de couvertures quatre et un par cinq copies de la racine tétraédrique de couverture un : l'intégrité locale peut rester vraie, mais la session ancrée doit refuser cette source et le rejeu depuis les racines doit échouer. Ce cas interdit de promouvoir une somme de couvertures en preuve de provenance. Les tailles de reçus sont rejetées avant recomputation au-delà de neuf pour les stricts ou de la profondeur LBVH plus un pour la pile. Les autres caps atomiques de sortie et de classification restent à exercer avec le futur codec supérieur, sans campagne exhaustive.

Un contrôle statique du graphe de dépendances et des allocations du target produit interdit :

- les symboles Gamma exhaustifs et leurs overlays;
- une capacité dérivée de $\binom{n}{k}$ ou $\binom{n}{k+1}$;
- une arène de tous les labels ou cellules top-$m$;
- une sortie tronquée sans frontière de reprise et statut budgétaire.

Le lot `9.1-RCPU` couvre déjà la borne exacte, l'égalité, la partition sans prune, la paire longue, un prune strict, le shell supplémentaire sparse, les sept familles de budgets, l'accord exhaustif ciblé à $n=14$ pour $K_{\max}\in\left\lbrace1,4,9,10\right\rbrace$ et le rejeu hostile du résultat. `9.3a-RCPU` ajoute le checkpoint réinjectable lié aux autorités, le produit actif, le curseur témoin, des reçus stricts partiels réellement conservés dans le checkpoint en mémoire, l'expansion différée, l'ordre inter-types et la chaîne de digests indépendante du découpage, le rejeu idempotent d'un chunk non publié et la reprise des sept motifs d'arrêt vers le même audit que le run résident. Les fixtures rehashent volontairement les falsifications afin d'exercer après le checksum les plages, doublons, relations ancêtre–descendant, recouvrements reçu–support et reçu–frontière active, cardinalités, états, nœud différé feuille et liens du curseur à l'audit. Le run ancré rejette préfixe incomplet, transition supprimée, réordonnée ou dupliquée, digest source et ordre de records mutés, ainsi qu'un terminal auto-cohérent sans filiation; des vecteurs hexadécimaux dorés figent manifeste, checkpoint initial, checkpoint mi-curseur et chaîne de sortie v1.

`9.3b-RCPU` ajoute un vérificateur réellement incrémental qui ne conserve que le checkpoint courant. Pour le contexte principal, le test exige exactement un calcul instrumenté du manifeste malgré plusieurs dizaines de chunks unitaires, avec un hash exactement une fois de chaque point, feuille et nœud, l'égalité des records, de l'ordre typé, de l'audit et du digest terminal avec le run résident, puis zéro chunk retenu par `verify_next`. Chaque contexte distinct utilisé par les tests de durée de vie et de singleton possède son propre calcul unique. Une mutation du digest source empoisonne le vérificateur sans avancer son checkpoint; le chunk successeur est ensuite explicitement rejeté, comme toute exception de rejeu, et les constructions depuis des autorités temporaires sont interdites à la compilation. Un singleton constitue un run vide et rejette un chunk terminal redondant. Pour chaque checkpoint valide, les compteurs exigent au plus deux rectangles et quatre événements de sweep par produit, au plus deux tests de voisinage par rectangle, exactement un intervalle par reçu ou entrée active, exactement une comparaison par paire d'intervalles adjacents après tri et exactement une recertification géométrique par reçu. Les mutations 9.3a continuent d'exercer les recouvrements et relations ancêtre–descendant contre ce nouveau vérificateur quasi linéaire. Cette section reste ouverte : les reçus terminaux détaillés de tous les prunes et leurs mutations de signe restent à ajouter avec CUDA P1; le codec hostile, les limites de décodage et la publication atomique sur stockage durable restent une porte 9.3; l'exhaustif pour chaque $n\leq14$ et plusieurs permutations reste aussi ouvert.

`9.3c-RCPU` ajoute trois tests courts. Le premier fige l'encodage binaire v1, vérifie le round-trip exact de chunks initiaux, partiels et mixtes, puis falsifie magic, version, kind, flags, checksum, longueur, troncatures, octets terminaux, booléens, enums, comptes, textes rationnels et quotas agrégés; les comptes impossibles sont rejetés par faisabilité minimale avant `reserve` ou construction de `BigInt`. Le deuxième publie des chunks bornés jusqu'au terminal, ferme la reprise avec mêmes digests, refuse un second verrou, les symlinks, FIFO bloquant, trou de séquence, final tronqué ou corrompu et conserve les quatre compteurs d'architecture à zéro. Le troisième lance un vrai sous-processus pour chacun des quatre points `temporary_file_written`, `temporary_file_synchronized`, `transition_renamed` et `directory_synchronized`, appelle `_Exit`, puis rouvre : les deux premiers retrouvent l'ancien checkpoint et nettoient le temporaire, les deux derniers rejouent exactement un nouveau final; le retry pré-renommage est byte-identique et une seconde reprise ne double pas la transition. `prepare_next` est aussi testé séparément : abandon sans avance, commit unique, puis rejet fail-closed d'un jeton périmé. Cette matrice certifie seulement la perte de processus sur le filesystem Unix local testé avec `flock`; aucune campagne power-cut, rollback de préfixe, VM, Hyperdisk, Windows ou filesystem réseau n'est simulée.

`9.3d-RCPU` garde une matrice courte centrée sur les nouveaux invariants. Le wire chunk v1 doit conserver son golden avec un cap exactement égal à sa taille, rejeter le cap inférieur d'un octet et éviter tout second vecteur payload. `open_existing` sur un répertoire sans `HEAD` et `create_new` sur un `HEAD` existant échouent; un temporaire `HEAD_0` complet laissé avant le premier lien est validé, nettoyé et recréé, et un budget différent viole le contrat du run. Une ancre externe exacte est recertifiée, un `HEAD` local plus récent est accepté après passage par son checkpoint, tandis qu'une ancre future ou de digest faux est rejetée. La suppression d'un chunk committé relativement au `HEAD`, un orphelin invalide et une cible concurrente au nom final échouent sans suppression hostile ni avancement; la cible concurrente reste intacte grâce à la publication no-replace. Quatre pertes de processus représentatives — temporaire chunk synchronisé, lien final créé, temporaire `HEAD` synchronisé et `HEAD` remplacé — suffisent pour distinguer ancien préfixe, orphelin validé puis nettoyé et nouveau préfixe. La reprise terminale multi-chunk conserve un seul chunk et les compteurs d'architecture nuls. Aucun benchmark long n'appartient à ce jalon protocolaire.

`9.3e-RCPU` ajoute seulement des tests fd courts. Un fichier encodé doit être octet pour octet égal au golden vectoriel v1, conserver la position du descripteur et réussir aux caps exacts; le cap inférieur d'un octet et, pour l'encodeur seulement, un fd non régulier, non seekable, non vide, `O_RDONLY`, `O_APPEND` ou `O_DIRECT` doivent échouer sans image acceptée. Une fixture dont un champ traverse la frontière de 64 Kio vérifie la boucle positionnelle. Le vérificateur wire falsifie longueur, troncation, checksum et reçu. Le décodeur fd doit accepter `O_RDONLY` et rendre exactement la décision et le chunk du décodeur span sur les cas acceptés et hostiles, après une passe checksum préalable et une seconde authentification. Le sink durable doit publier et reprendre les mêmes digests avec `materialized_transition_wire_byte_count=0`; une corruption avant relecture, la substitution de l'inode du temporaire chunk, celle de `.HEAD.tmp` et un temporaire partiel à la reprise vérifient l'échec fermé ou le nettoyage strictement non committé. Le test de la borne binary64 construit les extrêmes du milieu et du rayon carré et exige des textes sous les bornes démontrées 958 et 1914, donc sous la limite par défaut 2048. Aucun benchmark de débit n'est requis.

### 4.4 Énumération combinatoire des K-polyèdres

Pour chaque $k$, l'oracle énumère tous les labels $Q\in\binom{X}{k}$ et tous les co-labels $S\in\binom{X}{k+1}$. Il calcule exactement le rayon carré $\beta(Q)$ de la plus petite boule englobante de $Q$ et $\beta(S)$ pour les cofaces.

Soit $\Theta_k$ l'ensemble trié des valeurs distinctes de ces rayons, complété par des sentinelles strictement avant et après chaque niveau. À chaque $t\in\Theta_k$, l'oracle construit le graphe fini suivant :

- un sommet pour chaque $Q$ tel que $\beta(Q)\leq t$ ;
- une adjacency entre les facettes de $S$ lorsque $\beta(S)\leq t$ ;
- une réduction en composantes connexes sans supposer que les identifiants du backend coïncident.

Cette construction sert de référence directe aux K-polyèdres. Une variante séparée construit le sous-graphe K-Gabriel afin de vérifier sa garantie de connectivité positive, de mesurer les connexions manquantes et de falsifier toute promotion abusive. Elle doit reproduire le désaccord permanent `gabriel-point-set-counterexample-5-points-v1`; elle ne vérifie plus le théorème de réduction comme base exacte.

### 4.5 Comparaison à tous les seuils

Comparer seulement la liste des fusions est insuffisant en présence de niveaux égaux. Pour chaque valeur critique $a$, on compare les états :

1. à un niveau strictement inférieur à $a$ ;
2. au niveau fermé $a$ ;
3. à un niveau strictement supérieur à $a$ et inférieur au niveau distinct suivant.

La comparaison porte sur les partitions de labels actifs, les unions de points couvertes, les parents de la forêt et les flèches verticales. Deux sorties sont équivalentes si ces objets coïncident après canonicalisation, même si elles choisissent des représentants DSU ou des étoiles d'hyperarête différents.

### 4.6 Budget combinatoire et surveillance de l'oracle

L'oracle journalise le nombre de sous-ensembles, le temps et le pic mémoire. Les limites $n=12$ et $n=14$ sont des valeurs par défaut, non des hypothèses mathématiques :

- PR : toutes les fixtures déterministes et au moins 100 graines aléatoires par classe pour $n\leq12$ ;
- nocturne : au moins 1 000 nuages aléatoires répartis entre $4\leq n\leq14$ ;
- release : reprise de toutes les graines ayant déjà révélé un défaut, sans limite d'ancienneté ;
- minimisation automatique d'un contre-exemple par suppression de points et réduction des coordonnées.

Toute graine fautive devient une fixture permanente avant correction du code.

## 5. Tests unitaires des prédicats

### 5.1 Matrice des prédicats

| prédicat | cas positifs et négatifs obligatoires | sortie exigée |
|---|---|---|
| orientation affine en 3D | points génériques, coplanaires, quasi coplanaires | signe exact ou zéro exact |
| côté d'un plan de bissection | deux labels, grands offsets, différence d'un ULP | signe certifié |
| centre de support | $\lvert U\rvert=1,2,3,4$ | centre, dimension, témoin |
| relative-intériorité | support aigu, droit, obtus, barycentrique presque nul | tous les signes barycentriques |
| intérieur/frontière/extérieur | écarts larges puis quasi nuls | classe exacte |
| rang fermé | shells de tailles $1$ à $8$, multiplicités | $(\lvert I\rvert,\lvert U\rvert,s)$ |
| miniball | support effectif de taille $1$ à $4$, points redondants | rayon, centre, support minimal |
| égalité de niveaux | valeurs rationnelles égales, voisines et mal conditionnées | lot exact ou ordre exact |
| niveau rationnel homogène | supports 3/4, dénominateurs de signes initiaux variés, facteurs communs | `ExactLevel` canonique avec dénominateur positif |
| top-$k$ avec exclusions | $1\leq k\leq10$, jusqu'à 9 identifiants exclus | voisins, shell final, certificat |
| clé canonique | permutations, duplications d'émission, plateaux | sérialisation identique |

Pour la revue G1 qui clôt la Phase 2A, « tout T1 » désigne toutes les décisions numériques signées et toutes les constructions exactes effectivement livrées par le laboratoire de prédicats CPU : prédicats, centres, rangs affines locaux, classifications de supports et de sphères, niveaux, égalités et canonisation. Les requêtes spatiales globales de la matrice — notamment top-$k$ avec exclusions, shell global et miniball d'un nuage complet — restent soumises à leurs phases d'implémentation et aux portes G2 à G4; elles ne peuvent ni bloquer rétroactivement la référence arithmétique de 2A, ni être réputées validées par sa fermeture. Cette séparation n'autorise aucun faux signe : toute primitive numérique appelée ultérieurement reste couverte par G1 et doit conserver zéro décision erronée.

Dans le découpage incrémental de la Phase 2A, le lot 2A.5 couvre les centres circonscrits non triviaux de deux à quatre points définis par la feuille de route. Le lot 2A.6 complète la matrice avec le singleton, dont le centre est le point lui-même, la dimension affine zéro et le niveau carré nul; il certifie aussi les signes barycentriques, la réduction exacte des supports de frontière et le côté exact d'une sphère. Le lot 2A.7 ajoute le produit croisé instrumenté des `ExactLevel`, le tie-break par cardinalité puis identifiants canoniques, l'agrégation des provenances réduites et les lots de niveaux exactement égaux. Le centre reste un témoin vérifié, jamais un critère supplémentaire masquant l'association contradictoire d'un même support à deux niveaux. Cette chaîne locale ne remplace pas l'énumération de tous les sous-supports exigée par l'oracle miniball.

### 5.2 Cascade de précision

Chaque décision suit le contrat : proposition rapide, filtre borné, expansion exacte, puis multiprécision CPU rare. Les tests instrumentent séparément les étages `fp32`, `fp64_filtered`, `expansion` et `cpu_mp`.

Les obligations sont :

- aucun signe accepté par un filtre ne diffère de l'oracle exact ;
- un filtre indécis transmet le cas à l'étage suivant ;
- la désactivation de `fp32` ou de `fp64_filtered` ne change jamais la sortie canonique ;
- le mode `certified` ne tolère aucun résultat `unknown` et ne publie `exact` qu'après décision de tous les signes ;
- le taux de repli est mesuré, mais aucun seuil de taux ne justifie une approximation ;
- les calculs compilés avec et sans contraction FMA, et avec les optimisations rapides désactivées, donnent la même sortie exacte.

Un corpus spécialisé est généré en déplaçant un point de part et d'autre du zéro d'un déterminant par pas de $1$, $2$, $4$ et $8$ ULP, dans les deux directions.

Les centres et rayons de supports trois ou quatre sont testés comme rationnels, pas comme dyadiques supposés. Pour $a/b$ et $c/d$ avec dénominateurs positifs, le verdict est celui du signe exact de $ad-bc$. La suite construit des niveaux égaux avec représentations non réduites différentes, des niveaux stricts arbitrairement proches, des chaînes de tri mêlant plusieurs tailles de support et des cas qui forcent le fallback bigint. Tri, lot égal, hash, sérialisation, checkpoint et reprise doivent rester identiques.

Pour les formes et plans de 2A.8c, le corpus distingue quatre origines fermées : coefficients binary64, plan par trois points binary64, bisecteur de puissance binary64 et plan rationnel exact sans provenance. Il force séparément orientation 2D, côté d'un plan, rang normal et augmenté, intersection unique ou affine, incohérence et incidence d'un quatrième plan. Chaque famille doit atteindre les signes disponibles aux intervalles, à l'expansion et au fallback multiprécision; ce dernier contient aussi des recettes binary64 valides dont l'expansion échoue fermée, et pas seulement des plans exacts dépourvus de provenance. Les rangs, dimensions, étages et compteurs JSON sont vérifiés avec des types entiers stricts afin qu'un booléen ne soit jamais accepté comme `0` ou `1`. Permutations des plans liants, inversion d'orientation, recettes obliques et bisecteurs multi-points complètent les cas d'axes. Le mode adaptatif et `multiprecision_only` doivent publier le même témoin scientifique, tandis que l'étage diagnostique peut différer.

### 5.3 Témoins de sommets

Si l'algorithme manipule des cellules polyédriques, chaque sommet décisif est accompagné d'un témoin combinatoire : contraintes liantes, approximation, classe de dégénérescence et intervalle d'erreur. Les tests reconstruisent le signe de toute contrainte supplémentaire à partir de ce témoin, et vérifient qu'un sommet géométriquement imprécis ne puisse jamais produire un certificat de fermeture par simple tolérance.

### 5.4 Campagne de fermeture GPU de phase 2B

La porte 2B est testée par deux campagnes résumables distinctes. La première rejoue exactement la graine et les racines scientifiques gelées par la phase 2A. La seconde utilise la graine indépendante `4257325f47505532` et doit produire une racine de corpus différente. Chaque campagne de production contient 10 800 000 cas de base — 3 600 000 pour chacun de `compare_squared_distances`, `orientation_3d` et `power_bisector_side` — plus 1 080 000 signes métamorphiques. L'agrégat attendu contient donc 23 760 000 signes lorsque les deux certificats sont complets. Une graine indépendante et deux racines distinctes interdisent la substitution d'un seul corpus aux deux campagnes; elles ne constituent pas une preuve de disjonction cas par cas et le certificat ne doit pas la revendiquer.

Pour chaque entrée, le signe attendu de l'oracle entier-dyadique indépendant doit coïncider avec le replay CPU forcé en multiprécision. Toute transformation métamorphique doit en plus satisfaire sa relation de signe déclarée. Le chemin GPU est ensuite comparé à ces trois contrôles : un signe connu doit être un certificat FP64 strict de même signe; un `unknown` doit être compté, transmis au CPU et résolu sans `remaining_unknown`. Les compteurs par prédicat, signe, relation, étage GPU et fallback doivent fermer exactement sur le nombre de lignes.

Un chunk contient des lots homogènes par prédicat et au plus 196 608 cas de base. Le checkpoint conserve les hashes des exécutables, du générateur, de la configuration et du commit propre, la chaîne contiguë des chunks et quatre racines ordonnées pour corpus, oracle, sortie CPU et sortie GPU. La suite négative couvre au minimum : reprise après un seul chunk, deuxième écrivain, chunk altéré ou orphelin, racine forgée, état complet forgé, compteur incohérent, sortie JSON non canonique, changement du dépôt ou d'un exécutable, relation métamorphique fausse, signe GPU contradictoire et témoin de puissance non représentable. Toute anomalie échoue fermé et laisse le dernier checkpoint committé inchangé.

Le certificat final persiste les répartitions par transformation et par relation métamorphique. Il impose en outre l'égalité entre zéros exacts et signes nuls, la fermeture du tri-state GPU, puis le replay multiprécision de chaque ligne. Un mode de vérification en lecture seule relit une copie quiescente de tous les checkpoints, chunks, certificats et exécutables sans créer ni modifier de fichier; toute altération de l'agrégat racine doit être détectée, y compris en mode mono-campagne. Ce dernier porte `single_campaign_only` et ne peut jamais usurper la justification `independent_seed_and_distinct_corpus_root` réservée aux deux certificats compatibles.

Un certificat incomplet conserve `phase_gate_closed=false` et `qualification_claimed=false`. Même deux certificats complets ne ferment la phase qu'après vérification de leurs références, de leurs racines et de l'agrégat, puis application séparée des autres obligations matérielles et opérationnelles de la porte.

### 5.5 Contexte GPU résident

Le contexte résident est testé sur des séquences grand-petit-grand pour les trois prédicats, puis sur leur alternance dans un même handle. Les décisions, identifiants, compteurs, audits et fallbacks doivent être identiques au chemin éphémère. Deux futures partageant un contexte sérialisent seulement leur section GPU; deux contextes distincts restent indépendants. La destruction ou le déplacement du handle après planification ne doit pas invalider la future, tandis qu'un handle déplacé refuse tout nouveau travail.

Un lot vide ne crée ni stream ni allocation. Une capacité déjà suffisante ne provoque aucune nouvelle allocation et chaque sortie utile est entièrement réécrite afin de détecter un résultat résiduel. Une faute injectée empoisonne seulement le contexte concerné et toutes ses utilisations suivantes échouent fermé. La qualification réelle exécute ces séquences sous `compute-sanitizer` avec contrôle des fuites; le benchmark publie séparément le processus froid et `warm_context_e2e`, qui inclut encore validation, packing et transferts et ne doit pas être présenté comme un débit de noyau pur.

La mesure finale `warm_context_e2e` de Phase 2B utilise un même `PredicateFilterContext`, 65 536 cas déterministes par prédicat, un échauffement non mesuré puis 31 répétitions mesurées. La génération des cas précède l'échauffement; le chronomètre hôte entoure l'appel asynchrone propriétaire jusqu'à `.get()` et inclut donc copie du lot, validation, packing, transferts, noyau, fallback CPU exact et synchronisation. Dans chaque lot, les indices congrus à deux modulo trois sont des zéros exacts et imposent le chemin `unknown` GPU vers CPU; les deux autres classes sont bien conditionnées et doivent être certifiées sur le GPU réel. Le JSON conserve dans l'ordre les 31 durées en nanosecondes, puis `min_ns`, `p50_ns`, `p95_ns`, `max_ns` et la déviation absolue médiane `mad_ns`, tous rejouables par rang le plus proche. L'artefact de production lie cette sortie à un commit propre, au hash du harness, à l'image CUDA, à NVCC et à l'unique G4 visible; il exige par prédicat 2 031 616 entrées, 1 354 421 signes GPU connus, 677 195 replis et zéros exacts, 31 lots de fallback et aucun inconnu restant. Le test hôte réduit à neuf cas et trois répétitions avec lanceur simulé valide le schéma mais ne peut jamais fermer la porte matérielle. Aucun seuil de latence ne conditionne la correction de Phase 2B : une mesure lente reste publiable et ferme cette obligation si ses comptes et sa provenance sont valides.

### 5.6 Primitive de puissance bornée du jalon 7.1

Le checker de source exige exactement les commits Paragram et cuBQL du manifeste, la licence attendue et les faits structuraux audités; il refuse un checkout sale, un gitlink différent ou une capacité silencieusement modifiée. Ce contrôle de provenance ne compile pas l'amont et ne certifie aucune cellule.

L'oracle hôte couvre $1\leq n\leq8$, une boîte dyadique explicite et des poids dyadiques signés. Pour chaque cellule, il vérifie le plafond de treize plans, les 286 triplets au plus, les 286 sommets rationnels au plus et les 3718 incidences au plus. Chaque sommet accepté est l'intersection exacte de trois plans de rang trois, satisfait toutes les inégalités $\Phi\leq0$ et porte l'ensemble complet de ses plans incidents; l'ordre d'arrivée des sites doit restituer le même résultat canonique complet.

La validation hôte courante couvre : couture avec le prédicat de bisecteur singleton non pondéré; deux sites sur un axe avec frontière analytique; poids propriétaire positif ou négatif; translation commune des poids; cube exact produit par six concurrents; cellule coupée par la boîte; permutation canonique; sites confondus de poids égal, inférieur ou supérieur; cellule vide par deux demi-espaces propres incompatibles; cellules fermées de dimensions deux, un et zéro; les cinq capacités juste insuffisantes; cap excessif; boîte plate; identifiants dupliqués et domaine trop grand. Les futurs différentiels ajouteront les sites presque confondus, les limites binary64 et les grands nombres de faces.

Le différentiel futur traite séparément Paragram avec fast math, Paragram avec `NO_FAST_MATH=1` et l'oracle exact. Toute sortie amont est une proposition : un statut non nul, un overflow ou une éviction de parcours interdit de l'accepter sans recertification; un statut nul ne la certifie pas. Le test ne compare une géométrie amont qu'après exposition des sommets, plans et incidences par le candidat fork ou la primitive interne. Les builds et CTests ciblés de l'oracle doivent passer sous GCC et Clang stricts; aucun benchmark long ni aucune session G4 ne sont requis à ce jalon.

### 5.7 Série fail-closed du jalon 7.2

Le checker de série exige un checkout 7.1 propre, le manifeste `complete_series_required`, des ordres contigus, des chemins normalisés sous le dépôt, la taille et le SHA-256 exacts de chaque patch, puis les arbres Git exacts avant et après chaque application. Il refuse les renommages, copies, binaires, changements de mode, liens symboliques et actions non déclarées. Il ne lance ni clone, ni fetch, ni initialisation de sous-module et réutilise uniquement les objets du checkout local dans un dépôt temporaire.

Après application, le contrôle structurel exige une ABI entière unique avec les codes historiques 0 à 5 inchangés et `traversal_stack_overflow=6`; un résultat de parcours typé; trois push contrôlés dans chacun des deux parcours de boîte; l'absence du remplacement silencieux et du helper de rayon abandonné; la priorité à l'erreur géométrique déjà acquise; l'absence d'adjacence brute pour tout statut non nul; et le filtrage des deux extrémités avant symétrisation. Le corps du gather ne peut avoir aucun retour anticipé, car tous les threads doivent traverser ses barrières.

La fixture CPU minimale emploie quatre lignes brutes dont les statuts sont succès, overflow de pile, succès et overflow de plans. Seule l'arête entre les deux cellules saines doit subsister, et les deux lignes fautives doivent être vides. Ce modèle valide le contrat de quarantaine, pas l'exécution CUDA. Une étape ultérieure compilera deux extensions identifiables, dont une avec capacité de pile égale à un pour forcer l'erreur sur les chemins ordinaires et pondérés. Elle devra aussi exécuter tout le pipeline sur le stream PyTorch courant et propager les erreurs de lancement et d'exécution avant le compactage CSR. Jusqu'à cette compilation et au différentiel géométrique, `cuda_compile_status=not_run` et toute sortie reste `proposal_only`.

### 5.8 Boîte explicite du jalon 7.3

La série complète doit appliquer le patch AABB après le patch fail-closed et reproduire l'arbre final déclaré. Le checker structurel exige `bounds` obligatoire et keyword-only dans les deux API Python, sa transmission aux deux bindings et sa revalidation native avant contiguïté, allocation ou lancement GPU. Les points, poids et guesses directs sont validés avant copie, leurs devices doivent coïncider et le `CUDAGuard` doit précéder toute activité CUDA.

Les tests CPU de l'API couvrent une vue non contiguë dont les six mots binary32 restent identiques, les extrêmes finis au seul niveau de l'entrée, les mauvais conteneur, dtype, device et forme, NaN, les deux infinis, égalité et inversion sur chaque axe, les zéros signés et l'obligation keyword-only. Ils n'importent pas le binding CUDA. Le checker exige en outre les six affectations de `world_bounds`, l'absence de boîte issue de la racine BVH et de padding `1.0f`, l'absence de `CUBE_EPSILON` dans les deux initialiseurs, la suppression des rayons initiaux fixes et leur reconstruction après `CCInit`.

Ces tests ne qualifient ni les valeurs extrêmes sur GPU, ni l'arrondi extérieur des rayons de pruning, ni le stream PyTorch courant, ni les erreurs asynchrones, ni l'inclusion des sites dans la boîte. Une future exécution enregistre les six mots dans l'ordre `lx,ly,lz,ux,uy,uz`, l'arbre Paragram final et les SHA-256 ordonnés de la série. Aucun statut exact n'est promu par un test d'interface ou un accord différentiel.

### 5.9 Fermeture sémantique du jalon 7.4

Le test atomique reçoit la table complète $K$, une amorce $J$ et la boîte, puis vérifie que chaque identifiant de $J$ est unique et appartient à $K$. Il reconstruit exactement $H_i(J)$ avant de scanner toutes les contraintes omises sur tous ses sommets. Le résultat incomplet retourne en un seul lot trié chaque `violating_halfspace`, chaque `missing_active_incidence` et chaque `competitor_dominates`; une contrainte propre strictement négative sur tous les sommets n'est pas ajoutée. Le premier témoin reste déterministe par `PointId`, privilégie une violation sur une égalité pour ce même identifiant, puis suit l'ordre canonique des sommets.

Les fixtures obligatoires couvrent : un coupeur omis puis ajouté; une contrainte authentique strictement redondante; une contrainte égale à une face de boîte; un lot simultané contenant un violateur et une incidence active; une cellule de dimension basse; les trois classifications de sites coïncidents; une cellule candidate déjà vide; l'invariance aux permutations; un doublon et un identifiant inconnu; une boîte invalide non masquée par le budget; le plafond conservateur exact de 360 scans et les budgets cellule et scan juste insuffisants.

La propriété de réparation vérifie séparément qu'après ajout simultané de tous les identifiants retournés, aucune contrainte propre omise ne peut devenir active : le nouveau polytope est inclus dans l'ancien, où chacune était strictement négative. Une étape suivante matérialise ce rebuild unique et compare sa cellule finale à l'oracle complet. Aucun test 7.4 ne compile CUDA, ne qualifie Paragram ou ne change un statut public.

### 5.10 Matérialisation atomique du jalon 7.5

`repair_exact_bounded_power_cell_subset_closure` doit appeler le décideur 7.4 une seule fois, puis construire au plus une cellule supplémentaire. Une amorce déjà fermée non vide conserve une construction et aucun lot; une amorce vide par demi-espaces propres conserve une construction, aucun scan et aucun lot. Un cas contenant simultanément un violateur et une incidence active exige deux constructions, un scan, zéro post-scan et un lot dont les identifiants sont triés. Dans la fixture où ce lot complète exactement $K$, la cellule réparée est égale structurellement à l'oracle exact construit avec toute la table, sans utiliser cet oracle pour prendre la décision. Si une amorce fermée omet des contraintes strictement redondantes ou des ties, seule sa projection normalisée — décision, sommets exacts et incidences propres actives — peut être comparée; l'égalité brute des métadonnées est alors interdite.

Les constantes omises sont testées séparément : `owner_dominates` et `coincident_tie` restent dans l'audit initial sans rejoindre l'amorce fermée, tandis que `competitor_dominates` rejoint le lot et produit `repaired_complete_empty`. Un `competitor_dominates` déjà semé relève au contraire de `already_complete_empty`. Le wrapper couvre aussi la permutation de $K$; les dimensions basses, plans coïncidents, tangence, permutation de $J$, sur-amorce et orientation réciproque restent les fixtures du décideur 7.4 partagé et ne sont pas dupliquées artificiellement dans 7.5.

Les deux préflights échouent chacun juste sous leur besoin : budget 7.4 insuffisant, puis budget de reconstruction complète insuffisant. Dans les deux cas les compteurs de construction et de scan valent zéro, `initial_closure`, `repaired_cell` et la cellule finale sont absents. Les exigences atteignent exactement les caps conservatifs 506 triplets et sommets cumulés et 6358 incidences cumulées pour $f=7,c=6$. À l'inverse, $J=K$ accepte un budget de seconde construction nul. Les cinq décisions sérialisables et l'enum invalide complètent les tests propres à 7.5; les IDs et boîtes invalides restent verrouillés sur le préflight partagé par les tests 7.4. Aucun test 7.5 ne compile CUDA ou ne promeut un statut public.

### 5.11 Noyau H-polytope générique du jalon 7.6

La boîte seule doit produire exactement huit sommets, 20 triplets, 24 incidences et la dimension affine trois. Une coupe propre oblique ou axiale vérifie l'orientation $h\leq0$, puis deux identifiants distincts portant le même plan doivent rester deux frontières et deux incidences sémantiques sur chaque sommet actif. Des paires de demi-espaces opposés ferment successivement des polytopes de dimensions deux, un et zéro. Deux contraintes propres incompatibles et une constante positive donnent séparément une décision vide; les constantes négative et identiquement nulle restent classifiées sans plan.

La permutation de l'ordre d'insertion, y compris entre contraintes de rôles parent et clip, doit conserver le résultat canonique lorsque les mêmes tuples d'entrée sont fournis. Les tests rejettent un identifiant composite dupliqué, les enums domaine et rôle invalides, une boîte plate ou non canonique même sous budget insuffisant, plus tout budget supérieur aux caps. Chaque capacité — entrées, frontières, triplets, évaluations de faisabilité, sommets et incidences — passe à son exigence exacte et échoue juste en dessous sans payload géométrique.

Le préflight maximal construit 55 contraintes simples avec IDs uniques et vérifie les exigences $B=61$, 35990 triplets, 2195390 tests de faisabilité, 35990 sommets conservatifs et 2195390 incidences sans lancer cette géométrie. Les tests de non-régression 7.1–7.5 repassent ensuite par l'adaptateur puissance et doivent rester structurellement identiques. GCC et Clang stricts suffisent à ce jalon; aucun benchmark long, faux lanceur, NVCC, G4 ou statut public n'en découle.

### 5.12 Faux lanceur du transcript H-polytope 7.7

Deux cellules fournies dans l'ordre inverse de leurs identifiants et des contraintes fournies dans des ordres différents doivent produire le même CSR canonique et le même digest. Pour chaque ligne réussie à $B$ frontières, le faux lanceur réécrit exactement $\binom{B}{3}$ slots à l'epoch courant, et la position de chaque slot doit correspondre à son ordinal combinadique. Le cube, une coupe axiale, une coupe oblique, une cellule de dimension basse avec incidences multiples et une cellule vide sont tous comparés au résultat 7.6 construit indépendamment. La recette saine exerce les trois statuts de record : inconnu exact, rejet avec témoin recertifié et survivant dont les intervalles contiennent le sommet rationnel.

Le masque `could_be_active` est testé comme sur-ensemble. Un faux positif dans le domaine reste accepté; l'effacement d'une incidence exacte, un bit au-delà de $B$, un triplet absent ou dupliqué, un mauvais ordinal, une epoch répétée ou avancée deux fois, une sentinelle résiduelle dans le préfixe, une écriture dans la queue, un offset ou ID de cellule falsifié, un intervalle non fini et un faux témoin de rejet doivent tous échouer sans résultat puis empoisonner le contexte. Une faute asynchrone simulée suit le même chemin. Les fallbacks prudents `interval_unknown`, `capacity_exhausted` et `unsupported_projection` gardent au contraire la décision CPU exacte et une ligne GPU vide. Une constante positive exacte partage un batch avec une cellule saine : elle doit produire `complete_empty` par 7.6, un fallback d'intervalle vide et aucun empoisonnement; l'identifiant `UINT64_MAX` reste licite.

Les prévalidations couvrent offsets d'entrée, IDs de cellule dupliqués, IDs de contrainte dupliqués, domaine ou rôle invalide, boîte non canonique, segment de 56 contraintes, débordement de sommes et budget exact juste insuffisant. Elles précèdent le lancement et ne doivent pas empoisonner le contexte. La capacité n'est jamais testée par une géométrie maximale : la fixture de cap vérifie seulement les besoins $B=61$ et 35990 slots avant fallback. GCC et Clang stricts exécutent le target hôte; aucune TU `.cu`, compilation NVCC, session G4 ou promotion publique n'appartient à 7.7.

### 5.13 Source CUDA et qualification courte 7.8

Le checker statique relit l'API, l'ABI POD, le wrapper exact, le faux lanceur, la primitive d'intervalles 2B, la TU CUDA, CMake et les presets. Les tableaux des POD transférés puis indexés par le device doivent être des `std::uint64_t[N]`, jamais des `std::array<std::uint64_t, N>` dont les accesseurs peuvent rester hôte sous NVCC; une mutation négative doit figer chaque champ concerné. Les autres mutations doivent détecter la suppression d'un arrondi dirigé, du zéro intégral de capacité, du stream non défaut, de la garde CUDA 12.9, de l'écriture directe par ordinal ou du target dans un preset. Il refuse epsilon, tolérance, `nextafter`, fast math, FMA, racine, puissance, émission par `atomicAdd` et tout verdict exact ou public dans la TU.

L'exécutable réel utilise quatre cellules dans un ordre non canonique : cube, coupe axiale, constante positive vide et second cube hors capacité. La capacité 55 doit produire deux lignes réussies de 20 et 35 slots, puis deux lignes vides de statuts distincts; inconnu, rejet et survivant doivent tous apparaître. Deux appels sur le même contexte exigent les epochs 1 et 2 et des résultats bit à bit identiques après neutralisation de l'epoch. Chaque résultat exact est comparé au noyau 7.6.

La porte G4 reste courte : compilation release et audit AOT, exécution analytique, contrôle sans PTX, `compute-sanitizer --tool memcheck` puis `--tool racecheck`. Le compagnon doit reparcourir les résumés sanitizer, fermer les empreintes des journaux et du binaire, lier l'image et le SHA, puis rester `worker_passed_pending_shutdown`; il ne devient `passed` et n'est publié qu'après relecture indépendante de la cible exacte en `TERMINATED`. Une collision de publication doit retirer les liens déjà créés par la transaction. La fixture $B=61$ reste un préflight de taille et n'est pas exécutée géométriquement. Un échec de compilation, runtime, sanitizer ou fermeture de session laisse 7.8 non qualifié et ne modifie aucun statut scientifique.

## 6. Tests de Gamma, du catalogue Gabriel et de la réduction

### 6.1 Gamma exact et événements Gabriel partiels

Le backend `reference_cpu` exact énumère tous les sommets, cofaces et incidences de Gamma. Pour toute sphère critique de rang $k+1$, l'ensemble $S=I\cup U$ est comparé au simplex K-Gabriel attendu. La voie Gabriel doit émettre les $k+1$ facettes $S\setminus\lbrace x\rbrace$ et une hyperarête de niveau $r^2$, sans revendiquer l'exhaustivité de Gamma.

Les tests vérifient :

- la même connectivité pour une clique complète et pour une étoile de $k$ unions ;
- l'indépendance vis-à-vis du choix du pivot de l'étoile ;
- l'absence de perte après tri, déduplication et streaming ;
- la conservation de l'union de points couverte ;
- le cas $q=0$ qui crée une naissance réduite, $q=1$ qui ajoute seulement un `coverage_delta`, et $q\geq2$ qui crée une multifusion ;
- l'activation de toutes les facettes d'un événement, y compris celles qui ne sont pas des bras stricts ;
- l'égalité du profil exact avec Gamma exhaustif pour $n\leq14$ ;
- la présence de chaque `GammaCoface` dans exactement un lot du même ordre et niveau, y compris lorsqu'aucun `CriticalEvent` n'existe à ce niveau ;
- l'inclusion de chaque connexion Gabriel dans une composante Gamma au même niveau ;
- la divergence attendue du contre-exemple exact, avec interdiction de `public_status=exact` pour la voie Gabriel ;
- le rejet des simplexes non Gabriel, même s'ils proviennent d'une liste locale de voisins.

### 6.2 Lots de niveaux égaux

Toutes les naissances, facettes et hyperarêtes d'un même niveau exact sont appliquées comme un lot. Le test exécute artificiellement toutes les permutations d'un petit lot, puis plusieurs ordonnancements GPU aléatoires. Après canonicalisation, la forêt multifusion, les composantes et les flèches verticales doivent être identiques.

Un lot n'est jamais binarisé en fusions artificielles dépendant de l'ordre des threads. Si un format de sortie impose des nœuds binaires, les nœuds auxiliaires doivent être marqués, partager exactement le même niveau et se contracter vers la même multifusion canonique.

### 6.3 Streaming et runs externes

Les scénarios de reconstruction de cellules top-$m$ ci-dessous sont les tests historiques de l'oracle cellulaire de Phase 8, désormais gelé à $n\leq8$; ils ne sont pas une porte du chemin produit. Le streaming produit est testé séparément sur les frontières, événements, attaches et checkpoints des Phases 9 et 15.

Pour chaque fixture moyenne, la réduction est rejouée avec des budgets de lot qui forcent :

- un seul lot ;
- 2, 3, 7 et 31 lots ;
- des runs externes très petits ;
- un changement de frontière de lot au milieu d'un niveau égal ;
- une reprise après chaque run écrit.

Le résultat canonique doit être invariant. Un niveau égal ne peut être finalisé tant que tous les runs susceptibles de contenir ce niveau n'ont pas été fusionnés.

Pour $m<m_{\star}$, les fragments provenant de parents ou chunks différents ne sont jamais unis géométriquement dans la v2. Les tests font découvrir le même label $Q$ depuis une, deux, trois et plusieurs sources, puis reconstruisent $C_{m+1}(Q)$ depuis la boîte paddée $\Omega$ avec des amorces de contraintes différentes, y compris l'amorce vide. À chaque sommet provisoire, les co-maximiseurs de $Q$ sont comparés exactement aux co-1-NN de $X\setminus Q$; toutes les égalités actives sont réconciliées. Cellule, strates et certificat final doivent être identiques. À $m=m_{\star}$, les morceaux et leurs strates sont fermés, mais le test exige qu'aucun $C_{m_{\star}+1}$ ne soit construit ou propagé. Un prototype futur de complexe de fragments reste non certifiant tant qu'il ne passe pas ce différentiel et ne possède pas sa propre preuve.

La boîte $\Omega$ est testée sur des centres critiques situés sur les six faces, les douze arêtes et les huit sommets de l'AABB non paddée. Le padding dyadique doit les placer strictement à l'intérieur; seules les faces du padding portent le bit artificiel et elles ne produisent aucun événement.

Le jalon 8.1 teste séparément le constructeur de boîte avant toute fermeture de diagramme. La matrice courte couvre singleton et zéros signés, axes constants, nuages colinéaires ou coplanaires, sous-normaux autour de zéro, frontière de binade, extrema ex æquo avec témoin au plus petit `PointId`, permutations d'entrée et accord bit à bit avec l'AABB racine du LBVH. Les mots immédiatement intérieurs aux deux extrema finis doivent réussir avec une face égale à l'extremum; le plus grand fini négatif comme minimum ou le plus grand fini positif comme maximum doit produire `unsupported_finite_binary64_range`, tous les masques affectés et aucun payload.

Le vérificateur frais rejette les mutations indépendantes d'une face, d'un extremum, d'un témoin, d'une marge, d'un compteur, d'un booléen de portée, d'un masque, de la décision et du nuage source. Une réussite alimente `build_exact_bounded_h_polytope_reference` sans demi-espace sémantique : le résultat attendu est un $C_0$ tridimensionnel complet, huit sommets et exactement six faces artificielles sans identifiant de contrainte. Ces tests restent ciblés sous GCC et Clang stricts; ils ne remplacent pas les futurs différentiels cellule par cellule de la Phase 8.

Le jalon 8.2 teste ensuite la fermeture d'une cellule ordinaire unique sur le domaine borné $1\leq n\leq8$. Le singleton doit rendre exactement la boîte et l'amorce vide d'une paire doit enregistrer le plus petit concurrent extérieur comme germe de repli. Une mauvaise amorce contenant seulement un site lointain doit retrouver le site proche à un sommet exact violateur. Réciproquement, une amorce contenant le site proche peut fermer la cellule en laissant un site lointain strictement redondant hors de son ensemble fermé. La chaîne colinéaire $0,2,4,8,16$, amorcée par $16$, doit révéler successivement $8$, $4$ puis $2$ avant la ronde terminale. Le scénario « violateur seulement dans l'intérieur sans sommet violateur » est exclu : une différence affine positive quelque part atteint un maximum positif à un sommet et ne constitue donc pas une fixture mathématique valide.

La matrice d'incidences emploie les huit sommets d'un cube avec trois voisins axiaux dans l'amorce. La première ronde doit récupérer simultanément les quatre concurrents diagonaux et conserver les shells complets de face, d'arête et de sommet, jusqu'au shell de huit co-minimiseurs. Une fixture tangentielle ajoute un concurrent omis dont le demi-espace ne change pas la géométrie mais dont l'égalité au sommet doit être réconciliée. Une amorce permutée produit le même transcript canonique.

Les caps exacts du pire chemin après germe sont testés à leur valeur puis un par un juste en dessous : constructions, triplets, sommets, incidences, requêtes, distances, entrées de shell, faisabilité stricte du propriétaire et ajouts simultanés. Chaque insuffisance doit produire `insufficient_budget` avant toute ronde et avec audit nul; un plafond supérieur à la confiance est invalide. Le vérificateur frais doit rejeter au minimum les mutations de décision, propriétaire, identifiant complet hors plage, file vide, lot simultané, shell, incidence naturelle, nature artificielle, compteur, boîte et nuage source. Le résultat final est comparé à l'oracle toutes contraintes par sommets et identifiants actifs normalisés, pas par la liste brute des plans redondants.

Ces tests ferment seulement `bounded_n8_single_ordinary_cell_only`. Ils ne valident pas l'ensemble des cellules, les incidences réciproques, un catalogue, `closed_parent_orders[1]` ou un statut public.

Le jalon 8.3 construit ensuite toutes les cellules pour $1\leq n\leq8$ avec amorces vides canoniques et exige la bijection entre le shell complet de chaque sommet global et ses occurrences propriétaires. La somme des tailles de shells globaux doit donc égaler le nombre total de sommets dans les cellules finales. Les contacts $K_Q$ sont générés seulement lorsque leur liste de sommets $V_Q$ est non vide, puis leur shell carrier est recertifié au barycentre rationnel de $V_Q$.

La matrice géométrique minimale comprend : singleton sans contact; quatre sites collinéaires avec exactement trois faces adjacentes et aucune paire non adjacente; paire avec une face réciproque; triangle avec trois faces et une arête; tétraèdre avec six faces, quatre arêtes et un sommet; carré cocirculaire avec quatre faces, une unique arête de shell quatre, deux diagonales et quatre triples classés `noncanonical_quotient_contact`; cube cosphérique avec douze faces, six arêtes de shell quatre et un unique sommet de shell huit. Le carré interdit les fausses faces diagonales et le cube toute triangulation de la dégénérescence.

Une fixture portée par la boîte emploie $B=2^{52}$ et les sites $(-2,B+1,0)$, $(2,B+1,0)$ et $(-1,B+2,0)$. Leur contact de shell trois est la droite $x=0,y=B$, entièrement contenue dans la face artificielle basse en $y$ puisque $\mathrm{pred64}(B+1)=B$; il doit devenir `box_supported_contact` de masque 4, jamais `natural_edge`. Les extrémités artificielles ordinaires d'une vraie strate dont l'intérieur relatif quitte la boîte ne suffisent pas à la rendre artificielle : le test porte sur le ET commun des masques.

Le pire cas cube contrôle les 21 dimensions du budget à leur valeur, juste en dessous puis juste au-dessus : cellules, constructions, triplets, sommets, incidences, requêtes, distances, shells locaux, tests stricts, nombre de lots, total ajouté, taille maximale d'un lot, occurrences, sommets globaux, shells globaux, contacts, identifiants de requête et de carrier, références de sommets, requêtes et distances de témoins. Une insuffisance laisse cellules, sommets, contacts et audit vides. Le manifeste exact est testé avec deux nuages de même cardinal et de même certificat de boîte mais un point intérieur différent; le reçu insuffisant du premier doit être rejeté sur le second.

Le vérificateur rejette aussi cellule manquante, propriétaire local altéré, manifeste, occurrence ou shell hors plage, indice de sommet hostile, faux kind facial, bit artificiel, claim, audit, boîte et nuage. L'invariance à une permutation d'entrée est testée sur le carré. GCC et Clang stricts suffisent à ce jalon court; aucun benchmark de débit ne remplace le futur différentiel indépendant.

La réussite certifie seulement `bounded_n8_all_ordinary_cells_auditable_contacts_and_reciprocal_natural_strata_only`. Elle ne ferme ni la Phase 8, ni `closed_parent_orders[1]`, ni l'extraction des supports, `RelevantGP`, un catalogue ou un statut public.

Le jalon 8.4 ajoute un oracle Python `Fraction` indépendant. Son contrôle d'architecture doit rester couvert par `tools/check_oracle_independence.py`; le module n'importe aucune primitive C++, aucun paquet géométrique tiers et aucun backend de production. Le décodage des mots binary64, les différences de distances, la RREF, les systèmes actifs, les rangs et les barycentres sont réimplémentés localement.

Pour tout sous-ensemble non vide $Q$, le test construit directement son $K_Q$ par égalités d'équidistance, inégalités nearest et boîte, puis énumère les frontières dans l'espace affine réduit. Les singletons sont comparés aux cellules finales normalisées. Les contacts non singleton sont comparés aux positions référencées par 8.3 sans passer par ses indices. Avant toute conversion en dictionnaire, le test rejette les doublons de cellules, sommets, requêtes et positions afin qu'une collision ne masque jamais un payload amputé.

La campagne batch obligatoire comprend : singleton avec zéro signé et sous-normal; paire oblique; triple porté par la boîte; quatre sites collinéaires; carré cocirculaire; carré avec un sommet déplacé vers le prédécesseur puis le successeur binary64; tétraèdre; tétraèdre avec site central; six points d'une courbe des moments; cube privé d'un sommet; cube exact; cube avec un coin déplacé d'un ULP. Trois permutations fixes contrôlent la canonisation sans comparer les indices source.

Le comparateur exige l'égalité exacte des sommets de chaque cellule, des sommets globaux, distances nearest, shells, propriétaires, masques, contacts, carriers, dimensions, rangs, témoins et kinds. Le carré et le cube perturbés interdisent toute fusion tolérante; la chaîne collinéaire interdit l'expansion abusive au power-set; le site central doit détruire les incidences du tétraèdre vide; le ET des masques distingue une strate seulement tronquée d'un contact entièrement porté par la boîte.

Les caps 8.4 sont exercés par le test unitaire de l'oracle : valeurs exactes, chacune juste insuffisante avec payload géométrique vide, puis juste au-dessus de la confiance avec rejet. À $n=8$, ils couvrent 255 sous-ensembles, 769 égalités, 2546 inégalités, 13349 systèmes, 142982 formes, 108768 distances, 2288 références de cellule, 11061 références de contact et 247 témoins. Le CTest différentiel emploie un seul subprocess, un timeout de 45 secondes et aucune campagne aléatoire longue.

La réussite valide uniquement `bounded_n8_independent_affine_subset_oracle_only` et l'accord sémantique de 8.3. Elle ne valide pas les rondes, amorces, compteurs de travail ou budgets de 8.3, CUDA, $n>8$, les supports Morse, `RelevantGP`, le catalogue, `closed_parent_orders[1]` ou un statut public.

L'atlas `Fraction` est gelé à cette portée. Avant d'ajouter un autre oracle de Voronoï ou d'étendre cette baseline au-delà de huit sites, le plan de test doit comparer explicitement l'option d'un adaptateur Geogram épinglé : version et licence fixées, configuration reproductible, canonisation des identifiants, cas dégénérés et projection exacte à recertifier. Une baseline externe peut détecter des désaccords et mesurer le coût, mais ne décide jamais seule un statut exact. Aucun test de cet oracle CPU ne compte comme mesure du chemin chaud; les portes produit restent le p95 inférieur à une seconde à 50 000 points et $K_{\max}\leq10$, puis l'absence d'OOM et la reprise transactionnelle à dix millions de points ou davantage.

Le jalon 8.5 teste `morsehgp3d.phase8.exact_bounded_depth_zero_natural_supports.v1` sans campagne longue. Le singleton doit fermer une extraction vide. Le tétraèdre régulier doit produire six supports de taille deux, quatre de taille trois et un de taille quatre; avec `K_max=1`, seuls les six couples sont acceptés et les cinq strates supérieures restent classées au-dessus de la fenêtre, sans être supprimées du transcript candidat.

Le pentagone cocirculaire exact exerce un carrier naturel de shell cinq et ses quotients. Les sous-supports sont dédupliqués avant analyse; centre, niveau et barycentriques sont recalculés depuis les sites. La fixture doit rencontrer dépendance affine, barycentrique nul, centre extérieur, intérieur strict différé, extra-shell pertinent et support accepté. Le catalogue critique exhaustif sert uniquement ici d'oracle : après filtrage sur les supports de tailles deux à quatre, intérieur strict vide et fenêtre de rang, l'ensemble des supports acceptés et celui des diagnostics pertinents doivent être égaux dans les deux sens à la sortie 8.5.

Le triangle $(-1,0,0),(0,y,0),(1,0,0)$ est rejoué pour le prédécesseur binary64 de un, un exactement, puis son successeur. Sous la frontière, le diamètre absent d'un carrier naturel n'est pas inventé comme support différé. À la frontière, le triangle est `boundary_reduced_support` et le diamètre expose un extra-shell exact. Au-dessus, les quatre supports proposés sont acceptés. Une permutation complète doit conserver la boîte, le diagramme source, les indices canoniques de proposition et le résultat.

La fixture à base $2^{52}$ conserve le contact triple porté par la boîte uniquement dans le diagramme source : l'extracteur émet les deux faces naturelles et jamais le quotient ni le support artificiel. Pour tout support accepté de taille $r$, le test exige shell égal au support, rang fermé $r$, rang affine $r-1$, dimension $4-r$, masque commun nul et kind face, arête ou sommet selon $r=2,3,4$.

Les six caps 8.5 sont exercés à $n=8$ sans construire le diagramme du cube. Une capacité juste sous 247, 4704, 13440, 154, 504 ou 1232 doit produire un reçu `insufficient_budget` lié au nuage et à la boîte, sans source embarquée, candidat, support, diagnostic, audit géométrique ni début d'extraction. Chaque cap juste au-dessus de sa confiance est rejeté avant tout rejeu source. Une vraie source ordinaire `insufficient_budget` certifiée sous son propre budget, puis une source qui affirme `complete` mais perd un contact, doivent toutes deux arrêter 8.5 avant la première proposition.

Le vérificateur frais reçoit toujours le nuage, la boîte, la source, `K_max` et le budget comme autorités externes. Les mutations indépendantes couvrent source embarquée, décision candidat, indice de contact hors plage, association diagnostic, compteurs de prédicats, audit, fait terminal, décision globale, budget et nuage. Aucun payload observé ne doit piloter le rejeu. La réussite valide seulement les candidats de profondeur zéro dans `bounded_n8` et les supports acceptés dans la fenêtre de pertinence; elle ne certifie ni singleton H0, ni profondeur positive, ni `RelevantGP` global, ni `CatalogCertificate`, ni `closed_parent_orders[1]`, ni GPU, débit ou statut public.

### 6.4 Multiplicité de Morse et nombre de bras

Pour un événement de support frontal $U$ et d'indice $\mu$, l'oracle enregistre la multiplicité de Reani–Bobrowski $\Delta=\binom{\lvert U\rvert-1}{\mu}$. À l'indice un, il construit les $\lvert U\rvert$ bras et vérifie que le lot tue au plus $\lvert U\rvert-1$ classes de $H_0$. Les fixtures obligatoires couvrent :

- $\lvert U\rvert=2$ avec fusion binaire;
- $\lvert U\rvert=3$ avec trois bras et fusion triple;
- $\lvert U\rvert=4$ avec quatre bras et fusion quadruple;
- deux ou plusieurs bras déjà dans la même composante avant le lot;
- tous les bras déjà connectés, donc aucune fusion $H_0$ même si l'événement peut créer une classe de $H_1$ non représentée.

La sortie est une multifusion canonique. Elle ne doit jamais être binarisée par l'ordre des threads; tout nœud binaire auxiliaire imposé par un format est marqué et se contracte vers le lot unique.

### 6.5 Tour globale de boules saturées

Cette suite teste la voie candidate de la phase 17 sans l'activer comme base de preuve publique. Pour chaque nuage exact de $n\leq14$, l'oracle structurel interne couvre $1\leq k\leq n$, y compris le cas terminal; les assertions du contrat v2 restent séparément bornées à $1\leq k\leq\min(10,n)$.

Le système sous test utilise le noyau exact C++ issu de la phase 2A. La référence Gamma reste l'oracle Python `Fraction`, soumis au contrôle d'indépendance existant. Les deux chemins ne doivent pas appeler la même implémentation de miniball, de classification de sphère ou de Kruskal; un différentiel seulement structurel qui partagerait sa géométrie ne fermerait pas 17A.

L'oracle :

1. énumère tous les supports affinement indépendants de tailles un à quatre;
2. calcule leur miniball exacte et exige le centre dans $\mathrm{relint}\,\mathrm{conv}(U)$;
3. classifie chaque observation comme intérieur, shell ou extérieur de la boule fermée;
4. construit $S=\mathrm{Sat}(U)$ et vérifie $\beta(S)=\beta(U)$ ainsi que l'idempotence;
5. déduplique les boules et saturés tout en conservant tous les supports témoins;
6. matérialise $\Delta(S)$ seulement lorsque le budget combinatoire le permet;
7. compare l'union descendante au complexe de Čech exhaustif;
8. matérialise l'union des graphes de Johnson $J_k(S)$ et compare exactement ses $k$-faces et arêtes à Gamma;
9. développe chaque composante de $H_k(a)$ en sa famille de $k$-faces et compare cette partition ainsi que sa couverture aux composantes de Gamma;
10. compare de la même façon une forêt de Kruskal de poids maximum seuillée à $H_k(a)$ et à Gamma;
11. répète les étapes 7–10 pour les coupes ouvertes $t<a$ et fermées $t\leq a$.

Le graphe $H_k(a)$ n'est pas isomorphe à Gamma : ses sommets sont des générateurs, tandis que ceux de Gamma sont des $k$-faces. La comparaison exacte des sommets et arêtes concerne donc Gamma et l'union matérialisée des graphes de Johnson. Pour $H_k(a)$ et sa forêt seuillée, elle concerne les composantes après expansion en $k$-faces, puis les couvertures en observations, pas seulement leur nombre. Les générateurs dont $\lvert S\rvert<k$ sont absents de la coupe d'ordre $k$. Pour $k\geq2$, une composante omise par `hgp_reduced` est identifiée sémantiquement comme une seule $k$-face isolée; le nombre de supports témoins ne doit jamais la rendre artificiellement non triviale.

La forêt statique de chaque coupe est comparée à la mise à jour insertionnelle lot par lot. Les tests mélangent l'ordre des supports, des arêtes, des poids égaux et des threads simulés, puis exigent :

- le même ordre total canonique de Kruskal;
- les mêmes composantes à chaque seuil d'ordre;
- le même état strict pré-lot et fermé post-lot;
- les mêmes naissances, prolongements, multifusions et `coverage_delta`;
- les mêmes identifiants canoniques après rejeu;
- les mêmes applications verticales et carrés de naturalité;
- une reprise après checkpoint identique à une exécution continue.

Le checkpoint contient les checksums de l'entrée et de la configuration, le catalogue et la déduplication actifs, les postings, la forêt courante, le dernier lot entièrement committé, le curseur du flux restant, l'identifiant de l'ordre total canonique et le journal de rejeu. Le test régénère le suffixe depuis les checksums et le curseur, puis exige son identité. Un état pris au milieu d'un lot est temporaire : il est annulé ou rejoué intégralement et ne devient jamais un snapshot publiable.

Les remplacements d'arêtes de la forêt de générateurs ne sont jamais comparés directement aux événements HGP. Seules les différences de composantes pré-lot/post-lot alimentent le `MergeForest` attendu.

Fixtures obligatoires :

- `gabriel-point-set-counterexample-5-points-v1`, avec le générateur `ACDE` au niveau $33/2$, puis l'intersection de capacité deux avec `ABC` au niveau $83886/3563$;
- `morse-rank-window-regression-v1`, qui impose $\lvert I\rvert=2$, $\lvert U\rvert=2$, les seuls ordres critiques trois et quatre, $D_2(c)<a$ malgré l'action combinatoire descendante et $D_5(c)>a$ au-dessus du rang fermé;
- une famille obtenue en ajoutant arbitrairement des points strictement dans la miniball d'une petite face, afin de falsifier toute troncature $\lvert S\rvert\leq K_{\mathrm{eff}}+1$;
- shells cosphériques de plus de quatre points et plusieurs supports minimaux de la même boule;
- niveaux égaux provenant de boules distinctes et niveaux rationnels presque égaux;
- nuages colinéaires, coplanaires et tridimensionnels, doublons refusés selon le contrat courant;
- saturés emboîtés, disjoints, fortement recouvrants et antichaînes d'inclusion;
- le contre-exemple non monotone de l'audit, où $Q\subseteq R$ mais $\mathrm{Sat}(Q)\nsubseteq\mathrm{Sat}(R)$;
- triangle abstrait de capacités $2,2,1$, qui rejette une forêt seulement maximale ne contenant pas les deux arêtes de capacité deux.

Le pruning par inclusion est désactivé dans la baseline. Sa variante expérimentale doit produire les mêmes coupes, couvertures, journaux et morphismes, avec une table explicite de domination et de provenance. Elle est testée contre des suppressions d'un sommet feuille et d'un sommet interne de la forêt; l'algorithme ne peut libérer les anciennes arêtes avant rewiring certifié.

Pour chaque sous-famille de générateurs, la suite vérifie l'inclusion dans Gamma et l'absence de fausse connexion. Elle construit aussi des sous-familles qui retardent volontairement une naissance et une fusion, afin d'interdire la promotion de leurs nœuds partiels en événements HGP exacts. ANN, Delaunay ou le raffinement courant ne sont que des sources de propositions; chaque support retenu est resaturé exactement et l'artefact conserve la sémantique scientifique interne `partial_refinement` tant que l'univers complet n'est pas certifié. Aucun `MorseHGP3DResult` v2 n'est émis pour cette voie avant ajout contractuel d'une base de preuve dédiée.

La campagne de performance CPU bornée couvre $n\in\left\lbrace16,24,32,48,64,96,128\right\rbrace$ avec arrêt budgétaire. Elle est hors CI et explicitement opt-in. Chaque manifeste fixe avant exécution `time_budget_s`, `host_budget_bytes`, `scratch_budget_bytes` et `output_budget_bytes`; une observation censurée par budget est une sortie valide à publier comme telle, pas un échec à masquer. La campagne compare graphe statique complet, postings dynamiques, forêt recalculée, mise à jour insertionnelle et pruning expérimental sur les familles volumique, surfacique, en amas et adversariale. Une moyenne favorable ne change aucun statut scientifique.

## 7. Invariants de hiérarchie

### 7.1 Invariants horizontaux

Pour chaque ordre $k$ :

1. les niveaux le long de toute arête enfant–parent sont non décroissants ;
2. la forêt est acyclique et tout identifiant actif a un représentant ;
3. à chaque seuil d'oracle, les composantes reconstruites depuis la forêt coïncident avec celles du graphe exhaustif ;
4. une fusion en lot relie exactement les composantes incidentes juste sous le niveau ;
5. une croissance $q=1$ conserve la racine et ajoute exactement son `coverage_delta` ;
6. aucune composante ne se scinde lorsque $t$ augmente ;
7. le nombre de composantes vérifie la comptabilité naissances–fusions du lot ;
8. l'union de points stockée pour un nœud est celle de ses facettes ou enfants ;
9. les recouvrements entre composantes de la couverture d'ordre $k\geq2$ sont préservés, non arbitrairement résolus en partition.

### 7.2 Invariants verticaux

La monotonie $L_{k+1}(t)\subseteq L_k(t)$ impose qu'une composante d'ordre $k+1$ soit incluse dans une unique composante d'ordre $k$ au même niveau. Pour chaque flèche verticale, les tests vérifient :

- existence et unicité de la cible ;
- inclusion de l'union de points couverte lorsque cet invariant est pertinent au périmètre réduit ;
- cohérence avec l'ancre de rang de chaque sphère critique ;
- absence de cible créée à un niveau strictement supérieur ;
- commutation avec la montée en échelle.

Si $h^k_{t,t'}$ désigne l'application horizontale et $v^k_t$ l'application verticale de l'ordre $k+1$ vers l'ordre $k$, l'identité testée est

$$v^k_{t'}\circ h^{k+1}_{t,t'}=h^k_{t,t'}\circ v^k_t.$$

Elle est vérifiée à tous les couples de seuils consécutifs de l'oracle, ainsi qu'autour de chaque niveau partagé par plusieurs ordres.

### 7.3 Cohérence des préfixes en ordre

Une exécution avec $K_{\max}=10$ doit donner, après restriction, exactement le même résultat que des exécutions indépendantes avec $K_{\max}=1,2,\ldots,9$. Ce test détecte les caches inter-ordres mal invalidés et les heuristiques dont le budget dépend involontairement de l'ordre maximal.

### 7.4 Miniball local, arcs et segments de descente

Pour toute facette de cardinal $1\leq k\leq10$, l'oracle `reference_cpu` énumère exactement les sous-supports de tailles un à quatre. Les compteurs doivent égaler $\sum_{j=1}^{4}\binom{k}{j}$, donc 385 à $k=10$, et chaque support relativement intérieur doit classifier les $k$ points sans arrêt anticipé. Le rejeu compare la facette canonique, le support choisi, le centre, le rayon carré, la partition intérieur--shell, les comptes par décision, le nombre de supports optimaux, le statut et le scope.

Les cas ciblés obligatoires du miniball local sont le singleton, le triangle obtus, le triangle rectangle avec shell supplémentaire, le triangle aigu, un tétraèdre à support quatre, un tétraèdre réduit à une face, le carré à deux supports optimaux, six points cosphériques où un triple extérieur englobant doit être rejeté au profit d'un support positif de cardinal quatre, et dix points colinéaires. Les falsifications portent séparément sur chaque champ. L'oracle indépendant `reference/morsehgp3d_oracle/geometry.py::minimum_enclosing_ball` partage désormais la sémantique des supports positifs; son test à six points est obligatoire, mais le rejeu C++ frais ne remplace pas le futur raccord différentiel.

Le jalon 6.2 recoupe ensuite le résultat local avec `brute_force_closed_ball` sur tout $X$ et `brute_force_top_k` sans exclusion au centre exact. Pour $I_{\mathrm{ext}}=I_X\setminus F$ et $U_{\mathrm{ext}}=U_X\setminus F$, le rejeu exige : identité des intersections locales intérieur--shell; rang fermé égal à $k+\lvert I_{\mathrm{ext}}\rvert+\lvert U_{\mathrm{ext}}\rvert$; cutoff top-$k$ inférieur ou égal à $\beta(F)$; égalité des partitions top-$k$ et boule globale lorsque les deux niveaux sont égaux; appartenance de $F$ à la famille top-$k$ exactement lorsque $I_{\mathrm{ext}}$ est vide. Cette appartenance ne se teste ni par `canonical_choice_ids==F`, ni par `cutoff==beta`.

Les décisions sont fail-closed. `strict_descent_admissible` exige une facette inactive, `boundary_point_ids==support_point_ids`, un unique support optimal et $U_{\mathrm{ext}}=\varnothing$. `already_active_at_own_center` exige les mêmes préconditions régulières et $I_{\mathrm{ext}}=\varnothing$. Toute autre combinaison retourne `unsupported_degeneracy`, tout en conservant séparément le booléen exact d'activité. La validation ciblée comprend obligatoirement : la paire $(-1,0,0),(1,0,0)$ avec l'intrus $(0,0,0)$, inactive et strictement admissible bien que son cutoff top-2 vaille encore un; la paire $(-2,0,0),(2,0,0)$ avec les deux intrus $(-1,0,0),(1,0,0)$, qui exerce un cutoff top-2 strictement inférieur; la première paire avec un point extérieur au shell; un triangle rectangle de frontière non essentielle avec un choix top-3 descendant et un choix de niveau constant; une paire active avec un point lointain; et une paire active appartenant à une famille top-2 dont le représentant canonique est différent. Un shell top-$k$ multivalué n'est pas classé comme plateau lorsque les préconditions universelles de stricte décroissance sont satisfaites.

Le rejeu falsifie séparément la miniball embarquée, les deux partitions globales, l'identité du nuage, les compteurs, les faits d'essentialité, d'activité et de régularité, la décision et la portée; le test contrôle aussi la constante statique `proof_basis`. Il certifie seulement `global_shell_and_top_k_preconditions_only`. Le représentant canonique peut être conservé comme donnée de la partition, mais il n'est ni construit ni publié comme successeur; aucune égalité ou baisse de miniball d'un arc choisi, aucun segment sous-niveau, aucune attache, aucune forêt, aucun `public_status` et aucune qualification CUDA/G4 ne sont couverts.

Le jalon préparatoire 6.3 appelle `build_exact_facet_descent_arc` et son rejeu. Un payload d'arc n'est permis que si le résultat 6.2 fraîchement certifié vaut `strict_descent_admissible`. Le test exige alors $G$ égal à `canonical_choice_ids`, $G\in\mathcal{N}_k(c_F)$, $G\neq F$, une miniball de $G$ fraîchement reconstruite et les comparaisons exactes $\beta(G)\leq D_k(c_F)\leq\beta(F)$ et $\beta(G)<\beta(F)$. `no_arc_already_active_at_own_center` et `no_arc_unsupported_degeneracy` laissent absents les identifiants et la miniball cibles, laissent faux tous les faits d'arc et ferment leurs compteurs à `(1,0,0,0)`.

Deux fixtures strictes fixent les témoins numériques sans se limiter à un accord avec le rejeu. Pour $X=\left\lbrace(-1,0,0),(0,0,0),(1,0,0)\right\rbrace$ et $F$ égal à la paire extrême, le choix canonique est la première extrémité avec l'intrus, $c_F=0$, $\beta(F)=d_2(c_F)=1$, le centre cible vaut $-1/2$ sur l'axe et $\beta(G)=1/4$. Pour $X=\left\lbrace(-2,0,0),(-1,0,0),(1,0,0),(2,0,0)\right\rbrace$ et la paire extrême, le choix canonique est la paire intérieure, les deux centres valent zéro et $\beta(G)=d_2(c_F)=1<\beta(F)=4$. Les cas 6.2 actif et non pris en charge doivent produire zéro arc, même lorsqu'un représentant top-$k$ diagnostique existe.

Les mutations 6.3 portent séparément sur les préconditions source, l'absence d'un optionnel obligatoire, l'injection d'un optionnel dans une décision sans arc, la substitution d'un membre top-$k$ valide mais non canonique, le désaccord entre identifiants choisis et facette de la miniball cible, les cinq faits booléens, les quatre compteurs, la décision, la portée et l'identité d'un nuage jumeau. Une égalité ou inversion de niveau après `strict_descent_admissible` doit échouer fermée, jamais devenir `unsupported_degeneracy`. La cible unitaire passe en Release strict sous GCC et Clang avec ces mutations. La portée reste `canonical_top_k_selected_strict_level_arc_only`, sans segment, DAG, pointer-jumping, attache, forêt, `public_status`, différentiel indépendant, CUDA ou G4.

Le jalon préparatoire 6.4 appelle `build_exact_facet_descent_segment` et `verify_exact_facet_descent_segment`. Il ne produit un `segment_witness` qu'à partir de `strict_descent_arc_certified`; les décisions `no_segment_already_active_at_own_center` et `no_segment_unsupported_degeneracy` conservent l'arc source rejoué mais n'émettent aucun témoin. Pour $R=\beta(F)$, le test recalcule sur tous les points de $G$ la valeur $a=g_G(c_F)=\max_{x\in G}\left\Vert c_F-x\right\Vert^2$, puis exige $a=D_k(c_F)\leq R$, $b=\beta(G)<R$ et $\delta=\left\Vert c_G-c_F\right\Vert^2\geq0$. Les compteurs stricts, ordonnés comme dans l'API, valent `(1,k,k-1,1,4,1)`; pour chaque branche sans segment ils valent `(1,0,0,0,0,0)`.

Le certificat analytique est vérifié sans échantillonnage : pour chaque $x\in G$ et $\gamma(t)=(1-t)c_F+tc_G$, l'identité exacte est $\left\Vert\gamma(t)-x\right\Vert^2=(1-t)\left\Vert c_F-x\right\Vert^2+t\left\Vert c_G-x\right\Vert^2-t(1-t)\delta$. Le passage au maximum certifie seulement l'inégalité $g_G(\gamma(t))\leq q(t)=(1-t)a+tb-t(1-t)\delta$, jamais une égalité universelle. Comme $D_k(\gamma(t))\leq g_G(\gamma(t))$ et $q(t)-R=(1-t)(a-R)+t(b-R)-t(1-t)\delta$, le segment fermé $t\in[0,1]$ reste dans le sous-niveau non strict et le demi-segment $t\in(0,1]$ est strict. La source peut satisfaire $a=R$; le test ne demande donc pas une stricte sous-niveauté à $t=0$. Le cas $\delta=0$ est accepté exactement lorsque les centres sont égaux, ce qui impose aussi $a=b$.

Les fixtures 6.4 validées prolongent les deux arcs stricts. La première fixe `(a,b,delta)=(1,1/4,1/4)`, `source_endpoint_strict_sublevel=false` et utilise $q(1/2)=9/16$ uniquement comme diagnostic exact, pas comme preuve du segment. La seconde fixe `(a,b,delta)=(1,1,0)`, des centres égaux et une source déjà strictement sous le niveau $R=4$. Les cas actif et non pris en charge exigent l'absence du témoin. Les mutations portent séparément sur l'arc source, l'absence ou l'injection du témoin, $a$, $b$, $\delta$, chacun des cinq booléens du témoin, chacun des six compteurs, la décision, la portée et l'identité d'un nuage jumeau.

La cible unitaire 6.4 passe en Release strict sous GCC et Clang avec ces fixtures et mutations; le statut est `validated_host_software`. La portée est `canonical_strict_arc_half_open_sublevel_segment_only` sous `exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1`. Elle couvre un seul segment et exclut concaténation, DAG, pointer-jumping, germe, attache, forêt, `public_status`, différentiel indépendant, CUDA et G4.

Le jalon préparatoire 6.5 appelle `build_exact_facet_descent_chain` avec une politique de budget externe explicite, puis `verify_exact_facet_descent_chain` avec une politique fiable séparée. Chaque transition reconstruit 6.4 depuis la facette courante, exige que sa cible soit la source suivante et rejoue exactement la facette, le centre rationnel et le niveau de miniball à la couture. Les niveaux de nœuds vérifient $\beta(F_{i+1})<\beta(F_i)$ à cardinal fixé; le test exige donc l'absence de répétition et la borne théorique d'au plus $\binom{n}{k}-1$ segments. Le booléen `finite_strict_facet_orbit_theorem_certified` certifie cette implication finie, tandis que `effective_maximum_committed_strict_segment_count` et la décision terminale disent séparément si le budget a tronqué la chaîne.

La fixture exacte à six points part de $F_0=\left\lbrace0,1,2,4\right\rbrace$ et ferme trois nœuds de niveaux $52$, $85/4$ et $325/16$. Les deux segments ont respectivement $(a,b,\delta)=(50,85/4,65/4)$ et $(85/4,325/16,5/16)$; elle ferme la concaténation, les compteurs et les budgets. Une seconde fixture à cinq points part de $F_0=\left\lbrace0,2\right\rbrace$ et ferme les niveaux $58$, $49/4$ et $1/4$ avec les témoins $(50,49/4,109/4)$ puis $(41/4,1/4,8)$. Elle impose $D_k(c_{F_1})=41/4<\beta(F_1)=49/4$ et interdit ainsi de confondre le niveau de miniball cousu avec le niveau atomique du segment suivant. Les deux terminaisons exactes sont actives.

Les budgets obligatoires sont zéro, un et deux sur cette fixture. Le budget zéro conserve seulement le nœud initial et le prochain segment dans `stopping_probe`; le budget un conserve exactement le premier segment et son nœud cible; le budget deux atteint la terminaison active. Les deux premiers retournent `certified_prefix_strict_segment_budget_exhausted`. Une fixture régulière active termine sans segment, une fixture non essentielle termine par `certified_prefix_blocked_unsupported_degeneracy`, et toute limite supérieure au plafond interne de 4096 est rejetée avant construction.

Les compteurs de chaîne, les compteurs agrégés de 6.4, les nœuds, les segments, le probe terminal, la décision, la portée, la base de preuve et l'identité du nuage sont falsifiés séparément. Les mutations couvrent aussi le budget observé, la politique fiable du vérificateur, le potentiel strict, l'unicité des facettes, chaque couture exacte, chacun des quatre faits globaux et `finite_strict_facet_orbit_theorem_certified`. La portée `single_source_canonical_strict_descent_chain_only` ne couvre ni germe initial depuis un centre critique, ni fermeture multi-source en DAG, pointer-jumping, plateau, attache, forêt, `public_status`, différentiel indépendant, CUDA ou G4.

Le jalon validé 6.6 appelle `build_exact_critical_arm_initial_segment` depuis le shell critique complet $U$ et un point retiré $u\in U$. Le rejeu reconstruit la miniball de $U$, exige $2\leq\lvert U\rvert\leq4$, un support positif minimal égal à $U$, puis classifie tout le nuage afin de vérifier que le shell global est exactement $U$. Pour la partition $S=I\cup U$, il fixe le rang fermé $\lvert S\rvert\leq11$, l'ordre $k=\lvert S\rvert-1\leq10$ et la facette $F_u=S\setminus\left\lbrace u\right\rbrace$ sans jamais demander une miniball bornée de $S$. Une source non minimale, un shell global incomplet ou un rang fermé supérieur à onze retourne une décision non prise en charge sans payload de bras.

Pour une source prise en charge, le test reconstruit la miniball fraîche de $F_u$, exige $\beta(F_u)<a$, $\delta=\left\Vert c_{F_u}-c\right\Vert^2>0$, puis vérifie le sens sortant du point retiré. Avec $d=c_{F_u}-c$, chaque point extérieur conserve exactement sa clairance $A_p=\left\Vert c-p\right\Vert^2-a$ et son coefficient complet $B_p=2(c-p)\mathbin{\cdot}d$; seuls les cas $B_p<0$ contribuent la borne conservative $A_p/(-2B_p)$. Le minimum rationnel de ces bornes et de un doit être strictement positif et certifier tout le demi-segment source-ouvert jusqu'à ce paramètre, sans échantillonnage.

La fixture numérique obligatoire utilise $U=\left\lbrace(-2,0),(0,3),(2,0)\right\rbrace$, retire le premier point et ajoute $(2,2)$ à l'extérieur. Elle fixe $c=(0,5/6)$, $a=169/36$, $F_u=\left\lbrace(0,3),(2,0)\right\rbrace$, $c_{F_u}=(1,3/2)$, $\beta(F_u)=13/4$, $\delta=13/9$, le coefficient sortant $46/9$, puis $A_p=2/3$, $B_p=-50/9$ et $\tau=3/50$. Un tétraèdre critique à support positif de cardinal quatre ferme séparément $(a,b,\delta,B_u)=(3,8/3,1/3,2)$. Une fixture colinéaire avec point intérieur exerce $S\neq U$; la frontière de rang fermé onze construit exactement une facette de dix points, tandis que le rang douze échoue fermé. Les entrées vides, singleton, de cardinal supérieur à quatre, dupliquées, hors domaine ou dont le point retiré n'appartient pas à $U$ sont refusées.

`build_exact_critical_arm_descent` doit raccorder exactement $F_u$, son centre et son niveau au premier nœud de 6.5. Les budgets de chaîne zéro et un excluent toujours le segment initial : zéro conserve ce germe et un probe 6.5 non engagé; un engage exactement la transition vers la facette active suivante. `committed_composite_path_segment_count` compte uniquement les segments engagés, soit le segment initial plus les segments de chaîne engagés; il exclut tout `stopping_probe` certifié mais non engagé. Une source critique non prise en charge n'émet aucune chaîne; une source prise en charge dont $F_u$ possède une frontière de triangle rectangle non essentielle mappe exactement l'arrêt 6.5 par dégénérescence. Un budget supérieur à 4096 est rejeté avant toute classification, y compris lorsque le shell fourni mènerait ensuite à une source non prise en charge. Le rejeu falsifie séparément miniball critique, coefficients initiaux, contrainte extérieure, borne locale, faits, compteurs, décisions, portées, chaîne embarquée, couture, convention budgétaire et identité d'un nuage jumeau. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_miniball` passent; 6.6 est `validated_host_software`. La portée `single_index_one_critical_arm_plus_canonical_strict_chain_only` demeure mono-bras et ne couvre ni fermeture de labels, racine, attache, DAG, pointer-jumping, plateau, forêt, `public_status`, différentiel indépendant, CUDA ou G4.

Le jalon 6.7 appelle `build_exact_critical_arm_family_descent` puis `verify_exact_critical_arm_family_descent`. Pour le shell critique complet canonisé $U$, le test exige exactement $\lvert U\rvert$ bras ordonnés par le `PointId` retiré, un rejeu 6.6 indépendant sous le même budget pour chacun et l'identité exacte de la source critique partagée. Tout bras complet fournit un terminal $T_u$ régulier, actif, canonique et de niveau $\beta(T_u)<a$. Les labels égaux doivent porter le même témoin géométrique exact, être regroupés dans une classe unique ordonnée et conserver la liste canonique de tous les points retirés qui les atteignent.

`complete_terminal_label_partition_certified` est exigé exactement lorsque tous les bras terminent sur une facette régulière active. Une source critique non prise en charge doit retourner `no_family_unsupported_critical_source` sans classe terminale complète. Les budgets insuffisants, les dégénérescences de chaîne et leur coexistence exercent séparément `incomplete_chain_budget_exhausted`, `incomplete_unsupported_degeneracy` et `incomplete_mixed_stops`; les classes éventuellement observées restent des données partielles et ne peuvent jamais promouvoir le drapeau de complétude. La validation 6.7 couvre les permutations du shell, les supports critiques de cardinal deux, trois et quatre, le cas $S\neq U$ et plusieurs labels distincts. La fixture canonique à cinq points $(-8,1),(-5,-7),(-3,-8),(4,8),(5,-7)$, de shell $U=[1,2,3]$, niveau $25925/338$, ordre trois et budget deux ferme séparément trois bras en seulement deux classes : les retraits un et trois atteignent tous deux $[0,1,2]$, dont la provenance vaut $[1,3]$, tandis que le retrait deux atteint $[0,1,3]$. Le rejeu 6.7 falsifie séparément le budget commun, l'identité des bras, une descente 6.6, un terminal, la provenance d'une classe, les compteurs, le drapeau de complétude, la décision, la portée et l'identité d'un nuage jumeau.

Les builds GCC et Clang Release stricts et le CTest ciblé `morsehgp3d.hierarchy_miniball` ferment la validation hôte de 6.7. La portée `all_index_one_critical_arms_independent_canonical_strict_chains_terminal_labels_only` certifie seulement la famille événement-locale et les classes d'identité de facettes. Une facette terminale active n'est pas une racine globale; des labels distincts ne prouvent pas des composantes Gamma distinctes; une classe d'identité n'est ni Gamma, ni une attache. Aucun `public_status` n'est produit.

### 7.5 Coupe Gamma strictement ouverte bornée 6.8

Le jalon 6.8 construit indépendamment la coupe exhaustive de Gamma strictement antérieure à un niveau exact $a$. Il accepte $2\leq n\leq14$, $1\leq k\leq10$ avec $k<n$, et entre une et quatre facettes sources canoniques distinctes de cardinal $k$. Une facette ou une coface est active exactement lorsque son niveau vérifie $\beta<a$; l'égalité reste inactive. Les composantes contiennent les familles canoniques de facettes, y compris les facettes actives isolées du profil `full_pi0`, et non leurs seules unions de points.

Le préflight calcule avant toute géométrie $\binom{n}{k}$ facettes, $\binom{n}{k+1}$ cofaces et $k\binom{n}{k+1}$ tentatives d'union. Les trois budgets doivent couvrir simultanément ces valeurs. Sinon `no_cut_preflight_budget_insufficient` ne contient ni catalogue actif, ni composante, ni classification source. Une exécution complète distingue `complete_all_sources_active_and_classified` de `complete_with_inactive_sources`; une source de niveau égal ou supérieur à la coupe est inactive sans indice de composante, mais la coupe exhaustive reste complète.

Pour $k<10$, chaque coface possède au plus dix points et reçoit une miniball directe. Au bord $n=11$, $k=10$, le test exige l'identité $\beta(Q)=\max_{q\in Q}\beta(Q\setminus\lbrace q\rbrace)$, le premier maximiseur lexicographique, puis la vérification exacte que sa boule couvre le point omis. Les onze miniballs de facettes sont celles du cache exhaustif de taille dix; aucune primitive hors de sa borne ne peut être appelée.

Les fixtures obligatoires couvrent : une coface exactement au niveau de coupe et donc inactive; deux cofaces strictes reliées par une facette-pont; trois sources réparties entre une composante à plusieurs facettes et une composante isolée; une famille partiellement inactive; le refus atomique d'un budget insuffisant; `gabriel-point-set-counterexample-5-points-v1`, dont les incidences Gamma silencieuses doivent rester actives; et le bord $n=11$, $k=10$. Le rejeu frais falsifie séparément le budget demandé, l'entrée et le niveau de coupe externes, les comptes de préflight, une facette, une coface, une composante, une classification source, les compteurs exhaustifs, un fait de résultat, la décision, la portée et le témoin de suppression à onze points. Un différentiel indépendant compare en outre cinq coupes exactes au Gamma Python `Fraction` : binaire, médiée, ternaire, contre-exemple Gabriel à incidences silencieuses et bord $n=11$, $k=10$; il ne partage ni miniball, ni DSU, ni réduction C++.

La base `exact_bounded_exhaustive_strict_gamma_full_pi0_source_component_classification_v1` et la portée `bounded_exhaustive_strict_gamma_full_pi0_source_components_only` certifient seulement la coupe bornée et la localisation des facettes sources. Un indice de composante n'est ni une racine hiérarchique, ni une attache de Morse, ni un nœud de DAG ou de forêt. Le raccord effectif des terminaux complets de 6.7 à ces composantes reste le jalon suivant; 6.8 ne produit aucun `public_status` et ne qualifie ni CUDA, ni G4, ni la scalabilité.

### 7.6 Raccord borné des bras aux composantes strictes 6.9

Le jalon 6.9 doit reconstruire 6.7 depuis le nuage, le shell critique et le budget par bras, puis dériver de cette source partagée l'ordre $k$ et le niveau critique $a$. Une coupe 6.8 exhaustive sous $\beta<a$ ne peut être demandée que si la famille terminale est complète. Elle reçoit exactement une source par classe de label terminal, jamais une source par bras. Le résultat conserve séparément les projections classe--composante, bras--classe--composante et les groupes de composantes incidentes.

La fixture positive obligatoire utilise les points canoniques $A=(-2,0)$, $Q=(-2,2)$, $B=(0,3)$, $C=(2,0)$ et $P=(2,2)$, le shell $U=[0,2,3]$, l'ordre $k=2$ et le niveau $a=169/36$. Avec un budget de chaîne par bras égal à un et le budget Gamma minimal $(10,10,20)$, les terminaux canoniques $[0,1]$, $[0,3]$, $[2,4]$ doivent être classés dans les composantes $(0,1,0)$. Les classes zéro et deux restent des labels distincts mais appartiennent au même groupe Gamma; les provenances retirées de ce groupe sont $[0,3]$. Ce cas interdit de remplacer la connectivité Gamma par l'égalité des labels.

Un budget de chaîne nul laisse la famille incomplète et doit produire zéro construction Gamma et zéro projection. Avec une famille complète et le budget Gamma $(9,10,20)$, le préflight doit annoncer exactement les besoins $(10,10,20)$ puis produire zéro miniball, zéro union, zéro composante, zéro classification source et zéro projection 6.9. Le shell critique non minimal doit rester une source exacte non prise en charge sans engager Gamma. Les nuages de plus de quatorze points et les budgets au-dessus des plafonds sont rejetés avant composition.

Le rejeu falsifie séparément les deux budgets stockés et externes, le shell stocké et externe, la famille 6.7, l'ordre, le niveau, la présence et le contenu de Gamma 6.8, chaque projection, la provenance d'une composante incidente, chaque fait, les compteurs, la décision, la portée et l'identité d'un nuage jumeau. La base `exact_complete_critical_arm_family_strict_path_bounded_exhaustive_open_gamma_component_classification_v1` et la portée `bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only` ne certifient ni racine, ni fusion, ni attache publique, ni lot simultané de niveaux égaux, ni DAG, ni forêt, ni `public_status`, CUDA, G4 ou scalabilité.

La fixture à cinq points de 6.7 est aussi rejouée intégralement par 6.9 avec le budget Gamma minimal $(10,5,15)$. Ses trois bras produisent deux sources Gamma, deux composantes incidentes et les indices de composante par bras $(0,1,0)$; la classe commune conserve la provenance $[1,3]$, `same_terminal_label_arm_coalescence_count` vaut un et `distinct_terminal_label_component_coalescence_count` vaut zéro. Elle distingue donc une coalescence exacte de labels de la coalescence Gamma de labels distincts déjà exercée par la première fixture 6.9.

### 7.7 Transition Gamma ouverte--fermée bornée 6.10

Le jalon 6.10 appelle `build_exact_gamma_equal_level_transition` puis son vérificateur frais avec le nuage, l'ordre, le niveau exact et le budget fiable. Le test exige l'identité du catalogue strict embarqué avec 6.8, les catalogues exhaustifs $\beta=a$, la coupe fermée $\beta\leq a$, la projection de chaque composante stricte et la partition canonique des changements en groupes. Chaque incidence d'une coface égale doit porter exactement l'un des deux témoins exclusifs : indice de composante stricte ou facette nouvellement active.

Les fixtures positives couvrent : une facette égale isolée donnant $q=0$; une coface redondante dans une seule composante stricte donnant $q=1$; une facette nouvelle et une coface égale réunissant deux composantes strictes donnant $q=2$; deux cofaces égales chevauchantes qui doivent former un unique groupe $q=5$, jamais deux contractions séquentielles; un niveau sans aucune égalité, qui conserve la coupe et n'émet aucun groupe; et un nuage à dix points qui porte simultanément trois groupes déconnectés $q=0$, $q=1$ et $q=2$. Les catégories restent diagnostiques et ne sont jamais comparées à des racines publiques.

Le bord colinéaire $n=11$, $k=10$, $a=25$ exige deux facettes strictes de niveau $81/4$, neuf facettes égales, une coface égale, onze incidences et un groupe $q=2$. Son témoin de suppression choisit le premier maximiseur lexicographique $[0,1,2,3,4,5,6,7,8,10]$, omet le point neuf et vérifie la distance carrée 16. Les plafonds $n=14$ sont exercés sans géométrie par un budget nul : $(3003,3432,20592)$ à $k=6$ et $(3432,3003,21021)$ à $k=7$; $n=15$ est rejeté.

Chacune des trois dimensions du préflight est rendue juste insuffisante et doit produire zéro catalogue égal, zéro composante fermée et zéro groupe. Les falsifications modifient budget et entrées externes, coupe stricte embarquée, facette et coface égales, témoin taille onze, incidence tokenisée, composante fermée, projection stricte--fermée, groupe, fait, compteur, décision, portée et identité d'un nuage jumeau. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_gamma_transition` ferment la validation hôte.

Le CTest `morsehgp3d.hierarchy_gamma_transition_differential` reconstruit indépendamment la filtration avec la géométrie Python `Fraction`, appelle séparément les coupes ouverte et fermée de l'oracle et dérive sans données intermédiaires C++ les catalogues stricts et égaux, l'exclusivité des jetons d'incidence, la projection stricte--fermée et les groupes touchés. Les six cas sont $q=0$, $q=1$, $q=2$, deux cofaces chevauchantes donnant $q=5$, trois groupes déconnectés $q=1$, $q=0$, $q=2$, puis le bord $n=11$, $k=10$. Le dump par défaut du différentiel 6.8 reste inchangé; le mode opt-in `--transitions` ne partage ni miniball, ni DSU, ni réduction avec l'oracle. Aucun CUDA, G4, benchmark, statut public ou revendication de scalabilité n'en découle.

### 7.8 Superposition fournie événements--groupes Gamma 6.11

Le jalon 6.11 appelle `build_exact_supplied_critical_event_gamma_overlay` avec le nuage, une liste de shells et de budgets de chaîne, un budget global événements--bras et le budget Gamma fiable. Le vérificateur frais doit reconstruire indépendamment chaque certificat 6.9 embarqué, puis le certificat exhaustif 6.10 unique. Il exige que le couple $(k,a)$ ne soit publié qu'après accord de tous les événements complets, que les cœurs stricts 6.9 et 6.10 coïncident et que chaque coface critique fournie soit présente au niveau exact.

Pour chaque événement $S=I\cup U$, les tests partitionnent explicitement ses $k+1$ suppressions : chaque $S\setminus\left\lbrace u\right\rbrace$ pour $u\in U$ doit porter le jeton de composante stricte déjà attribué au bras 6.9; chaque $S\setminus\left\lbrace i\right\rbrace$ pour $i\in I$ doit être une facette nouvellement active 6.10. La projection conserve l'indice canonique de l'événement, sa coface égale, son groupe et sa composante fermée. Les overlays doivent être alignés structurellement un à un sur tous les groupes 6.10, avec indices canoniques triés et uniques, une référence exactement par événement et la liste exhaustive relative à 6.10 des cofaces égales sans provenance fournie.

Les fixtures positives comprennent : deux événements fournis, dans un ordre permuté, qui documentent le même groupe simultané $q=5$; un événement redondant $q=1$; un événement avec une suppression intérieure nouvellement active; un groupe contenant une seconde coface égale non fournie et un autre groupe sans événement fourni; deux événements canoniques projetés dans deux groupes distincts; et le bord colinéaire $k=10$, $n=11$, $a=25$ avec deux bras stricts, neuf facettes nouvelles et onze suppressions réconciliées. Le test distingue ainsi présence existentielle de provenance et exhaustivité du catalogue, qui n'est jamais revendiquée.

Les branches fail-closed couvrent séparément les budgets événement et total de bras insuffisants avant géométrie, la famille 6.9 incomplète, le budget Gamma insuffisant, des niveaux exacts différents, des ordres différents, les shells dupliqués et les plafonds invalides. Les falsifications modifient budgets stockés et externes, requêtes canoniques, classifications 6.9, couple commun, transition 6.10, indice et incidences d'une projection, relations structurelles et résidu de provenance d'un groupe, faits, compteurs, décision et portée. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_critical_event_gamma_overlay` ferment uniquement la portée `bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only`; aucun différentiel indépendant, CUDA, G4, benchmark, catalogue critique complet, statut public ou revendication de scalabilité n'en découle.

### 7.9 Catalogue critique exhaustif borné 6.12

Le jalon 6.12 appelle `build_exact_critical_catalog` avec le nuage canonique, $K_{\max}$ et les deux budgets fiables. Le test exige un préflight atomique de $C(n)=\sum_{j=1}^{\min(4,n)}\binom{n}{j}$ supports et $nC(n)$ classifications ponctuelles. Un budget juste insuffisant dans l'une ou l'autre dimension doit publier les besoins 1470/20580 à $n=14$, mais zéro support analysé, zéro partition globale, zéro événement, zéro dégénérescence et zéro lot.

Les fixtures positives ferment : deux points avec trois supports acceptés; un triangle aigu avec sept événements; un triangle obtus avec six événements et un circumcentre extérieur; un triangle rectangle avec cinq événements, une réduction de frontière et une égalité extra-shell pertinente; le miroir à quatre points avec deux selles distinctes d'ordre deux au niveau $169/36$; et la ligne $0,\ldots,10$, dont les 561 candidats se répartissent en 66 supports minimaux acceptés et 495 supports dépendants. Son diamètre extrême possède rang fermé onze, selle d'ordre dix au niveau 25 et aucune naissance dans la fenêtre.

La clé canonique complète est vérifiée comme $(a,r,I,S,U,c)$, puis falsifiée par permutation et réindexation. Les lots $(k,a)$, les classifications, les compteurs, les budgets et les dégénérescences sont mutés séparément avant rejeu frais. Une ligne générique $0,1,3,10$ avec $K_{\max}=1$ exerce `minimal_support_above_rank_window` et la somme des huit issues, dont `not_classified=0`.

Trois régressions protègent `RelevantGP`. Premièrement, le support diamétral de $(-1,0),(0,1),(1,0)$ conserve un rang de pertinence deux malgré un shell observé de rang trois. Deuxièmement, $(-2,0),(0,0),(0,2),(2,0)$ possède une égalité horizontale hors fenêtre mais aussi deux égalités latérales pertinentes : conclure globalement depuis la première seule est faux. Troisièmement, remplacer le point central par $(0,1/2)$ conserve l'égalité hors fenêtre sans obstruction pertinente. La sphère de niveau 25 portée par $(-5,0),(0,-5),(0,5),(3,-4),(3,4)$ agrège en plus une paire et des triangles minimaux dans la même dégénérescence; le test vérifie l'association positionnelle support--indice après canonisation.

Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_critical_catalog` ferment la base `exhaustive_exact_supports_up_to_four_global_closed_ball_critical_catalog_h0_batches_v1` sous la portée `bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only`. Aucun raccord aux bras ou à Gamma, CUDA, G4, benchmark, racine, fusion, attache, forêt ou `public_status` n'en découle.

### 7.10 Lot Gamma réduit local borné 6.13

Le jalon 6.13 appelle `build_exact_reduced_gamma_batch` puis `verify_exact_reduced_gamma_batch` avec le nuage canonique, l'ordre, le niveau exact et les trois budgets fiables de 6.10. Le domaine positif est $2\leq k<n\leq14$ avec $k\leq10$; $k=1$ et $k=n$ doivent être rejetés explicitement. Chacune des trois dimensions du préflight Gamma est rendue juste insuffisante et doit conserver uniquement le sous-certificat de préflight, avec zéro classification stricte, zéro groupe et zéro delta partiel.

Le test recalcule, pour chaque composante de $\Gamma_k^{<a}$, son nombre de facettes et ses incidences aux cofaces strictes. Il exige l'équivalence entre non-trivialité et incidence, puis attribue une racine réduite antérieure exactement aux composantes non triviales. Pour chaque groupe 6.10 avec coface égale, le nombre $q_R$ de telles racines impose `birth` si $q_R=0$, `continuation` si $q_R=1$ et `multifusion` si $q_R\geq2$; les composantes strictes isolées sont absorbées sans devenir parents. Une facette égale seule doit produire `deferred_isolated_facet`, sans delta, et un niveau sans égalité ne doit produire aucun groupe.

Les deltas sont comparés comme différences exactes d'ensembles : facettes de la composante fermée moins union des facettes des parents, puis points couverts moins union des points des parents. Le nuage $A=(0,0)$, $B=(4,0)$, $C=(1,3)$, $z=(1,1)$ avec $n=4$, $k=2$ et $a=5$ exerce `fully_redundant=true` : une racine stricte unique couvre déjà les six facettes via trois cofaces strictes de remplacement, seule la coface $ABC$ est égale, le groupe est une continuation et les deux différences sont vides. Le test doit conserver ce groupe; `fully_redundant` ne qualifie que le delta de couverture et ne peut supprimer aucune transition topologique, notamment une multifusion.

Les autres fixtures positives couvrent le triangle $q_{\Gamma}=2$ et le miroir simultané $q_{\Gamma}=5$, tous deux classés comme naissances parce que leurs composantes strictes sont isolées, puis E5 aux quatre niveaux exacts. E5 donne une naissance au niveau $25/16$, une seconde au niveau $1105/242$, une multifusion binaire au niveau $13/2$ et une continuation au niveau $17/2$; les deux derniers groupes ajoutent une facette mais aucun point. La ligne $0,\ldots,10$ à $k=10$, $n=11$, $a=25$ exerce la borne positive : les deux facettes strictes isolées et les neuf facettes égales forment une naissance réduite couvrant les onze points. Les falsifications modifient séparément budget et entrées externes, transition 6.10 embarquée, classification stricte, groupe et delta, faits, compteurs, décision et portée; chaque mutation doit faire échouer le rejeu frais.

Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_reduced_gamma_batch` ferment la base `exact_bounded_exhaustive_gamma_strict_nontrivial_component_reduction_and_equal_level_batch_semantics_v1` sous la portée `bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only`. Aucun différentiel indépendant, raccord exhaustif du catalogue 6.12, identifiant persistant, attache verticale, M.1, DAG, pointer-jumping, plateau, forêt, CUDA, G4, benchmark, scalabilité ou `public_status` n'en découle.

### 7.11 Historique Gamma réduit persistant d'un ordre 6.14

Le jalon 6.14 appelle `build_exact_persistent_reduced_gamma_order_history` puis son vérificateur frais pour $2\leq k\leq\min(10,n)$ et $2\leq n\leq14$. Pour $k<n$, le test recalcule le diamètre carré exact $D^2$, exige la coupe haute $2D^2$ et vérifie qu'elle contient les $F=\binom{n}{k}$ facettes et les $C=\binom{n}{k+1}$ cofaces. Les niveaux d'activation doivent être exactement l'union triée et dédupliquée des niveaux de ces deux catalogues. Les compteurs logiques ferment la coupe haute plus un lot par niveau, sans prétendre mesurer les rejeux internes réels.

Chaque dimension du préflight est rendue juste insuffisante séparément : budget Gamma des facettes, cofaces et unions, niveaux, trois travaux logiques, nœuds, références d'enfants, références de racines antérieures, groupes, facettes nouvellement actives, cofaces égales, facettes de delta et références de points. L'échec doit publier les seuls comptes dérivés, sans diamètre, coupe haute, niveau, lot, groupe, nœud, delta, racine ni compteur géométrique. Les plafonds statiques sont comparés aux maxima combinatoires $F,C\leq3432$, $U\leq21021$, $F+C\leq6435$, $(F+C+1)F\leq22088352$, $(F+C+1)C\leq22088352$, $(F+C+1)U\leq135291156$ et $kF\leq24024$.

À chaque lot, le test fige les racines antérieures, reconstruit l'union de leurs facettes, ajoute le delta et exige l'égalité exacte avec la composante fermée du rejeu 6.13. Les points couverts sont recalculés depuis les facettes; les points de delta doivent être exactement la différence avec les couvertures antérieures. Aucun état, compteur ou ensemble global ne doit être engagé avant la résolution complète du lot. Toutes les facettes nouvellement actives et toutes les cofaces égales apparaissent exactement une fois dans le journal; toutes les facettes apparaissent exactement une fois dans les deltas. Un groupe différé n'a aucun effet persistant, une continuation conserve son unique identifiant sans nœud, et naissance ou multifusion créent exactement un nœud. Les enfants d'une multifusion précèdent leur parent dense et ne sont consommés qu'une fois.

La fixture terminale $n=k=2$ conserve `exhaustive_facet_count=1` mais toutes les capacités historiques à zéro, aucune géométrie et une décision complète vide. Elle protège la contradiction selon laquelle la somme normale des facettes ajoutées ne vaut pas $F$ au terminal et interdit tout calcul de $C-1$. Le triangle simple ferme une naissance et la racine finale. Deux triangles translatés ferment deux naissances simultanées au niveau quatre sur le même snapshot, puis une multifusion ultérieure sans binarisation artificielle.

La fixture symétrique $A=(0,0)$, $E=(1/4,2)$, $D=(2,-1)$, $C=(2,3)$, $F=(15/4,2)$, $B=(4,0)$ ferme deux naissances simultanées au niveau $193/64$, une troisième au niveau $49/16$, puis une unique multifusion ternaire au niveau $13/4$. Son nœud trois doit avoir exactement les enfants zéro, un et deux; les deux cofaces égales sont conservées dans le même groupe et aucun nœud binaire intermédiaire n'est permis.

La fixture silencieuse $D=(0,0,1)$, $A=(0,0,7)$, $B=(0,9,6)$, $C=(1,4,0)$, $E=(4,1,2)$ à $k=2$ exige une naissance au niveau $162/25$, une continuation au niveau $189/17$, puis au niveau $33/2$ une continuation portée par les cofaces non critiques $DAC$ et $ACE$ qui ajoute seulement la facette $AC$, sans point ni nœud. Le nuage à quatre points de 6.13 conserve au niveau cinq une continuation avec delta présent mais vide et incrémente le compteur `fully_redundant`.

E5 ferme l'historique discriminant complet. Ses douze niveaux sont $1,5/4,25/16,5/2,13/4,17/4,1105/242,13/2,17/2,9,85/9,10$. Le journal contient treize groupes : six différés, deux naissances, quatre continuations et une multifusion; les trois nœuds ont les identifiants denses zéro, un et deux, le dernier possède les deux premiers comme enfants, et la racine finale est deux. Les sept deltas ajoutent exactement les dix facettes, six références ponctuelles et conservent le delta entièrement redondant du niveau $85/9$. Les lots simultanés ne peuvent lire aucune racine créée dans leur propre niveau.

Les falsifications modifient séparément le budget, les entrées, le diamètre, la coupe haute, un niveau, une métadonnée de lot, un groupe différé, chaque type d'effet persistant, un enfant, un identifiant dense, un delta vide, une activation de facette, une coface égale, une racine finale, chaque fait, compteur, décision et portée. Le vérificateur doit reconstruire chaque lot 6.13 transitoirement sans jamais indexer un payload observé avant d'en avoir vérifié les tailles. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_reduced_gamma_history` ferment la base `exact_bounded_exhaustive_gamma_all_exact_levels_persistent_reduced_root_genealogy_v1` sous la portée `bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only`. Aucun raccord catalogue--Gamma, identifiant public, stockage durable, attache verticale, M.1, DAG global, pointer-jumping, plateau, CUDA, G4, benchmark, scalabilité ou `public_status` n'en découle.

### 7.12 Coupe arbitraire du journal Gamma réduit 6.15

Le jalon 6.15 appelle `build_exact_reduced_gamma_cut` depuis un historique 6.14 déjà construit et séparément vérifié. Pour chaque niveau critique $a_i$, la coupe `strict_open` doit sélectionner exactement les lots d'indice strictement inférieur par `lower_bound`, tandis que `closed` inclut aussi le lot $i$ par `upper_bound`. Les requêtes sous le premier niveau, entre deux niveaux et au-dessus du dernier ferment respectivement le préfixe vide, l'état postérieur commun aux deux frontières et l'état final. Le curseur doit avancer même lorsqu'un lot ne contient qu'une facette différée ou un delta entièrement redondant.

L'audit global valide les caps imbriqués avant tout scan, la croissance stricte des niveaux, les intervalles, labels, compteurs et la forêt scalaire complète; les tests exigent que ses compteurs restent distincts du rejeu du préfixe, y compris sous budget nul. Le dry-preflight du préfixe recalcule ensuite lots, groupes, nœuds, références antérieures et d'enfants, activations, deltas, pic de racines, sorties, travaux de reconstruction et incidences. Les identités $R_p=Q_p-A_p$ et $H_p=N_p-A_p$ sont obligatoires. La capacité de points de sortie utilise la borne $\min(nA_p,kD_{F,p})$ avant rejeu; les tailles réelles sont comparées ensuite. Chacune des seize dimensions budgétaires, dont les deux travaux d'incidence, passe à sa valeur exacte et échoue juste en dessous sans racine partielle. Un budget supérieur à un cap et une valeur de frontière enum invalide sont rejetés.

Les fixtures discriminantes couvrent le terminal $n=k=2$ sous plusieurs seuils, le triangle juste avant et juste après le niveau quatre, E5 aux niveaux $13/2$, $17/2$, $85/9$ et entre niveaux, la continuation silencieuse $33/2$, la continuation entièrement redondante et la multifusion ternaire $13/4$ résolue en un seul lot. Les états comparent identifiants locaux, familles complètes de facettes et points couverts; les points peuvent se recouvrir entre racines, seules les facettes les partitionnent.

Le vérificateur reçoit séparément journal, seuil, frontière et budget, reconstruit son attendu sans jamais lire le curseur, les décisions ou les faits de la coupe observée pour choisir une branche, puis falsifie chaque couche du résultat. Les mutations structurelles bornées du journal couvrent niveaux non croissants, intervalles hors borne, cardinalité imbriquée, identifiants de nœuds futurs, ordre de représentants, comptes de composantes, enfant invalide et coface affectée au mauvais groupe. Le rejeu exige que le représentant soit la première facette résultante, que toute nouvelle facette appartienne au delta et que chaque suppression d'une coface appartienne à la racine du groupe. Ce test ne revendique aucune recertification géométrique de la source : un journal cohérent forgé reste explicitement hors du certificat relatif, et seule une taille hostile de `BigInt` est exclue du modèle mémoire borné.

Le différentiel Python `Fraction` construit indépendamment la filtration Gamma et sa forêt `hgp_reduced`, puis compare facettes et points couverts aux deux frontières sur tous les niveaux critiques E5, leurs milieux et les deux extrêmes. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_reduced_gamma_cut` ferment la base `exact_certified_persistent_reduced_gamma_journal_prefix_cut_replay_v1` sous la portée `bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only`. Aucun raccord catalogue--Gamma, identifiant public, stockage durable, attache verticale, M.1, DAG global, forêt multi-ordre, CUDA, G4, benchmark, scalabilité ou `public_status` n'en découle.

### 7.13 Raccord exhaustif catalogue--histoire Gamma réduite 6.16

Le jalon 6.16 appelle `build_exact_critical_catalog_reduced_gamma_overlay` avec le nuage, un ordre $2\leq k<n\leq14$ et un budget composite. Le test exige que les six capacités propres à l'overlay soient dérivées de $(n,k)$ et vérifiées avant la construction du catalogue ou de l'histoire. Chacune passe à sa valeur requise et échoue juste en dessous sans source ni payload; les budgets 6.12 et 6.14 insuffisants produisent ensuite leurs décisions distinctes sans projection partielle. Un cap dépassé et les ordres un, terminal ou supérieurs à dix sont rejetés.

La fixture triangle générique vérifie qu'une naissance de rang deux rejoint une facette différée et qu'une selle de rang trois peut créer une naissance réduite. Le triangle rectangle produit une extra-shell pertinente : le catalogue frais est conservé comme diagnostic, mais l'histoire, les projections, les slots et les groupes restent absents. E5 ferme les dix rôles utiles de l'ordre deux : six naissances vers les six facettes différées et quatre selles vers deux naissances réduites, une multifusion et une continuation. Les quatre facettes et six cofaces restantes sont des résidus exacts; la coface du groupe entièrement redondant au niveau $85/9$ reste présente sans provenance acceptée. Une fixture ternaire exige que deux selles simultanées rejoignent des slots distincts du même groupe multifusion sans être séquentialisées.

Pour chaque résultat complet, les tests reconstruisent les labels depuis les indices des sources stockées et comparent le couple exact `(squared_level, closed_point_ids)`. Les projections sont canoniques par indice d'événement, les slots couvrent exactement les $F$ facettes et $C$ cofaces de l'histoire, les overlays couvrent tous les groupes, et chaque slot possède au plus une projection. Les naissances sont en bijection avec les slots différés; une selle est toujours non différée, mais les tests obtiennent son `kind` exclusivement depuis le groupe 6.14 référencé. Les comptes ferment $P_b+R_f=F$ et $P_s+R_c=C$ sans qualifier les résidus de silencieux ou non critiques.

Le vérificateur reconstruit les deux sources et toute la couche d'indices depuis le nuage, l'ordre et les budgets. Les falsifications mutent séparément la source catalogue, la source historique, un rôle, un indice de lot, un slot, sa projection optionnelle, une plage de groupe, une référence de groupe, les faits, les compteurs, la décision et la portée; aucun champ observé ne doit choisir le chemin de rejeu. Les builds Release stricts GCC et Clang et le CTest `morsehgp3d.hierarchy_critical_catalog_reduced_gamma_overlay` ferment la base `exact_critical_closed_label_h0_references_reconciled_with_exhaustive_persistent_reduced_gamma_equality_slots_v1` sous la portée `bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only`. Aucun identifiant durable ou public, stockage, attache verticale, M.1, DAG global, forêt multi-ordre, CUDA, G4, benchmark, scalabilité ou `public_status` n'en découle.

### 7.14 Raccord exhaustif de tous les bras catalogués aux composantes Gamma strictes 6.17

Le jalon 6.17 appelle `build_exact_critical_catalog_arm_gamma_overlay` puis son vérificateur frais avec le nuage, un ordre $2\leq k<n\leq14$, $k\leq10$, et un budget composite. Il reconstruit 6.12 avec $K_{\max}=k$, sélectionne chaque selle d'ordre $k$ et exige une famille 6.7 complète par selle avant tout calcul Gamma. Pour chaque lot H0 contenant au moins une telle selle, il construit et vérifie exactement un lot 6.13 transitoire; aucun résultat 6.13 complet ne doit persister dans le payload compact.

Le préflight propre au raccord calcule $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$, $F=\binom{n}{k}$ et la limite de chaîne $L\leq4096$. Ses sept caps sont, dans l'ordre de l'API : $E\leq1456$ selles, $A\leq5824$ bras, $E\leq1456$ lots, $A\leq5824$ composantes ciblées, $EF\leq4996992$ références de facettes, $k(EF+A)=kE(F+4)\leq35025536$ références ponctuelles incluant le représentant canonique de chaque cible et $AL\leq23855104$ segments engagés. Chacune des sept capacités passe à sa valeur requise et échoue juste en dessous avant toute géométrie subordonnée. Les budgets imbriqués 6.12, 6.7 et 6.13 sont testés séparément, ainsi que leurs plafonds; une extra-shell pertinente, une seule famille incomplète ou un seul préflight Gamma insuffisant doit laisser vides toutes les composantes-cibles et toutes les incidences `arm_targets`, sans overlay partiel.

Pour chaque bras retirant $u\in U$ d'une selle $S=I\cup U$, le test reconstruit la facette initiale $F_u=S\setminus\lbrace u\rbrace$ depuis l'événement catalogué et la facette terminale $T_u$ depuis le chemin 6.7. Deux lookups indépendants dans la coupe stricte 6.13 doivent retrouver la même unique composante `full_pi0`; cette composante doit en plus figurer dans l'unique groupe non différé contenant la coface $S$. Vérifier seulement que les deux facettes sont absorbées par le même groupe est insuffisant, car un groupe simultané peut réunir plusieurs composantes strictes. Chaque cible conserve toute la famille canonique de facettes et sa couverture ponctuelle. `prior_nontrivial_reduced_root` ou `omitted_isolated_facet`, puis `birth`, `continuation` ou `multifusion`, sont comparés comme annotations `hgp_reduced` séparées et ne peuvent jamais choisir l'indice de cible.

La fixture `mirror_simultaneous_q5` contient deux selles simultanées dans un seul lot, six bras, cinq composantes-cibles isolées et exactement une cible partagée entre les deux événements; les deux familles héritent de l'annotation réduite `birth` sans l'inférer. La fixture `shared_terminal_interior` ferme trois familles dans trois lots et sept bras. Pour la selle de niveau $25925/338$, les retraits un et trois partagent la même composante stricte non triviale, le retrait deux vise une facette isolée omise, et le groupe réduit est une `continuation`; les classes terminales 6.7 doivent conserver séparément leur provenance. Les budgets de chaîne zéro, un et deux distinguent famille incomplète atomique, fermeture courte et chaîne plus longue, sans compter le segment critique initial dans le budget 6.5.

Le rejeu falsifie séparément les sept capacités stockées et fiables, la source catalogue, une famille 6.7, un enregistrement de lot, son niveau ou son groupe, une composante-cible complète, son annotation réduite, une incidence initiale ou terminale, un indice `arm_target`, les faits, les compteurs, la décision, la portée et l'identité d'un nuage jumeau. Aucun budget, indice, terminal, groupe ou diagnostic observé et aucune composante observée ne peut sélectionner une branche du vérificateur frais. Les builds Release stricts GCC et Clang ferment les CTests ciblés `morsehgp3d.hierarchy_critical_catalog_arm_gamma_overlay` et `morsehgp3d.hierarchy_critical_catalog_arm_gamma_overlay_differential`; les temps hôte observés sur le code final sont respectivement 14,54 s et 5,26 s sous GCC, puis 12,05 s et 5,56 s sous Clang.

Le différentiel indépendant Python `Fraction` reconstruit le catalogue critique, la filtration Gamma stricte et la forêt réduite sur `mirror_simultaneous_q5` et `shared_terminal_interior`. Il compare exactement les treize enregistrements `arm_targets`, soit six plus sept, dérive chaque cible depuis la facette initiale du bras, compare sa famille complète de facettes et les deux annotations réduites, puis exige que la facette terminale C++ appartienne à cette même composante indépendante et possède un niveau strictement pré-lot. Il ne réimplémente volontairement pas la descente canonique 6.7 et ne prétend donc pas dériver indépendamment le label terminal; cette limite est distincte de la double appartenance Gamma effectivement vérifiée.

La base `exact_exhaustive_critical_catalog_index_one_arm_families_reconciled_with_strict_gamma_full_pi0_components_and_separate_reduced_annotations_v1` et la portée `bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only` ferment uniquement la couture bornée mono-ordre. Aucun `Attachment` public, identifiant durable ou public, certificat M.1, transaction de forêt simultanée, flèche verticale, forêt multi-ordre, plateau, CUDA, G4, benchmark, résultat de scalabilité ou `public_status` n'en découle.

### 7.15 Journal Gamma mono-ordre exhaustif et typé 6.18

Le jalon 6.18 appelle `build_exact_critical_catalog_typed_gamma_journal` puis `verify_exact_critical_catalog_typed_gamma_journal` avec le nuage, un ordre $2\leq k<n\leq14$, $k\leq10$, et un budget composite qui contient les budgets 6.16 et 6.17. Avant toute géométrie, le test exige l'identité des deux budgets de catalogue et celle du budget Gamma partagé par l'histoire 6.16 et les lots 6.17. Il diminue ensuite d'une unité chacune des neuf capacités propres : labels, selles, classes terminales, bras, cibles strictes, références ponctuelles des classes, références de selles vers classes et bras, facettes cibles et points cibles. Chacune doit produire `no_journal_preflight_budget_insufficient` avant les deux producteurs sources et sans histoire ni journal. Un cap déclaré au-dessus de son plafond est rejeté, tout comme $k=1$, $k=n$ et $n<3$.

Le triangle `q2` ferme trois groupes historiques et quatre labels : deux `catalog_birth`, un `catalog_saddle`, un `residual_newly_active_facet` et aucun `residual_equal_level_coface`. Les deux naissances gardent leur provenance H0 mais aucun enregistrement de selle; la facette résiduelle du niveau quatre reste dans le groupe de la selle sans provenance inventée. L'unique selle se factorise en deux classes, deux bras et deux cibles strictes singleton distinctes. Son rôle H0 reste `catalog_saddle` alors que l'effet réduit du groupe est `birth`. Le test compare les quatre labels, tous les indices de batch et de groupe, les compteurs exacts et l'unique histoire conservée.

La fixture miroir ferme deux selles simultanées dans le même lot H0 et le même groupe historique au niveau $169/36$, six classes, six bras et cinq cibles singleton. Les deux selles restent deux rôles distincts et partagent uniquement la cible stricte de facette `(0,3)` par deux bras. Les deux cofaces égales sans rôle H0 restent `residual_equal_level_coface`. La fixture d'ordre trois au terminal commun ferme trois selles et sept bras; au niveau $25925/338$, les retraits un et trois partagent une classe et une cible antérieure `full_pi0` à sept facettes, tandis que le retrait deux vise la facette isolée `(0,1,3)`. `prior_nontrivial_reduced_root`, `omitted_isolated_facet` et `continuation` sont comparés comme annotations séparées, jamais comme sélecteurs de cibles ou identifiants de racines.

Les échecs subordonnés sont atomiques. Un budget de slots 6.16 insuffisant bloque 6.17; un budget de cibles 6.17 insuffisant suit une provenance complète mais ne publie aucun enregistrement; la fixture transverse avec budget de chaîne nul conserve seulement le diagnostic de famille incomplète. Le rejeu frais falsifie simultanément budget, taille dérivée, décision source, histoire, sémantique de label, selle, classe terminale, bras, témoin strict, faits, compteurs, décision et portée, puis rejette séparément un nuage jumeau de même cardinal. Les deux overlays, leurs catalogues, les familles et les chaînes ne doivent jamais persister dans le résultat complet.

Le build Release strict ciblé passe sous GCC en 8,2 s, puis le CTest `morsehgp3d.hierarchy_critical_catalog_typed_gamma_journal` passe seul en 65,45 s. Sous Clang strict, le build ciblé passe en 17,94 s et le même CTest passe seul en 50,19 s. Aucun nouveau différentiel 6.18 n'est ajouté : les deux sources 6.16 et 6.17 conservent déjà leurs oracles indépendants, tandis que ce test court cible leur factorisation et le rejeu frais du journal. La base validée est `exact_fresh_catalog_h0_provenance_and_strict_full_pi0_arm_targets_reconciled_through_one_typed_single_order_reduced_gamma_journal_v1` sous la portée `bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only`. Aucun lien cible--racine, `Attachment` public, identifiant durable ou public, certificat M.1, flèche verticale, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre, CUDA, G4, benchmark, scalabilité ou `public_status` n'en découle.

### 7.16 Raccord des cibles typées aux racines locales pré-lot 6.19

Le jalon 6.19 appelle `build_exact_critical_catalog_typed_gamma_root_overlay` puis son vérificateur frais avec le nuage, un ordre $2\leq k<n\leq14$, $k\leq10$, un budget propre et un journal 6.18 externe construit sous exactement le budget imbriqué. Les dix capacités propres sont vérifiées avant la recertification de cette source : liaisons, états de racines vivants, références de facettes et de `PointId` de ces états, unités logiques de rejeu en facettes et en points, unités logiques de comparaison des familles cibles, puis unités logiques d'indexation des snapshots. Ces unités comptent les références du payload sémantique, pas chaque instruction de comparaison, recherche dans un arbre, tri ou rescan défensif du conteneur CPU.

Le test diminue d'une unité chacune des dix capacités et exige `no_root_overlay_preflight_budget_insufficient`, zéro vérification de la source, zéro rejeu de lot et aucune liaison partielle. Une couture discordante du budget 6.18 échoue au même endroit; un cap supérieur au plafond, $k=1$, $k=n$ ou $n<3$ est rejeté. Un balayage sans géométrie de tous les couples $3\leq n\leq14$, $2\leq k<n$, $k\leq10$ recalcule indépendamment les dix formules et doit retrouver exactement les maxima $5824$, $858$, $6864$, $48048$, $22084920$, $154594440$, $4996992$, $34978944$, $4996992$ et $34978944$.

Sur `q2`, les deux cibles singleton du lot de naissance réduit doivent être explicitement `omitted_isolated_singleton`, sans jamais recevoir l'identifiant de la racine créée dans ce même lot. Sur E5, la multifusion binaire de racines zéro et un vers la racine deux ne peut lier ses cibles qu'aux deux racines du snapshot pré-lot; l'échange de ces identifiants entre familles complètes est rejeté. La première continuation postérieure, au niveau $17/2$, est résiduelle et ne porte aucune cible cataloguée; le témoin positif sélectionne donc la continuation cataloguée ultérieure de niveau $9$ et exige que ses liaisons emploient la racine persistante deux, alors déjà présente dans le snapshot.

Le rejeu reconstruit chaque cible exactement une fois, vérifie l'égalité de toute famille non triviale, l'absence exhaustive de tout singleton et l'appartenance de la racine au groupe visé avant de contrôler l'annotation réduite. Les groupes d'un lot sont ensuite préparés sous les bornes $R$ et $F$, puis committés simultanément. Les diagnostics de source incomplète ou falsifiée restent sans payload. Les mutations simultanées du budget, des tailles dérivées, de la décision source, d'une liaison, des faits, des compteurs, de la décision et de la portée, ainsi qu'un nuage jumeau de même cardinal, sont rejetées par rejeu frais.

La campagne reste volontairement ciblée : les compilations Release strictes passent sous GCC et Clang, puis le seul CTest `morsehgp3d.hierarchy_critical_catalog_typed_gamma_root_overlay`, lancé simultanément sur les deux builds finaux, passe en 152,12 s sous GCC et 132,14 s sous Clang. Aucun différentiel, sanitizer, benchmark, CUDA ou G4 nouveau n'est requis pour cette couture locale. La base est `exact_fresh_typed_full_pi0_target_families_reconciled_with_frozen_pre_batch_local_reduced_gamma_roots_v1` et la portée `bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only`. Les identifiants restent locaux au journal externe recertifié; aucun `Attachment`, identifiant durable ou public, M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre, scalabilité ou `public_status` n'est revendiqué.

### 7.17 Composition événement-locale bras--cible--racine 6.20

Le jalon 6.20 appelle `build_exact_critical_catalog_typed_gamma_arm_root_composition` avec le nuage, un ordre $2\leq k<n\leq14$, $k\leq10$, le journal externe 6.18, l'overlay externe 6.19 et un budget embarquant exactement celui de cet overlay. Le test exige la validation récursive de tous les plafonds imbriqués avant le préflight propre : un cap 6.20 juste insuffisant combiné à un cap Gamma profond supérieur à son maximum doit lever `std::invalid_argument`, jamais produire un diagnostic certifié. Une seule capacité propre est dérivée, $A=4\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}\leq5824$; un balayage sans géométrie de chaque couple du domaine retrouve ce maximum et s'arrête avant tout rejeu source.

Sur `q2`, les deux bras deviennent deux candidats denses distincts, chacun lié à son singleton omis et sans racine locale. Les coutures de budgets 6.18 et 6.19 sont falsifiées séparément et doivent arrêter la composition avant la recertification. Un overlay 6.19 incomplet est reconnu comme source complète de son propre diagnostic mais ne produit aucun candidat 6.20; une liaison falsifiée est rejetée. Sur la fixture d'ordre trois au terminal commun, sept bras donnent sept candidats : les retraits un et trois partagent cible, liaison et racine réduite antérieure tout en gardant deux indices et deux clés distincts, tandis que le retrait deux reste un singleton explicitement omis.

Le rejeu frais compare le budget, les tailles dérivées, la décision source, tous les candidats, les faits, les compteurs, la décision et la portée. Une mutation composite de ces couches et un nuage jumeau de même cardinal échouent fermés. Les builds Release ciblés passent sous GCC et Clang stricts; conformément à la campagne courte, le seul CTest fonctionnel est lancé sous GCC et passe en 143,09 s. Aucun CTest Clang redondant, différentiel, sanitizer, benchmark, CUDA, G4 ou GCP n'est lancé.

La base est `exact_fresh_typed_critical_arm_target_indices_composed_with_recertified_target_root_bindings_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only`. Cette couche ne conserve aucun chemin rejouable et ne publie aucun `Attachment`, identifiant durable ou public, certificat M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre, scalabilité ou `public_status`.

### 7.18 Overlay interne rejouable bras--cible--racine 6.21

Le jalon 6.21 appelle `build_exact_critical_catalog_typed_gamma_arm_root_path_overlay` avec les trois sources externes 6.18--6.20 et un budget embarquant exactement celui de 6.20. Le test diminue séparément d'une unité chacune des cinq capacités dérivées sur un domaine sans événement : chemins, segments 6.5 engagés, segments composites, références de `PointId` des nœuds et contraintes extérieures. Chaque branche doit décider l'insuffisance avant toute recertification ou géométrie. Un cap strict-Gamma profondément imbriqué au-dessus de son plafond doit lever `std::invalid_argument` avant ce préflight.

Sur `q2`, les deux candidats 6.20 deviennent deux chemins denses distincts et rootless. Chaque record conserve le témoin analytique du germe, satisfait `nodes.size() == committed_segments.size() + 1`, certifie la couture, la stricte sous-niveauté et le terminal régulier, et recopie exactement la clé, la cible, la liaison et la disposition. Les compteurs retrouvent le catalogue unique, la famille unique, les deux appartenances initiales, les deux appartenances terminales et toutes les tailles du payload compact.

Une composition 6.20 falsifiée est rejetée avant reconstruction; une composition 6.20 certifiée mais incomplète reste un diagnostic atomique sans chemin. Le vérificateur frais rejette aussi la mutation du booléen analytique de stricte demi-ouverture d'un germe. La campagne courte compile le target sous GCC et Clang stricts, puis lance uniquement le CTest GCC `morsehgp3d.hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay`, vert en 7,25 s. Aucun terminal commun supplémentaire, nuage jumeau, différentiel, sanitizer, benchmark, CUDA, G4 ou GCP n'est lancé pour ce jalon.

La base est `exact_fresh_event_local_typed_critical_arm_strict_descent_paths_replayed_and_linked_to_full_pi0_targets_with_separate_local_reduced_dispositions_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only`. Les chemins restent internes : aucun `Attachment`, identifiant durable ou public, H5, O3, M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre, scalabilité ou `public_status` n'est revendiqué.

### 7.19 Identifiants v2 des événements et tuples durables des bras 6.22

La primitive `canonical_v2_id_from_canonical_json_unchecked` est d'abord confrontée à trois digests calculés indépendamment en Python : une projection `CriticalEvent` complète, une projection vide sous ce même domaine et une entrée vide sous le domaine `Input`. Le test exige l'hexadécimal minuscule exact, le round-trip strict et le rejet des tailles, majuscules ou types contenant `/`. Le schéma n'est pas modifié et aucun hash du domaine `Attachment` n'est produit.

Le test fonctionnel construit une chaîne source 6.18--6.21 sur `q2`, puis appelle `build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog`. Il exige un événement, deux tuples retirant exactement les points zéro et deux, le digest `f6acb8180318d9879b84d5ebfd4368ca9841b9dc767be97eeaf4b51fedf1d88c`, les références d'agrégation `[0,1]` et les compteurs exacts. Le même nuage fourni en ordre inverse doit, après canonicalisation et reconstruction complète des sources, produire un résultat bit-à-bit égal. Cette invariance ne s'étend pas à un relabelling arbitraire des `PointId`. La fixture miroir ajoute deux selles simultanées, six bras, deux digests strictement triés et trois références correctement jointes à chaque événement; elle protège la logique multi-événement sans lancer la fixture d'ordre trois plus longue.

Sur quatre points et sans source géométrique, chacune des quatre capacités propres est abaissée séparément à la borne conservative moins un : événements $9<10$, bras $39<40$, références événement--bras $39<40$ et références de projection $69<70$. Chaque branche s'arrête avant le vérificateur 6.21 et publie deux arènes vides. Chacun des quatre caps propres au-dessus de son plafond et un cap 6.21 imbriqué excessif lèvent `std::invalid_argument`. Une couture 6.21 discordante possède un diagnostic distinct avant rejeu. Une source dont le terminal certifié est falsifié produit seulement un diagnostic rejeté; les mutations d'un `event_id` observé et d'un tuple de bras sont refusées par le rejeu frais.

La campagne courte compile uniquement les deux nouvelles cibles en Release strict sous GCC et Clang, puis lance les deux CTests ciblés sous GCC; ils passent en 37,52 s cumulées. Aucun CTest Clang redondant, fixture d'ordre trois, différentiel, sanitizer, benchmark, CUDA, G4 ou GCP n'est lancé. La base est `v2_domain_separated_sha256_critical_event_keys_with_full_projection_collision_checks_and_exhaustive_single_order_arm_identity_tuple_catalog_v1` et la portée `bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only`. Aucun `attachment_id`, `Attachment`, `target_node_id`, `EqualLevelBatch`, H5, O3, M.1, forêt globale, verticalité ou `public_status` n'est revendiqué.

### 7.20 Sweep Morse--Gamma des partitions mono-ordre 6.23

Le jalon 6.23 construit d'abord, sans consulter Gamma, une généalogie locale à partir des naissances de rang demandé du catalogue critique et des familles complètes de bras des selles d'indice un. Chaque facette terminale régulière doit désigner exactement une naissance strictement antérieure. Les selles d'un même niveau sont ensuite contractées contre une photographie gelée des racines : l'ordre inverse des selles doit produire les mêmes groupes, nœuds et partitions. Une fois cette généalogie entièrement fixée, un oracle Gamma exhaustif rejoue tous les niveaux d'activation, en coupe stricte puis fermée, et exige une bijection entre les composantes Gamma et la partition des seules facettes de naissance actives. Il est donc permis qu'une facette ou une coface résiduelle de Gamma enrichisse une composante sans constituer une naissance Morse; toute composante Gamma sans naissance du catalogue ou toute fusion inexpliquée est en revanche une contradiction.

Le préflight propre couvre séparément les naissances, selles, bras, nœuds, enfants, lots, groupes, racines de groupes, références de lots et checkpoints d'oracle, avant toute géométrie subordonnée. Chaque capacité juste insuffisante doit publier un diagnostic atomique sans généalogie ni checkpoint; une capacité propre ou imbriquée hors plafond doit lever `std::invalid_argument`. Le résultat complet est rejoué depuis le nuage par un vérificateur frais, qui compare aussi les décisions subordonnées, les compteurs, les faits et les témoins de partition.

Les fixtures minimales permanentes sont `q2`, qui impose deux naissances puis une fusion malgré la facette Gamma résiduelle, la fixture miroir, qui impose deux selles simultanées dans une seule contraction gelée, la fixture d'ordre trois au terminal commun et E5, qui exerce au moins une continuation sur une unique racine. Le niveau résiduel $9$ du miroir doit produire un checkpoint sans lot Morse, avec les mêmes nombres de composantes avant et après et deux bijections vraies. Les tests falsifient aussi une naissance, l'ordre des lots observés, un checkpoint, un fait, la décision finale et un nuage jumeau de même cardinal; le constructeur compare séparément le quotient canonique au rejeu interne des selles en ordre inverse.

La cible `morsehgp3d.hierarchy_morse_gamma_partition_sweep` compile en Release strict sous GCC et Clang. Le dernier CTest fonctionnel ciblé, sous GCC, passe en 24,37 s avec les dix capacités juste insuffisantes, les caps local et imbriqué excessifs, les mutations et le rejeu frais. Aucun CTest Clang redondant, différentiel, sanitizer, benchmark, CUDA, G4 ou GCP n'est lancé. Ce jalon ne publie aucun identifiant durable, `Attachment`, `target_node_id`, transaction de forêt, certificat M.1, morphisme vertical, statut `exact` ou `public_status`.

## 8. Ancrage fondamental à $k=1$

À l'ordre $1$, la hiérarchie doit coïncider avec le single linkage porté par un arbre couvrant euclidien minimal. Une arête EMST $(u,v)$ de longueur carrée $\lVert u-v\rVert^2$ produit dans $L_1$ le niveau de fusion $\lVert u-v\rVert^2/4$ ; les tests vérifient explicitement ce facteur au lieu de comparer deux conventions implicites.

Pour $n\leq14$, l'oracle utilise Kruskal sur le graphe euclidien complet. Pour les tailles intermédiaires, il utilise une implémentation CPU exacte indépendante ; une implémentation GPU d'EMST publiée peut servir de second différentiel, jamais d'unique oracle.

En l'absence d'ex æquo, on compare les arêtes de l'EMST et leurs poids. En présence d'ex æquo, plusieurs arbres sont valides : on compare plutôt la partition induite à tout seuil distinct, le poids total exact et la multifusion canonique. Les tests couvrent :

- $n=1$, forêt réduite triviale et aucun raffinement ;
- $K_{\max}=1$ avec $n\geq2$, profondeur $m_{\star}=0$, minima de rang un et seules selles de rang deux ;
- points aléatoires volumiques ;
- points collinéaires et coplanaires ;
- grilles et shells avec nombreux poids égaux ;
- deux amas reliés par une arête unique ;
- changements d'échelle et grands offsets ;
- exécution mono-lot et streaming.

Un désaccord à $k=1$ bloque immédiatement tout benchmark de production.

Le profil de travail séparé de la voie external-1NN exacte mesure par ronde les évaluations exactes de graines, les bornes point--AABB, les distances point--point, les visites de nœuds et les pics par source. Son checker exige l'identité entre bornes AABB et visites, la partition exacte des visites entre élagages stricts, expansions, distances et réutilisation de graine, les sommes inter-rondes, la réduction au moins par moitié des composantes et zéro record candidat persistant. Une matrice G4 ne change aucun statut de phase : son artefact reste `benchmark_only`, sans revendication de qualification, de scalabilité, de résultat scientifique ou de réduction hiérarchique.

La régression hôte `morton_overlap` exécute $s=4,8,16$ avec $g=s^2$, $n=4g+2$ et $W=1$. Elle exige l'ordre source canonique, un groupe central de collision Morton de code $7$ et de taille $n-2$, l'accord complet avec l'ancre Borůvka indépendante à $s=4$, puis le minimum analytique $d^2$ de chacune des $2g$ requêtes. Les expansions internes et les visites/bornes doivent être au moins $(512,1536)$ pour $n=66$, $(8192,24576)$ pour $n=258$ et $(131072,393216)$ pour $n=1026$. Les distances de graines, bornes AABB et distances ponctuelles doivent aussi fermer séparément la borne $X_r\leq3n^2$. La preuve couvre tout $W\geq1$ dans le modèle dyadique à précision paramétrique; ces trois tailles sont les témoins binary64 permanents, pas un substitut à l'argument asymptotique.

Le resolver self-dual reçoit les mêmes labels et graines, mais son audit doit en plus fermer exactement les $\binom{n}{2}$ paires non ordonnées entre prunes uniformes, prunes AABB strictes et feuilles évaluées. Le recalcul final doit retrouver chaque maximum d'incumbents dynamique; une égalité de borne descend. La chaîne $8\to4\to2\to1$, le carré à ex æquo et `morton_overlap` doivent donner les mêmes minima par point et par composante que les oracles existants. L'audit recalcule la hauteur $H$ du LBVH, exige `certified_depth_first_frontier_bound=2H+1`, refuse toute insertion qui dépasserait cette pile, recalcule `certified_node_pair_visit_bound=n(n+1)-1` et rejette tout compteur supérieur. Sur `morton_overlap` avec $W=1$, les tailles 66, 258 et 1026 imposent les plafonds de fixture $48g$ visites, $22g$ expansions, $7g$ distances, $3g$ diminutions strictes et $3g$ mises à jour d'ancêtres; leurs pics de pile observés lors du jalon sont 16, 20 et 24, chacun inférieur à $2H+1$. Ces seuils de fixture protègent la suppression du bloc construit; seule la borne quadratique des visites est générale et elle ne constitue pas une preuve de scalabilité.

Le resolver direct par composante reçoit la même partition figée, mais son type d'audit ne contient aucun champ de minimum ponctuel. Le test doit vérifier exactement $n$ graines de points, $n$ offres supplémentaires aux composantes cibles sans nouvelle distance exacte, $c$ incumbents initiaux de composantes, $2n-1$ valeurs de l'enveloppe max figée, $c$ minima de sortie, la couverture des $\binom{n}{2}$ paires et les deux bornes DFS. Chaque paire de feuilles externe peut mettre à jour zéro, une ou deux composantes; le nombre de mises à jour est borné par deux fois le nombre de distances évaluées. La chaîne $8\to4\to2\to1$ reconstruit l'enveloppe après chaque contraction et contracte depuis les minima directs; le carré ferme les ex æquo. La fixture 3D canonique `[(0,53,45),(1,48,41),(2,21,4),(3,32,46),(4,15,14)]`, avec cinq singletons et $W=1$, doit donner à la composante 3 l'arête $(1,3)$ : elle échoue si un nœud remonte le minimum des cutoffs ou si seule la première extrémité est relâchée. La fixture entière `[(0,37,4),(1,1,51),(2,3,48),(3,7,39)]`, avec labels `[0,0,2,2]` et $W=1$, doit verrouiller l'ordre Morton `[1,0,2,3]`, compter quatre offres cibles, exactement une mise à jour stricte et remplacer le cutoff source-only $2134$ de la composante 2 par l'arête $(1,2)$ de carré $14$; elle échoue si la graine n'est réduite que pour sa source. La régression dyadique `[(0,0),(9/16,9/16),(7/16,7/16),(1,0),(0,1)]` verrouille ensuite le mapping canonique des indices source `[0,4,2,1,3]` et l'ordre Morton `[0,2,1,4,3]` : les labels d'ordre d'entrée non remappés `[0,1,0,0,0]` produisent zéro mise à jour cible, tandis que les labels canoniques `[0,0,0,3,0]` produisent deux mises à jour sous $\kappa$, dont une diminution stricte, et l'arête $(2,3)$ de carré $1/32$. Enfin `morton_overlap` à $n=66$ doit provoquer au moins une diminution stricte au-delà des graines et rejoindre l'ancre Borůvka indépendante. Ce test de ronde qualifie seulement le chemin hôte local; CUDA et la réduction hiérarchique restent distincts.

La comparaison discriminante doit exécuter `frozen_initial` et `sparse_witness_path_monotone` depuis exactement le même nuage canonique, la même partition, la même politique et le même audit de graines. Les minima de composantes et l'audit Morton doivent être identiques. Pour chaque feuille $q$, le certificat sparse doit fermer l'invariant $D_{\ell(q)}(t)\leq\widehat{D}_q(t)\leq D_{\ell(q)}(0)$; chaque proxy interne doit être exactement le maximum de ses deux enfants et rester inférieur ou égal à son homologue figé. Chaque baisse stricte doit produire exactement une mise à jour de feuille témoin, et le total des mises à jour d'ancêtres doit être au plus le nombre de mises à jour de feuilles multiplié par la profondeur maximale du LBVH; le mode figé doit garder ces deux compteurs nuls. Le parcours déterministe doit enfin vérifier séparément que visites, expansions, évaluations exactes AABB--AABB et distances exactes du mode sparse sont inférieures ou égales à celles du mode figé. La fixture `uniform`, $n=12$, $W=2$, graine un, suit $12\to3\to1$ et verrouille les totaux figés `(130,57,17)` contre sparse `(124,54,13)` pour visites, expansions et distances, ainsi que quatre mises à jour de feuilles et quatre d'ancêtres; ces valeurs n'ont qu'un rôle de régression hôte. Une falsification du mode, d'un proxy feuille, d'un maximum interne, d'un des deux certificats ou d'un compteur doit échouer fermée. Ce contrat n'établit aucune dominance sur le resolver dynamique par point, le temps total ou l'asymptotique, et ne qualifie ni CUDA, ni G4, ni hiérarchie, ni statut public.

Le mode `exact_current_maximal_uniform_roots` doit certifier que les racines uniformes maximales ont des intervalles Morton disjoints dont l'union contient exactement les $n$ feuilles, que chaque composante possède au moins une racine et que leur nombre total $M$ satisfait $c\leq M\leq n$. Les offsets et indices CSR doivent contenir chaque racine exactement une fois sous son slot de composante. L'accesseur de cutoff doit ignorer toute valeur historique stockée sous un nœud uniforme et lire l'incumbent courant de son tag; tout nœud mixte doit rester le maximum exact de ses deux enfants effectifs. Une baisse stricte de $C$ doit mettre à jour exactement les racines de son intervalle CSR; les recomputations mixtes doivent être au plus les mises à jour de racines multipliées par $H$, et les mises à jour mixtes au plus les recomputations. Une fixture où une composante possède plusieurs racines doit échouer si une posting, une feuille de couverture, un accès live, un maximum mixte ou un compteur est falsifié. Les minima et contractions doivent rejoindre l'ancre et les trois autres resolvers.

Le mode `exact_current_deduplicated_mixed_ancestors` doit réutiliser exactement la même partition CSR, les mêmes cutoffs live et le même accès effectif que `exact_current_maximal_uniform_roots`. Pour chaque baisse stricte d'une composante possédant $r$ racines uniformes maximales, le test reconstruit indépendamment l'union $A$ de leurs ancêtres mixtes et exige exactement $\lvert A\rvert+r-1$ découvertes, $r-1$ doublons et $\lvert A\rvert$ ancêtres distincts. L'égalité agrégée doit être `duplicate_mixed_ancestor_discovery_count = uniform_root_update_count - strict_component_cutoff_decrease_count`. Le parcours bottom-up doit traiter chaque nœud marqué exactement une fois, recomputer au plus $\lvert A\rvert$ nœuds et effectuer au plus autant de mises à jour; son état scratch doit être nul et réutilisable après chaque baisse. Une composante fragmentée dont plusieurs chemins partagent un long tronc mixte doit fermer ces identités, conserver les mêmes minima, les mêmes quatre compteurs de parcours et la même contraction que la baseline courante, et ne pas effectuer plus de recomputations ou de mises à jour mixtes sur cette fixture ciblée. Les falsifications séparées d'une découverte, d'un ancêtre distinct, d'un doublon, du maximum distinct, de l'ordre bottom-up ou du certificat `deduplicated_mixed_ancestor_refresh_certified` doivent échouer fermées. Ces tests établissent une borne $O(n)$ par baisse, pas une borne globale sous-quadratique ni une dominance temporelle.

Le work-profile possède en plus un smoke hôte optionnel `--compare-resolvers`, séparé du schéma v1 et du pipeline G4 existants. Son document v3 doit se projeter exactement sur le v1 après retrait de `resolver_comparison`; pour chaque ronde, il doit fournir à `dynamic`, `direct_frozen` et `direct_sparse` la même partition et la même graine recertifiée, exiger l'égalité de leurs minima exacts de composantes et de leurs contractions canoniques, puis aligner les comptes pré- et post-contraction sur ceux du v1. Le checker valide séparément les champs des trois voies, la couverture de $\binom{n}{2}$ paires, l'identité `aabb_pair_bound_evaluation_count=node_pair_visit_count`, la partition des visites, l'arité $2E+2\leq V\leq3E+1$, les bornes de frontière, de visites et de mises à jour, les sommes inter-rondes et les quatre inégalités sparse--figé. La falsification cohérente `V=2`, `E=1`, une distance et aucun prune doit rester rejetée par l'arité. Sur `uniform`, $n=12$, $W=2$, graine un, le chemin doit valoir $12\to3\to1$; les totaux `(visites,expansions,distances)` doivent être `(148,66,21)` pour le dynamique, `(130,57,17)` pour le direct figé et `(124,54,13)` pour le direct sparse, avec quatre mises à jour de feuilles témoins et quatre d'ancêtres pour ce dernier. Ces valeurs protègent une fixture courte; le test ne doit conclure ni à une dominance sur le dynamique, ni à une accélération temporelle, ni à une borne sous-quadratique, ni à une qualification CUDA/G4 ou de scalabilité.

L'option séparée `--compare-current-envelope` doit émettre `morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v4`, puis se projeter exactement sur v3 par retrait de `direct_current`; v3 doit encore se projeter sur v1. Le checker impose, par ronde et sur les totaux, que visites, expansions, bornes AABB--AABB et distances de `direct_current` soient chacune au plus celles de `dynamic`, `direct_frozen` et `direct_sparse`. Sur `uniform`, $n=12$, $W=2$, graine deux, le chemin vaut $12\to4\to1$ et les totaux `(visites,expansions,distances)` sont respectivement `(141,61,15)`, `(147,64,22)`, `(147,64,19)` et `(153,67,27)`. Pour les deux rondes, les tuples `(racines,couverture,mises à jour racines,recomputations mixtes,mises à jour mixtes,baisses strictes)` valent `(12,12,3,7,6,3)` puis `(7,12,3,7,5,2)`. Ces comptes n'établissent aucune dominance du temps ou du travail total; les chemins partagés ne sont pas dédupliqués et aucune conclusion asymptotique, CUDA/G4, hiérarchique ou publique n'est permise.

L'option hôte `--compare-deduplicated-current-envelope` émet `morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v5`. Son document ajoute seulement `direct_deduplicated_current` à chaque ronde et aux totaux v4; retirer ces champs doit restituer exactement v4, puis les projections existantes doivent restituer exactement v3 et v1. Les cinq voies consomment la même partition et le même audit Morton, et leurs minima exacts, contractions canoniques et chemins de composantes doivent coïncider. Les quatre compteurs de parcours de `direct_deduplicated_current` doivent être identiques à ceux de `direct_current`, puisque leurs enveloppes effectives et leur ordre DFS sont identiques. Le checker v5 ferme par ronde et agrégat les identités de découvertes, ancêtres distincts et doublons, les bornes de recomputation et de mise à jour, puis les projections contractuelles. Sur `uniform`, $n=12$, $W=2$, graine deux, le test verrouille les tuples `(distincts,doublons,découvertes,recomputations,mises à jour,max distinct par baisse)` à `(10,0,10,7,6,4)` puis `(7,1,8,7,5,4)`, ainsi que les mêmes totaux de 141 visites, 61 expansions et 15 distances que `direct_current`. Le doublon positif de la seconde ronde est obligatoire pour exercer la collision; l'absence de réduction stricte des sept recomputations par ronde interdit d'interpréter cette fixture comme un gain de travail ou de temps. Le schéma v5 reste hôte et `benchmark_only`; il ne modifie ni la v1 par défaut, ni la v3, ni la v4, ni l'assembleur CUDA/G4, et ne revendique ni temps, ni scalabilité, ni hiérarchie, ni statut public.

La chaîne persistante self-dual possède un test hôte volontairement court et verrouille `proof_basis=gpu_bounded_morton_seed_cpu_exact_bidirectional_component_dual_tree_boruvka_v4`. Le singleton doit fermer sans lancement. Sur la chaîne $8\to4\to2\to1$, trois rondes doivent persister exactement 14 minima directs de composantes et sept arêtes, offrir les huit graines aux composantes cibles de chaque ronde, reconstruire les $2n-1$ entrées de l'enveloppe pour les partitions à huit, quatre puis deux composantes, reproduire chaque décision et contraction de `build_exact_lbvh_boruvka`, puis effectuer un second parcours direct dans un contexte neuf avant le certificat final. Producteur et rejeu frais doivent appeler explicitement `frozen_initial`, exiger nuls les compteurs witness, racines uniformes, ancêtres mixtes, découvertes, ancêtres distincts et doublons, contrôler les certificats live et pointwise, puis exiger faux les certificats propres aux modes courant et dédupliqué. Aucun CSR current, minimum ponctuel, arbre dynamique d'incumbents ou état de file ne figure dans le type persistant. Le type d'audit de ronde peut porter le `proof_basis` v5 sans modifier la sémantique ni le contrat persistant v4. Une politique fiable différente doit échouer avant lancement; une mutation isolée de la couverture, de la borne de visites, du mode, d'un certificat ou compteur current ou dédupliqué, de la réduction bidirectionnelle, du minimum de composante ou du statut hiérarchique doit invalider le témoin final. Ce test ne qualifie ni CUDA, ni la scalabilité, ni une réduction hiérarchique.

La réduction explicite external-1NN vers `K1CompactForest` possède un test hôte distinct. Il exige un rejeu frais du témoin avant construction, l'immuabilité et le statut `not_performed/local_emst_witness_only` de cette source, puis l'égalité de chacune des cinq arènes, de la racine, des poids et des compteurs avec les réductions fraîches du témoin et de l'ancre Borůvka. Le singleton, une chaîne multi-ronde et un carré à multifusion d'arité quatre sont obligatoires. Une mutation isolée de chaque arène, des statuts, du scope, des poids, des compteurs ou de la politique fiable doit faire tomber `local_k1_hierarchy_certified`. Seul le résultat séparé peut publier `compact_k1_forest_certified/local_k1_compact_forest_only`; ce certificat local ne vaut ni `public_status=exact`, ni tour multi-ordre.

## 9. Générateurs de données

Chaque générateur possède une version, une graine, des paramètres sérialisés et un hash du nuage produit. Les coordonnées brutes des grands jeux ne sont pas commitées si elles sont régénérables.

### 9.1 Familles volumiques et stochastiques

| famille | paramètres à balayer | difficulté visée |
|---|---|---|
| uniforme | cube, boule, pavé d'aspect $1$, $10$, $100$ | régime volumique favorable, effet des bords |
| Poisson inhomogène | gradients linéaire, radial et par morceaux ; rapport d'intensité jusqu'à $100$ | rangs locaux très variables |
| mélanges gaussiens | nombre d'amas, séparation, covariance, poids | naissances et fusions contrôlables |
| mélanges non gaussiens | Student, Laplace, uniforme ellipsoïdal | queues lourdes et frontières nettes |
| bruit et outliers | $0\%$, $1\%$, $5\%$, $20\%$ | petites branches et événements parasites |
| processus clusterisés | Thomas ou Neyman–Scott | nombreuses échelles locales |

Pour les mélanges, le nombre d'amas parcourt **chaque entier de 1 à 64** au moins dans la suite de taille moyenne. La suite de passage à l'échelle retient $1,2,4,8,16,32,64$. Les autres axes comprennent :

- séparation normalisée $\Delta/\sigma\in\{1,1.5,2,3,5,8\}$ ;
- conditionnement des covariances dans $\{1,10,100,1000\}$ ;
- rapports de masse maximal/minimal $1$, $2$, $10$ et $100$ ;
- amas emboîtés, satellites et tailles suivant une loi de puissance ;
- orientation aléatoire indépendante de chaque covariance.

Les étiquettes de mélange servent seulement aux métriques secondaires. Elles ne définissent pas l'oracle HGP.

### 9.2 Familles géométriques de dimension intrinsèque faible

Les jeux suivants sont échantillonnés avec et sans bruit normal :

- segment, cercle, hélice et courbe en Y ;
- plan, deux plans sécants, ruban et grille surfacique ;
- sphère, ellipsoïde, tore et coquilles concentriques ;
- tube, cylindre, cône, embranchement vasculaire et filament ;
- deux boules reliées par un pont de densité variable ;
- vallées imbriquées et amas séparés par une membrane clairsemée ;
- scènes LiDAR synthétiques avec sol, façades, objets minces et occlusions.

Ces familles sont indispensables : une méthode quasi linéaire sur un Poisson volumique peut devenir très dense sur une surface ou un filament.

### 9.3 Familles adversariales

| famille | paramètre critique | comportement attendu |
|---|---|---|
| courbe des moments | $n$ croissant | croissance proche du pire cas Delaunay |
| grande sphère cosphérique | taille du shell | plateau déclaré, jamais résolu par jitter silencieux |
| grille exacte | dimension et pas | niveaux massivement égaux |
| duplications | multiplicité $2$ à $1000$ | agrégation explicite et rang pondéré |
| quasi-coplanarité | hauteur de $2^{-p}$ | sollicitation des filtres exacts |
| tétraèdres quasi plats | barycentrique proche de zéro | décision de relative-intériorité |
| offset gigantesque | translation jusqu'à $10^{12}$ avec écarts jusqu'à $10^{-6}$ | annulation catastrophique |
| échelles extrêmes | facteur de $10^{-12}$ à $10^{12}$ | équivariance et normalisation |
| ponts longs | densité et longueur | nombreuses fusions proches |
| contre-exemple aux listes L-NN | nombre de distracteurs | vérification qu'aucune liste locale fixe ne certifie la complétude |
| sorties denses | $n$ et géométrie | garde de budget honnête, absence d'OOM non contrôlé |

Les cas dont la combinatoire dépasse le budget peuvent finir `conditional` ou `budget_exhausted`; ils ne peuvent jamais finir `exact` avec un catalogue tronqué.

### 9.4 Données réelles

Une suite non bloquante puis une suite de release utilisent des scènes LiDAR et des nuages 3D publics à licence compatible. Chaque jeu possède une fiche de provenance, une somme SHA-256, un script de préparation et une version figée. Les données réelles servent à mesurer le profil de charge, pas à établir l'exactitude mathématique.

## 10. Matrice des tailles

| classe | valeurs de $n$ | rôle |
|---|---|---|
| micro | $4$ à $14$ | oracle exhaustif et réduction de contre-exemples |
| petite | $10^3$, $3\times10^3$ | tests GPU rapides et profils de lancement |
| moyenne | $10^4$, $3\times10^4$, $5\times10^4$, $10^5$ | objectif interactif et matrice complète de difficultés |
| grande | $10^6$, $3\times10^6$ | streaming, mémoire et reprise |
| extrême | $10^7$ | démonstration hors cœur géométrique, uniquement sur familles sélectionnées |

Tous les ordres $1$ à $10$ sont demandés en une seule exécution dès que $n\geq10$. Les cas $n<10$ s'arrêtent naturellement à $k=n$.

La matrice exhaustive des paramètres n'est pas appliquée à $10^7$ points. Elle est structurée en trois anneaux :

1. **couverture** : tous les paramètres, jusqu'à $10^4$ points ;
2. **interaction** : plans fractionnaires de paramètres, jusqu'à $10^5$ points ;
3. **échelle** : familles représentatives et adversariales choisies, jusqu'à $10^7$ points.

## 11. Tests différentiels et métamorphiques

### 11.1 Différentiels

La même entrée est traitée par :

- l'oracle exhaustif et le backend CPU de référence pour les micro-tailles ;
- le backend CPU de référence et le backend GPU pour les tailles compatibles ;
- le noyau de cellules de référence et toute primitive externe accélérée ;
- le pipeline avec filtres rapides actifs puis désactivés ;
- l'hyperarête en étoile puis sa clique explicite ;
- le calcul en mémoire puis le calcul en runs externes.

Les sorties sont canonicalisées avant comparaison. Pour un mode `budgeted`, chaque événement produit doit appartenir à la sortie exacte de référence ; le rappel est mesuré mais n'est pas une condition d'exactitude.

### 11.2 Transformations métamorphiques exactes

| transformation | invariant attendu |
|---|---|
| permutation des points | même sortie après réindexation inverse |
| permutation signée des axes | mêmes bits de niveaux et même combinatoire |
| translation dyadique exactement représentable | même combinatoire et mêmes niveaux |
| homothétie de facteur $2^q$ exactement représentable | même combinatoire, niveaux multipliés par $2^{2q}$ |
| permutation des lots et des blocs CUDA | même sortie canonique |
| variation du budget de streaming | même sortie si les deux exécutions ont le statut `exact` |
| restriction de $K_{\max}$ | préfixe identique de la tour |
| changement de pivot d'une hyperarête | même composante et même multifusion |
| changement des étages de filtre | même décision exacte |

Une transformation n'entre dans cette table que si les coordonnées IEEE transformées sont exactement celles de la transformation dyadique abstraite, vérification faite avant l'appel au backend. Les générateurs réservent des marges d'exposant et rejettent tout cas avec arrondi, overflow, underflow ou passage sous-normal involontaire. Une rotation orthogonale générale, même rationnelle, appartient aux tests différentiels : on compare le backend à l'oracle sur la **nouvelle entrée arrondie**, mais on ne lui attribue pas abusivement l'invariance exacte de l'entrée initiale.

### 11.3 Propriétés d'inclusion

Les tests génératifs vérifient aussi :

$$L_{k+1}(t)\subseteq L_k(t),\qquad L_k(t)\subseteq L_k(t')\quad\text{si }t\leq t'.$$

Après ajout de points $Y$, on a à ordre fixé

$$L_k^X(t)\subseteq L_k^{X\cup Y}(t).$$

Sur l'oracle discret, l'ajout de points peut fusionner des composantes mais ne doit pas invalider cette inclusion géométrique. Ce test est appliqué par insertion progressive de points et comparaison des applications induites, sans supposer que les arbres eux-mêmes restent identiques.

### 11.4 Déterminisme

Pour une entrée, une configuration et un statut identiques, les enregistrements scientifiques canonicalisés doivent être bit à bit identiques. Les temps, compteurs d'ordonnancement et représentants internes DSU peuvent différer. Les tests répètent chaque petite entrée avec :

- 20 ordonnancements de blocs ;
- plusieurs tailles de bloc et de lot ;
- allocation mémoire fraîche ou réutilisée ;
- concurrence maximale puis exécution sérialisée de diagnostic.

## 12. Dégénérescences et politique d'échec

### 12.1 Cas obligatoires

La suite couvre : points dupliqués, supports collinéaires ou coplanaires, shells de plus de quatre points, sphères cosphériques, barycentriques nuls, rayons nuls, niveaux de fusion égaux et coordonnées non finies.

Pour chaque famille, le comportement attendu est déclaré dans la fixture parmi :

- `exact` après agrégation exacte des multiplicités ;
- `exact` avec hyperévénement de plateau explicitement pris en charge ;
- `unsupported_degeneracy` avec diagnostic et aucun résultat exact revendiqué ;
- `conditional` ou `budget_exhausted` si tous les événements émis restent certifiés mais la fermeture est abandonnée.

Le jitter aléatoire ou une tolérance utilisée comme égalité sont interdits en mode certifié. Une perturbation symbolique n'est acceptée que si la sortie précise qu'elle concerne le problème perturbé et si une procédure de contraction vers le plateau original est démontrée et testée.

Avec `mode=budgeted, forest_semantics=partial_refinement`, chaque fixture vérifie en plus le `PartialScope` : aucun ordre ou profondeur ouverte ne figure parmi les périmètres fermés, chaque cellule déclarée close possède son certificat canonique, tous les loci dégénérés sont sérialisés et le résultat porte `public_status=conditional` ou `public_status=budget_exhausted`. Avec `public_status=unsupported_degeneracy`, `forest_semantics` est absent et seuls les événements vérifiés sont exposés. La même entrée avec `require_exact=true` doit refuser une forêt partielle avant calcul.

### 12.2 Tests de frontière du domaine certifié

Pour chaque hypothèse de généricité, une paire de fixtures est fournie : une entrée qui la satisfait avec une marge décroissante et sa limite dégénérée. Le backend doit passer de `exact` au traitement de plateau ou à `unsupported_degeneracy` exactement au zéro certifié, et non à une tolérance dépendant de l'échelle.

## 13. Métriques secondaires de clustering

Les générateurs à amas connus permettent de mesurer, à titre exploratoire :

- stabilité des branches en échelle et en ordre ;
- rappel des vallées et des ponts implantés ;
- pureté, ARI ou information mutuelle après une règle de coupe déclarée ;
- sensibilité aux déséquilibres de masse et de densité ;
- distorsion des niveaux de fusion par rapport à l'oracle sur les tailles accessibles.

Ces métriques évaluent des choix heuristiques de condensation ou de coupe. Elles ne participent jamais au verdict d'exactitude de MorseHGP3D.

## 14. Protocoles de performance sur G4

### 14.1 Configuration figée

La configuration de référence est la VM G4 définie par les scripts actifs du dossier `gcp-migration`, en priorité `g4-standard-48` avec un seul GPU. Chaque rapport enregistre :

- projet, zone, type de machine et identifiant du GPU ;
- image, noyau Linux, pilote NVIDIA, version CUDA et hash du conteneur ;
- fréquence, limite de puissance, température initiale et éventuel throttling ;
- nombre de vCPU, mémoire hôte, mémoire GPU et espace disque local disponible ;
- hash Git, options de compilation, bibliothèques et variables d'environnement ;
- graine, hash du jeu, $K_{\max}$, $K_{\mathrm{eff}}$, mode, profil et budgets typés ;
- statut de préemption et identifiant de tentative.

Les benchmarks n'utilisent ni horloge verrouillée ni privilège spécial non documenté. Toute différence de configuration sépare les séries.

### 14.2 Trois chronométrages distincts

| protocole | début | fin | usage |
|---|---|---|---|
| `cold_e2e` | nouveau processus, données sur disque local | sortie et certificat écrits, GPU synchronisé | coût réel d'une première requête |
| `warm_e2e` | processus et allocateur initialisés, nouveau transfert du nuage | sortie matérialisée, GPU synchronisé | service réutilisant le runtime |
| `resident_core` | points et index spatial déjà résidents sur le GPU | catalogue et hiérarchie disponibles sur le GPU | coût algorithmique répété |

Le provisionnement de la VM, le téléchargement du jeu et la compilation ne sont inclus dans aucun de ces temps ; ils sont rapportés séparément. Le temps `warm_e2e` inclut validation, transfert hôte–GPU, construction de l'index, calcul et transfert du résultat. Le temps `resident_core` doit préciser exactement quels index sont réutilisés.

Les temporisations GPU utilisent des événements CUDA et une synchronisation explicite ; le total hôte utilise une horloge monotone. Les exécutions d'échauffement sont exclues mais comptées.

### 14.3 Répétitions et statistiques

- $n\leq10^4$ : 10 démarrages froids et 50 répétitions chaudes ;
- $10^4<n\leq10^5$ : 5 démarrages froids et 20 répétitions chaudes; la qualification produit spéciale à $n=50\,000$ en exige 30 conformément à la phase 14 ;
- $10^5<n\leq10^6$ : 3 démarrages froids et 7 répétitions chaudes ;
- $n>10^6$ : au moins 3 répétitions complètes, ou justification du coût avec trois tentatives indépendantes.

Le rapport publie $p50$, $p95$, maximum, écart absolu médian et chaque valeur brute. Une moyenne seule est interdite. Une exécution préemptée ne compte pas comme mesure de temps, mais reste dans le rapport de fiabilité.

### 14.4 Objectifs réfutables

Les seuils suivants concernent `warm_e2e`, $K_{\max}=10$, un seul GPU G4, sur une famille volumique favorable dont le certificat reste sparse. Ils incluent la matérialisation de la hiérarchie demandée.

Une porte structurelle précède tout chronométrage : le rapport doit montrer zéro allocation indexée par les univers de facettes ou cofaces, zéro cellule top-$m$ persistée et zéro construction Gamma dans le target produit. Il publie points, nœuds LBVH, pic de frontière, produits prunés, supports feuilles, événements, attaches et octets de runs. Échouer cette porte invalide l'architecture même si le temps mesuré est faible.

Avant la campagne finale, un smoke unique sur 12 500, 25 000 et 50 000 points calcule les deux exposants successifs de chaque compteur de travail. Deux exposants supérieurs à 1,35 pour les tests de boîtes, la frontière ou les supports feuilles suspendent les micro-optimisations et imposent une revue de l'algorithme. Ce smoke ne remplace ni les 30 répétitions finales, ni une preuve asymptotique.

`BenchmarkOutputContract-v1` impose le profil `hgp_reduced`, les dix forêts, les applications verticales, les lots et le certificat minimal copiés en mémoire hôte épinglée avant la fin du chronomètre. Le catalogue de replay complet, l'expansion des unions de points et l'écriture disque sont chronométrés séparément. Un benchmark qui change ce payload appartient à une autre série.

| $n$ | objectif $p95$ | condition |
|---:|---:|---|
| $1\,000$ | $\leq25$ ms | exécution certifiée |
| $10\,000$ | $\leq200$ ms | exécution certifiée |
| $50\,000$ | $\leq1$ s | exécution certifiée ; objectif interactif principal |
| $100\,000$ | $\leq3$ s | exécution certifiée |
| $1\,000\,000$ | $\leq60$ s | seulement si le catalogue et les runs restent sparse |
| $10\,000\,000$ | $\leq600$ s | seulement si le catalogue reste sparse ; streaming autorisé |

Ces seuils ne valent pas pour toutes les géométries. Le pire cas exact peut produire une sortie quadratique dès les structures de Delaunay 3D. En cas de dépassement, le test échoue par rapport à l'objectif de produit, mais ne conclut pas à une erreur mathématique. Le rapport conserve la mesure au lieu de masquer l'échec ou de modifier le seuil.

Pour $3\times10^6$ points, la campagne mesure sans imposer initialement un SLA distinct ; un objectif ne sera fixé qu'après deux releases reproductibles.

### 14.5 Régimes de benchmark

Chaque taille est testée au minimum sur :

1. un Poisson uniforme volumique ;
2. un mélange équilibré de 8 amas ;
3. un mélange de 32 amas avec rapport de masse $100$ ;
4. une surface bruitée ;
5. un pont de faible densité ;
6. une famille adversariale choisie pour densifier le catalogue.

Les objectifs du tableau précédent ne sont évalués que sur 1 et 2. Les autres familles mesurent la dégradation et le passage honnête vers `conditional`, `unsupported_degeneracy` ou `budget_exhausted`.

### 14.6 Budgets et stockage G4

Chaque test de streaming fixe `device_budget_bytes`, `host_budget_bytes`, `scratch_budget_bytes`, `output_budget_bytes` et `time_budget_s`. Les valeurs absolues, consommées et restantes sont comparées à un oracle de comptabilité à chaque frontière transactionnelle. Les cas limites placent chaque budget un octet sous, exactement sur et un octet au-dessus du besoin prévu.

Sur G4, les manifestes n'acceptent que Hyperdisk pour le stockage persistant et Titanium SSD pour le scratch local rapide; `g4-standard-48` peut en utiliser jusqu'à quatre. Une reprise qui doit survivre à la VM n'est validée qu'après copie et checksum du checkpoint sur Hyperdisk.

Pour chaque run et merge, la suite force une panne après création du temporaire, après écriture, après synchronisation, après renommage et avant publication du manifeste. L'ancien état reste lisible jusqu'à validation atomique du nouveau. Si l'espace ne couvre pas simultanément ancien état, temporaire, merge, checkpoint et marge, aucune nouvelle écriture ne commence et le résultat vaut `budget_exhausted` depuis le dernier checkpoint durable.

## 15. Compteurs obligatoires

Chaque exécution émet un enregistrement structuré, versionné, comprenant au minimum :

### 15.1 Entrée et sortie

- $n$, boîte englobante, échelle de normalisation, duplications et multiplicités ;
- $K_{\max}$, $K_{\mathrm{eff}}$, $s_{\max}$, $m_{\star}$, `profile`, mode demandé et statut obtenu ;
- nombre de composantes, branches, flèches verticales et points couverts par ordre ;
- nombre de niveaux distincts et taille maximale d'un lot égal ;
- raison exacte d'un statut `conditional`, `budget_exhausted`, `unsupported_degeneracy` ou `numeric_failure`.

### 15.2 Catalogue et géométrie

- candidats proposés, dédupliqués, acceptés et rejetés ;
- répartition par taille de support, rang fermé, ordre de détection et ordre d'usage ;
- labels parents, cellules ouvertes/fermées, contraintes de découpage et sommets ;
- violations de fermeture trouvées, rondes de fermeture et taille maximale des files ;
- visites LBVH, élagages, requêtes top-$k$, requêtes de rang et exclusions ;
- hyperarêtes, facettes, émissions redondantes, unions DSU utiles et inutiles ;
- runs externes, octets écrits/lus et volume de fusion;
- cellules canoniques reconstruites, amorces vides ou non, violateurs et co-ties ajoutés par ronde;
- pour la phase 17 : supports bruts par taille, supports bien centrés, boules et saturés distincts $M_{\mathrm{sat}}$;
- memberships $L_{\mathrm{sat}}=\sum_S\lvert S\rvert$, distribution des capacités et `peak_active_inclusion_maxima`, avec coût du join d'inclusion compté séparément;
- longueurs des postings $d_x$, $P_{\mathrm{post}}=\sum_x\binom{d_x}{2}$, paires uniques et pic de l'accumulateur;
- arêtes d'intersection examinées et retenues, remplacements de forêt, octets de l'historique et temps des requêtes de coupe;
- certificats de complétude des supports, ranges fermés, déduplication, lots, join, forêt, rejeu et applications verticales.

### 15.3 Numérique

- appels et décisions à chaque étage de précision ;
- replis multiprécision CPU, zéros exacts et cas indécis ;
- plateaux, shells de plus de quatre points et supports non essentiels ;
- marges minimale et quantiles des prédicats filtrés, sans les utiliser comme tolérance.

### 15.4 Temps et ressources

- temps `cold_e2e`, `warm_e2e`, `resident_core` et temps de chaque phase ;
- temps GPU par famille de noyaux, nombre de lancements et octets traités ;
- pics de mémoire GPU et hôte, pic des arènes, fragmentation et nombre d'allocations ;
- budgets device, hôte, scratch, sortie et temps : limite, pic, réserve et raison du premier refus ;
- octets Hyperdisk et Titanium SSD, espace transactionnel réservé et checkpoints rendus durables ;
- débit des tris, réductions, requêtes LBVH et écritures de runs ;
- énergie ou puissance moyenne si la télémétrie est disponible sans perturber la mesure ;
- événements de préemption, reprise et recomputation.

Un compteur absent rend le benchmark diagnostiquement incomplet, même si le temps total est disponible.

## 16. Fixtures et artefacts reproductibles

### 16.1 Arborescence cible

```text
tests/
  fixtures/
    exact/
    hierarchy/
    degeneracies/
    metamorphic/
    regressions/
  generators/
  oracle/
  property/
  differential/
  performance/
benchmarks/
  manifests/
  baselines/
```

Les gros nuages sont générés ou téléchargés sur la VM et ne sont pas versionnés comme blobs sans nécessité.

### 16.2 Manifestes

Chaque fixture ou benchmark décrit :

- identifiant stable et objectif du cas ;
- générateur, version, graine et paramètres ;
- hash SHA-256 des coordonnées canoniques ;
- convention d'unités et politique de duplications ;
- $K_{\max}$ et `profile` ;
- statut attendu et hypothèses de généricité ;
- sortie canonique attendue ou référence vers l'oracle ;
- provenance et licence pour les données externes.

Les sorties dorées stockent des enregistrements triés, pas seulement un hash : catalogues critiques, niveaux exacts sérialisés, hyperarêtes, multifusions, unions de points et flèches verticales. Le hash accélère la comparaison mais ne remplace pas un diff lisible.

### 16.3 Régressions

Tout défaut corrigé ajoute :

1. l'entrée minimale qui le reproduit ;
2. la sortie erronée et la sortie correcte ;
3. le prédicat ou l'invariant violé ;
4. la graine et le contexte de compilation ;
5. un test qui échoue sur le commit fautif.

## 17. Campagnes d'exécution

### 17.1 À chaque changement

- validation T0 ;
- tous les prédicats T1 ;
- fixtures exactes et dégénérées ;
- oracle complet jusqu'à $n=12$ ;
- propriétés métamorphiques courtes ;
- comparaison $k=1$ avec l'EMST exhaustif.

### 17.2 Campagne longue G4 manuelle

Cette campagne n'est jamais déclenchée par un workflow GitHub planifié. Chaque shard exige une autorisation utilisateur explicite, un manifeste de durée prévisionnelle et les deux coupe-circuits vérifiés. Sa durée GCE est strictement inférieure à huit heures; le runner cesse de prendre du travail et checkpoint au moins trente minutes avant l'échéance, puis arrête et vérifie la VM. Une matrice trop grande est répartie sur plusieurs sessions indépendantes, chacune terminée par `TERMINATED`.

La campagne de signes de phase 2B suit en plus la section 5.4 : chaque session ne commite que des chunks atomiques, reprend la chaîne de racines déjà vérifiée et peut s'arrêter entre deux transactions sans publier de certificat final. Les deux corpus de 11 880 000 signes sont achevés sur autant de sessions bornées que nécessaire; aucun shard isolé ni état `running` ne satisfait la porte.

- oracle aléatoire jusqu'à $n=14$ ;
- différentiel CPU/GPU sur au moins 1 000 nuages ;
- tous les nombres d'amas de 1 à 64 à taille moyenne ;
- matrice de précision et d'ordonnancement ;
- tailles jusqu'à $10^5$ sur les six régimes ;
- un cas $10^6$ tournant par shard ;
- test d'interruption et de reprise d'un checkpoint ;
- vérification finale de l'arrêt de la VM.

### 17.3 Release

- reprise de toutes les régressions historiques ;
- matrice complète jusqu'à $10^5$ ;
- au moins trois graines pour $10^6$ et $3\times10^6$ ;
- un passage $10^7$ sur Poisson favorable, mélange et charge dense contrôlée ;
- rapport de performance brut, environnement et compteurs ;
- test de préemption Spot pendant au moins trois phases distinctes ;
- validation des critères de passage de la section 19.

## 18. GCP, Spot, reprise et extinction

### 18.1 Garde de durée de vie

Aucune campagne ne démarre si la VM ne possède pas une durée de vie maximale finie conforme aux scripts `gcp-migration`, une action terminale explicite et un arrêt invité armé. Le pré-test doit vérifier l'état réel renvoyé par GCE, pas seulement les variables du script.

La suite GCP vérifie au minimum :

- refus d'une création ou d'un démarrage sans limite de durée ;
- refus avant mutation GCE d'une clé SSH de session absente, chiffrée, symbolique, trop permissive, mal appariée, physiquement située dans le dépôt ou absente d'un profil OS Login unique à expiration bornée; import temporaire, propagation explicite de la même clé et de la même échéance UTC absolue vers start, SSH et SCP, absence de renouvellement relatif, révocation après certification de génération `TERMINATED`, nettoyage après preuve qu'aucune génération n'a changé et conservation sous l'échéance initiale lorsque cette preuve manque ;
- présence d'un `maxRunDuration` borné et d'une échéance future : `terminationTimestamp` exposé et cohérent, ou à défaut somme certifiée d'un `lastStartTimestamp` frais et de la durée GCE relue; la borne sûre retranche toute tolérance d'acceptation, soit 300 secondes dans les scripts actuels ;
- action de fin `STOP` et non redémarrage automatique incontrôlé ;
- arrêt invité de secours armé ;
- labels de projet permettant un balayage de sécurité ;
- arrêt même après erreur de test, signal ou préemption.
- deadline de travail par défaut au moins trente minutes avant cette borne GCE sûre, sans nouvelle unité lancée après cette deadline; une campagne transactionnelle et reprenable peut employer une marge réduite d'au moins quinze minutes seulement si chaque unité est bornée à 240 secondes au plus, si un checkpoint atomique est publié après chaque unité, si vérification, copie et nettoyage sont eux-mêmes bornés par le temps restant et si les époques permettent d'auditer la marge choisie; preflight, build et unités conteneurisées restent bornés dynamiquement, avec une réserve explicite pour tuer les descendants et nettoyer avant la deadline. Toute autre campagne conserve la marge de trente minutes.
- `timeout`, `date` et `sleep` résolus par leurs chemins système fixes, avec binaire et chaîne de parents root et non inscriptibles par le groupe ou les autres avant le premier calcul de deadline ; GNU `timeout` exécuté en mode de groupe de processus, sans `--foreground`, avec preuve hermétique qu'un descendant qui survit à `SIGTERM` reçoit le `SIGKILL` terminal.
- avant suppression des temporaires invités, remontée bornée des 240 dernières lignes et de 65 536 octets au plus du journal de toute unité de qualification en échec, sans publication d'un artefact de succès.
- résolution de `nvcc` malgré un `PATH` non interactif réduit, depuis un `CUDA_HOME` absolu ou les emplacements CUDA 12.9 usuels, sans accepter une autre version du toolkit.
- accès Docker direct ou via le chemin fixe `/usr/bin/docker` certifié root et non inscriptible avant `sudo -n`, sans exécuter un client injecté par `PATH`, avec timeout individuel, retry non mutatif borné et aucune nouvelle sonde après la deadline, puis diagnostic borné du daemon, de systemd, des paquets et du journal avant tout build en cas d'échec ;
- chaque conteneur de qualification possède un `cidfile`, un nom et un label privés; tout repli par nom atteste aussi l'image, le label et le CID disponible avant `docker rm -f`, puis exige une absence stable après succès, échec ou signal, et tout CID invalide ou échec de nettoyage interdit l'artefact final ;
- si et seulement si l'option explicite de préparation Docker est présente : relecture préalable des deux bornes, Ubuntu 22.04 `amd64` et toolkit NVIDIA figé, refus des familles de paquets concurrentes, des états `dpkg` partiels, des chemins élevés ou configurations ambigus, simulation APT sans suppression ni changement d'un paquet installé, installation des candidats exacts `docker.io` et `docker-buildx` depuis les seuls dépôts existants, validation de `daemon.json`, activation bornée des services, conservation du boot ID et de l'arrêt invité, recertification GCE après préparation, puis premier conteneur GPU laissé au worker non mutatif.

### 18.2 Checkpoints

Les checkpoints sont atomiques et contiennent : hash de l'entrée, hash Git, paramètres, ordre courant, runs achevés, dernier niveau entièrement fermé, compteurs et sommes de contrôle des artefacts. Un checkpoint n'est publiable qu'après écriture dans un chemin temporaire, synchronisation puis renommage atomique.

Les tests interrompent le processus :

1. après construction de l'index spatial ;
2. au milieu de la génération de cellules ou de candidats ;
3. entre deux runs externes ;
4. au milieu d'un grand lot de niveau égal ;
5. pendant la réduction hyper-Kruskal ;
6. juste avant l'écriture de la sortie finale.

Après reprise, la sortie canonique et le statut doivent être identiques à une exécution sans interruption. Aucun événement ni union ne peut être dupliqué. Un lot égal partiellement appliqué est annulé ou rejoué intégralement.

### 18.3 Préemption Spot

Une campagne de qualification force ou simule une préemption aux points d'interruption 2, 3 et 5 de la section 18.2. Elle mesure le temps perdu, le volume relu et la quantité recomputée. Une préemption ne peut transformer une exécution incomplète en `exact`.

### 18.4 Condition terminale obligatoire

À la fin de **toute** session G4, succès ou échec :

1. exécuter le script d'arrêt et de vérification prévu dans `gcp-migration` ;
2. attendre l'état GCE `TERMINATED` de l'instance ciblée ;
3. balayer toutes les instances portant le label du projet E-HGP ;
4. inventorier et signaler les autres instances actives sans faire échouer la campagne à cause d'elles ;
5. ne jamais arrêter, modifier ou supprimer une instance autre que la cible exacte sans autorisation explicite ;
6. enregistrer l'heure, l'instance ciblée et sa preuve d'état dans l'artefact de campagne.

Une campagne n'est pas « terminée » tant que cette condition n'est pas satisfaite. Les workflows GitHub ordinaires ne doivent jamais créer ni démarrer automatiquement une ressource facturable.

## 19. Portes de passage et critères go/no-go

| porte | condition `go` | condition `no-go` immédiate |
|---|---|---|
| G0 — formats | schéma v2 actif, v1 archivé, manifests et sorties canoniques stables | sortie non versionnée, base de preuve interdite ou résultat non reproductible |
| G1 — prédicats | zéro faux signe sur tout T1 | un filtre certifie un mauvais signe |
| G2 — catalogue | égalité exhaustive jusqu'à $n=14$ | événement manquant ou faux avec statut `exact` |
| G3 — hiérarchie | référence exacte égale à Gamma; voie Gabriel incluse positivement et marquée partielle | ordre de threads modifiant la hiérarchie, connexion inventée ou Gabriel brut annoncé exact |
| G4 — verticalité | unicité et commutation à tous les seuils | flèche absente, multiple ou non commutative |
| G5 — GPU | zéro différence canonique seulement pour une future base exacte prouvée; sinon toutes les connexions GPU sont incluses dans Gamma et le statut reste conditionnel | approximation silencieuse, connexion absente de Gamma, repli non signalé ou faux statut exact |
| G6 — 50k interactif | objectif $p95\leq1$ s atteint sur les deux familles favorables | dépassement : pas de revendication interactive, profilage requis |
| G7 — million | fin sous budget sur cas sparse avec statut conforme à la base disponible, sans dépassement mémoire | OOM non contrôlé, perte de run/checkpoint ou faux statut exact |
| G7b — trois millions | trois graines sparse avec `forest_semantics=exact`, `public_status=exact`, streaming transactionnel et reprise vérifiée | statut limité, OOM, support non fermé, état durable incohérent ou faux statut exact |
| G8 — adversarial | sortie exacte, ou statut limité honnête et exploitable | catalogue tronqué annoncé `exact` |
| G9 — exploitation GCP | reprise identique et instance ciblée vérifiée `TERMINATED` | instance ciblée encore active ou garde de durée absente |

Les portes G1 à G5 sont absolues. Les objectifs G6, G7 et G7b sont des décisions de produit : leur échec déclenche une analyse et peut reporter le jalon scalable, mais ne doit jamais conduire à affaiblir les tests mathématiques.

## 20. Jalons de validation

### 20.1 `v1-correctness`

Ce jalon peut être qualifié uniquement si :

1. son domaine de statut `exact` est écrit sans ambiguïté ;
2. T1–T4 ne révèlent aucun désaccord ;
3. le périmètre `hgp_reduced` est validé exhaustivement jusqu'à $n=14$ pour tous les ordres ;
4. `full_pi0`, s'il n'est pas démontré, est désactivé ou explicitement conditionnel ;
5. les données dégénérées sont traitées ou refusées sans jitter silencieux ;
6. le backend GPU produit soit la même sortie sous une future base exacte prouvée, soit une connectivité positive incluse dans Gamma avec statut conditionnel ;
7. les compteurs permettent d'expliquer le coût et tout abandon de budget ;
8. le test $k=1$ coïncide avec l'EMST ;
9. le jalon $50\,000$ points possède une mesure G4 reproductible, sans que l'étiquette `interactive` soit accordée si elle dépasse une seconde ;
10. chaque campagne GCP se clôt par une preuve d'état `TERMINATED` de son instance exactement ciblée.

### 20.2 `v1-interactive-scalable`

Ce jalon exige `v1-correctness`, la Phase 12 et M.1 fermés, la migration contractuelle versionnée vers la base exacte du flux direct, puis G6, G7 et G7b. La preuve dédiée `three_sparse_3m_exact_runs` matérialise les trois graines de G7b et interdit qu'un statut limité satisfasse implicitement ce jalon. Il exige aussi au moins une campagne à dix millions qui se termine sans corruption ni OOM non contrôlé : soit `forest_semantics=exact, public_status=exact` si le certificat reste sparse, soit `mode=budgeted, forest_semantics=partial_refinement` avec `public_status=conditional` ou `public_status=budget_exhausted`, checkpoint durable, `PartialScope` et diagnostic complet. Seul `public_status=exact` peut valider l'objectif conditionnel de dix minutes.

Le principe final est simple : **chaque événement peut être rapide ou lent, mais il doit être vrai ; chaque catalogue peut être complet ou budgété, mais son statut doit le dire ; chaque VM peut être préemptée, mais elle ne doit jamais rester allumée par oubli.**
