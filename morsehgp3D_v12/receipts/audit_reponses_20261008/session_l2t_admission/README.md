# L2t : 9,11 millions de sites calculés sur G4, une passe froide

8 octobre 2026, Codex. Admission des métadonnées, sources et journaux de la session
`v12.20261008.mesb2t`, source `8b9eab40a902e0322314ac059a3d0c24d76c46a9`,
publiée par `94fa3a53b`. Aucun moteur, compilation, test natif, GCP, contrôleur,
coordonnée ni identifiant de point lu ou exécuté par cet audit.

**Résultat confirmé : FULL K1..5 sur IGN Paris sans sol, 9 111 422 sites,
46,453149620 s à froid.** Catalogue GPU, tour recouverte CPU W48, u21,
budgets distincts hôte 160 Gio/appareil 88 Gio, cache hôte 8 Gio par défaut
épinglé. Cela dépasse le précédent maximum GPU admis de Marseille brut
(6 709 045 sites). Ce n'est ni une mesure chaude ni le contrat de 100 ms.

| Scène entière déclarée | Sites | Résultat | Temps FULL froid |
| --- | ---: | --- | ---: |
| Paris sans sol | 9 111 422 | succès | 46,453149620 s |
| Paris brut | 14 551 520 | `memory_budget`, passe 0 | absent |
| Lyon sans sol | 24 016 862 | `memory_budget`, passe 0 | absent |
| Lyon brut | 32 412 887 | `memory_budget`, passe 0 | absent |

Quatre processus, quatre passes demandées, **une FULL et zéro passe chaude**.
Les trois refus publient uniquement `open ok` puis `exit
resource_exhausted/memory_budget`, code 2 déclaré par le pilote. Aucun refus
n'est supprimé de la cohorte. Les codes par processus sont ceux du rapport
épinglé, sans capture indépendante supplémentaire ; la commande englobante
se clôture avec code 0. ETH3D courtyard figure dans le jeu déclaré historique,
mais est explicitement absente du plan L2t : aucune nouvelle mesure sur cette
scène. Aucune passe CPU ni K10 dans L2t.

Sur Paris sans sol : P **0,657233896 s**, C **13,852636412 s** (transferts
**2,433765969 s** inclus), G **14,256867596 s**, queue après G
**17,366315726 s**. La queue est le plus grand de ces postes muraux disjoints ;
les fenêtres de travail parallèles ne s'ajoutent pas au mur. Validation
**20,855674531 s** et libération **1,953213473 s** sont hors FULL ; le processus
est rapporté à 70,0 s. Ni ces coûts ni ouverture/lecture ne doivent être
confondus avec les 46,453 s contractuelles.

Critères rejugés : **B1 non tenu** (5,098342456 s par million, seuil 2 s),
**B2 tenu uniquement sur l'unique scène jouée sous 10 millions**, B3 et B4 non
évalués. Les refus au-dessus de 10 millions restent tolérés par B1 tel qu'écrit ;
cela ne transforme pas ces scènes en succès. Aucun FUL1 n'est demandé : les
quatre scènes dépassent le seuil déclaré de 1,6 million, `empreintes={}`.
La session n'ajoute donc aucune identité différentielle massive.

## Mémoire : ne pas convertir nvidia-smi en budget

Pour Paris sans sol réussi, le journal distingue :

- pic comptable appareil **48 322 934 812 octets**, soit **45,004 Gio** ;
- capacité conservée **109 341 160 octets**, exactement `12n+4096`, compatible
  avec x/y/z/fault seuls après la finition par tranches ;
- maximum nvidia-smi échantillonné **46 675 Mio**, soit **45,581 Gio** ;
- pic actif du budget hôte **88 150 866 270 octets**, soit **82,097 Gio** ;
- RSS maximal **89 989 001 216 octets** ; transit épinglé **67 108 864 octets**.

