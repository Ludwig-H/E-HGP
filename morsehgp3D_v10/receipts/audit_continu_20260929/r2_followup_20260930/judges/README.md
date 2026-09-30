# Juges R1/R2 : rejeu autonome des petits dumps d'audit

30 septembre 2026. Deux exécutions, normale et `−O`, code 0 et résultats
identiques. Aucun appel au moteur, aucun changement de production ou GCP.

Les sources des juges R1 et R2 sont figées ici. Chaque passage les applique
aux trois objets de contrôle, sept mutants de catalogue et trois mutants
de tour de l'audit précédent. R2 refuse désormais les cinq corruptions de
listes/ordre encore acceptées par R1, l'attache morte et la fusion ternaire
binarisée. Les contrôles positifs restent refusés, les objets valides admis.

Le reçu donne commandes, codes, sorties et hashes avant/après. Le manifeste
ferme ces copies, pas une campagne du développeur ni son binaire futur.
`evidence/` contient les petits dumps historiques nécessaires au rejeu,
octet pour octet ; leurs chemins et dates historiques ne sont pas réécrits.

Rejeu depuis ce dossier :

`python3 -B scripts/rejeu_contre_audits.py . sources/r1 sources/r2`

Remplacer `-B` par `-B -O` pour le second mode. Ce script n'écrit pas de
nouvelle capture dans le dossier fermé. Les assertions des scripts jugés
ne sont pas supposées constituer à elles seules les refus sous `−O`.

Ce contrôle ciblé des sorties n'est ni une preuve générale de FULL ni un
test de croissance, de performance, de GPU ou du raccord commun R2.

Deux fichiers du développeur sont observés, non rejoués : la campagne R2
33/33 CTests, footer rc=0, et la classification des 53 changements de
témoin survivants comme même MEB/même composante Γ. Ils ne sont pas des
preuves produites par notre script autonome ni une campagne commune.
