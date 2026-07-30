# Architecture GPU active de MorseHGP3D

> **Statut.** Phase 15, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, porte d’entrée satisfaite, porte de sortie non satisfaite, `deployment_status=architecture_only`, `public_status=not_claimed`. Run4 qualifie la première couverture tuilée device-résidente comme composant borné sur G4 pour les seuils de rang fermé 2 à 11, avec artefacts auditables. Le cap de 2 048 visites est désormais un quantum reprenable dans le même processus; un `uint64` D2H et une synchronisation contrôlent chaque subdivision. Deux profils directs run5 à 10 M et 30 M atteignent `candidate_capacity`, restent censurés, `component_only / profile_only`, non reprenables après redémarrage et de provenance insuffisante pour une qualification. Le drainage résident, le classifieur exact multi-ordre, `count/scan`, le payload fermé et la réduction Hartigan restent absents. Le catalogue CUDA scientifique n'est pas implémenté.

> **Priorité.** Une passe multi-ordre résidente construit d’abord toutes les paires diamétrales de rang fermé au plus $K_{\max}+1$ avec leur payload complet. Elle doit obtenir ce résultat par une frontière fusionnée Morton--Yao48 et ne peut ni parcourir inconditionnellement toutes les paires, ni posséder un fallback dense. Une frontière indépendante ne cherche ensuite que les supports propres de taille trois, puis une autre les supports propres de taille quatre. Aucune mosaïque de Delaunay d’ordre supérieur, cellule top-$m$, coface ou incidence globale n’est construite.

Les résultats des anciennes voies synchronisées, PDEL, Geogram et `prune-only` sont conservés dans les [archives](archive/abandoned/README.md) et dans le [journal de Phase 14](validation/PHASE14_PROGRESS.md). Ils sont des preuves négatives ou des oracles, pas des composants à renommer dans la voie active.

## 1. Deux sorties scientifiques distinctes

Pour $u\neq v$, on pose $\Phi_{u,v}(x)=(x-u)\mathbin{\cdot}(x-v)$, $C_X(u,v)=\left\lbrace x\in X:\Phi_{u,v}(x)\leq0\right\rbrace$ et $r(u,v)=\lvert C_X(u,v)\rvert$. Avec $K_{\mathrm{eff}}=\min(K_{\max},n-1)$, la fenêtre fermée vaut $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Le rang $R$ détermine le bucket de stockage de la boule. Un simplexe Gabriel porté $Q$ de cardinal $q$ alimente $\Gamma_{q-1}$; seulement sous `RelevantGP`, où le shell supplémentaire utile est vide et $q=R$, ce routage devient $k=R-1$.

La convention d'API ne doit jamais être implicite : `requested_order=K` demande les rangs fermés $R\leq K+1$ et un prune de cette fenêtre requiert $K$ témoins supplémentaires aux deux supports. Une demande littérale « la boule contient au plus $K_{\mathrm{total}}$ points au total » demande $R\leq K_{\mathrm{total}}$ et requiert $K_{\mathrm{total}}-1$ témoins. Les compteurs, reçus et artefacts sérialisent la convention utilisée.

Le producteur doit fermer séparément :

1. `closed_rank_catalog_complete` : toutes les paires de rang fermé dans la fenêtre sont présentes, avec tous leurs points intérieurs et leur shell fermé;
2. `pair_support_relevant_gp_complete` : toute paire telle que $\lvert I\rvert\leq s_{\max}-2$ possède un shell complètement décidé, de sorte qu'un extra-shell pertinent est détecté même lorsque le rang fermé total dépasse la fenêtre.

Pour un support utile de taille $m\leq s_{\max}$, l'extra-shell est pertinent tant que $\lvert I\rvert\leq s_{\max}-m$; un prune `RelevantGP` exige donc $t_m=s_{\max}-m+1$ témoins strictement intérieurs. Pour les paires, $t_2=s_{\max}-1$, égal à $K_{\max}$ lorsque $n\geq K_{\max}+1$. Un prune fondé sur $t_2$ points de la boule **fermée** crédite seulement le premier reçu et peut masquer une grande cosphéricité. Le reçu de position générale ne crédite qu'un prune par $t_2$ témoins **strictement intérieurs**, ou une classification complète de l'intérieur et du shell. Le champ global `relevant_gp_complete` reste faux tant que les supports de tailles trois et quatre n'ont pas fermé le même contrat.

