# Contre-vérification adverse : source E-HGP et Zoltan

4 octobre 2026, 13 h 18 UTC (`date -u`). Contre-vérificateur adverse du rapport
`../fouille/ehgp_zoltan.md` (trois idées). Rappel de l'utilisateur : **le contrat reste 100 ms** (FULL K1..5,
trames SemanticKITTI sans sol, G4 W48).

```text
phase=exploration_v11_hors_registre (contre-vérification, lecture seule)
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Étiquettes : **M** mesuré (reçu ou sortie nommés), **M-loc** mesuré localement sur oracle borné, **E** estimé,
**C** conjecturé. v11 lue sur `origin/main` = `56216392e` (un commit après celui lu par l'auditeur, `eb036dbe2` ;
vérifié : rien de ce commit ne touche aux juges).

## 0. Verdict d'ensemble

- **Vers 100 ms, les trois idées valent zéro** : ce sont des outils de test hors du chrono. Le verdict de
  l'auditeur (« vers 100 ms : rien » pour E-HGP et Zoltan) est confirmé ; je n'ai trouvé aucune brique de calcul de
  ces deux sources qui touche la passe unique ou la résolution régulière.
- Leur intérêt est réel mais indirect : chaque levier vers 100 ms est aujourd'hui qualifié par **identité octet
  pour octet contre la v11 précédente** (exemple du jour : commit `56216392e`, « FULL dumps byte-identical on ng00
  and ng02 (K = 5, u21) »), et le différentiel v10/v11 sur LiDAR reste ouvert (`docs/DEVELOPPEMENT.md` l. 65). Une
  faute déterministe déjà présente se propage donc sans être vue. Les juges proposés regardent la géométrie.
- Verdicts : **03 garder** (égalité exacte, théorème déjà écrit dans la v11, non implanté) ; **01 à mesurer**
  (validité prouvée, force sur LiDAR conjecturée) ; **02 à mesurer** (validité prouvée, force et coût non mesurés,
  et il ne voit pas les naissances perdues).

## 1. Vérifications communes

| Affirmation de l'auditeur | Vérification | Résultat |
| --- | --- | --- |
| La v11 n'a pas d'oracle géométrique à l'échelle | `bench/full_semantic.py` l. 1 : « Strict FULL codec, without a geometric oracle on the measured cloud » ; `docs/FULL_FORESTS.md` l. 144-146 : « il ne remplace pas l'oracle géométrique indépendant des petites fixtures » ; `git grep` sur `src`, `bench`, `tests`, `reference` de `origin/main` : aucun juge d'arbre couvrant, de tranche ni de segment (les « J2 » du catalogue sont un autre prédicat : `docs/CATALOGUE_OPTIMISATIONS.md` l. 41) ; rien non plus dans `receipts/audit_deep_20261004` | **confirmé** |
| `docs/ARCHITECTURE.md` exige des invariants globaux à l'échelle | l. 150-152 : « (c) des invariants globaux à l'échelle » | **confirmé** |
| Le dump contient tout ce qu'il faut | `full_semantic.py` l. 104-160 : sites (x, y, z, poids 1, PointId), par ordre parent, enfants (CSR), niveau rationnel, centre rationnel de chaque naissance, image verticale vérifiée « vivante à la coupe fermée » ; à K1 « births == sites » et centre = site ; un seul arbre (`edges == count-1`, racine unique) | **confirmé** |
| Cardinalités ng00 | `Zoltan/FoundationModel/OBJET.md` l. 114-122 (K1..5) : somme des nœuds 1 541 750, des naissances 897 776, nœuds K2..5 = 1 462 069 ; identiques à `../cartes/CARTE_V11.md` § 2.5 (nœuds, naissances, verticales de ng00) | **confirmé** (recalcul local) |
| Mesure locale | `sha256sum` de `ehgp_zoltan_verif.py` (`ac38d28f…`), `.out` (`208456af…`), `_j2.out` (`984f459e…`) : conformes ; chiffres du rapport identiques aux sorties | **confirmé** |
| Pistes fermées | `docs/archive/abandoned/README.md`, `morsehgp3D_v3/audits/PISTES_FERMEES.md` : rien sur tranche, segment ou arbre couvrant (les « séparation » de v3 sont la séparation WSPD) | **aucune piste fermée** |

Lecture adverse du script local (`fouille/ehgp_zoltan_verif.py`) : oracle = étage A de la référence v11
(`Definition`, exhaustif) ; les surfaces sont exactes (plans à normale entière, boîtes à bornes demi-entières,
distance carrée rationnelle au bord, intérieur compris) ; T1/T2/T3 et le segment sont décidés en `Fraction`. Le
contrôle J2 (`kruskal_plateaus`) confirme au passage que le niveau v11 est le carré du rayon (fusions à d²/4, 0
désaccord), donc que la convention A_k(S) en distances carrées est la bonne. Limite : 8 sites, K = 3, nuages denses
dans 10³ ; aucune trame LiDAR ; aucun moteur natif.

## 2. ehgp_zoltan_01 — juge de tranche sur tout le dump FULL

**Preuve.** Source vérifiée : `E-HGP/docs/MOTEUR_ET_COUTS.md` l. 111-121 (commit `110f375e3`) et
`E-HGP/src/ehgp/engine/separation.py` l. 9-33 (`94df32d51`), énoncé pour un hyperplan ; sûreté auditée « 0 faux
certificat » sur 244 (`E-HGP/audits/AUDIT_OUVERTURE_20260926.md` l. 59). L'extension au **bord d'une boîte** est de
l'auditeur, pas d'E-HGP ; je l'ai refaite : un point y de S n'est couvert que par les boules dont le site est à
distance carrée au plus a de S ; si moins de k, y n'est pas dans L_k(a) ; le bord d'une boîte sépare l'intérieur de
l'extérieur, donc toute composante de L_k(a), connexe, est d'un seul côté. T2 tient parce que la composante d'un nœud
à son niveau contient les centres de toutes les naissances de son sous-arbre (chaque centre c vérifie
D_k(c) = niveau de naissance, et les sous-niveaux sont emboîtés). T3 tient parce que le dump impose que l'image
verticale soit vivante à la coupe fermée du niveau de v, et que L_k ⊆ L_{k-1}. Preuve **correcte** ; M-loc :
0 violation sur 565 311 contrôles. La leçon F-AUD-4 (`E-HGP/audits/FIXTURES_AUDIT_20260926.md` l. 80-98) est bien
lue : une porte qui ne regarde que la valeur finale laisse passer un certificat faux ; les mutants du juge sont
nécessaires.

**Déjà dans la v11 ?** Non (§ 1).

**Doctrine.** Respectée : décisions en rationnels exacts, flottant seulement pour proposer des surfaces, O(nœuds)
par surface, aucun tableau par paire, condition nécessaire seulement, aucune promotion de statut.

**Objections.**

1. **Force à l'échelle non démontrée.** Sur les petits nuages, le meilleur minorant ne vaut que 0,495 fois le vrai
   niveau de rencontre en médiane, 0,2 % d'égalités ; les mutants ne sont détectés qu'à 30,7 % (fusion précoce) et
   37,3 % (verticale fausse) (M-loc). L'argument « le vide entre objets rendra les certificats serrés » est C. Sur
   LiDAR sans sol, les grands objets (bâtiments, végétation) sont connexes et étendus ; la part des nœuds réellement
   contrôlée par une boîte posée dans le vide est inconnue.
2. **Faute visée.** Le juge ne voit que les fusions **trop précoces** (niveau publié abaissé, union parasite, course
   qui réunit deux racines). Les fautes connues de l'histoire (v10 N3 : boules perdues, `../cartes/CARTE_V10_VITESSE.md`
   l. 317) produisent des fusions **trop tardives** ou des naissances absentes, qu'il ne voit pas.
3. **Coût sous-estimé (E).** « Une minute par trame » suppose ~6·10⁷ opérations ; le test de côté d'un centre
   rationnel contre une boîte coûte six comparaisons de fractions, soit ~3,5·10⁸ comparaisons Python pour
   0,90 M naissances × 64 boîtes, plutôt plusieurs minutes, sauf masques par axe triés. Hors chrono, donc non
   bloquant, mais à mesurer.

**Verdict : à mesurer.** Banc : session G4 gardée après les dumps FULL appariés (`bench/full_paired.py`) des trois
trames du contrat, W48, K = 5 puis K = 10 ; publier par ordre et par trame le nombre de surfaces actives, la part des
nœuds et des verticales contrôlés par au moins un certificat, la distribution de A_k(S) / niveau, le taux de
détection des deux mutants injectés **dans le dump réel** (niveau de fusion abaissé dans son intervalle légal,
verticale échangée contre un autre nœud vivant), et le temps du juge. Le garder seulement si la part contrôlée et la
détection sont non négligeables à l'échelle.

## 3. ehgp_zoltan_02 — juge d'échantillon par segments

**Preuve.** `E-HGP/docs/OBJET_ET_DIMENSION.md` § 5 (l. 110-136 dans l'arbre de travail ; ce fichier est **modifié
non commité**, les lignes du commit `94df32d51` peuvent différer) : coefficient dominant commun, maximum en t = 0,
t = 1 ou à un croisement ; audité « tient » (`AUDIT_OUVERTURE_20260926.md` l. 57). Monotonie par sous-nuage
(`E-HGP/src/ehgp/engine/fast_point_tower.py` l. 64-92, `8cd859193`) : a_k sur C ⊆ X majore a_k sur X, donc toute
liste C (|C| ≥ k) donne un majorant valide. Le majorant du niveau du plus petit ancêtre commun de deux naissances
suit (le segment relie leurs centres dans L_k du maximum). **Correct** ; M-loc : 0 violation sur 27 754 paires.

**Déjà dans la v11 ?** Non.

**Doctrine.** Respectée (rationnels exacts, échantillon, aucun juge O(n³)).

**Objections.**

1. **Ce qu'il voit est plus étroit qu'annoncé.** Il ne compare que deux naissances **présentes** dans le dump : une
   boule de naissance perdue est invisible ; une selle perdue n'est vue que si la fusion publiée dépasse le majorant
   du segment. Le rapport cite le mutant N3 de la v10 (2 134 à 9 523 boules perdues) comme cible, sans montrer qu'il
   serait détecté ; pour les pertes de boules, l'identité d'Euler J3 (`docs/MATHEMATIQUES.md` l. 350-366, hors de
   cette source) est la cible directe.
2. **Force non mesurée sur LiDAR.** Majorant médian 1,162 fois le vrai niveau, 4 % d'égalités, mutant « fusion
   repoussée au parent » détecté à 48,8 % (M-loc, petits nuages denses). Entre deux lignes de balayage, le segment
   droit traverse des zones peu couvertes : le majorant peut y être lâche (C).
3. **Coût non mesuré** : |C| de quelques dizaines à quelques centaines de sites, calcul naïf O(|C|²) par paire en
   `Fraction` (E).

**Verdict : à mesurer.** Même session G4 que 01, sur les mêmes dumps : part des paires où le majorant est égal au
niveau publié, distribution majorant / niveau, détection d'un mutant « fusion repoussée » **et** d'un mutant de
perte de boules du catalogue (le seul qui justifierait l'idée), temps par paire. Le garder seulement si ce second
mutant est tué à l'échelle.

## 4. ehgp_zoltan_03 — juge exact de l'ordre 1 (arbre couvrant minimal entier, plateaux N-aires)

**Preuve.** Théorème écrit dans la v11 elle-même : J2, `docs/MATHEMATIQUES.md` l. 345-349 (« Comparer la structure
N-aire aux plateaux, pas seulement le multiensemble des longueurs »), et dans E-HGP (`OBJET_ET_DIMENSION.md` § 3 (b),
juge `scipy` 28 accords / 0 désaccord, `AUDIT_OUVERTURE_20260926.md` l. 53). La convention de comparaison (unions
simultanées à longueur égale, niveau d²/4, enfants vus comme ensembles de sites) égale l'étage A de la référence v11
sur 200 nuages, 1 610 fusions dont 304 N-aires, 0 désaccord (M-loc, `ehgp_zoltan_verif_j2.out`). Le dump l'autorise :
à K1, naissances = sites et centre = site (`full_semantic.py`). Les 88 enfants surnuméraires de ng00 se recalculent :
2 × 39 885 − 1 − 79 681 = 88 (E sur chiffres M) ; le juge exerce donc de vrais plateaux N-aires de la grille de 1 mm.
Aucun doute sur la preuve.

**Déjà dans la v11 ?** Énoncé seulement. Seuls contrôles de k = 1 : petits nuages (fixture F13 de
`bench/points_flat_gate.py`, suite `reference`). Aucun juge à l'échelle (`git grep`, § 1).

**Doctrine.** Respectée si l'arbre couvrant est exact en entiers (u21 : d² < 2⁴⁴, entier Python sans risque) ; le
Borůvka flottant de `hdbscan` (`Zoltan/demos/tools/hierarchy.py`) ne peut être qu'un recoupement.

**Objections et limites.**

1. **Couverture étroite.** L'ordre 1 ne dépend que des paires (q2) : le juge ne voit ni q3/q4, ni les descentes, ni
   les verticales, ni les ordres 2..5, c'est-à-dire là où portent la plupart des leviers vers 100 ms (q3 différé,
   compteurs, census, arènes). Il voit en revanche les fautes propres à l'échelle sur la voie q2 et la publication :
   boules q2 perdues par l'ordonnancement parallèle (classe du N3 de la v10), courses à W48, plateaux mal groupés.
2. **Coût sous-estimé (E).** Un Borůvka sur grille en Python nu peut ralentir dans les derniers tours, quand
   quelques composantes séparées de plusieurs mètres cherchent leur voisine à travers des cellules vides ;
   « 1 à 3 minutes » n'est pas mesuré. Hors chrono, non bloquant ; à faire tourner hors de la fenêtre FULL chronométrée
   (Python 3.10 sans numpy sur G4, tester sous `python3 -S`).

**Verdict : garder.** Égalité exacte d'un ordre entier à la taille du contrat, théorème déjà dans la v11, outillage
commun (lecture du dump, forme canonique) réutilisable par 01 et 02. Gain vers 100 ms : nul ; à placer comme porte
de non-régression des refontes de la voie q2 et de la publication, avec deux mutants (plateau binarisé, fusion q2
retirée) et des planchers (fusions comparées, plateaux N-aires ≥ 1).

## 5. Conclusion

| id | verdict | gain vers 100 ms | raison courte |
| --- | --- | --- | --- |
| ehgp_zoltan_03 | garder | nul | J2 écrit, non implanté ; égalité exacte à l'échelle ; couverture limitée à q2 et K1 |
| ehgp_zoltan_01 | à mesurer | nul | preuve correcte, force à l'échelle conjecturée, ne voit que les fusions précoces |
| ehgp_zoltan_02 | à mesurer | nul | preuve correcte, ne voit pas les naissances perdues, force et coût non mesurés |

Aucune des trois n'approche le contrat de 100 ms ; elles ne doivent pas passer devant les leviers de vitesse, mais
03 coûte peu et donne enfin un juge exact indépendant de la v11 sur les trames du contrat.

FIN
