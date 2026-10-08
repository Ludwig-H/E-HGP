# Repo6 : premières chaînes de la composition, profil 21

Lecture des traces closes le 8 octobre 2026, sans nouvelle exécution native.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Ce jalon porte sur **repo6/final6**, distinct de
[repo5 et de son arrêt au profil 32](../audit_tmv_repo5_profils_20261008/README.md).

Source annoncée : base `a5e0dbc776f2817f4e7e3a23fc791f1035948b47` et patch
`b3c78ae365a99568457bc6cd5d894a1b73d02b7b19ae3d774f9fdfa094a58bab`.
L'arbre réellement relu comporte 382 fichiers, empreinte
`1b5d1fb38c2c2cbcbfaac86525eeaaeeab0f044afb1a4aa490a6f249d5b73279`, inchangée
depuis notre observation après MES-M0 et avant/après cette capture. Le cache
désigne repo6, Release, profil 21, tous les modules et **CUDA OFF**. Les sources,
le conducteur, les logs, le cache et les deux binaires joués sont épinglés.
Cette lecture ne reconstruit pas la chaîne de compilation.

Les six étapes closes ont chacune un code 0 et un bilan primaire concordant :

- construction à 01:28:03 UTC ; suite rapide à 01:33:45 : **691 sélectionnés,
  690 Passed, sentinelle LiDAR Skipped** ;
- MES-M0 à 01:38:55 : **9 cas × 2 modes** de cibles, identité à l'octet du
  profil 21, 1 contre 8 fils, JUG-EMST sur l'ordre un ;
- chaîne native CPU C/G → T/M/V/R → export : trois lots de trois cas clos à
  01:49:35, 01:50:16 et 01:51:27, **9 cas conformes** aux empreintes sémantiques
  FULL de la v11, avec déterminisme à 1 et 8 fils dans chaque cas.

La cohorte comprend les uniformes 8k/16k/32k à K5 et ng00/01/02 à K5 et K10.
Les logs de la porte d'admission confirment aussi 160 étapes, 40 égalités du
pic, 25 383 lignes et 685 contrôles sans échec. Aucun payload ni export FULL
n'est parcouru par ce lecteur : les bilans sont ceux des pilotes épinglés,
recoupés avec leurs codes, caches, binaires et sources.

Cette fois, les chaînes exécutent la **composition G-c + TMVR** : module
`12219c37…` et manifeste `75fa8a67…` correspondent au
[raccord proposé](../composition_gc_tmvr_20261008/README.md), avec la correction
du comparateur de naissances et la portée du validateur explicitement réduite.
Ce dernier ne reçoit pas ici les gardes de l'historique altéré (CST-0240).
Les contrôles Release ne deviennent pas une qualification `_GLIBCXX_DEBUG`.
La campagne suivante de **27 mutants n'est pas close dans ce jalon** ; les
16 mutants de repo5 ne lui sont pas transférés. Les profils 24/32 de repo6
restent également non qualifiés ici. Cette source prototype n'est pas encore
une livraison produit fusionnée sur main à la capture.

Le conducteur enregistre immédiatement le code comme argument de `step`, ou
avant la substitution de date pour une chaîne. Sa sortie finale seule ne
prouve pas la campagne : le lecteur exige les six codes nommés et leurs
marqueurs, sans hacher le fichier de codes encore alimenté par la suite.
Les codes de configuration CMake ne sont pas enregistrés séparément ; leurs
logs et les caches font partie des pièces épinglées.

Relecture depuis ce dossier :

```sh
python check.py --prototype DOSSIER_TMVR
python -O check.py --prototype DOSSIER_TMVR
```

Les deux sorties doivent être identiques au champ `result` de `capture.json`.
Le lecteur réutilise seulement la fonction d'empreinte d'arbre du
[reçu de traces antérieur](../audit_tmv_traces_20261007/README.md), elle-même
épinglée. Une dérive du patch partagé b3c78ae3 ou des sources fait refuser la
relecture ; elle ne transforme pas rétroactivement les traces en preuve du
code nouveau. Aucun build, moteur, GPU, GCP ou nouveau chrono n'est exécuté.
Il ne découle de ce jalon ni un contrat FULL à 100 ms, ni une qualification
GPU de la chaîne complète, ni une preuve sur tous les nuages.