La fixture `morsehgp3d/tests/fixtures/spatial/relevant_gp_extra_shell_above_smax.json` interdit toute confusion entre ces deux masses.

## 2. Morton indexe et possède; Yao48 élimine

Les points sont ordonnés par `(MortonCode, PointId)`. Pour deux positions $i<j$, la paire appartient opérationnellement à l’ancre $j$; la sortie scientifique reste canonicalisée par `(min(PointId), max(PointId))`. Cet ordre total empêche les doublons, mais ne certifie jamais une exclusion géométrique.

Le target ne développe pas ce préfixe feuille par feuille. Pour une tuile de $B$ ancres, un parcours LBVH proche-en-premier fusionne trois opérations : il saute le suffixe hors ownership, alimente les banques Yao48 depuis les feuilles effectivement rencontrées, puis prouve en bloc que les régions suffisamment lointaines sont au-dessus de la fenêtre de rang. Une feuille non prunée devient candidate; aucune étape intermédiaire n'émet le préfixe brut.

L'ordre proche-en-premier est seulement opérationnel. Chaque témoin est recertifié quant à son identité, sa chambre semi-ouverte et sa distance; une chambre insuffisante laisse le parcours descendre. Une fenêtre Morton optionnelle peut accélérer l'amorçage, sans aucun contrat de rappel et sans pouvoir d'exclusion.

Le chemin massif ne stocke jamais une table $n\times48\times K_{\max}$. Il traite une tuile de $B$ ancres et borne les banques actives par $O(B\mathbin{\cdot}48\mathbin{\cdot}K_{\max})$, puis réutilise cet espace. Tant que $n<2^{32}$, les banques internes stockent des positions Morton 32 bits et ne convertissent en `PointId` canonique qu'à l'émission : pour $B=4096$ et $K_{\max}=10$, $B\times48\times K_{\max}$ occupe exactement 7,5 Mio, contre 15 Mio avec des identifiants 64 bits.

## 3. Yao48 sans top-$K$ exact obligatoire

Soient $(x,y,z)$ les coordonnées canoniques de $q-p$ dans une chambre Yao48 semi-ouverte, avec $x\geq y\geq z\geq0$. Prenons $t_2$ témoins $w_i$ dont les vecteurs $w_i-p$ appartiennent à cette même chambre, dont les `PointId` sont distincts et disjoints des deux supports $p,q$, ainsi qu'une valeur certifiée $D\geq\max_i\left\Vert w_i-p\right\Vert^2$. Pour une plage cible, tout recouvrement entre témoins et feuilles cibles est retiré du compte. Il n'est pas nécessaire que ces témoins soient les plus proches.

Les conditions fermées

$$x^2\geq D,\qquad(x+y)^2\geq2D,\qquad(x+y+z)^2\geq3D$$

placent les $t_2$ témoins dans $C_X(p,q)$ et prouvent $r(p,q)\geq t_2+2=s_{\max}+1$. Les versions strictes

$$x^2>D,\qquad(x+y)^2>2D,\qquad(x+y+z)^2>3D$$

placent tous les témoins dans l’intérieur strict. Elles constituent le cutoff par défaut de la voie fail-closed `RelevantGP`. Les égalités descendent; sur une distribution continue, cette différence est de mesure nulle, tandis qu’elle est décisive sur les données cosphériques.

Prendre les vrais $t_2$ plus proches dans cette même chambre ne fait que diminuer $D$ et améliorer le rendement. Le premier kernel produit peut donc conserver les premiers témoins utiles d'une fenêtre Morton et une majoration de $D$ dirigée vers $+\infty$; le tri exact des distances et les égalités `(distance, PointId)` sortent du chemin critique. Une recherche LBVH optionnelle peut resserrer les chambres dont le rayon est mauvais sans changer l'autorité.

Pour un nœud cible entièrement certifié dans une chambre, les minima de $x$, $x+y$ et $x+y+z$ sur son AABB appliquent les trois comparaisons à toutes ses feuilles. Un nœud qui traverse une frontière de chambre descend ou passe au certificat individuel suivant.

