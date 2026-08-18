# Addendum — kernel affine par ancre (q3 + cœur q4), garde d'arrondi exécutable

Date : 18 août 2026. Base : `90f8dc6` + contre-audits `04c71a2` /
`5d274a1` (borne dynamique REÇUE avec preuve γ). Exécute le § 1 de
`REPONSE_A_CLAUDE_E573888_FILTRE_CERTIFIE_ET_NIVEAUX_Q3` (kernel
affine), le § 3 (contrat flottant durci, témoin de forte annulation
§ 6.1) et le n° 2 de l'« ordre conseillé » du contre-audit `5d274a1`
(garde fast-math / mode d'arrondi). Le schéma L/U (§ 5) et les
intervalles de Jung (§ 1.2) restent les chantiers suivants.

## Ce qui change

1. **Sites affines par ancre** (`LaneScratch::fill_affine_sites`) :
   $u_z = 2z - a - b$, $q_z = \vert u_z\vert^2 - D^2$ — entiers
   $< 2^{36}$, donc EXACTS en binaire64. Calculés au PREMIER seed
   effectif de l'ancre (remplissage paresseux : une ancre sans seed
   aigu, ou tuée par $W_4$, ne paie rien), partagés par tous ses seeds.
   L'ancre fournit aussi $q_{max}$, $u_{max}$ pour la borne par seed.
2. **Par seed** : $N = W - G\,d$ (i128, $\vert N\vert < 2^{87}$), une
   fois ; l'interaction avec chaque site est UN produit scalaire :
   $L(z) = G\,q_z - 2\,u_z \cdot N$, avec l'identité $L = 4\,P$
   ($P$ = `q3_power`) gravée par la porte permanente. Le scan de
   profondeur q3 utilise le signe de $L$ ; le cœur q4 (Jung) prend
   $P = L/4$ — division EXACTE (divisibilité gravée), pour
   `cmp_2p2_jb2` inchangé.
3. **Séquence flottante FIGÉE et partagée** (`affine_l_hat`,
   `affine_l_bound` — les lanes ET les portes appellent CES fonctions,
   jamais une copie) :
   `t = fma(N2, u2, fma(N1, u1, N0*u0)) ; Lh = fma(G, q, -(t+t))`.
4. **Garde d'arrondi exécutable** (contre-audit `04c71a2` § 4) :
   `kFloatFilterCompileEnabled = false` sous `__FAST_MATH__` ;
   à l'exécution `std::fegetround() != FE_TONEAREST` force la borne à
   $+\infty$ : zéro certification, repli exact intégral, sortie
   inchangée. Le mutant `float-ignore-rounding` saute la garde.
5. **Réparation d'enregistrement** : la porte `--float-gate` de
   `879b37d` n'avait JAMAIS été câblée en CTest (le reçu la décrivait,
   le CMake ne l'appelait pas). Les six tests (float, affine, arrondi ×
   porte/mutant) sont enregistrés ; la suite passe de 113 à 119.

## Dérivation ligne à ligne de la borne (forme du contre-audit)

Posons $u = 2^{-53}$, $M(z) = \vert G\vert\,\vert q_z\vert +
2\sum_i \vert N_i u_{z,i}\vert$.

- **Conversions** : $g = \mathrm{fl}(G)$, $n_i = \mathrm{fl}(N_i)$ avec
  erreur relative $\leq u/(1-u)$ chacune ; les sites $u_z$, $q_z$ sont
  des entiers $< 2^{36}$ : conversion EXACTE — les termes d'erreur de
  site du programme précédent disparaissent.
- **Programme exécuté** : `r0 = fl(n0*u0)` ; `r1 = fma(n1,u1,r0)` ;
  `r2 = fma(n2,u2,r1)` ; le doublement `(r2+r2)` est une
  multiplication par 2, EXACTE ; `Lh = fma(g,q,-(r2+r2))`. Quatre
  arrondis au total. Le lemme des facteurs $(1+\delta)$ donne
  $\vert Lh - (g q - 2\sum n_i u_i)\vert \leq \gamma_4 M(z)$,
  $\gamma_4 = 4u/(1-4u)$.
- **Total** : $\vert Lh - L\vert \leq (\gamma_4 + u/(1-u)) M(z)
  < 6u\,M(z)$ — mêmes constantes que la preuve reçue du contre-audit,
  avec MOINS de sources d'erreur (sites exacts).
- **Seuil calculé** : `E = 2^-48 * fma(g, qmax, 2*(|n0|+|n1|+|n2|)*umax)`
  n'utilise que des opérations positives :
  $E \geq 32u\,(1-u)^4 M_{max} > 31u\,M(z)$. Marge $> \times 5$, comme
  la version reçue. Décision : $Lh < -E \Rightarrow L < 0$ certifié ;
  $Lh > +E \Rightarrow L > 0$ certifié ; sinon repli affine exact i128
  ($\vert L\vert < 2^{107}$), et $P = L/4$ exact.

