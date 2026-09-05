# First-C : clôture par composition avec le juge v2

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**La demande first-C est désormais traitée par un compagnon distinct**, SHA-256 `8f8aed03755d9c92775566b21d4fdd9dcba31f171adf4b83e9802a988a450370`. L'autorité qualifiée est la conjonction du juge v2 scellé `8d8a612a` et de ce compagnon. Celui-ci ne remplace ni les contrôles de terminal, de géométrie déclarée, de digest et de protocole du juge v2, ni les scellements du paquet. Les [observations historiques](probe_admission_review.md) du juge v2 restent inchangées.

Le compagnon impose, sur un ordre lazy réussi, `cache_inserts = min(cache_entries, portal_requests)` et `cache_skips = max(0, portal_requests − cache_entries)`. Il conserve seulement les bornes de préfixe pour un refus et pour le dernier travail diagnostique. C'est le domaine correct : une insertion est payée avant l'allocation qui peut échouer, et une requête interrompue n'a pas nécessairement atteint son insertion ou son skip.

Les [rejeux normaux](probe_admission/companion_normal.json) et [optimisés](probe_admission/companion_optimized.json) passent sur les **24 vrais reçus d'admission clos**, avec **117 lignes lazy contrôlées**. Le [runner de composition](probe_admission/replay_companion.py) vérifie que les verdicts v2 déjà conservés portent sur exactement le même paquet et le SHA du juge exigé par le compagnon. Il rejoue ensuite le compagnon sur chaque reçu. Les deux autorités sont donc liées aux mêmes entrées, sans supposer qu'un succès isolé du compagnon suffit.

Pour chaque mode Python, les selftests du compagnon passent sur une vraie capture EAGER puis une vraie capture lazy C1 : douze mutations distinctes rejetées par fixture, neuf modèles scalaires de succès et trois modèles de préfixe refusé. Les dix-neuf mutations historiques du juge v2 restent attribuées à leurs propres rejeux. Ces contrôles de données ne sont pas des mutants moteur.

La contre-fixture issue du vrai reçu `n8_s8_k10_lazy_c1000000`, ligne K2, est également rejouée. Deux requêtes et une capacité d'un million exigent deux insertions. Le remplacement coordonné du brut et du miroir par une insertion et un skip passe toujours le cœur v2, mais le compagnon le refuse exactement avec `first_c_success`, normalement et sous `-O`. La conjonction refuse donc ce défaut. La corruption ne préserve pas les hashes du paquet historique et ne constitue pas un contournement de ses scellements.

La [map SHA exhaustive des fichiers publics](probe_admission/public_files_sha256.json) contient les 469 chemins du paquet d'admission : 467 entrées du manifeste, le manifeste lui-même et SHA256SUMS. Elle provient de fichiers dont l'inventaire, les tailles et les hashes ont été vérifiés pendant les rejeux. Elle ne contient aucune pièce de la campagne lourde.

Les commandes Python de cette clôture ont été exécutées avec affinité CPU0. Aucun moteur, build, benchmark ou commande GCP n'a été exécuté ; aucun résultat de `heavy_paired` n'est consommé. Cette clôture renforce le jugement des compteurs, sans nouvelle autorité géométrique, performance ou qualification à K10 effectif. GCP non utilisé.
