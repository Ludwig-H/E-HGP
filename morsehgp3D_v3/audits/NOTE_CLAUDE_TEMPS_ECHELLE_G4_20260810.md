# Note de Claude — les temps à l'échelle, CPU et GPU sur le même hôte

Date : 10 août 2026 UTC. Cadre : `phase=exploration_v3_hors_registre`, famille
TRONQUÉE `smax=11`/K=5 (`partial_refinement` déclaré — décision de Louis : pas
de famille certifiée exigée à 50 k ; l'exactitude reste jugée aux tailles
bornées et par vérifications STRUCTURELLES à l'échelle, voir §3). Session G4
gardée (troisième), VM certifiée TERMINATED, clé révoquée et supprimée.

## 1. La table des temps (g4-standard-48 + RTX PRO 6000, graine 20260810)

| n | générateurs | incidences K=5 | catalogue CPU (1 cœur) | fold CPU | join device | différentiel |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 800 | ~271 k | ~106 M | ~55 s | 21,5 s | **115,7 ms** | identiques ×5 |
| 1600 | 534 142 | 242 746 830 | 134,4 s | 53,4 s | **282,9 ms** | identiques ×5 |
| 2400 | 845 752 | 386 648 099 | 236,5 s | 86,7 s | **454,3 ms** | identiques ×5 |

Périmètres : « join device » = émission+tri+owner+étoiles (hors H2D/D2H/rejeu,
réception `23379d4`) ; « fold CPU » = fold face-owner complet. Le flux
d'arêtes device est identique au CPU arête par arête sur chaque ordre de
chaque taille. Débit device ≈ 850 M incidences/s, linéaire en I ; pic modèle
8,2 Go à n=2400 (k=5).

## 2. Extrapolation 50 k (`forecast_only`, exposant ~1,6 mesuré sur 800→2400)

- catalogue CPU un cœur : ~8-9 h — LE poste dominant (parallélisation = le
  levier ; le générateur est par graines indépendantes) ;
- fold CPU : ~3 h ; **join device : ~1 minute** — mais I(50k) ~ 5×10^10 exige
  les RUNS BORNÉS (pic modèle ~2,6 To ≫ 95 Go VRAM) : le découpage par
  intervalles de générateurs, déjà spécifié par l'audit, devient obligatoire ;
- rejeu DSU hôte : petit devant le reste (unions ~ dizaines de millions).

## 3. L'exactitude à l'échelle : vérifications structurelles (plan reçu de Louis)

Aux tailles sans vérité exhaustive, l'exactitude se contrôle par invariants :

1. **k=1 — ÉGALITÉ avec le single-linkage.** Proposition (à faire recevoir par
   l'auditeur) : Γ_1 est exactement le single-linkage — sommets = singletons
   (niveau 0), arêtes = paires au niveau (d/2)² — donc la forêt k=1 du fold
   doit ÉGALER le dendrogramme EMST, calculable exactement à 50 k par
   `exact_lbvh_yao48_emst.hpp` (phase 15). Égalité niveau à niveau, pas une
   borne.
2. **k=2 — borne Delaunay.** Chaque fusion k=2 doit être à un niveau inférieur
   ou égal au niveau du triangle formé de deux arêtes Delaunay adjacentes qui
   la témoigne (suggestion de Louis) — un certificat d'inégalité par fusion.
3. Identités déjà internes : binomiales==incidences, masses par ordre,
   `P_post`==prédit, garde d'événement.

GCP : troisième session gardée, génération courte, TERMINATED certifié.
