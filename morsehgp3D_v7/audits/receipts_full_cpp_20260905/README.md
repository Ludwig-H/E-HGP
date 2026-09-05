# Qualification indépendante du composant FULL C++

5 septembre 2026. Entrée maintenue : [certificat FULL C++ courant](../CERTIFICAT_FULL_CPP_COURANT.md). Sources constructeur `f4c0734c53a18d1e2de477ca09584c8f15c938f9`, autorité `structural_only`, statut public `not_claimed`. Aucun fichier produit modifié, aucun GCP utilisé.

## Attribution des preuves

La [lecture sémantique](semantic_review.md) et ses [pins](semantic_review.json) ferment les invariants de la forêt fournie. La [contre-vérification des reçus constructeur](constructor_receipt_review.md) et son [inventaire](constructor_receipt_review.json) relisent les deux CTests par build et leurs limites de provenance. Ces deux revues ne lancent aucun C++.

La qualification indépendante ci-dessous compile le [pont d’audit](../full_cpp_bridge.cpp) avec une [capture littérale](source/morsehgp3D_v7/src/forest/full_certificate.hpp) du composant et ses sept en-têtes locaux dépendants. Les [pins sources](source_pins.json) et les dépendances effectives de chaque binaire distinguent la source nominale et les trois mutations privées. Le pont n’incorpore aucun attendu ni oracle.

Le [juge indépendant](../full_cpp_audit.py) part du [journal FULL scellé](../receipts_gabriel_20260905/full_normal.json) : 50 ordres, 285 événements. Il réadresse les parents et trie les lots selon l’API, puis confronte les arènes et lectures du C++ aux états attendus. Gamma borné, relu sans réexécuter le producteur FullPortal, juge séparément les composantes, leurs points et leur naturalité par inclusion des minima. Les [fixtures JSON](fixtures.json) et leur [transport texte](fixtures.txt) sont déterministes et identiques en préparation normale et optimisée.

## Résultats exécutés

| Par binaire nominal | Corpus Gamma | Corpus structurel | Total |
| --- | ---: | ---: | ---: |
| Représentations de forêts | 100 | 6 | 106 |
| Nœuds | 570 | 46 | 616 |
| Minima | 356 | 26 | 382 |
| Références parentales | 470 | 40 | 510 |
| Coupes | 4 530 | 78 | 4 608 |

Chaque forêt est représentée en fractions réduites puis avec des multiples différents dans les lots et les coupes : les 100 représentations géométriques correspondent aux mêmes 50 ordres initiaux. Le juge contrôle 4 450 carrés horizontaux sur ce seul corpus Gamma. Il n’attribue aucune géométrie aux trois fixtures structurelles de recouvrement, renommage non monotone et lot mixte.

Les 616 couvertures passent aux plafonds exacts de descendants et de références ponctuelles. Diminuer séparément chaque plafond d’un produit 1 232 refus exacts sans valeurs partielles. Diminuer le nombre total de nœuds autorisés produit 106 refus de construction sans arène publiée. Ces nombres comptent les représentations, pas des cas statistiques indépendants.

Les [sorties O2](O2_output.json) et [ASan/UBSan](sanitized_output.json) sont identiques octet pour octet. Les jugements [O2](O2_normal.json) et [sanitizer](sanitized_normal.json) passent avec code 0 ; leurs versions optimisées sont égales hors indicateur `python_optimized`. Les diagnostics compilateur et sanitizer sont vides. Aucun nouveau CTest n’est exécuté par ce pont : les 2+2 CTests du constructeur gardent leur provenance séparée.

| Mutation de copie privée | Sortie du pont | Rejet du juge |
| --- | --- | --- |
| [Côté de coupe inversé](closed_side.patch.txt) | Code 0 | Code 1, `full_cpp.cut_boundary` |
| [Ancien parent conservé actif](retained_parent.patch.txt) | Code 0 | Code 1, `full_cpp.cut_parent_still_active` |
| [Points répétés conservés](point_multiplicity.patch.txt) | Code 0 | Code 1, `full_cpp.coverage_duplicates` |

Les trois sorties fautives sont conservées dans `*_output.json`, avec leurs rejets `*_normal.json` et `*_optimized.json`. Les [cinq appels normaux du juge](judgments_normal.json) et les [cinq appels optimisés](judgments_optimized.json) distinguent deux jugements nominaux et trois rejets par mode.

## Reproduire et vérifier les sources

Les [commandes de compilation et d’exécution](execution.json) sont des arguments structurés, avec options exactes, overrides ASan/UBSan, codes de sortie, empreintes des cinq binaires et neuf dépendances locales par build, pont inclus. Les binaires temporaires ne sont pas versionnés. Recréer leurs dossiers sous `audits/.work_full_cpp_20260905/` avant compilation ; pour les mutants, copier l’arborescence `source/morsehgp3D_v7/` puis appliquer uniquement le patch correspondant. Tous les en-têtes nominaux correspondent aux octets du commit constructeur.

Le rejeu des preuves brutes ne demande aucune compilation :

```bash
python3 -B morsehgp3D_v7/audits/full_cpp_audit.py --check morsehgp3D_v7/audits/receipts_full_cpp_20260905/O2_output.json --result morsehgp3D_v7/audits/receipts_full_cpp_20260905/O2_normal.json
python3 -B -O morsehgp3D_v7/audits/full_cpp_audit.py --check morsehgp3D_v7/audits/receipts_full_cpp_20260905/sanitized_output.json --result morsehgp3D_v7/audits/receipts_full_cpp_20260905/sanitized_optimized.json
```

`--prepare` régénère les deux fixtures depuis le reçu scellé, sans lancer le produit. Le juge vérifie les pins de ses dépendances et compare cette régénération aux fichiers consommés. Les arènes attendues conservent les liens, niveaux et labels ; les couvertures seules ne servent pas d’identités. Dans les cas structurels, deux racines de mêmes points restent distinctes jusqu’à leur fusion explicitement fournie. Le lot mixte exerce une naissance et deux fusions simultanées, absentes ensemble des journaux géométriques scellés.

La [revue du delta publié](source_delta_review.json) isole CMake : les neuf lignes ajoutées enregistrent seulement la nouvelle cible et ses deux portes. Les 142 autres pins F sont inchangés. La variante complète `G_full_structural` contient ces 143 fichiers actualisés et les deux nouvelles sources, sans transfert de la campagne F339. Les trois changements documentaires relus remplacent les états « prévu » par « livré » ; leurs anciens reçus restent intacts.

## Limites

Il s’agit du raccord entre une forêt déjà décidée et sa représentation C++, pas d’un producteur FULL complet. Les en-têtes système et la toolchain ne forment pas une capture hermétique. Les niveaux de cette sonde tiennent dans le transport borné déclaré ; elle ne remplace pas les preuves de grandes largeurs. Les pannes persistantes d’allocation appartiennent aux portes constructeur contre-vérifiées, sans nouvelle injection de ce type dans le pont.

Portails géométriques, complétude du catalogue, manifeste terminal, verticale, masses, sérialisation, streaming, coût RSS et performance restent à intégrer et qualifier. Un horizon absent ne peut être deviné depuis les seuls nœuds. Aucun résultat GPU, palier massif, SLO ou statut public exact n’est acquis.
