# Fausses pistes et simplifications refusées

13 septembre 2026. Mémoire courte de l'audit et de la première tranche P0.
Les sources et contre-fixtures complètes sont dans les rapports liés.

## Constats exécutés dans la première brique P0

- Un pool global ne couvre pas toutes les régions utiles : sur les rails
  q4/n2718, il laisse 1 846 881 paires ; blocs et tubes en laissent 2 916.
  Le pool reste pertinent pour certains rectangles q2, pas comme réponse unique.
- Un préfiltre en quelques millisecondes ne règle pas le coût des nappes :
  les trois méthodes gardent 256 millions de paires à n32k. Raffiner le
  produit ou employer un autre certificat, sans annoncer le carré supprimé.
- Les tubes ne dominent pas les blocs en volume résiduel ; les blocs ne
  dominent pas les tubes en temps de préparation. Choisir sur le coût total,
  pas sur l'un de ces deux chiffres isolés.
- Ne pas confondre D≥10R et d²≥25 diam² lorsque d=2(cB−cA) : il faut le
  facteur 100. Une consigne erronée a été corrigée avant codage ; une
  fixture permanente expose le faux témoin q4 que donnerait 25.
- Un shared_ptr vers const ne rend pas immuable une copie mutable de
  l'objet : interdire les copies/déplacements/affectations du propriétaire
  lui-même. La contre-fixture de réaffectation perdait trois paires q2.
- Une ligne JSON réussie n'est pas un reçu conforme : la confronter à la
  commande, garder les sorties brutes en échec et vérifier la stabilité
  du binaire et des sources avant de fermer la campagne.

Voir les [preuves et mesures](../receipts/p0_local_credits_20260913/README.md).
Les méthodes explorées restent des variantes utiles du même module,
pas trois copies concurrentes d'un moteur FULL.

## Leçons de la préparation partagée et du filtre axial

- Trois voies q2/q3/q4 partagées ne sont pas la tour K=1..10 : le gain
  porte sur une préparation locale, pas sur les objets FULL ni leurs parents.
- Un rang projeté ne suffit pas en 3D. Le filtre axial exige les deux
  autres coordonnées exactement égales ; une colonne approximative
  réclamerait un autre certificat géométrique.
- La borne par ancre d'une grille complète ne vaut pas pour une dernière
  rangée tronquée : une fixture conserve 540 candidates contre la borne
  abusivement transférée de 441. Les entrées `sheet` et `sheet_full` restent distinctes.
- Une préparation non quadratique et des plages compactes ne rendent
  pas automatiquement le résidu, les visites d'index ou l'aval linéaires.
  Le filtre axial peut conserver toutes les paires hors de ses alignements.
- Une nappe alignée ne représente pas toutes ses orientations : une
  rotation entière conserve les profondeurs mais peut supprimer toutes
  les colonnes exploitables. L'auditeur conserve ce contrepoint isométrique ;
  les gains sur alignements ne sont pas des gains génériques.
- L'affectation membre par membre d'un plan peut changer le propriétaire
  avant une panne d'allocation de ses vecteurs. Copier entièrement puis
  échanger conserve l'identité de la cible même en cas d'exception.

Voir le [contrat de cette tranche](P0_PARTAGE_ET_FILTRE_AXIAL.md).

## Leçons conservées de l'audit

Troisième tranche : additionner les colonnes exactes est sûr, parce que
leurs témoins sont disjoints hors de l'ancre. Additionner ensuite ces
comptes à ceux de Pool/Dual/Tubes ne l'est pas : une fixture à trois sites
réfute ce double comptage. Leur **intersection de résidus** est implémentée.
Un filtre plus sélectif peut néanmoins coûter plus cher à construire :
le nombre de candidates ne remplace pas le temps de sélection plus census.
La spécialisation aux axes ne résout pas les nuages tournés. Voir le
[contrat additif](P0_ADDITION_ET_INTERSECTION.md).

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
| Il faut connaître les histogrammes locaux exactement avant de sélectionner | Le rejet n'exige que des minorants certifiés ; le census reste exact | P0 : comparer des architectures sans préparation A×A/B×B systématique |
| Un petit ensemble de témoins règle à lui seul toute la complexité | Il peut laisser un grand nombre de candidates inutiles | Comparer les alternatives et payer aussi raffinement, candidates et aval |
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
| Déplacer un vecteur suffit à rendre ses coordonnées immuables | Les pointeurs mutables vers son tampon survivent au déplacement | Copie privée à la frontière du propriétaire, ensuite partage sans mutation |
| Réutiliser des marques simplement bien formées | Elles peuvent désigner une autre histoire/composante vivante | Appartenance certifiée avant réemploi |
| Supprimer les contrôles pour tenir le temps | Transforme un gain supposé en perte du contrat | Certifier une fois, réutiliser la preuve compacte |

Références : [implémentation](../audits/IMPLEMENTATION_PARALLELISATION.md)
et [mesures](../audits/CONTRATS_ET_MESURES.md).

Les contre-exemples anciens restent dans la v7 ; ils ne sont pas effacés.
La v8 garde ici leur leçon, puis devra porter explicitement les fixtures
pertinentes. Aucun fichier utilisateur ou archive de preuve n'est supprimé
par l'audit d'ouverture.
