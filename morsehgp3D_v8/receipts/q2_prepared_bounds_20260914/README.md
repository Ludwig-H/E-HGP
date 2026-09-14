# Bornes q2 préparées — comparaison de révisions

14 septembre 2026. `cpu_reference`, mono-thread, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. **Composant sur un rectangle**, pas
la tour HGP FULL ni une exécution G4. GCP non utilisé.

## Résultat

Les constantes de chaque tâche Shared sont préparées une fois, dans
48 octets sans pointeur. Le parcours ne change pas : candidates, tâches,
visites, supports et digests sont identiques entre les deux révisions.
Le [contrat](../../docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md) distingue
la preuve arithmétique, les coûts et les objets proposés pour la suite massive.

À n32k/Kmax10/s8, médianes de trois mesures **par ordre et par révision**,
temps total du composant Shared, préparation et collecte incluses :

| Famille | Ordre des bras | Ancien (ms) | Préparé (ms) | Baisse observée |
| --- | --- | ---: | ---: | ---: |
| Grille | individuel puis partagé | 137,69 | 132,12 | 4,0 % |
| Grille | partagé puis individuel | 137,96 | 130,13 | 5,7 % |
| Nappe complète | individuel puis partagé | 2607,36 | 2499,14 | 4,2 % |
| Nappe complète | partagé puis individuel | 2642,34 | 2391,09 | 9,5 % |

La baisse du seul comptage partagé est respectivement 7,7 %, 7,5 %,
5,8 % et 11,8 %. Ce sont des observations exploratoires, sans intervalle
de confiance. Le bras individuel, dont l'algorithme n'a pas changé,
varie lui aussi de −4,2 % à +9,6 % sur ces quatre comparaisons : bruit
machine, code généré et chronologie interdisent d'attribuer chaque écart
à la seule économie de produits. Aucun choix automatique de Shared n'est
introduit. À 32k, il n'est pas systématiquement plus rapide que Pairwise.

## Croissance et mémoire

Les 8k→16k→32k sont réellement exécutés, sans projection. Tous les s et
les deux bras/ordres sont inclus dans les intervalles ci-dessous :

| Famille | Rapport de temps total à chaque doublement | Rapport de visites de comptage |
| --- | ---: | ---: |
| Grille | 1,65–2,27 | 1,56–2,04 |
| Nappe complète | 1,98–2,19 | 2,09–2,19 |
| Facteurs déséquilibrés | 1,58–2,40 | 1,74–1,92 |

Ces ratios restent inférieurs à 4 dans **ces familles**. La transformation
ne change aucune classe de complexité ; le coût ajouté par tâche est O(1)
et l'état constant est de 48 octets. Elle n'ajoute aucun tableau A×A,
B×B ou A×B. La somme des visites sur une vraie WSPD et le volume de sortie
ne sont toujours pas bornés globalement par ces essais.

Le coût résiduel reste visible : sur la nappe32k, 3 928 390 candidates,
438 362 descripteurs, 6 844 491 tâches Shared, 146 039 452 visites de
comptage, 13 830 224 visites de couverture, puis 44 739 766 visites de
collecte. Sont émis 1 625 608 IDs intérieurs et 4 271 328 IDs de coquille,
avec répétitions entre supports. La collecte par boule canonique reste
à traiter ; un tri après cette émission ne rembourse pas son coût.
RSS de pointe et VRAM ne sont pas mesurées dans cette campagne.

## Protocole et filiation

Quatre campagnes closes, 124 lignes au total, chacune comparant deux bras :

- `baseline/main_matrix` puis `candidate/main_matrix` : 54 lignes chacun,
  trois familles, trois tailles, Kmax10, s8/10/12, intersection Pool,
  deux ordres, une mesure par configuration.
- `candidate/focus_32k` puis `baseline/focus_32k` : huit lignes chacun,
  grille/nappe32k, Kmax10, s8, deux ordres, deux mesures supplémentaires.

