# Prétest q2 E : qualification locale conservée

Le [contrat constructeur](../docs/OPTIMISATION_MEB_Q2.md) décrit le prétest. Le header E `f75a136a`, distinct de D, conserve support, niveau, refus et facturation ; `public_status=not_claimed`.

L’identité indépendante est `P₂(z)=(z−a)·(z−b)`. Sur u16, les trois produits et sommes partielles sont bornés par `3·65535²=12884508675<2³⁴`, dans les i64 réellement utilisés. Rejeter seulement une puissance strictement positive conserve les zéros et le contrôle final de coquille. Le candidat retenu appelle les mêmes constructeurs exacts dans le même ordre ; le compteur est chargé avant le prétest.

Le [rejeu E](receipts_20260905/e_q2/q2_addendum.json) retrouve les sorties D sur 89 ensembles, 431 appels, 164 refus cap, six refus shell et 6 176 puissances q3/q4. UBSan n’émet aucun diagnostic. Le mutant réel qui rejette le shell q2 est détecté ; ses codes sont distingués du CTest dédié. Sources, patch et bruts restent sous `receipts_20260905/e_q2/`.

La [qualification intégrée E](AUDIT_QUALIFICATION_20260905.md) a ses propres reçus. Les [mesures principales](../docs/RESULTATS_MONO_Q2_20260905.md) ne démontrent ni gain statistique à partir d’une paire, ni SLO. Aucun résultat D n’est transféré par simple renommage.
