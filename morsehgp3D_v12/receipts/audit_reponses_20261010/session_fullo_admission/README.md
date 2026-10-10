# Session O : admission des preuves FULL et petits nuages

10 octobre 2026. Candidat A6c `aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66`,
B2/T1-d/R1/A6c, avant réintégration B3. Relecture indépendante de métadonnées,
sources et journaux ; aucun moteur, compilation, appel cloud ou payload LiDAR.

**Relecture concordante, provenance conditionnelle.** Les limites de FULL N
restent présentes : les codes et stderr individuels des sondes ne sont pas
archivés. Le zéro utilisé par le lecteur est inféré du producteur épinglé et
du rapport sans refus ; ce n'est pas un code natif retrouvé. Les deux sondes
des pilotes sont hachées avant leurs campagnes seulement. Le manifeste final
du worker couvre d'autres exécutables ; aucun hash final des deux sondes
mesurées n'est fourni.

- 496 fichiers de sources/tests/configuration identiques au commit annoncé ;
  profil Release, u21, CUDA. Archive de résultats **1 916 923 octets**, ses
  156 fichiers manifestés et ses 6 835 187 octets décompressés contrôlés.
- Trois commandes terminées avec code0 et stderr agrégé vide ; **755 CTests
  déclarés réussis**, aucun saut dans ce socle. Cela ne qualifie pas les portes
  de pénurie ON manquantes, ni une campagne mutante qui ne figure pas dans O.
- FULL : **38 processus, 610 passes, 392 chaudes**. Statistiques, identités
  CPU/GPU K5 et répétitions, configuration et verdict recalculés exactement.
  Ng00–02 K5 GPU respectent le critère local ; la cohorte37 ne le respecte pas.
- MES-C : **56 processus, 3 608 FULL achevés, 2 392 passes retenues**, 48 succès
  et huit refus `wide_leaf`, aucun dépassement de délai. Régression OLS et
  critères concordants ; C1/C2/C3 restent non tenus. La cohorte est projetée
  depuis ses métadonnées publiques épinglées, sans nouvelle lecture des données.
- Arrêt de cette session certifié RUNNING→TERMINATED, sans erreur/alerte du
  contrôleur. Cet arrêt historique ne prouve pas l'état d'une session suivante.

Les [statistiques et décompositions indépendantes](../fullo_temps/README.md)
distinguent le maximum brut, la pire médiane, les cohortes et les comparaisons
intercampagnes. La réduction du temps par rapport à N est descriptive ; O/N
ne remplace pas le banc A/A/B A6c pour attribuer le gain.

## Rejeu et adaptation explicite

```sh
python3 -B -S check.py --repo /chemin/depot --session /chemin/v12.20261010.fullo
python3 -O -B -S check.py --repo /chemin/depot --session /chemin/v12.20261010.fullo
```

Le lecteur FULL N est repris depuis le commit `589e3b752`, avec son SHA fixé
dans `capture.json`. Seules cinq constantes AST changent : pin du produit,
empreinte de son inventaire, nombre de sources 488→496, nombre de CTests
747→755 et libellé correspondant. Aucune formule statistique, cohorte,
garde, règle d'admission ou limite de preuve n'est modifiée. Les sources
des pilotes et du lecteur FULL sont prises au nouveau pin ; le lecteur
MES-C indépendant garde son origine et son adaptation au schéma recouvert.

Les entrées privées de contrôle sont épinglées avant/après le rejeu. Le
résultat conserve seulement des métadonnées sans identité ; les tables
temporelles détaillées sont dans le reçu voisin, afin d'éviter leur doublon.
`--record` sert uniquement à la capture initiale, avant fermeture du reçu.
