# L10 audits registry - notes de travail (28 sept. 2026), base ce8a649dd (worktree v9-open-worktree)

Mesures faites ici (lecture seule du depot, nice -n 19, <=2 fils) :
1. oracle2d.py / oracle2d.json (sha256 415cfb3e..., b5987a98...) : HDBSCAN sklearn 1.9.1, 5 graines du plan
   (2026092800..04), n=2000 g8 : oracle 1D du banc (min_samples=None=min_cluster_size, EOM, grille 8 mcs)
   contre oracle 2D (min_samples in {None,1,2,3,5,10} x {eom,leaf}) :
   spherical medium 0.7683 -> 0.8966 (ms=1/eom) ; spherical hard 0.3799 -> 0.7536 (ms=1/leaf) ;
   filaments medium 0.8563 -> 0.9234 ; hierarchical medium 0.9482 -> 1.0000 ; shells medium 1.0 -> 1.0.
   Le 0.7683 reproduit exactement la moyenne des 5 lignes hdbscan_oracle du recu synthetic_bench_20260928/r1.
2. plateau_probe.py : cluster.merge_tree sur 3 cofaces K=1 de meme niveau (0,1),(2,3),(1,3) -> deux noeuds
   de meme niveau (cascade) et racine rendue 'n0' qui ne contient pas la facette (3,) : masse perdue.
   plateau_real.py sur exports K=2 n=2000 de la session precedente : 0 cascade, 0 facette perdue (defaut latent).
3. comps.py : composantes du graphe de facettes Gabriel (cda636b5e) : spherical n2000 K5 : 435 (gabriel) / 62
   (boundary) ; hierarchical K3 : 102/32 ; hierarchical K2 : 19/9 ; spherical K2 : 11/2. FULL : 1 racine par ordre.
4. shatter_probe.py : condense() ne termine pas un cluster quand tous ses enfants sont sous le seuil ;
   les facettes tombent a leur naissance (lambda 2) au lieu du niveau de rupture (lambda 1/2) : stabilite 6 au lieu de 0.
5. Recu synthetic_bench r1 : table "Groupes" g8 = 0.527/0.683 moyenne les axes de bruit 0/0.1/0.3 (15 runs) ;
   la scene g8 sans bruit vaut 0.396/0.768 ; defaut 0.73 a la calibration (graines 11-15) mais 0.396 sur les graines du plan.
Audit geant du 28 sept. (session b64b3f68, tasks/wnuhkauke.output, 39 agents) : extrait dans audit_20260928_extract.txt.