Les processus sont séquentiels, sans compilation ni grande campagne
constructeur concurrente, sans échauffement préalable. Les ordres des
bras ne sont jamais mélangés dans les médianes. Le paramètre s vérifie
la séparation des rectangles fixes ; **aucune WSPD s8/10/12 n'est générée**.
Kmax10 est le seuil de census, pas dix hiérarchies calculées. Aucune
nouvelle mesure 50k : celles de la quatrième tranche restent historiques.

La référence est `f4815cd42d572db6aef27ec73d100f52303fff26`. Son binaire
épinglé est `243387ac6df40e3f0f0c38a1d7d7a86735ed260ba11d904bf032bc4eb1c6e432`.
Le runner est lu dans l'ancien export `v8_index_census_20260913.5OH9JA` ;
ses 18 sources ont été confrontées aux objets Git de f481. La candidate
utilise 19 sources, dont le nouveau header, et le binaire
`b76591c05ffd2be8d77979366a1ee3c4526e5913e37cdac24a43b60277b27079`.
Les deux emploient le runner `892bd3ae…`, GCC 13.3, Release ; les
manifestes conservent les options complètes, commandes, machine, sources,
bruts et hashes de fermeture. Le HEAD de contexte et le worktree sale
ne se substituent jamais aux pins des contenus effectivement consommés.

[VALIDATION.json](VALIDATION.json) authentifie chaque révision séparément
et vérifie l'identité des entrées, sorties et compteurs. [SUMMARY.json](SUMMARY.json)
est une projection déclarée des temps total/comptage/collecte, du travail
et des doublements ; les données complètes restent dans les bruts.
Le [comparateur](../../bench/compare_q2_revisions.py) recalcule le résumé
intégral avec `--summary`, sans écrire ni réétiqueter les preuves :

```bash
python3 -B morsehgp3D_v8/bench/compare_q2_revisions.py morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/baseline morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/candidate --summary
python3 -B -O morsehgp3D_v8/bench/compare_q2_revisions.py morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/baseline morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/candidate
```

La lecture future demande les sources candidate épinglées ; l'ancienne
campagne q2 de 204 lignes se relit à f481, pas avec ce nouveau moteur.
Les petites sondes préparatoires hors fenêtre de mesure ne font pas
partie de ce résultat. Le [reçu de qualification](QUALIFICATION.json)
conserve 50 pins de sources/juges et les XML de 34 CTests en Release
et sous ASan/UBSan (deux tranches disjointes 32+2 par build). La nouvelle
primitive passe 1 424 cas, huit modèles faux et six rejets de boîtes ;
le comparateur passe deux contrôles positifs, dont les médianes connues,
et 20 mutants, également sous Python −O. Les données synthétiques des
gates ne sont pas des chronométrages produit.

Le premier export neuf passe les 34 CTests complets (87,88 s) et produit
le même binaire de capture octet pour octet. Avant publication, l'auditeur
signale que la comparaison omettait les options de lien et IPO. Leurs
valeurs réelles sont compatibles : aucune mesure n'est invalidée. Le
lecteur inclut désormais ces familles, suffixes par configuration compris,
avec quatre mutants supplémentaires. Les premières qualifications et leurs
pins sont conservés dans [l'archive explicite](BEFORE_LINKER_PROFILE_CHECK.json).
Les deux gates modifiées sont requalifiées sur les deux builds, et une
nouvelle suite complète est exécutée dans l'export actualisé. Le reçu
courant distingue ces étapes ; aucun moteur, binaire ni brut de mesure
n'est modifié pour cette correction de lecteur.

Décision : conserver la préparation exacte à coût constant, mais passer
maintenant au propriétaire/index globaux et aux contextes de rectangles,
puis aux continuations distribuables. Ne pas consacrer une nouvelle
tranche aux seules micro-variantes de ce calcul. Contrats de tour sur G4,
q3/q4, parents FULL et plusieurs dizaines de millions restent ouverts.
