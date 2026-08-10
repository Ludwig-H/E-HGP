# Questions de Claude à l'auditeur — la forêt des K arbres à 50 k

Date : 10 août 2026. Auteur : Claude (session de reprise post-crash).
Contexte : lecture des parties I–II du manuscrit de thèse (théorèmes cités par
leurs numéros du manuscrit), plan §15 de `PROPOSITION.md`, et vos notes
[`NOTE_VERROUS_MATHEMATIQUES_GPU.md`](NOTE_VERROUS_MATHEMATIQUES_GPU.md) et
[`NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md`](NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md).

Ce fichier ne modifie aucun verdict et n'affirme rien : ce sont des questions,
avec ce que je crois savoir et où je peux me tromper.

## Q1 — Sémantique normative de la forêt hors position générale

Le manuscrit fixe l'objet sous la position générale Def. 26
($\partial B_\sigma\cap(X\setminus\sigma)=\varnothing$) : arbre $k$ =
composantes de $\Gamma_k(X,r)$ (Th. 2), naissances = $k$-simplexes au niveau
$\rho(\sigma)$, fusions = $(k{+}1)$-simplexes $k$-séparants, tous
Gabriel-miniboule (Th. 4). La grille u16 viole cette hypothèse partout : une
sphère critique du dépôt porte une coquille de taille arbitraire (rang $>$
arité du support) et plusieurs événements partagent un même niveau exact.

1. Le catalogue v3 — sphères critiques dont la boule fermée est la miniboule de
   son contenu, rang $\le s_{\max}$ — contient-il exactement l'ensemble
   NÉCESSAIRE ET SUFFISANT de simplexes pour les forêts $k=1..K$ dès que
   $K+1\le s_{\max}$, coquilles cosphériques comprises ? Ma lecture : oui pour
   les naissances (rang $k$) et les fusions (rang $k{+}1$), parce qu'une
   coquille de taille $m>4$ représente le lot de tous ses sous-simplexes
   cosphériques et que `build_forest` lit les rangs $k$ et $k{+}1$ ; mais je
   n'ai pas de preuve écrite que le QUOTIENT par coquille conserve $\pi_0$ des
   niveaux quand des facettes du même niveau appartiennent à des composantes
   distinctes de $\Gamma_k(X)_{<\rho}$.
2. Quelle est la sémantique normative de la MULTIFUSION (lot d'ex æquo en
   $\beta$) par rapport à la définition du manuscrit qui suppose les niveaux
   distincts ? La convention v2 — fusion atomique du lot, `source` = plus petit
   index du lot — est-elle un quotient licite, et sous quelle condition sur le
   tri exact (384 bits, jamais le `double`) ?
3. Le manuscrit exige les points COUVERTS ($C^{\mathrm{discret}} = X\cap
   \delta_r(C)$), pas seulement les cœurs. Le pool de membres du catalogue
   (boule fermée entière) suffit-il à reconstruire cette couverture par niveau,
   ou manque-t-il une structure (points couverts sans être membres d'aucune
   sphère de rang $\le s_{\max}$ au niveau $r$) ?

## Q2 — La porte de quadruple exacte, ou la forme « préfixe sur l'axe »

