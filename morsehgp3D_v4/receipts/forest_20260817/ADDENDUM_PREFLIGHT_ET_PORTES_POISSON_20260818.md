# Addendum — préflight de sortie, cardinalités par K, porte q2_birth_lower_bound

Date : 18 août 2026. Base : `3169d21` (réponse aux contre-audits
Poisson) ; code livré dans le commit portant ce reçu. Exécute les
actions minimales des contre-audits `0328bf5`/`772a8d9` qui ne
dépendent ni de la décision de contrat `product` (remontée à
l'utilisateur) ni du chantier de tuilage (politique u64/u32 : différée
à l'existence des tuiles, comme le § 6.1 du contre-audit le veut —
aucune promotion aveugle en u64 n'a été faite).

## `--output-preflight-only`

Pipeline jusqu'au census, puis expansion par tranches de boules SANS
matérialisation (chaque événement est libéré aussitôt compté) :
compteurs u64 par K — événements, incidences ($q+d$), octets projetés
au format `ForestEvent` résident — puis totaux. Le préflight dit ce
que coûterait la matérialisation AVANT de la payer. Recoupé au compte
près : à uniform n=400, total 104 802 événements = la valeur gravée du
reçu des pentes. Porte de fumée en CI (code 0).

## Trois cardinalités par K (contre-audit § 6.2)

Chaque run publie désormais, par K : `evenements` (certificats
générés), `facettes` (sommets du K-graphe vus), `deltas` (transitions
critiques émises), `attachements`, `fusions`. Sur uniform n=400 :

| K | événements | facettes | deltas | fusions |
|---|---|---|---|---|
| 1 | 1 339 | 400 | 340 | 399 |
| 2 | 2 600 | 2 675 | 1 409 | 2 674 |
| 3 | 4 209 | 8 013 | 3 492 | 8 012 |
| 4 | 6 161 | 17 421 | 5 472 | 17 419 |

La distinction K=1 du contre-audit est VISIBLE en production : 1 339
certificats de Gabriel pour 399 arêtes critiques (×3,4 — l'asymptote
Poisson est 4). Le rapport deltas/événements par K mesure le gain
encore accessible (RNG-HGP, Borůvka) ; facettes/événements la part
incompressible d'une sortie au niveau des facettes.

## Porte `q2_birth_lower_bound` (oracle borné, n = 200)

Oracle exhaustif $O(n^3)$ — la règle « jamais de vérification
exhaustive » exclut expressément les oracles bornés qui ÉTABLISSENT la
vérité. Pour chaque paire : $j$ = intérieurs stricts de la boule
diamétrale ouverte (test entier exact $\vert 2z-(a+b)\vert^2 < D^2$).
Résultat sur uniform n=200 :

- $N_0..N_4 = 629 / 557 / 529 / 497 / 497$ — le théorème du
  contre-audit (« 4 par point à CHAQUE profondeur, indépendamment de
  $j$ ») est empiriquement au rendez-vous : les $N_j$ sont plats à
  travers les profondeurs (l'écart à $4n$ est l'effet de bord du cube
  fini, $\approx 2{,}6$–$3{,}1$ par point ici).
- L'argument d'INJECTION est vérifié tel que prouvé : chaque
  $\tau_z = \sigma \setminus \lbrace z \rbrace$ a $(a,b)$ pour diamètre
  unique (toute autre paire strictement plus courte — une égalité
  exacte hors position générale serait comptée dégénérée, jamais un
  échec silencieux : 0 rencontrée ici), et les 19 544 $\tau$ propres
  sont globalement DISTINCTES — 0 violation.
- Planchers anti-vacuité : $N_0, N_1, N_2 \geq 1$ et distinction K=1
  ($N_0 \geq 2(n-1)$).
- Mutant `birth-dup-tau` (un $\tau$ dupliqué présenté au vérificateur
  de distinction) : tué à code 4.

109 CTest verts. Reste de la tâche : la politique
`global u64 / local u32 + refus de tuile` naîtra AVEC les tuiles
(streaming/out-of-core), et le champ `product` attend la décision de
l'utilisateur — la campagne G4 reste d'ici là une mesure du coût de
découverte, non contractuelle.
