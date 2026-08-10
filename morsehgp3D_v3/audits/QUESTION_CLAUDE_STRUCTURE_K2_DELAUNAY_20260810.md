# Question de Claude — l'invariant structurel k=2 par triangles Delaunay

Date : 10 août 2026 UTC. Suite du vérificateur k=1 livré.

## Ce qui est reçu pour k=1 (à réauditer)

THÉORÈME (k=1 == single-linkage, insensible à la troncature). Direct : deux
points d'une boule de niveau L sont à distance <= 2r, donc single-linkage les
merge au plus à L. Réciproque : toute arête EMST est une arête de GABRIEL
(boule diamétrale vide) — son saturé a RANG 2, jamais censuré par `smax` — et
son générateur fusionne les deux composantes exactement à (d/2)². Vérifié :
`mhgp3v_structural_scale_check` compare le multiset des niveaux de fusion k=1
(pondérés arité-1) au multiset des (d_e/2)² de l'EMST exact (Prim i128,
niveaux par `sphere2`+`sphere_cmp_beta`) — égalité 199/199 à n=200, mutant de
décalage tué, portes permanentes. Praticable à 50 k (Prim O(n²) ~ minutes).

## La question k=2 (suggestion de Louis à formaliser)

Louis propose : vérifier que toutes les fusions k=2 sont à un niveau inférieur
aux « triangles formés de deux arêtes Delaunay ». Je cherche l'énoncé exact à
recevoir :

1. **Candidat faible (borne globale)** : pour tout couple de composantes k=2
   qui fusionnent, il existe un triangle T = {a,b,c} avec {a,b} et {b,c}
   arêtes Delaunay (ou Gabriel ?), un sommet dans chaque composante, tel que
   la fusion a lieu à un niveau <= niveau(miniball(T)). Vrai ? Sous quelle
   forme la troncature le préserve-t-elle (le saturé de miniball(T) peut
   dépasser smax — quel est l'analogue « rang jamais censuré » des Gabriel ?
   les triangles de DEUX ARÊTES GABRIEL ADJACENTES ont-ils un saturé borné ?).
2. **Candidat fort (égalité)** : existe-t-il un graphe calculable à l'échelle
   (l'analogue EMST d'ordre 2 — Gabriel-2 ? RNG-2 ? les triangles « deux
   arêtes Delaunay » de la thèse ?) dont le dendrogramme ÉGALE la forêt k=2,
   troncature comprise — comme l'EMST pour k=1 ? Ma piste : les fusions k=2
   passent par des paires partagées ; l'arête H_2 (M,N) exige |M∩N|>=2 ; le
   « pont » minimal entre deux composantes serait un TRIANGLE {a,b,c} avec
   {a,b} d'un côté, {b,c} de l'autre, au niveau miniball{a,b,c} — le graphe
   des ponts triangulaires minimaux joue-t-il le rôle de l'EMST, et son
   calcul exact à 50 k est-il praticable sans mosaïque (O(n²·votes) ?) ?
3. Si l'égalité est hors de portée : quel jeu MINIMAL de certificats
   d'inégalité (borne par triangle témoin par fusion) rend la vérification
   k=2 falsifiable à 50 k, et quel mutant la mord ?

GCP non utilisé pour cette note.
