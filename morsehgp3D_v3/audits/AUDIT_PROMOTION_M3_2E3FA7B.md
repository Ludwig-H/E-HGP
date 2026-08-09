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

La cause est encore visible dans le live : `seed_shell` ajoute un point coplanaire constant seulement si `face.side(...) == 0`, mais ne compte pas le cas strictement intérieur. Les deux parcours initialisent ensuite sans condition `Vertex{root_shell, 0}`. Le niveau du germe n'est donc pas démontré par la construction; il est forcé à zéro après avoir ignoré précisément les témoins constants qui peuvent le rendre positif.

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

### 4.3 L'identification mathématique du README n'est vraie qu'en position simple

Les lignes 28--29 identifient le catalogue de rang fermé au bas niveau par la formule `rang = 4 + niveau`. Elle ne vaut que pour un sommet simple de coquille quatre. Pour un sommet d'arrangement portant une coquille de taille $m>4$ et $\ell$ points strictement intérieurs, le rang fermé de sa boule est $m+\ell$, pas $4+\ell$. Les supports critiques d'arité deux ou trois ne sont, quant à eux, pas des sommets de l'arrangement de quatre hyperplans : le prototype essaie de les récolter depuis un sommet propriétaire. L'égalité affichée ne prouve donc ni leur existence dans la sortie, ni la complétude de cette récolte.

La ligne 33 affirme ensuite qu'« un seul point change d'état » le long d'une arête, alors que les lignes 44--48 reconnaissent les lots de points cosphériques. Hors simplicité, plusieurs points peuvent changer ensemble; transporter systématiquement le niveau par $\pm 1$ contredit précisément le modèle multiplicitaire que le paragraphe suivant revendique. Ce n'est pas une imprécision pédagogique : le témoin coplanaire, le cube et le pont montrent que cette confusion modifie niveaux et connectivité.

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

Le coût mémoire annoncé n'est pas davantage qualifié. Extrapoler 700 à 740 sommets visités par point donne environ 35 millions de `Vertex` à 50 k. À seulement 32 octets par objet, le stockage contigu minimal vaut déjà environ 1,12 Go, avant les allocations de coquilles, les nœuds et buckets du `seen` global, le catalogue et les forêts. Le couple `visited` plus `seen` matérialise ainsi une fraction massive du sous-complexe de Delaunay d'ordre supérieur que l'invariant d'architecture demande précisément d'éviter. Sans pic mémoire et sans mécanisme de streaming borné, ce point est une porte d'architecture, pas seulement un facteur de temps.

## 6. La porte de phase et la traçabilité ne sont pas respectées

Le README live remplace l'état exploratoire par « M1, M2 et M3 » et affirme que seule la performance reste ouverte. Or :

