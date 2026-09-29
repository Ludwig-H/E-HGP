# Errata des reçus v10

`public_status=not_claimed`. Les reçus sont immuables. Une erreur relevée après coup est corrigée ici, datée et
sourcée, jamais dans le reçu lui-même.

| Reçu | Énoncé | Correction | Source |
| --- | --- | --- | --- |
| `bench_dev_cover_20260929` | « Le point entre donc à sa première couverture α_K(x) […]. Il entre dans la composante de cette boule. » | Vrai de l'attache, faux de ce que lit la tête. En position générale, la première boule couvrante est une naissance d'ordre K. Une tête qui condense avec mcs > K ne lit donc pas α_K(x) : le point sort à la fusion qui absorbe cette naissance. Les scores du reçu ne changent pas. | Audit du 29 septembre 2026 (`audits/audit_hierarchie_knn_20260929/`) ; commentaire corrigé dans `src/tower/tower.hpp` |
