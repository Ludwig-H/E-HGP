# Contre-audit numérique — contrat v12 du 7 octobre 2026

**Avis : repères locaux approuvables ; contrat e264de6f2 à corriger avant port.** Deux défauts peuvent devenir une erreur de résultat lors d'un port direct : certificat q3 appliqué au mauvais domaine et identité des sites fondée sur une clé Morton devenue tronquée. La garde des boîtes et la préparation du réservoir demandent également un contrat explicite. Il s'agit de défauts du contrat proposé, **pas de défauts observés d'un moteur v12**.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`. Aucun GCP, GPU, build ou moteur natif v12 exécuté. Lecture de sources et témoins Python entiers/rationnels seulement. Aucun code développeur modifié, aucun commit/push.

## Épingles et portée

- Worktree : commit `13c52bc602a4e7ea90964ee23e155a8ad2cfd301`.
- Proposition initiale figée : `/tmp/ehgp-v12-contract-input-20261007/CONTRAT_NUMERIQUE.md`, SHA256 `39c19a00fea6d01ca8ee4240c91a7a065c327774481d03c24980a9498e3632f0`.
- **Autorité de cette note** : `git show e264de6f2:morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md`, SHA256 `40baf861e474c89e3155513cdbfa3f7a61896412a85948029c776bd1ad03af0e`.
- Source moteur relue : v11 `ac081a06f` ; référence des chemins v11 ci-dessous. Les lignes du contrat correspondent à e264de6f2, celles des sources à ac081a06f.
- L'ouverture AGENTS, README v12, DECISIONS, ARCHITECTURE et le contrat mathématique ont été lus. La profondeur des boîtes et l'extrémité fermée `2^32` sont traitées par l'auditeur principal : elles ne sont pas redémontrées ici.

Rejeu autonome : `python3 witness.py` puis `python3 -O witness.py`. `RESULT.json` est la sortie exacte ; aucune assertion du témoin n'est désactivée par `-O`. Ce témoin modélise les expressions citées ; il ne qualifie aucune nouvelle implantation.

## 1. Bloquant : le certificat q3 dépend bien du domaine des requêtes

Contrat **91–93** : « ils ne lisent ni B ni s » est faux. `src/num/power_certificate.hpp:8–15` emploie explicitement `kCoordBits` :

`0 < D < 2^(123−2B)` et `|N_j| < 2^(124−B)`.

`src/num/orientation_certificate.hpp:8–16` emploie de même `D < 2^(124−3B)` et `|N_j| < 2^(124−2B)`. Les coefficients ne suffisent pas à certifier des requêtes de taille arbitraire.

**Témoin qui passe la nouvelle garde.** Poser `M=2^20`, `h=M−1`, support aigu `S={(0,0,0),(h,h,0),(h,0,h)}`, donc `s=20`. Sa représentation v11 est `D=6h^4`, `N=(4,2,2)h^5`. Le site `q=(3M−1,3M−1,3M−1)` passe strictement le pavé `(-2M,3M)^3` de NUM-GARDE. Le certificat construit avec `s=20` accepte. Pourtant le **premier produit** de puissance vaut

`D |q|² = 215333976975021689807285368499032031250 >= 2^127`.

Le résultat final vaut `151531357695222388613514543567625781250 < 2^127`. Cela réfute précisément un calcul signé i128 non contrôlé : le résultat rentre, un intermédiaire déborde. Le certificat construit pour `s+2=22` refuse ce témoin. Voir `certificate_guard` dans RESULT.json ; expression native v11 dans `src/num/predicates.cpp` et fabrique dans `src/num/sphere.cpp:44–56`.

**Correction requise.** Un certificat doit porter son domaine garanti : exposant de requête et propriétaire/repère, ou un exposant maximal conservé dans la boule. Pour le census protégé par NUM-GARDE, le certificat q3 doit être calculé au moins pour la borne effective des différences support–requête, ici `s+2`, jamais seulement pour `s`. Un booléen hérité de la fabrique v11 ne doit pas être réinterprété silencieusement.

