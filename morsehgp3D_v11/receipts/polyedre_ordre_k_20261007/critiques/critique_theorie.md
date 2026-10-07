# Critique mathématique adverse des quatre notes « complexe alpha d'ordre k »

6 octobre 2026, de 23 h 29 à 23 h 55 UTC (heures lues par `date -u`). Rôle : critique mathématique adverse du
workflow « généraliser le complexe alpha à l'ordre K ». Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé. Aucun commit, aucune écriture hors de `build/`. Statuts : **prouvé** (preuve relue et jugée
complète), **vérifié borné** (calcul exact sur fixture, oracle de correction seulement), **mesuré** (sorties réelles
du moteur, lecture seule du cache), **conjecture**. Toute conclusion de taille porte la mention « non mesuré à
l'échelle » quand elle ne vient pas de n = 8 000, 16 000 ou 32 000.

## 0. En bref

1. **Auditeur : aucune réponse postérieure à `28d70f8ab`.** Après `git fetch`, les sept commits suivants sur
   `origin/main` sont des commits du développeur (`v11: …`, levier O2, exécuteur partagé, reçus G4). Le seul
   fichier modifié sous `morsehgp3D_v11/audits/` est `REPONSE_CLAUDE_SUPPORTS_20261004.md` (développeur, autre sujet).
   Aucun fichier `receipts/audit_*` n'a été ajouté.
2. **Deux réfutations exactes, une nuance chiffrée et une mesure contraire.**
   - **N3 réfuté** (`theorie_robustesse`, « arbre δ-contracté »). Trois sites alignés, k = 1, δ = 1 : la chaîne
     racine contractée est portée par le site 0 dans P et par le site 99 dans P'. La trace de la chaîne du site 0 est
     coupée, à l'intérieur de sa vie contractée, par une fusion entre deux nœuds de P' de vie 49/2 > 2δ. N1, énoncé
     pour les nœuds FULL, reste vrai : le défaut est dans le passage aux chaînes. RR3 n'est donc pas établi.
   - **P5 réfuté dans sa lecture** (`theorie_reduction`). Σ μ(G) est une borne inférieure valide, mais elle n'est
     pas atteignable en général. Avec cinq sites, k = 1, la borne compte un tétraèdre, alors que les 16
     appariements acycliques à sommets protégés en gardent tous deux. La fermeture par faces et partenaires force une
     cellule non critique. « Seuls les tétraèdres critiques restent » et « profil (68.6, 120.2, 79.0, 27.5) du
     représentant réduit » sont donc faux. Le « quart des cellules » sur LiDAR repose sur cette borne.
   - **Certificat de κ (G2–G3) : sain mais inutilement pessimiste.** Le max de R(∪ labels)/r échoue (10/9) sur une
     coquille sans point critique, où τ ≤ 4/5. Diviser par max(r, √a_τ) répare.
   - **Mesuré** sur les trois trames, en lecture seule du cache mhgp11 `07428324e` (taille native, pas
     8 000 / 16 000 / 32 000). La part des nœuds avec d_v ≥ 3 b_v vaut 0,6–0,7 % à K = 2, 0,01 % à K = 5 et 0 à
     K = 10. La « coupe intérieure de marge ×3 » recommandée pour D_v n'existe donc presque jamais. Par ailleurs, au
     plus 22–28 % des nœuds à K = 5 (12–17 % à K = 10) ont une vie > 4δ (δ = √3/2 mm) : c'est la borne supérieure de
     la part des nœuds qui ont une coupe certifiable au sens de E3.
3. **Rejeux indépendants confirmés** avec l'oracle `oracle_ak`, dont le code est distinct de celui des théoriciens :
   E11, F3/F4, N2, S6 (avec √3 ≈ 1,732) et F6. Aucune autre contradiction n'a été trouvée. Les preuves de R0, N1,
   N4, Q1, G0, G1, D1, R1, L1 (réduction), T2, P4, T6, P7, P8, L0, E1–E5, E7, E12–E14, O1, O3, D3 et D5 ont été
   relues et tiennent.
