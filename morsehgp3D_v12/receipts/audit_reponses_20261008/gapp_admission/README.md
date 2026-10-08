# G-APP : campagne admise, règle de poursuite rejetée

**Les primaires sont admises ; le verdict préannoncé reste `rejete`.** Sur les trois trames, le census et les premières sondes passent leur seuil GPU/CPU de0,20 ; les propositions dépassent leur seuil de0,50. Cette campagne mesure trois lots de primitives, **pas G complet, le catalogue ou FULL**. Aucun moteur, build ou appel distant n'a été lancé par l'audit.

## Provenance et fermeture

Session `v12.20261008.gapp`, source **389b5e453cdc1e70839a8e5a37eea7c42ff93a8d**. Paquet SHA `f659af96…`, plan `181f62e9…`. L'archive de33 865 octets a pour SHA `64a3beaf2b8a8f551768b363346471dad7b702b3c22dd7156070688fdd63a494` : ses **87 entrées** sont rehachées, et **366 sources** du paquet sont identiques aux objets Git, dont les sept fichiers du microbanc. Les cinq hashes de sources du microbanc et les six sources produit déclarées portées concordent avec le paquet.

Les deux commandes sont closes avec code0 : auto-test0,486 s ; construction/campagne/rapport43,236 s. Worker0, DONE0, completed, aucune erreur ; arrêt ciblé certifié code0 en une tentative, garde intacte, réserve libérée, observation `RUNNING → TERMINATED`. Ces murs de pilotes ne sont pas des latences par trame.

La construction annonce u21/CUDA, noyaux sm_120 et contraction flottante désactivée. Les hashes des deux exécutables et de la bibliothèque sont archivés ; le pilote compare les exécutables à l'entrée et à la sortie et recontrôle les cinq sources du microbanc, puis archive `empreintes_stables=true`. Les valeurs finales des hashes ne sont pas émises séparément : cette fermeture déclarée par le pilote n'est pas présentée comme une seconde mesure indépendante de l'ELF. Les32 observations d'isolation (avant/après15 processus et le mutant) sont présentes et vides, conformément au contrôle `nvidia-smi` du pilote.

## Relecture des primaires

Trois trames : ng00 **39 885**, médiane du lot37 **64 740**, maximum du lot37 **99 099** entrées déclarées. Leurs tailles XYZ/12 et IDs/4 concordent ; aucun payload n'est ouvert. **K5/W48, cinq processus neufs entrelacés par trame ; une passe initiale puis cinq chaudes**. Chaque poste mesure deux prises hôte et deux prises appareil par passe pour A/A. Soit15 journaux nominaux,90 passes dont75 chaudes ; un16e journal concerne le mutant.

La relecture vérifie cohorte, phases uniques et ordre exact, profils/sites, codes entiers0, indices de passes, types des temps, hashes des journaux et égalité avec les lignes incorporées à `campagne.json`. Les comparaisons finales des15 processus déclarent zéro écart et zéro requête non résolue ; leurs couvertures sont raccordées aux comptes récoltés. Le mutant `cote_nul`, code1, produit **232 162 écarts** identiques côté noyau partagé CPU et GPU sur ng00, alors que le lot produit reste inchangé. Il est tué géométriquement, pas par timeout ; cela ne constitue pas un oracle général du futur G GPU.

| Trame | Requêtes census / distinctes | Représentants sondés | Propositions |
|---|---:|---:|---:|
| ng00 |252 152 /177 863|3 419 932|847 125|
| médiane |290 357 /211 461|5 477 969|1 253 601|
| maximum |646 621 /462 107|9 998 189|2 481 101|

## Mesures et règle inchangée

Temps en ms : médiane des cinq médianes de processus. Les ratios sont les **moyennes géométriques des rapports appariés par processus**, pas les quotients des deux médianes affichées.

| Trame | Census CPU /GPU | Sondes CPU /GPU | Propositions CPU /GPU |
|---|---:|---:|---:|
| ng00 |7,8165 /0,8342|5,6491 /0,2971|2,4999 /1,8924|
| médiane |8,7407 /0,9231|8,7985 /0,4483|3,7010 /2,7309|
| maximum |20,9761 /1,9175|16,4895 /0,7627|7,4507 /5,4947|

Les rapports GPU/CPU census sont0,10700/0,10558/0,09133 et sondes0,05260/0,05093/0,04632. Les propositions donnent0,75755/0,73785/0,73771, avec bornes hautes95 % **0,76193/0,73966/0,73864**, toutes supérieures à0,50. Les18 ratios A/A agrégés sont dans **[0,97960 ;1,00315]**, donc dans la fenêtre préannoncée[0,90 ;1,10]. Bootstrap10 000 tirages sur les cinq processus, graine20261008 ; aucune règle ni seuil modifié après coup.

Le noyau partagé joué sur CPU donne pour census **4,4242/5,0844/12,3006 ms**, rapports au lot produit **0,56988/0,58148/0,58471**. C'est une piste CPU concrète à porter et mesurer dans G. Les propositions partagées CPU restent proches du produit : rapports0,97465/0,97436/0,97986. Ces informations n'entrent pas dans le verdict GPU et ne promettent pas le même gain dans le pipeline.

Les noyaux GPU chronométrés ont leurs entrées résidentes ; la collecte des requêtes, les préparations et transferts hôte/appareil sont hors de ces fenêtres et publiés séparément. Les données ne sont pas encore produites par un étage G résident intégré. **Additionner les médianes des trois postes joués séparément, par exemple45→8,2 ms, ne donne aucune latence intégrée ni gain FULL.** Il reste notamment les structures intermédiaires, dépendances, contrôle exact et chemins résiduels à qualifier.

Le rejeu du juge livré retrouve exactement les verdicts, motifs, règles et IC ; quatre champs flottants diffèrent d'au plus4 ULP entre le rapport Python3.10 et le rejeu local3.12. Ils sont listés dans `results.json`. Le recalcul indépendant des agrégats/bootstrap compare les valeurs avec une tolérance relative2e−15, sans modifier aucun test de seuil du juge. Normal/−O locaux produisent exactement le même résultat.

## Portée et reproduction

Les faiblesses générales d'admission du pilote sont traitées séparément : ce lot ne lui confère pas une validation universelle. Les primaires présentes ont été contrôlées plus strictement, et leur issue ne dépend d'aucune de ces tolérances de format. Ce reçu ne qualifie ni un nouveau produit, ni K10/u24/u32, ni un coût CPU/GPU total.

```sh
python3 -B check.py --repo DEPOT --session SESSION_GAPP
python3 -B -O check.py --repo DEPOT --session SESSION_GAPP
```

[SHA256SUMS](SHA256SUMS) couvre le lot. Les helpers d'archives et de comparaison Git sont réutilisés ;36 fichiers retournés (136 739 octets) restent dans la capture persistante extérieure au dépôt. Aucun brut de scène, identité de compte, ELF ou rapport complet n'est recopié ici.
