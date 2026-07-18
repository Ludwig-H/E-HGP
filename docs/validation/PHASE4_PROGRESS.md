# Progression Phase 4 — canonisation et oracle spatial exact

## Statut

- phase : `4`, en cours;
- backend : `reference_cpu`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 2A, 2B et 3 fermées;
- porte de sortie : ouverte; la référence GPU indépendante et les chemins spatiaux GPU restent à livrer.

Les deux premiers lots construisent la vérité terrain spatiale CPU puis son premier accélérateur certifié Morton-LBVH. Ils certifient leurs propres partitions exactes, mais ne produisent aucune forêt, ne ferment ni G2 ni la Phase 4 et ne promeuvent aucun `public_status` vers `exact`.

## Contrat mathématique du top-k

Pour une requête rationnelle exacte $q$, un ensemble d'exclusions $E$ et les sites admissibles $A=X\setminus E$, on pose $d_i=\left\Vert q-x_i\right\Vert^2$ et $\tau=d_{(k)}$. La sortie n'est pas une liste arbitraire de $k$ identifiants : elle conserve la partition exacte

$$S_{<}=\left\lbrace i\in A:d_i<\tau\right\rbrace,\qquad S_{=}=\left\lbrace i\in A:d_i=\tau\right\rbrace.$$

L'invariant vérifié est $\lvert S_{<}\rvert<k\leq\lvert S_{<}\rvert+\lvert S_{=}\rvert$. Le shell $S_{=}$ est toujours complet, y compris pour le 1-NN et lorsque sa taille dépasse $k$, $s_{\max}$ ou quatre. `canonical_choice_ids()` ajoute à $S_{<}$ les plus petits identifiants nécessaires de $S_{=}$, puis trie le résultat; ce représentant déterministe ne remplace jamais le shell.

Les distances sont des `ExactLevel` calculés directement entre `ExactRational3` et coordonnées dyadiques. Le calcul homogène forme un dénominateur commun et la somme des trois carrés entiers, puis ne normalise la fraction qu'une fois par site. La frontière spatiale reconstruit aussi requête et rayon par leurs constructeurs validants avant tout calcul : une fraction déplacée ou non canonique ne peut donc pas publier une partition complète. Aucune requête rationnelle n'est convertie en `double`, aucune égalité n'utilise un epsilon et chaque site admissible est évalué exactement une fois par requête brute-force. Le cutoff est sélectionné par `nth_element`; un scan complet termine ensuite le shell, puis seuls le strict et le shell sont triés pour la sortie canonique.

## Rang fermé global

`ClosedBallPartition` classe chaque site de $X$ dans exactement une des trois listes triées `interior_ids`, `shell_ids` ou `exterior_ids`. Son rang fermé vaut $\lvert I\rvert+\lvert U\rvert$ sur le nuage global. Il n'existe volontairement aucune surcharge avec exclusions : le rang filtré d'un top-k sur $X\setminus E$ ne doit pas être confondu avec le rang fermé d'un événement de Morse.

La lecture « arrêter dès que le rang dépasse $s_{\max}$ » est désormais bornée à une primitive interne incomplète. Elle ne peut produire ni partition globale, ni `shell_complete`, ni certificat `RelevantGP`. La fixture [`relevant_gp_extra_shell_above_smax.json`](../../morsehgp3d/tests/fixtures/spatial/relevant_gp_extra_shell_above_smax.json) conserve le cas minimal : deux points candidats sur l'axe x, un troisième point extérieur au candidat mais sur la même sphère, $s_{\max}=2$ et rang fermé trois. Tronquer au rang deux manquerait précisément la dégénérescence qui impose `unsupported_degeneracy`.

## Canonisation du nuage

`CanonicalPointCloud` refuse le nuage vide et les doublons géométriques. Il normalise réellement chaque `-0` vers `+0`, trie les trois mots binary64 par ordre total lexicographique, attribue les `PointId` de zéro à $n-1$ et conserve séparément l'indice source. Le domaine est lié par assertion statique au domaine JSON exact du contrat v2, soit des identifiants et un compte au plus $2^{53}-1$. Un jeton immuable identifie le namespace canonique : une copie le conserve, un déplacement le transfère et la source déplacée devient non interrogeable.

`ExclusionSet` est construit contre ce jeton plutôt que contre le seul nombre de points; il ne peut donc pas réinterpréter silencieusement les identifiants d'un autre nuage de même taille. Il trie sans dédupliquer, rejette les répétitions et les identifiants hors nuage, puis impose $0\leq m_{\star}\leq\min(9,\max(0,n-2))$ et $\lvert E\rvert\leq m_{\star}$. Le cas $m_{\star}=0$ de $K_{\max}=1$ accepte donc exactement l'ensemble vide, et un run ne peut pas annoncer une profondeur impossible pour son nuage. Les affectations par copie du nuage, des exclusions et des partitions sont transactionnelles, et tous les auto-déplacements sont gardés. Les sources déplacées deviennent invalides; de même, un résultat top-k ou boule déplacé perd explicitement son bit de complétude et ne conserve ni vues ni cutoff publiables.

## Index Morton-LBVH et bornes exactes

Le premier index accéléré utilise $Q=2^{21}$ cellules par axe. Pour l'intervalle global exact $[l_a,u_a]$, la coordonnée Morton est calculée en rationnels, sans soustraction binary64 susceptible d'overflow :

$$b_a(x)=0\ \text{si}\ l_a=u_a,\qquad b_a(x)=\min\left(Q-1,\left\lfloor Q\frac{x-l_a}{u_a-l_a}\right\rfloor\right)\ \text{sinon}.$$

