# Complément R2 : arrondi, schémas et limites du juge en flux

30 septembre 2026. Trois captures distinctes, sans moteur modifié,
compilation de moteur ni GCP. Elles ne requalifient pas une intégration
commune. Le [rapport courant](../../../audits/audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md)
sépare les correctifs vérifiés et les gardes restantes.

- `sitetree/` : porte native existante rejouée code 0, 0,989 s, quatre
  modes `cfenv`, 5 969 requêtes par mode. Filtre nearest seulement ; les
  trois modes dirigés ont un repli exact. Neuf empreintes stables.
  Le [reçu](sitetree/receipt.json) et [l'analyse](sitetree/analysis.json)
  détaillent représentation, emprunt et limites FTZ/DAZ/FULL.
- [bancs/](bancs/README.md) : 44 appels courts normal/−O sur quatre unités
  de scores fabriqués. ARI hors domaine maintenant refusé ; alpha/config
  et en-têtes ambigus encore acceptés. Aucune comparaison statistique
  réelle HGP/HDBSCAN ni nouvelle expérience de signal.
- [stream/](stream/README.md) : deux lectures normal/−O de cinq petits
  dumps. Un ordre entier manquant et des points d'attache étrangers passent
  le lecteur structurel, mais pas le juge exact ; contrôle incomplet
  correctement refusé. Aucun dump LiDAR réel fautif observé.

Les trois `SHA256SUMS` originaux restent octet pour octet. Le manifeste
du présent dossier ferme leur agrégation sans réécrire les chemins
historiques des commandes. Les scripts de collecte écrivains ne doivent
être rejoués que dans une **nouvelle copie temporaire**, jamais sur cette
archive. Les scripts en lecture seule restent identifiés dans leurs README.
`public_status=not_claimed` ; aucun contrat FULL/G4 ou 100 ms promu.
