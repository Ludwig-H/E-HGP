# Progression Phase 5 — ancre exacte $k=1$ et EMST

## Statut

- phase : `5`, en cours;
- backend de proposition : `cuda_g4` (chemin à graine fixe, émission chunkée et graine Morton bornée qualifiés sur G4; profils de travail Morton et external-1NN exact exécutés séparément comme benchmarks sans revendication scientifique);
- backend de rejeu indépendant des propositions : `cuda_g4` (replay complet chunké et rejeu à politique Morton séparée qualifiés sur G4);
- backend de recertification, décision et contraction : `reference_cpu`;
- profil : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; l'émission chunkée, le resserrement Morton et leurs rejeux CUDA réels sont qualifiés sur G4; le branch-and-bound external-1NN exact supprime le payload candidat et son benchmark G4 observe jusqu'à 16 384 points une croissance très inférieure au producteur exhaustif, mais une famille dyadique adversariale établit un travail $\Omega(n^2)$ pour sa frontière point--LBVH indépendante par source. Un parcours self-dual partagé exact est maintenant certifié sur hôte, neutralise le bloc adversarial construit et ferme une chaîne Borůvka locale avec rejeu frais; sa pile DFS est bornée par la profondeur du LBVH et ses visites par $n(n+1)-1$, sans borne sous-quadratique. Cette chaîne réduit désormais les graines directement par composante sous une enveloppe figée et ne crée aucun minimum ponctuel. La réduction explicite ferme une forêt compacte locale $k=1$; la qualification CUDA du parcours partagé, son profil de travail, les ordres supérieurs, les applications verticales et le statut public global restent ouverts.

Les cinq premiers jalons CPU de Phase 5 livrent l'ancre EMST exacte, le catalogue exhaustif des paires, la réduction rang-deux du backend CPU, leur différentiel indépendant jusqu'à $n=14$, la forêt hiérarchique compacte construite depuis un EMST déjà certifié et un producteur Borůvka exact sur le LBVH global. La voie hybride enchaîne maintenant les rondes stackless dans un contexte de proposition résident, décide chaque minimum par le CPU exact, contracte canoniquement les composantes et vérifie le témoin EMST complet par un second rejeu GPU et l'ancre CPU. La boucle complète, son overload chunké à graine fixe et la graine locale Morton bornée sont qualifiés sur G4 pour un témoin EMST local, avec politiques de confiance fournies séparément au vérificateur et pic physique du payload candidat certifié. Le profil de travail final mesure ensuite ce même producteur sans oracle indépendant et confirme que la graine Morton réduit fortement les constantes mais ne rend pas les familles uniformes ou en amas scalables. La voie self-dual hôte possède désormais sa propre chaîne multi-ronde directe : chaque ronde recertifie les graines, les réduit par composante, reconstruit l'enveloppe figée, décide seulement les minima de composantes puis détruit toute la frontière; seuls les deux audits, ces minima et la contraction persistent. Un contexte neuf reproduit la même chaîne directe et l'ancre CPU recertifie séparément le témoin. L'adaptateur explicite recertifie le témoin external-1NN historique, construit `K1CompactForest` et lui attribue seulement `compact_k1_forest_certified` dans le scope `local_k1_compact_forest_only`. Le résultat EMST source reste immuable avec `hierarchy_reduction_status=not_performed` et `scientific_status=local_emst_witness_only`. Aucun de ces jalons ne ferme la Phase 5 ou la porte globale G2 du catalogue, ni ne publie `public_status=exact` pour une tour MorseHGP3D complète.

## Ancre mathématique

Le nuage est d'abord transformé en `CanonicalPointCloud`; les sommets du graphe complet sont donc les `PointId` canoniques $0,\ldots,n-1$. Pour chaque paire distincte $(u,v)$, le noyau recalcule exactement la longueur carrée dyadique $d^2(u,v)$ et lui associe le niveau HGP $a(u,v)=d^2(u,v)/4$.

Toutes les fractions passent par `ExactRational` et tous les niveaux par `ExactLevel`. Le facteur $1/4$, les comparaisons, l'égalité des poids et le poids total sont donc rationnels exacts; aucune tolérance flottante n'intervient.

Les $n$ minima de rang un sont matérialisés comme feuilles singleton de niveau zéro. Pour $n=1$, la racine est cette feuille, le graphe et l'EMST sont vides, la coupe stricte au niveau zéro est vide et la coupe fermée contient le singleton.

## Lots égaux et multifusions

Les arêtes du graphe complet sont triées par `(longueur carrée exacte, u, v)`, puis groupées par égalité rationnelle exacte. Pour chaque lot :

1. les composantes strictement antérieures sont figées;
2. toutes les arêtes du lot sont projetées sur ces racines;
3. chaque composante connexe du graphe quotient devient une unique multifusion canonique;
4. les enfants sont ordonnés par leur couverture canonique en `PointId`;
5. Kruskal sélectionne ensuite un représentant déterministe de l'EMST;
6. les unions globales sont appliquées et l'état fermé du lot est enregistré.

La suite binaire des unions de Kruskal ne définit donc jamais la généalogie publique. Un chemin de six points séparés par la même longueur produit un seul nœud d'arité six, pas cinq fusions binaires dépendantes de l'ordre.

En présence d'ex æquo, le choix déterministe des arêtes permet le rejeu et l'audit, mais l'identité scientifique repose sur les coupes exactes, le poids total et les multifusions canoniques. Le résultat conserve aussi toutes les arêtes du graphe complet et tous les lots, y compris ceux devenus redondants après la connexion de l'arbre.

## API et audit

La cible installable `morsehgp3d::hierarchy` expose `build_exact_complete_graph_emst`, `build_compact_k1_forest`, `build_exact_lbvh_boruvka` et `verify_exact_lbvh_boruvka`. Le résultat de référence complet contient :

- les arêtes complètes et les arêtes EMST avec longueur carrée et niveau HGP;
- les feuilles et nœuds de multifusion de $T_1$;
- les lots égaux avec états pré-lot et post-lot;
- les multifusions avec composantes enfants figées et union fusionnée;
- les poids totaux exacts géométrique et HGP;
- des compteurs de distances, lots, redondances, événements et arités;
- le rejeu des coupes strictes ou fermées depuis le graphe complet ou l'EMST sélectionné.

L'ancre complète est volontairement quadratique : elle matérialise le graphe euclidien complet et sert de vérité terrain exacte CPU. Cette portée est distincte de l'adaptateur compact décrit ci-dessous et de la future voie GPU destinée à plusieurs millions de points.

## Forêt hiérarchique compacte

`K1CompactForest` reçoit les $n-1$ arêtes d'un EMST exact déjà certifié par un producteur géométrique. L'adaptateur brut vérifie le domaine des sommets, la positivité des longueurs, la relation exacte entre longueur carrée et niveau, l'absence de boucle, cycle ou doublon, puis la connexité. Il ne peut pas déduire des seules arêtes que leurs poids sont bien les distances du nuage ni que l'arbre est minimal; ces deux propriétés sont des préconditions explicites. L'overload depuis `K1EmstResult` est la voie ancrée actuelle.

Les feuilles singleton gardent les identifiants de $0$ à $n-1$ mais deviennent implicites. Les données persistantes sont limitées à cinq arènes : niveaux exacts distincts portés par l'EMST, arêtes de l'arbre, nœuds internes, références d'enfants CSR et descripteurs de lots. Ces niveaux sont exactement ceux où la partition change; contrairement à l'ancre par graphe complet, l'arène compacte ne recopie pas les lots de longueurs devenus redondants. Elle reste néanmoins interrogeable à tout seuil exact. Un nœud interne conserve son indice de niveau, sa tranche CSR, son plus petit `PointId` et la taille de sa couverture; aucun vecteur de couverture n'est stocké. Les couvertures, nœuds riches et multifusions restent matérialisables à la demande pour l'oracle et le diagnostic, mais matérialiser tous les nœuds d'une chaîne produit volontairement une sortie en $\Theta(n^2)$ et ne fait pas partie de la voie scalable.

La construction trie d'abord les arêtes par niveau exact puis extrémités, fige les composantes avant chaque lot, construit le quotient du lot entier et alloue un nœud par composante connexe de ce quotient. Elle prend $O(n\log n)$ avec une entrée d'ordre arbitraire et conserve $O(n)$ enregistrements persistants. Si les $m$ nœuds internes ont les arités $a_i\geq2$, l'identité d'un arbre enraciné donne :

$$\sum_{i=1}^{m}(a_i-1)=n-1,\qquad m\leq n-1,\qquad \sum_{i=1}^{m}a_i=n+m-1\leq2n-2.$$

Le nombre de niveaux et de lots est au plus $n-1$. En comptant un enregistrement par arête, niveau, nœud interne, référence d'enfant et lot, les cinq arènes satisfont donc :

$$\lvert E_T\rvert+\lvert L\rvert+m+\sum_{i=1}^{m}a_i+\lvert B\rvert\leq6(n-1).$$

Cette borne porte sur les enregistrements principaux : elle ne compte ni les métadonnées d'allocateur, ni le nombre de limbs des entiers multiprécision. Le compteur exécutable expose la borne $6(n-1)$, sa valeur réellement occupée et `stored_coverage_point_id_count=0`.

Pour la canonisation, les enfants d'une multifusion sont des composantes non vides et deux à deux disjointes du même état pré-lot. Leurs minima sont donc distincts, et l'ordre lexicographique de leurs couvertures triées est exactement l'ordre de leur plus petit `PointId`. Un seul identifiant et une seule taille remplacent ainsi la copie de chaque couverture sans modifier l'ordre public des nœuds.

