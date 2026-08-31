# Mathématiques v6 — objet, réduction, certificats

Document ouvert le 31 août 2026. Hérite de `morsehgp3D_v5/docs/MATHEMATIQUES.md`
au pin `3bad233d` ; chaque statut est re-déclaré ici. Légende des statuts :
`theoreme_manuscrit`, `theoreme_externe`, `recu_auditeur_v4`, `recu_v5`,
`derive_v5_non_recu`, `derive_v6`, `mesure`, `ouvert`. Un statut `derive_v6`
ne porte aucune autorité tant qu'oracle et mutants ne l'ont pas requalifié.

Conventions : `D² = ||b−a||²` pour l'ancre `(a,b)`, `m = (a+b)/2`, le niveau
est un **rayon au carré exact** (fraction non réduite), « intérieur » = strict,
« coquille » = sur la sphère.

## 1. L'objet (inchangé, normatif)

Identique à la v5 § 1 : complexe de Čech (Déf. 20), K-polyèdres (Déf. 21),
miniboule ρ(σ) (Déf. 25), position générale (Déf. 26), simplexes K-séparants
(Déf. 27), Gabriel (Déf. 28), K-graphe de Gabriel (Déf. 29), K-MST (Déf. 30,
Th. 5). Bijection événements↔boules (`recu_auditeur_v4`) : sous positions
distinctes, un événement de la forêt K est une boule de support S (arité
q = |S| ∈ {2,3,4}, centre intérieur relatif à conv(S)) et d'intérieur
|I_B| = K+1−q. Seuils de mort par lane : `h_q = smax − q + 1` = **10/9/8** à
smax = 11. Census : I_B strict, U_B coquille **complète** (plafond
`shell_cap ≤ 12` sinon `resource_exhausted`).

Réserve d'objet (v5 § 1.2, inchangée) : ce que la ligne rend est le flot de
Gabriel des événements vérifiés (`forest_semantics=verified_events_only`),
portée horizontale ; le contrat Gamma (incidences silencieuses, fixture
`gabriel-point-set-counterexample-5-points-v1`) reste un P0 documentaire
séparé ; aucune sortie ne porte `require_exact=true`.

## 2. Réduction par ancre (inchangée, reçue)

- Ancre = arête maximale canonique (départage EdgeKey). Jung
  (`theoreme_externe`) : R ≤ D/2, D/√3, D·sqrt(3/8) selon la lane. Lentille :
  sommets dans B̄(a,D) ∩ B̄(b,D). Cover coefficient 3 :
  `||2z−a−b||² ≤ 3D²` (q3 sharp ; minorant q4).
- Fuseaux de mort (`recu_auditeur_v4`) : avec `w = z−a`, `d = b−a`,
  `H = d·w − |w|²`, `Ξ = |d×w|²` : W2 : `H > 0` (exact, boule diamétrale
  ouverte) ; W3 : `H > 0 ∧ 3H² > Ξ` ; W4 : `H > 0 ∧ 2H² > Ξ` ;
  `W4 ⊂ W3 ⊂ W2` ; z ∈ W_q ⟹ z tue tout support d'arité q de l'ancre
  (fail-open, réciproque fausse en q3/q4).
- q3 : support ssi triangle strictement aigu ; forme de Gram
  `G = DE − F² > 0`, puissance `P(z) = G|v|² − v·W` (i128), niveau
  `D·E·X/(4G)`.
- q4 : support ssi centre strictement intérieur (Cramer, `det > 0`) ; lemme du
  préfixe ternaire (`recu_auditeur_v4`) : au moins une face `abv` strictement
  aiguë ⟹ source seed aigu + complétion, exact-once par plus petit PointId
  des faces incidentes aiguës. Niveau `|N'|²/det²` (U192/i128, non réduits),
  comparaisons U320.
- Théorème de disjonction et corollaire fail-open (`recu_auditeur_v4`) :
  h_coeur (hors A∪B, universel rectangle), h_a(a) (dans A, universel
  {a}×B), h_b(b) (dans B, universel A×{b}) sont deux à deux disjoints par
  identité ; `|X ∩ W_q(a,b)| ≥ h_coeur + h_a(a) + h_b(b)`.
