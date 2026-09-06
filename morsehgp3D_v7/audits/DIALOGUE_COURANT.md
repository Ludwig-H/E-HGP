# Dialogue actif avec le constructeur

**6 septembre : raccord FULL `20b28b1d` qualifié indépendamment sur le corpus borné.** Les headers `a946e31d` / `f922544b`, avec F inchangé `f75a136a`, passent nos nouveaux builds O2 et ASan/UBSan : **2 784 sorties et 214 704 coupes par build**, 48 caps exacts, 540 refus cap−1 et 36 conflits API. Forêts, 33 champs historiques et préfixes de refus restent identiques à L sur son corpus. Les [reçus](receipts_full_meb_20260906/README.md) distinguent ce rejeu de la [contrelecture de vos captures](receipts_full_meb_20260906/constructor_capture_review.md) : 30+30 CTests, quatre mutants et douze injections tardives par build, sans inventer douze états individuels publiés.

## Compléments maintenant clos

Le rejeu ajoute deux ordres n=14/K9/K10 jugés par Gamma rationnel. Les pas de chaîne C0 prédits sont retrouvés, dont une demande MEB sur onze sites à K10. À grand P, les formes passent de 1 634 à 61 (K9) et de 1 471 à 50 (K10), à nombre d’appels inchangé. Ce sont des comptes de travail sur de petits témoins, pas des mesures de tour.

La sentinelle tétraèdre est maintenant compilée sur le helper produit : P3 replie avec p=3/A=11 ; P6 certifie avec p=6/A=0. Le mutant q4-first garde toute la géométrie mais change neuf calendriers. Votre port permanent de ce complément dans la porte locale est vu en préparation ; il reste distinct de notre capture. Le mutant FULL de remise à zéro de Work est réfuté par 90 sorties à P1, sans changer leurs forêts ni leurs préfixes historiques. Cette réfutation vise Work entier, pas seulement p.

Les avis de contrat sur Work persistant, charge prospective, miroirs et exceptions sont levés par le port et ses preuves ; leurs détails appartiennent désormais au [contrat principal](../docs/CONTRAT_MEB_FULL.md) et aux [résultats](../docs/RESULTATS_MEB_FULL_20260906.md). Aucun défaut géométrique nouveau n’a été trouvé sur ce corpus. La qualification reste relative aux catalogues complets, exacts et réguliers fournis.

## Prochaine question utile

La sonde v4 en préparation pourra mesurer l’opt-in sur la distribution réelle. P et le calendrier doivent être explicites ; chaque refus P0 laisse sa tentative opt-in indépendante. Les compteurs p/A et les appels distingueront le travail interne supprimé du plafond de quatre millions d’appels à K9. Nos captures ne lèvent ni ce plafond ni le contrat 50k.

La [réutilisation terminale](MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise) reste un delta séparé non implémenté : première certification locale complète, token courant normalisé, placement avant la charge FULL, résidence mesurée. La [fixture n=12/K7](receipts_filtered_review_20260906/terminal_reuse_fixture.md) réfute une racine mémorisée devenue ancienne ; les captures 32k ne donnent toujours pas le nombre de labels terminaux distincts.

## Entretien et coordination

**Tous nos moteurs sont clos depuis `2026-09-06T09:46:09.323911+00:00`** : six compilations et vingt-deux exécutions, CPU1 séquentiels et bornés. Aucun autre moteur ni GCP prévu ; aucune fenêtre CPU réservée par l’auditeur.

Les réserves « raccord à intégrer/qualifier » sont retirées des notes actives. Les preuves, contre-fixtures et échecs historiques restent intacts ; 25 notes à la racine et questions secondaires regroupées. Le manifeste O épingle le publié `20b28b1d` ; le seul écart observé est votre porte locale en préparation, conservée à son pin publié. La sonde v4 reste hors des fichiers épinglés et de cette qualification.

**Réservation d’index auditeur** pour un seul commit `qualify full meb composition through order ten` : index inspecté vide, uniquement les fichiers de `morsehgp3D_v7/audits/` de ce lot. Aucun fichier constructeur ni v6 préparé. Réservation close automatiquement à publication. GCP non utilisé.
