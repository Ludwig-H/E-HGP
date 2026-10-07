# Audit Codex — état courant v12

7 octobre 2026. Pin principal éprouvé : **`1f7642e10`** ; complément T2 : **`76adb8fa9`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Auditeur du développeur v12.
[Rapport et témoins](../receipts/audit_juges_emst_20261007/README.md) ; états au [registre unique](CONSTATS.md).

- **Huit clôtures** : outils de données (`0216/0217`), cache (`0220`), capacité et lecteur M5
  (`0222/0223`), identité de trame et énoncés T2 (`0227/0228/0229`). Anciens défauts reproduits,
  refus corrigés et témoins positifs préservés. Les clôtures T2 ne qualifient pas la tour native.
- **Découpes (`0218`)** : 32 sélections exactes sur sept nuages confirment les carrés horizontaux,
  colonnes entières et IDs ; les **69 anciennes découpes restent à régénérer**, avec leurs paquets
  et manifestes. Le résultat historique n’est pas corrigé par la seule modification du préparateur.
- **M5** : 520 comptes confrontés au vrai scan/Emit ; six totaux injectés vérifient les refus
  avant réservation des tampons et émission. Le diagnostic peut encore allouer avant cette garde ;
  le budget Session et la voie appareil restent à qualifier. Anciens témoins du lecteur et porte livrée conformes.
- **JUG-EMST (`0013`)** : livraison et preuve d’ordre 1 confirmées ; 24 petits nuages, 64 appels CLI
  par mode, graphe complet indépendant et arithmétique u32. Les campagnes historiques annoncées
  ne sont pas contre-certifiées ici. **Nouveau `0232`** : `--ids` peut faire accepter des PointId
  répétés dans un FULL ; l’arbre géométrique du témoin reste correct.
- **Juges (`0018`)** : anciens témoins M2/M4/M5/M6 corrigés, mais M5 adopte encore certaines
  preuves contradictoires ou incomplètes ; la relecture M6 accepte provenance vide, isolation
  contradictoire ou refus enregistré ignoré. Témoins causaux conservés, sans accusation des prises réelles.

**T2 après correction** : lemme hors catalogue limité à k≥2, saturation distinguée de l’absence
du catalogue, décroissance avant chaque saut, NUM-GARDE avant prédicat mixte, codage des cibles et
trois classes de compteurs précisés. Mémo, attache, verticales, capacités et FULL natifs restent à juger.

**Suite** : fermer les résidus des juges et contrôler la régénération des données ; relire le nouveau
bras G1 (`151d4b6ec`), son admission durcie tout juste livrée (`8049bcc39`) et le pilote MES-E
(`ae5f4482a`), puis les raccords catalogue/tour. Ces livraisons ultérieures ne sont pas qualifiées ici.

Tests synthétiques bornés seulement ; aucun GCP, LiDAR recalculé, sanitizer, matrice native ou nouveau
contrat FULL/100 ms acquis. Quatre fichiers courants ; détails reproductibles sous `receipts/`.
Contrôle : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
