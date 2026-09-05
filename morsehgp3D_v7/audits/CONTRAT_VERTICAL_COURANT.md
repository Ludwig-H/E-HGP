# Verticales : preuve et raccord conservés

La verticale descend de K+1 vers K **au même niveau géométrique et au même côté de coupe**. Garder le même seuil de densité normalisée en changeant K changerait le rayon. `public_status=not_claimed` ; aucun export vertical produit n’est certifié ici.

## Cible unique et naturalité

Chaque label S de cardinal K+1 devient une coface à l’ordre K : ses K-faces sont connectées. Deux labels source adjacents partagent une K-face ; leurs groupes inférieurs se rencontrent. Un chemin source donne donc une unique composante cible, indépendamment de la face choisie. L’argument conserve les coupes ouvertes/fermées et n’exige pas de régularité. Le théorème 2 du manuscrit le transporte aux régions témoins.

L’inclusion des composantes vers un niveau supérieur conserve les faces témoins : les carrés horizontaux/verticaux commutent. Descendre plusieurs ordres donne la même cible que la composition des descentes adjacentes. La couverture source est incluse dans la cible, sans égalité, bijection ou conservation de masse imposée. Une couverture de points ne permet pas d’identifier la cible dès K≥2.

Le rejeu E est qualifié sur les coupes positives et l’état initial zéro fermé, avec racines K1 normatives ; il ne qualifie pas zéro ouvert, qui relève du contrat FULL distinct.

Une source, même isolée dans FULL, engendre une cible non triviale en bas. La restriction réduite reste donc stable par descente. À K=n, FULL garde sa feuille terminale tandis que le réduit est vide ; E ne fournit que les ordres de sa propre fenêtre. Aucune carte vers un ordre absent n’est inventée.

## 5. Construction totale depuis `born` et `parents`

Cette construction concerne les **vraies naissances réduites E** (`parents=[]` dans l’histoire qualifiée), pas l’entrée dans une vue filtrée par taille.

Soit Q une coface du lot de naissance à l’ordre k+1, de cardinal k+2 et de niveau a. Retirer un essentiel u donne S de niveau strictement inférieur. Si un point étranger z appartenait à la boule fermée de S, S+z aurait déjà une incidence supérieure avant a. Q toucherait alors une composante non triviale ancienne, contredisant la naissance sans parent. S est donc Gabriel ; sa régularité pertinente est garantie par le census E, et toutes ses k-faces sont présentes dans le catalogue inférieur avant a.

À une naissance, aucune facette incidente du lot n’est déjà `seen` : toutes les faces de Q figurent dans `born`, dont S. Parcourir les labels de `born`, retirer pour chacun son plus grand PointId et chercher cette face dans l’état inférieur **fermé au niveau a**. L’essai correspondant à S réussit ; tout succès plus précoce désigne aussi la cible unique. Il faut au plus `|born|` lookups, sans MEB, census ou resolver géométrique général. Un parcours interrompu refuse ; l’absence de token n’est jamais une absence mathématique de cible.

Conserver cette ancre à la naissance, la propager aux continuations, normaliser les ancres parentales au même état inférieur à chaque fusion et vérifier leur accord. Les successeurs inférieurs font avancer les tokens historiques. À la consultation ouverte, appliquer seulement les lots strictement inférieurs ; à la consultation fermée, fermer aussi le lot égal. Les numéros de batches locaux ne sont pas une horloge commune.

Pour FULL, la route plus directe utilise chaque minimum comme directe de l’ordre inférieur, avec son ancre après fermeture du plateau. Le scan `born` réduit ne lui est pas transféré implicitement. Entrée, hashes des deux payloads, ordres, coupe exacte et succès terminal doivent accompagner la carte exportée.

## 6. Contre-fixtures et reçus bornés

Les [reçus du lecteur](receipts_resolver_20260905/README.md) conservent, par provenance scellée E O2/UBSan, 764 cartes, 720 carrés et 400 compositions sur les 16 sorties originales. Un réindexage exige cinq misses avant le sixième succès ; une multifusion source synthétique est séparée du corpus produit. Trois corruptions du lecteur et un budget nul sont refusés. Les [premières sondes](receipts_vertical_20260905/README.md) restent attribuées à leur modèle.

Ces preuves ferment la méthode de reconstruction. Son intégration, les plateaux hors domaine et l’autorité des [masses](CONTRAT_MASSES_VOTE_COURANT.md) demeurent des contrats distincts. Le resolver géométrique arbitraire n’est pas une exigence de cette route ; les sujets différés sont regroupés dans [un seul fichier](QUESTIONS_SECONDAIRES.md).
