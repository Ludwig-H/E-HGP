# Croissance du travail — tranche33, captures closes

## Conclusion

Les nouveaux filtres corrigent le mauvais saut du travail H/Xi observé sur
le scan0 entre16k et32k : en mode Affine, les principaux comptes du filtre
restent sous×3 aux deux doublements, pour K5 et K10. Ils ne réduisent pas
le travail situé après la sélection exacte des arêtes : les couvertures,
graines, census q3 et traitements q4 appariés sont inchangés.

Le calcul complet q3/q4 n'est donc pas devenu globalement sous-quadratique.
Local28 conserve des visites de carte à×4,018 puis×5,603 sur scan0/K5.
Sur scan200, le premier doublement donne×4,317 pour les bornes du census
q3 de comptage et×6,540 pour les visites de carte q4. Ces contre-régimes
font partie du résultat, pas d'une annexe écartée du verdict.

Les temps sont des observations sous charge concurrente, pas une preuve
d'accélération stable. Aucun nouveau calcul GCP, GPU, FULL ou de tour50k
n'a été effectué par cette analyse.

## Autorités, fermeture et couverture réelle

L'[analyseur](analyze_growth.py), hors des211 sources produit qualifiées,
porte le SHA256 `052b7d6e6c09babf84438488226bf586255c1418b4162eb49f02b3e1b950f8b1`.
Il appelle les lecteurs historiques32 et33, sans exécutable natif.
Les [résultats normaux](GROWTH_ANALYSIS.json) et
[optimisés Python−O](GROWTH_ANALYSIS_OPTIMIZED.json) sont identiques hors
métadonnées d'exécution. Le [readback fermé](GROWTH_READBACK.json) épingle
et revérifie292 fichiers avant/après, dont les deux rapports, l'analyseur,
les lecteurs, les211 sources courantes, les entrées, binaires et reçus.
Les captures anciennes conservent leurs propres manifestes : aucune ligne
historique n'est convertie en exécution33.

| Capture close | Mesures réellement analysées |
|---|---|
| [33 comparaison](performance/lidar_cuwsvxnv/COMPLETION.json) | 6 : scan0/8k/K5/s8/Local28, W1 et W4, Legacy/Exclusion/Affine |
| [33 matrice principale](performance/lidar_gladtapx/COMPLETION.json) | 12 : scan0,8k/16k/32k,K5/10,Local28/Window30,W4,s8,Affine |
| [33 séparation](performance/lidar_8yueb3v7/COMPLETION.json) | 6 : scan0,8k/16k/32k,K5,Window30,W4,s10/12,Affine |
| [33 autres scans](performance/lidar_h2s4wo62/COMPLETION.json) | 6 : scans100/200,8k/16k/32k,K5,Local28,W4,s8,Affine |
| [32 matrice de référence](../q34_indexed_20260921/lidar/lidar_86twby55/COMPLETION.json) | 12 : scan0,8k/16k/32k,K5/10,Local28/Window30,W4,s8 |
| [32 séparation de référence](../q34_indexed_20260921/separation/lidar_2w21ewdx/COMPLETION.json) | 6 : scan0,8k/16k/32k,K5,Window30,W4,s10/12 |

Soit48 observations,30 nouvelles et18 historiques, et28 doublements
analysés. Le scan0/8k/K5/Local28/Affine/W4 apparaît dans deux captures :
les deux chronos restent publiés, le travail déterministe identique forme
un seul point de croissance. Tous les cas utilisent le front `samples`,
le masque6, les témoins `rectangle-pair` et le census q3 `boxes`.

Les modes Legacy/Exclusion/Affine ne sont tous mesurés en33 qu'à8k/K5/
Local28. Aux autres tailles, la comparaison est Affine33 contre la vraie
référence32, et non contre une exécution Legacy33 inventée. Les nouveaux
scans100/200 ne couvrent ni Window30, ni K10, ni une référence32 appariée.

