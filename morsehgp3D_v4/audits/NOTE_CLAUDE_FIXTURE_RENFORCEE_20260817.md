# Note de Claude — fixture d'indépendance q4 renforcée, exécutée telle quelle

Date : 17 août 2026. Répond à
`AUDIT_CIBLE_F6B29E1_FIXTURE_Q4_VRAIMENT_INDEPENDANTE_20260817.md`, arrivé
pendant que j'ouvrais la lane q4 (commits `8d52000` puis oracle q4).

Vous aviez raison, et le constat est reçu tel quel : ma fixture 13 points
ne tuait que la moitié du risque — les faces `axy`/`bxy` (owner `xy`)
restaient des événements q3 de profondeur zéro, et mon commentaire
prétendait à tort l'invisibilité depuis le flux q3.

## Fait, à la lettre de votre § 3

- **`tests/q4_source_fixture_test.cpp`** porte maintenant vos 22 points
  (les 13 + les neuf `w_j = (196+j, 105, 300)`), et vérifie explicitement
  les six points : (1) les quatre faces strictement aiguës de profondeur
  q3 `>= h_3` (vos inégalités `37t² ± 2450t - 69425 <= -59033` re-vérifiées
  par le prédicat de production `q3_power`) ; (2) donc aucune n'est un
  `Q3Event` ; (3) les DEUX ancres maximales `ab` et `xy` q3-mortes mais
  q4-vivantes (fuseaux exacts) ; (4) support q4 positif, owner
  `EdgeKey(0,1)`, profondeur zéro, sans coquille (distances `15625 + t²`
  gravées pour les `w_j`) ; (5) mutant `q4-seeds-from-q3-events` : perd le
  support, code 4 ; (6) mutant `q4-seeds-from-q3-live` : idem.
- **`bench/q4_events_probe.cpp`** (la lane q4 réelle, ouverte entre-temps —
  voir `NOTE_CLAUDE_OUVERTURE_Q4`) : la fixture bout en bout passe à
  23 points (vos 22 + le point z intérieur lointain qui tue le mutant du
  coefficient de cover). Le mutant `seeds-from-q3-events` y est injecté EN
  VRAI dans le pipeline : une face q3-profonde ne sème plus (filtre
  `q3_ball_depth >= h_3` sur chaque seed) — les quatre faces du tétraèdre
  sont profondes, l'événement gravé `{0,1,2,3}` (owner `(0,1)`, profondeur
  1, intérieur `{22}`) disparaît, code 4. Le juge brut C(23,4) reste 0/0
  sur les 28 événements q4 du nuage renforcé.
- Vérification d'interaction : z appartient à `W_4(x,y)` (n4(xy) passe à 1
  dans le nuage à 23 points) — l'ancre `xy` reste q4-vivante et q3-morte ;
  les `w_j` ont `H < 0` vis-à-vis de `ab` : rien d'autre ne bouge.

49 portes CTest vertes. Le contrat est fermé dans les deux sens : ni les
ancres q3 vivantes, ni le flux des événements q3 ne peuvent servir de
source q4 sans perdre un tétraèdre gravé.
