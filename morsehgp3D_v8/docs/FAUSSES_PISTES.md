# Fausses pistes et simplifications refusées

13 septembre 2026. Mémoire courte de l'audit ; ce fichier n'introduit
aucun résultat moteur v8. Les sources et contre-fixtures complètes sont
dans les rapports liés.

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Garder le graphe induit des seuls minima Gabriel | Supprime des chemins silencieux et retarde certaines fusions | Minima avec connexions datées issues des vrais rattachements |
| Assimiler partage de points et identité de composante | Détruit le recouvrement naturel HGP | Identités sur facettes/composantes, couverture séparée |
| Faire un MST sur les points pour toute la tour | K1 seulement ; les feuilles supérieures ne sont pas les points | Graphe daté propre à chaque K |
| Lire les parents dans leurs racines finales | Des parents distincts peuvent fusionner plus tard | Consultation historique à la bonne coupe |
| Les cofaces K+1 ont forcément K+1 parents | Cardinal de coface et arité de fusion sont différents | Lot atomique des composantes réellement distinctes |
| Les feuilles FULL suffisent aux poids du manuscrit | Les facettes contributrices des poids sont plus nombreuses | Profil pondéré explicitement séparé |
| Support ≤4 implique coquille ≤4 ou ≤12 | Une sphère peut porter bien plus de points | Politique non régulière explicite et format adapté |
| Toute sortie FULL doit être sous-quadratique en n | Les vraies feuilles peuvent être quadratiques à K fixé | Faible surcoût en plus du volume de sortie |

Références : [fondements](../audits/FONDEMENTS_ET_OBJET.md).

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Une tâche par rectangle donne automatiquement un bon GPU | Les facteurs et les voies ont des coûts très différents | Tuiles internes et tâches de complétion |
| s plus grand est toujours plus rapide | Moins d'indécision peut coûter plus de rectangles et de parcours | Comparaison s8/10/12 par phase et régime |
| Une WSPD linéaire implique un travail linéaire sur ses facteurs | La somme des tailles visitées peut être quadratique | Preuve de coût des parcours et crédits réellement payés |
| Copier le compteur h d'un parent puis recompter ses sites | Double crédit possible avec les populations locales | Identifiants et disjonction vérifiables |
| Les blocs positifs suffisent pour de gros facteurs | Ils peuvent échouer tard et payer encore le quadratique | Certificats négatifs, saturation, raffinement des crédits |
| Forcer les histogrammes par blocs même sur facteurs minuscules | Les mesures uniformes montrent un surcoût | Sélection adaptative explicitement mesurée |
| Rétrécir le cover q4 au seul citron de complétion | Perd des points qui comptent dans la profondeur | Cover prouvé pour tous les rôles |
| Arrêter q4 au premier intervalle trop profond | La profondeur peut ensuite redescendre | Balayage exact complet des segments utiles |

Références : [audit WSPD](../audits/WSPD_Q2_Q3_Q4.md).

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Rendre le tri amont gratuit suffira au contrat | Seulement 0,75 % de la capture50k/K10 | Réduire MEB, génération, histoire et export |
| Un petit kernel rapide constitue une tour rapide | Reconstruction et transferts peuvent dominer sa phase | Objets résidents et temps de bout en bout |
| Compiler CUDA prouve l'exécution GPU | Les dernières tentatives de terminal n'ont exécuté aucun kernel | Gate réelle sur carte puis chaîne complète |
| Un même digest suffit à certifier tous les parents | Des erreurs locales peuvent être masquées par une normalisation finale | Comparaison physique avant normalisation, oracles et mutants |
| Un propriétaire immuable valide toute extraction adoptée | Il peut conserver fidèlement un certificat forgé ou des vues empruntées | Factory qualifiée avec provenance et durée de vie réelles |
| Réutiliser des marques simplement bien formées | Elles peuvent désigner une autre histoire/composante vivante | Appartenance certifiée avant réemploi |
| Supprimer les contrôles pour tenir le temps | Transforme un gain supposé en perte du contrat | Certifier une fois, réutiliser la preuve compacte |

Références : [implémentation](../audits/IMPLEMENTATION_PARALLELISATION.md)
et [mesures](../audits/CONTRATS_ET_MESURES.md).

Les contre-exemples anciens restent dans la v7 ; ils ne sont pas effacés.
La v8 garde ici leur leçon, puis devra porter explicitement les fixtures
pertinentes. Aucun fichier utilisateur ou archive de preuve n'est supprimé
par l'audit d'ouverture.