Enfin, si $T$ est un EMST exact du graphe euclidien complet $G$, leurs partitions coïncident de chaque côté de tout seuil :

$$\pi_0(T_{<\lambda})=\pi_0(G_{<\lambda}),\qquad \pi_0(T_{\leq\lambda})=\pi_0(G_{\leq\lambda}).$$

En effet, si un chemin actif de $G$ reliait deux composantes encore séparées dans $T$, l'une de ses arêtes traverserait la coupe créée par une arête inactive du chemin de $T$. L'échange remplacerait cette dernière par une arête strictement plus légère, en contradiction avec la minimalité de $T$. Les deux partitions encadrant un lot déterminent son graphe quotient et ses multifusions; deux témoins EMST différents sous ex æquo produisent donc les mêmes nœuds, coupes et multifusions canoniques, même si leurs arêtes sélectionnées diffèrent.

## Producteur Borůvka exact sur le LBVH

`build_exact_lbvh_boruvka` remplace la matérialisation du graphe complet par des recherches point-vers-LBVH exactes sur l'index Morton global déjà construit. La ronde $r$ fige les composantes et leur label canonique, égal au plus petit `PointId`; elle ne modifie ces labels qu'après avoir déterminé tous les minima sortants. Pour une arête aux extrémités canoniques $u<v$, l'ordre total est

$$\kappa(e)=\bigl(d^2(e),u,v\bigr).$$

Chaque composante choisit son unique arête sortante minimale pour $\kappa$. Cette arête est le minimum unique de la coupe définie par la composante; la propriété de coupe l'inclut donc dans l'arbre de Kruskal canonique $T^{\star}$ construit avec le même ordre total. Par induction sur les rondes, toutes les arêtes acceptées appartiennent à $T^{\star}$, et leur ensemble dédupliqué est une forêt. Tant qu'il reste $c_r>1$ composantes, chacune est incidente à une arête choisie; aucune composante contractée n'est isolée dans cette forêt. Toute composante post-ronde contient donc au moins deux composantes pré-ronde, d'où

$$c_{r+1}\leq\left\lfloor\frac{c_r}{2}\right\rfloor,$$

et au plus $\lceil\log_2(n)\rceil$ rondes pour $n\geq2$. Les arêtes finales sont retriées par $\kappa$ avant leur remise à `K1CompactForest`; l'ordre des rondes Borůvka ne définit jamais les lots égaux ni les multifusions publiques.

À chaque ronde, un passage postordre marque chaque nœud LBVH comme uniforme pour une composante figée ou mixte. Un sous-arbre uniforme de la composante requérante est rejeté sans évaluation de feuilles. Pour les autres nœuds, la borne exacte de distance à l'AABB ne permet un prune que lorsqu'elle est strictement supérieure au meilleur poids courant. Une égalité descend obligatoirement, car une arête de même poids peut encore gagner sur les extrémités de $\kappa$; ni epsilon ni ordre flottant ne décide ce cas.

Le résultat `K1ExactBoruvkaResult` reste volontairement distinct de `K1EmstResult` : il ne matérialise ni graphe complet, ni hiérarchie, ni couverture persistante, et son `emst_witness_certified` est un certificat local, pas un `public_status`. `verify_exact_lbvh_boruvka` rejoue séparément les labels figés, minima de composantes, arêtes acceptées, contractions, poids exacts et borne de rondes depuis le nuage et l'index immuables; il ne fait confiance ni au booléen de résultat, ni aux compteurs fournis. Ce producteur et ce vérificateur forment le socle CPU de correction pour la ronde de propositions GPU stackless; ils ne qualifient encore aucun backend GPU.

## Ronde GPU à graine fixe et sur-ensemble de candidats

`K1BoruvkaCandidateContext` possède son propre contexte résident et ne réutilise pas les files de la Phase 4. À l'entrée d'une ronde, les labels canoniques des composantes sont figés. Pour chaque source $q$, le CPU choisit déterministement un point $s(q)$ d'une autre composante et calcule exactement

$$R_q=d^2(q,s(q)),\qquad A_q=\left\lbrace p\in X:\ell(p)\neq\ell(q),\ d^2(q,p)\leq R_q\right\rbrace.$$

Le véritable minimum sortant de $q$ appartient à $A_q$, puisque $s(q)$ est lui-même une cible sortante admissible. Il suffit donc que le GPU retourne un ensemble $P_q$ vérifiant $A_q\subseteq P_q\subseteq\left\lbrace p:\ell(p)\neq\ell(q)\right\rbrace$. Après validation de cette extériorité et filtrage exact par $d^2(q,p)\leq R_q$, le minimum selon la clé totale $\kappa(e)=(d^2(e),u,v)$ est exactement le minimum sortant de $q$; le minimum des points d'une composante figée est ensuite son minimum Borůvka exact. Cette implication ne donne aucun pouvoir de décision aux distances GPU : elle réduit seulement le domaine que le CPU doit réévaluer.

Le cutoff transmis au device est un majorant binary64 $U_q\geq R_q$, ou $+\infty$ lorsque le niveau exact n'est pas représentable par un nombre fini. Le LBVH immuable reçoit des cordes d'échappement left-first : la corde du fils gauche vise le fils droit, celle du fils droit hérite de la corde du parent et celle de la racine vise la sentinelle. Un passage postordre étiquette en outre chaque nœud comme uniforme pour une composante figée ou mixte.

Pour une source, le noyau rejette directement un sous-arbre uniforme de sa propre composante. Sur les autres nœuds, il calcule une borne inférieure dirigée vers le bas de la distance à l'AABB et ne prune que sous l'inégalité stricte `lower > U_q`; une égalité descend. Toute borne invalide, non finie ou affectée par un dépassement provoque aussi une descente. Ces règles rendent le pruning conservateur par rapport à $A_q$, mais le certificat final ne leur fait pas confiance seules.

En effet, pour toute cible $p\in A_q$ et tout nœud ancêtre $N$ de sa feuille, la borne device valide $\underline{b}(q,N)$ vérifie la chaîne

$$\underline{b}(q,N)\leq b(q,N)\leq d^2(q,p)\leq R_q\leq U_q.$$

Le prune strict est donc impossible sur ce chemin. Aucun ancêtre de $p$ ne peut non plus porter le tag uniforme de la composante de $q$, puisque $\ell(p)\neq\ell(q)$; les cordes left-first atteignent ainsi sa feuille, sauf borne invalide auquel cas la règle impose précisément de descendre. Le passage count inclut donc $p$, et l'identité de parcours avec emit plus le préfixe exact conserve cette inclusion dans la sortie.

La capacité logique de sortie est déterminée sans troncature par deux parcours identiques : un noyau compte les candidats de chaque source, le CPU relit les comptes et construit un préfixe vérifié, puis un second noyau remplit exactement ces segments. Il y a deux lancements et deux synchronisations par ronde, sans compteur global atomique ni synchronisation par requête. La capacité physique privée peut être réutilisée entre les rondes, tandis que l'audit publie la capacité logique exacte du préfixe courant et une époque de tampon strictement croissante.

Cette première construction privilégie le contrat mathématique à la scalabilité : une graine éloignée peut faire descendre presque tout l'arbre pour chaque source, avec

$$\sum_{q\in X}\lvert P_q\rvert\leq n(n-1),$$

et une taille atteignable en $\Theta(n^2)$. Aucun plafond silencieux ne tronque alors le sur-ensemble : un overflow de préfixe ou un échec d'allocation arrête la ronde. Des graines plus serrées et un protocole chunké certifiable restent nécessaires avant toute revendication scalable.

Après le retour device, le CPU valide les offsets, segments, identifiants, composantes, doublons, comptes de passages et invariants de capacité. Il reconstruit ensuite indépendamment $A_q$ par un parcours exact du même LBVH : à une feuille, la borne AABB exacte est la distance exacte au point, et toute cible requise absente invalide la proposition. Ce rejeu établit le contrat de sur-ensemble avant toute résolution. Le CPU réévalue enfin chaque candidat, élimine ceux au-delà de $R_q$ et résout les minima par $\kappa$, d'abord par source puis par composante. Les champs `candidates` et `cpu_exact_component_minima` restent séparés; l'API de ronde ne contracte pas et ne produit aucun statut public. La boucle hybride consomme ensuite ces minima dans une étape CPU exacte distincte.

Le cas terminal à une composante renvoie une proposition vide certifiée sans lancer de noyau. Une faute du lanceur empoisonne seulement le contexte concerné; les appels ultérieurs sur ce contexte échouent fermés, tandis qu'un autre contexte reste indépendant.

## Boucle hybride complète et contraction exacte

`build_gpu_proposed_cpu_exact_k1_boruvka` conserve un unique `K1BoruvkaCandidateContext` pendant toutes les rondes productrices non terminales. À la ronde $r$, il fournit les labels canoniques figés au producteur GPU, remet les minima de composantes décidés selon $\kappa$ à `contract_exact_k1_boruvka_round`, puis remplace seulement après cette décision les labels par le plus petit `PointId` de chaque nouvelle composante. Le réducteur est volontairement pur et CPU : il valide la partition figée, la source et la géométrie exacte de chaque minimum déjà certifié, trie et déduplique les arêtes, rejette tout cycle, ferme la réduction par moitié et renvoie les nouveaux labels. Il ne prétend pas décider lui-même la minimalité selon $\kappa$; cette obligation reste dans la décision exacte qui le précède.

