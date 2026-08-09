# Audit de la promotion M3 — commit `2e3fa7b` et README live

> [!CAUTION]
> **Verdict : promotion M3 invalide; remettre le générateur au statut `exploration/diagnostic_only`.** Le commit affirme que le chemin rapide est exact alors qu'il embarque lui-même deux audits NO-GO, une primitive de grille qui sous-balaie sous UBSan, un validateur qui ignore les niveaux et les P0 de navigation dégénérée. Le README live va plus loin en déclarant que seule la performance reste ouverte. Cette phrase est directement falsifiée par le code committé, les fixtures permanentes manquantes, les sorties G4 `HORS DOMAINE` et la correction de coquille commencée immédiatement après la promotion.

## 1. Objets promus et empreintes

| objet | identité ou SHA-256 |
|---|---|
| commit promu | `2e3fa7b1ca5d6c2fc286babd923ddeebbb3cf7b6` |
| parent | `5a6cdb1af030a264ce07adddd312be2c458459b4` |
| commit correctif postérieur | `468635cf55d804dc6740b83fe527a09253e431d7` |
| commit de mesure et d'intégration postérieur | `4e8bbfda611225da941985c720d37e17eee43426` |
| header `order_k` dans le commit | `a6d0a3efe82fe9c17f5ce234d0e9bad40ffe0692ac2c1aab65bfa99ae088f6cc` |
| README dans le commit | `6611f773e98f9985f84a8ca2c85c27ac2999656d76d18ab3a49d019acd3634f4` |
| premier README live promu, stable à 04:20:26 UTC | `9547e3fc34f5e858ef28d1d907d95e9feadb1daa3c6d6c0ccdefe0b4bf4024d4` |
| README live après résultats G4, stable à 04:34:09 UTC | `2de6817addde98d65cd0066824b9515cff75c9150b8e4791eba3f7a3fda549ec` |
| header live déjà corrigé après le commit | `47ee37638ec5f27e840c85d1fa3aca22646f8074a7f99d05cb89c30a03bcb7ca` |
| `CMakeLists.txt` v3 au commit promu | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |
| `CMakeLists.txt` live avec CTest slow | `5b3cbc0fe61d202cea22dfcc8e2f657c86adf51f470bc9cf6f9b1d35b11a767e` |
| oracle v3 | `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |
| `PROPOSITION.md` live répétant la décision M3 | `34ecfd6bb0d25340d5d6780f1d3dac44d1753bab812d06078dc2fa6c494f5b91` |

Le commit touche onze fichiers sous `morsehgp3D_v3` : le header, l'index et neuf rapports d'audit. Il ne modifie ni le README, ni le CMake, ni l'oracle, ni `docs/implementation_status.toml`. La promotion du README est donc un delta live postérieur au commit; elle n'est pas une décision atomique liée au code annoncé.

Le header live avait déjà divergé du commit au moment de l'audit : après les sorties G4 à 8 000 et 20 000 points, Claude a identifié qu'un membre de l'ancienne coquille constant le long du pinceau était exclu de la nouvelle coquille. Le patch `47ee376...`, ensuite committé sous `468635c`, corrige ce défaut et un nouveau run 8 k ne sort plus du domaine. C'est un résultat positif ciblé. Découvrir et corriger cet invariant immédiatement après avoir écrit « l'exactitude n'est plus ouverte » invalide néanmoins la promotion antérieure; le run de montée en charge ne juge ni les niveaux, ni la complétude.

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

Le CTest slow ajouté plus tard ne compare pas davantage `bfs::Vertex.level` comme payload : `order_k_catalogue` l'utilise pour naviguer et couper, puis `try_emit` recompte globalement membres et rang avant la comparaison du catalogue et des forêts. Un transport de niveau faux peut donc rester invisible tant qu'il ne change ni la portée visitée ni la coupe.

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

Le patch `47ee376...`, ensuite committé sous `468635c`, ferme seulement la perte de membres constants. Le commit n'ajoute ni fixture permanente ni résultat d'oracle pour ce défaut, et ne répare ni le cube, ni le pont, ni la coupe par rang, ni les petites arités. Le rapport sur la [voie multiplicitaire](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md) propose une construction; cette proposition n'est pas l'implémentation committée.

### 4.3 L'identification mathématique du README n'est vraie qu'en position simple

Les lignes 28--29 identifient le catalogue de rang fermé au bas niveau par la formule `rang = 4 + niveau`. Elle ne vaut que pour un sommet simple de coquille quatre. Pour un sommet d'arrangement portant une coquille de taille $m>4$ et $\ell$ points strictement intérieurs, le rang fermé de sa boule est $m+\ell$, pas $4+\ell$. Les supports critiques d'arité deux ou trois ne sont, quant à eux, pas des sommets de l'arrangement de quatre hyperplans : le prototype essaie de les récolter depuis un sommet propriétaire. L'égalité affichée ne prouve donc ni leur existence dans la sortie, ni la complétude de cette récolte.

La ligne 33 affirme ensuite qu'« un seul point change d'état » le long d'une arête, alors que les lignes 44--48 reconnaissent les lots de points cosphériques. Hors simplicité, plusieurs points peuvent changer ensemble; transporter systématiquement le niveau par $\pm 1$ contredit précisément le modèle multiplicitaire que le paragraphe suivant revendique. Ce n'est pas une imprécision pédagogique : le témoin coplanaire, le cube et le pont montrent que cette confusion modifie niveaux et connectivité.

## 5. Le fast n'est intégré ni au catalogue ni à la porte M1

L'oracle courant appelle explicitement :

```cpp
catalogue = mhgp3v::order_k_catalogue(points, s_max, &order_k, &ood);
```

`order_k_catalogue` appelle le parcours slow. Au snapshot promu, aucun appel à `order_k_vertices_fast` n'existe dans l'oracle et le CMake v3 n'enregistre aucun CTest `order_k`. Le driver rapide ne construit ni les arités basses, ni le catalogue final, ni les forêts.

Par conséquent :

- un vert fast/slow ne qualifie pas le catalogue complet;
- un vert manuel de l'oracle slow ne qualifie pas le fast;
- le claim « arités 1 à 4, forêts, niveaux exacts passent M1 » n'a pas de reçu gardé dans le commit;
- les sorties chronométrées excluent le census, la déduplication, les forêts et la sérialisation.

Avant une promotion, il faut un sujet oracle fast distinct, une vérité indépendante, des CTests permanents et un reçu liant commit, binaire, paramètres, sorties et diagnostics injectés.

Un delta live postérieur ajoute bien `mhgp3v_oracle_order_k` au CMake et son exécution isolée passe en 130,96 s. Ce progrès intègre le **catalogue slow** à un CTest; il ne change pas le constat sur le fast. Le commentaire CMake promet en outre des « cosphéries traitées », alors que la référence incrémente `degenerate_shells` puis abandonne tout support ayant un membre de coquille supplémentaire; la garde symétrique peut alors classer le nuage entier hors domaine. Le protocole autorise donc une censure symétrique, mais le replay de cette campagne annonce en réalité `attempted=30`, `decided=30` et `rejected_domain=0` : son vert n'exerce aucune cosphéricité. Il ne contient aucune des fixtures cube, pont, coplanaire, coquille constante, `n=2/3` ou grille u16. Les 25 tests annoncés verts précédaient l'ajout; seul ce 26e test a ensuite été exécuté séparément, sans reçu gardé dans le commit.

La reprise locale après crash confirme le fait logiciel sans élargir sa portée : en Release avec GCC 13.3.0, `mhgp3v_oracle_order_k` passe seul en 194,19 s, puis les 25 autres CTests passent en 212,93 s avec deux workers. Ces deux exécutions séparées ne forment toujours pas un reçu scellé et n'exercent ni le fast ni les fixtures P0 citées ci-dessus. Le sérialiseur de reçu possède en outre une dette de provenance : faute de branche dédiée à `subject == "order_k"`, il étiquette ce sujet `mhgp3v anchored_catalogue`.

Le coût mémoire annoncé n'est pas davantage qualifié. Extrapoler la borne basse de 700 sommets visités par point donne environ 35 millions de `Vertex` à 50 k. Sous l'hypothèse basse de 32 octets pour le seul objet de contrôle `Vertex`, le stockage contigu minimal vaut déjà environ 1,12 Go, avant la capacité excédentaire, les allocations de coquilles, les nœuds et buckets du `seen` global, le catalogue et les forêts. Le ratio observé plus récent de 777 sommets par point porterait ce seul plancher à environ 1,24 Go. Le couple `visited` plus `seen` matérialise ainsi une fraction massive du sous-complexe de Delaunay d'ordre supérieur que l'invariant d'architecture demande précisément d'éviter. Sans pic mémoire et sans mécanisme de streaming borné, ce point est une porte d'architecture, pas seulement un facteur de temps.

## 6. La porte de phase et la traçabilité ne sont pas respectées

Le README live remplace l'état exploratoire par « M1, M2 et M3 » et affirme que seule la performance reste ouverte. Or :

- le commit ne met pas à jour `docs/implementation_status.toml`;
- le registre conserve `current_phase = "15"` pour le produit et ne contient aucune ouverture atomique de ce M3 expérimental;
- ni `docs/SPECIFICATION_MORSEHGP3D.md`, ni `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne définissent ou ne promeuvent une phase v3/M3 correspondant à ce README;
- aucun `backend`, `profile` et `mode` qualifiant n'accompagne la promotion;
- aucune exécution gardée de `tools/check_implementation_status.py` n'est liée au commit;
- le README lui-même rappelle qu'aucun statut public n'est ouvert, puis emploie quelques lignes plus loin le langage d'une exactitude fermée.

