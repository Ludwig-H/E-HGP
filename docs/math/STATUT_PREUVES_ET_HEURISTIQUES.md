# Statut des preuves, hypothèses et heuristiques

Ce registre interdit les glissements entre résultat mathématique, certificat d'exécution et espoir de performance. Il doit être mis à jour avant toute modification du statut public de MorseHGP3D.

## 1. Échelle de statut

| statut scientifique | sens |
|---|---|
| `theorem_external` | résultat publié ou démontré dans le manuscrit, avec hypothèses citées |
| `proved_here` | preuve complète donnée dans la documentation active |
| `conditional_theorem` | preuve complète si des oracles ou hypothèses explicitement vérifiés sont fournis |
| `proof_obligation` | énoncé cible plausible mais preuve ou validation exhaustive manquante |
| `heuristic` | politique de proposition sans pouvoir d'exclusion |
| `experimental_target` | critère de benchmark réfutable |
| `false_in_general` | revendication accompagnée d'un contre-exemple ou d'une obstruction |

Le statut d'une exécution est distinct : `exact`, `conditional`, `budget_exhausted`, `unsupported_degeneracy`, `numeric_failure` ou `perturbed`.

## 2. Noyau mathématique

| énoncé | statut | portée |
|---|---|---|
| K-polyèdres $\leftrightarrow\pi_0(L_k(a))$ | `theorem_external` | théorème 2 du manuscrit |
| les adjacences élémentaires suffisent | `theorem_external` | proposition 5 du manuscrit |
| un simplexe séparant est de Gabriel | `theorem_external` | théorème 4, position générale |
| Gabriel préserve les K-polyèdres non triviaux | `false_in_general` pour le graphe élagué formulé dans le manuscrit | la fixture exacte `gabriel-point-set-counterexample-5-points-v1` contredit la proposition 6 comme égalité de collections d'ensembles de points |
| un K-MST élagué préserve ces composantes | `false_in_general` pour le graphe élagué formulé dans le manuscrit | le théorème 5 hérite du contre-exemple à la proposition 6; une structure corrigée peut demander plus que le K-graphe brut |
| centre critique si et seulement si bien centré dans la fenêtre $\lvert I\rvert<k\leq s$ | `theorem_external` | Reani–Bobrowski, position générale; équivalent à $0\leq\mu\leq\lvert U\rvert-1$ |
| indice $\mu=s-k$ | `theorem_external` | Reani–Bobrowski, uniquement aux ordres de la fenêtre critique |
| multiplicité locale d'indice $\mu$ égale à $\binom{\lvert U\rvert-1}{\mu}$ | `theorem_external` | Reani–Bobrowski dans la fenêtre critique; ce rang local ne préjuge pas des attaches globales |
| à l'indice un, le sous-niveau strict possède les $\lvert U\rvert$ bras $F_u$ | `theorem_external` | une seule sphère peut tuer jusqu'à $\lvert U\rvert-1$ classes de $H_0$ |
| seuls les rangs $s=k$ et $s=k+1$ modifient $H_0$ | `theorem_external` | conséquence des indices zéro et un |
| une sphère de rang $k+1$ équivaut à un $k$-simplexe de Gabriel | `proved_here` | position générale du manuscrit |
| à $k=1$, le graphe des paires dont la boule diamétrale fermée contient exactement ses deux extrémités contient tout EMST et préserve toutes les composantes de seuil | `proved_here` | points distincts, sans hypothèse de position générale; un point intérieur ou supplémentaire sur le shell donne un cycle aux deux autres arêtes strictement plus courtes, puis l'inclusion EMST–rang-deux–graphe complet donne l'égalité de toutes les coupes; certificat exécutable dans [`PHASE5_PROGRESS.md`](../validation/PHASE5_PROGRESS.md) |
| tout EMST exact détermine les mêmes partitions strictes et fermées que le graphe euclidien complet, donc la même forêt compacte canonique | `proved_here` | à chaque seuil, une connexion présente dans le graphe mais absente de l'arbre fournirait, par échange sur le chemin de l'EMST, une arête strictement plus légère que l'arête traversée; les multifusions sont alors déterminées par les deux partitions encadrant chaque lot, indépendamment du témoin sous ex æquo; preuve et bornes dans [`PHASE5_PROGRESS.md`](../validation/PHASE5_PROGRESS.md) |
| le Borůvka canonique ordonné par $\kappa(e)=(d^2(e),u,v)$ sélectionne une sous-forêt de l'arbre de Kruskal canonique et réduit au moins de moitié le nombre de composantes à chaque ronde non terminale | `proved_here` | extrémités canoniques $u<v$, labels de composantes figés au début de la ronde et minimum sortant unique pour l'ordre total; la propriété de coupe place chaque sélection dans le même arbre de Kruskal, puis l'absence de sommet isolé dans la forêt sélectionnée donne $c_{r+1}\leq\left\lfloor\frac{c_r}{2}\right\rfloor$; le producteur LBVH exact et son vérificateur de rejeu séparé sont documentés dans [`PHASE5_PROGRESS.md`](../validation/PHASE5_PROGRESS.md) |
| un sur-ensemble à graine fixe suffit à décider exactement une ronde Borůvka | `proved_here` | pour chaque source $q$, choisir une cible $s(q)$ hors de sa composante figée, poser $R_q=d^2(q,s(q))$ et $A_q=\left\lbrace p:\ell(p)\neq\ell(q),\ d^2(q,p)\leq R_q\right\rbrace$; tout $P_q$ vérifiant $A_q\subseteq P_q\subseteq\left\lbrace p:\ell(p)\neq\ell(q)\right\rbrace$, exactement filtré par $R_q$ puis minimisé selon $\kappa$, contient et restitue le minimum sortant de $q$; le minimum selon $\kappa$ sur les points d'une composante restitue son minimum Borůvka exact; le rejeu exécutable et la séparation proposition–décision sont documentés dans [`PHASE5_PROGRESS.md`](../validation/PHASE5_PROGRESS.md) |
| l'enchaînement des sur-ensembles recertifiés, des minima exacts selon $\kappa$ et des contractions canoniques produit le témoin Borůvka canonique | `proved_here` | chaque ronde restitue ses minima exacts par l'affirmation précédente; la propriété de coupe et la réduction par moitié donnent par induction un arbre couvrant de $n-1$ arêtes appartenant à l'arbre de Kruskal canonique; cette conclusion porte sur un témoin EMST local, pas sur une hiérarchie publique |
| partitionner l'émission d'une ronde Borůvka en plages contiguës de sources complètes préserve les minima exacts et la contraction | `proved_here` | labels, graines et cutoffs figés; chaque $A_q$ est recertifié avant minimisation selon $\kappa$; aucune source n'est scindée et aucune décision n'est publiée avant le dernier chunk; preuve et contrat mémoire dans [`PHASE5_PROGRESS.md`](../validation/PHASE5_PROGRESS.md) |
| une étoile remplace la clique de facettes pour $H_0$ | `proved_here` | tous niveaux, traitement simultané |
| une vraie naissance isolée possède un rang fermé $k$ | `proved_here` | modèle de facettes complet |
| Gamma exhaustif puis omission de la généalogie publique des facettes isolées définit exactement `hgp_reduced` | `proved_here` | conséquence définitionnelle de la correspondance K-polyèdres–composantes de $\Gamma_k$; base v2 limitée au backend `reference_cpu` |
| le flot de Gabriel brut reconstruit le profil normatif `hgp_reduced` | `false_in_general` | contre-exemple exact générique à $k=2$; le profil ne peut plus publier `exact` comme hiérarchie des K-polyèdres sans définition et preuve corrigées |
| le flot de Gabriel brut fournit une connectivité positive incluse dans Gamma | `proved_here` | chaque hyperarête Gabriel est une coface de Gamma; cela autorise seulement `gabriel_positive_connectivity` et `partial_refinement` |
| une coface non-Gabriel stricte ne fait qu'une attache silencieuse $q=1$ | `proved_here` | support essentiel unique, non-supports et intrus strictement intérieurs; preuve constructive par deux remplacements intrus–support |
| toute croissance Gamma silencieuse $q=1$ reste présente dans `coverage_log`, même sans nouveau point couvert | `proved_here` | invariant de rejeu pré-lot/post-lot; fixture permanente `tests/fixtures/regressions/gamma_q1_coverage_delta.json` |
| Gabriel complété par toutes les attaches silencieuses reconstruit Gamma | `proved_here` | induction par lots exacts, lorsque toutes les cofaces sont connues |
| une génération sparse certifiée de toutes les attaches nécessaires reconstruit `hgp_reduced` | `proof_obligation` | la future base `incidence_complete_reduction_proved` reste absente du schéma v2 jusqu'à fermeture de la complétude algorithmique et des dégénérescences |
| les seules unions de points forment un invariant inductif suffisant pour élaguer les cofaces non-Gabriel | `false_in_general` | une facette peut être attachée silencieusement par une coface non-Gabriel puis participer à une fusion Gabriel ultérieure |
| le flot de Gabriel seul reconstruit toute facette isolée | `false_in_general` comme conséquence automatique du théorème 5 | une attache supplémentaire est requise |
| catalogue critique complet et attaches exactes reconstruisent `full_pi0` | `proof_obligation` | M.1 reste à démontrer, notamment aux niveaux égaux |
| le remplacement intrus–support produit un chemin positif vers une coface Gabriel | `proved_here` | supports essentiels uniques, baisse stricte de $\beta$ à chaque descendant et absence de plateau |
| ce chemin localise toujours la racine verticale exacte dans un DSU Gabriel incomplet | `proof_obligation` | requiert une complétion certifiée des incidences; faux comme conséquence du seul flot brut |
| la saturation fermée conserve exactement la miniball et est idempotente | `proved_here` | théorème S.1 de [`TOUR_BOULES_SATUREES.md`](TOUR_BOULES_SATUREES.md); aucune position générale |
| les simplexes des générateurs saturés actifs engendrent exactement le complexe de Čech | `proved_here` | théorème S.3; coupes fermées par $t\leq a$, coupes ouvertes par $t<a$ |
| les composantes de Gamma sont celles du graphe de générateurs avec $\lvert S\cap T\rvert\geq k$ | `proved_here` | théorème S.4; graphes de Johnson connexes et couverture en observations par unions de saturés |
| une forêt couvrante de poids maximum préserve simultanément toutes les composantes après seuillage en ordre | `proved_here` | théorème S.5; Kruskal décroissant, et non forêt seulement maximale au sens d'inclusion |
| une sous-famille de générateurs exacts ne crée aucune connexion Gamma fictive | `proved_here` | théorème S.6; sémantique scientifique interne `partial_refinement`, sans sérialisation v2 avant migration |
| les snapshots de la forêt de générateurs constituent déjà le `MergeForest` contractuel | `false_in_general` | les remplacements d'arêtes ne sont pas des événements topologiques; lots, `coverage_log`, généalogie et flèches restent à construire |
| un générateur saturé est un événement critique de $D_k$ à tous ses ordres descendants | `false_in_general` | fixture `morse-rank-window-regression-v1` : deux points intérieurs donnent $D_2(c)<a$ alors que le saturé agit combinatoirement à l'ordre deux |