Le payload persistant d'une ronde sépare trois objets et trois statuts fail-closed : l'audit du sur-ensemble GPU, les minima exacts CPU, puis les arêtes et le compte issus de la contraction canonique. Les graines, offsets et candidats complets meurent à la fin de la ronde. Si $c_r$ est le nombre de composantes au début de la ronde productrice et $R$ le nombre de rondes, les bornes de Borůvka donnent

$$R\leq\lceil\log_2(n)\rceil,\qquad \sum_{r=0}^{R-1}c_r<2n,\qquad \sum_{r=0}^{R-1}(c_r-c_{r+1})=n-1.$$

Les minima et arêtes persistants sont donc linéaires en nombre d'enregistrements, indépendamment de l'ordre des rondes. Le vecteur courant de labels occupe $O(n)$, mais le pic éphémère des candidats reste quadratique dans le pire cas : le jalon ferme la boucle mathématique, pas encore la mémoire scalable.

Le résultat final retrie les $n-1$ arêtes par $\kappa$, additionne exactement leurs longueurs carrées et niveaux HGP, et porte `proof_basis=gpu_candidate_superset_cpu_exact_boruvka_v1`. Il reste un témoin EMST local : `hierarchy_reduction_status=not_performed` et `scientific_status=local_emst_witness_only`. La construction de `K1CompactForest` demeure un appel explicite ultérieur; aucune généalogie, multifusion ni valeur `public_status` n'est publiée par la boucle.

`verify_gpu_proposed_cpu_exact_k1_boruvka` ne fait confiance ni aux drapeaux, ni aux statuts, ni aux audits, ni aux compteurs reçus. Il crée un second contexte GPU, rejoue les propositions depuis les labels identité, recertifie les payloads bruts éphémères et compare séparément l'audit de proposition et les minima exacts. Il reconstruit aussi l'ancre Borůvka CPU, rejoue les contractions depuis le nuage et recalcule les deux poids exacts. Ce second passage certifie que la chaîne enregistrée est reproductible par le backend courant; il ne constitue pas une attestation cryptographique de provenance historique. Les compteurs du résultat décrivent seulement la chaîne productrice, tandis que le rapport du vérificateur expose séparément ses rondes, minima, lancements et synchronisations.

## Émission chunkée par sources complètes

Pour une ronde aux labels figés $\ell$, la source $q$ reçoit une graine externe $s(q)$ et le rayon exact $R_q=d^2(q,s(q))$. L'ensemble que le producteur doit couvrir est

$$A_q=\left\lbrace p:\ell(p)\neq\ell(q),\ d^2(q,p)\leq R_q\right\rbrace.$$

Le GPU émet un sur-ensemble $P_q$ de cibles externes. La recertification CPU vérifie $A_q\subseteq P_q$, filtre exactement par $d^2(q,p)\leq R_q$, puis minimise selon $\kappa(e)=(d^2(e),u,v)$. La vraie arête minimale sortante de $q$ a un poids au plus $R_q$; elle appartient donc à $A_q$ et le minimum filtré est inchangé. Le minimum des minima des sources d'une composante est associatif et commutatif : les frontières de chunks ne modifient ni la décision, ni la contraction, à condition qu'une source ne soit jamais scindée et que les labels restent figés jusqu'à la fermeture de la ronde.

L'implémentation fait un passage `count` global, partitionne ensuite l'intervalle des sources en plages contiguës complètes et exécute un passage `emit` par plage. Chaque callback synchrone valide offsets, identifiants, extériorité et doublons, recertifie $A_q$ et accumule seulement un minimum interne. Aucun minimum de composante, aucune arête et aucun nouveau label ne sont retournés avant la réussite du dernier chunk. Une faute tardive détruit l'accumulateur, n'avance pas l'époque et empoisonne le contexte.

Si $c_q=\lvert P_q\rvert$, les trois quantités distinctes sont

$$L_r=\sum_q c_q,\qquad H_r=\max_j\sum_{q\in Q_j}c_q,\qquad M_r=\max_q c_q.$$

$L_r$ est le volume logique de la ronde et peut rester en $\Theta(n^2)$. Le budget $B$ impose $M_r\leq H_r\leq B$; si une source seule dépasse $B$, la ronde échoue sans fallback. Une copie device et une copie hôte de records de seize octets bornent le seul payload candidat simultané par $32B$ octets. Cette borne n'inclut pas le LBVH, les coordonnées, labels, tags, cutoffs, comptes, offsets, plans de chunks, témoins exacts ni limbs multiprécision.

Les anciens champs `gpu_output_capacity`, `gpu_output_capacity_sum` et `peak_gpu_output_capacity` conservent l'étendue logique historique pour comparer les chemins; ils ne sont jamais interprétés comme une capacité physique en mode chunké. Les capacités device et hôte, le pic de chunk et `candidate_payload_peak_bytes` portent seuls le certificat mémoire. De même, `count_emit_cardinality_and_visit_count_certified` affirme l'égalité des cardinalités émises et des nombres de visites, pas une identité d'ensembles; la sûreté scientifique repose sur la recertification exacte de $A_q$.

L'overload chunké de la boucle persiste ces audits par ronde et les agrège séparément. Son vérificateur reçoit la politique de budget comme paramètre de confiance, la compare au résultat non fiable avant toute allocation, puis rejoue toute la chaîne dans un contexte frais. La chaîne $8\to4\to2\to1$ passe avec $L_r>H_r$, mêmes minima, contractions, arêtes, poids et forêt compacte que l'ancre CPU; un budget sérialisé falsifié est rejeté avant tout rejeu GPU.

Le replay réel v2 au SHA `6d944132d2f7d261a934a1864788c2fb7a81831f` qualifie sur G4 le singleton, cette chaîne et le carré à ex æquo. Pour la chaîne, le budget vaut sept records, le volume logique agrégé vaut 86, seize chunks sont émis, le pic device et hôte vaut sept records et le payload candidat simultané culmine à 224 octets. L'artefact `phase5-k1-boruvka-6d944132d2f7d261a934a1864788c2fb7a81831f.json`, de SHA-256 `c247c1de8dc1a0d6d4aad31fada79c1bc3ca09019146e427cb35fc6ab41d68a4`, ferme AOT `sm_120` sans PTX, `memcheck`, `racecheck`, le replay GPU chunké indépendant et le témoin EMST local. Cette qualification borne le payload candidat sur les fixtures fermées; elle ne prouve ni un volume logique sous-quadratique, ni un travail exact scalable, ni une réduction hiérarchique publique.

## Graine Morton bornée et cutoff exact monotone

Le fallback canonique associe à chaque source $q$ une cible externe $s_0(q)$ et le rayon exact $R_q^0=d^2(q,s_0(q))$. Le nouveau producteur inspecte au plus $W$ feuilles à gauche et $W$ feuilles à droite de $q$ dans l'ordre Morton. Il ignore les feuilles de la composante figée et son noyau désigne heuristiquement un `PointId` externe $t_W(q)$ par distance carrée binary64 RN puis `PointId`. Le certificat ne revendique pas que ce classement flottant est un argmin fiable : il utilise seulement l'appartenance de la cible à la fenêtre, son extériorité recertifiée et la comparaison exacte qui suit.

Le CPU vérifie de nouveau que la cible proposée appartient à la fenêtre de rayon $W$, qu'elle est dans le domaine et qu'elle est externe. Il recalcule ensuite exactement les deux arêtes et choisit selon la clé canonique totale :

$$s_W(q)=\min_{\kappa}\left\lbrace s_0(q),t_W(q)\right\rbrace,\qquad R_q^W=d^2(q,s_W(q))\leq R_q^0.$$

Si aucune cible externe n'est proposée, le fallback est conservé. Dans tous les cas, $s_W(q)$ reste une cible externe; si $m(q)$ est la vraie cible sortante minimale, alors :

$$d^2(q,m(q))\leq R_q^W\leq R_q^0.$$

L'ensemble requis $A_q^W=\left\lbrace p:\ell(p)\neq\ell(q),\ d^2(q,p)\leq R_q^W\right\rbrace$ contient donc toujours la vraie cible minimale. Le lemme de sur-ensemble à graine fixe, la recertification exacte des chunks et la minimisation selon $\kappa$ s'appliquent sans changement. Cette monotonie, et non la qualité de la distance GPU, porte la correction.

Le scan GPU de graines inspecte au plus $\min(n-1,2W)$ voisins par source, soit au plus $2Wn$ inspections par ronde. Le wrapper hôte recalcule indépendamment les cardinalités externes par une fenêtre glissante de fréquences de labels en $O(n)$ et recertifie l'appartenance de la cible grâce à la permutation Morton inverse en $O(1)$ par source; il ne rescane pas les $2Wn$ voisins. Le CPU calcule ensuite les $n$ distances exactes du fallback et au plus une distance proposée distincte par source; ce resserrement ajoute donc au plus $2n$ évaluations exactes de graine et $O(n)$ stockage. Il ne donne aucune borne universelle sur $L_r$ : une fenêtre sans cible utile, des ex æquo ou une géométrie adversaire peuvent encore conserver $L_r=\Theta(n^2)$, et la reconstruction exacte de $A_q^W$ reste elle aussi potentiellement quadratique.

L'API persiste séparément le mode de graine, la politique `window_radius`, le nombre de voisins inspectés et externes, les propositions flottantes, les propositions sélectionnées après comparaison exacte, les améliorations strictes, les fallbacks et les évaluations exactes. Le vérificateur exige la politique Morton comme entrée de confiance distincte du résultat non fiable; l'ancien overload qui ne reçoit que le budget de chunks rejette ce mode avant tout nouveau lancement GPU. Un second contexte recalcule les propositions, les audits de graines, les candidats chunkés, les minima et les contractions.

