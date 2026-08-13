# Note de Claude — plan de route vers le contrat 50 k, puis vers `10^7`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Ceci est un recul, pas un résultat. Je le soumets pour être réfuté sur l'ordre,
pas seulement sur les chiffres.

## 1. Le constat qui m'implique

`morsehgp3D_v3` contient **42 sondes** et **720 CTests**. La chaîne ne produit
**aucun objet** : ni `BallKey`, ni `SupportKey` avec ses ensembles `I_B/U_B`, ni
census, ni fold, ni payload. Vos audits le répètent depuis `af08b0e` et je ne
l'ai jamais fait.

Ce n'est pas un oubli de priorité, c'est une **erreur de méthode**. Un certificat
qui ferme `96 %` d'une masse ne se juge pas contre un autre certificat ; il se
juge contre l'objet qu'il est censé préserver. Sans sortie, chaque mesure est
non falsifiable au sens du contrat, et le nombre de portes vertes ne mesure que
ma propre activité.

Le verrou principal n'est donc pas mathématique. Il est que **la boucle n'a
jamais été fermée**.

## 2. Le budget, chiffré, pour savoir de quoi on parle

Le SLO primaire est `p95 warm_e2e < 100 ms` à `n = 50 000`, `K_max = 10`, sur un
seul G4, sortie matérialisée et GPU synchronisé compris. Le `< 1 s` n'est que
secondaire. Le moteur de référence mesuré fait `78,841 s` sur `uniform`, soit
environ **790 fois** la cible primaire.

À `100 ms` et une bande passante de l'ordre du To/s, la passe entière peut
déplacer environ `100 Go`. Le rejeu de l'audit donne `315,7` millions de
recertifications à `n = 6 000`, `s=8`, soit `52 600` par point. Si ce taux tient
à `50 000`, cela fait `2,6 \cdot 10^9` recertifications ; à quelques dizaines
d'octets chacune, on est **à un facteur de trois à dix du budget**, pas à un
facteur mille.

C'est un constat encourageant et exigeant : **le combat est sur les constantes,
pas sur l'asymptotique** — à condition que le résiduel soit linéaire. Les
`790 \times` viennent de l'ordonnance CPU par paire, pas d'une impossibilité.

## 2 bis. Ordre corrigé par l'audit, et état

L'audit `1aa487d` confirme l'ordre et le corrige sur trois points : la tranche
verticale doit être **complète et bornée**, alimentée par l'oracle exhaustif ;
le changement de profil numérique n'est **pas** une étape quatre de la
qualification v3 mais un successeur formel ; et la couture
`ExactKernel / SphereIdentity` doit être préparée maintenant sans implémenter
`binary64`.

```text
0A  BallForm -> BallEvent exact et politique de degenerescence     [FAIT]
0B  oracle exhaustif borne -> fold -> payload complet
1   remplacer SEULEMENT la source par E3/E4, mesurer E/M/BallRuns
2   Q3 owner-edge + PrimitiveSphereKey + range-count, shallow q4
3   portage device apres parite de TOUTE la tranche
4   changement de profil numerique, phase separee
```

**Étape 0A close.** `prototype/ball_event.hpp` et sa sonde produisent
`PrimitiveSphereKey`, `SupportKey`, `I_B`, `U_B`, owner et disposition sur un
petit nuage. Le juge est indépendant par sa route — Gram dans la base du
support, centre et rayon rationnels, comparaison par produit croisé — là où le
sujet emploie la forme close `W` et un Cramer en coordonnées sans jamais former
de centre. Zéro désaccord sur trois familles ; `grid` porte `15` cosphères sur
`33` supports, `86` formes dégénérées et `58` refus de domaine. Huit portes,
dont trois mutants — owner par index, coquille comptée intérieure, clé non
réduite — et un refus de domaine du juge au-delà de `coord=64`.

Un de mes trois mutants a d'abord survécu, et c'était ma faute : j'avais
désactivé sous injection la comparaison même censée le tuer. Le juge n'est plus
jamais désarmé par une injection.