Les trois mots de 21 bits sont entrelacés en un code de 63 bits. Les feuilles sont triées par `(morton_code, PointId)`; un intervalle dont tous les codes coïncident est partagé en son milieu. Cette règle traite les collisions massives de façon équilibrée et déterministe. Morton, le choix des partages et l'ordre de parcours ne participent jamais à l'identité scientifique d'une sortie.

Chaque AABB interne conserve, pour chaque axe, les identifiants des descendants qui réalisent exactement le minimum et le maximum. Ces bornes sont donc des dyadiques d'entrée exacts et déjà dirigés vers l'extérieur avec erreur intérieure nulle, y compris à `DBL_MAX`; aucun `nextafter` n'est appliqué. Pour une boîte $B=\prod_{a=1}^{3}[l_a,u_a]$ et une requête exacte $q$, les deux certificats utilisés sont :

$$D_{\min}(B,q)=\sum_{a=1}^{3}\delta_a^2,\quad \delta_a=\max(l_a-q_a,0,q_a-u_a),\qquad D_{\max}(B,q)=\sum_{a=1}^{3}\max\left((q_a-l_a)^2,(q_a-u_a)^2\right).$$

Le premier minore la distance de tout descendant et le second la majore. Pour le top-$k$, aucun nœud n'est élagué avant que le heap contienne $k$ sites admissibles. Si son cutoff courant vaut $\tau_t$, seul le test strict $D_{\min}>\tau_t$ élague. La suite $\tau_t$ est décroissante; tout descendant ainsi retiré vérifie donc $d_i\geq D_{\min}>\tau_t\geq\tau_{\mathrm{final}}$. À égalité, le nœud est visité afin de conserver le shell complet. Pour une boule fermée de rayon carré $r^2$, un sous-arbre est extérieur seulement si $D_{\min}>r^2$, intérieur seulement si $D_{\max}<r^2$, et toute égalité descend.

Les compteurs ferment les partitions de travail sans présenter une classification AABB comme une distance ponctuelle :

$$N_{\mathrm{exact}}+N_{\mathrm{pruned,eligible}}=\lvert X\setminus E\rvert\quad\text{pour le top-}k,\qquad N_{\mathrm{exact}}+N_{\mathrm{bulk,out}}+N_{\mathrm{bulk,in}}=\lvert X\rvert\quad\text{pour la boule}.$$

Chaque décision de sous-arbre enregistre une marge rationnelle strictement positive; le minimum exact de ces marges est exposé. Les parcours `near_first` et `far_first` utilisent réellement l'ordre des bornes AABB opposé, mais publient la même projection scientifique. L'index et les résultats conservent le jeton du nuage; une copie reste valide, tandis qu'un autre namespace ou un objet déplacé échoue fermé.

## Vérifications intégrées

Le test C++ déterministe couvre la requête rationnelle de cutoff $13/36$, six co-minimiseurs sur les axes, zéro signé, permutations et provenance, ordre à un ULP, minimum sous-normal, maximum binary64 fini, neuf exclusions, erreurs de rang, affectations et déplacements, ainsi que la distinction entre rang filtré et rang fermé global. Les résultats internes vérifient aussi leurs ordres, unicités, tailles, compteurs et invariants de partition avant construction. La fixture RelevantGP embarque un manifeste complet et verrouille le SHA-256 de ses coordonnées binary64 canoniques.

Le différentiel indépendant utilise Python `Fraction`, pas les routines exactes C++. Il recalcule canonisation, distances, cutoff, strict, shell, choix canonique et boule globale. La campagne CI compte 1 006 requêtes exactes : six cas ciblés, dont le rang maximal $s_{\max}=11$ avec shell non trivial, puis une requête pour chacune des tailles $1\leq n\leq1\,000$, avec requêtes rationnelles et jusqu'à neuf exclusions. Toutes les partitions, tous les identifiants, toutes les fractions et tous les compteurs sont comparés; l'accord observé est une preuve de non-régression du lot, pas une fermeture de Phase 4.

Le différentiel LBVH ajoute 1 006 cas indépendants : six fixtures ciblées puis chaque taille $1\leq n\leq1\,000$. Les nuages sont véritablement tridimensionnels dès quatre points, les requêtes sont rationnelles, les exclusions atteignent neuf IDs et les cas ciblés exercent collisions Morton, sous-normaux, extrema binary64, shell à égalité, élagage extérieur et classification intérieure en bloc. Python `Fraction` produit seul l'attendu mathématique; les deux ordres de parcours sont comparés à cet attendu puis entre eux. La campagne vérifie aussi les identités comptables, l'existence effective des chemins accélérés et la positivité exacte de chaque marge minimale publiée.

La matrice locale ferme 35 tests sous GCC 13 en Release, puis les deux tests LBVH sous Clang 18. Le test unitaire passe sous ASan/UBSan en Debug; la campagne différentielle exhaustive passe sous ASan/UBSan en Release instrumenté. Les consumers installés Release et sanitizer reconstruisent et exécutent le chemin LBVH hors de l'arbre source.

L'installation exporte `morsehgp3d::exact`, `morsehgp3d::spatial` et la cible interne de sanitizer. L'archive spatiale est PIC pour son futur lien dans un module partagé. Un package sanitizer propage ses options de lien au consumer au lieu de produire des références ASan/UBSan non résolues. Le consumer installé construit aussi un `MortonLbvhIndex`, puis confronte son 1-NN et sa boule fermée aux partitions de référence afin de détecter tout en-tête, archive ou dépendance CMake manquante.

## Limites du lot

- aucune force brute GPU indépendante et aucun chemin cuVS;
- aucun LBVH GPU ni index résident qualifié sur G4;
- aucune primitive bornée de rejet anticipé;
- aucun catalogue, événement critique, réduction hiérarchique ou résultat public.

## GCP

GCP non utilisé pour ce lot CPU.
