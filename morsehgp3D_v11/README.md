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
u18 et u24 restent explicites. La voie native q1/q2/q4 couvre les trois profils ; q3 emploie un certificat natif ou un essai contrôlé, avec repli entier large. La qualification de cette voie est épinglée à `9df774947` ; les reçus distinguent les trois profils.
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

La tour FULL native est implémentée : catalogue, index/census, MEB et descentes datées,
plateaux atomiques, parents et verticales. Défaut u21 ; profils u18/u24 distincts.
FULL exige des sites de poids unitaire. La projection native sur les points reste à construire.

Moteur **c40f40798** jugé par sa propre campagne G4 : **4073/4073 portes,
326 mutants tués**, GCC18/21/24, ASan/UBSan et TSan ; Clang absent.
Propriétaire, admission mémoire et tri/FENV ont leurs portes ciblées.
ASan18 couvre num/index/tower, sans catalogue/FENV.

Le [banc clos](receipts/qualification_performance_20261003/README.md) réussit **81/81 prises**
aux sorties entières identiques : baseline v11 **895680ff8/2047** et courant 2047/16379,
trois trames entières sans sol de la séquence 08 (grille 1 mm), K5/u21, W1/W8/W48, trois prises.
À W48/16379, médianes FULL **489,099 / 345,066 / 432,397 ms**, soit **×2,68–3,28**
plus rapide que la baseline. Réservations Buffer+Cloud 346,0 / 298,7 / 368,3 MiB, hors RSS.
Processus/owners neufs, caches OS non vidés. FULL inclut index/domaine/forêts,
hors préparation, IO/Cloud/Pool, dumps et Python. Les sessions sont arrêtées et les
lectures normal/−O concordantes. [État courant et prochains leviers](docs/DEVELOPPEMENT.md).

Tranche 3 du 3 octobre (voie liée de la table de populations, naissances par blocs, pipeline
résolution/publication/verticales) : qualification de développement G4 **conforme** (suite `fast` 666/666, TSan
ciblé, 11 mutants tués, sorties identiques à chacune des 36 prises). À W48/16379, médianes **412 / 352 / 381 ms**
contre 447 / 440 / 440 ms pour la base appariée ; les forêts gagnent 38 à 76 ms, le domaine (200–255 ms) borne
désormais le total. [Audit et chemin vers 200 ms](audits/NOTE_CLAUDE_AUDIT_V11_20261003.md),
[reçu](receipts/developpement_20261003/pipeline_g4/README.md).

Hiérarchie de points (3 octobre au soir) : à $k$ fixé, règle retenue $H^{r}_{k+1}=P_1\circ\Pi_{k+1}$, l'ancrage
persistant de la v10 en rayon sur la couverture qualifiée. Elle est fidèle à FULL, laminaire, indépendante de mcs et
stable en $3\varepsilon$ ; ses dates $\sqrt{t}+\sqrt{m}-\sqrt{q}$ sont décidées exactement. Banc Python sur l'export
natif des incidences fortes, porte contre l'oracle de la définition (dates et propriétaires exacts par site,
12 fixtures, 4 mutants). Mesures G4 de développement au meilleur bloc : synthétique à $n=8\,000$, +0,008 à +0,078
sur HDBSCAN de $k=2$ à $10$ ; démos de `Zoltan/` : trois vélos et deux voitures que HDBSCAN manque sont rattrapés,
une perte. Verrou cibles contre stabilité ouvert. [Note](docs/HIERARCHIE_POINTS.md).

Cibles 200 ms et 100 ms ouvertes. K10, GPU, multi-millions, plusieurs séquences et hiérarchie
native de points non acquis. Le différentiel canonique v10/v11 sur LiDAR entier reste ouvert :
les 81 prises présentes comparent deux sources v11. [Provenances](docs/PROVENANCE.md).

## Audits ouverts

Les notes courantes et la reprise côté développement sont dans [`audits/`](audits/) ; **tout agent qui
écrit ou relit un module lit d'abord celles qui le concernent et traite leurs constats** (correction et porte, ou
contestation argumentée). Les réponses du développeur sont les fichiers `REPONSE_CLAUDE_*` du même dossier.

## Lire d'abord

1. [Architecture](docs/ARCHITECTURE.md) : modules, règles, profil numérique.
2. [Provenance](docs/PROVENANCE.md) : ce qui est porté de la v10, depuis quelle source, comment c'est requalifié.
3. [Canal des audits](audits/README.md).
4. [Mathématiques](docs/MATHEMATIQUES.md) et [conception du moteur](docs/CONCEPTION_MOTEUR.md).