Sur le lanceur hôte, la chaîne $8\to4\to2\to1$ avec budget sept et rayon $W=1$ inspecte 42 voisins sur trois rondes, trouve 22 voisins externes, publie 16 propositions flottantes, en sélectionne exactement 11 qui améliorent strictement le cutoff et conserve 13 fallbacks. Les volumes logiques par ronde valent 8, 16 et 17, soit 41 au total contre 86 pour le fallback fixe; neuf chunks remplacent les seize chunks historiques, avec le même pic de sept records. Les décisions exactes, contractions, arêtes EMST et poids restent identiques. Ces nombres sont une observation de fixture, pas une revendication asymptotique.

Le chemin CUDA utilise des opérations binary64 RN explicites, un thread par position Morton et des buffers résidents pour l'ordre et les records de proposition. Le replay v2 historique reste byte-identique avec le SHA-256 `9e686fc8147bf43860180f723c5005eb6bbb9277b06ca33e5521d370db56e121`; le replay v3 ajoute une quatrième fixture dédiée sans modifier les trois fixtures historiques et possède le SHA-256 canonique `d30760c4cd19743f4587dac6024cdf86b9d3a5a450f2923e811ab74be61edf71`.

La chaîne matérielle est qualifiée au SHA `7c4933b678cbc6d9860e33596522ab971c0c5df5` sur la G4 Blackwell gardée. L'audit ferme un ELF CUDA 12.9 exclusivement `sm_120`, zéro entrée PTX, `memcheck`, `racecheck`, la couverture complète des sources, la fenêtre bornée, la recertification externe, le cutoff exact monotone et un rejeu GPU Morton indépendant. La fixture de rayon un retrouve les comptes hôte `42/22/16/11/13/36` et la réduction `86/16` vers `41/9`, avec décisions, contractions, arêtes EMST et poids exacts inchangés. L'artefact `phase5-k1-boruvka-7c4933b678cbc6d9860e33596522ab971c0c5df5.json`, de SHA-256 `b0ef2101bb37bacffaffffc4051ea5219aa8b79fda0a5fe9510d58467ebd7a01`, conserve `scientific_result_claimed=false`, `scientific_public_status=null` et `hierarchy_reduction_status=not_performed`; il qualifie ce chemin logiciel sur les fixtures fermées, pas sa scalabilité ni une hiérarchie publique.

## Profil de travail Morton sur G4

La campagne finale au SHA propre `4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed` mesure le producteur CUDA sur les familles déterministes `uniform`, `clusters` et `lattice`, pour $n\in\left\lbrace64,256,1024\right\rbrace$, la graine un, le budget $B=n-1$ et les rayons $W\in\left\lbrace1,4,16\right\rbrace$. Les neuf mesures comparent chaque rayon au fallback canonique, conservent les mêmes décisions exactes, contractions, arêtes EMST et poids, et publient séparément $L$, $H$, $M$, les visites LBVH, les bornes AABB exactes et les distances exactes.

L'artefact `phase5-k1-boruvka-work-profile-4cbdb2bb7f0fb9decc9ede1c9a313727cb8b93ed.json` a le SHA-256 `e39e1355c3b4381858e6bfd1272d7a7048aa1078648a0e8ec8fcc0f372c07e54`. Il lie le binaire de SHA-256 `d47ea05ee5bcefb5f87cff0d0353d2104115a60cf05c78600dc161cfdf39d82a` à l'environnement Phase 3 de SHA-256 `e15597be190786f9cd27107d647d98a943675f4ef5781f617b1ebfd9a21101a7`, à l'image CUDA 12.9.2 épinglée, au pilote `13.0.0` et à la `NVIDIA RTX PRO 6000 Blackwell Server Edition` de capacité `12.0`. Son schéma impose `artifact_role=benchmark_only`, `mode=benchmark`, `qualification_claimed=false`, `scalability_claimed=false`, `scientific_result_claimed=false`, `scientific_public_status=null` et `hierarchy_reduction_status=not_performed`.

Pour comparer les opérations exactes sans prétendre modéliser leur coût en bits, on note $A$ le nombre d'évaluations exactes de bornes AABB, $S$ celui des distances exactes de graines et $X$ leur somme non pondérée avec les distances candidates. Dans cette campagne, chaque candidat logique est recertifié exactement, donc :

$$X=L+A+S.$$

Le tableau donne, pour chaque voie, les triplets aux tailles $(64,256,1024)$. Les valeurs de $H$ sont partout $(63,255,1023)$, sauf `lattice/W1` dont la dernière vaut $1022$; ce pic suit donc le budget linéaire et ne mesure pas à lui seul le volume logique.

| famille | voie | $L$ | $M$ | $X$ |
|---|---|---|---|---|
| `uniform` | fallback | $(9243,166204,3357203)$ | $(62,253,1019)$ | $(32417,556667,10792253)$ |
| `uniform` | $W=1$ | $(3260,53399,1230412)$ | $(59,249,1014)$ | $(15754,209831,4170161)$ |
| `uniform` | $W=4$ | $(854,18041,554363)$ | $(40,227,1000)$ | $(8923,98646,2021279)$ |
| `uniform` | $W=16$ | $(322,4692,216143)$ | $(5,144,944)$ | $(6916,52982,938791)$ |
| `clusters` | fallback | $(6835,150144,2971082)$ | $(61,243,985)$ | $(23442,477662,9192950)$ |
| `clusters` | $W=1$ | $(3278,79447,1581973)$ | $(56,241,982)$ | $(12948,264487,4979612)$ |
| `clusters` | $W=4$ | $(2145,57692,1025548)$ | $(53,219,968)$ | $(9650,199013,3288600)$ |
| `clusters` | $W=16$ | $(1116,37723,681410)$ | $(30,219,916)$ | $(6662,138998,2240688)$ |
| `lattice` | fallback | $(2807,47098,789072)$ | $(63,255,1023)$ | $(9481,150872,2460880)$ |
| `lattice` | $W=1$ | $(288,1305,5522)$ | $(6,15,10)$ | $(2334,12407,60427)$ |
| `lattice` | $W=4$ | $(288,1280,5516)$ | $(6,6,6)$ | $(2332,12342,60414)$ |
| `lattice` | $W=16$ | $(288,1280,5516)$ | $(6,6,6)$ | $(2332,12342,60414)$ |

Pour un quadruplement de $n$, une croissance par un facteur seize correspond à l'exposant deux. Entre 256 et 1024, les voies Morton `uniform` et `clusters` multiplient $X$ par des facteurs compris entre $16.12$ et $20.49$, soit des exposants empiriques compris entre $2.005$ et $2.178$. À $n=1024$, $W=16$ réduit néanmoins $X$ de $91.3\%$ sur `uniform` et de $75.6\%$ sur `clusters`; ces gains restent des réductions de constante, car $M/(n-1)$ vaut encore respectivement $0.923$ et $0.895$. Quelques sources reconstruisent donc presque tout leur ensemble externe.

La famille `lattice` se comporte différemment : pour $W=4$ ou $W=16$, $L/n$ vaut successivement $4.50$, $5.00$ et $5.39$, tandis que $M=6$. Entre 256 et 1024, $L$ croît par un facteur $4.31$ et $X$ par un facteur $4.89$, compatibles sur cette matrice avec un travail proche de $n\log n$. Ce cas favorable ne compense pas la croissance quadratique observée sur les deux familles non structurées et ne constitue aucune borne asymptotique.

La décision empirique était donc de conserver $W=16$ comme initialiseur de cutoff, sans l'interpréter comme primitive scalable, puis d'implémenter une recherche LBVH exacte du plus proche voisin externe par source. Cette recherche devait parcourir les nœuds par branch-and-bound, élaguer les nœuds uniformes internes et certifier exactement qu'une borne point--AABB est strictement supérieure à l'incumbent; une égalité devait descendre pour préserver le tie-break canonique. Elle devait émettre un seul voisin exact par source avant la réduction exacte par composante. Le profil G4 a ensuite mesuré ce chemin, et la section d'obstruction ci-dessous ferme négativement sa prétention sous-quadratique dans le modèle de complexité paramétrique; la réduction hiérarchique reste un objet distinct.

## Résolution external-1NN exacte

`K1BoruvkaCandidateContext::resolve_round_exact_external_1nn` implémente maintenant cette décision pour une partition figée non terminale. Le GPU ne fournit qu'une graine Morton bornée par source; le CPU recertifie cette graine, puis explore le LBVH par borne exacte point--AABB dans une file de priorité. Un nœud uniforme de la composante source est rejeté. Pour tout autre nœud $N$, la borne $\delta(q,N)$ est un minorant exact de chaque distance carrée de $q$ à une feuille de $N$; le nœud n'est donc élagué que si $\delta(q,N)>d^2(q,s)$. Le cas d'égalité descend jusqu'aux feuilles, où l'ordre exact $\kappa=(d^2,u,v)$ choisit l'incumbent. À épuisement de la frontière, celui-ci est ainsi le minimum sortant exact de la source; une seconde réduction par le même ordre donne exactement un minimum par composante figée.

Le résultat sépare `morton_seed_audit`, `search_audit`, les minima par point, les minima par composante et leurs statuts. Il ne conserve aucun payload candidat, ne contracte aucune composante et ne construit aucune hiérarchie. Le test hôte rejoue plusieurs rondes contre `build_exact_lbvh_boruvka`, couvre un carré à ex æquo et reconstruit le même témoin EMST après contractions explicites. La sortie persistante de chaque ronde est linéaire. Le proxy exact possède la borne générale $X_r\leq3n^2$ par ronde et $X\leq3n^2\lceil\log_2 n\rceil$ sur la chaîne; la famille adversariale ci-dessous atteint déjà $\Omega(n^2)$ au premier tour pour ce parcours. Ce jalon prouve donc la décision exacte, pas sa scalabilité.

