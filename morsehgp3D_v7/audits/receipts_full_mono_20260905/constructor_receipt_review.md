# Contrelecture des reçus constructeur FULL mono

Observation close le 5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Les deux paquets sont cohérents : une admission micro, puis cinq tentatives mono closes, dont trois réussites horizontales relatives à 8k et deux refus d’alias à 16k/32k. Le [reçu indépendant](constructor_receipt_review.json) conserve les pins, commandes, contrôles des flux, mesures extraites et limites. Aucun moteur, build, sanitizer, benchmark ou Git n’a été exécuté par cette contrelecture. GCP non utilisé.

## Sceaux et sources réellement consommées

Les sceaux des paquets [micro](../../receipts/full_gabriel_probe_20260905/README.md) et [mono](../../receipts/full_gabriel_mono_20260905/README.md) couvrent respectivement 11 et 22 fichiers, hors `SHA256SUMS` lui-même. Le manifeste mono couvre exactement 21 pièces ; son propre hash appartient au sceau. Aucun doublon JSON, chemin ambigu ou fichier omis n’a été rencontré dans ces inventaires. La publication `98bb6578` est attribuée au message du constructeur ; aucun objet Git n’a été interrogé ici.

Les 51 pins sont identiques entre les snapshots micro avant/après et mono avant/après. Ce total est un inventaire source : le depfile `-MMD` contient exactement **39 dépendances utilisateur**, toutes présentes avec les mêmes hashes. La sonde exécutée est `f3de0d3ca850611f328cb41b251ec66c914afe473eed8e55f89eb889898f1849`, le producteur historique `e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170` et le binaire privé `d6126f7778d7d7bb370cc59d356eb927bffa57f4cefeb72f8719a77ef6720204`. Le binaire encore disponible reproduit ce dernier hash.

**Le worktree a changé pendant la revue.** Une première lecture retrouvait les 51 sources ; la fermeture retrouve 50 concordances et le seul delta `full_gabriel.hpp`, observé à `13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627`. L’analyse des alias ci-dessous utilise la [capture historique e02d](../receipts_full_producer_20260905/source/morsehgp3D_v7/src/forest/full_gabriel.hpp), avec son pin et l’extrait relu dans le JSON. Ces campagnes ne qualifient pas le delta courant.

Le compilateur et l’environnement sont observés après le micro. Le binaire n’est pas archivé, les en-têtes système et bibliothèques ne sont pas capturés, et aucune reconstruction hermétique n’est prétendue. Les sources relues sont identifiées séparément des fichiers seulement hashés.

## Admission micro distincte

Les 13 commandes sont terminales : compilation neuve code 0, six positifs `n=8`, `s=8/10/12`, `Kmax=5/10` code 0, puis six refus de parsing code 2. Les 26 chaînes UTF-8 de `raw_streams.json` reproduisent exactement les longueurs, SHA et fichiers privés stdout/stderr. Les 13 reçus privés et leurs intents concordent avec les commandes publiées ; tous les stderr sont vides.

La compilation emploie C++20, `-O3 -DNDEBUG`, les quatre options de warnings, `-pthread`, `-MMD`, sans `MHGP7_TESTING` ni sanitizer. Le runner crée un dossier neuf exclusif, abaisse `RLIMIT_AS`, borne chaque enfant à 60 secondes et draine son groupe au retour. Aucun timeout n’est observé. L’admission ferme **39 lignes d’ordre** : trois demandes 1..5 et trois demandes effectives 1..8. Les nœuds sont `[15,22,26,23,19]`, prolongés par `[12,5,1]` ; K=n conserve une feuille, sans parent ni connexion supérieure. Les refus concernent option manquante, doublon, `n=9`, `s=9`, `kmax=0` et option inconnue, sans ordre construit. Ces contrôles restent des admissions à huit points, distinctes de toute mesure 8k et de la qualification sanitizer antérieure du producteur.

## Les cinq tentatives mono

La commande gardée est `timeout --signal=TERM --kill-after=10s 600s taskset -c 6 /usr/bin/time -v … --n=N --s=S --kmax=10`. Pour chaque tentative, intent, JSONL brut, reçu extrait, terminal et code GNU time concordent. Chaque terminal est unique, dernière ligne JSONL avant le rapport GNU time. Les deux échecs comportent aussi le diagnostic externe de sortie 2. Aucun timeout ni signal livré n’est observé.