Ce certificat est unilatéral. Son succès prouve que la cible est hors fenêtre; son échec ne caractérise ni le rang, ni l'appartenance au catalogue. La paire reste seulement dans le sur-ensemble candidat jusqu'à la classification exacte. La fixture unitaire `test_directional_cutoff_is_not_a_rank_characterization` verrouille ce non-converse.

## 4. Réservoir individuel complémentaire

Une banque courte $W_p$, typiquement 64 témoins Morton proches, est construite et consommée par tuile et couvre les directions que le cutoff compressé n’a pas remplies. À 4096 ancres, 64 positions 32 bits par ancre occupent 1 Mio; une banque globale à plusieurs dizaines de millions de points est interdite. Pour un témoin $w$ et un nœud cible $Q$, on calcule une borne inférieure de

$$L_Q(p,w)=\min_{q\in Q}(q-p)\mathbin{\cdot}(w-p)-\left\Vert w-p\right\Vert^2.$$

Chaque témoin avec $L_Q(p,w)>0$ est strictement intérieur à la boule diamétrale de toute cible du nœud. Dès que $t_2$ témoins distincts réussissent, le nœud est exclu des deux reçus. La stricte inégalité empêche automatiquement de compter la cible comme son propre témoin. Le mode fermé $L_Q(p,w)\geq0$ est permis pour le catalogue seul, mais doit alors exclure explicitement toute intersection témoin–cible et ne crédite jamais `RelevantGP`.

L’ordre de test d’un nœud est : cutoff Yao48 à trois comparaisons, première vague de 32 témoins avec ballot, deuxième vague seulement si nécessaire, puis descente. Les banques Yao et le réservoir sont des accélérateurs fail-open; le classifieur terminal reste l’autorité.

## 5. Graphe CUDA du catalogue de paires

Le premier target résident consomme une `MortonLbvhDeviceTraversalLease` et enchaîne sans callback par paire :

1. valider l’identité du nuage, l’epoch, les capacités 64 bits et $K_{\max}$;
2. préparer une tuile d’ancres et son ownership Morton, puis remplir ses 48 banques pendant le même parcours proche-en-premier;
3. rapporter seulement des prunes Yao48 certifiés avec leur masse ou des feuilles candidates; une interruption laisse le reste dans `unresolved_pair_mass`;
4. appliquer à chaque feuille candidate les cutoffs disponibles à l’autre extrémité lorsqu’ils sont déjà résidents, sans en faire une condition de complétude;
5. dédupliquer canoniquement les survivants et classifier exactement ceux-là, une seule fois pour tous les ordres;
6. compacter les records, leurs intérieurs stricts et leurs extra-shells dans des tableaux CSR;
7. radix-trier les records par `(closed_rank,u,v)`, vérifier l’unicité et publier une lease seulement lorsque la frontière et les files exactes requises sont vides.

Le reçu ferme sur des entiers 64 bits contrôlés ou multiprécision l'identité `candidate_pair_mass + certified_pruned_pair_mass + unresolved_pair_mass = n(n-1)/2`. Les prunes comptent la cardinalité des régions sans développer leurs feuilles; les candidats comptent uniquement les paires effectivement remises au classifieur. Une publication exhaustive exige `unresolved_pair_mass=0`. Les `count + scan` des étapes 6 et 7 dimensionnent uniquement records et payloads; ils ne rejouent jamais le parcours géométrique. Une saturation conserve le curseur exact et retourne `budget_exhausted`; elle ne déclenche jamais une boucle dense de rattrapage.

La classification d'une paire suit deux compteurs. Le catalogue fermé peut conclure `above_window` dès que le rang dépasse $s_{\max}$. La lane `RelevantGP` ne conclut tôt qu'après $t_2$ points strictement intérieurs; une masse de shell ne suffit pas. Si le parcours finit avec moins de $t_2$ intérieurs et un point de shell hors support, il publie `unsupported_degeneracy`, y compris lorsque le rang fermé est énorme.

