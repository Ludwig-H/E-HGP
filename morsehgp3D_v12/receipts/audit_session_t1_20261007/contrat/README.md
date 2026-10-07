# Contre-lecture du contrat T1 — catalogue

7 octobre 2026 ; pin `4147c546000b198b5239646063bfb1e3ed6d28fc`.
Cadre `exploration_v12_hors_registre`, `cpu_reference`, `full_pi0`,
`quantized_u21_input_only`, `public_status=not_claimed`. Aucun GCP, aucun natif
ni gros jeu exécuté ici. Ce reçu répond au contrat proposé avant implantation ;
il ne qualifie pas un catalogue v12 encore absent.

**Réponse 1 : oui à la source J3 commune**, sous mesure de la voie CPU complète
sur `MES-P` et requalification des prédicats en repère local. Le coût local
annoncé ×1,4–1,9 ne suffit ni à rejeter ce choix ni à le qualifier. **Réponse 2 :
oui au lecteur de transition ; non à une tolérance limitée aux seules coquilles
multi-supports pour les sorties globales.** Le changement d'ordre des boules
à niveau égal modifie aussi Kruskal et `cover_v10` sur des supports uniques.

Dans les références, `C` = `docs/CONTRAT_CATALOGUE.md`, `N` =
`docs/CONTRAT_NUMERIQUE.md`, `A` = `docs/ARCHITECTURE.md`, tous dans v12.
Les approfondissements d'ordre ci-dessous relèvent de **CST-0113**, existant ;
aucun nouveau défaut de calcul natif n'est affirmé. La porte d'entrée T1 reste
`MES-M5` jugé (`PLAN.md:57`), encore « en cours » dans `C:5` à ce pin.

## 1. Source unique sur l'hôte : accord conditionné et concret

La maintenance d'une seule énumération, de ses seuils et de ses émissions est
un avantage suffisant pour essayer cette architecture (`C:29–32`). Garder le
DFS v11 gelé comme témoin et l'oracle borné indépendant : CPU/GPU partageant le
même algorithme peuvent partager le même défaut. Leur identité n'est donc pas,
à elle seule, une preuve de complétude.

Conditions d'adoption : mesurer le catalogue CPU entier puis la chaîne des
petits nuages, pas seulement la feuille ; choisir le seuil CPU/GPU selon
`MES-P` (100, 300, 1 000, 3 000, 10 000 sites, froid/chaud), en vérifiant les
objectifs publiés de `MESURE.md:43`. Le ×1,4–1,9 d'un microbanc de feuilles ne
préjuge pas du temps total, du parallélisme ni des coûts fixes. Si le résultat
échoue, réviser ouvertement la décision de conception avant une deuxième
implantation produit. Ne pas assouplir les objectifs après les mesures.

La source commune doit aussi décrire la terminaison du **repli exact hôte** :
la même structure J3, avec des opérations exactes de largeur suffisante, doit
résoudre les feuilles que le dispositif numérique GPU refuse. Rejouer une
feuille sur le même domaine arithmétique limité ne la résout pas. Ceci relève
du port numérique prévu, pas d'un défaut actuellement reproduit. Aligner la
phrase ancienne `A:78` (« DFS exact pour les feuilles larges et les tests »)
sur le choix plus récent `C:29–32` (DFS réservé aux tests).

## 2. Lecteur de transition : comparer l'objet puis les conventions

L'énoncé « le choix du **sous-ensemble** S* ne change que sur une coquille à
plusieurs supports de cardinal minimal » est juste (`C:15–18`). Il ne décrit
pas toutes les différences d'encodage, d'ordre et de sélection. `A:20–21`
prescrit les coordonnées pour tous les ordres publiés ; `N:211–239` change
Morton et les formats `supports`/`cover`.