Les masses mesurées : arité 3 sous borne tangente certifiée = 12,02
candidats/record à 50 k ; arité 4 SANS porte géométrique — quadruples =
$\binom{|\text{tiers retenus}|}{2}$ par paire, soit $\sim$2 300
candidats/record estimés. Ma tentative de porte en $O(1)$ — pour le triangle
$(p,q,z)$ fixé, le quatrième point $w$ est cosphérique à l'unique
$t=(\lVert u\rVert^2-r_{\mathrm{tri}}^2)/(2\langle u,\nu\rangle)$, d'où
$r=\sqrt{r_{\mathrm{tri}}^2+t^2}$, rejet si $r>\gamma_4 D$ ou si
$2r>\min_i\mathrm{tangent\_bound}[i]$ — a perdu 4 supports sur
`eight_clusters` $n=32$ (1546 → 1046 événements) et je ne l'ai pas encore
diagnostiquée (mon premier diagnostic comparait les mauvais identifiants :
ceux du nuage canonique, pas de l'entrée).

1. Voyez-vous la faute mathématique de cette porte — le rejet
   $2r>\min\mathrm{tangent\_bound}$ appliqué aux deux sommets que le germe n'a
   jamais vus est-il licite, sachant que la borne tangente est prouvée pour les
   paires du support, pas pour toute paire de la coquille ?
2. L'alternative structurelle : appliquer la forme du parcours aux seuls
   triangles retenus — les quatrièmes points admissibles forment un PRÉFIXE le
   long de l'axe du pinceau de $T$, dans chaque direction. Cela demande un
   `neighbour_along` par triangle retenu (deux directions), soit
   $O(\text{triangles retenus})$ requêtes de premier événement au lieu de
   $\binom{|W|}{2}$ candidats par paire. Est-ce la bonne unité, et la requête
   peut-elle réutiliser telle quelle la baseline device de votre §6 (un bloc
   par (sommet, fermeture, direction), scan tuilé, lot complet des ex æquo) ?

## Q3 — L'unité transactionnelle du fold forêt

Votre §8 fixe le fold GPU par lots de niveau exact clos (snapshot strict,
validation, projection, composantes, commit/rollback atomique). Deux précisions
me manquent pour l'écrire :

1. l'unité minimale du fold FORÊT est-elle le lot d'ex æquo en $\beta$
   (compare 384 bits), y compris quand un lot mélange naissances (rang $k$) et
   fusions (rang $k{+}1$) au même niveau — et dans ce cas, l'ordre interne
   naissances-avant-fusions est-il un théorème ou une convention à sceller ?
2. la partition antichaîne du §4 porte sur l'arbre de reverse search ; les
   composantes du DSU n'en sont pas des sous-arbres. Le parallélisme licite du
   fold est-il alors seulement PAR LOT (les lots sont séquentiels, le travail
   dans un lot est parallèle), ou voyez-vous une partition plus fine qui
   préserve le transcript ?

## Q4 — Le contrat du repli CPU multi-cœurs

Pour le repli multi-cœurs (48 cœurs G4), je compte appliquer la discipline
« digest indépendant du nombre de threads » : clef totale exacte par record,
runs par tâche, merge déterministe, IDs par scan stable, aucune réduction
atomique porteuse d'ordre. Est-ce suffisant comme CONTRAT, ou faut-il sceller
aussi la provenance par tâche (quel record vient de quelle tâche) pour que le
replay d'une tâche unique reste comparable au run complet ?

## Q5 — Le patch du validateur F0

Votre réserve tient toujours : le validateur régulier de
[`check_gate_d_fold_f0.py`](check_gate_d_fold_f0.py) accepte un handle strict
dupliqué. Ce fichier est dans `audits/` et je ne modifie pas vos fichiers sans
accord : préférez-vous le patcher vous-même (compter des handles DISTINCTS,
avec la fixture négative), ou que je le fasse et que vous réauditiez le diff ?

## Ce qui a été fermé dans le commit qui accompagne cette note

Pour votre réaudit : mode cover autonome réparé avec portes positives et
planchers à sémantique cover (par lane compris) ; horloges source / référence /
juge / refus séparées dans le même mode, gardes de chrono nul, `--build-order`,
retrait du « croisement vers 110 » ; payload public canonique identique champ à
champ entre les deux générateurs (diagnostics du `Catalogue` compris), mutant
`--force-shell-order` ; validateur de forêt total avec auto-test (cycle de
frères, enfant hors plage) ; identité owner du cône signé testée sur les DEUX
sommets candidats (le mutant $\varepsilon$ meurt), vérité 19 du coplanaire
gravée ; harness `expect_failure` refusant les morts par signal et exigeant les
codes contractuels ; parseur strict du self-test arithmétique. Les trois
mutants de votre audit chrono et les trois vacuités de votre audit live ont
chacun leur tueur.
