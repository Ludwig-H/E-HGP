# Certificat FULL C++ : qualification structurelle

Le composant `full_certificate.hpp`, publié dans `f4c0734c`, est qualifié sur les forêts déjà décidées de son corpus. Son autorité reste `structural_only` ; `public_status=not_claimed`.

Le [contrat constructeur](../docs/CONTRAT_CERTIFICAT_FULL.md) décrit arènes, identités, lots et budgets. Les [preuves indépendantes](receipts_full_cpp_20260905/README.md) conservent la lecture du code et les exécutions :

- O2 et ASan/UBSan : 106 représentations, 4 608 coupes, 616 couvertures ; sorties identiques, aucun diagnostic sanitizer.
- 1 338 refus budgétaires et trois mutants privés détectés : côté de coupe inversé, anciens parents laissés actifs, doublons de couverture conservés.
- Deux racines de même couverture ponctuelle restent distinctes ; réindexage non monotone et lot mêlant naissance et fusion sont exercés.
- Les 2+2 CTests constructeur ont leur [contre-vérification séparée](receipts_full_cpp_20260905/constructor_receipt_review.md), avec leurs limites de provenance historiques.

Le lecteur ne découvre ni minima ni parents géométriques. Leur [producteur](PRODUCTEUR_FULL_GABRIEL_COURANT.md), puis le [cache lazy](CACHE_FULL_COURANT.md), ont des qualifications distinctes. Les preuves structurelles ne deviennent ni une certification de catalogues, ni une tour exportée, ni une mesure de coût. Aucun résultat F n’est réattribué à FULL.
