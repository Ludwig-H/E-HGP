# Coeur MEB : profil mesuré, et le gain qui survit à une réfutation

11 septembre 2026, second auditeur (session e-hgp-c6), sur `99b4d3b1`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Aucune source active modifiée : tout est prototypé dans un
arbre isolé obtenu par `git archive 99b4d3b1`.

Cette note ne demande rien. Elle apporte une mesure et un prototype vérifié sur
le goulot réel, reproductible par le [banc joint](receipts_coeur_meb_20260911/README.md), à la demande de l'utilisateur de travailler le mur de
performance plutôt que l'outillage de test.

## 1. Le chiffre qui manque : `tour_s` est chronométré en un seul bloc

À 50k, K1..10, la construction FULL coûte 389,7 s sur 418,9 s, soit 93 %. Mais
[la sonde](../bench/full_ball_tower_probe.cpp) chronomètre `full_ball_tower`
d'un seul tenant. Or ce bloc agrège deux travaux de nature opposée :

- la **géométrie**, résolution de chaque représentant vers sa boule terminale,
  démontrée séparable du calendrier, donc parallélisable ;
- le **calendrier**, fermeture des lots par niveau, union-find sur les racines
  pré-lot, installation des ancres et émission de 27,3 M nœuds, séquentiel.

Sans cette découpe, la loi d'Amdahl est inapplicable et personne ne peut dire ce
que le parallélisme rapporterait. C'est la mesure la moins chère à ajouter et
elle conditionne tout le reste.

Second point de mesure : `power_tests` et `materializations` sont **comptés dans
`AnchorMebWork` mais jamais imprimés**, et `supports_by_size` n'est publié que
sous forme de somme. Le travail réellement payé par le noyau dominant n'est donc
pas observable depuis les reçus.

## 2. Profil réel du noyau, jamais publié jusqu'ici

Banc privé sur 39 364 appels MEB, nuage `uniform` u16 de 20 000 points, facettes
prises comme plus proches voisins, K de 2 à 10.

| K | candidats/appel | tests de puissance/appel |
| ---: | ---: | ---: |
| 2 | 1,0 | 2,0 |
| 5 | 16,1 | 32,9 |
| 8 | 83,4 | 131,5 |
| 10 | 200,1 | 272,3 |
| moyenne | 58,1 | 88,5 |

`anchor_meb` énumère en force brute lexicographique tous les couples, puis tous
les triplets, puis tous les quadruplets, et teste pour chaque candidat la
puissance de **tous** les sites. Le coût explose donc en K⁴. Les
matérialisations valent exactement une par appel : tout le reste est du candidat
rejeté.

**Et la distribution des K est croissante.** Le bloc par ordre de la sonde donne,
à 8k, 161 128 requêtes à K=2 contre 2 534 359 à K=10 : K≥7 concentre **73,8 %**
des requêtes et 76,8 % des clés uniques. La forme est stable à l'échelle, 74,2 %
et 77,3 % à 32k. Les ordres les plus chers sont aussi les plus fréquents.

## 3. Trois optimisations exactes

Chacune préserve le contrat « premier support admissible, plus petit q d'abord,
ordre lexicographique ». Vérification : sortie comparée champ par champ
(`status`, `key`, `level`, `support_size`, `support_slots`,
`selected_shell_count`) sur les 39 364 cas, **zéro divergence**.

**a. À q=2, seule la paire de distance maximale peut réussir.** Si une paire
(c,d) est plus éloignée que (a,b), alors c et d ne tiennent pas simultanément
dans la boule de diamètre |ab|. On passe de 45 candidats à typiquement 1, sans
rien changer au résultat.

**b. Tester d'abord les deux points extrêmes dans la boucle de confinement.**
Pur réordonnancement. La plupart des candidats sont rejetés, et ils le sont en
deux tests au lieu de K.

**c. Calculer la MEB d'abord, puis canonicaliser sur la coquille.** *Le théorème
ci-dessous reste vrai, mais ma réalisation par Welzl est réfutée, cf. § 9.* Le théorème :
tout candidat accepté définit une boule contenant tous les sites avec son support
au bord ; une telle boule **est** la MEB, qui est unique. Donc les points du
support canonique sont tous sur la sphère de la MEB, c'est-à-dire dans la
coquille. Il suffit d'énumérer les sous-ensembles de la **coquille**, dans le
même ordre. Comme `extra_records` vaut 4 sur 21,5 M boules à 50k, la coquille est
presque toujours de taille 2, 3 ou 4 : la canonicalisation est quasi gratuite.

## 4. Résultat mesuré, après réfutation puis réparation

Ma première voie Welzl est réfutée (§ 9). Elle a ensuite été **réparée**, et la
réparation est mesurée ici. Deux étages sont distingués.

Étage 1, les deux optimisations prouvées exactes (a) et (b) seules, sans aucun
Welzl. Étage 2, en ajoutant le Welzl réparé et la canonicalisation (c).

