# Contre-lecture mathématique de T2 — 7 octobre 2026

Pin `274592a30f6961cb7702125dcd2f031ff22b7df2`. Cadre : exploration v12 hors registre,
CPU de référence, objet FULL π₀, profil d'entrée quantifié u21, `public_status=not_claimed`.
Lecture du contrat T2, de l'oracle et des preuves ; aucun produit modifié, aucun test natif,
GPU, GCP ou jeu réel. Les petits nuages de ce reçu sont tous sans doublon.

## Réponses aux quatre questions du § 13

1. **La lecture de l'élément d'union est correcte pour la forêt.** Si une résolution s'arrête
   sur `(b',k)`, son niveau est au plus β(F₀), strictement inférieur au niveau λ de la
   jonction appelante. Après le plateau de b', tous ses représentants appartiennent à la
   même composante ; le premier représentant et l'élément enregistré désignent donc la
   même composante à la coupe ouverte λ. L'appelant doit relire la **racine courante** de
   cet élément. Cela demande de traiter toutes les cellules de fenêtre non naissances,
   **inertes comprises**, et de terminer leur plateau avant toute lecture à un rang
   supérieur. C'est ce que fait `_forest_v12`. Aucun usage avant β(F₀) n'est autorisé.
2. **G-L3 donne un pas valide.** K candidats distincts ne suffisent pas : il faut bien
   k **sites distincts strictement intérieurs**, chacun prouvé par un test exact. Ils
   constituent alors une k-partie de I, donc un pas du théorème D, avec rayon minimal
   strictement décroissant. Le départage par `SiteIdx` n'affecte pas cette preuve. Si le
   compte manque, le census reste nécessaire ; le fait de repli livré est rejoué ici.
3. **LEM-HORS-CAT est correct après restriction aux boules positives, donc k≥2 sur des
   sites distincts.** L'énoncé actuel omet cette restriction (`CST-0229`, ci-dessous).
   Un support minimal
   du centre a des coefficients strictement positifs et des sites affinement indépendants
   après élimination des dépendances ; Carathéodory donne q_min ≤ 4, y compris sur une
   coquille cosphérique. L'admission p + q_min ≤ K + 1 implique hors catalogue p ≥ K − 2 ;
   p < k implique k ≥ K − 1. À p = K − 2, nécessairement q_min = 4. On a même q_min ≤ k
   pour la boule minimale d'une k-partie. Un échec de recherche par support n'établit
   **pas** l'absence géométrique du catalogue. Un catalogue incomplet viole la prémisse.
4. **L'adaptateur de test respecte un chemin produit unique** s'il construit le même type
   `Catalogue`, applique ses invariants et reste une entrée de test. Il ne qualifie pas T1.
   La sortie T2 exige bien T1 dans la chaîne mesurée ; cette condition est écrite au § 10.

## Points à corriger ou préciser avant le natif

- **Domaine de LEM-HORS-CAT, § 4.2 (`CST-0229`).** T1 définit Cat_K comme les boules
  **positives**, q_min≥2, en excluant les sites de niveau zéro. Sur trois sites distincts
  `(0,0,0),(2,0,0),(4,0,0)`, K=3 et F le premier singleton (k=1), B(F) est donc hors de
  Cat_3, avec p=0 et q_min=1 : la conclusion p≥K−2=1 est fausse. Exiger k≥2 (sites
  distincts) ou directement un rayon positif. La branche k=1 du résolveur rend le site
  immédiatement et G1 travaille aux ordres k≥2 : aucun faux résultat de ces chemins n'en
  découle. Attention à la différence de convention de l'oracle : `Reference._by_key`
  **inclut** les boules de rayon nul ; le contrôle des 37 parties hors catalogue ci-dessous
  ne pouvait pas révéler ce bord. `HORS_CAT_zero_radius_counterexample` le grave maintenant
  avec le catalogue positif de T1, filtré explicitement. La première version de ce reçu
  avait accepté l'énoncé sans cette réserve ; la contre-lecture du coordinateur l'a corrigée
  avant publication.
- **Compteurs, § 8 (`CST-0228`).** Les profondeurs d'attache ne sont pas intrinsèquement indépendantes
  de l'ordre de visite. Le cycle d'un carré à K1 donne quatre jonctions au même niveau.
  Union par taille, même départage par indice : ordre `(01,12,23,30)` donne profondeur
  maximale 1 ; ordre `(01,23,12,30)` donne 2. Même multifusion, mêmes trois unions, borne
  logarithmique respectée. Un census arrêté après k témoins peut également examiner 2 ou
  3 sites suivant la visite, pour un même certificat saturé. Le petit modèle du script
  rend ces différences explicites ; il ne prétend pas mesurer l'index natif. Il faut
  fixer l'ordre canonique jusque dans les routes comparées, ou sortir ces compteurs de
  l'empreinte intrinsèque et publier séparément le travail déterministe d'une politique
  donnée. L'invariance W1/W48 reste une bonne porte pour une politique fixée.
