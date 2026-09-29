# Errata des reçus v10

`public_status=not_claimed`. Les reçus sont immuables. Une erreur relevée après coup est corrigée ici, datée et
sourcée, jamais dans le reçu lui-même.

| Reçu | Énoncé | Correction | Source |
| --- | --- | --- | --- |
| `bench_dev_cover_20260929` | « Le point entre donc à sa première couverture α_K(x) […]. Il entre dans la composante de cette boule. » | Vrai de l'attache, faux de ce que lit la tête. En position générale, la première boule couvrante est une naissance d'ordre K. Une tête qui condense avec mcs > K ne lit donc pas α_K(x) : le point sort à la fusion qui absorbe cette naissance. Les scores du reçu ne changent pas. | Audit du 29 septembre 2026 (`audits/audit_hierarchie_knn_20260929/`) ; commentaire corrigé dans `src/tower/tower.hpp` |
| `catalogue_filter_j2_20260929` | `RECU_AGENT_J2.md`, § 8 « Reproduire » : `differentiel_j2.sh` prend par défaut `build/v10-wt/mhgp10_catalogue` comme référence `568d45297`. | Ce binaire a été reconstruit au HEAD le 29 septembre à 13 h 11 et contient J2. La référence doit être passée explicitement : `build/v10-j2/build-base/mhgp10_catalogue` (`433b4b97…`). Les différentiels du reçu restent valides : ils ont été joués avant la reconstruction, puis rejoués contre `build-base`. | Agent J2, `RECU_J2_ASSEMBLAGE.md` § 6 |
