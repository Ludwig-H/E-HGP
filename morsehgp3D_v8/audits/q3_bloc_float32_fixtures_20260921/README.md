# Fixtures exactes pour les bornes de bloc q3 de la voie float32 (enveloppe de centres, bornes de puissance)

Auditeur B, 21 septembre 2026. Réponse exécutable à la question A/B du journal du
constructeur (« enveloppe de centres conditionnelle aux graines q3 positives :
conv(a,b,X), resserré par W/(2G) quand G est certifié positif, puis puissance par
(z − a)·(z + a − 2c) ») et relecture des sources non suivies
`src/core/float32_q3_block.*` et `src/lanes/float32_q3_census.*` (lues, jamais
compilées ici ; sans reçu ni test à cette date). Tout est calculé en
`fractions.Fraction`, sans flottant ni moteur :
[block_envelope_fixtures.py](block_envelope_fixtures.py) `run` écrit
[BLOCK_ENVELOPE_FIXTURES.json](BLOCK_ENVELOPE_FIXTURES.json) (valeurs attendues en
chaînes exactes), `read` recalcule tout et exige l'égalité ; aucun `assert`.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Ce sont des **fixtures d'égalité** pour la porte de bloc à
venir, pas une mesure ni une qualification. F9 (ajoutée après la question A/B
du journal sur le port global « propriété dans X ») fixe la population des
témoins : tout l'index, jamais les seules graines propriétaires ou valides.

## Notations

d = b − a, u = x − a, D = |d|², E = |u|², F = d·u, G = DE − F² = |d × u|²,
W = E(D − F)d + D(E − F)u, centre c = a + W/(2G) = m + ξ·h(x) avec m = (a + b)/2,
h(x) = x − a − (F/D)d et ξ = D(E − F)/(2G) ; coordonnées barycentriques du centre
α = F|x − b|²/(2G), β = E(D − F)/(2G), ξ ; notations de A : J = 4G, Q = 4(E − F),
P = 2(Du − Fd), λ = DQ/J = 2ξ. Trois schémas d'enveloppe de centres sont
reproduits sans arrondi : « constructeur » = hull(a,b,X) ∩ (a + W/(2G)) par
intervalles corrélés, avec repli sur le hull si G n'est pas certifié positif ou
si l'intersection est vide ; « A universelle » = m + [0, λ_max]·P(X)/(4D) avec
λ_max = 2/3 sous propriété d'arête et 1 sans ; « A resserrée » = la même avec
λ borné par D·Q/J sur la boîte. Les bornes de puissance sont, par axe et par
extrémité de C_i, minimum au sommet borné z = clamp(c, Z_i) et maximum aux
extrémités de Z_i (schéma du code) ; les bornes « coins seulement » évaluent les
64 couples (coin de Z, coin de C).

## Fixtures et valeurs attendues