Pour les trois refus, les nombres **47 987 / 67 365 / 72 847 Mio** proviennent
uniquement de nvidia-smi. **Aucun pic budgétaire, usage hôte ni stade de refus
n'est publié.** Le README développeur les nomme à tort pics de l'appareil et
conclut que « c'est donc le budget de l'hôte ». Cette causalité reste ouverte :
`CudaExecutor::grow` réserve avant `cudaMalloc`. Une réserve trop grande peut
échouer bien avant que l'occupation physique ne s'approche de la limite ; le
pic échantillonné peut aussi manquer un maximum bref. Ni l'origine hôte ni
l'origine appareil n'est éliminée par ces journaux.

Le pic hôte de Paris réussi représente environ **9 675 octets par site**.
Ce quotient d'un seul succès ne borne pas la mémoire des scènes refusées.
[documentation.patch](documentation.patch) corrige les colonnes, la causalité
et la comparaison temporelle ; application et inversion rejouées en copie.
Le diagnostic distinct proposé dans
[le reçu TU Wien](../b1t_tuwien_memoire/README.md) reste pertinent pour nommer
le budget, le stade et la demande refusée.

Le L2 historique avait calculé Paris sans sol en 112,8658 s et Paris brut en
165,7890 s sur CPU. Le rapport descriptif d'environ ×2,43 pour Paris sans sol
change backend, produit et protocole (ancien séquentiel contre recouvert,
cache et optimisations intervenues). Ce n'est pas l'effet causal du seul T1-d.
Le refus GPU de Paris brut constitue une différence de capacité à diagnostiquer,
sans preuve d'une régression du même chemin CPU.

## Provenance et portée du rejeu

Archive **13 138 octets**, SHA-256
`49b967e27f2ef7da8c727fedd2d81dff86dae42b009c51eb0ae06796b89889dd` ;
**44 entrées** du manifeste vérifiées. Les huit fichiers JSONL/stderr sont
identiques à ceux publiés, tous les stderr vides. Les valeurs numériques du
rapport publié et du rapport original sont identiques. Worker, commande et
DONE valent 0 ; résultats vérifiés, erreurs vides ; arrêt ciblé certifié,
code 0 en une tentative, état `RUNNING` puis `TERMINATED`. Aucun appel réseau
n'a été fait pour cet audit et aucune relecture cloud indépendante n'est
ajoutée à la certification archivée.

Le paquet source est intégralement raccordé au Git : **369 fichiers**
src/bench/tests/CMake/lecteur FULL, plus pilote MES-B et son helper. `src/` et
la sonde FULL sont identiques octet pour octet à **R1 `47feedc96`**. Release,
u21, CUDA ON sont déclarés ; hash ELF initial
`b1a18fd52562c244134ba9db63c7500aa28436679e047ae8cb2e6eb55fba6357`.
**Pas d'empreinte ELF finale archivée**, ni nouvelles portes natives dans ce
plan. GPU sans application listée aux deux extrémités, sans preuve de solitude
pendant tout l'intervalle.

Le manifeste des données n'existe plus au chemin externe du préflight.
L'audit ne prétend pas l'avoir relu : son SHA `fa7d1544…` et les sites sont
raccordés à la capture L2 publique déjà admise ; **les 17 déclarations de
fichiers de L2t sont exactement celles de L2**, tailles et SHA compris. Les
empreintes originales restent des déclarations de provenance, pas une nouvelle
lecture ou qualification des coordonnées.

Le lecteur reprend explicitement l'enveloppe indépendante L1r (commande 0,
schéma recouvert, pin du pilote), puis applique le lecteur FULL épinglé du
produit. Cohorte extérieure, séquences, refus, mémoire, statistiques et critères
sont rejugés ; un second passage du lecteur FULL doit reproduire le rapport.
Le décodeur strict de l'enveloppe rejette clés dupliquées, nombres non finis
et types incohérents. Rejeu normal et `-O` identiques :

```sh
python3 -B -S check.py --repo DEPOT --session SESSION_L2T --snapshot CAPTURE_EXTERNE
python3 -B -O -S check.py --repo DEPOT --session SESSION_L2T --snapshot CAPTURE_EXTERNE
```

`capture.json` épingle les sources, helpers, données déclarées et fermeture ;
`results.json` conserve uniquement les métadonnées utiles, dont la FULL complète.
