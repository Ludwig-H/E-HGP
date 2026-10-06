# Recouvrement des feuilles : correction de durée de vie avant adoption

6 octobre 2026. Source **cf28afb04** ; cadre
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Revue de sources uniquement : aucun build, test natif ni lancement G4 par l’audit.

## Corriger la sortie sur refus

Dans `generate_single`, `OverlapLane lane` est déclaré avant `claimed`.
`start` conserve un span sur ce tableau de pointeurs. Si la passe des tâches
ou `prefix` refuse, le retour détruit d’abord `claimed`, puis le destructeur
de la lane annule et joint le fil. Celui-ci peut être déjà entré dans
`run_chunk` et lire encore ce tableau dans `gather_leaves`. `abort` n’interrompt
pas un rassemblement en cours. La survie des queues `Output` ne prolonge pas
celle du tableau de pointeurs prêté.

**Correction minimale : déclarer `claimed` avant `lane`.** La jonction se
termine alors avant la destruction de tout ce qu’emprunte le fil. Le
[patch proposé](claimed_before_lane.patch) est prêt à appliquer au pin relu ;
l’audit ne modifie pas le code produit. Autre solution : posséder une copie
bornée des pointeurs dans `State` avant le lancement.

Le retour C++ détruit les variables dans l’ordre inverse de construction ;
la mémoire encore présente ne suffit pas à autoriser un accès après la fin
de vie. Références : [ordre de destruction](https://eel.is/c++draft/stmt.dcl#2),
[durée de vie](https://eel.is/c++draft/basic.life#2),
[accès hors durée de vie](https://eel.is/c++draft/basic.life#8).
Le chemin de succès appelle déjà `finish` et joint avant le retour : c’est
bien la sortie anticipée qui impose cette correction.

Sur G4, exercer un refus d’une tâche tardive pendant que le fil traite un
sous-lot antérieur, et vérifier jonction, restitution mémoire et absence de
publication. Garder le test d’identité K5/K10 et les deux mutants existants ;
ils ne remplacent pas cette porte de refus concurrent, sous ASan/UBSan et TSan.
Aucun crash ni diagnostic sanitizer de cette faute n’est prétendu ici.
Le [modèle borné et ses ancrages source](lifetime/README.md) reproduisent
l’ordre fautif ; les lectures normal/−O concordent et l’ordre corrigé
n’admet plus de lecture après fin de vie dans ce modèle.

## Garder la borne des compteurs sur le lot réuni

Chaque sous-lot hôte refuse au-delà de `leaf_device::kMaxBatchJobs`, mais
`finish` additionne les compteurs de jusqu’à seize sous-lots sans reconduire
cette garde pour leur union. La preuve de somme du lot unique ne se transfère
pas en multipliant sa taille admissible par seize. Après le total `jobs` et
avant l’allocation/réduction, garder la même garde globale, ou contrôler les
additions. C’est un raccord de preuve ; aucun nuage provoquant un débordement
réel n’a été construit. La limite CUDA supplémentaire reste distincte.

## Ce que cette première tranche permet de vérifier

La lecture de l’ordre, des décalages des sites/préfixes et du repli exact est
favorable. Les feuilles sont locales à leur liste : les `LeafRecord` ne
portent pas d’index global de feuille à décaler. La fin de tâche sous mutex
publie sa queue avant consommation ; les tâches non terminées restent exclues.

Cette version lance des sous-lots de tâches ; elle n’est pas encore un anneau
à deux buffers bornés en feuilles. Elle garde les résultats des sous-lots
jusqu’à leur réunion. Chaque appel CUDA recharge aussi XYZ : la future
résidence devra extraire ce stockage hors de la boucle des sous-lots.
Mesurer cette version précise avant de décider la suite. Les différentiels
hôtes annoncés au commit ne constituent pas une qualification CUDA ; celle-ci
reste à établir sur G4 au pin exécuté. Les passes intermédiaires de la sonde
résidente ne publient toujours pas de dump individuel.