## 3. Énumération

| énoncé | statut | condition |
|---|---|---|
| identité de raffinement $C_{m+1}(Q)=\bigcup P(Q\setminus\lbrace u\rbrace,u)$ | `proved_here` | toute dimension |
| les supports critiques sont des strates du raffinement ancré | `proved_here` | shell complet et cellule parente exacte |
| la séparation aux sommets ferme une cellule convexe bornée | `proved_here` | tous les sommets et co-minimiseurs exacts |
| la génération de colonnes termine | `proved_here` | nombre fini de sites, aucune troncature |
| la reconstruction canonique D.3 produit exactement $C_p(Q)\cap\Omega$ | `proved_here` | fermeture finie par contraintes croisées, tous les co-farthest et co-1-NN exacts, égalités actives réconciliées |
| l'induction D.4 énumère exactement tous les labels et parents non vides jusqu'à $m_{\star}$ | `proved_here` | base $C_0\cap\Omega$, D.1, diagrammes restreints fermés par D.2 et reconstruction D.3 |
| la fermeture de tous les parents donne un catalogue complet | `conditional_theorem` | enfants canoniques D.3, incidences naturelles réconciliées, shell et rang globaux exacts |
| la seule déduplication des labels réconcilie les coutures de fragments | `false_in_general` | une représentation canonique ou un complexe de coutures certifié est requis |
| propager directement un complexe de fragments avec coutures | `proof_obligation` | optimisation future : couverture, appariement, plans non générateurs, aucune perte ni double compte des strates naturelles |
| les profondeurs $m\leq m_{\star}=s_{\max}-2$ détectent toute violation de `RelevantGP` utile | `conditional_theorem` | ensembles $A$ à support propre, de taille au plus $s_{\max}$ et sans intrus strict; toutes les strates et shells sont énumérés |
| dépasser $s_{\max}$ autorise à certifier un shell global sans finir les égalités | `false_in_general` | [`relevant_gp_extra_shell_above_smax.json`](../../morsehgp3d/tests/fixtures/spatial/relevant_gp_extra_shell_above_smax.json) : un point extérieur au candidat reste sur le shell au rang fermé trois lorsque $s_{\max}=2$ |
| un moteur de puissance flottant certifie la cellule | `false_in_general` | il ne fait que proposer |
| une liste $L$-NN fixe contient tous les supports | `false_in_general` | contre-exemple de la paire de Gabriel |
| matérialiser la mosaïque d'ordre supérieur est nécessaire | `false_in_general` | le raffinement restreint suffit à l'énumération |
| la sortie intermédiaire est toujours linéaire | `false_in_general` | pire cas 3D superlinéaire ou quadratique |
| le coût moyen est linéaire sur Poisson volumique à ordre fixé | `theorem_external` pour les comptes étudiés | résultat moyen, pas borne déterministe du code |
| tous les générateurs saturés 3D sont obtenus par supports de tailles un à quatre | `proved_here` | existence d'un support minimal de miniball; supports multiples agrégés par boule exacte et saturé |
| l'implémentation par supports de tailles un à quatre est exhaustive | `conditional_theorem` | univers des supports complet, classifications fermées exactes, déduplication et lots certifiés |
| borner les générateurs saturés par $K_{\mathrm{eff}}+1$ suffit aux ordres demandés | `false_in_general` | une petite face peut avoir un saturé de cardinal arbitrairement proche de $n$ |
| la saturation est monotone pour l'inclusion | `false_in_general` | contre-exemple exact de la section 1 de [`TOUR_BOULES_SATUREES.md`](TOUR_BOULES_SATUREES.md) |
| le pruning par inclusion s'intègre sans preuve à la forêt insertionnelle persistante | `proof_obligation` | l'inclusion préserve les complexes de coupe, mais la suppression exige contraction, rewiring et provenance historiques |

