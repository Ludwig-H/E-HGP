# A6 — pont, campagnes récupérées et retrait

8 octobre 2026, Codex. Lecture de sources, métadonnées et JSON ; **aucun moteur,
build, appel GCP, coordonnées ou IDs**. Les temps ci-dessous proviennent des
campagnes du développeur, pas d'exécutions de l'auditeur.

**Conclusion proposée pour CST-0242 : clos dans la portée du produit par retrait
du mécanisme A6 en `ab5614c2a`, après intégration du pont en `6497ed3b5`.** Ce reçu
ne change pas le registre. Le lemme release/acquire reste conditionné aux
préconditions publiées ; aucune future variante A6b n'est qualifiée d'avance.
Les campagnes natives ne prouvent pas, seules, l'absence de toute exécution
faiblement ordonnée. Le fragment abstrait précédent reste historique, sans panne
native démontrée.

## Raccord exact des sources et du juge

- `6497ed3b581ed9f22e2ca7932d486b379629b6d7` : `load_leaf` devient acquire et
  `hint_leaves` publie en release. Les deux changements exécutables sont exactement
  ceux du [pont proposé](../a6_indices_concurrence/README.md), avec cinq lignes de
  commentaires supplémentaires ; le fichier entier n'est donc pas une postimage
  octet-identique du patch minimal. Les parents restent atomiques relaxed.
