# Idée historique retenue : extrema q2 couplés pour le census global

Pin de lecture : `4fac501181bf9d9c6f0cb27736355bd413828253`. Aucun code produit modifié, compilation/natif/fit/GCP exécuté. Capsule portable de preuve et proposition ; aucune qualification ancienne transférée.

Les formules de `source/morsehgp3D_v8/src/spindle/q2_prepared_bounds.hpp:61–75`, inchangées hors namespace en v9, conservent le couplage quadratique/linéaire. Pour une présentation q2 certifiée, la puissance actuelle est P(z)=2|z−a|²−2(b−a)·(z−a). Poser C=a+b et S=|b−a|² donne exactement **2P(z)=Σ(2z_j−C_j)²−S**. Sur une boîte continue fermée, la plus proche valeur de chaque intervalle à C donne le minimum ; l'extrémité la plus éloignée donne le maximum. Aucun rayon, division ou approximation n'est requis.

C'est utile car `src/num/predicates.cpp:110–139` sépare les extrema, donc reste parfois ambigu sur une boîte strictement intérieure. Exemple : a=(1,6,6), b=(11,6,6), boîte [2,10]×{6}². UB actuelle de P=142, UB couplée de 2P=−36. Le nouveau test certifierait le bloc intérieur.

**Le port doit ajouter un helper/préparateur explicitement destiné au census**, appelé depuis les parcours de `index/census.cpp:45` et `census_workspace.cpp:53`. Ne pas remplacer les fonctions publiques actuelles : `geometry.hpp:122` impose que power_bound_signs rende les signes de power_bounds ; `tests/num/bounds_test.cpp:88–94` le vérifie. Sur a=0,b=4,Q=[0,4], les bornes publiques sont [-32,32], signes[-1,+1] ; les nouvelles bornes de 2P sont [-16,0], signes[-1,0]. Les deux certificats sont sûrs, leurs contrats diffèrent. Ne pas publier 2P comme PowerBounds : un minimum de 2P=−1 correspond à P=−1/2.

Limiter le nouveau chemin à **presentation_arity==2**, factory certifiée (D=2,N=b−a), jamais qmin==2 ni un centre q3/q4 retypé. Les autres présentations suivent le parcours existant. Les constantes locales C,S ne nécessitent pas de nouveau propriétaire, copie du Cloud ou arène. Préparer une fois par requête est une option à mesurer. Aux profils18/21/24, C≤2M, chaque carré≤4M² et |2P|≤12M²<2^(2B+4)≤2^52 : i64 suffit, promotion avant produit.

Le modèle indépendant reproduit l'index médian preorder/ranges/escape et l'ordre Morton sur 17 sites unitaires distincts : 9 intérieurs, 6 coquille, 2 extérieurs. Pour seuils2,9,10,18, **mêmes I/U et même complete/saturated**, dont tous les contacts et la coquille entière sous seuil. À seuil10, le modèle passe de17 à13 bornes, sans baisse des8 tests ponctuels : ce n'est pas une mesure native. 633 gardes passent normal/−O, JSON identiques ; extrema continus, profils extrêmes et contrat public sont vérifiés.

Avant port : portes natives comparant scan/I/U, contacts, seuil atteint, tag3 de boule à qmin2 et sorties FULL canoniques ; mesurer census global par arité et coût total du helper. Les captures actuelles ne ventilent pas l'arité : aucun gain ou pourcentage de temps annoncé. Le scan linéaire des feuilles du catalogue n'utilise pas ces bornes et n'est pas accéléré directement.

Les 11 sources nécessaires sont épinglées dans SOURCE_BEFORE/SOURCE_API_BEFORE, rejugées à SOURCE_AFTER. COMMANDS conserve versions/commandes/sorties ; verify.py lit le ledger portable. SHA256SUMS exclut seulement lui-même.