## 4. Attaches et lots

| énoncé | statut | condition |
|---|---|---|
| la descente miniball diminue strictement $\beta$ | `conditional_theorem` | support essentiel et shell extérieur vide |
| chaque segment reste sous le niveau précédent | `proved_here` | convexité et décroissance stricte |
| une descente d'un bras connu trouve sa composante globale | `conditional_theorem` | chemin initial correct et catalogue antérieur complet |
| le pointer-jumping préserve la racine | `proved_here` | DAG fonctionnel certifié |
| contraction des plateaux par composantes fortement connexes | `proof_obligation` | exactitude locale à démontrer |
| traitement séquentiel de niveaux égaux | `false_in_general` | peut binariser une multifusion |
| hyper-Kruskal par lot préserve les composantes | `proved_here` | incidences complètes et niveaux exacts |
| un sous-flot certifié produit une connectivité partielle incluse dans l'exacte | `proved_here` | garantie unilatérale; les nœuds de la `partial_forest` ne sont pas des événements HGP |
| l'ancre rang $s$ relie la naissance `full_pi0` dans $T_s$ à l'état post-lot de $T_{s-1}$ | `conditional_theorem` | $2\leq s\leq K_{\mathrm{eff}}$ et deux tranches complètes; ce minimum isolé est omis par `hgp_reduced` |
| une ancre Morse existe toujours comme nœud source de `hgp_reduced` | `false_in_general` | pour $s\geq2$, utiliser `locate_reduced_root`; l'ancre ne contrôle que les sources réduites effectivement représentées |
| tous les carrés verticaux commutent après propagation | `proof_obligation` logiciel | propriété à tester sur chaque run exact |

