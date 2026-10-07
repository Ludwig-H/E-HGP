# Audit Claude — état des contre-lectures

7 octobre 2026. Synthèse éditoriale établie par Codex pour le nettoyage demandé par l'utilisateur ; les avis
résumés restent attribués à Claude, auditeur de contre-lecture. Aucun nouvel avis n'est émis au nom de Claude.
État repris du [registre](CONSTATS.md) au pin `3e6e6a8e7` ; aucune clôture ajoutée par cette synthèse.

| Périmètre et preuve | État enregistré | Suite attendue |
| --- | --- | --- |
| [Lemmes T1, T3–T7](../receipts/audit_canal_20261007/archives/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md), lecture de `13c52bc60`, note `fc1f913ce` | `CST-0101` à `0103` clos par `a0e31abfe` ; `0104` à `0107` en cours | graver les témoins mémo, refus hors domaine, cercle T7 et ordre canonique dans les portes concernées |
| [Contrat numérique](../receipts/audit_canal_20261007/archives/AUDIT_CONTRAT_NUMERIQUE_20261007.md), lecture de `e264de6f2`, note `2a7a5f346` | `CST-0112` clos par `8865e32c1` ; `0108` à `0111` et `0113` en cours | types de boules certifiées, census local, distances u32, témoins de paliers et lecteur à translation près |
| [Budgets mixtes et test du milieu](../receipts/audit_canal_20261007/archives/ADDENDUM_CONTRAT_NUMERIQUE_20261007.md), lecture de `8865e32c1`, note `f6f65a0d8` | `CST-0114` en cours ; contrat corrigé par `d3fc3ff09` | porter et tester la forme locale du milieu sur l'hôte et l'appareil |

La [réponse du développeur](../receipts/audit_canal_20261007/archives/REPONSE_CLAUDE_LEMMES_T_ET_OUVERTURE_20261007.md)
(`a0e31abfe`) accepte les constats des lemmes et consigne les suites. Ses formulations sont historiques ; le registre
porte les états courants et les documents de contrat leurs corrections ultérieures.

Preuve exécutée rapportée dans l'addendum : `reference/test_witness_t1.py` sous `python3 -S -O`, code 0 en référence,
4 pour le mutant `sans_inclusion`, 2 pour une option inconnue. Les autres conclusions sont des contre-lectures
mathématiques et de code, avec les limites explicites des notes archivées ; elles ne qualifient pas un moteur u32.
PO-T2, T8 et I1 n'ont pas été contre-lus dans ces notes. Les questions de décroissance et de contrôle croisé de
l'index demeurent à traiter dans leur tranche.

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. GCP non utilisé pour ces contre-lectures.
