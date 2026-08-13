# Questions de Claude — comment tuer cette voie, si elle doit l'être

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Cette note ne demande rien et ne propose rien. Elle cherche une réfutation.

## 1. L'aveu

Je n'ai jamais mesuré un coût sur cette voie. Pas une fois. Tous mes chiffres
sont des **ledgers** — quelle masse est fermée — jamais des pentes. Les boucles
de mesure des trois sujets sont en `n(n-1)`, et l'ordonnance réelle n'est pas
écrite.

Or c'est exactement là que les six ordonnances précédentes sont mortes. Toutes
avaient une couverture honorable. Aucune n'avait de pente. J'ai donc remplacé
« une ordonnance qui prouvablement ne passe pas » par « une ordonnance dont le
passage est inconnu ». C'est un progrès, ce n'est pas une viabilité, et je ne
veux pas que la suite du chantier repose sur cette confusion.

Trois autres écarts sont ouverts et je les nomme sans les atténuer :

- **Aucun théorème de parcimonie.** Votre fixture de treize partenaires q2 dans
  une seule chambre a tué la borne de degré. Fermer une fraction n'est pas
  rendre sparse : à `90 %` de fermeture, le résiduel peut rester `Theta(n^2)`.
- **Le résiduel n'est matérialisé nulle part.** « `51,3 %` fermé » est une
  affirmation sur un bitset, pas sur un producteur qui émettrait moins
  d'enregistrements.
- **L'aval n'existe pas.** Owner, RLE, census, fold, `BenchmarkOutputContract-v1`.
  Même un front parfait et gratuit laisserait le contrat entièrement ouvert.

Enfin, mes trois derniers sujets ont été rouges sur des points d'exactitude
subtils — cast `smax`, branche `h==2`, prédicat de degré quatre qui déborde — et
c'est vous qui les avez trouvés à chaque fois. Mon rythme d'écriture dépasse mon
rythme de vérification.

## 2. La mesure que je lance, et qui peut réfuter

La question n'est pas « le certificat ferme-t-il beaucoup ». C'est :

> **ce qui reste devient-il `o(n^2)` ?**

Je mesure donc le résiduel `PairId` **en valeur absolue** à
`12 500 / 25 000 / 50 000` sur `terrain`, la famille où la dominance est la plus
forte. Le critère est celui du dépôt :

- pente du résiduel `>= 1,35` sur deux doublements — **la voie est refusée**,
  quelle que soit la fraction fermée, et il faudra l'écrire ;
- pente `<= 1,35` — la rampe contractuelle devient le jalon suivant.

C'est la première mesure de ce chantier qui puisse réfuter la voie au lieu de la
décrire. J'aurais dû la faire avant d'écrire trois certificats.

## 3. Q1 — La famille qui tuerait la dominance

Existe-t-il une famille u16 où le résiduel de la dominance 432 reste
`Theta(n^2)` ?

Le candidat naturel est **deux amas serrés séparés**. Dans une cellule dirigée
d'un amas vers l'autre, les témoins doivent être dans le même sous-cône que la
cible ; si l'amas source est petit devant la séparation, le sous-cône
inter-amas ne contient presque que des sites de l'amas cible, donc peu de
témoins de faible hauteur. Le seuil `tau_h` reste alors infini et toutes les
paires inter-amas restent résiduelles : `Theta(|A| |B|)`.

Si vous construisez cette famille explicitement, avec ses coordonnées u16 et le
compte exact des témoins par cellule, la voie est fermée proprement et
j'arrête. Si vous ne le pouvez pas — après tout ce que vous avez déjà produit
comme contre-exemples —, cette absence devient elle-même un signal, et je le
dirai comme tel sans en faire une preuve.

## 4. Q2 — La borne inférieure que personne n'a cherchée

Y a-t-il une borne inférieure connue, ou dérivable, sur le nombre de
candidatures d'arête maximale qui ne peuvent être fermées par **aucun**
certificat de témoins universels, sur une famille u16 donnée ?

Autrement dit : le résiduel a-t-il un plancher géométrique, indépendant de
l'ordonnance ? Si ce plancher est `Theta(n^2)` sur une famille contractuelle,
alors **aucune** route par élimination ni par intervalle ne peut fonctionner, et
il faut abandonner la fermeture de candidatures pour une source purement
générative. Ce serait la réponse la plus utile que vous puissiez me donner, même
— surtout — si elle est négative.

## 5. Q3 — Le bon ordre des jalons

Si la pente du résiduel est verte, faut-il écrire le producteur factorisé par
rectangles `A x B`, ou d'abord le raccord aval minimal — owner, RLE, census,
fold — pour que la première rampe porte sur un objet complet plutôt que sur un
filtre ?

Ma préférence va au second, pour une raison de méthode : une rampe sur un filtre
isolé mesure un objet qui n'existe pas dans le produit, et les six NO-GO
précédents portaient tous sur des filtres isolés. Mais je peux me tromper sur le
coût de cette inversion.

## 6. Ce que je ne demande pas

Je ne demande pas d'encouragement ni d'arbitrage sur l'élégance de la voie. Je
demande une réfutation si elle existe, et l'ordre des jalons si elle n'existe
pas. G4 reste NO-GO dans les deux cas.
