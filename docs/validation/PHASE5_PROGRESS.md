# Progression Phase 5 — ancre exacte $k=1$ et EMST

## Statut

- phase : `5`, en cours;
- backend de proposition : `cuda_g4` (chemin à graine fixe et émission chunkée qualifiés sur G4; graine Morton bornée implémentée et validée sur lanceur hôte, qualification G4 ouverte);
- backend de rejeu indépendant des propositions : `cuda_g4` (replay complet chunké qualifié sur G4; rejeu à politique Morton séparée validé sur lanceur hôte);
- backend de recertification, décision et contraction : `reference_cpu`;
- profil : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; l'émission chunkée et son replay CUDA réel sont qualifiés sur G4, et le resserrement Morton est fermé mathématiquement et sur lanceur hôte, mais sa qualification CUDA réelle, le travail exact scalable, la réduction hiérarchique et le passage à l'échelle restent ouverts.

Les cinq premiers jalons CPU de Phase 5 livrent l'ancre EMST exacte, le catalogue exhaustif des paires, la réduction rang-deux du backend CPU, leur différentiel indépendant jusqu'à $n=14$, la forêt hiérarchique compacte construite depuis un EMST déjà certifié et un producteur Borůvka exact sur le LBVH global. La voie hybride enchaîne maintenant les rondes stackless dans un contexte de proposition résident, décide chaque minimum par le CPU exact, contracte canoniquement les composantes et vérifie le témoin EMST complet par un second rejeu GPU et l'ancre CPU. La boucle complète et son overload chunké à graine fixe sont qualifiés sur G4 pour un témoin EMST local, avec politique de budget fournie séparément au vérificateur et pic physique du payload candidat certifié. Un nouvel overload propose en plus une graine locale dans une fenêtre Morton bornée, recertifie exactement sa cible et ne la retient que si sa clé canonique améliore le fallback; sa preuve et son rejeu hôte sont fermés, mais pas encore sa qualification G4. Aucun de ces jalons ne ferme la Phase 5 ou la porte globale G2 du catalogue, ni ne publie `public_status=exact` pour une tour MorseHGP3D complète; le volume logique et la recertification restent potentiellement quadratiques.

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

Le chemin CUDA utilise des opérations binary64 RN explicites, un thread par position Morton et des buffers résidents pour l'ordre et les records de proposition. La compilation hôte stricte et le test ciblé passent; le replay v2 historique reste byte-identique avec le SHA-256 `9e686fc8147bf43860180f723c5005eb6bbb9277b06ca33e5521d370db56e121`. La compilation AOT CUDA 12.9 `sm_120`, les sanitizers et le replay réel de ce nouvel overload ne sont pas encore exécutés : son statut matériel reste donc ouvert.

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

Le test hôte emploie un lanceur GPU simulé séparé du code de décision. Il couvre le singleton terminal sans lancement, les égalités du carré, une chaîne déjà contractée, les labels non canoniques, les objets déplacés et l'isolation de l'empoisonnement. Il injecte aussi des offsets incohérents, une cible sortante requise manquante, une cible de même composante, un identifiant hors domaine et une faute GPU; chaque corruption échoue avant de publier un minimum certifié. La boucle complète ajoute un singleton sans ronde et une chaîne de huit points contractée selon $8\to4\to2\to1$; ses trois décisions, ses sept arêtes, ses poids et la forêt compacte induite coïncident avec l'ancre CPU. Des falsifications séparées de la proposition, de la décision et de la contraction invalident uniquement leur certificat propre avant de faire échouer l'agrégat. Le test Morton ciblé ferme en plus les comptes exacts `42/22/16/11/13/36`, la réduction de 86 à 41 candidats logiques, le rejeu avec les deux politiques de confiance et le rejet sans lancement lorsque le rayon de confiance manque. Le build Release strict et les tests ciblés du producteur CPU et du contexte hybride passent.

L'exécutable `morsehgp3d_gpu_k1_boruvka_replay` compare sur un carré et une chaîne contractée les minima décidés après proposition GPU aux rondes correspondantes du producteur Borůvka CPU. Il exige deux passes, deux synchronisations, la capacité exacte, l'absence de troncature, le sur-ensemble certifié et la résolution CPU complète, puis émet le schéma compact `morsehgp3d.phase5.k1_boruvka_gpu_replay.v1`. Une variante hôte réutilise le faux lanceur pour valider le contrat sans matériel. Ce schéma v1 reste une qualification de rondes isolées; il ne constitue pas une qualification matérielle de la boucle complète.

