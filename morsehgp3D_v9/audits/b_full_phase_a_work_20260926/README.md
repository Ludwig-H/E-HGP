# FULL : supprimer les barrières par niveau, sur tous les ordres

26 septembre 2026. Audit de `52ff41802`, sans changement de moteur, sans
GCP et sans commit. Profil u18 / grille 1 mm ; `not_claimed`.

**Proposition prioritaire : un sidecar de graphe événementiel qui calcule
en parallèle les groupes, parents et IDs de tous les ordres, puis écrit
la même tour explicite.** Le point nouveau par rapport au
[graphe temporel précédent](../PHASE_A_GRAPHE_TEMPOREL_20260923.md) est de
remplacer la construction encore indéfinie d'une « hiérarchie de
composantes avec maximum des marques » par des ancêtres pondérés, des
prédécesseurs d'événements et du saut de pointeurs. Le graphe de départ,
la forêt couvrante minimale et les deux coupures viennent des audits B
du 23 septembre ; ce dossier ne s'en attribue pas l'idée.

## 1. Ce qui coûte et ce qui doit rester

Lecture des 36 `vm/probe_N.stdout` de R24-B avec leurs empreintes de
commande ; les tableaux sont recalculés dans
[`ACCOUNTING.json`](ACCOUNTING.json), jamais copiés du résumé historique.
Médianes de deux processus, s8, tour seule CPU dans la chaîne GPU G4 :

| entrée / Kmax | recouvrement + E4 | sans E4 | sans recouvrement |
| --- | ---: | ---: | ---: |
| sans sol 00 / 5 | 285,7 ms | 296,1 ms | 326,4 ms |
| sans sol 00 / 10 | 1 196,3 ms | 1 307,5 ms | 1 548,7 ms |
| brute b00 / 5 | 649,6 ms | 668,7 ms | 730,5 ms |
| brute b00 / 10 | 2 420,5 ms | 2 606,7 ms | 3 093,7 ms |

Ce sont deux masques de la même trame 08/000000, pas plusieurs séquences.
L'absence de recouvrement change aussi la concurrence entre phases ; la
différence n'est pas le coût isolé d'une instruction. Les 36 lignes,
y compris les témoins moteur et s10/s12, restent dans le JSON.

| entrée / Kmax | représentants traités | nœuds explicites | contributions | parents + successeurs + verticales, u64 |
| --- | ---: | ---: | ---: | ---: |
| 00 / 5 | 3 621 785 | 1 541 750 | 897 776 | 37,00 Mo |
| 00 / 10 | 17 389 031 | 7 426 215 | 4 414 230 | 178,23 Mo |
| b00 / 5 | 7 387 583 | 3 532 035 | 1 992 129 | 84,77 Mo |
| b00 / 10 | 33 690 300 | 15 887 830 | 9 248 847 | 381,31 Mo |

La dernière colonne ne compte **que trois tableaux** de l'API actuelle,
pas les nœuds avec niveaux exacts, contributions datées ni banque de
populations. Les liens verticaux absents de K1 sont tout de même des
cases de la sortie actuelle. Ce n'est ni une borne minimale de tout
format possible, ni une mesure de débit ou de VRAM. Les millions de
nœuds ne sont pas un travail supprimable sous le contrat FULL explicite.

Dans le passage 00/K5 numéro 0, la phase 0 prend 84,0 ms, dont 48,7 ms
de résolution, 19,1 ms de collecte, 12,6 ms de classes et seulement
3,7 ms dans le champ historique `static_sort` (en mode haché, ce champ
inclut l'index de graines, ce n'est pas uniquement un tri). À 00/K10
numéro 7 : 733,0 ms, dont **556,3 ms de résolution** contre 12,5 ms
dans `static_sort`. Accélérer seulement le tri ne vise pas le gros coût.

La fenêtre nominale est `max_K(prêt(K)+A(K))`, avec des temps de
préparation reconstruits depuis les durées par K. À 00/K5 numéro 0,
annuler A(5) seule ne retire que **15,8 ms** à cette fenêtre ; à 00/K10
numéro 7, **0 ms**, puisque K8 termine après K10. Ce calcul à dates
`prêt(K)` figées n'est pas une prédiction de port : le placement et la
contention changeraient. Les deux répétitions et tous les K sont publiés.

## 2. Une fausse économie à ne pas poursuivre

Pour une boule régulière, écrire p=|I|, q=|U|=arité, m=p+q. Le produit
fait déjà exactement ceci (`visit_block_at`, `count_block_at`) :

