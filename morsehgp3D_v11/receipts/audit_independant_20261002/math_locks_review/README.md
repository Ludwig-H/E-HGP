# Témoins exacts pour les questions moteur Q1–Q4

Capture de [L02](sources/L02_MATH_TOUR.md) et des [questions](sources/QUESTION_CLAUDE.md)
avant tests ; [empreintes initiales](SOURCE_BEFORE.json), [recoupe](SOURCE_AFTER.json).
Programme autonome en Fraction, petits cas plans, aucune référence ni bibliothèque
produit importée. MEB par supports exhaustifs de taille 1 à 3 ; Γ par toutes
les unions de deux k-parties, sans heuristique. Un exemple binary64 nearest est
également exécuté. Les deux modes calculent depuis une copie `/tmp`.

[check.py](check.py), [normal](normal.json) et [−O](optimized.json) :
**159 contrôles identiques**, dont 19 coupes d'Euler. [RUN.json](RUN.json).

- Q1 : trois points 0,2,4, K2 : deux naissances β=1 possibles pour la descente
  des extrêmes β=4. Avec un quatrième point (2,3), les deux morceaux locaux
  sont déjà reliés globalement par des triangles de β=13/4<4.
- Q2 : x=32767, A=512x³, expression (A+1)−A−1 : exact 0, binary64 nearest −1.
  Le degré trois et l'entrée quinze bits seuls ne prouvent pas F2.
- Q3 : X={(0,5),(8,9),(8,1),(35,5),(45,5)}, K1. À β=25, la sphère des trois
  premiers points a poids d'Euler +1 ; celle des deux derniers −1. Retirer les
  deux groupes conserve toute la courbe d'Euler, mais supprimer la paire
  retarde la fusion des deux derniers sites : trois composantes au lieu de deux.
  Catalogue élargi d'Euler et graphe Γ sont calculés exactement sur ces cinq
  points u18. **Aucune amputation d'un catalogue produit n'est exécutée**.
- Q4 : reflection x→4−x fixe le point médian et échange les deux composantes
  admissibles cover à β=1. Un singleton choisi ne peut être équivariant sous
  cette symétrie ; le set à deux éléments l'est.

Ces preuves bornées n'établissent ni complétude générale du générateur, ni
borne de coût ou défaut d'un moteur. Le témoin Q3 réfute seulement le pouvoir
de certification universel du scalaire Euler, même observé à tous les niveaux.
[SHA256SUMS](SHA256SUMS) ferme les copies, code et flux.
