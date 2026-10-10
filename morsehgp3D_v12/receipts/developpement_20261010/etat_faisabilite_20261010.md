<!-- Produit par le workflow wf_a9c921f5-fef (10 octobre 2026, 20 h 10 à 21 h 55 UTC) : 10 analystes par zone, 10 sceptiques, une synthèse et une critique ; lecture seule, GCP non utilisé. Les fichiers par agent sont dans le journal du workflow, hors dépôt. -->
# morsehgp3D_v12 : état du chantier et ce qui est faisable (10 octobre 2026, 21 h 20 UTC)

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**État du dépôt.**
- `main` est à `cd28a802d`. Les commits depuis `2aaed1847` ne contiennent que des notes de l'auditeur et des reçus. Le produit n'a donc pas changé : Session recouverte, cache de 8 Gio, B2, T1-d, R1, A6c et B3-K.
- La VM G4 est arrêtée (`TERMINATED`). L'auditeur l'a vérifié à 21:12 UTC et aucune instance n'est active (`audit_reponses_20261010/g4_controle_211205.json`).
- Ce bilan est en lecture seule. GCP non utilisé. Aucun chiffre ci-dessous ne change `public_status`.

**Statuts.** [G4 mesuré], [local], [déduit], [supposé].

**Sources.** Les chemins sont sous `morsehgp3D_v12/receipts/`, sauf mention contraire.

| Clé | Chemin |
|---|---|
| **O** | `g4_fullo_20261010` |
| **B3b** | `g4_t2db3b_20261010` |
| **A6c** | `g4_a6c_20261010` |
| **L1o** | `g4_mesb1o_20261010` |
| **L2t** | `g4_mesb2t_20261008` |
| **L1r**, **L2** | `g4_mesb1r_20261008`, `g4_mesb2_20261008` |
| **GAPP** | `g4_gapp_20261008`, `g4_gapp2_20261008` |
| **D6** | `g4_fullm_20261008`, `g4_fullk_20261008` |
| **T2dC** | `g4_t2dc_20261008` |
| **AUD/x** | `audit_reponses_20261010/x` |
| **ÉTUDE C** | `~/v12_sauvegarde/v12_t2d_C/etude_c_contrat/ETUDE_C.md` |
| **RÈGLE C** | `~/v12_sauvegarde/v12_t2d_C/mesc7/repo/morsehgp3D_v12/microbancs/mes_c_levier/pilote_c_levier.py` |
| **CONSTATS** | `morsehgp3D_v12/audits/CONSTATS.md` |

## 1. Où l'on en est

**Régime (a) : contrat FULL K5 en 100 ms. Non tenu.**
- **Dernière mesure complète.** La session O porte sur `aa6338ee8`, c'est-à-dire avant B3-K [G4 mesuré, O] :
  - médiane des 37 trames du v12set : **116,6 ms** ;
  - maximum : **241,9 ms**, sur 00/001896 (95 586 sites). C'est le maximum des médianes par processus, chacune ne comptant qu'une visite chaude ;
  - **15 trames sur 37** passent sous 100 ms.
- **Ce qui passe.** ng00, ng01 et ng02 passent : 80,4 / 66,9 / 79,3 ms [G4 mesuré, O], puis 78,6 / 64,9 / 78,1 ms avec B3-K [G4 mesuré, B3b]. Toutes les trames de moins de 50 000 sites passent. Aucune trame de plus de 53 754 sites ne passe.
- **Produit actuel.** Il n'a pas été remesuré sur les 37 trames. B3-K donne un rapport de 0,971 à 0,986 sur cinq trames [G4 mesuré, B3b]. On en déduit une médiane d'environ 114 ms et un maximum d'environ 233 ms [déduit]. La dispersion entre sessions (±1 à 3 %) est du même ordre que l'effet de B3-K : il faut lire 110–117 ms et 225–240 ms.
- **Ce qui fixe le mur.** Le mur suit le nombre de boules K5 du catalogue : −8,0 + 62,6 ms par million de boules, R² 0,988 [déduit, O + T2dC].
  - Trame médiane : on paie 53 ns par boule, il en faudrait 47.
  - Trame maximale : on paie 60 ns par boule, il en faudrait 26.
- **Trajectoire** (comparaison descriptive) [G4 mesuré] :
  - médiane : 241 → 161 → 142 → 117 ms (sessions K → M → N → O) ;
  - maximum : 468 → 242 ms.
- **Face à la v11** (comparaison descriptive, AUD/v11_v12_cpu_gpu) : la voie GPU K5 est 3,1 à 3,3 fois plus rapide que la référence v11 ; la voie CPU reste 13 à 17 % plus lente.

**Régime (b) : scènes entières de plusieurs millions de sites.**
- **Session L1o.** Elle porte sur le produit actuel (`fbd5923a8`, arrêt certifié) : 12 cas calculés sur 17 [G4 mesuré, L1o].

| Scène | Sites | Mur | Par million de sites |
|---|---|---|---|
| ETH3D meadow | 6,18 M | 13,5 s | 2,18 s |
| IGN Marseille (froid) | 6,71 M | 13,5 s | 2,02 s |
| TU Wien | 6,24 M | 36,1 s | — |

- **Ce qui a changé depuis L1t.** La queue tombe de 15–19 s à 2–3 s (comparaison descriptive avec L1t, A6c et B3-K confondus).
- **Objectif B1 (2 s par million).** La médiane des 10 cas K5 réussis vaut 2,54 s par million [déduit].
- **Catalogue sur la voie en flux.** C seul coûte 2,4 à 3,4 s par million de sites (TU Wien, SCION sans sol) [G4 mesuré, L1o].
- **Refus `memory_budget`, inchangés.** Aucun ne publie son stade ni son budget :
  - TU Wien sans sol, en seconde passe (CST-0243) ;
  - NIBIO 12 (7,8 M sites) ;
  - Boreas 50 trames (7,9 à 10,8 M sites).
