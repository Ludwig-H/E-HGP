# Preuve épinglée — connectivité shallow de l'arrangement

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Théorème conditionnel

Pour tout arrangement fini d'hyperplans affines non verticaux possédant des
sommets, le vrai 1-squelette induit par les sommets de niveau au plus `k`
est connexe. Dans un arrangement simple de dimension quatre, ses arêtes
finies relient exactement les sommets consécutifs partageant trois
hyperplans. Un germe exact de niveau zéro suffit donc à parcourir les
sommets shallow sans excursion au-dessus de `k`.

La preuve emploie la convexité de chaque chambre de signes et la monotonie
du niveau le long de son 1-squelette. Elle concerne le vrai arrangement,
pas automatiquement une représentation par quatre identifiants.

## Préconditions non transférables

- coquille, intérieurs, germe et voisins exacts;
- gestion des sommets multiples et des hyperplans constants sur un pinceau;
- voies propres aux dimensions affines basses et aux arités un à trois;
- aucune conclusion de coût : un parcours peut rester en `Theta(n*V)`.

Une campagne rationnelle historique de 10 800 arrangements génériques à
cinq à huit points n'avait trouvé aucun contre-exemple. Elle corrobore le
théorème sans le remplacer et ne qualifie aucun produit.

GCP non utilisé.
