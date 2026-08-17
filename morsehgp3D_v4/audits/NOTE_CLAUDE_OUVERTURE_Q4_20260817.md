# Note de Claude — la lane q4 est ouverte, en baseline jugée

Date : 17 août 2026. Exécute l'ordre de `bc5b05d` § 3 (ABI de la source
q4, `AcuteSeed` en amont, paquet `base_4`, cover d'arête coefficient 4,
exact-once du seed). Reçu : `receipts/q4_events_20260817/README.md`.
Dossier : `docs/MATHEMATIQUES.md` § 4.5 (nouveau) et question Q12.

## Ce que vous trouverez

- La source est la lane q4 du front partagé, jamais q3 : votre fixture 13
  points est maintenant une porte BOUT EN BOUT (`--fixture --judge`), avec
  un 14e point z intérieur lointain qui n'est visible que du cover
  coefficient 4 — il tue le mutant `cover-coef3`, et votre branchement
  interdit est injecté tel quel (`seeds-from-q3-live` = filtre d'ancre par
  `n3 < h_3` exact) et meurt sur la fixture.
- Complétion énumérée dans le cover, PAS la sélection axiale : c'est la
  baseline de réception (comme site-major avant l'arbre de centres). Juge
  brut C(n,4) sur records complets : 0/0 sur 8,2 M sous-ensembles
  (uniform n=120, 7 909 événements) et sur eight_clusters n=120 (3 235).
- Arité 4 STRICTE par quatre orientations homogènes (centre sur une face
  ⟹ le candidat appartient à une autre lane — ignoré, jamais publié).
  Compteur : 67 % des complétions meurent là ; c'est le grand filtre.
- La boule q4 porte le MÊME gabarit BallKey à cinq coefficients que q3 ;
  le contrat causal en deux temps (forme brute au candidat, pgcd à la
  publication) est repris.

## Deux points à arbitrer

1. **Q12 (niveau q4)** : `R² = |N'|²/det²` a un numérateur `< 2^146`,
   hors i128. J'ai pris l'option (b) : représentant NON réduit
   `(U192, det²)`, égalité par champ, ordre à venir par produits croisés
   U320 — l'identité de boule reste portée par la BallKey primitive.
   L'option (a) (fraction pgcd-réduite) exigerait pgcd et division 192
   bits. Contester si la forêt exige (a).
2. **Lemme du préfixe ternaire** (`theoreme_v3` : tout q4 bien centré
   d'arête maximale ab a AU MOINS une face aiguë incidente à ab) : je
   l'invoque pour la complétude des seeds. Le juge C(n,4) le teste en
   creux (0 manquant sur 8,2 M sous-ensembles × 2 familles + fixture),
   mais une PREUVE v4 écrite manque encore au dossier — si l'un de vous
   veut la rédiger ou la réfuter, c'est le maillon le plus faible de la
   chaîne q4 actuelle.

## Suite proposée

(i) Oracle indépendant q4 (Cramer 4 points en OBig, mêmes standards que
q3 : fixtures u16 extrêmes, mutants) ; (ii) U320 et l'ordre mixte q3/q4 ;
(iii) sélection axiale § 4 reçue CONTRE cette baseline ; (iv) la forêt
(macro-lots compris). Sauf avis contraire, je commence par (i) — la
symétrie avec la discipline q3 l'impose avant toute optimisation.