- **Mesuré seulement sur l'ancien produit** [G4 mesuré, L2t] : Paris sans sol (9,1 M sites en 46,5 s), et les refus de Paris entière (14,6 M) et de Lyon (24 à 32 M).
- **Verdicts MES-B.** B1, B2 et B4 ne sont pas tenus ; B3 n'est pas évalué.
- **Identité FUL1** de la voie en flux : établie jusqu'à 1,5 M sites seulement.

**Régime (c) : petits nuages. Jamais tenu, sur cinq sessions (C, C2, C3, N, O).**
- **Session O** [G4 mesuré, O] :
  - C1 = 13,8 ms, pour une cible de 2 ms ;
  - C2 = 9,56 µs par site, pour une cible de 3,727 ;
  - C3 : 4 refus `wide_leaf` à K5, sur les quasi-sphères de 3 000 et 10 000 sites, dans les deux voies.
- **Voie jugée.** La règle écrite juge la voie CPU à 48 fils. La voie appareil (5,11 ms + 1,96 µs par site) est plus rapide sur les 132 nuages réels [G4 mesuré, O].
- **Familles difficiles, face à la v11** (comparaison descriptive) :
  - le réseau de 10 000 sites passe de 8,4 s à 160 ms ;
  - l'uniforme de 100 sites (24 ms) et la quasi-sphère de 1 000 sites (243 à 259 ms) restent plus lents que la v11 (6 et 171 ms). La cause est le repli sur l'hôte des feuilles non résolues [cause déduite du code].

**K10.**
- **Objectif D4 (0,5 s).** Il n'est tenu que sur ng00 à ng02 : 492,6 / 365,5 / 422,9 ms. La marge n'est que de 1,5 % sur ng00, qui fait 499,7 ms dans une autre session du même produit [G4 mesuré, O et B3b].
- **Poids de G.** G fait 79 % du mur et croît de 9,5 fois entre K5 et K10, comme Σ k × représentants. Cette loi n'est établie que sur ng00 [déduit, B3b].
- **v12set.** Non mesuré : environ 0,6 à 0,7 s à la médiane et 1,35 à 1,56 s au maximum [déduit, plutôt optimiste].
- **Scènes à K10.** 14,7 à 24,0 s par million de sites, pour 10 visés, avec 43 à 73 Ko d'hôte par site [G4 mesuré, L1o].

**Précision (D6).**
- Le produit n'est qualifié qu'au profil u21. D6 n'est pas tranchée : CST-0207 est en cours et aucune REGLE_D6 n'est écrite.
- **Mesures disponibles.** Sessions K et M : voie CPU, C et G mesurés séparément, ng00 à ng02, bases antérieures à B2 [G4 mesuré, D6].
  - u24 coûte de 0 à 2 % sur C et sur la résolution de G.
  - u32 coûte de 0,6 à 2,5 % sur C, mais **+5 à +7 % sur la résolution de G**, de façon reproductible.
- **Non mesuré aux profils 24 et 32** : P (la clé de Morton passe en u128 dès u24), la voie appareil et la Session.
- **Projection.** La seule résolution coûterait à u32 environ +2 à 3 % du mur médian [déduit]. u32 dépasserait donc probablement le seuil de 3 %. u24 est plausible, mais pas démontré.

**T3 (vues) et T5 (comparaison à HDBSCAN).**
- **Registre.** R existe en mémoire, adopté avec R1 (rapport 0,963 à 0,973) [G4 mesuré]. Il n'existe aucune vue, ni API, ni CLI.
- **Seule sortie.** La seule sortie est l'export différentiel FUL1, qui est un instrument et pas la vue prévue. Son empreinte coûte 611 ms en médiane à K5, soit 4,3 à 5,3 fois le mur [G4 mesuré, A6c].
- **T5.** Il n'a rien dans la v12.
- **Héritage de la v11** (`morsehgp3D_v11/docs/AUDIT_GEANT_V11.md` § 3.4) [G4 mesuré, v11] :
  - niveau B : +0,008 à +0,078 sur synthétique, de k = 2 à k = 10 ;
  - sur LiDAR, l'avantage est faible : HDBSCAN n'échoue que sur 33 instances sur 2 918 ;
  - niveau C : non établi.
- **Préenregistrement E1 : inachevé.** S3a n'a pas de reçu. S3b et P08 n'ont jamais été lancés, alors que leurs données et leurs plans sont prêts dans `build/v11-persist/` [local].

## 2. L'écart au contrat

| Étage (ms) | Médiane 00/003624, O | après B3-K | cible illustrative | Max 00/001896, O | après B3-K | cible illustrative |
|---|---|---|---|---|---|---|
| P | 2,9 | 2,9 | 3 | 4,7 | 4,7 | 4 |
| C | 37,8 | ≈ 38,5 | ≤ 30 | 64,2 | ≈ 65,4 | ≤ 40 |
| G | 64,0 | ≈ 58,7 | ≤ 55 | 150,3 | ≈ 137,6 | ≤ 46 |
| queue | 11,5 | ≈ 13,1 | ≤ 10 | 21,2 | ≈ 24,5 | ≤ 10 |
| mur | 116,6 | ≈ 113,6 | 100 | 239,8 (241,9) | ≈ 232 (233,5) | 100 |

Colonnes O : [G4 mesuré, O]. Colonnes « après B3-K » : [déduit], à partir des effets par étage de B3b. Cibles : [supposé]. Les médianes d'étage ne s'additionnent pas exactement au mur.

**Médiane.**
- Il manque environ 14 ms (−12 %). Les trames de rang 16 à 19 doivent perdre 4, 8, 12 et 14 ms [déduit].
- Un étage seul ne comble l'écart qu'au prix de l'une de ces baisses :
  - C −35 % ;
  - G −23 %, soit environ −30 % une fois comptée la part reprise par la queue ;
  - G et queue ensemble −19 %.
- Une queue nulle laisserait encore la médiane vers 100,5 ms [déduit].

**Maximum.** Il manque environ 134 ms : le mur doit être divisé par 2,3 à 2,4. Même avec G nul, P + C + queue font au moins 90 ms [déduit, O].

