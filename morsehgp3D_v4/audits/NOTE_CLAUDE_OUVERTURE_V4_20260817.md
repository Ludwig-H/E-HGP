# Note de Claude — ouverture du chantier v4, état au premier jour

Date : 17 août 2026 UTC.
Cadre : `phase=exploration_v4_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## 1. Ce qui a été lu avant d'écrire une ligne

Manuscrit Parties I–II en intégralité (pages PDF 35–134), `PROPOSITION.md` v3
en intégralité, `README.md` v3, `AUDIT_ETAT_COURANT.md`, `PISTES_FERMEES.md`,
les 118 fichiers d'audits v3 par tranches chronologiques, le code
`prototype/` et `oracle/` v3, et la chaîne d'autorité `docs/`. Les rapports
de lecture détaillés (19 documents) sont versionnés sous
[`lectures_20260817/`](lectures_20260817/) — ils citent énoncés, numéros et
pages exactes, et chacun se termine par ses ambiguïtés.

Conformément à la consigne de l'utilisateur : **tout ce qui vient de la v3
est traité comme une piste, jamais comme un acquis** — la v3 a commis et
rétracté de nombreuses erreurs (WSPD rendue quadratique par un cap de masse
dans le critère terminal, scission par population, réfutation invalide du
fair split tree comparant deux arbres, boule d'apex non fail-open, sonde J0
à faux vert `two_lines`, …). Seules les contre-fixtures aux coordonnées
exactes sont reprises telles quelles.

## 2. Ce qui existe au premier jour (commits `f775c98`, `7bd3281` et suivants)

- Documents fondateurs : `docs/MATHEMATIQUES.md` (chaîne normative du
  manuscrit, réduction événements-boules, fuseaux, statuts par énoncé,
  questions Q1–Q5), `docs/ARCHITECTURE.md`, `docs/PLAN_DE_TESTS.md`.
- Code : arbre radix sur positions uniques (plus de tie-break par index —
  la limite v3 `n<=65535` tombe), front WSPD par vagues (terminal ⟺ séparé,
  scission par diamètre, prédicat entier sans racine), descente ternaire qui
  tue les ancres PENDANT la récursion (boule-cœur à arithmétique dirigée +
  `Hmin` exact par axe + borne minimax d'élagage), trois lanes q2/q3/q4 sur
  une seule vague avec masque de lanes.
- Portes : 17 CTests verts — invariants d'arbre, ledgers de masse exacts en
  128 bits, équivariance par permutation, juges fail-open par échantillon des
  blocs morts, mutants tués (`drop-rect`, `cap-terminal` — le bug WSPD v3
  re-gravé —, `radius-ceil` sur fixture d'arrondi gravée).
- Reçus (CPU 4 cœurs, non contractuels) : front WSPD 36 configurations
  (masse exacte partout, ~13–15 % plus petit que v3 à configuration égale) ;
  descente q2 12 configurations (98,74–99,84 % de masse fermée pendant la
  descente, zéro fausse mort, 18× moins de rectangles à instruire à
  n=32000) ; trois lanes à n=1200 (94,1/82,4/80,0 %).

## 3. Questions à l'auditeur (par priorité)

Les cinq premières sont détaillées dans `docs/MATHEMATIQUES.md` § 6 :

- **Q1** — bijection événements-boules (§ 2.1) : ma dérivation, à valider.
- **Q2** — qualité du minorant `h_coeur+h_a+h_b` : borne sur le mou,
  Campbell–Mecke en régime Poisson.
- **Q3** — complétude fail-open de bout en bout de la source WSPD.
- **Q4** — convention `F_K` du rendu § 9.1.
- **Q5** — politique des ex æquo (cosphères u16).

S'y ajoutent, issues de l'implémentation du premier jour :

- **Q6** — re-dérivation des formes `W_3 : H>0 ∧ 3H²>Ξ`, `W_4 : H>0 ∧ 2H²>Ξ`
  et des constantes `κ_3 = 1/(2√3)`, `κ_4 = sin 15°` (la v3 les affirme,
  leurs preuves sont dispersées dans ses audits ; mes constantes point-fixe
  `kA3/kA4` sont volontairement en retrait du floor exact, avec
  static_asserts — le serrage attend la preuve).
- **Q7** — l'autorité 64 coins (`corner64_all_lane`) : la v3 revendique
  l'exactitude sur l'enveloppe AABB continue par convexité séparée en a et b.
  Je n'en consomme que le sens suffisant (ALL ⟹ crédit), sous surveillance du
  juge ; l'énoncé et sa preuve restent à écrire proprement.
- **Q8** — la mesure v3 « un certificat évalué à chaque nœud ne peut pas
  économiser plus de visites qu'il n'en coûte » condamne-t-elle ma tentative
  de mort à TOUT niveau de la vague ? Ma première mesure appariée dit non
  (uniform n=8000 : même fermeture, 1,37× plus vite que tuer aux terminaux
  seulement), mais c'est une mesure à une graine sur une famille — le
  contre-exemple v3 (SOC64/BJD) portait sur des certificats plus chers.

## 4. Prochaines étapes côté implémentation

`h_a`/`h_b` par auto-jointure dual-tree à range-add (cutoff ~256, coins
distincts, masques — les trois pièges v3 gravés en portes), instruction des
ancres survivantes (q3 : lentille + acuité≡positivité ; q4 : seed + théorème
axial re-dérivé), BallKey/RLE/census, dix forêts + juge indépendant.
