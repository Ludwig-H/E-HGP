# Census emprunté, un parcours exact

Cette brique ajoute une primitive index optionnelle. `census` possédé garde
son algorithme de comptage puis remplissage et reste la référence. Aucun
raccord à la descente, aux verticales ou au moteur FULL n'est réalisé ici.
Aucun gain temporel ni qualification native n'est annoncé avant G4.

`CensusWorkspace::make(index, budget)` possède exactement `n` `SiteIdx`
dans un `Buffer`, soit `4n` octets réservés. Son objet de contrôle a une taille
constante et une allocation distincte hors compte Buffer. La factory rend
un `unique_ptr` ; l'objet lui-même ne se copie et ne se déplace pas. Il
n'expose aucun alias mutable de son stockage. L'index d'origine et le budget
doivent lui survivre ; l'index reste immobile pendant son utilisation.
Le déplacement du pointeur propriétaire conserve l'adresse du workspace.

`query(index, sphere, threshold, context, callback)` acquiert une garde
atomique, valide l'identité de l'index, sa cardinalité, le seuil et le
callback, puis effectue le calcul. Une requête concurrente ou réentrante,
un index étranger ou déplacé, un seuil nul ou un callback nul sont refusés
avec `parameter_out_of_range`. Le propriétaire et l'index restent vivants
pendant tout l'appel. Leur destruction concurrente est hors contrat.

Le callback reçoit un `BorrowedCensus` non copiable, contenant uniquement
des vues constantes certifiées, le type de certificat et le travail réel.
Il est appelé après le succès complet du parcours. Les vues expirent au
retour du callback : C++ n'empêche pas de conserver frauduleusement un span,
le contrat ne prétend pas le contraire. Un consommateur capture des valeurs
dans un brouillon et ne les publie qu'après succès. Un refus du callback
est propagé ; `bad_alloc` devient `memory_budget`, toute autre exception
`task_exception`. La garde est libérée sur chacun de ces chemins.

## Stockage et ordre

L'index construit récursivement la plage gauche `[begin,mid)` avant la
droite `[mid,end)`. Le parcours préfixe avec `escape` ne revient jamais sur
une plage antérieure ; les boucles des feuilles et des blocs intérieurs
énumèrent les `SiteIdx` croissants. C'est l'ordre des sites Morton déjà
certifié par Cloud, pas l'ordre des `PointId` externes.

Les intérieurs sont écrits depuis le début du tampon. Chaque point de
coquille est empilé depuis la fin. Un site appartient à une seule des
deux listes et n'est traité qu'une fois, donc `p+m<=n` : aucun chevauchement,
même quand une des listes ou leur réunion occupe tout le tampon. Le code
garde aussi les additions réelles avant écriture.

À saturation, les `threshold` premiers intérieurs sont publiés et la
coquille est vide. Sinon, l'unique inversion de la tranche finale rend
toute la coquille croissante. Les extrémités et contacts sont conservés.
Le support de la sphère peut être extérieur au nuage ; aucune borne sur
la taille de la coquille autre que `n` n'est supposée.

Le parcours possède les mêmes choix, arrêts, bornes et prédicats que
chacune des deux passes du census possédé. Son ledger compte une passe,
sans multiplier artificiellement ses compteurs. La comparaison native
exige exactement le double pour les six champs du ledger historique.
L'inversion et les écritures ne sont pas des tests géométriques.

## Mémoire et futur raccord

Aucune allocation propre, réservation ou manipulation de compte partagé
n'a lieu dans `query` ; les allocations éventuelles du callback lui
appartiennent. Le même tampon se réutilise après retour, sans conserver
la capacité d'une ancienne requête variable : la capacité est fixée au
nuage d'origine. Ni croissance par doublement ni allocation paresseuse.

Un futur pilote peut admettre `4nC` octets avant le Pool, avec
`C=min(W,L,Q)` slots d'exécution simultanée, et construire C workspaces.
Ce calcul ne prescrit pas une workspace par lane logique. Après jointure,
le pilote sériel peut réutiliser un de ces slots. Les tableaux et mémos
existants restent comptés simultanément. Un refus de factory ne transfère
aucune propriété de l'index ; un refus de requête ne rend aucun certificat.
Le contenu résiduel privé du scratch n'est pas une sortie persistante.

## Portes préparées

- `borrowed_test.cpp` : parité avec le census possédé, seuils 1/n et u32max,
  intérieur plein, coquille coplanaire pleine, réunion I/U pleine, entrée
  modifiée, domaine étranger/déplacé, budget exact/refus, callbacks en erreur,
  réentrance et concurrence déterministe, puis quatre workspaces privés.
- `borrowed_fault.cpp` : les deux allocations de factory refusées séparément,
  récupération, puis 120 requêtes sous refus système total sans un seul
  appel d'allocation propre.
- `borrowed_probe.cpp` et `borrowed_oracle.py` : 1 010 requêtes du juge
  Gram/Fraction existant par profil, appariées au census possédé, 47 paires
  de permutations, premiers K intérieurs exacts et mémoire `4n`.
- Le modèle pur vérifie 3 030 réponses aux trois profils et refuse 36
  corruptions. Il contrôle le lecteur ; il ne qualifie pas le C++.

Branchement proposé, laissé au pilote : ajouter `census_workspace.cpp`
aux sources index ; unité `mhgp11_index_borrowed` avec groupes `fixtures
ownership callbacks concurrency`, faute `mhgp11_index_borrowed_fault` avec
groupe `starvation`, cible `mhgp11_index_borrowed_probe`. Portes Python
`borrowed_oracle.py BORROWED_PROBE OWNED_PROBE` (180s) et
`borrowed_oracle.py --selftest` (60s), chacune avec sa jumelle `-O`.
La seconde attend `borrowed_model_verdict conforme positives3030 corruptions36 native0`.

Les mutations proposées omettent l'inversion de U, exposent U après
saturation, ou livrent les derniers plutôt que les premiers intérieurs.
Elles doivent être tuées par un certificat géométrique incorrect, après
compilation réussie. Les sources originales `index/build.cpp` et les corps
de `index/census.cpp` restent la référence ; seul l'accès interne aux nœuds
est déplacé dans `index/access.hpp` pour éviter deux définitions divergentes.