**Ce qui gouverne les gains.**
- **C passe en entier au mur.** C précède G strictement, car G attend les rangs globaux et la table S* complète.
- **G est limité par le débit.** La région de G est saturée : environ 41 fils sur G et 6,5 sur la forêt, sur 48 [déduit, B3b].
- **Un gain sur G passe à environ 70 % seulement.** La forêt cachée sous G réapparaît dans la queue, qui a repris 28 à 32 % du gain de B3-K [G4 mesuré, B3b].
- **La tour se ferme sur les registres des petits ordres depuis A6c.** Le dernier registre publié est celui de K2 dans 105 passes sur 185, et de K3 dans 59 [G4 mesuré, AUD/fullo_temps].
- **La scène pèse autant que la taille.** À 3 % de sites près, le mur varie de 44 ms : 105,9 ms pour 00/001502 contre 150,3 ms pour 05/002060 [G4 mesuré, O]. Une cohorte de décision limitée aux trames de rang 16 à 20, toutes ou presque de la séquence 00, ne suffit donc pas.
- **Les budgets d'ARCHITECTURE § 3 sont périmés.** Ils visent une trame de type ng00, d'environ 40 000 sites. G y était prévu à 25–30 ms ; il en fait 45 sur ng00.

**Lecture du maximum (je ne la tranche pas).**
- Le texte en vigueur inclut le maximum du v12set :
  - D7, que vous avez décidée, dit « toutes tailles » et « à la médiane et au maximum » ;
  - MESURE § 2 fait du v12set la base de toute décision de vitesse ;
  - l'auditeur le lit ainsi (AUD/REPONSE_G_TRAVAIL.md, Q4).
- Toute autre lecture serait un amendement de D7. Chiffres pour en juger, après B3-K [déduit, O] :

| Lecture | Médiane | Maximum | Remarque |
|---|---|---|---|
| Tout le v12set | ≈ 114 ms | ≈ 233 ms | texte en vigueur |
| Bande de 50 000 à 70 000 sites (14 trames) | ≈ 124 ms | ≈ 146 ms | la médiane empire |
| Normalisation : 1,67 µs par site | 1,80 µs | 2,42 µs | 15/37 passent ; favorise les grandes trames alors que G est superlinéaire (exposant 1,46) |
| 90e centile | ≈ 156 ms | — | — |

- Correction à faire dans MESURE § 2 : le « maximum de 126 267 sites » est une trame avec sol. Le maximum sans sol est de 99 099 sites.

## 3. Leviers classés

**État de chaque levier :** [A] prouvé par l'auditeur, correctif prêt ; [P] codé, à rebaser et à qualifier ; [C] à concevoir.

Gains en ms sur le mur de la trame médiane puis de la trame maximale.

| # | Levier [état] | Zone | Gain médiane / max | Statut du gain | Risque pour l'objet | Effort ; prérequis |
|---|---|---|---|---|---|---|
| 1 | Lot C : un lot de feuilles par trame (2^19), kCase 128, une seule attente, sans synchronisations de diagnostic [P] | C | −3 à −6 / −5 à −10 | prédiction écrite d'avance, RÈGLE C [supposé] ; C1 seul −3,2 [déduit, ajustement sur 37 trames] | nul : catalogue identique à l'octet (FUL1) ; cases ≤ 1,5 Gio | moyen : rebase de `mesc7` sur `2aaed1847`, 4 portes rouges plus des échecs CUDA u21/u24 |
| 2 | Clôture du grand livre dans la région (`fill_ledger` par morceaux) [C] | queue | ≈ −2 / ≈ −3,7 | borne [déduit] de l'intervalle mesuré (A6c, B3b) ; attribution [supposé] | nul : compteurs identiques, mutant à ajouter | faible (< 1 jour) ; publier la fin de région à part |
| 3 | Égalité exacte dans `PopulationTable::find` ; LEM-T1 sur F\S [A] | G | −0,5 à −1,5 / −1 à −2,5 | [supposé] : les modèles comptent des comparaisons, pas des temps | nul sur un catalogue valide ; F\S perd la détection incidente d'un S* corrompu | très faible : 2 mutants réancrés (`0dcac3919`), bras séparés ; effet sous la résolution d'une règle |
| 4 | P en parallèle de C (index de G construit pendant C) [C] | P/C | ≤ −2 / ≤ −2,5 | [supposé] ; seule la part index de P peut recouvrir C | nul | faible à moyen |
| 5 | Clés B3-K sur 8 octets (sous 2^21 sites) ou construites sur l'hôte [C] | C | −0,2 à −1 / −0,5 à −1,1 | transfert [G4 mesuré, B3b] ; gain net [supposé] | faible : porte d'égalité, témoins à 2^21 − 1 et 2^21 | faible ; bras du lot C |
| 6 | Ouverture de G hors du chemin critique [C] | G | −2 à −5 / −4 à −10 | plafond ≈ 6 / 11 [déduit] ; ouverture 13,9 / 24,4 dont tables 8,6 / 15,4 [G4 mesuré] | nul ; peut défaire le groupement des tables de B2 | moyen (1 à 2 jours) ; publier les sous-chronos de l'ouverture |
| 7 | Survivants de G traités par fronts avec préchargement ; arrêt en un accès mémoire [C] | G | −4 à −9 / −8 à −16 | [supposé] ; les survivants font 23 % des représentants et ≈ 62 % du temps-fil [déduit] | nul : calcul inchangé | élevé (2 à 4 jours) ; microbanc sur captures réelles |
| 8 | Queue : chaîne A6c par ordre et aides bridées (A6d) ; étapes séquentielles de M, H et R parallélisées [C] | queue | −1 à −4 / −2 à −6, non additifs | [supposé] | nul ; risque de perte sur les petites trames (A6 et A6b ont été rejetés pour cela) | moyen ; CST-0244 et CST-0245 fermés d'abord |
| 9 | C guidé par le profil : boucle de niveaux sur l'appareil, rapatriement allégé, taille de feuille puis noyau J3 [C] | C | −2 à −5 / −4 à −10 | [supposé] | nul à faible | moyen à élevé ; MES-C-PROFIL joué d'abord |
| 10 | Formes de cellules publiées par C (R2) [C] | C→G | −2 à −5 / −5 à −8 | plafond 5,4 / 8,8 [déduit] ; net [supposé] | moyen : prédicats exacts sur l'appareil | élevé ; recoupe le n° 6, contrats à amender |
| 11 | Catalogue hors fenêtre pour l'ordre K (G-L4) [C] | C/G | ≤ −1,5 / ≤ −4 ; à K10, ≈ −20 sur G | [supposé] ; borne K10 [déduit] | touche le contrat du catalogue | moyen à élevé ; mesure MES-G2 d'abord |
| 12 | G sur l'appareil : ouverture et premières sondes, puis fronts complets [C] | G | −15 à −20 / −35 à −50 (première étape) | noyaux ×9 à ×22 [G4 mesuré, GAPP, microbanc] ; intégration [supposé], probablement optimiste | élevé : identité hôte/appareil ; D1 et D2 déjà rejetés | très élevé (1 à 2 semaines) ; décision D16, règle nouvelle |

