# Contre-audit du port q3 — 26 septembre 2026

Lecture indépendante au HEAD `92c709bc8f48aedd09d5c5f1ce3a283598fe23bc`.
Le moteur examiné est inchangé depuis `9751bae69` ; `f9f273bb0` corrige
la fixture de preuve multi-passage, pas la géométrie. Aucun GCP, changement
moteur, recompilation ou remplacement d'un build épinglé dans cet audit.

**Verdict : aucun défaut fonctionnel nouveau démontré.** Le transport et
l'import sont cohérents avec leur contrat. Les preuves disponibles ne
justifient ni un défaut ON, ni les 100 ms, ni la croissance sous-quadratique
de toute la chaîne. Il faut poursuivre le chantier structurel q34, pas
chercher quelques pourcents supplémentaires dans ce port.

## Ce qui a été vérifié dans le code

- **Complétude, sans faux second census.** Pour une boule q3 régulière,
  le producteur fournit le compte global `depth` et exactement autant d'IDs.
  L'import exige une clé recomputée identique, le domaine entier, le niveau
  positif, des IDs distincts strictement intérieurs et un support positif
  sur la frontière. Si le compte producteur est exact, un sous-ensemble
  distinct de même cardinalité est tout l'intérieur. De même, trois sites
  de support sur une coquille annoncée de taille trois donnent toute la
  coquille. **L'import seul ne prouve pas ces cardinalités globales** :
  leur autorité vient du census producteur, de sa couverture complète et
  des différentiels. La raison suffixée `_payload` est donc nécessaire.
- **Pas de corruption par tri.** `Presentation` garde 112 octets ; son
  ordinal u32 se déplace avec la valeur. Les records ne sont pas réordonnés
  sans leurs IDs : slot physique fusionné → staging → destinations q3/q4
  → ordinal du record → paquets de 4096 → plages de tri. Le code refuse
  plus de `UINT32_MAX` records avant la conversion des offsets. Le dernier
  ordinal utilisable n'est jamais la sentinelle. Une seule arène possédée
  survit jusqu'à la fin du catalogue ; le thread producteur est joint avant
  la lecture par les sinks.
- **Aucune concaténation par clé.** La fusion choisit le représentant de
  plus petite arité/support, vérifie profondeur/coquille et refuse les
  présentations doublées. Les intérieurs de plusieurs présentations d'une
  boule ne sont jamais ajoutés. Les IDs d'entrée sont convertis une fois
  vers les rangs de l'index de tour, puis contrôlés géométriquement.
- **Replis conservés.** Un CPU tail n'a pas de handle et refait le census.
  Une coquille q3 étendue ne s'importe pas : census global et `ShellTable`
  restent actifs. q4 ne consomme pas les sentinelles de son sidecar. K2
  autorise le payload actif à stride zéro ; K1 n'appelle pas le producteur.
  Census q2 anticipé, première erreur par clé et statistiques restent communs.
- **Juge réellement indépendant.** Avant le tri, le juge scanne tous les
  points pour comparer les IDs. À l'import, il recense indépendamment puis
  compare la `BallData` entière ; ce travail est ajouté aux visites de census.

Points de code : `src/gpu/lanes.hpp:568`, `lanes_tasks.hpp:333`,
`filter_runner.cu:1073,1219`, `src/gen/pipeline/wspd_q34.cpp:1267,1414,1855`,
`src/chain/tower_chain.cpp:555,882,1554,1909`.

## Contre-tests réellement rejoués

Les deux binaires Release épinglés ont été hachés avant/après puis exécutés,
sans reconstruction. Ils ne font pas zéro travail :

- producteur : 140 appels, 9 602 q3 / 1 648 q4, 18 873 IDs, 3 768 reports,
  68 replis fusionnés, 28 cas de frontières de chunks, profondeur huit ;
