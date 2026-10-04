# Poids exacts de la présentation q4 — 3 octobre 2026

`Q4Candidate` et `Sphere` conservent le booléen privé
`q4_presentation_strictly_inside()`. Il concerne exactement les quatre sites
fournis à `through4`, vaut `false` aux autres arités et accompagne les
coefficients lors des copies et de `materialize()`. Il ne donne pas `qmin`.
Une présentation affine indépendante à poids nul ou négatif reste constructible.
Une présentation dégénérée garde le résultat vide habituel.

Le prédicat générique `strictly_inside(center,a,b,c,d)` reste inchangé : une
autre présentation de la même boule peut avoir des poids de signes différents.
Les seuls consommateurs remplacés sont `catalogue::q4_of` et le cas q4 de
`tower::strict_support`, immédiatement après leur fabrique sur le même tuple.
Les coefficients, niveaux non réduits et compteurs géométriques sont conservés.

## Dérivation

Pour les sites `(a,b,c,d)`, poser `u=b-a`, `v=c-a`, `s=d-a`, puis
`vs=v×s`, `su=s×u`, `uv=u×v`, `δ=u·vs ≠ 0` et
`N=|u|²vs+|v|²su+|s|²uv`. Le centre est `a+N/(2δ)`.
Les coordonnées barycentriques se déduisent de la base duale : par exemple
`λ1=((centre-a)·vs)/δ=(N·vs)/(2δ²)`.
Avec `H=2δ²>0`, les quatre numérateurs exacts sont donc

```text
w1 = N·vs ; w2 = N·su ; w3 = N·uv ; w0 = H-w1-w2-w3.
```

Le code conserve l'ordre de rejet `w0,w1,w2,w3`, avec arrêt dès un poids `<=0` :
`face=vs+su+uv=(v-u)×(s-u)`, `w0=H-N·face`, puis les deux produits scalaires
`w1,w2`, et enfin `w3=H-w0-w1-w2`. Les poids sont calculés avec **N brut**,
avant la normalisation simultanée du signe de `N` et de `2δ`.
Ainsi une permutation de déterminant négatif ne change pas la positivité.

## Tous les intermédiaires et choix de largeur

Soit `L` la plus grande étendue des quatre points sur un axe. Ils appartiennent
à un même cube de côté `L`. Pour trois points de ce cube, chaque composante
du produit vectoriel à ancrage commun est bornée par `L²` : c'est le déterminant
du triangle projeté dans un carré ; sa forme multiaffine atteint ses extrema
aux sommets du carré, où la borne se vérifie. Ce résultat ne s'applique pas
à deux vecteurs arbitraires dont les coordonnées sont seulement bornées par L.

Les calculs existants de coefficients restent natifs aux profils 18/21/24 :

| Intermédiaire | Majorant de la valeur absolue |
|---|---:|
| coordonnée de `u,v,s` ; chaque produit d'un cross | `L` ; `L²` |
| différence calculant un cross ; résultat à ancrage commun | `2L²` ; `L²` |
| norme carrée et ses sommes partielles | `3L²` |
| produits/sommes partielles de `δ` | `3L³` |
| produit composant `N` ; somme partielle composant `N` | `3L⁴` ; `9L⁴` |
| `2δ` | `6L³` |
| sommes partielles de `face` ; résultat | `3L²` ; `L²` |
| `δ²` ; `H` | `9L⁶` ; `18L⁶` |
| produit `N_j*normal_j` ; somme partielle du produit scalaire | `9L⁶` ; `27L⁶` |
| `w0` ; `H-w0` ; `H-w0-w1` ; `H-w0-w1-w2` | `45L⁶` ; `63L⁶` ; `90L⁶` ; `117L⁶` |

Les différences/cross/normes tiennent dans `i64`, les coefficients dans `i128`.
La somme formant `face` tient dans `i64` même avant la décision de certificat.
Si `L<=2^20`, **tous** les nouveaux intermédiaires sont `<2^127`, puisque
`117·2^120<2^127`. La boîte est contrôlée avant les multiplications de degré six.
Sinon le calcul repart des coefficients dans `Wide`, avec borne de magnitude
`6B+7` bits (`115/133/151` aux trois profils), produits à largeur totale et
additions vérifiées. Il n'y a ni troncation, allocation ni filtre flottant.
Tous les q4 du profil18 satisfont le certificat ; les profils21/24 ont les deux voies.

## Coût et qualification

La vérification réutilise les trois cross et `N` déjà nécessaires au centre.
Elle ajoute le contrôle de boîte, `δ²`, puis un à trois produits scalaires :
respectivement `4/7/10/10` multiplications scalaires selon le premier poids
refusé, ou dix si tous sont positifs, hors multiplications par constantes.
Les deux consommateurs faisaient déjà leur test strict avant l'admission aval.
En revanche, les autres appels à `through4` paient désormais cette propriété
même s'ils demandent seulement un centre. Le bilan dépend aussi du recours à
Wide ; ce décompte d'opérations ne constitue pas un gain de temps mesuré.

Le juge indépendant résout le centre par Gram/Fraction et les poids par un
système affine 4×4. Il compare candidat, matérialisation et fabrique Sphere,
y compris signes et échelles non réduites. Couverture : 24 permutations,
poids nul/négatif, face obtuse d'un q4 positif, dégénérescences, autres arités,
quadruplets différents d'une même coquille, extrêmes et seuil `2^20±1` translaté.
Normal et `python -O` donnent **1761 requêtes, 53877 contrôles, 114 corruptions**.
Attendus du raccord Fraction : B18 `491/14213`, B21/B24 `635/19829`
(requêtes/contrôles), et deux refus de syntaxe par profil.
Un défaut initial du juge masquait code et flux derrière un `require` générique.
Le raccord conserve désormais argv, code/timeout et extraits bornés stdout/stderr
avec longueur/empreinte, ainsi que la première requête non validée si localisable
(ce repère n'est pas une preuve de causalité). Les mocks normal/−O couvrent
13 scénarios et 58 contrôles, sans exécution native, y compris le raccord `run`.

| Groupe natif | Contrôles attendus statiquement | Plancher |
|---|---:|---:|
| `presentation` | 3648 | 3600 |
| `boundaries` | 53 en B18 ; 170 en B21/B24 | 50 |
| `foreign_and_owners` | 42 | 40 |

Ces attentes incluent les tailles `Sphere=128/144/160` et `Q4Candidate=80` ;
elles attendent encore la compilation et la qualification native G4.
La source gelée de **graph2 exclut ce port** : aucun résultat de cette campagne
ne le qualifie. À ce stade, seuls modèles Python, parseur, style et revues
statiques sont validés ; aucune qualification native, ABI ou performance nouvelle.
