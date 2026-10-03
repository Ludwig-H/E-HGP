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

Pour $k = 1, \ldots, K_{\max}$ et $a \geq 0$, soit $D_k(y)$ le carré de la distance de $y$ à son $k$-ième plus proche point et $L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace$. La **tour FULL** est, pour chaque $k$, l'arbre de fusion des composantes connexes de $L_k(a)$ quand $a$ croît, avec les applications verticales $L_{k+1}(a) \subseteq L_k(a)$ ; les niveaux sont des rationnels exacts. La prochaine étape sera une hiérarchie laminaire sur les points, à comparer à celle de `sklearn.cluster.HDBSCAN` (jamais réimplémenté).

## Ordre des travaux

1. Audit de la v10 : [synthèse et lacunes restantes](docs/AUDIT_V10_SYNTHESE.md).
2. Fondations : statuts, tampons comptés, ordonnanceur, arithmétique exacte à budget de bits, nuage, entrées et
   sorties, oracle de référence exact.
3. Moteur : catalogue critique, tour FULL, comparés octet pour octet à la v10 figée et à l'oracle borné.
4. Hiérarchie de points.
5. Performance sur trames LiDAR (G4), menée avec le moteur avant la hiérarchie de points.
6. Comparaison à HDBSCAN : bancs synthétiques, puis LiDAR réel (démos `Zoltan/demos/` et nouveaux cas).

## Construction

Ces commandes sont exécutées dans le worker G4 gardé, conformément à la
consigne utilisateur ; aucun build ou test natif dans le Codespace.

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

État consolidé sur **479f53f0b** ; dernière capture G4 close examinée :
[reuse1 / ae817d09e](receipts/full_regular_vertical_20261003/reuse1/README.md).
3339/3339 portes, ASan18 299/299, 292 mutants et29/29 FULL K5 ;
Clang absent. Les ports ultérieurs ne sont pas couverts par cette source.

| Couche | État courant |
|---|---|
| fondations, num, Cloud, Pool | propriétaires privés, réservations budgétées, profils18/21/24, arithmétique exacte et refus ; ports et preuves historiques dans [PROVENANCE](docs/PROVENANCE.md) |
| catalogue et index | catalogue complet, census global ; cacheJ2, tri indirect, frontière adaptative, assemblage parallèle et une passe déjà qualifiés ; options inactives par défaut |
| MEB et descentes | diamètre exact, premier support strict contenant, mémo avant MEB avec dates distinctes, workspaces census privés réutilisés |
| forêts FULL | naissances, multifusions atomiques, parents et verticales fermées ; lots parallèles, balayage, lookup dense et graines verticales réutilisées qualifiés dans reuse1 |
| performances closes | FULL K1..5/W48/mode2047 : **1154–1531ms** sur les trois sous-nuages entiers sans sol1mm en u21/u24 ; une exécution par case, sans projection de points ; cible200ms ouverte |
| ports à qualifier sur G4 | ef75/479 : coupes de préfixes, tri F3/F4, tables de populations, ordres concurrents, unions et BirthRuns ; progrès locaux documentés ; deux réserves mémoire/contexte dans l’audit courant ; graph4 non rapatrié, poids q4/contacts/MEB différée non qualifiés G4 |
| points, tête et API | modules produit encore absents ; core/cover et fixtures dans la référence ; comparaison effective à HDBSCAN après le jalon moteur |
| voie rapide du 3 octobre | plan lourd d'abord, ordres concurrents, table de populations, coupes de feuille, tri F3/F4, census en signes ; sorties identiques octet pour octet ; −37 à −41 % de mur à W4 local sur les trois trames, ~0,32 s estimées sur G4 à W48, **non mesurées sur G4** ([note](audits/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md), [mécanismes](docs/PERFORMANCE_FULL.md)) |

La [reprise développeur et le diagnostic v10/v11](audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md)
remplace les anciens états de cet auditeur. La v10 mesure204–254ms sur ces
entrées K5 ; les profils, processus et répétitions diffèrent, et le
comparatif canonique LiDAR entier reste à fermer. Les trois trames sont
issues d’une même séquence. Aucun résultat GPU ni multi-millions acquis.

## Audits ouverts

Les notes courantes et la reprise côté développement sont dans [`audits/`](audits/) ; **tout agent qui
écrit ou relit un module lit d'abord celles qui le concernent et traite leurs constats** (correction et porte, ou
contestation argumentée). Les réponses du développeur sont les fichiers `REPONSE_CLAUDE_*` du même dossier.

## Lire d'abord

1. [Architecture](docs/ARCHITECTURE.md) : modules, règles, profil numérique.
2. [Provenance](docs/PROVENANCE.md) : ce qui est porté de la v10, depuis quelle source, comment c'est requalifié.
3. [Canal des audits](audits/README.md).
4. [Mathématiques](docs/MATHEMATIQUES.md) et [conception du moteur](docs/CONCEPTION_MOTEUR.md).
