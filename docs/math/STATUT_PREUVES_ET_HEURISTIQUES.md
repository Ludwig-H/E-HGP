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
| Gabriel préserve les K-polyèdres non triviaux | `false_in_general` pour le graphe élagué formulé dans le manuscrit | la fixture exacte `gabriel-point-set-counterexample-5-points-v1` contredit la proposition 6 comme égalité de collections d'ensembles de points; une formulation enrichie en incidences reste à étudier |
| un K-MST élagué préserve ces composantes | `false_in_general` pour le graphe élagué formulé dans le manuscrit | le théorème 5 hérite du contre-exemple à la proposition 6; une structure corrigée peut demander plus que le K-graphe brut |
| centre critique si et seulement si bien centré | `theorem_external` | Reani–Bobrowski, position générale |
| indice $\mu=s-k$ | `theorem_external` | Reani–Bobrowski |
| multiplicité locale d'indice $\mu$ égale à $\binom{\lvert U\rvert-1}{\mu}$ | `theorem_external` | Reani–Bobrowski; ce rang local ne préjuge pas des attaches globales |
| à l'indice un, le sous-niveau strict possède les $\lvert U\rvert$ bras $F_u$ | `theorem_external` | une seule sphère peut tuer jusqu'à $\lvert U\rvert-1$ classes de $H_0$ |
| seuls les rangs $s=k$ et $s=k+1$ modifient $H_0$ | `theorem_external` | conséquence des indices zéro et un |
| une sphère de rang $k+1$ équivaut à un $k$-simplexe de Gabriel | `proved_here` | position générale du manuscrit |
| une étoile remplace la clique de facettes pour $H_0$ | `proved_here` | tous niveaux, traitement simultané |
| une vraie naissance isolée possède un rang fermé $k$ | `proved_here` | modèle de facettes complet |
| le flot de Gabriel brut reconstruit le profil normatif `hgp_reduced` | `false_in_general` | contre-exemple exact générique à $k=2$; le profil ne peut plus publier `exact` comme hiérarchie des K-polyèdres sans définition et preuve corrigées |
| les seules unions de points forment un invariant inductif suffisant pour élaguer les cofaces non-Gabriel | `false_in_general` | une facette peut être attachée silencieusement par une coface non-Gabriel puis participer à une fusion Gabriel ultérieure |
| le flot de Gabriel seul reconstruit toute facette isolée | `false_in_general` comme conséquence automatique du théorème 5 | une attache supplémentaire est requise |
| catalogue critique complet et attaches exactes reconstruisent `full_pi0` | `proof_obligation` | M.1 reste à démontrer, notamment aux niveaux égaux |
| le remplacement intrus–support localise une racine Gabriel verticale | `proved_here` | position générale, baisse stricte de $\beta$ et traitement post-lot |

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
| un moteur de puissance flottant certifie la cellule | `false_in_general` | il ne fait que proposer |
| une liste $L$-NN fixe contient tous les supports | `false_in_general` | contre-exemple de la paire de Gabriel |
| matérialiser la mosaïque d'ordre supérieur est nécessaire | `false_in_general` | le raffinement restreint suffit à l'énumération |
| la sortie intermédiaire est toujours linéaire | `false_in_general` | pire cas 3D superlinéaire ou quadratique |
| le coût moyen est linéaire sur Poisson volumique à ordre fixé | `theorem_external` pour les comptes étudiés | résultat moyen, pas borne déterministe du code |

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

Une expansion flottante certifie un signe lorsqu'une borne d'erreur l'exclut de zéro; elle ne représente pas une division exacte. Tout centre ou niveau ambigu est matérialisé par le fallback big-int ou rationnel. Une erreur de signe invalide le certificat entier, même si la partition finale semble plausible.

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
| $50\,000$ points en moins d'une seconde | `experimental_target` |
| un million de points exacts tient toujours en VRAM | `false_in_general` |
| diffuser cellules et incidences peut préserver l'exactitude | `conditional_theorem` | chaque objet fermé avant éviction, merge externe exact |
| l'Unified Memory rend automatiquement le calcul scalable | `false_in_general` |