| n / s | Code | Ordres terminés / refus | Avant terminal | GNU time, processus | Pic RSS GNU time |
| --- | --- | --- | --- | --- | --- |
| 8k / 8 | 0 | 1..10 | 150,776472 s | 150,82 s | 1 837 632 KiB |
| 16k / 8 | 2 | 1..8 ; refus K9 | 275,497460 s | 275,57 s | 2 893 256 KiB |
| 32k / 8 | 2 | 1..6 ; refus K7 | 464,273326 s | 464,37 s | 5 130 120 KiB |
| 8k / 10 | 0 | 1..10 | 150,879291 s | 150,92 s | 1 831 988 KiB |
| 8k / 12 | 0 | 1..10 | 151,794939 s | 151,86 s | 1 833 128 KiB |

Les refus sont exactement `full_gabriel_alias_budget`, avec **8 000 000 alias**. Les entrées minima+connexions du dernier ordre comptent respectivement 2 301 919 et 3 040 851 records, sous leur propre cap de 8 millions. Dans la source historique, `put_alias` recherche d’abord une entrée existante, puis charge avant `emplace` ; atteindre le cap interdit l’installation suivante. Ce verrou concerne les alias installés, pas une saturation du budget de records, ni une mesure de la mémoire totale. Les cinq compteurs de payload du dernier ordre refusé sont nuls ; ses compteurs de travail persistent. Les huit et six ordres antérieurs sont diagnostiques, leurs forêts ne sont pas publiées.

Les trois livres de masse par tentative ferment chacun sur le nombre de paires attendu ; les six compteurs de workers valent 1. Les trois essais 8k conservent 3 113 381 boules au census et les mêmes champs d’ordre hors durées/mémoire. Le front diffère : 3 144 017, 3 129 992 et 3 123 497 candidats bruts. Ces constats portent sur les **volumes**, sans établir l’égalité des forêts.

## Attribution des temps et limites de capture

Les observations de collecte vont de 15:57:12 à 16:24:21 UTC ; elles ne mesurent pas la durée des processus. Le chronomètre interne inclut entrée, index, front, construction FULL, lectures, destructions et émissions provisoires ; il exclut configuration initiale et terminal. GNU time mesure le processus entier. Les stages ne constituent pas une partition exhaustive : le résidu après somme des stages et émissions est notamment de 85,151 ms et 151,741 ms sur les refus, où le nettoyage hors blocs chronométrés reste inclus dans le temps global. Les 275,497 s et 464,273 s sont des temps de tentative terminée en refus, jamais des temps de réussite d’un préfixe.

Le proxy de payload de 8 GiB, les caps d’alias et de records et la limite d’espace virtuel de 26 GiB ont des objets distincts ; aucun ne borne seul le RSS total, les maps, les capacités STL ou l’allocateur. Les certificats sont détruits ordre par ordre : le pic ne mesure pas une archive complète conservée.

La comparaison des bruts mono aux sorties originales de l’outil est une attestation de `verification.json` : ces objets de conversation ne sont pas un second fichier disponible à cette contrelecture. La chaîne publiée est néanmoins cohérente entre bruts, intents, reçus, résumé et rapports GNU time. Les **dix rejeux Python** du lecteur, cinq en mode normal et cinq sous `-O`, ainsi que ses deux selftests reproduisent exactement les sorties publiées. Chaque selftest conserve un positif réel, deux refus synthétiques et neuf mutants de cohérence ; il ne relance aucun moteur. Un reçu cohérent en refus garde `attempt_success=false`.

Une seule tentative par configuration sur hôte partagé, sans capture d’exclusivité CPU ni contrôle `/proc` par enfant, ne permet aucun gain statistique, classement des s ou speedup apparié avec F. Le front réutilise les contrôles produit ; les sentinelles de racine/couverture ne sont pas un oracle de son catalogue. L’autorité reste horizontale et relative aux catalogues complets, exacts et réguliers fournis. Aucun digest entrée/forêt, tour inter-K, masses, archive, SLO 50k ou résultat G4 n’est qualifié.