**Témoin exact, trois sites** : A=(0,1,1), B=(1,0,1), C=(1,1,0). Les trois
boules de diamètre AB, AC, BC ont rayon carré 1/2, p=0, m=q=2 et un support
unique. La boule du triangle a centre (2/3,2/3,2/3), niveau 2/3 et q=3 ; elle
n'appartient pas à Cat_1. Aux mêmes coordonnées :

| Convention | Ordre des trois boules de niveau 1/2 | Kruskal à K1 | Choix cover à K2 pour A/B/C |
| --- | --- | --- | --- |
| SiteIdx/Morton v11 | BC, AC, AB | BC, AC | AC / BC / BC |
| Listes de positions lexicographiques | AB, AC, BC | AB, AC | AB / AB / AC |

À K2 et au niveau 1/2, les trois intersections de paires sont trois points
distincts ; elles ne se rejoignent qu'au niveau 2/3. Les choix de composante
du tableau sont donc réellement différents. La forêt géométrique et la
relation de couverture multivaluée sont identiques. Cette preuve exacte est
un **modèle des conventions**, pas une exécution d'un exporteur v12.
Sources des conventions : v11 `docs/MATHEMATIQUES.md:930–942` ; v12
`reference/hgp12_ref/dumps.py:6–14` (port explicite du format ancien).

Le lecteur à graver avant le catalogue doit :

1. Refaire l'identité de site par position et celle de boule par centre exact
   et rayon carré, sans utiliser S* comme identité entre versions. Exiger une
   bijection : absence, doublon ou boule supplémentaire sont des échecs.
2. Comparer exactement p, m, q_min, I, U et les niveaux ; interpréter les
   rationnels non réduits par leur valeur. Un retri ne tolère aucune population
   ni géométrie changée.
3. Pour chaque S* différent comme ensemble, certifier la même boule, le même
   cardinal minimal et le minimum lexicographique propre à chaque convention.
   `m>q` seul n'est pas une preuve de plusieurs supports admissibles.
4. Recalculer l'ordre publié et les renumérotations. Pour `supports`, refaire
   la sélection canonique de Kruskal ; pour `cover`, vérifier la relation de
   couverture **et** le choix prescrit par la nouvelle convention. Accepter
   seulement « un élément possible » de la relation serait trop permissif.

Le différentiel FULL à coordonnées identiques et la porte de translation
restent séparés comme `N:233–239,281–284` le prévoit. Le présent témoin complète
`WIT-TRANSL` et le carré à deux diagonales ; il empêche qu'un lecteur correct
rejette une transition attendue, ou masque un choix canonique erroné.

## 3. Le front : ordre parent à fixer pour MES-M5

**CST-0113, précision de contrat avant port.** L'égalité des feuilles par BFS
et DFS est prouvable si chaque nœud prépare la même liste ordonnée à partir
du même parent : réservoir avec le même départage, filtre strict, compactage
stable, enveloppe, axe et milieu identiques. Une induction sur l'arbre donne
alors les mêmes nœuds/feuilles, indépendamment de leur calendrier. Le potentiel
Σceil(log2 largeur) donne bien profondeur ≤3B, pas une borne du front mémoire.

Mais `N:223–231` remplace Morton absolu par Morton normalisé, et `A:63` départage
les distances du réservoir par rang dans la liste parente. Ce changement peut
modifier le filtrage lui-même. Le petit modèle indépendant de
v11 `src/catalogue/boxes.cpp:16–81,115–154` prend les six points :

`(2,5,2),(4,7,4),(8,10,9),(9,12,11),(10,8,6),(12,10,4)`.

Avec K1, réservoir 3K et seuil de feuille **3**, il atteint la boîte
[2,7)×[5,13)×[2,7). Les deux premiers témoins sont identiques. Le troisième,
à distance doublée au carré 134 du centre, est (10,8,6) en ordre absolu et
(8,10,9) en ordre normalisé. Le second domine strictement (9,12,11), avec
marge minimale **7** sur la fermeture entière de la boîte ; le premier ne
l'élimine pas. Les deux parcours du modèle donnent 34 feuilles mais des
contenus différents et 1 044 contre 1 042 tests G1. À ordre initial fixé,
BFS et DFS rendent exactement les mêmes feuilles et compteurs.