## 9. Verrous ouverts prioritaires

### V0 — contre-exemple à la réduction de Gabriel

La fixture `tests/fixtures/regressions/gabriel_point_set_counterexample.json`, issue du cas automatiquement minimisé `tests/fixtures/regressions/oracle_campaign/oracle-campaign-5b8d545852e858b3313346708d5341b223d0233e75a1a08e99aa6cfb3df4f385.json`, contient cinq points de dimension affine trois, sans dégénérescence exacte et avec `relevant_gp_complete=true`. À l'ordre deux et au niveau carré fermé $83886/3563$, le graphe complet de facettes Gamma possède l'unique union de points `(0,1,2,3,4)`, tandis que le flot Gabriel élagué possède les deux unions recouvrantes `(0,1,2)` et `(0,2,3,4)`.

La facette `(0,2)` est attachée dès le niveau $33/2$ par deux cofaces non-Gabriel. Cette attache ne change pas alors l'union de points de sa composante, mais elle devient décisive lorsqu'une coface Gabriel la réutilise au niveau $83886/3563$. La preuve par induction des pages 90–91 du chapitre 8 conserve seulement les unions de points et oublie cette incidence; le flot élagué retarde la fusion jusqu'au niveau $24$.

Avant toute reprise de l'optimisation réduite, il faut choisir puis prouver l'une des voies suivantes : enrichir le flot Gabriel par les incidences silencieuses nécessaires, construire un locator horizontal certifié qui restitue exactement ces attaches, ou redéfinir le profil comme une hiérarchie propre au graphe Gabriel sans l'identifier aux K-polyèdres. Cette dernière voie exige une décision de contrat et ne peut porter le statut public `exact` pour la cible HGP actuelle. La phase 1 reste ouverte et la porte G2 ne peut pas être déclarée satisfaite sur le profil réduit.

### V1 — corollaire complet de reconstruction

Formaliser, dans le langage de la théorie de Morse des sélections continues, que minima de rang $k$, germes d'indice un et attaches sous-niveau suffisent à reconstruire exactement $\pi_0(L_k(a))$, événements simultanés compris. Le test exhaustif est nécessaire mais ne remplace pas la preuve.

### V2 — dégénérescences cosphériques

Prouver l'équivalence entre les germes locaux de $D_k$ et un arrangement directionnel fini sur $S^2$ lorsque le shell contient plus de quatre points. Déduire une règle de lot et d'attache indépendante de toute perturbation.

### V3 — reconstruction canonique et streaming

La v1 certifiée reconstruit chaque enfant dans sa H/V-représentation canonique par D.3 avant propagation; les coutures des fragments n'entrent pas dans le chemin exact. Il reste à prouver les invariants de streaming : une cellule fermée contre l'oracle global peut être évincée sans être invalidée par une future colonne, et les signatures de frontières naturelles suffisent à réconcilier les incidences après merge externe. La propagation directe d'un complexe de fragments avec coutures demeure une optimisation séparée, assortie d'une preuve d'absence de perte et de double compte.

### V4 — longueur des descentes

Obtenir des bornes moyennes ou au moins des lois empiriques robustes par famille de nuages. Si les descentes dominent, le flot de Gabriel reste le chemin normatif réduit.

### V5 — sortie sensible à $H_0$

À partir des profils de complexité, rechercher un branch-and-bound certifié qui exclut des régions sans fermer tous les parents top-$m$. Tant qu'un tel oracle ne possède pas de preuve de complétude, il reste le mode `budgeted`.

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

Tant que V0 n'est pas fermé par une définition et une preuve corrigées, `hgp_reduced` ne peut pas publier `exact` comme hiérarchie des K-polyèdres, même si son exécution est bit-à-bit déterministe et son catalogue exhaustif. Le profil `full_pi0` conserve séparément son statut conditionnel lié à M.1.

Un bon ARI, une stabilité sous bruit ou un accord moyen avec une baseline ne remplace aucune de ces preuves.