- **Branche saturée, § 4.1.** Le commentaire « sphère hors de Cat_K » n'est pas vrai.
  Carré `(0,0),(4,0),(4,4),(0,4)`, intérieurs `(2,2),(2,1)`, K=5, F la diagonale
  `(4,0),(0,4)`, k=2 : boule de niveau 8, p=2, q_min=2, admise au catalogue, mais son
  S* est l'autre diagonale. La sonde par le support local échoue et le census sature.
  `saturated_in_catalogue` grave ce cas, complément de la correction des census complets.
- **Décroissance, même pseudo-code.** Le `continuer` du census saturé précède l'exigence
  de décroissance. Placer cette exigence et la mise à jour de l'état précédent avant
  toute sortie de boucle par saut, y compris cette route. L'oracle Python actuel vérifie
  effectivement chaque boule minimale ; la preuve mathématique du saut est correcte.
- **Budget G-L3, § 6.** Le budget mixte `6s+11` suppose la garde spatiale du certificat.
  Les voisins et les fenêtres de Morton peuvent fournir des candidats extérieurs au
  pavé. Exiger le rejet NUM-GARDE avant l'évaluation mixte, ou un repli de budget général.
  C'est une obligation de l'appelant futur, pas une réouverture des primitives CPU déjà
  testées (`CST-0108`, `CST-0111`). `512` bits couvre la comparaison maximale annoncée
  `14s+20` à s≤32 (468 bits), sans attribuer ce budget aux candidats non gardés.
- **Cible de 4 octets, § 7.** Spécifier l'encodage naissance/cellule et les sentinelles.
  Un bit de genre exige aussi une borne de 31 bits sur l'indice cellule ; un domaine
  concaténé exige le contrôle de sa somme. La borne annoncée seulement sur les naissances
  ne suffit pas à elle seule à prouver la capacité du type cible. Aucun débordement d'un
  produit T2 n'est allégué : ce produit n'est pas livré à ce pin.

## Preuve exécutée et limites

`check.py`, Python nu, sans `assert`, compare directement à l'étage A de définition
**10 nuages, 35 ordres, trois politiques** : arbres, verticales, core, cover et coupes
ouvertes/fermées. Tous passent. **2 274** résolutions tracées, **2 286** boules minimales,
**6** cibles inertes effectivement lues ; chaque saut descend strictement et chaque cible
de représentant précède strictement sa jonction. **37** parties hors catalogue satisfont
les bornes de LEM-HORS-CAT. Les deux faits nommés du développeur passent également.

Les dates sont contrôlées explicitement : WIT-D2 descend de 64 à 1 alors que le niveau
précédent du catalogue vaut 41 et la jonction 1681/25 ; WIT-MEMO descend de 9 à 1. Le
terminal ne donne donc pas une date d'usage anticipée. `CST-0104` demeure **en cours** :
la règle oracle et ces témoins d'audit sont couverts, mais la porte native du mémo, les
appels datés et le mutant « date terminale » restent à livrer. `CST-0105`, `0106`, `0107`
ne sont pas clos par ce reçu : ni historique d'attache natif, ni quotient LEM-T7, ni
vidage FULL apparié à la v11 ne sont exécutés ici. Les verticales de cet oracle utilisent
encore la descente historique et la naturalité ; leur réussite ne qualifie pas T5/T6 natifs.

Les nouvelles lignes SUP-KRUSKAL distinguent désormais couverture par C.3 et connexité par
P : correction documentaire recevable. La sélection garde des hyperarêtes avec leurs
branches d'origine ; l'expression « arbre couvrant de l'hypergraphe » n'interdit pas des
liaisons redondantes internes à une hyperarête retenue, ce que la source précise. Seuls
les événements binaires ayant réellement uni deux composantes forment un arbre au sens
ordinaire. Le juge EMST et la publication native demeurent des portes distinctes.

Le test livré complet de 342 nuages et ses quatre mutants n'est **pas** relancé : ses
totaux ne deviennent pas des résultats de cet audit. Le script d'audit a d'abord refusé
son propre jeu de neuf fixtures, car aucune cible inerte n'était exercée ; il a ensuite
ajouté la fixture distincte `grid3_11_3` de la suite publiée. La recherche de cette fixture
a parcouru 116 petits nuages distincts (moins d'une seconde), sans rejouer la suite entière.
Le seul changement était au harnais d'audit ; aucun défaut du produit n'en est déduit.

Rejeu depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v12/receipts/audit_t2_20261007/mathematiques/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_t2_20261007/mathematiques/check.py
```

`normal.json` et `optimized.json` sont identiques octet pour octet, codes 0. Les sources
sont hachées, comparées au pin Git avant lecture, puis rehachées après l'exécution ; le
script vérifie aussi sa propre stabilité. `verification.json` et `SHA256SUMS` ferment la
capture. Pas de gain G4, de qualification FULL à l'échelle ou de contrat 100 ms acquis.