## 3. Les cinq étapes, avec leurs portes et leurs critères de mort

### Étape 0 — le squelette qui produit l'objet (le verrou jamais franchi)

Recevoir `BallFormToBallEvent-v0` à petit `n` : d'une arête et de son tape de
formes, produire `(BallKey, SupportKey, I_B, U_B, owner)`, puis un **fold
streamé**, jamais un catalogue matérialisé.

- jugé contre les oracles exhaustifs bornés et contre v2, sur les **ensembles**
  et non sur des comptes ;
- équivariance par permutation, tuilage, ordre des événements ;
- la contrainte `10^7` s'applique **dès ici** : si le fold a besoin d'un
  catalogue global, il faudra tout réécrire. Le layout se décide maintenant.

**Porte** : accord exact des ensembles sur toutes les fixtures bornées.
**Critère de mort** : si le fold ne s'exprime pas sans catalogue global,
l'architecture est réfutée et il faut changer d'objet, pas d'optimisation.

*Pourquoi en premier* : rien en aval n'est falsifiable sans lui, et c'est lui qui
fixe l'ABI que le portage device devra honorer. C'est aussi votre directive
depuis `ab32c9d`, et je ne l'ai pas suivie.

### Étape 1 — rendre le candidat petit sur les cinq familles

Trois leviers, dans cet ordre :

1. le certificat **anisotrope sur rectangle**, avec l'intervalle exact de
   `T = \lVert z-a\rVert^2 - \lVert z-b\rVert^2` que votre `§6` donne et que
   j'ai vérifié exhaustivement. Le certificat en production teste aujourd'hui la
   boule **inscrite** ; sur `eight_clusters` l'exact fait passer la fraction de
   cœur vide de `13,8 %` à `0,9 %` et le taux fermable de `71,3 %` à `95,0 %` ;
2. le **raffinement local** des seuls terminaux ouverts, piloté par les marges
   exactes, jamais jusqu'à la feuille, avec tâches et HWM bloquantes ;
3. le `s` par lane au-dessus de son seuil (`2`, `2\sqrt{3}`,
   `\sqrt{209/14}`), choisi par coût composé et non « juste au-dessus ».

**Porte** : pente `sum E_4 < 1{,}35` sur les **cinq** familles et plusieurs
graines, `max E_4` borné, `fenetre_finale=OUI` avec `pending=0`, ledger massique
exclusif.
**Critère de mort** : si `eight_clusters` reste au-dessus de `1{,}7` après les
trois leviers, la route du certificat central est réfutée pour les nuages en
amas, et il faut passer aux cages et fleurs que vous proposez.

### Étape 2 — le coût par arête, et seulement alors les moteurs locaux

`EdgeActiveFormCounter-v0` publie `M = \sum_e m_e`, tâches, blocs, octets, HWM.
Ensuite seulement : `Q3FootPowerRange-v0` sur les blocs porteurs que je viens de
livrer, et `LocalShallowBall-v0` par niveaux `P-P/N-N/P-N` pour q4.

**Porte** : `M` linéaire, coût par arête borné, `J` et `H` publiés.
**Critère de mort** : `M` superlinéaire malgré une fenêtre linéaire signifie que
la fenêtre ne borne pas le travail, et la métrique de l'étape 1 était la
mauvaise.

### Étape 3 — le portage device, puis le chronomètre

`count -> scan -> fill`, résident, aucune file dynamique, aucune récursion. La
parité avec l'oracle borné est exigée **avant** toute mesure de temps. Puis les
six préflights, puis la **porte structurelle** — zéro allocation indexée par les
univers de facettes ou cofaces, zéro cellule top-m persistée — qui invalide
l'architecture même si le temps est bon. Puis seulement `warm_e2e` : 30 nuages
frais à `50 000`, p50, p95, maximum, écart médian et **chaque valeur brute**.

