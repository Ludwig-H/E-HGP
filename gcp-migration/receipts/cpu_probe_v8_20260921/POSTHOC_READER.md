# Lecteur des captures CPU v8 closes

`../../read_cpu_probe_v8.py` est un lecteur auxiliaire, hors des 196 sources
producteur. Il ne lance aucun programme métier et ne contacte pas GCP. Il ne
lit comme résultat qu'une session dont la récupération et l'arrêt ciblé de la
génération sont certifiés. Une session interrompue peut fournir un préfixe
achevé ; cela ne transforme jamais son statut producteur `failed` en campagne
scientifique complète.

Le validateur global `run_wspd_q34_lidar.py` n'était pas dans le paquet196 : sa
révision exacte `ad76a103dac4baeb16cf9be1d9a6d9ddea6770692e803d6720fb97312521676c`
est conservée dans `posthoc_sources/`. Ses dépendances Python proviennent du
snapshot transporté, dont tous les hashes sont vérifiés. Cette origine séparée
est rendue explicitement dans chaque lecture, sans modifier le paquet exécuté.

Le lecteur lie les intentions aux commandes finales et aux sorties brutes,
reconstruit la compilation stricte22 unités/2 liens et la gate92 `--selftest`,
puis chaque commande du plan. Il vérifie le SHA des fichiers u16le et recalcule
indépendamment le FNV64 sur le préfixe réellement utilisé. L'alias de basename
`n8000.u16le` exigé par le validateur n'est appliqué qu'après cette liaison ; il
n'est pas présenté comme le chemin exécuté (`scan0_n8000.u16le`).

Tous les compteurs et les digests W1/W48 sont comparés après normalisation de
deux seuls maxima de capacité : `work.peak_edge_buffer_bytes` et
`work.q3.peak_shell_bytes`. Les ratios de croissance sont ceux des points
effectivement achevés, jamais des commandes manquantes ou interrompues. Les
mesures ne sont ni un moteur GPU, ni la tour FULL, ni une preuve de complexité
sous-quadratique universelle.

Option `--local-record` : vérifie la clôture de la capture locale et la commande
achevée sélectionnée, puis compare le même nuage/travail si le résultat G4
apparié existe. La campagne `global_tsd9ofnm` reste `failed` (interruption de la
commande suivante), même si `record_0000.json` est achevé et revalidé. Le ratio
de temps inter-machines W4/W48 n'isole pas à lui seul le gain parallèle.

Qualification auxiliaire avant résultatsR3 : `posthoc_preflight_checks.json`.
Quatre tests purs passent en Python normal et `-O` ; ils couvrent commandes,
SHA/logs, FNV/préfixe et neutralisation exacte des deux pics. La lecture R2
fermée est identique dans les deux modes : gate−15, zéro mesure, neuf commandes
non exécutées ; le statut est `validated_partial`, pas un succès de la gate.
Trois erreurs de développement du lecteur sont conservées dans ce reçu avec
leur diagnostic. Aucun résultatR3 n'a été lu pour cette qualification.

Chaque résultat contient le SHA du lecteur et toutes les empreintes de preuve
avant/après lecture. La correspondance intégrale archive/fichiers extraits est
recontrôlée à la fermeture. Les chemins start/stop/handoff/lifecycle sont liés
au même hôte d'origine, même si son répertoire a ensuite été archivé ailleurs.

Commande après fermeture de la sessionR3 :

```sh
python3 -B gcp-migration/read_cpu_probe_v8.py --host /tmp/ehgp-v8-g4-r3-20260921.k5RPv9mL/cpu_v8_host --local-record morsehgp3D_v8/receipts/lidar_global_20260921/global_tsd9ofnm/record_0000.json
```

Rejouer avec `python3 -O -B` ; conserver les deux sorties et leur égalité. Ne
jamais éditer les captures pour satisfaire ce lecteur.
