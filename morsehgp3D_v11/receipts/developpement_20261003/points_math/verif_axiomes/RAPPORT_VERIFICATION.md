# Vérification adverse du rapport « Axiomatique, impossibilités et caractérisation »

3 octobre 2026, de 21 h 55 à 22 h 27 UTC (heures lues par `date -u`). Label `verif_axiomes`. Rapport vérifié :
`build/v11-points-math/axiomes/RAPPORT.md`, écrit à 21 h 53.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle exact Python ; implantation indépendante vfull.py)
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git, aucun natif. Écritures : build/v11-points-math/verif_axiomes/ seulement.
```

## 1. Méthode

Le code du rapport (`hk.py`, `e*.py`) n'est pas réutilisé. `vfull.py` recode tout de façon indépendante :

- boules minimales exactes en `Fraction` (supports d'au plus 4 points) ;
- Γ_k et arbre de Kruskal binaire, lu en coupes fermées ;
- règles P_κ∘Π_m (rayon, `mpmath` à 60 chiffres), Q_κ∘Π_m (niveau carré exact), core, cover et first (LCA), fermeture qualifiée.

L'oracle du dépôt (`hgp11_ref`, `points_reference`) sert de seconde route. Toutes les preuves du rapport ont été
relues ligne à ligne : théorèmes A, B, C (lemme C1 et points a à d), E et F ; propositions D, H et I. Les
entrelacements du lemme C1 et de la constante atteinte ont été reconstruits à la main.

**Recoupement préalable** (`c0_oracle.py 20261007 400`, 29 s) : 400 nuages, 42 835 comparaisons exactes des
ultramétriques. Les règles margin1 (= Q_1∘Π_1), margin (= Q_1∘Π_{k+1}), core, cover et first donnent 0 écart.
En rayon, P_1∘Π_m contre `reference_radius_rules` donne aussi 0 écart sur 17 134 paires.

## 2. Ce qui est réfuté

### R1. « Sans objet pour la qualification » (RAPPORT § 0 point 7 et § 7 point 3) : faux

L'énoncé du rapport : les retards de H_{k+1} « portent sur des groupes d'au plus k sites, hors de la composante
géante ». Il est repris tel quel, marqué « prouvé », dans la garantie H5 de `HIERARCHIE_POINTS.md` (version de
22 h 01).

**Contre-exemple exact.** K = 2, 8 sites : x (0,0,0), y (10,0,0), s2 (20,0,0), s3 (30,0,0), b1 (44,0,0),
b2 (54,0,0), w1 (−20,10,0), w2 (−20,−10,0).

- Au rayon F = 12, le nœud FULL de {x, y, s2, s3} (né à 10) fusionne avec la paire de fond b1b2 : c'est une vraie
  fusion parasite. Coupe fermée de l'oracle à 144 : composante {x, y, s2, s3, b1, b2}.
- x est un point cœur de cette composante : d_2 = 10 ≤ 12, α = 5, α + d_2/2 = 10 ≤ F.
- P_1∘Π_1 fait entrer x à 9,631 : bloc {x, y, s2, s3, b1, b2} à F.
- H_3 n'a pas encore fait entrer x à F : 13,311 en rayon, 13,919 en niveau carré, valeur que redonne la règle
  `margin` de l'oracle. Bloc à F : {y, s2, s3, b1, b2}, sans x. Dans la variante `c7` (F = 2,5, composante de
  6 sites), les dates sont 2,662 en rayon et 2,784 en niveau carré (31/4 à l'oracle), toutes deux après F.
- La cause : x est qualifié dès t′ = 10, mais un rival qualifié {w1, w2, x} né à 12,5 (après F) rejoint sa lignée
  à 15,81. C'est exactement le mécanisme que le rapport réserve à Q_1.

Commandes : `c7b_fusion_parasite.py`, et `c7_qualif_F.py` (variante à F = 2,5, composante de 6 sites). Deux routes
concordent : vfull et l'oracle `margin`.

**Énoncé correct** (même preuve que la proposition I, par L6) : P_κ∘Π_{k+1} place x à F dès que
t′_{k+1}(x) + d_k(x)/2 ≤ F, avec t′_{k+1} ≤ d_{k+1}(x). La perte est bornée, mais la couche perdue est plus épaisse
que pour P_κ : t′ y remplace α.

### R2. « La famille P_κ échoue exactement T0 et Q1 » (RAPPORT § 4, lecture 2) : faux hors d'une fenêtre de κ

Commande : `c8_kappa_fenetre.py`. P_κ∘Π_1 (rayon) ne passe Q2, Q3 et Q4 en lecture stricte, ainsi que Q-Π2, que
pour κ ∈ [1,3969 ; 3,7852]. Les deux bornes sont obtenues par dichotomie et ont une forme close :

- Q2 : κ ≥ (75,260 − 61)/(60,208 − 50) ;
- Q4, faces à 866,03 : κ ≥ 1,0966 ;
- Q4, chaîne après les faces : κ ≤ (1334,166 − 866,025)/(816,497 − 692,820).

Dès κ = 3,79, la chaîne CmD se forme avant les faces (839,46 à κ = 4 ; 692,82 à κ = 1000) : c'est l'option (c)
de Q4, que l'utilisateur a écartée. κ = 1 est en retard sur Q2 et Q4. La recommandation 2 du rapport (« κ, une
position sur un front ») omet cette contrainte : les cellules imposent κ ∈ [1,40 ; 3,78], et κ = 2 ou 3 s'y
trouvent.

## 3. Ce qui est confirmé

| Affirmation | Contrôle indépendant |
| --- | --- |
| H_m = Q_1∘Π_m ; version en rayon = P_1∘Π_m = ancrage persistant v10 | `c0` (0 écart) ; formule identique au mémo v10 § 3.1 ; P_2 redonne 54,844 (Q2) et 1 487,404 (Q1), les valeurs publiées en v10 |
| P_1 ≤ Q_1, même lignée, hauteurs ; strict si la barre décisive naît après α | identité 2(c−t)(c−m) ≤ 0 relue ; `c4_random.py 31337 500` : 6 006 sites, 0 violation (dates, ancres, 16 004 hauteurs), 2 792 strictement plus tôt |
| Q_1 sans constante de Lipschitz uniforme en rayon | `c2_families.py` : écarts 1,1833 ; 3,2373 ; 10,0696 ; 31,7896 (L = 10 à 10⁴) ; niveaux 2L+2 et 3L+9/2 exacts |
| Proposition D : (1+2κ)δ en dates et hauteurs ; les 5δ de H3 sont lâches | preuve refaite pas à pas (déjà au § 5.2 de la synthèse v10) ; `c5_stab.py 4242 400 20 40` : maxima P_1 1,62 (montée adverse), P_2 2,55, H_3 en rayon 1,50, core 1,91, fermeture 0,976 ; aucune violation |
| Théorème B (anticipation) | preuve relue (A5_glob dans Y, causalité à r = 1, réflexion à δ = 0) ; témoin : P_1 2,0 puis 1,8167, Q_1 4,6904 puis 4,2426, FULL identique jusqu'au rayon 10 inclus |
| Théorème C (cadre intrinsèque) | entrelacements de C1 (i), C1 (ii) et de (t+δ, s−δ, M+δ) reconstruits ; points (a) à (d) justes |
| Théorème E | niveaux de T0 : lentilles 1/2, ABC 2/3, racine 3/2 ; la lentille CD (nœud 3) n'a pour parent que la racine |
| Théorème F | `c3_seuil.py`, m ≤ 10, κ ∈ {1, 2, 4, 1000}, deux échelles : Q1bis passe seulement à m = 3 ; Q2, Q3 seulement à m ≤ 2 ; Q4 seulement à m ≤ 3 ; Q-Π2 à m ∈ {1, 2, 10} |
| Proposition G (m = 9 sur Q3) | bloc {x, c0..c7} de 453,471 à 796,117 ; avec m = 3, x dans l'amas à 549,087 (rayon) et 590,540 (niveau) |
| Proposition H : P_κ jamais après le cœur à K = 2 ; au plus 1,5 d_k, borne atteinte | {0,10,20,30} : 15 pour tout κ ; aléatoire : 0 violation de α + d_k/2, maxima 1,393 (K = 3) et 1,386 (K = 4) |
| Proposition I, et échec de Q_1 | 30 398 contrôles, 0 violation ; Q_1 sur {−2L,0,2} ; Q_1 manque aussi x en R1 (12,25 > 12) |
| FX-A9 : P_κ ne respecte pas le cœur à K = 3 | à r = 5, core {2,7} \| {10} ; P_1, P_2 et cover {0,2} \| {7,10,13} |
| Cellules et fixtures (T0, Q1 à Q4, cinq points) | toutes les dates du rapport retrouvées à 10⁻⁴ (`c1_fixtures.py`) |
| Piste λ ∈ ]0,969 ; 1,813[ | `c6_lambda.py` : 0,9694 ; 87,4458 ; 44,4333 ; 1,8128 |
| `reference_radius_rules` : propriétaire mort aux égalités exactes | `c9_owner_bug.py` : contexte global à 28 chiffres ; sur T0, les **six** sites, pas seulement C, reçoivent l'enfant du nœud vivant ; e est en défaut de 2·10⁻³⁰ à 3,5·10⁻²⁸ |

## 4. Réserves de formulation, sans réfutation

1. **Théorème C, constante minimale.** Le théorème est juste dans le cadre intrinsèque : profils abstraits, A3^int.
   Mais le résumé (« pour les règles locales au profil, la constante des dates est au moins 3 ») et la note du
   développeur (« constante minimale de toute règle continue de sa classe ») omettent ce qualificatif. Pour la
   stabilité géométrique, rien n'est prouvé : la v10 ne connaît que [2κ, 1+2κ]. Le corps du rapport le dit
   (remarque 3).
2. **Théorème E.** « T0 force une règle non monotone » vaut parmi les règles locales au profil **brut**. H_{k+1}
   passe T0 avec Φ = P_1 monotone sur Π_3. Le corps du rapport le dit.
3. **Piste λ.** La borne 1,8128 vient des triplets mixtes {C, m, P/Q/R}, nés à 1 009,95 et morts à 1 078,17, qui
   contiennent le point médian m de la chaîne. Les faces seules donnent 2,4971. L'intervalle dépend donc de
   structures qui mêlent les deux côtés.
4. **Bogue du développeur.** « Dates non affectées » est vrai à la tolérance près. Les dates ne sont pourtant
   justes qu'à 28 chiffres, pas aux 120 annoncés.
5. **Note du développeur.** « La note ne cite pas la v10 » était vrai à 21 h 53. La version de 22 h 01 cite
   l'ancrage persistant et reprend B, C, D, F et H5. Elle reprend donc aussi l'erreur R1 et la réserve 1.

## 5. Non vérifié

- ER0h(1, 12), 125/125 : la citation du VERDICT_FINAL v10 (§ 3.1, pente maximale 102,7) est exacte. Le résultat
  n'a pas été rejoué.
- Les deux conjectures : existence d'une règle qui passe A8 avec une stabilité uniforme ; absence d'une règle la
  plus précoce dans le cadre décoré. Les valeurs 8,17 / 60,36 et 1,2247 / 0,8165, qui montrent l'incomparabilité
  de H_1 et H_{k+1}, sont retrouvées.
- Les chiffres agrégés G4 du développeur.

## 6. Reproduction

Depuis ce dossier, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B` (Python 3.12.1, mpmath 1.4.1). Temps CPU total
mesuré : environ 3 min.

| Commande | Reçu |
| --- | --- |
| `c0_oracle.py 20261007 400` | `recus_c0_oracle.json` |
| `c1_fixtures.py` | `recus_c1_fixtures.json`, `c1_fixtures.out` |
| `c2_families.py` | `recus_c2_families.json` |
| `c3_seuil.py` | `recus_c3_seuil.json` |
| `c4_random.py 31337 500` | `recus_c4_random_31337.json` |
| `c5_stab.py 4242 400 20 40` | `recus_c5_stab_4242.json` |
| `c6_lambda.py` | `recus_c6_lambda.json` |
| `c7_qualif_F.py`, `c7b_fusion_parasite.py` | `recus_c7_qualif_F.json`, `recus_c7b_fusion_parasite.json` |
| `c8_kappa_fenetre.py` | `recus_c8_kappa_fenetre.json` |
| `c9_owner_bug.py` | `recus_c9_owner_bug.json` |

**Fixture minimale permanente à graver** (règle du dépôt) : le contre-exemple R1 à 8 sites. Il faut aussi corriger
H5 dans `HIERARCHIE_POINTS.md` et le § 0 point 7 du rapport `axiomes`. Je n'ai rien modifié hors de ce dossier.