## 5. Prédicats numériques

| décision | exigence du mode exact |
|---|---|
| comparaison de distances | signe exact relatif aux coordonnées dyadiques |
| centre critique | déterminants homogènes canoniques $(C_x,C_y,C_z,D_c)$ avec $D_c>0$, sans quotient flottant |
| niveau carré | fraction canonique **ExactLevel** $=(N,D)$, $D>0$; comparaison par le signe de $N_1D_2-N_2D_1$ |
| appartenance à une cellule | filtre avec borne d'erreur puis fallback exact |
| intersection de plans | témoin par plans liants, pas seulement coordonnées flottantes |
| barycentriques | signes exacts et support minimal canonique |
| rang intérieur et shell | parcours global avec bornes AABB sûres |
| boîte de clipping | padding dyadique strict avec $\mathrm{conv}(X)\subset\mathrm{int}(\Omega)$ |
| égalité de niveaux | comparateur algébrique exact, jamais epsilon |
| hash de label | collision vérifiée par le label complet |
| overflow | file secondaire ou arrêt explicite, jamais troncature |
| environnement FTZ ou DAZ | filtre FP64 désactivé et fallback exact obligatoire |

Les recettes binary64 de 2A.8c sont évaluées directement comme polynômes homogènes. Si une recette produit une ligne affine brute $\widetilde{r}_i$ et que le plan exact stocke sa primitive orientée $r_i=\lambda_i\widetilde{r}_i$ avec $\lambda_i>0$, cette réduction conserve tous les signes scientifiques utilisés. Pour l'orientation 2D, $n=\lambda\widetilde{n}$ donne $\mathrm{sign}\left(n\mathbin{\cdot}((b-a)\mathbin{\times}(c-a))\right)=\mathrm{sign}\left(\widetilde{n}\mathbin{\cdot}((b-a)\mathbin{\times}(c-a))\right)$. Pour trois plans, la multiplication indépendante des lignes par des facteurs strictement positifs conserve les rangs normal et augmenté; dans l'ordre canonique des plans, elle conserve aussi le signe du déterminant des normales. Enfin, si $N$ est la matrice des trois normales liantes, $p$ leur unique intersection, $f_4$ la quatrième forme et $H$ le déterminant homogène des quatre lignes affines, alors $H=\det(N)f_4(p)$; ainsi $\mathrm{sign}(f_4(p))=\mathrm{sign}(\det(N))\mathrm{sign}(H)$ sans division flottante. Ces identités justifient les filtres, mais chaque objet riche reste revérifié par le témoin rationnel exact.

