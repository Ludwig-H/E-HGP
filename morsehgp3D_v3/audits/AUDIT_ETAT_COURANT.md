# Audit courant de MorseHGP3D v3

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot

`HEAD=2b89ea127d979a60981e6741470f8d8bb49c63d6`, commit
`mark stage 0A closed in the roadmap, with their corrected ordering`.

Au relevé de livraison, Claude modifiait concurremment :

```text
M morsehgp3D_v3/prototype/rect_front.hpp
M morsehgp3D_v3/prototype/wspd_wavefront_probe.cpp
```

Ces deltas logiciels ne sont ni inspectés comme pin stable, ni attribués à
l'auditeur. Les modifications d'audit restent limitées à `README.md`,
`PROPOSITION.md` et `audits/`. GCP non utilisé.

## Verdict

Le contrat G4 reste ouvert. Aucun `BenchmarkOutputContract-v1` complet, kernel
résident, campagne p95 ou temps à 50k ne reçoit la cible secondaire d'une
seconde.

Le pin ajoute `BallFormToBallEvent-v0`, première chaîne qui produit une clé de
sphère, des supports, `I_B/U_B`, un owner et une disposition. Huit CTests ciblés
passent en `0,08 s` sur `coord<=64`. C'est un progrès architectural, mais le
claim « étape 0A close » est réfuté pour `quantized_u16_input_only`.

## Blocages P0 de `BallEvent-v0`

1. `ball_event.hpp:125,156` convertit en `long long` des numérateurs de centre
   qui atteignent 81 bits en q3 et 67 bits en q4 sur des fixtures u16 valides.
2. `ball_event.hpp:75-83` construit `den^2` et `N^2` avant pgcd ; la fixture q3
   maximale atteint environ 162 bits dans `i128`.
3. La positivité q3 remultiplie les numérateurs et dépasse encore 128 bits.
4. Le probe borne ses coordonnées à 64. Son test `--coord=128` meurt au parseur,
   pas dans un preflight géométrique u16.
5. Le juge ne recertifie pas la positivité ni la clé primitive. Le mutant de
   clé ajoute artificiellement `runs.size()` à ses fautes et ne juge rien.
6. Indices et `PointId` sont confondus dans `BallEvent`; aucune permutation de
   stockage à IDs non denses n'est testée.
7. Manquent epoch/profile/schema, niveau exact, complétude census/supports,
   statut initial pending, caps transactionnels et publication atomique.
8. `U_B!=S` ne suffit pas seul : la violation `RelevantGP` dépend du support
   propre pertinent et de sa lane `p+q<=smax`.

Audit, valeurs exactes et dix fixtures :
[`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## Cycle owner : autre auditeur reçu puis fermeture Claude

L'autre auditeur a correctement démontré au parent `1aa487d` que l'owner q3
comparait des positions Morton/`GenerationRank` et que sujet et juge partageaient
la même erreur. Sa fixture équilatérale relabellée impose l'arête `(0,1)` par
`PointId`, alors que l'ancien code choisissait `(1,2)`.

Claude ferme ce défaut au commit `f516198` : `spid` est passé au comparateur, le
juge scientifique reste non muté, la clé globale trie les vrais labels, trois
relabelings passent et le mutant `owner-generationrank` mord. Le build ciblé,
les deux fixtures owner/rang et les six portes q3 passaient au relevé précédent.
Cette réception est locale ; elle ne couvre ni `M3`, ni BallEvent, ni fold.

Contre-audit de la réponse de l'autre auditeur :
[`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Déblocages mathématiques actifs

- `SOC64` : succès des 64 couples de coins de `(C-A)×(B-C)` implique `ALL` ;
  échec `UNKNOWN`.
- `CORNER512` : les 512 triples de coins caractérisent `ALL` pour l'enveloppe
  AABB continue ; un échec fictif n'est pas `NONE` pour les points stockés.
- LP projectif : un groupe crédite une paire si `d` est dans son cône positif
  et `kappa(d)<||d||^2`; une base optimale emploie au plus trois IDs.
- Multiplicité LP : huit extractions disjointes sont un fast path q4 ; l'arbre
  exact demande jusqu'à 3280 LP et reste oracle pairwise relatif au pool.
- Cages : une base positive minimale 3D a quatre à six sites et jusqu'à huit
  formes de fleur ; tétra-only est incomplet.

Ces objets restent propositions/falsificateurs. Aucun n'est un producteur 50k
reçu. Les corrections de contre-audit sont intégrées : largeur du constructeur
LP distincte des 87 bits de `F`, base de rang inférieur, complétude seulement à
pool total, cages cinq-sites `omega=4`, comptes de la fixture axiale bornés à la
banque alignée et recalcul des fleurs après minimisation.

## Ordre bloquant

```text
réparer 0A sur tout u16
  -> juge indépendant de toutes les identités
  -> fermer 0B jusqu'au payload borné
  -> intégrer SOC64 / LP / cages au même sink
  -> mesurer E4 puis M4 puis H et le coût transitif
  -> portage device et parité
  -> campagne G4 50k
```

La proposition consolidée est
[`../PROPOSITION.md`](../PROPOSITION.md). L'index court des audits est
[`README.md`](README.md).

## Rejeux ponctuels

Sur le worktree avant les deux deltas logiciels concurrents :

```text
cmake --build build/v3 --parallel --target mhgp3v_wspd_wavefront_probe
fixtures owner/rang : 2/2 en 0,02 s
q3_* : 6/6 en 0,30 s

cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel --target mhgp3v_ball_event_probe
ball_event_* : 8/8 en 0,08 s
```

Ces verts sont locaux. Ils ne promeuvent aucun statut public et ne compensent
pas les blocages ci-dessus.
