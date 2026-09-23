# Audit C — implémentations alternatives pour le contrat LiDAR K5/K10 sur G4

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Date : 23 septembre 2026. Dépôt lu : le worktree `v9-audit-c-publish` à `5bfbc5a4`, en lecture seule. Chaîne mesurée localement : `93066733`. Reçu G4 de référence : R7b, paquet `8e8b83a3`. GCP non utilisé.

Aucun temps de ce document ne promeut un statut. Les chiffres locaux viennent d'un hôte partagé dont la charge a varié de 4 à 24. Seuls les compteurs déterministes valent preuve ; les temps sont indicatifs.

Sources : six propositions (D1 à D6), trois jurys (rigueur, performance G4, risque et doctrine) et six vérifications adverses (théorème et projection de D5, D1 et D3). Quand une vérification adverse a réfuté un chiffre, ce document retient le chiffre corrigé et le signale.

## Lecture de l'auditeur C

Méthode : six concepteurs indépendants ont chacun défendu une famille
d'implémentations (D1 continuité, D2 délétion locale de Delaunay, D3
séparation d'échelles, D4 inversion par site, D5 tour FULL maigre, D6 chaîne
GPU résidente), avec expériences locales sur les coupes LiDAR ; trois jurys
(rigueur, performance G4, risque et doctrine) les ont notées ; six
vérificateurs adverses ont tenté de réfuter le théorème et le chiffrage des
trois finalistes. Les propositions, notes de jury et réfutations sont
archivées dans [`c_alternatives_20260923/`](c_alternatives_20260923/README.md).

Ce que j'en retiens, en six points :

1. **Le verrou est le travail, pas seulement son ordonnancement.** Sur la trame entière
   08/000200 à K5, les arêtes propriétaires de plus de 1,6 m portent 66 à 70 %
   du CPU q3/q4 pour 5,9 % des boules émises ; c'est là que vit la croissance
   superlinéaire. Aucun découpage local (D3, D4, tuiles GPU de D6) ne
   l'atteint : il faut **réfuter les ancres longues avant l'expansion**,
   exactement la cible des certificats de bloc en cours chez le développeur et
   B. Critère de réussite chiffré : ramener cette part sous 25 %.
2. **La tour FULL a un levier mesuré et exact : D5.** Index des selles, saut
   au centre (lemme B, avec la « règle 0 » ajoutée par la réfutation), phase A
   sans allocation, images de naissance directes, sortie compacte. Racines
   identiques au produit sur 5,04 M facettes LiDAR et 221 556 facettes de
   nuages dégénérés ; gain réaliste ×6–13 sur FULL (K10 : 3,2 s → 0,25–0,6 s ;
   K5 : 0,73 s → 0,06–0,12 s). Nécessaire à toute cible, non suffisant.
3. **1 s à K10 est hors de portée de toutes les familles étudiées**
   (probabilité subjective < 0,1) : même avec q3/q4 et FULL gratuits, q2,
   fusion, recensement et plomberie dépassent déjà 1 s à K10 sur 000200.
4. **1 s à K5 reste possible** avec D5, la plomberie compacte **et** un port
   GPU de q3/q4 qui emporte aussi la réfutation des ancres longues — sous
   réserve d'une expérience G4 unique, à seuils fixés d'avance.
5. **100 ms** exige un ordre de grandeur de travail en moins, un générateur
   qui émette par niveau (pour recouvrir génération et FULL) et une sortie
   compacte hors digest : aucune des six familles ne le fournit. C'est à
   reformuler avec l'utilisateur plutôt qu'à poursuivre tel quel.
6. **Juges avant changement de générateur** : Euler (condition nécessaire),
   Kmax+2 à K5, égalité clé par clé avec la route CPU, et un juge
   d'échantillon par fenêtre autonome pour les ordres K9–K10. Une réfutation a
   confronté la chaîne actuelle à des oracles exacts sur 1 562 nuages adverses
   (775 566 boules) : **zéro écart**.

### Raccord avec R8 (reçu après la rédaction)

L'étude a été chiffrée sur R7b. Le [reçu R8](../receipts/g4_tower_r8_20260923/README.md)
(paquet `515b3666`, CPU seul) donne des chaînes à moins de 3 % de R7b
(K5 3,67/5,46/6,39 s, K10 9,40/13,69/15,19 s) : aucune conclusion ne change.
Deux faits nouveaux précisent la feuille de route.

**Le budget q3/q4 de 1 s à K5 dépend fortement de la trame.** Avec le
résidu hors q3/q4 publié par le [contre-audit R8 de B](CONTRE_AUDIT_B_G4_R8_20260923.md)
et la tour FULL ramenée par D5 (×6 à ×13) :

| Trame 08/ | Chaîne − q3/q4 (R8) | dont tour | Résidu avec D5 | Budget q3/q4 pour 1 s | q3/q4 W48 (R8) | Facteur exigé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 1,147 s | 0,699 s | 0,50–0,56 s | 0,44–0,50 s | 2,518 s | ×5–6 |
| 000000 | 1,507 s | 0,866 s | 0,71–0,79 s | 0,21–0,29 s | 3,953 s | ×14–18 |
| 000200 | 1,685 s | 0,921 s | 0,83–0,92 s | 0,08–0,17 s | 4,710 s | ×29–57 |

La probabilité 0,25–0,35 du § 1 vaut pour la trame légère. Sur la trame la
plus lourde, 1 s à K5 exige **en plus** de retirer 0,3 à 0,5 s des
0,76 s hors q3/q4 et hors tour (q2, fusion, recensement, plomberie) : c'est
l'étape 2 (pool persistant, arènes, temps système), qui n'est plus
optionnelle. Sans elle, je ramène la probabilité « trois trames sous 1 s à
K5 » à 0,1–0,2.

**L'ordonnancement de q3/q4 est un levier réel, à prendre avant le GPU.**
À W48, les fils q3/q4 attendent la file 35 à 49 % du temps à K5, et le CPU
cumulé passe de 42,5 à 77,3 CPU·s de W1 à W48 sur 000100
([rectificatif](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md)). Un
ordonnancement idéal à travail inchangé laisserait q3/q4 vers 1,6 s sur
000100 : c'est un gain possible de 0,9 s, sans effet sur le verdict « CPU
seul ≈ 0 ». Conséquence pour l'expérience GPU unique (§ 3.3, étape 5) : le
seuil ×11 doit se mesurer **contre la base CPU déjà réordonnancée**, avec
le même ledger de travail, sinon le gain GPU serait gonflé par l'attente
de file que le CPU aurait pu supprimer seul. La croissance des formes par
arête que B mesure à boîte fixe (moyenne 96→165→290 formes non-supports
par cœur) converge avec le poids des ancres longues : dans les deux cas,
le levier est un certificat **avant** `A×B`.

**Rappel de l'objet.**
- On part d'un nuage $P$ de $n$ sites distincts sur la grille 1 mm (18 bits), et on pose $L_{K}(r)=\lbrace x : \vert P\cap \bar{B}(x,r)\vert \geq K\rbrace$.
- La tour FULL est, pour chaque $K\leq K_{\max}$, l'arbre de fusion de $L_{K}(r)$ quand $r$ croît, plus les applications verticales $L_{K}\subseteq L_{K-1}$.
- Elle se déduit du catalogue des boules minimales critiques. Une telle boule a un support $q\in\lbrace 2,3,4\rbrace$ et son centre dans l'intérieur relatif de l'enveloppe du support. Le catalogue retient celles qui vérifient $p+q_{\min}\leq K_{\max}+1$.