Une expansion flottante certifie un signe lorsqu'une borne d'erreur l'exclut de zéro; elle ne représente pas une division exacte. Tout centre ou niveau ambigu est matérialisé par le fallback big-int ou rationnel. Une erreur de signe invalide le certificat entier, même si la partition finale semble plausible.

La fixture permanente `morsehgp3d/tests/fixtures/predicates/orientation_daz_regression.json` enregistre une contradiction numérique découverte pendant la Phase 2A. Son déterminant exact vaut $2^{-75}>0$, alors qu'une première sonde qui comparait des valeurs sous-normales sous DAZ laissait le filtre s'activer et certifier un signe négatif. La sonde active inspecte désormais MXCSR sur x86 et les bits d'une opération sous-normale sur les autres cibles. Sur x86 avec SSE2, les tests activent séparément DAZ, FTZ et leur combinaison, exigent `FilterResult::uncertain`, puis vérifient le signe positif par `cpu_multiprecision`; les autres cibles exercent la sonde générique sans prétendre muter un registre matériel non portable. Cette régression interdit toute certification filtrée lorsque les sous-normaux ne sont pas préservés.

La fixture permanente `morsehgp3d/tests/fixtures/predicates/affine_power_proper.json` enregistre une contradiction de représentation découverte pendant le développement du noyau affine 2A.4. Pour $R=\left\lbrace (2,0,0)\right\rbrace$ et $Q=\left\lbrace (0,0,0)\right\rbrace$, la forme exacte est $H_{R,Q}(y)=-4x+4$, tandis que son plan homogène primitif est $-x+1=0$. Une première version non intégrée divisait les coefficients de la forme par leur PGCD puis exposait encore son évaluation comme la valeur exacte de $H_{R,Q}$. `ExactAffineForm3` conserve désormais séparément l'échelle rationnelle exacte et la clé homogène primitive; le replay v3 vérifie les quatre coefficients exacts et le plan classifié. Cette régression interdit de substituer un représentant homogène à une valeur de coût, même lorsque son signe et son lieu zéro sont préservés.

## 6. Dégénérescences

| cas | première politique certifiée | extension requise |
|---|---|---|
| événements distincts de même niveau | lot exact | aucune |
| points coplanaires avec support indépendant | traiter dans l'enveloppe affine | prédicats dimensionnels |
| support barycentrique sur une face | réduire au support minimal | canonisation exacte |
| doublons | refuser d'abord, puis agréger en multiplicités | oracle multiensemble |
| shell critique de plus de quatre points | `unsupported_degeneracy` | arrangement local sur $S^2$ et preuve d'isotopie |
| point extérieur sur la frontière d'un candidat Gabriel utile | `unsupported_degeneracy` et `relevant_gp_complete=false` | `RelevantGP` ne quantifie que les $A$ de taille au plus $s_{\max}$, à support propre et sans intrus strict |
| égalité extérieure rencontrée par le locator non-Gabriel | `unsupported_degeneracy` | certificat individuel du locator; ne suffit pas à invalider `RelevantGP` |
| plateau de successeurs | `unsupported_degeneracy` | quotient multivalué prouvé |
| presque-dégénérescence | fallback exact | aucune perturbation |

