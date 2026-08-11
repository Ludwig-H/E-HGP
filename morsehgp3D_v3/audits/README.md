# Audits de MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier sépare le verdict indépendant, les notes de livraison de Claude,
les reçus reproductibles et les archives. Une note de livraison ne devient pas
un reçu par son titre ou par le seul passage de ses tests.

## État live

Le snapshot committé courant est
`cbac109a09c2575cdf875b19de1570265bd5bf08`. Le worktree concurrent contient
la sonde q2 `pair_selfjoin_probe.cpp`, son raccord CMake et des corrections
documentaires; aucun de ces changements ne reçoit une lane. Toute réception
live est liée à une empreinte précise; une modification exige un nouveau rejeu.

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : unique verdict consolidé,
  distinction stable/worktree, contrat non rempli et ordre des portes.
- [`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md) : baseline bit à bit du sidecar `9483b1c`, plus audit du plan
  séparateur et borne des triples q4; son statut sidecar est supplanté par le
  delta `cbac109` ci-dessous.
- [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) :
  contre-audit du correctif sidecar `cbac109`, trois reproductions hostiles et
  audit de la sonde q2 concurrente.
- [`../PROPOSITION.md`](../PROPOSITION.md) : architecture candidate
  séparant EMST, lane q2 dual-tree et recherche d'ancres q3/q4, avec ses
  invariants et portes encore ouvertes.
- [`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md) : preuve du pont H0, fast principal, resolver et
  contre-fixture du prune trop fort.

L'autorité du théorème d'inertie est
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md).
Le contrat et le statut public restent régis par
[`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) et
[`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).

## Livraisons récentes

- [`NOTE_CLAUDE_FAST_EXAEQUO_RECU_ET_PRUNE_20260811.md`](NOTE_CLAUDE_FAST_EXAEQUO_RECU_ET_PRUNE_20260811.md) : fast principal multi-lot reçu au commit
  `84ba459` relativement à la table fournie. Le plan séparateur reste une
  sonde de masse, sans exposant asymptotique déduit de trois tailles.
- [`NOTE_CLAUDE_SIDECAR_FACTORY_V0_20260811.md`](NOTE_CLAUDE_SIDECAR_FACTORY_V0_20260811.md) : provenance du sidecar `9483b1c`, reclassé comme harnais v0
  déclaré et non reçu.
- [`NOTE_CLAUDE_SIDECAR_CORRECTIFS_CBAC109_20260811.md`](NOTE_CLAUDE_SIDECAR_CORRECTIFS_CBAC109_20260811.md) :
  provenance des corrections livrées à `cbac109`; l'annonce initiale de
  fermeture S1--S4 y est explicitement retirée après contre-audit.
- [`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md) : session CPU 48 threads sur machine G4, trois familles et
  deux pas. Aucun kernel ni tuple; aucune lane admise. La note de session
  documente la cible comme `TERMINATED`.
- [`NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md`](NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md) : provenance des portes CPU et des campagnes antérieures.

Les nombres `52/52`, `57/57`, `191/191` et la campagne TSan sont des
déclarations de livraison tant qu'un manifeste brut ne les lie pas au commit,
aux binaires et à l'environnement. Ils ne compensent aucun mutant absent.

## Reçus G4 mass-only

Les sorties sont dans
[`../receipts/g4_massonly_20260811/`](../receipts/g4_massonly_20260811/).

| fichier | SHA-256 |
| --- | --- |
| `cell_50k_raw.txt` | `6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe` |
| `mask_scale_raw.txt` | `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740` |

Après prune d'axe, q2 conserve
`465 371 500--2 862 879 000` tuples, q3
`14 667 530 000--131 762 100 000` et q4
`330 437 400 000--9 968 861 000 000`. Les temps count-only sont
`0,174--29,153 s`. Aucun tuple n'a été formé : ces nombres refusent la route
combinadique, ils ne mesurent pas un débit de source.

Le pinceau q4 conserve lui-même, au pas 6, plus de `2,74e9`, `1,063e10` et
`1,020e9` triples canoniques sur les trois familles. Un range reporter ne peut
être proposé comme solution après avoir payé cette masse.

## Décisions cohérentes

- `q_min` est la provenance Morse; `q_cert` est une arité de support propre
  positif effectivement prouvée et sert seulement au certificat d'inertie.
- `p+q_cert>=K+2` autorise une tombstone du quotient horizontal normalisé avec
  resolver. Il ne supprime ni Gamma, ni ses incidences, ni ses verticales.
- Le fast principal en lot multiple exige `q<=k+1`, une fermeture autoritative
  et des lookups stricts pré-lot. `prefix-all` reste relatif à sa table.
- Séparer `C` de `conv(A_C)` exclut uniquement la branche locale pertinente
  contenue dans `A_C`. Un support de haut niveau peut subsister; le verdict
  n'est jamais `no_support`.
- `k=1` suit une lane EMST exacte distincte. Le prune diamétral q2 ne supprime
  jamais une ancre q3/q4; les arités supérieures attendent leur propre source
  sparse complète et leurs niveaux peu profonds.
- L'ancien `center-cover` à plus de 600 secondes est rejeté. La nouvelle sonde
  q2 par blocs doit fermer toutes les paires, rejouer directement les blocs
  prunés et publier visites, microtuiles, octets et high-water; elle ne
  réutilise pas l'ordonnance rejetée.
- Une mesure count-only, un accord moyen ou un digest ne qualifie ni la source,
  ni le fold complet, ni le statut public.

## Archives

Tous les autres audits datés sont des snapshots historiques. Ils restent
utiles comme preuves, contre-exemples ou provenance, mais leurs phrases au
présent ne décrivent pas le worktree. En cas de divergence, le verdict live et
les autorités mathématiques ci-dessus prévalent. Toute contradiction nouvelle
devient une fixture permanente avant optimisation.

GCP non utilisé pour cette consolidation.
