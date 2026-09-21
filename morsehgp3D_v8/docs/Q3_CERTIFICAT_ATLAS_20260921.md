# Graines q3 certifiées par l'atlas de centres q4 (21 septembre 2026)

Cadre : `exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`. Tranche de
phase 1 du plan de reprise
([audit](AUDIT_REPRISE_DEVELOPPEUR_20260921.md) § plan). GCP non utilisé.

## Ce qui coûtait

Sur la scène 0 sans sol (39 815 sites, K5, un worker), le profil gprof attribue
29 % du temps au census q3 : 179,7 M graines construites et recensées depuis la
racine de l'index global pour 663 443 boules acceptées (99,6 % rejetées par
profondeur). Sur la même arête, l'atlas q4 avait déjà certifié, cellule par
cellule du plan bissecteur, un nombre de sites strictement intérieurs à toute
boule dont le centre est dans la cellule ; la voie q3 n'en profitait pas.

## Objets et convention

Pour une arête (a, b), v = b − a, D = |v|², m = (a + b)/2. Tout centre c d'une
boule passant par a et b est dans le plan bissecteur P de ab. L'atlas
paramètre P par deux vecteurs entiers A et B orthogonaux à v (`a_basis`,
`b_basis`) et des coordonnées entières (α, β) à l'échelle Q = 2^20 (cellule
racine [−2Q, 2Q]²). La forme d'un site z, `form(z) = (c0, fx, fy)` avec
w = 2z − 2m, c0 = |w|² − D, fx = −2 w·A, fy = −2 w·B, vérifie, pour le centre
c = m + (α A + β B) / (2Q) : Q·c0 + fx·α + fy·β = 4Q·puissance(z ; c). La
convention est donc **coordonnées de cellule = 2Q fois les coefficients réels
sur (A, B)** ; le disque du domaine (`outside`) |αA + βB|² ≤ D Q²/2 est le
disque réel |c − m| ≤ |ab|/(2√2), atteint par le tétraèdre régulier.

## Lemme du certificat

Pour une cellule fermée C et son fragment, `inside_count(C)` est le nombre de
sites de la couverture certifiés strictement intérieurs à B(c) pour tout
centre c ∈ C (blocs de l'index entièrement intérieurs, jamais saturé). Si le
circumcentre c_x d'une graine q3 (a, b, x) appartient à C, alors la boule
(a, b, x) contient strictement au moins `inside_count(C)` sites, car sa boule
est B(c_x) et la couverture (boule fermée de centre m et rayon |ab|) contient
tout site intérieur à une boule propriétaire de (a, b) (rayon ≤ |ab|/√3, centre
à moins de |ab|/(2√3) de m). Donc `inside_count(C) ≥ K − 1` rejette la graine
exactement, sans construction de boule ni census ; sinon rien n'est transmis
(aucun crédit) et le census complet est payé comme avant. Le certificat vaut
sur la cellule fermée : un centre sur une frontière peut être localisé dans
n'importe laquelle des cellules voisines. Une cellule `Outside` (hors domaine
q4, possible pour un centre q3) ne certifie rien. Le certificat de la cellule
racine s'applique à toute la voie q3 de l'arête (`root_lane_skips`).

## Centre q3 en coordonnées de cellule

Sur la droite L_x(u) = c0 + fx u1 + fy u2 = 0 des centres de sphères passant
par a, b, x (coordonnées non mises à l'échelle u = (α, β)/Q), le circumcentre
est le point de rayon minimal ; comme |c − a|² = |c − m|² + D/4 pour c ∈ P, c'est
le point de la droite le plus proche de m dans la **métrique réelle** de la
base (A, B) non orthogonale, de Gram (aa, ab, bb). Le gradient de |c − m|² y est
parallèle à (fx, fy) : seconde droite u1 (aa fy − ab fx) + u2 (ab fy − bb fx) = 0.
Par Cramer, avec p = aa fy − ab fx, q = ab fy − bb fx et det = fx q − fy p :
u1 = −c0 q / det, u2 = c0 p / det. La forme det = −(bb fx² − 2 ab fx fy + aa fy²)
est définie négative (base indépendante) : det = 0 ⟺ fx = fy = 0 ⟺ x sur la
droite ab, jamais une graine strictement aiguë. Bornes (M = 65 535) :
|c0| < 2^36, |fx|, |fy| < 2^35, Gram < 2^33, d'où |p|, |q| < 2^69,
|det| < 2^105, numérateurs < 2^105 ; le test d'appartenance Q·|x| < 2^125 et
2Q·den < 2^126 reste sous 2^127 : tout tient en i128, sans division.
Vérification indépendante : sur 199 graines de nuages aléatoires, le centre
reconstruit m + (u1 A + u2 B)/2 coïncide exactement (rationnels) avec −B/(2A)
de la boule exacte ; la comparaison sans le facteur 1/2 échoue sur les 199,
ce qui fixe la convention ci-dessus.

## Implémentation et mesure

`Q4LocalGeometry::q3_center`, `Q4LocalAtlas::certified_inside_count` (descente
de la quadtree construite, cellules fermées) et
`root_certified_inside_count` ; `WspdQ34Options::q3_atlas_consultation`
(explicite, faux par défaut pour garder l'historique bit à bit ; jeton CLI
`atlas`, schéma de sonde v5 avec le registre `q3_atlas`). Quand les deux voies
sont actives sur Local28 (K ≥ 3), l'atlas est construit avant la voie q3 puis
réutilisé tel quel par le balayage q4 (`run_q4_local_edge_candidates` à atlas
fourni). Sorties bit-identiques à la référence :

| entrée | graines q3 | rejets par atlas | requêtes de census (avant → après) | mur W1 (référence → 2^20 → +atlas) |
| --- | ---: | ---: | --- | --- |
| quart sans sol x+y+ (7 067 sites, K5) | 11 599 818 | 9 557 306 (90,7 %) | 11 599 818 → 2 042 512 | 48,5 s → 39,9 s → 27,3 s |
| préfixe 8k avec sol (K5) | 1 911 457 | 1 223 781 (73,8 %) | 1 911 457 → 687 676 | 11,1 s → 8,4 s |

Hôte partagé, une répétition : diagnostic, pas un reçu. Le poste dominant
devient la partition de l'atlas (125,6 M tests de blocs, 400,8 M tests de
points sur le quart), inchangée par cette tranche.

## Portes

`mhgp8_wspd_q34_gate` : consultation activée sur les configurations `boxes`
Local28 avec les options d'atlas réelles et `leaf_sites = 2` (les fixtures de
6 à 30 points ne dépasseraient jamais la population de feuille par défaut),
comparaison à l'oracle rationnel inchangée, identités de registre
`q3_blocks.queries + rejets = graines`, `ball_builds + rejets = graines`,
planchers : rejets > 0, localisations non rejetées > 0, centres hors domaine
> 0 (5 468 rejets sur 11 712 localisations). Mutant compilé
`q3_atlas_rejects_at_k_minus_2` (seuil K − 2) tué par l'oracle dans
`tests/wspd_q34_mutations.py` (quatre mutants, ancres du mutant historique
réalignées, schéma v2).
