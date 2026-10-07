# Contre-audit des campagnes réelles C/D — 7 octobre 2026

**Les résultats locaux annoncés sont confirmés.** M5 passe sa règle dans C ;
M3 passe à K10 avec cinq prises par trame dans D ; M4 est conforme en identité
dans D, satisfait les seuils chronométriques publiés à K5 et dépasse toujours
celui de contraction à K10. Aucun nouveau défaut de ces campagnes n'est établi.

Pin audité : `07ee13ef6bebc0b6b85da90207f3f067ffff1755`, addendum C `2f7b41380` inclus.
Instantanés mesurés : C `320db4a125a21c0c71c7a6e18af21aaff9f72da5`, D
`bf70e8b993d33837b8c3bce4bd3a3606411c350a`. Contre-lecture CPU locale seulement :
aucun GPU, GCP, build, ni lecture de nuages ou vidages binaires.
Cadre : `exploration_v12_hors_registre`, `quantized_u21_input_only`, `not_claimed`.

## C — le verdict M5 est soutenu par les prises réelles

[Reçu C](../../g4_t0c_20261007/README.md). Les douze cas sont présents : trois
trames × K5/16, K5/24, K10/24, plus les uniformes 8k/16k/32k à K5/24. Chaque
cas a un tour 0 écarté puis cinq tours conservés. **144 commandes CPU/GPU**
recollent aux journaux bruts ; les 171 étapes ont le code zéro.

La médiane GPU est recalculée sur les quinze durées après trois échauffements.
La médiane v11 porte sur les neuf dernières des dix passes, à 48 fils. Son temps
est bien `prefix_ns + single_pass_ns` ; le diagnostic `walk_ns`, joué seulement
au tour 0, n'entre pas dans le dénominateur. Le numérateur GPU inclut copie du
nuage, niveaux et rapatriement des feuilles. Aucun tableau de médianes déclaré
n'est pris comme autorité sans comparaison aux durées brutes.

| Cas décisionnel | Rapport géométrique | IC95 recalculé | GPU / v11, médianes en ms |
| --- | ---: | --- | ---: |
| ng00 K5/24 | 0,098586 | [0,098025 ; 0,099075] | 4,72275 / 47,805204 |
| ng01 K5/24 | 0,105165 | [0,104660 ; 0,105646] | 4,25555 / 40,462142 |
| ng02 K5/24 | 0,099226 | [0,098967 ; 0,099486] | 4,78752 / 48,200060 |
| ng00 K10/24 | 0,073128 | [0,072991 ; 0,073219] | 10,85880 / 148,415411 |
| ng01 K10/24 | 0,075523 | [0,075280 ; 0,075813] | 9,28682 / 123,270726 |
| ng02 K10/24 | 0,072980 | [0,072535 ; 0,073418] | 10,45430 / 143,207478 |

Les douze rapports et IC sont retrouvés à moins de `1e-12`. Une énumération
indépendante des 5^5 rééchantillonnages possibles par cas confirme que les six
bornes décisionnelles restent sous 0,25. Ce sont **60 paires de processus**,
900 durées GPU par métrique et 540 passes v11 retenues. Les autres cas restent
informatifs conformément à la règle écrite avant mesure.

L'identité GPU est vraie pour tous les tours, avec zéro feuille manquante/en trop,
zéro désaccord de liste/métadonnées et zéro allocation pendant les passes mesurées.
Les populations, compteurs de parcours et digests concordent avec les vidages et
l'identité hôte. Les **19 cas hôte** comprennent les douze cas mesurés, le crop4000
et six fixtures. La fixture `coquille48_k5_l8_m8` est un **refus attendu de statut 1**
des deux côtés, pas un parcours réussi ; les autres identités et contrôles de nœuds
sont positifs. Les six mutants sont tués sur l'hôte ; le mutant requis
`repere_enfant` est tué par la coquille u32 sur hôte et appareil.

Les trois Compute Sanitizer sont propres, journaux bruts compris. Domaine exact :
**crop de 4 000 sites + `coquille48_u32_k5_l24` + `uniforme_u32_k3_l8`** ; aucune
qualification sanitizer des douze cas entiers n'en découle. Les cinq empreintes
binaires, les sources et leurs pins concordent. Les ELF ne sont pas conservés pour
un rehachage autonome ultérieur. L'isolation GPU est relevée au démarrage.

**CST0018 reste ouvert pour le juge M5.** Ses vulnérabilités génériques ne sont pas
déclenchées par cette capture complète. Le résultat qualifie ce microbanc, pas
l'intégration parcours-feuilles, la fin d'étage ou un chrono FULL. Additionner le
parcours C et le noyau feuille A reste une estimation entre microbancs distincts.

## D — réplication et rattachement acquis sur cette capture

[Reçu D](../../g4_t0d_20261007/README.md). Les prises décisives viennent de
**quinze processus de résolution distincts à K10**, cinq par trame, une passe par
processus. Les quinze autres prises de résolution K5 sont publiées séparément,
avec trois passes internes et sans décision M3.

| Trame K10 | Rapport géométrique recalculé | IC95 recalculé | Réduction |
| --- | ---: | --- | ---: |
| ng00 | 0,545047666 | [0,544368366 ; 0,545727813] | 45,50 % |
| ng01 | 0,538794259 | [0,537839936 ; 0,539661648] | 46,12 % |
| ng02 | 0,547584358 | [0,546439892 ; 0,548864926] | 45,24 % |

