# Contrelecture des propriétaires, statuts et descentes

Sources e02a6c235, 4 octobre 2026. Les 29 fichiers listés dans SOURCES.json
sont relus ; aucun nouveau défaut établi, aucun natif/build/fit/GCP exécuté.
Les qualifications antérieures restent attachées à leurs sources. Cette
note complète les revues num/index/catalogue, mathématiques et parallélisme.

Le Cloud copie les coordonnées dans son stockage et conserve tous les IDs
et multiplicité ; l'entrée doit rester stable pendant l'appel synchrone.
Un ID externe peut valoir 0xFFFFFFFF, un rang dense ne le peut pas. Morton
couvre 54/63/72 bits sans masque perdu. Les doublons de positions ne sont
pas silencieusement comptés pour un : le catalogue refuse tout poids !=1.
Cela ne qualifie pas encore FULL pondéré, ni la restitution d'une entrée
quantifiée ayant fusionné plusieurs retours.

Les Buffer gardent un compte partagé atomique, rendent leur réservation
sur refus système et ne font pas passer leur pic pour RSS. admit est une
vérification préalable, pas une réservation. L'absence d'allocation
concurrente indépendante pendant un étage est son hypothèse. Les types et
alignements limitent les produits avant allocation ; les résultats de
module se déplacent sans exception. Les résultats refusés ne construisent
pas T. La réduction merge prend le minimum d'un ordre total, donc ne dépend
pas du premier worker arrivé. Ledger n'est pas le registre de boucle chaude.

FullDomain déplace index/catalogue/lookup uniquement au succès ; les
contextes de descente et de parallélisme finissent avant son transfert.
Un support local trouvé dans le catalogue certifie la même MEB ; son absence
n'est pas une preuve d'absence globale. La coquille complète est canonisée
avec arité minimale, sans accepter un tétraèdre de poids nul comme support4.
Le recensement saturé ne sert pas à fabriquer une coquille complète.

Les descentes choisissent une k-partie strictement intérieure ou une trace
strictement séparable ; chaque étape vérifie la baisse du niveau. Le mémo
vérifie d'abord propriétaire et entrée, conserve date initiale et terminal
séparés et ne stocke pas de racine DSU mutable. La table de populations
répond par égalité exacte, pas par hash seul, et son raccourci terminal
exige toute la population d'une boule. Il ne suppose pas que toutes les
naissances ont population=k : la croix U4/k3 de la contrelecture mathématique
réfute cette généralisation, correctement évitée par le code actuel.

Limites maintenues : preuves conditionnelles à complétude/populations,
coquilles étendues combinatoires et sorties éventuellement quadratiques.
Le domaine numériquement accepté ne démontre pas la terminaison rapide ni
100 ms sur LiDAR ou des dizaines de millions de sites.