**Preuves conservées sous la bonne portée.** Avec tous les deltas bornés par `M=2^t`, puissance : `D|δ|² < 3·2^123`, chacun des trois termes `2N_jδ_j < 2^125`. La somme des magnitudes est `<15·2^123<2^127`, ce qui borne aussi toute somme partielle. Pour l'orientation, `|N_j+D·offset_j|<2^(125−2t)` et le produit vectoriel de **trois points d'un même cube** a chaque composante `<2^(2t)` : trois produits de magnitude `<2^125`, somme `<3·2^125<2^127`. Cette dernière borne renforcée ne s'applique pas à deux vecteurs arbitraires. Voir preuve v11 `src/num/predicates.cpp:186–216`.

## 2. Bloquant : la clé Morton sert aussi d'identité géométrique en v11

Contrat **115–120** : la phrase « la clé ne décide donc rien » ne décrit pas tout le moteur v11.

- `src/cloud/cloud.cpp:96–100`, `count_sites`, compte les clés distinctes.
- `src/cloud/cloud.cpp:110–126`, `fill_cloud`, groupe les clés égales en **un site**, sans comparaison XYZ.

Avec `P={(0,0,0),(1,0,0),(2^32−1,0,0)}`, les clés proposées se calculent après un décalage de 11 bits. Les deux premiers points distincts ont la même clé ; le groupeur hérité produit **2 sites pour 3 positions**. Suivant le contrôle D8 raccordé, le résultat serait une fusion indue ou un refus de doublon indu. Le départage par PointId ne répare pas cette identité.

Même remplacer l'égalité de clé par une comparaison XYZ entre voisins du tri `(clé,PointId)` ne suffit pas : les enregistrements `(A,id1),(B,id2),(A,id3)` d'une même clé mettent les deux vrais doublons A à distance. `morton_identity` conserve les deux témoins.

**Correction requise.** Séparer la clé de localité et l'identité XYZ exacte ; détection/refus/dédoublonnage exact avant fabrication du Cloud, ou regroupement XYZ explicite dans chaque classe de clé. Conserver les IDs et multiplicités conformément à D8. La recherche radix doit aussi employer les coordonnées **normalisées/décalées** correspondant à la nouvelle clé : `src/index/build.cpp:24–35` lit actuellement le bit de la coordonnée originale. La branche clés égales et sa profondeur doivent être prouvées séparément.

Portes : trois positions dont deux clés égales ; deux vrais doublons séparés par un autre site de la même clé ; permutations avec IDs stables ; booléen D8/refus et mode sites distincts ; boîtes exactes et ensemble des sites intacts.

## 3. Garde : preuve valide pour une boule critique certifiée, pas toute présentation

Contrat **49–63** : `c∈conv(S)` et `R≤diam(S)` sont corrects pour une boule critique positive ou une MEB exacte. Une coquille étendue ne les invalide pas : conserver son support critique `S*⊂U`, ou un support dont l'enveloppe convexe contient effectivement le centre. Le nombre d'autres points sur la coquille ne change pas la preuve.

**Avant certification de positivité, non.** `Sphere::through`/`Q3Candidate::through` ne certifie que la non-dégénérescence du triangle (`src/num/sphere.cpp:35–56`), et `Q4Candidate` conserve les présentations non positives (`geometry.hpp:107–110`, `sphere.cpp:78–98`). Ni une proposition flottante ni une sphère passant simplement par 3/4 sites ne bénéficie automatiquement de la garde.

Témoin exact : `S={(419,0,0),(435,15,0),(434,14,0)}` a `s=5` et un centre circonscrit `(419/2,479/2,0)`. Le point entier `q=(0,479,0)` est **sur sa sphère**, mais hors du pavé proposé autour de l'ancre `(419,0,0)` ; la garde le rejetterait. Tous les points sont dans u32. La présentation est non critique : ce témoin oblige précisément à qualifier le propriétaire auquel on attache NUM-GARDE. Une proposition flottante doit d'abord être certifiée, ou rester dans un chemin générique exact sans cette garde.