- ordre K=m−1 : q représentants `I ∪ (U \ {u})`, donc au plus quatre ;
- ordre K=m, s'il est demandé : zéro représentant et une contribution ;
- aucun bloc aux autres ordres.

Il n'énumère **pas** tous les sous-ensembles de I∪U à chaque K. Le
petit contrôle combinatoire porte sur 39 configurations Kmax5/10,
q2/3/4, p admissible et retrouve au plus quatre représentants par boule.
Ce constat est déjà vrai dans le moteur : ce n'est pas un gain nouveau.
Les coquilles étendues conservent `ShellTable::rank(K)` et ses composantes
strictes ; remplacer cette table par la formule régulière serait faux.

La préparation d'un descripteur de boule peut être partagée entre ses
ordres, mais **pas l'identité d'une composante**. La résolution terminale
dépend de K et de sa fenêtre de rang. Une même clé géométrique n'autorise
pas à réutiliser une ancre d'un autre ordre. Le partage des requêtes
identiques et des graines de population existe déjà à l'intérieur de K.

## 3. Sidecar proposé : des événements, pas les graphes Γ exhaustifs

Conserver le front géométrique, le catalogue et la phase 0 exacts comme
producteurs témoins. Pour chaque K, un sommet par bloc présent dans
`programs[K]`, et les sites initiaux à K1. Une arête par représentant
effectivement émis vers sa **cible terminale**, pondérée par le rang
exact du niveau source. Les contributions restent des données du bloc,
pas des arêtes. Les portails silencieux restent dans ce graphe auxiliaire.
Les sommets `(K, BallId)` de deux K sont distincts.

### A. Obtenir les composantes ouvertes et fermées sans barrière par niveau

Une forêt couvrante minimale conserve toutes les composantes de tous
les préfixes de poids. Enraciner chaque arbre arbitrairement. Dans un
arbre filtré à un seuil, une composante possède un unique sommet le plus
proche de la racine. Le label de v est donc son plus haut ancêtre
atteignable sans traverser d'arête au-dessus du seuil.

Préparer par doublement `ancestor[v,j]` et le poids maximal jusqu'à cet
ancêtre. Chaque requête monte en O(log V), indépendamment des autres :
poids < λ pour les parents, poids ≤ λ pour les groupes du plateau.
Regrouper les nouveaux blocs par `(K, λ, label_fermé)` donne les mêmes
groupes que la DSU du lot. Les labels ouverts distincts de leurs cibles
donnent leur nombre de parents. On ne publie aucune fusion intermédiaire
des arêtes de même poids.

### B. Retrouver les IDs historiques sans requête « maximum des marques »

Pour chaque groupe, conserver un événement `(K, label_fermé, λ)`,
**y compris les continuations sans contribution**. Pour une composante
ouverte de label h demandée à λ, chercher le dernier événement de label h
à un niveau strictement inférieur à λ. Cet événement décrit exactement
son dernier état avant le plateau.

Pourquoi le prédécesseur existe-t-il ? Les composantes ne font que
fusionner. Dans l'arbre enraciné, un label ne peut évoluer que vers un
ancêtre ; une fois disparu il ne réapparaît pas. Tant que h est le label
d'une composante vivante, tout événement qui la modifie est enregistré
sous h ; sa première naissance l'est aussi. Un événement d'une autre
composante ne peut porter ce même label à cette date.

Trier les groupes dans l'ordre canonique `(K, λ, premier bloc)`.
Zéro parent ou plusieurs parents : un nouveau nœud, numéroté par préfixe.
Un parent : un pointeur vers l'événement précédent de cette composante,
sans nouveau nœud. Ces pointeurs vont strictement vers le passé ; un
doublement simultané les ramène en O(log G) tours aux événements qui
créent un nœud. Les racines historiques de tous les parents et toutes
les ancres sont alors connues en parallèle.

### C. Écrire toute la sortie demandée

Préfixes pour les emplacements des nœuds, parents et contributions ;
écriture des CSR, successeurs et ancres avec les IDs du témoin. Garder
les niveaux exacts **dans leur représentation actuelle**, notamment
le BallId du premier bloc du lot : le rang entier ne remplace pas
silencieusement le numérateur/dénominateur publié. Les contributions
d'une continuation sont conservées et datées ; les groupes sans sortie
restent comptabilisés. La banque de populations et les verticales ne
disparaissent pas du chrono.

Le premier sidecar conserve la phase C actuelle. Une extension possible
est de répondre aux images dans K−1 par le même index d'événements,
mais avec coupure **fermée**, plus une propagation depuis une naissance
descendante et vérification de naturalité sur tous les parents. Ce
remplacement n'est **pas testé ici** et doit rester un chantier séparé.

