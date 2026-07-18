# Progression Phase 4 — canonisation et oracle spatial exact

## Statut

- phase : `4`, en cours;
- backend : `reference_cpu`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 2A, 2B et 3 fermées;
- porte de sortie : ouverte; la référence GPU indépendante et le filtre borné de rejet AABB strict sont qualifiés sur G4 réelle, mais le LBVH résident et les chemins spatiaux GPU de production restent à livrer.

Les quatre premiers lots construisent la vérité terrain spatiale CPU, son premier accélérateur certifié Morton-LBVH, une référence CUDA exhaustive dont les propositions flottantes sont séparées des décisions scientifiques, puis une primitive CUDA bornée dont chaque rejet est recertifié exactement. Ils certifient leurs propres sorties locales, mais ne produisent aucune forêt, ne ferment ni G2 ni la Phase 4 et ne promeuvent aucun `public_status` vers `exact`.

## Contrat mathématique du top-k

Pour une requête rationnelle exacte $q$, un ensemble d'exclusions $E$ et les sites admissibles $A=X\setminus E$, on pose $d_i=\left\Vert q-x_i\right\Vert^2$ et $\tau=d_{(k)}$. La sortie n'est pas une liste arbitraire de $k$ identifiants : elle conserve la partition exacte

$$S_{<}=\left\lbrace i\in A:d_i<\tau\right\rbrace,\qquad S_{=}=\left\lbrace i\in A:d_i=\tau\right\rbrace.$$

L'invariant vérifié est $\lvert S_{<}\rvert<k\leq\lvert S_{<}\rvert+\lvert S_{=}\rvert$. Le shell $S_{=}$ est toujours complet, y compris pour le 1-NN et lorsque sa taille dépasse $k$, $s_{\max}$ ou quatre. `canonical_choice_ids()` ajoute à $S_{<}$ les plus petits identifiants nécessaires de $S_{=}$, puis trie le résultat; ce représentant déterministe ne remplace jamais le shell.

Les distances sont des `ExactLevel` calculés directement entre `ExactRational3` et coordonnées dyadiques. Le calcul homogène forme un dénominateur commun et la somme des trois carrés entiers, puis ne normalise la fraction qu'une fois par site. La frontière spatiale reconstruit aussi requête et rayon par leurs constructeurs validants avant tout calcul : une fraction déplacée ou non canonique ne peut donc pas publier une partition complète. Aucune requête rationnelle n'est convertie en `double`, aucune égalité n'utilise un epsilon et chaque site admissible est évalué exactement une fois par requête brute-force. Le cutoff est sélectionné par `nth_element`; un scan complet termine ensuite le shell, puis seuls le strict et le shell sont triés pour la sortie canonique.

## Rang fermé global

`ClosedBallPartition` classe chaque site de $X$ dans exactement une des trois listes triées `interior_ids`, `shell_ids` ou `exterior_ids`. Son rang fermé vaut $\lvert I\rvert+\lvert U\rvert$ sur le nuage global. Il n'existe volontairement aucune surcharge avec exclusions : le rang filtré d'un top-k sur $X\setminus E$ ne doit pas être confondu avec le rang fermé d'un événement de Morse.

La lecture « arrêter dès que le rang dépasse $s_{\max}$ » est désormais bornée à une primitive interne incomplète. Elle ne peut produire ni partition globale, ni `shell_complete`, ni certificat `RelevantGP`. La fixture [`relevant_gp_extra_shell_above_smax.json`](../../morsehgp3d/tests/fixtures/spatial/relevant_gp_extra_shell_above_smax.json) conserve le cas minimal : deux points candidats sur l'axe x, un troisième point extérieur au candidat mais sur la même sphère, $s_{\max}=2$ et rang fermé trois. Tronquer au rang deux manquerait précisément la dégénérescence qui impose `unsupported_degeneracy`.

## Référence CUDA exhaustive et recertification

