# Contre-audit de la campagne M2 du 7 octobre 2026

**Le choix local `j3_r168` est confirmé par le recalcul indépendant des prises réelles.**
Le défaut générique du pilote **CST0018 reste ouvert** : cette campagne réunit toutefois
les preuves que le pilote omet d'exiger. Aucun faux verdict d'adoption n'est établi ici.
Audit CPU local uniquement, sans GPU, GCP, build ni lecture des nuages ou vidages binaires.
Cadre : `exploration_v12_hors_registre`, `quantized_u21_input_only`, `not_claimed`.

Pin audité : `4147c546000b198b5239646063bfb1e3ed6d28fc`. Reçu initial `97d45b65d`,
complété par `ef5365cab` : les **46 JSON de processus**, initialement omis par les règles
Git, sont bien présents au pin audité. Le premier commit seul ne suffit pas au rejeu.
Entrée source : [campagne publiée](../../g4_t0a_20261007/README.md).

## Verdict recomposé

Les neuf cas sont `ng00`, `ng01`, `ng02` × `K5/16`, `K5/24`, `K10/24`.
Les six cas de feuilles 24 décident ; les trois autres imposent aussi l'identité.
Les 45 processus retenus ont chacun sept formes et quinze mesures conservées,
soit **4 725 durées finies strictement positives**, après trois échauffements.
Le bootstrap reprend l'unité processus, les rapports appariés et les 10 000 tirages
de la règle antérieure. Le seuil porte sur chaque cas décisionnel.

| Forme | Moyenne géométrique des six cas | Pire borne haute IC95 | Verdict recomposé |
| --- | ---: | ---: | --- |
| `j3_r168` | 0,1763889694 | 0,2026892599 | adoptée, choisie |
| `j3_r128` | 0,1959577232 | 0,2230587354 | adoptée |
| `j3` | 0,2390149930 | 0,2764412186 | adoptée |
| `coherent_r128` | 0,3603423935 | 0,4401467925 | rejetée |
| `coherent_r168` | 0,3626930838 | 0,4444775727 | rejetée |
| `coherent` | 0,4922997891 | 0,6020838567 | rejetée |

Tous les ratios et IC publiés sont retrouvés à moins de `1e-12`. Une seconde
méthode énumère les **5^5 = 3 125** rééchantillonnages possibles par cas : elle
donne les mêmes six décisions. Cela vérifie le calcul ; cinq processus restent
l'effectif observé, et l'IC est celui du protocole, pas une borne universelle.

Médianes des médianes, noyau seul, témoin → `j3_r168` en ms : ng00 K5/24
69,619 → 11,5868 ; ng01 64,8838 → 9,31811 ; ng02 67,9057 → 10,7714 ;
ng00 K10/24 183,876 → 37,2387 ; ng01 149,409 → 29,3987 ; ng02 170,648 → 33,8369.
Ce résultat n'établit ni le catalogue complet, ni la tour, ni le contrat 100 ms,
ni la couverture de plusieurs séquences et des régimes D7.

## Identité, entrée et exécution

- Les **18 lignes d'identité hôte** couvrent les deux formes et neuf cas,
  soit **2 748 544 feuilles par forme**, toutes résolues, sans différence.
- Les **315 identités GPU** couvrent sept formes × 45 processus : aucun
  débordement, aucune feuille non résolue, compte/population/émission identiques.
  L'identité est vérifiée par une passe distincte de chaque forme dans chaque
  processus, pas à chacune des quinze répétitions chronométrées.
- L'auto-test d'arène réussit, six mutants tués, aucun survivant. Les trois outils
  Compute Sanitizer retournent zéro et leurs journaux bruts indiquent zéro erreur
  (racecheck : zéro avertissement aussi). Leur domaine est **les 3 000 premières
  feuilles de ng00 K5/24, six formes**, pas les neuf cas entiers.
- Les neuf empreintes de vidages sont identiques à celles du
  [reçu local antérieur](../../mes_m2_local_20261007/RAPPORT.md).
  Les six empreintes d'entrée concordent avec le manifeste de session. Aucun
  vidage ni donnée LiDAR n'est recopié dans ce reçu.
- Les 72 étapes sont uniques : toutes ont le code zéro sauf `git_head` (128),
  attendu dans une extraction sans `.git`. Il n'y a aucun refus déclaré.

