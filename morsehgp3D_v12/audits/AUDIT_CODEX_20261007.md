# Audit Codex — état courant v12

7 octobre 2026. Dernier pin examiné : **`c3de9d73d`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Auditeur du développeur v12.
Les sondes de cette tranche sont compilées explicitement au profil 32.

**Dernière relecture : socle numérique u32, format T1 et publication des reçus.**
[Rapport et preuves](../receipts/audit_u32_20261007/README.md) ; états au [registre unique](CONSTATS.md).

- **Socle CPU conforme aux témoins exacts** : 224 présentations, 220 sphères valides et 880 puissances ;
  2 439 contrôles de distances, Morton/Cloud, repères, garde et census. Oracles rationnels indépendants,
  captures normal/`-O` identiques. Le témoin q3 de `0201` prend correctement le repli large.
  Les preuves des primitives avancent `0108`–`0111`, `0114`, `0204/0208` ; le raccord catalogue/GPU
  et les mutants demandés restent à qualifier.
- **Deux clôtures précises** : identité Morton/Cloud u32 (`0202`, clé complète, doublons et PointId
  préservés) ; ancienne déclaration erronée du port de `reference/tests.cmake` (`0115`).
  Le refus des doublons par défaut selon D8 reste une qualification séparée du point d’entrée.
- **Nouveau défaut majeur `0225`** : le lecteur MHGP12DP accepte un fichier de 88 octets annonçant
  2^62−1 éléments absents, par débordement d’addition. Aucune donnée hors fichier n’a été lue par la sonde.
  Contrôler tailles et offsets avant de former la vue, puis graver le refus avant réutilisation pour T1.
- **Publication `0224` corrigée partiellement** : les noms devenant identiques sont refusés ; une collision
  fichier/dossier après anonymisation provoque encore une exception et laisse un reçu sans manifeste.
- **Provenance courante à actualiser (`0226`)** : 30 classes de ports sont dépassées par la tranche numérique,
  et `PROVENANCE.md` annonce toujours u32 refusé. La correction historique de `0115` reste acquise.

**T1 reste un contrat** : le lecteur de transition n’est pas livré au pin. Le format retenu n’exporte pas
les PointId annoncés ; préciser la portée ou l’extension (`0113`). Le repli de l’architecture est maintenant
aligné avec le contrat J3 élargi. Les centres génériques peuvent dépasser i64 ; le calcul fonctionne en
i128/Big, mais l’énoncé « 64 bits » doit être borné aux boules certifiées.

Les [sessions C/D déjà relues](../receipts/audit_cd_corrections_20261007/README.md) restent des preuves
de microbancs. L’addition feuille + parcours, environ 16 ms, projette un budget ; elle ne mesure pas
le catalogue intégré. Les résidus des juges (`0018`), l’échec intermittent du différentiel (`0024`),
les outils de données et les autres états du registre demeurent ouverts.

**Suite** : contre-lire le contrat T2 arrivé en `5f797660d`, puis les corrections et leurs portes causales.
Ce commit postérieur n’est pas qualifié ici. Tests locaux bornés seulement ; aucun GCP, test lourd ou
nouveau contrat D6/catalogue/FULL/100 ms acquis. Quatre fichiers courants ; détails sous `receipts/`.
Contrôle : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