`SpatialReferenceContext` est lié au même jeton de namespace que le nuage canonique. Pour chaque coordonnée rationnelle de la requête, l'hôte trouve par recherche binaire exacte les deux binary64 adjacents, compare la valeur au milieu rationnel et applique la règle ties-to-even. Le rapport distingue `exact`, `rounded`, `underflow` et `overflow_clamped`; une coordonnée hors de la plage binary64 finie est rabattue visiblement vers le maximum fini du même signe pour la seule proposition. Cette projection n'est jamais présentée comme la requête scientifique et la décision CPU conserve la coordonnée rationnelle originale.

Le kernel `sm_120` parcourt tous les indices par une boucle grid-stride sans plafond de visites. Pour chaque `PointId`, il applique dans l'ordre fixe soustraction, carré et accumulation avec `__dsub_rn`, `__dmul_rn` et `__dadd_rn`, sans fast-math, FMA implicite ni FTZ, puis renvoie `(PointId, proposed_squared_distance_bits)`. Un overflow vers `+inf` est une proposition diagnostique valide; un NaN, une distance négative, un ID répété, absent ou hors domaine invalide et empoisonne le contexte.

La règle de publication est plus forte qu'une vérification des seuls candidats : l'hôte exige d'abord que le retour GPU soit une permutation exacte de $\left\lbrace 0,\ldots,n-1\right\rbrace$, puis appelle l'oracle CPU sur tous les sites admissibles pour le top-$k$ et sur tout $X$ pour la boule fermée. Les bits de distance GPU ne sont accessibles au chemin scientifique qu'au sein du validateur et de son hash diagnostique; ils ne déterminent ni exclusion, ni cutoff, ni égalité, ni shell, ni rang.

La preuve de conservation est immédiate. Après validation de la permutation, l'ensemble réévalué exactement est précisément $X\setminus E$ pour le top-$k$ et $X$ pour la boule. La partition publiée est donc la même fonction déterministe du même multiensemble de distances exactes que la force brute CPU, indépendamment de l'ordre, des ties et des overflows de la proposition. La fixture [`gpu_fp64_tie_split.json`](../../morsehgp3d/tests/fixtures/spatial/gpu_fp64_tie_split.json) montre pourquoi une politique plus faible serait fausse : la requête exacte médiane donne deux distances égales à $2^{-106}$, alors que sa projection RN-even produit les propositions distinctes zéro et $2^{-104}$.

## Filtre CUDA borné de rejet AABB strict

`SpatialBoundsContext` reçoit un lot immuable de boîtes `ExactDyadicAabb3`. L'hôte valide d'abord que leurs six bornes sont des mots binary64 finis canoniques et que chaque intervalle est ordonné. Chaque coordonnée rationnelle de la requête et le cutoff carré exact sont ensuite projetés vers leurs deux binary64 adjacents par recherche binaire rationnelle. Le statut distingue `exact`, `enclosed` et `unsupported_range`; dans ce dernier cas, aucun kernel n'est lancé et toutes les boîtes retournent explicitement `unknown`.

Le kernel calcule un encadrement sortant de $D_{\min}$ avec `__dsub_rd`, `__dsub_ru`, `__dmul_rd`, `__dmul_ru`, `__dadd_rd` et `__dadd_ru`, toujours sans fast-math, FMA implicite ni FTZ. Il propose `prune` seulement si la borne inférieure vérifie strictement $D_{\min}^{\mathrm{low}}>\tau^{\mathrm{high}}$, `visit` seulement si $D_{\min}^{\mathrm{high}}<\tau^{\mathrm{low}}$, et `unknown` dans tous les autres cas. Une égalité, un recouvrement, un overflow ou un intervalle non fini publie donc `unknown`; l'overflow canonique est représenté par l'intervalle diagnostique $[0,+\infty]$.

