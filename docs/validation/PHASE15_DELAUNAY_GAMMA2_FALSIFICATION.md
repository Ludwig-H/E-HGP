# Phase 15 — falsification et correction Delaunay de Gamma$_2$

## Périmètre annoncé

- phase : 15, porte d'entrée satisfaite, porte de sortie inchangée;
- backend de falsification : `python_oracle`;
- backend géométrique étudié : `ordinary_delaunay_1_skeleton`;
- profile : `hgp_reduced`;
- mode : `bounded_exact_falsification_then_proved_one_edge_reduction`;
- déploiement : `architecture_only`;
- statut public : `not_claimed`.

Cette tranche ne construit aucune mosaïque de Delaunay d'ordre supérieur. L'oracle Gamma exhaustif est borné aux petites fixtures et ne devient pas l'architecture produit.

## Verdict

Le surrogate Morton reste réfuté dès l'ordre un par le différentiel historique à 50 k. À l'ordre un, l'EMST extrait du graphe de Delaunay ordinaire reste le candidat exact sous les hypothèses génériques habituelles, après recertification des niveaux.

À l'ordre deux, la restriction historique aux triplets possédant au moins deux arêtes de Delaunay est fausse. La première campagne positive de 2 684 coupes exactes n'était pas une preuve; une fixture générique à six points produit maintenant une divergence exacte. Les fermetures locales par étoile, carré du graphe et fan de faces sont également fausses sur une fixture générique à huit points.

La correction combinatoire consiste à garder tous les triplets possédant au moins une arête du 1-squelette complet de Delaunay. Sous position générale, cette famille préserve exactement les facettes actives et les composantes de Gamma$_2$ réduit à toute coupe stricte ou fermée. La preuve est dans [DELAUNAY_ORDINAIRE_GAMMA2.md](../math/DELAUNAY_ORDINAIRE_GAMMA2.md).

Cette correction n'est pas le chemin produit : son énumération explicite est trop grande de plusieurs ordres de grandeur et violerait l'invariant architectural de MorseHGP3D.

## Fixture à six points : wedges à deux arêtes

La fixture [`delaunay_two_edge_gamma2_counterexample.json`](../../tests/fixtures/regressions/delaunay_two_edge_gamma2_counterexample.json) est de dimension affine trois et en position générale exacte. Ses trois tétraèdres de Delaunay et leurs marges de puissance externes sont rationnellement certifiés. Elle possède vingt cofaces Gamma$_2$.

- la règle à deux arêtes garde 18 cofaces sur 20;
- les cofaces $124$ et $245$ sont omises;
- les deux naissent au niveau carré exact $281/4$ avec support $24$;
- à la coupe fermée, la facette $24$ manque dans la restriction;
- la couverture reste l'unique ensemble de six points, donc un test limité aux unions de points aurait produit un faux positif;
- la règle à une arête inclut les vingt cofaces et restaure cette fixture.

## Fixture à huit points : rayon graphe deux

La fixture [`delaunay_local_gamma2_counterexample_n8.json`](../../tests/fixtures/regressions/delaunay_local_gamma2_counterexample_n8.json) est générique, avec 70 orientations et 56 déterminants cosphériques non nuls. Ses sept tétraèdres ont des sphères exactement vides. Son champ informatif `minimality` enregistre une exploration par suppression pour l'étoile fermée; ce champ n'est pas une obligation rejouée par le checker et aucune minimalité globale n'est revendiquée.

À la coupe fermée de niveau carré exact $13\,956\,479\,554$ :

- Gamma$_2$ possède 56 cofaces et active les 28 facettes;
- `two_edge` garde 42 cofaces;
- `closed_star`, `square_clique` et `link_face_fan` en gardent chacun 50;
- les quatre variantes omettent la facette $01$;
- les cofaces simultanées causales $014$, $016$ et $017$ ne possèdent chacune qu'une arête de Delaunay;
- les extrémités $0$ et $1$ sont à distance trois dans le graphe de Delaunay et n'ont aucun centre d'étoile fermé commun;
- `one_edge` garde ici les 56 cofaces et reproduit l'état exact.

Le checker recalcule les miniboules avec `Fraction`, compare les facettes, les partitions et les couvertures à toutes les coupes strictes et fermées, puis exige ces deux divergences à chaque exécution. Les fixtures ne dépendent donc pas d'un niveau binary64 ni d'un digest de seule couverture.

Pour ne pas tester `one_edge` seulement sur des cas où il serait exhaustif, la fixture positive [`delaunay_one_edge_gamma2_positive_n9.json`](../../tests/fixtures/regressions/delaunay_one_edge_gamma2_positive_n9.json) possède 84 cofaces Gamma et 82 candidats. Les triplets sans arête $014$ et $568$ sont réellement omis; les 168 états stricts ou fermés restent identiques. Pour les trois fixtures, le checker énumère exactement chaque circumball de quatre sites, refuse toute incidence cosphérique et exige que le catalogue des sphères vides coïncide avec les tétraèdres stockés avant de construire le graphe de Delaunay.

