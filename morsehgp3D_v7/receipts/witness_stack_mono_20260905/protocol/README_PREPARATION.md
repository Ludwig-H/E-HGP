# Protocole E/F mono — préparation seulement

Port étroit du runner D/E `build/v7_next_pair_20260905/runner.py`, SHA `ae95c0cb65a1564edb2d45ae83671e204d70994d00356aea87aba147ed8ad3ba`, et de ses selftests `57ec5c55…`. Les anciens fichiers et reçus restent intacts. Les helpers, parseurs, caps, snapshot source/build, drainage des processus et comparaison stricte des six projections sont conservés. Seuls les rôles publics, le schema de reçu et la liaison de baseline changent.

E est `build/v7_next_q2_qualification/mhgp7`, SHA `df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6`. Il est lié au build public `morsehgp3D_v7/receipts/meb_q2_mono_20260905/build_E/build_D.json`, SHA `30f133d6c052e1d53c8c539457b5b553267d202c11b043188feb6c0f30af1e16`, et à la qualification324 publique. Les sources E restent historiques, jamais celles du worktree F. Les quatre CLI C/C/D/E sont protégés, ainsi que les métadonnées de compilation E. F vise `build/v7_f_qualification/mhgp7` ; son SHA est encore inconnu dans cette préparation et son reçu de build réel doit être fourni explicitement avant exécution.

Plan fixe : **E puis F**, deux processus frais. Tour candidate complétée K1..10, uniforme n=8000, coord=65536, seed=3, CSR, digest inclus, `--complete-incidences`, aucune archive. CPU logique6, threads=1, fold-inflight=1, fold-join=1 ; timeout600s et RLIMIT_AS26GiB par processus (espace virtuel, pas plafondRSS), proxy partiel16GiB ; caps silent8M core/2M chain/2M cofaces/1 milliard queries/1 milliard MEB par ordre. `normalized_horizontal_h0_candidate`, `public_status=not_claimed`. L'absence de création de threads reste une porte produit séparée du runner.

1. Après gel source, build F et portes intégrées nouvelles réussies, demander GO pour la paire **s8** seulement.
2. Vérifier deux completions, égalité stricte digests/cardinalités/comptages/silent/caps, pins stables et absence de défaut. En cas de refus, censure, sortie invalide ou divergence, conserver le reçu et ne pas lancer automatiquement les autres séparations.
3. Après revue de cette première paire, faire **s10**, puis **s12**, chacune avec sa nouvelle observation E et F et son dossier neuf. Les chronos E historiques et la paire s8 ne remplacent jamais une baseline fraîche aux autres s. Comparer aussi les objets entre séparations, distinctement des compteurs de travail.

Le contrôleur humain/root porte ces GO successifs ; le runner n'invente pas de stratégie de reprise. Aucune permutation implicite : l'ordre reste E→F et le juge rejette F→E. Un éventuel ordre alterné exigerait un protocole explicite, testé et revu **avant** lancement ; il n'est pas ajouté ici.

Prévisualisation inerte :

```bash
python3 -B build/v7_f_pair_20260905/runner.py --candidate build/v7_f_qualification/mhgp7 --separation 8 --output build/v7_f_pair_s8_20260905
```

Seulement après GO, ajouter `--candidate-build-receipt /chemin/du/recu_F.json --execute`. Le schema de build validé reste `mhgp7-mono-meb-build-v1` ; un filename legacy `build_D.json` n'attribue pas D au candidat F. Choisir des noms distincts pour s10/s12, ne reprendre ni écraser aucun dossier existant. Les mutations de sources/helpers/binaires/cache/base/récépissé aux frontières invalident toute égalité, y compris après la collecte finale Git. Refus/censure restent distincts ; une paire incomplète n'a ni égalité ni ratio.

Les 21 selftests normaux et `-O` sont rejoués avec rôles E/F : 12 historiques inchangés et 9 adaptations, dont24 scénarios lifecycle synthétiques et les erreurs de séparation. La seule sonde processus réelle est le drainage hérité d'un minuscule descendant pendant1s ; aucun moteur ni benchmark. Les tests sont épinglés CPU0. Cette préparation ne qualifie pas F, ne démontre pas un gain statistique ou un SLO1s/100ms, et n'effectue aucun Git/GCP.
