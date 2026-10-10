# LEM-T1 : chercher seulement les sites hors du support

10 octobre 2026, source produit B3-K `a05e4f5a2`. Proposition non appliquée,
aucune compilation ni exécution native. Le changement vise les recherches
de populations dans `resolve.cpp`, à tous les ordres ; il ne change ni la
table B3-K ni son transfert.

## Énoncé et authentification

Après une réponse exacte `find_support(S)=b`, le contrat du Catalogue
immuable garantit **S=S*(b)⊆U_b**. Si S⊆F a été contrôlé, on a donc

`F⊆I_b∪U_b  ⇔  F\S⊆I_b∪U_b`.

La preuve est l'inclusion déjà acquise des sites de S dans la coquille.
Les seuls sites capables de faire échouer le test sont dans F\S. Les garder
dans leur ordre antérieur conserve le même premier rejet. Un curseur dans
S trié permet d'omettre leurs recherches sans allocation ni nouveau tableau.
Lorsque |S|=|F|, aucune recherche de population n'est nécessaire.

Les **deux appelants** du helper privé sont couverts :

- `lem_t1` conserve sa garde d'arité et `sorted_subset(S,F)`, y compris pour
  les appels directs des tests. Il ne passe S au helper qu'après succès de
  `find_support(S)`. `table_miss` reste faux pour support invalide ou partie
  extérieure, vrai seulement pour une recherche support réellement absente.
- La seconde recherche de `locate` utilise **`cert->support`**, pas la
  proposition flottante. `certify_part` a reçu un support authentifié par
  `sorted_subset`, ou par `exact_support`. Son support canonique est choisi
  parmi les sites de F sur la sphère ; il peut différer de `prop.s`. C'est
  exactement ce support certifié que la seconde table vient de retrouver.

Il n'y a **aucune** suppression du contrôle S⊆F, de la recherche exacte
S*→boule, des tests sur F\S, du contrôle de décroissance, du census ou de
sa confrontation au catalogue. Le helper reste privé ; ce n'est pas une
API acceptant arbitrairement un « support connu ».

Sur un Catalogue contractuellement valide, mêmes décisions, cibles, routes,
compteurs logiques et refus de résolution. Le changement ne prétend pas
conserver la détection d'une **corruption interne** où S* manquerait à U :
l'ancien scan pourrait la découvrir incidemment, le nouveau s'appuie sur
l'invariant déjà garanti par le constructeur et ses portes. Le modèle
contient cette obstruction explicitement. Une table corrompue ne devient
pas un domaine autorisé ; ses contrôles dédiés restent indispensables.

## Différence avec les leviers antérieurs

Le reçu du 7 octobre `audit_g_pistes_20261007` faisait déjà omettre les
prédicats de côté sur le support **après certification géométrique** dans
`certify_part` ; ce port est présent. Ici, l'authentification provient de la
table exacte du Catalogue et l'on omet des **recherches d'identifiants** dans
`part_in_population`, qui relit encore tous les sites sur B3-K.

B3-B, retiré, remplaçait les dichotomies par un balayage ; cette proposition
conserve les mêmes dichotomies sur les seuls sites restants. Elle ne reprend
pas B3-B. Pas de nouveau cache, de nouveau support ou de nouvelle route.

L'ordre k=2 a toujours F=S, mais le raccourci s'étend à q=3/4 et aux parties
plus grandes. Omettre DWelzl pour k=2 est mathématiquement sûr, mais déjà
présent dans `exact_small_support(n==2)` du levier L4 rejeté ; aucune nouvelle
qualification isolée n'en découle. Ce reçu **ne modifie pas la proposition**.
Un échec de table sur une paire n'autorise pas à supprimer le census : la
diagonale non canonique d'un carré peut demander un census complet, et
WIT-D2 un census saturé. Ces portes existantes doivent rester vertes.

## Coût et portes proposées

Chaque site de S effectivement visité par l'ancien scan déclenchait une
recherche dans I, nécessairement négative, puis une dans U, positive.
Le raccourci supprime donc exactement **deux appels de recherche par site
de support rencontré avant le premier rejet**. Sur succès, cela fait 2q
appels ; sur échec précoce, parfois aucun. Ce sont des appels, pas un compte
de comparaisons ni un gain de temps : les branchements/cursor ajoutés et la
localité peuvent changer le résultat mesuré.

`proposition.patch` ne modifie que le helper privé et ses deux appels, sur
le pin indiqué. Le modèle vérifie la postimage et le diff exacts ; il ne
compile ni n'applique le patch au produit.

Portes natives utiles avant adoption :

- `lem_t1_table`, `witness_t1_square`, WIT-D2 et garde temporelle WIT-MEMO ;
  supports hors F, mal triés ou répétés, absence de table, sans changement
  de la sémantique de `table_miss`.
- q=2, 3 et 4 avec F=S, puis avec des sites de F\S dans I, dans U et hors
  population ; un site extérieur avant, entre et après les sites de S dans
  l'ordre des identifiants. Le mutant « omettre aussi le dernier site hors
  S » doit produire un faux succès et être tué causalement.
- Route post-certificat avec support canonique différent de la proposition
  et route de repli. Le carré conserve sa canonisation globale et le census.
- Comparaison G exacte et compteurs logiques à W1/W48, séquentiel/recouvert,
  puis FULL et registre. Mesurer le mur FULL sur règle préenregistrée ;
  aucune extrapolation du pourcentage historique LEM-T1 au gain du produit.

Exemples géométriques pour q=3/4 : triangle aigu
`(0,0,0),(4,0,0),(2,4,0)` avec site intérieur `(2,2,0)`, et tétraèdre
`(0,0,0),(0,2,2),(2,0,2),(2,2,0)` avec intérieur `(1,1,1)`. F égal au
support est une partie propre de la population, donc ne se réduit pas
nécessairement à un arrêt immédiat sur la table des naissances. Faire varier
les identifiants par placements admissibles, pas en changeant leur contrat.

## Rejeu borné

`python3 -B -S [-O] check.py --repo <repo>`

**24 300 configurations** de six identifiants, q=2/3/4, F/I/U indépendants
sous S⊆F et S⊆U, I∩U=∅ : 12 250 succès, 12 050 rejets identiques ; même
premier rejet et même ordre des recherches restantes. 1 890 cas F=S n'ont
plus de recherche. Six supports invalides, miss authentifié, extérieur
hors support et contre-exemple de Catalogue invalide. Modèle d'inclusion
et de contrôle, pas oracle géométrique ni qualification native. Relectures
normale et optimisée réussies ; aucun temps annoncé.
