# Tranche T2-c : cout de l'etage G (voie CPU du produit) -- rapport tenu au fil de l'eau

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
objet=full_pi0
quantification=quantized_u21_input_only (profils 21, 24, 32 compiles et testes)
public_status=not_claimed
GCP non utilise par moi (le developpeur joue les sessions G4)
```

- HEAD de base au depart : `9c5809919fa4fcaa6303f9265aadbad60a7ddeea` (copie `git archive HEAD morsehgp3D_v12` dans
  `repo/`) ; **base de la serie livree : `781fbe8d13e943bc67cbcf3996d6791523f1a422`** (deux rebasages : 6f362f0bf puis
  781fbe8d1, voir plus bas). Livrables : `patch_tour_Gc_1_empreinte.diff` (prealable), `patch_tour_Gc_2_leviers.diff`,
  `patch_tour_Gc.diff` (cumul), `plan_t2c_g.json`, `v12_src_avant_t2c.tar.gz`, pilote dans le patch 2
  (`microbancs/mes_t2c_g/pilote_t2c.py`), ce rapport.
- Debut : 2026-10-07 21:36 UTC (lu par `date -u`).

## Journal

- 21:36 UTC : consignes lues, copie extraite.
- 21:41 UTC : base construite (`build_base`, `-DMHGP12_MODULES=tower`, profil 21) ; ng00 K5 a un fil (taskset, charge
  du codespace 4 a 7) : resolution 3,97 s puis 4,97 s (bruit), empreinte `231d826bb0d4fe57` (= porte). Lectures faites :
  CONTRAT_TOUR, PLAN, src/tower, portes, rapport de G, recu G4 t2e, microbanc (replique, table S* de hachage, table de
  populations v11 a lignes contigues K+3 mots).
- Premieres hypotheses de cout unitaire (a mesurer) : (a) sonde produit = case (defaut) -> birth_keys -> balls_ et
  population_offsets -> valeurs de la CSR : 3 a 4 defauts DEPENDANTS, contre 2 pour la table v11 (case -> ligne
  contigue) ; puis controle du rang par balls_[birth_keys[i]] (2 de plus) alors que ResolvedOrder::birth_ranks existe ;
  (b) LEM-T1 : `Catalogue::find_support` cherche par dichotomie dans la ligne CSR du premier site en lisant balls_
  a chaque pas (~6 defauts dependants), contre une table de hachage a 2 defauts dans la replique ; (c) F dans P_b :
  deux recherches binaires dans la CSR.
- 21:51 UTC : **instrumentation livree** (option de construction `MHGP12_TOWER_PROFILE`, sans option CMake : la
  construction de profil se configure avec `-DCMAKE_CXX_FLAGS=-DMHGP12_TOWER_PROFILE`) : `src/tower/profile.hpp`
  (lecture du compteur encadree par lfence, sections imputees par `SectionClock::lap`), sections dans `resolve.cpp`
  (trace, sonde, proposition, LEM-T1, certificat, repli, census sature, census complet, pas, arret) et total des
  tranches dans `stage.cpp` ; `ResolutionDiagnostics` recoit `table_ns[k]`, `profiled`, `profile[k]` (seul ajout a
  `tower.hpp`, declare) ; la sonde ecrit une ligne `profil_g` par ordre avec le biais des sections (62 cycles ici) et
  `table_ns` dans sa ligne `tour_g`. Hors construction de profil : aucune instruction ajoutee (sections vides).
  Empreinte de la construction de profil : `231d826bb0d4fe57` (inchangee).
- Profil local de la BASE (ng00 K5, un fil, charge 7 a 8 ; ns nets du biais par occurrence) :

| k | trace | sonde | proposition | LEM-T1 | certificat | census sature | census complet | arret | total ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 18,4 % (60 ns) | 49,2 % (193 ns) | 4,8 % (126 ns) | 17,7 % (540 ns) | 0,4 % | 3,2 % (2,5 us) | 0 | 5,8 % (228 ns) | 189 |
| 3 | 12,3 % (44 ns) | 40,8 % (188 ns) | 8,1 % (160 ns) | 20,9 % (453 ns) | 1,3 % | 9,0 % (2,6 us) | 0 | 6,9 % (202 ns) | 379 |
| 4 | 9,0 % (44 ns) | 31,4 % (193 ns) | 9,5 % (211 ns) | 18,8 % (443 ns) | 3,2 % | 21,2 % (3,2 us) | 0,3 % | 5,9 % (202 ns) | 758 |
| 5 | 5,8 % (62 ns) | 20,3 % (244 ns) | 8,4 % (329 ns) | 11,8 % (473 ns) | 6,1 % (641 ns) | 27,4 % (4,3 us) | 16,8 % (5,9 us) | 3,4 % (212 ns) | 2041 |
| 2..5 | 8,0 % | 26,7 % | 8,4 % | 14,7 % | 4,6 % | 22,6 % | 10,3 % | 4,5 % | 3367 |

  Lecture : (1) LEM-T1 (`find_support` par dichotomie indirecte dans `balls_`, puis F dans P_b) coute plus que la
  proposition flottante elle-meme (440 a 540 ns contre 126 a 329) : c'est l'ecart avec la replique (table S* de
  hachage) ; la table S* est au catalogue (module d'un autre agent, T1-b) : recommandation, pas de modification ici.
  (2) La sonde produit (190 a 245 ns) n'est PAS plus chere que celle de la replique en local (285 ns) : l'hypothese
  « verification contre la CSR » n'explique pas l'ecart. (3) Census garde 1,6 a 2 fois plus cher que le census v11 de la
  replique (sature 2,5 a 4,3 us contre 2,0 ; complet 5,9 contre 2,8) : module index, a mesurer a part. (4) Le parcours
  du census, s'il partait de la feuille du premier site (G-L7), changerait `census_nodes` et `census_sites`, compteurs du
  travail DANS l'empreinte de la resolution (section CNTR de l'export) : interdit par la consigne (lignes des portes
  inchangees) ; non fait.
- 22:03 UTC : premiere version de l'index des naissances (seaux par bits de tete de l'empreinte additive, repertoire,
  lignes contigues, construction parallele deterministe par tri par base stable) et de G-L5 (empreintes des traces, tri
  par seau, fusion, verification dans la passe de resolution) : **empreinte de ng00 K5 inchangee** (`231d826bb0d4fe57`,
  compteurs du travail compris). Chronometrage fin a un fil (ordre 5) : index 2,3 + 20,4 + 5,6 + 16,4 ms (comptage,
  populations, tri, disposition), jointure 23,7 + 29,6 + 11,4 ms (empreintes, tri, fusion) ; sonde par representant
  116 a 135 ns (verification : repertoire, naissance et rang non precharges). Gain net faible a ce stade : a reprendre
  (enregistrements contigus cle + naissance + rang + ligne, tri a chiffres de 8 bits, empreintes de la coquille par
  cellule).
- 22:10 UTC : index a fiches contigues (empreinte, naissance, rang, population : une ligne par place, prechargee 8
  representants en avance) ; tri a chiffres de 8 bits ; empreintes de la coquille calculees une fois par cellule.
  Sonde par representant 47 a 72 ns aux ordres 3 a 5 (base 188 a 244 ns) ; empreinte toujours `231d826bb0d4fe57`.
- 22:20 UTC : A/B local a UN fil, ng00 K5, 3 tours de processus alternes, minimum des passes par processus (indicatif,
  charge 5 a 8) : etage G base 2 641 ms (min ; mediane 3 565), **index + G-L5 2 304 ms** (rapport median 0,724, bruite),
  index sans G-L5 (ablation : chaque representant sonde l'index) 2 457 ms (0,946). Index : 92 ms a un fil (base,
  table sequentielle par ordre : 65 ms) ; jointure G-L5 : 157 ms a un fil (empreintes, tri, fusion).
- 22:24 UTC : A/B local a TROIS fils (taskset 5-7, 4 tours, 3 passes, min par processus) : base 1 170 ms, index + G-L5
  1 050 ms (-10 %), index sans G-L5 1 063 ms (-9 %) : **la jointure triee n'apporte rien a trois fils sur le CPU**
  (son tri par base, limite par la bande passante, coute ce qu'il economise : 79 ms de jointure a trois fils, 33 ms a
  l'ordre 5, deux fois moins qu'a un fil seulement) ; le gain vient de l'index compact et parallele (sonde dans le
  cache de dernier niveau au lieu de trois defauts dependants). Piste suivante : G-L7 (sondes prechargees en file dans
  la passe de resolution, sans tri), a mesurer contre G-L5.

## Chiffres G4 recus du developpeur (session `v12.20261007.t2h`, produit au commit `99fa83246`, base de cette tranche)

Sonde `mhgp12_tower_probe`, G4, medianes des passes chaudes, ms (transmis le 7 octobre vers 22:25 UTC, non joues par moi) :

| cas | fils | G | count | fill | tables | resolve | par ordre |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| ng00 K5 | 48 | 80,1 | 0,6 | 1,4 | 18,6 | 48,7 | 0,2 / 3,2 / 6,6 / 11,9 / 26,1 |
| ng01 K5 | 48 | 63,2 | | | 15,4 | 37,0 | |
| ng02 K5 | 48 | 75,7 | | | 19,0 | 43,5 | |
| ng00 K5 | 1 | 1 567 | 15,7 | 24,2 | 40,3 | 1 476 | 2 / 90 / 190 / 361 / 833 |
| ng00 K10 | 48 | 633,6 | | | 120,6 | 437,9 | ordre 10 : 159 |
| ng01 K10 | 48 | 449,3 | | | 82,4 | 307,7 | |
| ng02 K10 | 48 | 517,5 | | | 101,0 | 342,0 | |
| uniformes K5 8 000 / 16 000 / 32 000 | 48 | 31,2 / 75,4 / 181,9 | | | | | |

Empreintes identiques aux portes locales. **Lecture du developpeur** : la resolution passe a l'echelle (x30 de 1 a 48
fils) mais les TABLES ne gagnent que x2,2 (une tache par ordre, insertion sequentielle : le chemin critique est la table
de l'ordre K) : ~25 % de G a K5, ~20 % a K10 ; budget 25 a 30 ms a K5 : il faut des tables paralleles ou residentes
(<= 2 a 3 ms) ET une resolution environ deux fois moins chere par unite (G-L5, LEM-T1 avec moins de defauts de cache).

**Ordre des leviers retenu en consequence** (regle d'adoption inchangee, ecrite d'avance dans le pilote) :
1. index des naissances parallele et deterministe (remplace les tables : vise <= 2 a 3 ms a 48 fils) ;
2. sonde de l'index sans la CSR (fait : fiche contigue) et premieres sondes (G-L5 triee, ou file prechargee G-L7 si
   elle gagne en local : la jointure triee ne gagne rien a 3 fils) ;
3. LEM-T1 : table S* -> boule a deux defauts au lieu de la dichotomie indirecte du catalogue (~ 15 a 20 % de la
   resolution a un fil) ;
4. le census (30 a 35 % de la resolution) est au module index et ses compteurs (`census_nodes`, `census_sites`) sont
   dans l'empreinte : hors de cette tranche, signale.

## Decision du coordinateur (22:35 UTC) et rebasage

- **Empreinte = objet seul** (contrat § 8) : `resolution_digest` ne hache plus que naissances, cellules, traces, cibles
  et compteurs de l'objet ; les compteurs du travail restent dans les lignes « ordre » et res.bin, compares ligne a
  ligne entre nombres de fils (W1 = W48) hors empreinte. **Patch 1 separe et prealable** aux leviers, avec la preuve
  que l'objet n'a pas change. Ensuite les leviers peuvent changer le travail. Le census (module index) peut etre
  touche si le gain le justifie, portes index vertes.
- `main` a avance pendant le travail : 9c5809919 -> 136e07628 (recu H, juge de determinisme durci par le developpeur :
  schema strict, ligne exit, fausse sonde et 5 portes) -> 6f362f0bf (audits : relecture des mesures H ; proposition
  G-L5 de l'auditeur : jointure par cle COMPLETE, fusion monotone, refus des naissances en double, borne O(k(B+Q))
  meme si toutes les empreintes sont egales ; lecture du prototype Gc : frontieres des chronos changees, reste de
  9 a 11 ms hors count/fill/tables/resolve a 48 fils dans le produit H (check_catalogue sequentiel, points, admissions,
  espaces de census, liberations)). Seuls `tests/tower/{g_determinism.py, g_fausse_sonde.py, tests.cmake}` et des
  documents ont change dans morsehgp3D_v12 ; `src/` et `bench/` sont identiques a 9c5809919. **Rebasage de la serie
  sur le HEAD courant.**
- Premiere version du patch 1 (sur 9c5809919, abandonnee pour le rebasage) : portes de la tour 20/20 vertes, export
  res.bin et cat.bin identiques a l'octet avant/apres (ng00 K5 `414cc189...`, u8000 `e3e163f4...`), oracle inchange.

## Patch 1 (prealable) : empreinte de l'objet seul — `patch_tour_Gc_1_empreinte.diff`

- Base : `6f362f0bf4806d410657da8b3cdf36836837eb4f` ; `git apply --check` vert sur ce HEAD (22:45 UTC).
- `bench/tower_export.hpp` : `resolution_digest` hache les sections de l'objet (BKEY, BRNK, CBAL, CRNK, CFLG, COFF,
  TMSK, TARG) et une section COBJ des 5 compteurs de l'objet au lieu de CNTR ; res.bin garde CNTR complet (lecteur
  g_dump inchange). En-tete de la sonde mis a jour.
- `tests/tower/g_determinism.py` (version durcie du developpeur conservee) : objet (empreinte + compteurs de l'objet)
  puis travail compares separement, ligne a ligne, ordre par ordre ; message qui dit lequel differe.
- `tests/tower/g_fausse_sonde.py` : modes `travail_8` et `objet_8` ; deux portes `mhgp12_tower_juge_ecart_*` (code 1).
- Lignes des portes (16 hexadecimaux conserves, choix du developpeur) : u8000 `a40f1b2ef8547269`, u16000
  `cf7c7745fcb4ae6e`, u32000 `d1f08fd0dbdf48eb`, ng00 K5 `e5a81154fb1b15f1` ; comptes de l'objet inchanges.
- **Preuve que l'objet n'a pas change** : exports res.bin (tous les compteurs) et cat.bin identiques a l'octet avant et
  apres (ng00 K5 `414cc189b346...`, u8000 `e3e163f44541...`) ; oracle borne inchange (`mhgp12_tower_oracle` et `_opt`
  verts sur la meme ligne : 342 nuages, 21 225 cibles) ; portes d'echelle 1 contre 8 fils et lidar ng00 vertes avec les
  memes comptes ; aucun fichier du produit (src/) touche, donc MES-M0 inchange par construction.
- Portes de la tour sur le patch 1 : **34/34** (unitaires, oracle, juge temoin et 4 refus, 2 ecarts, echelle 8 000 /
  16 000 / 32 000, lidar ng00), `check_style` 267 fichiers, `check_constats` verts.

## G-L5 (jointure triee) contre G-L7 (file de sondes prechargees), local, trois fils

22:50 UTC, ng00 K5, taskset 5-7, 4 tours x 3 passes, minimum par processus, charge 8 a 9,5 (indicatif) :

| bras | G min (ms) | mediane | resolve min | rapport median au base |
| --- | ---: | ---: | ---: | ---: |
| base (99fa83246) | 1 119 | 1 654 | 1 039 | 1 |
| index + G-L5 (jointure triee, 4 ordres : 12 + 16 + 19 + 33 ms de jointure) | 1 166 | 1 505 | 1 126 | 0,965 |
| index + G-L7 (file de 16 representants : case du repertoire prechargee a l'entree, fiches a mi-file) | **911** | 1 295 | 873 | **0,914** |

**Decision locale (indicative, a juger sur G4)** : la voie produit du CPU est **G-L7** (aucune memoire en plus, pas de
tri : chaque sonde tombe dans l'index compact, prechargee 8 a 16 representants a l'avance). G-L5 reste ecrit, teste
(portes de l'index : candidats de la jointure egaux a ceux de la sonde par representant, resolution complete rejouee a
masque nul) et rejouable par un bras du pilote (substitution d'une ligne) : c'est la voie de l'appareil, pas celle de
l'hote. Recherche bornee sous collisions (proposition de l'auditeur) : index dans l'ordre canonique (empreinte,
population), dichotomie dans le seau, O(k log E) meme a masque nul ; population repetee refusee.

## Patch 2 (leviers) — etat a 23:00 UTC (copie `repo2/` = base + patch 1 + leviers)

- **Index des naissances** (`populations.cpp`, remplace la table a sondage lineaire) : ordre canonique (empreinte
  additive, population), repertoire des seaux, fiches contigues (empreinte, naissance, rang, population) ; construction
  parallele et deterministe (comptage par blocs, tri par base stable sur les bits du seau, tri de chaque seau, fiches) ;
  recherche par dichotomie dans le seau (O(k log E) meme a masque nul) ; population repetee refusee ; tampons gardes
  d'un ordre a l'autre (un seul objet pour tous les ordres).
- **Premieres sondes** : voie produit **G-L7** (`passes.cpp`, file de 16 representants par tranche, case du repertoire
  prechargee a l'entree, fiches a mi-file) ; **G-L5** (`first_probes.cpp`, jointure triee) ecrite, testee et jouee par
  un bras du pilote (une ligne substituee).
- **Table S* -> boule du catalogue** (`catalogue/table.cpp`, `catalogue.hpp`, `internal.hpp`) : entree contigue (queue
  de S*, boule) de 16 octets au lieu d'un BallIdx compare via `balls_` a chaque pas de dichotomie ; meme ordre des
  lignes ; S* repetes refuses (catalogue_invariant). **Touche le module catalogue (T1-b de l'agent GPU) : a fusionner
  par le developpeur ; l'export et l'empreinte du catalogue n'en dependent pas.** +12 octets par boule (+15,7 Mo a K5
  sur ng00 : 1 306 696 boules).
- **Certificat** (`supports.cpp`) : patch propose par l'auditeur (`audit_g_pistes_20261007`) applique : pas de predicat
  `side` pour les sites de S (deja sur la sphere certifiee).
- **Frontieres des chronos** (`tower.hpp`, `stage.cpp`, `passes.cpp`, sonde) : prepare, count, setup, fill, workspace,
  puis par ordre table / join / pass dans order ; `resolve_ns` = somme des passes (exclusif) ; `reste_ns` publie par la
  sonde. **Preparation parallele** (controle des incidences et des S*, points exacts) : le reste du mur de H (9 a 11 ms a
  48 fils) contenait ces balayages sequentiels.
- **Corrections gratuites** : rang de la naissance lu dans la fiche (plus `balls_[birth_keys[i]]`), lectures de la CSR
  differees aux branches qui en ont besoin, cible de fenetre lue sans le tableau des premiers ordres.
- Compteurs : les compteurs de l'objet ET du travail restent identiques a ceux de la base (verifie sur ng00 K5 :
  lignes « ordre » identiques octet pour octet) ; empreinte de l'objet identique (`e5a81154fb1b15f1` sur ng00 K5).
- Portes locales (profil 21, `-DMHGP12_MODULES=tower`) : 38/38 avant les derniers ajouts (echelle 8 000 / 16 000 /
  32 000 et lidar ng00 aux empreintes du patch 1, oracle, index, juge) ; nouvelles portes `support_table` et
  `duplicate_population` vertes ; manifeste des mutants : 17 mutants (7 + 10 nouveaux), `--check` vert.
- Pilote `microbancs/mes_t2c_g/pilote_t2c.py` (bras avant / avant_bis / sans_gl7 / apres / gl5, REGLE_T2C) : auto-test
  vert en normal et `-O` ; essai local en cours (construction des bras, petite campagne). Archive du bras avant :
  `v12_src_avant_t2c.tar.gz` (sha256 `eaac2e539ab93d5e9e3e62f9ebf9f6eaef2128793cb853b11b997b71bfdf0a55`, git archive de
  base + patch 1). Plan : `plan_t2c_g.json`.
- 23:21 UTC (fin du journal) : **suite rapide complete du produit modifie (toutes unites, profil 21) : `ctest -LE long` 657/657 verts**
  (dont catalogue avec la table S* a fiches contigues, tour, reference, style) ; construction sans avertissement.
- 23:10 UTC : **essai local du pilote** (`--essai`, 2 tours x 3 passes a 3 fils, non decisif, juge = « refuse » comme
  attendu) : les 5 bras se construisent (substitutions exactes appliquees), empreintes de l'objet identiques dans tous
  les bras et processus de chaque trame (ng00 `e5a81154fb1b15f1` = porte ; ng01 `10bb4c6d14096b2d` ; ng02
  `d66647a043f682da`). Chiffres de l'essai (bruit +/-20 % sur le codespace, A/A 0,92 a 1,20) : lot 0,67 / 0,82 / 0,78 ;
  G-L5 au lieu de G-L7 1,08 / 1,07 / 1,12 (plus lente). Correction faite pendant l'essai : la copie des sources
  ignorait `src/index/build.cpp` (motif « build* ») ; seuls les DOSSIERS build* et __pycache__ sont ecartes.
- 23:15 UTC : file G-L7 : empreinte des traces calculee par cellule (I une fois, sites de U une fois, puis la somme des
  sites de A par masque) au lieu de k melanges par representant ; 18e mutant (`empreinte_de_file_sans_interieur`) : ne
  change que le travail (cibles identiques, sondes manquees) et n'est vu que par la resolution rejouee qui compare les
  compteurs du travail de l'etage — exemple de faute que l'empreinte de l'objet seul ne voit plus, d'ou les portes de
  travail (`weak_key_resolution`, determinisme W1 = W8).
- 23:42 UTC (fin du journal) : **profils 24 et 32** (`-DMHGP12_MODULES=catalogue;tower`, `-DMHGP12_COORD_BITS=24|32`) : construction sans
  avertissement, `ctest -LE long` **71/71** a chaque profil (portes du catalogue avec la table S* a fiches contigues,
  portes de la tour dont echelle 8 000 / 16 000 / 32 000 : memes empreintes de l'objet qu'au profil 21).
- vers 23:30 UTC (lecture) : **audits de l'auditeur Codex sur ce chantier** (lus sur main, commits b06cd1449, afa21ac16, 4981b09cd) :
  (a) `gc_index_borne` : recherche bornee O(log E) sous collisions confirmee ; tri des seaux en une tache si toutes les
  empreintes sont egales (seulement sous masque adverse) ; (b) `t2c_pilote_proposition` : patch d'admission stricte des
  journaux du pilote (schema exact par bras, rejeu des journaux dans le juge, refus des sorties annexes) et **correction
  des empreintes de reference du pilote (encore celles d'avant le patch 1 : le juge aurait rejete ng00 a tort)** :
  **applique tel quel** (preimage `323068ab...` = mon pilote, `git apply --check` vert, sha du patch = SHA256SUMS de
  l'auditeur) ; (c) `gc_plan_mesure` : avec 5 bras et l'ordre tournant, **10 tours** placent chaque bras deux fois a
  chaque position (8 tours non) -> REGLE_T2C passe a 10 tours minimum, plan a 10 tours ; **mesurer aussi le catalogue**
  (la table S* a fiches contigues deplace du cout vers le catalogue : +12 octets par boule) -> le pilote construit et
  joue aussi `mhgp12_catalogue_probe` (base et produit, processus alternes, 3 tours par trame) et publie catalogue et
  catalogue + G par tour (jamais juge) ; (d) `gc_rapports` : mes sorties `ab_local.py` ne gardaient pas les passes
  brutes -> nouveau banc `ab_local2.py` qui garde toutes les passes dans un JSON ; (e) `gc_hash_cellule` : empreinte
  par cellule de la file G-L7 confirmee exacte ; (f) le mode `rapport` seul joue desormais l'auto-test du juge d'abord.
- 23:40 UTC (fin du journal) : **campagne de mutants de la tour : 18/18 tues, tous par code** (`run_mutants.py --jobs 3`, temoin vert ;
  `mutants_ok module=tower mutants=18 tues=18 dont_signal=0 dont_delai=0 dont_construction=0 plancher=18`) : les 7
  anciens et 11 nouveaux (recherche sans egalite des SiteIdx, verification sans collisions, recherche sans dichotomie,
  seau non trie, population repetee admise, repertoire decale, tri instable, file decalee, empreinte de trace sans
  interieur (G-L5), table S* a queue partielle (catalogue), empreinte de file sans interieur (G-L7)).
- 23:46 UTC (fin des journaux) : **ThreadSanitizer** (`-DMHGP12_TSAN=ON`, `setarch -R`) : portes de l'index (5 groupes), determinisme de la
  tour (1 contre 8 fils), sonde sur u8000 a 8 fils et ng00 a 3 fils : 0 alerte ; empreintes = portes.
- Correctif d'horodatage (23:49 UTC, `date -u`) : quatre heures ecrites plus haut avaient ete estimees ; remplacees par
  l'heure de fin des journaux correspondants (`date -r`).

## Mesures locales finales (indicatives : codespace partage, charge 7 a 11 ; rien ne decide ici)

Bras construits par le pilote sur les sources finales (`pilote_essai/travail/`) : avant = archive base + patch 1,
apres = produit, sans_gl7 = file d'une place, gl5 = jointure triee. Statistique de la regle (mediane des passes 2..P
par processus, moyenne geometrique des rapports apparies par tour) ; **passes brutes gardees** dans
`runs/ab_final*.json` (recalcul independant possible, reserve `gc_rapports` de l'auditeur).

| cas | tours x passes | avant (ms) | apres (ms) | apres/avant | sans_gl7/avant | gl5/avant |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K5, 3 fils (`ab_final2_w3_ng00.json`) | 4 x 6 | 999 | 843 | **0,861** | 0,898 | — |
| ng00 K5, 3 fils (`ab_final_w3_ng00.json`, profil joue en meme temps sur un autre coeur) | 4 x 6 | 1 073 | 862 | 0,861 | 1,200 (bruit) | 1,362 (bruit) |
| ng00 K5, 1 fil (`ab_final_w1_ng00.json`, charge 9 a 11) | 3 x 3 | 4 024 | 2 796 | 0,599 (bruit fort) | — | — |

Lecture : le lot est plus rapide a chaque tour (8 tours sur 8 a 3 fils, 3 sur 3 a 1 fil) ; la file G-L7 gagne environ
4 % sur la sonde par representant dans l'index (0,861 / 0,898) ; la jointure G-L5 est plus lente sur l'hote dans toutes
les prises locales. Les rapports a 1 fil sont trop bruites pour etre cites comme gain. Rien de cela n'est un temps G4.

Profil par composante a UN fil (constructions `MHGP12_TOWER_PROFILE`, base 99fa83246 + instrumentation contre version
finale, meilleure de 2 x 2 passes alternees, ng00 K5, ordres 2..5, ns par occurrence nets du biais de 58 cycles ;
`runs/prof_final_compare.md`) :

| section | base : part ; ns ; occurrences | final : part ; ns ; occurrences |
| --- | --- | --- |
| trace (former F ; final : + empreinte par cellule, file, prechargements) | 8,2 % ; 48 ; 3 419 932 | 15,6 % ; 104 ; 3 419 932 |
| sonde (table / index) | **27,7 % ; 195** ; 3 781 161 | **13,7 % ; 78** ; 3 781 161 |
| proposition | 8,9 % ; 238 ; 1 012 161 | 10,8 % ; 274 ; 1 012 161 |
| LEM-T1 (table S*, F dans P_b) | 15,1 % ; 422 ; 1 012 161 | 14,8 % ; 384 ; 1 012 161 |
| certificat | 4,4 % ; 495 ; 252 152 | 4,3 % ; 452 ; 252 152 |
| census sature | 21,0 % ; 3 209 ; 194 136 | 24,2 % ; 3 461 ; 194 136 |
| census complet | 9,7 % ; 4 975 ; 58 016 | 10,3 % ; 4 941 ; 58 016 |
| pas | 0,3 % ; 51 ; 109 080 | 0,4 % ; 78 ; 109 080 |
| arret | 4,8 % ; 197 ; 650 932 | 5,9 % ; 228 ; 650 932 |
| total des passes, ordres 2..5 | 2 989 ms (176 / 370 / 758 / 1 684) | 2 793 ms (147 / 351 / 806 / 1 489) |

Occurrences identiques (meme travail logique). Le profil sous lfence casse le recouvrement memoire dont vit la file
G-L7 (les prechargements partent, mais chaque lecture du compteur serialise) : il sous-estime son gain, que la mesure
sans instrumentation donne. LEM-T1 ne baisse que de 9 % : la table S* contigue retire la dichotomie indirecte, mais
F dans P_b garde deux defauts dependants (decalage puis ligne de la CSR) ; le census (31 a 35 %) devient le premier
poste, avec la proposition et LEM-T1.

## Second rebasage (00:08 UTC le 8 octobre) : main = c903774b1

`main` a integre T1-b (voie appareil du catalogue, fin d'etage partagee CPU/CUDA qui construit desormais la table S*
dans `finish_driver.hpp` ; `table.cpp` ne garde que `find_support`), la reponse CST-0234, et le controle de type de
`exit.order` dans le juge de determinisme avec deux modes de fausse sonde (`sortie_ordre_booleen`,
`sortie_ordre_flottant`). Consequences pour la serie :
- **la table S* a fiches contigues est retiree du patch 2** : sa mesure locale ne donne que -9 % sur la section LEM-T1
  (422 -> 384 ns, soit environ 1,4 % du total a un fil) pour +12 octets par boule et une materialisation dans la
  publication commune CPU/CUDA (recu `gc_support_fusion` : pic +16E) ; le gain ne justifie pas de toucher la fin d'etage
  partagee qui vient d'etre fusionnee. Recommandation au proprietaire du catalogue plus bas. Plus aucun fichier du
  module catalogue dans la serie ; la porte `support_table` reste (elle juge `find_support` de main contre une table
  ordonnee) et son mutant vise desormais la comparaison de main (`support[0]` seul) ;
- mes ajouts redondants sur `exit.order` (mode `sortie_booleenne`) sont retires : main ferme deja ce residu ;
- le pilote ne joue plus la sonde du catalogue (plus de changement du catalogue a payer) ;
- patch 1 et patch 2 rebases sur c903774b1.

## Serie livree (rebasee sur `781fbe8d13e943bc67cbcf3996d6791523f1a422`)

| fichier | contenu | sha256 |
| --- | --- | --- |
| `patch_tour_Gc_1_empreinte.diff` | patch 1, prealable : empreinte de l'objet seul, juge et portes (5 fichiers) | `499a2588...` |
| `patch_tour_Gc_2_leviers.diff` | patch 2, sur le patch 1 : leviers, portes, mutants, pilote (18 fichiers) | `89d7b4f4...` |
| `patch_tour_Gc.diff` | cumul 1 + 2 (pour qui n'applique qu'un fichier) | `772630d7...` |
| `plan_t2c_g.json` | plan de session G4 (4 commandes) | |
| `v12_src_avant_t2c.tar.gz` | archive du bras avant (git archive de main 781fbe8d1 + patch 1), a placer dans les donnees de la session | `45a69c21275b4b0192988b5444d0699e8299cbe6b4dbdd40f47b0654de0c7f48` |

Fichiers du patch 2 : `src/tower/` (tower.hpp : diagnostics et types du profil seulement ; internal.hpp ; populations.cpp
reecrit ; radix.cpp, first_probes.cpp, passes.cpp, profile.hpp nouveaux ; stage.cpp, stage.hpp, resolve.cpp,
supports.cpp ; module.cmake : trois sources ajoutees), `bench/tower_probe.cpp`, `tests/tower/{index_unit.cpp,
unit_support.hpp, tests.cmake}`, `tests/mutants/tower.json`, `microbancs/mes_t2c_g/pilote_t2c.py`. **Aucun fichier
des etages T, M, V, R (forest_*, vertical_*, registry_*, export_*) ni du catalogue.** `tower.hpp` : seuls ajouts aux
diagnostics (frontieres, profil) ; `module.cmake` : `radix.cpp first_probes.cpp passes.cpp` ajoutes.

## Portes (serie finale sur main 781fbe8d1, profil 21 sauf mention ; codes exacts)

| porte | code | resultat |
| --- | ---: | --- |
| construction complete (toutes unites, `-Werror`) | 0 | 0 avertissement (00:13 UTC) |
| `ctest -LE long` (suite rapide complete) | 0 | **675/675** (00:19 UTC) |
| `ctest -L lidar` (catalogue ng00-02, Euler, determinisme de la tour ng00 1 contre 8 fils) | 0 | **6/6** (00:21 UTC) ; tour ng00 : empreinte de l'objet `e5a81154fb1b15f1` |
| `mhgp12_tower_oracle` (+ `_opt`) | 0 | ligne inchangee : 342 nuages, 21 225 cibles, 2 359 cellules |
| `mhgp12_tower_scale8000/16000/32000` (+ `_opt`), 1 contre 8 fils | 0 | empreintes de l'objet `a40f1b2ef8547269` / `cf7c7745fcb4ae6e` / `d1f08fd0dbdf48eb`, comptes de l'objet inchanges |
| `mhgp12_tower_unit_*` (9 groupes + inventaire) | 0 | inchanges |
| `mhgp12_tower_index_*` (radix_stable, index_reference, weak_key_resolution, support_table, duplicate_population + inventaire) | 0 | nouveaux |
| `mhgp12_tower_juge_temoin`, `_refus_*` (6 modes), `_ecart_travail_8`, `_ecart_objet_8` (+ `_opt`) | 0 / 2 / 1 | ecarts du travail ou de l'objet a empreinte egale : code 1 (nouveaux) |
| `mhgp12_mutants_tower_manifest` (+ `_opt`) | 0 | 18 mutants, plancher 18 |
| profils 24 et 32 (`catalogue;tower`, `ctest -LE long`) | 0 | 71/71 chacun, memes empreintes (sur la serie precedente, base 6f362f0bf ; le rebasage ne touche que le catalogue de main) |
| ThreadSanitizer (index, determinisme, sonde u8000 W8 et ng00 W3) | 0 | 0 alerte (serie precedente, memes sources de la tour) |
| `check_style` / `check_constats` (serie finale) | 0 | `style_ok fichiers=290` / 73 constats |

## Plan de session G4 (`plan_t2c_g.json`, non joue : GCP non utilise par moi)

Format `ehgp.v12.session_plan.v1` (celui de `plan_t2_h.json`), `default_build: true` (le paquet = main + patch 1 +
patch 2), `python_packages: none`, quatre commandes dans cet ordre (le pilote, decisif, avant les commandes longues) :
1. `socle_ctest` : `ctest --no-tests=error -LE long --output-on-failure -j 40` (1 200 s) ;
2. `t2c_pilote` : `python3 {src}/morsehgp3D_v12/microbancs/mes_t2c_g/pilote_t2c.py tout --src {src} --travail
   {build}/t2c --donnees {data} --sortie {out}/t2c --avant-archive {data}/v12_src_avant_t2c.tar.gz --avant-sha256
   45a69c21... --fils 48 --jobs 44 --processus 10 --passes 10` (3 000 s ; estime 25 a 30 min : 4 constructions de bras
   et le profil, 150 processus de la campagne, informations) ;
3. `lidar_ctest` : `ctest --no-tests=error -L lidar --output-on-failure -j 6` (1 500 s) ;
4. `mutants_tour` : `ctest --no-tests=error -R ^mhgp12_mutants_tower$ --output-on-failure` (1 800 s).
Donnees : `lidar_ng0{0,1,2}.u32le` et `.ids.u32le` (comme g4data_h) **plus** `v12_src_avant_t2c.tar.gz`
(sha256 ci-dessus ; si la serie est commise autrement, refaire l'archive par `git archive --format=tar <commit main +
patch 1> morsehgp3D_v12 | gzip -n -9` et changer `--avant-sha256` dans le plan).

**Regle ecrite d'avance (REGLE_T2C, dans le pilote, avant toute mesure G4)** : par processus, temps de G = mediane des
passes 2 a 10 du mur de `resolve_tower` ; par tour, rapport entre deux bras ; moyenne geometrique des rapports des 10
tours et IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261007), par trame. Un levier est **adopte** si
l'empreinte de l'objet est identique pour tous les processus de tous les bras de chaque trame (ng00 : celle de la
porte) ET si la borne haute de l'IC est sous 1 sur **chacune** des trames ng00, ng01, ng02 a K5 ; **rejete** sinon ;
**refuse** si une prise manque (processus en echec, journal hors schema ou modifie, passes ou empreinte absentes, moins
de 10 tours valides par trame), si un binaire change pendant la campagne, ou si l'auto-test du juge echoue. Leviers
juges : `lot_t2c` (avant -> apres : la tranche entiere), `G-L7` (sans_gl7 -> apres), `index_et_corrections` (avant ->
sans_gl7), `G-L5_au_lieu_de_G-L7` (apres -> gl5). A/A (avant -> avant_bis) publie, sans veto. Bras : 5 ; 10 tours
dans l'ordre tournant (chaque bras deux fois a chaque position, recu `gc_plan_mesure`). Le juge relit et revalide
tous les journaux (admission stricte de l'auditeur, recu `t2c_pilote_proposition`, appliquee telle quelle puis
completee : 10 tours, auto-test joue par `rapport`, essai `--essai` non decisif).

## Ecarts, points du contrat et recommandations (aucune contradiction mathematique trouvee)

1. **Empreinte** (coordinateur, 22:35) : l'empreinte de la resolution portait les compteurs du travail, contre le § 8 ;
   corrige par le patch 1 (objet seul). Consequence a garder en tete (recu `gc_hash_cellule`) : une faute qui ne
   change que le travail (exemple : le mutant `empreinte_de_file_sans_interieur`, sondes manquees, cibles identiques)
   n'est vue ni par l'empreinte ni par W1 = W8 (meme faute deterministe aux deux) ; seul un rejeu independant du
   travail la voit (`weak_key_resolution` compare tous les compteurs du travail de l'etage a une resolution rejouee).
   Et l'empreinte reste celle d'une politique de saut fixee (`TARG` en depend, recu `gc_empreinte`).
2. **G-L5 tel qu'ecrit au contrat (jointure triee) ne paie pas sur l'hote** : a 3 fils en local, la jointure coute
   autant qu'elle economise (tri par base limite par la bande passante, 2 x R x 16 octets par ordre) ; la sonde par
   representant dans un index compact (dans le cache de dernier niveau) puis prechargee en file (G-L7) gagne. Proposition
   de mise a jour du § 4.3 apres G4 : G-L5 = voie de l'appareil ; voie de l'hote = G-L7. Le bras `gl5` du pilote le
   juge sur G4.
3. **« census partant de la feuille du premier site » (G-L7 du contrat)** : non fait. Avec la politique `v12_indices`,
   un census sature doit rendre les k plus petits SiteIdx de I (le saut en depend, donc `TARG`) : partir d'une feuille
   ne les garantit pas ; changer l'ordre de visite changerait la cible (politique differente, G-L6), pas seulement le
   travail.
4. **Table de populations residente (contrat § 2)** : non faite ; l'index de chaque ordre est construit en parallele
   (il n'existe pas encore de Session pour le garder) ; ses tampons sont gardes d'un ordre a l'autre (une allocation).
   La residence entre trames n'a de sens que pour le meme domaine (recu `t2c_tables`).
5. **Catalogue (recommandation au proprietaire de la fin d'etage)** : une table S* a entrees contigues (queue de S* et
   boule, 16 octets) retire la dichotomie indirecte dans `balls_` ; mesure locale : LEM-T1 -9 % (422 -> 384 ns), soit
   environ 1,4 % de la resolution a un fil, pour +12 octets par boule et une materialisation a la publication (pic +16E,
   recu `gc_support_fusion`). Le reste de LEM-T1 est F dans P_b (decalage puis ligne de la CSR, deux defauts
   dependants) : une entree S* qui porterait aussi le decalage de la population en retirerait un. Non fait ici.
6. **Census (module index, 31 a 35 % de la resolution a un fil)** : premier poste restant avec la proposition et
   LEM-T1. Pistes (non faites, a mesurer) : canonisation du S* de la coquille dans le census complet (O(m^4)
   predicats exacts), comparaison de niveaux exacts du controle, preparation de la garde (trois divisions i128).
7. **Budget** : l'admission de l'etage se fait avant tout calcul (index au plus grand ordre, sans la jointure qui n'est
   pas sur la voie produit) ; `bytes_for` majore par toutes les naissances. Le bras gl5 alloue la jointure hors
   admission (mesure seulement, budget illimite de la sonde).
8. **Frontieres des chronos** (recu `t2c_tables`) : prepare, count, setup, fill, workspace, puis par ordre table / join /
   pass dans order ; `resolve_ns` est desormais exclusif (passes seulement) : ne pas comparer `resolve_ns` entre la base
   (tables hors resolve) et le produit sans le dire ; le pilote juge le mur de G entier.
9. **A 48 fils** (non joue) : la base passe 18,6 ms dans les tables (une tache par ordre) ; l'index se construit par
   blocs sur tout le Pool (comptage, populations, 2 a 3 passes de tri par base, repertoire, seaux, fiches) : attendu
   1 a 3 ms, a verifier. Le reste du mur de H (9 a 11 ms : controle du catalogue sequentiel, points, liberations)
   devient `prepare_ns` (parallele) et `reste_ns` (publie). Chaque ordre ajoute une dizaine de `parallel_for` : leur
   cout fixe a 48 fils est a lire dans `table_ns`.

## Ecart a la consigne des fils (declare)

La suite rapide finale, les portes lidar et la campagne finale des mutants ont tourne avec une affinite de 6 coeurs
(`taskset -c 2-7`) et `-j3` / `--jobs 3` : au plus 3 taches a la fois, mais les portes de determinisme lancent 8 fils
dans leur tache ; la consigne disait au plus 3 fils. Les mesures de temps, elles, ont toutes tourne sur 1 ou 3 coeurs
dedies (`taskset -c 5` ou `5-7`), mais sur un codespace partage (charge 3 a 15 d'autres agents) : indicatives.

## Pourquoi la v12 coutait plus par unite que la replique de MES-M7 (question du rapport de G)

Mesure (profil du produit, un fil, ng00 K5) contre le profil local de la replique (recu `mes_g1_m7_local`) : la sonde
n'etait PAS la cause (produit 190 a 245 ns, replique 285 ns ; la table du produit est par ordre, plus petite) ; le
certificat non plus (4 % ; la materialisation du niveau sert au controle de decroissance du census). Les deux postes
plus chers que la replique sont **LEM-T1** (dichotomie dans la ligne CSR du premier site de S* qui lit `balls_` a
chaque pas, 440 a 540 ns, contre une table de hachage S* -> boule a deux defauts dans la replique) et le **census
garde** (sature 2,5 a 4,3 us et complet 5 a 6 us contre 2,0 et 2,8 us pour le census de la v11 dans la replique ;
module index, garde NUM-GARDE du contrat numerique). Corrige ici ce qui etait gratuit dans la tour : rang lu dans la
fiche de l'index, lectures CSR differees, cible de fenetre directe, predicats `side` evites pour les sites de S.
- 00:33 UTC (fin du journal) : **campagne finale des mutants de la tour sur la serie rebasee : 18/18 tues, tous par
  code** (`mutants_ok module=tower mutants=18 tues=18 dont_signal=0 dont_delai=0 dont_construction=0 plancher=18`,
  `runs/mutants_tower_final.json`) ; le mutant de la table S* vise desormais la comparaison de `find_support` de main.

## Essai final du pilote sur la serie rebasee (01:00 UTC ; local, 3 fils, 2 tours x 6 passes, `--essai`, non decisif)

`pilote_final/` : construction des 5 bras (substitutions exactes appliquees sur la copie), campagne K5, informations,
rapport (auto-test joue d'abord). **Admission stricte verifiee sur les VRAIES sorties de `tower_probe`, ligne `exit`
finale comprise** (recommandation du coordinateur) : 30 journaux K5 (schemas « avant » et « apres »), 18 journaux K10,
12 a un fil, 6 uniformes et les 2 prises du bras profil (12 et 20 lignes `profil_g`) : **tous admis**, aucune
`admission` d'erreur ; le juge refuse comme attendu (2 tours sur 10 exiges), code 3 (verifie a part : le `pilote=0` du
journal de mon script est un artefact, `$?` y lisait le code de `date` ; les preuves sont les lignes de bilan des
journaux : 675/675, 6/6, `mutants_ok`). Chiffres de l'essai (A/A dans la fenetre : 0,996 / 0,994 / 0,992) :

| trame K5, 3 fils | avant | apres | lot (apres/avant) | G-L7 (apres/sans_gl7) | index (sans_gl7/avant) | gl5/apres | empreinte de l'objet |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| ng00 | 1 131 | 983 | 0,870 | 0,912 | 0,953 | 0,968 | `e5a81154fb1b15f1` (porte) |
| ng01 | 852 | 697 | 0,818 | 0,942 | 0,869 | 1,026 | `10bb4c6d14096b2d` |
| ng02 | 984 | 779 | 0,791 | 0,875 | 0,904 | 1,026 | `d66647a043f682da` |

Informations de l'essai (3 fils sauf mention, ms) : K10 ng00 / ng01 / ng02 : avant 10 149 a 10 412 / 6 060 a 7 220 /
7 317 a 7 916, apres 9 034 a 9 335 / 5 266 a 6 384 / 6 181 a 6 809 (-11 a -14 %), empreintes K10 identiques entre bras
(`918dbb9383edb29b`, `349c286d4a1a6c05`, `d8bb9f962f2b3dc1`) ; un fil ng00 K5 : avant 2 890 a 3 316, apres 2 257 a
2 546, sans_gl7 2 396 a 2 596, gl5 2 267 a 2 348 ; uniformes K5 : 8 000 406 -> 398, 16 000 990 -> 856, 32 000 2 438 ->
1 921, empreintes = portes d'echelle. **Ce sont des temps locaux d'un codespace partage, sur 2 tours : rien n'est
adopte ici ; la decision appartient a la session G4 et a REGLE_T2C.**

## Synthese (01:01 UTC)

- **Patch 1** (prealable, 5 fichiers) : empreinte de l'objet seul ; travail compare ligne a ligne hors empreinte ;
  preuves : exports res.bin/cat.bin identiques a l'octet, oracle et comptes de l'objet inchanges, portes 38/38 de la
  tour sur la base 781fbe8d1.
- **Patch 2** (18 fichiers, aucun du catalogue ni des etages T/M/V/R) : index des naissances parallele et canonique
  (remplace les tables sequentielles par ordre, chemin critique de 18,6 ms a 48 fils sur G4), file de sondes
  prechargees G-L7 (voie produit), G-L5 (jointure triee) ecrite et jouee en bras, preparation parallele, frontieres des
  chronos, corrections gratuites, instrumentation par option de construction ; compteurs de l'objet et du travail
  identiques a la base sur ng00 K5 ; portes : 675/675 rapides, 6/6 lidar, 18/18 mutants, profils 24/32 et TSan verts
  (sur la serie precedente pour ces deux derniers, tour identique).
- **Local (indicatif)** : etage G -14 % a 3 fils sur ng00 K5 (rapport apparie 0,861 sur 4 tours, deux campagnes), -13 a
  -21 % dans l'essai du pilote ; sonde 195 -> 78 ns par representant ; LEM-T1, census et proposition restent les
  premiers postes (census 31 a 35 %).
- **G4 : non joue.** Plan `plan_t2c_g.json` (4 commandes, pilote decisif en deuxieme), regle REGLE_T2C ecrite d'avance.
