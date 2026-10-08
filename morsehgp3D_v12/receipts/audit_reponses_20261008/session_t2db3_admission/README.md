# B3 — contrelecture de la session close

8 octobre 2026, `exploration_v12_hors_registre`, u21, catalogue CUDA G4 et tour CPU,
`public_status=not_claimed`. **Les trois leviers B3 sont rejetés par la règle publiée ;
A/A est valide.** Cette lecture ferme les statistiques et les primaires disponibles.
Elle ne transforme pas les preuves d’identité incomplètes du pilote v1 en admission
stricte du pilote v2 proposé.

`protocol.py` compare le paquet à Git `545ed987e0f5a06dbb518fe42ed6c5f100f32770`
(490 fichiers sélectionnés) et l’archive de référence à
`8a0716e7470197c95953b38d79f26b8d8f2379fc` (487). Il reconstruit les quatre bras
par substitutions uniques épinglées ; `apres` est exactement le produit livré B3.
L’archive des résultats contient 999 445 octets, 457 fichiers manifestés, aucune
entrée surnuméraire. Worker 0, trois commandes 0, aucune erreur/alerte du reçu,
arrêt ciblé certifié de `RUNNING` à `TERMINATED`. Aucun contrôleur n’est exécuté
par ce lecteur.

Les 387 journaux natifs sont tous relus : 373 processus FULL / 2 816 passes,
10 résolutions G et quatre profils / 16 passes G. Le jugement porte sur cinq
trames, six bras, dix processus par bras et huit passes par processus :
300 processus, 2 400 passes FULL dont **2 100 chaudes**. Les rapports appariés
utilisent la médiane des sept passes chaudes de chaque processus. Les deux
écarts entre ce recalcul Python et les nombres enregistrés par le worker sont
exactement d’un ULP, listés dans `results.json`, sans effet sur un seuil.
Aucun tour ni point extrême n’est retiré.

| Levier | Trames qui empêchent l’adoption | Bornes supérieures IC95 du rapport |
|---|---|---|
| Lot B3 | ng01, ng02 | 1,046561 ; 1,017223 |
| Clés | ng02 | 1,012667 |
| Balayage | ng00, ng01, 64 740 sites | 1,000937 ; 1,000225 ; 1,001474 |

La règle exige une borne supérieure strictement inférieure à 1 sur chacune des
cinq trames. Les cinq moyennes géométriques A/A restent entre 0,998693 et
1,000782. Par exemple, sur ng01 la médiane des dix médianes diminue, mais le
lot subit un tour de rapport 1,184673 ; conserver ce tour explique pourquoi
la seule médiane ne suffit pas à annoncer une adoption.

| FULL K5, ms — médiane des dix médianes de processus | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Référence R1 | 80,019409 | 66,097462 | 83,379041 |
| Lot B3 rejeté | 78,356173 | 65,045839 | 84,151464 |

Les deux autres trames décisives (64 740 / 67 114 sites) donnent R1
144,843259 / 169,167390 ms et B3 144,095704 / 167,883982 ms. La trame informative
99 099 sites donne 281,305658 → 279,683148 ms (trois processus). K10, informatif,
ne comporte qu’un processus de cinq passes par bras et par trame ; les médianes
des quatre passes chaudes sont 498,193039 → 466,843964, 368,106103 → 350,770963 et
422,932989 → 404,384168 ms. Ces résultats K10 ne remplacent ni la règle K5, ni
une campagne K10 appariée répétée. Les mesures séquentielles et de profil sont
conservées dans `results.json` ; elles ne qualifient pas le contrat FULL.

Limites de preuve explicites :

- Les 25 journaux FUL1 (50 passes) portent code 0 et SHA256, sont relus avec
  configuration attendue et donnent une seule empreinte par trame ; ng00
  retrouve l’empreinte externe R1. Les dix résolutions G donnent une seule
  empreinte par trame après relecture stricte avec le code du correctif
  proposé `d9910490…`. **Leurs codes de retour ne sont pas archivés** : le code
  0 est seulement inféré conditionnellement de `valide=true` et du producteur
  épinglé. Ce correctif n’était pas embarqué dans la session.
- Aucun stderr individuel natif n’est rapatrié. Les stderr des trois commandes
  englobantes sont vides ; cela ne prouve pas les stderr de leurs sous-processus.
  Le [défaut du juge v1](../b3_identite_admission/README.md) demeure ; cette
  contrelecture n’invente ni codes ni stderr absents.
- Les empreintes ELF des six bras FULL sont déclarées identiques avant et
  après la campagne décisive ; les deux bras A/A ont le même ELF. Pour les
  sondes G/profil, il n’existe qu’une empreinte préalable. Il n’y a pas de
  fermeture ELF supplémentaire après les mesures informatives FULL.
- Le socle rapporte **747 Passed, zéro saut**. Les deux CTests de mutants
  catalogue/tour sont Passed (582,36 / 192,33 s), avec manifestes source de
  38 / 57 mutants. Le lanceur configure des copies CPU, CUDA non transmis et
  désactivé par défaut. Les rapports individuels de mutants ne sont pas
  rapatriés : succès agrégé du lanceur, aucune cause individuelle « par code »
  contrevérifiée ici.

Rejeux normal et `-O` passés, uniquement Python, depuis le dépôt :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/session_t2db3_admission/check.py --repo /workspaces/E-HGP --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/t2db3_closed_20261008
python3 -B -O -S morsehgp3D_v12/receipts/audit_reponses_20261008/session_t2db3_admission/check.py --repo /workspaces/E-HGP --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/t2db3_closed_20261008
```

La capture externe conserve les sources, métadonnées de clôture et journaux,
pas les entrées LiDAR. Les données licenciées, identités de compte et noms de
cible ne sont pas copiés dans ce reçu. Aucun moteur natif, compilateur, test
natif ni GCP n’est exécuté par l’auditeur.
