# ACCUSÉ — GO conditionnel § 5.12 : SHA d'exécution gravé

Date : 1er septembre 2026. Accusé bref prévu par votre § 5.12 (confirmé
§ 5.15 : « seul l'accusé bref du SHA exact restera avant le démarrage
gardé »), après fermeture des coutures § 5.13, § 5.14 et § 5.15
(détail : `REPONSE_CLAUDE_MULTICPU_GPU_20260901.md` § 5).

**SHA d'exécution : `5d886db1`**
(`5d886db1` = commit `serie c v6 : coutures 5.13-5.15 fermees — signatures
canoniques, juge unique fail-fast, pin a treize fichiers, profil 7h`,
poussé sur `main`).

Preuves rejouées À CE SHA, worktree propre :

- `selftest_campagne_v6.sh` : TOUT vert (nominal idempotent, profil G4,
  mutants décision, série C — nominal + 23 falsifications à cause exacte,
  fail-fast ×3, liaison littérale du canon, identité device/en-tête,
  budget gravé 13 552 s / 20 995 s) ;
- `selftest_cycle_vie_v6.sh` : 53/53 (inventaire repo-relatif à 13
  fichiers, 13 refus de pin) ;
- `v6_campaign_pin.sh` rejoué directement sur le HEAD : matérialisation et
  manifeste conformes, `morsehgp3D_v6/tests/pilote_juge.py` épinglé ;
- 117/117 portes `gate` v6 en Release ;
- contrôles racine (`check_docs`, `check_passation`, `check_scope`,
  `check_implementation_status`, `check_contracts`, `check_references`,
  `check_gcp_workflows`, contrats, `test_gcp_safety` 81/81) verts.

Session à venir, conformément au § 5.12 et au profil `g4_serie_c_v1`
épinglé à ce SHA : `CAMPAIGN_PROFILE=g4_serie_c_v1`, une seule VM
g4-standard-48 SPOT, `instanceTerminationAction=STOP`, 7 h GCE
(`maxRunDuration=25200`) / 415 min invité, ≥ 30 min de rapatriement,
selftests à la main avant le lancement (fait, ci-dessus), arrêt certifié
`TERMINATED` sur la génération exacte, succès ou échec. Ordre des phases :
matrice + attribution AVANT tout nvcc, build CUDA (`arch=120` contractuel),
inventaire exact puis 16 portes `gpu`, pilote 4 familles 50k (échauffement
+ 4 répétitions ABBA, juge après chaque famille).

Aucun résultat ne changera `public_status=not_claimed` sans audit du reçu.
GCP non utilisé par cet accusé ; la session démarre par
`session_campagne_v6_g4.sh` seulement.
