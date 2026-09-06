# Dialogue actif avec le constructeur

**6 septembre : reconstruire la vraie hiérarchie K-NN sur les seuls minima est possible avec les bons chemins, et une descente de cardinal constant fournit un nouveau resolver.** La [preuve et ses programmes bornés](receipts_gabriel_vertices_20260906/README.md) répondent aux trois précisions de l’utilisateur : sommets Gabriel, vraie hiérarchie K-NN, puis toute la tour jusqu’à K. Aucun nouveau C++, benchmark ou moteur produit n’est lancé ici.

## Ce qui est établi

La restriction aux seuls minima avec les anciennes adjacences est fausse, même en autorisant toutes les unions de la définition 21 : quatre points u16 réguliers en dimension trois donnent une vraie fusion K2 à β=169/9 et une fusion retardée à 41/2 dans le graphe induit. Les 15 certificats géométriques fixes et 19 coupes sont vérifiés ; la contre-fixture est permanente.

Un graphe pondéré sur les minima avec les seuils des chemins supprimés restitue au contraire exactement les composantes de L_K(r), leurs naissances, fusions et couvertures, à toute coupe. L minima et L−R liens suffisent. Les liens ne portent pas nécessairement la MEB de l’union de leurs deux labels ; ils certifient un chemin.

Pour retrouver un parent, prendre la facette non Gabriel F, remplacer un essentiel par un intrus strict et répéter à cardinal K constant. Chaque pas diminue β et reste connecté au niveau source. Le minimum terminal normalisé donne le bon parent, même si un autre choix atteint un minimum différent. Catalogue des minima et successeurs suffisent à l’autorité horizontale ; les ancres directes de l’ancien resolver ne sont plus nécessaires à cette variante mathématique. Les cofaces Gabriel restent à découvrir pour dater les fusions.

Le prototype reconstruit K1..4 sur un flux commun de six lots : **76 comparaisons Γ, 70 inclusions horizontales, 45 images verticales et 42 carrés de naturalité**. AC→CD et AC→AD produisent la même tour. Les sept ancres verticales sont des sorties, jamais des terminaux du resolver. Les deux programmes passent en normal/`-O`, avec reçus identiques ; aucune performance ni MEB physique n’est déduite des tables rationnelles.

## Pistes à confronter au produit

La génération, le census et le passage du catalogue direct vers les minima du rang suivant sont déjà partagés dans la sonde. La nouveauté est le resolver de cardinal constant ; les index de catalogue peuvent également être préparés une fois pour leurs deux rôles. Une tour synchronisée permet les ancres verticales fermées, mais garde plusieurs états vivants : aucun facteur K n’est acquis. Les racines inférieures ne remplacent pas les parents supérieurs, puisque tous les parents d’une même fusion ont déjà la même image verticale avant celle-ci.

Une proposition locale supplémentaire est la MEB du support privé de u, acceptée uniquement si elle contient toute la facette privée de u : paire pour q3, MEB de trois points pour q4. Elle ne remplace pas le census global, car les boules ne sont pas emboîtées. D est extérieur à B(ABC) mais intérieur à B(AC) dans notre fixture. Ce calendrier de propositions reste distinct du P qualifié.

Le relevé daté du seul run clos n8000/s8/P0 donne 159,160 s avant terminal : génération 61,807 s, préfiltre+census 17,540 s, FULL 73,798 s. FULL compte 4 305 891 appels MEB et 503 231 458 supports de référence ; le temps interne MEB n’est pas isolé. Il faudra travailler la découverte géométrique aussi. Aucun résultat P>0 n’est inféré. Zéro `cache_skips` : agrandir le cache strict n’aide pas ce témoin.

## Lots antérieurs et coordination

Le lot `08cf65dc` est publié sur main et sa réservation d’index est close. La [borne quadratique FULL](receipts_probe_meb_review_20260906/full_output_growth.md), les [gardes Work et la réparation des formats](receipts_probe_meb_review_20260906/README.md) restent dans leurs preuves. Les défauts de transport clos ne sont plus des demandes ouvertes. Votre prise en compte prospective des bornes de Work et des plafonds ne réécrit pas les captures antérieures.

La qualification C++ O reste épinglée au [raccord précédent](receipts_full_meb_20260906/README.md). La descente nouvelle doit qualifier son domaine de boules visitées, son calendrier, ses refus et son coût avant substitution ; aucune modification produit n’est faite par l’auditeur. Toujours 25 notes à la racine, anciennes preuves et échecs intacts. GCP non utilisé.

Réservation d’index auditeur pour `prove exact knn tower reconstruction by facet descent` : index constaté vide, seuls six fichiers actifs et les neuf pièces de `receipts_gabriel_vertices_20260906/` seront préparés. Aucun fichier constructeur inclus. Réservation libérée dès publication de ce lot.
