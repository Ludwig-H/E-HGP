# Audit des corrections et des sessions C/D

7 octobre 2026. Pin **`07ee13ef6bebc0b6b85da90207f3f067ffff1755`**.
Relecture des corrections `320db4a12` et `2f7b41380`, des reçus G4 C/D,
et de la réponse au contrat T1. Cadre `exploration_v12_hors_registre`,
`cpu_reference ; cuda_g4 pour le catalogue`, `full_pi0`,
`quantized_u21_input_only`, `not_claimed`.

| Preuve | Portée |
| --- | --- |
| [M2](m2/README.md) | lecteur natif corrigé et décisions du vrai pilote aux frontières simulées |
| [M3/M4](m34/README.md) | répétitions de résolution, préservation des prises, FLOWER et preuves des juges |
| [Sessions C/D](campagnes/README.md) | recomptage indépendant des campagnes historiques et des décisions |
| [Publication](publication/README.md) | protection de la destination, anonymisation et collision de noms |
| [Contrat T1](contrat/README.md) | réponse aux conditions de conception, sans qualification du produit |

Les corrections du code et la validité d'une campagne réelle sont des
conclusions distinctes. Les nouveaux témoins résiduels des juges prolongent
`CST-0018` : une campagne effectivement complète peut être recevable alors
que son pilote reste trop permissif sur d'autres entrées. Le publieur ferme
les défauts initiaux `CST-0219/0221`, mais ouvre `CST-0224` : deux résultats
sélectionnés peuvent s'écraser sous un même nom anonymisé.

Le détail des clôtures retenues est dans le [registre courant](../../audits/CONSTATS.md).
Cette tranche clôt `CST-0213`, `0214`, `0215`, `0219` et `0221` sur leurs
témoins précis. Le rattachement des exécutables M3/M4 est présent dans D,
sans réparer les lacunes historiques de B ni clore tout `CST-0021`.

La session C confirme l'adoption locale M5 : douze cas, cinq tours retenus,
ratios des six cas décisionnels entre 0,073 et 0,105 et bornes hautes sous ¼.
La session D confirme M3 K10 : cinq prises de résolution par trame, réduction
de 45,2 à 46,1 %, bornes hautes sous 0,60. La conformité M4 est étayée ; les
seuils par ordre passent à K5, mais la contraction K10 reste à 3,24–4,25 ms
pour un seuil de 3 ms. Ces statistiques et leurs frontières exactes sont
détaillées dans le reçu des campagnes, avec la distinction entre médianes
de processus et répétitions internes.

Les preuves de cette capture restent épinglées ; elles ne qualifient ni le
catalogue intégré, ni FULL, ni le contrat 100 ms, ni d'autres jeux de données.

Aucun GPU ni appel GCP lancé par cet audit. Les sessions C/D sont des
exécutions du développeur relues depuis leurs traces. Tests locaux limités
aux petits témoins et aux lecteurs ; aucun scan LiDAR, binaire compilé ou
arbre de sources ajouté. Les captures normal/`-O`, leurs domaines simulés,
les empreintes et les commandes sont décrits dans chaque section.

Les commits `6a38f7e4b` (socle numérique) et `f4a11f49e` (format de transition),
arrivés pendant l'audit, demandent une tranche séparée. Aucun état n'est
clos par anticipation sur la foi de leurs messages de commit.
