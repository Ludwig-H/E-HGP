# q3 différé — raccord MEB borné, 4 octobre 2026

Source Git figée **56216392e**, comparée à **17514012b**. Lecture indépendante ciblée de MEB ; catalogue/API numérique/portes mutantes relus séparément par l'auditeur geometry_catalogue. Cadre exploration_v11_hors_registre / cpu_reference / profils18,21,24 déclarés / not_claimed. Aucun moteur, natif, build, fit, GCP ou fichier produit/audit modifié.

**Conclusion : aucun défaut matériel établi sur ce raccord.** Le calcul du Level q3 est retardé jusqu'à l'inclusion complète, tout en conservant les certificats, le gagnant et les sept compteurs logiques de MEB. Cela ne constitue ni une qualification native ni une mesure du gain.

## Invariants relus

- `meb.cpp:107–123` : classification dégénéré/non_strict/strict inchangée ; seul le strict construit Q3Candidate. Strict implique affine indépendant, donc l'absence de candidat devient un refus d'invariant, pas un rejet géométrique silencieux.
- `contains:68–76` teste tous les points de la partie dans le même ordre, conserve side=0 et s'arrête au premier extérieur. Le candidat possède le même centre et le même tag3/certificats que la Sphere ; le raccord ne le retag pas q4.
- Après containment vrai seulement, `materialize` rend la Sphere exacte, puis `accept:80–87` conserve ancre/support local, padding kNone et arité. Les parties sont copiées/triées/validées avant recherche ; aucun SiteIdx externe/Cloud étranger n'est introduit.
- `consider:126–141` conserve exactement presentations, nondegenerate, positive, point_tests, containing, comparisons=0 et diamètre. Les incréments dénombrent les candidats/tests logiques, pas les constructions de Level. Les bornes existantes n≤12 restent inchangées.
- `extend:144–150`, puis `bounded_meb:157–174` : diamètre canonique d'abord, tous q3 puis q4 lexicographiques, arrêt au premier support strict contenant. Un rejet q3 ne coupe jamais un préfixe q4 ; cas affines/obtus toujours traités par la recherche complète restante.
- `sphere.cpp:34–72` : Q3Candidate conserve les trois Point, N/D et les certificats. La matérialisation refait **|u|²|v|²|c−b|²/(4|u×v|²)**, même ordre des produits et aucun PGCD/réancrage ; D=2g donc le dénominateur écrit2D est bien4g. La factory publique Sphere::through3 continue de matérialiser même les triangles droits/obtus non dégénérés.
- Refus de factory, side et materialize propagés ; aucune allocation/cache/vue empruntée nouvelle. Les capacités/localité du MEB restent1..12, la coquille globale n'est pas tronquée par ce port.

## Contrôles autonomes bornés

`check.py`, stdlib Fraction/Gram/Gauss : **1344 gardes**, normal/−O identiques. Trois profils18/21/24, deux homothéties, cinq triplets (aigu, droit, obtus, aligné, plan oblique), six permutations : dégénérescence, centre Gram indépendant, rayon brut de degré6, positivité barycentrique et signe de puissance avant matérialisation concordent.

Le tétraèdre0/2 fournit une garde non vide : quatre faces q3 strictes rejetées par le sommet opposé, donc quatre Level q3 évités ; diamètre puis q3/q4 donnent exactement **6 présentations,17 tests de points**, gagnant q4 centre(1,1,1),β3. Le gagnant q3(0,0),(4,0),(2,3) garde le niveau brut **2704/576**, pas169/36 réduit.

La porte C++ `meb_deferred_test.cpp` compare l'ancienne structure de recherche eager, ses compteurs, supports et champs bruts avec le nouveau MEB. Elle partage cependant désormais Sphere::through3/Q3Candidate pour l'arithmétique : c'est un différentiel de chemin, pas à elle seule un oracle arithmétique indépendant. Le juge Gram déjà prévu et les portes numériques séparées gardent ce rôle. Nous n'avons exécuté aucune de ces portes natives.

Les sources Git sont revérifiées à la clôture. Certaines demandes de noms initiales étaient absentes et restent explicitement marquées dans SOURCES.json ; aucun fichier inventé ou LIVE/WIP ne remplace un objet Git. Les anciennes capsules restent intactes.
