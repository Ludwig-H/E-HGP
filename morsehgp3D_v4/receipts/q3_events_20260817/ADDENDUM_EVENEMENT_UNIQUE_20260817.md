# Addendum — l'événement q3 devient UN enregistrement, jugé champ par champ

Date : 17 août 2026. Base : durcissement oracle (commit précédent). Répond au
verrou n° 1 commun aux trois audits du jour (`AUDIT_CIBLE_5964214_Q3_EVENT_
ET_PROCHAIN_VERROU`, `CONTRE_AUDIT_6BEEB0_COUPLAGE`, `AUDIT_CIBLE_AVANT_Q4`).

## Ce qui change

1. **`Q3Event` matérialisé** (`src/events/q3_event.hpp`) : `SupportKey3`
   (trois `PointId` u32 triés — plus jamais des `i32`), `EdgeKey owner`,
   `Q3BallKey`, `Q3Level`, `depth`, `interior[8]` trié par `PointId`. Ordre
   total et égalité sur TOUS les champs.
2. **Contrat causal en deux temps** : la forme brute de la boule (5
   coefficients) et le niveau non réduit sont formés dans le CANDIDAT, avant
   tout census (`q3_ball_form`, `q3_level_raw`) ; la canonisation pgcd/signe
   (`q3_ball_key_reduce`, `q3_level_reduce`) est une fonction PURE de cette
   forme pré-census, appliquée à la publication comme le tri des intérieurs.
   Aucun champ ne peut provenir du census — et le pgcd n'est payé que par
   les survivants (mesure : ~300 candidats meurent pour 1 publié ;
   la canonisation par candidat coûtait un facteur 3, voir « Coût »).
3. **Contrat de capacité** : `smax > 11` est REFUSÉ (code 2, porte
   `mhgp4_q3_refus_smax`) — le profil `K_max <= 10` dimensionne les
   certificats et `interior` à `h_3 - 1 <= 8` ; plus aucune troncature
   silencieuse possible.
4. **Invariants forts par rectangle/ancre survivante** (code 3) :
   `core_ids == h_cœur` (collecteur contre compteur à coins),
   `collecteur h_a/h_b == histogramme`, paquet sans doublon,
   `taille du paquet == h_cœur + h_a + h_b < h_3`, et à la publication
   `|interior| == depth` — la liste d'intérieurs EST la profondeur.
5. **Juge sur enregistrements COMPLETS** : l'énumération brute construit les
   mêmes records (owner désormais départagé sur les VRAIS `PointId`, plus
   jamais les rangs Morton — audit § 3) et la comparaison est le
   multiensemble entier : un record faux d'un seul champ compte en manquant
   ET en trop. `q3_ball_depth` gagne un collecteur d'intérieurs optionnel
   (les blocs crédités sont énumérés feuille à feuille en mode collecte).
6. **Oracle indépendant étendu** (`tests/q3_oracle_test.cpp`) :
   - BallKey du sujet contre la forme oracle `A_o = det²`,
     `B_o = -2·det·num`, `C_o = |num|² - |a·det - N|²`, comparée
     PROJECTIVEMENT (produits croisés < `2^286`, aucun pgcd côté oracle) ;
   - listes d'intérieurs (IDs externes triés) sujet contre oracle ;
   - fixture owner AU-DESSUS DU BIT 31 : triangle équilatéral
     `(0,0,0),(1,1,0),(1,0,1)`, quatre affectations d'IDs dont
     `{2^31+7, 2^31+3, 5}` et `{2^32-2, 1, 2^32-1}` — l'arête owner gravée
     est la plus petite EdgeKey u32 ; un `PointId` signé la déplacerait.

## Mesures

| configuration | résultat |
|---|---|
| `uniform n=400 --judge` (records complets) | 48 965 événements, 0 manquant, 0 en trop |
| appariements `--packet=off`, `--cover=root`, `--census=tree` | même verdict 0/0 sur les records complets |
| mutants `packet-no-exclude`, `cover-dmin` | tués (code 4) |
| `--smax=12` | refus code 2 |
| oracle (8 nuages, 39 852 triangles) | 0 désaccord, y compris `ballkey`/`interieurs` |
| 33 portes CTest | toutes vertes |

## Coût (eight_clusters n=2000, s=8, seed 3, CPU conteneur)

| étape | t_instruction |
|---|---|
| avant (base 12,7 s, sans candidat ni records) | 12,7 s |
| canonisation pgcd par CANDIDAT (77,5 M porteurs) | 38,6 s |
| pgcd binaire de Stein (essai, rejeté) | 62,5 s |
| forme brute par candidat + canonisation par survivant | **16,6 s** |

Le pgcd hybride (Euclide u128 jusqu'à passer sous 64 bits puis Euclide
matériel, sortie anticipée à pgcd 1) est gravé dans `q3_event.hpp`. Les
~4 s restantes sont la matérialisation de la forme brute par candidat —
le prix assumé du contrat « candidat complet avant census » sur 77,5 M
porteurs pour 249 093 événements, bit à bit identiques à la base.

## Ce qui reste (ordre des audits)

- comparateur de niveaux U192 + macro-lots de niveaux égaux (contre-audit
  489c617 § 2 — le verrou exact avant la forêt) ;
- fixture `q4_source_independent_from_q3` (audit bc5b05d § 2) ;
- `AcuteSeed` extrait en amont du census, puis q4 axial ;
- API `InputPoint{id, position}` de bout en bout (le probe fabrique encore
  ses `PointId` depuis l'ordre du nuage ; l'oracle, lui, les reçoit déjà).