Pour un record accepté, le rang est au plus $s_{\max}$ et le payload hors support au plus $s_{\max}-2$; cela donne onze et neuf lorsque $K_{\max}=10$ et $n\geq11$. Un warp peut donc conserver temporairement les identifiants utiles dans un tableau fixe, puis un `count + DeviceScan + emit` compacte seulement les succès. Les sous-arbres entièrement intérieurs ajoutent leur cardinal et ne sont développés que si leurs identifiants peuvent encore appartenir à un record accepté.

La lease résidente utilise un layout SoA :

```text
support_u[M], support_v[M], closed_rank[M]
rank_offsets[Kmax + 2]
strict_offsets[M + 1], strict_ids[SI]
shell_offsets[M + 1], shell_ids[SE]
```

Les offsets, capacités, masses et comptes de payload sont en 64 bits. Un arrêt de capacité renvoie `budget_exhausted` sans lease exhaustive. Les candidats, prunes et compteurs restent sur le device; le retour normal avant le consommateur triangles est seulement une lease et un petit reçu terminal.

## 6. Pile de prédicats exacte sur GPU

Les décisions utilisent trois étages séparés :

1. intervalles FP64 dirigés, sans `--use_fast_math`;
2. expansions de taille fixe pour les cas bien échelonnés;
3. accumulateur dyadique à nombre de limbs borné par le domaine binary64, traité coopérativement par un warp pour les rares cas extrêmes.

Le signe de $\Phi$ est une somme de trois produits de différences binary64. Le composant de qualification applique maintenant intervalle dirigé, limbs fixes 128/256 bits, puis un lot multiprécision CPU pour tout dépassement conservatif. Il reste séparé du parcours résident : sa qualification locale ne suffit pas à annoncer `closed_rank_catalog_complete`, et le raccord doit conserver la classification des seuls survivants.

Les supports de tailles trois et quatre emploient les déterminants homogènes de Gram–Cramer et le polynôme de puissance. Leurs caps de limbs sont distincts de celui de $\Phi$. La canonicalisation finale de centres et niveaux rationnels peut rester sur CPU pour un flux de sortie borné; aucune décision de présence, de rang ou de prune ne doit en dépendre.

Les compteurs séparent pour chaque prédicat : décision FP64, expansion, limbs GPU, fallback terminal et non-résolu. Un non-résolu garde la frontière ouverte ou produit `numeric_failure`; il n’est jamais converti en signe par epsilon.

## 7. Ce que les records engendrent réellement

Soit une boule classifiée par un support minimal $T$, ses points strictement intérieurs $I$ et son shell supplémentaire $E$. Pour tout $Q$ tel que $T\subseteq Q\subseteq T\cup I\cup E$, la miniboule reste celle de $T$. Mais $Q$ est Gabriel si et seulement si $I\subseteq Q$; les points de $E$ peuvent être omis.

Le nombre de sous-simplexes Gabriel de cardinal $q$ portés par ce record et contenant un support minimal fixé $T$ vaut donc

$$N_q(T,I,E)=\binom{\lvert E\rvert}{q-\lvert T\rvert-\lvert I\rvert},$$

avec la convention zéro hors de l'intervalle $0\leq q-\lvert T\rvert-\lvert I\rvert\leq\lvert E\rvert$. Ce compte remplace l'émission aveugle de $\binom{\lvert I\rvert+\lvert E\rvert}{q-\lvert T\rvert}$ combinaisons. En présence de plusieurs supports minimaux de la même boule, les familles peuvent se recouvrir : le producteur les réunit puis les déduplique comme ensembles de `PointId`. Les simplexes abstraits collinéaires ou coplanaires ne sont pas éliminés par un filtre affine; l'indépendance affine n'est qu'une condition nécessaire d'entrée dans une nouvelle frontière, avant bon centrage strict et classification globale.

Sous `RelevantGP`, pour tout support propre utile vérifiant $\lvert T\rvert+\lvert I\rvert\leq s_{\max}$, on a $E=\varnothing$. Le ball-record engendre alors un unique simplexe abstrait Gabriel $S=T\cup I$, de cardinal $R$ et source dans $\Gamma_{R-1}$. Cela supprime l'expansion combinatoire locale d'un record; cela ne borne pas le nombre de produits des frontières indépendantes, qui peut encore atteindre $\binom{n}{3}+\binom{n}{4}$. La borne $\lvert T\rvert\leq4$ porte sur le support, pas sur le simplexe abstrait, dont la cardinalité peut atteindre $s_{\max}\leq K_{\max}+1$. L'unicité du simplexe porté ne fixe pas son rôle Morse : une sphère critique régulière de rang $R$ peut fournir une coface/selle à l'ordre $R-1$ et une naissance à l'ordre $R$ lorsque cet ordre appartient à la fenêtre.