La position générale n'est pas une tolérance numérique. Un cas presque cosphérique est générique s'il est exactement non cosphérique; il doit être décidé comme tel.

## 7. DTM et régularisation entropique

| proposition | statut |
|---|---|
| la DTM quadratique est la moyenne des $k$ premières distances carrées | `theorem_external` |
| une puissance ou entropie lisse le choix top-$k$ | `proved_here` au niveau local |
| ce lissage conserve les sous-niveaux HGP pour température positive | `false_in_general` |
| l'entropie réduit le nombre de cellules top-$k$ à certifier | `false_in_general` |
| le cas $k=1$ entropique remplace la structure EMST | `false_in_general` |
| la continuation propose efficacement des candidats | `heuristic` |
| une borne de troncation peut parfois exclure un candidat | `conditional_theorem` si la borne globale est fournie |

Conclusion normative : l'entropie régularise un **oracle local**, pas la définition de la hiérarchie. La régularisation utile à MorseHGP3D est une génération sparse accompagnée de certificats d'exclusion.

## 8. GPU et performance

| affirmation | statut |
|---|---|
| clipping de cellules indépendantes, tris et DSU sont GPU-friendly | `proved_here` au sens structurel |
| Paragram fournit une primitive de proposition pertinente | `heuristic` tant que ses sorties ne sont pas certifiées |
| FP32 seul suffit | `false_in_general` |
| FP32 puis filtres exacts peut conserver la sémantique | `conditional_theorem` logiciel |
| une proposition de distances binary64 conserve les égalités exactes et peut fixer le cutoff top-$k$ | `false_in_general` | [`gpu_fp64_tie_split.json`](../../morsehgp3d/tests/fixtures/spatial/gpu_fp64_tie_split.json) : les deux distances exactes valent $2^{-106}$, tandis que la projection RN-even propose zéro et $2^{-104}$ |
| une énumération GPU complète suivie d'une réévaluation CPU exacte de tout site admissible conserve top-$k$, shell et rang fermé | `proved_here` | le retour GPU est d'abord vérifié comme permutation de tous les `PointId`; aucune valeur proposée n'entre dans le calcul exact exhaustif qui construit la partition canonique |
| la ronde CUDA Borůvka à graine fixe est qualifiée sur G4 | `validated_software` | SHA `9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e`, AOT CUDA 12.9 limité à `sm_120` sans PTX, replay réel conforme à la décision CPU exacte, `memcheck` et `racecheck` passés, puis cible relue `TERMINATED`; artefact `phase5-k1-boruvka-9f29ffcb9ba8ca66f3cfc7c0c9285c34cbeee70e.json` de SHA-256 `cf6e58e6f35b3fbbc1d1357b0504072b6db28240858b195ae3002e0dc6b5b74e`; aucune contraction, forêt complète, scalabilité ou promotion de statut public n'en découle |
| la boucle hybride Borůvka complète avec rejeu indépendant est qualifiée sur G4 | `validated_software` | l'implémentation sépare les trois payloads, libère les candidats après chaque ronde, rejoue une seconde chaîne GPU et compare le témoin à l'ancre CPU; au SHA `c199651d86e861eb755357986d036889839578d4`, le replay réel couvre singleton, chaîne $8\to4\to2\to1$ et carré à ex æquo, ferme les poids exacts $8127$ et $8127/4$, AOT `sm_120` sans PTX, `memcheck` et `racecheck`; artefact de SHA-256 `b10e3bb8c94d6e8fa0f70223d5faa99d94a5144701a87c087587753c912a7215`, cible relue `TERMINATED`; le résultat reste `local_emst_witness_only` avec `hierarchy_reduction_status=not_performed`, et le chunking reste ouvert |
| $50\,000$ points en moins d'une seconde | `experimental_target` |
| un million de points exacts tient toujours en VRAM | `false_in_general` |
| diffuser cellules et incidences peut préserver l'exactitude | `conditional_theorem` | chaque objet fermé avant éviction, merge externe exact |
| l'Unified Memory rend automatiquement le calcul scalable | `false_in_general` |

## 9. Verrous ouverts prioritaires

### V0 — contre-exemple à la réduction de Gabriel

