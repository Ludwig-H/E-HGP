# Note Claude — conception fondatrice de MorseHGP3D v6 (31 août 2026)

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Pin v5 de référence : `3bad233d` (digests gravés dans
`receipts/conformite_v5/`, binaire sha256 `945c9a7f…`).

Cette note fixe l'architecture de la v6 et ses raisons. Elle ne promeut aucun
statut ; chaque certificat neuf naît `derive_claude` et doit passer oracle +
mutants avant requalification. Elle répond à la commande : repartir de zéro,
viser un coût sous-quadratique **par compteurs** sur les régimes annoncés,
fortement parallélisable et GPU-isable, en repensant d'abord les lanes q3/q4.

## 1. Ce que la relecture complète a établi avant de concevoir

1. **La barrière de sortie est un théorème.** La construction liée
   d'Edelsbrunner–Pach donne `Θ(n²)` boules critiques q3 et q4 de profondeur
   zéro ; la contre-fixture entière `linked_arcs_u16` (N = 6/10/18/34 →
   q3 12/40/144/544, q4 4/16/64/256) la transporte au profil u16. Aucune
   garantie « sous-quadratique pour toute entrée » n'est recevable pour un
   catalogue explicite. **Le contrat v6 est sortie-sensible** : préparation
   quasi linéaire + termes proportionnels aux sorties et incidences payées,
   chaque terme publié au grand-livre avec sa pente propre.
2. **Le nombre de rectangles WSPD n'a jamais été le problème.** Les cinq coûts
   quadratiques v5 sont : auto-produits `corner_histograms`, ancres développées
   malgré leurs crédits, covers re-scannés par ancre (`H_scan`), produit
   seed×cover (q3), scan cœur+corde par seed puis produit C×D (q4). Le mur
   mesuré de la lane q4 est le **scan de cœur par seed** (48–75 % du travail
   compté), pas les complétions (3,3–10,5 %).
3. **Les familles `terrain`/`scanline` du dépôt sont dilatées** (hauteurs
   ∝ `coord` ∝ `sqrt(n)` à espacement sol constant) : la super-quadraticité y
   est imputable à la famille (gel des deux échelles ⟹ q4 linéaire, exposants
   dans `[1,003 ; 1,014]` sur trois graines). Toute pente de lane opposable se
   mesure sur des familles **stationnaires**, à coder en première classe.
4. **Le front reste la WSPD binaire radix-Morton** : seule source complète et
   exact-once des paires qui survit aux pistes fermées (ternaire symétrique
   fermée par le théorème cercle–axe, kNN à préfixe borné fermé par fixture,
   cap terminal interdit, deux arbres interdits). La borne `R = O(n)` de la
   variante implémentée reste **ouverte** (programme SepQ/quotient octree de
   l'auditeur, hors chemin critique).

## 2. Méthode de conception

Cinq architectures candidates ont été développées indépendamment puis
soumises à trois contre-lectures adverses (complétude/exactitude, honnêteté de
complexité, réalisme d'échelle) :

- **D1** « meilleur-de-v5 consolidé » : descente fusionnée, requêtes de
  facteurs saturées, crédits composés typés, **sweep de corde unifié** ;
- **D2** « jointure duale de blocs » : blocs d'ancres × blocs de témoins,
  classification ALL/NONE/MIXED par tri-convexité séparée ;
- **D3** « espace des centres » : profondeur = rang dans l'arrangement des
  droites du plan bissecteur ; q4 = sommets peu profonds ; Tier R (grille 3D
  de centres par rectangle) ;
- **D4** « masque profond » : table globale de profondeur par cellules Morton
  (les h plus petites `dmax²`), tueur O(1) amont ;
- **D5** « GPU-native » : dataflow en tuiles, requêtes saturées, profil de
  corde, chiffrage du wire à 10 M.

Verdicts croisés des contre-lectures (conservés dans les archives de session,
reproductibles) :

- la seule attaque sur l'**exposant** du mur mesuré est D3 (niveaux ≤ h de
  l'arrangement : le coût par ancre lourde passe de `seeds×m_e` à
  `m_e·(log m_e + h)`, théorème externe Alon–Győri / Everett–Robert–van
  Kreveld) — mais l'adaptation aux arrangements **non simples** (bundles u16,
  concurrences) est ouverte (O1), exactement la cause de la fermeture v3 du
  parcours d'arrangement global ; risque de mise au point maximal ;
