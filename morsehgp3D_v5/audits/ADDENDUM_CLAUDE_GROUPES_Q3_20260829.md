# Addendum Claude — la mesure receiptée corrige mes deux propositions : c'est le RESCAN, pas la proposition d'ancre (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `ac43ab1a`. Reçu :
`morsehgp3D_v5/receipts/echelle_par_lane_20260829/` (pin `a3c15d84`, cible
produit `mhgp5`, arbre propre, trois tailles). Cet addendum **rétrograde les
propositions A et B** de la note précédente sur la foi de cette mesure.

## 1. Exposants receiptés à trois points — les deux bouts sont linéaires

| | `scanline` q3 | `scanline` q4 | `terrain` q3 | `terrain` q4 |
|---|---|---|---|---|
| rectangles vivants | **1,00** | **0,99** | **1,03** | **1,03** |
| ancres proposées | 1,43 | 1,56 | 1,22 | 1,27 |
| seeds | 1,59 | 1,50 | **1,96** | 1,71 |
| complétions | — | 1,58 | — | **1,93** |
| candidats (l'objet) | **1,00** | **0,81** | **1,02** | **1,10** |

Le squelette WSPD est **exactement linéaire** et l'objet aussi. Tout l'excès
vit strictement entre les deux, et il n'est pas au même endroit selon le
régime :

- `scanline` : ancres et seeds montent ensemble (1,43–1,59) ;
- `terrain` : le **catalogue d'ancres va bien** (1,22–1,27) et ce sont les
  **seeds et complétions** qui explosent (1,96 et 1,93).

Sur `terrain`, le nombre de seeds **par ancre** passe de 8,4 à 23,8 entre 8000
et 32 000, soit $n^{0{,}75}$ : chaque ancre porte de plus en plus de tiers
aigus. C'est la pathologie propre à un champ de hauteur, où le voisinage est
quasi coplanaire.

## 2. Ce que cela fait à ma proposition A

L'escalier d'histogramme attaque la boucle $\lvert A \rvert \times \lvert B \rvert$,
c'est-à-dire la **proposition d'ancres**. Sur `terrain`, cette boucle est en
$n^{1{,}22}$ — ce n'est pas le problème. A ne peut donc pas être présentée
comme le levier ; elle vaut sur `scanline`, où les ancres sont en $n^{1{,}43}$,
et pas ailleurs. Je la maintiens comme optimisation exacte et gratuite, pas
comme réponse à la question de Louis.

## 3. Ce que cela fait à ma proposition B — je la rétrograde aussi

J'ai proposé de tester la lentille et l'acuité au niveau du handle. Le compte
par ancre, à `terrain` $n = 8000$, est le suivant :

- environ **51 sites de cover** par ancre, donc environ 51 tests
  `is_acute_seed`, chacun **trois comparaisons de distances** ;
- environ **12,4 seeds survivants**, chacun **rescannant le cover** : environ
  $12{,}4 \times 51 \approx 632$ tests de profondeur, chacun une estimation
  flottante certifiée et parfois un repli exact i128.

**Le filtrage des seeds, que B accélère, pèse donc environ 8 % du travail par
ancre ; le rescan de profondeur, que B ne touche pas, en pèse le reste.** B
attaque la partie bon marché. C'est le troisième mécanisme d'affilée que je
propose et qui vise à côté de la masse — après le raffinement post-séparation
et le filtre d'enveloppe. Le point commun des trois : ils réduisent des
**propositions**, alors que le coût est dans les **rescans**.

## 4. La vraie cible, et le seuil mesuré

Le produit `seeds × taille du cover` est la masse. C'est exactement ce que
votre étage 2 remplace : l'arrangement des droites $h_x = 0$ dans le plan
bissecteur, dont on ne veut que les niveaux peu profonds
($\kappa_3 = s_{\max} - 3 = 8$), en
$O(m_e \log m_e + m_e \kappa_3)$ au lieu de $O(m_e^{2})$ effectifs.

Avec le rapport mesuré seeds/cover $\approx 0{,}24$, le coût actuel vaut
environ $0{,}24\,m^{2}$ et l'arrangement $m \log_2 m + 8m$ :

| $m$ (sites par ancre) | 38 | 50 | 100 | 200 | 462 | 1000 |
|---|---|---|---|---|---|---|
| verdict | scan direct | scan direct | **× 1,6** | **× 3,1** | **× 6,6** | **× 13,4** |

**Le seuil de rentabilité est $m \approx 60$–$100$.** Et le travail est
extrêmement concentré — `terrain`, $n = 8000$, part estimée du travail de
rescan par route :

| route | ancres | part des ancres | $m$ | part du travail |
|---|---|---|---|---|
| q3 cover | 287 363 | 48,1 % | 38 | 6,2 % |
| q3 requête | 9 111 | 1,5 % | 462 | 28,7 % |
| q4 cover | 286 038 | 47,9 % | 39 | 6,5 % |
| q4 requête | 14 607 | 2,4 % | 521 | 58,5 % |

**3,9 % des ancres portent environ 87 % du travail de rescan.** (Modèle
$\propto 0{,}24\,m^{2}$, pas une mesure directe : les sorties anticipées le
rendent optimiste en valeur absolue. La **concentration**, elle, ne dépend que
des $m$ mesurés.)

## 5. Le point qui rend cela immédiatement actionnable

Ces 3,9 % d'ancres denses sont **déjà identifiées par le code** : ce sont
exactement celles que `pretest_query_min_points` route vers les pré-tests par
requête, parce que leurs handles sont gros. **Le seuil de routage de
l'arrangement existe donc déjà**, il est déjà mesuré, et il sélectionne déjà la
bonne minorité. Un constructeur shallow n'a pas besoin d'être universel : il
suffit qu'il prenne la route dense.

## 6. Questions

- **V57.** Acceptez-vous cette rétrogradation de A et B, et la conclusion que
  les trois mécanismes que j'ai proposés jusqu'ici (post-séparation,
  enveloppe, A/B) partagent le même défaut — réduire des **propositions**
  quand le coût est dans les **rescans** ?
- **V58.** Le seuil $m \approx 60$–$100$ et la coïncidence avec
  `pretest_query_min_points` vous paraissent-ils exploitables pour borner le
  chantier R2 à la route dense, au lieu d'un constructeur universel ?
- **V59.** Sur `terrain`, seeds par ancre croît en $n^{0{,}75}$. Est-ce que
  cela invalide l'hypothèse de « profondeur bornée ⟹ $m_e$ borné » sur laquelle
  repose la cible $O(m_e \log m_e + m_e \kappa)$, ou est-ce précisément ce que
  l'arrangement absorbe sans que $m_e$ ait besoin d'être borné ?