`build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka` intègre ensuite cette résolution à toutes les rondes. Un contexte résident produit les graines, les minima par point meurent après leur réduction, puis `contract_exact_k1_boruvka_round` réalise séparément la contraction canonique. Le résultat persiste seulement les audits de graine et de recherche, les minima de composantes, les arêtes acceptées et les compteurs agrégés. Avant de certifier le témoin EMST local, le constructeur crée un nouveau contexte, rejoue toutes les graines et recherches exactes avec la politique Morton fournie séparément, recontracte les composantes, puis compare chaque décision, chaque contraction, l'arbre et les poids à `build_exact_lbvh_boruvka`.

Le test hôte ferme le singleton sans lancement et la chaîne $8\to4\to2\to1$ : trois rondes, 24 requêtes ponctuelles, 14 minima de composantes et sept arêtes. Il falsifie séparément l'audit Morton, l'épuisement de frontière exact, la décision, la contraction et les compteurs agrégés; chaque couche conserve son certificat propre quand elle n'est pas touchée, tandis que le certificat final tombe. Une politique de rayon différente échoue avant le rejeu GPU. Cette chaîne supprime le payload candidat et garde une sortie persistante linéaire, mais sa qualification CUDA réelle et son profil de travail restent ouverts; `hierarchy_reduction_status=not_performed` et aucun statut public n'est produit.

`gpu_k1_boruvka_exact_search_work_profile` instrumente maintenant la nouvelle chaîne sans modifier sa décision. Son schéma `morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v1` publie par ronde et au total les visites de nœuds, expansions internes, bornes AABB exactes, distances ponctuelles exactes, réutilisations de la distance de graine, élagages uniformes et stricts, pire source et pic de frontière. Il conserve `artifact_role=benchmark_only`, `qualification_claimed=false`, `scalability_claimed=false`, `scientific_result_claimed=false`, `scientific_public_status=null` et précise que les compteurs couvrent la chaîne productrice, hors coût du rejeu frais et de l'ancre. Les cibles hôte stricte et CUDA sont câblées.

La matrice hôte propre au SHA `88b02383760880acef8af0db9cce67e3540abff6` emploie la graine un, $W=16$, les familles `uniform`, `clusters`, `lattice` et les tailles $(64,256,1024,4096)$. On note $X$ la somme non pondérée des distances exactes de graines, des bornes AABB exactes et des distances ponctuelles exactes; ce proxy ne modélise pas leur coût en bits. Le tableau donne aussi le maximum de visites d'un LBVH par source $V_{\max}$, le pic de frontière $F_{\max}$ et le nombre total $C$ de minima de composantes persistés à $n=4096$.

| famille | $X$ | $V_{\max}$ | $F_{\max}$ | $C$ à 4096 |
|---|---|---|---|---:|
| `uniform` | $(6318,37904,238745,1380236)$ | $(40,79,117,172)$ | $(19,38,54,80)$ | 5690 |
| `clusters` | $(4029,26996,171832,1140347)$ | $(20,37,66,88)$ | $(10,17,33,44)$ | 5814 |
| `lattice` | $(2268,12086,59390,272892)$ | $(42,56,78,90)$ | $(16,23,34,40)$ | 4096 |

À chaque quadruplement, les exposants empiriques de $X$ restent dans $[1.266,1.328]$ pour `uniform`, $[1.335,1.372]$ pour `clusters` et $[1.100,1.207]$ pour `lattice`. Entre 1024 et 4096, $X$ est multiplié respectivement par $5.781$, $6.636$ et $4.595$, très loin des facteurs quadratiques seize à vingt observés avec la reconstruction exhaustive des candidats. Les douze exécutions ferment le rejeu frais, l'ancre CPU, zéro record candidat et `hierarchy_reduction_status=not_performed`. Cette observation sur `fake_gpu`, un seul seed et 4096 points ne prouvait aucune complexité sous-quadratique; elle justifiait à ce stade de profiler la même voie plus loin sur G4.

## Profil external-1NN exact sur G4

Le benchmark gardé au SHA propre `a81d8e50e4655a2f1b6acad74bbffddbc98ff0ba` exécute le même schéma avec le backend de graines `cuda_g4`, $W=16$, la graine un et la matrice fermée $(1024,4096,16384)\times(\text{uniform},\text{clusters},\text{lattice})$. Chaque journal brut est validé avant assemblage; les neuf mesures ferment le rejeu frais, l'ancre CPU, tous les certificats locaux, zéro record candidat et `hierarchy_reduction_status=not_performed`. Les comptes à 1024 et 4096 reproduisent exactement le profil hôte.

| famille | $X$ | $V_{\max}$ | $F_{\max}$ | $D_{\max}$ à 16 384 | rondes à 16 384 |
|---|---|---|---|---:|---:|
| `uniform` | $(238745,1380236,7704162)$ | $(117,172,235)$ | $(54,80,93)$ | 1 | 7 |
| `clusters` | $(171832,1140347,5835914)$ | $(66,88,138)$ | $(33,44,69)$ | 1 | 8 |
| `lattice` | $(59390,272892,1236596)$ | $(78,90,112)$ | $(34,40,51)$ | 5 | 1 |

Ici $D_{\max}$ est le maximum de distances point--point exactes recalculées pour une source, hors distance de graine déjà recertifiée. Entre 4096 et 16 384, les facteurs de croissance de $X$ sont respectivement $5.582$, $5.118$ et $4.531$, soit les exposants empiriques $1.240$, $1.178$ et $1.090$. Sur l'intervalle complet de facteur seize, les exposants valent $1.253$, $1.271$ et $1.095$. Les maxima ponctuels restent 235 visites et 93 éléments de frontière.

L'artefact `phase5-k1-boruvka-exact-search-work-profile-a81d8e50e4655a2f1b6acad74bbffddbc98ff0ba.json` a le SHA-256 `8c66a5027ff446a6df43faa87ff7da5360e4b9479e662cbb3637dde925d54a10`; son environnement Phase 3 a le SHA-256 `da73d41d19f6ef0b09c77ab66262bec5171b20d74826b201d4c1241a12ee3a9b` et son binaire le SHA-256 `ec8f9113903203b25a22152298278705cd117dba407edcc16a8c26b5e571308b`. Il conserve `artifact_role=benchmark_only`, `qualification_claimed=false`, `scalability_claimed=false`, `scientific_result_claimed=false` et `scientific_public_status=null`. Trois familles, une graine et trois tailles ne prouvent ni une borne déterministe sous-quadratique ni un comportement adversaire. La famille exacte suivante invalide maintenant toute extrapolation worst-case de ces mesures favorables; une stratégie partagée, batchée ou dual-tree redevient nécessaire avant toute prétention scalable.

## Obstruction quadratique du parcours external-1NN par source

La portée est strictement `resolve_round_exact_external_1nn` dans sa forme actuelle : une file point--LBVH indépendante pour chaque source. Le résultat ne vise ni external-1NN en général, ni Borůvka ou EMST en général, ni un futur parcours partagé, batché ou dual-tree.

Dans un modèle dyadique à précision suffisante, prenons $s=2^k$ avec $k\geq2$, $g=s^2$, $d=\frac{1}{2s}$ et $n=4g+2$. Le nuage contient $o=(0,0,0)$, $z=(2^{24}d,2^{24},2^{24})$, les gardes $a_i=(x_i,9,9)$ et $b_i=(x_i,11,11)$ avec $x_i=9d+\frac{(i+1)d}{8g}$ pour $0\leq i<g$, et les requêtes $q_{j,\ell}=(9d+\frac{d}{4},\frac{19}{2}+jd,\frac{19}{2}+\ell d)$ pour $0\leq j<s$ et $0\leq\ell<2s$.

L'AABB global est porté par $o$ et $z$. Les $4g$ points centraux quantifient exactement en coordonnées Morton $(1,1,1)$, donc au code $7$; l'ordre canonique place d'abord les $2g$ gardes par paires $(a_i,b_i)$, puis les $2g$ requêtes. Comme le groupe de collision a une taille puissance de deux, les coupures médianes du LBVH font de chaque paire de gardes deux feuilles sœurs.

Chaque requête possède une requête voisine à distance carrée exacte $d^2$. Toute autre requête est à distance carrée au moins $d^2$ puisque la grille a pour maille $d$; toute garde est à distance carrée au moins $1/2$, tandis que $d^2\leq1/64$, et les deux extrêmes sont encore plus éloignés. Le minimum externe exact de chaque requête vaut donc $d^2$ et tout incumbent valide pour cette source vaut au moins $d^2$. L'AABB de la paire $(a_i,b_i)$ contient les coordonnées $y,z$ de toute requête et sa borne carrée vaut $\Delta_i^2$ avec $\Delta_i=\frac{d}{4}-\frac{(i+1)d}{8g}$, donc $0<\Delta_i^2<\frac{d^2}{16}$. Au premier tour singleton, la paire est mixte et ne satisfait jamais l'élagage strict. Les $2g$ requêtes développent chacune les $g$ paires, d'où au moins $2g^2=\frac{(n-2)^2}{8}$ expansions internes. Le code courant évalue puis empile aussi les deux feuilles avant leur élagage; chaque paire entraîne donc trois visites et trois bornes AABB, soit au moins $6g^2=\frac{3(n-2)^2}{8}$. La constante de visites dépend de cette stratégie d'empilement, tandis que la borne quadratique des expansions est structurelle. Le raisonnement est indépendant du rayon Morton $W\geq1$.