### Ce qui a effectivement été comparé

- 36 appariages de nombres de sorties et digests complets somme+xor,
  y compris entre backends, séparations et versions ; les SHA256 des
  fichiers d'entrée appariés sont identiques.
- 23 comparaisons de tout le travail aval, dont21 interversions : front,
  préparation partagée et tous les champs aval sont conservés. Seuls le
  travail du filtre, la pré-expansion avant le filtre exact des paires et
  deux pics de réemploi de buffers sont exclus de cette projection.
- Un appariage Legacy32/Legacy33 vérifie aussi tous les anciens compteurs
  du filtre et de pré-expansion. Les nouveaux compteurs y valent zéro.

Les deux pics normalisés sont uniquement `work.q3.peak_shell_bytes` et
`work.peak_edge_buffer_bytes`. Leur capacité dépend de l'ordre des jobs.
Les autres maxima ne sont pas effacés. Les grandes captures utilisent
des digests, pas les listes intégrales de candidats : cette comparaison
ne se présente donc pas comme un nouvel oracle exhaustif de grands nuages.

## Méthode : travail payé, populations et sous-comptes

Le JSON conserve tous les compteurs bruts et une liste de postes principaux
avec leurs chemins exacts. Les tableaux suivants comptent des évaluations,
visites, comparaisons et lectures réellement effectuées, pas une somme
arbitraire censée représenter le temps CPU.

Une évaluation Xi peut servir à l'exclusion et à l'admission : elle n'est
comptée qu'une fois. En revanche, leurs comparaisons de seuil sont deux
tests distincts et leur somme est publiée. Les tests H/Xi spécifiques
Affine sont des sous-comptes des tests H/Xi généraux, pas un supplément
à leur ajouter. Les douze nouveaux compteurs restent tous dans le rapport.

Les bornes q3 préparées pour un enfant finalement non visité sont du vrai
travail déjà inclus dans `count_bounds_prepared`. On ne les ajoute pas
une seconde fois. Les masses de couverture, populations de blocs admis,
sites hérités et `total_unordered_pairs=n(n−1)/2` ne sont pas des lectures
individuelles ; leur ratio ne devient pas un coût scalaire. Une branche
`depth_stops`, en revanche, compte réellement une décision d'arrêt de
construction : ce n'est ni une profondeur maximale ni une capacité mémoire.

## Les filtres : amélioration réelle, mais pas gratuite

Scan0/8k/K5/Local28, mêmes comptes pour W1 et W4 ; valeurs en millions.

| Travail rectangle+paires | Legacy | Exclusion | Affine |
|---|---:|---:|---:|
| Bornes H | 61,494 | 34,880 | 33,756 |
| Bornes Xi, total | 24,653 | 26,249 | 25,629 |
| Dont Xi malgré Hmin≤0 | 0 | 18,997 | 18,564 |
| Tests d'admission par voie | 38,899 | 7,702 | 7,589 |
| Tests d'exclusion par voie | 0 | 43,578 | 42,522 |
| Admission+exclusion | 38,899 | 51,280 | 50,111 |

Les visites H baissent, mais les évaluations Xi et les comparaisons de
seuil augmentent ici. Présenter seulement les admissions aurait caché
une partie importante du nouveau travail.

Sur scan0/Affine/s8, valeurs en millions ; les deux ratios portent sur
8k→16k puis16k→32k. Les deux backends donnent le même travail de filtre.

| K | Poste | 8k / 16k / 32k | Ratios |
|---|---|---|---|
| 5 | H | 33,756 / 83,150 / 212,533 | 2,463 / 2,556 |
| 5 | Xi | 25,629 / 64,766 / 171,302 | 2,527 / 2,645 |
| 5 | Admission+exclusion | 50,111 / 125,814 / 325,597 | 2,511 / 2,588 |
| 10 | H | 80,355 / 193,469 / 456,637 | 2,408 / 2,360 |
| 10 | Xi | 62,861 / 154,474 / 371,322 | 2,457 / 2,404 |
| 10 | Admission+exclusion | 134,800 / 323,989 / 764,785 | 2,403 / 2,361 |