Chaque ratio est recomposé depuis les lignes `resolution_un_fil`, en additionnant
les ordres 2..K pour chacun des deux bras répliques. L'unité bootstrap est le
processus. Les IC publiés sont retrouvés à moins de `1e-12`, et le bootstrap
exhaustif donne la même décision sous 0,60. Aucun transfert des processus du
microbanc MEB vers l'effectif de résolution n'est effectué.
Les bornes exhaustives diffèrent légèrement des bornes du pilote à 10 000 tirages ;
l'accord porte sur le verdict, pas sur une égalité entre les deux estimateurs d'IC.

Les six nonces de campagne sont distincts. Chaque prise possède son répertoire
`cas/resolution/campagne/pN`, sa commande, son journal et le hash du binaire
construit inchangé. **100 journaux de preuves** sont vérifiés contre leurs hashes
originaux, puis contre les copies publiées anonymisées. Les sources de chaque
exécution et les dépendances compilées concordent avec le pin M3/M4 et la v11
`ac081a06f`. Les cinq hashes binaires sont identiques entre les lots K5/K10.
Le reçu indique la comparaison des vidages réécrits à ceux de référence pour
chaque prise ; ces binaires de données sont volontairement absents de cet audit.

Le microbanc M3 possède 24 processus et 141 lignes par ordre. M4 en possède
30 et 225 : cinq par trame et par K. Toutes les identités sont positives ; T6
juge toutes les naissances des ordres ≥2, soit **14 829 064** comptées une fois
sur les six cas. Les portes et mutants causaux sont non vides. Le code des
exécutables est rattaché aux sources mesurées par les empreintes enregistrées ;
il ne s'agit pas d'un build reproductible avec ELF conservés.

| Cas | Noyau, un fil, ms | Contraction, 48 fils, ms |
| --- | ---: | ---: |
| ng00 K5 | 8,85332 | 1,75110 |
| ng01 K5 | 7,23387 | 1,54445 |
| ng02 K5 | 9,49609 | 1,96082 |
| ng00 K10 | 26,61310 | 4,25018 |
| ng01 K10 | 19,87320 | 3,23610 |
| ng02 K10 | 24,67560 | 4,10237 |

Ces valeurs restent les médianes interprocessus de minima de cinq passes, par
ordre. Le juge automatique M4 rend **« conforme » en identité**, et précise que
les temps sont publiés sans verdict automatique d'adoption. La comparaison aux
seuils confirme K5 ; la contraction K10 dépasse 3 ms. Ce n'est pas une adoption
de K10, ni un temps mural de tous les ordres exécutés ensemble.

La publication intermédiaire `005_m34_k10_publier_resolution` conserve le refus
M3 faute de preuve d'identité M3. Le rapport final `007_m34_k10_publier` ajoute
cette preuve et adopte ; ses prises de résolution sont **inchangées**. Le lot K5
rend toujours « refusé » pour M3 car les cas décisionnels K10 n'y figurent pas.

Le résidu générique du juge de mutant M4 découvert dans l'autre volet de l'audit
n'est pas déclenché ici : les deux mutants réels portent explicitement `ng00`,
le bon K, tous les ordres 1..K et les nombres de naissances, cellules,
représentants et nœuds v11 des inventaires attendus. Leur écart géométrique et
code 1 sont conservés. Le script vérifie cette couverture sans se contenter
du booléen `tue` ou d'une seule ligne d'identité fausse.

Conclusion de portée : les lacunes **CST0213 et CST0021 sont comblées pour D**.
La session B reste historique avec ses limites ; elle n'est pas requalifiée.
La clôture générique des pilotes et du lecteur relève de leurs témoins propres,
distincts de cette contre-lecture des campagnes.

## Empreintes, fermeture et reproduction

Les manifests publics contiennent 158 fichiers pour C et 165 pour D ; tous sont
vérifiés. Les **156 et 163 fichiers de résultats publiés** correspondent aux
archives brutes après anonymisation. Les archives de résultats pèsent 243 412 et
273 258 octets ; les paquets sources 1 390 678 et 1 447 501 octets. Leurs hashes
ainsi que ceux des plans JSON/shell concordent avec les reçus. Seuls ces petits
artefacts ont été ouverts en mémoire, sans afficher d'identité de compte.
Les reçus gardés certifient historiquement les arrêts C à 13:22:41 et D à
14:00:29 UTC ; aucune observation actuelle de GCP n'est revendiquée.

Depuis la racine du worktree, historique Git accessible :

```sh
python3 morsehgp3D_v12/receipts/audit_cd_corrections_20261007/campagnes/check_campaigns.py
python3 -O morsehgp3D_v12/receipts/audit_cd_corrections_20261007/campagnes/check_campaigns.py
```

Sans archives locales, ajouter `--published-only`. Les statistiques et preuves
publiques sont rejouées ; le résultat indique explicitement l'absence de
contre-lecture brute et le nombre de journaux dont l'anonymisation empêche de
reproduire le hash original. Les fichiers consommés doivent égaler le pin et
sont rehachés en fermeture. Aucun contrôle ne dépend de `assert`.
[results.normal.json](results.normal.json) donne calculs et empreintes ;
[verification.json](verification.json) atteste les sorties normal/`-O` identiques,
le mode public seul et les codes de sortie.
