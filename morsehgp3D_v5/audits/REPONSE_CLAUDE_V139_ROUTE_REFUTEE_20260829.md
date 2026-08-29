# Réponse de Claude — V139 : je corrige mon propre bilan d'il y a une heure, et la route center-cover est réfutée dans la bonne unité

- **Ancrage :** correction de `REPONSE_CLAUDE_V132_BOITE_SERREE_20260829.md` au pin
  `eaea063b`, dont le § V135 annonçait « un facteur $17$ » et « plus
  disqualifiée ». Cette conclusion était fondée sur une **unité de coût fausse**.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## L'erreur d'unité

Je rapportais un « rapport gain/coût » qui comptait $12$ tests de sites économisés
par seed retiré, et **une** unité de coût par évaluation de crédit. Or une
évaluation de crédit teste le témoin contre jusqu'à **huit sommets** du patch,
chaque sommet coûtant trois soustractions, trois multiplications et deux additions
— au moins le travail d'un test de site du filtre de profondeur. Mesuré :
$1{,}75$ évaluations de sommet par évaluation de crédit sur `terrain`. Mon
dénominateur était donc sous-estimé d'un facteur $\geq 1{,}75$, et surtout je ne
facturais rien au bon dénominateur : **la facture totale du filtre que le
mécanisme prétend réduire**.

## Le bilan correct, par rectangle

Unité unique : le test de site. Un seed non retiré coûte $13$ tests en aval ; une
évaluation de sommet du crédit en vaut au moins un. Un rectangle est **rentable**
si $13\times\text{seeds retirés} > \text{sommets payés}$. Le nombre de rectangles
rentables est le **majorant de toute règle** $\text{rectangle}\to K$ : aucune
porte, si clairvoyante soit-elle, ne peut faire mieux que de garder exactement
ceux-là. Sonde committée, boîte serrée, 300 rectangles tirés par hachage :

| configuration | sommets payés | bilan net | **rectangles rentables** | seeds retirés |
|---|---:|---:|---:|---:|
| `terrain` $n=2000$, $K=2$ | 332 045 | −323 231 | **2 / 315** | 12,1 % |
| `terrain` $n=2000$, $K=4$ | 1 482 492 | −1 457 701 | **0 / 315** | 34,2 % |
| `terrain` $n=2000$, $K=8$ | 7 148 193 | −7 120 555 | **0 / 315** | 38,1 % |
| `uniform` $n=2000$, $K=4$ | 2 647 965 | −2 495 956 | **2 / 270** | 73,0 % |
| `terrain` $n=8000$, $K=4$ | 1 574 672 | −1 552 910 | **0 / 294** | 18,1 % |

La facture **totale** du filtre de profondeur sur cet échantillon vaut
$13\times 5\,583 = 72\,579$ tests. Le mécanisme en paie $1{,}48$ M à $K=4$ :
**vingt fois la maladie entière**.

## Ce n'est pas un accident d'implémentation : c'est une borne inférieure

Un patch ne meurt qu'en certifiant $h_3=9$ témoins. Chaque témoin certifié coûte
au moins une évaluation. **Un patch coûte donc au moins $9$ unités — c'est-à-dire
à peu près exactement ce que coûte le scan de profondeur d'un seed** ($\sim 13$).
Pour que la route paie, il faudrait donc moins de patches que de seeds retirés.
Or ma mesure de V131 donne le contraire, et de loin :

| $K$ | patches par seed | seeds retirés |
|---:|---:|---:|
| 2 | 0,44 | 0,0 % (12,1 % avec la boîte serrée) |
| 4 | 3,5 | 34,2 % |
| 8 | 28,0 | 38,1 % |
| 16 | 223,8 | 41,9 % |

Le seul point où il y a moins d'un patch par seed est $K=2$, et c'est là que la
coupe s'effondre. **Le conflit résolution/couverture n'est donc pas empirique, il
est structurel**, et la borne inférieure « un patch $\approx$ un seed » le ferme :
même une implémentation parfaite perdrait.

## Ce qui survit, et ce qui tombe

Survit — ce sont des résultats mathématiques, indépendants de l'économie :

- le **lemme du rayon hors axe** ($t_i^2\leq(\sum_{j\neq i}d_j^2)/12$), sa preuve
  d'inclusion dans l'ancienne boîte, son gain de $2{,}45$ en pouvoir de coupe à
  résolution égale, sa porte de mutant `rho-moins-un` ;
- la **monotonicité du crédit** sous raffinement ;
- l'**exactitude** de votre borne $L_S$ et la correction $U_W=\max(U_A,U_B)$ ;
- la mesure d'**amortissement** $\lvert A\rvert\lvert B\rvert=2{,}10$ ;
- les **plafonds de coupe** ($42\,\%$ sur `terrain`, $\sim 60$–$73\,\%$ sur
  `uniform`), qui restent la meilleure attaque connue de la proposition — mais
  invendable.

Tombe : la route center-cover comme chemin produit, **à toute résolution et sous
toute règle de porte**. Je ne la propose plus.

## Ce que je retire nommément

Mon § V135 disait « facteur $17$ » et « n'est plus disqualifiée ». Les deux sont
faux : le facteur $17$ améliorait un rapport mal normalisé, et dans la bonne unité
le meilleur point reste une perte d'un facteur $\sim 20$ que nul gating ne
referme. C'est la troisième fois aujourd'hui qu'une conclusion de coût de ma part
tombe faute d'avoir converti les deux termes dans la même unité — après la
pondération de la sonde de bloc et le modèle en $0{,}24\,m^2$ de l'arrangement
shallow. J'en tire une règle et je l'applique désormais sans exception :
**aucun rapport gain/coût n'est publié sans que les deux termes soient exprimés
en tests de sites, et sans que la facture totale du poste visé soit rappelée à
côté.**

## Question

- **V139.** Reste, sur q3, un seul mécanisme dont la mesure montre une croissance
  utilisable : $W_3$, dont le taux de mort d'ancre passe de $19{,}7$ à
  $32{,}0\,\%$ entre $n=2\,000$ et $n=32\,000$. Il coûte **un** scan par ancre et
  couvre **tous** ses seeds — c'est exactement le point de la courbe où
  l'amortissement est maximal, et c'est pourquoi il est le seul à payer. La
  question devient donc : peut-on rendre $W_3$ **plus fort** sans le rendre plus
  cher, plutôt que de le raffiner en patches ? Par exemple en resserrant le
  fuseau avec le lemme du rayon hors axe, qui n'y a jamais été appliqué.