- le squelette au moindre risque avec conformité précoce est D1 ; son apport
  décisif : **chaque complétion D est une racine du sweep de corde**
  (`μ_d = P(d)/B(d)`), donc une passe triée par seed survivant remplace le
  produit C×D **et** le filtre de profondeur par candidat ; et la **frontière
  canonique des digests descend au post-préfiltre exact** (tue le pattern
  « un digest qui mesure un filtre ») ;
- D2 apporte un vrai théorème (ALL exact aux ≤ 512 coins triples par
  convexités séparées) mais son certificat de cellule de bloc (B3) est troué
  côté fausses morts (max de deux concaves aux sommets) : non recevable en
  l'état, la version saine est le certificat unilatéral OU (celui de D3) ;
- D4 apporte la discipline : **sonde contrefactuelle appariée avant tout
  raccord** ; son tueur est toutefois aveugle exactement sur scanline (pas de
  témoins) ;
- D5 chiffre seul le wire à l'échelle (430 Go de candidats, ~1,7 To d'E/S de
  tri à 10 M sur 290 Mio/s : le wire du flux est un P0 d'échelle) mais son E3
  double-compte les crédits (fausse mort structurelle) et son profil de corde
  omet la règle des incidents.

## 3. Décision d'architecture

**v6 = squelette D1 corrigé, discipline D4, options D3 conditionnelles.**

```text
E0  index : tri Morton 48 bits → buckets uniques → arbre radix (Karras),
    boîtes serrées + cube de packing Q exposé ; ledger global des paires
E1  UNE descente WSPD fusionnée à masques de lanes (q2/q3/q4),
    rectangles vivants MultiAliveRect{rect, mask, core[3]}
E2  fermeture des facteurs : route S (facteurs minuscules : produit direct,
    le cas massivement majoritaire à s=8) ; route M (requêtes one-sided
    ALL/NONE/MIXED saturées au seuil + range-add) au-dessus d'un seuil figé
E3  par ancre survivante : AnchorCredit typé (unique opération compose),
    ResidualTape à rôles (témoin / support) et masques de lane ;
    tueurs d'ancre en coût croissant : W_q exact saturé, secteurs (forme
    corrigée max-par-secteur-avant-min), grille de cellules 10.5
E4  lanes :
    q2 : émission directe (profondeur au census)
    q3 : seeds aigus + filtre de profondeur à la génération (scan saturé)
    q4 : SWEEP DE CORDE UNIFIÉ par seed — passe 1 O(m_e) = scan cœur saturé
         (identique v5, sortie anticipée) ; passe 2 O(m_e log m_e) pour les
         seuls seeds survivants : racines μ_z = P/B triées, minimum exact sur
         corde, fragments shallow, et CHAQUE complétion lue à sa racine
         μ_d = P(d)/B(d) — le produit C×D et la profondeur par candidat
         n'existent plus
E5  tri stable + RLE par BallKey → préfiltre exact count-only
    (ball_depth_at_least, arrêt au seuil) → census I_B/U_B complet
    (shell_cap ≤ 12 sinon resource_exhausted) → événements (plateaux par
    quotient exact) → fold streamé par K (fold_inflight borné) → rendu
E6  (conditionnel, post-conformité, sur pentes stationnaires mesurées) :
    Tier R (grille 3D de centres par rectangle, certificat unilatéral i64,
    forme device idéale) puis moteur plan par ancre lourde (niveaux ≤ h)
```