4. **La question épineuse reste ouverte, et le bilan est plus sévère que celui des notes.** Aucun candidat n'est à
   la fois petit et certifié.
   - Le représentant certifié (A_k réduit, causal, à sommets protégés) a une topologie et une hiérarchie prouvées.
     Mais sa taille est ≥ 2V − β0 plus le forçage, et il exige une mosaïque locale d'ordre k (~10³ faces par point à
     k = 5) : hors des 100 ms.
   - Le représentant petit (D_v, ~28 faces par point) n'est pas homotope, partage des sites entre nœuds vivants, et
     sa garantie ×3 est vide à l'échelle des nœuds LiDAR.
   - Côté robustesse, l'identité tient pour les nœuds FULL (N1), pas pour les chaînes contractées.

## 1. Méthode

- J'ai lu les quatre notes en entier : robustesse (597 lignes), réduction (282), alternatives (sections 0 à 5 et
  tableaux) et niveaux (sections 0 à 3 et tableaux). Pour chaque preuve, j'ai cherché l'hypothèse cachée, le cas
  dégénéré (égalités, plateaux, alignements) et l'extension non justifiée.
- J'ai écrit trois scripts en arithmétique exacte, en Fraction, avec `oracle_ak` en appui. Ils rendent un code non
  nul en cas d'échec et n'utilisent pas `assert` :
  - `verif_critique.py` : 14 contrôles, verts ;
  - `verif_rejeux.py` : 5 contrôles, verts ;
  - `verif_f6.py` : 1 contrôle, vert.
- J'ai fait une mesure en lecture seule du cache du harnais (`mesure_fenetres.py`, 9 lignes : 3 trames × K = 2, 5,
  10).

## 2. Contre-exemples nouveaux (gravés dans `fixtures_critique.json`)

### 2.1 `contraction_instable` : réfute N3, donc RR3

k = 1, P = {0, 50, 99} et P' = {0, 49, 99} (sur l'axe x), δ = 1 (le site 50 se déplace de 1).

- Dans P, {50} et {99} fusionnent en 49/2 : les deux enfants vivent 49/2 > 2δ, c'est une vraie fusion. Le parent
  {50, 99} vit 1/2, puis rejoint {0} en 25. La fusion est contractée et la chaîne racine continue le **site 0**.
- Dans P', c'est {0} et {49} qui fusionnent en 49/2 (deux enfants robustes). {0, 49} vit 1/2, puis rejoint {99} en
  25. La chaîne racine continue le **site 99**.
- La chaîne du site 0 dans P a pour intérieur [1, +∞). Sa trace dans P' est portée par la chaîne 0' jusqu'à 49/2,
  puis par la racine, qui est la chaîne 99'. Elle est coupée en 49/2 par une fusion entre deux nœuds de vie 49/2.
- N1 (b) n'est pas en défaut : 49/2 > d_v − δ = 24 pour le nœud FULL {0}.

Diagnostic. N1 utilise que v ne subit aucune fusion pendant sa vie. Une chaîne contractée en subit, et une branche
« éphémère » peut contenir des petits-enfants robustes. La règle écrite (« un seul enfant de vie > 2δ ») ne regarde
d'ailleurs que la vie du nœud FULL. Deux fusions éphémères en cascade, à moins de 2δ l'une de l'autre, coupent donc
une chaîne que l'on voulait continuer.

Aucune identité de chaîne ne peut être stable aux fusions presque simultanées : c'est l'instabilité de la règle de
l'aîné. Ce qui est prouvé : l'identité des **nœuds FULL** sur [b_v + δ, d_v − δ] (N1) et la correspondance des
**composantes** à une coupe (E2 de `theorie_niveaux`).

### 2.2 `bipyramide_forcage` : réfute la lecture de P5 (« au mieux », « mousse »)

k = 1. Sites A = (0,0,30), B = (10,0,0), C = (−6,8,0), D = (−6,−8,0), E = (0,0,−5). BCD est inscrit dans le
cercle de rayon 10. Delaunay = {ABCD, BCDE}.

- **Niveaux** : a_ABCD = 2500/9 ; ABCD est critique (centre circonscrit (0,0,40/3) intérieur) et seule dans son
  groupe. a_BCD = a_BCDE = 625/4, de même témoin (0,0,15/2) : le groupe {BCD, BCDE} est une paire, μ = 0.
- **Partenaires** : ABCD n'a aucun partenaire de même naissance. Le seul partenaire possible de BCDE est BCD.
- **Conclusion** : dès r² ≥ 2500/9, tout certificat garde ABCD, puis BCD par fermeture, puis BCDE, qui n'a pas
  d'autre partenaire. L'énumération des 16 appariements acycliques à sommets protégés donne au minimum 2 tétraèdres.
  La borne Σ μ(G) en compte 1.

