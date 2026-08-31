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
aucune pente tant qu'il n'est pas publié (exigence de l'audit du 31 août).

| Terme | Définition |
|---|---|
| `R` | rectangles WSPD terminaux vivants (par lane, via masques) |
| `V_wspd` | visites de la descente fusionnée (nœuds + appels témoins) |
| `V_R, C_R, P_R` | route M (candidat J3) : nœuds classés, couples de coins, couples PENDING |
| `P_factor` | évaluations d'auto-produits des histogrammes (`p_factor[3]`, câblé) |
| `H_rect` | (candidat J3) Σ_R handle_mass(R) — masse offerte par les covers |
| `H_scan` | (candidat J3) masse reparcourue par ancre (Σ ancres_surv × handle) |
| `M_anchor` | (candidat J3) Σ_e m_e — sites de tape par ancre survivante |
| `E` | (candidat J3) ancres résiduelles, étiquetées par lane |
| `W_sweep1` | passe 1 du sweep : sites scannés (`q4_core_site_tests`, câblé) |
| `W_scan_q3` | masse du filtre de profondeur q3 (`q3_depth_site_tests`, câblé) |
| `sweep_root_comparisons` | comparaisons exactes payées par le tri des racines (câblé) |
| `sweep_pass2_seeds` | seeds q4 survivants entrés en passe 2 |
| `sweep_roots_onchord` | racines sur corde construites et triées (observable, jamais « × log ») |
| `sweep_root_groups` | blocs de racines égales traités (règle de bloc) |
| `sweep_roots_offchord` | racines strictement hors corde (rejet exact) |
| `P_role` | complétions soumises à la cascade (= `q4_completions`) |
| `S3, S4, S4_surv` | seeds q3, seeds q4, seeds q4 survivants de passe 1 |
| `kills[t]` | morts par tueur t (W_q, secteurs, grille, cœur, corde, hist, …) |
| `Q_try` | candidats entrés en cascade finale |
| `C_emit` | candidats remis au tri/RLE |
| `B` | BallKey uniques après RLE |
| `B_pref` | clés survivantes au préfiltre exact (la frontière de digest v6) |
| `S_shell` | Σ \|U_B\| (`census_shell`, câblé) |
| `V_census` | (candidat J3) visites de l'index par le préfiltre + census |
| `S_forest` | enregistrements de forêt publiés (facettes, deltas, nœuds — câblés) |
| `T_input, V_motif` | (candidat J3) coût de FABRICATION des entrées — les familles stationnaires évaluent chaque point contre ~n motifs : la génération d'entrée est elle-même quadratique et doit être séparée de toute pente du pipeline |
| HWM | pic mémoire par rôle (rss_mb par paliers, câblé ; par rôle : candidat J3) |

Les temps sont exclusifs ou explicitement inclusifs (`dont`), jamais
additionnés s'ils s'emboîtent.

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