| nom | entrée | attendu (exact) |
| --- | --- | --- |
| F1 minimum au sommet | a = (0,0,0), b = (10,0,0), x = (5,6,0), Z = [0,10] × {0} × {0} | centre (5, 11/12, 0) ; coins seulement [0, 0] ; sommet borné [−25, 0] ; z = (5,0,0) a la puissance −25 (témoin strict que « coins seulement » nie) |
| F2 axiale (propriété vraie) | a = (20,20,20), b = (40,20,20), X = {30} × [32,37] × {20}, Z = {(30,31,20)} | vraie plage [−1722/17, −58/3] ; constructeur [−6791/16, 29261/289] indécis ; A universelle [−311/3, 21] indécis ; A resserrée [−1722/17, −58/3] = vraie plage, intérieur strict ; largeur en y : hull 17, constructeur 94725/9248, A universelle 17/3, A resserrée 190/51 = vrai hull |
| F2 bis, voie sans propriété (λ ≤ 1) | même seed, même Z | A universelle c_y ∈ [20, 57/2], bornes [−166, 21] indécises ; A resserrée inchangée, décidée |
| F3 tournée | rotation ((2,−2,1),(1,2,2),(−2,−1,2)) + 500 : a = (520,600,480), b = (560,620,440), X = [506,516] × [634,644] × [443,448] (726 graines valides), quatre boîtes Z de 27 points | A resserrée : intérieur, extérieur, intérieur, indécis (fidèle à la vraie plage) ; constructeur indécis sur les quatre ; toutes les enveloppes contiennent le vrai hull |
| F4 λ sans propriété | (0,0,0), (4,0,0), (2,10,0) ; (0,60,60), (2,60,60), (1,9,8) ; bissectrice x = (1, 10⁴, 0) ; équilatéral (30,30,30), (36,36,30), (36,30,36) | λ = 24/25 ; 5304/5305 ; 1 − 10⁻⁸ ; 2/3 (propriété vraie, barycentriques 1/3) : la borne 2/3 exige la propriété d'arête, sup λ = 1 sinon |
| F5 G ambigu | a, b de F2, X = {30} × [y_lo, 40] × {20}, y_lo ∈ {36, 30, 21, 20} | statuts constructeur `tightened`, `tightened`, `tightened`, `gram_unresolved_hull` ; à y_lo = 30 déjà c_y = [20, 40] = hull ; A universelle c_x = {30}, c_y = [20, 30] dans les quatre cas |
| F6 témoins mutuels | a = (0,0,0), b = (4,0,0), x₁ = (2,3,0), x₂ = (2,20,0), toutes deux aiguës | puissance de x₁ dans la boule (a,b,x₂) = −272/5 ; dans sa propre boule 0 ; de x₂ dans la boule (a,b,x₁) = 1088/3 : une feuille de X dans Z n'est jamais décidable pour le bloc |
| F7 barycentriques | équilatéral ; rectangle en x (0,0,0), (10,0,0), (2,4,0) ; rectangle en a ; obtus en x (5,2,0) ; obtus en b (12,3,0) | (1/3, 1/3, 1/3) ; ξ = 0, centre sur ab ; α = 0 ; ξ = −21/8, centre hors de hull(a,b,x) ; β = −17/10 ; acuité ⟺ F, D − F, E − F > 0, somme 1 |
| F8 identités | 300 triangles entiers non colinéaires, graine 21, coordonnées dans [0, 200) | J = 4G, Q = 4(E − F), P = 2(Du − Fd), c = m + ξ·h(x), (c − m)·d = 0, λ = 2ξ, somme des barycentriques 1 |
| F9 population des témoins | a = (0,0,0), b = (10,0,0), x = (5,8,0) (aigu, propriétaire : centre (5, 39/16, 0), r² = 7921/256) ; z₁ = (9,5,0) ; z₂ = (5,1,0) | z₁ est aigu mais non propriétaire (arête la plus longue az, 106 > 100) et sa puissance vaut −67/8 : témoin strict ; z₂ est obtus (graine invalide) et sa puissance vaut −231/8 : témoin strict. Un port qui restreindrait Z aux graines propriétaires ou valides de X sous-compterait la profondeur de 2 |
| F10 égalité de propriété | a = (0,0,0), b = (10,0,0), x = (6,8,0) | |ab|² = |ax|² = 100, |bx|² = 80, aigu : deux arêtes maximales ; propriétaire par ab au sens large (≤), pas au sens strict (<). Un port qui testerait la propriété strictement quelque part et largement ailleurs émettrait cette boule deux fois ou jamais ; la règle u16 (égalité départagée par la paire d'IDs triés) doit être la même dans la sélection de X et à l'émission |

Le README de A donne pour F2 une plage resserrée −486226/4800 ≈ −101,30 (ses
conditions entières u16 la rendent un peu plus lâche que la plage réelle) ;
la décision est la même.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/block_envelope_fixtures.py read
python3 -B -O morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/block_envelope_fixtures.py run --output /tmp/BLOCK_ENVELOPE_FIXTURES.json
```
