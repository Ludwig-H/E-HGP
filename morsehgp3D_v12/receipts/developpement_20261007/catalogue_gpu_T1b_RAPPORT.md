# T1-b : voie appareil du catalogue (rapport de l'agent, tenu au fil de l'eau)

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 pour le catalogue ; cpu_reference pour l'identite
objet=full_pi0 (catalogue Cat_K)
quantification=quantized_u21_input_only (profils 21, 24, 32)
public_status=not_claimed
GCP non utilise
```

**Rien n'a tourne sur un GPU.** Tout ce qui concerne l'appareil (identite, determinisme, temps, transferts, mutant
« un fil par feuille ») est NON JOUE : le code de la voie appareil est construit par nvcc 12.9 (sm_120) sans
avertissement et joue sur l'hote par l'executeur Pool (meme source), jamais sur une carte.

**La voie appareil est hybride** (lecture de l'auditeur Codex, reprise telle quelle) : l'appareil joue les feuilles
d'au plus 32 sites et d'etendue locale d'au plus 16 bits ; les autres sont reprises EXACTEMENT sur le CPU (compactees,
rapatriees, rejouees par `LeafStage`, puis remontees) ; les plafonds de 256 candidats et de 64 sites de coquille sont
ceux de la voie CPU ; une construction aux profils 24 ou 32 ne signifie pas un traitement integral sur l'appareil.
Sur les trames reelles jouees en local : ng00 K5, 1 feuille reprise sur 123 581 ; ng02 K5, aucune ; ng00 K10, 2.

## 1. Livrables

| Fichier (dans `v12_gpu/`) | Contenu | sha256 |
|---|---|---|
| `patch_catalogue_gpu.diff` | correctif A : voie appareil T1-b (hybride), fin d'etage partagee (voie CPU parallele), comptes et transferts, pilote et juge G4 ; 45 fichiers ; base `99fa83246` | `9eb114da465cf212317d7e267e536ece765953b71647dfbba499a13cf0a935d2` |
| `patch_cst0234_feuille.diff` | correctif B (CST-0234), 4 fichiers, EMPILE sur A | `ea9795f8de04dca209f1e794bcce7881e6bc114458fe8013bdcf560aa911647a` |
| `plan_t1b_g4.json` | plan de session `ehgp.v12.session_plan.v1` (une commande, 3 300 s) | `f2a7690bdb1eafc9a11ca0966486bbe8041639a392844eab0e636f8dc078f3c8` |
| `patch_catalogue_gpu_base671072339.diff` | trace : ancien correctif A contre `671072339` (ne s'applique plus sur main) | `8001a30966f1f5d2c034e08b512b03f6e98b8b1208e665413d5816747cec98ba` |

Application, a la racine du depot : `git apply patch_catalogue_gpu.diff`, puis, si B est retenu, `git apply
patch_cst0234_feuille.diff`. Base : `git archive 99fa83246 morsehgp3D_v12` ; `git apply --check` puis application,
A puis B, VERTS sur des extractions neuves de `99fa83246`, `9c5809919`, `136e07628` et du HEAD de 23:42 UTC
`7e87b58d2` (aucun fichier commun avec les commits de main posterieurs a 99fa83246) ; l'arbre A+B obtenu sur
99fa83246 est identique, octet pour octet, a la copie jugee `frozenB3` (et A seul a `frozenA3`). Depot de travail :
`v12_gpu/repo99` (`03f28fb` = base, `66c41ed` = A, `3b445b7` = B) ; sauvegarde hors de /tmp :
`~/v12_gpu_sauvegarde/` (correctifs, plan, rapport, outils, comptes rendus de mutants, faisceaux git).

## 2. Ce que fait le correctif A

- **Une implantation, deux executeurs** (`CONTRAT_CATALOGUE.md`, § 10, livre dans A) : pilote du parcours en largeur
  (`traversal_driver.hpp`, port de `Driver<B>` de MES-M5), feuille J3 (`run_leaf<32, Narrow>`, un warp par feuille,
  `j3_r168`), lots de feuilles de l'appareil (`device_leaves.hpp`, `device_driver.hpp`, `device_pipeline.hpp`) et fin
  d'etage (`finish_level.hpp`, `finish_sort.hpp`, `finish_kernels.hpp`, `finish_driver.hpp`) ecrits une fois ;
  l'executeur Pool (`exec_host.hpp`, warps simules sur `sched::Pool`) les joue pour la voie CPU et pour les portes
  locales de la voie appareil ; l'executeur CUDA (`device_cuda.cu`, `device_leaf.cu`) les joue sur l'appareil. Les
  en-tetes de feuille et de parcours sont compiles tels quels par nvcc (aucun second algorithme).
- **Feuilles sur l'appareil** : lots d'au plus 2^17 feuilles, classes (repere, cause), ordonnees par taille
  decroissante, comptees (cases de 64 emissions), balayees (decalages exacts), puis admises (arene des boules agrandie
  dans le budget) et copiees ; feuilles non resolues reprises sur l'hote AVANT l'admission ; feuilles de plus de 64
  emissions reecrites par la meme source (Replay) aux places du comptage.
- **Comptes de la voie hybride** (constat de l'auditeur) : reprises par cause (`replayed_wide` : plus de 32 sites ;
  `replayed_span` : etendue au-dela de 16 ; `replayed_leaves`), reecritures par lieu (`rewritten_device`,
  `rewritten_host`) dont la somme est `leaves_rewritten`, egal a celui de la voie CPU ; publies dans la ligne de la
  sonde ; graves dans `pipeline_witnesses` (egalite avec la voie CPU sur neuf temoins, planchers non nuls) et tues par
  deux mutants.
- **Fin d'etage partagee, parallele aussi sur la voie CPU** (complement du coordinateur) : niveaux exacts a mots
  (memes N/D non reduits que `num::Sphere::through`), cle F3 et marge F4, cle des positions par rangs
  lexicographiques, tri par base stable, verification exacte de toute paire non certainement ordonnee, chaines mal
  ordonnees retriees en exact (repli hote compte), rangs, CSR I puis U et table S* -> boule par sommes prefixes par
  tuiles et tri par base. `sort.cpp`/`sort.hpp` retires ; `assemble.cpp` et `table.cpp` reecrits autour.
- **Mesure (CST-0235)** : `TransferMeter` dans les deux executeurs : TOUTE copie hote <-> appareil de l'appel (nuage,
  totaux de niveau, feuilles reprises et boules remontees, repli des chaines, sorties) comptee en duree, octets par
  sens et operations ; sur l'appareil, chaque copie attend d'abord les noyaux en file, hors chrono ; durees d'etape
  nettes des transferts ; la porte verifie que leur somme tient dans la duree murale, le juge G4 refuse une passe ou
  elle la depasse.
- **Capacite** : compter puis reserver ; tout tableau de l'appareil et toute memoire epinglee reserves dans le
  `MemoryBudget` (`BudgetReservation`, `swap` ajoute) avant `cudaMalloc`/`cudaMallocHost` ; refus transactionnel
  `memory_budget` sans publication ; sorties d'erreur de `grow` et `stage` nettoyees (lecture de l'auditeur) ; contexte
  resident `CatalogueDevice` reutilise d'un appel a l'autre (D1) ; raisons `device_unavailable`, `device_fault`.
- **Construction** : option `MHGP12_ENABLE_CUDA` (OFF par defaut, sm_120, exclusive des sanitizers, dialecte CUDA20
  declare pour CMake 3.22 de la VM comme dans la v11) ; sans elle, `device_stub.cpp` rend `device_unavailable`.
- **Session G4 preparee** : sonde `--device`, pilote `bench/g4_catalogue_device.py`, juge
  `bench/g4_catalogue_judge.py` (regle ecrite d'avance, auto-test 18 injections), mutants appareil
  `bench/g4_catalogue_mutants.json` ; portes `mhgp12_catalogue_device_unit` (10 groupes), `mhgp12_catalogue_g4_judge`,
  `mhgp12_catalogue_g4_mutants` ; 6 mutants hote de plus dans `tests/mutants/catalogue.json` (15 au total).
- **Rebasage** (troisieme message du coordinateur) : raisons apres celles de la tour, table de `status_test.cpp` a 31
  raisons (plancher 104, compte exact), ligne `catalogue` d'ARCHITECTURE.md modifiee a cote de la ligne `tower`,
  `BudgetReservation::swap` sur le `buffer.hpp` sous cache de `7b7d025b3` avec son cas dans `block_cache_limit`
  (plancher 48 : held et used inchanges a l'echange) et le mutant `echange_sans_les_octets`
  (`tests/mutants/core.json`, plancher 96).

## 3. Le correctif B (CST-0234), empile sur A

Sur l'hote seulement (branches `MHGP12_SIMT_WARP` : noyau de l'appareil inchange) : `pair_rows` evalue chaque couple
une fois (meme fonction `pair_relation` que la voie appareil, orientation canonique i < j) et ecrit ses deux lignes ;
le census s'arrete effectivement au (theta+1)-ieme interieur (`simt::serial_stop`) ; la remise a zero de H ne touche
que les lignes lues (i < j < m). Emissions, grand livre et compteurs inchanges par construction (`census_tests`
comptait deja jusqu'au site d'arret inclus) ; seule difference possible : un depassement de l'arithmetique large
(`healthy(WideExact)`, impossible sous le contrat numerique B <= 32) situe APRES le site d'arret ne serait plus vu
sur l'hote ; la politique etroite (i128) ne peut pas faillir (`healthy(i128)` vaut vrai), et la politique exacte ne
tourne que sur l'hote dans les deux voies (reprise `LeafStage`), donc l'identite CPU/appareil tient dans tous les cas.

## 4. Portes locales et codes (codespace partage, 8 coeurs, GCC 13.3, nvcc 12.9 local, sans GPU)

Toutes jouees sur l'arbre rebase (A = `66c41ed`, copie `frozenA3` ; A+B = `3b445b7`, copie `frozenB3`), profil 21 sauf
mention ; « code » = code de sortie de la commande ; aucune porte n'a joue la branche CUDA (pas de GPU local :
`device_open` constate `device_unavailable`).

| Porte | A | A+B |
|---|---|---|
| Construction Release, tous modules (GCC 13.3, `-Werror`) | code 0, aucun avertissement | code 0, aucun avertissement |
| Suite rapide `ctest -LE long --no-tests=error -j3` (sans donnees) | 651/651, code 0 (sentinelle LiDAR sautee) | 651/651, code 0 (idem) |
| Inventaire des portes rapides | base 99fa83246 : 637 ; A : 651 (14 ajoutees, aucune retiree) | 651 |
| `mhgp12_catalogue_device_unit_pipeline_witnesses` (voie appareil sur le Pool contre voie CPU, neuf temoins, comptes et transferts) | 146 controles, plancher 130, code 0 | idem |
| Juge G4 (`--selftest-judge`, normal et `-O`) | `juge_g4_t1b_ok injections=18` | idem |
| Motifs des mutants appareil (`--check-mutants`) | `mutants_appareil_ok mutants=3` | idem |
| Style `tools/check_style.py` ; `tools/check_constats.py` | `style_ok fichiers=283` ; OK | `style_ok` |
| Empreintes de la voie CPU (sonde, 3 fils) contre F2 : ng00, ng01, ng02 a K5 et K10, uniformes 8 000, 16 000, 32 000 a K5 | 9/9 egales, `ecarts=0` | 9/9 egales, `ecarts=0` |
| Voie appareil jouee sur l'hote contre voie CPU (outil `essais/multibatch.cpp`) : les neuf cas d'identite du plan | catalogue ET diagnostics physiques egaux, 9/9 | non rejoue (B ne touche que l'hote, identite deja sur la voie CPU) |
| CUDA (nvcc 12.9, sm_120, module catalogue) : construction et `ctest -LE long` | profil 21 : aucun avertissement, 44/44, code 0 | profils 21, 24, 32 : aucun avertissement, 44/44 chacun, code 0 |
| Mutants appareil du plan G4 construits avec CUDA (profil 21) | 3/3 sans avertissement | 3/3 sans avertissement |
| Campagne de mutants du catalogue (`run_mutants.py`, `--jobs 3 --build-jobs 1`) | `mutants_ok ... mutants=15 tues=15 dont_signal=1 ... plancher=15`, code 0 | idem, code 0 |
| Mutant du socle `echange_sans_les_octets` (`--only`) | tue (code), code 0 | — |
| ASan + UBSan (module catalogue) | — | 44/44, code 0, aucun rapport (budgets sans cache : cache actif non exerce) |
| TSan (`setarch -R`, module catalogue : unite de la voie appareil, determinisme, scale8000) | — | 12/12, code 0, aucun rapport |
| Plan G4 (`v12_session.validate_plan` de main) | — | `plan_valide commandes=1 delai=3300` |

Avant le rebasage (A sur `671072339`, pour trace) : ASan + UBSan 32/32, TSan, CUDA aux profils 21, 24 et 32, campagne
interrompue au profit de la base rebasee (voir journal).

## 5. Plan de session G4 (`plan_t1b_g4.json`) et regle

- Schema `ehgp.v12.session_plan.v1`, `default_build` faux (le pilote construit lui-meme avec CUDA), `python_packages`
  `none` (Python 3.10 nu), resultats plafonnes a 64 Mio, UNE commande `t1b_catalogue_appareil` (delai 3 300 s) :
  `python3 {src}/morsehgp3D_v12/bench/g4_catalogue_device.py --src {src} --data {data} --work {build}/t1b --out
  {out}/t1b --jobs 44 --threads 48 --processes 5 --passes 10`. Valide par `v12_session.validate_plan` (version de main
  du moment) contre l'arbre B et les 14 fichiers de `g4data_v12_t0`. Le correctif doit etre applique et commis sur
  main avant l'instantane de session.
- Etapes du pilote : environnement (nvcc, cmake, GPU, processus de calcul) ; construction Release au profil 21 avec
  CUDA ; portes rapides de cette construction (`device_open` y joue la VRAIE voie appareil contre la voie CPU sur neuf
  temoins) ; identite a l'octet (empreinte MHGP12DP, grand livre, comptes, diagnostics physiques, reprises et
  reecritures) voie appareil contre voie CPU sur ng00, ng01, ng02 a K5 et K10 et uniformes 8 000, 16 000, 32 000 a
  K5, 3 passes appareil par processus (determinisme d'un appel a l'autre) ; voie CPU apres la fin d'etage parallele
  sur les douze cas de F2 (48 fils, 10 passes, empreintes egales a F2, temps compares a F2) ; temps de l'etage C a
  chaud sur l'appareil : K5, 5 processus x 10 passes par trame (passes 2 a 10), K10 publie (3 x 5) ; mutants appareil
  (`feuille_non_resolue_admise_sans_rejeu`, `fin_sans_departage_exact`, `un_fil_par_feuille`) ; juge.
- Publication (CST-0235) : par trame, mediane et maximum de chaque etape nette (parcours, feuilles, emission, fin
  d'etage, transferts du raccord complet, publication), total et non ventile ; octets transferes par sens dans la
  section `device` de chaque sonde.
- Regle ecrite d'avance (docstring du pilote, completee vers 22 h UTC apres la lecture de l'auditeur) : « refuse » si
  une preuve manque ou est incoherente (outil, construction, sonde en echec, cas ou passe manquants, GPU non isole,
  contrat de mesure non respecte, etapes dont la somme depasse le total) ; « rejete » si une condition tombe (porte
  rapide, `device_open` sans voie appareil, identite ou determinisme, diagnostics ou reprises mal comptes, voie CPU
  differente de F2, mutant non tue, budget) ; « adopte » seulement si tout tient ET si, pour chacune de ng00, ng01,
  ng02 a K5, la mediane des 45 passes chaudes et le maximum des medianes par processus sont au plus 45 ms.
- Duree estimee : 35 a 45 min (construction CUDA, portes, 18 sondes d'identite, 120 passes CPU, 150 + 45 passes
  appareil, trois constructions mutantes) ; delai dur 55 min.

## 6. Reponses aux constats de l'audit de performance (`f601b36ac`)

- **CST-0233** (fin d'etage CPU en serie) : la fin d'etage partagee suit le schema de la preuve de l'auditeur
  (`receipts/audit_performance_20261007/assemblage/`, balayage par blocs avec halo, 3 537 confrontations). Ses quatre
  mutants ont leurs analogues dans `tests/mutants/catalogue.json`, tues sur l'hote : sans halo ->
  `finition_sans_halo` (porte `finish_plateau`, plateau traversant 8 tuiles) ; formes au lieu des valeurs ->
  `finition_formes_au_lieu_des_valeurs` (`finish_ties`) ; prefixe inclusif -> `finition_prefixe_inclusif`
  (`finish_random`) ; dernier representant -> `finition_dernier_representant` (`finish_ties`). La table S* -> boule
  est sa proposition B (tri des quatre SiteIdx, ici par base, puis dichotomie des debuts de ligne). Temps a 48 fils :
  non joue (plan G4, voie CPU sur les douze cas de F2).
- **CST-0234** (feuille J3 CPU) : correctif B separe et empile (§ 3).
  Sur l'hote seulement (noyau de l'appareil inchange) : couples evalues une fois, census arrete au rejet, remise a
  zero de H limitee aux lignes lues. Portes sur A+B : suite rapide 651/651, NEUF empreintes de F2 retrouvees, campagne de
  mutants du catalogue 15/15, ASan + UBSan 44/44 et TSan 12/12 sans rapport, construction CUDA aux profils 21, 24 et
  32 sans avertissement (dont le mutant appareil « un fil par feuille », qui compile ces branches de l'hote pour
  l'appareil). Effet local indicatif sur ng00 K5 a un fil (codespace charge, alternance A/B, 3 tours x 2 passes) :
  etage des feuilles 11,38 s -> 10,46 s (rapport 0,92) ; a mesurer sur G4 par la voie CPU du plan si B est applique
  avant la session.
- **CST-0235** : la mesure G4 publie a part parcours, feuilles, emission, fin d'etage, transferts du raccord complet
  (toute copie hote <-> appareil, chronometree apres l'attente des noyaux en file, octets par sens) et publication,
  plus total et non ventile, en mediane et maximum, contre le budget de 45 ms ; etapes nettes des transferts, disjointes
  (porte locale : somme <= duree murale ; juge : refus sinon). Valeurs : non jouees.

## 7. Notes de fusion pour le developpeur

- Ordre : `patch_catalogue_gpu.diff` (A) puis, si retenu, `patch_cst0234_feuille.diff` (B). Aucun fichier commun avec
  les commits de main posterieurs a `99fa83246` jusqu'a `7e87b58d2` (verifie : A puis B s'appliquent sur `99fa83246`,
  `9c5809919`, `136e07628` et `7e87b58d2`).
- `src/core/reasons.def` : `device_unavailable` (29, `resource_exhausted`) et `device_fault` (30, `invariant_violated`)
  APRES les cinq raisons de la tour ; `tests/core/status_test.cpp` : 31 raisons, plancher du test `reasons` porte a
  son compte exact 104 (main gardait 83 pour 98 controles).
- `src/core/buffer.hpp` (version sous cache de `7b7d025b3`) : `BudgetReservation::swap` seul ajout (echange compte et
  octets ; aucune tenue ne bouge) ; cas dans `block_cache_limit` (plancher 40 -> 48) ; mutant
  `echange_sans_les_octets` (`tests/mutants/core.json`, plancher 95 -> 96).
- `tests/mutants/catalogue.json` : 15 mutants (plancher 15) ; `sort.cpp`/`sort.hpp` retires (mutant de l'ancien tri
  reporte sur `finish_kernels.hpp`).
- `src/tower/source_pins.json` epingle `src/catalogue/leaf_census.hpp@671072339` (convention du support canonique) :
  l'epingle reste un fait historique ; B change le fichier courant sans changer cette convention.
- Voie CPU : `catalogue_digest` inchange (neuf empreintes de F2 retrouvees) ; diagnostics : `assemble_ns` garde les
  copies de l'executeur Pool, `transfer_ns` reste nul ; nouveaux champs de `CatalogueDiagnostics` nuls sur la voie CPU.

## 8. Decisions a contre-lire

(a) mode serie dormant de `simt.hpp` (`MHGP12_SIMT_SERIAL`, seulement pour le mutant appareil) ; (b) cases de 64
emissions et lots de 2^17 feuilles (192 Mio de cases) plutot que l'arene a curseurs atomiques du microbanc, pour
compter avant de reserver ; (c) repli des chaines mal ordonnees sur l'hote ; (d) cle des positions par rangs
lexicographiques ; (e) premiere faute : profondeur puis fusion des refus ; (f) tri par blocs de l'ancienne voie CPU
retire ; (g) voie hybride : reprise exacte sur le CPU des feuilles hors du perimetre de l'appareil, comptee par cause ;
(h) transferts chronometres apres l'attente des noyaux en file (l'attente reste dans l'etape qui l'a causee) ; (i) B :
arret du census sur l'hote au rejet (un depassement de l'arithmetique large apres ce site ne serait plus vu).

## 9. Leviers et suites

- Repli des chaines : `finish_repair` rapatrie verdicts, ordre et cles en entier (22,5 Mio pour UNE chaine sur ng02
  K5, mesure locale de l'executeur Pool) ; compacter les chaines sur l'appareil avant de rapatrier si G4 en voit.
- Sorties vers l'hote : rapatriement par tranches de memoire epinglee puis copie parallele ; double tampon (copie de
  l'appareil et copie hote en recouvrement) si les transferts pesent dans les 45 ms.
- Reecritures : 3,2 % des feuilles de ng00 K5 (3 990 sur 123 581) depassent 64 emissions et sont rejouees par
  Replay (J3 joue deux fois) ; cases plus grandes ou ecriture directe a curseur si Replay pese.
- Perimetre hybride : feuilles de plus de 32 sites ou d'etendue au-dela de 16 reprises sur le CPU (1 ou 2 par trame
  LiDAR en local) ; voie large de T1-c pour les petits nuages denses et les profils 24/32.

## 10. Ce qui reste a jouer sur G4 (NON JOUE ici)

Toute la session du plan : construction CUDA sur la VM (CMake 3.22.1, nvcc 12.9), `device_open` sur une vraie carte,
identite et determinisme de la voie appareil, comptes de la voie hybride sur l'appareil, temps de la voie CPU apres la
fin d'etage parallele contre F2, temps de l'etage C a chaud contre 45 ms (avec etapes et transferts), K10, mutants
appareil dont « un fil par feuille ». Non prevus dans ce plan : profils 24 et 32 sur l'appareil (construits ici
seulement), et B sur G4 (B ne change que l'hote ; la mesure CPU du plan le jugerait s'il est applique avant la session).

## 11. Journal (tenu au fil de l'eau, heures UTC lues par `date -u`)

Base : `main` = `6710723399876266d1efd7b45cf0d6adcd49a862` (671072339).
Copie de travail : `v12_gpu/repo` (git archive HEAD morsehgp3D_v12, depot git local pour le diff).

### Journal

- 19:03 UTC : consignes lues, copie faite.
- 19:20 UTC : essai nvcc 12.9 local (sm_120) : `traversal_kernels.hpp`, `traversal_scan.hpp` et `run_leaf<32, Narrow>`
  compilent TELS QUELS pour l'appareil aux profils 21, 24 et 32 (feuille sous `__launch_bounds__(128, 3)` : 168
  registres, 0 octet deverse, 20 736 octets de memoire partagee pour 4 warps), sans avertissement.

### Conception retenue (19:27 UTC)

Une seule voie appareil, ecrite une fois pour deux executeurs : l'executeur CUDA (produit, `MHGP12_ENABLE_CUDA`) et un
executeur hote de test (warps simules sur le Pool, `tests/catalogue/`), qui rejoue localement EXACTEMENT le code de la
voie appareil (noyaux et pilote) pour les portes sans GPU. Le parcours en largeur passe sur un pilote generique
(`traversal_driver.hpp`, port de `Driver<B>` de MES-M5) : la voie CPU l'instancie avec son executeur Pool (memes
noyaux, meme ordre : sorties inchangees), la voie appareil avec l'executeur CUDA.

1. Feuilles : lots d'au plus 2^17 feuilles accumules d'un niveau a l'autre (une trame de 60 000 sites a K5 tient en
   un lot) ; par lot : Classify (repere : coin minimal et etendue, comme `num::Frame`), ordre par taille decroissante
   (un passage de tri par base), Count (J3 `run_leaf<32, Narrow>`, un warp par feuille, cases de 64 emissions comme
   la voie CPU), balayage des comptes (decalages exacts), lecture des totaux, admission (arene des boules
   agrandie avant toute ecriture), Fill (copie des cases, rejeu des feuilles qui debordent leur case par la meme
   source). Non resolues (m > 32 ou s > 16) : comptees par cause, compactees, rapatriees et rejouees sur l'hote par
   `LeafStage` (meme source, politique exacte ou warp virtuel) AVANT l'admission du lot.
2. Fin d'etage sur l'appareil : niveau exact de chaque boule en entiers a mots (memes formules que
   `num::Sphere::through`, donc memes numerateurs et denominateurs non reduits), cle F3 (meme fonction que
   `sort.cpp`), cle des positions de S* (rangs lexicographiques des sites) ; tri par base stable par positions puis par
   cle F3 ; verification exacte de chaque paire de voisins non certainement ordonnes (marge F4) ; chaines fautives
   retriees en exact (repli hote, compte) ; rangs ; CSR I puis U ; table S* -> boule par tri par base des SiteIdx.
3. Rapatriement compact vers un `Catalogue` hote identique (niveaux materialises pour les rangs distincts).
4. Contexte resident (`CatalogueDevice`) : flux, tableaux de l'appareil et memoire epinglee reutilises d'un appel a
   l'autre ; toute reservation (appareil, epinglee) comptee dans le `MemoryBudget` par `BudgetReservation`.

### Complement du coordinateur (recu vers 19:32 UTC)

Session G4 F2 (`receipts/g4_t1f_20261007`, `0fd6c2286`) : voie CPU du catalogue a 48 fils 373 a 470 ms a K5, 1,4 a
1,75 s a K10 (environ 2,3 fois la v11) ; assemblage (82 a 121 ms a K5) et table S* (40 a 52 ms) a peine paralleles.
Demande : fin d'etage parallele AUSSI sur l'hote, en partageant ce qui peut l'etre (blocs et sommes prefixes, table S*
parallele), sorties identiques a l'octet (`catalogue_digest` inchange) et deterministes quel que soit le nombre de
fils ; ajouter au plan G4 la mesure de la voie CPU apres correction (memes 12 cas que F2, 48 fils, 10 passes).

Decision (19:34 UTC) : la fin d'etage devient UNE implantation en source unique (noyaux de warps, sommes prefixes par
tuiles, tri par base stable) jouee par deux executeurs : le Pool (voie CPU, executeur hote du parcours deja en
service) et CUDA (voie appareil). La voie CPU garde ses feuilles (`LeafStage`) ; seule sa fin d'etage change
(`assemble.cpp`, `sort.cpp`, `table.cpp`). Les empreintes de F2 (frame `probe`) sont gravees comme attendus de la
mesure CPU apres correction : ng00 K5 `698efd3d...`, K10 `0f92e4f1...` ; ng01 K5 `12e76841...`, K10 `2c0018b2...` ;
ng02 K5 `d0578a53...`, K10 `aea71fe1...` ; u8000 `b38e2657...`, u16000 `38388519...`, u32000 `2fe412a5...`.
- 19:50 UTC : fin d'etage partagee (finish_level/sort/kernels/driver.hpp, executeur Pool exec_host.hpp) branchee sur
  la voie CPU : empreinte de ng00 K5 `698efd3d...` IDENTIQUE a la base et a F2, nuage 2 500 sites identique ; aucune
  chaine a retrier sur ng00. Mesure locale indicative (codespace partage, charge 3 a 4) sur ng00 K5, fin d'etage
  (niveaux + tri + assemblage + table) : 1 fil 812 ms contre 960 ms (base), 3 fils 301 ms contre 490 ms ; toutes les
  etapes sont paralleles (rapport 1 -> 3 fils : 2,7). Aucune decision : G4 jugera a 48 fils.
- 20:04 UTC : voie appareil ecrite (device_leaves.hpp, device_driver.hpp, device_pipeline.hpp) et jouee sur l'hote par
  l'executeur Pool (meme code) : porte `mhgp12_catalogue_device_unit` verte localement (8 tests) : niveaux a mots
  egaux a num (11 224 controles, etendues 3 a B), tri par base et sommes prefixes contre std::stable_sort, fin d'etage
  sur 3 000 enregistrements construits et sur quasi-egalites contre une reference independante (repli exact
  declenche et conforme), voie appareil complete identique a la voie CPU sur 9 temoins (feuilles non resolues
  rejouees : triangle long a l'etendue B + 1, coquille de 48 sites sur warp virtuel), refus WIT-SPHERE50 identique,
  etat resident, memes octets a 1, 3 et 8 fils ; device_open : device_unavailable (pas de GPU local).
- 20:31 UTC : construction CUDA (profil 21, nvcc 12.9 local, sm_120, `-Xcompiler=-Wall,-Wextra,-Werror`,
  `-Werror=all-warnings`, `-fmad=false`) : bibliotheque, sonde et portes construites sans avertissement ; suite rapide
  `ctest -LE long` sur cette construction : 616/616 (sentinelle LiDAR sautee : pas de MHGP12_DATA_DIR), dont les
  9 portes de `mhgp12_catalogue_device_unit` et les echelles 8 000/16 000/32 000 (comptes v11, 1/4/8 fils) avec la
  fin d'etage partagee ; `--device` sans GPU : device_unavailable (code 2). Noyaux (ptxas) : Count 168 registres,
  12 octets deverses ; Replay 168 registres, 204 octets deverses (feuilles de plus de 64 emissions seulement) ; tous
  les autres sans deversement (BallKey 100 registres).
- 20:25 UTC : pilote G4 `bench/g4_catalogue_device.py`, juge `bench/g4_catalogue_judge.py` (auto-test : 16
  injections conformes, aussi sous `python3 -O`), manifeste `bench/g4_catalogue_mutants.json` (3 mutants appareil).

### Second message du coordinateur (recu vers 20:53 UTC) : constats CST-0233, CST-0234, CST-0235 (audit f601b36ac)

- CST-0233 (fin d'etage CPU en serie) : confronter la fin d'etage partagee a la preuve de l'auditeur
  (`receipts/audit_performance_20261007/assemblage/` : scan par blocs avec halo, 3 537 confrontations, quatre mutants :
  sans halo, formes au lieu des valeurs, prefixe inclusif, dernier representant ; table par permutation des quatre
  SiteIdx) et le citer.
- CST-0235 : la mesure G4 doit publier separement parcours, feuilles, emission, fin d'etage, transferts et total a chaud
  (mediane et maximum) contre le budget.
- CST-0234 (feuille J3 CPU : paires evaluees deux fois, census poursuivi apres rejet) : correction en source unique
  proposee en commit separe si simple et emissions conservees mot a mot, sinon suite.
- Le socle recevra un correctif de MemoryBudget sous cache (CST-0007) ; API de Buffer et BudgetReservation inchangee.
  Note : ce correctif-ci AJOUTE `BudgetReservation::swap` (echange, sans changer le reste de l'API).
- 21:12 UTC : profil 32 avec CUDA : construction complete sans avertissement, portes catalogue/coeur/style 62/62
  (dont niveaux a mots a l'etendue 32, plateau traversant 8 tuiles, temoins de la voie appareil). Voie appareil jouee
  sur l'hote (executeur Pool, outil de travail `essais/multibatch.cpp`, hors correctif) contre la voie CPU, 3 fils :
  ng00 K10 identique (5 512 670 boules, 7 lots, 2 feuilles rejouees sur l'hote) ; ng00, ng01, ng02 et uniforme
  32 000 a K5 identiques (1 lot chacun ; 1, 3 et 0 feuilles rejouees, sans boule ; ng02 : UNE chaine retriee en exact,
  repli emprunte sur donnee reelle, resultat identique).
- 21:27 UTC : ASan + UBSan (`-fno-sanitize-recover=all`, module catalogue) : 32/32 portes du catalogue (unite de la
  voie appareil jouee sur l'hote, unite du catalogue, oracle borne 342 nuages, Euler, juge G4), aucun rapport. TSan
  (`setarch -R`) : `mhgp12_catalogue_device_unit` 9/9 et determinisme du catalogue (1, 4, 8 fils) sans course.
  Profil 24 avec CUDA (module catalogue) : 44/44, construction sans avertissement. Etat A fige (depot de travail).
- 21:30 UTC : correctif principal `patch_catalogue_gpu.diff` (etat A) : 42 fichiers, s'applique proprement
  (`git apply --check`) sur un export du HEAD commis de main `791e8492a` (le worktree partage porte des modifications
  NON commises du developpeur dans `core/buffer.hpp`, `core/reasons.def`, `tests/core/status_test.cpp`,
  `docs/ARCHITECTURE.md`, `cmake/modules.cmake` : fusion a prevoir, voir la section Fusion). Campagne de mutants
  locale lancee sur une copie figee de l'etat A (13 mutants, `--jobs 1 --build-jobs 3`).
- 21:31 UTC : CST-0234 ecrit en correctif separe (etat B, au-dessus de A) : relation de chaque paire calculee une fois
  sur l'hote (`pair_rows`, meme fonction `pair_relation` que la voie appareil), arret effectif du census au rejet
  logique sur l'hote (`simt::serial_stop`), effacement des seules lignes H accessibles sur l'hote ; noyau de
  l'appareil inchange (branches `MHGP12_SIMT_WARP`). A construire et juger apres la campagne.
- 21:33 UTC : campagne de mutants locale sur l'etat A (ancienne base) en cours ; elle sera abandonnee au profit de la
  base rebasee (message du coordinateur ci-dessous) : arretee a 21:39 UTC par son groupe de processus (kill -TERM
  -<pgid>), apres le temoin vert et 5 mutants, sans compte rendu ecrit (resultat non publie, non compte).

### Troisieme message du coordinateur (recu vers 21:38 UTC) : rebaser sur main 99fa83246

Main a recu 7b7d025b3 (MemoryBudget sous cache, CST-0007/0019) et 99fa83246 (etage G de la tour, 5 raisons en fin
de reasons.def, table de status_test.cpp a 29 raisons, ligne tower d'ARCHITECTURE.md et de cmake/modules.cmake). Le
correctif A ne s'applique plus (conflits ARCHITECTURE.md, reasons.def, status_test.cpp). Demande : le refaire contre
`git archive 99fa83246 morsehgp3D_v12` (ou le HEAD du moment, l'indiquer), raisons APRES celles de la tour, table
etendue, ligne d'ARCHITECTURE a cote de celle de la tour, `BudgetReservation::swap` sur le buffer.hpp de 7b7d025b3
(cas ajoute a block_cache_limit : sous cache, une reservation echangee garde sa tenue dans held), `git apply --check`
sur une extraction neuve, portes rejouees (dont la campagne de mutants du catalogue) sur la base rebasee ; livrer A
puis B (CST-0234) en deux correctifs empiles.

Note sur la base de l'ancien correctif : `v12_gpu/repo` (commit `eddf181`) est IDENTIQUE a `git archive 671072339
morsehgp3D_v12` (verifie par `diff -r`, 21:39 UTC) : l'ancien correctif etait calcule contre le commit, pas contre
l'arbre de travail partage ; il ne retire de status_test.cpp que 4 lignes, remplacees (taille de la table, plancher,
commentaire, derniere raison). Contre 99fa83246 (29 raisons), ses blocs ne trouvent plus leur contexte.

- 21:40 UTC : rebasage dans `v12_gpu/repo99` (depot local, historique lineaire : `03f28fb` = `git archive 99fa83246
  morsehgp3D_v12`, `714c173` = A', `c85e415` = B'). Resolutions : reasons.def, `device_unavailable` et `device_fault`
  APRES `cell_capacity` (codes de la tour inchanges) ; status_test.cpp, table de 31 raisons, derniere `device_fault`,
  plancher du test reasons porte a son compte exact 3 x 31 + 11 = 104 (main gardait 83 pour 98 controles) ;
  ARCHITECTURE.md, ligne catalogue modifiee en place (la ligne tower suit, colonne Depend de inchangee, porte
  mhgp12_style verte) et puce CUDA ; buffer.hpp, `swap` applique tel quel sur la version sous cache (echange des
  comptes et des octets : la tenue dans held et used ne bouge pas). Ajouts demandes : cas d'echange dans
  `block_cache_limit` (plancher 40 -> 48, 8 controles : held et used inchanges a l'echange, chaque reservation rend
  ensuite les octets qu'elle porte), mutant `echange_sans_les_octets` dans tests/mutants/core.json (plancher 96).
  Un .pyc egare dans le premier commit A' (cree par check_style) retire avant tout correctif.
- 21:40 UTC : `patch_catalogue_gpu.diff` (A', 44 fichiers, sha256 `cc2c6160...`) puis `patch_cst0234_feuille.diff`
  (B', 4 fichiers, sha256 `ea9795f8...`) : `git apply --check` VERT sur des extractions neuves de `99fa83246` et du
  HEAD du moment `9c5809919` (audits et recus seulement depuis 99fa83246), A puis B ; l'arbre obtenu est identique a
  `714c173`. Ancien correctif garde pour trace : `patch_catalogue_gpu_base671072339.diff` (sha256 `8001a309...`,
  inclut le correctif de fuite CUDA).
- 21:41 UTC : construction Release (profil 21, tous modules) de A' fige (`frozenA2`) lancee, 3 fils.
- 21:43 UTC : contrat (§ 10) complete dans A' : point 7 (constats CST-0233, CST-0234, CST-0235 de l'audit `f601b36ac`)
  et decision (c) corrigee (une chaine retriee en local, ng02 a K5). Correctifs regeneres : A' = `3443e35`
  (`patch_catalogue_gpu.diff`, 44 fichiers, sha256 `3d841f4a...`), B' = `56382a6` (`patch_cst0234_feuille.diff`,
  4 fichiers, sha256 `ea9795f8...`, inchange) ; `git apply --check` puis application VERTS, A puis B, sur des
  extractions neuves de `99fa83246` et de `9c5809919` ; copies figees `frozenA2` = A', `frozenB2` = B' (verifie par
  `diff -r`).
- 21:43 UTC : construction Release de A' (profil 21, tous modules dont tower, GCC 13.3, `-Werror`) : code 0, aucun
  avertissement (2 min 20 s a 3 fils). Suite rapide `ctest -LE long -j3` lancee.
- 21:51 UTC : suite rapide de A' (`ctest -LE long -j3`, sans MHGP12_DATA_DIR) : 651/651, code 0 (sentinelle LiDAR
  sautee, comme sans donnees), 416 s. Inventaire : la base `99fa83246` configuree de meme compte 637 portes rapides ;
  A' en ajoute 14 (10 de `mhgp12_catalogue_device_unit`, juge et mutants G4, et leurs variantes `_opt`) et n'en retire
  aucune (le 663 du developpeur inclut des portes a donnees).
- 21:52 UTC : empreintes de la voie CPU de A' (sonde, 3 fils, une passe, outil de travail `essais/empreintes.py`) :
  les NEUF empreintes de F2 retrouvees a l'identique (ng00, ng01, ng02 a K5 et K10 ; uniformes 8 000, 16 000,
  32 000 a K5), chaines retriees en exact : ng02 K5 1, ng02 K10 2, aucune ailleurs. Temps locaux sans valeur de
  decision (codespace charge par une autre campagne).
- 21:53 UTC : campagne de mutants du catalogue sur `frozenA2` (`--jobs 3 --build-jobs 1`), puis mutant
  `echange_sans_les_octets` du socle (`--only`, `--floor 1`) ; journaux `mutA2/`.
- 22:02 UTC : campagne de mutants sur A' (`3443e35`, `frozenA2`) : `mutants_ok module=catalogue mutants=13 tues=13
  dont_signal=1` (finition_prefixe_inclusif tue par signal), code 0 ; mutant du socle `echange_sans_les_octets` tue
  (code), code 0. Comptes rendus gardes : `mutA2/report_A3443e35.json`, `mutA2/report_core_A3443e35.json`. Etat depasse
  par la correction suivante : campagne a rejouer sur A''.

### Quatrieme message du coordinateur (recu vers 21:59 UTC) : lecture de l'auditeur Codex (cuda_integration)

L'auditeur (`receipts/audit_reponses_20261007/cuda_integration/README.md`, lecture statique de `device_cuda.cu`
`5e215fe2...`) confirme le nettoyage des deux sorties memoire et releve : (1) `leaves_rewritten` reste nul sur la voie
appareil alors que des feuilles sont reecrites (Replay au-dela de 64 emissions) et reprises sur l'hote ; corriger,
compter par cause, publier dans la ligne de la sonde, graver un cas qui l'exige non nul ; (2) la voie est HYBRIDE
(appareil pour <= 32 sites et 16 bits locaux, reprise CPU exacte sinon) : le dire ainsi dans le rapport et le contrat ;
(3) CST-0235 : publier a part les transferts du raccord complet.

- 22:11 UTC : corrige dans A'' (`66c41ed`) :
  - reecritures : `BatchStats.rewritten` (feuilles resolues de plus de 64 emissions, comptees par la reduction du lot,
    celles que Replay reecrit) -> `rewritten_device` ; reecritures de la reprise de l'hote (`LeafStage::totals()`)
    -> `rewritten_host` ; `leaves_rewritten` = somme, meme sens que la voie CPU ; reprises par cause `replayed_wide`
    (plus de 32 sites) et `replayed_span` (etendue au-dela de 16), plus `replayed_leaves` (une feuille peut avoir les
    deux causes) ; tout publie dans la section `device` de la ligne de la sonde ;
  - porte `pipeline_witnesses` (plancher 30 -> 130, 146 controles) : sur chacun des neuf temoins, paliers, etendue
    maximale et `leaves_rewritten` EGAUX a ceux de la voie CPU, `leaves_rewritten` = appareil + hote, reprises par
    cause egales aux paliers exact et warp virtuel de la voie CPU ; planchers : reprises des deux causes, au moins une
    reecriture sur l'appareil, au moins une dans la reprise de l'hote (coquille48) ; mutants
    `reecritures_appareil_non_comptees` et `reecritures_reprise_hote_non_comptees` (manifeste 15, plancher 15) ;
  - transferts du raccord complet : `TransferMeter` dans les deux executeurs (`transfer_meter.hpp`), TOUTE copie
    hote <-> appareil comptee (duree, octets par sens, operations) ; sur l'appareil, chaque copie attend d'abord les
    noyaux en file hors chrono ; durees d'etape nettes des transferts (`NetWatch`) : parcours, feuilles, emission, fin
    d'etage ; `transfer_ns`, `transfer_h2d_bytes`, `transfer_d2h_bytes`, `transfer_ops` publies ; voie CPU inchangee
    (copies de l'executeur Pool laissees dans l'assemblage, `transfer_ns` nul) ; la porte verifie que la somme des
    etapes publiees tient dans la duree murale de l'appel ;
  - juge G4 : identite etendue aux diagnostics physiques et aux comptes de la voie hybride (rejete sinon) ; passe dont
    les etapes depassent le total : refuse ; auto-test 18 injections (16 + `reecritures_perdues`,
    `etapes_recouvertes`), aussi sous `python3 -O` ; regle du pilote completee (datee) ;
  - contrat § 10 : point 3 « voie appareil, hybride » (perimetre de l'appareil, reprise exacte sur le CPU, plafonds
    256/64, profils 24/32 non integralement sur l'appareil, comptes publies), point 7 (transferts du raccord complet).
- 22:11 UTC : voie appareil jouee sur l'hote (executeur Pool, outil `essais/multibatch.cpp`) contre la voie CPU,
  3 fils : ng00 K5 identique, 1 feuille reprise (etendue), 3 990 reecritures (appareil 3 990, hote 0) = voie CPU
  3 990 ; ng02 K5 identique, 0 reprise, 5 180 = 5 180, une chaine retriee ; ng00 K10 identique, 7 lots, 2 reprises
  (etendue), 20 263 = 20 263 ; somme des etapes publiees = duree murale a 0,2 ms pres (21 838,3 sur 21 838,5 ms).
- 22:19 UTC : A'' (`66c41ed`), construction Release profil 21 tous modules (`bwork`) : code 0, aucun avertissement ;
  suite rapide `ctest -LE long -j3` : 651/651, code 0 (sentinelle LiDAR sautee) ; porte de style verte ;
  `pipeline_witnesses` 146 controles (plancher 130).
- 22:23 UTC : correctifs regeneres : A'' = `patch_catalogue_gpu.diff` (45 fichiers, sha256 `9eb114da...`), B'' =
  `patch_cst0234_feuille.diff` (4 fichiers, sha256 `ea9795f8...`, inchange) ; `git apply --check` puis application,
  A puis B, VERTS sur des extractions neuves de `99fa83246`, `9c5809919` et du HEAD du moment `136e07628` (aucun
  fichier commun avec les commits de main posterieurs a 99fa83246) ; arbre A+B sur 99fa83246 identique a `frozenB3`.
- 22:27 UTC : A'' avec CUDA (profil 21, module catalogue, nvcc 12.9 local, sm_120) : construction code 0, aucun
  avertissement ; 44/44 portes, code 0 (`device_open` : device_unavailable, pas de GPU local).
- 22:38 UTC : B'' (`3b445b7`, `frozenB3`), construction Release profil 21 tous modules : code 0, aucun
  avertissement ; suite rapide 651/651, code 0 ; empreintes de la voie CPU : les NEUF de F2 retrouvees (`ecarts=0`),
  chaines retriees ng02 K5 1, ng02 K10 2.
- 22:43 UTC : B'' avec CUDA, profil 21 (module catalogue) : construction code 0, aucun avertissement, portes code 0 ;
  profil 32 : construction code 0, aucun avertissement.
- 22:44 UTC : effet local de B (CST-0234), indicatif seulement : etage des feuilles (`count_ns`) de la voie CPU sur
  ng00 K5, 1 fil, sondes A'' et B'' jouees en alternance (3 tours x 2 passes chaudes), codespace charge (charge 8 a 9
  sur 8 coeurs) : mediane A 11,38 s (9,12 a 14,06), B 10,46 s (7,86 a 12,04), rapport 0,92 ; aucune decision (G4).
- 22:50 UTC : diagnostics physiques sur les neuf cas d'identite du plan (voie appareil jouee sur l'hote par
  l'executeur Pool, code de A'', contre la voie CPU, 2 fils ; journal `essais/physique_A3.log`) : catalogue identique
  ET paliers, etendue maximale, reecritures et reprises par cause egaux partout (le juge G4 l'exige desormais) :
  ng00 K5 1 reprise (etendue 17) et 3 990 reecritures ; ng01 K5 3 et 3 772 ; ng02 K5 0 et 5 180 ; u8000 0 et 980 ;
  u16000 0 et 2 235 ; u32000 0 et 4 107 ; ng00 K10 2 et 20 263 ; ng01 K10 13 et 17 647 ; ng02 K10 0 et 25 129 ; les
  feuilles reprises de ces trames n'emettent aucune boule ; toutes les reecritures sont faites sur l'appareil.
- 22:59 UTC : campagne de mutants du catalogue sur A'' (`frozenA3`, `--jobs 3 --build-jobs 1`) : `mutants_ok
  module=catalogue mutants=15 tues=15 dont_signal=1 dont_delai=0 dont_construction=0 plancher=15`, code 0 (temoin
  vert ; `finition_prefixe_inclusif` tue par signal, les 14 autres par code, dont les deux nouveaux mutants des
  compteurs) ; socle : `echange_sans_les_octets` tue (code), code 0. Comptes rendus : `mutA3/report.json`
  (empreinte des sources `abbe985c...`), `mutA3/report_core.json`.
- 23:13 UTC : campagne de mutants du catalogue sur B'' (A+B, `frozenB3`) : `mutants_ok module=catalogue mutants=15
  tues=15 dont_signal=1 dont_delai=0 dont_construction=0 plancher=15`, code 0 (temoin vert, memes causes que sur A'').
  Compte rendu : `mutB3/report.json`.
- 23:24 UTC : B'' sous ASan + UBSan (`-fsanitize=address,undefined -fno-sanitize-recover=all`, module catalogue,
  socle de `7b7d025b3` compile et instrumente) : construction sans avertissement, `ctest -LE long` 44/44, code 0,
  aucun rapport. ERRATUM (23:42 UTC, contre-lecture de l'auditeur `7e87b58d2`, `cuda_b3_portes_locales`) : les
  budgets de ces portes sont construits sans cache (`cache_bytes = 0`) ; le cache actif et l'empoisonnement de ses
  blocs inactifs ne sont PAS exerces par ces 44 portes ; sous cache, seul le cas d'echange de `block_cache_limit`
  (porte du socle, suite rapide Release) exerce une reservation de ce correctif.
- 23:28 UTC : B'' sous TSan (`setarch -R`, commande dans `scripts/chaine_2b.sh`, module catalogue) :
  `mhgp12_catalogue_device_unit` (neuf groupes et son inventaire ; voie appareil jouee sur le Pool a 1, 3 et 8 fils),
  `mhgp12_catalogue_unit_determinism` et `mhgp12_catalogue_scale8000` (comptes v11 a 1, 4 et 8 fils) : 12/12,
  code 0, aucun rapport. Dans ces lots comme partout ici, `device_open` ne fait que constater `device_unavailable` :
  la branche CUDA n'est pas jouee.
- 23:37 UTC : les trois mutants appareil du plan G4 (`bench/g4_catalogue_mutants.json`), appliques comme le pilote
  (`apply_mutant`) puis construits avec CUDA (profil 21, module catalogue, cibles de la sonde et de l'unite de la voie
  appareil) : 3/3 sans avertissement sur A'' et 3/3 sur B'' (dont `un_fil_par_feuille`, qui compile pour l'appareil
  les branches de l'hote de B). Leur jugement (critere identite ou temps) n'existe que sur G4 : NON JOUE.
- 23:42 UTC : B'' avec CUDA au profil 24 (module catalogue) : construction sans avertissement, 44/44, code 0. Les
  profils 21, 24 et 32 sont donc construits avec CUDA sans avertissement sur l'arbre rebase (A'' au profil 21, B'' aux
  trois).
- 23:42 UTC : main avance a `7e87b58d2` (audits et recus, plus les commits de `136e07628`) : A puis B s'appliquent
  toujours (`git apply --check` puis application sur une extraction neuve) ; aucun fichier commun avec le correctif.
- 23:45 UTC : empreintes de la voie CPU de A'' (`bwork`, sonde, 3 fils) : les NEUF de F2 retrouvees (`ecarts=0`) ;
  ng00 K10 voie appareil jouee sur l'hote : catalogue et diagnostics physiques egaux a la voie CPU (2 reprises,
  20 263 reecritures, toutes sur l'appareil) ; somme des etapes publiees 19 306,3 ms sur 19 306,5 ms.
