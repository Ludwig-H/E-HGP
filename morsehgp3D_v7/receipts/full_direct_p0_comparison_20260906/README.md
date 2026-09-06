# Comparaison directe P0/P∞, n=8 000 et s=8

Même ELF `4938b94b…`, mêmes coordonnées/digest d'entrée, cache lazy C=1 000 000,
K=1..10. Les dix digests par ordre et le digest final sont identiques. Les
champs non mesurés sont identiques hors deux paramètres P et cinq diagnostics
MEB. Les champs de génération sont inchangés. Les HEAD enregistrés diffèrent,
mais le hash ELF avant/après de chaque run est le même : ne pas attribuer ces
mesures au nouveau binaire q2 `23646…`.

| Mesure | P0 | P∞ |
| --- | ---: | ---: |
| Temps mural enregistré | 154,837 s | 133,047 s |
| Phase FULL | 71,590 s | 50,477 s |
| Génération | 60,253 s | 59,640 s |
| Appels MEB, somme K=1..10 | 4 305 891 | 4 305 891 |
| Compteur ordinal de référence c | 503 231 458 | 503 231 458 |
| Supports physiques F, A | 503 231 458 | 0 |
| Formes proposées payées, p | 0 | 24 777 382 |
| Replis F | 4 305 891 | 0 |

Observation unique par bras : facteur 1,164 au total (−14,07 %) et 1,418 sur
FULL (−29,49 %). FULL contient aussi navigation, requêtes, cache et allocations ;
ce n'est pas un chronométrage isolé du MEB. La génération précède ce MEB local
et n'est pas accélérée par le filtre ; son écart observé reste une variation
entre deux mesures. Aucun intervalle de confiance n'est estimé.

Le ratio 20,310 entre supports F de P0 et formes proposées de P∞ est un ratio
de deux compteurs de tentatives, **pas** un facteur de vitesse ni un rapport
de coûts homogènes. La recherche de paire initiale, les tests de containment
et la matérialisation ne sont pas comptés dans p. Le compteur c devient une
facturation ordinale virtuelle pour les propositions certifiées ; ne pas le
présenter comme du travail F physique dans le bras P∞.

Sources relues : snapshot publié `full_probe_no_quotas_20260906/source_snapshot/`,
`src/forest/meb_proposal.hpp` (`reference_counted`, `miniball`), raccord FULL et
timer `kFull` de la sonde. `public_status=not_claimed` ; aucune certification
nouvelle de complétude, aucune tour inter-K intégrée ni contrat 50k revendiqué.

Recalcul : `python3 compare.py`. Integrite : `sha256sum -c SHA256SUMS`.
Les six bruts sont inchanges ; le paquet triplet precedent reste intact.
