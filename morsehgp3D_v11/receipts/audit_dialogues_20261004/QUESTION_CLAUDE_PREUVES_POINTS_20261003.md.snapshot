# Questions du développeur : preuves manquantes pour la hiérarchie de points $H^{r}_{k+1}$

3 octobre 2026, 22 h 45 UTC (Claude, développeur), sur demande de l'utilisateur : « Tu peux demander à
l'auditeur des preuves mathématiques manquantes ». Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

Merci pour la contrelecture [hm_review](../receipts/hm_review_20261003/README.md) (a23e3919d) : ses trois
corrections sont faites (règle en rayon seule retenue ; égalités de radicaux certifiées par classes de carrés avec
refus explicite ; filtres flottants bornés en erreur absolue, y compris la présélection des rivaux) et ses deux
témoins exacts sont des fixtures de la porte. Les cohortes sont désormais publiées séparément (démos, échecs
dédoublonnés par empreinte des sites, voisines, témoins). Une session de requalification G4 (`claudepts4`) tourne
avec ce code ; sa réponse viendra dans le commit de livraison.

**État de travail cité** (arbre `build/v11-claude-20261003`, non encore commité, empreintes SHA-256) :
`docs/HIERARCHIE_POINTS.md` 4b2bdb8a, `bench/points_radius.py` 58952a8b, `bench/points_hierarchy.py` 0da8fce4,
`bench/points_reference.py` bd05bc2b, `bench/points_gate.py` eb467b91. Rapports du workflow `wf_92a63749-ee7` :
`build/v11-points-math/{marge_cibles,majorite_vote,fermeture,axiomes}/RAPPORT.md` et leurs vérifications
(`verif_*`), à verser dans un reçu.

**Règle.** Pendaison fidèle à $k$ fixé ; qualification $\Pi_{k+1}$ (au moins deux $(k-1)$-simplexes) ; en rayon,
$e_i=t_i+\sup_q(m(p_i,q)-h(q))$, propriétaire = ancêtre de la première lignée qualifiée vivant à $e_i$
(ancrage persistant $P_1$ de la v10 appliqué au profil qualifié). L'utilisateur a fait primer le modèle
mathématique sur les cibles Q2/Q3 du catalogue v10.

## Q1. Stabilité de bout en bout, hypothèses exactes

Acquis : sous un entrelacement transportant les couvertures qualifiées, dates et réunions bougent d'au plus
$3\delta$ (votre preuve et la proposition D du workflow). Manque : la chaîne complète depuis la géométrie, avec
ses hypothèses écrites une fois — P5 (sites distincts, poids un, identifiants appariés, déplacement au plus
$\varepsilon$) fournit-il bien un $\varepsilon$-entrelacement **de l'arbre FULL vu comme espace** qui transporte
$R_i^{(k+1)}$ dans les deux sens, coupes fermées et plateaux N-aires compris ? Que devient l'énoncé avec des
retours multiples (poids), une insertion ou une suppression de point ? Forme souhaitée : preuve, et une fixture
d'égalité si la constante 3 est atteinte.

## Q2. Optimalité avec qualification

Le théorème C du workflow (rapport `axiomes`, § 3) montre, pour les règles locales au profil couvrant **brut** :
aucune constante inférieure à 3, et $P_1$ est la plus précoce de constante 3 parmi les règles monotones. Manque :
la même chose pour les règles qui ne voient que le profil **qualifié** $\Pi_m(R_i)$. Est-ce une conséquence directe
(le profil qualifié est encore un profil transporté), ou la qualification casse-t-elle un lemme (par exemple
l'entrelacement explicite C1 (ii), qui suppose qu'une branche seule peut être isolée) ? Preuve ou contre-exemple.

## Q3. Borne de retard sous qualification

Pour $P_1$, la v10 donne $e-\alpha\leq d_k/2$. Pour $H^{r}_{k+1}$, $t_i$ est la première couverture **qualifiée**
et les rivaux sont qualifiés. Manque : une borne de $e_i-t_i$, et une comparaison de $e_i$ avec le temps de cœur
$d_k(x_i)$, avec la condition exacte sous laquelle un site entre après son cœur (le rapport `axiomes` indique que
cela n'arrive que si toutes les composantes couvrantes ont au plus $k$ sites ; à prouver tel quel pour la version
en rayon qualifiée).

## Q4. Chapitre 7 de la thèse, asymptotique

Le théorème 3 de la thèse mesure la fraction d'un amas dense récupérée avant la fusion parasite, en sémantique de
couverture $\Theta^{poly}$. Manque : une preuve que $H^{r}_{k+1}$ récupère asymptotiquement la même fraction que
FULL (retards et qualification sans coût asymptotique), et, de l'autre côté, l'énoncé asymptotique exact pour la
fermeture qualifiée (à $n$ fini, le rapport `fermeture` montre qu'elle peut faire mieux, 1 contre 1/2, et qu'elle
réunit au plus un facteur 2 en rayon trop tôt).

## Q5. Compatibilité verticale entre ordres

À même rayon $r$, chaque bloc de $H^{r}_{k+1}$ à l'ordre $k$ est-il inclus dans un bloc de $H^{r}_{k}$ à l'ordre
$k-1$ ? Les composantes FULL s'envoient verticalement, mais propriétaires, qualifications ($k+1$ contre $k$) et
retards diffèrent. Preuve, ou contre-exemple minimal (la fixture P4 croisée de MATHEMATIQUES.md est un point de
départ). C'est le premier pas vers la synthèse multi-$k$.

## Q6. Cibles contre stabilité : impossibilité générale ?

Chaque règle connue tombe d'un côté : la famille $H_m$ échoue Q1bis ou Q2 (théorème F), ER0h passe les 125 cibles
sans borne uniforme (proposition S : rapport au moins $\tfrac{64\kappa}{129}\rho^{2}-1$). Manque : un énoncé
général — aucune règle fidèle, équivariante, uniformément lipschitzienne ne réalise à la fois T0/Q1bis et Q2 — ou
une construction qui le fait. Une preuve dans un sens ou dans l'autre clôt la question pour l'utilisateur.

## Q7. Vérification indépendante des résultats nouveaux du workflow

Les théorèmes B (anticipation obligatoire), C (front exact), E (T0 contre monotonie), F (seuil contre cibles), les
propositions D (hauteurs $3\delta$) et S (ER0h sans borne uniforme, contre le verdict v10) ont été vérifiés par un
agent adverse du même workflow seulement. Une relecture indépendante de C et de S suffirait en priorité.

## Q8. Contrat d'un port natif

Une date de point est $\sqrt{t}+\sqrt{m}-\sqrt{q}$ (trois niveaux du catalogue). Manque : les budgets de bits des
comparaisons exactes (deux contre deux par élévations au carré ; test de classe de carrés), un type nommé pour ces
dates et leur ordre total commun avec les niveaux FULL dans les plateaux fermés, comme vous l'aviez demandé.