À32k/K5, H passe de701,165M en32 à212,533M en33, Xi de443,262M à
171,302M. Pour les seules recherches de paires, le dernier doublement
passe de×5,364 à×2,773 pour H, et de×7,440 à×2,926 pour Xi. EnK10,
les deux comptes totaux32k passent de1490,427M/969,582M à456,637M/
371,322M. Cela qualifie une baisse de travail sur ces observations,
pas un coût asymptotique général ni un gain de temps stable.

## L'aval reste le poste à traiter

Scan0/s8/W4, valeurs en millions. Ces valeurs aval sont identiques entre
les exécutions32/33 appariées : la tranche33 ne les optimise pas.

| Poste | K | 8k / 16k / 32k | Ratios |
|---|---:|---|---|
| Produits du front | 5 | 1,104 / 2,310 / 4,632 | 2,093 / 2,005 |
| Visites de couverture | 5 | 18,621 / 46,776 / 117,231 | 2,512 / 2,506 |
| Visites génération q3 | 5 | 16,402 / 42,171 / 113,462 | 2,571 / 2,691 |
| Boules/graines q3 | 5 | 1,911 / 6,033 / 20,363 | 3,156 / 3,375 |
| Bornes q3, comptage+coquille | 5 | 72,936 / 225,238 / 760,044 | 3,088 / 3,374 |
| Bornes q3, comptage+coquille | 10 | 342,892 / 861,715 / 2253,761 | 2,513 / 2,615 |
| Local28, bornes de partition+tests points | 5 | 80,895 / 247,266 / 866,771 | 3,057 / 3,505 |
| Local28, visites de requête | 5 | 24,982 / 100,372 / 562,408 | **4,018 / 5,603** |
| Local28, sites balayés | 5 | 12,960 / 27,234 / 57,555 | 2,101 / 2,113 |
| Local28, tris+groupes | 5 | 19,076 / 38,706 / 78,909 | 2,029 / 2,039 |
| Window30, comparaisons+orientations de sélection | 5 | 310,863 / 1112,923 / 4121,191 | 3,580 / 3,703 |
| Window30, deux scans | 5 | 32,140 / 78,870 / 188,712 | 2,454 / 2,393 |
| Window30, comparaisons de racines | 5 | 51,335 / 124,862 / 296,480 | 2,432 / 2,374 |
| Window30, comparaisons+orientations de sélection | 10 | 2558,195 / 7015,460 / 22038,384 | 2,742 / 3,141 |
| Window30, deux scans | 10 | 609,665 / 1505,548 / 3698,060 | 2,469 / 2,456 |
| Window30, comparaisons de racines | 10 | 1591,726 / 3877,237 / 9439,393 | 2,436 / 2,435 |

La préparation partagée reste petite : comparaisons d'unicité0,122/0,266/
0,561M, visites de préparation d'index0,275/0,585/1,239M. Les sortiesK5
croissent à0,105/0,212/0,427M callbacks et0,325/0,658/1,328M IDs de
coquille, soit environ×2 par doublement. Les tris de coquilles q3 donnent
0,328/0,663/1,336M comparaisons. Le volume de sortie ne suffit donc pas
à expliquer les mauvais sauts de visites de carte.

Les agrégats ne doivent pas masquer leurs sous-postes : Local28/K5,
16k→32k, a des bornes de blocs seules à×4,112, des tests de ligne à×5,492
et des nœuds laissés indécis à×5,849. EnK10, les décisions `depth_stops`
font×4,454. Le JSON conserve aussi les sauts des sous-classes de contacts,
constantes et fenêtres ponctuelles ; ils ne sont pas additionnés à leurs
totaux parents. Aucun de ces chiffres ne prouve que la fenêtre30 supprime
tous les pires cas : ses deux scans et sa sélection restent coûteux.

