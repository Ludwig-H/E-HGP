# Redistribution des sous-arbres WSPD en attente

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=implementation_v8_p0`,
`public_status=not_claimed`. Tranche14 qualifiée localement. Ce composant ne
calcule ni q3/q4, ni les parents FULL, ni une tour sur GPU.

## Pourquoi cette étape

La tranche précédente partageait un préfixe du front en sous-arbres indépendants.
Cela fonctionne bien si les sous-arbres ont des coûts voisins. Mais un CPU qui
termine sa liste ne pouvait pas aider celui encore occupé dans un gros sous-arbre.
Les audits LiDAR ont montré ce cas, même avec beaucoup de jobs initiaux.

La nouvelle option `Donate` permet de céder un produit encore en attente dans
la pile de parcours. Le destinataire reprend exactement là où il faut : il ne
refait ni les ancêtres ni leurs recherches de témoins. `Coarse` reste le défaut.
Le census d'un rectangle déjà commencé reste indivisible : cette étape ne
prétend donc pas résoudre le déséquilibre causé par un seul gros callback.

## Ce que possède un travail transférable

Un descripteur contient les deux identifiants de nœuds A/B, le masque des voies
encore actives, la profondeur et la position dans le parcours DFS logique.
Il ne contient ni copie de coordonnées, ni plan Pool, ni contexte emprunté à
la pile d'un autre worker. Le dispatcher conserve le propriétaire des jobs
initiaux et de l'index immuable partagé ; détruire le handle externe ne les
invalide pas. Ses appels doivent tous finir avant sa propre destruction.

Les terminaux déjà comptabilisés dans le préfixe sont consommés sans nouveau
test. Seuls les produits non visités peuvent être donnés. Les comptes partiels
des témoins ne sont toujours pas ajoutés au census : son contrat est inchangé.

## File bornée, exploration complète

Chaque worker continue son parcours local et consulte périodiquement la demande
de travail. Si un autre slot est inactif, il peut proposer le prochain produit
de sa pile, à condition de conserver aussi du travail local. Les slots pas encore
démarrés sont inclus dans cette demande ; c'est une heuristique de distribution,
pas une preuve géométrique.

L'offre utilise une prise de verrou sans attente. Une file pleine ou un verrou
occupé laisse le produit dans la pile du donneur, qui poursuit immédiatement.
La capacité de file ne borne donc jamais le nombre de produits explorés.
Le produit est retiré de la pile seulement après publication réussie, sans
opération susceptible de lever une exception entre publication et retrait.

Les receveurs ne prennent un nouveau fragment que lorsque leur pile est vide.
À tout instant, chaque produit restant appartient exactement à un des trois
endroits : jobs initiaux non pris, file commune, pile privée. Un compteur
d'activité inclut les parcours et les callbacks en cours. La fin exige à la
fois l'épuisement des jobs initiaux, une file vide et aucune activité privée.
Une simple file vide ne suffit pas.

## Exceptions et attentes

Les workers sans travail attendent une condition, sans boucle de scrutation.
L'annulation change le prédicat sous le même verrou que cette attente puis
réveille les receveurs. Cela évite un réveil perdu entre le contrôle du prédicat
et l'endormissement. Un échec de lancement de thread appelle la même notification
que l'échec d'un callback. Tous les threads démarrés sont joints avant propagation
de l'exception ; les émissions antérieures ne sont pas annulées.

Le dispatcher est à usage unique. La réentrance synchrone utilise un nouveau
dispatcher, pas le même : attendre un fragment extérieur depuis son propre
callback serait une dépendance circulaire. Avec un seul worker déclaré, le
parcours évite les consultations atomiques, offres et verrous par produit.
L'annulation de ce chemin mono n'est donc observée qu'entre fragments.
Déclarer plus de slots que l'on en lance ne bloque pas la terminaison, mais
peut faire reprendre au donneur ses propres offres : les slots ne sont pas
comptés comme une activité tant qu'ils ne possèdent aucun travail.

## Travail, mémoire et portée de la preuve

Les six familles de compteurs géométriques restent séparées des compteurs de
distribution : racines prises/finies, dons, fragments repris/finis, propositions,
refus, attentes et réveils. Les maxima de file et de pile se réduisent par maximum,
les autres compteurs par somme contrôlée. Les racines finies désignent leur
reliquat local ; les descendants donnés se ferment dans le compteur des fragments.
Au retour sans annulation : toutes les racines sont finies et chaque don est
repris et fini. Le dispatcher direct peut retourner un bilan partiel après
annulation explicite ; le pipeline ne publie pas un tel bilan comme un succès.
Les attentes/réveils comptent les entrées/retours de l'attente à prédicat, pas
ses réveils internes ou spontanés. Une fin de fragment ne réveille tous les
receveurs que si elle ramène l'activité à zéro ; un don réveille un receveur.

Les profondeurs et maxima DFS géométriques gardent leur sens logique mono. Ils
ne mesurent pas la RAM parallèle. Avec Q places de file et W workers, le nombre
logique de produits pendants est borné par Q+97W, en plus des jobs initiaux.
Les97 places proviennent de la profondeur de l'index u16, pas d'un plafond
d'exploration. Ce n'est pas une borne en octets sur les capacités allouées des
vecteurs. Ajouter séparément produits actifs/copies temporaires, index, plans
Pool actifs, buffers de sortie, objets et piles de threads. Le compteur de
capacité de file n'est pas une RSS.

Le partage ne modifie pas le travail géométrique, mais ajoute du travail de
distribution. Une baisse du temps mur n'établit aucune borne sous-quadratique.
Les mesures doivent vérifier n8k/16k/32k et s8/10/12, inclure les propositions
de don et les refus, et comparer les résultats complets ainsi que les compteurs.
Le temps mur inclut distribution, créations/jointures, réduction et destructions.
Les temps workers et payload sont des sommes d'intervalles, parfois supérieures
au mur : ne pas les lui soustraire.
Dans Donate, le temps de présence de chaque worker inclut ses attentes de fin
globale. Des durées presque identiques ne prouvent donc pas l'équilibre du
travail utile. Rapporter également la répartition des produits, visites census
et supports ; cette tranche ne mesure pas séparément la durée d'attente.

## Qualification propre

Les répertoires de cette capture sont
`build/v8_dynamic_front_20260914`, `build/v8_dynamic_front_sanitize_20260914`
et `build/v8_dynamic_front_tsan_20260914`. Ces builds sont désormais épinglés,
ainsi que la reprise `build/v8_dynamic_front_tsan_clang_20260914`.
Les témoins de la tranche13 ne sont
ni reconstruits ni réinterprétés. Les résultats de qualification et campagnes
sont clos dans les [reçus propres](../receipts/q2_dynamic_front_20260914/README.md) :
57 CTests Release/Clang ASan/UBSan, deux gates Clang TSan,32différentiels et
192mesures. Les échecs GCC TSan restent conservés. Le gain de temps n'est
pas stable sur tous les régimes ; `Coarse` reste le défaut.

Les données LiDAR éventuellement utilisées sont lues sous forme xyz u16 little
endian, sans échantillonnage ou quantification supplémentaire ; taille, commande
et hash sont épinglés. Leur préparation indépendante ne qualifie pas ce port.
Les résultats négatifs, interruptions et surcoûts font partie du bilan.
