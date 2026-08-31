# Grand-livre v6 — termes de coût publiés et portes go/no-go

Le contrat de coût v6 est **sortie-sensible** : aucune promesse
« sous-quadratique pour toute entrée » n'est recevable pour un catalogue
explicite (barrière `linked_arcs_u16`, voir `docs/MATHEMATIQUES.md` § 5).
La forme du contrat : `temps = préparation quasi linéaire + Σ termes`, chaque
terme payé publié avec sa pente propre.

**Méta-clause : un terme payé omis du grand-livre est un NO-GO en soi**
(pattern d'erreur : le vert par dilution — une somme de termes ou un
ajustement global peuvent masquer un terme en n^1,8).

## 1. Termes publiés par run

Statut d'instrumentation : un terme sans compteur câblé dans
`GenerateStats` et la sortie est marqué `(candidat J3)` — il n'entre dans
aucune pente tant qu'il n'est pas publié. Depuis le cinquième cycle d'audit
(31 août), chaque monnaie câblée déclare **sa population, son point
d'incrément et une identité fermante** ; le validateur `bench/pentes.py`
vérifie les identités fermantes des vecteurs d'octaves.

| Terme | Définition (population ; point d'incrément ; identité fermante) |
|---|---|
| `R` | rectangles WSPD terminaux vivants (par lane, via masques ; `rect_alive`) |
| `V_wspd` | descente fusionnée, DEUX composantes : `nœuds_temoins` = nœuds d'index visités par `count_universal_witnesses` (population : appels initiaux et terminaux) ; `coins` = évaluations de COUPLES de coins (`corner_evals` — masse d'évaluations géométriques, pas un nombre d'« appels témoins ») |
| `V_R, C_R, P_R` | route M (candidat J3) : nœuds classés, couples de coins, couples PENDING |
| `P_factor` | évaluations d'auto-produits des histogrammes (\|A\|²+\|B\|² ; par rectangle vivant ; `p_factor[3]`) |
| `H_rect` | Σ points des handles, UNE fois par (rectangle vivant, lane q3/q4) ; incrément à la construction des handles (`h_rect`) |
| `H_scan` | nœuds d'index visités par `anchor_cover_from_handles` ; population = ancres ENTRÉES ; incrément par delta de `AnchorScratch::visits` au site d'appel commun (`h_scan`) |
| `M_anchor` | Σ tailles de cover à l'ENTRÉE du corps par ancre — population COMMUNE q3/q4 = `entrees_ancres` (après le prétest par requête, avant W3/W4/secteurs/grille) ; identité : population publiée à côté (`entrees_ancres=q2/q3/q4`) |
| `entrees_ancres` | ancres entrées dans le corps par lane (= hist_survivants − tués du prétest par requête) ; identité : Σ octaves `ancres` == `entrees_ancres[q4]` |
| `E` | (candidat J3) ancres résiduelles, étiquetées par lane |
| `W_sweep1` | passe 1 du sweep : ÉVALUATIONS ÉLIGIBLES (après le saut des trois indices du seed ; `tests_coeur` = `q4_core_site_tests`) ; identité : Σ octaves `w1` == `tests_coeur` ; les itérations complètes sont `iters_coeur` |
| `W_sweep2` | passe 2 du sweep : sites rescannés par seed survivant, une évaluation (P, B) par site après le saut des trois indices (`tests_passe2` = `sweep_pass2_site_tests`) ; les itérations complètes sont `iters_passe2` ; population : seeds de `seeds_passe2` |
| `W_scan_q3` | masse du filtre de profondeur q3 (`q3_depth_site_tests`, câblé) |
| `sweep_root_comparisons` | comparaisons exactes payées par le tri des racines (`tri_comparaisons`, incrément dans le comparateur du tri) |
| `sweep_pass2_seeds` | seeds q4 survivants entrés en passe 2 ; identité : Σ octaves `passe2` == `seeds_passe2`, et par octave o : `seeds[o] == cellules[o] + coeur[o] + corde[o] + passe2[o]` (les quatre issues d'un seed q4) |
| `sweep_roots_onchord` | racines sur corde construites et triées (observable, jamais « × log ») |
| `sweep_root_groups` | blocs de racines égales traités (règle de bloc) |
| `sweep_roots_offchord` | racines strictement hors corde (rejet exact) |
| `P_role` | complétions soumises à la cascade (= `q4_completions`) |
| `S3, S4, S4_surv` | seeds q3, seeds q4, seeds q4 survivants de passe 1 ; identité : Σ octaves `seeds` == `seeds[q4]` |
| `kills[t]` | morts par tueur t (W_q, secteurs, grille, cœur, corde, hist, …) ; par octave pour q4 : `cellules`/`coeur`/`corde` (Σ == scalaires respectifs) |
| `Q_try` | candidats entrés en cascade finale |
| `C_emit` | candidats remis au tri/RLE |
| `B` | BallKey uniques après RLE |
| `B_pref` | clés survivantes au préfiltre exact (la frontière de digest v6) |
| `S_shell` | Σ \|U_B\| (`census_shell`, câblé) |
| `V_census` | DEUX composantes déclarées : `prefiltre_nœuds`/`range_add` = traversées de `ball_depth_at_least` (count-only) ; `census_nœuds`/`census_feuilles` = traversées de `ball_census` (passe 2, tests de puissance en feuille) — jamais additionnées |
| `S_forest` | enregistrements de forêt publiés (facettes, deltas, nœuds — câblés) |
| `T_input, V_motif` | (candidat J3) coût de FABRICATION des entrées — les familles stationnaires évaluent chaque point contre ~n motifs : la génération d'entrée est elle-même quadratique et doit être séparée de toute pente du pipeline |
| HWM | pic mémoire par rôle (rss_mb par paliers, câblé ; par rôle : candidat J3) |

Les temps sont exclusifs ou explicitement inclusifs (`dont`), jamais
additionnés s'ils s'emboîtent. La sonde de queue E6 publie sept vecteurs de
16 octaves (`octaves_q4` : ancres, seeds, w1 ; `octaves_q4_seeds` :
cellules, coeur, corde, passe2) — l'octave d'une ancre est
`floor(log2(taille du cover))`, et le vecteur `ancres` compte les ENTRÉES de
`process_anchor_q4` (population `entrees_ancres[q4]`).

## 2. Ce qui a le droit de rester quadratique, et où

- `B`, `C_emit`, `S_shell`, `S_forest` sur `linked_arcs_u16` : c'est la
  sortie — contrat, pas échec. La porte vérifie que les termes de préparation
  n'explosent pas avec eux.
- `M_anchor`, `H_scan` sur la contre-fixture calotte–lentille : aucun
  certificat à témoins ne peut fermer ces ancres ; publié comme réfutation
  bornée.
- `W_sweep1`, `S4` sur les familles **dilatées** : association diagnostique
  v5 (cohortes non appariées) avec les échelles de hauteur du générateur ;
  stress non extrapolable, jamais une pente de lane.
- `V_R` si tout reste MIXED (peignes entrelacés) : compteur de réfutation de
  la route M.

## 3. Portes go/no-go

GO d'un jalon (sur `uniform`, `eight_clusters`, `terrain_stationnaire`,
`scanline_stationnaire` ; 3 graines ; pentes sécantes aux deux pas) :

- pente de **chaque** terme payé < 2 strictement ; cible de travail ≤ 1,25
  pour les termes de préparation (`V_wspd`, `H_rect`, `W_sweep1`) sur
  `uniform`/`eight_clusters` ;
- coût apparié par famille ≤ référence v5 sur les familles faciles (banc
  apparié contrebalancé intra-processus, médiane des rapports par paire) ;
- `W_sweep2/W_sweep1` publié (un ratio qui dérive est un signal, pas un
  réglage) ;
- ratio `M_anchor/B_pref` publié (coût de préparation par unité de sortie).

NO-GO :

- un terme omis ; une somme de termes dans une porte de pente ; un ajustement
  global ;
- `M_anchor` ou `W_sweep1` de pente ≥ 2 sur un régime **stationnaire**
  annoncé ⟹ l'étage E6 (Tier R, moteur plan) devient prioritaire, précédé de
  sa sonde contrefactuelle appariée ; si E6 échoue aussi, le régime est
  publié comme mur documenté ;
- balayage répété non borné de bitsets denses ; réénumérations de carriers
  sans compteur.

## 4. Références des seuils

Les seuils numériques d'une porte sont figés **avant** la mesure qu'ils
jugent et versionnés avec elle. Un seuil ajusté après coup est le pattern
d'erreur « statut déclaré au lieu de mesure ».
