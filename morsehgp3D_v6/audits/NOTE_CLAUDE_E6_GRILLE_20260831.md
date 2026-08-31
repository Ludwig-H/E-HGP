# Note Claude — étage E6 implémenté en opt-in : grille raffinée G=16 sur les ancres lourdes, objet bit-identique, la pente de la pire graine repasse sous 2

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Ceci est une MESURE appariée locale à compteurs
déterministes — jamais un GO : l'activation par défaut attend la campagne
de confirmation hors échantillon et votre lecture.

## Le mécanisme (`--e6-grille`, opt-in, désactivé par défaut)

Suite de la sonde contrefactuelle (`NOTE_CLAUDE_SONDE_E6_20260831`) qui a
établi que (a) les cellules bloquantes des seeds tuées par cœur sont
presque mortes à G=8 (60–77 % à min ≥ h/2) et (b) le veto `near_m` exclut
la grille des ancres riches en réfutant sa propre prédiction :

- `CellGridT<G>` : la grille de cellules devient PARAMÉTRIQUE en G — le
  certificat 10.5 (condition affine aux quatre sommets), la borne du
  localisateur (relative en G, terme absolu 2⁻⁴⁰ inchangé) et la
  construction sont symboliques en G ; l'alias `CellGrid = CellGridT<8>`
  laisse la production STRICTEMENT inchangée (refactor pur, portes
  bit-identiques) ;
- sur une ancre q4 à cover ≥ 2¹⁰ avec `--e6-grille` : la grille RAFFINÉE
  `CellGridT<16>` est TOUJOURS construite (vetos `near_m` et ratio levés) ;
  une cellule 4× plus petite exige des témoins communs à une région 4×
  plus petite — condition plus faible, comptes plus élevés — et le kill de
  corde (th. 10.4/10.5) convertit les scans de cœur en consultations de
  cellules.

## Sûreté d'objet — prouvée, pas déclarée

Une seed tuée par cellules porte le certificat « toute complétion a
profondeur ≥ h4 » : dans la base OFF elle n'émettait RIEN (morte au cœur, à
la corde, ou racines toutes profondes). Le multiensemble ÉMIS est donc
identique — vérifié : `digest_all`, dix forêts, post-préfiltre ET digest
brut avant RLE identiques sur chaque paire ON/OFF (reçu
`e6_grille_appariee_20260831`), et gravé en porte permanente
`mhgp6_e6_grille_objet` (trois familles, égalité des quatre monnaies,
W_sweep1 jamais augmenté, plancher ≥ 100 grilles construites contre le vert
par vacuité). 70/70 portes.

## Mesure appariée (compteurs déterministes, graine 5 = la pire)

| famille, n | W_sweep1 OFF → ON | Δ |
|---|---|---|
| scanline_st 16000 | 429 349 237 → 365 929 479 | −14,8 % |
| scanline_st 32000 | 2 277 544 947 → 1 359 667 302 | **−40,3 %** |
| terrain_st 16000 | 174 918 842 → 146 783 460 | −16,1 % |
| terrain_st 32000 | 740 313 876 → 498 320 908 | **−32,7 %** |
| eight_clusters 16000 | 1 406 480 323 → 1 235 186 692 | −12,2 % |

Pente sécante g5 au pas 16000→32000 : scanline **2,41 → 1,89**, terrain
**2,08 → 1,76** — la pire graine repasse sous le seuil 2 du GRAND_LIVRE
§ 3. La réduction CROÎT avec n (la queue qu'elle attaque porte une part
croissante de W_sweep1) : c'est le comportement anti-queue attendu, à
confirmer sur les tailles supérieures (G4) et hors échantillon.

## Ce qui manque avant toute activation par défaut

1. Verdict de la campagne de CONFIRMATION hors échantillon (en cours,
   graines 6/7/8, tailles 10000/20000/40000) sur la base OFF — puis une
   campagne appariée ON préenregistrée si E6 est confirmé actif ;
2. le coût de construction G=16 dans le grand-livre (il est compté dans les
   monnaies existantes ; un terme dédié `V_grille` est un candidat) ;
3. votre lecture : seuil `cover ≥ 2¹⁰`, levée des vetos, et la question du
   second niveau (hiérarchique) restent ouverts à l'audit.
