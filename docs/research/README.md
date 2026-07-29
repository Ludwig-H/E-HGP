# Replis de recherche maintenus

La voie privilégiée reste unique : une passe multi-ordre résidente GPU construit des banques certifiées de $K_{\max}$ témoins dans les 48 chambres, utilise Morton/LBVH pour ordonner et élaguer, émet exactement une fois chaque paire survivante, décide son rang fermé et restitue son payload complet. Les témoins n'ont pas besoin d'être les plus proches; une fenêtre Morton peut les proposer sans rappel et toute insuffisance reste fail-open. Les triangles aigus puis les tétraèdres bien centrés ont ensuite leurs frontières indépendantes.

Trois replis seulement restent maintenus :

1. **LBVH exact sans cutoff Yao48.** Le même pipeline peut ignorer une chambre sous-remplie ou tout cutoff directionnel indisponible et descendre une frontière bloc--bloc conservatrice. Il est plus lent mais conserve la complétude; ce n'est pas un second backend.
2. **Oracle GPU dense borné.** Un scan tuilé de toutes les paires contre tous les points sert au différentiel, aux égalités de coque et aux petits nuages. Ses caps interdisent qu'il devienne le produit ou qu'il soit lancé sur 50 000 points.
3. **Oracles CPU de preuve.** Gamma exhaustif et la [tour des boules saturées](../math/TOUR_BOULES_SATUREES.md) contrôlent les petites fixtures, les incidences silencieuses et la réduction hiérarchique. Ils restent indépendants du producteur.

Le repli numérique — filtre flottant, expansion exacte GPU, puis file rare multiprécision CPU — appartient à la voie principale. PDEL, Geogram, un préfixe Morton fixe, un ANN, une grille ou un catalogue Delaunay ne sont pas des replis autorisés; ils figurent dans les [archives](../archive/abandoned/README.md).
