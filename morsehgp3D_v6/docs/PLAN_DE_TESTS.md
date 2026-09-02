# Plan de tests v6

Conventions héritées v5, toutes reconduites :

- Portes à code de sortie **exact** via `cmake/run_expect.cmake`
  (`mhgp6_gate`) : 0 conforme, 1 désaccord du juge, 2 refus avant calcul,
  3 plancher/invariant violé, 4 mutant tué. Crash par signal refusé partout.
- Labels : `gate` (défaut), `oracle` (petits n, vérité établie),
  `scale8000/16000/32000` (les seules tailles où une conclusion de coût
  s'énonce), `slow`.
- **Planchers `--min-*`** sur toute porte contre le vert-par-vacuité
  (violation = code 3).
- **Mutants causaux** : registre unique (`src/core/mutants.hpp::kMutants`),
  points d'injection `MHGP6_MUTANT("nom")` dans le code de production,
  compilés seulement sous `MHGP6_TESTING` (posé par `mhgp6_executable`,
  jamais par `mhgp6_product_executable`) ; nom inconnu refusé code 2. Cible :
  chaque nom = un point d'injection + une porte code 4 EXÉCUTÉE. État réel
  (recompté au palier KeyCSR du 2 septembre par
  `grep -rn MHGP6_MUTANT src/ cli/ oracle/`, hors `#define`, passage de correction KeyCSR compris) : 98 noms au
  registre, 100 points d'injection pour 91 noms distincts porteurs d'un site
  hôte (les sept `gpu-*` de census sont traduits en drapeaux device, sans
  site `MHGP6_MUTANT` hôte ; `wspd-cap-terminal` et `wspd-split-heaviest`
  ont un site sur la route FUSIONNÉE en plus de la route brute) ;
  **70 noms distincts** tués par une porte exécutée, dont les 15 `csr-*`
  (`mhgp6_fold_csr_mutant_*` sur le champ annoncé ; la boucle
  `mhgp6_mutant_csr_*` ne reprend que ceux qui changent l'objet — jamais un
  refus compté comme mise à mort) : mutants dédiés + boucle de divergence d'objet
  `mhgp6_mutant_*` + `family-scanline-overshoot`.
  `wspd-drop-rect` est désormais UNE omission par DESCENTE appliquée après la
  fusion ordonnée, masse omise soustraite du grand-livre reconstruit
  (`emis + tués + omis == attendu`, delta −1 littéral gravé par
  `mhgp6_fused_mutant_droprect` ; un mutant hors déclaration rend 3, jamais
  4). Le reste `[PRÉVU]` avec les portes v5 à porter
  (`fold-inject-b-exception-k3` exige le juge d'in-flight dédié : il termine
  par signal, jamais par la boucle de conformité). Un contrôle textuel
  registre ≡ grep est un complément, jamais un kill.
- Équivariance par permutation physique et par réétiquetage (`PointId` ≠
  index dense ≠ rang Morton, mutant `dense-pointid`).
- Sortie **bit-identique** quel que soit le nombre de fils (fils ∈ {1, 8, max})
  et `fold_inflight` ∈ {1, 2, 8} ; ouvriers mesurés, jamais déclarés.
- Jamais de vérification exhaustive : les théorèmes s'invoquent, on grave
  leurs fixtures d'égalité ; exception : oracles bornés n ≤ 12–14 qui
  **établissent** la vérité.
- Oracle à arithmétique volontairement autre (`oracle/obig.hpp`, limbes
  32 bits signe-magnitude, échec fermé par drapeau collant) ; le juge du juge
  (`mhgp6_obig_selftest`) contre `__int128` et une reconstruction
  indépendante.

## Portes par étage — état RÉEL au 31 août ; tout ce qui n'est pas dans
`CMakeLists.txt` est `[PRÉVU]`, jamais implicite

| Étage | Portes |
|---|---|
| cœur arithmétique | `mhgp6_arith_selftest` (bornes, U192/U320, DI128 vs __int128 échantillonné), `mhgp6_sha256_selftest` (FIPS + streaming). `[PRÉVU]` : `mhgp6_level_cmp` contre l'oracle 384 bits (mutant `level-trunc-hi`), `mhgp6_dint_gate` complet, porte d'égalité SHA-NI/portable |
| familles | `mhgp6_families_fixture` : déterminisme, profil, unicité, cardinalité (l'égalité bit à bit aux nuages v5 a été vérifiée hors porte à la livraison — 36 configurations). `[PRÉVU]` : digests gravés par famille et mutant `family-scanline-overshoot` raccordé |
| index | `mhgp6_tree_selftest` (structure, boîtes, équivariance) |
| WSPD | ledger exact `Σ émis + Σ tués = C(n,2) − Σ C(mult,2)` ; mutants `wspd-cap-terminal`, `wspd-split-heaviest`, `wspd-drop-rect` ; `--check-permutation` |
| descente fusionnée | `mhgp6_fused_descent_gate` : listes identiques à la triple descente test-only, avec `smax_effective` (cas `collinear_seven` à 9 points gravé) ; mutant `fused-mask-stuck` |
| fuseaux/facteurs | fixtures W2 ⊃ W3 ⊃ W4, boule-cœur ⊆ fuseau ; route M : porte différentielle contre le produit direct (`min(hist, need)` par lane), mutants `endpoint-credit-use-weight`, `factor-none-overclaim` |
| crédits/tape | mutants `credit-compose-sum`, `core-partial-exclude` ; fixture croisée de lanes (W3-pas-W4) ; fixtures rôles A∪B (membre de A complétion valide, seed valide) |
| tueurs d'ancre | fixtures F1–F11 portées ; secteurs : fixture croisée + mutant `sector-credit-global` ; grille : fixtures F9/F10 + mutants `cell-kill-h-minus-one`, `cell-kill-nonstrict` |
| sweep q4 | **oracle du sweep** (re-balayage exhaustif en μ, échange des quantificateurs, racines/frontières) ; fixtures : relais `F1=μ+1, F2=1−μ`, racines confondues, complétion incidente (compte zéro), clip d'égalité à μ*, les trois cas B=0, sortie dans cellule profonde avant portion shallow ; mutants `sweep-drop-exit-root`, `sweep-nonstrict-depth`, `sweep-skip-fragment`, `sweep-completion-from-witness-tape`, `chord-dead-skip-positive` (hérité) |
| RLE/census | mutants `rle-drop`, `depth-threshold-minus-one`, `range-add-max-le-zero`, `census-nonstrict`, `skip-full-census` ; fixtures plateau (carré cocyclique) |
| fold/rendu | mutants `drop-nonmerge`, `attach-prebatch`, `repr-ties`, `binary-ties`, `canonical-is-uf-root`, `fold-inject-a-failure-k2` tués (boucle de divergence d'objet) ; sondes d'ablation du reduce `ablation-mat-sans-copie`, `ablation-mat-sans-tris`, `ablation-post-cle-factice` (chacune change l'objet, tuée dans la même boucle ; binaire `mhgp6_profile_sonde` sous `MHGP6_TESTING` seul à accepter `--inject=`, le produit refuse : `mhgp6_profile_refuse_inject`, `mhgp6_profile_sonde_refuse_inconnu`, allowlist des trois ablations sans item vide `mhgp6_profile_sonde_refuse_inject_vide` / `_virgule` / `_mutant_production` ; reçu `bench/sonde_ablation_reduce.sh`, jamais un mur). **KeyCSR** (stockage `csr_facet_keys_v1`, route `--layout=csr`, même objet, aucune route de repli) : `mhgp6_fold_csr_{fixtures,offsets,overflow,copie,pipeline}` — 13 fixtures gravées du fold + refus amont sous csr (kind csr signé, payload vide, `--min-refus`) (bras classique contre texte, compteurs et pins de digest ; bras csr contre le classique par `first_divergence`, lecteur tiers de `tests/forest_witness.hpp` lisant les deux stockages À CRU, sans l'accesseur `delta(i)`), validateur d'offsets (cinq contrôles, message exact), gardes de capacité (plafond d'append ET majorant, deux crochets test-only distincts), copie autonome post-callback, matrice fils × inflight × join × layout ; 15 mutants `csr-*` tués sur le champ ANNONCÉ (`--expect-divergence=`, sinon code 1, jugé en `--fixtures` comme en `--overflow`) + refus pipeline `mhgp6_fold_csr_refus_csr-offset-*` (invariant, zéro callback, provisoires vides — SEULE preuve des `csr-offset-*`, exclus de la boucle de conformité où tout statut non complet vaudrait 4 par vacuité de refus) + `csr-inject-bad-alloc` (`bad_alloc` d'arène capturé dans le fold → `resource_exhausted`, payload vide, zéro callback, jamais une exception hors de `reduce_fold`) + boucle `mhgp6_mutant_csr_*` (mutants qui changent l'objet seulement) ; rejeu « catalogue + deltas → partition » (seconde autorité, deux layouts, fixtures et pipeline) ; dent de compilation : `delta(i)` et `for_each_delta` refusés sur un `ForestResult` temporaire ; pré-inscription de mesure `mhgp6_plan_keycsr_gate` (`bench/plan_keycsr.py` : graine dérivée `0xa2ffb4db2884ddc4`, SplitMix64, Fisher–Yates spécifié, fixture des orientations). `[PRÉVU]` : juge borné n ≤ 14 (miniboule + cliques + Kruskal à lots), K=1 ≡ MST indépendant, juge d'in-flight (`fold-inject-b-exception-k3`), `render-active-only`, planchers |
| conformité v5 | `mhgp6_conformity_*` : `digest_all` + `digest_forest_K*` (l'OBJET) ≡ `receipts/conformite_v5/` sur 5 familles × {8000, 16000, 32000} (labels scale*) et petites tailles en `gate` ; le digest candidats v5-compat est rapporté, jamais un critère (cover q4 coefficient 4) ; golden post-préfiltre v6 gravé (uniform 400) ; + `mhgp6_conformity_csr_*` : MÊMES reçus sous `--layout=csr` (8 petites en `gate`, 15 `scale*`) avec non-vacuité `csr_fallback=0` et `ordres_storage_conformes=kmax_eff`, refus 2 d'un layout inconnu (`mhgp6_conformity_refus_layout_inconnu`, `mhgp6_cli_refus_layout_{inconnu,vide}`), signatures CLI `mhgp6_cli_layout_{classic,csr}_signature` |
| cover q4 | `mhgp6_cover_coef4` (contre-fixture tétraèdre régulier + z, frontière de génération) + mutant `q4-cover-coef3` |
| barrière de génération/census | `mhgp6_linked_arcs_u16` + mutant d'oracle i64 (portée : génération→census ; l'extension aux facettes de forêt est `[PRÉVU]`) |
| frontières du sweep | `mhgp6_sweep_frontieres` (F1–F5 : racines égales, extrémité de Jung exacte, B=0, complétion dans le facteur, profondeur h4−1) + 2 mutants |
| parallélisme | mutants `par-drop-shard`, `par-drop-ball-chunk` tués (boucle) ; `mhgp6_fold_csr_pipeline` : matrice fils {1,8} × inflight {1,2} × join {0,1} × layout {classic,csr} sur les sept familles des petites conformités, témoin complet par K (`first_divergence`) contre la référence classique 1 fil, digests/cartes/totaux identiques, rejeu csr par K. `[PRÉVU]` : `parallel-sort-unstable`, `fold_inflight=8` |
| profil reduce (§ 5.10) | cibles EXPLICITES `mhgp6_profile` et `mhgp6_profile_liveness` (identité de build signée par la cible, jamais par des flags) ; `mhgp6_profil_identite` : la PROJECTION DÉTERMINISTE NOMMÉE (`digest_all` + `digest_forest_K*` + `cardinalites K=` — ni `batch_levels` ni le `ForestResult` complet) identique entre normal/profil/vivacité × join 0/1 × layout classic/csr (jeton `layout=` exigé sur `profil_kind=`, aucune colonne ajoutée à `profil_reduce`/`profil_intern`), builds DISCRIMINÉS (zéro ligne `profil_*` côté normal), structure valide (K cohérents entre forêt/cardinalités/reduce/intern/vivantes, temps finis non négatifs, fermeture somme/résiduel aux bornes INTERNES `mur_reduce_interne`, planchers strictement positifs), causalité de `fold_join` (chaîne A→reduce ordonnée par K ; join=1 ⟹ sérialisation inter-K et pics à 1), vivacité (pic intra-lot > 0, frontière ≤ pic) ; `mhgp6_profil_contrat_echec` + `mhgp6_profil_contrat_echec_k2` (COMPILÉS, inspectent le `RunResult` — le CLI ne print jamais après un refus ; scène K2 sous jonction, profil K1 non vide vérifié au callback, effacement au terminal) ; `mhgp6_profil_contre_fixture` (§ 5.13 : la scène « neuf composantes nulles, somme=0.008, residuel=0.012, mur=0.020 » tuée par les seuils serrés 0.0051/0.006 ; audit post-session : DENTS ISOLÉES — dérive de somme 0.008 à fermeture exacte tuée par la seule dent somme avec son message, écart de fermeture 0.007 à somme exacte tué par la seule dent fermeture, frontière honnête 0.005 acceptée — le juge exercé est le VRAI `check_profile_output` importé). Le binaire de profil n'est JAMAIS un mur de débit |
| caps/budget | `mhgp6_caps_refus` (fenêtres (u)/(a)/(a0)/(a2)/(b)/(c)/(w)/(w2)/(f)/(d)) + mutants `caps-drop-emission`, `caps-late-wave-check`, `caps-skip-prefusion-budget` (garde 2E AVANT la fusion globale, § 6.1 de la réponse auditeurs — sautée, le refus retombe sur le tri APRÈS la matérialisation du payload logique nommé) ; signature CLI `mhgp6_cli_budget_signature` (code + ligne exacte en une exécution) |
| GPU série C (hôte) | `mhgp6_executor_pool` + mutants `pool-serial`, `pool-drop-exception`, `pool-worker-resume-after-fatal` (fixture permanente scénario 10 : second travail en file, fatal déclenché, AUCUN travail post-fatal — la course § 5.6), `pool-activate-after-unlock` (scénario 13 : hook test-only, fermeture linéarisée après le pop — le ticket doit être VU actif) ; scénarios 11 (échecs de construction) et 12 (fenêtre file→actif N=2) ; témoin stub `mhgp6_device_witness_stub` : nominal 0, trois dents à 4 (carry, skip-arith, skip-native — tableaux séparés) + contre-fixture composée skip+carry gravée à code 1 (preuve C++ hôte — jamais un reçu device) |
| GPU série C — wire et kernels (hôte) | `mhgp6_wire` (aller-retour bit-exact du `GpuCloudIndexWire`, t1 contre balayage exhaustif, 3 refus hors-domaine, digest gravé) + mutants `gpu-index-drop-node`/`wire-t1-plus-one` ; `mhgp6_census_device_stub` (bit-identité boule à boule contre le scalaire sur candidats réels, 2 familles) + 4 mutants (`gpu-range-add-le`, `gpu-stack-shallow`, `gpu-swap-push-order` à multiset égal, `gpu-census-nonstrict`) ; `mhgp6_pilot_stub` (pipeline COMPLET : objet identique CPU vs route série C, refus du run entier sous mutants) ; `mhgp6_pilote_stub_*` (syntaxe/logique du pilote .cu, refus de parsing) ; JUGE DES RECORDS (§ 5.13-5.15, `tests/pilote_juge.py` — LE MÊME juge pour la porte stub, le runner G4 en mode fichier et le validateur) : `mhgp6_pilote_juge_contre_fixtures` (27 flux falsifiés intégrés tous tués : records/ABBA/signatures recalculées/formules d'octets 112-100-100/chronos fermés et enveloppants/stabilité inter-répétitions/identité d'en-tête famille-n-graine-fils-**arch**/lot_effectif=min/parité imprimée/grammaire hex64), `mhgp6_pilote_stub_juge` + `_ordre_inverse` (le pilote stub réel jugé sous les deux ordres de base) |
| GPU série C (device, `MHGP6_ENABLE_CUDA`, G4 seulement) | `mhgp6_device_witness` + 3 mutants (socle arithmétique PARTIEL) ; `mhgp6_census_device` + 4 mutants (jumelle device de la porte stub, arch compilée signée) ; `mhgp6_pilote_parite_400` + refus de parsing (pilote `mhgp6_cuda` : deux routes, parité de tous les digests, coûts wire/H2D/kernels/D2H). `[PRÉVU]` : contre-fixture composée du témoin portée sur device ; le reçu de GAIN = profil de campagne G4, jamais un gate |

## Fixtures permanentes aux coordonnées exactes

Corpus hérité : carré cocyclique (110/100/90...), `q2_one_interior_attachment`,
croissance unaire, cœur q4 discriminant, « dix témoins q2 qui ne ferment pas
q4 », fixtures q4 13/22 points, skinny 89°–89°–2°, témoin de forte
annulation, contre-familles `two_lines`/`collinear_seven`, F1–F11 des tests
d'ancre, sphère diamétrale à 37 sites. Corpus neuf : fixtures du sweep
(ci-dessus), `linked_arcs_u16`, fixture de masque de lane
`a=(1000,1000,1000), b=(2000,1000,1000), z=(1010,1016,1000)`,
calotte–lentille (V6-Q4), peigne de facteurs singletons. Fixtures du fold
(palier KeyCSR, `tests/fold_csr_gate.cpp`, dérivées à la main puis
re-dérivées machine, pins du bras classique) : F1 born-only, F2/F2b
parents-only (support inversé), F3 continuation (`batch_levels` de taille 2
pour un seul delta), F4/F4-min multi-parents S5 (ordre de `pre_list` ≠ ordre
des canons), F5 S2 (deux racines post d'un lot, ordre par racine UF ≠ ordre
par output), F6 forêt vide, S1/S1a/S1b inter-segments (output figé au lot,
niveau = représentation du premier événement du lot), R2/R2b encodage réel
K=2 ; refus amont (deux identifiants égaux) sous les deux routes : même
refus, kind csr signé, payload vide.

## Campagnes

Mesures d'échelle : compteurs déterministes (grand-livre), 5 familles
dilatées + 2 stationnaires × {8000, 16000, 32000} × graines {3,4,5} ; pentes
sécantes par terme ; reçus immuables dans `receipts/` (pin, hash de binaire,
sorties brutes). Temps : localement seulement en banc apparié contrebalancé ;
sinon G4 avec reçu.

Le validateur `bench/pentes.py` est prouvé fail-closed par
`tests/pentes_gate.py` (cinquième cycle : nominal + 20 falsifications à
code 3 et stdout vide — dont famille dupliquée du META, entier invalide sans
traceback, compteur dupliqué, digest dupliqué/non hexadécimal, fichier
d'extension inattendue, identités fermantes des octaves violées — + zéro
légitime sur un compteur réellement parsé avec `-` affiché). Le juge de
conformité refuse une référence à clefs de forêt HORS PROFIL (ensemble exact
`{1..kmax_eff}` exigé ; porte `mhgp6_juge_refus_k_en_trop`, K1 correct + K10
en trop à n=2) en plus du narrowing et de la référence tronquée. Le
protocole G4 v6 (`gcp-migration/session_campagne_v6_g4.sh` : conformité
v5≡v6 à 50 000, bench apparié ABBA sans digest, queue stationnaire) a son
selftest transactionnel à faux pilotes (`selftest_campagne_v6.sh`, à lancer
à la main avant toute session payante).

Reprise persistante (audit série C § 5.18.6, `selftest_cycle_vie_v6.sh`) :
le bootstrap matérialise `WORK` dans une base 0700 persistante sur le
volume du dépôt (`/workspaces/.ehgp-sessions`, jamais `/tmp`), le cycle de
vie s'exécute en session de processus propre (`setsid`) et publie avant
toute mutation `session.env`, `superviseur.pid` (pid + starttime +
boot_id) et `marques/` ; le garde `start_and_verify.sh` y publie deux
marques exclusives et distinctes — `guest_guard_pending` (génération
certifiée par la garde GCE) puis `double_guard_verified` (armement invité
relu). `recover_v6_session.sh` (épinglé, deux étages ré-authentifiés par
`git show`) ne démarre JAMAIS la VM : superviseur vivant → refus ;
registre `targeted_stopped` → rien ; sans seconde marque → arrêt immédiat
sur la génération exacte ; avec → scp bornée, un STOP, validateur épinglé,
classification FORCÉE `partiel_ou_invalide`, reçu `…_reprise_<epoch>`
jamais une décision ; génération inconnue → blocage 71 avec la commande à
lancer à la main. Scénarios : SIGKILL de toute la session après le
handshake puis reprise (R1), superviseur vivant refusé (R2), tué entre les
deux marques (R3), mutants génération discordante / copie épinglée altérée
/ cible discordante / pid recyclé (R4), scp en échec (R5), base 755 refusée.
Durcissements du contre-audit (`CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902`) :
exclusion par verrou noyau (`flock -n` tenu jusqu'à la sortie, pid/starttime
en diagnostic) ; vivacité par pid + starttime + boot_id ET par sid/pgid
gravés dans `superviseur.pid` (cinq champs) ; registre strict (un état qui
implique une cible démarrée porte une génération non vide) ; marques
parsées comme objets stricts (fichier régulier, keyset exact, `mark=<nom>`,
génération non vide) ; describe en tuple exact (RUNNING, génération) avant
la scp, scp vers un staging puis relecture du tuple avant promotion, toute
autre génération → 71 sans STOP ; `REMOTE_DIR` lié à (commit, époque de la
génération) ; entrée en `targeted_stop_failed`/`targeted_stopping` →
STOP-FIRST (ni describe, ni scp, ni validateur avant l'arrêt certifié),
arrêt non certifié → témoin MINIMAL ; purge des credentials VÉRIFIÉE avant
le témoin `recu_publie` (reprise et cycle nominal), échec → rc 67 et
re-purge locale sans appel GCP ; la reprise n'exécute QUE la garde d'arrêt
épinglée ré-authentifiée (le harnais exerce la VRAIE `stop_and_verify.sh`
sous un faux `gcloud`) ; politique des rejeux explicite (manuels, un STOP
chacun, jamais de boucle). Dents : deux reprises simultanées (D1),
`targeted_stopped` sans génération (D2), génération concurrente avant /
pendant la scp (D3), marque au champ `mark=` falsifié (D4), orphelin de
session sans `WORK` dans son argv (D5), purge en échec puis re-purge locale
(D6, D8 nominal), stop-first et troisième rejeu (D7). Coutures du § 5.21 :
tous les choix terminaux (session déjà conclue comprise) sont pris SOUS le
verrou ; le staging n'est promu en `out/` que si la scp a réussi et si le
tuple postérieur est lisible, exact et de la génération attendue, sinon le
partiel est conservé sous `out.partiel_<epoch>` et seul l'arrêt ciblé se
poursuit ; l'échec de la publication du témoin domine le succès (code 68,
marqueur `temoin_non_publie`, dans la reprise et le cycle nominal) ; le
témoin minimal après arrêt non certifié ne porte que des champs fixes et des
tails plafonnés à 64 Kio (ni `out/`, ni `marques/`, ni `sync`) ; les rejeux
sont bornés par tentative, jamais par un ledger ; la vivacité couvre
l'identité sid/pgid enregistrée, pas un descendant qui refait `setsid`. Dents :
témoin non publiable (D9 reprise, D11 nominal) puis publication sans appel
GCP, describe indisponible après la scp (D10 : partiel conservé, rien promu).
Le revalidateur recompare aussi l'inventaire des répertoires (mutant :
répertoire vide créé par le validateur). Dents du § 5.22 : funnel d'arrêt
inconditionnel (dès que la génération est connue, une erreur locale pré-STOP
déclenche exactement un arrêt ciblé, rc 74, témoin minimal — D12, faux `tee`
en échec après la scp) ; l'arrêt précède toute promotion ou sauvegarde
locale ; provenance de `out/` par marqueur atomique `out.promotion`
(génération, commit, `scp_rc`), seul un `out/` promu par CE rapatriement est
validé (D13 : `out/` résiduel + scp en échec ⇒ aucun validateur) ; purge
nominale incomplète ⇒ code 67 (priorité 67 > 68 > 0/65, D8 l'exige) ; reçu
minimal aux champs bornés (marques connues seulement, résidus non énumérés) ;
D11 causal (rendez-vous fatal, exactement un STOP, fast-path et
`issue=arret_certifie_par_le_garde`) ; les manifestes de reçus n'excluent que
le `SHA256SUMS` racine (un `out/SHA256SUMS` est inventorié : leurre du faux
scp assert en R1). Revalidateur : validateur canonique authentifié (sha256
gravé ; autre chemin seulement sous `EHGP_REVALIDATE_SELFTEST=1`), résumés
attendus exigés après l'appel (juge muet ⇒ rc 3), inventaire NUL injectif
(type, mode, nom ; nom à saut de ligne refusé), « intact » = noms, types,
modes et octets — `selftest_revalidate_v6.sh` (22 scènes : un résumé
re-produit différent d'un octet ⇒ rc 3 ; répertoire racine « out marques »
refusé par l'allowlist NUL en Python). Retour des auditeurs sur le WIP § 5.22 :
funnel armé dès la génération résolue (sous `errtrace`, hérité par les
fonctions), garde d'arrêt exécutée D'ABORD et tout en best effort autour
(registre, journal), 70 domine 74 ; marqueur `out.promotion` lié à un
identifiant de TENTATIVE (D13 précharge un marqueur valide d'une tentative
antérieure : aucun validateur) ; dents D14 (panne de journal dès la génération
connue ⇒ un STOP, rc 74) et D15 (panne de `publish_state` ⇒ garde exécutée
exactement une fois, code non nul, aucun témoin).