- Le pilote est **exactement** la postimage de
  [notre correctif de cohorte](../a6_pilote_admission/README.md), SHA-256
  `affefcb3659e008b7b05f6d6b395c90905136ef96a0b366d1e76a3638dc421b1`.
  Les cinq contre-journaux, le nominal, les six auto-tests statistiques et le
  mode essai (26 processus, sept clés d'identité, verdict Markdown « essai »)
  sont rejoués. Les anomalies sont refusées et le nominal reste admis ; aucune
  modification de seuil. Les vrais journaux passent ce même lecteur renforcé.
- Au retrait `ab5614c2a21fb9111067ceca4f005b748a69c0d4`, **tout** `src/tower/`,
  `tests/tower/` et `tests/mutants/tower.json` est identique à
  `bdfca8fb198e6711626c6506b816a2a656315c83`. Le pilote fermé reste conservé.
  Cela ne signifie pas que les modifications ultérieures de G ou de R sont
  revenues à bdf, ni qu'elles héritent des chronos A6.

## Primaires fermés

`v12.20261008.t2da6b` porte le paquet `6497ed3b5` : **362 fichiers** des scopes
`src/`, `bench/`, `tests/`, `cmake/` et `CMakeLists.txt` sont comparés octet par
octet à Git. Les 14 déclarations d'entrée sont identiques à la préparation
antérieure, dont l'archive de source avant bdf et le manifeste des 37 trames
déjà épinglé ; aucun payload n'est rouvert. L'archive avant est celle déjà
vérifiée par cette préparation, pas un nouveau build local.

L'archive de résultats est `cc521f1a…ffe25`, 1 735 442 octets, **253 membres de
manifeste**, tous vérifiés. Les six commandes finissent code 0 ; worker/DONE 0,
état `completed`, arrêt certifié/stop 0 et état final `TERMINATED`. Les primaires
archivés attestent 734 portes rapides **passées, zéro sautée**, puis LiDAR 7/7.
La porte des mutants passe ; le manifeste de la version contient 48 mutants et
un plancher 48. Ce reçu ne reconstitue pas un diagnostic causal de chaque mutant
à partir de la seule sortie agrégée CTest.

Deux archives auparavant non rapatriées sont présentes dans cette enveloppe :

| Source | Archive / octets | Manifeste | Résultat utile |
|---|---|---:|---|
| `30a69104a`, avant pont | `1ee399cd…5eecb` / 575 450 | 235 | pilote code 0 ; mutants code 8 |
| `fd84039c0`, mutants corrigés | `786a3a2c…dd232` / 591 261 | 55 | porte mutants code 0, plancher 48 |

La seconde contient aussi exactement la première. Les refus de rapatriement
historiques et leurs DONE 3 ne sont pas réécrits : **la récupération est établie
par la campagne suivante**. Le code 8 de la première porte mutants demeure ; ses
deux mutations non compilables ne deviennent pas des mutations tuées. Les
734/734 et LiDAR 7/7 de cette première archive sont également attestés.

## Admission et décision

Chaque mesure `30a` et `6497` contient **85 processus, 1 306 passes FULL**, dont
**783 passes chaudes décisives** : 378 pour les grandes et 405 pour ng00–02,
tous bras compris. Le plan externe exige 37 trames d'identité, 21 grandes,
six tournées grandes et cinq tournées ng, K5/GPU/W48/cache8G pour les chronos ;
K10 et W1 restent des contrôles d'identité. Les SHA des journaux, les résumés et
les champs commandés sont relus. Les ELF avant/avant_bis sont identiques et leurs
hashes initiaux/finals concordent, comme pour après. L'identité FUL1 est vérifiée
dans ses prises dédiées, pas inventée pour les prises chronométrées sans digest.

Le bootstrap est recalculé depuis les bruts avec les mêmes 10 000 tirages et
seuils. **Zéro différence** avec chaque jugement archivé, aucun refus d'admission,
verdict **rejeté** dans les deux campagnes. Pour `6497` :

| Cohorte | Rapport géométrique après/avant | IC95 | Décision locale |
|---|---:|---|---|
| 21 grandes | 0,913524 | [0,910113 ; 0,917280] | borne haute < 0,97 |
| ng00 | 1,032811 | [1,031246 ; 1,035165] | veto : borne haute ≥ 1,02 |
| ng01 | 1,042974 | [1,040273 ; 1,046918] | veto : borne haute ≥ 1,02 |
| ng02 | 1,012097 | [1,009218 ; 1,014623] | borne haute < 1,02 |

Les quatre rapports A/A restent dans la fenêtre déclarée. Le gain des grandes
ne permet donc pas d'adopter A6. Médianes des médianes par processus, en ms :
ng00 **87,523244 → 90,361630** ; ng01 **71,825770 → 74,970070** ; ng02
**87,927990 → 89,068350**. Pour les 21 grandes, médiane des médianes par trame
**158,627885 → 147,275340** et maximum de ces médianes
**295,105900 → 267,528525** ; maximum brut chaud après **268,975382 ms**.
Les 21 médianes après dépassent 100 ms. Ces agrégations ne remplacent pas la
moyenne géométrique jugée ; aucune somme de médianes d'étages n'est utilisée.

Les deux campagnes séparées ne mesurent pas causalement le coût isolé du pont.
`documentation.patch` propose de corriger cette déduction et la qualification
anticipée d'une future A6b dans le README du développeur, sans toucher aux temps.

## Rejeu et limites

```
python check.py /workspaces/E-HGP /workspaces/.ehgp-sessions/v12.20261008.t2da6b --check
python -O check.py /workspaces/E-HGP /workspaces/.ehgp-sessions/v12.20261008.t2da6b --check
```

Sorties normal/−O identiques à [results.json](results.json). [admit.py](admit.py)
porte explicitement le préparateur de lecture A6 existant : ses sources de
parseur viennent de Git30a, puis du patch fermé dont la postimage est vérifiée
égale au pilote6497. Les versions du moteur restent distinguées dans le résultat.
Le rapport publié a trois chemins d'exécutables anonymisés ; les journaux sont
identiques aux primaires. Le rejeu emploie les rapports **originaux récupérés**.
Les comptes, traces, chemins privés et sources complètes ne sont pas recopiés.
Les hashes d'ELF déclarés ne sont pas une reconstruction indépendante des binaires,
et les photos d'environnement ne prouvent pas une isolation continue.
