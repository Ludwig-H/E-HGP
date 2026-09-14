# Dialogue courant de l'auditeur indépendant B (v8)

14 septembre 2026, après **1bf806f0**, sur main. Second auditeur du dossier
`morsehgp3D_v8/audits/`, arrivé ce jour ; l'auditeur indépendant A conserve
[DIALOGUE_COURANT.md](DIALOGUE_COURANT.md) et ses notes P0_* ; l'auditeur
complémentaire conserve `audits/morsehgp3D_v8_complementaire/`. Écritures
limitées à ce dossier. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Ce qui est vérifié aujourd'hui

Lecture intégrale des Parties I–II du manuscrit (chapitres 2 à 9, Défs 20–31,
Th. 2–7, Prop. 5–9, Alg. 1), des six rapports d'ouverture, des cinq contrats
P0, des sources `src/`, des juges et des deux corpus d'audit. Construction
neuve `build/v8-audit-20260914` (GCC 13.3 Release) : **34/34 CTests en 29 s** ;
`build/v8-audit-san-20260914` (Clang 18, ASan/UBSan) : **34/34 en 101 s**,
aucun rapport de sanitizer. Ces deux builds ne touchent pas aux builds
épinglés du constructeur.

Relecture mathématique favorable, sans réserve trouvée, sur : les seuils
`h_q = Kmax + 2 − q` ; les fuseaux W3/W4 (Jung : circumrayon d'un support
aigu ≤ |ab|/√3, d'un tétraèdre à circumcentre intérieur ≤ |ab|·√(3/8), la
stricte inégalité étant nécessaire aux cas équilatéral et régulier) ; le
lemme des tubes et ses marges (`3H² − Ξ ≥ x²y²/8`, `2H² − Ξ ≥ 41x²y²/128`)
et sa réalisation entière (`d² ≥ 100·diam²`, égalité permise dans
`Q_C ≤ c·Δ²`, `Δ > 0` strict) ; les extrema `h_minimum`,
`h_maximum_times_four`, `Q2PreparedBounds` ; le parcours partagé à curseur
DFS ; les certificats `Credit`/`NoCredit`. Une contre-vérification
adversariale par agents (prédicats, tubes, filtre axial, census, robustesse,
mutants) est en cours ; ses constats survivants seront ajoutés ici avec
leurs entrées exactes, ou rien ne sera ajouté s'ils ne survivent pas.

## Mesure nouvelle : le régime réel des rectangles WSPD

[REGIME_WSPD_20260914.md](REGIME_WSPD_20260914.md) et son reçu rejouable
[wspd_regime_20260914/](wspd_regime_20260914/WSPD_REGIME_CHECKS.json)
mesurent le front WSPD pur v4 (même arbre radix, sans élimination) sur les
familles du plan de test. À `s=8` v4 : 3,4 M / 8,3 M / 19,6 M rectangles sur
l'uniforme 8k/16k/32k, 45 M à 64k, 219 M à 256k, soit environ +90 rectangles
par point à chaque doublement ; 75 à 94 % des rectangles ont deux facteurs
d'au plus sept sites ; aucun facteur n'atteint 1 024 sites (maximum 694,
terrain 32k). Les facteurs de 16 000 à 25 000 sites des fixtures P0
n'apparaissent dans aucune de ces familles.

Trois demandes en découlent, toutes compatibles avec la passation courante :

1. **Fixer une seule convention entière de séparation** et calibrer `s` sur
   elle : `gap ≥ s·diam` (v8) à `s=8` est encadrée par `d ≥ (s+2)R` (v4) à
   `s=16` et `s=14`, soit 54 à 68 millions de rectangles à 32k au lieu de
   19,6 millions sur le même arbre. Les mesures « s8/10/12 » n'ont de sens
   qu'après ce choix.
2. **Aucun coût fixe par rectangle** sur le chemin des petits facteurs : à
   50k, environ 33 millions de rectangles pour le seul front pur laissent un
   budget de l'ordre de 30 ns par rectangle en séquentiel équivalent. La
   boîte d'un facteur doit venir du nœud de l'arbre WSPD lui-même, pas d'une
   requête par rectangle sur un arbre d'intervalles ; le `PreparedCloud` en
   chantier est utile comme propriétaire unique de la copie validée, à
   condition que sa requête `bounds(range)` reste hors du chemin par
   rectangle du futur pilote.
3. **Publier, avec le pilote WSPD, les compteurs de ce reçu** (rectangles,
   Σ|A|+|B|, plus grand facteur, masse de paires par classe) aux séries
   8k→64k, puis la somme du census q2 sur tous les rectangles : c'est le
   seul juge possible du critère de clôture de P0 « travail total, aval
   compris ».

## Invariant à écrire dans le futur catalogue

L'élimination est **par lane**, la rétention est **par boule**. Une lane de
support q ne présente une boule qu'avec `q ≥ q_min`, donc `h_q ≤ h_{q_min}` :
chaque élimination de lane est sûre. Une boule vivante (`p < h_{q_min}`) est
produite par la lane `q_min` avec son seuil exact. Le catalogue doit donc
conserver une clé exacte dès qu'**une** lane la conserve ; rejeter une clé
parce que la lane q3 l'a éliminée alors que la lane q2 la garde serait faux
(triangle aigu dont la circumsphère porte aussi une paire diamétrale, à
`p = Kmax − 1`). Corollaire pour q3/q4 : le fuseau repose sur Jung avec
`L = |ab|`, donc chaque support doit être engendré depuis sa **plus longue
arête**, avec règle de départage des égalités écrite ; ce point n'est pas
encore documenté côté v8.

## Entretien du dossier

Aucun fichier des autres auditeurs ni du constructeur n'est modifié. Les
candidats d'archivage seront proposés ici avant tout déplacement, avec la
liste de leurs références entrantes. Contrôles : les Markdown de ce dossier
hors périmètre canonique sont validés explicitement par la fonction
`validate` de `tools/check_docs.py` ; reçu rejoué en `python3 -O`.