L'hôte exige une permutation complète des indices de boîtes, valide chaque intervalle et chaque comparaison flottante, puis recalcule exactement $D_{\min}(B,q)$ pour toute proposition `prune`. La décision terminale n'est publiée que si la comparaison rationnelle stricte $D_{\min}(B,q)>\tau$ est confirmée; sa marge exacte positive contribue au minimum d'audit. Une contradiction, une troncature, un doublon, un indice ou un code invalide échoue fermé et empoisonne uniquement le contexte concerné. `visit` reste un indice de parcours conservateur et `unknown` impose le repli exact; cette primitive ne publie ni top-$k$, ni boule fermée, ni shell, ni rang.

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

Le lot CUDA ajoute des faux lanceurs host-only capables de renverser la permutation, de proposer des valeurs croissantes, décroissantes, nulles ou infinies, puis d'injecter cardinalité tronquée, doublon, ID hors plage, NaN et distance négative. Toutes les propositions valides, même volontairement fausses, publient exactement la partition CPU; toute corruption post-GPU échoue fermé et empoisonne seulement son contexte. Le replay et son oracle Python `Fraction` couvrent 1 013 cas : les 1 006 cas spatiaux et sept cas GPU ciblés pour le tie exact séparé par RN-even, l'underflow signé, une distance sous-normale, l'overflow lors de l'accumulation, les frontières subnormal–normal, les coordonnées finies maximales et le rabattement diagnostique hors plage, avec toutes les tailles $1\leq n\leq1\,000$. Le résumé exige séparément les six couvertures IEEE `addition_only_overflow`, `finite_subnormal_distance`, `max_finite_query`, `normal_subnormal_tie`, `overflow_clamped_query` et `subnormal_tie`. L'émulateur de la recette binary64 reproduit aussi le digest FNV-1a ordonné par ID; les 1 013 partitions, projections et digests coïncident.

La qualification G4 au SHA `01be0f150ee35a01bc939d9240b0a5675e3ae800` ferme les mêmes 1 013 cas sur le replay CUDA réel, soit 2 026 lancements pour les requêtes top-k et boule fermée. Le memcheck borné en rejoue 20, soit 40 lancements. Les deux campagnes conservent `cpu_exact_all_points`, les six couvertures IEEE et les quatre états de projection; l'artefact atteste aussi `sm_120` seulement, aucune entrée PTX, les workflows release et audit, puis `compute-sanitizer` sans erreur.

Le filtre borné ajoute un faux lanceur avec permutation inversée, `unknown`, `visit`, faux rejet et six corruptions post-GPU. Son replay borné et son oracle Python `Fraction` couvrent treize cas ciblés : égalité stricte, requête et cutoff non binary64, sous-normaux signés, carré sous-normal, boîtes dégénérées, extrema finis, overflows d'accumulation et requête ou cutoff hors plage. Le différentiel host-only et le replay G4 final au SHA `24e33d4fc80d2b5c687c939f8240fa50571d1951` publient six rejets certifiés et zéro faux rejet; les trois états d'encadrement sont observés. L'ELF est exclusivement `sm_120`, aucune section PTX n'est présente et le memcheck des treize cas rapporte zéro erreur et zéro fuite.

La matrice locale compte désormais 41 tests sous GCC 13 en Release. Le nouveau contexte host-only, le replay émulé et leurs validateurs statiques passent aussi sous ASan/UBSan; les différentiels host-only ferment séparément les 1 013 cas de la référence exhaustive et les treize cas du filtre borné. Les validations Clang, consumers installés et différentielles sanitizer du lot CPU précédent restent inchangées.

L'installation exporte `morsehgp3d::exact`, `morsehgp3d::spatial` et la cible interne de sanitizer. L'archive spatiale est PIC pour son futur lien dans un module partagé. Un package sanitizer propage ses options de lien au consumer au lieu de produire des références ASan/UBSan non résolues. Le consumer installé construit aussi un `MortonLbvhIndex`, puis confronte son 1-NN et sa boule fermée aux partitions de référence afin de détecter tout en-tête, archive ou dépendance CMake manquante.

## Limites du lot