La règle de dépôt exige que toute ouverture ou fermeture de phase mette à jour le registre dans le même commit et satisfasse sa porte d'entrée. Ici, le code, les audits, la documentation d'état et les preuves appartiennent à quatre snapshots différents.

Le delta live de `PROPOSITION.md` ne ferme pas cette dette : son nouveau §0 ter reprend comme prémisses `rang = 4 + niveau`, transport par $\pm 1$, « cosphéricité traitée » et « seule question ouverte ». Ce sont précisément les claims réfutés aux §§3--4; les répéter dans une seconde documentation ne les transforme pas en porte de preuve.

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

### 7.2 Delta G4 après le correctif `468635c`

Le README live ajoute deux sorties utiles du header corrigé :

| profil | `n` | $s_{\max}$ | sommets | parcours seul | candidats/requête | replis | statut interne |
|---|---:|---:|---:|---:|---:|---:|---|
| LiDAR synthétique | 8 000 | 11 | 6 217 704 | 389,70 s | 95 | 28 044 | `ok` |
| LiDAR synthétique | 50 000 | 5 | 1 320 545 | 179,52 s | 275 | 2 183 | `ok` |

Le premier run crédite le correctif : l'abandon à 8 k du snapshot précédent disparaît. `ok` signifie toutefois seulement que ce parcours n'a pas déclenché son drapeau de domaine; ce run ne le compare à aucune vérité indépendante. Les P0 de germe, grille et multiplicité restent donc ouverts.

