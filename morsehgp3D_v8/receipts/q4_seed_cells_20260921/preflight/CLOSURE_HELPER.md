# Préflight du helper de clôture

Le 21 septembre 2026, le premier appel de `close_candidate_reads.py` a échoué
avant toute commande lecteur, avec `InvalidReceipt: r2 leak instrumentation missing`.
Ce helper hors des 216 sources supposait à tort que la capture
`qualification_sanitize_r2/smoke_lqs4dx7b` avait forcé les variables des sanitizers.
Son manifeste enregistre `ASAN_OPTIONS=null`, `UBSAN_OPTIONS=null` et
`TSAN_OPTIONS=null`, avec `MHGP8_SANITIZE:BOOL=ON` dans le cache de compilation.

Il ne s'agit ni d'un échec du moteur ni d'un diagnostic ASan/UBSan. La capture
native reste une capture instrumentée avec environnement par défaut ; elle
n'est pas présentée comme une exécution où la détection des fuites a été
explicitement forcée. Aucun benchmark n'a été rejoué par le helper en échec.

Le helper exact est conservé dans
[`close_candidate_reads_before_san_env_fix.py`](close_candidate_reads_before_san_env_fix.py),
SHA256 `1afe6b37618d53a9191f3761e0cd255fc40cc4b5e2f477639519e8b12bd96eae`.
La commande était
`python3 -B morsehgp3D_v8/receipts/q4_seed_cells_20260921/close_candidate_reads.py`
et son code de sortie était 1.

La reprise décidée par le constructeur est une nouvelle capture distincte,
sur le même build R2 inchangé, avec
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
La clôture finale désignera explicitement cette nouvelle capture. Les 216
sources produit/tests/sondes/lecteurs ne sont pas modifiées par cette reprise.
