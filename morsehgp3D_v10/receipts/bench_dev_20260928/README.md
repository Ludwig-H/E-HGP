# Reçu : campagne de DÉVELOPPEMENT du banc contre HDBSCAN (28 septembre 2026)

Graines de l'espace `dev` seulement (dérivées par SHA-256 de la spécification ; l'espace `test` n'a jamais servi).
Aucune conclusion de test : ce reçu sert à choisir, pas à revendiquer.

## Rejouer

- Code : commit `93710d076` (binaire figé construit depuis `git archive`), `bench/synthetic/run_campaign.py` du même
  commit : `python3 run_campaign.py --build <build> --out dev_r2.csv --split dev --sizes 2000,8000 --replicates 2
  --jobs 3 --threads 2`.
- Expérience de sélection par scène : commit `bdfa2dc19`, `adaptive_dev.py --sizes 2000 --jobs 2 --threads 2`.
- Hôte : codespace de 8 cœurs partagé et chargé ; les temps ne comptent pas, seuls les scores comptent.
- Métrique principale ARI_s (bruit vrai et prédit en singletons). sklearn 1.9.1, `algorithm="kd_tree"`.

## Plan

8 familles du banc v9 (`spherical`, `anisotropic`, `heteroscedastic`, `unbalanced`, `shells`, `bridge`,
`hierarchical`, `filaments`) × 4 niveaux × bruit {0 ; 0,1} × n {2 000 ; 8 000} × 2 graines = 256 unités, 8 groupes.
Tour : K ∈ {1..5}, mcs ∈ {10, 20, 50, √n}, z ∈ {1, ẑ}, EOM ou feuilles, remplissage aucun ou complet (160 configurations).
HDBSCAN (sklearn, tel quel) : min_samples ∈ {1, 2, 3, 5, 8, 12, 20}, mêmes mcs, EOM ou feuilles, α ∈ {1, 2}, même
remplissage (224 configurations) ; plus HDBSCAN par défaut. Aucun refus de la tour.

## Résultats (moyenne d'ARI_s sur les 256 unités)

| Méthode | Configuration unique choisie sur dev | ARI_s | Oracle par scène |
| --- | --- | ---: | ---: |
| tour v10 | K = 1, mcs = √n, z = ẑ, EOM, remplissage complet | 0,7444 | 0,8266 |
| HDBSCAN réglé sur dev | min_samples = 20, mcs = √n, feuilles, α = 1, remplissage complet | 0,7444 | 0,8592 |
| HDBSCAN par défaut | mcs = 5 | 0,5563 | 0,5692 |

Par famille, configuration globale de chacun : la tour gagne sur `shells` (0,941 contre 0,822), `unbalanced`
(0,720 contre 0,652), `heteroscedastic` (0,619 contre 0,591), `hierarchical` (0,501 contre 0,488) ; elle perd sur
`filaments` (0,639 contre 0,778), `anisotropic` (0,787 contre 0,840), `bridge` (0,863 contre 0,882), `spherical`
(0,884 contre 0,900). À n = 8 000 : 0,743 contre 0,735 ; à n = 2 000 : 0,745 contre 0,754.

Lecture : **égalité** globale avec un HDBSCAN réglé sur les mêmes graines, **large victoire** sur HDBSCAN par défaut.
La meilleure configuration de la tour est d'ordre 1 : le gain vient de la tête (échelle de densité λ = r^(−ẑ)),
pas de la géométrie de la tour. L'oracle par scène de HDBSCAN est plus haut parce que sa grille monte à
min_samples = 20 quand celle de la tour s'arrêtait à K = 5 : `hierarchical` (0,600 contre 0,828) le montre.

Sélection par scène sans étiquettes (n = 2 000, 128 unités, mcs = √n, K ∈ {1..5}, α = 1 seulement) : DBCV
choisit mieux que la meilleure configuration fixe, pour la tour (0,752 contre 0,746) comme pour HDBSCAN
(0,742 contre 0,722) ; la stabilité inter-K n'aide pas.

Suite : grille de la tour jusqu'à K = 10, sélection DBCV symétrique (HDBSCAN avec α ∈ {1, 2}), puis
préenregistrement et campagne de test 8k/16k/32k sur G4.