Conséquences sur `theorie_reduction` :

- La proposition 4.1 reste vraie comme minorant.
- La colonne « gardées au mieux » (6k − 2 sur 12k − 6, 295 sur 1 246) n'est pas une taille atteignable.
- Le profil (68.6, 120.2, 79.0, 27.5) n'est pas celui d'un représentant.
- « En dimension 3 ne restent que les tétraèdres critiques » est faux, dès k = 1 et sans dégénérescence.
- La note mesure elle-même, dans le plan, un rapport causal de 0,73 à 0,88 contre une borne de 0,48 à 0,61 : le
  forçage est la règle, pas l'exception. En 3D, rien n'est mesuré.
- L'énoncé L (« le LiDAR garderait environ un quart ») hérite du défaut : il serait au mieux un minorant.

### 2.3 `certificat_kappa_lache` : nuance G2–G3

k = 1, A = (−10,0,0), B = (10,0,0), C = (0,5,0). Coquille : r² = 81, (r + Δ)² = 625/4. Aucun point critique de
d_1 n'y tombe : AB n'est pas de Gabriel, le centre circonscrit (0, −15/2) est hors du triangle, et les deux selles
sont en 125/4.

- Le certificat publié vaut max R(∪ labels)²/r² = 100/81 > 1 : il échoue.
- La vraie borne est τ ≤ 4/5.

Correction proposée, saine pour la même raison que l'original : pour x dans l'intérieur relatif de F_τ, on a
Σ(x) ⊆ ∪ labels(τ) et d_k(x) ≥ √a_τ. D'où :

$$\tau\le\max_{\tau}R(\cup\,\mathrm{labels}(\tau))/\max(r,\sqrt{a_{\tau}})$$

Le maximum porte sur les cellules dont la face duale rencontre la coquille par son intérieur relatif. Pour une
cellule non critique, on a alors R(∪ labels(τ)) < √a_τ, et le certificat n'échoue plus qu'aux cellules (presque)
critiques. Le théorème G2 lui-même tient (lemme de déformation non lisse cité de mémoire).

## 3. Mesure : fenêtres de garantie sur LiDAR

Source : `mesure_fenetres.py`, lecture seule de `supports` (MHGP11SP v2, mhgp11 `07428324e`). Trames sans sol de
taille native, n = 35 551 à 45 845 ; pas 8 000 / 16 000 / 32 000.

| Trame | K | nœuds non racine | d_v ≥ 2 b_v | d_v ≥ 3 b_v | vie > 4δ (δ = 0,866 mm) | naissance médiane (mm) |
| --- | --- | --- | --- | --- | --- | --- |
| 08/000000 | 2 | 178 126 | 3,91 % | 0,72 % | 45,3 % | 88 |
| 08/000000 | 5 | 576 370 | 0,02 % | 0,01 % | 22,8 % | 179 |
| 08/000000 | 10 | 1 638 572 | 0 | 0 | 12,0 % | 272 |
| 08/000100 | 5 | 478 264 | 0,02 % | 0,01 % | 27,7 % | 234 |
| 08/000200 | 5 | 609 375 | 0,02 % | 0,01 % | 22,2 % | 115 |
| 08/000100 et 08/000200 | 10 | 1,27 M ; 1,56 M | 0 | 0 | 16,8 % ; 12,3 % | 396 ; 169 |

Lecture :

- Les garanties **multiplicatives** ne disent presque rien à l'échelle des nœuds : D_v (×3, D3), l'offset (×2, F1),
  et Alonso (c > 1).
- Les naissances sont de l'ordre du décimètre, les vies de l'ordre du millimètre (vie médiane 0,58 à 0,87 mm à K = 5,
  mesurée par `theorie_robustesse`). L'ancêtre au rayon 3r est presque toujours un nœud bien plus grossier.
- Les garanties **additives** (E2–E3, N1) gardent un sens pour un quart des nœuds à K = 5. C'est une borne
  supérieure : un palier est plus court que la vie.

## 4. Verdicts par énoncé

### 4.1 `theorie_robustesse`

