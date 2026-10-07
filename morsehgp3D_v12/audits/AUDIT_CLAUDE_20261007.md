# Audit Claude — état des contre-lectures

7 octobre 2026. Note vivante de Claude, auditeur de contre-lecture. Les trois premières lignes reprennent la
synthèse éditoriale de Codex (pin `3e6e6a8e7`, nettoyage demandé par l'utilisateur), sans retouche ; la dernière
est un avis nouveau de Claude. Les états courants font foi au [registre](CONSTATS.md).

| Périmètre et preuve | État enregistré | Suite attendue |
| --- | --- | --- |
| [Lemmes T1, T3–T7](../receipts/audit_canal_20261007/archives/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md), lecture de `13c52bc60`, note `fc1f913ce` | `CST-0101` à `0103` clos par `a0e31abfe` ; `0104` à `0107` en cours | graver les témoins mémo, refus hors domaine, cercle T7 et ordre canonique dans les portes concernées |
| [Contrat numérique](../receipts/audit_canal_20261007/archives/AUDIT_CONTRAT_NUMERIQUE_20261007.md), lecture de `e264de6f2`, note `2a7a5f346` | `CST-0112` clos par `8865e32c1` ; `0108` à `0111` et `0113` en cours | types de boules certifiées, census local, distances u32, témoins de paliers et lecteur à translation près |
| [Budgets mixtes et test du milieu](../receipts/audit_canal_20261007/archives/ADDENDUM_CONTRAT_NUMERIQUE_20261007.md), lecture de `8865e32c1`, note `f6f65a0d8` | `CST-0114` en cours ; contrat corrigé par `d3fc3ff09` | porter et tester la forme locale du milieu sur l'hôte et l'appareil |
| [Socle T0 et microbanc MES-M2](../receipts/audit_claude_socle_t0_20261007/README.md), lecture de `a0091e2b7` | lemme des faces juste (cache J2 complet pour $m\leq 32$, arrêt du recensement, conflit du lemme R : vérifiés dans la v11) ; table des ports conforme sauf `reference/tests.cmake` (`CST-0115` ouvert) | corriger cette ligne de `PORTS.md` et le bilan des portes ; relire chaque dépôt de tranche |

La [réponse du développeur](../receipts/audit_canal_20261007/archives/REPONSE_CLAUDE_LEMMES_T_ET_OUVERTURE_20261007.md)
(`a0e31abfe`) accepte les constats des lemmes et consigne les suites. Ses formulations sont historiques ; le registre
porte les états courants et les documents de contrat leurs corrections ultérieures.

Preuve exécutée rapportée dans l'addendum : `reference/test_witness_t1.py` sous `python3 -S -O`, code 0 en référence,
4 pour le mutant `sans_inclusion`, 2 pour une option inconnue. Pour le socle T0 : contrôle mécanique de
`docs/PORTS.md` (`verifier_ports.py`, sortie et empreintes dans le reçu) : 209 empreintes conformes, 28 copies et
111 renommages seuls exacts sur 112. Les autres conclusions sont des contre-lectures
mathématiques et de code, avec les limites explicites des notes archivées ; elles ne qualifient pas un moteur u32.
PO-T2, T8 et I1 n'ont pas été contre-lus dans ces notes. Les questions de décroissance et de contrôle croisé de
l'index demeurent à traiter dans leur tranche.

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. GCP non utilisé pour ces contre-lectures.
