# Prédicats q3 : essai i128 contrôlé puis repli exact

Source de départ : `e5f6a5683237005117a6b9f2c23bdc7eda6e3caa`.
La provenance ajoutée dans `tests/num/source_pins.json` conserve les révisions antérieures.
Cette tranche est préparée pour G4 ; aucun résultat natif ni gain de temps n'est acquis ici.

`power`, `side` et `power_bounds` gardent leur domaine public fermé : Sphere fabriquée depuis des Point
certifiés, ou Q4Candidate pour les deux premiers. Aucun coefficient arbitraire ni largeur locale de support
n'est accepté par ces API. Le helper `num/power_checked.hpp` reste interne.
Les voies natives déjà prouvées de q1/q2/q4 aux trois profils et q3 u18 restent inchangées.
Seul q3 avec `Budget::side>127`, donc u21/u24, essaie la nouvelle voie avant Wide.

## Preuve des intermédiaires

Poser $M=2^B$, $v=z-a$ et $H=D\lVert v\rVert^2-2N\cdot v$.
Les différences convertissent les coordonnées en signé avant soustraction ; $|v_j|<M$.
Les carrés et toutes les sommes partielles de la norme sont positifs et inférieurs à $3M^2<2^{50}$.
Chaque facteur $-2v_j$ a magnitude inférieure à $2M\leq2^{25}$.
Ces opérations i64, exécutées **avant** les contrôles, sont donc exactes pour B=18/21/24.
Les coefficients N et D sont déjà des i128 certifiés par les fabriques ; leur conversion n'est pas rétrécissante.

Le helper forme quatre produits avec `__builtin_mul_overflow` et trois sommes avec
`__builtin_add_overflow`. Ces opérations ne débordent pas en arithmétique C++ signée : le builtin signale
l'absence de représentation. Toute valeur produite avec un drapeau d'erreur est ignorée immédiatement.
L'optional n'est rempli qu'après la réussite de toutes les opérations ; sinon la formule Wide historique
repart des coefficients initiaux. Une annulation finale ne justifie jamais un intermédiaire débordant.
La conversion finale `to_wide` traite aussi i128 minimum par sa magnitude non signée, sans négation signée.
Les gardes historiques `require_fit<Budget::side>` restent actifs ; le repli ne change ni les niveaux ni leur ordre.

Pour une boîte fermée, les extrema séparables donnent deux normes inférieures à $3M^2$ et six facteurs
linéaires de magnitude inférieure à $2M$. Chacune des deux bornes utilise le même helper.
La publication exige les **deux** essais réussis puis `checked_bounds` ; si un seul refuse, les deux bornes
sont intégralement recalculées par l'ancienne voie Wide. Aucun carré de N ni nouveau degré dix n'est introduit.

## Témoins et limites de portée

- Triangle $(0,0,0),(L,L,0),(L,0,L)$ : au quatrième coin, $H=4L^6$ ; sur un sommet,
  $H=0$ mais le premier produit vaut $12L^6$. Aux grands profils, le repli est requis même pour le résultat nul.
- Support de largeur19bits, $L=2^{19}-1$, avec témoin aux coordonnées maximales u24 :
  la puissance dépasse i128. La largeur du support ne borne pas les témoins globaux.
- Triangle $(2,0,7),(1,6,2),(7,3,1)$ et témoin $(0,7,1)$, tous multipliés par269568 :
  les quatre produits tiennent, une somme partielle déborde, puis le résultat final tient.
- Triangle $(3,2,7),(7,6,8),(3,6,3)$ et témoin $(1,2,5)$, multipliés par476192 :
  les produits tiennent mais la puissance finale dépasse i128 ; ignorer le débordement d'addition donne
  une valeur publique incorrecte, sans qu'un signal ou UBSan soit nécessaire pour tuer le mutant.
- Des boîtes déclenchent séparément le débordement de la borne basse, haute, ou des deux ;
  les boîtes singleton, contacts exactement nuls et six permutations des supports sont conservés.

Le juge Python calcule le centre par Gram/Gauss rationnel, la puissance depuis centre/rayon,
les bornes par extrema rationnels séparables et leurs inclusions dans les extrema continus.
Il juge séparément les indicateurs du helper par entiers Python sans limite ; ces indicateurs décrivent
un essai interne, pas un compteur de performance public. Les tests C++ comparent les résultats publics
à une référence qui reste entièrement Wide, puis exercent les limites synthétiques i128 du helper.

Les modèles normal et `-O` ont passé329requêtes par profil, avec3542/3546/3548contrôles et15corruptions
refusées par profil. Les portes natives préparées attendent12contrôles pour les limites et259/266/273
pour les prédicats publics u18/u21/u24 (plancher250). Six mutants supplémentaires donnent38mutants num ;
leur compilation et leur mort causale restent à qualifier sur G4. Le premier script exploratoire de recherche
de témoins s'est arrêté sur une division par zéro pour quatre termes nuls ; ce cas a été exclu de cette recherche,
avant fixation des témoins, sans exécution du produit ni changement des planchers.

Le taux de réussite de l'essai, le coût des contrôles et la pénalité d'un essai suivi du repli ne sont pas mesurés.
Une égalité exacte des résultats ne démontre pas une accélération du catalogue ou de FULL.