**Boîte : “sort du pavé” doit devenir “est disjointe du pavé”.** Une boîte qui déborde partiellement peut contenir tout le support et le centre. Témoin : support q2 `(100,100,100),(102,100,100)`, garde `(92,112)^3`, boîte `[0,200]^3`. Rejeter cette boîte perd les deux sites de coquille. Deux solutions sûres : raffiner la boîte partielle ; ou en intersecter le domaine avec la garde pour le **minorant seulement**, en gardant la connaissance que la partie exclue est extérieure. Aucun comptage “tout intérieur” du nœud initial sur un majorant calculé dans la seule boîte découpée.

**LEM-LATTICE : point entier requis.** La projection continue du centre n'est généralement pas entière. Pour le segment q2 de 0 à 1, puissance `2x²−2x` : minimum continu `−1/2` à `x=1/2`, minimum entier `0`. Reprendre le plancher/plafond exact puis la saturation à la boîte de `src/num/lattice_bounds.hpp:1–17` et `predicates.cpp:301–320`, pas une projection rationnelle annoncée comme minimum entier. Une borne continue peut être sûre, mais c'est un autre contrat et un autre coût.

**Repère uniforme.** Pour une requête unique passée par la garde, les différences entre cette requête et les sites du support sont `<3M<2^(s+2)` : bon budget de puissance. En revanche, tout le pavé a une largeur presque `5M` ; le minimum par axe de **toutes** les requêtes ne donne pas un unique NUM-REPERE à `s+2`. Écrire soit “ancre de support conservée, preuve par requête”, soit un enveloppement commun à `s+3`. La première solution conserve l'intérêt de la garde.

La porte **147–148** “site sur la sphère à la limite du pavé” est impossible sous les hypothèses strictes de cette garde : la sphère est strictement dans le pavé. Distinguer contact à la sphère, contact au pavé extérieur, et contact boîte/pavé.

## 4. Réservoir G1 : une expression auxiliaire manque au tableau

Le budget `2s+3` de G1 lui-même est justifiable : norme carrée `<3M²`, différence de deux normes de magnitude `<3M²`, somme des trois termes positifs `2·largeur·(x−y)` `<6M²`. La fermeture de boîte et les deux sites doivent tous être couverts par le repère.

Mais `src/catalogue/boxes.cpp:16–27` prépare avant G1 un réservoir par la distance `Σ(2x−lo−hi)²`, de magnitude `<12M²`, donc budget **`2s+4`**. À `s=30`, boîte `[0,1]^3`, site `(2^30−1)^3`, il vaut `3(2^31−3)² > 2^63`. `reservoir` donne sa valeur exacte. Le seuil i64 de ce réservoir est **s≤29**, pas celui s≤30 du prédicat G1. Porter un étage entier en i64 sur le seul seuil de G1 serait incorrect. Ajouter les préparations auxiliaires au budget, ou les garder en i128.

## 5. Budgets, centres absolus et réponses aux quatre questions

Les seuils du tableau **72–85** sont arithmétiquement corrects pour les expressions citées et pour `|Δ|<2^s` : i64 si budget≤63 ; i128 si budget≤127. `cutoffs` recalcule tous les seuils. Ce sont des **garanties suffisantes**, pas des impossibilités de calcul natif au-delà : “ne sont jamais natifs” aux lignes 96–99 doit se lire “ne sont plus garantis natifs par la borne uniforme”. Les constantes 192/320 à la ligne 91 doivent inclure la voie 512 explicitement nécessaire aux comparaisons de niveaux annoncée ligne 99.

Le changement de B en s exige de reconstruire les types et conversions des intermédiaires : par exemple v11 impose `DotInt=i64` (`predicates.cpp:247–252`), des centres i128 (`budgets.hpp:29–30`, `sphere.cpp:52/86`), et un global i128 dans `centers.cpp:7–16`. Une conversion rétrécissante avant `__builtin_*_overflow` ne certifie rien. Repartir des coefficients exacts originaux en cas de refus.

