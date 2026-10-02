# Morse HGP 3D v11

Ouverte le 2 octobre 2026, sur `main`. Base de code **neuve** : la v10 est un sujet différentiel et une source de
fixtures ; tout ce qui en est repris est un port explicite, épinglé et requalifié
([provenance](docs/PROVENANCE.md)).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=implementation_v11_catalogue
public_status=not_claimed
```

## Demande de l'utilisateur (2 octobre 2026)

« Repartir de zéro pour avoir quelque chose de plus propre. Mêmes contrats : 100 ms sur nuages LiDAR sans sol
(éventuellement avec sol), avec K = 5 et si possible K = 10. Toujours très rigoureux mathématiquement. Il faut ensuite
se comparer à HDBSCAN, sur données synthétiques mais aussi sur données réelles. Il y a notamment des tests dans le
dossier `Zoltan/` où la hiérarchie HDBSCAN échoue ; on peut en trouver d'autres. Il faudrait des exemples où la
hiérarchie HGP réussit. » Puis : commencer sans attendre la fin de l'audit de la v10, en reprenant les bases très
solides.

## Objet

Pour $k = 1, \ldots, K_{\max}$ et $a \geq 0$, soit $D_k(y)$ le carré de la distance de $y$ à son $k$-ième plus proche point et $L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace$. La **tour FULL** est, pour chaque $k$, l'arbre de fusion des composantes connexes de $L_k(a)$ quand $a$ croît, avec les applications verticales $L_{k+1}(a) \subseteq L_k(a)$ ; les niveaux sont des rationnels exacts. La v11 calcule ensuite une hiérarchie laminaire sur les points, à comparer à celle de `sklearn.cluster.HDBSCAN` (jamais réimplémenté).

## Ordre des travaux

1. Audit de la v10 : [synthèse et lacunes restantes](docs/AUDIT_V10_SYNTHESE.md).
2. Fondations : statuts, tampons comptés, ordonnanceur, arithmétique exacte à budget de bits, nuage, entrées et
   sorties, oracle de référence exact.
3. Moteur : catalogue critique, tour FULL, comparés octet pour octet à la v10 figée et à l'oracle borné.
4. Hiérarchie de points.
5. Performance sur trames LiDAR (G4).
6. Comparaison à HDBSCAN : bancs synthétiques, puis LiDAR réel (démos `Zoltan/demos/` et nouveaux cas).

## Construction

```bash
cmake -S morsehgp3D_v11 -B build/v11 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v11 --parallel
ctest --test-dir build/v11 -LE long --output-on-failure   # portes rapides ; les portes « long » passent sur G4
```

La matrice complète (GCC 11.4 de la VM, ASan + UBSan, TSan, profils 21 et 24 bits, tampons empoisonnés, mutants,
suite complète de la référence) s'exécute sur G4 par `tools/g4_matrix.py`, dans une session gardée
`gcp-migration/v11_session.py` (voir `gcp-migration/README_V11.md`).

## État

Reprise du développement après les audits : [état courant et prochaines tranches](docs/DEVELOPPEMENT.md).
Les résultats d'audit sont des preuves bornées ; chaque port conserve ses propres portes.

| Couche | Fichiers | État au 2 octobre 2026 |
| --- | --- | --- |
| socle | `src/core`, `tests/support`, CMake et outils | qualifié avec num/cloud sur `a97180667` : Release 205/205 ; ASan/UBSan et TSan 130/130 chacun ; premiers échecs conservés |
| oracle de référence | `reference/` (définition $\Gamma_k$, construction, juge, sérialisations) | suite complète et quatre faits cover/MR₂/mémo/LCA inclus dans les 205 portes Release G4 ; aucun transfert au moteur |
| outillage G4 | contrôleur, worker, matrice | trois sessions closes ; dernière matrice verte, arrêts ciblés certifiés ; reçus dans `receipts/developpement_20261002/` |
| `num`, `cloud` | calcul exact et propriétaire du nuage | port explicite qualifié sur G4 aux profils 18/21/24 ; 103 mutants du socle/num/cloud détectés au profil 18 |
| `sched`, `io`, CLI | — | restent à intégrer et qualifier |
| catalogue | `src/catalogue` | [port séquentiel](docs/CATALOGUE.md) qualifié à `f391bf13e` ; LiDAR/K5 non terminé au plafond 30 s, contrat 100 ms non atteint ; ablation des feuilles en préparation |
| tour, points, tête | — | [mathématiques](docs/MATHEMATIQUES.md) et [conception](docs/CONCEPTION_MOTEUR.md) disponibles ; implémentation à poursuivre |

## Audits ouverts

Deux auditeurs suivent la v11 en continu. Leurs notes courantes sont dans [`audits/`](audits/) ; **tout agent qui
écrit ou relit un module lit d'abord celles qui le concernent et traite leurs constats** (correction et porte, ou
contestation argumentée). Les réponses du développeur sont les fichiers `REPONSE_CLAUDE_*` du même dossier.

## Lire d'abord

1. [Architecture](docs/ARCHITECTURE.md) : modules, règles, profil numérique.
2. [Provenance](docs/PROVENANCE.md) : ce qui est porté de la v10, depuis quelle source, comment c'est requalifié.
3. [Canal des audits](audits/README.md).
4. [Mathématiques](docs/MATHEMATIQUES.md) et [conception du moteur](docs/CONCEPTION_MOTEUR.md).
