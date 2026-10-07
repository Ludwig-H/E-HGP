# Fouille du corpus des auditeurs (v3 → v11) : ce qui se transpose utilement dans la v11

4 octobre 2026, rédigé à partir de 13 h 05 UTC (heure lue par `date -u`). Auditeur de la source « corpus des
auditeurs, toutes versions » pour l'audit des transpositions (`../CONTEXTE.md`). Rappel de la demande : **le contrat
reste 100 ms** (tour FULL K = 5 sur trame sans sol de 30 000 à 60 000 sites, G4 48 fils ; K = 10 si possible).

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Méthode. Lecture des deux cartes (`../cartes/CARTE_V11.md`, `../cartes/CARTE_V10_VITESSE.md`), puis du corpus :
audits v11 sur `origin/main` = **`2b1abb6a5`** (aucun changement de `src/`, `audits/` ni `bench/` depuis `eb036dbe2` ;
`src/` identiques à `3bd4d734e`), reçus d'auditeurs v11, audits v3 à v10 (`morsehgp3D_v*/audits/`), audits racine
(`audits/`), pistes fermées, et deux travaux **non publiés** lus en place (voir [WIP-A] et [WIP-D]). Petits calculs
Python sur des JSON de reçus : [`auditeurs_pts4_borne.py`](auditeurs_pts4_borne.py) (`python3 -B`, code 0 vérifié à
13 h 00 UTC). Étiquettes : **M** mesuré (reçu nommé), **E** estimé (arithmétique sur mesures citées), **C** conjecturé.

## Sources et abréviations

Chemins relatifs à `morsehgp3D_v11/` (lus par `git show origin/main:…` depuis `build/v11-claude-20261003`) sauf mention.

