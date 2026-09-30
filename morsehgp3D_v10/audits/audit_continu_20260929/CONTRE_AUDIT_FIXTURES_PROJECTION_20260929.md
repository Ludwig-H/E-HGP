# Contre-audit des fixtures de projection FULL → points

29 septembre 2026. `cpu_reference`, `quantized_u18_input_only`, hors registre,
`public_status=not_claimed`. Aucun build, moteur, CLI ou fichier d'un autre
acteur modifié ; aucun GCP. Copies corrigées examinées :
`build/v10-fixes/faits_math/src` et `faits_math-verif/src`.

Le lot est mathématiquement cohérent dans son domaine déclaré. F1–F4
établissent un recouvrement incompatible avec une partition exclusive,
une instabilité de première couverture, un croisement entre ordres et
l'optimalité de la constante 2ε de core. Elles ne qualifient pas une nouvelle
projection des points frontière ni un vote. Nous ne trouvons pas de nouvel
écart de l'oracle `Line` ; un contre-exemple abstrait supplémentaire précise
pourquoi un argmax réévalué à chaque coupe ne conserve pas la laminarité.

Preuves indépendantes :
[`projection_facts_corrected/`](../../receipts/audit_continu_20260929/projection_facts_corrected/).
Les JSON final normal et −O sont identiques et les deux commandes terminent
avec code 0. Les sources originales et leurs copies ont les mêmes hashes
avant/après. Le dossier distingue les résultats exécutés ici des résultats
développeur seulement observés.

## 1. Notre contre-oracle exact, borné et indépendant de la voie fenêtres

`check_projection_exact.py` construit Γ_k en énumérant **toutes** les
k-parties et (k+1)-parties. Pour une partie de sites alignés, sa miniboule
a rayon `(max−min)/2`, car les extrêmes imposent cette borne et leur milieu
la réalise. Les arêtes Γ sont datées par cette miniboule ; aucune fenêtre,
aucune sortie native et aucun départage du moteur ne sont repris dans ce
constructeur. Les partitions core y utilisent les dates d'entrée exactes
et le sommet k-NN. Les unions des k-parties de chaque composante donnent
les couvertures discrètes.

Nous confrontons ce constructeur à la classe `Line` corrigée sur les
26 sous-nuages de 2 à 5 sites de `{0,1,2,3,5}`, à **tous** les ordres
`K=1..n`, aux niveaux d'événement, entre événements et après le dernier :

| Contrôle indépendant | Nombre | Écarts |
| --- | ---: | ---: |
| couples nuage/K | 75 | 0 |
| partitions core, nombres de composantes et couvertures discrètes | 604 de chaque | 0 |
| entrées α et bornes `d_K/2 ≤ α_K ≤ d_K` | 235 de chaque | 0 |
| hauteurs de fusion core | 280 | 0 |
| niveau zéro ; cas K=n | 75 ; 26 | 0 |
| coupes ouvertes/fermées contre `hgp10_ref.point_partition_gamma` | 50 | 0 |
| stabilité sur huit déplacements appariés de trois sites, K1/K2/K3 | 72 paires et 72 entrées | 0 |

Ces nombres sont **par passage** : le normal et le −O réexécutent chacun
le même corpus de 75 couples et les mêmes contrôles. Ils attestent deux
exécutions identiques, pas 150 cas indépendants ni 1 208 partitions distinctes.

Les 50 comparaisons à la référence du dépôt utilisent ses circoncentres
et miniboules exacts généraux, pas la formule des extrêmes. Elles portent
sur de petits cas, dont F1, le témoin multi-K à cinq sites et K=n.
Ces tests appuient le contre-oracle ; ils ne remplacent pas la preuve
générale de stabilité ni un oracle 3D natif de projection complet.

Le raisonnement de `Line` est correct : projeter orthogonalement vers l'axe
ne s'éloigne d'aucun site. Chaque intersection de K boules fermées contient
donc, avec un point, tout son segment vers l'axe. La trace de L_K est
la réunion des intervalles `[s_{i+K−1}−r, s_i+r]` des fenêtres consécutives.
Composantes et traces correspondent ; pour un site sur l'axe, la distance
à une composante égale la distance à sa trace. Les cas alignés sont ainsi
de vrais cas 3D, tout en restant un domaine spécialisé.