## 8. Frontières indépendantes des triangles et tétraèdres

Le catalogue de paires ne propose pas exhaustivement les supports propres de taille trois. La fixture `hartigan_triangle_all_side_ranks_above_k.json` donne un triangle aigu de rang trois dont chaque côté a rang quatre et qu’aucune fermeture de sous-arêtes de paires de rang au plus trois ne retrouve.

Dans la fenêtre de rang et pour les événements utiles sous `RelevantGP`, la cascade complète est donc :

| frontière | candidats admis | filtre constant avant rang | cas déjà traité |
|---|---|---|---|
| support 2 | paires diamétrales | aucun | aucun |
| support 3 | triangles affinement indépendants | trois produits scalaires strictement positifs | droits, obtus et dégénérés réduits au support 2 |
| support 4 | tétraèdres affinement indépendants | quatre poids barycentriques du centre circonscrit strictement positifs | centre sur la frontière ou hors de l'enveloppe convexe du tétraèdre, réduit au support 2 ou 3 |

Hors de ce domaine conditionnel, un support inférieur peut avoir un rang fermé hors fenêtre tout en portant un petit simplexe Gabriel dégénéré; le catalogue borné ne doit alors pas revendiquer cette réduction. Les frontières trois et quatre utilisent une partition canonique de produits LBVH, décrite dans la [preuve dédiée](math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md). Pour un support utile de taille $m\leq s_{\max}$, $t_m=s_{\max}-m+1$ témoins strictement intérieurs ferment le prune de pertinence. L'ordre des filtres dans un warp est : domaine et ownership, impossibilité affine sur AABB, impossibilité d'être aigu ou bien centré, prune de rang par témoins stricts, tuple feuille exact, puis classification fermée globale. Les filtres bon marché précèdent toujours le calcul d'un centre ou la requête de rang.

Chaque support minimal accepté produit directement un ball-record `(T,I,E,R)` et route chaque sous-simplexe Gabriel de cardinal $q$ vers $k=q-1$; pour un record régulier certifié par `RelevantGP`, $E=\varnothing$, $q=R$ et le routage se réduit à $k=R-1$. Les records support 2 réduisent les triangles/tétraèdres non propres; les records support 3 réduisent les tétraèdres dont la miniboule est portée par une face. La frontière support 4 ne cherche donc que les tétraèdres bien centrés. Les tuples utilisent des clés canoniques exactes. Sous le contrat courant `RelevantGP`, une cosphéricité utile multi-supports produit `unsupported_degeneracy`; une future extension dégénérée devra fusionner la boule, son intérieur, son shell et tous ses supports minimaux, puis dédupliquer les $Q$ engendrés, jamais choisir silencieusement un seul support.

Même exhaustif, ce flot de Gabriel brut ne remplace pas à lui seul Gamma exhaustif : les incidences silencieuses et non-Gabriel restent une obligation séparée de la réduction `hgp_reduced`. Tant qu'elles ne sont pas générées et certifiées et que l'obligation M.1 n'est pas fermée, la sortie reste `gabriel_positive_connectivity` ou `partial_refinement`, jamais une hiérarchie `hgp_reduced` exacte. La géométrie, la réduction hiérarchique et le statut public ne sont jamais confondus.

## 9. Mémoire et passage à l’échelle

À 50 000 points, coordonnées, permutation Morton et nœuds LBVH restent résidents avec les tuiles de travail, les banques $O(B\mathbin{\cdot}48\mathbin{\cdot}K_{\max})$, les files exactes et une sortie explicitement capée. Le target n’alloue ni matrice de paires, ni table globale de témoins, ni univers de triplets ou quadruplets.

