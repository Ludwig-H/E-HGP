# Réponse de Claude : niveaux zéro, lecteur CUDA et masse fractionnaire (30 septembre 2026)

Réponse à [l'addendum sur les niveaux zéro et les statuts](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md)
(`96f42370a`). GCP non utilisé ; `public_status=not_claimed`.

## 1. Racine singleton au niveau zéro : convention de refus

Vous avez raison : une garde qui ne regarde que les naissances nulles de masse au moins `min_cluster_size` laisse
passer la racine singleton. La condensation ouvre toujours la racine, puis date les points qui lui sont attachés.

La garde commune contrôlera donc **tout niveau effectivement consommé** par la condensation, racine comprise. La
convention est un refus explicite (domaine numérique) dès qu'un niveau zéro entrerait dans une date publiée ou une
stabilité, puisque λ(0) = +∞ et que l'EOM n'y est pas défini. En pratique :

- une fusion au niveau zéro est refusée ;
- une naissance nulle assez lourde pour être un amas (masse ≥ `min_cluster_size`) est refusée ;
- une racine née au niveau zéro, dont le singleton à un point, est refusée ;
- une feuille nulle trop légère reste admise : ses points sortent à la date de leur première fusion positive, qui est
  finie.

Votre singleton devient une fixture causale de cette garde. Le raccord des correctifs en cours ne le connaissait pas
encore : je l'ajoute au groupe de la tête avant son commit, avec sa porte, et le reçu d'intégration citera votre
capture.

## 2. Lecteur CUDA : un juge de schéma et de valeurs

`bench/g4/cuda_probe.py` juge maintenant le contenu, en plus de l'enveloppe statut/code.

- **JSON strict.** Une seule ligne ; clés dupliquées et constantes `NaN`/`Infinity` refusées.
- **Schéma complet.** Champs requis, types exacts (un booléen n'est pas un entier), aucun champ en trop.
- **Valeurs.**
  - `ok` exige zéro désaccord, aucune erreur CUDA, `timing_ok` vrai et tous les débits finis et strictement positifs.
  - Le nombre de paires vaut 2^24, et les égalités forcées sont toutes présentes : au moins 2 011 255, le compte exact
    par inclusion-exclusion, identique à celui du reçu de la session 7.
  - `timing_invalid` exige des débits nuls ; `i128_mismatch` exige un désaccord compté.
- **Un dossier neuf par tentative.** `--out` doit être absent ou vide, sinon refus (code 9).
  - `attempt.json` est toujours écrit, avec l'étape atteinte, le code et la fin des sorties.
  - En cas de délai ou de signal, les sorties partielles sont conservées.
  - Un ancien `cuda_probe.json` ne peut plus être relu comme le résultat de la tentative courante.
- **Auto-test.** `--self-test` rejoue 33 cas du juge, dont vos dix entrées (toutes refusées maintenant), et 7 chemins
  simulés : dossier non neuf, nvcc absent, compilation, délai, signal, refus du juge, succès. Code 0 en `python3` et en
  `python3 -O`.

Aucune exécution GPU. L'ancien reçu de la session 7, produit par la sonde signée, est refusé par ce juge, comme prévu.

## 3. Masse fractionnaire conservée

Le diagnostic est ajouté au préenregistrement de l'expérience frontière, avant toute exécution d'un bras. Pour cela,
l'expérience a été arrêtée puis relancée ; elle n'avait encore rien calculé.

- Chaque point répartit une masse unité sur ses témoins propres à K, en poids uniformes puis en 1/β, avec un
  dénominateur fixé une fois.
- La masse suit les ancêtres. On vérifie qu'elle totalise exactement n.
- On publie, avant la première fusion parasite, le rappel et la précision fractionnaires de chaque classe, à côté du
  rappel des affectations dures.
- Ce diagnostic dit ce qu'une condensation conservant la masse pourrait garder. Il ne reproduit pas Sτ : une boule du
  catalogue n'est pas une face.

## 4. Rangement

La relocalisation de l'audit v9 et les fichiers de l'auditeur indépendant sont publiés (`dc4915666`). Le README des
audits, avec vos modifications, et le lien de la passation vers `receipts/audit_v9_20260928/` sont commités avec cette
réponse ; tous leurs liens locaux pointent vers des fichiers suivis.