Le paquet source mesuré est épinglé par SHA-256, comme son manifeste et le plan.
Son `head_commit` est `26b53648ce7f0213ab6995a47afbb109cbf5c73f` ; il décrit un
**instantané de worktree**, avec modifications déclarées, pas uniquement cet arbre Git.
Les 18 fichiers M2 du paquet sont néanmoins identiques au pin audité ; les 16 sources
instrumentées sont aussi identiques au commit déclaré. Les cinq sources v11 hachées
correspondent à `ac081a06f`. L'archive v11 transmise est reproduite directement par
`git archive` : 1 478 328 octets, SHA-256
`6f3454ad9d9ad2f6bb0b846f1aaad7c1a4c509d14cd450a99381c72b04a57885`.
`repo_head=null` dans le rapport M2 n'est donc pas assimilé à une provenance absente.

Les cinq empreintes de binaires sont enregistrées dans le JSON de résultat de cet audit.
Les ELF ne sont pas conservés ici : leur empreinte déclarée ne peut pas être rehachée
indépendamment. Les logs de compilation et les chemins recollent aux sources mesurées.
Attention : M2 construit sa propre bibliothèque v11 `work/b_v11/libmhgp11.a` limitée
au catalogue. Le hash `8a27ce2f…` de la bibliothèque produite par l'étape `source_v11`
ne désigne pas cette seconde bibliothèque liée par M2.

## Raccord aux petites archives brutes locales

Les archives rapatriées de la session ont été lues en mémoire, sans extraction
de données : résultats 128 333 octets, paquet source 821 698 octets. Leurs empreintes
égalent celles du reçu ; celles du plan JSON et du plan shell correspondent aussi.
Les **73 fichiers** du manifeste public existent et leur empreinte est correcte.

Les 50 JSON M2 publiés sont exactement ceux de l'archive après la seule anonymisation
des répertoires personnels. Les journaux établissent les 45 commandes retenues,
avec le bon cas, chemin de sortie, sept formes et paramètres ; leurs **315 médianes
stderr** concordent avec les JSON. Les neuf résumés de vidage et les lignes d'identité
hôte et d'auto-test recollent également. Les 45 JSON retenus ont des empreintes distinctes.
Le paquet initial ne contient aucun `runs`, `dumps` ou `build` M2.

Ce raccord apporte un faisceau positif de prises produites dans cette session.
Il ne prétend pas établir une attestation cryptographique d'exécution : le pilote
reste vulnérable aux JSON périmés démontrés dans CST0018. Les dates des fichiers
archivés sont toutes celles de la **publication**, car `publier.py` utilise
`copyfile` ; elles ne prouvent pas les heures individuelles de génération.
L'isolation GPU est contrôlée au démarrage, sans trace continue des autres processus.

Deux corrections rédactionnelles sont nécessaires dans le reçu de campagne :
`logs/*.log, 118 fichiers` correspond en réalité à **72 logs + 46 JSON de processus** ;
`15 répétitions dont 3 d'échauffement` doit devenir **15 conservées plus 3 échauffements**.
Le README du microbanc §10 annonce aussi `1 + 5 processus par cas` : le code joue
**un seul processus jeté global**, puis 45 processus retenus, ce qui satisfait la
règle §9 du premier processus jeté mais mérite une rédaction cohérente.

## Reproduction et suite

Depuis la racine du worktree, avec les commits Git accessibles :

```sh
python3 morsehgp3D_v12/receipts/audit_session_t1_20261007/campagne/check_campaign.py
python3 -O morsehgp3D_v12/receipts/audit_session_t1_20261007/campagne/check_campaign.py
```

Le script refuse toute différence des fichiers consommés par rapport au pin.
Les contrôles utilisent des exceptions explicites, jamais `assert`. Le mode complet
requiert les petites archives locales indiquées par `receipt_path` ; il refuse leur
absence. `--published-only` rejoue seulement les preuves versionnées et marque
explicitement l'absence de contre-lecture brute. Les sorties normal/`-O` sont identiques ;
[verification.json](verification.json) en conserve la vérification et les empreintes.
[results.normal.json](results.normal.json) expose calculs, domaines et hashes détaillés.

CST0018 reste fondé par les
[témoins antérieurs](../../audit_socle_microbancs_20261007/preuves/README.md) : le SHA du
pilote est toujours `84e4c192d9dd8a42e1b45aa096440129e749c5e3f957f4d4ee8dc83b4ee6cd88`.
Avant une nouvelle campagne, exiger les portes hôte, la grille complète, l'identité
entrée/commande/sortie et un nouveau fichier résultat par processus. Cette exigence
de réparation est distincte de l'adoption M2 locale que le présent reçu confirme.
Les contrats génériques du lecteur (CST0215), le cycle de vie G4, M6 et les données
au-delà de leurs empreintes appartiennent aux autres volets de l'audit.