- chaîne : 192 configurations, 16 juges, huit refus, 90 862 imports /
  351 828 IDs, 40 898 replis ; bras multi-chunks de 85 363 records,
  45 421 imports et 16 plages de fusion. Tous passent.

SHA256 producteur `6d506ffd705351fac4ebd4c327245377c0172d998632ec891b5d20f007185dc4` ;
chaîne `bbabdebb68f8c6a65845395dc22f53ffc6dd5481d36de8cd58859ddae1b1fbf8`.
Recette et sources : `receipts/q3_payload_native_20260926/INVENTORY.json`.
Ce rejeu est CPU, pas une nouvelle gate CUDA. La gate CUDA archivée réalise
84 appels device, avec reports et replis fusionnés effectivement non nuls.

## Preuves G4 et lecture des phases

Les 26 probes bruts ont été relus directement : **18 GPU + 8 engine CPU**,
neuf paires ON/OFF. Trois digests et générateur égaux dans chacune des neuf
paires, 15 023 346 imports et 62 086 877 IDs cumulés sur les neuf exécutions
ON — pas autant de points distincts. Tous les cas ont `frames=1` et
`lanes_deferred=0`. Le reçu conserve 19 comparaisons entre configurations.

00/K5/s8, médianes de deux processus par bras, en ms :

| Mesure | OFF | ON |
|---|---:|---:|
| FULL | 927,789 | 922,663 |
| q34 | 488,485 | 496,8555 |
| Census | 102,5415 | 82,8245 |
| Construction de la tour | 284,3405 | 291,079 |

La réduction de visites est certaine ; l'attribution d'un gain FULL stable
ne l'est pas (deltas appariés −11,474 puis +1,222 ms). Le census q2
anticipé et certaines sous-étapes GPU/CPU se chevauchent : on ne peut
additionner tous les sous-chronos, ni transformer une baisse de census
en gain de même taille sur la chaîne. La ligne census ci-dessus est le
census principal, distinct du temps de census q2 anticipé.
Contexte CUDA/réservations de session sont avant `chain_total` ; digests et
lecture fichier sont séparés. Il n'y a ici **aucun passage chaud**.

Autorité : `receipts/g4_q3_payload_20260926`, SUMMARY SHA256
`946da1f39949e3184e4ed1ae6b033e2f7d3aafbd63e174543a31b2e4141b4283`.
Source transportée `f9f273bb0`, pas transfert implicite de qualification
au HEAD documentaire. Les captures échouées restent à part.

## Deux limites concrètes à garder pour la suite

1. **Pression mémoire GPU.** Le coût par record est désormais
   `128 + 4*(K−2)` octets avec payload ; les capacités automatiques utilisent
   ce coût (`filter_runner.cu:1446`). À mémoire limitée, ON peut donc décider
   moins d'arêtes et envoyer davantage de travail aux CPU tails. Ce n'est
   pas une erreur géométrique, mais l'égalité des ledgers ON/OFF observée ici
   n'est pas garantie aux tailles massives. Les replis forcés sont testés,
   pas cette transition de capacité automatique sur une grande scène G4.
2. **Futurs chronos chauds.** `frames.results` conserve statut, raisons,
   trois digests et comptes de tour de chaque passage, mais les ledgers
   détaillés/payload du corps JSON restent ceux du premier. Une future
   campagne multi-passage peut vérifier l'objet sans pouvoir attribuer les
   mêmes travaux à tous les chronos. Ne pas prétendre le contraire ; aucune
   modification de protocole n'est nécessaire pour les 26 cas actuels.

Les tests 8k/16k/32k locaux ne démontrent pas une borne générale : expansions
q34 des amas ×3,977/×3,987, sorties q4 terrain ×4,07/×4,18, certains replis
amas au-delà de ×4. L'import ne change aucun de ces candidats/travaux.
La prochaine optimisation doit supprimer des expansions/censuses redondants
avant émission et préserver la complétude, non seulement paralléliser ce
travail presque quadratique. Aucun nouveau plafond proposé.
