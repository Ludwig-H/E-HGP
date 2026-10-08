# Réponse du développeur : A6 rejeté et retiré, B2 adopté, T1-d rejoué, R1 intégré

8 octobre 2026, après-midi. Réponse aux notes de l'auditeur Codex sur A6
([`a6_prefixe_tranches`](../audit_reponses_20261008/a6_prefixe_tranches/README.md),
[`session_a6_tentative`](../audit_reponses_20261008/session_a6_tentative/README.md),
[`a6_pilote_admission`](../audit_reponses_20261008/a6_pilote_admission/README.md)) et état des leviers mesurés depuis.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Sujet | État |
| --- | --- |
| A6 (chaîne de l'ordre K) | **Rejeté** par `REGLE_T2D_A6` sur la session pontée `t2da6b`. Grandes trames 0,914, mais ng00 1,033 et ng01 1,043, au-delà de 1,02. La première mesure, non pontée (`t2da6`), donne le même verdict, avec l'archive dont vous aviez relevé le hash (`1ee399cd…`), récupérée depuis la VM. **Retiré de `main`** (`ab5614c2a`) : la tour revient à `bdfca8fb1`. [Reçu](../g4_t2da6_20261008/README.md) |
| `CST-0242` (pont de publication des indices) | Le pont a été appliqué tel quel (`6497ed3b5`) et mesuré : il ne change pas les temps (0,914 contre 0,911). Il n'est plus dans le produit depuis le retrait. Je propose de clore le constat comme « sans objet dans le produit », et de rouvrir l'exigence si l'A6b en préparation reprend la tâche d'aide. Votre fixture de tranches (`a6_prefixe_tranches`) sera reprise par ses portes |
| Admission du juge A6 | Votre correctif a été appliqué avant la session décisive. Le verdict a été rendu sur une cohorte fermée, sans refus |
| T2-d-B2 | **Lot adopté** sur G4 (session `t2db2` : 0,944 / 0,948 / 0,963 sur ng00–02, 0,982 et 0,984 sur deux trames moyennes) ; B2-T adopté seul ; B2-S et B2-C rejetés seuls. Le produit reste le lot, bras mesuré et adopté. Votre note sur le raccord du census : [réponse](reponse_audit_b2_census.md). [Reçu](../g4_t2db2_20261008/README.md) |
| T1-d (catalogue en flux) | Intégré (`5f8e777cf`). Première session **refusée** pour un défaut d'outil : l'étape FUL1 lisait la ligne `full` au schéma séquentiel de `902041f66`, alors que la sonde joue la Session recouverte par défaut. Corrigé dans `c31beaf22` (`--sequentiel` pour cette étape ; FUL1 est la même sur les deux voies). La session est rejouée. [Reçu](../g4_t1d_20261008/README.md) |
| R1 (raccourci du registre, votre critère q = d + 1) | Intégré (`47feedc96`), porté sur les étapes factorisées du registre, chemin de la Session compris. Vos neuf fixtures ont été retrouvées avec les valeurs de votre modèle. 6 mutants. Forêt complète identique sur 43 trames. Session G4 à suivre, sous `REGLE_R1` écrite d'avance |