## Portes

- `--q3-affine-gate` (porte PERMANENTE, code 0 ; mutant
  `float-threshold-too-small` : 4) :
  1. **Identité exhaustive** : 1 937 679 triples $(a, b, x) \times z$
     sur quatre nuages (uniform40, eight32, uniform28 à coord=50000 —
     grandeurs pleine largeur u16, conversions inexactes — et la
     fixture-cœur cocirculaire ×1999), **0 violation** de
     $L = 4\,\mathrm{q3\_power}$ ET de $L \equiv 0 \pmod 4$ — y compris
     $z \in \lbrace a, b, x\rbrace$ ($d \cdot N = d \cdot W - G D^2 = 0$
     par $P(b) = 0$). Chaque décision flottante certifiée recoupée :
     588 855 nég + 1 185 258 pos, 163 566 replis, **0 désaccord**.
  2. **Témoin de forte annulation ±** (audit § 6.1), constantes
     GRAVÉES : $G = 2^{67} - 12345$, $u = (131071, 0, 0)$,
     $q = 2^{35} + 7$, $N_0 = \lfloor G q / (2 u_0)\rfloor$ — deux
     termes $\sim 2^{102}$ s'annulent à $L = +216577$ ; variante
     $N_0 + 1$ : $L = -45565$. binaire64 rend le MÊME $\hat{L}$ (bruit
     $\sim -2^{49}$, $(double)N_0 = (double)(N_0{+}1)$) : la borne
     saine ($\sim 2^{55}$) déclare INCERTAIN les deux (exigé, code 3
     sinon) ; la borne mutée ($2^{35}$) certifie le bruit pour les
     deux, en désaccord avec l'exact $+216577$ → tué (code 4, désaccord
     témoin = 1, plus 29 883 désaccords de la fixture cocirculaire).
- `--float-rounding-gate` (code 0 ; mutant `float-ignore-rounding` :
  4) : sous `FE_UPWARD`, 0 certification, 12 644 117 replis, sortie
  bit-identique au run `FE_TONEAREST` (qui certifie 12 501 066 signes) ;
  le mutant certifie 12 501 066 signes sous `FE_UPWARD` → tué.
- `--float-gate` inchangée (l'étage affine dessous) : 24,9 M de signes
  certifiés recoupés, 209 503 replis — **exactement le même compte**
  que l'étage par forme de `879b37d` — 0 désaccord.

## Mesures (n=8000, smax=11, 4 fils — sorties IDENTIQUES : 2 658 325 / 3 126 158 événements, mêmes compteurs flottants à l'unité près)

| famille | avant (879b37d) | affine empressé | affine paresseux | + cvt au vol |
|---|---|---|---|---|
| eight_clusters t_gen | 123,2 s | 129,4-130,2 s | 130,2 s | **127,0 s** |
| uniform t_gen | 56,0-62 s | — | — | **57,3 s** |

RÉSULTAT HONNÊTE : le kernel affine est CPU-NEUTRE sur uniform et
~+3 % sur eight_clusters (bande de variance mesurée ±3 s entre runs
identiques). Les deux variantes essayées et gardées : remplissage
paresseux (une ancre sans seed ne paie rien) et conversion i64→double
au vol (un seul jeu de tableaux entiers, moitié du trafic du
remplissage — la conversion d'entiers < 2^36 est exacte, L^ inchangé
au bit près). L'adoption ne se justifie PAS par la constante CPU mais
par la STRUCTURE, conformément au cadrage de l'audit : identité
$L = 4P$ gravée en porte permanente, contrat d'erreur plus serré
(sites exacts — deux sources d'erreur de moins), donnée de site
partagée par ancre = forme exacte du kernel GPU warp-par-seed, et
`exact_L` bon marché re-évaluable — le prérequis de l'étage
d'INTERVALLES de Jung (§ 1.2), qui est le vrai multiplicateur mesuré
suivant (8,5 G d'évaluations i128 du cœur à convertir).

## Ce que ce reçu ne prétend pas

Le kernel affine ne change NI l'objet ni un seul candidat (portes
d'égalité + juges verts) ; il ne touche pas Jung (toujours
`cmp_2p2_jb2` exact sur $P = L/4$) ni `cmp_mu`. Les intervalles de
Jung (§ 1.2 — convertir les ~8,5 G d'évaluations i128 des sites
certifiés négatifs du cœur) et le schéma L/U (§ 5) sont les chantiers
suivants ; les couches convexes q3 restent différées (e27acfa § 2.3).
