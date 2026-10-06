# Juge GPU : registre absent accepté — source du 6 octobre 2026

Constat sur `59509bbc816646f7bda0c77a3b948f57c79a8b9c`, fichier `morsehgp3D_v11/bench/gpu_sanitizer.py` (SHA256 `2b234283e02e511cec595d9ad864a57e5ed9e1c9a61b7360fac820bff9d0d369`). Le registre est obtenu par `domain.get('catalogue_work')` ligne 93 ; la garde ligne 96 ne refuse pas son absence ; l'égalité ligne 102 accepte alors `None == None`. Le juge peut annoncer la conformité du registre sans en avoir reçu un.

`replay.py` exécute le véritable `main()` Python capturé avec tous les appels `subprocess.run` simulés. Le contre-exemple retire le registre des douze réponses ordinaires et conserve codes zéro, statut réussi, dumps identiques et compteurs nécessaires à l'atteinte des branches. Les cinq réponses instrumentées ont leurs marqueurs propres et leurs dumps identiques. La source initiale rend code 0 et `gpu_sanitizer_verdict conforme`, avec douze registres absents. Le même mécanisme vérifie le raccord instrumenté : ses réponses avec registre absent/différent ou `exit.status=refused` restent acceptées par la garde lignes 121–123.

`fix_proposal.patch` propose uniquement de refuser `work is None` dans les réponses ordinaires et d'exiger, dans les réponses instrumentées, `exit.status == ok` ainsi qu'un registre égal à celui du CPU déjà admis. La proposition garde le cas valide conforme et refuse les quatre variantes de registre/statut. Les contrôles négatifs indépendants (code non nul ordinaire/instrumenté, dump différent, résumé outil absent) sont refusés avant et après cette proposition. Le patch n'est pas appliqué au travail du développeur.

La portée réellement inscrite dans `SANITIZED` ligne 29 compte cinq couples : memcheck A/B, racecheck A/B, synccheck A. Cette preuve ne qualifie aucun exécutable, outil, profil numérique ou matériel ; les dumps sont des octets de protocole factices. Aucun calcul natif, appel GCP ni lecture de LiDAR n'a lieu. Elle ne prend aucune session mutable pour preuve.

```sh
python3 -B replay.py
python3 -B -O replay.py
sha256sum -c SHA256SUMS
```

Les deux rejeux stdlib donnent le même JSON et comparent leurs résultats à `summary.json`. Le fichier source et le patch sont inclus ; aucune dépendance à un build ou à une archive G4 n'est requise.
