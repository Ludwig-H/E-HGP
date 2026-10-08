# Lecteurs FULL et apparié — intégration 85db49890

8 octobre 2026. Contrelecture du commit **85db49890e063e9818e4b49d6cd9f968a2db2a9b**,
sur des copies de ses objets Git : aucune source vivante modifiée, aucun moteur,
compilateur ou appel GCP lancé. Les 21 sources Python et cinq patches historiques
sont épinglés dans `capture.json`. Les reçus proposés restent immuables.

## Raccord des propositions

- Le lecteur FULL est exactement la postimage `15437e5f…` du
  [correctif d'horloges](../lf_recouvert_gardes/README.md).
- Le pilote MES-FULL contient exactement la correction du nombre de colonnes
  du [tableau](../pilotes_bascule/README.md).
- Les fixtures B/C/FULL sont exactement les postimages V=6 proposées dans
  [la réparation des quatre fixtures](../pilotes_fixtures_recouvert/README.md).
  Le texte de la fausse sonde appariée est lui aussi identique ; sa porte ajoute
  les quatre vérifications de fermeture.
- L'AST du pilote apparié est celui de la [triple composition](../apparie_composition_triple/README.md),
  sauf une suppression explicitement contrôlée : le second test de longueur
  des tours dans `judge` disparaît. `validate_campaign` exige déjà leur nombre
  **exact**, avant relecture/statistique. Son contrôle du type reste présent.
  Aucun autre changement de calcul, règle, seuil ou bootstrap n'est introduit.

La fermeture ELF est désormais relevée après les Sessions informatives puis
comparée à celle d'ouverture avant jugement. Deux relevés égaux ne prouvent
pas une immuabilité continue ; ce protocole est celui proposé, pas une nouvelle
garantie ajoutée par l'audit. Le `reread` ferme identité, configuration et résumés
des prises, avec types préservés par JSON canonique.

## Rejeux Python et causalité

Les cinq portes officielles LF, MES-FULL, apparié, MES-B et MES-C passent en
normal et `-O` avec leurs sondes **Python simulées** : aucun exécutable moteur
n'est fourni à la copie. Les sorties exactes sont dans `gates.json`.
Le LF annonce 29 lectures, 7 issues, 6 cas Session et 28 cas recouverts ; la porte
appariée annonce notamment ses quatre fermetures : résumé forgé, hash final
différent, cohorte vide et journal d'identité retiré. Le positif complet reste jugé.

Le contrôle indépendant conserve les **neuf sens d'altération** du reçu initial,
sur la fixture livrée : tour hors mur, ouvertures divergentes, fin G avant
ouverture, noyau avant G, M avant noyau, R avant M, V avant M propre, V avant M de
l'ordre inférieur, maximum G incohérent. Chacune est admise par l'ancien LF puis
rendue `illisible` par le lecteur livré. Les cinq nouveaux mutants du LF sont compilés comme
Python puis testés directement : le positif reste admis et chaque garde retirée
réadmet son contre-flux dédié. Le témoin M(k−1) ne dépend donc pas d'une autre
garde qui rejetterait déjà son flux. La sentinelle exacte d'un nanoseconde reste
admise ; aucun ordre V/R arbitraire n'est ajouté.

Les campagnes officielles des mutants LF et apparié sont également relues,
respectivement **27 et 17 tués**, dans les deux modes. Pour `-O`, l'environnement
`PYTHONOPTIMIZE=1` est transmis afin que leurs sous-processus Python le suivent.
Ce bilan officiel constate les codes non nuls ; la causalité des cinq nouveaux
mutants d'horloges est établie séparément comme décrit ci-dessus, sans créditer
un échec de syntaxe. Quatre mutants appariés sont aussi contre-jugés
séparément à partir d'une campagne complète simulée : le nominal reste jugé,
la livraison refuse le faux rapport, et le mutant le juge. Les contre-exemples
portent sur l'ELF final, un résumé CPU modifié, la cohorte entièrement vidée
(configuration et dictionnaires cohérents), et le journal d'identité absent.
Les campagnes de mutations B/C ne sont pas répétées ici :
leurs portes nominales et les fixtures corrigées ont été vérifiées.

## Portée de clôture

Le raccord des correctifs livrés est acquis à ce pin, à la portée de CST-0018.
Le résidu historique `code=False` du **lecteur partagé** reste admis par le
contre-flux nominal ; ce lot ne clôt donc pas globalement le constat. Le juge
apparié relu exige, lui, un entier strict zéro pour chaque code de prise.

La [session M](../session_m_provenance/README.md) a exécuté le source957, antérieur
à cette livraison : son ELF final n'était pas archivé. Ces correctifs ne créent
pas rétroactivement cette preuve, même si les bruts/stats ont été admis par les
relectures indépendantes. Ils ne donnent aucun nouveau chrono ni qualification
de la porte native CST-0241, chantier séparé. Le [défaut cache8 Gio](../cache_defaut_raccord/README.md)
et ses limites CPU/K10 restent également distincts.

## Reproduction

`python3 -B check.py --repo DEPOT` puis `python3 -B -O check.py --repo DEPOT`
reproduisent exactement `results.json` : sources/postimages/AST, neuf refus,
cinq mutants causaux d’horloges, quatre mutants appariés causaux et sentinelle1ns.
La campagne appariée employée est une fausse sonde Python fournie par la porte livrée. Ajouter `--gates` rejoue en outre les
cinq portes et les deux campagnes de mutations officielles sur copies temporaires,
avec exclusivement des sondes Python. Les sources et patches sont lus depuis
Git/les reçus épinglés, sans copie de journaux de scène ni de sources dans ce reçu.