**Le contrat.** Trames LiDAR sans sol de 30 à 60 k sites, K5 puis K10, objectif 1 s puis 100 ms. Machine : `g4-standard-48`, soit 24 cœurs physiques Zen 5 en SMT 2 et une RTX PRO 6000 sm_120.

## 1. Résumé exécutif

**Constat principal.** Aucune des six familles n'atteint 1 s à K10, et aucune n'approche 100 ms, sous quelque hypothèse défendable que ce soit.

La seule cible encore ouverte est **1 s à K5**. Elle exige une combinaison de trois choses :
- la tour FULL maigre (D5) ;
- une plomberie et une sortie compactes ;
- **et** un port GPU résident de q3/q4 qui déplace aussi sur le GPU la réfutation des ancres longues.

Ce port doit encore être justifié par une expérience de débit. Les cinq ports GPU antérieurs du dépôt n'ont jamais donné plus d'environ 10 % de gain de bout en bout.

**Ce que les mesures ont établi, au-delà des propositions.**

1. **Les ancres longues dominent q3/q4.**
   - Sur la trame entière 08/000200 à K5, les arêtes propriétaires de plus de 1,6 m portent environ 66 à 70 % du CPU q3/q4, pour 5,9 % des émissions.
   - La part qu'un traitement local ne peut pas prendre vaut 79,0 / 70,1 / 56,9 % pour h = 0,5 / 1 / 2 m. Les coupes 8k la sous-estiment de 25 à 34 points.
   - C'est là que se trouve la croissance superlinéaire, et aucune des six familles ne l'attaque directement.
2. **FULL est le second verrou, et il est séquentiel.**
   - Il pèse 3,20 s sur 9,58 s à K10/000100, et ne gagne que ×1,02 à ×1,12 en passant de 24 à 48 fils.
   - Sa « queue » (validation en série, populations, images, encodage, banque) vaut à elle seule environ 1,6 s sur G4 à K10/000100. Ce chiffre est inféré du différentiel MEB entre local et G4 ; il reste à mesurer directement.
3. **Le SMT n'est pas gratuit.** De 24 à 48 fils, q3/q4 gagne ×1,22 à ×1,45 pour 42 à 59 % de CPU·s en plus. Diviser des CPU·s par 48, ou compter 44 fils logiques utiles, donne une projection optimiste (de 20 à 30 % pour le plafond CPU de D1).
4. **Les postes « mineurs » interdisent à eux seuls 1 s à K10.** Sans q3/q4 ni FULL, il reste 1,15 s à K10/000100 (q2 0,41 s, fusion et recensement 0,46 s, reste 0,28 s) et 1,64 s à K10/000200.
5. **Le juge global d'échelle a un angle mort.**
   - L'invariant d'Euler, $n\cdot[K=1]+\sum_{B} e_{K}(B)=1$ pour $K\leq K_{\max}-2$, ne voit pas les ordres $K_{\max}-1$ et $K_{\max}$, soit 12 à 13 % des boules à K10 et 33 % à K5.
   - Il ne voit pas non plus certaines omissions qui se compensent.
   - Le protocole Kmax+2 est impossible à K10, car la chaîne refuse `kmax > 10`.
   - Tout changement de générateur exige donc aussi un juge d'échantillon exact.

**Recommandation, dans l'ordre.**

1. **D5 d'abord** (tour FULL maigre), avec les corrections imposées par la vérification adverse : règle 0 de descente, format compact qui conserve l'ordre des actions, tampon de racines sans borne fixe, porte par facette, chiffrage explicite de la queue FULL. Gain attendu sur FULL : ×6 à ×13. Nécessaire pour toute cible, jamais suffisant.
2. **Des portes communes à tout générateur**, à poser avant d'en changer :
   - propriétaire défini par le support, dans l'ordre global des identifiants ;
   - au moins une présentation d'arité $q_{\min}$ par clé ;
   - disjonction exacte ;
   - juge d'échantillon par fenêtre autonome ;
   - identités de conservation.
3. **La mesure décisive des ancres longues**, sur les trois trames entières, à K5 et K10, chronométrée par lot et non par arête.
4. **La recherche d'un certificat de bloc des ancres longues** (famille B), avec un critère d'arrêt chiffré.
5. **Une seule session G4 GPU**, qui regroupe trois essais : noyau cœur + certificat de voie morte, rejet des ancres longues en lot, census v7 sur le catalogue v9. Les seuils sont fixés d'avance, et le port complet de q3/q4 ne s'engage qu'au-delà de ×11 contre 48 fils.
6. **Abandonner** les micro-leviers CPU de q3/q4, D2 et D4 comme générateurs, et D3 comme levier principal. Garder D2 et D4 comme juges hors produit, et le théorème de raccord de D3.

**Probabilités subjectives retenues** (les plus basses défendables parmi celles des jurys) :

| Cible | Probabilité |
| --- | --- |
| K5 sous 1 s, CPU seul | environ 0 |
| K5 sous 1 s, D5 + GPU complet | 0,25 à 0,35, si un débit d'au moins 0,4 à 1 T instructions-modèle/s est mesuré |
| K10 sous 1 s | moins de 0,1 |
| 100 ms, quel que soit K | moins de 0,02 |

## 2. Tableau comparatif des familles

Chaîne G4 à 48 fils, trames de 35,5 à 45,8 k sites, chiffres après corrections adverses. « 000100 » désigne la trame la plus légère, « pire » la plus lourde. Référence R7b : 3,67 à 6,57 s à K5, 9,58 à 15,30 s à K10.

| Famille | Complétude (statut après vérification) | Chaîne K5 (000100 ; pire) | Chaîne K10 (000100 ; pire) | Risque | Coût | Verdict |
| --- | --- | --- | --- | --- | --- | --- |
| D5 tour FULL maigre | Lemmes A, B (avec règle 0), C prouvés ici, non inscrits ; D, E, F en esquisse ; F faux tel qu'écrit, corrigé | ≈ 3,0 s seul (FULL 0,73 → 0,06–0,12 s) | ≈ 6,6–7,0 s seul (FULL 3,20 → 0,25–0,6 s) | faible à moyen | 2–3 semaines (palier 1 corrigé) | **prioritaire**, nécessaire et non suffisant |
| D1 continuité CPU | Énoncé conditionnel (`Q34_PROPRIETAIRE_PASSAGE_AMONT`, `Q4_INDUCTION_ATLAS_EVENEMENTS`) ; 0 écart sur 1 562 nuages adverses | CPU 1,9–2,9 ; 2,8–4,7 s | CPU 6,2–8,2 ; 7,7–11,9 s | moyen | 1–2 mois | **référence exacte** ; porteur des leviers communs |
| D1 route GPU | Idem, plus identités de conservation à spécifier | 0,42–1,10 ; 0,59–1,48 s | 1,4–3,8 ; 1,9–4,8 s | élevé | 4–8 mois | fondue dans l'expérience GPU unique |
| D6 chaîne GPU résidente | Esquisse : L1, L3, L4 prouvés ; sûreté par restriction (L2) à vérifier chemin par chemin | avec D5, bande pessimiste 0,95–1,40 s ; invalide si les ancres longues restent sur CPU (≥ 1,6 s) | avec D5, 3,1–4,5 s (pessimiste) ; bande centrale non ancrée | très élevé | 13–18 semaines, 10–20 sessions | **à sonder**, forme révisée |
| D3 séparation d'échelles | Réduction prouvée, conditionnelle à la complétude arête par arête v9 et à l'ordre global des IDs | 2,8–5,5 s (idéal, local gratuit) | 7,5–12,2 s (idéal) | moyen | L1 3–5 jours, L2 4–8 semaines | **écarté comme levier** ; théorème et mesure gardés |
| D4 inversion par site | Esquisse T1–T4 ; règle de propriété fausse en dégénéré (contre-exemple exact) | ≈ 2,3–2,8 s (voie longue CPU) | ≥ plafond D3 à h = 1 m | élevé | 4–7 semaines | **générateur écarté** ; juge par site hors produit |
| D2 délétion locale | Oracle exact 0 écart (n ≤ 10, Kmax ≤ 5, perturbation entière et non symbolique) ; L3 et perturbation en esquisse | 1,1–2,5 s (confiance faible) | 5,2–14,7 s | très élevé, double réouverture | 3–6 semaines (sous-estimé) | **générateur écarté** ; oracle et fixtures gardés |

