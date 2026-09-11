# Coeur MEB : profil mesuré, trois optimisations exactes, et 2,9x vérifié

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
à 8k : 2 964 requêtes à K=2 contre 29 606 à K=10, et K≥7 concentre 69 % des
requêtes. Les ordres les plus chers sont aussi les plus fréquents.

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

**c. Calculer la MEB d'abord, puis canonicaliser sur la coquille.** Le théorème :
tout candidat accepté définit une boule contenant tous les sites avec son support
au bord ; une telle boule **est** la MEB, qui est unique. Donc les points du
support canonique sont tous sur la sphère de la MEB, c'est-à-dire dans la
coquille. Il suffit d'énumérer les sous-ensembles de la **coquille**, dans le
même ordre. Comme `extra_records` vaut 4 sur 21,5 M boules à 50k, la coquille est
presque toujours de taille 2, 3 ou 4 : la canonicalisation est quasi gratuite.

## 4. Résultat mesuré

Aiguillage : force brute avec (a) et (b) pour K<7, Welzl borné plus
canonicalisation (c) pour K≥7. **Jamais pire que l'existant, à aucun K.**

| K | candidats actuel → hybride | puissances actuel → hybride |
| ---: | ---: | ---: |
| 3 | 3,2 → 1,4 | 8,1 → 4,2 |
| 6 | 30,2 → 16,6 | 55,8 → 19,6 |
| 10 | 200,1 → 30,9 | 272,3 → 112,1 |

Gains moyens à K uniforme : 3,94x sur les candidats, 1,94x sur les puissances.
Pondérés par la distribution réelle des K : **4,42x** et **1,98x**.

Temps mural, trois exécutions consécutives, machine se calmant :

| exécution | référence | hybride | rapport |
| --- | ---: | ---: | ---: |
| 1 | 0,183 s | 0,062 s | 2,95x |
| 2 | 0,175 s | 0,058 s | 3,01x |
| 3 | 0,168 s | 0,058 s | 2,89x |

Soit environ **2,9x sur le noyau MEB**, à sortie identique.

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