## Théorème à une arête

Pour toute miniboule active $B(c,r)$, le sous-graphe de Delaunay induit par $S=X\cap B(c,r)$ est connexe. En effet, le segment radial d'un site $p\in S$ vers $c$ traverse uniquement des cellules de sites qui restent dans $B(c,r)$. Un arbre couvrant de ce sous-graphe suffit alors à connecter toutes les paires de $S$ par des triplets contenant une arête de Delaunay et dont la miniboule a un rayon au plus $r$.

Il en découle que toute relation Gamma$_2$ exhaustive est déjà réalisée, au même niveau ou avant, par des relations `one_edge`. L'inclusion inverse est immédiate puisque chaque triplet `one_edge` est une vraie coface Gamma. L'égalité vaut pour les facettes actives et leur partition, aux seuils ouverts comme fermés.

Cette preuve suppose le 1-squelette complet du nerf de Voronoï. Une triangulation SoS particulière en cas de cosphéricité doit être recertifiée ou échouer explicitement; aucune indépendance aux dégénérescences n'est revendiquée.

## Écrans exploratoires non archivés

Une exploration ad hoc a parcouru 6 144 nuages entiers aléatoires : 1 200 pour chacune des tailles cinq à neuf, 64 à dix points, 64 à douze points et 16 à quatorze points. Les niveaux Gamma ont été recalculés exactement et les relations ont été contractées par plateaux. La règle à deux arêtes y échoue dès six points; les trois fermetures de rayon deux y échouent dès huit points; `one_edge` n'y diverge sur aucun cas. La topologie de Delaunay aléatoire vient de SciPy et n'est pas recertifiée cellule par cellule. Ni le script ni un artefact de cette exploration ne sont archivés : elle ne constitue donc pas une validation reproductible du dépôt et n'est pas citée comme preuve dans `implementation_status.toml`.

Le seed exact 9 000 005 à neuf points omet réellement deux triplets sans arête de Delaunay sur 84 et reproduit néanmoins les 168 états stricts ou fermés. La campagne ne se réduit donc pas aux petits cas où `one_edge` coïnciderait accidentellement avec Gamma exhaustif.

Deux écrans flottants ad hoc, eux aussi non archivés et sans autorité de preuve, ont complété ce rejeu. Le premier comparait le premier niveau d'activation de chaque facette sur 20 000 nuages de dix et douze points. Le second comparait activations et nombre de composantes par plateaux sur 66 000 nuages de cinq à vingt points. Aucun n'a trouvé de divergence `one_edge`. Ces observations ne qualifient ni l'implémentation du dépôt, ni les niveaux binary64, ni les dégénérescences, ni un statut public.

## Échelle et SLO

Le run historique à 50 k possède 385 152 arêtes de Delaunay. L'énumération directe `one_edge` peut donc produire jusqu'à $385\,152\times49\,998=19\,256\,829\,696$ occurrences avant déduplication. L'univers final des facettes contient $\binom{50\,000}{2}=1\,249\,975\,000$ paires. Le run historique à 10 000 001 points possède 77 589 517 arêtes, soit jusqu'à 775 895 092 410 483 occurrences `one_edge`.

Ces bornes ferment le no-go de l'algorithme explicite : il ne peut ni matérialiser son univers, ni satisfaire le p95 `warm_e2e` strictement inférieur à 100 ms à 50 k/$K=10$. Accélérer par radix sort les 4 396 699 wedges historiques ne corrigerait pas leur incomplétude. Aucun benchmark GPU supplémentaire de cette relation réfutée n'a donc été lancé.

Le prochain chemin autorisé est implicite et sensible à la sortie : utiliser la connexité radiale comme certificat dans les descentes sparse de Phase 15, ne produire que les attaches utiles à la réduction hiérarchique, conserver une voie exacte pour toute ambiguïté et ne jamais créer l'arène des paires ou des triplets absents. Le SLO reste ouvert; `deployment_status=architecture_only` et `public_status=not_claimed` restent inchangés.

## GCP

Le préflight a consulté en lecture seule la cible `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a`. Elle est restée `TERMINATED`, `g4-standard-48`, `SPOT`, avec `instanceTerminationAction=STOP` et `maxRunDuration=3600`. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée pendant cette tranche.

Un suivi ultérieur et séparé a exécuté sur cette classe de cible le gate conditionnel de couverture Gabriel, sans réhabiliter la restriction Gamma$_2$ réfutée. Ses runs jusqu'à 30 000 001 points, ses garde-fous et son arrêt ciblé sont documentés dans le [rapport archivé](../archive/abandoned/phase15/PHASE15_GABRIEL_COVERAGE_G4.md).