Notes des jurys (sur 10) et classement agrégé :

| Famille | Rigueur | Performance G4 | Risque et doctrine | Somme | Rang |
| --- | ---: | ---: | ---: | ---: | ---: |
| D5 | 8 | 8 | 8,5 | 24,5 | 1 |
| D1 | 7 | 5 | 6,5 | 18,5 | 2 |
| D3 | 7,5 | 4,5 | 6 | 18 | 3 |
| D6 | 6,5 | 5,5 | 5 | 17 | 4 |
| D4 | 5,5 | 3,5 | 3,5 | 12,5 | 5 |
| D2 | 6 | 2,5 | 3 | 11,5 | 6 |

Le classement agrégé récompense la rigueur et le faible risque. Il ne dit pas que D1 ou D3 rapprochent du contrat. Classées par proximité du contrat seulement, les familles se rangent ainsi : D5, D6, D1, D3, D4, D2.

## 3. Familles retenues

### 3.1 D5 : tour FULL maigre (prioritaire)

**Principe.** Même objet, mêmes nœuds, mêmes lots, mêmes contributions : seule change la manière d'y arriver. D5 a cinq éléments.

- **Résolution des facettes en trois étages exacts.**
  - (a) Graines : $F=I_{T}\cup U_{T}$.
  - (b) Index des selles : pour une boule régulière $C$, si $S_{C}\subseteq F\subseteq I_{C}\cup S_{C}$, la boule minimale de $F$ est $C$, sans calcul de MEB.
  - (c) Saut au centre : on remplace la facette par les $K$ sites de plus petite puissance dans la boule fermée $D=\mathrm{MEB}(F)$, et on recommence tant que le niveau baisse.