- aucun chemin cuVS;
- aucun LBVH GPU ni index résident qualifié sur G4;
- le filtre borné AABB n'est pas encore intégré à un parcours LBVH GPU et ne produit aucun top-$k$ ni aucune boule fermée;
- aucun catalogue, événement critique, réduction hiérarchique ou résultat public.

## GCP

Le premier démarrage gardé du 18 juillet 2026 a ciblé `devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`. La cible était `TERMINATED`, `g4-standard-48`, `SPOT`, `instanceTerminationAction=STOP` et `maxRunDuration=3600`; l'API n'a toutefois pas exposé `terminationTimestamp` dans cette zone. La garde a donc échoué fermé avant tout travail GPU, puis `stop_and_verify.sh` a arrêté exactement la génération `2026-07-18T08:07:12.700-07:00`. La relecture finale donne `TERMINATED` avec `lastStopTimestamp=2026-07-18T08:08:11.558-07:00`; la clé OS Login de session a été révoquée et sa copie privée supprimée.

La qualification utile a ensuite ciblé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` avec les mêmes gardes `g4-standard-48`, `SPOT`, `STOP` et 3 600 secondes. L'échéance GCE calculée depuis la génération `2026-07-18T08:11:23.077-07:00` a été certifiée, puis l'arrêt invité `poweroff` à 45 minutes a été armé et relu avant le preflight. Le matériel observé est une NVIDIA RTX PRO 6000 Blackwell Server Edition de compute capability 12.0, pilote 13.0.0 et 101 973 950 464 octets de VRAM; l'image qualifiée porte le digest `sha256:2cc7bed14b90cbaa188974d96463488dda612052987b642a39b0586e982d6383`.

Après le worker, `stop_and_verify.sh` puis une relecture GCE indépendante ont certifié cette cible `TERMINATED`; `lastStopTimestamp=2026-07-18T08:15:19.091-07:00` et la preuve finale a été horodatée `2026-07-18T15:15:28Z`. Les artefacts hors worktree `phase3-01be0f150ee35a01bc939d9240b0a5675e3ae800.json` et `phase4-spatial-01be0f150ee35a01bc939d9240b0a5675e3ae800.json` ont alors seulement été promus vers `status=passed`; leurs SHA-256 sont respectivement `c3868af44dc435431d51cdca526fe1ad48bcc066edbd6d601458bc9176680894` et `31d6a1177a93913036ee1cc19228e89b5267d1389d57d1bdd3f42877c5270cf2`. La clé OS Login de cette session a été révoquée et supprimée localement; les deux cibles sont `TERMINATED` et aucune autre VM `project=e-hgp` active n'a été observée au passage de relais.

La qualification du filtre borné a réutilisé uniquement `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, après relecture `TERMINATED`, avec `g4-standard-48`, `SPOT`, `STOP`, `maxRunDuration=3600` et arrêt invité à 30 minutes. La première compilation réelle au SHA `6001a5ef3562eba33b69c1241a8c02f3fc2c0aab` a détecté que `std::array::operator[]` n'était pas appelable depuis le device sous NVCC 12.9; aucun test scientifique n'a été revendiqué pour ce build. Le stockage interne a été remplacé par des tableaux triviaux au commit `24e33d4fc80d2b5c687c939f8240fa50571d1951`, puis le build AOT, le différentiel et le memcheck ont réussi dans la même session gardée.

La génération `2026-07-18T09:02:51.523-07:00` a ensuite été arrêtée par `stop_and_verify.sh`; la relecture indépendante certifie `TERMINATED` et `lastStopTimestamp=2026-07-18T09:13:32.670-07:00`, sans autre VM `project=e-hgp` active. L'artefact hors worktree `phase4-spatial-bounds-24e33d4fc80d2b5c687c939f8240fa50571d1951.json` porte le SHA-256 `d61ffcf88f3967c8618ec04e0666391fa9592db4b5ab00ee0793cee17dc26239`. La clé OS Login a été révoquée et sa copie privée supprimée. Cette preuve qualifie uniquement le rejet AABB local; elle ne ferme ni la Phase 4 ni G2 et conserve `public_status=null`.
