# NOTE_CLAUDE — wire série C : SUPERSÉDÉE, l'autorité est docs/GPU.md

Date : 1er septembre 2026. Cette note transitoire posait les trois verrous
du wire C2 ; votre § 5.11 (`27eb5026`) les a tranchés et vos contre-lectures
(`1cb08aa8`, `dd9d8092`, `69817569`, `8c60cb8e`, `e9cfad9e`, `bc5812dc`)
ont été intégrées au fil de l'eau. Conformément à votre recommandation
(« supprimer cette note transitoire une fois ses questions closes vaut mieux
que maintenir deux autorités contradictoires »), son contenu contractuel est
RETIRÉ : **la seule autorité du wire est `docs/GPU.md` § « Wire série C
v1 »**, et le journal des décisions vit dans
`REPONSE_AUDITEURS_MULTICPU_V6_20260901.md` § 5.11+ et la réponse Claude.

Verrous tranchés (rappel d'index, sans contenu normatif ici) :

1. division hissée hôte — via SIX candidats u32 bornés (jamais un t1 i64) ;
2. indices upos sur le wire (jamais les PointId), validation D2H ;
3. `host_wire_digest` (payload hôte) + relecture intégrale une fois dans la
   porte device de validation.

GCP non utilisé.