**Critère de mort** : un `p95` au-dessus de `1 s` après constantes travaillées
signifie que l'ordonnance ne tient pas, et il faut revenir à l'étape 1 plutôt
que d'optimiser un facteur deux.

### Étape 4 — la montée

`1 M`, `10 M`, `30 M`, strictement séquentiels et chacun gardé par le précédent.
Deux contraintes structurelles s'imposent **avant** d'y arriver :

- le profil d'entrée doit sortir de `u16`. Les familles du plan enregistré sont
  en `binary64` ; la couche d'exactitude doit basculer sur les prédicats FP
  certifiés que `morsehgp3d/` possède déjà, et ce basculement se prépare à
  l'étape 0 dans l'ABI, pas à l'étape 4 ;
- aucune matérialisation par support. Le reçu G4 mesure `21,4` millions de
  supports à `50 000` points, soit environ `600 Mo` ; le même taux à `10^7`
  donnerait `4,3 \cdot 10^9` supports. C'est impossible, donc le fold streamé de
  l'étape 0 n'est pas une élégance mais la condition d'existence du palier `10 M`.

## 4. Ce qu'il faut arrêter

- **arrêter d'ajouter des certificats** tant que l'étape 0 n'est pas verte. J'en
  ai proposé quatre en deux jours ; aucun n'a jamais rencontré un objet ;
- **arrêter les sondes neuves.** 42 fichiers, dont beaucoup portent des routes
  abandonnées. Toute mesure nouvelle doit passer par une sonde existante ou
  remplacer la sienne ;
- **arrêter de mesurer sans banque.** Mon ramp q3 d'aujourd'hui mesurait une
  tautologie — sans certificat, toutes les paires sont ouvertes, donc les blocs
  sont quadratiques par construction ;
- **arrêter de citer une famille pour cinq.** `uniform` passe des portes que
  `eight_clusters` refuse, et c'est `eight_clusters` qui décide.

## 5. Les deux risques que je ne sais pas encore chiffrer

**Le premier** est le coût du census. Chaque boule acceptée exige ses ensembles
`I_B/U_B` exacts, donc un comptage sphérique. Si ce comptage est un rescan du
nuage par boule, il est quadratique quelle que soit la parcimonie amont. Le
`BallKey`-first avec RLE puis un seul range-count saturé par boule unique est la
réponse que vous proposez ; je n'ai aucune mesure de son coût, et c'est le
chiffre manquant le plus dangereux du plan.

**Le second** est la cosphère lourde. Votre fixture à `384` points sur un cercle
porte plus de deux millions de `SupportKey` sur une seule `BallKey`. Sur un
profil `u16` adversarial, cette multiplicité n'est ni un cas rare ni un cas
borné, et la route produit doit la conserver sans perte, appliquer le quotient
reçu, ou rendre `unsupported_degeneracy`. Je ne sais pas laquelle des trois, et
le choix change le layout de sortie — donc l'étape 0.

## 6. Trois questions

1. Confirmez-vous l'ordre — squelette de sortie **avant** parcimonie — ou
   jugez-vous qu'une fenêtre non linéaire rend le squelette prématuré ?
2. Le basculement `u16 -> binary64 certifié` doit-il être préparé dans l'ABI de
   l'étape 0, ou est-ce une couche séparable qu'on peut ajouter à l'étape 4 sans
   réécrire le fold ?
3. Pour la cosphère lourde : conserver la multiplicité, appliquer le quotient,
   ou refuser le domaine ? C'est le seul des trois choix qui n'est pas
   réversible une fois le layout de sortie figé.

## 7. Non-claims

Aucun chiffre de ce plan n'est un temps qualifiable. Le budget du `§2` emploie
une bande passante d'ordre de grandeur et une extrapolation linéaire d'un taux
mesuré à `n = 6 000`, ce qui n'est ni une mesure ni une borne. Les pentes citées
sont mono-graine sur des plages finies. Aucune étape de ce plan n'est reçue, et
son ordre est précisément ce que je soumets à réfutation.
