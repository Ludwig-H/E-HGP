# MEB : différer les constructions inutiles

Le port conserve l'énumération du diamètre puis des q3/q4 stricts, leur ordre
canonique et les sept compteurs de `MebLedger`. Ces compteurs dénombrent des
candidats logiques et des tests de points, pas les constructions de Sphere
ou de Level. Aucun gain natif n'est encore qualifié ; graph4 exclut ce port.

## q3 avant le centre

`classify_triangle` distingue dégénéré, non strict et strict. Trois angles
strictement aigus garantissent l'indépendance affine ; sinon le produit
vectoriel nul distingue les dégénérés des triangles droits/obtus.
Les différences, produits scalaires et composantes du produit vectoriel
restent i64 aux profils u18/u21/u24 (B≤24). Aucune norme carrée du produit
vectoriel n'est calculée en i64. La construction générique de Sphere garde
son domaine, y compris les triangles droits et obtus.

La recherche compte les présentations dégénérées et non strictes exactement
comme avant, puis construit seulement les q3 stricts. Le rejet q3 ne coupe
jamais une extension q4.

## q4 après inclusion

Le candidat porte ses coefficients exacts et la positivité de sa présentation
originale. Après positivité, les points de toute la partie sont testés dans
le même ordre, avec arrêt au premier extérieur. Le niveau exact n'est
matérialisé qu'après inclusion complète ; la Sphere produite garde les mêmes
numérateurs et dénominateurs bruts. Cette réduction ne modifie aucune règle
de support, de coquille ou de départage des égalités.

## Preuves préparatoires et qualification attendue

Les modèles indépendants Gram/Fraction passent normal/−O :852 requêtes,
1717 contrôles et72 corruptions pour q3 ;54 requêtes,655 contrôles et540
corruptions pour MEB. Le premier modèle MEB n'exerçait pas q3 dégénéré :
une fixture avec trois points alignés a été ajoutée avant gel. Le modèle
exerce aussi q4 positif avec un point extérieur, donc le report du Level.
Ses30 contre402 matérialisations sur ce petit lot ne sont pas un chrono.

La parité native préparée compare l'ancien chemin eager, les sept compteurs,
les supports globaux, et chaque entier brut séparément. Les planchers sont
240 et60 pour q3,438 pour MEB. Trois mutants historiques sont réancrés sans
changer leur cause ; trois mutants supplémentaires visent la classification
q3 et le refus q4 non strict. Compilation, sanitizers et morts causales des
mutants restent à établir dans une session G4 gardée.