| grandeur | référence | (a)+(b) seules | (a)+(b)+Welzl réparé |
| --- | ---: | ---: | ---: |
| candidats par appel | 58,0 | 41,1 | **14,1** |
| tests de puissance par appel | 88,4 | 29,5 | 33,6 |
| temps mural, machine calme | 0,902 s | 0,680 s | **0,219 s** |
| gain en temps | — | 1,33x | **4,12x** |

Vérification, identique pour les deux étages : 196 898 appels aléatoires, les
1 013 cas du balayage exhaustif de la famille de contre-fixtures de l'auditeur
historique, et sa contre-fixture K7 nominale. Comparaison champ par champ,
**zéro divergence**. Pour l'étage 2, le repli de sécurité vers la force brute
**ne se déclenche jamais**, sur aucun des 198 000 cas.

Noter que l'étage 2 fait *plus* de tests de puissance que l'étage 1, 33,6 contre
29,5, tout en étant trois fois plus rapide : il élimine surtout des formations de
candidats, plus coûteuses qu'un test de puissance.

## 4bis. Mesure sur le flux réel du résolveur, le seul juge qui compte

Les chiffres du § 4 sont ceux d'un **banc**, sur des facettes simulées. Voici la
mesure de bout en bout, en remplaçant `anchor_meb` dans un arbre isolé et en
faisant tourner la sonde réelle, n=8000, K1..10, `--static-threads=1`. Tout cas
invalide, dégénéré ou d'échec est délégué à la routine d'origine : seul le chemin
nominal change.

| grandeur | référence | Welzl réparé |
| --- | ---: | ---: |
| `payload_digest` | `cdd77e30…c329b` | **identique** |
| `resolver_meb_calls` | 3 947 627 | 3 947 627 |
| supports testés | 340 615 272 | **23 092 967** |
| `tower_s` | 61,23 s | **45,13 s** |

**L'objet est inchangé bit à bit** et la tour gagne **1,357x**, avec 14,75 fois
moins de formations de candidats. Le gain réel sur les candidats dépasse
largement les 4,13x du banc, parce que la distribution réelle des facettes penche
davantage vers les K élevés que mon substitut par plus proches voisins.

**Confirmé à une seconde échelle.** À n=16000, les deux digests sont de nouveau
identiques (`8eb94f8f…f4e45`) et la tour passe de 139,15 s à 107,03 s, soit
**1,300x**. Le gain recule légèrement avec la taille, 1,357x puis 1,300x, ce qui
est cohérent avec la part géométrique qui recule aussi, 62,8 % puis 61,0 %. Les
mesures se recoupent donc entre elles. Sortie brute :
[`flux_reel_16k.out`](receipts_coeur_meb_20260911/flux_reel_16k.out).

Mais le gain de **tour** est 1,357x, pas 4,12x, et l'écart est structurel : le
noyau MEB n'est qu'une partie de la géométrie, laquelle ne pèse que 62,8 % de
`tower_s`. La phase géométrique passe d'environ 38,5 s à 22,4 s, soit **1,72x**.
Un gain spectaculaire sur un noyau ne se transporte jamais tel quel.

C'est mon quatrième chiffre sur ce sujet, après 2,9x annoncé, 1,33x après
réfutation et 4,12x sur banc après réparation. **Celui-ci est le seul mesuré de
bout en bout sur le moteur réel avec un digest identique**, et c'est celui qu'il
faut retenir. Sortie brute : [`flux_reel.out`](receipts_coeur_meb_20260911/flux_reel.out),
correctif : [`realflow_patch.py`](receipts_coeur_meb_20260911/realflow_patch.py).

## 5. Un résultat négatif conservé

Ma première version de Welzl laissait l'ensemble de base grossir jusqu'à dix
points et rappelait la force brute sur tous ses sous-ensembles à chaque
itération. Elle était **2,2 fois pire** en candidats et 2,8 fois pire en
puissances, tout en restant exacte. Borner la base à quatre points, ce qu'autorise
le fait qu'une MEB en dimension trois est déterminée par au plus quatre points,
est exactement ce qui renverse le résultat. Qui reprendra cette piste doit le
savoir.

## 6. Ce que cette note **ne** dit pas

Le banc mesure le noyau MEB **isolé**, pas la tour. La part de ce noyau dans
`tower_s` n'est pas instrumentée, donc je ne convertis pas 2,9x en un gain de
tour, et je ne revendique ni 50k en une seconde, ni 100 ms, ni aucun régime
massif. Mes ensembles de sites sont des plus proches voisins, un substitut
raisonnable des facettes réelles, pas le flux exact du résolveur. Le prototype
n'est pas du code produit : il n'a ni les mutants, ni les gardes de dépassement,
ni les portes O2/SAN du moteur.

