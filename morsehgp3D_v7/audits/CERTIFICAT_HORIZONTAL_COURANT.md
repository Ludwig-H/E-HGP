# Certificat horizontal réduit E : portée conservée

**La route CPU E terminée conserve les composantes non triviales de Gamma pour K≥2, toutes les racines K1 et leur évolution ponctuelle.** Ce résultat porte sur `normalized_horizontal_h0_candidate`, distinct de FULL. `public_status=not_claimed`.

La [preuve principale de composition](../docs/PREUVE_HORIZONTALE_COMPOSITION.md) et la [qualification des primitives](../docs/QUALIFICATION_S1_PRIMITIVES.md) portent désormais l’exposé. Cette entrée conserve le raccord indépendant entre leur domaine et le payload réellement exécuté.

## Domaine et passage au fold

L’autorité est la [route u16 CPU qualifiée](DOMAINE_CPU_COURANT.md) : points distincts, `2≤n≤2^30−1`, s≥8, `2≤smax≤11`, `rmax=min(smax,n)`, sans `prefilter_census_override` ni mutant, sous environnement numérique stable. Elle exige census achevé, catalogues directs complets, refus des coquilles pertinentes et descentes locales certifiées. Elle ne suppose pas que toute boule du nuage soit régulière. Les blocs saturés hors fenêtre relèvent du théorème d’inertie, pas d’une tolérance flottante. Un refus terminal n’a aucune sortie certifiée.

Toute directe Q vérifie `p(B_Q)+q(B_Q)≤|Q|≤rmax` et figure dans le catalogue. Retirer un essentiel produit une facette stricte ; retirer un intérieur garde la boule régulière. Un bloc omis de haut rang possède un graphe strict connexe avec plusieurs facettes et une incidence antérieure pour chacune. Un contact égal irrégulier avec une facette régulière contredirait l’unicité de sa MEB ; les contacts stricts passent par les apex déjà identifiés. Chaque maillon retenu a donc son suffixe strict vers un direct actif, et chaque composante réduite de Gamma possède un ancrage direct.

L’inclusion des facettes retenues donne ainsi une bijection des composantes et conserve leurs points. La même inclusion à toutes les coupes donne la naturalité horizontale. Ce sont les incidences qui identifient les composantes, jamais leur seule couverture.

Le fold fige les parents avant les unions du lot. Une facette déjà incidente possède une racine ; une active encore latente n’ajoute pas de parent. `born` marque sa première matérialisation dans le sous-flot. Les changements ponctuels et les continuations du réduit restent donc différents des seuls minima et multifusions FULL. Les niveaux rationnels égaux sont regroupés, indépendamment des représentations ou des numéros de batches locaux.

## Preuves exécutées

Les [reçus indépendants](receipts_horizontal_20260905/README.md) conservent, par build O2/UBSan, 16 appels du vrai pipeline, 60 ordres, **840 coupes et 1 124 carrés de naturalité**. Les 820 facettes Gamma omises cumulées sur les coupes garantissent que le juge ne suppose pas un catalogue exhaustif. Le mutant retirant une coface est refusé malgré un succès moteur ; le premier essai d’audit invalide reste conservé séparément.

Le fold synthétique passe 272 coupes et sept mutants ; sa réalisabilité euclidienne n’est pas supposée. Les quatre nuages du pipeline sont globalement réguliers : l’inertie des blocs irréguliers hors fenêtre garde sa preuve statique et sa fixture de frontière propres. Les [qualifications D/E/F](AUDIT_QUALIFICATION_20260905.md) conservent leurs attributions distinctes.

Les [ancres verticales](CONTRAT_VERTICAL_COURANT.md) et [poids](CONTRAT_MASSES_VOTE_COURANT.md) sont des suppléments. La preuve du réduit ne livre ni leurs exports, ni FULL, ni un coût industriel. Les anciennes demandes horizontales closes ne sont pas rouvertes.