Le second run est à $s_{\max}=5$, soit les petits ordres, pas au contrat $K=10$ qui demande ici $s_{\max}=11$. Diviser idéalement 179,52 s par 48 cœurs pour annoncer environ 5 s n'est pas une mesure : le binaire observé n'utilise qu'un cœur, le parcours conserve un `seen` global et aucune parallélisation, mémoire maximale ou phase aval n'est qualifiée. Le run 8 k à $s_{\max}=11$ indique au contraire environ 16 000 sommets/s; l'extrapolation 50 k reste très loin de la seconde.

Même l'extrapolation la plus favorable, qui suppose constants les 777 sommets/point et le coût par requête, donne $389{,}70\times 50000/8000\approx2436$ s, soit 40,6 minutes sur un cœur. Une division idéale par 48 donne encore 50,7 s, et un facteur 100 donne 24,4 s; obtenir 5 s demande environ $487\times$, et 1 s environ $2436\times$. C'est encore une sous-estimation du prototype complet : le driver chronométré parcourt directement jusqu'au plafond 11, tandis que `order_k_catalogue(points, 11, ...)` appelle le parcours jusqu'au plafond 13 pour récolter les arités basses. Le journal final propose pourtant qu'un portage parallèle/GPU « gagne le facteur 48--100 » et place $K=10$ vers 1--5 s : les chiffres publiés ne soutiennent pas cette conclusion, avant même la croissance mesurée des candidats par requête.

La phrase live « aucun [fait] n'est un défaut de codage » est également trop forte : le passage antérieur hors domaine venait justement d'une coquille incomplète corrigée dans `468635c`, et les défauts numériques et de niveau ci-dessus sont toujours dans le code. Ces nouveaux chiffres rendent le NO-GO temporel plus quantitatif; ils ne réduisent pas les questions scientifiques à la seule performance.

## 8. Le README viole aussi la règle KaTeX du dépôt

L'équation des lignes physiques 28--29 est scindée :

```text
début d'une équation de bloc sur une ligne,
suite commençant par \qquad sur la ligne physique suivante
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

Décision : conserver le lemme local des deux boules et l'amélioration de constante comme résultats utiles. Refuser les formulations « M3 exact », « seule la performance reste ouverte », « toutes les dégénérescences traitées » et « catalogue complet du fast jugé sur le domaine revendiqué » tant que les portes ci-dessus ne sont pas fermées.

GCP non utilisé par cet audit; les sorties citées proviennent uniquement du journal local de Claude. Le même journal certifie ensuite l'arrêt ciblé de `ehgp-blackwell-spot-ai1a` par le script gardé, état GCE `TERMINATED`, puis la révocation de la clé. La dette de ressource est donc fermée; elle ne change pas le verdict scientifique.