Les bornes absolues **111–112** sont correctes en précisant `s` = étendue des supports, `s≤B`, et, pour deux boules, `s=max(s_a,s_b)` :

`|aD+N| < 2^(B+4s+6)` ; chaque produit croisé `<2^(B+8s+11)`.

Comparer les deux produits séparément, comme v11, évite un bit supplémentaire de soustraction. Au maximum B=s=32 : 166 bits pour un numérateur global, 299 pour un produit comparé, donc 192/320 bits conviennent. Les niveaux se calculent chacun sur leur propre support ; leur comparaison utilise le maximum des deux budgets, sans agrandir le repère à la distance entre les boules.

**Réponses au §9 révisé.**

1. **Non pour toutes les propositions ; oui pour les boules critiques certifiées et leurs coquilles étendues.** Attacher la garde à un type possédant réellement la preuve de positivité/MEB. Les candidates flottantes ou `through3/4` rejetées restent hors de cette portée (§3).
2. **La voie large suffit pour l'exactitude** si tous les sites de liste et la fermeture de boîte sont couverts, types et budget mémoire compris. Aucune borne arbitraire d'étendue n'est nécessaire ; un refus ou une découpe serait une politique de ressources explicitement déclarée, pas un lemme géométrique. La limite fermée `2^32` peut exiger s=33 pour une feuille (constat principal). Requiert portes sites hors boîte, palier exact, voie GPU non résolue puis replay hôte complet.
3. **Oui : identité, regroupement et comptage des sites dans Cloud**, réfutation précise de la prémisse (§2). Aucun autre lecteur direct de `morton_key` trouvé dans `src/` hors Cloud, Morton et la coupe d'index. Cette recherche ne prouve pas l'indépendance de toutes les décisions aval vis-à-vis de l'ordre SiteIdx ; les supports canoniques et ordres de Kruskal doivent être audités séparément.
4. **Non, pour la comparaison lexicographique de deux centres dans le moteur v11** : recherche dans tout `src/`, un unique appel effectif de `num::compare_centers` à `src/tower/forest_build.cpp:93`, dans les cohortes de même rang définies lignes 70–80. Garder cet ordre exact ; pas de raison de modifier la sémantique pour économiser un produit large rare. Attention : des **coordonnées absolues** sont aussi calculées en census par `LatticeSphere` (`predicates.cpp:314`) et en test de milieu (`predicates.cpp:235–241`). “Global hors chemin chaud” exige donc un port relatif de ces préparations ; cela ne découle pas automatiquement du seul changement du comparateur.

**Réponses à la proposition WIP initiale (§8).** (1) Oui, minimum par axe + enveloppe de tous les arguments suffit, avec traitement explicite de la fermeture u32. (2) La garde entière est utile après certificat critique ; garder une solution générique pour les candidates, et préciser les boîtes partielles/LEM-LATTICE. (3) Oui conditionnellement au domaine complet des requêtes ; non comme drapeau indépendant de s. (4) Conserver l'ordre lexicographique exact, déjà invariant par translation.

## Portes minimales avant port

1. Témoin q3 ci-dessus : certificat support seul mutant refusé, certificat domaine de garde/refus contrôlé effectivement testé ; vérification des intermédiaires, pas seulement du signe final.
2. Identité XYZ indépendante de la clé Morton tronquée, collision distincte et vrai doublon intercalé ; retour D8 exact.
3. Garde attachée seulement aux boules critiques certifiées ; candidate obtuse conservée dans la voie générique ; boîte partielle, disjointe, contact sphère et contact pavé séparés ; minimum lattice exact.
4. Budget et palier du réservoir indépendants de G1 ; chaque préparation/multiplication/conversion du code porté explicitement couverte.
5. Niveaux maximum u32, comparaison 512 bits, centres absolus 192/320 ; deux supports de tailles locales différentes et translations extrêmes ; seuils joués dans les types réellement compilés.

Aucun gain de temps, distribution réelle d'étendues, exactitude GPU ou coût de replay n'est établi par ces preuves. Ils relèvent des mesures et portes natives ultérieures.