## 4. Travail, mémoire et preuve effectivement obtenue

Avec V sommets auxiliaires, E représentants et G groupes ≤V, l'aval
proposé a O((V+E) log(V+E)) travail avec une forêt minimale parallèle
à balayages géométriquement bornés, et O(V log V+E) espace dans cette
première variante à tables d'ancêtres. Le coût de construction de la
forêt, de son enracinement parallèle et de chaque table doit être payé.
Il n'y a pas E fois le nombre de niveaux et pas une barrière GPU par
plateau. Cela n'établit **aucune borne sous-quadratique globale en n** :
il faut encore mesurer V, E, sorties et leur croissance LiDAR réelle.
Les tables peuvent coûter plus de mémoire que le témoin ; ce n'est pas
un port motivé par une réduction de mémoire annoncée d'avance.

[`event_model.py`](event_model.py) compare une DSU chronologique
indépendante à ce schéma sur **3 004 graphes et deux enracinements**,
soit **6 008 égalités** des nœuds, parents, actions, successeurs et
ancres historiques. La forêt est construite par **Kruskal séquentiel**
dans ce modèle ; Borůvka GPU, enracinement parallèle, MEB, catalogue,
verticales et chaîne FULL ne sont pas implémentés ni qualifiés par lui.
Les quatre mutations sont détectées : forêt maximale, coupure fermée
pour les parents, événement silencieux oublié, ordre canonique du
plateau inversé. Deux donnent une sortie différente, deux violent une
identité causale ; aucun n'est simplement tué par crash.

Une contrelecture séparée ajoute [`countercheck.py`](countercheck.py) :
4 185 programmes exhaustifs à niveaux `[0,1,1,2,3]`, deux enracinements,
soit 8 370 égalités supplémentaires, sortie [conservée](countercheck_result.json)
identique en Python normal/−O. Les sauts testent le **maximum** des poids
de tout le chemin, pas seulement son arête finale ; aucune monotonie des
poids le long des chemins MSF n'est supposée. Les naissances K1 doivent
être explicites, les cibles dans le même K et strictement antérieures,
et tous les événements silencieux présents. Les niveaux rationnels,
allocations, priorités de refus, CSR et populations du moteur ne sont
pas qualifiés par ces modèles combinatoires.

`account.py` rejoue ce modèle et les 36 lectures brutes en Python
normal et `-O`, avec **sorties JSON identiques**. Les empreintes du
code produit lu et des sorties R24-B figurent dans le JSON. Le
worktree développeur relu (`3cf62b8ca`) ne modifie pas cette phase A :
ses changements v29 non publiés concernent le worker et son autotest
(pins, sceau et raison), pas un nouveau noyau de construction.

## 5. Porte d'implémentation recommandée

1. Export audit-only des descripteurs/offsets/cibles terminales **du même
   catalogue** K5 et K10, tous les K, une seule passe de collecte. Pas
   d'énumération indépendante Γ, pas de nouvelle géométrie approchée.
2. Sidecar CPU du graphe, puis MSF/ancêtres/groupes/pointeurs sur GPU,
   avec coûts et capacités séparés. Garder le moteur courant comme juge.
3. Comparaison littérale des nœuds, niveaux, parents, successeurs,
   contributions, ancres, populations et verticales ; pas seulement
   les partitions finales ou un digest. Rejouer cas non réguliers,
   portails silencieux, même BallId dans deux K, cibles après échange,
   plateau à trois parents et continuation contributive.
4. Refus de cible hors programme/niveau non strict, priorité des
   erreurs, dépassements d'indices et échecs d'allocation à qualifier.
   Les compteurs du nouvel algorithme doivent distinguer son travail
   physique de l'ancien travail logique, sans falsifier `tower_work`.
5. Mesurer toute la chaîne et l'expansion explicite, puis coupes LiDAR
   capteur et trames entières, s8/10/12. Une accélération de A ne résout
   pas seule les 100 ms : K10 garde 556 ms de résolution de phase 0
   dans le passage cité, et K5 doit encore payer q3/q4 et le census.

**À garder** : catalogue exact, représentants minimaux déjà réduits,
portails nécessaires, ordres séparés, plateaux et sortie FULL.
**À remplacer expérimentalement** : la dépendance séquentielle entre
millions de niveaux et les recherches de racine temporelles de phase A.
Ne pas financer une nouvelle série de petits gains sur les tris avant
ce sidecar structurel mesuré ; aucun gain G4 n'est acquis par cette note.
