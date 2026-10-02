# Morse HGP 3D v11

Ouverte le 2 octobre 2026, sur `main`. Base de code **neuve** : la v10 est un sujet différentiel et une source de
fixtures ; tout ce qui en est repris est un port explicite, épinglé et requalifié
([provenance](docs/PROVENANCE.md)).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
mode=implementation_v11_full_forests
public_status=not_claimed
```

## Demande de l'utilisateur (2 octobre 2026)

« Repartir de zéro pour avoir quelque chose de plus propre. Mêmes contrats : 100 ms sur nuages LiDAR sans sol
(éventuellement avec sol), avec K = 5 et si possible K = 10. Toujours très rigoureux mathématiquement. Il faut ensuite
se comparer à HDBSCAN, sur données synthétiques mais aussi sur données réelles. Il y a notamment des tests dans le
dossier `Zoltan/` où la hiérarchie HDBSCAN échoue ; on peut en trouver d'autres. Il faudrait des exemples où la
hiérarchie HGP réussit. » Puis : commencer sans attendre la fin de l'audit de la v10, en reprenant les bases très
solides.

Précision ultérieure : « Il faut aussi passer à u21 voire u24 ». Le défaut de compilation devient u21 ;
u18 et u24 restent explicites. La voie native q1/q2/q4 couvre les trois profils ; q3 conserve les entiers
larges en u21/u24. La qualification de cette voie est épinglée à `9df774947` ; les reçus distinguent les trois profils.
Le niveau q4 différé est qualifié séparément à `ffc2ff95f`, avec sorties exactes et mémoire inchangées.
L'index global et son census sont qualifiés à `e8520481d`, aux trois profils et sous ASan18/24 et TSan21.

Priorité réaffirmée : poursuivre le développement jusqu’à **200 ms sur G4
pour FULL K=1..5**, puis viser K=1..10. L’étude de la hiérarchie de points
et la comparaison théorique et pratique à HDBSCAN sur `Zoltan/` viennent
ensuite. Toute la v10 peut inspirer la v11, avec examen critique et
requalification explicite des ports.

## Objet

Pour $k = 1, \ldots, K_{\max}$ et $a \geq 0$, soit $D_k(y)$ le carré de la distance de $y$ à son $k$-ième plus proche point et $L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace$. La **tour FULL** est, pour chaque $k$, l'arbre de fusion des composantes connexes de $L_k(a)$ quand $a$ croît, avec les applications verticales $L_{k+1}(a) \subseteq L_k(a)$ ; les niveaux sont des rationnels exacts. La v11 calcule ensuite une hiérarchie laminaire sur les points, à comparer à celle de `sklearn.cluster.HDBSCAN` (jamais réimplémenté).

## Ordre des travaux

1. Audit de la v10 : [synthèse et lacunes restantes](docs/AUDIT_V10_SYNTHESE.md).
2. Fondations : statuts, tampons comptés, ordonnanceur, arithmétique exacte à budget de bits, nuage, entrées et
   sorties, oracle de référence exact.
3. Moteur : catalogue critique, tour FULL, comparés octet pour octet à la v10 figée et à l'oracle borné.
4. Hiérarchie de points.
5. Performance sur trames LiDAR (G4), menée avec le moteur avant la hiérarchie de points.
6. Comparaison à HDBSCAN : bancs synthétiques, puis LiDAR réel (démos `Zoltan/demos/` et nouveaux cas).

## Construction

```bash
cmake -S morsehgp3D_v11 -B build/v11 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v11 --parallel
ctest --test-dir build/v11 -LE long --output-on-failure   # portes rapides ; les portes « long » passent sur G4
```

Le défaut est `MHGP11_COORD_BITS=21` ; choisir `-DMHGP11_COORD_BITS=24` pour le domaine 24 bits.

La matrice complète (GCC 11.4 de la VM, ASan + UBSan, TSan, profils 21 et 24 bits, tampons empoisonnés, mutants,
suite complète de la référence) s'exécute sur G4 par `tools/g4_matrix.py`, dans une session gardée
`gcp-migration/v11_session.py` (voir `gcp-migration/README_V11.md`).

## État

Reprise du développement après les audits : [état courant et prochaines tranches](docs/DEVELOPPEMENT.md).
Les résultats d'audit sont des preuves bornées ; chaque port conserve ses propres portes.

| Couche | Fichiers | État au 2 octobre 2026 |
| --- | --- | --- |
| socle | `src/core`, `tests/support`, CMake et outils | qualifié avec num/cloud sur `a97180667` : Release 205/205 ; ASan/UBSan et TSan 130/130 chacun ; premiers échecs conservés |
| oracle de référence | `reference/` (définition $\Gamma_k$, construction, juge, sérialisations) | suite complète et cinq faits cover/MR₂/mémo/LCA/inter-K inclus dans les 251 portes Release G4 à `e8520481d` ; aucun transfert à FULL natif |
| outillage G4 | contrôleur, worker, matrice | captures index et MEB conformes, premiers échecs conservés ; sessions natives arrêtées, échec de capacité sans nouveau démarrage documenté |
| `num`, `cloud` | calcul exact et propriétaire du nuage | qualifié à `e8520481d`, ASan/UBSan u18 et u24 ; défaut21, option24 ; bornes entières sur boîte fermée ; 114 mutants socle/num/cloud détectés |
| `sched`, `io`, CLI | — | restent à intégrer et qualifier |
| catalogue | `src/catalogue` | [port séquentiel](docs/CATALOGUE.md) qualifié à `ffc2ff95f` ; leaf16/u21 : 19,78–24,96 s sur les trois LiDAR/K5, sorties égales en18/21/24 ; K10 au plafond30s ; contrat100ms non atteint |
| index global | `src/index` | [propriétaire et census exact](docs/INDEX.md) qualifiés à `e8520481d` ; arbre LiDAR u21 : 0,341–0,418 ms après Cloud ; 64 requêtes choisies : 0,621–0,810 ms ; sans raccord FULL |
| MEB locale | `src/tower` | [MEB bornée et census](docs/MEB.md) qualifiés à `25792084e`, 1266/1266 + ASan18 55/55 et 18/18 essais ; support strict local distinct du support global |
| filtres de centres et domaine FULL | `num`, `catalogue`, `tower` | [J2 exact](docs/CENTER_REGION.md) et [propriétaire commun](docs/FULL_DOMAIN.md) qualifiés à `7f1922c77` : G4 1398/1398 + ASan18 73/73 ; [mesures mono partielles](receipts/center_region_20261002/README.md) |
| catalogue parallèle et cellules | `sched`, `catalogue`, `tower` | [capture c104](receipts/catalogue_parallel_20261002/README.md) : 1779/1779 + ASan18 107/107, 30 succès K5, six délais K10 à15s ; catalogue LiDAR K5/W48 environ3–4,7s, sans FULL |
| forêt FULL | `src/tower` | [plateaux et verticales exactes](docs/FULL_FORESTS.md) implémentés ; première qualification arrêtée par un défaut du test IO, correctif c6ca345e0 en rejeu G4 |
| cache J2 et tri indirect | `src/catalogue` | [deux options exactes](docs/CATALOGUE_OPTIMISATIONS.md), désactivées par défaut ; ablation et qualification propres en préparation |
| points et tête | — | hiérarchie et sélection à développer après le jalon moteur |

## Audits ouverts

Deux auditeurs suivent la v11 en continu. Leurs notes courantes sont dans [`audits/`](audits/) ; **tout agent qui
écrit ou relit un module lit d'abord celles qui le concernent et traite leurs constats** (correction et porte, ou
contestation argumentée). Les réponses du développeur sont les fichiers `REPONSE_CLAUDE_*` du même dossier.

## Lire d'abord

1. [Architecture](docs/ARCHITECTURE.md) : modules, règles, profil numérique.
2. [Provenance](docs/PROVENANCE.md) : ce qui est porté de la v10, depuis quelle source, comment c'est requalifié.
3. [Canal des audits](audits/README.md).
4. [Mathématiques](docs/MATHEMATIQUES.md) et [conception du moteur](docs/CONCEPTION_MOTEUR.md).