## Scans100/200 : le contre-régime est conservé

K5/s8/Local28/W4/Affine uniquement, valeurs en millions.

| Scan | Poste | 8k / 16k / 32k | Ratios |
|---|---|---|---|
| 100 | Bornes q3 comptage+coquille | 135,872 / 332,119 / 642,454 | 2,444 / 1,934 |
| 100 | Visites carte q4 | 47,959 / 130,086 / 495,961 | 2,712 / 3,813 |
| 200 | Boules/graines q3 | 6,075 / 25,577 / 44,844 | **4,210** / 1,753 |
| 200 | Bornes q3 de comptage | 237,043 / 1023,301 / 1434,621 | **4,317** / 1,402 |
| 200 | Bornes q3 comptage+coquille | 241,566 / 1033,249 / 1456,605 | **4,277** / 1,410 |
| 200 | Visites carte q4 | 109,117 / 713,668 / 1775,980 | **6,540** / 2,489 |

Au premier saut du scan200, les tests de ligne font×6,672 et les bornes
de blocs de partition×4,355. Au second saut, les raffinements terminaux
passent de72 à30607 (×425,097) et les décisions d'arrêt de352 à30773
(×87,423). Ce sont des opérations de construction réelles, même si leurs
bases initiales sont faibles et si les agrégats dominants restent alors
sous×4. Sur scan100, les raffinements8→62→1 sont aussi publiés, sans
transformer un seul petit ratio défavorable en loi asymptotique.

## Séparation s8/10/12, comparaison distincte

Scan0/K5/Window30/W4/Affine. Les sorties appariées restent identiques.
La matrice principale mesure seulement s8 ; s10/12 viennent de leur
capture séparée close, pas d'une interpolation.

| Poste à32k, en millions | s8 | s10 | s12 |
|---|---:|---:|---:|
| Produits du front | 4,632 | 5,402 | 6,082 |
| H rectangle+paires | 212,533 | 198,495 | 193,989 |
| Xi rectangle+paires | 171,302 | 157,060 | 152,212 |
| Tests admission+exclusion | 325,597 | 299,233 | 289,740 |

| Ratio16k→32k | s8 | s10 | s12 |
|---|---:|---:|---:|
| Front | 2,005 | 2,040 | 2,072 |
| H rectangles | 2,317 | 2,345 | 2,370 |
| H paires | 2,773 | 2,625 | 2,550 |
| Xi rectangles | 2,350 | 2,381 | 2,408 |
| Xi paires | 2,926 | 2,759 | 2,669 |

Une séparation plus forte paie davantage de front et réduit les recherches
aval du filtre. La comparaison ne suffit pas à désigner un s optimal
universel. Le travail q3/q4 après la sélection exacte reste inchangé.

## Chronos et portée du verdict

Les deux observations répétées8k/K5/Local28/Affine/W4 valent7,740s et
7,725s préparation partagée incluse ; aucune n'est supprimée. Dans la
matrice principale, les32k/K5 donnent52,295s enLocal28 et48,277s en
Window30 ; K10 donne195,547s et377,050s. Voir les
[conditions concurrentes](ENVIRONMENT.md) avant toute comparaison temporelle.
Ce ne sont ni des tempsGPU, ni la tourFULL, ni un contrat50k.

Les tests montrent une correction ciblée de la croissance du filtre sur
ces nuages, tout en laissant des coûts majeurs q3/q4 inchangés et des
doublements au-delà de×4. La prochaine réduction doit partager les
recherches de graines/census et les visites de cellules, pas seulement
accélérer les comparaisons déjà minoritaires. Cette phrase est une
orientation de travail, pas une optimisation implémentée ou qualifiée33.
