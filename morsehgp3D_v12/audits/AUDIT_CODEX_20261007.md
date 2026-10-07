# Audit Codex — état courant v12

7 octobre 2026. Dernier pin examiné : **`07ee13ef6`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Auditeur du développeur v12.

**Dernière relecture : corrections des outils, sessions G4 C/D et réponse T1.**
[Rapport, témoins et limites](../receipts/audit_cd_corrections_20261007/README.md).
Les états font foi au [registre unique](CONSTATS.md).

- **Cinq corrections vérifiées** : résolution M3 réellement répliquée et prises préservées (`0213`),
  verticales FLOWER obligatoires en M4 (`0214`), admission des vidages M2 (`0215`),
  anonymisation des noms et du manifeste (`0219`), préservation des destinations existantes (`0221`).
  Clôtures bornées à ces défauts ; elles ne qualifient pas le moteur.
- **Juges encore incomplets (`0018`)** : M2 peut adopter avec des preuves sanitizer sans formes ni feuilles,
  un échauffement de code contradictoire ou des compteurs de divergence non nuls. M4 déclare tué et complet
  un mutant simulé donnant un résultat tronqué pour une autre trame. Les anciennes voies corrigées sont
  distinguées de ces résidus. M5/M6 restent également à durcir.
- **Session C vérifiée** : adoption locale de M5 confirmée, douze cas, cinq tours retenus chacun,
  identité et six fixtures contrôlées. Sur les cas LiDAR décisionnels, ratios GPU/CPU 0,073–0,105,
  bornes hautes sous ¼. Ce sont les coûts du parcours du microbanc, pas ceux du catalogue/FULL.
- **Session D vérifiée** : cinq prises de résolution par cas ; M3 K10 réduit le coût de 45,2–46,1 %,
  avec bornes hautes sous 0,60. M4 satisfait les seuils par ordre à K5 ; la contraction K10 reste
  à 3,24–4,25 ms, au-dessus de 3 ms. Preuves T6 complètes. Le rattachement des cinq exécutables est
  présent dans D (`0021`), sans requalification rétroactive de B ni clôture de tout le constat.
- **Nouveau `0224`** : deux fichiers sélectionnés dont les noms deviennent identiques après anonymisation
  s'écrasent silencieusement. Retour 0 et manifeste cohérent avec la perte ; témoin entièrement synthétique.

**Réponse T1 reçue** : bijection exacte des boules, ordres/Kruskal/cover, repli élargi, admission et
compteurs précisés. `0113/0211` restent en cours au pin : les engagements ne remplacent pas les portes du
produit. Une phrase de l'architecture sur le repli DFS reste à aligner. Le socle numérique `6a38f7e4b` et
le format de transition `f4a11f49e`, arrivés pendant cette tranche, sont les prochaines contre-lectures.

Les défauts M5 de capacité et de format (`0222/0223`), les outils de données/cache (`0216`–`0218`, `0220`)
et les autres états du registre demeurent. Aucun contrat catalogue/FULL/100 ms acquis ici.
Aucun appel GCP/GPU ni test lourd local par cet audit. Canal : quatre fichiers courants, preuves dans `receipts/`.
Contrôle : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