| id | Verdict | Raison |
| --- | --- | --- |
| R0 | confirmé | Inclusions [A2] compatibles ; foncteurs π0, H_i ; composées = structure. |
| N1 | confirmé | (a)–(d) relus. L'hypothèse « aucune fusion pendant la vie » est utilisée en (b) et en (d), et elle est valide pour les nœuds FULL. |
| N2 | confirmé | Rejeu oracle (échelle 100) : P a 4 nœuds, racine en 5525 ; P' a une lentille isolée en 10 000, qui fusionne en 408080401/40804. |
| N3 | réfuté | Fixture `contraction_instable` : identité de chaîne contractée instable ; N1 ne s'étend pas aux chaînes. |
| N4 | confirmé | ψ∘φ vertical ; la profondeur m ≥ d + e suffit. |
| Q1 | confirmé | Comptage exact ; e(ρ) défini comme maximum local. |
| S1 | nuancé | Inclusions exactes, et ε ≥ 1/n pour S ⊊ P. Mais c'est une limite de la **garantie**, pas de la tour : un retrait loin de tout ne change pas Ω_k(r) pour k ≥ 2 aux petits r. La remarque VC est hors sujet. |
| G0 | confirmé | Argument composante par composante relu (pas de fusion dans [r − δ, r] pour P et P'). |
| G1 | confirmé | Projection de x sur conv Σ = centre de la boule minimale ; R² + abs(c − x)² = ρ². |
| G2-G3 | nuancé | Théorème correct, avec un lemme de déformation cité de mémoire. Certificat de κ lâche : fixture `certificat_kappa_lache` et correction ci-dessus. |
| G4 | confirmé | Labels et combinatoire constants ⇒ chaque point bouge d'au plus δ. |
| pente | confirmé | Au milieu de deux sites, la pente forte vaut 1 en un point de selle. |
| G5 | confirmé | Date de AB = 1 + ((1 − h²)/(2h))², d'ordre 1/(4h²). |
| Wrap | confirmé | Cité (Bauer–Edelsbrunner, th. 5.10), en position générale, k = 1 seulement. |
| D1 | confirmé | d_i ≥ d_j pour i ≥ j. |
| R1 | confirmé | Entrelacement le long d'une courbe monotone ; N1 s'y transporte pour les nœuds, pas pour les chaînes (cf. N3). |
| R1u | non vérifié | Conjecture. Les mesures du § 3 ne l'appuient pas : les vies sont ≪ rayons. |

### 4.2 `theorie_reduction`

| id | Verdict | Raison |
| --- | --- | --- |
| Déf. 2 | nuancé | « Plus petite fonction telle que e ≤ … » est la fonction nulle. La formule explicite (min sur les cellules libres atteignables) est la **plus grande** solution. Erreur de mot, sans effet sur T2. |
| L1 | confirmé | Stricte convexité de max abs(y − q)² sur F_σ. |
| T2 | confirmé | (1)–(6) relus. Le complément est une réunion de paires, effondrable par acyclicité (Forman). La localité découle de la monotonie des naissances le long des chaînes. |
| P4 | confirmé | Minorants (groupes, Morse forts, singuliers). |
| P5 | réfuté | Comme minorant, oui. Comme « au mieux », profil du représentant ou « seuls les tétraèdres critiques restent », non : fixture `bipyramide_forcage`. L'espace repose sur des constantes Monte-Carlo, d'où « prouvé » trop fort. |
| T6 | confirmé | Indépendance : toute arête garde une extrémité libre ; r + λ, λ ≤ 2r/k en position générale. |
| F1 | confirmé | d_2(y)² = 265/4 recalculé à la main. dist²(y, L) = 24649/340 n'est que rejoué (sortie de la note, appariement glouton), pas recalculé indépendamment. |
| P7 | confirmé | Sommets protégés ⇒ tout point d'une cellule effondrée est à ≤ diam τ ≤ θ r d'un sommet de L. |
| O | nuancé | Vérifié borné, mais partiel : (1,3,2) sur 400 000 / 967 680 ordres, (3,3,3) sur 20 000 / 1,15·10¹⁰. « Pour tout départage » n'est pas établi sur ces types. |
| O' (F3/F4) | confirmé | Rejeu oracle : niveaux, centres et profils (0,3,4,1) et (1,4,4,1) exacts. |
| O2 | non vérifié | Conjecture (0 échec sur 2 747). |
| P8 | confirmé | Les sites dans une sphère de centre dans C_v et de rayon ≤ r sont dans P_v(r). Le test ambiant ne sert qu'à écarter les sphères candidates mal centrées. |
| L (quart LiDAR) | réfuté | Extrapolation d'un minorant non atteignable (§ 2.2) ; non mesuré. |

### 4.3 `theorie_alternatives`

| id | Verdict | Raison |
| --- | --- | --- |
| L1 | confirmé | Demi-espace séparant. |
| O1 | confirmé | U_σ(y) = N_k(y) par perturbation vers u ; attribution par convexité de F_σ ∩ Ω_k(r). |
| O3 | confirmé | Cellules ⊆ conv(U_σ) ; L1 ; en 1D, réunion connexe d'intervalles à bouts couverts. |
| O4 | confirmé | Recalculé : centre (0, −15/2), R² = 289/4 > 16, (0,4) hors du cercle, z intérieur. |
| O5 | confirmé | Moyeu carré : anneau de 8 lentilles autour de 0, d_2(0)² = 4 > 3 ; ombre = losange. |
| O6 | confirmé | Preuve relue (nerf réalisé, homotopie rectiligne dans Ω_1(r + ε)). |
| D3 | confirmé | Trois inclusions relues ; facteur 3 atteint avec {0, 2}. |
| D4 | nuancé | Bifiltration et trace exactes. Mais D_v(r) et D_w(r) peuvent partager des sites pour v ≠ w vivants ({0,2,4}, k = 2, r = 1 : le sommet 2) : ce n'est pas une partition à la coupe. Le lien avec DelCore n'est pas vérifié. |
| D5 | confirmé | L1, puis ≤ r jusqu'à C_v. |
| D7 | non vérifié | Non rejoué. Mesure du Del_1 global en flottant (qhull), pas de D_v avec les dates g_k ; disponibilité de g_k « par les populations FULL » non vérifiée. |
| D9 | confirmé | F6 rejoué par l'oracle (échelle 20) : p, q couverts par le même nœud, m dans aucune conv(U_σ) ; F3 refait à la main. |
| F1 | confirmé | g_k ≤ d_k ≤ 2 g_k ; facteur 2 atteint. |
| A1 | non vérifié | Dépend des constantes d'Alonso (article non relu ici). |
| A2 | non vérifié | Arithmétique juste ((17 + 8√3)³ ≈ 2,94·10⁴), lemme non relu. |
| A4 | non vérifié | Proposition sans preuve relue. |
| DTM | non vérifié | Composition de bornes citées de mémoire (facteur √6 de Guibas–Mérigot–Morozov). |

### 4.4 `theorie_niveaux`

| id | Verdict | Raison |
| --- | --- | --- |
| L0 | confirmé | Le centre de la boule minimale de P est dans tous les W_Q(r) ; réunion étoilée. |
| E1 | confirmé | Propriétaire unique ; laminarité ; filtre par date (iii)–(v) relus. |
| E2 | confirmé | Composantes, pas nœuds : compatible avec N2. Bijection supposée. |
| E3 | confirmé | Sandwich de rangs correct. Mais « certifiée ssi palier > 4δ » doit se lire « si » (condition suffisante du certificat). |
| E4-E5 | confirmé | Marge = demi-palier ; argmax instable (reconnu). Mesuré : au plus 22–28 % des nœuds de K = 5 ont une vie > 4δ (§ 3). |
| E7 | confirmé | d_k 1-lipschitzienne ; 2r atteint. |
| E9 | confirmé | Phénomène élémentaire à K = 1 (Ω_1(2r)) ; fixtures `anneau8` et fer à cheval non rejouées. |
| E11 | confirmé | Rejeu oracle (échelle 4) : x sur l'arête d'ordre 2 de date 16, x seulement dans le triangle d'ordre 1 de date 65/4 > 16. |
| E12 | confirmé | Sandwichs (A6). |
| E13 | confirmé | Fonctorialité (énoncé presque tautologique). |
| E14 | confirmé | π0 d'un complexe cellulaire = π0 du 1-squelette ; Kruskal. |
| S6 | confirmé | Rejeu oracle avec √3 ≈ 1,732 : β = (3,0,0) à r² = 3,7, (1,2,0) de 3,8 à 3,99, (1,0,0) à 4,01. |

## 5. La question épineuse : les réponses tiennent-elles ?

| Exigence | Réponse des notes | Jugement |
| --- | --- | --- |
| Représentant **petit** | A_k réduit : « au mieux un quart » ; D_v : 28 faces par point | **Non établi.** Le quart est un minorant non atteignable (§ 2.2) ; le plancher prouvé est 2V − β0 (137 par point à k = 5, Poisson), plus le forçage. Le calcul exige la mosaïque locale d'ordre k (hors des 100 ms ; non mesuré à l'échelle). D_v est petit mais sans topologie certifiée. |
| **Topologie** certifiée | Représentant causal T2, sommets protégés | **Tient** (preuve relue, sans position générale), à corriger d'un mot (Déf. 2). |
| **Hiérarchie** | E1 (propriétaire, dates) + causal emboîté ; entre ordres, liens π0 seulement | **Tient** à k fixé. Pas d'inclusion géométrique entre ordres (E11, O4 confirmés). D_v s'emboîte entre ordres mais ne sépare pas les nœuds vivants. |
| **Géométrie** mesurée | D_v(r), règle θ, r + λ ; κ pour la stabilité du dessin | **Tient par nuage.** Le certificat κ est à corriger (§ 2.3). Rien n'est mesuré sur LiDAR. |
| **Robustesse** des niveaux | N1, arbre δ-contracté, coupes E3, profondeur d'ordre | **Partielle.** Les nœuds FULL (N1) et les composantes (E2–E3) tiennent ; les chaînes contractées non (§ 2.1). Au plus 22–28 % des nœuds de K = 5 ont une coupe certifiable au bruit de quantification. Les garanties ×2 et ×3 sont vides (§ 3). |