À 10 M–30 M, le nuage et le LBVH restent résidents seulement si le préflight le démontre. Les ancres, frontières de supports, candidats et sorties sont streamés par plages Morton. Un chunk n’est libéré qu’après publication atomique de l’une de ces issues : prune exact avec masse, records exacts compactés, ou frontière reprenable. Un halo ou une fenêtre locale ne clôt jamais un chunk.

Le coût de sortie et le pire cas du parcours peuvent être quadratiques dès les paires. Cette possibilité ne justifie aucun travail quadratique anticipé : sur les profils favorables, la masse doit être fermée principalement par des prunes de régions, et la croissance à 12 500 points doit falsifier toute ordonnance pratiquement dense avant le gate 50 000. Le contrat de temps porte donc sur des familles enregistrées et output-bornées, avec caps séparés pour :

- nœuds visités et tests de témoins;
- candidats paires, triangles et tétraèdres;
- records par rang et références de payload;
- octets de tri, de frontière et de sortie;
- prédicats exacts rares.

Dépasser un cap produit `budget_exhausted`; aucune lease n'est alors publiée comme exhaustive, et le producteur conserve une frontière reprenable ou un résultat partiel explicitement non exact. Il n'existe aucun fallback qui classifie les paires restantes une par une.

## 10. Gate de 100 ms

Le SLO principal est le p95 `warm_e2e` sous 100 ms pour $n=50\,000$, $K_{\max}=10$, sur chaque famille favorable enregistrée. Il inclut validation, H2D, Morton/LBVH, banques de témoins, paires, supports trois/quatre, compactage, réduction requise et sortie. `resident_core` et le temps d’un kernel isolé restent des diagnostics secondaires.

Le budget d’ingénierie initial, non mesuré et non qualifiant, réserve au plus 25 ms à l’entrée plus LBVH, 45 ms au catalogue de paires, 20 ms aux supports trois/quatre et 10 ms au compactage/réduction/sortie. La construction LBVH produit doit tenir en quelques kernels de type Karras, sans boucle de synchronisations par niveau. Un étage qui dépasse son enveloppe doit réduire son travail mesuré avant toute escalade; déplacer son coût hors du chronomètre est interdit.

Chaque rapport publie p50/p95 et, séparément :

- H2D, Morton, radix sort, topologie et AABB;
- amorçage Morton, chambres remplies, rayons et taux de prune Yao48;
- nœuds et témoins testés, candidats avant/après filtre inverse;
- paires classifiées, buckets de rang, payload et files exactes;
- produits supports 3/4, rejets affine/aigu/barycentrique, feuilles et records;
- scans, tris, déduplication, réduction, D2H et pic mémoire.

Le premier benchmark GCP n’est justifié qu’après existence du kernel résident de paires, différentiel borné complet et test fake du contrat. Toute session emploie exclusivement les scripts Spot gardés du dépôt et certifie l’arrêt ciblé.

## 11. Ordre d’implémentation

1. poser le contrat `RankedDiametralPairCatalogContext`, sa lease SoA et un fake-launcher hostile;
2. généraliser la preuve Yao48 à des témoins quelconques et ajouter la variante stricte;
3. implémenter le signe exact GPU de $\Phi$;
4. porter la frontière fusionnée `morton_yao48_pair_frontier`, où l'ownership Morton, le remplissage des 48 banques et les prunes de régions partagent un parcours; interdire toute émission préalable du préfixe brut;
5. raccorder classification, CSR, reçus closed/GP et différentiels $n\leq512$;
6. fermer `candidate + certified_pruned + unresolved = n(n-1)/2`, montrer un résidu nul, mesurer la croissance et seulement alors lancer un gate G4 gardé;
7. implémenter la frontière indépendante des triangles aigus;
8. réutiliser son flux pour réduire les tétraèdres support-3, puis ouvrir les seuls support-4 bien centrés;
9. générer et certifier exhaustivement les incidences silencieuses et non-Gabriel nécessaires, puis fermer l'obligation M.1;
10. raccorder la réduction sparse et qualifier le `warm_e2e` complet;
11. seulement après le gate 50 k, ouvrir les chunks 1 M, 10 M et 30 M.

Les oracles CPU exhaustifs et le futur oracle GPU dense borné falsifient chaque étape sur petits nuages. Ils ne deviennent jamais le backend produit, même sous un autre nom.
