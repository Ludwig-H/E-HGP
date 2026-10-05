# L2b — suivi du correctif FNV, lecture favorable

Base développeur **cc73784f0a55a18afe7c6957a2f68cd702e8cb83**. Le commit garde l’ancien hachage;
la correction est une capture **WIP distincte**, à trois fichiers, empreintes stables avant/après.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**Correction favorable en lecture.** `compute_supports` choisit désormais
`wanted = diagnostics == nullptr ? nullptr : &seen`, puis transmet `wanted` à `supports_parts`.
Le chemin public sans diagnostic transmet donc `registers=nullptr` à `build_order_full`;
`registers_of(log)` n’est pas appelé. Le diagnostic demandé conserve son calcul. Le parcours inutile
(2C+G mots, 16C+8G itérations FNV) est retiré dans cette capture; aucun gain de temps n’a été mesuré.

**Portes ajoutées seulement.** La sonde ajoute un appel public `api::compute` à W1 sans diagnostic :
succès, mêmes fichier/manifeste que FULL, pic `tree` égal à FULL et différent de l’ordre seul sur les cas K5
échelle/trames. Le manifeste ajoute `voie_supports_order_tree` (plancher23), jugé par la porte scale8000.
Aucun résultat n’est déduit de leur ajout; aucun test natif, build ou GCP exécuté par cet audit.

Les dix fichiers natifs de cc73784f0 étaient identiques à la capture de 18:03:24, sans nouveau défaut
important établi du journal, de la propriété ou de la concurrence. Ce correctif ne modifie pas ces chemins.
Cela transfère la portée de lecture seulement. Le brouillon reste à intégrer et sa source finale à requalifier.

`source_manifest.json` distingue le pin et les trois SHA WIP; `sources/` et `diffs/` fixent les octets relus.
`review.json` donne les sites causaux. `closure.json` fixe l’état observé au terme de la capture.
