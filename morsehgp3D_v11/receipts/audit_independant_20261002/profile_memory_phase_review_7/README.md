# Pic mémoire du catalogue : phase et largeur du profil

Source produit `9df774947`, mêmes sources de catalogue qu'avant le port
numérique ; captures profiles1 figées avant lecture. Aucun build, sizeof
natif, allocation géante ou GCP. Le calcul reconstruit l'ABI attendue
x86-64 de la VM et la confronte aux réservations Buffer publiées ; aucun
chiffre n'est un RSS ou un chrono de millions de points.

## Toutes les réussites atteignent le pic pendant l'assemblage

Noter n retours uniques, N boules, L niveaux **zéro compris**, P incidences.
Le pilote libère ses quatre buffers d'entrée avant catalogue et remet son
pic à zéro ; le Cloud reste vivant : U=28n+8 octets.
Les champs et alignements donnent la reconstruction suivante, corroborée
par les quinze mesures achevées, sans nouvelle exécution sizeof :

| B | sizeof Level reconstitué | sizeof Emission reconstitué |
| --- | ---: | ---: |
| 18 | 48 | 96 |
| 21 | 64 | 104 |
| 24 | 72 | 112 |

CatalogueBall vaut 32 dans cette ABI. Les buffers d'émission coûtent
E=e_B N+4P ; la sortie coûte F=40N+l_B L+4P+8. L'assemblage conserve E
pendant l'allocation/copie de F ; workspace et DFS sont déjà rendus.
Pour chacun des quinze succès K5, les équations vérifient EXACTEMENT :

- pic publié = U+E+F ;
- réservé après catalogue = U+F ;
- différence = E.

C'est une attribution du pic de réservations dans ces appels, pas une
attribution du temps CPU ni une borne générale dominant toujours le DFS.
La construction conserve la formule générale max(W+T+E,E+F)+U.

À 08/000200/K5/B21 : pic 326 509 940 octets, résultat+Cloud conservés
154 031 112, émissions temporaires 172 478 828. Leur ancienne population
seule vaut 26 058 788 octets ; toute la différence ne vient donc pas de
la duplication I/U. Réduire seulement les listes DFS ne réduit pas ce pic
observé si la coexistence E+F reste dominante.

## Surcoût de largeur sans changement de géométrie

Même n,N,L,P, sortie mathématique et travail discret déclarés identiques :
B21−B18 = 8N+16L octets au pic ; B24−B21 = 8N+8L.
Le fichier canonique augmente seulement de 8L à chaque palier : ce sont
d'autres layouts que les tableaux natifs, ne pas les confondre.
Les quinze métriques et ces dix deltas correspondent exactement aux
formules. Cela explique le surcoût mémoire mesuré ; aucune précision
physique nouvelle ni croissance de catalogue n'en est déduite.

Pour la suite, mesurer séparément durée et réservations de generation,
tri et assemblage. Conserver CSR et I/U complets dans toute optimisation.
Garder une population en ordre d'émission après tri des boules demande
un contrat d'offsets distinct ; ce n'est pas une copie qu'on peut supprimer
sans changer les accès ou réordonner ces données. Préflight massif : N,L,P
comptés et réservations coexistantes, pas extrapolation linéaire depuis n.

[Calcul et quinze cas](check.py), [sources/captures](SOURCE_BEFORE.json).
La recoupe géométrique de G4 et la fermeture cloud sont dans le reçu
voisin profiles_capture_review_7 ; ces résultats de campagne ne sont pas
qualifiés par la seule résolution des équations mémoire.