La fixture `tests/fixtures/regressions/gabriel_point_set_counterexample.json`, issue du cas automatiquement minimisé `tests/fixtures/regressions/oracle_campaign/oracle-campaign-5b8d545852e858b3313346708d5341b223d0233e75a1a08e99aa6cfb3df4f385.json`, contient cinq points de dimension affine trois, sans dégénérescence exacte et avec `relevant_gp_complete=true`. À l'ordre deux et au niveau carré fermé $83886/3563$, le graphe complet de facettes Gamma possède l'unique union de points `(0,1,2,3,4)`, tandis que le flot Gabriel élagué possède les deux unions recouvrantes `(0,1,2)` et `(0,2,3,4)`.

La facette `(0,2)` est attachée dès le niveau $33/2$ par deux cofaces non-Gabriel. Cette attache ne change pas alors l'union de points de sa composante, mais elle devient décisive lorsqu'une coface Gabriel la réutilise au niveau $83886/3563$. La preuve par induction des pages 90–91 du chapitre 8 conserve seulement les unions de points et oublie cette incidence; le flot élagué retarde la fusion jusqu'au niveau $24$.

La décision corrective v2 préserve la cible HGP : `hgp_reduced` exact est désormais construit depuis Gamma exhaustif sur `reference_cpu`, avec `proof_basis=gamma_exhaustive_reference`. Le flot Gabriel brut garde seulement `proof_basis=gabriel_positive_connectivity` et une sémantique `partial_refinement`. Enrichir ce flot par les incidences silencieuses ou construire un locator horizontal complet reste une piste de réduction; la base réservée `incidence_complete_reduction_proved` ne pourra entrer dans un schéma actif qu'après preuve et tests de falsification. La phase 0 est refermée après l'audit du contrat v2, la requalification Gamma de la phase 1 est terminée, la phase 2A est fermée avec `G1=go` pour les prédicats exacts CPU et la phase 3 a qualifié l'environnement CUDA G4. La phase 2B est fermée après qualification de la mesure chaude finale; la phase 4 est fermée après qualification de l'oracle spatial exact `reference_cpu` et du parcours LBVH parallèle recertifié; la phase 5 est active sur l'ancre $k=1$. Ses cinq premiers jalons CPU construisent l'EMST exact, décident globalement chaque boule diamétrale, contractent les seules paires de rang fermé deux, certifient l'égalité des coupes, multifusions et poids avec le graphe Gabriel diagnostique, passent le différentiel indépendant sur 50 nuages jusqu'à $n=14$, remplacent les couvertures persistantes par une forêt compacte à cinq arènes linéaires, puis produisent et vérifient un témoin Borůvka exact directement sur le LBVH sans matérialiser le graphe complet. La voie hybride `cuda_g4` propose maintenant toutes les rondes dans un contexte producteur résident; `reference_cpu` recertifie les sur-ensembles, décide les minima selon $\kappa$ et contracte canoniquement jusqu'au témoin EMST, puis un second contexte GPU et l'ancre CPU rejouent séparément la chaîne. Cette boucle complète et son témoin EMST local sont qualifiés sur G4, sans hiérarchie ni publication. La porte de phase reste ouverte jusqu'à l'émission chunkée à mémoire physique bornée et à sa qualification scalable; la porte G2 globale reste distincte.

Le réaudit du contrat v2 a aussi isolé une omission de rejeu dans `overlap-k2` : deux cofaces Gamma ajoutaient respectivement les facettes `(0,3)` et `(1,4)` sans modifier l'union des points, mais leurs deltas étaient absents du journal. La fixture minimale `tests/fixtures/regressions/gamma_q1_coverage_delta.json` fige ce cas. Le validateur reconstruit désormais chaque état post-lot depuis l'état strict pré-lot et exige l'égalité exacte avec les trois copies normatives du journal : lot, forêt et résultat.

Le premier pré-calcul de la campagne de fermeture 2A a été interrompu avant publication lorsqu'un contre-audit a montré que le générateur `splitmix64-counter-v1` classait comme bien conditionné le cas `H_RQ` d'indice 32 avec `R=Q`, donc identiquement nul. Ce défaut ne contredisait pas le prédicat exact, mais invalidait le décompte expérimental prétendant dépasser dix millions de signes stricts. La fixture minimale `morsehgp3d/tests/fixtures/predicates/campaign_well_conditioned_zero_regression.json` fige le contre-exemple; le générateur v2 impose des labels distincts et un resampling entier déterministe borné de toute nullité fortuite. Aucune racine v1 n'a été gelée.