## 2. Frontière : ce que F1 prouve effectivement

Notre Γ exact retrouve, pour `{0,2,4}`, K2 et r=1, les composantes-centres
`{1}` et `{3}`, de couvertures `{0,2}` et `{2,4}`. Core est vide à cette
date. Le site 2 participe donc **précocement aux deux amas discrets** ;
aucune partition exclusive ne peut publier simultanément ces deux blocs.

Les deux affectations immédiates admissibles sont `{0,2}|{4}` et
`{0}|{2,4}`. La porte accepte explicitement le second propriétaire comme
équivalent : elle ne fait pas d'un départage canonique une vérité physique.
Elle fixe toutefois le contrat actuel de première couverture : le mutant
qui diffère le point contesté jusqu'à la fusion est rejeté. C'est une
détection d'un changement de projection, **pas une preuve que l'ancrage
différé est mathématiquement invalide**. Il reste laminaire s'il respecte
un ancrage unique puis les ancêtres, mais perd cette participation précoce.

Les trois exigences « toutes les couvertures », « affectation exclusive
immédiate » et « partition laminaire » sont donc incompatibles dans F1.
Préserver la frontière au sens de toutes les appartenances demande de
conserver une relation de couverture/ambiguïté distincte de la partition
dure. Le lot ne propose ni ne qualifie cette relation supplémentaire,
une politique de décision, un seuil de confiance ou son coût.

## 3. Laminarité, vote et plusieurs ordres

À K fixé, les blocs core sont emboîtés par inclusion de L_K lorsque r
croît. Les singletons des points non entrés complètent la partition ;
ils ne doivent pas apporter une masse EOM avant la date d'entrée.
`cover` est également laminaire **par son ancrage durable**, puis la
remontée des parents. Cette garantie ne résulte pas de la géométrie du
recouvrement à elle seule.

Nouveau témoin exact de vote, sur un arbre abstrait à feuilles A1/A2/B :
un point q a les masses `(3/10,3/10,2/5)`. À la coupe fine, B gagne.
Après fusion de A1 et A2, la masse de A vaut `3/5` et A gagne contre B.
Il n'y a aucune égalité de scores ; les masses sont cohérentes et s'ajoutent
correctement dans les parents. Pourtant q passe d'une branche B à une
branche A qui n'est pas son ancêtre. Avec un point a propre à A et un point
b propre à B, les blocs `{q,b}` puis `{q,a}` se croisent. Notre script
vérifie explicitement l'échec du raffinement entre ces partitions.

Ce témoin réfute une garantie générale pour **argmax recalculé par coupe**,
même avec un vote doux cohérent. Il ne révèle pas de bug produit : le
correctif ne construit pas un tel vote. Pour obtenir une sortie dure
laminaire, décider une fois l'ancrage puis suivre les ancêtres est une
contrainte concrète à tester séparément. La fiabilité de la liste des
candidats et la pertinence statistique de cet ancrage restent ouvertes.

La **majorité stricte de masse totale fixe** est une autre construction,
non réfutée par ce témoin d'argmax. Sur un même arbre à fusions seulement,
des atomes fixes, positifs, datés et suivant les ancêtres donnent au plus
un propriétaire de masse `> W_x/2` ; une majorité acquise est conservée
dans l'ancêtre. Inclure les atomes futurs dans le dénominateur est essentiel.
La [note principale, §8–9](AUDIT_LAMINARITE_POINTS_20260929.md) donne cette
preuve et sa limite de rappel frontière. Nous avons relu le cas exact
`0,1,100,101`, K2 : les trois témoins propres ont β=`1/4,9801/4,1/4`
et fusionnent dans FULL à β=`2500`. Les deux points intérieurs n'ont que
la moitié de leur masse uniforme dans l'amas local ; les deux paires
ne sont donc jamais publiées avant fusion. Les poids `1/β` donnent au
contraire `4 > 4/9801` localement. Cette relecture du résultat déjà clos
n'est pas une nouvelle exécution native, ni une certification générale
de ces poids, d'une tête produit ou d'EOM.

F3 est aussi retrouvée par notre Γ : K1/r10 publie `{0,20,22}` et K2/r15
publie `{20,22,50,52}`. Les paramètres sont incomparables et les blocs
se croisent. Une tranche avec r croissant et K décroissant respecte les
inclusions ; la réunion libre de tous les ordres ne forme pas un arbre
unique de points. Ni une antichaîne ni un vote majoritaire ne suffisent
à réparer cette absence de laminarité sans règle supplémentaire déclarée.

