# MEB privée à deux budgets : qualification indépendante

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le certificat local, la charge prospective et le raccord au Builder privé sont qualifiés sur leurs octets capturés.** Ces reçus ne qualifient ni intégration CLI/archive ni producteur FULL, et ne donnent aucun gain de tour. Cette consolidation ne relance aucun moteur. GCP non utilisé.

La preuve du support unique, l'ordinal, le carré non régulier et la réfutation du budget ordinal seul sont exposés dans [PROPOSITION_MEB_ET_BUDGETS.md](../docs/PROPOSITION_MEB_ET_BUDGETS.md). Ses mentions « futur » décrivent sa préparation initiale. Les [résultats constructeur à deux budgets](../docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) et [résultats de coût](../docs/RESULTATS_COUT_MEB_20260905.md) ont leurs autorités propres. Cette note conserve les raccords et preuves indépendantes supplémentaires.

## Domaine et vérification du code capturé

Le [prototype](receipts_meb_dual_20260905/inputs/dual_pivot.hpp) `0645aa00` consomme le [helper historique](receipts_meb_dual_20260905/inputs/legacy_pivot.hpp) `d6dbba19` et F `f75a136a`, sur 2–11 sites distincts u16, indices valides, formes internes authentiques et observateurs passifs. Ces préconditions ne constituent pas une API validant des candidats arbitraires.

`form` trie les positions dans `sites[]` avant les primitives. La positivité et la contenance donnent l'unique MEB ; le shell final égal au support impose le premier accepté de F. Chaque pivot réussi augmente strictement le rayon. Le cap 16 est une borne de travail, pas une garantie de convergence. `q4_form` canonise le signe du déterminant : `det<=0` ne rejette aucune orientation supplémentaire. La matérialisation conserve le support entier, notamment `support[0]`, et le niveau q4 brut depuis le même tuple, sans reconstruction depuis la clé réduite.

La récurrence `choose` reste dans n≤11, q≤4 : valeur≤330, produit avant division≤3630 ; l'ordinal vaut 1..550. Le certificat local suffit donc à conserver le terminal et les champs littéraux, indépendamment de l'ordre de proposition.

## Charges et durée de vie

La [revue budgétaire](receipts_meb_dual_20260905/budget/review.json) distingue c legacy, p proposé et A réellement essayé dans F. Les gardes `p>=P` puis incrément avant forme, et `R>L-c` avant addition, sont sûrs jusqu'à MAX. Un certificat peut être refusé par L ; il laisse alors la boule sentinelle intacte et charge le préfixe virtuel restant. Une proposition échouée conserve p et retrouve F sans nouvel échec scientifique.

Exactement, $\Delta c=A+\sum\min(R,L-c_{\mathrm{avant}})$ sur les branches certifiées, tandis que $\Delta p$ compte les formes proposées. Ainsi $A+\Delta p\leq\Delta c+\Delta p$. Les plafonds restent deux u64 séparés : leur somme mathématique peut dépasser u64. Depuis un état cohérent A≤c, le miroir F conserve cette inégalité même sur exception. Les compteurs auxiliaires doivent garder leur marge ; injecter arbitrairement MAX dans un compteur déjà plein est hors contrat.

Un seul `Work` appartient à la tentative de l'ordre, sans réinitialisation par MEB ou repli. Le port conserve P=0 par défaut, une référence sans proposition et `reference_ordinal_plus_proposal_v1`. Les consommateurs publics doivent qualifier ce schéma séparément. Le [juge arithmétique](receipts_meb_dual_20260905/budget/budget_transition_probe.py) offre un `--check-only` portable ; ses transitions Python ne sont pas des exécutions C++.

## 7. Reçus privés et obligations de qualification actualisées

L'ancre historique conserve son contenu d'attribution. La [contrelecture constructeur normal](receipts_meb_dual_20260905/geometry_constructor/captured_normal.json) et [optimisée](receipts_meb_dual_20260905/geometry_constructor/captured_optimized.json) ferme 9 339 comparaisons, 1 507 ordinaux et le mutant prospectif : 46 437 violations, code 4. Les anciennes limites du reçu triangle ne restent pas des demandes ouvertes globales.