Conclusion. L'objet est bien fixé, et les certificats topologiques sont justes. Ce qui manque est exactement ce que
l'auditeur annonçait : un représentant à la fois petit et certifié, au coût visé. Les deux raccourcis proposés
(« un quart après réduction » et « D_v à marge ×3 ») tombent tous deux, l'un par un contre-exemple exact, l'autre par
la mesure. Pour le dessin de la hiérarchie, il reste trois éléments :

- les **nœuds FULL** de vie > 4δ comme identités ;
- **A_k réduit** comme certificat ;
- **D_v ou l'ombre** comme rendus nommés, sans garantie d'homotopie.

## 6. Manques

1. Aucune mesure de réduction certifiée en 3D, ni à n = 8 000, 16 000 ou 32 000. Le rapport causal 3D est inconnu.
2. Aucune notion d'identité au-delà des nœuds FULL n'est prouvée stable. Il faut redéfinir RR3 sur les nœuds
   (N1), ou passer à l'entrelacement d'arbres sans identité.
3. Le certificat κ n'est ni corrigé ni mesuré dans les notes.
4. Les paliers H1/H2 réels ne sont pas mesurés. Je n'ai mesuré que la borne « vie > 4δ ».
5. D7 mesure le Delaunay global, pas D_v avec ses dates g_k. La provenance de g_k depuis FULL n'est pas vérifiée.
6. La stabilité du choix d'appariement sous perturbation n'est pas traitée : le dessin réduit peut sauter.
7. A1, A2, A4 et la borne DTM ×√(6k) n'ont pas été relus contre les articles.

## 7. Registre et suite

Les réfutations portent sur des énoncés de notes en `build/`, pas sur le registre du dépôt. Les fixtures sont
gravées dans `fixtures_critique.json` et rejouables. La mise à jour de `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`
(règle de CLAUDE.md) revient à l'orchestrateur, puisqu'il m'est interdit d'écrire hors de `build/`.

## 8. Rejeu

```sh
PY=/workspaces/E-HGP/build/v11-persist/videos/venv/bin/python
cd /workspaces/E-HGP/build/v11-persist/polyedres_ordre_k/critique_theorie
nice -n 10 $PY -B verif_critique.py    # 14 contrôles, environ 1 s
nice -n 10 $PY -B verif_rejeux.py      # 5 contrôles, environ 1 s
nice -n 10 $PY -B verif_f6.py          # 1 contrôle, environ 1 s
nice -n 10 $PY -B mesure_fenetres.py   # lecture seule du cache, environ 60 s
```

Fichiers : `CRITIQUE.md`, `fixtures_critique.json`, `verif_critique.py` (+ `.json`), `verif_rejeux.py` (+ `.json`),
`verif_f6.py` (+ `.json`), `mesure_fenetres.py` (+ `.json`), `SHA256SUMS`.
