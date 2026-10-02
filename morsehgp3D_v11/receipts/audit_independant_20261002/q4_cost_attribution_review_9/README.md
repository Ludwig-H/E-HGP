# Niveau q4 différé : travail, temps observés et mémoire

Sources et rapports publiés à `d0dc9cd8b`, source exécutée `ffc2ff95f`.
Les copies Git sont figées ; un ajout WIP Box::make dans sphere.cpp a
interrompu la première capture LIVE. Ce constat est conservé dans
[capture_initial_error.json](capture_initial_error.json) ; les copies
initiales Git sont restées intactes. Le nouvel index n'est pas qualifié
par ces rapports. Aucun build, test produit ou GCP par cet audit.

## Ce que mesurent les nouveaux compteurs

q4_candidates=C compte les présentations non dégénérées avant positivité
et propriétaire ; q4_levels=L4 compte les matérialisations après S*/admission.
Sur un catalogue publié, L4 égale le nombre de boules qmin4. Le ledger décrit
une passe, et count/fill ont le même ledger : les calculs de niveau évités
sur l'appel complet sont donc **2(C−L4)**, comparés à l'ancien emplacement
qui calculait le niveau pour chaque présentation q4 non dégénérée.
Un refus n'expose aucun catalogue auquel attribuer ces compteurs partiels.

| Sous-nuage entier K5, un essai u21 | C par passe | L4 par passe | Calculs évités, deux passes | Part évitée |
| --- | ---: | ---: | ---: | ---: |
| 08/000100 | 41 963 267 | 121 303 | 83 683 928 | 99,710931 % |
| 08/000000 | 52 878 237 | 158 494 | 105 439 486 | 99,700266 % |
| 08/000200 | 48 091 032 | 143 105 | 95 895 854 | 99,702429 % |

Les compteurs sont identiques en B18/21/24 sur les cinq cas K5 terminés.
Pour les deux uniformes, environ 99,19 % de niveaux sont évités.
Les quinze sorties gardent hashes bruts/sémantiques et tailles **déclarés**,
les neuf compteurs géométriques, boules/niveaux/incidences et réservations
de la campagne précédente. Les payloads canoniques sont supprimés après
mesure : ils ne sont pas rehachés indépendamment dans cette capsule.

## Les calculs évités ne donnent pas la part du temps CPU

Le nouvel appel catalogue u21 dure 19,776976940 /24,962327081 /23,101086066 s
pour 08/000100 /000000 /000200. Les rapports ancien/nouveau valent environ
1,039 /1,035 /1,041 sur ces essais distincts. Une seule répétition par profil,
aucune comparaison appariée ni preuve de gain stable ; toutes les K10 jouées
et 32k/K5 expirent, le banc conserve failed_remote.

Même avec une proportion L4/C connue exactement, un modèle conditionnel
T_old=R+C*t, T_new=R+L4*t laisse R et t inconnus. Son gain dépend de la part
initiale du temps consacrée aux niveaux ; le nombre de calculs supprimés
ne permet pas d'identifier cette part. Les contrôles du script illustrent
plusieurs parts hypothétiques, pas une estimation du profil CPU mesuré.
Mesurer séparément génération/count/fill, construction des centres,
positivité/census/canonicalisation, tri et assemblage avant d'attribuer
les secondes restantes ou de choisir le prochain levier.

## Le pic des buffers n'a pas changé

Pour les quinze nouveaux succès, les équations du reçu mémoire précédent
restent exactes : Cloud=28n+8, E=e_B N+4P,
F=40N+l_B L+4P+8, pic=Cloud+E+F et conservé=Cloud+F.
N boules, L niveaux zéro compris, P incidences ;
(e_B,l_B)=(96,48)/(104,64)/(112,72) en B18/21/24.
Ce sont les mêmes layouts corroborés, sans nouveau sizeof natif.
À 08/000200/u21 : 326 509 940 octets au pic, exactement comme auparavant.
Éviter la construction de Level de candidats rejetés ne change pas les
émissions retenues ni leur coexistence avec le résultat à l'assemblage.
Les réservations Buffer ne sont ni le RSS ni la mémoire du décodeur Python.

[check.py](check.py) : 259 contrôles normal/−O identiques sur copies,
[RUN.json](RUN.json). Les pièces sont dans
[SOURCE_BEFORE.json](SOURCE_BEFORE.json) ; la qualification G4 et sa
fermeture sont recoupées dans le reçu voisin q4_qualification_review_9,
sans les déduire de ces seules équations de coûts.
