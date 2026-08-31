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

| Terme | Définition |
|---|---|
| `R` | rectangles WSPD terminaux vivants (par lane, via masques) |
| `V_wspd` | visites de la descente fusionnée (nœuds + appels témoins) |
| `V_R, C_R, P_R` | route M : nœuds classés, couples de coins, couples PENDING |
| `H_rect` | Σ_R handle_mass(R) — masse offerte par les covers, une fois par rectangle |
| `H_scan` | masse réellement reparcourue par ancre (Σ ancres_surv × handle) |
| `M_anchor` | Σ_e m_e — sites de tape par ancre survivante |
| `E` | ancres résiduelles, étiquetées par lane |
| `W_sweep1` | passe 1 du sweep : sites scannés (cœur saturé), tous seeds |
| `W_sweep2` | passe 2 : racines triées × log, seeds survivants seulement |
| `frag` | fragments shallow émis par les sweeps |
| `S3, S4, S4_surv` | seeds q3, seeds q4, seeds q4 survivants de passe 1 |
| `kills[t]` | morts par tueur t (W_q, secteurs, grille, cœur, corde, hist, …) |
| `Q_try` | candidats entrés en cascade finale |
| `C_emit` | candidats remis au tri/RLE |
| `B` | BallKey uniques après RLE |
| `B_pref` | clés survivantes au préfiltre exact (la frontière de digest v6) |
| `S_shell` | Σ \|U_B\| (≤ 12·B par plafond) |
| `V_census` | visites de l'index par le préfiltre + census |
| `S_forest` | enregistrements de forêt publiés (facettes, deltas, nœuds) |
| HWM | pic mémoire par rôle (amont / en construction / sortie / temporaires) |

Les temps sont exclusifs ou explicitement inclusifs (`dont`), jamais
additionnés s'ils s'emboîtent.

## 2. Ce qui a le droit de rester quadratique, et où

- `B`, `C_emit`, `S_shell`, `S_forest` sur `linked_arcs_u16` : c'est la
  sortie — contrat, pas échec. La porte vérifie que les termes de préparation
  n'explosent pas avec eux.
- `M_anchor`, `H_scan` sur la contre-fixture calotte–lentille : aucun
  certificat à témoins ne peut fermer ces ancres ; publié comme réfutation
  bornée.
- `W_sweep1`, `S4` sur les familles **dilatées** : imputable à la famille
  (verdict v5), stress non extrapolable.
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