- Théorèmes 10.3 (secteurs), 10.4 (morceaux de corde : centres du seed q4 sur
  une corde `c_μ = m + v₃ + μ·n/(2G)`, `|μ| ≤ μ* = sqrt(J/2)`,
  `J = D²(3G − 2·l_ax·l_bx) ≥ G·D²/3 > 0`, intériorité affine
  `P(z) − μ·B(z) < 0` avec `B(z) = n·(z−a)`), 10.5 (grille de cellules du
  plan bissecteur) : `recu_v5` (fixtures et mutants v5, re-requalifiés en v6
  par leurs portes).

## 3. Certificats v6 (le neuf)

### C1 — Localisation des complétions sur la corde (`derive_v6`)

Énoncé : pour une ancre `(a,b)`, un seed aigu `x` (Gram G > 0, normale
`n = (b−a)×(x−a)`, J > 0) et tout site `d` non coplanaire avec (a,b,x)
(`B(d) = n·(d−a) ≠ 0`), la sphère circonscrite à (a,b,x,d) a son centre sur la
corde de x à l'abscisse `μ_d = P(d)/B(d)`.

Esquisse : le centre est équidistant de a, b, x, donc sur la droite des
centres de la face, paramétrée par μ ; d est sur la sphère ⟺
`P(d) − μ·B(d) = 0` (identité d'intériorité affine du Th. 10.4) ⟺
`μ = P(d)/B(d)`. `B(d) = 0 ⟺ d` coplanaire ⟺ `det = 0`, jamais un support q4.

Conséquence (bornée par l'audit du 31 août) : le **rescan de profondeur par
candidat** est remplacé par une lecture du balayage des racines triées ;
l'incidence seed–complétion reste matérialisée (une racine par site éligible).
Le coût par seed survivant passe de `O(p_e · m_e)` à `O(m_e log m_e + p_e)`,
où `p_e` compte les complétions soumises à la cascade (`q4_completions`).

Largeurs : `|P| < 2^101` (i128), `|B| < 2^55` ; ordre de deux racines par
produits croisés signés `P_i·B_j` vs `P_j·B_i` (< 2^157, S192 signe-magnitude
ou i256 par pont U192 + signes) ; appartenance à la corde par
`2P² ≤ J·B²` (**non strict** : l'égalité est la borne de Jung, admissible),
`2P² < 2^213`, `J·B² < 2^214` — comparateur U320.

### C2 — Profondeur au point de racine, règle de bloc (`derive_v6`)

Énoncé : la profondeur stricte de la boule de (a,b,x,d) parmi les sites du
tape vaut `#{z : P(z) − μ_d·B(z) < 0}`. Au point `μ_d`, le balayage applique
la règle de bloc : retirer d'abord toutes les sorties dont la racine vaut
`μ_d`, compter les incidents (`P − μ_d·B = 0`) à zéro (coquille, jamais
intérieurs), ajouter les entrées après. Les racines confondues sont traitées
en bloc. Le carrier x est coquille sur toute la corde (exclu, asserté).

### C3 — Contrat de profondeur du sweep (`derive_v6`, fail-open)

Deux contrats cohérents existent ; **la v6-J2 implémente le contrat 1** et ne
mélange jamais les deux (audit du 31 août : ne jamais additionner à une
profondeur complète un crédit dont les témoins y figurent déjà) :

1. **Contrat 1 (courant)** : le sweep balaie le cover complet ; le verdict est
   `depth_at(μ_d) ≥ h₄`, sans aucun crédit ajouté — les témoins des crédits
   figurent déjà dans ce compte.
2. **Contrat 2 (J3, avec `ResidualTape`)** : le tape exclut A∪B∪U_core par
   identité et par lane ; le verdict devient
   `compose(depth_residual_at(μ_d)) ≥ h₄` (l'unique opération C5). La formule
   sectorielle suit le même choix.

Dans les deux cas la mort est fail-open : les domaines comptés minorent
|I_B| ; réciproque fausse, jamais une émission garantie.

### C4 — Minimum sur corde et fragments shallow (`derive_v6`)

`h_c_q4_chord(x) = min_μ depth(μ)` sur la corde entière — c'est
`min_μ #{actifs à μ}`, jamais `#{actifs pour tout μ}` (les témoins se
relaient ; fixture `F1 = μ+1, F2 = 1−μ` : min 1, témoins communs 0). Le compte
constant c0 inclut `B = 0 ∧ P < 0` **et** les racines hors corde actives
partout (`B > 0 ∧ P/B < −μ*`, `B < 0 ∧ P/B > +μ*`) ; les cas opposés hors
corde ne témoignent jamais. ≥ h₄ ⟹ le rôle seed est fermé ; les fragments
shallow (sous-intervalles où depth + crédits < h₄) sont tous conservés — en
oublier un serait un faux rejet. `h_c` ferme un rôle, jamais l'identité : le
point reste témoin, complétion, membre du census.

### C5 — Crédit composé (`derive_v6`, formule de l'auditeur)

Unique opération :
`compose(residual) = min(h_q, E_ab + r_core + max(h_core − r_core, residual))`
avec `E_ab = h_a(a) + h_b(b)`. V1 nominale : `r_core == h_core` (indices upos
recertifiés hors A∪B, par lane) ; sinon repli `r_core = 0` sans exclusion
d'indice. `r_core = 0` redonne `E_ab + max(h_core, residual)` ;
`r_core = h_core` la somme entièrement disjointe. Garde d'underflow : tester
d'abord `E_ab + h_core ≥ h_q` en signé. `h_c` et le `base_C` d'une grille
minorent le **même** compte résiduel : `residual = max(h_c, base_C)`, jamais
la somme.

### C6 — Rôles du tape (`derive_v6`, contrainte de complétude)

Le `ResidualTape` d'une ancre porte, par record : indice upos, rôle témoin
(actif ssi hors A∪B∪U_core[lane]), rôle support (actif pour tout site de la
lentille, **y compris les membres de A∪B autres que a, b**), masques de lane.
Un membre de A (≠ a) n'est jamais témoin universel (H = 0 au choix a = z)
mais peut être seed ou complétion d'un support valide : l'exclure du rôle
support perdrait des événements. Fixtures : membre de A complétion valide ;
membre de A seed valide ; site U_core-q4 encore complétion valide.

### C7 — Requêtes de facteurs saturées ALL/NONE/MIXED (`derive_v6`, route M)

Pour l'endpoint s ∈ A, la boîte partenaire T = Box(B) et un nœud témoin
Z ⊆ A : ALL par les couples de coins distincts de T×Z est un certificat exact
sur l'enveloppe continue (convexité de W_q en z à (s,t) fixé ; cône convexe en
t à (s,z) fixé — autorité v4) ; NONE conservateur par
`M = hmax4(point_box(s), T, Z) ≤ 0` ou `β_q·M² ≤ 16·Ξ_lb` (β₃ = 3, β₄ = 2,
`Ξ_lb = Σ_j dist(0, I_j)²` sur les intervalles exacts par composante de
`d×(z−s)`) ; MIXED descend, feuille = autorité exacte
`universal_over_corners`. Crédit en unité **upos** (jamais `range_weight`),
saturé à `need = h_q − core ≤ 9` ; la feuille z = s n'est jamais ALL (H = 0).
La route M n'est engagée qu'au-dessus d'un seuil de taille de facteur figé ;
en dessous, produit direct (route S, le cas massivement majoritaire à s = 8).

## 4. Statuts ouverts (déclarés, jamais annoncés comme acquis)

- Borne `R = O(n)` de la WSPD radix-Morton implémentée (programme
  SepQ/quotient octree de l'auditeur) : `ouvert`, hors chemin critique.
- Constructeur du sous-complexe de faibles profondeurs (moteur plan, étage E6
  conditionnel) : `theoreme_externe` en position générale (Alon–Győri,
  Everett–Robert–van Kreveld) ; adaptation aux arrangements non simples u16
  (bundles, concurrences) : `ouvert` (O1). Tier R (grille 3D de centres par
  rectangle, certificat unilatéral) : `derive_v6`, sonde contrefactuelle
  obligatoire avant raccord.
- Écrêtage top-seuil du sweep (≤ 2·h₄ racines conservées) : exige un lemme
  d'ordre, `ouvert` — la V1 trie toutes les racines du seed survivant.
- Passage Poisson→WSPD (Q2 v5) : `ouvert`, inchangé.

## 5. Contrat de sortie (barrière)

La construction liée d'Edelsbrunner–Pach (Maximum Betti Numbers of Čech
Complexes, § 3.1, lemme 3.5) impose `Θ(n²)` clés q3 et q4 de profondeur zéro
dans le modèle exact ; la contre-fixture entière `linked_arcs_u16`
(N = 6/10/18/34 → q3 12/40/144/544 = 2n(n+1), q4 4/16/64/256 = n²) est une
porte permanente v6 (littéraux sans libm, oracle OBig indépendant, marges en
arithmétique large — la plus petite puissance q3 extérieure à N = 34 dépasse
INT64_MAX). Le contrat de coût est sortie-sensible : voir
`docs/GRAND_LIVRE.md`.
