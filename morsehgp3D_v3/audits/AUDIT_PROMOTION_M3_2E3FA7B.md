# Audit de la promotion M3 — commit `2e3fa7b` et README live

> [!CAUTION]
> **Verdict : promotion M3 invalide; remettre le générateur au statut `exploration/diagnostic_only`.** Le commit affirme que le chemin rapide est exact alors qu'il embarque lui-même deux audits NO-GO, une primitive de grille qui sous-balaie sous UBSan, un validateur qui ignore les niveaux et les P0 de navigation dégénérée. Le README live va plus loin en déclarant que seule la performance reste ouverte. Cette phrase est directement falsifiée par le code committé, les fixtures permanentes manquantes, les sorties G4 `HORS DOMAINE` et la correction de coquille commencée immédiatement après la promotion.

## 1. Objets promus et empreintes

| objet | identité ou SHA-256 |
|---|---|
| commit promu | `2e3fa7b1ca5d6c2fc286babd923ddeebbb3cf7b6` |
| parent | `5a6cdb1af030a264ce07adddd312be2c458459b4` |
| header `order_k` dans le commit | `a6d0a3efe82fe9c17f5ce234d0e9bad40ffe0692ac2c1aab65bfa99ae088f6cc` |
| README dans le commit | `6611f773e98f9985f84a8ca2c85c27ac2999656d76d18ab3a49d019acd3634f4` |
| README live promu, stable à 04:20:26 UTC | `9547e3fc34f5e858ef28d1d907d95e9feadb1daa3c6d6c0ccdefe0b4bf4024d4` |
| header live déjà corrigé après le commit | `47ee37638ec5f27e840c85d1fa3aca22646f8074a7f99d05cb89c30a03bcb7ca` |
| `CMakeLists.txt` v3 | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |
| oracle v3 | `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |

Le commit touche onze fichiers sous `morsehgp3D_v3` : le header, l'index et neuf rapports d'audit. Il ne modifie ni le README, ni le CMake, ni l'oracle, ni `docs/implementation_status.toml`. La promotion du README est donc un delta live postérieur au commit; elle n'est pas une décision atomique liée au code annoncé.

Le header live avait déjà divergé du commit au moment de l'audit : après les sorties G4 à 8 000 et 20 000 points, Claude a identifié qu'un membre de l'ancienne coquille constant le long du pinceau était exclu de la nouvelle coquille. Le patch `47ee376...` vise ce défaut. Découvrir et corriger un invariant de coquille immédiatement après avoir écrit « l'exactitude n'est plus ouverte » suffit à invalider le statut, même avant le résultat de la nouvelle campagne.

## 2. Contradiction interne du commit

Le message de commit affirme notamment :

```text
The fast path is judged differentially against [the reference]
and agrees exactly on every cloud tested.
```

Le même commit ajoute pourtant :

- [`AUDIT_GRILLE_ORDER_K_4EF89A1.md`](AUDIT_GRILLE_ORDER_K_4EF89A1.md), verdict « NO-GO pour le parcours exact et pour 50 k »;
- [`AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md`](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md), verdict « NO-GO pour employer la grille comme certificat de complétude »;
- [`AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md`](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md), qui conserve les contre-exemples de coupe, niveau et coquille;
- [`AUDIT_DIAGNOSTICS_CRITIQUES_LOCALITE_RAYON_5A6CDB1.md`](AUDIT_DIAGNOSTICS_CRITIQUES_LOCALITE_RAYON_5A6CDB1.md), qui réfute les extrapolations de catalogue, rayon et localité reprises dans le message et le README.

Ce n'est pas une divergence d'interprétation : les rapports contiennent des exécutions hostiles et des fixtures exactes sur le même header ou sur des lignes inchangées. Un audit embarqué peut motiver une correction; il ne peut pas être simultanément archivé comme NO-GO et ignoré par le statut du même commit.

## 3. La comparaison fast/slow ne juge pas les niveaux

Le driver scratch `fast.cpp`, SHA-256 `74614c90da544ab01e858e351033104f18450cfa5573bd483ee000a025a8152e`, transforme chaque sortie en `set<vector<PointId>>`. Il compare les coquilles et le booléen de domaine, mais supprime les multiplicités et ignore `Vertex.level`.

La fixture coplanaire déjà publiée donne sur le chemin promu :

```text
harness_accord=1 fast_out=1 slow_out=1
fast_level=0 slow_level=0 exact_level=1
```

Le harnais écrit donc `ACCORD=oui` précisément lorsque les deux chemins partagent le même niveau faux. Le slow partage en outre `seed_shell`, les prédicats, le transport, la coupe et la reconstruction de coquille du fast; il n'est pas un oracle indépendant.

La qualification indépendante positive disponible porte sur de petits nuages simples. Elle est utile, mais ne couvre ni les limites u16, ni les coquilles multiples, ni les points constants, ni le pont cosphérique. Le claim « partout où la référence est calculable » doit être remplacé par le domaine exact, le nombre de cas et le hash du reçu.

## 4. P0 numériques et dégénérés toujours ouverts

### 4.1 La grille peut omettre des points

Dans le header committé et dans le live postérieur, `Grid::ball` convertit en `int` une borne de cellule flottante avant de la saturer. Une fixture u16 de déterminant affine 1 produit des indices de l'ordre de 100 milliards. UBSan signale la conversion hors plage et la grille rend un sous-ensemble de la coquille exacte.

Le filtre de distance effectue aussi une décision d'exclusion en `double`, contrairement au message « floating point sweeps too much, never decides ». Le lemme de l'union de deux boules est mathématiquement sain; son prérequis de sur-balayage ne l'est pas dans cette implémentation. Le détail, les hashes et la conception fail-open sont dans l'[audit numérique](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md).

Le repli conditionné par `best < 0` ne ferme pas ce défaut : une requête numériquement incomplète peut trouver un candidat plausible et ne jamais déclarer que sa couverture était inconclusive.

### 4.2 « Les dégénérescences sont traitées » est faux au snapshot promu

Les fixtures indépendantes montrent encore au commit :

- cube cosphérique : le germe de coquille 8 est coupé avant navigation et les douze boules diamétrales d'arêtes manquent;
- pont cosphérique : la coupe par rang fermé déconnecte le parcours et trois paires critiques manquent;
- témoin coplanaire : niveau stocké 0 au lieu de 1, puis transport négatif;
- coquille constante : huit sommets sur quatorze perdent des membres permanents;
- entrées de deux ou trois points : paires et triangles absents, seuls les singletons sont publiés.

Le patch live `47ee376...` tente de fermer seulement la perte de membres constants. Il ne constitue ni une fixture permanente, ni un résultat d'oracle, et ne répare ni le cube, ni le pont, ni la coupe par rang, ni les petites arités. Le rapport sur la [voie multiplicitaire](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md) propose une construction; cette proposition n'est pas l'implémentation committée.

## 5. Le fast n'est intégré ni au catalogue ni à la porte M1

L'oracle courant appelle explicitement :

```cpp
catalogue = mhgp3v::order_k_catalogue(points, s_max, &order_k, &ood);
```

`order_k_catalogue` appelle le parcours slow. Aucun appel à `order_k_vertices_fast` n'existe dans l'oracle, et le CMake v3 n'enregistre aucun CTest `order_k`. Le driver rapide ne construit ni les arités basses, ni le catalogue final, ni les forêts.

Par conséquent :

- un vert fast/slow ne qualifie pas le catalogue complet;
- un vert manuel de l'oracle slow ne qualifie pas le fast;
- le claim « arités 1 à 4, forêts, niveaux exacts passent M1 » n'a pas de reçu gardé dans le commit;
- les sorties chronométrées excluent le census, la déduplication, les forêts et la sérialisation.

Avant une promotion, il faut un sujet oracle fast distinct, une vérité indépendante, des CTests permanents et un reçu liant commit, binaire, paramètres, sorties et diagnostics injectés.

## 6. La porte de phase et la traçabilité ne sont pas respectées

Le README live remplace l'état exploratoire par « M1, M2 et M3 » et affirme que seule la performance reste ouverte. Or :

- le commit ne met pas à jour `docs/implementation_status.toml`;
- le registre conserve `current_phase = "15"` pour le produit et ne contient aucune ouverture atomique de ce M3 expérimental;
- aucun `backend`, `profile` et `mode` qualifiant n'accompagne la promotion;
- aucune exécution gardée de `tools/check_implementation_status.py` n'est liée au commit;
- le README lui-même rappelle qu'aucun statut public n'est ouvert, puis emploie quelques lignes plus loin le langage d'une exactitude fermée.

La règle de dépôt exige que toute ouverture ou fermeture de phase mette à jour le registre dans le même commit et satisfasse sa porte d'entrée. Ici, le code, les audits, la documentation d'état et les preuves appartiennent à quatre snapshots différents.

## 7. Les claims performance restent exploratoires

Les chiffres `7,3 à 10,6 critiques/point`, rayon critique 90 et coupe de 35 % proviennent de drivers post-sélectionnés par le parcours incomplet :

- le compteur critique ne compte que les coquilles trouvées et omet les petites arités;
- son ratio augmente déjà d'environ 45 % de 500 à 1 000 points;
- le driver de rayon emploie la miniboule de la coquille pour les sommets non critiques, pas leur sphère d'arrangement;
- le profil LiDAR est synthétique, à une graine par taille;
- `98,9 % rejetés` ne signifie pas que 98,9 % des sommets sont des slivers.

L'extrapolation à environ 400 000 sphères critiques à 50 k n'est donc ni une taille certifiée ni un reçu temporel. La phrase « rien n'interdit 100 ms » n'est soutenue par aucun pipeline complet.

Les runs G4 lus dans le journal sont eux-mêmes concurrents et détachés. Sur le snapshot `4ef89a1`, ils donnent notamment :

| profil | `n` | parcours seul | statut |
|---|---:|---:|---|
| LiDAR synthétique | 2 000 | 54,08 s | `ok` |
| LiDAR synthétique | 8 000 | 43,25 s | `HORS DOMAINE` après préfixe |
| LiDAR synthétique | 20 000 | 0,20 s | `HORS DOMAINE` après préfixe |
| uniforme | 2 000 | 45,02 s | `ok` |

Les temps courts à 8 k et 20 k sont des abandons sur niveau négatif, pas des accélérations. Aucun résultat 50 k n'était disponible à la promotion. Plusieurs jobs `nohup`/`setsid` de snapshots différents partageaient la même machine; aucun reçu de charge, hash de binaire distant ou arrêt ciblé n'était encore publié.

## 8. Le README viole aussi la règle KaTeX du dépôt

L'équation des lignes physiques 28--29 est scindée :

```text
$$...,
\qquad ...$$
```

Les instructions du dépôt exigent que chaque équation, délimiteurs compris, tienne sur une seule ligne physique. Cette erreur de rendu est secondaire devant les P0 scientifiques, mais elle confirme que le README n'a pas franchi sa propre porte documentaire.

## 9. Porte minimale avant de réouvrir M3

1. Remettre le README à `exploration_v3`, avec exactitude et performance toutes deux ouvertes.
2. Garder en fixtures permanentes le cast u16 extrême, le niveau coplanaire, le cube, le pont, la coquille constante et les cas `n=2/3`.
3. Fermer la grille en fail-open : saturation avant conversion, enveloppe prouvée, filtre de distance non excluant ou décision exacte, statut de couverture propagé même si un candidat existe.
4. Implémenter la navigation multiplicitaire sans coupe par rang fermé, puis comparer `(shell, level)` à un oracle rationnel indépendant.
5. Intégrer séparément fast, catalogue complet et forêts à CMake/CTest; injecter des fautes et exiger qu'elles soient détectées par le garde visé.
6. Mettre à jour le registre de phase dans le même commit seulement après ces portes, puis exécuter son checker.
7. Mesurer séquentiellement le pipeline complet sur des entrées scellées; publier le pic mémoire et tous les replis, abandons et ambiguïtés numériques.
8. Certifier l'arrêt de la cible G4 démarrée par la session qui a lancé les jobs.

Décision : conserver le lemme local des deux boules et l'amélioration de constante comme résultats utiles. Refuser les formulations « M3 exact », « seule la performance reste ouverte », « toutes les dégénérescences traitées » et « catalogue complet jugé » tant que les portes ci-dessus ne sont pas fermées.

GCP non utilisé par cet audit; les sorties citées proviennent uniquement du journal local de Claude.
