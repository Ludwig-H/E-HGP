# Conception S4 : voies q3/q4 des arêtes vivantes sur GPU (23 septembre 2026)

Trois rapports d'un workflow de conception à trois agents, en lecture seule
(rien compilé ni mesuré) :

- [carte des voies q3/q4](CARTE_VOIES_Q34.md) : cover, atlas, graines et
  recensement q3, balayage q4, prédicats, mémoire et compteurs ;
- [analyse d'architecture GPU](ANALYSE_ARCHITECTURE_GPU.md) : ce qui se porte
  comme S1 et S3 et ce qui doit d'abord être restructuré ;
- [plan par étapes](PLAN_S4.md) :
  - S4.0 : session d'appareil résidente ;
  - S4a : voie q3 sans atlas, une voie par graine sur le cover ;
  - S4b : voie q4 par la fenêtre exacte sur le cover, derrière une porte de
    coût ;
  - S4b′ : repli sur l'atlas aplati puis porté.

**Verdict du plan.** À 08/000000/K5, S4 ramènerait la chaîne de 2,20 s vers
1,55 à 1,65 s, et vers 1,3 à 1,4 s avec les autres leviers. La seconde
n'est pas atteinte sans alléger aussi la tour. Ce sont des projections, pas
des reçus.