Pour la borne supérieure, soit $X_r$ la somme des distances exactes de graines, des bornes AABB exactes et des distances ponctuelles exactes de la chaîne productrice à une ronde. Il y a au plus $2n$ distances de graines, $n(2n-1)$ bornes de nœuds et $n(n-1)$ distances de feuilles, donc $X_r\leq3n^2$. La contraction divise le nombre de composantes au moins par deux, donc $R\leq\lceil\log_2 n\rceil$ et $X\leq3n^2\lceil\log_2 n\rceil$. Ce proxy exclut le rejeu frais, l'ancre CPU, la construction et le tagging du LBVH, les inspections flottantes Morton, le tas, les contractions et la complexité binaire des rationnels.

Les abscisses $x_i$ demandent jusqu'à $2k+7$ bits significatifs. La famille reste distincte dans l'API binary64 actuelle pour $k\leq23$, mais une famille binary64 littéralement infinie n'existe pas; l'énoncé asymptotique concerne donc le modèle dyadique à précision paramétrique. La fixture hôte permanente exécute $s=4,8,16$ avec $W=1$, certifie le groupe Morton de taille $n-2$, les minima $d^2$, l'accord complet avec l'ancre indépendante à $s=4$, puis les bornes expansions/visites $(512,1536)$, $(8192,24576)$ et $(131072,393216)$. Elle vérifie aussi les trois termes de la borne $X_r\leq3n^2$ sous GCC et Clang stricts. GCP n'a pas été utilisé.

## Parcours self-dual partagé exact

`resolve_round_exact_external_1nn_dual_tree` réutilise le même LBVH mais remplace les $n$ files point--arbre par une pile DFS unique de couples de nœuds, localement ordonnée par borne croissante. Un couple identique se décompose canoniquement en $(L,L)$, $(L,R)$ et $(R,R)$; pour deux nœuds distincts, le code scinde celui dont la diagonale AABB exacte est la plus grande, puis départage par nombre de feuilles et indice. Les intervalles Morton des deux nœuds sont vérifiés disjoints à chaque insertion. Les feuilles non élaguées calculent une seule distance exacte et relâchent simultanément leurs deux minima orientés.

Chaque point $q$ part d'un incumbent externe exact $D_q$ fourni par la graine recertifiée. Chaque nœud $N$ conserve dynamiquement $U_N=\max_{q\in N}D_q$; une amélioration stricte remonte les maxima jusqu'à stabilisation. Pour un couple $(A,B)$, la borne exacte $\delta(A,B)$ minore toutes les distances entre ses feuilles. Si les deux nœuds sont uniformes dans la même composante, toutes leurs paires sont internes. Sinon le couple n'est élagué que sous $\delta(A,B)>\max(U_A,U_B)$; aucune arête qu'il contient ne peut alors améliorer l'une de ses extrémités. Une égalité descend toujours et préserve donc la clé canonique $\kappa=(d^2,u,v)$. Par induction sur la décomposition, les couples fermés et les feuilles évaluées partitionnent exactement les $\binom{n}{2}$ paires non ordonnées. Le compteur de couverture doit égaler cette valeur; les minima par point puis par composante sont ainsi exacts sans payload candidat.

Soit $h(N)$ la hauteur d'un nœud et $H=h(R)$ la hauteur de la racine. Le rang d'un couple vaut $\rho(A,B)=h(A)+h(B)$. Une expansion self-pair remplace une entrée par trois enfants, augmente donc la pile de deux, et diminue le rang d'au moins deux; une expansion de deux nœuds distincts remplace une entrée par deux enfants, augmente la pile d'un et diminue le rang d'au moins un. Une pile DFS ne conserve que les frères en attente le long du chemin actif : depuis le rang racine $2H$, sa cardinalité est donc toujours au plus $2H+1$. L'audit recalcule $H$, réserve cette borne, vérifie chaque insertion et publie `depth_first_frontier_bound_certified`. L'ordre local near-first réduit le travail observé, mais ni l'exactitude ni cette borne mémoire n'en dépendent.

La même décomposition donne une borne générale explicite sur les visites. Tout bloc terminal de couverture positive contient au moins une paire ponctuelle de la partition et leur nombre est donc au plus $\binom{n}{2}$. Les seuls blocs terminaux de couverture nulle sont les $n$ self-pairs de feuilles. Si $L$ est le nombre de feuilles de l'arbre d'expansion, alors $L\leq\binom{n}{2}+n$; chaque nœud interne ayant deux ou trois enfants, son nombre total de nœuds vaut au plus $2L-1$. Le parcours certifie donc $V\leq n(n+1)-1$, recalcule cette valeur avec contrôles de dépassement et échoue fermé si le compteur la dépasse. Cette borne quadratique ferme la comptabilité générale; elle ne prouve aucune scalabilité temporelle.

`resolve_round_exact_component_minima_dual_tree` retire ensuite la couche des minima ponctuels. Pour chaque composante figée $C$, les $n$ graines externes recertifiées sont d'abord réduites sous $\kappa$ en un incumbent faisable $e_C^0$ de cutoff $D_C^0=d^2(e_C^0)$. Chaque feuille $q$ reçoit la valeur de sa composante, puis un postordre construit le snapshot numérique $U_N^0=\max_{C\in S(N)}D_C^0$, où $S(N)$ est l'ensemble des composantes présentes sous $N$. Aucun ensemble $S(N)$ ni propriétaire dynamique n'est matérialisé : les feuilles lisent leur slot dense de composante et les nœuds internes prennent le maximum de leurs deux enfants.

Pendant toute la ronde, ce snapshot reste immuable. Les incumbents de composantes ne peuvent que décroître sous $\kappa$, donc $U_N^0$ demeure un majorant, éventuellement moins sélectif. Un couple uniforme de même composante est interne; tout autre couple n'est élagué que si $\delta(A,B)>\max(U_A^0,U_B^0)$, et l'égalité descend. Chaque paire externe de feuilles relâche directement les deux composantes avec la source orientée vers son extrémité; aucun minimum par point, postings composante--nœud ni remontée dynamique d'ancêtres n'est créé. Le snapshot, les tags et les slots sont reconstruits à chaque appel après contraction : réutiliser un cutoff d'une ronde antérieure serait faux, puisqu'une ancienne arête courte peut être devenue interne.

La sortie locale contient exactement les $c$ minima de composantes et un audit distinct `K1BoruvkaComponentDualTreeSearchAudit`; son état transitoire est $O(n+c)$ et ses visites conservent la borne quadratique précédente. La chaîne $8\to4\to2\to1$ compare à chaque ronde cette décision directe aux deux resolvers historiques et contracte depuis elle. Le carré à ex æquo ferme $\kappa$; une fixture 3D à cinq singletons falsifie le remplacement du maximum par un minimum et l'oubli de la relaxation de la seconde extrémité; `morton_overlap` à $n=66$ force des diminutions au-delà des graines et rejoint l'ancre indépendante. Le constructeur persistant et son rejeu frais utilisent désormais ce resolver direct; seule leur qualification CUDA reste ouverte.

Sur `morton_overlap`, une version à cutoffs initiaux $W=2$ neutralise analytiquement le bloc responsable de la borne précédente. Le sous-arbre des gardes a une diagonale carrée au moins $8$, celui des requêtes une diagonale carrée strictement inférieure à $5/4$; la règle de scission descend donc les gardes jusqu'aux feuilles en gardant les requêtes agrégées. Les cutoffs des gardes valent $(d/(8g))^2$, ceux des requêtes au plus $d^2$, tandis que chaque borne feuille-garde--requêtes vaut au moins $1/2$. Ce bloc demande exactement $2g-1$ expansions, $2g$ prunes, $4g-1$ visites et aucune distance feuille, contre $2g^2$ expansions du parcours indépendant. La fixture exécutable conserve $W=1$ afin d'exercer aussi les diminutions dynamiques; pour $s=4,8,16$, elle exige sur le parcours partagé au plus $48g$ visites, $22g$ expansions, $7g$ distances, $3g$ diminutions strictes et $3g$ mises à jour d'ancêtres, avec les mêmes minima exacts que l'ancien resolver et l'ancre à $s=4$. La pile observée vaut seulement 16, 20 et 24 entrées et reste vérifiée contre $2H+1$; ces plafonds de travail propres aux fixtures ne constituent toujours pas une borne asymptotique.

`build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka` applique le resolver direct à chaque partition figée puis contracte avec `contract_exact_k1_boruvka_round`. Aucun minimum ponctuel ni arbre dynamique d'incumbents n'existe dans cette chaîne; l'enveloppe figée et la file de couples sont transitoires. Le résultat persiste seulement l'audit Morton, l'audit composante, la décision exacte par composante et la contraction canonique, sous `proof_basis=gpu_bounded_morton_seed_cpu_exact_direct_component_dual_tree_boruvka_v2`. Comme chaque décision est le minimum sortant exact de sa coupe sous la clé totale $\kappa$, l'induction Borůvka existante conserve une sous-forêt de l'arbre de Kruskal canonique et divise au moins par deux le nombre de composantes.

Le passage v1 vers v2 change volontairement les types publics `dual_tree_search_audit` et `search_status` de `K1DualTreeExactBoruvkaRound` vers leurs variantes directes par composante. Le nom du constructeur est conservé, mais un consommateur C++ de la v1 doit être recompilé et adapter tout accès aux anciens champs ponctuels; le nouveau `proof_basis` interdit de confondre les deux sémantiques.

