# Réponse de Claude — V156 : « le cœur ne tue rien » et « le cœur tue 99,77 % » sont vrais tous les deux, et le crédit d'extrémité rend déjà mieux que le cœur

- **Ancrage :** pin `fc714db7`. Second passage adversarial sur la cascade des
  trois crédits : cinq analyses, cinq réfutations indépendantes, toutes retenues
  comme réfutations. Ce qui suit est ce qui en sort de solide.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Deux de mes faits paraissaient se contredire ; ils portent sur deux étages

Mon § V147 établissait : **zéro mort d'ancre attribuable au cœur seul**
($0/6753$, $0/2604$, $0/1391$, $0/3518$). Le contrôle adversarial établit :
**le cœur retire $99{,}766\,\%$ de la masse intrinsèque de paires** sur `terrain`.

Les deux sont exacts, et ils ne parlent pas du même étage :

| étage | population | qui tue |
|---|---|---|
| **descente WSPD** | toutes les paires du nuage | **le cœur, à $80{,}9$–$99{,}8\,\%$** |
| **lane, par ancre** | les ancres des rectangles *survivants* | **$h_a+h_b$, à $100\,\%$** |

Sur un rectangle vivant, le cœur est plafonné à $h_3-1$ **par construction**
(`generate.hpp` n'émet que `core <= 8` pour $h_3=9$) : il ne peut donc, par
définition, tuer aucune ancre à cet étage. Tout ce qu'il pouvait tuer, il l'a
tué pendant la descente.

Marginal exact du cœur, obtenu sans échantillonnage en posant $h_3 = +\infty$ —
la descente conserve alors les $\binom{m}{2}$ paires, vérifié sur $8$
configurations sur $8$ :

| cohorte | masse intrinsèque retirée par le cœur pendant la descente |
|---|---:|
| `terrain` | **99,766 %** |
| `scanline` | 99,195 % |
| `uniform` | 98,803 % |
| `eight_clusters` | 80,890 % |

C'est le vrai poids du cœur, et il explique pourquoi la descente WSPD — pente
$1{,}19$ à $1{,}36$, quasi linéaire — est ce qui rend l'ensemble praticable.

## Et sur l'unité qui décide, le crédit d'extrémité rend DÉJÀ mieux

En unité commune — une évaluation de prédicat entier au dénominateur, et
$\text{seeds tués} \times 11$ tests aval au numérateur — sur `eight_clusters`
$n=800$, $s=8$ :

| crédit | rendement |
|---|---:|
| cœur | $\times 15{,}98$ |
| **marginal $h_a+h_b$** | $\times \mathbf{18{,}69}$ |

**Au point de fonctionnement même où on les écartait, les crédits d'extrémité
rendent plus par unité de coût que le cœur.** Votre exigence — « il faut
$h_{\mathrm{coeur}}$ mais aussi $h_a$ et $h_b$ » — est donc confirmée deux fois :
sur le dénominateur des morts d'ancre ($100\,\%$ les exigent) et sur le rendement
par unité de coût.

## Mais le classement dépend du régime, et je le corrige

Le rapport $\text{rendement}(h_a+h_b)/\text{rendement}(\text{cœur})$, recalculé
avec les marginaux exacts, vaut à $s=2$ **$0{,}376$ sur `terrain`** et
**$0{,}225$ sur `scanline`** — donc le cœur y gagne largement. Sur
`eight_clusters` au contraire, à $s=2$ et $n=800$ : cœur seul $1{,}718\,\%$,
$h_a+h_b$ seuls $40{,}183\,\%$, marginal $h_a/h_b$ $42{,}005\,\%$.

Le couloir vide inter-amas, dont une analyse concluait qu'il vide le mécanisme
d'extrémité, fait en réalité à $s=2$, $n=800$ : $72{,}94\,\%$ des ancres,
$80{,}26\,\%$ de la masse, $h_a$ moyen $3{,}900$, $h_b$ moyen $3{,}843$, et
$44{,}11\,\%$ de cette masse tuée par $h_a+h_b$ **seuls**.

**Il n'y a donc pas un vainqueur mais une partition par régime** : le cœur porte
`terrain`, `scanline` et `uniform` ; les extrémités portent `eight_clusters`.
C'est exactement la lecture que vous proposiez, et c'est pourquoi aucun des deux
ne peut être abandonné.

## Une erreur de doctrine évitée de justesse

Une analyse concluait que le certificat aux $64$ coins de
$\mathrm{Box}(A)\times\mathrm{Box}(B)$ est **exact** et non seulement suffisant,
et recommandait de l'inscrire dans l'en-tête de `src/spindle/spindle.hpp` — un
fichier que `CLAUDE.md` désigne comme doctrine. C'est **faux au sens que le code
consomme** (`witness_count.hpp` : universalité sur les **points réels** de
$A\times B$, non sur les boîtes), et la réfutation donne le contre-exemple entier
minimal, trouvé par recherche exhaustive au rayon $L^\infty = 2$ :

$$A=\lbrace(-2,-2,0),(-1,-1,0)\rbrace,\quad B=\lbrace(-2,2,0)\rbrace,\quad z=(-1,0,0).$$

$z$ est témoin universel de $A\times B$ ($3H^2 = 27 > 16$ et $12 > 1$), mais le
coin $(-2,-1,0)$ de $\mathrm{Box}(A)$, **qui n'est pas un point de $A$**, donne
$3H^2 = 3 \leq 9$ : le certificat aux coins échoue. L'en-tête actuel, qui écrit
« certificat **suffisant** (sens consommé) », est donc correct et ne doit pas
être touché.

## Et la question de la décomposition est close

Aucune candidate ne bat le rectangle WSPD : le groupement par **segment** et le
**rectangle anisotrope** sont réfutés, et le deux-niveaux à « citron partagé »
est **strictement plus faible** — ce que j'avais moi-même établi en V150 par
domination du test $W_3$ par ancre. La question « quelle autre décomposition ? »
reçoit donc une réponse négative documentée.

## Question

- **V156.** Le cœur retire $99{,}77\,\%$ de la masse pendant une descente qui
  coûte $n^{1{,}19}$, et les extrémités retirent le reste à un rendement
  supérieur. La marge restante est donc dans le $0{,}23\,\%$ qui survit à la
  descente sur `terrain` — soit $8{,}7\times 10^5$ seeds portés par $93\,195$
  ancres à $n=2000$. Voyez-vous un certificat qui agirait **pendant** la
  descente, sur les rectangles que le cœur ne ferme pas, plutôt qu'après elle ?
