# Audit Codex — état courant v12

7 octobre 2026. Auditeur du développeur v12. Dernier pin examiné : **`4147c5460`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue produit`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Décisions D1–D15 enregistrées.

**Dernière relecture : session G4 A, données et contrat du catalogue T1.**
[Rapport et témoins reproductibles](../receipts/audit_session_t1_20261007/README.md).
Les états font foi au [registre unique](CONSTATS.md).

- **M2 confirmé sur cette campagne** : J3 r168, ratio moyen géométrique 0,176389 ; pire borne haute
  IC95 0,202689 < 1/3. Les 45 processus, 315 identités GPU et 18 contrôles hôte sont complets.
  Recalcul indépendant, bootstrap exhaustif concordant, raccord aux petites archives brutes.
  C'est le noyau de feuille ; ni catalogue entier, ni FULL, ni contrat 100 ms.
- **Les juges restent à durcir (`0018`)** : cette campagne ne présente pas les omissions précédemment
  reproduites. Le pilote M6 ajoute toutefois un témoin de succès avec neuf sorties vides.
  Les neuf prises M6 réelles sont complètes ; environ 8 µs par lancement chaud spin/yield.
  Leurs médianes entre processus ne démontrent pas un avantage yield au premier lancement.
- **Six nouveaux constats (`0216`–`0221`)** : chemin de rejeu déplacé incorrect, manifeste vide/hash nul
  acceptés, découpes qui tronquent une colonne verticale, identité conservée dans les noms publiés,
  espace récupérable surestimé avec des liens durs, destination préexistante effacée sur refus de publication.
  Témoins minuscules ; aucune fuite ni corruption historique imputée à ces reproductions.

**Réponse T1** : accord pour essayer J3 commun CPU/GPU, sous MES-P sur la chaîne des petits nuages et
requalification numérique ; garder les deux oracles indépendants. Accord pour le lecteur de transition,
avec une correction de portée (`0113`) : l'ordre des boules à niveau égal peut changer Kruskal et `cover`
même avec supports uniques. Un second modèle montre que normaliser Morton peut changer le réservoir G1
et les feuilles ; MES-M5 doit fixer l'ordre parent comparé. Aucun écart du futur moteur v12 n'est affirmé.

Les clôtures `0209`, `0210`, `0212` et les portes de port encore ouvertes restent celles de la
[relecture du socle](../receipts/audit_socle_microbancs_20261007/README.md).
**Suite utile** : corriger les six outils et les juges avant réutilisation, graver le lecteur T1, qualifier
le repère local et le budget physique. La session B (`f1ea04c40`) et son addendum (`abe9df451`), déposés
pendant cette tranche, restent à contre-lire ; aucun résultat B n'est validé ici.

Aucun appel GCP/GPU ni campagne lourde locale par cet audit. Canal limité à quatre fichiers courants ;
preuves dans `receipts/`. Contrôle : `python morsehgp3D_v12/tools/check_constats.py`.
