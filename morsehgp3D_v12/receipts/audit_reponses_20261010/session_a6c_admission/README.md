# A6c : admission indépendante de la session du 10 octobre

Source mesurée `aa6338ee8`, base `8a0716e74`, publication `9ffe6bd17`.
Relecture Python uniquement : aucun moteur, compilateur, test natif, appel cloud ou
payload de coordonnées/IDs/forêt. Le manifeste de la cohorte est lu séparément des données.

**Verdict rejoué : adopté selon la règle annoncée.** Les 85 processus, 1 306 passes
FULL, 100 passes d'identité et 783 passes chaudes décisives sont admis depuis les
JSONL, codes entiers 0 et 85 stderr vides. Les huit statistiques, empreintes FUL1 et
le verdict du juge sont reproduits exactement, sans différence flottante. Le juge
du produit est rejoué ; les ratios, médianes, bootstrap et identité sont aussi
recalculés par `admit.py`, adaptation explicite du lecteur A6b publié.

| Cohorte | Après / avant | IC 95 % | A/A, moyenne géométrique |
| --- | ---: | --- | ---: |
| 21 grandes, 6 tours | 0,854247822 | [0,852199867 ; 0,855715755] | 1,003789300 |
| ng00, 5 tours | 1,002550017 | [0,998825201 ; 1,006495364] | 0,999795096 |
| ng01, 5 tours | 0,997879186 | [0,990886563 ; 1,004079924] | 0,989579278 |
| ng02, 5 tours | 0,960761120 | [0,958611929 ; 0,962897313] | 0,995974458 |

Le veto A/A porte sur sa moyenne géométrique, pas sur son IC : ng01 passe la règle
même si sa borne basse vaut 0,983173292. Aucun changement de seuil ni retrait de passe.
Pour les grandes, une observation bootstrap est la moyenne des logarithmes des
rapports de 21 trames d'un tour, donc **six observations**, pas 126 indépendantes.

Médianes des médianes de processus, ng00/01/02, avant → après (ms) :
79,858566 → 79,996996 ; 66,331878 → 66,011837 ; 82,759538 → 79,474557.
Pour les 21 grandes, la médiane des médianes par trame est **152,435198 →
132,334109 ms** ; maximum des médianes 286,277215 → 241,210887 ms ; maximum
des 126 passes après 242,109925 ms, toutes au-dessus de 100 ms. Le nombre
142,0 → 117,9 ms du reçu développeur concerne sa trame nommée `00/003624`,
et ne représente pas la médiane de cette cohorte de 21 grandes.

**Clôture.** Archive de résultats 562 894 octets, 235 membres manifestés ;
quatre commandes closes, source/paquet/plan et arrêt RUNNING → TERMINATED liés.
Les 487 fichiers de sources de la base et les 495 du candidat (src, tests, bench,
cmake, microbancs, CMakeLists) sont exactement ceux des deux commits, inventaires
complets compris. Journaux publics identiques à ceux de l'archive, champs
numériques du rapport public concordants. Release/u21/CUDA, mêmes options des
deux bras ; hashes des sondes enregistrés avant et après toute la campagne,
A/A même hash. Aucun ELF physique n'est retourné : cette clôture porte sur les
enregistrements du pilote, pas sur une nouvelle construction indépendante.

**Portes.** 755 CTests rapides passés, aucun sauté ; 7 LiDAR passés ; une campagne
CTest de mutants passée. Son manifeste contient 72 mutants et un plancher 72,
mais les rapports individuels ne sont pas retournés. Leur issue individuelle est
donc inférée conditionnellement du lanceur épinglé ; aucune cause de mort précise
n'est requalifiée. CUDA n'est pas transmis aux copies des mutants (défaut CPU).
Les 13 passages TSan locaux cités par le développeur ne sont pas établis par cette
archive. Ces résultats ne ferment pas les constats sur la réserve inutile et la
couverture des pénuries de la chaîne active.

**Portée.** Le seuil 43 900 a été calibré sur les mêmes trames dans a6b/a6b2 ;
la nouvelle campagne ne fournit pas une validation sur des scènes indépendantes.
Les 32 activations sur 40 entrées sont un diagnostic calculé par le pilote à partir
des tailles, pas une trace d'activation native. Parmi les trames de 43 900 à 60 000,
seule ng02 est jugée ici en performance. Le reste des 37 reçoit une comparaison
FUL1 avant/après, sans oracle v11 externe dans cette campagne. Pas de contrat
global acquis, ni de transfert de ces temps à CPU, K10, massif ou autres profils.
Les temps contractuels ultérieurs de la session O sont une mesure distincte.

Rejeu normal ou `-O` (sources et résultats locaux clos requis) :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261010/session_a6c_admission/check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261010.a6c
```

`capture.json` ne conserve que pins, paramètres, comptes et états, sans identité de
compte ou cible cloud. `results.json` conserve les statistiques sans données brutes.