La frontière canonique de conformité v5↔v6 est **l'objet** : `digest_all` et
les dix `digest_forest_K*` (gravés au pin `3bad233d` sur les cinq familles ×
{8000, 16000, 32000}). `digest_balls` v6 est une nouvelle base, prise **après
le préfiltre exact** (l'ensemble des BallKey survivantes au préfiltre ne
dépend que de l'objet, jamais de la force des tueurs de génération) ; les
tueurs deviennent libres de leur force sans casser la porte.

## 4. Corrections imposées par les contre-lectures (dettes de conception)

Ces quatre points sont des trous identifiés dans D1/D5 et sont traités comme
des contraintes de conception, chacun avec fixture dédiée :

1. **Complétude des rôles dans A∪B** : un membre de A (≠ a) peut être seed ou
   complétion d'un support valide ; la disjonction § 4.7 ne l'exclut que du
   rôle *témoin universel*. Le `ResidualTape` porte donc les sites de A∪B avec
   rôle support actif et rôle témoin masqué. Fixture : membre de A complétion
   valide ; membre de A seed valide.
2. **Alignement des exclusions par lane** : `U_core` est recertifié par lane ;
   les exclusions du tape sont par lane (bits par record), jamais partagées
   implicitement entre q3 et q4. Fixture croisée de lanes (site W3-pas-W4).
3. **Règle des incidents du sweep** : au point de racine, retirer d'abord
   toutes les sorties, compter les incidents à zéro (coquille), ajouter les
   entrées après ; racines confondues traitées en bloc ; clip à ±μ* **non
   strict** (égalité 2P² = J·B² admissible, borne de Jung). Fixtures :
   racines confondues, complétion exactement incidente, racine à une
   extrémité en cas d'égalité exacte.
4. **Branche de repli des crédits** : le sweep ne consomme jamais
   `E_ab + h_core` directement ; toute composition passe par l'unique
   `AnchorCredit::compose(residual) = min(h_q, E_ab + r_core + max(h_core − r_core, residual))`,
   V1 nominale `r_core == h_core` sinon repli sans exclusion d'indice.
   Mutant `credit-compose-sum`.

## 5. Grand-livre et portes go/no-go (contrat de mesure)

Chaque run publie : `R, V_wspd, V_R, C_R, P_R, H_rect, H_scan, M_anchor,
distribution des m_e, W_sweep (scindé passe 1 / passe 2), fragments,
seeds avant/après chaque tueur, Q_try, C_emit, B, S_shell, V_census,
S_forest`, temps exclusifs, HWM. Pentes sécantes aux deux pas
8000→16000→32000, trois graines minimum, par terme — jamais une somme, jamais
un ajustement global. **Méta-clause : un terme payé omis du grand-livre est un
NO-GO en soi.**

Ce qui a le droit de rester quadratique, et où : `B` et tout terme ∝ sortie
sur `linked_arcs_u16` (contrat) ; `M_anchor`/`H_scan` sur la contre-famille
calotte–lentille (à graver en fixture bornée u16) ; `W_sweep` sur les familles
dilatées (stress non extrapolable). Les décisions GO se prennent sur
`uniform`, `eight_clusters`, `terrain_stationnaire`, `scanline_stationnaire`.

Déclencheur de l'étage E6 : si sur `scanline_stationnaire` la pente de
`M_anchor` ou de `W_sweep` reste ≥ 2 (trois graines), le Tier R et le moteur
plan deviennent le chantier prioritaire, précédés de leur sonde
contrefactuelle appariée (discipline D4) ; s'ils échouent aussi, le régime est
publié comme mur documenté, jamais comme succès partiel.

## 6. Familles stationnaires (régimes de coût de première classe)

Spécification : constantes de la loi dilatée évaluées à la taille de
référence `n0 = 8000`, **nombre de motifs ∝ aire**, seule la fenêtre croît.

- `terrain_stationnaire(n, seed)` : `coord = sqrt(25·n)` (densité aréale
  1/25 inchangée) ; `c0 = sqrt(25·8000)` arrondi = 447 ; nombre de bosses
  `round(6·n/8000)` ; rayon ∈ [c0/6, c0/3] = [74, 149] ; amplitude
  ∈ [c0/16, c0/8] = [27, 55] ; canopée 1/50, lift ∈ [1, 55] ; jitter {0,1,2}.
- `scanline_stationnaire(n, seed)` : `coord = sqrt(40·n)` ; `c0 = 565` ;
  calottes `round(5·n/8000)` (rayon [94, 188], amp [35, 70]) ; plateaux
  `round(4·n/8000)` (côté [47, 113], hauteur [35, 70]) ; capteur inchangé
  (pas 2 le long, pitch 8, lignes clairsemées 1/3, trous markoviens,
  échos 1/8 avec lift ∈ [2, 56]).

À `n0 = 8000` les lois coïncident avec les familles dilatées (mêmes bornes de
distribution) ; au-delà, seule la fenêtre grandit. Le piège mesuré du « gel »
v5 (aplatissement relatif, exposants sous-linéaires artificiels) est évité :
les motifs restent de taille absolue fixe et leur densité surfacique est
constante. Les familles dilatées v5 sont conservées bit à bit (digests de
conformité) comme stress non extrapolables.

## 7. Parallélisme et GPU (doctrine héritée, formes prévues)

CPU : brouillons par ouvrier, fusion en ordre d'ouvrier, tri stable + RLE
canonisent — sortie bit-identique quel que soit le nombre de fils, ouvriers
mesurés. GPU subordonné aux reçus G4 (jamais de copie par site, géométrie et
index résidents, wires u32, tuile = unité de travail, pool persistant) ; les
formes retenues sont device-amicales par construction (passe 1 du sweep =
warp par seed, hérité du kernel de cœur v5 ; Tier R = un bloc par rectangle,
compteurs 512 u8 en mémoire partagée, i64 pur) mais **aucun étage device
n'est engagé sans reçu de gain sur G4** (plafond d'Amdahl 1,10× gravé sur
uniform 50 k ; seul le poste rects de scanline justifie le device).

