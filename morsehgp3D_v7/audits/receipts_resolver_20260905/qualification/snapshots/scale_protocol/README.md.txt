# Sonde F seule, 16 000 et 32 000 points — préparée, non lancée

Périmètre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=bounded_F_scale_observation`, `public_status=not_claimed`. Cette préparation ne promeut ni l'exactitude HGP ni un SLO. Aucun test moteur, compilation ou GCP n'est autorisé par la préparation ; chaque lancement exige un nouveau GO racine après la qualification F et les paires 8k.

Chaque palier est une observation indépendante du seul binaire F `ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85`, reçu de build `522c950c70b60ca58759c4fa9b9a24ff995fe829b9aa1adf5b2f51b7b2177ac4`. Aucune moyenne avec les trois paires E/F à 8k, aucun ratio de gain. Les sources de build restent liées à la lignée courante, pas à une reconstruction présumée de F.

## Identité et limites immuables

- `uniform`, `n=16000` ou `32000`, `coord=65536`, seed 3, `s=8`.
- Toute la tour K=1…10 : `--smax=11 --complete-incidences`, route candidate horizontale, sans substitution par Gabriel.
- `--threads=1 --fold-inflight=1 --fold-join=1 --layout=csr --digest`, aucune archive. Les compteurs de workers et la sérialisation sont jugés ; le runner ne mesure pas lui-même les créations de threads.
- CPU6 pour le futur moteur, 600 secondes et `RLIMIT_AS=26 GiB` par processus. Cette limite d'adressage n'est pas un plafond de RSS physique.
- Proxy mémoire partiel 17 179 869 184 octets ; caps `core_records=8000000`, `chain_steps=2000000`, `cofaces=2000000`, `query_nodes=1000000000`, `meb_supports=1000000000`, tous conservés depuis le runner apparié.

## Réutilisation et autorité

Le runner E/F `20f956612c598da256e24f8de893e7df5132f6dff0221dbc2edcdd7fe2ecce3d` fournit les protections historiques C/C/D/E et les snapshots. Son autorité historique `3abe27f8a10173867a0395430ff56392bb82b5cbee3eb766fb293ffa65cc2645` fournit la liaison binaire–sources–build, les caps, les garde-fous d'environnement et le complément de validation mono/coûts. Le pilote de processus est celui de `compare_v6_v7.py`, pin `fe22493fcb7494813e79fea9826873cce8cf14918097ce500b44018f1a64f2ef`, avec arrêt du groupe précis en succès, timeout ou exception.

Le classificateur **inchangé** `incidence_campaign.py`, pin `6ca21d8b1c89e6baea99ecc3dd414b35d06581df50833e76bac9bdc1d5d1c20a`, reçoit simplement son paramètre `n` existant. Aucune substitution de source ni normalisation des sorties. La seule adaptation est le remplacement unique du token argv `--n=8000` ; les selftests doivent vérifier ce remplacement, l'acceptation des deux identités et le rejet d'une identité différente. Les sorties synthétiques des selftests sont explicitement des fixtures, jamais des bruts moteurs.

## Reçus et statuts

Répertoires neufs, refusés s'ils existent : `n16000_receipts/` et `n32000_receipts/`. Chaque tentative conserve commande, stdout, stderr, fichier GNU time, statuts/dates/code de sortie, snapshots avant/après, provenance, observation hôte et hashes. Le refus avant admission ne crée pas de reçu moteur. Le runner protège aussi ses trois fichiers de protocole pendant l'exécution ; les sources v6/v7 produit, CLI, oracle et CMake sont couvertes par les helpers, sans prétention de gel de tous les documents du dépôt.

Une complétion exige identité, caps, ordres K=1…10, cardinalités, les dix digests de forêt et leur digest chaîné, coûts de pipeline/étages/processus et RSS externe. Cela valide la structure de cette observation, pas l'exactitude mathématique de son objet. Un refus reconnu conserve ses diagnostics sans publier de préfixe ; une censure conserve les bruts partiels sans les présenter comme complets. Un fichier GNU time incomplet sous censure ne fournit pas un RSS à inventer. La mémoire/coûts de refus restent dans les diagnostics bruts jugés par le helper ; les champs de succès ne sont pas recyclés.

Codes du runner : `0` observation moteur complète ; `2` observation de refus ou censure correctement conservée ; `1` observation invalide ou échec. Dans les deux premiers cas, le résumé `observations_completed` signifie que l'observation est close, pas que la tour a nécessairement réussi. L'état moteur est séparé dans `runs.json` et les compteurs de succès/refus/censure du résumé.

## Commandes inertes

```bash
taskset --cpu-list 0 python3 -B build/v7_f_scale_20260905/runner.py --n=16000
taskset --cpu-list 0 python3 -B build/v7_f_scale_20260905/runner.py --n=32000
taskset --cpu-list 0 timeout 30s python3 -B build/v7_f_scale_20260905/selftest.py
taskset --cpu-list 0 timeout 30s python3 -B -O build/v7_f_scale_20260905/selftest.py
```

`--execute` est absent de ces commandes. Les selftests n'appellent ni moteur, ni CMake, ni CTest, ni Git : subprocess et métadonnées Git sont simulés dans les scénarios de cycle de vie. Aucun nouveau test dynamique de drain n'est revendiqué ; le helper déjà épinglé est réutilisé. GCP non utilisé.
