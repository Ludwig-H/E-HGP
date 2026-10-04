## 5. Sortie plate

**Factorisation** (synthèse du workflow, § 5.1, prouvée et contrôlée sur 29 692 et 13 133 blocs). Pour toute
pendaison fidèle $H$ et tout mcs, la règle $H^{\mathrm{mcs}}$ de même lignée et de date $\max(e_x,a_x)$, où $a_x$ est
le premier rayon où la lignée de $x$ couvre au moins mcs sites, a, à tout rayon, les mêmes blocs d'au moins mcs
points que $H$. « Projeter sans mcs, puis condenser » réalise donc exactement la proposition de l'utilisateur du
1er octobre (propriétaire sur la tour condensée), lue en « absorption » ; la lecture « admission stricte » est
réfutée par Q1bis et Q-Π2.

**Condensation.** Masses entières après engagement (un point compte pour 1) ; clusters = blocs d'au moins mcs
points. Les masses fractionnaires du § 9.1 de la thèse ($m_\tau=S_\tau\sum_{x\in\tau}1/T_x$) donnent à chaque
triangle de T0 une masse $8/3<3$ : ils ne seraient plus des clusters à mcs 3. Le critère d'**existence** d'un
cluster (Q-Π2 : un point de bord ne fait pas exister un cluster) relève de cet étage et reste ouvert.

**Sélection.** Hors de cette note ; même sélection pour toutes les hiérarchies comparées, HDBSCAN calculé sur la
même machine (l'ordre des ex æquo de `sklearn` en dépend).

**Complétion facultative**, après sélection, d'un point resté seul mais couvert par un cluster sélectionné : par
défaut le long de sa lignée (sans paramètre, sans réunion nouvelle) ; le vote de la thèse en bras déclaré, jamais
comme projection (par niveau il n'est pas laminaire ; figé, il est discontinu ; il dépend des faces construites et
de l'exposant $p$).