## 8. Jalons

- **J0** : squelette du dépôt, docs fondateurs, cœur arithmétique
  (types/morton/intmath/wide/dint/mutants/sha256/parse), familles (port
  contractuel v5 + stationnaires), index, WSPD, fuseaux, descente fusionnée,
  ledger global, portes selftest + familles + équivariance.
- **J1** : facteurs (routes S/M), crédits typés, tape à rôles, tueurs d'ancre
  (W_q, secteurs corrigés, grille 10.5), fixtures et mutants associés.
- **J2** : lanes q2/q3/q4 avec sweep de corde unifié (oracle du sweep
  d'abord), RLE, préfiltre, census, événements, fold streamé, rendu, digest —
  **pipeline complet + campagne de conformité v5↔v6** (forêts, 5 familles ×
  3 tailles).
- **J3** : familles stationnaires en campagne (pentes par terme, 3 graines) ;
  décision E6.
- **J4** : Tier R en sonde contrefactuelle, moteur plan si justifié.
- **J5** : parallèle 48 fils mesuré ; G4 seulement ensuite, reçus à l'appui.

Aucun jalon n'ouvre le suivant sans ses mutants tués, ses planchers et ses
pentes publiées. Aucun claim de sous-quadraticité globale n'est jamais émis :
le contrat est le grand-livre.

## 9. Questions aux auditeurs

- **V6-Q1.** La frontière de digest post-préfiltre exact comme monnaie
  canonique v6 (au lieu du multiensemble de candidats) vous convient-elle
  comme contrat de conformité internes v6↔v6, la conformité v5↔v6 restant
  sur `digest_all`/`digest_forest_K*` ?
- **V6-Q2.** Le certificat C1 du sweep (chaque complétion est une racine,
  règle de bloc aux incidents) et sa composition avec les crédits (§ 4.4)
  vous semblent-ils recevables sur preuve écrite avant implémentation, ou
  voulez-vous l'oracle exécutable d'abord ?
- **V6-Q3.** Pour `scanline_stationnaire`, la spécification § 6 (motifs de
  taille absolue fixe, densité surfacique constante, capteur inchangé)
  répond-elle à votre objection contre le « gel » (aplatissement relatif) ?
- **V6-Q4.** La calotte–lentille : acceptez-vous de la graver comme
  contre-fixture bornée u16 avec les marges de votre stratégie du 30 août,
  ou souhaitez-vous fournir la réalisation vous-mêmes ?