**À ne pas poursuivre pour le contrat :**
- une résolution par composante locale : elle ne touche que 0,01 à 0,05 % des résolutions à K5 [déduit] ;
- la première sonde virtuelle, tant que la trace n'est pas ventilée (±1,5 ms) ;
- une retouche du seuil A6c ;
- le partage de cibles entre ordres, faux en général.

**Leviers hors contrat :**

| Levier [état] | Régime | Effet attendu |
|---|---|---|
| `645b428` : état de l'appareil rendu entre deux appels, et ligne `refus` (stade, budget) [P] | (b) | corrige probablement CST-0243 ; rend les refus attribuables [local, transit simulé] |
| `a85afb9` : repli séquentiel quand l'admission de la région refuse [P] | (b) | pic de mémoire plus bas ; Paris entière plausiblement admise [supposé] |
| Borne d'admission resserrée [C] | (b) | la borne vaut environ 1,75 fois l'usage réel sur Paris sans sol [déduit] ; le seul bras `--cache=0` ne suffirait pas pour Paris entière [déduit] |
| Fin d'étage par tranches de C plus rapide [C] | (b) | −1 à −1,7 s par million de sites [supposé] |
| Voie large T1-c (CST-0237) [C] | (b), (c) | ETH3D courtyard et critère C3 ; aucune borne de coût [supposé] |
| Équipe de fils proportionnée au travail [C] ; latence de la voie appareil [P puis C] | (c) | CPU vers 120 sites : 9,7 → ≈ 6 ms [déduit] ; partie fixe de C sur l'appareil : 3,3 → moins de 1 ms visé [supposé] |
| REGLE_D6, mesure FULL appariée 21/24 (32 en information), qualification u24 [C] | D6 | tranche D6 ; le lecteur FULL est déjà paramétrable, les pilotes sont figés à 21 |
| Full compact et signature par colonnes en parallèle [C] | T3 | empreinte : 611 → 40 à 100 ms [déduit] |
| T5-0 : S3a, S3b et P08 avec les instruments v11 gelés [P] | T5 | clôt le préenregistrement E1 ; S2b fait déjà échouer R1(5) et R1(10) |

## 4. Ce qui est faisable

Projection sur le mur de la trame médiane (00/003624) et de la trame maximale (00/001896), en ms :

| Palier | Contenu | Médiane | Maximum | Statut |
|---|---|---|---|---|
| mesuré | session O, `aa6338ee8` | 116,6 | 241,9 | [G4 mesuré, O] |
| 0 | produit `2aaed1847` (B3-K) | ≈ 114 (110–117) | ≈ 233 (225–240) | [déduit] |
| 1, court terme (≈ 1 semaine à partir du 12 octobre) | leviers 1 à 5 | ≈ 102–109 | ≈ 213–223 | [supposé] |
| 2, moyen terme (2 à 4 semaines) | leviers 6 à 10 | ≈ 85–100 | ≈ 175–205 | [supposé] |
| 3, architecture | G sur l'appareil (D16), C réduit d'un tiers, queue ≈ 10 ms | ≈ 50–80 | ≈ 100–150 | [supposé], aucune mesure d'intégration |

**Médiane : atteignable, mais seulement en empilant des leviers.**
- Il faut le palier 1 et une bonne part du palier 2, chacun près de sa prévision.
- Seul C1 est chiffré par un ajustement sur mesures. Plusieurs leviers prédits ont déjà été rejetés sur G4 : A6, A6b, G-L5, D1 et D2 de G sur l'appareil, le balayage B3-B.
- Si la moitié seulement des gains se réalise, la médiane reste vers 100–105 ms.

**Maximum : hors de portée des leviers CPU et catalogue.**
- La seule voie identifiée combine trois chantiers : G sur l'appareil, C réduit d'un tiers et une queue d'environ 10 ms.
- L'étude qui annonce un G de 19 à 40 ms au maximum est antérieure aux mesures G-APP. Or les propositions en binaire64 n'y vont que 1,3 fois plus vite.
- La forêt impose aussi un plancher : le noyau T de l'ordre K finit environ 11 ms après la résolution de cet ordre [G4 mesuré, B3b].

**K10.**
- Les paliers 1 et 2 donneraient −10 à −20 % [supposé] : environ 0,4 s sur ng00 et 0,5 à 0,6 s à la médiane du v12set.
- 100 ms à K10 demanderait de diviser G par 9 et C par 2 : c'est hors de cette architecture [déduit].

**Régime (b).**
- Plafonds d'admission du produit actuel [déduit] : environ 12,7 M sites (ETH3D), 14,1 M (Marseille), 8 à 9 M (Boreas 10 trames) et 5,1 M (SCION).
- Avec le repli séquentiel et une borne resserrée, les refus actuels sous 10 M deviennent plausiblement calculables, et 20 M pour les scènes de type IGN, avec peu de marge [supposé ; plafonds tirés de mesures sans cache].
- 30 M ne tient pas sans changer la représentation de la tour : la fin de tour mesure 5,3 à 7,6 Ko par site, soit 150 à 210 Gio pour un budget de 160 Gio [déduit, L1r et L2].