## 4. Stabilité, zéro et domaine de K

La preuve 2ε est correcte : transporter les K témoins donne
`L_K^X(r) ⊆ L_K^Y(r+ε)` ; les segments entre points correspondants sont
dans `L_K^Y(r+2ε)`. Cela transporte les connexions core ; la symétrie donne
la valeur absolue. Les statistiques d'ordre des distances aux voisins
vérifient la même borne. F4 atteint 2ε avec deux points écartés chacun de ε.
La preuve porte sur les **rayons**, l'effectif et les identifiants appariés,
un K fixé, et les contacts fermés. Elle ne couvre pas EOM, suppression
de points, changement de K ni fidélité à des classes physiques.

Notre Γ recalcule aussi F2 sans oracle de fenêtres : pour L=1000 et 100000,
la fusion cover gauche–milieu passe de `(L−1)/2` à L, alors que core passe
de `L−1` à `L+1`. Le saut est donc exact et ne dépend d'aucun ex æquo.
La discontinuité est un énoncé sur la géométrie réelle quand δ→0 ; sur
une grille u18 finie, les fixtures prouvent une forte amplification et
l'échec du transfert de la borne 2ε. Elles ne prouvent pas l'absence de
toute constante Lipschitz sur un ensemble fini. Le registre explicite
déjà cette distinction dans la portée de F2.

Les formules testées exigent `1 ≤ K ≤ n`, sites distincts. À r=0 et K1,
la coupe fermée contient les n sites, core et cover ont entrée zéro ;
la coupe ouverte est vide. Pour K>1, L_K(0) est vide sur des sites
distincts. À K=n, Γ a un unique sommet, sans arête de (n+1)-partie :
les expressions de maxima vides sont bien traitées dans `Line`.
Une diagonale ultramétrique nulle est une convention distincte de la
date d'entrée. Rien ici ne qualifie λ(0), la condensation d'atomes lourds
ou la tour pondérée : celle-ci reste explicitement refusée. Sur des
observations coincidentes conservées séparément, la séparation positive
de deux labels doit être distinguée de la pseudo-ultramétrie obtenue
avant quotient par position.

## 5. Exécutions indépendantes et résultats seulement observés

Nos seules exécutions sont le script exact normal puis −O ; chacun
termine en moins d'une seconde, code 0, zéro écart et JSON identiques.
**Zéro exécution produit**, aucun gros build ou rejeu de campagne longue.
Le hash de `test_projection_facts.py` vaut
`855177a778f95315f102bc0aa0da8ee1512f72a5253f0d450ae8eca97295e4ea`
dans les deux copies développeur et notre snapshot, avant/après.

Résultats développeur observés et copiés, sans les réexécuter :

- `faits_math/apres/ctest_gate.log` : 10/10, `ctest_gate_code=0` ;
- `faits_math-verif/apres/ctest_gate.log` et son fichier de code : 10/10,
  `ctest_gate_code=0`, après une campagne encore active à notre première
  lecture ;
- les quatre appels directs de la porte sur référence/nouveau, normal/−O,
  sont terminés avec code 0 ; **chaque appel** annonce 37 exécutions
  natives et 133 contrôles, F1–F4, cinq mutants tués et l'autre propriétaire
  accepté. Ce sont quatre répétitions du même jeu, pas quatre corpus
  indépendants ;
- le contre-oracle développeur Line/Γ affiche son résumé terminal :
  224 contrôles de fixtures, puis 4110 sur 155 couples nuage/K, zéro
  écart. Le processus n'est plus présent. Son fichier de sortie ne contient
  pas de code de retour explicite : nous conservons cette différence
  avec le reçu CTest fermé.

Le premier CTest développeur interrompu est nommé dans `RECU_faits_math` ;
sa réussite ultérieure ne doit pas effacer cet essai. Les annonces ASan,
mutants compilés et différentiels supplémentaires de ce reçu ne sont pas
requalifiées par nous. Le lot grave des faits de projection ; il ne
certifie ni une nouvelle méthode frontière, ni un gain de clustering,
ni FULL à 100 ms, ni GPU/G4.
