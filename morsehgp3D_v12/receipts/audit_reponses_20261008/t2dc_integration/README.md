# T2-d-C livré : cohorte commune et comparaison des sources

Lecture du commit `02b735d6b`, 8 octobre 2026, puis du pool livré `5b3362bbd`. Inventaires Git, Python synthétique
et application de patch en copie temporaire seulement : aucun moteur, compilation, GPU, GCP ou jeu de données.
La session T2-d-C signalée par le coordinateur est épinglée à **02b735d6b**, donc **sans le pool 5b**. Sa provenance
et ses mesures seront admises séparément ; ce reçu ne les invalide pas.

## Défaut confirmé dans la publication

Le juge attend 10 tours × 3 trames × 7 bras, mais ignore les prises étrangères dans `campaign_table`. Le tableau
relit toutes les prises du même couple trame/bras, sans vérifier leur tour. Ajouter onze prises synthétiques
`ng00/apres`, tours 10..20 au lieu de 0..9, laisse **tout le jugement identique, « adopte »**, mais fait passer la
médiane publiée de **28,097 ms à 0,001 ms**. Un seul processus étranger illisible est également ignoré. Ce sont des
contre-JSON dérivés du positif officiel, pas des résultats GPU observés ; ce positif est une fixture du juge,
pas une trace native ni une qualification de ses valeurs géométriques.

`cohorte.patch` propose une fonction commune `closed_campaign` : ensemble exact des triples attendus, types
stricts, aucune observation étrangère, aucun doublon, aucune prise manquante. Le juge l'appelle avant toute
agrégation de campagne et rend **refuse**. Les tableaux utilisent la même fonction ; même une ancienne décision
`adopte` stockée dans le rapport ne peut plus publier de médiane pour cette cohorte invalide. Un nominal et huit
négatifs passent : vide, manque, doublon, tour/trame/bras étranger, tour booléen et tour non hachable. Le patch a
été appliqué et vérifié uniquement dans une copie temporaire. Il ne ferme pas les autres étapes d'identité ou
les informations facultatives ; il ne vérifie pas les positions temporelles de lancement des bras.

## Ne pas confondre les leviers C et le nouveau pool

L'inventaire complet `src/` est fermé par chemin relatif et SHA dans le lecteur et les résultats :

- **902 → 02** : douze fichiers du catalogue changent, plus `io/io.hpp` et `io/sha256.cpp` (SHA matériel).
  Aucun changement du pool. Les empreintes sont hors du mur C ; nous n'attribuons aucun effet temporel mesuré à
  cette différence, mais elle empêche de déclarer les sources strictement identiques hors C.
- **902 → 5b** : les mêmes différences plus `sched/pool.cpp`, `sched/sched.hpp`, `sched/source_pins.json`.
  Le pilote construit `apres` directement depuis `--src`, et `avant` depuis l'archive 902. Si le prochain paquet
  emploie 5b, le rapport du lot et le levier « flux » compareront aussi deux pools. Les ablations construites
  depuis le même `--src` conservent leur pool commun. Aucun nouveau G4 sur 5b n'est déduit de cette observation.

Proposition concrète avant prochaine campagne : fixer **un seul pin P** pour les deux arbres, fabriquer
`avant_C` en inversant uniquement le delta natif C `02^ → 02` sur une copie de P, puis vérifier que tous les
fichiers produit hors catalogue sont identiques. `capture.json:recipe_allowlist` ferme les 20 chemins : douze
produit catalogue, la sonde du catalogue et sept fichiers de portes/manifeste. Garder le même pool, SHA,
compilateur et options. La sonde doit également revenir à son corps antérieur : le corps récent lit les trois
champs diagnostics introduits par C et ne compile pas tel quel contre le header antérieur. La différence de
sonde est déclarée ; `avant` n'émet pas la ligne `sorties`. Fermer également les inventaires des outils de mesure.

`check.py` produit le delta Git exact sur cette liste, exécute `git apply --reverse --check` puis l'application
sur une extraction limitée de **P=5b**, et compare chaque postimage à `02^`. Le catalogue reconstruit égale celui
de 902 ; chaque fichier produit hors catalogue reste exactement celui de 5b. Les SHA d'arbres et du patch sont
publiés dans `results.json`. **Pas de compilation de cette composition** : une qualification native/GPU est
encore nécessaire. Pour un futur P modifiant C, toute application conflictuelle exige une nouvelle composition
explicite, jamais une qualification héritée.

## Portes et résidus du lecteur

Les **39 injections officielles**, dont 25 d'admission, passent en Python normal et `-O`. La fenêtre A/A ±1,5 %
est maintenant bloquante et les décisions emploient les bornes non arrondies : les trois témoins officiels A/A
1,01 / 1,02 et borne 0,99996 les couvrent. Cela n'est pas une précision statistique garantie par la seule réussite
A/A. Les noms actuels déclarent correctement le repli sélectif conservé ; `repli_cles_entieres` n'est pas l'ancien
rapatriement complet une fois.

Le lecteur reste **dae75c32**, identique à celui du [suivi antérieur](../t2d_c_suivi/README.md). Rejeux ciblés,
sans nouveau constat géométrique : une passe de mutant déclarée CPU pour une commande appareil, ou
`invariant_violated/memory_budget`, est encore comptée « mutant tué » et laisse « adopte » ; `read_full` accepte
encore `code=False` et `liberation.pass=False` au rang zéro. Correctifs séparés à prévoir : mêmes gardes de
métadonnées pour les passes en échec que pour les succès, correspondance raison/statut depuis `reasons.def`,
types entiers exacts pour code et indice. Une comparaison absente/malformée doit être **refusée**, pas compter
comme mutant tué. Le patch de cohorte ne prétend pas corriger ces résidus.

## Rejeu

```sh
python check.py /chemin/du/depot > /tmp/t2dc-normal.json
python -O check.py /chemin/du/depot > /tmp/t2dc-opt.json
cmp /tmp/t2dc-normal.json /tmp/t2dc-opt.json
cmp /tmp/t2dc-normal.json results.json
```

Les octets Git épinglés sont requis ; aucune dépendance aux anciennes copies temporaires. `capture.json` conserve
les pins et dates réelles de capture. `SHA256SUMS` couvre les fichiers du reçu. Aucun état du registre n'est changé.
