# Réduction directe sur naissances : qualification et mesure mono

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype privé, moteur actif inchangé. GCP non utilisé.

## Résultat livré

La source dense `87eb210e…` supprime le domaine/DSU des hubs, leur certificat,
la projection et la compaction native finale. Elle garde un DSU par K sur
les naissances, des labels φ stables, les dates et toutes les marques.
Les terminales géométriques et les forêts FULL restent inchangées.
Les tris du reconstructeur d'histoires subsistent : « aucune compaction
native finale » ne signifie pas « aucun tri dans toute la tour ».
La [note de raccord](../../docs/CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md)
détaille l'invariant et les structures conservées.

O2 et ASan/UBSan/LSan passent chacun 26 commandes, avec sorties identiques :
114 census, 456 essais de fenêtres, 253 224 terminales, 30 619 873 contrôles,
29 784 coupes et 15 594 832 contrôles verticaux d'oracle borné. La gate
compare φ, marques, arêtes datées et travail géométrique à la voie ordonnée,
puis FULL/contributions/verticales au Builder et aux juges existants.
Le n32 est un différentiel, pas un oracle exhaustif supplémentaire.
Les 16 causes incluent trois vraies mutations sources : φ remplacé par
une racine courante, pivot oublié entre fenêtres, conversion prématurée.
Arguments manquants/inconnus : code 2 ; causes prévues : code 4.

Le pool CPU est qualifié **séparément**, sans géométrie dans sa propre gate :
O2 et ASan/UBSan/LSan, 2 500 contrôles identiques, 10 refus, trois créations
partielles et cinq réutilisations après refus. Les threads persistent entre
lots ; toute exception attend la quiescence. La capture TSan échoue avant
le test, code 66, `unexpected memory mapping` : conservée, pas promue.
Les qualifications du raccord parallèle sont dans un paquet distinct.

## Tour complète à 8 000 points

Uniforme u16, seed3, s8, K1..10, W65536, un thread par étape. Le temps
inclut index, génération, census, validation commune, reconstruction,
export et libérations ; toute la tour et les verticales restent retenues.
Synthèse, digests et comparaison sont hors de ce total et chronométrés
séparément. Pic RSS externe pour le processus entier.

| Mesure dense | Résultat |
| --- | ---: |
| Tour entière | 186,354254893 s |
| Atlas, géométrie et réduction dense | 58,532551438 s |
| Reconstruction des histoires | 8,012921894 s |
| Export et consultations historiques | 17,037439236 s |
| Pic RSS | 2 752 852 KiB |
| Occurrences / pivots | 10 456 312 / 3 113 381 |
| Tentatives d'union natives | 7 342 931 |
| Naissances / arêtes retenues | 2 404 646 / 2 404 636 |
| MEB / nœuds FULL | 4 359 540 / 3 976 472 |

Le digest dense reste
`a19a83fa4d646e4e0505ba2968ed9120fe8cc80aacccbf0fec61e53ad09b331f`.
Les 3 113 381 pivots ne paient plus d'union. Le domaine cumulé des DSU
passe mécaniquement de 5 518 027 hubs à 2 404 646 naissances, soit
38 474 336 octets logiques cumulés pour leurs deux tableaux size_t.
Cette somme par K n'est pas un pic de résidence.

Le [témoin ordonné parent](../ordered_streaming_20260911/README.md) mesurait
207,866762405 s, dont 72,111171096 s pour l'extraction/compaction, avec
les mêmes MEB et sortie. La nouvelle observation est favorable ; elle
ne constitue pas une statistique de speedup sur hôte exclusif. Le début
de génération chevauche la fin de la gate SAN parallèle ; aucun autre
benchmark ROOT ni compilation ROOT n'est actif pendant ce grand run.
La baisse du RSS reste faible : le catalogue et la sortie dominent encore.
Les nouveaux 16k/32k ne sont pas mesurés dans ce paquet mono ; le triplet
ordonné historique ne devient pas un triplet dense par héritage.

Les cinq microprocessus n200/400/800/s8 et n800/s10/12 exécutent chacun
une comparaison physique linéaire avec la référence matérialisée.
Même tour dans chaque paire ; à n800, même digest et même travail
géométrique entre s8/10/12. Leurs temps partagés ne choisissent pas un s
optimal. Le refus de fenêtre nulle est une capture CLI séparée, code 2,
sans payload FULL ni temps de complétion.

## Provenance, lecture et reproduction

Treize captures, 82 commandes : douze réussites et un échec TSan conservé.
Le parent unique est le paquet ordonné, manifeste `75a09eab…` ; ses sources
communes sont empruntées par hash, pas requalifiées automatiquement.
Les sources nouvelles sont conservées dans les snapshots et objets du
manifeste. Le brouillon parental `b2a472db…` reste son objet non compilé ;
il n'est pas réétiqueté comme source de ces nouveaux tests.

Deux versions du recorder sont explicitement liées : `923c5823…` pour
les gates, `36e97047…` pour le benchmark. Le second corrige uniquement
le chemin JSON de lecture des capacités streaming, avant toute capture
bench ; aucun ancien reçu n'est réécrit. Chaque capture garde ses propres
octets, arguments, sources minimales, dépendances -M/-MD, compilateur,
ELF et sorties brutes. Les exécutables sont omis du dépôt mais hashés.
Ce n'est pas un environnement de lien/runtime hermétique.

```bash
python3 -B morsehgp3D_v7/receipts/birth_streaming_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/birth_streaming_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/birth_streaming_20260911/verify.py --extract build/birth_streaming_replay_fresh
```

L'extraction exige un répertoire absent, recrée les chemins relatifs de
sources et reçus, sans exécuter les binaires historiques. Pour reconstruire,
utiliser le recorder extrait dans son répertoire `build/v7_birth_streaming_20260911`
avec un nouvel identifiant `--out`. La gate requiert Boost déjà disponible
au chemin déclaré ; aucune installation ou ressource GCP n'est déclenchée.

Aucune preuve générale de complétude WSPD ne découle de ce paquet. Le
census exact complet immuable demeure la prémisse de l'extraction. Les
contrats 50k sous 1 s, 100 ms et dizaines de millions sur G4 restent ouverts.