Le constructeur ne fait confiance ni à ce premier parcours ni à ses propres drapeaux. `verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka` crée un nouveau contexte, reproduit chaque parcours direct avec la politique Morton fournie séparément, reconstruit l'enveloppe après chaque contraction, puis compare décisions, arêtes et poids à `build_exact_lbvh_boruvka`. Le test hôte ferme le singleton sans lancement et la chaîne $8\to4\to2\to1$ en trois rondes, 14 minima de composantes et sept arêtes; le constructeur effectue trois propositions et trois rejeux frais. Une politique non fiable échoue avant lancement, tandis que les mutations de couverture, borne de visites, certificat d'enveloppe, minimum de composante et statut local font tomber leurs certificats respectifs et le témoin final.

Cette intégration supprime l'obstruction construite pour la frontière indépendante et ferme un témoin EMST local; elle ne prouve aucune borne sous-quadratique générale sur le travail. Les métadonnées résidentes sont linéaires, la pile de couples est bornée par $2H+1$ et les visites par $n(n+1)-1$. La chaîne directe évite désormais tout minimum ponctuel et toute remontée d'incumbent sur la profondeur du LBVH. Son enveloppe figée peut toutefois être moins sélective que l'enveloppe dynamique : cette suppression d'état ne garantit donc pas, à elle seule, une baisse du nombre de visites. Les resolvers historiques restent disponibles comme différentiels hôte, mais le chemin persistant v2 est direct par composante. Il demeure sans qualification CUDA, réduction hiérarchique supplémentaire, statut public ou revendication de scalabilité. GCP n'a pas été utilisé.

## Réduction explicite locale du témoin external-1NN

`build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka` reçoit le nuage, son LBVH, une politique Morton fiable et un `K1SeededExactBoruvkaResult` non fiable. Il commence par rejouer entièrement la chaîne external-1NN et refuse toute construction si le témoin EMST, ses poids, ses contractions ou la séparation de ses statuts ne sont pas recertifiés. Il construit ensuite les cinq arènes de `K1CompactForest` depuis les arêtes recertifiées, sans modifier le résultat source.

`verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka` reconstruit à nouveau une ancre Borůvka exacte depuis le LBVH. Le même réducteur compact est appliqué séparément au témoin reçu et à cette ancre; cette comparaison lie les deux sources exactes, mais ne prétend pas constituer un second oracle logiciel du réducteur. Le vérificateur compare les arêtes sélectionnées, niveaux et lots égaux, nœuds de multifusion, enfants CSR, racine, deux poids exacts, compteurs et borne des cinq arènes. L'indépendance sémantique du réducteur reste fournie par la preuve EMST--coupes et par le différentiel existant jusqu'à $n=14$.

Le résultat distinct `K1SeededExactCompactHierarchy` porte `proof_basis=verified_exact_emst_equal_level_compact_k1_reduction_v1`, `reduction_status=compact_k1_forest_certified` et `scope=local_k1_compact_forest_only`. Ses valeurs par défaut sont `not_certified` et `unspecified`. Le témoin source conserve obligatoirement `hierarchy_reduction_status=not_performed` et `scientific_status=local_emst_witness_only`; aucun `public_status`, ordre $k>1$, application verticale ou certificat de tour complète n'est ajouté.

Le test hôte strict couvre le singleton, une chaîne $8\to4\to2\to1$ et le carré dont les trois arêtes EMST de même niveau deviennent une unique multifusion d'arité quatre. Il vérifie que la source reste inchangée, puis falsifie séparément le statut source, le statut et le scope locaux, chacune des cinq arènes, la racine, les poids, les compteurs et la politique fiable. GCC et Clang en Release strict ferment ce test ciblé. Cette logique est CPU et n'ajoute aucun noyau CUDA; GCP n'a donc pas été relancé pour ce jalon.

## Catalogue rang-deux et décision Gabriel

Pour chaque paire canonique $(u,v)$, une seconde voie reconstruit indépendamment son centre diamétral $c=(u+v)/2$, son rayon carré $a=\left\Vert u-v\right\Vert^2/4$, puis interroge tout le nuage par `brute_force_closed_ball`. La partition globale exacte sépare intérieur strict, shell complet et extérieur; aucune liste locale, exclusion de support, tolérance ou limite de visites n'intervient.

La décision distingue trois cas :

1. `rank_two_critical` : l'intérieur est vide et le shell vaut exactement $\lbrace u,v\rbrace$;
2. `extra_shell_degeneracy` : l'intérieur est vide mais le shell contient un autre site;
3. `interior_blocked` : un site au moins appartient à l'intérieur strict.

Les deux premiers cas appartiennent au graphe de Gabriel diagnostique du contrat existant. Seul le premier entre dans la réduction rang-deux certifiée. Le second reste une décision exacte et une arête diagnostique, mais force `catalog_status=unsupported_degeneracy` et interdit `locally_supported`; il n'est ni masqué comme rejet numérique, ni promu sous l'hypothèse de position générale.

Le catalogue conserve les enregistrements des $\binom{n}{2}$ paires, leurs centres, longueurs, niveaux, intérieurs, shells et rangs fermés, ainsi que les trois projections d'arêtes : graphe complet, rang-deux critique et Gabriel diagnostique. Son coût de référence est $O(n^3)$ évaluations exactes de distance et sa mémoire est quadratique.

## Certificat EMST–rang-deux

Une arête d'un EMST ne peut avoir ni point strictement intérieur à sa boule diamétrale, ni point supplémentaire sur son shell : dans les deux cas, ce troisième sommet forme avec ses extrémités un cycle dont les deux autres arêtes sont strictement plus courtes. Toute arête EMST est donc `rank_two_critical`.

À chaque seuil exact, l'arbre témoin de Kruskal est ainsi inclus dans le graphe rang-deux, lui-même inclus dans le graphe complet. Le rejeu du certificat compare explicitement, aux niveaux réunis des trois graphes, les coupes strictes et fermées des cinq chemins suivants : graphe complet, EMST sélectionné, graphe rang-deux, arbre témoin rang-deux et graphe de Gabriel diagnostique. L'égalité des états strictement pré-lot et fermés post-lot impose alors les mêmes multifusions canoniques.

La réduction rang-deux possède son propre parcours de lots. Elle fige les composantes strictement antérieures, construit le graphe quotient du lot entier, matérialise une multifusion par composante connexe, puis seulement sélectionne et applique les arêtes témoins. Le certificat sépare l'égalité sémantique des coupes, multifusions, hiérarchie et poids de l'égalité supplémentaire du témoin déterministe. Cette dernière reste exposée comme contrôle d'implémentation mais n'entre pas dans l'agrégat `anchor_equivalence_certified`, car plusieurs EMST sont valides sous ex æquo.

## Vérifications des lots CPU

Le test C++ strict couvre le singleton, le facteur exact $3\mapsto3/4$, deux fusions disjointes dans un même lot, une multifusion d'arité six, les $24$ permutations d'un carré avec ex æquo, les poids exacts et l'accord des coupes du graphe complet avec celles de l'EMST à chaque seuil strict et fermé. Il vérifie également que les lots partitionnent le graphe complet, que chaque perte de composante égale le nombre d'arêtes sélectionnées et que la racine couvre exactement tous les `PointId`.

La compilation GCC avec avertissements transformés en erreurs, le test ciblé, le profil ASan/UBSan et l'export CMake passent. Le différentiel indépendant du troisième lot complète ci-dessous cette non-régression du noyau.

Le second test strict couvre les trois décisions de paire, une perturbation d'un ULP de chaque côté d'un shell diamétral, deux multifusions disjointes dans le même lot, un tétraèdre régulier 3D dont les six arêtes rang-deux se contractent en une multifusion d'arité quatre, les diagonales extra-shell et les $24$ permutations d'un carré, les coupes strictes et fermées exactement au seuil, ainsi que la fixture non locale où chaque extrémité possède cinq observations plus proches que la paire Gabriel recherchée. Il ferme aussi tous les compteurs du catalogue et compare nœuds, poids et témoins à l'ancre EMST.

Le test strict de la forêt compacte compare ses matérialisations à l'ancre complète sur le singleton, une chaîne de 32 points à niveaux tous distincts, deux fusions simultanées, une multifusion d'arité six et les $24$ permutations du carré. Il permute aussi l'ordre et l'orientation des arêtes d'entrée, ferme chaque compteur et la borne $6(n-1)$, puis rejette les arbres incomplets, cycliques ou munis de poids incohérents. Deux EMST distincts sous ex æquo sont enfin comparés sur un carré prolongé par une fusion ultérieure : les arêtes témoins diffèrent, mais l'induction par lots restitue exactement les mêmes nœuds, coupes, multifusions et poids. Les compilations GCC et Clang en avertissements stricts, le CTest ciblé Release et le même test instrumenté ASan/UBSan passent.

Le test Borůvka couvre le singleton, une chaîne dont deux rondes sont nécessaires, un carré prolongé par une fusion ultérieure et ses $24$ permutations. Il compare le témoin final à Kruskal sur le graphe complet, ferme la réduction par moitié, les tags LBVH et les compteurs, puis vérifie que le rejeu ignore un booléen de certification falsifié mais rejette un poids exact altéré, un index étranger ou un nuage déplacé.

## Vérification hôte de la boucle hybride et qualification restante