Les frontières rapides q2 à c=MAX−1 et q3 à c=MAX−4/MAX−1 sont appelées à P401. Leur branche rapide est raccordée par non-interférence : `propose` ne lit pas c et les mêmes scènes exigent un certificat dans `named_fast`. Aucun compteur rapide par frontière n'est inventé. La vraie scène q4 de rang 550 est appelée aux caps 549/550/551 ; ses deux pivots essaient seize formes. La combinaison q4, c=MAX−550, L=MAX n'est pas mesurée.

## Qualification locale indépendante

Le [bridge](meb_dual_bridge.cpp), le [juge rationnel](meb_dual_oracle.py) et le [reçu compilé](receipts_meb_dual_20260905/geometry/run.json) conservent deux builds C++20 stricts O2 et O1/UBSan. Par build : 89 nuages, 178 ordres, 3 430 MEB, 1 507 ordinaux ; 416/310/64 succès rapides q2/q3/q4, 1 650 refus legacy et 40 refus de coquille. Les [rejeux normal](receipts_meb_dual_20260905/geometry/normal.json) et [optimisé](receipts_meb_dual_20260905/geometry/optimized.json) jugent les mêmes sorties.

Le Gram rationnel, les coordonnées barycentriques, la contenance, le shell et l'énumération lexicographique Python fixent indépendamment support, R et caps. Pour q4, le déterminant rationnel fournit aussi les trois limbs bruts attendus. Trois copies fautives sont réfutées : shell omis, ordinal +1, numérateur/dénominateur q4 doublés. La dernière conserve le rayon : son rejet contrôle réellement la représentation brute. L'[erreur initiale de classement du juge](receipts_meb_dual_20260905/geometry/initial_judge_rejection.json) reste conservée.

Cette campagne part de Work frais. Son maximum rapide observé est R=465, quatre pivots ; 44 succès q4 utilisent le limb supérieur et 114 succès ont au moins deux pivots. Les [dépendances](receipts_meb_dual_20260905/geometry/dependency_review.json) sont liées aux captures, sans prétention de build hermétique.

## Builder persistant et exceptions

Le [dossier Builder](receipts_meb_builder_20260905/README.md) qualifie l'overlay `6e517c57`/helper `33255ebc`, distinct de F. La [revue sémantique](receipts_meb_builder_20260905/semantic_review.md) vérifie le corps F littéral, les callers inchangés et les miroirs ; l'identité locale fournit l'induction sur le même parcours silencieux.

Les [trois builds nominaux](receipts_meb_builder_20260905/compiled/run.json) passent chacun 3 444 appels locaux et 60 wrappers. Six séquences exécutent quatorze étapes sur le même Builder ; les 23 [attendus dérivés](receipts_meb_builder_20260905/budget_cases.json) ne sont pas tous exécutés. CHAIN5 produit huit MEB de rang un et une coface silencieuse déjà présente avant le troisième appel ; à P2, p=2 et A=6.

Huit injections contrôlent charges et exceptions. `bad_alloc` purge réellement une sortie non vide ; `runtime_error` se propage, sans rendre observable le résultat interne détruit du wrapper. Ses miroirs restent testés à l'appel local. Quatre copies fautives ciblent reset Work, miroirs p/A et charge après forme ; les [jugements normal](receipts_meb_builder_20260905/compiled/normal.json) et [optimisé](receipts_meb_builder_20260905/compiled/optimized.json) exigent leurs motifs causaux précis.

## Autorité native et coût

La [qualification native v2](receipts_meb_native_20260905/README.md) ferme `NoObserver` : 9 351 états confrontés complètement à F/Trace avant et après mesure, captures consommées sous chrono. La [revue indépendante du coût](receipts_meb_native_20260905/cost_review.md) distingue 10 722 candidats physiques contre 67 884 pour les 1 152 appels principaux P401, ralentissement du n=2 répété et contrôle P0 sensible à AB/BA. Ces observations ne fixent aucun seuil produit. Les reçus ultérieurs de coût gardent leur qualification propre ; aucune nouvelle campagne n'est demandée ici.
