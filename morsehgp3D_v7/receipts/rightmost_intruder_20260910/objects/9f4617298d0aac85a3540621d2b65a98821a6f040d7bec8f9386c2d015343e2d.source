# Découpler la résolution géométrique de la normalisation temporelle : preuve conditionnelle

10 septembre 2026, second auditeur. Réponse à la question du constructeur (canal
de coordination, reprise de 20:43 UTC) : les MEB et les descentes d'intrus
d'un ordre peuvent-elles être calculées en grands lots, CPU ou GPU, **avant**
la reconstruction des parents ? Cadre : `phase=exploration_v7_hors_registre`,
`public_status=not_claimed` ; aucune implémentation ni promotion ; GCP non
utilisé par cette note.

**Réponse : oui, sous les prémisses déjà exigées par le raccord, avec deux
énoncés distincts.** (A) Le terminal admis d'une descente ne dépend pas de
l'état des ancres au moment où elle est exécutée. (B) La racine pré-lot
obtenue ne dépend pas du choix des intrus le long de la descente. Seule
l'association « clé terminale → jeton de composante pré-lot », puis les
unions du lot, restent temporelles ; leur coût est celui d'une recherche
dans le catalogue et d'un union-find.

## Notations et prémisses

Ordre K fixé, catalogue C de boules (clé, niveau exact, I, U, q_min) supposé
complet au sens S1 sur la fenêtre `p+q_min ≤ min(Kmax+1, n)`. Les lots sont
les classes de boules de même niveau exact, traités par niveau croissant.
Pour un lot de niveau r, chaque bloc fournit ses représentants stricts F
(`MEB(F) < r` par construction du quotient local ou des retraits réguliers).
Intervalle d'ancre d'une boule à l'ordre K : `[p+q_min−1, min(Kmax, p+u)]` ;
le raccord installe l'ancre `(K, clé)` de chaque boule d'un lot, pour tous
ses ordres programmés, à la fermeture de ce lot, et jamais avant.

Règle géométrique pure, sans consulter les ancres : `F₀ = F` ; à l'étape i,
`B_i = MEB(F_i)` de clé `k_i` et niveau `ℓ_i` ; si `k_i ∈ C` et
`K ∈ intervalle(k_i)`, arrêt sur le **terminal admis** `t(F) = i` ; sinon
`z_i` = un point strictement intérieur à `B_i` hors de `F_i`, choisi par une
règle déterministe quelconque ; s'il n'en existe pas, arrêt en **échec** ;
sinon `F_{i+1} = F_i − v_i + z_i` avec `v_i` un sommet du support choisi de
`B_i`.

## (A) Indépendance vis-à-vis de l'état

1. Les niveaux le long de la chaîne sont non croissants et tous strictement
   inférieurs à r : `F_{i+1} ⊂ B_i` donc `ℓ_{i+1} ≤ ℓ_i`, et `ℓ_0 = MEB(F) < r`
   (BALL_ANCHORS § 4 ; à rayon égal la coquille sélectionnée décroît, la
   chaîne est finie).
2. Pour tout i avec `k_i ∈ C`, la boule `B_i` a un niveau `< r` : son lot est
   fermé avant le lot courant, et son ancre à l'ordre K existe **si et
   seulement si** `K ∈ intervalle(k_i)`, puisque `programs[K]` contient
   exactement les boules dont l'intervalle contient K. Le prédicat « ancre
   présente » consulté par le resolver séquentiel coïncide donc, à chaque
   étape, avec le prédicat statique « clé admise à l'ordre K ».
3. Le resolver séquentiel (`resolve` de `full_ball_tower.hpp`) prend les mêmes
   décisions que la règle pure à chaque étape ; il retourne à l'étape `t(F)`
   le jeton `root(anchor[k_t])` normalisé dans l'état pré-lot, toutes les
   résolutions du lot précédant toute création de nœud ; il refuse exactement
   lorsque la règle pure échoue (terminal faible absent de C) ou rencontre une
   clé admise sans ancre, ce qui est une faute de calendrier impossible sous
   la prémisse 2. ∎

Conséquence : pour un ordre K, l'ensemble des chaînes et de leurs terminaux
admis est une fonction de la géométrie et du catalogue seuls. Il peut être
calculé pour tous les blocs de tous les lots, dans n'importe quel ordre, par
lots arbitraires, sur CPU ou GPU. La phase temporelle se réduit, lot par lot,
à : lire `root(anchor[k_t])` pour chaque représentant, grouper les blocs par
racines partagées, créer naissances et multifusions, dater contributions et
images verticales, installer les ancres du lot. Rien de cela ne calcule une
MEB.

## (B) Indépendance vis-à-vis du choix des intrus