Une seconde contradiction de rejeu concernait les flèches verticales : comparer la couverture source au `MergeNode` cible figé à sa naissance rejette à tort une racine qui a grandi ensuite sans nouvelle fusion. La fixture minimale `tests/fixtures/regressions/vertical_q1_growth_target.json` fixe quatre points colinéaires où la cible d'ordre deux naît avec `012`, gagne `3` par croissance `q=1`, puis reçoit au niveau fermé `4` la composante source d'ordre trois couvrant `0123`. Le validateur consulte désormais le dernier snapshot post-lot fermé au niveau de la flèche; le nœud de naissance reste immuable.

### V1 — corollaire complet de reconstruction

Formaliser, dans le langage de la théorie de Morse des sélections continues, que minima de rang $k$, germes d'indice un et attaches sous-niveau suffisent à reconstruire exactement $\pi_0(L_k(a))$, événements simultanés compris. Le test exhaustif est nécessaire mais ne remplace pas la preuve.

### V2 — dégénérescences cosphériques

Prouver l'équivalence entre les germes locaux de $D_k$ et un arrangement directionnel fini sur $S^2$ lorsque le shell contient plus de quatre points. Déduire une règle de lot et d'attache indépendante de toute perturbation.

### V3 — reconstruction canonique et streaming

La v1 certifiée reconstruit chaque enfant dans sa H/V-représentation canonique par D.3 avant propagation; les coutures des fragments n'entrent pas dans le chemin exact. Il reste à prouver les invariants de streaming : une cellule fermée contre l'oracle global peut être évincée sans être invalidée par une future colonne, et les signatures de frontières naturelles suffisent à réconcilier les incidences après merge externe. La propagation directe d'un complexe de fragments avec coutures demeure une optimisation séparée, assortie d'une preuve d'absence de perte et de double compte.

### V4 — longueur des descentes

Obtenir des bornes moyennes ou au moins des lois empiriques robustes par famille de nuages. Si les descentes dominent, un flot complété en incidences peut devenir le candidat de réduction, sans remplacer la base Gamma exhaustive avant sa preuve.

### V5 — sortie sensible à $H_0$

À partir des profils de complexité, rechercher un branch-and-bound certifié qui exclut des régions sans fermer tous les parents top-$m$. Tant qu'un tel oracle ne possède pas de preuve de complétude, il reste le mode `budgeted`.

### V6 — tour globale de boules saturées

Les théorèmes S.1–S.6 de [`TOUR_BOULES_SATUREES.md`](TOUR_BOULES_SATUREES.md) donnent une représentation combinatoire exacte de Gamma par saturés et une forêt de Kruskal commune aux ordres. Il reste à construire un oracle indépendant borné, à convertir ses coupes en `MergeForest`, `coverage_log` et applications verticales canoniques, puis à certifier la persistance et les dégénérescences. La voie brute possède jusqu'à $O(n^4)$ supports, $O(n^5)$ memberships et $O(M^2)$ paires de générateurs; elle est donc un oracle petit $n$, pas un remplacement scalable de la voie actuelle. Une sous-famille proposée puis saturée reste `partial_refinement`.

## 10. Règles de publication d'un résultat

Une expérimentation ou une API ne peut employer le mot `exact` que si elle publie :

- le profil demandé et obtenu;
- les hypothèses d'entrée;
- la complétude du catalogue par rang;
- la complétude des attaches par ordre;
- le statut des lots et morphismes verticaux;
- `relevant_gp_complete`, `canonical_children_complete` et `active_cross_incidences_complete`;
- la sémantique `exact` ou `partial_refinement` de la forêt;
- le nombre de fallbacks et d'ambiguïtés restantes;
- les budgets et la raison d'arrêt;
- les tailles intermédiaires et le pic mémoire;
- la version du code, du compilateur, du pilote et du matériel.

Le contrat v2 ferme l'ambiguïté de définition sans fermer la recherche de réduction : `hgp_reduced` ne peut publier `exact` qu'avec `hgp-reduced-v2`, `gamma_exhaustive_reference` et `effective_backend=reference_cpu`. Le flot Gabriel brut reste conditionnel, même si son exécution est bit-à-bit déterministe et son catalogue critique exhaustif. La tour saturée n'est pas encore une base de preuve du schéma actif; une future base distincte demanderait une migration contractuelle. Le profil `full_pi0` conserve séparément son statut conditionnel lié à M.1 dans le contrat courant.

Un bon ARI, une stabilité sous bruit ou un accord moyen avec une baseline ne remplace aucune de ces preuves.