Le test hôte emploie un lanceur GPU simulé séparé du code de décision. Il couvre le singleton terminal sans lancement, les égalités du carré, une chaîne déjà contractée, les labels non canoniques, les objets déplacés et l'isolation de l'empoisonnement. Il injecte aussi des offsets incohérents, une cible sortante requise manquante, une cible de même composante, un identifiant hors domaine et une faute GPU; chaque corruption échoue avant de publier un minimum certifié. La boucle complète ajoute un singleton sans ronde et une chaîne de huit points contractée selon $8\to4\to2\to1$; ses trois décisions, ses sept arêtes, ses poids et la forêt compacte induite coïncident avec l'ancre CPU. Des falsifications séparées de la proposition, de la décision et de la contraction invalident uniquement leur certificat propre avant de faire échouer l'agrégat. Le test Morton ciblé ferme en plus les comptes exacts `42/22/16/11/13/36`, la réduction de 86 à 41 candidats logiques, le rejeu avec les deux politiques de confiance et le rejet sans lancement lorsque le rayon de confiance manque. Le test de réduction locale recertifie ensuite la source avant allocation, compare toutes les arènes aux deux témoins exacts et échoue fermé sur chaque mutation ciblée. Les builds Release stricts GCC et Clang et les tests ciblés passent.

L'exécutable `morsehgp3d_gpu_k1_boruvka_replay` compare sur un carré et une chaîne contractée les minima décidés après proposition GPU aux rondes correspondantes du producteur Borůvka CPU. Il exige deux passes, deux synchronisations, la capacité exacte, l'absence de troncature, le sur-ensemble certifié et la résolution CPU complète, puis émet le schéma compact `morsehgp3d.phase5.k1_boruvka_gpu_replay.v1`. Une variante hôte réutilise le faux lanceur pour valider le contrat sans matériel. Ce schéma v1 reste une qualification de rondes isolées; il ne constitue pas une qualification matérielle de la boucle complète.

La première ronde est qualifiée matériellement au SHA `9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e`. Sur la cible gardée `europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, le replay réel ferme les deux fixtures et leurs 15 candidats requis, l'audit AOT ne trouve qu'un ELF `sm_120` et aucune entrée PTX, puis `compute-sanitizer` passe en `memcheck` et `racecheck`. L'artefact `phase5-k1-boruvka-9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e.json`, de SHA-256 `cf6e58e6f35b3fbbc1d1357b0504072b6db28240858b195ae3002e0dc6b5b74e`, n'est publié qu'après la relecture indépendante `TERMINATED` de la génération ciblée. Son champ `scientific_scope=gpu_candidate_superset_with_cpu_exact_resolution_only`, son absence de `public_status` et `scientific_result_claimed=false` interdisent de déduire de cet artefact une contraction, une boucle complète qualifiée, une forêt ou une scalabilité.

L'exécutable distinct `morsehgp3d_gpu_k1_boruvka_full_replay` ferme le singleton terminal, la chaîne $8\to4\to2\to1$ et un carré à longueurs égales. Il publie séparément audits et compteurs du producteur, décisions et contractions CPU exactes, puis certificats et compteurs d'un rejeu GPU frais. Sur la chaîne, les quatorze minima produisent sept arêtes et les poids exacts valent $8127$ puis $8127/4$; `hierarchy_reduction_status=not_performed`, `scientific_scope=local_emst_witness_only` et l'absence de `public_status` empêchent toute promotion hiérarchique.

Cette boucle est qualifiée matériellement au SHA `c199651d86e861eb755357986d036889839578d4` sur la même cible gardée. L'audit ferme un ELF CUDA 12.9 exclusivement `sm_120`, zéro entrée PTX, le replay canonique `morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v1`, `memcheck` et `racecheck`. L'artefact `phase5-k1-boruvka-c199651d86e861eb755357986d036889839578d4.json`, de SHA-256 `b10e3bb8c94d6e8fa0f70223d5faa99d94a5144701a87c087587753c912a7215`, revendique `scientific_result_claimed=false` et n'a été publié qu'après la relecture indépendante `TERMINATED` de la génération ciblée.

## Différentiel indépendant jusqu'à $n=14$

`morsehgp3d_hierarchy_k1_dump` accepte des coordonnées binary64 brutes par un protocole versionné, canonise le nuage, construit l'ancre C++ et émet un objet JSON canonique par cas. Le document expose chaque paire avec centre, niveau, partition globale et classification, les graphes rang-deux et Gabriel, les deux arbres témoins, leurs poids, les nœuds canoniques jusqu'à la racine, les multifusions, le certificat et, pour chaque niveau exact, les coupes strictes puis fermées des cinq chemins de rejeu. Le validateur ne se contente donc pas de relire les booléens du certificat.

`check_hierarchy_k1.py` recalcule directement chaque boule diamétrale avec `fractions.Fraction`, construit son propre graphe rang-deux et son propre quotient gelé par lots, puis utilise l'EMST stdlib indépendant de `tests/reference_emst`. L'oracle Gamma exhaustif fournit une troisième implémentation des niveaux et des coupes Gamma/Gabriel. Les sorties C++ ne fournissent aucun cutoff, candidat ou résultat intermédiaire à ces oracles.

La campagne contient 50 nuages exacts. Elle couvre toutes les tailles de $1$ à $14$, les dimensions affines un à trois dès leur cardinal minimal, les trois classifications de paire, les niveaux égaux, deux multifusions simultanées, les multifusions d'arité six et huit, un carré et un tétraèdre régulier, les deux côtés d'un shell à un ULP, des fractions dyadiques, les exposants binary64 extrêmes et la fixture Gabriel absente des petites listes locales. Pour chaque cas, les centres, niveaux, arêtes, poids, coupes, nœuds et multifusions coïncident exactement entre les voies applicables. Cela exerce un oracle par graphe complet exhaustif sur chaque nuage jusqu'à $n=14$; cela ne prétend ni énumérer tous les nuages possibles, ni remplacer les campagnes statistiques ultérieures.

L'identité des arêtes témoins reste un diagnostic d'implémentation séparé. Le validateur accepte explicitement un autre arbre témoin sous ex æquo dès lors qu'il appartient au graphe certifié et conserve toutes les coupes et le poids exact; la porte scientifique repose sur ces invariants, les nœuds canoniques et les multifusions.

GCP n'a pas été utilisé pour les lots CPU ni pour la validation hôte. La qualification réelle de la première ronde GPU a utilisé une G4 Spot gardée; sa génération `2026-07-18T16:58:02.206-07:00` a été arrêtée puis relue `TERMINATED`. Pour la boucle complète, un premier essai a échoué fermé avant replay parce que la cible n'était pas encore inscrite dans le preset CUDA; cette génération a été arrêtée et certifiée avant le correctif. La génération monolithique qualifiée `2026-07-18T18:17:12.622-07:00` a ensuite été arrêtée à `2026-07-18T18:20:30.222-07:00` et relue `TERMINATED`. La génération chunkée `2026-07-18T23:22:16.145-07:00` sur `europe-west4-ai1a/ehgp-blackwell-spot-ai1a` a été arrêtée à `2026-07-18T23:26:37.300-07:00`, puis relue `TERMINATED` le `2026-07-19T06:26:57Z`. La génération Morton `2026-07-19T00:35:42.428-07:00` sur la même cible a été arrêtée à `2026-07-19T00:39:13.137-07:00`, puis relue `TERMINATED` le `2026-07-19T07:39:23Z`. La session finale du profil Morton a démarré par le point d'entrée gardé dans le projet `devpod-gpu-exploration`, sur la cible `europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, à `2026-07-19T02:18:17.482-07:00`; le coupe-circuit invité de 45 minutes a été vérifié, puis cette cible précise a été arrêtée par le point de sortie gardé et relue `TERMINATED` le `2026-07-19T09:25:18Z`. La session du profil external-1NN exact a démarré la même cible Spot à `2026-07-19T04:00:38.844-07:00`, avec `maxRunDuration=3600`, action GCE `STOP` et arrêt invité de 45 minutes vérifiés; elle a été arrêtée par `stop_and_verify.sh` puis relue `TERMINATED` le `2026-07-19T11:08:46Z`. Sa clé OS Login a été révoquée, sa copie privée supprimée et aucune autre VM `project=e-hgp` active n'a été observée.

## Suite immédiate

Le profil final du producteur exhaustif ferme son étape de mesure : malgré les gains de constante du rayon $W=16$, les familles `uniform` et `clusters` conservent une croissance quadratique du travail exact. La résolution external-1NN exacte et sa chaîne complète avec rejeu frais éliminent le payload candidat. Leur artefact G4 fermé étend la matrice à 16 384 points, conserve zéro record candidat et observe sur le dernier quadruplement des exposants empiriques entre $1.090$ et $1.240$, avec au plus 235 visites et une frontière de 93 nœuds par source. La famille `morton_overlap` montre toutefois que la frontière point--LBVH indépendante actuelle exige un travail quadratique dans le pire cas du modèle paramétrique; la preuve scalable est donc fermée négativement pour ce resolver, sans toucher à son exactitude ni à sa forêt locale. Le parcours self-dual ferme exactement les minima, la partition des paires, le bloc adversarial et sa chaîne Borůvka locale sur hôte; sa pile DFS est bornée par $2H+1$ et ses visites par $n(n+1)-1$. Sa chaîne persistante et son rejeu frais réduisent maintenant les graines et cutoffs directement par composante sous une enveloppe max figée, sans minima ponctuels ni mises à jour d'ancêtres. La transformation explicite du témoin recertifié en `K1CompactForest` reste fermée avec un statut local séparé. Le prochain lot doit instrumenter et comparer le travail de cette chaîne directe avant toute qualification G4, puis rechercher une réduction prouvable du nombre de couples visités; une preuve sous-quadratique, les ordres au-delà de l'ancre locale $k=1$, les applications verticales et tout statut public restent non promus.