La première ronde est qualifiée matériellement au SHA `9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e`. Sur la cible gardée `europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, le replay réel ferme les deux fixtures et leurs 15 candidats requis, l'audit AOT ne trouve qu'un ELF `sm_120` et aucune entrée PTX, puis `compute-sanitizer` passe en `memcheck` et `racecheck`. L'artefact `phase5-k1-boruvka-9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e.json`, de SHA-256 `cf6e58e6f35b3fbbc1d1357b0504072b6db28240858b195ae3002e0dc6b5b74e`, n'est publié qu'après la relecture indépendante `TERMINATED` de la génération ciblée. Son champ `scientific_scope=gpu_candidate_superset_with_cpu_exact_resolution_only`, son absence de `public_status` et `scientific_result_claimed=false` interdisent de déduire de cet artefact une contraction, une boucle complète qualifiée, une forêt ou une scalabilité.

L'exécutable distinct `morsehgp3d_gpu_k1_boruvka_full_replay` ferme le singleton terminal, la chaîne $8\to4\to2\to1$ et un carré à longueurs égales. Il publie séparément audits et compteurs du producteur, décisions et contractions CPU exactes, puis certificats et compteurs d'un rejeu GPU frais. Sur la chaîne, les quatorze minima produisent sept arêtes et les poids exacts valent $8127$ puis $8127/4$; `hierarchy_reduction_status=not_performed`, `scientific_scope=local_emst_witness_only` et l'absence de `public_status` empêchent toute promotion hiérarchique.

Cette boucle est qualifiée matériellement au SHA `c199651d86e861eb755357986d036889839578d4` sur la même cible gardée. L'audit ferme un ELF CUDA 12.9 exclusivement `sm_120`, zéro entrée PTX, le replay canonique `morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v1`, `memcheck` et `racecheck`. L'artefact `phase5-k1-boruvka-c199651d86e861eb755357986d036889839578d4.json`, de SHA-256 `b10e3bb8c94d6e8fa0f70223d5faa99d94a5144701a87c087587753c912a7215`, revendique `scientific_result_claimed=false` et n'a été publié qu'après la relecture indépendante `TERMINATED` de la génération ciblée.

## Différentiel indépendant jusqu'à $n=14$

`morsehgp3d_hierarchy_k1_dump` accepte des coordonnées binary64 brutes par un protocole versionné, canonise le nuage, construit l'ancre C++ et émet un objet JSON canonique par cas. Le document expose chaque paire avec centre, niveau, partition globale et classification, les graphes rang-deux et Gabriel, les deux arbres témoins, leurs poids, les nœuds canoniques jusqu'à la racine, les multifusions, le certificat et, pour chaque niveau exact, les coupes strictes puis fermées des cinq chemins de rejeu. Le validateur ne se contente donc pas de relire les booléens du certificat.

`check_hierarchy_k1.py` recalcule directement chaque boule diamétrale avec `fractions.Fraction`, construit son propre graphe rang-deux et son propre quotient gelé par lots, puis utilise l'EMST stdlib indépendant de `tests/reference_emst`. L'oracle Gamma exhaustif fournit une troisième implémentation des niveaux et des coupes Gamma/Gabriel. Les sorties C++ ne fournissent aucun cutoff, candidat ou résultat intermédiaire à ces oracles.

La campagne contient 50 nuages exacts. Elle couvre toutes les tailles de $1$ à $14$, les dimensions affines un à trois dès leur cardinal minimal, les trois classifications de paire, les niveaux égaux, deux multifusions simultanées, les multifusions d'arité six et huit, un carré et un tétraèdre régulier, les deux côtés d'un shell à un ULP, des fractions dyadiques, les exposants binary64 extrêmes et la fixture Gabriel absente des petites listes locales. Pour chaque cas, les centres, niveaux, arêtes, poids, coupes, nœuds et multifusions coïncident exactement entre les voies applicables. Cela exerce un oracle par graphe complet exhaustif sur chaque nuage jusqu'à $n=14$; cela ne prétend ni énumérer tous les nuages possibles, ni remplacer les campagnes statistiques ultérieures.

L'identité des arêtes témoins reste un diagnostic d'implémentation séparé. Le validateur accepte explicitement un autre arbre témoin sous ex æquo dès lors qu'il appartient au graphe certifié et conserve toutes les coupes et le poids exact; la porte scientifique repose sur ces invariants, les nœuds canoniques et les multifusions.

GCP n'a pas été utilisé pour les lots CPU ni pour la validation hôte. La qualification réelle de la première ronde GPU a utilisé une G4 Spot gardée; sa génération `2026-07-18T16:58:02.206-07:00` a été arrêtée puis relue `TERMINATED`. Pour la boucle complète, un premier essai a échoué fermé avant replay parce que la cible n'était pas encore inscrite dans le preset CUDA; cette génération a été arrêtée et certifiée avant le correctif. La génération monolithique qualifiée `2026-07-18T18:17:12.622-07:00` a ensuite été arrêtée à `2026-07-18T18:20:30.222-07:00` et relue `TERMINATED`. La génération chunkée `2026-07-18T23:22:16.145-07:00` sur `europe-west4-ai1a/ehgp-blackwell-spot-ai1a` a été arrêtée à `2026-07-18T23:26:37.300-07:00`, puis relue `TERMINATED` le `2026-07-19T06:26:57Z`. La clé OS Login de chaque session a été révoquée et aucune autre VM `project=e-hgp` active n'a été observée.

## Suite immédiate

Le producteur Borůvka CPU ne matérialise plus le graphe complet, mais son parcours séquentiel exact sert encore de référence de correction et non de voie scalable. La primitive GPU stackless, la boucle complète chunkée, son budget de confiance et son rejeu indépendant sont qualifiés sur G4. La graine locale Morton bornée est maintenant prouvée et intégrée avec recertification CPU exacte monotone, politiques de confiance séparées et compteurs de travail, mais elle peut encore conserver $L_r=\Theta(n^2)$ et une recertification CPU quadratique. Le prochain lot doit étendre le schéma de replay et l'assembleur de qualification à ce mode, le compiler et le sanitizer sur G4, puis mesurer taux d'amélioration, fallbacks, $L_r$, $M_r$ et travail exact sur des tailles croissantes. Si ces mesures restent quadratiques, la suite n'optimisera plus seulement les graines : elle devra introduire un protocole exact de recherche LBVH qui évite de reconstruire chaque $A_q$ exhaustivement. La réduction hiérarchique explicite reste distincte et aucune promotion de phase ou de statut public n'est autorisée avant ces fermetures.
