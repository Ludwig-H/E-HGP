# Contrelecture B — G4 R9, ordonnanceur q3/q4 v14

23 septembre 2026. Lecture **provisoire** du dossier
[`g4_tower_r9_20260923`](../receipts/g4_tower_r9_20260923/README.md)
dans le worktree du développeur : au moment de cette contrelecture, ce
dossier est encore **non suivi par Git**. Repères de ce snapshot :
`SUMMARY.json` SHA-256 `29fbfac0e3ab1bcfef338478b9691169e1f4e117809331f287da24c38f329867`,
`SHA256SUMS` SHA-256 `9270c097115cc9275722be654e4930541dd37018a878c901f1e2c43a40eca36a`.
Ne pas promouvoir la ligne en preuve publiée avant de figer le dossier
et de relire ces empreintes. Aucun GCP lancé par B.

## Ce qui a été relu

La VM SPOT G4 a exécuté le paquet source `fe1142b5` (sonde v14, plan v6),
**CPU seul**, 48 workers q3/q4 et 48 fils FULL, grille 1 mm u18,
séparation s8, trois trames **sans sol** 08/000000, 000100, 000200,
K1..5 et K1..10. Deux répétitions entrelacées allument ou éteignent
**ensemble** `q34_jobs_by_mass` et `q34_fine_jobs` : ce n'est pas un
factoriel 2×2, donc leurs effets individuels ne sont pas attribués.

Les reçus hôte et VM indiquent `completed`, 24/24 cas
`complete_relative`, 18 comparaisons d'objet égales, Euler `holds`,
arrêt ciblé certifié et cible `TERMINATED`. Une contrelecture locale a
fait tourner le validateur de production **épinglé à `fe1142b5`** sur
les sorties brutes, GNU time, invocations, dépendances et comparaisons :
codes 0 en Python normal et sous `-O`, 24 cas dans les deux sorties.
`sha256sum -c SHA256SUMS --quiet` : code 0. Le lecteur de ce snapshot
n'est pas un juge indépendant du code du validateur épinglé. Le
`PACKAGE.json` est le **plan initial** et dit encore `GCP_used=false` ;
l'exécution est attestée par les reçus hôte/VM, pas par ce champ.

## Résultat apparié, deux répétitions

| Trame 08/ | Tour | Chaîne OFF (s) | Chaîne ON (s) | q3/q4 OFF → ON (s) |
| --- | --- | ---: | ---: | ---: |
| 000100 | K1..5 | 3,687 / 3,805 | **2,799 / 2,816** | 2,524/2,648 → 1,656/1,681 |
| 000000 | K1..5 | 5,242 / 5,483 | **3,796 / 3,803** | 3,698/3,890 → 2,210/2,156 |
| 000200 | K1..5 | 6,301 / 6,289 | **3,992 / 4,072** | 4,590/4,635 → 2,365/2,387 |
| 000100 | K1..10 | 9,514 / 9,449 | **8,596 / 8,652** | 5,339/5,305 → 4,444/4,462 |
| 000000 | K1..10 | 13,597 / 13,574 | **11,752 / 11,753** | 8,066/8,012 → 6,244/6,243 |
| 000200 | K1..10 | 14,945 / 15,086 | **11,892 / 11,854** | 9,631/9,608 → 6,613/6,622 |

L'attente des workers q3/q4 passe de **35–49 % à 0,4–0,9 %** à K5.
Le plus long job du front passe de **2,5–3,7 s à 0,23–0,31 s**.
Ce gain de calendrier est crédible car, à travail et objet égaux,
les deux répétitions le reproduisent. La partie `job_sum_s` ne couvre
cependant que les jobs du front, pas chaque plage publiée ; elle ne
permet pas seule une attribution détaillée de la queue des plages.

Avec ON, le mur q3/q4 n'est plus que **1,02–1,03 fois** `cpu_sum_s/48`
à K5 et **1,01–1,08 fois** à K10. Pour ce binaire et ce travail CPU
inchangé, l'ordonnancement des 48 workers est donc presque saturé :
une autre redistribution seule ne peut promettre les facteurs encore
nécessaires. Cela **n'est pas** une borne pour un autre algorithme, pour
moins de travail exact, ni pour le GPU.

Même en supprimant fictivement q3/q4, le chemin CPU actuel garde
**1,135–1,685 s** à K5 selon trame/répétition, et **4,152–5,510 s** à
K10. Sur les cas ON, FULL représente déjà 0,709–0,940 s à K5 et
2,971–3,805 s à K10.
Le contrat demande donc simultanément moins de travail q3/q4 et moins
de coût hors q3/q4 ; D5 ne couvre le second qu'à titre de projection.

## Statut de contrat et suite

Le meilleur cas K1..5 reste **2,799 s** et K1..10 **8,596 s**, pas
moins de 1 s. R9 ne mesure ni GPU, ni trame brute avec sol, ni plusieurs
séquences, ni s10/s12, ni pente 8k/16k/32k de ce même binaire. Les
digest et comparaisons relatives n'établissent pas la complétude de
toutes les `BallKey` absentes du catalogue. `public_status=not_claimed`.

À la suite de ce reçu, prioriser un certificat **avant expansion** des
produits q3/q4 et mesurer `core_form_sites` sauvés sur les arêtes qui
atteignaient réellement le cœur ; ne pas compter seulement des rejets
déjà obtenus par les voies mortes ou le cache. Garder en parallèle le
premier port FULL exact, limité et apparié, ainsi que les coûts q2,
catalogue et sortie. Les chronos ON de R9 sont maintenant la base CPU
appropriée pour un futur port GPU à ledger identique.