**Régime (c).**
- C2 est déjà tenu par la voie appareil, si la règle y est réancrée.
- C1 : environ 3 ms vers 120 sites en quelques jours [supposé]. Descendre à 2 ms exige un graphe CUDA résident : 1 à 2 semaines, issue incertaine. Sur la voie CPU, il faudrait un facteur d'environ 20 [déduit].
- C3 dépend de T1-c : plusieurs semaines, sans borne de coût.

**D6, T3 et T5.**
- D6 : u24 se tranche en une session. u32 est improbable à court terme.
- T3 : full compact et squelette demandent 2 à 3 jours et une session ; les vues points et plat, 3 à 5 jours.
- T5-0 : 3 à 4 sessions de 70 minutes au plus.

## 5. Plan proposé

**Jusqu'au 12 octobre 07:00 UTC : le développeur seul, sans nouveau levier.**
1. Intégrer le correctif de l'auditeur pour CST-0244, puis celui pour CST-0245 ; tous deux sont prêts. Mettre le registre à jour :
   - écrire « A6c adopté sous réserve de CST-0245 » ;
   - requalifier CST-0242 ;
   - inscrire la régression de Paris entière et l'anomalie de SCION (passe chaude plus lente que la froide).
2. Écrire un juge commun v2 pour tous les pilotes : identités relues dans les journaux bruts, codes et stderr par processus, hash final des sondes, rapports individuels des mutants.
3. Ajouter l'instrumentation, sans changer le temps :
   - fin de région distincte de la fin de la tour ;
   - début et fin de chaque étape, par ordre ;
   - compteurs d'aides ;
   - sous-chronos de l'ouverture de G ;
   - ligne `refus`, reprise de `645b428`.
4. Documents : corriger MESURE § 2, mettre les budgets d'ARCHITECTURE § 3 à la taille des trames médiane et maximale, créer la table de réconciliation.

**Session G4 « Q » : qualification, sans levier.**
- Socle, `lidar_ctest` (MES-M0), mutants de la tour et du catalogue avec rapports individuels.
- MES-FULL sur les 37 trames du produit exact, pour remplacer les chiffres déduits.
- K10 en information sur les trames médiane et maximale.
- TSan, puis ASan/UBSan, sur la tour avec la chaîne forcée.
- Deux sessions si le tout dépasse 4 200 s.

**À partir du 12 octobre : trois agents en parallèle, une VM à la fois.**
- **Agent C (catalogue).**
  - Rebaser `mesc7` en deux lots : les correctifs du régime (b) (`0001` à `0003`), puis la tranche C (`0004`).
  - Réparer les portes rouges sans les affaiblir, y compris les deux portes déjà assouplies par `676e707`.
  - Redéclarer la RÈGLE C : cohorte avec 00/003624 et des trames chères de même taille (10/000430, 05/002060) ; prédiction de mur corrigée à 0,95–0,98.
  - Ajouter les bras des clés et MES-C-PROFIL en information.
  - Session G4 « C » : le plan déclaré (5 400 s) doit passer sous 4 200 s. Y joindre un MES-B ciblé : TU Wien sans sol en 3 passes, Paris entière, bras séquentiel.
- **Agent B (étage G).**
  - Égalité exacte et F\S, en bras séparés.
  - Comptages locaux : ventilation de la trace, doublons de census par tranche, cellules étendues.
  - Puis l'ouverture hors du chemin critique, puis les fronts, après un microbanc.
- **Agent A (tour).**
  - Clôture du grand livre dans la région.
  - Porte d'échelle avec la chaîne forcée : de 32 000 à 64 000 sites, 1 et 8 fils, même empreinte FUL1.
  - Puis A6d et les étapes séquentielles.
- **Session G4 « G1 ».** Partagée entre A et B, en bras séparés, sous une règle écrite d'avance.

**Ensuite.** Session D6 si vous la décidez ; session (b) sur L2 (Paris, Lyon) avec la ligne `refus` ; palier 2 ; T5-0 dans les créneaux libres de la VM. Au total, environ 8 à 11 sessions G4 sur 2 à 3 semaines, chacune certifiée `TERMINATED`.

## 6. Décisions demandées

