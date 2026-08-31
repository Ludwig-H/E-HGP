# Pistes fermées — mémo v6 (append-only)

Hérite intégralement de `morsehgp3D_v5/docs/PISTES_FERMEES.md` (lui-même
héritier v3/v4) ; ne sont recopiées ici que les entrées qui contraignent
directement la conception v6, plus les entrées propres v6. Règle de
réouverture : nouveau théorème de complétude + fixture qui falsifie le motif
d'abandon sans casser les contre-exemples ; jamais un benchmark.

## Contraintes directes sur la génération q3/q4

1. **Décomposition ternaire symétrique** fortement séparée exact-once comme
   source q3 linéaire — fermée (Théorème 4 v3, famille cercle–axe, Ω(n²)
   blocs). Survivantes explicitement ouvertes : WSSD approximatives, sources
   asymétriques ancre–tiers, restriction par profondeur, arrangement local de
   centres.
2. **Cap de population dans le critère terminal WSPD** — fermée (force
   ≥ C(n,2)/C² rectangles). Mutant `wspd-cap-terminal`.
3. **Scission du facteur le plus peuplé** — fermée (mesure −14,7 % en
   scindant le diamètre ; invariant d'empilement). Mutant
   `wspd-split-heaviest`.
4. **Deux arbres spatiaux** — fermée. Un seul arbre radix.
5. **Source kNN à préfixe borné pour les ancres** — fermée (rang de voisinage
   non borné, fixture 50 000 points).
6. **Couches convexes pour le scan q3** — fermée (mesure).
7. **Cover q4 coefficient 3 à la génération** — la fermeture v5 (qui
   IMPOSAIT le coefficient 3 pour la conformité `digest_balls` v4) est
   SUPERSÉDÉE par le P0 v6 du 31 août : le coefficient 3 perd des témoins
   intérieurs q4 (contre-fixture au tétraèdre régulier + z, gravée dans
   `mhgp6_cover_coef4`). Contrat v6 : coefficient 3 pour q3 (sharp), 4 pour
   q4 ; les monnaies de candidats sont gelées séparément de l'objet ; mutant
   `q4-cover-coef3` (l'inverse du mutant v5).
8. **Sélection axiale bornée** — fermée (opt-in négatif mesuré).
9. **Cœur universel de Jung sur arête maximale comme source** — fermée (pire
   cas quadratique) ; survivent les relaxations `3U < L` / `2H² > Ξ`.
10. **Boule d'apex unique pour h_a** — fermée (plus lente + P0 du signe) ;
    la piste **auto-jointure dual-tree à range-add reste ACTIVE** (route M).
11. **Parcours global de l'arrangement relevé (BFS/GPU)** — fermée v3
    (énoncés fondateurs faux hors position simple ; volume quadratique là où
    la sortie est linéaire). Le moteur plan E6 est **local par ancre** et
    conditionnel ; il ne rouvre pas cette piste globale.
12. **Mosaïque de Delaunay d'ordre K / catalogue ∝ C(n,k) comme produit** —
    interdit d'architecture.
13. **Fenêtre Morton fixe comme autorité exhaustive** — fermée ; Morton reste
    une clé de tri et de localité.

## Entrées propres v6

14. **Certificat de cellule de bloc par max de deux concaves aux sommets**
    (conception D2-B3 du panel du 31 août) — fermée avant code : le max de
    deux fonctions concaves n'est pas concave, « positif aux sommets » ne
    s'étend pas à l'intérieur ⟹ fausses morts. Survit : le certificat
    **unilatéral** (un même côté certifie les huit sommets, verdict = OU des
    deux tests), forme retenue pour Tier R.
15. **Requête de fuseau saturée avec crédits soustraits du seuil**
    (conception D5-E3) — fermée avant code : les témoins crédités sont dans
    W_q, le compte plein les revaudrait deux fois ⟹ fausses morts. Survit :
    la discipline `ResidualTape` (exclusions par identité et par lane) ou le
    compte contre h_q nu.

## Patterns d'erreur reconduits (v5 § F)

1 promettre avant de mesurer ; 2 deux bornes divergentes ; 3 un cap dans un
critère terminal ; 4 un témoin partagé sujet/juge ; 5 un statut déclaré au
lieu d'une mesure ; 6 un digest qui mesure un filtre ; 7 un vert par
vacuité ; 8 un mutant hors de sa porte. Plus : la porte vacueuse (code de
retour ignoré) et le certificat qui coûte plus qu'il ne rapporte (gain mesuré
apparié contre exécution désarmée, jamais déclaré).
