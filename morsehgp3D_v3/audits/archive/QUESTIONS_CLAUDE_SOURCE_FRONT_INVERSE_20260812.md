# Questions de Claude — source et front inverse — résolues

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette file de questions est close. La formulation corrigée, les preuves, les
contre-fixtures et les conseils d'implémentation sont dans
[`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md).
Ce fichier n'est pas une autorité indépendante.

Verdicts consolidés :

1. `|I_B|+|S|<=11` catalogue des supports minimaux pertinents; il ne signifie
   ni coface unique ni rang fermé `|I_B|+|U_B|<=11`.
2. L'arrangement des bissecteurs décrit les incidences de shell. Le support est
   obtenu seulement à la projection auto-centrée, et tous les supports d'une
   même `BallKey` restent attachés au record.
3. Le graphe des seules sorties auto-centrées et ses transitions proposées ne
   sont pas connexes. Un vrai parcours shallow doit conserver les états de
   transit et chercher les intersections consécutives du pinceau.
4. Ni la requête de boule ni le pivot LBVH ne possède de borne
   sortie-sensible; leur pire cas reste linéaire par requête.
5. Le saturé fermé induit un bloc de Johnson connexe, mais une facette
   canonique par coface ne préserve pas ses multifusions ni ses interfaces
   futures. Il faut un token saturé et un join complet.
6. `k=1` se réduit exactement à un EMST. La route industrielle est Yao-1
   sparse, pas l'énumération du Gabriel profond.

Les mesures d'extra-shell à `n=1 500` restent une campagne diagnostique d'une
graine et d'une fenêtre de supports non certifiée. Elles ne prouvent aucune loi
du régime cible ni l'impossibilité d'un quotient exact de plateau.

GCP non utilisé.
