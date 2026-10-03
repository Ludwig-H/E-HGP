# Vérification adverse du rapport « fermeture qualifiée et approximations extérieures »

3 octobre 2026, 22 h 19 UTC (heure lue par `date -u`). Vérificateur adverse, label `verif_fermeture`. Rapport jugé :
`build/v11-points-math/fermeture/RAPPORT.md`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git, aucune construction native. Worktree lu seulement, aucun .pyc écrit.
```

## Méthode

Tout est recalculé par un oracle écrit ici, `vf_oracle.py`, sans le code du rapport (`lib_fermeture.py`) ni l'oracle
de référence. La plus petite boule englobante est trouvée par le critère M1 : support affinement indépendant dont le
circumcentre, donné par les formules fermées du § 2 de MATHEMATIQUES.md, est dans l'enveloppe convexe, la boule
contenant la partie. La référence prend au contraire le minimum des sphères circonscrites contenantes. Γ_k est
balayé avec un DSU propre, et la fermeture ainsi que les règles core, cover, first, H_m (`margin`) et EC sont
réécrites. `croise_reference.py` ne sert qu'à valider ce code : 10 265 contrôles contre la référence, 0 écart.
Les générateurs du rapport ne sont recopiés que pour rejouer ses nuages aléatoires (mêmes graines).
Temps total : environ 4 min de CPU.

## Verdicts

| Affirmation du rapport | Verdict | Preuve |
| --- | --- | --- |
| Cinq garanties de l'auditeur (laminarité, équivariance, 1ε, minmax, identité (k,k+1)=(k+1,k+1)) | confirmée | preuves relues ligne à ligne ; 0 violation exacte de 1ε sur 170 870 comparaisons (pire 0,921ε) ; minmax 6 484/6 484 ; identité 1 469/1 469 ; m ≤ k 3 472/3 472 ; équivariance 1 339/1 339 ; (k,k+2)=(k+1,k+2) faux 1 465 fois sur 1 469 |
| T1 : fermeture = liaison simple de w_k' (k' = max(k,m) ≥ 2, m ≤ k+1) | confirmée | preuve juste ; 4 565/4 565 ; le mutant k' = k diffère 1 091 fois sur 1 093 |
| T2 : √w ≤ d_mr ≤ 2√w, SL_mr(r) ⊆ Π(r²) ⊆ SL_mr(2r) | confirmée (portée à corriger) | 4 565/4 565, facteur 2 atteint. Mais T2∘T3 donne la même chose pour FULL : couvertures FULL et HDBSCAN sont 2-entrelacées (41 499 contrôles, 0 échec). « HDBSCAN à facteur 2 près » ne distingue donc pas la fermeture de FULL |
| T3 : bloc au rayon r dans une seule couverture FULL au rayon 2r ; constante atteinte | confirmée | 6 484 + 6 484 contrôles ; max w/u = 4,000 ; vallée 50 contre 100 rejouée |
| T4 : w ≤ u_F ≤ 4 max(e_i, e_j, w) pour core, cover, first, H_1, H_m, EC | confirmée | 12 × 6 484 contrôles, 0 écart ; rapports max 3,890 (first, cover, EC), 2,25 (core), 1,8385 (H) |
| T5 (pas d'extérieur fidèle) | confirmée | énoncé tautologique, juste ; fermeture non faiblement fidèle dans 3 867 triplets sur 6 484 |
| Fermeture avant FULL (vallée, Q3, Q4, contact, cinq points) | confirmée | tous les niveaux rejoués : 50/55,227/66,708/79,057 contre 100 ; Q3 700,501 contre 796,117 ; Q4 1 078,174 contre 1 334,166 |
| Fréquences two_blobs 300/300 à (2,3) et (3,4) | confirmée, une erreur mineure | 300/300 et 300/300 rejoués, médiane 0,8174 ; le minimum à (2,3) est 0,5303 : le 0,5015 vient de (2,1). Toute la table des événements (18 lignes) est reproduite à l'identique |
| « La fraction ne peut que baisser » réfutée (amas scindé) | confirmée à n fini seulement | fermeture 1 contre FULL 1/2, quel que soit le germe (FULL au mieux 2/3). Hors du modèle du chapitre 7 : A est une paire reliée par un pont, pas un convexe homogène |
| « FULL réunit A et B à r = 110,11 » (même fixture) | réfutée | 110,114 n'est que la lignée du germe pris dans A1. La lignée A2 (qui couvre 4 sites sur 6 de A) fusionne avec B dès 91,382, soit 0,15 % après la fermeture (91,241) |
| Rival lointain : H_3 ×13,9, sans borne | confirmée | rejoué : ×13,90 à D=300, ×25,3 à D=1000 ; la même marge en rayon reste ≤ 2,250 (×1,8) |
| EC fidèle, laminaire, équivariante, discontinue ; retards H_m 10,7 % / 17,4 % contre EC 0,3 % / 0,5 % | confirmée | rejoué à l'identique. Nouveau : EC = first (entrée et nœud) pour m ≥ k+1 sur les 2 633 triplets testés et sur les 4 394 sites de son échantillon ; écarts seulement à m = 1. EC n'est pas une règle nouvelle à m = k+1 |
| Q1–Q4, volet m = k+1 (fermeture, H_{k+1}, EC, first : Q1 oui, Q2–Q4 non) | confirmée | sous toutes les lectures ; raison structurelle : à m = k+1, un groupe de k sites (CD, {x,a}, {x,f1}, CmD) n'est jamais un cluster |
| Q1–Q4, volet « H_1 donne Q2, Q3 et Q4 » | réfutée | sur les fenêtres posées à l'utilisateur et sur les cellules ancrées v10, H_1 échoue les quatre : x seul sur [61 ; 67,37[ (Q2), sur [705 ; 744,18[ (Q3), PQR et STU absents sur [866,03 ; 899,51[ (Q4). Le juge v10 comptait déjà un retard de 4,5 % comme un échec (MMt). first_1 et EC_1 (sans marge) rendent Q2 et Q3 strictement ; Q4 n'est accepté que par la cellule ancrée, pas par l'option (a) littérale |
| Inégalités asymptotiques Θ^CL ≥ Θ^poly, λ_c^CL ≤ λ_c^poly ≤ 2^p λ_c^CL | confirmée (preuve relue) | correctes à la rigueur de la proposition 1 de la thèse ; non mesurables ici |
| Conjectures C1–C2 | non vérifiée | énoncées comme conjectures ; aucune mesure de vitesse de percolation |

## Ce qui change la lecture du rapport

1. **Les mathématiques tiennent.** Garanties, T1–T5 et identités : aucune faille trouvée. Les preuves ont été relues
   et recodées, avec 0 écart sur plus de 250 000 contrôles exacts.
2. **Le grading de Q1–Q4 est laxiste au profit de H_1.** Sous la lecture que le verdict v10 appliquait, la marge en
   niveau carré fait perdre Q2–Q4 à H_1. « C'est m qui décide » n'est donc vrai qu'à moitié. À m = k+1, toutes les
   règles donnent Q1 et perdent Q2–Q4. À m = 1, c'est la marge qui décide : first_1 et EC_1 rendent Q2 et Q3, H_1
   ne rend aucune des quatre cibles.
3. **Le facteur 2 avec HDBSCAN ne discrimine pas.** Les couvertures FULL elles-mêmes sont 2-entrelacées avec la
   liaison simple de l'atteignabilité mutuelle (min_samples = k, site compté). Il y a même un côté sans facteur :
   avec la convention de la thèse, chaque couverture FULL au rayon r tient dans un bloc HDBSCAN au rayon r.
   L'argument du verdict (§ 7.1) doit donc reposer sur T1 (structure : seuls les w_k' comptent) et sur les réunions
   précoces mesurées, pas sur « HDBSCAN à facteur 2 près ».
4. **La contre-fixture « amas scindé » est juste mais présentée de façon trompeuse.** FULL y crée aussi une fusion
   parasite, A2–B à 91,382, presque en même temps que la fermeture. Ce qui distingue la fermeture, c'est qu'elle a
   déjà recousu A1 et A2 par le site du pont, à 60,83.

## Reproduction

Depuis `build/v11-points-math/verif_fermeture/`, avec `PYTHONDONTWRITEBYTECODE=1 python3` (Python 3.12) :

```text
croise_reference.py --clouds 120 --seed 99                 -> 10 265 contrôles, 0 écart        (8 s)
verif_theoremes.py --clouds 450 --seed 4242 --seconds 300  -> resultats/theoremes.txt          (78 s)
verif_stabilite_vf.py --clouds 500 --seed 777 --seconds 200 -> resultats/stabilite_vf.txt      (34 s)
full_vs_hdbscan.py --clouds 300 --seed 5150                -> resultats/full_vs_hdbscan.txt    (8 s)
familles_vf.py ; amas_scinde.py ; fixtures_auditeur_vf.py  -> resultats/*.txt                  (< 2 s)
cibles_vf.py ; juge_fenetres.py                            -> resultats/cibles_vf.txt, juge_fenetres.txt
frequences_vf.py ; evenements_vf.py ; ec_vs_hm_vf.py       -> graines 11 et 23 du rapport      (51 s)
ec_contre_first.py ; equivariance_et_marge_rayon.py        -> resultats/*.txt
```

Limites : tous les calculs sont faits pour n ≤ 9 (14 pour Q3). Ce sont des oracles de correction, sans aucune
portée de fréquence ni de pente à n = 8 000. Les fenêtres de Q1–Q4 sont reprises de `QUESTIONS_UTILISATEUR.md` et de
`CIBLES_REVISEES.md` (§ 2.2–2.6), sans les variantes ± 1 du juge v10.
