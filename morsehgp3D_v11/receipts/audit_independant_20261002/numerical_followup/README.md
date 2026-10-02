# Suivi numerique F3/F4 — 2 octobre 2026

Lecture seule de `docs/ARCHITECTURE.md`, capture locale dans
[ARCHITECTURE.snapshot.md](ARCHITECTURE.snapshot.md), source datee et hachee dans
[SOURCE_BEFORE.json](SOURCE_BEFORE.json). Ce recu complete le contre-exemple
historique de [floating_bounds](../floating_bounds/README.md) ; il ne le remplace
pas. Aucun moteur, build, fichier developpeur ou recu clos n'a ete modifie.

**F3 est corrige sous les hypotheses ecrites.** Posons t = 1 - u. Les ratios
approche / exact sont positifs et appartiennent a [t^E, t^-E]. Un produit
additionne les expositions et un arrondi ; la reutilisation dans un carre donne
donc 2 E + 1. L'inverse d'un ratio garde le meme intervalle : quotient direct
demande au plus E_a + E_b + 1, inverse arrondi puis produit au plus
E_a + E_b + 2. Une somme de termes de meme signe est une moyenne ponderee des
ratios avant l'arrondi, d'ou max E_i plus au plus n - 1 arrondis. Une FMA
contractee utilise moins d'arrondis que l'expression non contractee et reste dans
sa borne conservative. Les transformations admises restent celles enumerees par
la doctrine. Le domaine normal/fini doit couvrir leurs intermediaires, y compris
l'inverse si cette transformation est admise ; ce recu ne demontre aucun budget
de domaine d'une future cle HGP.

**F4 est correct pour y > 0.** Avec q_x, q_y les ratios et delta celui de
l'arrondi du produit par c, le test implique
x < c y q_y delta / q_x. Si y > 0, le membre de droite est au plus y lorsque
c <= t^(E_x + E_y + 1). Pour S <= 4096, Bernoulli donne
t^S >= 1 - S u >= 1 - 4096 u = 1 - 2^-40 : la constante annoncee convient.

**Condition de domaine a expliciter dans F4.** La capture F3 mentionne x != 0,
et F4 ne dit pas que les cles sont positives. Pour x = y = -1 et E_x = E_y = 0,
le produit exact c y = -c est superieur a -1 : le test accepte x < y alors que
x = y. Le temoin est execute en binary64 et le produit est representable
exactement. Les niveaux geometriques non negatifs permettent de fermer ce point
en limitant le filtre aux cles strictement positives et en passant les niveaux
zero a la comparaison exacte. Il s'agit d'une clarification normative avant
implementation, pas d'un defaut observe dans un filtre produit.

[check.py](check.py) est autonome : modeles Fraction aux bornes des intervalles,
reutilisation, quotient direct et par inverse, somme, FMA, Bernoulli et temoin
binary64 negatif. Les controles finis illustrent les preuves generales ci-dessus
sans qualifier tous les modes d'arrondi ni un moteur. Les sorties normal et -O,
commandes et empreintes sont conservees ; [SOURCE_AFTER.json](SOURCE_AFTER.json)
indique si le document LIVE a evolue. Le verdict s'ancre a la capture precedente.