**Portée : modèle Python exact, pas une divergence du front natif reproduite,
ni un témoin aux réglages mesurés 16/24, ni une perte de Cat_K.** Un filtre
K-certifié peut conserver des surensembles différents sans perdre une seule
boule. Le contrat doit donc fixer l'ordre parent de référence pour MES-M5 ;
une normalisation d'ordre déclarée se juge par Cat_K, avec écarts de feuilles
et de coûts publiés, et non par une égalité inconditionnelle à la partition v11.

## 4. Paliers, admission et compteurs : préconditions à conserver

**Ambiguïté mineure, pas de débordement démontré.** `C:38–39` dit que s≤17 est
entièrement étroit. Cela couvre bien la voie historique v11
`leaf_device_predicates.hpp:191–204`, de diamètre d'enveloppe D≤2^20, reprise
par le microbanc. Ce n'est pas le palier **proposé** s≤16 de `N:185–188` : la
borne générale d'orientation 7s+9 vaut 121 bits à s16, 128 à s17. Fixer le
palier réellement retenu et ses prédicats/certificats ; les supports s≤15
ne couvrent pas automatiquement tous les autres sites interrogés. Les seuils
sont suffisants : dépasser un seuil ne prouve pas un dépassement effectif.

Les changements de planification n'altèrent pas G1–G4 sous leurs hypothèses :
racine complète, K plus proches avec tous les ex æquo conservés, propriété
demi-ouverte des centres, census I/U complet après admission, positivité
exacte et choix S* parmi tous les supports survivants. Changer le minimum
canonique de Morton aux positions conserve l'argument d'élagage de ses
préfixes. Préciser aussi « catalogue positif, q=q_min≥2 » dans `C:11` évite
de confondre les naissances de sites à niveau zéro avec ses boules.

Le comptage exact avant écriture (`C:46–51`) répond bien au défaut de prévision
empirique CST-0211. Son port doit inclure le repli des feuilles non résolues
**avant admission**, les offsets/sommes contrôlés, le budget des listes du
front et de la fin d'étage, puis l'engagement transactionnel de toute la trame.
Le résultat partiel et les compteurs d'une tentative non résolue sont jetés ;
une feuille contribue une fois aux compteurs logiques après succès. Les deux
passes comptage/écriture ne doublent pas ces compteurs. Les coûts physiques
des tentatives et des deux passes restent mesurés. Ce sont des conditions
d'implantation, pas des défauts natifs constatés.

Enfin « une trame de 60 000 sites tient en un lot » (`C:53`) est une prévision
de régime, non une conséquence de n seul ; la règle d'admission certifiée et
le refus transactionnel doivent continuer de prévaloir. Les mêmes feuilles,
paliers, conventions et paramètres sont nécessaires pour comparer les quinze
compteurs de feuille ; l'ordre du réservoir montre pourquoi cela ne se déduit
pas de la seule égalité du catalogue. Pas de nouvelle contre-lecture du lemme
des faces ni de M4 dans ce reçu.

## Rejeu et empreintes

`check.py` contient les deux témoins et l'arithmétique des seuils. `sources.json`
épingle douze fichiers relus ; le script contrôle les sources, les blobs au pin,
HEAD et sa propre empreinte avant/après. `normal.json` et `optimized.json` ont
été obtenus avec succès et sont identiques octet pour octet, sans `assert`.

```sh
python3 morsehgp3D_v12/receipts/audit_session_t1_20261007/contrat/check.py
python3 -O morsehgp3D_v12/receipts/audit_session_t1_20261007/contrat/check.py
```

Un HEAD ultérieur est admis si les sources épinglées sont identiques ; sinon
rejouer au pin. `SHA256SUMS` clôt ce reçu. Aucun produit, audit courant ou
registre modifié, aucun commit/push.