| Abréviation | Source |
| --- | --- |
| [AC11] | `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (note moteur des auditeurs, état `1235da4ac`) |
| [AV11] | `audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` (note mathématique) |
| [AO11] | `audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md` (auditeur indépendant d'ouverture) |
| [Q100] | `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (développeur, `d597ed9ba`, sept verrous) |
| [HER] | `receipts/audit_heritage_20261004/` : `README.md`, `q3_deferred/README.md`, `q2_coupled/README.md` (`1235da4ac`) |
| [DEEP] | `receipts/audit_deep_20261004/` : `README.md`, `performance/README.md`, `performance_precision/README.md`, `geometry/README.md` |
| [IND] | `receipts/audit_independant_20261002/` (revues 2 à 18 de l'auditeur indépendant) |
| [PTS4] | `receipts/pts4_review_20261003/README.md` et `case_metadata.json.gz` |
| [WIP-A] | **non publié** : `build/v11-auditor-20261003/morsehgp3D_v11/` — diff de la note moteur (section « Réponses R1–R7 ») et `receipts/audit_selfreview_20261004/` (non suivi : `counter_arena/`, `cell_memo/`, `subgrid_bounds/`, `flat_verdict/`), lus entre 12 h 45 et 13 h 00 UTC ; peut changer avant publication |
| [WIP-D] | **non publié** : diff non commis du développeur dans `build/v11-claude-20261003` (port du q3 différé, 9 fichiers, +138/−30) |
| [AB7], [Q], [PROF1] | reçus G4 v11 décrits dans `../cartes/CARTE_V11.md` § 0 (b872 cinq prises, c40 81 prises, profils `perf`) |
| [EC10] | `/workspaces/E-HGP/morsehgp3D_v10/audits/AUDIT_ETAT_COURANT.md` |
| [PROTO10] | `/workspaces/E-HGP/morsehgp3D_v10/audits/audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md` |
| [L05] | `/workspaces/E-HGP/build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md` et `preuves_l05_code_catalogue/ablation_kT.txt` |
| [C9] | `/workspaces/E-HGP/morsehgp3D_v9/audits/AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md` |
| [PC9] | `…/morsehgp3D_v9/audits/PLAN_CRITIQUE_100MS_FULL_20260924.md` |
| [B9] | `…/morsehgp3D_v9/audits/CONTRE_AUDIT_B_GRAND_AUDIT_C_100MS_20260924.md` |
| [GS9] | `…/morsehgp3D_v9/audits/AUDIT_B_GAINS_STRUCTURELS_100MS_20260926.md` |
| [MAXID9] | `…/morsehgp3D_v9/audits/PHASE_A_MAX_ID_COMPOSANTE_20260923.md` |
| [PART9] | `…/morsehgp3D_v9/audits/FULL_PARTAGE_INTER_ORDRES_20260926.md` |
| [OBS9] | `…/morsehgp3D_v9/audits/OBSTACLES_GPU_SOUS_SECONDE_20260923.md` |
| [P0-8] | `/workspaces/E-HGP/morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md` § 9 |
| [CC8] | `/workspaces/E-HGP/audits/morsehgp3D_v8_complementaire/P0_CENSUS_CPP_Q2.md`, `P0_CONSOMMATION_INDEXEE_Q2.md` |
| [PF3], [PF6], [AB] | `morsehgp3D_v3/audits/PISTES_FERMEES.md`, `morsehgp3D_v6/docs/PISTES_FERMEES.md`, `docs/archive/abandoned/README.md` |

---

## 0. Réponse courte

1. **Aucune idée du corpus des auditeurs ne rapproche à elle seule la v11 des 100 ms.** Les deux reprises retenues par
   les auditeurs le 4 octobre (`1235da4ac`) sont justes, sûres et bon marché, mais petites : le q3 différé vaut au plus
   ~1–1,5 % du CPU à W1 (**E**, `Sphere::through(a,b,c)` = 2,37 % du CPU [AB7]) et il est **déjà en cours de port**
   [WIP-D] ; les extrema q2 couplés ne servent que les census de sphères q2 (**E** : de l'ordre de 0,2–0,6 % du CPU).
2. **Le contrat est sous-dimensionné, c'est le constat le plus utile de cette fouille** (auditeurs-01). Il couvre les
   trames jusqu'à 60 000 sites ; les trois trames mesurées en ont 35 551 à 45 845, d'une seule séquence. Sur les
   127 trames c08 sans sol de [PTS4], les nœuds d'ordre 5 croissent comme n^1,39 ; entre 50 000 et 60 000 sites, ils
   valent ×1,30 (médiane) à **×1,87** (c08_003412, 58 418 sites) ceux de 08/000000 (**M** pour les comptes). Si le temps
   suit ce travail (**E**), la cible effective sur 08/000000 est **~54 à 77 ms** et l'écart à la médiane actuelle
   (412 ms) **×5,4 à ×7,7**, non ×4,1.
3. **Levier structurel** (auditeurs-02) : la publication de l'ordre 5 (42–46 ms de travail séquentiel à W1, 36–52 ms
   à W48 en étage séparé) et son balayage vertical (26–40 ms) ne sont cachés aujourd'hui que par la lenteur de la
   résolution. Diviser la résolution par 3 les expose. Le « lemme du maximum d'ID » de la v9 [MAXID9] donne les
   identifiants canoniques **sans rejouer les plateaux**, donc une publication parallèle à sorties identiques.
4. **Les réponses R1–R7 que l'auditeur prépare** [WIP-A] rendent admissibles quatre leviers demandés par [Q100] :
   compteurs locaux bornés (feuille m ≤ 1024 ⇒ comptes < 2^46), arènes par tâche (réservation effective comptée),
   mémo de cellule typé par sa date de validité λ_b, route GPU avec repli exact avant admission. Gains **C**/**E**.
5. **Le verrou 6 (sous-maille T6) est une fausse bonne idée sur LiDAR** : la v10 l'a mesuré neutre (même dump, nœuds
   +0,2 %, `t_boxes` dans le bruit) [L05] ; inutile d'y consacrer une session G4.
6. **Census** (auditeurs-03) : la v11 ne ventile pas ses census par arité ; mesurer d'abord, choisir ensuite entre q2
   couplé, bornes exactes toutes arités, départ local ou listes de feuilles (avec la réserve des auditeurs : une liste
   de feuille ne certifie pas un centre hors de sa boîte).
7. **Sortie plate** (auditeurs-08) : deux corrections de protocole avant P08, vérifiées dans le code — le bras primaire
   z = 2 manque à la porte contre l'oracle, et H_L2 est revendiqué sur `p_holm < 0,05` seul, sans la borne IC95 %
   préenregistrée.
8. **Fausses bonnes idées** (§ 3) : T6 ; minorant commun des extensions q4 (signes à 153–201 bits par triplet pour
   ~0,3 candidat évité) ; moments de groupe ; index des selles ; MEB « support + extérieur » ; partage géométrique entre
   ordres ; projections en CPU·s/48 ; verdicts d'impossibilité sans borne inférieure.

---

## 1. Ce que le corpus des auditeurs établit sur la route des 100 ms

### 1.1 Calibrations héritées, toujours valables

- **Les verdicts d'impossibilité ont été trop pessimistes, mais le constat de travail tient.** L'auditeur C de la v9
  donnait « 100 ms, quel que soit K : moins de 0,02 » [C9 § 1] ; l'auditeur B a corrigé : « 100 ms infaisable avec les
  algorithmes connus » n'est « pas démontrée » [B9 l. 17–25]. La v10 a ensuite ramené la chaîne K5 de 3,67 s
  (R7b, [OBS9] l. 32) à 0,204–0,254 s (**M**, S4) en changeant d'architecture. Leçon : pas de verdict d'impossibilité
  sans borne inférieure ; mais « il faut réduire le travail, pas seulement mieux le répartir » est confirmé par les
  deux cartes.
- **Le SMT n'est pas gratuit** : de 24 à 48 fils, q3/q4 v9 ×1,22–1,45 pour +42 à +59 % de CPU·s [C9 l. 174] ; boîtes
  v10 ×1,75 (**M**, S4 via `CARTE_V10_VITESSE.md` § 1.4). Ne jamais projeter en CPU·s/48 [C9 l. 538–543].
- **« Le certificat qui coûte plus qu'il ne rapporte »** est le premier motif d'échec du dépôt : « Évalué à chaque nœud
  visité, il ne peut pas économiser plus de visites qu'il n'en coûte » ; « un gain se mesure apparié, contre une
  exécution désarmée » [PF3 § « Les deux motifs d'échec »], repris en v6 [PF6 « Patterns d'erreur »]. Il a tué en v9
  le shadow des moments [B9 l. 95–104] et l'index des selles [C9 l. 264]. Il s'applique aux idées 03, 05 et 06.
- **La taille de la sortie n'est pas un plancher** : 207 Mo de tableaux FULL K5 en 100 ms = 2,1 Go/s [B9 l. 27–35].
- **Budget par chaîne de dépendance** : [PC9 l. 51–72] répartit 100 ms en 10/35/15/35/5 ms et en déduit les facteurs
  exigés par poste ; la carte v11 (§ 6) fait déjà ce travail. Règle conservée : ne jamais sortir un coût du chrono
  (digest, expansion, transferts) pour « gagner ».

### 1.2 Les deux reprises retenues le 4 octobre (`1235da4ac`), à leur juste poids

| Reprise | Ce que prouvent les auditeurs | Ce qu'elle peut rapporter | État v11 |
| --- | --- | --- | --- |
| q3 différé, catalogue et MEB [HER] | même Level brut de degré 6, sans PGCD ; quatre Level jetés évitables sur le tétraèdre 0/2 ; 2 071 contrôles | ≤ ~1–1,5 % du CPU W1 (**E**) | port en cours [WIP-D] → idée 06 |
| extrema q2 couplés pour le census [HER] | 2P = Σ(2z_j − C_j)² − S, i64 en u18/u21/u24 ; 633 gardes ; modèle 17 → 13 bornes, 8 tests ponctuels inchangés | census q2 seulement (**E** ~0,2–0,6 % du CPU) | absent → idée 03 |

Le reçu dit lui-même : « Aucun gain 100/200 ms, GPU ou massif annoncé » [HER README l. 74], et renvoie
« l'inventaire critique des pistes écartées » hors dépôt (l. 7–8) ; cet inventaire n'a pas été retrouvé dans `build/`.

### 1.3 Les réponses R1–R7 en préparation [WIP-A]

L'auditeur répond point par point aux sept verrous de [Q100]. Sa règle d'ensemble : « Les trois budgets ~3 CPU·s et
~50/~50 ms sont des objectifs conditionnels au parallélisme observé, pas des bornes physiques… mesurer même périmètre
FULL, mur, CPU et chemin critique », puis « Mesurer chaque changement séparément, puis leur combinaison ».

| Verrou | Réponse (résumé fidèle) | Idée de ce rapport |
| --- | --- | --- |
| R1 compteurs | oui ; borner la feuille **réelle** m ≤ 1024 (pas `leaf_size` = 32) ; flush `checked_add` avant publication ; `side` total seulement sous certificat couvrant coefficients, sites et replis | 04 |
| R2 q3 différé | tous les champs actuels de `CatalogueLedger` restent identiques ; diagnostics séparés | 06 |
| R3 arènes | oui, réservation effective budgétée ; `count×(3B−depth)` borne les listes simultanées du suffixe, pas tous les nœuds ni les émissions | 04 |
| R4 census | d'abord q2 couplé, puis ablation de partition ; « F6 ≈ 1 % ne démontre pas un goulet mémoire » | 03 |
| R5 mémo | oui comme certificat typé (date de validité λ_b, jamais la date terminale) | 05 |
| R6 T6 | mesurer d'abord ; gardes 5B+6+T ≤ 127 et 2(B+T)+5 ≤ 63 ; reformulation QE contre Dr en 4B+5+T bits (6 219 gardes) | écartée § 3 |
| R7 GPU | voie autorisée ; exact sur le device ou `unresolved` repris sur CPU **avant admission** | 07 |

---

## 2. Idées retenues

### auditeurs-01 — Dimensionner le contrat à sa borne haute : trames de 50 000 à 60 000 sites, plusieurs séquences, maximum et non médiane

*Catégorie : outillage_tests.*

**Sources.** Contrat : `CLAUDE.md` (« trames SemanticKITTI sans sol de 30 000 à 60 000 sites ») et `../CONTEXTE.md`.
Principe des auditeurs : « Une enveloppe vérifiable pour le **maximum** des trames K5 sans sol » [PC9 l. 53] ;
« répétitions froides et chaudes et maximum des trames, d'abord plusieurs trames sans sol de plusieurs séquences »
[PC9 l. 155–158] ; « ne pas prendre le meilleur cas pour la garantie » [OBS9 l. 59] ; « Les mesures ne couvrent que trois
trames d'une seule séquence (08), de 35,5 à 45,8 k sites. À 60 k sites, le travail croît plus vite que n … les
projections CPU se dégradent d'au moins ×1,4 à ×1,5 » [C9 l. 609–615] ; « 100/200 ms, GPU, temps sur plusieurs
séquences, massif et points natifs restent ouverts » [AC11 l. 133–134] ; « Toutes les entrées ne sont pas sans sol à
30–60k : lidar-a a 32 462–126 267 sites » [PTS4 README l. 28–29].

**Mécanisme.** Le 100 ms doit tenir sur la trame la plus lourde de la plage déclarée. Le script
[`auditeurs_pts4_borne.py`](auditeurs_pts4_borne.py) relit les exports natifs de `claudepts3/4` (K1..10, sorties
identiques entre les deux sessions hors clés de temps, [PTS4 README l. 19–22]) :

| Grandeur (trames c08 sans sol de PTS4) | Valeur | Statut |
| --- | --- | --- |
| trames distinctes ; plage de sites ; au-dessus de 60 000 | 127 ; 32 462–98 560 ; 85 | **M** |
| nœuds d'ordre 5 ∝ n^α (identiques entre tours K5 et K10) | α = 1,386 | **M** (régression sur mesures) |
| boules K10 ∝ n^α | α = 1,439 | **M** |
| [40 000, 50 000) : 25 trames, nœuds d'ordre 5 médiane / max rapportés à 08/000000 (576 371) | ×0,92 / ×1,35 | **M** |
| [50 000, 60 000] : 5 trames, médiane / max | **×1,30 / ×1,87** | **M** |
| trame la plus lourde ≤ 60 000 : c08_003412, 58 418 sites | 1 076 856 nœuds d'ordre 5 ; 11 066 031 boules K10 (×2,01 de 5 512 670) | **M** |
| `full_ns` K10 ∝ n^α (22 processus × 4 fils simultanés : non contractuel) | α = 1,07 | **M** sous contention |

**Dans la v11 ?** Non : [Q] et [AB7] ne chronomètrent que ng00/ng01/ng02 (35 551–45 845 sites, séquence 08).

**Gain attendu.** Aucun gain de vitesse : l'idée **corrige la cible**. Si le temps FULL K5 suit les nœuds d'ordre 5
(**E** ; la v9 trouvait déjà ×1,4–1,5 à 60 k [C9]), la cible effective sur 08/000000 est 100/1,87 ≈ **54 ms**
(pire trame de la plage) à 100/1,30 ≈ 77 ms (médiane 50–60 k), et l'écart à la médiane mesurée de 412 ms devient
**×5,4 à ×7,7** au lieu de ×4,1. Conséquence : rattraper la v10 (÷1,5–1,7) couvre au mieux le quart du chemin.

**Mise en œuvre.** Une session G4 : ajouter à la matrice FULL trois trames de 52 000 à 58 500 sites (c08_001881,
c08_002992, c08_003412 dans [PTS4]) et deux ou trois trames d'autres séquences, **préparées avec le masque du contrat**
(Patchwork++ v8 ; PTS4 vient de `bench/points_lidar_prepare.py`, masque peut-être différent : recompter n), K5 W48,
cinq prises, avec les compteurs de travail. Publier le maximum sur l'ensemble déclaré. Échantillon PTS4 non aléatoire
(trames choisies pour leurs objets) : il borne le problème, il ne l'estime pas statistiquement.

**Doctrine, piste fermée.** Aucune modification du moteur ; aucune piste fermée.

### auditeurs-02 — Publier un ordre sans barrière de plateau : forêt minimale et « lemme du maximum d'ID » (v9), un seul index pour la publication et les verticales

*Catégorie : vitesse (architecture).*

**Sources.** [MAXID9] (lemme, § « Reconstruction hors des barrières de niveau », § « Gardes et porte
d'industrialisation ») ; [PART9] § 4 (« Réutiliser l'index événementiel A pour les images C ») et § 5 (contrôle `live`
en lots) ; [PC9] l. 106–125 (« Refaire la résolution et FULL, non le seul Kruskal ») ; [B9] l. 61–66 (« les
`lots_by_k[5]` … pas un plancher invariant pour toute reconstruction parallèle »).

**Mécanisme.** Pour un ordre K : (1) graphe bloc → terminal résolu, poids = rang exact du niveau ; forêt couvrante
minimale par Borůvka à clé (rang, ordinal), au plus ⌈log₂ V⌉ tours ; (2) hiérarchie de composantes répondant à
C<(v, λ), C≤(v, λ) et au maximum des marques ; (3) groupes de plateau = blocs de même (λ, label C≤), parents = C<
distinctes de leurs cibles ; (4) préfixe des groupes qui créent un nœud (0 ou ≥ 2 parents) → IDs exacts. **Lemme** :
« pour toute composante vivante au seuil ouvert λ, sa racine historique canonique est le maximum des IDs marqués »
[MAXID9 § « Lemme du maximum d'ID »]. Le même index sert les images verticales : pointer chaque nœud vers une naissance
par doublement, puis demander la composante fermée de (K−1, b) au rang du nœud [PART9 § 4].

**Dans la v11 ?** Non. La publication est **une tâche par ordre** (`src/tower/forest_concurrent.cpp`, `publish`) ; à
W48 le pipeline réserve 39 résolveurs, 5 publieurs, 4 suiveurs à K5 et 29/10/9 à K10 [AC11 l. 22–24]. La numérotation
v11 est compatible avec le lemme (**lu**) : naissances dans [0, b) par `ranked_births`, puis chaque fusion prend
`NodeIdx node{result.count_}` en ordre de plateau (`src/tower/forest_plateau.cpp` l. 79–93) ; la racine d'une
composante est donc sa plus grande marque.

**Mesures.** Publication de l'ordre 5 : 41,8 / 34,3 / 46,0 ms à W1 ; 51,5 / 35,7 / 47,2 ms à W48 en étage séparé (c40) ;
queue W48 dans le pipeline 36,2 / 29,7 / 23,9 ms (b872). Balayage vertical de l'ordre 5 à W48 en étage séparé :
39,5 / 31,2 / 25,7 ms (**M**, [Q], [AB7] via `CARTE_V11.md` § 2.3 et § 6). Taille de l'ordre 5 sur ng00 :
438 011 plateaux, 1 350 288 traces, 576 371 nœuds (**M**, [AB7] `t_new_lidar_ng00_w1_r0.stdout`).

**Gain attendu.** Ces deux chaînes séquentielles ne sont masquées que par la résolution (86–116 ms à W48). La diviser
par 3, ce qu'exige le contrat, rend la publication de l'ordre 5 (≥ 36 ms) et son balayage (26–40 ms) **critiques** :
sans parallélisation, ~60–90 ms à W48 resteraient sur le chemin (**E**). Version parallèle : travail O(E log V) avec
E ≈ 1,35 M et V ≈ 0,58 M ; **C** de l'ordre de 5–10 ms par chaîne à 48 fils, non mesuré (« ce n'est pas encore un
résultat de performance v9 » [MAXID9]). Nécessaire, pas suffisant. À K10, rend aussi les 19 fils réservés aux
publieurs et suiveurs.

**Preuve.** Lemme : contrôle combinatoire sur 3 000 historiques abstraits
(`check_phase_a_temporal_max_id_20260923.py`), **pas** une qualification FULL. Sur LiDAR, rien n'est mesuré.

**Doctrine et risques.** Sorties identiques octet pour octet par sidecar sur les mêmes catalogues : nœuds, niveaux,
parents, `next`, ancres, contributions, populations, verticales, statuts et compteurs [MAXID9 § gardes]. Conserver
l'atomicité des plateaux, les multifusions N-aires, l'ordre des groupes dans un plateau (premier ordinal de bloc),
la priorité des refus. En v11, les cellules étendues (m > qmin) sont résolues dans la publication
(`CARTE_V11.md` § 1, étage 3e) : les résoudre avant. Une requête de composantes en O(E × niveaux) tuerait l'idée ; aucune
barrière par niveau (2 081 320 lots singletons à K10 en v9). Pas une piste fermée (aucune mosaïque, aucun catalogue
global). **Recoupe** v7-1 (MSF composables), v6-I2 (noyau DSU minimal), v10_tour_01 ; l'apport propre est le lemme qui
garantit les mêmes IDs et le réemploi de l'index pour les verticales (v7-2).

### auditeurs-03 — Census des descentes : ventiler par arité avant d'optimiser, puis extrema q2 couplés (reprise retenue)

*Catégorie : vitesse (et outillage).*

**Sources.** [HER] `q2_coupled/README.md` ; [AC11] l. 157–172 ; [P0-8] § 9 (extrema exacts sur boîtes continues ; le
maximum n'est pas aux seuls coins) ; [CC8] `P0_CENSUS_CPP_Q2.md` (juge C++ indépendant, sept mutations rejetées) et
`P0_CONSOMMATION_INDEXEE_Q2.md` l. 79–84 (« Chaque requête peut encore visiter O(n) nœuds ; un arrêt au seuil ne promet
pas O(log n) ») ; [WIP-A] R4 ; réserve sur les listes de feuilles : [IND] `q4_level_math_review_7/README.md`, dernier
paragraphe (« Une liste de feuille ne certifie pas un census au centre d'une MEB descendue hors de sa boîte »).

**Mécanisme.** (a) **Compteurs par arité** dans le census des descentes : appels, nœuds, bornes, blocs intérieurs et
extérieurs, tests ponctuels, saturations ; sans effet sur les sorties. (b) Pour une présentation q2 certifiée, préparer
C = a + b et S = |b − a|² une fois par requête, puis certifier une boîte par 2P(z) = Σ(2z_j − C_j)² − S (proche/loin
par axe) ; exemple a = (1,6,6), b = (11,6,6), boîte [2,10]×{6}² : borne actuelle P = 142, borne couplée 2P = −36.

**Dans la v11 ?** Non. `src/index/census_workspace.cpp` l. 47–64 parcourt l'index depuis la racine et appelle
`power_bound_signs` à chaque nœud ; `src/num/predicates.cpp` l. 104–143 (`bound_terms`, `center_power_bounds`) sépare les extrema ; le JSON FULL ne publie
par ordre que `census_calls` et `census_point_tests` [AB7].

**Mesures.** Census ≈ 10,3 % du CPU W1 [PROF1] ; à W1 b872 : `power_bound_signs` 4,97 %, `bound_terms` 2,02 %,
`CensusWorkspace::query` 2,14 % [AB7 `perf_new_self.stdout`] ; ordre 5 sur ng00 : 217 326 appels, 17 055 923 tests
ponctuels, soit 78,5 par appel (**M**) ; ≈ 3,9 µs par appel (**E**, `CARTE_V10_VITESSE.md` § 3.4).

**Gain attendu.** (a) rien par lui-même, mais il **décide** entre les leviers de census des autres fouilles (bornes
exactes toutes arités v6-I1/v7-3/v8-1, départ local, liste de feuille v10_moteur_06) au lieu de les essayer à
l'aveugle. (b) **E** faible : si les sphères q2 font 30–50 % des appels et que les bornes, ~moitié du coût, baissent de
24 % comme dans le modèle, −2 à −6 % du census, soit ~0,2–0,6 % du CPU W1, ~1–2 ms à W48.

**Doctrine.** Helper distinct, constantes privées, tag 2 garanti par fabrique ; ne jamais publier 2P comme `PowerBounds`
(un minimum 2P = −1 vaut P = −1/2) ; jamais pour un q3/q4 retypé à qmin = 2 ; i64 suffit (|2P| ≤ 12M² < 2^52) [HER].
Tout certificat local doit couvrir aussi les sites et boîtes interrogés hors de sa feuille [WIP-A R4]. Pas de piste
fermée.

### auditeurs-04 — Boucle chaude du catalogue sans compteurs vérifiés ni atomiques par nœud : les bornes qui rendent le port admissible (R1, R3)

*Catégorie : vitesse.*

**Sources.** [Q100] § B.1 et B.3 ; [WIP-A] R1 et R3, capsule `counter_arena/` (2 313 contrôles, `native_execution:
false`) ; mécanismes v10 G4/G6 (`CARTE_V10_VITESSE.md` § 2.1) ; [PROTO10] § 1 (feuille J3 : CPU de `t_boxes` ×1,371 à
K5, ×1,548 à K10, mêmes 13 compteurs, A/B ABBA local).

**Mécanisme.** (R1) Accumulateurs u64 locaux à la feuille, ajoutés une fois au ledger par `checked_add` avant
publication. La capsule borne chaque compte pour une feuille réelle m ≤ 1024 : census/incidences ≤ 46 821 361 844 224
(< 2^46), préfixes ≤ 45 723 987 200, tests de lignes ≤ 136 813 521 152 ; à m = 32 : 1 325 312 census. Aucun débordement
local possible ; ledger identique. `side` sans `Result` seulement sous un certificat couvrant ses coefficients, ses
sites et tous ses replis : « m ≤ 32 ne le certifie pas ». (R3) Un `Buffer` privé par tâche active, marque/rewind
du DFS ; `count×(3B−depth)` borne les listes simultanées du suffixe, pas tous les nœuds ni les émissions ; publier le
pic des réservations à part des octets utiles.

**Dans la v11 ?** Non. `src/catalogue/leaf.cpp` : `checked_add` par test de dominance (l. 41), par paire (l. 65, 69),
quatre à cinq par test de droite (l. 84–93), par jugement et par site recensé (l. 151, 156), par préfixe (l. 204, 246,
252) ; `src/catalogue/boxes.cpp` l. 60 : `Buffer::allocate` par nœud, soit deux CAS et un `fetch_sub` sur des lignes
partagées (`src/core/buffer.cpp` l. 19, 25, 47).

**Mesures.** `extend` 16,06 %, `filter` 8,00 %, `enumerate_leaf` 6,83 % du CPU à W1 [AB7] ; ≈ 0,5 milliard d'additions
vérifiées par trame (**E**, `CARTE_V10_VITESSE.md` § 3.3) ; à W48 : `buffer_acquire` + `buffer_release` 0,86 %, fautes
de page 3,1 % en cumul, `native_queued_spin_lock_slowpath` 1,55 % [PROF1] ; passe unique ×22,7–28,0 de W1 à W48 contre
×34,0 pour les boîtes v10 (**M**).

**Gain attendu.** **C** : compteurs, 5–15 % du CPU de la passe unique (≈ 9–27 ms sur 167–180 ms à W48) ; arènes, si
le passage à l'échelle remonte de ×25 à ×30, ≈ −15 % de la passe unique à W48 (≈ −25 ms). Rien n'est mesuré ; à jouer
en deux A/B séparés sur G4, sorties identiques.

**Doctrine.** Ledger publié identique octet pour octet ; réduction checked conservée ; portes m 32/33/256/1024,
compteur global proche de u64max, refus sans sortie ; pour R3, portes plafond/−1, panne d'allocation, abandon, budget
revenu à zéro [WIP-A]. Pas de piste fermée. **Recoupe** v10_moteur_05 et v10_tour_06 : l'apport propre est la preuve
de non-débordement et la règle de comptabilité qui rendent ces ports conformes à ARCHITECTURE § 4.

### auditeurs-05 — Mémo de cellule partagé, typé par sa date de validité (R5)

*Catégorie : vitesse.*

**Sources.** [WIP-A] R5 et capsule `cell_memo/` (30 contrôles Γ2 exacts, trois témoins) ; [AO11] l. 137–140 (« Un
terminal mémoïsé (b,k) vaut à coupe fermée a ≥ λ_b, ou ouverte a > λ_b. Conserver λ_b ≤ β(R) < λ_parent avant un
parent ») ; [EC10] l. 761–768 (« l'Atlas ne mémoïse pas les états profonds p ≥ K … Mesurer les répétitions sur LiDAR
avant de porter une table concurrente ») ; mesures v10 S4 (`CARTE_V10_VITESSE.md` § 1.5).

**Mécanisme.** Pour une cellule b d'ordre k, toute partie R avec |R| = k et R ⊆ P_b complet partage la composante au
niveau λ_b ; un semis trouvé pour l'une, remonté à la coupe demandée, vaut pour a ≥ λ_b (fermée) ou a > λ_b (ouverte).
Pour le prédécesseur strict d'un plateau λ, exiger λ_b < λ ; aux verticales fermées, ≤ suffit. Les témoins de la
capsule tuent les implémentations fautives : `pieces_before_cell` (X = 0, 2, 4 : connexe à la coupe fermée 4, pas à
l'ouverte), `same_date_different_cells` (X = 0, 2, 10, 12 : même date 1, fusion à 25), `invalid_terminal_date`
(X = 0, 2, 4, 6 : date de validité 9, date terminale 1).

**Dans la v11 ?** Pas dans la voie rapide : `DescentMemo` (65 536) et mémos de lane (4 096) existent
(`src/tower/descent_memo.hpp/.cpp`), mais le bit 4 est hors de 16379, `pipeline_lanes` rend 0 si un mémo est actif,
et les mémos de lane faisaient 2,6 % de succès (`CARTE_V11.md` § 5.1).

**Gain attendu.** **E** : la v11 fait 4 797 474 pas sur ng00 contre 4 399 127 en v10 (+9 %), écart attribué à l'absence
du mémo (`CARTE_V10_VITESSE.md` § 3.4) ; la v10 comptait 307 177 succès à K5 (7 % des pas) et 2,14 M à K10 (9 %).
À 0,6–0,7 µs par pas à W1, 0,4 M pas ≈ 0,25–0,3 s de CPU W1, soit ≈ 8–10 ms à W48 (÷29). Modeste ; plus utile à K10.

**Doctrine.** Certificat typé, jamais un faux résultat de descente ; conserver `DescentMemo` par tuple complet pour les
graines et les deux Level bruts ; partage concurrent publié séparément ; doit coexister avec le pipeline. Mesurer
d'abord les répétitions sur LiDAR [EC10]. Pas de piste fermée. **Recoupe** v10_tour_05.

### auditeurs-06 — Niveau q3 différé, commun au catalogue et aux MEB (reprise retenue ; port en cours)

*Catégorie : vitesse.*

**Sources.** [HER] `q3_deferred/README.md` (2 071 contrôles, 80 parties) ; [DEEP] `geometry/README.md` (feuille
accessible : Level brut 3000/464 construit puis jeté) et `performance_precision/README.md` ; [AC11] l. 109–114 et
142–155 ; [WIP-A] R2.

**Mécanisme.** Candidat q3 privé (ancre, N, D, tag 3, certificats de puissance et d'orientation) ; le Level brut
|u|²|v|²|c−b|²/(4g), degré 6/4, sans PGCD, n'est matérialisé qu'après propriétaire, census, S* et admission ; même
report dans la recherche MEB (`tower/meb.cpp`). Retaguer en q4 est interdit (puissances de 128/146 bits en u21/u24).

**Dans la v11 ?** Eager sur `origin/main` : `src/catalogue/leaf.cpp` l. 113–120 et 229–233 ; `src/num/sphere.cpp`
l. 35–52 (niveau l. 46–49). **Port en cours** [WIP-D] : `Q3Candidate`, `leaf.cpp`, `sphere.cpp`, `meb.cpp`,
`predicates.cpp`, `tests/num/q3_candidate_test.cpp`, non commis.

**Gain attendu.** **E** borné : `Sphere::through(a,b,c)` = 2,37 % du CPU à W1 [AB7], dont seule la part « niveau » des
candidats rejetés disparaît ; côté MEB, `bounded_meb` pèse 0,9 % (2,6 % avec enfants) [PROF1]. Au plus ~1–1,5 % du CPU,
≤ 4–6 ms à W48. À garder parce que sûr et déjà écrit, sans l'escompter pour les 100 ms.

**Doctrine.** Tous les champs actuels de `CatalogueLedger` restent identiques (`judged`, `census_tests`, `prefixes`,
`region_*`, `q4_candidates/q4_levels`) ; compteurs de constructions évitées dans des diagnostics séparés ; A/B natif
G4 à sorties entières égales [WIP-A R2]. Pas de piste fermée.

### auditeurs-07 — Route GPU : le contrat d'exactitude et les leçons des ports précédents (R7)

*Catégorie : architecture.*

**Sources.** [WIP-A] R7 ; [C9] l. 163 (« Les cinq ports GPU antérieurs du dépôt n'ont jamais donné plus d'environ 10 %
de gain de bout en bout »), § 6 l. 533–555 (ne pas porter avant l'expérience unique ; pas de vagues pilotées par
l'hôte ; CUDA sans `--use_fast_math`, avec `-fmad=false`, sans FTZ, sans FP64 sur le chemin chaud), § 7.2 (débit exigé
×8–38 du meilleur noyau mesuré à 100 ms) ; [L05] Q1 (feuille sur GPU estimée 17–52 ms à K5 pour 13 % d'efficacité SIMT,
non vérifiée) ; [PROTO10] § 1 (en-tête J3 compilé pour sm_120 : 126 registres, pile 5 632 octets, zéro spill —
compilation seulement) ; [AB] (frontière `prune-only` pilotée par l'hôte : 3,229 s ; parcours relancé par paire :
incompatible avec 50 k).

**Mécanisme.** Si les deux grands postes CPU ne tiennent pas ~50 ms chacun, la passe unique du catalogue (feuilles
indépendantes ≤ 16 sites à K5, i64/i128 ; 4,75 s de CPU W1 sur ng00) est la candidate. Contrat des auditeurs : « Centres
i128 ne signifient pas catalogue i128 : q3 conserve checked/Wide, puissance 134/152 bits et niveaux jusqu'à 180/134 ou
204/152 en u21/u24. Exact device, ou `unresolved` repris exactement sur CPU avant admission ; un débordement/refus
n'est jamais un rejet géométrique » ; S*, ordre, contacts, sentinelles, baux et refus transactionnels conservés ;
host/pinned/device budgétés ensemble ; préparation, transferts, retour et canonicalisation chronométrés.

**Dans la v11 ?** Non (aucune option CUDA).

**Gain attendu.** **C**. Historique défavorable (≤ ~10 % de bout en bout sur cinq ports) ; seule une route conçue de
bout en bout, résidente, comparée à son témoin CPU avec coûts producteurs, transferts et sortie, donnera un chrono.

**Doctrine.** Identité octet pour octet avec le CPU ; portes CPU/device puis FULL identique ; jamais de vague par
niveau pilotée par l'hôte. Les routes GPU fermées (Geogram/PDEL, frontière `prune-only`) le sont pour leurs mécanismes,
pas pour le GPU [AB]. **Recoupe** v6-I5 (coût mesuré et couture C6) et v10_moteur_07.

### auditeurs-08 — Sortie plate E1 : deux corrections de protocole avant P08 (bras z = 2 contre l'oracle ; borne IC de H_L2)

*Catégorie : outillage_tests (sortie plate).*

**Sources.** [WIP-A] : note moteur en préparation (paragraphes « Avant les mesures primaires z = 2, compléter la
porte » et « Avant P08, appliquer tout le critère H_L2 ») et `audit_selfreview_20261004/README.md` § « Deux conditions
avant les campagnes E1 », capsule `flat_verdict/` (89 gardes stdlib/AST : le helper rend `claimed=True` pour
p_Holm = 0,0300969903 et IC = [−0,021 ; −0,019] ; une borne exactement −0,02 passe aussi) ; préenregistrement
`plans/e1_prereg_lidar_20261004.json` (H_L2 : borne basse IC95 % strictement > −0,02, en plus de Holm).

**Dans la v11 ?** Non, vérifié sur `origin/main` `2b1abb6a5` : `bench/points_flat_gate.py` l. 38
`LINES = (('eom', 1), ('eom', 3), ('leaf', 1))` — pas de z = 2, alors que `lidar_decision` teste `T_eom2` en primaire
(`bench/points_flat_summary.py` l. 220) ; l. 237 `rows[name]['claimed'] = adj < 0.05`, sans condition d'IC.

**Gain attendu.** Aucun en vitesse ; évite une revendication P08 fausse et qualifie le bras primaire contre l'oracle
indépendant. Coût : une heure et un rejeu G4 de la porte plate.

**Doctrine.** Fixture à la frontière −0,02 ; p et IC gardés séparés ; bras z = 2 contre le même oracle et ses gardes
algébriques ; z = 1/2/3 seulement (l'auditeur réfute « toute EOM, à tout z » par un témoin à z = 16, 504 gardes
Fraction [WIP-A]).

### auditeurs-09 — Port natif de la hiérarchie de points et de la tête plate : le contrat déjà établi par les auditeurs

*Catégorie : points_clustering.*

**Sources.** [AC11] l. 213–239 (« Arbre de points N-aire, après suppression des vides/unaires : ≤ 2n−1 nœuds. Le
produire depuis FULL et les attaches, sans matrice n² ni liste de membres par ancêtre. DP : score/décision par cluster,
puis un passage d'émission des labels » ; PointRadiusDate à trois rangs ; majorants des niveaux 156/116, 180/134,
204/152 bits ; produits de comparaison à quatre racines 2022/2334/2646 bits ; « Wide2048 ne couvre pas le majorant u21
complet » ; 8192 bits comme budget avec refus) ; `receipts/points_answers_20261003/root/Q8_CONTRAT.md` (comparateur
√n1+√n2 contre √n3+√n4 par W = 4(x−y) − U², puis W² contre 16U²y, gardes de signe avant les carrés) ; [AV11] l. 68–88
(égalité EOM certifiable par 1/e et au plus quatre classes carrées pour la forme H ; 888 gardes) ;
`receipts/flat_model_followup_20261004/` (raffinement EOM en z prouvé à arbre et cohortes fixés, 12 917 gardes) ; [EC10]
l. 1135–1138 (LCA d'une antichaîne par deux extrêmes d'Euler, sans tri ni stockage).

**Dans la v11 ?** Python seulement : `bench/points_radius.py` (LCA, `sqrt_cmp2`, classes de radicaux, budget 8192 et
`Refusal`), `bench/points_flat.py`. Le « un LCA par point » du critère privé B [AV11 l. 12–21] est déjà la forme de la
règle retenue H^r (`order.lca` dans `points_radius.py`) : rien à reprendre de ce côté.

**Mesures.** Sur c08_000054 (39 873 sites), H^r (`margin_r`) coûte 3,1 s à k = 5 et 7,4 s à k = 10, contre 4,2 s pour
`sklearn.cluster.HDBSCAN` dans la même campagne (**M** sous contention, [PTS4] `case_metadata.json.gz`).

**Gain attendu.** **C** : ≥ ×10 en natif par rangs entiers et arbre linéaire ; hors du chrono FULL, mais nécessaire aux
campagnes (P08 : 141 trames × ordres) et à une comparaison de temps équitable avec HDBSCAN.

**Doctrine.** Dates exactes par rangs, aucune décision flottante sans borne prouvée, refus au budget, mémoire linéaire
comptée avec FULL. Pas de piste fermée.

---

## 3. Fausses bonnes idées écartées

| Idée | Pourquoi l'écarter | Source |
| --- | --- | --- |
| Sous-maille des centres T6 (verrou 6 de [Q100]) | Neutre sur LiDAR : lidar02_full K5, T = 6/3/0 → même dump `8a850649ff10`, nœuds 734 083 / 734 261 / 735 601 (+0,2 %), triplets 28,59 / 28,60 / 28,67 M, `t_boxes` 2,84 / 2,80 / 2,74 s (bruité) ; idem à K10 ; seule la grille dense 16³ change (**M**). Comptes G4 v10 (T6) / v11 (T0) : 781 865 / 783 071 nœuds. Copier kT = 6 en u24 casse la garde i64 2(B+T)+5 ≤ 63. Utile seulement aux grilles synthétiques denses ; les bornes R6 [WIP-A] restent prêtes si besoin | [L05] L05-15 et `ablation_kT.txt` ; `CARTE_V10_VITESSE.md` G1 ; [DEEP] `performance_precision` |
| Minorant commun des extensions q4 (témoins coplanaires, ligne propriétaire) | Signe à 153/177/201 bits (3–4 mots) par site et par triplet, « ne bénéficie pas de la voie q4 i128 » ; pour ~0,33 quadruplet par triplet (9,38 M / 28,59 M, trame 02 K5) → le certificat coûte plus qu'il n'évite (**E**) | [IND] `q4_level_math_review_7`, `q4_owner_line_witness_review_8` ; [PROTO10] § 1 ; [PF3] |
| Crédit par moments de groupe, palette ponctuelle | Shadow négatif sur les rectangles lourds ; palette : une arête fermée sur 55 657 ; en v10, premières feuilles M16 seulement, aucun gain mesuré | [B9] l. 95–104 ; [PC9] l. 84–88 ; [EC10] l. 1114–1128 |
| Index des selles (lemme A de D5) | Évite 1,01 M MEB sur 2,70 M, mais construire et trier 10,2 M entrées coûte autant : code retiré (**M**) ; la v11 a déjà la table de populations | [C9] l. 264 |
| MEB « support + extérieur » inspirée de la v10 | Modèle sur 108 fixtures : présentations 341 → 392, tests 828 → 1 294 ; la variante q3 directe (291/770) régresse dans trois cas | [AO11] l. 89–96 |
| Saut par orthants (borne O(B) des sauts saturés) | Pas un levier K5 : 1,32 pas par trace sur ng00 (4 797 474 / 3 622 258) ; le coût est par pas, pas leur nombre. Garder comme garantie si une famille pathologique apparaît | [EC10] l. 651–661 ; [AB7] |
| Partager la géométrie entre les K forêts | « Ne pas chercher K copies d'un même calcul géométrique : elles n'existent pas sous cette forme » : une boule régulière ne travaille qu'aux ordres m−1 et m | [PART9] l. 10–11 |
| Deux cases par boule pour les tableaux par ordre | v11 : `kinds` (u8) et `dense_` (u32) de taille B pour chaque ordre ≥ 2 (`forest_internal.hpp` l. 136, `forest_build.cpp` l. 211–235) ; K5 ≈ 26 Mo, gain ≈ 15 Mo et < 1 ms ; K10 ≈ 248 Mo, gain **E** ≈ 200 Mo et ~20 ms à W48 (classification 55 M → 11 M examens). Hors contrat K5 ; à garder pour K10 et le massif | [PART9] § 3 |
| Filtre flottant du census | « recensement : aucun gain » ; F6 v11 ≈ 1 %, retiré | [L05] L05-09 ; `docs/PERFORMANCE_FULL.md` l. 186–188 |
| `-march`, mémos de lane, préchargement DSU | mesurés sans gain sur G4 | `CARTE_V11.md` § 5.3 |
| Fronts en vagues ou en largeur à barrières | vagues compactes 6–8 % plus lentes que le DFS ; frontière v10 ×2,1 de 1 à 48 fils | [GS9] l. 152–155 ; `CARTE_V10_VITESSE.md` N2 |
| Projeter en CPU·s/48 ou sur 44 fils | SMT mesuré ×1,22–1,75 de 24 à 48 fils | [C9] l. 174, 538–543 |
| Verdict « 100 ms impossible » sans borne inférieure | probabilités < 0,02 réfutées par l'architecture suivante | [C9] § 1 ; [B9] l. 17–25 |
| Euler seul, ou catalogue scellé sans juge d'échantillon | Euler aveugle aux ordres Kmax−1 et Kmax (33 % des boules à K5) | [C9] § 1 point 5, § 6 |
| Compter sur la frontière v3b pour le catalogue | `t_frontier` ×2 en local, mais catalogue entier 1,027 / 0,906 (K5/K10 à 4 fils, régression K10 conservée) ; projection G4 ×1,13–1,15 seulement, non mesurée | [PROTO10] § 2 |

---

## 4. Défauts récurrents relevés par les auditeurs, et leur état dans la v11

| Défaut | Où il a été relevé | État v11 |
| --- | --- | --- |
| Porte vacueuse (code ignoré, `WILL_FAIL`, regex littérale) | [PF3] « Les deux motifs d'échec » ; [L05] § 5.5 (une seule porte catalogue v10) | portes à code exact (`cmake/run_expect.cmake`), planchers ; en place |
| Mutant « tué » par signal ou délai, sélection vide acceptée | [EC10] l. 749–760 (collecteur R2) ; [PROTO10] § 1 (juge de fuzz acceptant deux délais symétriques) | `tests/mutants/run_mutants.py` : comptés à part (`dont_signal`, `dont_delai`), plancher (code 3) ; partiellement corrigé |
| Certificat plus cher que ce qu'il évite | [PF3], [PF6], [B9], [C9] l. 264 | règle à appliquer aux idées 03, 05, 06 |
| Compteur logique présenté comme un temps | [DEEP] `performance/README.md` l. 49 ; [AC11] l. 95–98 | reçus v11 séparés ; à maintenir |
| Captures non appariées (passe chaude v10, processus neufs v11) | [DEEP] ; [AC11] l. 83–90 | A/B v10/v11 apparié toujours ouvert |
| Réservations prises pour une RSS ; pic par étage absent | [DEEP] ; [AC11] l. 126 ; `CARTE_V11.md` § 3 | ouvert (ARCHITECTURE § 7.1) |
| Arrondi supposé, marges flottantes figées u18 | `CARTE_V10_VITESSE.md` N1 | doctrine F1–F6 de la v11 |
| Course du Pool | correctif v10 `8e3b76245` | TSan v11 (7/7 b872 ; matrice `claudequal2`) |
| Bras diagnostique W8 supprimé par un échec W48 ; garde Σ tâches ≤ min(W, J)·mur absente | [IND] `parallel_diagnostics_review_18` | à vérifier dans les bancs d'occupation à venir (v8-3, v9-03) |
| Porte Python important `numpy` sur le Python nu de la G4 | session `claudequal1` (`receipts/developpement_20261004/qualification_p1p2/README.md`) | corrigé (bibliothèque standard seule) |
| Trois trames d'une séquence pour un contrat de plage | [C9] § 7.4 ; [AC11] l. 128–134 ; [PTS4] | ouvert → idée 01 |

---

## 5. Pistes nommées par les auditeurs, non instruites (à ne pas compter)

- **Générateur émettant par niveau**, seule façon de recouvrir catalogue et forêts : « Aucune proposition n'existe ;
  c'est une piste pour 100 ms, pas un prérequis démontré. À n'ouvrir qu'avec un théorème de complétude par niveau »
  [C9 l. 620]. En v11, le domaine (219–253 ms) doit finir avant les forêts (tri des niveaux, assemblage, table des
  populations). Aucun gain ne doit en être escompté sans théorème.
- **Réfutation des ancres longues** et **auto-jointure dual-tree à range-add** [C9 § 8 ; PF3] : propres aux
  architectures WSPD v3–v9, sans objet dans la v11 (boîtes de centres).
- **Clé exacte par support hors coquilles étendues** [C9 § 8] : déjà la forme v11 (S* canonique, `find_support`).

---

## 6. Ordre conseillé

1. **auditeurs-01** avant tout levier : une session G4 de mesure qui fixe la vraie cible (maximum des trames de la
   plage, plusieurs séquences).
2. **auditeurs-08** (une heure, avant P08).
3. Mesures séparées, puis combinées, sur G4 : compteurs par arité (**03**), compteurs locaux et arènes (**04**),
   q3 différé déjà écrit (**06**).
4. Conception de **auditeurs-02** (publication et verticales parallèles à IDs identiques), indispensable dès que la
   résolution baisse.
5. **auditeurs-05** après mesure des répétitions sur LiDAR.
6. **auditeurs-07** si les postes CPU ne tiennent pas leur part.
7. **auditeurs-09** après le choix de la tête plate.

---

## Annexe A — Borne haute du contrat

`python3 -B auditeurs_pts4_borne.py` (lecture `git show origin/main:…/case_metadata.json.gz`, aucune écriture). Sortie
du 4 octobre, 13 h 00 UTC :

```text
trames c08 distinctes 127 sites 32462 - 98560
au-dessus de 60 000 sites : 85
pente noeuds ordre 5 ~ n^1.386 (trames)
pente boules K10 ~ n^1.439 (cas)
pente full_ns K10 ~ n^1.070 (cas, sous contention, non contractuel)
[30000, 40000) : 12 trames ; noeuds ordre 5 med 449807 max 465522 ; med/ref 0.78 max/ref 0.81
[40000, 50000) : 25 trames ; noeuds ordre 5 med 532098 max 775715 ; med/ref 0.92 max/ref 1.35
[50000, 60001) : 5 trames ; noeuds ordre 5 med 751226 max 1076856 ; med/ref 1.30 max/ref 1.87
trame la plus lourde <= 60 000 : c08_003412, 58418 sites, 1076856 noeuds ordre 5 (x1.87), 11066031 boules K10 (x2.01)
cible effective sur 08/000000 si temps ~ noeuds ordre 5 : 54 ms ; ecart a la mediane 412.4 ms : x7.7
```

Références : 576 371 nœuds d'ordre 5 de 08/000000 ([DEEP] `performance/README.md`), 5 512 670 boules K10 (v10 S4),
médiane FULL 412,4 ms ([AB7]). Les nœuds d'ordre 5 d'une tour K10 égalent ceux d'une tour K5 (même objet, ordres 1..5).

## Annexe B — Sources lues (en plus de celles du tableau)

- v11 : `audits/README.md`, `REPONSE_CLAUDE_POINTS_20261003.md` ; reçus `audit_giant_20261004/` (README, `mathematics/`,
  `g4_protocol/`), `audit_independant_20261002/README.md` et les revues 7, 8, 10, 17, 18 citées ;
  `developpement_20261004/qualification_p1p2/README.md` ; code `src/catalogue/leaf.cpp`, `src/num/sphere.cpp`,
  `src/index/census.cpp`, `src/index/census_workspace.cpp`, `src/tower/forest_build.cpp`, `forest_plateau.cpp`,
  `forest_internal.hpp`, `bench/points_radius.py`, `bench/points_flat_gate.py`, `bench/points_flat_summary.py`,
  `tests/mutants/run_mutants.py`, `tools/g4_matrix.py` ; archive [AB7] (`t_new_lidar_ng00_w1_r0.stdout`,
  `perf_new_self.stdout`).
- v10 : `audits/AUDIT_ETAT_COURANT.md` (l. 1–172, 636–700, 740–800, 850–890, 1110–1160), `AUDIT_MASSIF_LIDAR_20260930.md`,
  `audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md`, reçu `sitetree_corrected` ;
  `build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md` et ses preuves.
- v9 : `AUDIT_B_GAINS_STRUCTURELS_100MS_20260926.md`, `PLAN_CRITIQUE_100MS_FULL_20260924.md`,
  `CONTRE_AUDIT_B_GRAND_AUDIT_C_100MS_20260924.md`, `CONTRE_AUDIT_FULL_R20_OCTETS_100MS_20260924.md`,
  `AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md` (§ 1–3.1, 6–8), `FULL_PARTAGE_INTER_ORDRES_20260926.md`,
  `PHASE_A_MAX_ID_COMPOSANTE_20260923.md`, `DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md` (début),
  `OBSTACLES_GPU_SOUS_SECONDE_20260923.md` (début) ; `build/v10-persist/audit_v9/L15_heritage/NOTES.md`.
- v8 : `audits/P0_SOUS_RECTANGLES_ET_GROUPES.md` § 9, `P0_TUBES_ET_RANGS.md` (début) ; audits racine
  `morsehgp3D_v8_complementaire/P0_CENSUS_CPP_Q2.md`, `P0_CONSOMMATION_INDEXEE_Q2.md`.
- v7 : `audits/CACHE_FULL_COURANT.md` ; v3/v6 : pistes fermées ; `docs/archive/abandoned/README.md` ; titres des
  fichiers de coordination racine V7, V8, V9 (architectures WSPD, peu transposables).

FIN