1. **Maximum (D7).** Confirmer le texte actuel, le maximum sur tout le v12set, ou l'amender : bande de tailles, normalisation par site, centile.
2. **Ordre de travail.** Faire une session de qualification avant tout nouveau levier. Fixer le statut d'A6c : adopté sous réserve de CST-0245, ou retour à la voie de base jusqu'à ce que les portes passent avec la chaîne engagée.
3. **D16, G sur l'appareil.** Ouvrir dès maintenant une étude écrite, sans code. C'est la seule voie identifiée vers le maximum, et vers 0,5 s à K10 sur le v12set. G-APP et G-APP-2 l'ont refermée sur la base du 8 octobre.
4. **D6.** Mesurer u24 maintenant, en une session, pour juger les leviers suivants au profil final. u32 resterait un candidat de conception.
5. **Régime (c).** Juger C1 et C2 aussi sur la voie appareil. Garder le seuil de C2 figé à 3,727 µs par site, ou l'indexer sur le régime principal, soit 1,80 µs aujourd'hui.
6. **Régime (b).** Réviser B1 et B3 par famille de scènes. Pour 20 à 30 M sites, dire si la résolution de G fait partie de l'objet FULL en mémoire. Sinon, seul resterait le stockage hors mémoire, exclu par ARCHITECTURE § 4.6.
7. **T3 et T5.** Lancer T5-0 (3 à 4 sessions). Trancher D10 (masse du § 9.1 ou comptage entier) et D13 avant les vues. Fixer l'objectif K10 suivant, par exemple 0,5 s à la médiane du v12set.
8. **D14** (CST-0015 et CST-0016, réécriture de l'historique) attend votre décision depuis le 7 octobre.

## 7. Risques, dette et constats qui comptent

- **Le produit adopté n'est pas qualifié.**
  - CST-0245 (majeur) : aucune porte d'admission, de refus ou de pénurie ne couvre la chaîne A6c engagée. Le constat a été inscrit à 18:11 UTC, avant l'adoption d'A6c à 18:44.
  - CST-0244 reste ouvert, et CST-0242 est clos alors qu'A6c réintroduit le pont et les aides.
  - Ni MES-FULL ni MES-M0 n'ont tourné sur `2aaed1847`, et aucun ASan/UBSan n'est déclaré depuis T2-d-A.
  - Les mutants de la voie appareil n'ont tourné qu'en t1bi, alors que `src/catalogue` a changé depuis de +1 838 / −291 lignes.
- **Preuves.**
  - Les exécutables (ELF) ne sont jamais rapatriés et les rapports individuels de mutants manquent.
  - Le juge du pilote B3 se fie aux résumés : 4 corruptions synthétiques sur 7 passent. Le juge v2 n'est pas intégré.
- **Registre.**
  - 80 constats : 37 non clos, dont 28 majeurs ; 3 bloquent u32 et 5 sont à régler avant port [local, CONSTATS].
  - CST-0243 persiste en L1o.
  - Le canal a été compacté par l'auditeur (`cd28a802d`) : 61,7 Ko sur 64 Kio.
- **Méthode.**
  - La dispersion entre sessions (±1 à 3 %) vaut l'effet d'un levier.
  - Le maximum ne repose que sur une visite chaude par processus.
  - Le seuil A6c est calibré sur les trames qui le jugent (37 groupes, pas 40) et dérive quand G accélère.
  - La règle B3 a été révisée trois fois, sans registre des tentatives.
  - « Trame médiane » désigne tantôt 02/001606 (médiane en taille), tantôt 00/003624 (médiane en temps).
- **Régime (b).** Les refus ne sont pas attribuables : aucun ne publie son stade ni son budget.
- **Documents.**
  - CONTRAT_TOUR, ARCHITECTURE § 3, MESURE, DECISIONS et LECONS décrivent l'état du 7 et du 8 octobre.
  - Sur les correctifs proposés par l'auditeur, 25 s'appliquent encore et n'ont pas été triés ; 48 sont périmés.
- **Moyens.**
  - Les agents sont arrêtés jusqu'au 12 octobre 07:00 UTC, et il n'y a qu'une VM G4.
  - `/workspaces` est rempli à 91 % (5,9 Go libres) et `/tmp` est vidé à chaque redémarrage.
  - Aucune CI ne couvre la v12.

## Annexe : critique de complétude (à lire avec le rapport)

Une dernière relecture indépendante a vérifié le rapport en lecture seule. Ses corrections priment sur le texte ci-dessus quand elles le contredisent.

**Avis.** Le rapport est solide et presque toujours exact. Je l'ai vérifié en lecture seule : reçus, notes de l'auditeur, CONSTATS, et quelques petits scripts Python sur les journaux bruts hors dépôt (/workspaces/.ehgp-sessions/…, rien construit). GCP non utilisé.  Chiffres retrouvés à l'identique : - les tableaux de O (116,6 et 241,9 ms, 15 trames sur 37, le seuil de 53 754 sites, les rangs 16 à 19) ; - l'ajustement du mur sur le nombre de boules (−8,0 + 62,6 ms par million de boules, R² 0,988), l'exposant de G (1,46), la bande de 50 000 à 70 000 sites, le 90e centile ; - le tableau de L1o et la médiane de 2,54 s par million ; - les comptes de CONSTATS (80 constats, 37 non clos, 28 majeurs, 3 bloquent u32, 5 avant port), le canal à 61,7 Ko ; - l'inscription de CST-0245 (18:11:21) avant l'adoption d'A6c (18:44:51) ; - les +1 838 / −291 lignes du catalogue depuis t1bi, l'absence de toute modification du produit depuis 2aaed1847, et l'état TERMINATED à 21:12:05.  Les conclusions centrales tiennent : la médiane n'est atteignable qu'en empilant des leviers, et le maximum reste hors de portée sans G sur l'appareil. Deux éléments absents les renforcent : - par séquence, les séquences 00, 02 et 10 n'ont aucune trame sous 100 ms ; - la trame maximale coûte 7,8 CPU·s, ce qui exclut 10 Hz au maximum, même en recouvrant deux trames.  Avant diffusion, corriger quatre points de fond : - le plafond Boreas, contredit par le refus mesuré à 7,86 M ; - C2 « tenu » sur l'appareil, qui dépend d'un seuil figé ; - l'attribution à l'utilisateur de la règle médiane + maximum de D7 ; - l'argument de plancher sur T(K).  Compléter aussi : - les obligations de D1 et D2 (froid, cadence, CPU·s) : la cadence est le repli déclaré par MESURE, et c'est la seule option crédible à court terme pour un flux à 10 Hz à la médiane. Elle mérite une décision explicite, et la session Q devrait mesurer froid et cadence ; - D6, avec le surcoût des tables de G à ng02 et les grilles fines ; - K10 et la famille uniforme pour les petits nuages ; - la dérive mémoire du régime (b) ; - la représentativité du v12set pour le maximum ; - le statut [local] de S2b et les chiffres sans source.

**Erreurs relevées.**

- Commit de L1o. La session a tourné sur ae8f8107c (le produit 2aaed1847 plus des notes de l'auditeur), et non sur fbd5923a8, qui est le commit du reçu (g4_mesb1o_20261010/README.md, ligne 3).
- Plafond Boreas « 8 à 9 M (Boreas 10 trames) » [déduit], contredit par la mesure. Dans la même session L1o, Boreas 50 trames sans sol (7 857 268 sites) est refusé dès l'ouverture : open puis exit, aucune FULL (AUD/mesb1o_temps). Des plafonds tirés de lois par site ne sont pas des bornes (CST-0211), et les refus ne sont pas localisés. Ces plafonds sont donc à requalifier en [supposé].
- « C2 est déjà tenu par la voie appareil, si la règle y est réancrée » : vrai seulement contre le seuil figé de 3 727,2 ns par site, soit 241,3 ms / 64 740 sites de la session K (microbancs/mes_c_petits/pilote_c.py, ligne 31). MESURE § 2 demande un coût par site « jamais supérieur à celui du régime principal ». Ce régime vaut aujourd'hui 1,80 µs par site (116,6 ms / 64 740 ; médiane par trame 1,83), et la pente de l'appareil 1,958 µs [G4 mesuré, O]. C2 n'est donc pas tenu. Le rapport se contredit avec sa propre décision 5.
- « D7, que vous avez décidée, dit … à la médiane et au maximum » : attribution surinterprétée. La citation de l'utilisateur dans DECISIONS.md ne parle ni de médiane ni de maximum. Elle dit aussi : « Je te laisse libre pour ces choix de mesure et de régime ». Les décisions ont été « prises … par le développeur sur cette délégation, sauf mention contraire ». Le registre marque D7 « décidée (utilisateur) », mais la règle médiane + maximum est une formulation du développeur. Le rapport doit le dire, puisqu'il demande à l'utilisateur de confirmer ou d'amender D7.
- « La forêt impose un plancher : le noyau T de l'ordre K finit environ 11 ms après sa résolution » : non établi pour le maximum. Depuis A6c, G traite les ordres de K vers 1. À 00/003624, le G de l'ordre 5 finit à 39,0 ms, contre 64,0 ms pour les ordres 1 et 2, et la tour se ferme sur R2 et R3 (73,7 et 72,9 ms). À 00/001896, la queue est faite de V et R des ordres 2 à 4 (V4 à 165,5 ms, R3 à 167,3 ms) [G4 mesuré, fins_par_ordre_ns des journaux O ; AUD/fullo_temps : K2 dernier dans 105 passes sur 185]. Le retard de T(K) n'est pas sur le chemin critique.
- « G croît de 9,5 fois entre K5 et K10 » : l'étage G entier croît de 8,6× (O : 45,2 → 388,2 ms) ou de 8,7× (B3b, bras avant : 45,5 → 394,7 ms). On n'obtient 9,4× qu'en retirant l'ouverture (journaux k10 de B3b).
- « Les mutants de la voie appareil n'ont tourné qu'en t1bi » : vrai seulement pour les mutants natifs CUDA. Les 38 mutants du catalogue, dont une trentaine sur des portes device_unit exécutées sur l'hôte sans drapeau CUDA, ont tourné en B3b (AUD/session_b3b_admission). La différence de +1 838 / −291 lignes depuis d2f39fe82 est exacte.
- Petits nuages : la cause « repli sur l'hôte des feuilles non résolues » [déduite du code] est fragile. La voie CPU, qui n'a pas d'appareil, est aussi lente : uniforme 100 à 28,5 ms contre 24,4 ms sur l'appareil, sphère 1 000 à 258,6 ms contre 243,2 ms (rapport_c.json, O). De plus, l'uniforme fait partie du groupe régulier synthétique, pas des « familles difficiles » de MES-C.
- Imprécisions mineures. Tableau L1o : TU Wien vaut 5,79 s par million (mur froid, comme Marseille) et non « — ». La médiane de 2,54 s par million mélange murs chauds et froids. « R existe, adopté avec R1 » : R est livré le 8 octobre avec T, M, V, R ; R1 n'est qu'un raccourci (0,963–0,973). Normalisation par site : dans O, 14 trames sur 37 passent, et non 15 ; ng01 (08/000100, 65 ms) y échouerait à 1,83 µs par site, effet pervers à signaler. « Correctifs CST-0244/0245 prêts » : ils sont proposés et vérifiés sur modèle, mais ni intégrés ni exécutés (CONSTATS).

**Manques.**

- Cadence (D2) et CPU·s par trame, absents du rapport. D2 impose de mesurer et publier la cadence (recouvrement de deux trames), et MESURE § 1 en fait le « repli déclaré ». Aucun reçu g4_* de la v12 ne la mesure, et le rapport ne pose pas la décision. MESURE § 1 exige aussi de publier les « CPU·s par trame » : 3,37 CPU·s à 00/003624 et 7,79 CPU·s à 00/001896 [G4 mesuré, cpu_ns des journaux bruts O, /workspaces/.ehgp-sessions/v12.20261010.fullo/.../brut/v12set_r*.jsonl ; l'attente active des fils peut y être comptée]. Bornes optimistes [déduit] : environ 12 à 14 trames/s à la médiane, mais environ 6 au maximum, même avec un recouvrement parfait. 10 Hz au maximum est donc exclu sans réduire le travail. C'est la seule piste de faisabilité à court terme pour un flux à 10 Hz sur la médiane, et elle n'est ni chiffrée ni soumise à l'utilisateur.
- Régime froid (D1, « toujours publié à côté ») : rien pour le régime (a). Dans O, la « 1re passe » n'est pas un froid : l'auditeur écrit « La première visite d'une trame n'est pas un nouveau démarrage à froid » (AUD/fullo_temps), et l'ouverture de Session est hors du mur. Mesures [G4 mesuré, lignes open des journaux O] : ouverture de 117 à 356 ms, puis première passe de 92 à 103 ms sur ng00–02. Le froid réel serait d'environ 210 à 460 ms [déduit], jamais publié. À ajouter à la session Q avec la cadence.
- Mesures B3b non exploitées. La plus grande trame, 08/002119 (99 099 sites), a été mesurée avec B3-K : 244,0 → 235,6 ms, rapport 0,966, hors de la fourchette 0,971–0,986 citée [G4 mesuré, g4_t2db3b README, information, 3 processus]. À K10, le lot B3 donne 499,7 → 480,1, 376,8 → 360,2 et 437,0 → 418,3 ms (tableaux_t2d_b3.md, bloc k10, 1 processus × 4 passes chaudes) [G4 mesuré, information]. La marge K10 sur ng00 est donc d'environ 4 % avec le produit actuel, et non 1,5 %.
- D6 incomplet. Les chiffres cités (u24 : 0 à 2 % ; u32 : +5 à +7 %) ne valent que pour resolve_ns. L'étage G entier coûte plus à ng02 [G4 mesuré ; recalcul local sur les journaux bruts fullk/fullm *_mes_d6_profils/brut] : +10,8 % en u24 et +13,6 % en u32 dans M (AUD audit_reponses_20261008/session_m_d6), +6,8 % et +11,2 % dans K. La cause est un surcoût reproductible des tables de G à ng02 (+9 à +15 %). Les grilles fines, qui sont le vrai usage de u24 et u32, coûtent plus encore (g4_fullm README § 3) : ×8 donne C +3 à +7 % ; ×2048 donne un C CPU ×2,93 à ×2,99 et G +9 à +13 %. L'auditeur ajoute que ×2048 reste sous 2^29 : aucun test ne couvre le domaine u32 entier. Le critère « < 3 % » (mêmes coordonnées ou grille fine) reste ouvert (CST-0207). « u24 plausible » est à tempérer, et la lecture du critère est à soumettre à l'utilisateur.
- Petits nuages : K10 et famille uniforme oubliés. À K10, MES-C O donne réseau 10 000 = 11,2 à 11,9 s, réseau 3 000 = 7,3 s, sphère 1 000 = 1,59 à 1,62 s [G4 mesuré, tableaux_c.md]. La régression de l'uniforme est plus forte que ce que dit le rapport. Uniforme 1 000 à 48 fils : 74,8 ms sur l'appareil et 82,5 ms sur le CPU, contre 19,1 ms en v11 (g4_t0g). Elle est non monotone : 36,6 ms à 3 000 sites sur l'appareil, et 309 ms sur le CPU à 4 fils (rapport_c.json).
- Dérive mémoire du régime (b), non signalée. De L1t à L1o, les 23 pics de l'hôte augmentent et C augmente dans 22 passes sur 23 (AUD/mesb1o_temps). Exemple : TU Wien sans sol passe de 80,07 à 84,45 Go [G4 mesuré]. B3-K ajoute 16 octets par boule. Les plafonds d'admission et la phrase « refus sous 10 M plausiblement calculables » n'en tiennent pas compte.
- Exactitude du produit mal située. MES-FULL ne contrôle que la cohérence interne (passes, processus, appareil = CPU ; digests() de pilote_full.py), jamais les empreintes de la v11 (MESURE § 4). Les FUL1 v12 de ng01 et ng02 (2d58a72a…, 27d7650a…) diffèrent à l'octet de MHGP11FUL1 (5212a2ce…, 78feb765…). Seule l'identité sémantique tient : MES-M0 7/7 sur aa6338ee8 (g4_a6c, lidar_ctest). Cette dernière preuve est à citer. Aucune référence v11 n'existe pour les 37 trames (CST-0001 ouvert).
- Représentativité du maximum et des séquences. Le v12set compte 37 trames tirées de 275 trames en cache, sur 6 séquences sur 11 : la 08 en fournit 234, la 05 trois, la 10 deux, et les séquences 01, 03, 04, 07, 09 manquent (DONNEES § 4). Les 15 trames qui passent viennent de 06 (8/8), 08 (6/11) et 05 (1/3) ; les séquences 00, 02 et 10 n'en ont aucune [G4 mesuré, O]. Le mur suit les boules (vérifié : −8,0 + 62,6 ms par million, R² 0,988), et les boules par site vont de 24,8 à 40,6. Une trame d'environ 100 000 sites plus dense dépasserait donc 242 ms [déduit]. La décision sur D7 devrait aussi figer ou élargir le jeu de référence.
- T5 v12 non chiffré. PLAN § 6 prévoit les trois niveaux A/B/C, le bras « même tête sur l'arbre de sklearn », MR_k-bord, un développement sur les séquences 00–07 et 09–10 (absentes du cache) et un bilan sur la 08. Le rapport ne propose que T5-0, avec les outils de la v11. Par ailleurs, « S2b fait déjà échouer R1(5) et R1(10) » n'a pas de reçu (AUDIT_GEANT_V11 § 3.4 : « S2b ... sans reçu dans le dépôt » ; seul build/v11-persist/e1_s2/choice.json existe) : statut [local] au mieux.
- Voie CPU, référence D5 et voie jugée par C1 et C2. Elle est 13 à 17 % plus lente que la v11, et le catalogue CPU fait 84 à 85 % de son mur (AUD/fullo_temps). CST-0233 et CST-0234 sont ouverts (« majeure »). Le protocole apparié de l'auditeur (audit_reponses_20261010/protocole_cpu_v11_v12.md : 35 processus, 350 passes, une session) n'apparaît ni dans les leviers ni dans le plan.
- Leviers système non examinés, pourtant peu coûteux à mesurer. D'abord le nombre de fils et le placement SMT pour G et la forêt : W48 est plus lent que W4 sur les 20 petits nuages (AUD/fullo_temps). Ensuite l'allocateur : le réglage glibc « @tas » a donné le meilleur mur de la v11 (196,8 ms, MESURE § 3.1), et l'environnement glibc de la v12 n'est pas enregistré dans O (AUD/v11_v12_cpu_gpu).
- Risque non cité pour le levier 9 (boucle de niveaux sur l'appareil, capacités par majorant) : CST-0205. La profondeur du parcours n'est pas bornée à 38 niveaux, un témoin K5 u21 atteint la profondeur 63, et le produit comme la voie GPU sont à requalifier.
- Chiffres sans source. « 25 correctifs s'appliquent encore, 48 sont périmés » n'a ni chemin ni statut (receipts/ contient 94 fichiers .patch dans 89 dossiers). Les plafonds d'admission (12,7 / 14,1 / 8–9 / 5,1 M) et le facteur 1,75 n'ont pas de source traçable.
