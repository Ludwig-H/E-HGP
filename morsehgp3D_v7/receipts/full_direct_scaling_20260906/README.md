# Trois mesures directes FULL — 6 septembre 2026

Nuages uniformes quantifiés u16, seed 3, n=8 000/16 000/32 000, s=8,
P=`unlimited`, cache lazy de 1 000 000 entrées, CPU 6 mono-thread.
Les trois processus ont rendu le code 0 et un terminal `complete_relative`
après dix ordres horizontaux K=1..10. Une seule observation par taille.

Les neuf fichiers bruts sont copiés à l'identique. `run.json` porte les
commandes, les hashes des flux et le même hash ELF avant/après.
`bench/run_full_probe.py` conserve le runner ; celui-ci n'appliquait ni quota
d'opérations ni timeout. Il enregistre HEAD, pas une preuve hermétique du worktree.
Le build et les sources de cet ELF sont référencés dans `BUILD_REFERENCE.json`
et dans le paquet voisin [full_probe_no_quotas_20260906](../full_probe_no_quotas_20260906/README.md).
L'échec de raccord des auto-tests de ce paquet voisin n'est pas réétiqueté.

`python3 read_results.py` recalcule `results.json` : hashes, commandes, P/s/n,
dix ordres et terminal, puis temps, phases, volumes et ratios d'échelle.
Ce petit lecteur n'est **pas un juge de géométrie ou de complétude**.
Les volumes additionnés sur K ne sont ni une RAM simultanée ni une archive
conservée. Le pic GNU RSS est distingué des échantillons RSS/HWM de la sonde.
Les ratios comparent des tailles de nuages, pas deux algorithmes.

`public_status=not_claimed`. Ni tour inter-K intégrée, ni contrat 50k acquis,
ni mesure GPU. Aucun futur P0/s10/s12 n'est inclus ici. Vérification des octets :
`sha256sum -c SHA256SUMS` depuis ce dossier.
