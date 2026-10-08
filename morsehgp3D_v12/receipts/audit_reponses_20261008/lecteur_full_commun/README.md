# Lecteur FULL commun livré : progrès vérifiés et limites restantes

Commit **a2c2fccfd65f89a4bb874b8bf5e1bd2c737856b1**, sources Git dans [pins.json](pins.json).
MES-FULL et MES-B appellent maintenant le même `microbancs/outils/lecteur_full.py`.
Aucune mesure passée n'hérite de ce lecteur : L1 est antérieure au schéma mémoire,
L1R utilise encore le pilote 457d. Aucun moteur, compilation ou GCP par cet audit.

Les contrôles de ligne FULL sont effectivement partagés : clés exactes, entiers u64
sans booléens, ordre open/full/libération/exit, paramètres et sites par trame, index
modulo le nombre de trames, budget appareil séparé ou commun demandé, durée positive,
partition dans le mur et mémoire par étage. L'identité de toutes les passes FULL reste
jugée par MES-FULL. Ce pilote refuse maintenant un environnement incomplet ou un GPU
non connu vide aux deux instantanés ; il publie hashes sonde/pilote/lecteur, extrait
CMakeCache, journal de compilation et temps CPU médian. Cela améliore les futures preuves,
sans inventer les journaux absents de K ni une isolation continue du GPU.

**Portes rejouées sur les octets Git.** Trois portes officielles, chacune en normal et −O,
passent avec sorties identiques. Leurs sondes sont de petits programmes Python fictifs ;
les fichiers d'entrée sont fabriqués par les tests. Quelques commandes lisent seulement
l'environnement local. Aucun moteur HGP n'est lancé.

Les deux runners officiels sont observés en conservant leurs substitutions et arguments :
**16 mutants du lecteur + 12 mutants de MES-B**, tous valides syntaxiquement et tous code1.
Vingt-sept produisent un diagnostic de porte ; `pente_sans_garde` produit causalement
`ValueError: math domain error` sur log(0). Aucun échec d'import ou de syntaxe ne remplace
une mort causale. Ce sont 28 mutants distribués autrement après factorisation, pas 28
nouveaux défauts distincts. [Résultats des portes](gates_results.json), [pins](gates_pins.json).

**Contre-exemples ciblés encore admis.** Le [rejeu indépendant](check.py) emploie la fixture
synthétique officielle, pas des mesures. Il confirme :

- les trois cohérences mémoire déjà signalées dans [MES-B](../mes_b_memoire/README.md)
  restent absentes : capacité appareil supérieure à son pic séparé, épinglé supérieur au
  pic hôte, pic de G inférieur à l'usage en fin de C. Une libération valide C=4 puis G=1/pic4
  reste, correctement, admise ; ne pas exiger des usages monotones ;
- `code=False` et `code=0.0` sont assimilés à zéro, `code=2.0` à un refus. Les paramètres
  transmis par subprocess sont normalement des entiers : le témoin porte sur le contrat
  du lecteur et ses usages de relecture, pas sur un processus réel observé ;
- un refus avec raison inconnue, ou `unsupported_degeneracy/memory_budget`, est admis comme
  résultat. Le second couple contredit `reasons.def`, où memory_budget relève de
  resource_exhausted. Fermer les couples statut/raison avant d'admettre le refus.

Ces défauts ne réfutent aucune des mesures L1/L1R. Les champs `cpu_ns`/`rss_max_octets`
présents mais null rendent volontairement la lecture illisible : un contrôle manque,
aucun zéro n'est substitué. Le reçu ne demande pas de transformer une mesure absente
contre son gré en temps nul. Le type exact du code et les trois gardes mémoire sont des
correctifs indépendants du partage de lecteur. Un [patch partiel](coherence_proposed.patch) les propose
dans le module commun : six contre-JSON deviennent illisibles, nominal et libération valide restent admis.
Application testée sur copie ; les couples statut/raison restent à fermer, aucun produit modifié.

[results.json](results.json) est identique en normal et −O ; les sources sont extraites
et vérifiées depuis leurs objets Git, sans modification produit. `replay_gates.py` est
l'adaptation à un chemin explicite du conducteur de contrelecture des portes ; il dépend
de l'extraction extérieure épinglée dans `gates_pins.json` (sources non dupliquées ici).

```sh
python3 -B check.py --repo /workspaces/E-HGP
python3 -B -O check.py --repo /workspaces/E-HGP
python3 -B replay_gates.py --evidence /chemin/extraction_git
```