À chaque étape, `F_i ∪ {z_i}` est une coface de cardinal K+1 contenue dans
`B_i`, donc de niveau `≤ ℓ_i < r` ; `F_i` et `F_{i+1}` en sont deux
K-facettes, donc adjacentes dans le graphe strict de l'ordre K avant r. Par
récurrence, toute la chaîne est dans la composante pré-lot de F, et le
terminal admis `B_t` aussi : après fermeture de son niveau, toutes les
K-facettes de `S(B_t)` sont dans une même composante (BALL_ANCHORS § 3), qui
est celle de F. Deux règles d'intrus différentes donnent des chaînes et des
terminaux différents, mais la **même racine pré-lot**. ∎

Conséquence : une implémentation par lots peut choisir ses intrus par toute
règle déterministe (premier point d'une plage, point de plus grande puissance
négative, résultat d'un balayage device), sans changer l'objet. En revanche
les compteurs de travail (`intruder_queries`, `same_radius_steps`,
`max_chain_steps`) dépendent de la règle : ils ne doivent entrer dans aucune
empreinte ni porte d'équivariance.

## Les quatre points soulevés par le constructeur

- **Clé née strictement avant le bloc** : automatique, `ℓ_t ≤ MEB(F) < r`
  (point 1). Une assertion `ℓ_t < r` dans la phase statique est gratuite et
  protège contre un catalogue dont un niveau serait faux.
- **Lots simultanés** : aucune chaîne ne rencontre une boule de niveau r
  (inégalité stricte), donc les blocs d'un même lot ne dépendent pas les uns
  des autres ; le regroupement par racines pré-lot partagées reste exactement
  celui du raccord actuel et se fait hors ligne, lot après lot, avec le même
  union-find « tout résoudre, puis unir ». La preuve d'unicité de la MEB
  garantit toujours qu'une facette née à r n'appartient pas à deux boules de
  niveau r.
- **Cas non régulier** : un bloc à coquille supplémentaire a plusieurs
  représentants, chacun avec sa chaîne ; les naissances de plateau n'ont pas
  de chaîne et prennent leur image verticale dans l'ancre `(K−1, clé)` fermée
  au même niveau ; les contributions gardent leur date de lot. Les
  continuations inertes gardent leur ancre. Rien n'est propre au régime
  régulier dans (A) et (B).
- **Aucun catalogue de toutes les facettes** : la phase statique ne manipule
  que les représentants (un par composante stricte et par bloc) et leurs
  chaînes transitoires ; les seules recherches sont des recherches de clé dans
  C. Le mémo de facettes reste facultatif.

## Ce que la preuve ne donne pas

Elle ne dit rien du coût : le nombre de représentants, la longueur des
chaînes et le nombre de MEB restent à mesurer, et la phase statique en calcule
au moins autant que le resolver séquentiel (elle ne bénéficie pas des arrêts
sur ancre… qui sont eux aussi statiques : elle s'arrête au même terminal).
Elle ne couvre pas l'ordre K=1, résolu sans MEB. Elle suppose le catalogue
complet ; une clé admise absente reste une faute de catalogue, à refuser.
Elle ne promeut ni la génération WSPD, ni l'archive, ni aucun temps.

## Obligations pour une implémentation

1. Égalité de l'objet (dix forêts, contributions, images verticales) entre le
   resolver séquentiel et la phase statique + temporelle, sur la porte à 28
   nuages et sur le corpus aléatoire de 2 507 nuages, avec au moins une règle
   d'intrus différente de celle du resolver (témoin de (B)).
2. Mutant tué : phase statique qui consulte l'état (par exemple qui saute une
   clé admise parce que son ancre n'est pas encore installée à l'instant du
   calcul) — un contrôle nommé doit exiger que le terminal soit purement
   statique, sinon un ordonnancement device différent changerait l'objet.
3. Assertion `ℓ_t < r` et refus explicite d'une clé admise sans ancre, jamais
   un repli.
4. Compteurs de travail publiés séparément des empreintes.

Témoin exécutable joint (`mutant_intruder_last/`) : le resolver du header
`0b72b4e9` modifié pour choisir le **dernier** intrus rencontré au lieu du
premier passe la porte à 28 nuages et laisse les empreintes de payload
identiques sur trois nuages générés à n=2 000, dont deux à coquilles
supplémentaires ; seuls les compteurs de travail changent. C'est une instance
de (B), pas une preuve de (A) ; (A) est prouvée ci-dessus.

Résultats du témoin (`mutant_intruder_last/`) : porte à 28 nuages verte
(170 320 contrôles), fixture à neuf points complète, empreintes identiques sur
`uniform`, `scanline_overlap_multiecho` et `scanline_single_pass` à n=2 000.
Les compteurs changent : sur `scanline_overlap_multiecho`, 28 870 requêtes
d'intrus contre 107 717 et 45 407 appels MEB contre 124 254 avec la règle
« premier intrus » ; sur `uniform`, 270 662 contre 311 864. Le choix de
l'intrus est donc aussi un paramètre de coût, à mesurer, jamais un paramètre
d'objet.
