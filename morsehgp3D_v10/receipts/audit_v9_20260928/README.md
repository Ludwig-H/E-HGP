# Audit critique de la v9 (28 septembre 2026)

Demandé par l'utilisateur avant l'ouverture de la v10. Base jugée : `ce8a649dd`.

- [Audit consolidé](AUDIT_V9_CRITIQUE_20260928.md) : constats A9-xx, sévérité, statut de vérification, preuves.
- [Base de connaissances pour la v10](CONNAISSANCES_V10_20260928.md) : objet, théorèmes invocables, fixtures, chiffres de
  référence, objectifs hérités, spécification du clustering de la thèse, protocole recommandé.
- `lentilles_et_verifications.json` : sorties brutes des 15 lentilles et des 83 vérifications adverses.
- `preuves/` : scripts et journaux légers des lentilles et des vérificateurs (aucune donnée de nuage ; les
  entrées LiDAR se régénèrent depuis leurs empreintes).

Méthode : 15 lentilles indépendantes en lecture seule, un vérificateur adverse par constat grave (100 agents),
puis deux synthèses. GCP non utilisé.
