# Régimes v6 — familles de mesure, familles stationnaires, doctrine de pente

## 1. Deux classes de régimes, jamais confondues

**Familles dilatées** (héritées v3/v4/v5, portées bit à bit, digests de
conformité) : `uniform`, `eight_clusters`, `terrain`, `scanline_single_pass`,
`scanline_overlap_multiecho`. Dans `terrain` et `scanline_*`, les échelles
verticales croissent avec `coord ∝ sqrt(n)` à espacement sol constant :
amplitudes des bosses et hauteurs des plateaux dans `[coord/16, coord/8]`,
canopée dans `[1, coord/8]`, échos dans `[2, coord/10]` — l'étendue verticale
croît en `sqrt(n)`. Verdict v5 (TERRAIN_DEUX_ECHELLES, reçus terminaux) : la
super-quadraticité q3/q4 y est imputable à la famille — geler les deux
échelles rend q4 linéaire (exposants dans `[1,003 ; 1,014]`, trois graines).
**Rôle v6 : conformité différentielle et stress non extrapolable. Jamais une
pente de lane.**

**Familles stationnaires** (neuves, régimes de coût de première classe) :
hauteurs et tailles de motifs fixes en unités absolues, densité surfacique de
motifs constante, seule la fenêtre croît. Qualification exacte (audit du
31 août) : régimes **synthétiques stationnaires, physiquement motivés** —
l'épaisseur bornée par la physique motive la construction, mais aucune
comparaison à un corpus capteur n'est fournie ; une pente mesurée ici reste
une mesure synthétique. **Rôle v6 : toute conclusion de pente s'énonce ici.**

Le piège du « gel » v5 est documenté et évité : geler la seule hauteur en
laissant les supports horizontaux croître en `sqrt(n)` aplatit la famille
relativement (exposants sous-linéaires artificiels, `0,53–0,93` mesurés) ;
le témoin stationnaire correct fixe hauteurs ET tailles horizontales des
motifs, et fait croître leur **nombre** avec l'aire.

## 2. Spécification des familles stationnaires

Principe : les constantes sont la loi dilatée évaluée à `n0 = 8000` ; à
`n = n0`, les bornes de distribution coïncident avec la famille dilatée ; le
nombre de motifs est proportionnel à l'aire (arrondi au plus proche).
L'emprise `coord` emploie une **racine carrée entière à règle d'arrondi
explicite** (au plus proche, milieu vers le haut : `r+1` ssi `m > r(r+1)`),
jamais libm — la frontière du domaine est portable (audit du 31 août).

### `terrain_stationnaire(n, seed)`

- `coord = clamp(round_isqrt(25·n), 4, 65536)` (densité aréale 1/25,
  espacement moyen 5, inchangés) ; `c0 = round_isqrt(25·8000) = 447`.
- Nombre de bosses : `nb = max(1, round(6·n/8000))`.
- Par bosse : centre uniforme dans `[0, coord)²` ; rayon uniforme
  `[max(2, c0/6), max(3, c0/3)] = [74, 149]` ; amplitude uniforme
  `[max(1, c0/16), max(2, c0/8)] = [27, 55]`.
- Sol : somme des bosses actives (quadratique entière, division plancher),
  jitter uniforme {0,1,2}.
- Canopée : probabilité 1/50, lift uniforme `[1, max(2, c0/8)] = [1, 55]`.
- Clamp z ≤ 65535, déduplication, garde 200·n tirages.

### `scanline_stationnaire(n, seed)`

- `coord = clamp(round_isqrt(40·n), 4, 65536)` ; `c0 = round_isqrt(40·8000) = 566`
  (sqrt(320000) = 565,685… arrondi au plus proche).
- Champ : `nc = max(1, round(5·n/8000))` calottes (rayon `[94, 188]`,
  amplitude `[35, 70]`) et `ns = max(1, round(4·n/8000))` plateaux à bord
  franc (côté `[47, 113]`, hauteur `[35, 70]`).
- Balayage (anisotropie 4:1 gravée) : pas 2 le long des lignes,
  pitch 8 entre lignes, une ligne sur trois clairsemée, trous markoviens
  (entrée 1/40, sortie 1/8), jitter sol {0,1,2}, multi-échos : probabilité
  1/8, lift `[2, max(3, c0/10)] = [2, 56]`, second écho lift/2 avec
  probabilité 4/8 ; passes de complément bornées comme en v5.

`scanline_stationnaire` est un **hybride neuf** (audit du 31 août) : les
multi-échos sont actifs dès la passe principale et dans les passes de
complément, sans la passe de recouvrement initiale de
`scanline_overlap_multiecho` — « capteur inchangé » désigne les seuls
paramètres de balayage (pas 2, pitch 8, lignes clairsemées, trous), pas
l'identité d'une famille v5.

Les deux familles sont **neuves** (aucun digest v4/v5) : la v6 grave ses
propres digests de non-régression (`mhgp6_families_fixture`).

## 3. Contre-familles et fixtures de réfutation (jamais des régimes)

- `two_lines`, `collinear_seven` : portées bit à bit (réfutations gravées).
- `linked_arcs_u16` : porte permanente de la barrière de sortie
  (N = 6/10/18/34 → q3 12/40/144/544, q4 4/16/64/256, profondeur zéro,
  coquille = support, exact-once, équivariance, marges au-delà d'i64).
- Calotte–lentille (contre-famille de l'auditeur, 30 août) : à graver comme
  contre-fixture bornée u16 (`M_anchor`/`H_scan` quadratiques malgré tous les
  certificats à témoins) — question V6-Q4 aux auditeurs.
- Peigne `x_i = L^i` (facteurs singletons, masse des handles quadratique) : à
  graver avec le raccourci de facteurs comme porte associée.

## 4. Doctrine de pente

- Tailles d'intérêt : n = 8000, 16000, 32000 (labels `scale8000/16000/32000`).
  n ≤ 2000 = oracle de correction seulement, jamais une pente.
- Trois graines minimum (3/4/5) avec étendue publiée ; toute conclusion
  mono-graine est sans valeur (facteur 6,79 mesuré en v5 entre graines).
- Pentes **sécantes locales aux deux pas** 8000→16000→32000, par terme du
  grand-livre, jamais un ajustement global ni une somme de termes.
- Compteurs sémantiques déterministes pour les pentes (reproductibles quel
  que soit le nombre de fils) ; temps seulement avec reçu (binaire épinglé,
  répétitions appariées contrebalancées, machine au repos ou VM G4).
- Les mesures hors profil se retirent **nommément** (leçon
  PROFIL_SEPARATION v5).