- le commit ne met pas à jour `docs/implementation_status.toml`;
- le registre conserve `current_phase = "15"` pour le produit et ne contient aucune ouverture atomique de ce M3 expérimental;
- ni `docs/SPECIFICATION_MORSEHGP3D.md`, ni `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne définissent ou ne promeuvent une phase v3/M3 correspondant à ce README;
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

Le rayon critique maximal 90 ne borne pas non plus la grille du parcours : celle-ci interroge toutes les sphères d'arrangement visitées, majoritairement non critiques. Le driver de rayon mesure de surcroît la miniboule de la coquille pour ces sommets, qui peut être strictement plus petite que leur sphère d'arrangement. En déduire une coupe ou un pas de grille complet est circulaire; l'[audit de localité et de rayon](AUDIT_DIAGNOSTICS_CRITIQUES_LOCALITE_RAYON_5A6CDB1.md) donne une fixture u16 exacte où les deux rayons carrés valent respectivement 4 et 6.

L'extrapolation à environ 400 000 sphères critiques à 50 k n'est donc ni une taille certifiée ni un reçu temporel. Elle prolonge un ratio observé sur une graine synthétique alors que ce ratio croît déjà entre 500 et 1 000 points. La phrase « rien n'interdit 100 ms » n'est soutenue par aucun pipeline complet.

Les runs G4 lus dans le journal sont eux-mêmes concurrents et détachés. Le sweep lancé sur le header committé `a6d0a3e...` donne notamment :

| snapshot | profil | `n` | parcours seul | statut |
|---|---|---:|---:|---|
| `a6d0a3e...` | LiDAR synthétique | 2 000 | 54,08 s | `ok` |
| `a6d0a3e...` | LiDAR synthétique | 8 000 | 43,25 s | `HORS DOMAINE` après préfixe |
| `a6d0a3e...` | LiDAR synthétique | 20 000 | 0,20 s | `HORS DOMAINE` après préfixe |
| `a6d0a3e...` | uniforme | 2 000 | 45,02 s | `ok` |

Les temps courts à 8 k et 20 k sont des abandons sur niveau négatif, pas des accélérations. Aucun résultat 50 k n'était disponible à la promotion. Plusieurs jobs `nohup`/`setsid` de snapshots différents partageaient la même machine; aucun reçu de charge, hash de binaire distant ou arrêt ciblé n'était encore publié.

### 7.1 Réponse à la question « ramener le facteur 100 »

Pour **ce parcours complet**, le facteur entre sommets visités et sphères critiques n'est pas une surcharge de conteneur : ce sont les sommets que l'algorithme choisit d'énumérer. Un reverse search ou un streaming peut supprimer `seen`, réduire la mémoire et améliorer les constantes; il ne supprime pas les quelque 35 millions de visites extrapolées.

Deux mesures G4 concurrentes du snapshot antérieur `4ef89a1...` donnent environ 410 184 sommets en 14 s, soit 29 000 sommets/s, et 1,478 million en 76,62 s, soit 19 000 sommets/s. Le second sweep `a6d0a3e...` annonce bien 54,08 s pour les mêmes 1 477 918 sommets, mais il change le header et s'exécute sous une autre concurrence; ce n'est pas un doublon du run de 76,62 s. Tous ces nombres sont des diagnostics, pas un reçu séquentiel qualifiant. Même en retenant la fenêtre observée de 19 000 à 29 000 sommets/s, 35 millions de visites demanderaient environ 1 200 à 1 800 s. Atteindre 1 s exige donc un facteur d'environ 1 200 à 1 800 sur le débit observé; atteindre 100 ms, environ 12 000 à 18 000. Le facteur critique/visité proche de 100 ne suffit pas à expliquer ni à fermer cet écart.

Réduire réellement le nombre de visites exige un autre générateur, ou un raccourci qui atteint directement les sommets critiques avec une preuve de complétude et de connectivité. Aucun tel certificat n'est présent au commit. La réponse honnête à la question du README est donc : streaming utile pour la mémoire, optimisation locale utile pour la constante, mais contrat temporel encore architecturalement ouvert.

## 8. Le README viole aussi la règle KaTeX du dépôt

L'équation des lignes physiques 28--29 est scindée :

```text
$$...,
\qquad ...$$
```

Les instructions du dépôt exigent que chaque équation, délimiteurs compris, tienne sur une seule ligne physique. Cette erreur de rendu est secondaire devant les P0 scientifiques, mais elle confirme que le README n'a pas franchi sa propre porte documentaire.

## 9. Porte minimale avant de réouvrir M3

1. Remettre le README à `exploration_v3`, avec exactitude, contrat HGP, architecture mémoire et performance tous ouverts.
2. Garder en fixtures permanentes le cast u16 extrême, le niveau coplanaire, le cube, le pont, la coquille constante et les cas `n=2/3`.
3. Fermer la grille en fail-open : saturation avant conversion, enveloppe prouvée, filtre de distance non excluant ou décision exacte, statut de couverture propagé même si un candidat existe.
4. Implémenter la navigation multiplicitaire sans coupe par rang fermé, puis comparer `(shell, level)` à un oracle rationnel indépendant.
5. Intégrer séparément fast, catalogue complet et forêts à CMake/CTest; comparer coquilles, multiplicités et niveaux, puis injecter des fautes et exiger qu'elles soient détectées par le garde visé.
6. Définir la porte M3 dans la spécification et le registre des preuves, puis mettre à jour le registre de phase dans le même commit seulement après ces portes et exécuter son checker.
7. Mesurer séquentiellement le pipeline complet sur des entrées scellées; publier le pic mémoire et tous les replis, abandons et ambiguïtés numériques.
8. Certifier l'arrêt de la cible G4 démarrée par la session qui a lancé les jobs.

Décision : conserver le lemme local des deux boules et l'amélioration de constante comme résultats utiles. Refuser les formulations « M3 exact », « seule la performance reste ouverte », « toutes les dégénérescences traitées » et « catalogue complet jugé » tant que les portes ci-dessus ne sont pas fermées.

GCP non utilisé par cet audit; les sorties citées proviennent uniquement du journal local de Claude.
