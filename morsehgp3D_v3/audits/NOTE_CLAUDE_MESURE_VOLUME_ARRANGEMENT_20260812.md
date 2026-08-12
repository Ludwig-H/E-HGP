# Note de Claude — volume de l'arrangement — résolue

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Les trois questions de cette note sont closes dans
[`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md).
Le présent fichier conserve seulement la provenance de l'échange; il n'est pas
une autorité indépendante.

Verdicts consolidés :

1. Les mesures à coupe fixe sont utiles, mais une rampe finie sur une famille
   n'établit ni `Theta(n)`, ni cinquante millions d'états obligatoires à 50 k.
   Les sommets de transit du plein arrangement ne sont pas une sortie normative
   que toute architecture exacte doit énumérer.
2. `K_max=10` et dix forêts restent contractuels. Le réduire définirait un
   autre profil; cela ne peut pas qualifier `BenchmarkOutputContract-v1`.
3. La requête exacte du prochain croisement orienté est une expérience admise,
   avec ledger et largeur arithmétique prouvée pour sa formulation précise. Elle
   ne possède encore aucune borne logarithmique. Le successeur concurrent suit
   désormais les événements entrants et sortants et doit être repincé comme tel.
4. q2/q3 sont récoltables par projection depuis les shells sous le théorème de
   propriétaire en dimension affine trois, mais la projection ne conserve pas
   le niveau et exige un census/owner propres.
5. Le théorème de connectivité du vrai 1-squelette s'applique à une transition
   réellement consécutive et à des flats fermés. Les accords différentiels du
   successeur corroborent son implémentation; ils ne remplacent pas la preuve de
   conformité de ses transitions au théorème.
6. `reverse_live_high_water` borne les memberships du chemin, pas les octets ni
   la mémoire totale de l'index, du scratch et des sorties.

Les campagnes larges mentionnées dans la version initiale n'étaient pas
accompagnées ici d'un reçu brut pincé. Elles restent des diagnostics déclarés,
pas des portes durables ni des mesures du SLO.

GCP non utilisé.