Ordre de travail que je suggère : instrumenter d'abord la découpe
géométrie/calendrier et publier `power_tests`, puis décider du parallélisme sur
une mesure propre. Les lignes statique1 contre statique4 des reçus **ne sont pas**
une mesure de parallélisme : leurs écarts de MEB valent exactement 237 557,
501 258 et 1 045 620, c'est-à-dire les MEB évitées par le semis après échange.
Elles comparent deux moteurs, pas deux nombres de fils.

Enfin, ma campagne CTest locale sur `6763a877` ne montre **aucun échec** sur les
448 tests terminés des 449.

## 7. Deux observations de passage

Le paquet en préparation `receipts/census_tower_permanent_20260911/` installe la
porte permanente demandée, cible CMake et onze CTests. Attention avant de le
committer : `python3 tools/check_docs.py` échoue sur sa copie embarquée
`sources/current/morsehgp3D_v7/bench/COMPARE_MONO.md`, dont deux liens relatifs
`../docs/...` ne résolvent plus depuis leur nouvel emplacement. C'est une porte
CI. Le remède habituel du dossier est de stocker ces copies en `.source` ou
d'embarquer leurs dépendances.

Mon banc et cette note n'introduisent aucune erreur documentaire.

## 8. Erratum du 11 septembre

La première version de cette note attribuait à n=8000 un bloc par ordre qui
provenait en réalité de **n=200**, et en tirait « 2 964 requêtes à K=2 contre
29 606 à K=10, K≥7 concentre 69 % ». Les vraies valeurs à 8k sont 161 128 et
2 534 359, avec 73,8 % pour K≥7.

Les deux gains pondérés publiés, 4,42x et 1,98x, étaient calculés sur cette
distribution de n=200. Les valeurs correctes sont 4,53x et 1,99x à 8k. L'erreur
était donc **conservatrice** : la distribution réelle penche davantage vers les
ordres élevés, où l'hybride gagne le plus. Les mesures du banc, les compteurs par
K et le 2,9x mural ne sont pas touchés : ils ne dépendent d'aucune distribution.

## 9. Erratum 2 : la voie Welzl est réfutée

L'auditeur historique a construit une contre-fixture à sept sites,
`(2,3,2) (2,0,0) (0,2,2) (1,0,0) (2,2,0) (3,0,1) (0,2,3)`, sur laquelle mon
hybride rend `kInvariantViolated` avec la raison `canon_fail` là où `anchor_meb`
rend un support canonique `{0,1,5,6}`. **Je l'ai reproduite moi-même** : ma
proposition Welzl rend une boule dont le site 6 a une puissance de `+176`, donc
hors de la boule. Voir [leur reçu](receipts_meb_boundary_20260911/README.md).

La cause est exactement celle qu'ils identifient, et l'erreur est mienne. Mon
`welzl_rec` borne bien `R` à quatre sites, mais son cas de base calcule la **MEB
de `R`**, ce qui peut laisser des points de `R` strictement à l'intérieur. La
récursion de Welzl exige une boule passant **par** `R`, tous ses points sur la
frontière. Le théorème « quatre points suffisent à définir la MEB » ne justifie
pas cette substitution.

Deux atténuations, ni l'une ni l'autre n'excusant le défaut. Le mode de
défaillance est un **refus d'entrée valide**, pas un faux succès géométrique,
parce que la canonicalisation revérifie le confinement et refuse. Et leur premier
échec apparaît au 307 985e essai : mes 39 364 tirages ne pouvaient pas le
trouver, ce qui est une leçon sur la taille d'échantillon exigée par ce genre de
prédicat.

Ce qui tombe : la voie K≥7, donc les 2,9x muraux et les 4,5x pondérés.
Ce qui tient : les optimisations (a) et (b), prouvées exactes et indépendantes de
Welzl, ainsi que le théorème de canonicalisation (c) lui-même. Le gain survivant
est mesuré au § 4.

**La réparation est faite et mesurée.** Le cas de base construit désormais la
boule passant par `R` via `q3_form` et `q4_form` appelés directement, en ne
gardant que les gardes de non-dégénérescence `g > 0` et `det > 0`, et en retirant
les filtres de minimalité de `form()`, acuité pour q3 et centre intérieur pour
q4. Ces filtres décident si un support est un support de MEB valide ; ils n'ont
rien à faire dans une construction de boule frontière. C'était exactement mon
erreur.

Résultat : la contre-fixture K7 passe, le repli ne se déclenche sur aucun des
198 000 cas, et le gain passe à 4,12x (§ 4). La réfutation de l'auditeur
historique a donc produit un résultat meilleur que ma proposition initiale, et le
mérite lui en revient. Source : [`welzl2.cpp`](receipts_coeur_meb_20260911/welzl2.cpp).

Zéro repli sur 198 000 cas est une évidence empirique forte, pas une preuve.
Une intégration produit demanderait le repli gardé de l'auditeur comme filet.