- **Phase A maigre**, sans allocation par bloc, avec un chemin rapide pour les lots singletons (97 à 99 % des lots). Au palier 2, tranches figées de plateaux entiers.
- **Images de naissance directes** (lemme C : l'image d'une naissance à l'ordre $K$ est l'ancre de la même boule à l'ordre $K-1$), et pointeurs de saut pour les fusions.
- **Sortie compacte** : trois tableaux u32 par ordre (`ball`, `next`, `image`), soit 12 octets par nœud, plus une liste d'exceptions pour les coquilles étendues. L'expansion vers le format explicite est une fonction séparée, chronométrée à part.
- **Catalogue scellé** : la chaîne ne revalide pas le catalogue qu'elle vient de produire.

**Théorème et statut.**

- **Lemme A** (index des selles) : preuve complète en trois lignes. Il reste vrai sans régularité ; la régularité ne sert qu'à indexer.
- **Lemme B** (témoin central) : le centre de $D$ est un point commun de $C_{F}(r)$ et $C_{G}(r)$ pour tout $r\geq r_{D}$, donc $F$ et $G$ sont dans la même composante de $L_{K}(r)$.
  - **Correction obligatoire** (vérification adverse, fixture `fx_cz`) : la règle de descente telle qu'écrite refuse là où le produit réussit.
  - Il faut une *règle 0*, appliquée à chaque état, y compris après un saut : si la clé de $D$ est au catalogue et que $p_{D}+q_{\min}-1\leq K\leq p_{D}+u_{D}$, la cible est $D$.
  - Avec cette règle, on démontre que le repli par échange d'intrus n'est jamais atteint sur un catalogue complet. La terminaison devient une descente stricte de niveau, et le repli devient un refus d'invariant, donc un détecteur d'omission.
- **Lemme C** (image d'une naissance) : prouvé et confronté au code (`order_images`).
- **Propositions D** (image d'une fusion par une feuille) **et E** (tranches figées) : esquisses.
- **Proposition F** (bijection du format compact) : **fausse telle qu'écrite**.
  - Dans un lot, une continuation contributive prend le rang du premier bloc de son groupe, et ce bloc peut être muet.
  - Fixtures `lat5_3` (K8) et `lat5_1` (K9) : la contribution d'une continuation y précède des naissances du même niveau. Une expansion qui range chaque contribution à la position de sa boule change alors l'ordre et les identifiants de population.
  - Correction : chaque exception porte la position de programme du premier bloc de son groupe (4 octets).
- **Défaut d'implémentation mesuré au palier 1.**
  - Le chemin des lots singletons range les racines dans un tampon de 13 cases. Or un bloc dont la coquille compte 12 sites peut avoir 32 racines distinctes (fixture `fx_ico12` : fusion à 32 parents dans le produit).
  - Il y a donc écriture hors bornes, et la sonde ne l'a pas détectée.
  - Correction : passer au chemin général à offsets CSR au-delà d'un petit tampon.
- **Statut** : `proved_here` proposé pour A, B (avec règle 0) et C ; rien n'est inscrit au registre. Comme le produit, D5 reste relatif à un catalogue complet.

**Vérifications faites.**
- Racines pré-lot identiques au produit sur 5,04 M facettes LiDAR (08/000100, 8k/K10, 8k/K5, 16k/K5).
- Même résultat sur 221 556 facettes de douze nuages très dégénérés (grilles 5³ et 6³, grille plane, carrés cocirculaires) : 0 repli, 0 naissance à remonter, nœuds et fusions égaux au produit à chaque $K$.
- MEB divisés par 5,3 à 5,7 ; pas maximal de descente ramené de 159 à 10.

**Estimation corrigée.**

- **Index et jointure.** Le coût de l'index (a)+(b) et de la jointure de toutes les facettes n'était pas chronométré. Mesuré avec des tables plates, il vaut 0,79 à 1,02 s nets à 8k/K10. Le rapport produit/D5 tombe donc à ×2,5–2,6, au lieu de ×2,5–3,5 ; le coût par facette croît avec la table, de 68 à 325 ns entre K2 et K10.
- **Conversion.** Un fil local vaut au mieux 1/42 à 1/59 de 48 fils G4, pas 1/58 à 1/70.
- **Phase A.** 124 ns/bloc sur le même catalogue (87 ns était le meilleur tirage). Sur trame entière, les défauts de cache peuvent la porter à 0,2–0,37 s à K10.
- **Sortie.** Le format compact réutilise le catalogue comme banque, soit 224 o/boule et 0,98 Go sur la trame. Le gain « ÷15 » est donc retiré. En réencodant le catalogue (environ 56 o/boule), on obtient environ 0,32 Go au total, soit ÷3,7.
- **Index.** 7,1 M entrées à K10 sur la trame ; 0,43 à 0,9 Go si les dix ordres coexistent.

| Poste FULL | Aujourd'hui (R7b) | D5 palier 1 corrigé | D5 palier 2 |
| --- | ---: | ---: | ---: |
| K5, 000100 | 0,73 s | 0,06–0,12 s | 0,03–0,06 s (non mesuré) |
| K10, 000100 | 3,20 s | 0,25–0,5 s | ≥ 0,15–0,3 s tant que (a)+(b) reste sur CPU |
| K10, 000000 et 000200 | 3,8–4,0 s | 0,3–0,6 s | idem |

**Réserve de synthèse.** Ces chiffres supposent que la queue FULL (passe 2 de validation, populations, banque, encodage) disparaisse réellement. Or le jury de rigueur exige de garder la construction des `ShellTable` des coquilles étendues et le contrôle `full_ball_census_geometry`. Chaque élément conservé doit être chronométré : le poste FULL ne descendra pas sous la part de la queue qui subsiste, qui peut aller jusqu'à environ 1,6 s à K10 sur G4.

**Expérience de falsification (E1, locale, sans GCP).** Champ : les neuf coupes emboîtées (s00, s01, s02 × 8k, 16k, 32k), à K5 et K10, plus les douze nuages dégénérés et les fixtures `fx_cz`, `lat5_3`, `lat5_1`, `fx_ico12`. Trois exigences :

- égalité octet par octet de l'expansion avec la tour produit (`same_payload`, `tower_digest`), statuts compris ;
- une porte par facette qui ne s'appuie pas sur la composante : le niveau de $\mathrm{MEB}(F)$ est inférieur à $\lambda$, la boule terminale figure au catalogue avec la fenêtre $K$ admise, le niveau de la cible est inférieur à $\lambda$, la racine est prise à $\lambda^{-}$. Les deux mutants v7 de clé partielle de semis doivent être tués ;
- des relevés par $K$, **y compris l'ordre $K_{\max}$**, où aucun census n'est lu dans le catalogue (les nœuds n'y sont divisés que par 2,06). Index et énumération des facettes chronométrés ; phases FULL ventilées par la sonde v13 (`tower_phases_ms`, `c768e06a`).

**Critères d'arrêt.**
- Tout écart dans les deux premières exigences devient une fixture et arrête la branche.
- À 32k/K10, le saut est rejeté dans trois cas : travail de résolution pondéré réduit de moins de ×2 ; pas maximal qui double à chaque doublement de $n$ ; plus de 2 sauts par facette non hachée.
- Phase A au-delà de 250 ns/bloc : les tranches figées deviennent obligatoires.

**E2 : une session G4 CPU seule, en appariement.** Trames 08/000000, 000100 et 000200, K5 et K10, deux répétitions entrelacées. Objectif : FULL K10 ≤ 0,5 s. Arrêt si FULL K10 dépasse 0,6 s, ou si la phase A de K10 dépasse la moitié du poste FULL ; dans ce dernier cas, tranches figées avant tout GPU.

### 3.2 D1 : continuité de la chaîne v9 (référence exacte)

**Principe.** Garder l'objet, le catalogue et les prédicats, et porter les leviers déjà identifiés :
- L1 : certificat de bloc avant expansion ;
- L2 : cœur compté par nœuds ;
- L3 : découpage à l'intérieur d'une arête ;
- L5 : phase A parallèle ;
- L6 : MEB proposé par Welzl ;
- L7 : q2, tri et recensement sur GPU ;
- L8 : plomberie (pool persistant, arènes, pages géantes, digest hors appel, sortie compacte).

**Théorème et statut.** L'énoncé visé : l'ensemble des clés émises égale le catalogue admissible. Statut réel : **énoncé de complétude conditionnel**, pas un théorème.
- q2 est prouvé dans les notes v8, mais non inscrit au registre.
- Le passage amont des arêtes propriétaires est une « preuve conditionnelle de programme ».
- L'induction de l'atlas q4 est en revue.

La vérification adverse n'a trouvé aucun contre-exemple :
- 1 562 nuages adverses : grilles, lignes de balayage colinéaires, plans, sphères entières, fenêtres LiDAR réelles, images 18 bits pleines ;
- 775 566 boules comparées clé par clé à des oracles exhaustifs exacts : 0 manquante, 0 en trop ;
- refus corrects pour les coquilles de plus de 12 sites.

Elle a aussi revérifié le critère d'admission. Pour $m\leq q_{\min}-2$, le lien local est connexe (nerf à graphe de Johnson connexe, lemme de Gordan). Donc $p+q_{\min}\leq K_{\max}+1$ est exactement la condition nécessaire et suffisante.

**Deux affirmations de D1 sont à retirer.**
- « D1 ne change rien à l'énoncé » est faux pour ses propres leviers. L1 et L2 sont de nouveaux certificats d'élagage, avec leurs obligations :
  - ne créditer que des sites distincts et strictement intérieurs ;
  - ne pas additionner de crédits entre cellules ;
  - ne pas exclure par les coins. Les 32 coins majorent la forme sans la minorer. Exemple : avec $a=(0,0,0)$, $b=(4,0,0)$ et le carré $[0,4]^{2}\times\lbrace 0\rbrace$, la forme est positive aux coins et vaut −16 en $(2,0,0)$.
- « Le GPU ne change que l'ordonnancement » est vrai pour l'objet, faux pour les compteurs de politique. De plus, une vague qui perd une tâche passe inaperçue de `complete_relative` et d'Euler.

**Estimation corrigée.** La vérification adverse a réfuté la projection de D1 sur trois points :
- la queue FULL est oubliée : environ 1,6 s à K10/000100 et 0,37 s à K5/000100, qu'aucun des leviers L5, L6 ou L8 ne vise ;
- l'occupation de L3 est comptée en fils logiques, alors qu'un fil SMT supplémentaire rapporte environ 0,26 cœur ;
- la plomberie hôte–GPU est chiffrée sans mécanisme. En v6 et v7, les noyaux ne pesaient que 2 à 4 % de leur étage.

| Cas | CPU seul, D1 | CPU seul, corrigé | GPU complet, D1 | GPU complet, corrigé (hors plomberie hôte) |
| --- | ---: | ---: | ---: | ---: |
| K5/000100 | 1,9–2,4 s | 1,9–2,9 s | 0,22–0,84 s | 0,42–1,10 s |
| K5/000200 | 2,8–3,5 s | 2,8–4,7 s | 0,31–1,13 s | 0,59–1,48 s |
| K10/000100 | 6,0–7,5 s | 6,2–8,2 s | 0,63–2,47 s | 1,4–3,8 s |
| K10/000200 | 8,2–10,1 s | 7,7–11,9 s | 0,86–3,32 s | 1,9–4,8 s |

Les micro-leviers CPU de q3/q4 ont des rendements décroissants mesurés : de ÷2 à 3 en R3, on tombe à −2 à −4 % de chaîne en R7b. Les ombres avant expansion (palette, octant, DFS par ligne, scission) ne dépassent pas −6 % de CPU q3/q4.

**Falsification et arrêt.** Étape 0, locale : relever le CPU par fil de chaque poste, la plus longue arête indivisible, la phase A par ordre et les sections sérielles de FULL. Si ce que L3 et D5 ne peuvent pas découper dépasse 0,5 s en équivalent G4 à K5, l'objectif 1 s à K5 est abandonné pour D1. L'étape 1 GPU est fondue dans l'expérience de D6 (§ 3.3).

**Rôle retenu.** Référence exacte et juge permanent. À K10, l'égalité clé par clé avec la route CPU est la seule garantie globale des ordres 9 et 10 ; le juge par fenêtre du § 4 n'en donne qu'un échantillon. Les leviers L6 et L8 sont à porter tout de suite. L5 passe par la voie D5 (tranches figées), plus sûre que le lemme du maximum d'identifiant, validé seulement sur 3 000 historiques abstraits et limité à la voie statique.

### 3.3 D6 : chaîne GPU résidente (à sonder, forme révisée)

**Principe.** Une architecture d'exécution, pas un nouvel algorithme. Index, q3/q4, fusion, census et cibles statiques FULL restent sur la RTX PRO 6000 jusqu'au catalogue trié. Règles d'exécution :
- files de travail device et graphe CUDA ;
- aucune boucle hôte par tuile ou par arête ;
- jamais de cover copié ;
- pas de FP64 sur le chemin chaud.

Ces règles viennent des échecs mesurés : frontière pilotée par l'hôte à 34 M visites/s ; census v6 en 154 ms de noyaux pour 7,7 s d'étage ; census v7 en 189 ms pour 4,54 s.

**Théorème et statut.**
- **Prouvés** :
  - L1, localité de l'arête propriétaire : tout site intérieur ou de coquille est à moins de $0{,}966\,l$ du milieu de l'arête, de longueur $l$ ;
  - L3, partage exact, par comparaison entière de la longueur d'arête et par le plancher entier du milieu ;
  - L4, support unique : si la coquille a exactement $q_{\min}$ sites, l'arité et les identifiants suffisent à identifier la boule.
- **À démontrer** : L2, sûreté par restriction (un sous-ensemble de témoins ne peut qu'ôter des crédits). Vrai pour les certificats à comptage ; reste à prouver pour les classifications géométriques de l'atlas et pour le census q3 `GlobalBoxes`.
- **Piège** : les sous-cellules de centres doivent être semi-ouvertes. Une double fermeture est bruyante (`chain_duplicate_presentation`) ; une double ouverture est une omission silencieuse.
- Statut global : esquisse.

**Estimation.** Le modèle d'instructions entières est calibré sur 60 cas locaux (±30 %). q3/q4 vaut 158 à 237 G instructions-modèle à K5 et 497 à 752 G à K10. Trois hypothèses de débit :
- pessimiste, 0,4 T/s : le meilleur noyau mesuré dans le dépôt, un census régulier ;
- centrale, 1,5 T/s : quatre fois mieux, sans ancrage ;
- optimiste, 4 T/s.

**Correction de synthèse, décisive.** D6 supposait que la passe globale CPU des arêtes plus longues que $L_{0}$ (1 à 2 m) pèse au plus 10 % du travail. D3 mesure 31 à 68 % sur les coupes 8k et 66 à 79 % sur trame entière. Laissée sur CPU, cette passe vaut environ 1,6 s à K5/000100 : la forme « tuiles GPU + passe longue CPU » ne peut pas donner 1 s.

La forme retenue inverse donc le placement : **le rejet des ancres longues passe lui aussi sur le GPU**, en lot. C'est une requête point-dans-boule à arrêt anticipé, du même type que le census, le seul noyau dont le dépôt ait mesuré un débit. Pour la même raison, q2 doit passer sur GPU à K10 : il pèse 0,41 à 0,82 s et ne gagne que ×1,07 à ×1,28 de 24 à 48 fils.

**Expérience de falsification unique (qui regroupe l'étape 1 de D1 et l'E1 de D6).**

Préalable local, E0 : mesurer le chemin critique séquentiel du calendrier FULL. Si l'ordre $K_{\max}$ dépasse 0,6 s sur G4 à K10, D6 passe après D5 pour K10.

Puis une session G4 gardée, au plus 1 h de GPU, avec l'option CUDA à OFF par défaut et un stub hôte au bit près :

- (a) noyau « cœur diamétral + certificat de voie morte » sur les 3 673 260 arêtes de 000100/K10 (1 732 176 à K5), un warp par arête, frontières en masques de bits, comptes saturants par `popc` ;
- (b) rejet des ancres de plus de 1,6 m en lot, sur les mêmes trames ;
- (c) census v7 sur le catalogue v9 de 000100/K10 (4,38 M boules, 392 M visites ; 20 à 60 ms attendus), pour calibrer le débit sur LiDAR.

Exigences :
- égalité décision par décision avec le CPU, avant toute lecture de temps ;
- mesure du même lot sur CPU à 48 fils, dans la même session ;
- publication des registres, de l'occupation, des octets transférés et du nombre de lancements.

Critères, sur l'accélération $A$ = temps CPU W48 du lot / temps mural de l'étage GPU, transferts compris :

| Mesure | Décision |
| --- | --- |
| $A\geq 11$, et temps mural au plus double de celui des noyaux | port complet autorisé ; K10 reste une ambition |
| $3\leq A<11$ | port pour K5 seulement, et seulement si (b) donne aussi $A\geq 3$ |
| $A<3$, ou temps mural supérieur au double des noyaux, ou désaccord inexpliqué | abandon du port q3/q4 ; le GPU se limite au census et aux cibles statiques FULL, si (c) le justifie |

**Coût.** 1 à 2 semaines et une session pour l'expérience ; 13 à 18 semaines et 10 à 20 sessions pour le port complet. Pour comparaison, la v5 a consommé 13 sessions pour deux voies, sans gain net.

### 3.4 D3 : séparation d'échelles (théorème conservé, levier écarté)

**Principe.** Couper le catalogue selon le rayon :
- les petites boules ($r\leq h$) sont énumérées par tuiles indépendantes, avec un halo de $2h$ ;
- les grandes boules sont confiées au générateur global, restreint aux ancres longues.

Deux lemmes. Lemme A : la plus longue arête d'un support centré vérifie $\vert ab\vert^{2}\geq 2qr^{2}/(q-1)$, soit au moins $8r^{2}/3$. Lemme B : si $r\leq h$, tout site de la boule fermée est à moins de $2h$ de chaque extrémité de l'ancre.

**Théorème et statut.** La réduction est prouvée, sous condition de la complétude arête par arête de la v9, et n'est pas inscrite. La vérification adverse (oracle i128 indépendant sur 08/000000 8k) a corrigé trois points de la preuve écrite :

- **Propriété et égalités.** L'étape « la propriété ne lit que $P\cap\bar{B}$ » est fausse dès que la plus longue arête est ex aequo, car la v9 départage alors par la paire d'identifiants.
  - Sur 183 287 présentations q3 de rayon au plus 0,5 m, 16 ont une plus longue arête ex aequo, dont 12 avec des extrémités propriétaires distinctes.
  - Une renumérotation locale des tuiles produit alors soit un doublon, soit une perte.
  - Prémisse obligatoire : tout départage lit l'**ordre global** des identifiants.
- **Doublons.** « La fusion absorbe un doublon » est faux : une présentation identique émise deux fois déclenche `chain_duplicate_presentation`.
- **Halo.** « Une erreur de halo ferait échouer la chaîne » est faux pour une clé manquante, car le recensement ne contrôle que les clés émises.

La fixture « ancre de carré exactement $8h^{2}/3$ avec $r=h$ » est irréalisable dans $\mathbb{Z}^{3}$ : l'égalité exige un simplexe régulier. Il faut la remplacer par trois fixtures :
- $r=h$ exact en q2 ;
- $r=h$ exact en q3 non régulier, sur un cercle de réseau ;
- $\vert ab\vert^{2}=8h^{2}/3$ avec $3$ divisant $h$ (on a alors $r<h$).

**Estimation corrigée.**
- Mesure directe sur la trame entière 08/000200 K5, avec des compteurs égaux au reçu et à R7b : part globale 79,0 / 70,1 / 56,9 % pour h = 0,5 / 1 / 2 m, contre 53,6 / 36,6 / 25,3 % à 8k. L'écart va de +47 à +92 % : la barre d'erreur de ±25 % annoncée est réfutée.
- Chronométrer chaque arête avec `CLOCK_THREAD_CPUTIME_ID` coûte environ 1 µs d'appel système par fenêtre. Le coût réel d'une arête longue est de 1,8 à 4,3 µs.
- Chaîne D3 idéale (partie locale gratuite et recouverte) : K5 2,8 à 5,5 s, K10 7,5 à 12,2 s, soit un gain de ×1,2 à ×1,5.

**Falsification.** Elle est faite, et elle écarte D3 comme levier principal. Critère fixé d'avance : part globale supérieure à 50 % à h = 0,5 m sur au moins deux scènes. Résultat : dépassé sur 7 coupes sur 7 et sur la trame entière.

D3 ne se rouvre que si un certificat de bloc ramène la part des ancres de plus de 1,6 m sous 25 % du CPU q3/q4 sur les trois trames entières.

Ce qui survit :
- le théorème de raccord ;
- l'arithmétique étroite des petites boules : coordonnées locales sous $2^{11}$ pour h = 0,5 m, puissances sous $2^{52}$, exactes en i64 ;
- surtout, la **sonde d'attribution**, qui sert de boussole pour toute la suite.

### 3.5 D4 et D2 : générateurs écartés, juges conservés

**D4 (inversion par site).**
- **Principe.** Pour un site $a$, le nombre de sites strictement intérieurs à une boule passant par $a$ et centrée en $c$ est le niveau de $c$ dans l'arrangement des plans bissecteurs. Le catalogue q3/q4 incident à $a$ se trouve dans le $\leq(K-2)$-niveau. On le parcourt par BFS sur les droites, découpé par une boîte de demi-largeur $h$. Sans découpe, le « ciel » LiDAR rend le niveau non borné (17,5 et 96 ms par site).
- **Théorème T1–T4** (connexité du 1-squelette de niveau borné, certificat de préfixe $2R\leq\rho$) : esquisse jugée juste par le jury de rigueur, non inscrite.
- **Faille exacte.** La règle « le propriétaire est le plus petit identifiant de la coquille » omet des boules. Contre-exemple exécuté :
  - coquille $a=(0,0,5)$, $x=(5,0,0)$, $y=(-3,4,0)$, $z=(-3,-4,0)$, centre $0$, $R=5$, $q_{\min}=3$ ;
  - la seule présentation centrée est $\lbrace x,y,z\rbrace$ ;
  - si $a$ porte l'identifiant minimal, personne n'émet la boule, et Euler ne la voit pas si $p=K_{\max}-2$.
- **Coût.** Pas de gain CPU : 0,66 à 3,6 fois la v9 sur q3/q4, arêtes de face et filtres compris. Le débit GPU supposé est 4 à 15 fois au-dessus de ce que le dépôt a calibré.
- **E0 déjà tranché par les données de D3.** Le travail des ancres de plus de 1,633 m pèse au moins 48,6 et 56,3 % à K10 sur deux trames entières (minorants du modèle ; 70,1 % mesurés à K5 sur 000200). C'est au-delà du seuil d'abandon de 35 %. La mesure directe de l'étape 3 de la feuille de route doit le confirmer.
- **Rôle retenu** : juge d'échantillon par site, exact, hors produit. Éventuellement, noyau à court rayon si les ancres longues sont un jour résolues ailleurs.

**D2 (délétion locale, pavage rhomboïdal).**
- **Principe.** Une boule critique d'intérieur $J$ est la boule minimale d'une face d'un tétraèdre de la région de conflit de $\mathrm{Del}(P\setminus J)$.
- **Vérification.** L'oracle rationnel indépendant donne 0 boule manquante et 0 en trop sur 36 nuages et 2 157 boules, dont 398 dégénérées. Il a aussi trouvé deux pièges utiles à tous les générateurs :
  - la lecture littérale « la boule apparaît parmi les nouveaux simplexes » est fausse dans 64 % des cas q2/q3 ;
  - un pré-filtre par arité présentée peut manquer une boule à $p=K_{\max}-1$ (quadrilatère cocirculaire de diamètre AC, perturbation qui choisit BD).
- **Limites.** n ≤ 10, Kmax ≤ 5, perturbation entière et non symbolique.
- **Doctrine.** Double réouverture : mosaïque d'ordre supérieur, triangulation ordinaire.
- **Coût.** Plus de prédicats par boule que la v9 : 1 500 à 2 000 contre environ 560.
- **Rôle retenu** : oracle borné et générateur de fixtures, hors produit.

## 4. Portes communes à tout nouveau générateur

Ces règles viennent des failles trouvées par les jurys et les vérifications adverses. Elles s'appliquent à D1-GPU, D3, D4, D6 et à tout générateur futur.

1. **Propriété par le support, dans l'ordre global.** Le propriétaire d'une présentation se calcule sur son support, jamais sur la coquille, avec les identifiants globaux. Aucune renumérotation locale (tuile, Morton relatif) ne doit servir aux départages.
2. **Au moins une présentation d'arité $q_{\min}$ par clé**, sans pré-filtre par l'arité présentée. La garde `chain_qmin_differs_from_min_presented_arity` ne détecte un $q_{\min}$ manquant que si une autre présentation de la même clé est émise.
3. **Disjonction exacte.** Un doublon de présentation est fatal ; une clé absente est silencieuse. Aucune preuve ne peut s'appuyer sur « la fusion absorbe ».
4. **Juges d'échelle.**
   - Égalité clé par clé avec la route CPU, sur les coupes 8k/16k/32k et les trames entières, et aussi sur le flux complet des présentations (clé, support, profondeur, coquille).
   - Euler pour $K\leq K_{\max}-2$.
   - Protocole Kmax+2 obligatoire à K5 (catalogue K7 restreint).
   - À K10, un **juge d'échantillon par fenêtre autonome**.
     - Énoncé : soient une fenêtre $W$ et $W'=P\cap(W\oplus\bar{B}(0,R))$. Toute boule de rayon au plus $R/2$ qui a un site de support dans $W$ a la même boule fermée dans $W'$ et dans $P$, donc le même triplet $(I,U,q_{\min})$. Les deux catalogues restreints sont égaux.
     - Mise en œuvre : un énumérateur exact indépendant, l'oracle i128 exhaustif pour $\vert W'\vert\leq 150$ environ, D4 exact par site au-delà. Il couvre les ordres $K_{\max}-1$ et $K_{\max}$, là où Euler est muet.
     - Il couvre aussi l'angle mort des compensations tronquées. La contribution générique d'une boule est $x^{p+1}(x-1)^{q-1}$ ; si l'on omet une q2 de profondeur $p$ et une q3 à chaque profondeur de $p$ à $p+T$, $E_{K}$ reste intact jusqu'à $K=p+T+1$.
5. **Identités de conservation bloquantes**, par vague et par tuile : entrées = rejetées + émettrices + vides + réenfilées. Elles sont publiées dans le reçu.
6. **Fixtures permanentes :**
   - le contre-exemple de propriété de D4, complété de sites intérieurs jusqu'à $p=K_{\max}-2$, avec le mutant « minimum de coquille » ;
   - un triangle isocèle synthétique dont la plus longue arête est ex aequo, extrémités propriétaires dans deux tuiles, avec le mutant « renumérotation locale » ;
   - le quadrilatère AC/BD à $p=K_{\max}-1$ ;
   - `fx_cz`, `lat5_3`, `lat5_1`, `fx_ico12` ;
   - « deux murs » : un support à cheval sur plusieurs tuiles, avec une arête plus longue que $L_{0}$.

   Aucune fixture ne reprend de coordonnées dérivées de SemanticKITTI.
7. **Registre.** Le lemme A de D5 et le L4 de D6 énoncent le même fait : une présentation régulière identifie sa boule par ses identifiants. Une seule entrée suffit dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`. Toute contradiction devient une fixture minimale et met ce registre à jour avant de continuer.

## 5. Feuille de route ordonnée pour le développeur

Chaque étape a une porte d'entrée, un livrable et un critère d'arrêt. Les étapes 1, 2 et 3 peuvent avancer en parallèle. L'étape 5 attend que les étapes 1 et 3 soient faites et que le juge de l'étape 0 soit en place.

**Étape 0 — portes communes (quelques jours, en local).**
- Contenu : le § 4, c'est-à-dire le juge par fenêtre autonome, le protocole Kmax+2 à K5 comme porte, les fixtures listées et les identités de conservation dans le schéma de sonde.
- Critère : chaque mutant nommé est tué, et le juge par fenêtre donne 0 écart sur 10 fenêtres × 6 coupes × 2 ordres. Tout écart devient une fixture et arrête l'étape.

**Étape 1 — D5 palier 1 corrigé (2 à 3 semaines, en local).**
- Contenu : règle 0 ; format compact avec la position du premier bloc de groupe ; tampon de racines sans borne fixe ; images par le lemme C et pointeurs de saut ; index plat avec préchargement ; queue FULL chiffrée poste par poste.
- Porte : E1 du § 3.1, ASan sur le chemin des lots singletons, TSan pour les tranches, égalité W1/W48.
- Décisions de contrat, à faire acter explicitement par le développeur **et par l'utilisateur** :
  - la sortie compacte, qui s'écarte de l'hypothèse 2 de la synthèse v9 § 9 ;
  - le digest hors chronomètre ;
  - l'expansion chronométrée à part.

**Étape 2 — plomberie L6/L8 (1 à 2 semaines, en local, en parallèle).**
- Contenu :
  - un pool persistant à la place des threads créés par région et des attentes `yield` (9,5 à 15,5 CPU·s de temps système à K5) ;
  - des arènes et des pages géantes (1,75 à 2,2 M défauts mineurs à K10) ;
  - un digest optionnel ou asynchrone (0,80 à 1,01 s à K10).
- Portes existantes : `same_payload`, T2, mutants, W1/W48.

**Étape 3 — mesure des ancres longues (1 à 2 jours, en local, compteurs déterministes).**
- Contenu : sonde d'attribution par longueur d'ancre sur les trois trames entières, K5 et K10. Chronométrage par lot (rdtsc) et non par arête ; front WSPD compté à part ; phase q2 attribuée elle aussi.
- Livrable : les parts par seuil (0,8 / 1,6 / 3,2 m) et les exposants de croissance de 8k à 32k puis à la trame.
- Portée : cette mesure tranche l'E0 de D4 et fournit la variable qui manque à D6.

**Étape 4 — certificat de bloc des ancres longues (famille B ; 2 à 4 semaines, en local).**
- Cible : les rectangles WSPD d'écart supérieur à 1,6 m, dont 92 à 97 % des paires développées sont aujourd'hui rejetées une à une par les témoins.
- Obligations d'exactitude : ne créditer que des sites distincts et strictement intérieurs, ne pas additionner entre cellules, ne pas exclure par les coins.
- Succès : la part des ancres de plus de 1,6 m passe sous 25 % du CPU q3/q4 sur les trois trames. Cela rouvrirait D3 et simplifierait D6.
- Arrêt (critère proposé par cette synthèse) : gain inférieur à 20 % du CPU q3/q4 sur trame entière après deux itérations. Ce serait rejoindre la série des ombres déjà mesurées (palette −6 %, octant, DFS par ligne, scission).

**Étape 5 — session G4 GPU unique (1 à 2 semaines de noyau, une session).**
- Protocole et seuils du § 3.3, par les seuls scripts gardés, avec certification `TERMINATED`.
- Base de comparaison de $A$ : la chaîne CPU **après** l'ordonnancement q3/q4 en cours chez le développeur, au même ledger de travail (voir « Raccord avec R8 »).
- Aucun noyau n'entre dans le build produit sans reçu G4 de gain.

**Étape 6 — conditionnelle, selon $A$.**
- Si $A\geq 11$ : port complet sur GPU de q3/q4 (ancres longues comprises), de q2, de la fusion et du recensement. Il s'accompagne :
  - des identités de conservation ;
  - de la comparaison du flux complet des présentations ;
  - de bornes du repère local redémontrées en fonction de l'étendue locale (`static_assert`, routage hors domaine, jamais de troncature).
- Si $3\leq A<11$ : port pour K5 seulement.
- Sinon : le livrable est la chaîne CPU avec D5, L8 et le résultat de l'étape 4. Le contrat de 1 s est déclaré hors de portée sur cette architecture, reçu à l'appui.

## 6. Ce qu'il ne faut pas faire

- **Porter q3/q4 sur GPU avant l'expérience unique** du § 3.3, ou le porter avec des vagues pilotées par l'hôte, des covers copiés ou une reconstruction hôte mono-fil : c'est exactement le profil des cinq échecs du dépôt.
- **Laisser la passe des ancres longues sur CPU** dans une architecture GPU : cela suffit à interdire 1 s à K5.
- **Continuer les micro-leviers CPU de q3/q4** sans cible nouvelle : les rendements mesurés sont tombés à −2 à −4 % de chaîne.
- **Projeter avec CPU·s/48** ou avec 44 fils logiques. Utiliser les facteurs mesurés de 24 à 48 fils :
  - q3/q4 ×1,22 à ×1,45 ;
  - tour ×1,02 à ×1,12 ;
  - q2 ×1,07 à ×1,28 ;
  - recensement ×1,16 à ×1,21.
- **Transposer à une trame entière une part mesurée à 8k** (erreur de +47 à +92 % sur D3), ou **chronométrer par arête** avec une horloge qui coûte un appel système.
- **Définir le propriétaire par la coquille**, renuméroter localement sans l'ordre global, ou pré-filtrer par l'arité présentée.
- **Sceller le catalogue** en supprimant la construction des `ShellTable` ou sans juge d'échantillon. **Comparer des cibles statiques ou des compteurs** entre le produit et D5, au lieu de comparer des racines et de passer la porte par facette.
- **Déclarer le catalogue complet sur la seule foi d'Euler** : il est aveugle aux ordres $K_{\max}-1$ et $K_{\max}$ et à certaines omissions compensées.
- **Rouvrir la mosaïque de Delaunay d'ordre supérieur** (D2 ou D4 comme générateurs) sans appliquer toute la règle de réouverture. En particulier : jamais de dédoublonnage global des droites ou des sommets de D4, et jamais un rayon ou une fenêtre fixe comme garantie de complétude.
- **Décider seul** de la sortie compacte ou du digest hors chronomètre : ce sont des décisions de contrat.
- **Sur GPU**, éviter :
  - `--use_fast_math` ;
  - la contraction FMA par défaut (compiler avec `-fmad=false` ou des intrinsèques `_rn`) ;
  - le FTZ ;
  - le FP64 sur le chemin chaud (1/64 du débit FP32 sur GB202) ;
  - les bornes entières reprises de la v7, dont le test de plateau i128 donnait de vraies réponses fausses à 18 bits.
- **Descendre sous s = 8**, versionner des données dérivées de SemanticKITTI, ou tirer une conclusion d'échelle de quelques centaines de points.

## 7. Planchers physiques

### 7.1 Amdahl par poste (R7b, 48 fils, hors digest)

| Poste | K5/000100 | K10/000100 | K10/000200 |
| --- | ---: | ---: | ---: |
| q3/q4 | 2,52 s | 5,23 s | 9,84 s |
| FULL | 0,73 s | 3,20 s | 3,82 s |
| q2 | 0,24 s | 0,41 s | 0,82 s |
| fusion + recensement | 0,12 s | 0,46 s | 0,56 s |
| reste | 0,06 s | 0,28 s | 0,26 s |
| **total** | **3,67 s** | **9,58 s** | **15,30 s** |
| total si q3/q4 = 0 | 1,15 s | 4,35 s | 5,46 s |
| total si q3/q4 = 0 et FULL = 0 | 0,42 s | 1,15 s | 1,64 s |
| digest synchrone (hors total) | 0,16 s | 0,80 s | 1,01 s |

**Lecture.**
- À K10, même avec q3/q4 et FULL gratuits, q2, fusion, recensement et plomberie dépassent déjà 1 s. Atteindre 1 s à K10 exige donc de réduire **chaque** poste, y compris ceux que le SMT n'accélère presque pas.
- Les étages sont en série : le catalogue doit être complet avant FULL, parce que le générateur émet dans l'ordre spatial et non par niveau. Aucun recouvrement n'est possible sans un générateur qui émette par niveau, et personne n'en a proposé.

Pour situer l'étape 4 (hypothèse non démontrée) : si un certificat de bloc retirait 80 % du coût des ancres longues, q3/q4 de K5/000100 passerait d'environ 2,52 à 1,2 s. La chaîne CPU avec D5 et L8 resterait alors vers 1,6 à 1,8 s à K5 et vers 4 s à K10. Le CPU seul n'atteint 1 s dans aucun scénario.

### 7.2 Débit GPU exigé

On prend le modèle d'instructions de D6 (q3/q4 : 158 à 237 G instructions-modèle à K5, 497 à 752 G à K10) et 0,4 T/s pour le meilleur noyau déjà mesuré dans le dépôt :

| Budget q3/q4 | Débit exigé à K5 | Débit exigé à K10 | Rapport au meilleur noyau mesuré |
| --- | ---: | ---: | ---: |
| 0,5 s (cible 1 s) | 0,3–0,5 T/s | 1,0–1,5 T/s | ×1 à ×4 |
| 50 ms (cible 100 ms) | 3,2–4,7 T/s | 10–15 T/s | ×8 à ×38 |

La crête entière de la carte est d'environ 60 T/s. Tenir 10 à 15 T/s d'entiers exacts sur des parcours d'arbre divergents à arrêt anticipé représenterait 17 à 25 % de la crête, sans précédent dans le dépôt. D'où le verdict pour 100 ms à K10 : il faut d'abord réduire le travail d'un ordre de grandeur, ce que ne fournissent ni D1 à D6 ni le certificat de bloc espéré.

### 7.3 Sortie, transferts et mémoire (K10, trame 000100)

| Élément | Taille | Coût plancher |
| --- | ---: | --- |
| Tour explicite actuelle | 770–968 Mio | écriture 4–10 ms à 100–200 Go/s ; environ 30 ms à 28 Go/s (débit v7, sans reçu) |
| Catalogue (224 o/boule, 4,38 M boules) | 0,98 Go | même ordre d'écriture ; pour l'ensemble tour + catalogue, environ 290 000 défauts de page (0,15–0,3 CPU·s) et 20–25 M allocations (0,5–1,2 CPU·s) |
| Tour compacte (12 o/nœud, 5,95 M nœuds) | 71,4 Mo | ≤ 1–3 ms |
| Catalogue réencodé (environ 56 o/boule) | environ 0,25 Go | 2–9 ms |
| Retour device → hôte, format compact (environ 0,32 Go) | — | 6–7 ms en mémoire épinglée (45–50 Go/s) ; environ 23 ms sinon (14 Go/s) |
| Retour device → hôte, format explicite (environ 1,9 Go) | — | 38–42 ms en mémoire épinglée ; environ 135 ms sinon |
| Digest synchrone | — | 0,80–1,01 s : à sortir du chronomètre |

**Conséquences.**
- À 100 ms, la sortie explicite et le digest consomment à eux seuls tout le budget. Le format compact et un digest hors appel y sont **nécessaires**, et ce sont des décisions de contrat.
- À 1 s, ils restent utiles : ils représentent 0,1 à 0,25 s sur le chemin séquentiel de K10.
- Mémoire : le RSS actuel est de 3,8 à 5,2 Gio à K10. Tout ce qui est proposé tient largement dans les 180 Gio de l'hôte et les 96 Go du GPU.

### 7.4 Échelle du contrat

Les mesures ne couvrent que trois trames d'une seule séquence (08), de 35,5 à 45,8 k sites. À 60 k sites, le travail croît plus vite que $n$ :
- chaîne ×1,7 à ×2,8 par doublement ;
- `core_sites` jusqu'à $n^{3{,}05}$ ;
- paires développées jusqu'à $n^{2{,}27}$ ;
- incidences site–cover déjà de l'ordre de $n^{2}$ à K10.

À 60 k, les projections CPU se dégradent d'au moins ×1,4 à ×1,5, et les projections GPU de ×1,2 à ×2,2 sur q3/q4. Les trames brutes avec sol restent hors de portée de toutes les familles : sur G4 à K5, le flux q3/q4 v8 prenait de 34 à 505 s pour 120 k sites.

## 8. Pistes non couvertes par D1 à D6

- **Réfutation des ancres longues sans expansion.** C'est le seul levier de générateur à fort potentiel que les mesures aient identifié. Il relève de la famille B (notes `PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md`, `Q34_BLOCS_LIDAR_SHADOW_20260923.md`, `SHADOW_HA_Q34_LIDAR_20260923.md`) ; c'est l'étape 4 de la feuille de route.
- **Générateur émettant par niveau**, pour faire se recouvrir génération et FULL. Aucune proposition n'existe ; c'est un prérequis de 100 ms. À n'ouvrir qu'avec un théorème de complétude par niveau.
- **Clé exacte par support, hors coquilles étendues** (L4 de D6, lemme A de D5). Elle supprime le PGCD de fusion et une partie des recherches de clés (27,2 Gcycles avant l'index). La clé canonique resterait réservée aux 135 à 280 boules à coquille étendue.

## 9. Artefacts

Archivés dans [`c_alternatives_20260923/`](c_alternatives_20260923/README.md) :
les six propositions complètes, les notes des trois jurys, les six
vérifications adverses, et les sources et résultats agrégés des expériences
(`experiences/` : `d1/`, `d5/`, `d5_refut/`, `refute_d5_full/`, `d3_echelles/`,
`d3_refut/`, `d3_refut_juge/`, `jure_D1_completude/`, `jure_rigueur/`, `d2/`,
`d4_inversion/`, `D6_gpu/`), **sans** binaires, sans les catalogues `cat_*.bin`
ni aucune coordonnée dérivée de SemanticKITTI. Les fixtures `fx_cz`, `lat5_3`,
`lat5_1` et `fx_ico12` sont synthétiques. Plusieurs scripts citent les chemins
du poste de l'auditeur ; ils se relancent en adaptant ces chemins et la
bibliothèque v9 construite depuis `93066733`.

GCP non utilisé. `public_status=not_claimed`.
