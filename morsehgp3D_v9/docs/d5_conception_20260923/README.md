# Conception D5, étape 1 (23 septembre 2026)

Trois rapports produits par un workflow de conception à trois agents
(lecture seule du code, rien compilé ni modifié) :

- [carte de la tour FULL](CARTE_TOUR.md) : phases, résolution des
  facettes, grand livre, point d'insertion et plafond du gain ;
- [analyse du sidecar D5 de l'auditeur C](ANALYSE_SIDECAR_C.md) : les trois
  étages, la règle 0, les défauts connus et le port minimal ;
- [plan de l'étape 1](PLAN_ETAPE1_SAUT.md) : saut au centre dans la phase 0,
  ombre par facette, grand livre, fixtures, mutants, critères d'arrêt.

**Décision.** Le chemin critique chevauché de la tour est la phase A
mono-fil de l'ordre le plus haut. Même une phase 0 gratuite ne gagnerait
que 104 ms à K5 et 539 ms à K10 sur R13. La phase A a donc été allégée
d'abord ([reçu](../../receipts/tower_phaseA_lean_local_20260923/README.md)).
Le saut reste l'étape suivante pour K10. Ce plan n'est ni un reçu ni un
statut public.
