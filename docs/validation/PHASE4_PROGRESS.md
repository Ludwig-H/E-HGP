# Progression Phase 4 — canonisation et oracle spatial exact

## Statut

- phase : `4`, en cours;
- backend : `reference_cpu`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 2A, 2B et 3 fermées;
- porte de sortie : ouverte; Morton, LBVH, bornes AABB, référence GPU et différentiels des parcours accélérés restent à livrer.

Ce premier lot construit la vérité terrain spatiale CPU. Il certifie ses propres partitions exactes, mais ne produit aucune forêt, ne ferme ni G2 ni la Phase 4 et ne promeut aucun `public_status` vers `exact`.

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

## Vérifications intégrées

Le test C++ déterministe couvre la requête rationnelle de cutoff $13/36$, six co-minimiseurs sur les axes, zéro signé, permutations et provenance, ordre à un ULP, minimum sous-normal, maximum binary64 fini, neuf exclusions, erreurs de rang, affectations et déplacements, ainsi que la distinction entre rang filtré et rang fermé global. Les résultats internes vérifient aussi leurs ordres, unicités, tailles, compteurs et invariants de partition avant construction. La fixture RelevantGP embarque un manifeste complet et verrouille le SHA-256 de ses coordonnées binary64 canoniques.

Le différentiel indépendant utilise Python `Fraction`, pas les routines exactes C++. Il recalcule canonisation, distances, cutoff, strict, shell, choix canonique et boule globale. La campagne CI compte 1 006 requêtes exactes : six cas ciblés, dont le rang maximal $s_{\max}=11$ avec shell non trivial, puis une requête pour chacune des tailles $1\leq n\leq1\,000$, avec requêtes rationnelles et jusqu'à neuf exclusions. Toutes les partitions, tous les identifiants, toutes les fractions et tous les compteurs sont comparés; l'accord observé est une preuve de non-régression du lot, pas une fermeture de Phase 4.

L'installation exporte `morsehgp3d::exact`, `morsehgp3d::spatial` et la cible interne de sanitizer. L'archive spatiale est PIC pour son futur lien dans un module partagé. Un package sanitizer propage ses options de lien au consumer au lieu de produire des références ASan/UBSan non résolues. Le consumer installé exerce un 1-NN à deux co-minimiseurs et une boule fermée complète afin de détecter toute archive ou dépendance CMake manquante.

## Limites du lot

- aucune structure Morton ou LBVH;
- aucune borne AABB dirigée vers l'extérieur et aucun élagage certifié;
- aucune force brute GPU indépendante et aucun chemin cuVS;
- aucune primitive bornée de rejet anticipé;
- aucun catalogue, événement critique, réduction hiérarchique ou résultat public.

## GCP

GCP non utilisé pour ce lot CPU.
