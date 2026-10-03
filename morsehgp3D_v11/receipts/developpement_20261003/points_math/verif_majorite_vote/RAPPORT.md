# Vérification adverse : règles à masses et vote de la thèse

3 octobre 2026, `date -u` = 22:33 UTC à la fin des calculs. Label `verif_majorite_vote`. Cible :
`build/v11-points-math/majorite_vote/RAPPORT.md`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only (oracles exacts, tailles d'oracle seulement)
mode=verification_adverse
public_status=not_claimed
GCP non utilisé. Aucune commande git. Aucune construction ni test natif.
Écritures : uniquement sous build/v11-points-math/verif_majorite_vote/.
```

## Méthode

J'ai réécrit les règles depuis leurs définitions écrites, dans `mes_regles.py`, sans importer de code de règle du
rapport :

- ER0h : verdict v10, § 2.2, par crédits explicites et balayage direct des niveaux ;
- ER0 : même règle, sans héritage ;
- ER0hr : remise $\kappa'\mu(\sqrt{e}-\sqrt{A})$ ;
- $H_m$ en niveau, qui est la première version du développeur ;
- $H^{r}_m$ en rayon, qui est sa règle retenue au 3 octobre.

`vote_these.py` réécrit le vote du § 9.1 de la thèse rendu hiérarchique ($V_{1/2}$, avec ou sans cône, faces de
Gabriel ou toutes les faces, $p\in\lbrace 0,2\rbrace$).

Arbres utilisés :

- `vfull` (v10) ;
- l'oracle v11 `hgp11_ref.Definition`, qui recoupe `vfull` sur les fixtures de la proposition S, sur Q1, sur Q2 et
  sur les retards.

Le juge v10 (`cellules.py`) est importé sans modification. `delta` est un appariement structurel : naissances lues
sur les sites couverts, fusions sur les enfants. Il refuse toute combinatoire différente. Le temps CPU total est
d'environ 6 minutes. Tous les reçus sont dans `recus/` (`SHA256SUMS`) ; le code est couvert par `SHA256SUMS_code`.

## Verdicts

| # | Affirmation | Verdict | Preuve (commande ou fichier) |
| --- | --- | --- | --- |
| 1 | Prop. M (médiane sur l'arbre ⇒ pendaison fidèle) | confirmé | relue ligne à ligne ; c'est F1, plus le fait que le point médian est dans $R_x$ |
| 2 | Lemme 4a, corollaire D | confirmé | preuve relue ; 1 640 dates aléatoires, 0 violation, pire $t/\alpha=1{,}90$ (`recus/argmax_et_corD.txt`) |
| 3 | Argmax par niveau non laminaire (4 sites) | confirmé | scores recalculés : $S_{23}=S_{02}=46/65$, $S_{12}=4/5$ ; bloc {2,3} à 1/2 puis {1,2} à 2, avant la fusion à 5/2. Égalité exacte à 5/4, qui demande en plus un départage |
| 4 | Prop. S (ER0h sans borne uniforme) | confirmé | `python3 -B prop_s.py isocele` : 304,584 ; 1 204,63 ; 4 804,64 ; 19 204,6 ; 76 804,6 ; 307 204,6. Héron et toutes les inégalités vérifiés pour $\rho=2\ldots40$ |
| 5 | Unités v10 : 146,8 → 9 597 ; selle 394 → 11 878 | confirmé | `prop_s.py radiale` : 146,850 ; 596,775 ; 2 396,756 ; 9 596,752. `prop_s.py selle` : 394,4 ; 926,2 ; 2 901,3 ; 11 878,0 |
| 6 | Prop. U (héritage proportionnel forcé) | confirmé (cadre abstrait) | équation $h(a,b+c)=h(a+b,c)h(a,b)$, cocycle, puis Cauchy : correct. Portée limitée, voir plus bas |
| 7 | ER0 et ER-hv(2/5) discontinus | confirmé | témoins rejoués (`recus/temoins_er0_erhv.txt`) : 106,507 → 1 066,905 ; 118,034 → 1 180,340. Ma recherche : ER0 30 sauts sur 3 407 configurations |
| 8 | ER0hr : 125/125 pour $\kappa'\geq 6$, ≈ $\kappa'/2$, aucun saut | confirmé | `juge_mes_regles.py` ; $\kappa'=4$ donne 85/125 mais échoue **aussi** Q4 (20/30), pas seulement T0 ; ma recherche : 0 saut |
| 9 | ER0hr sans petite constante (pire aléatoire 69,5) | confirmé | les pires cas sont rejoués exactement (69,471 ; 152,866 ; 2,621 ; 2,000). Six votes feuilles, $\mu$ de 0,00414 à 0,00123. Sur ce cas, ER0h ne fait que 27,55 |
| 10 | ER0h et ER0hr continus | non vérifié | conjecture. 0 saut sur 3 407 configurations de treillis (×1 000 et ×10 000), avec un témoin positif ER0 à 30 sauts |
| 11 | ER0hr : $\vert\Delta e\vert\leq C(\kappa',\eta)N_x\delta$ | non vérifié | conjecture. Argument heuristique compatible : $W\geq\eta A$, et un saut d'héritage sensible est dominé par le saut suivant |
| 12 | Prop. O (aucun $H_m$ ne réalise les réponses) | confirmé | faits structurels relus sur l'oracle v11 : CD à 722 500 est l'unique point le plus bas de $R_C$ ; la rencontre avec ABC a lieu à la racine 3 194 656 ; {x,b1,b2} à 60,36 ; $a$ à 75,122. Q3 : amas à 453,47. Q4 : CPQR à 866,03 |
| 13 | Décomptes du juge | confirmé | ER0h, ER0hr(6–24), ER0, ER-hv : 125. $H_{k+1}$, $H_{\max}$ : 70. $H_1$ : 35. Vote : 70, 60, 25, 20 (ventilations identiques) |
| 14 | Retard de $H_m$ non relatif | confirmé pour la version en niveau | $e/A=r+2$ et $(r+1)^{2}/4$ exacts ; mais $H^{r}_1$ donne $e/\alpha=2$ |
| 15 | $V_{1/2}$ discontinu (égalité exacte ; bascule de Gabriel) | confirmé | masses exactes $1/2$ et $1/2$ dans $X$ ; saut/D 0,0711 ; 0,0706 ; 0,0706. Gabriel {1,2,3,4} oui puis non ; saut/D 0,1072 ; 0,1068 ; 0,1067 |
| 16 | $V_{1/2}$ toutes faces avec cône : dates au-delà de la racine, 20–25/125 | confirmé | Q3 : date max 2 192,28 pour une racine à 796,12 ($p=0$) ; aucun saut sur le témoin 2 |
| 17 | Dépendance à $F_K$ et à $p$ (T0_P1) | confirmé | $p=0$ toutes faces : rien avant la racine ; $p=2$ toutes faces : AB\|EF ; Gabriel $p=2$ : ABC\|DEF |
| 18 | La v10 « sous-estime » les pentes | confirmé sur le fond, étiquette « réfute » exagérée | pente ≈ $\kappa\rho^{2}$, confirmée. La v10 écrivait déjà « non bornées a priori… aucune constante uniforme n'est revendiquée » |

## Ce que la vérification change

1. **$H_m$ étudié dans une version abandonnée.** Le rapport juge $H_m$ à marge en niveau, carré du rayon. Le
   développeur retient désormais $H^{r}_{k+1}$, à marge en rayon, et qualifie la version en niveau de régression.
   - J'ai rejoué la version en rayon. $H^{r}_{k+1}$ obtient le même **70/125**, avec la même ventilation, et la
     proposition O s'applique, puisqu'elle ne dépend que de la lignée.
   - Le défaut « $e/\alpha^{2}=r+2$ » ne la touche pas : $H^{r}_1$ donne $e=2\alpha$. Seul le retard dû à la
     qualification subsiste, avec $H^{r}_3=(r+1)/2$.
   - Sur la famille radiale, $H_1$ en niveau saute de 0,56 à 4,47 par déplacement unité, comme annoncé ; $H^{r}_1$
     et $H^{r}_3$ restent à 0,25 (`recus/radiale_H_rayon.txt`).
2. **La proposition O n'est pas nouvelle.** `HIERARCHIE_POINTS.md` § 3 cite déjà le « théorème F du workflow » :
   aucun seuil ne passe à la fois Q1bis et Q2. Le rapport ne le cite pas.
3. **Poids des 125 jugements.** La recommandation 3 tient les réponses ancrées pour ce qui « fait foi ». Or
   `HIERARCHIE_POINTS.md` rapporte une consigne de l'utilisateur : « Q2 ou Q3 ne sont que de peu d'importance par
   rapport au modèle mathématique ». L'arbitrage proposé au point 4 reste juste, mais il est déjà en partie tranché
   par cette consigne.
4. **ER0hr est ajustée et validée sur les mêmes cellules.** Ses 125/125 sont un résultat dans l'échantillon. Seules
   quatre valeurs de $\kappa'$ ont été testées, alors que le rapport écrit « de 6 à 24 ». Le catalogue v2 n'a pas été
   rejoué. Les trois familles où ER0h explose sont celles pour lesquelles le cône relatif a été conçu. ER0hr n'est
   pas meilleure partout : 69,5 contre 27,6 pour ER0h sur le même cas. En unités v10, ma recherche donne un saut
   maximal de 10,3 pour ER0hr et de 249,7 pour ER0h.
5. **Portée de la proposition U.** Elle force la proportionnalité de la *mesure de crédits* dans une classe sans
   rétention. Elle ne montre pas que la continuité de la *pendaison* l'exige. Le rejet de toute rétention repose
   sur deux témoins et une conjecture. Le rapport le dit au § 10, mais le résumé écrit « le seul cohérent ».
6. **Imprécisions mineures.**
   - « 307 200 » vaut en fait 307 204,6.
   - Le résumé place « hors diagonale, de 394 à 11 878 » sous « En unités v10 ». Ce sont des rapports en niveau ; en
     unités v10, la selle donne 167,7 → 5 140,6.
   - « Six poids d'environ 500 » : ils bougent en réalité de 500 à 1 484.
   - À forme fixe, le saut de la selle croît avec $\lambda$ puis sature. C'est une pente finie mais grande, pas une
     discontinuité, ce qui est cohérent avec le rapport.

## Limites

- Oracles bornés seulement : $n\leq 7$ dans la recherche, 14 sites pour Q3.
- La continuité et la borne locale ne sont pas tranchées.
- Je n'ai pas rejoué les 36 et 50 sauts de la campagne ER0 du rapport, mais ma propre recherche en trouve 30.
- Je n'ai pas rejoué la recherche ER-hv(1/20).
- Je n'ai jugé ni le catalogue v2 ni les mesures LiDAR.

## Reproduction

```bash
cd /workspaces/E-HGP/build/v11-points-math/verif_majorite_vote
python3 -B mes_regles.py
python3 -B prop_s.py isocele; python3 -B prop_s.py radiale; python3 -B prop_s.py selle
python3 -B juge_mes_regles.py ER0h > recus/juge_ER0h.json
python3 -B juge_mes_regles.py ER0hr4,ER0hr6,ER0hr10,ER0hr16,ER0hr24,ER0hr3,ER0hr2,ER0 > recus/juge_ER0hr_ER0.json
python3 -B juge_mes_regles.py H1,Hk1,Hmcs,Hr1,Hrk1,Hrmcs > recus/juge_H.json
python3 -B juge_mes_regles.py VOTEg2nc,VOTEg2,VOTEa2,VOTEa0 > recus/juge_VOTE.json
python3 -B juge_mes_regles.py ERhv20,ERhv5,ERhv25 > recus/juge_ERhv_v10.json
python3 -B recherche_sauts.py 20261003 200 > recus/recherche_sauts.json
```

Les contrôles ponctuels (témoins, proposition O, retards, pires cas) sont des scripts en ligne de commande dont la
sortie est gravée dans `recus/*.txt`.
