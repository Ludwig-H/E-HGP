# T2d-B — préaudit du pilote et des bras, sans campagne

Prototype `pilote_t2d_b.py` **4785dd76**, manifeste **3b387221**, base Git **902041f66** ; [pins](capture.json). Les sources sont figées hors Git et restent inchangées au dernier relevé. Aucun moteur, build, payload LiDAR ni appel GCP de l’audit.

Les six bras sont reconstruits **en mémoire** depuis leurs objets Git de base : 31 transformations de fichiers, chaque motif unique et chaque SHA avant/après conforme. Les dix fichiers distincts du bras `apres` sont identiques aux sources courantes observées. Cela prouve le découpage annoncé, pas la construction de leurs binaires. Les commandes décisives construisent `mhgp12_tower_probe` CPU et chronomètrent **G seul** ; catalogue CPU construit une fois avant les passes. Profil et FULL avec catalogue GPU sont des informations séparées, construites après le premier verdict. Aucun gain GPU/FULL ne découle de ce juge G.

[check.py](check.py) appelle le vrai `juger(..., verifier_journaux=True)` sur **240 journaux distincts × 10 passes**, générés dans un répertoire temporaire. La forme provient du témoin natif synthétique 8 points déjà publié ; configuration, temps et identités sont synthétiques. Les sommes temporelles du nominal, y compris `reste_ns`, sont cohérentes. Les empreintes ne constituent ici aucun oracle géométrique ni certificat de binaires. Normal/−O donnent les mêmes [résultats](results.json).

| Témoin | Verdict observé |
| --- | --- |
| Cohorte complète, ratios 0,8 | Six leviers adoptés. |
| Mauvais nombre de fils, journal réutilisé, trame absente | Refus. |
| Identité finale différente, brut et résumé réhachés | Rejet. |
| Mur G ramené à 1 ns, diagnostics inchangés et reste recalculé | Adoption malgré un comptage interne plus long. |
| A/A = 1,20 | Adoption, avec contrôle publié hors fenêtre. |

**Borne temporelle G manquante.** `count_ns` est un mur, pas un cumul CPU : `stage.cpp:126–138` encadre admission, allocations et `count_cells` par un `Stopwatch`; `tower_probe.cpp:274–278` encadre l’appel entier `resolve_tower`. Le contrat `tower.hpp:145–152` et l’émetteur `tower_probe.cpp:159–167` donnent la partition `prepare + count + setup + fill + workspace + orders`, incluse dans `wall_ns`. Le lecteur vérifie types/tailles, mais aucune de ces bornes. Le témoin conserve le clamp de l’émetteur `reste=max(0,wall−inside)` et reste adopté. Vérifier `inside<=wall`, puis le reste, fermerait ce témoin. **Ne pas ajouter tables/joins/resolve à orders**, qui les contient déjà. Ces fichiers d’instrumentation ne changent dans aucun bras.

**FULL : mauvais littéral et admission incomplète.** Le producteur épinglé écrit `voie="device"` (`full_probe.cpp:259–263`) ; `lire_full` exige `"appareil"`. La forme complète émise est donc refusée ; changer ce seul mot dans la fixture la fait admettre. Une autre fixture, `"appareil"`, sans configuration, sans libération et avec indice0 booléen, est également admise. Le [patch proposé](full_device_proposed.patch), contrôlé par application sur copie, corrige uniquement le littéral ; le contrôle positif FULL passe ensuite. Il reste nécessaire de relier les paramètres/indices à la commande et d’admettre la séquence et les blocs complets, en réutilisant le lecteur strict du schéma 902. Ces informations n’influencent pas le verdict G actuel.

**A/A est un choix annoncé**, pas une garde promise puis oubliée : la docstring indique qu’il ne change aucun verdict. La fixture 1,20 en démontre la conséquence. Recommandation avant campagne : déclarer un veto si cette dérive invalide l’appariement, plutôt que l’interpréter après les mesures. Aucun écart de vitesse réel démontré ici.

L’ordre des bras mérite aussi un ajustement préalable : pour huit bras, rotation de `t` puis inversion aux tours impairs conserve la parité de position de chaque bras. Sur dix tours, `avant` occupe 0/2/4/6 (4/2/2/2 fois) et `apres` 1/3/5/7 (3/2/2/3 fois). Augmenter seulement le nombre de tours ne corrige pas cette séparation ; annoncer un ordre équilibré avant mesure. A/A occupe les mêmes positions que `apres`, ce qui fournit un diagnostic, sans prouver l’absence de biais ni un gain réel.

Les préfixes ng00/uniformes correspondent aux portes `tests/tower/tests.cmake:50–60`; l’identité entre processus porte le SHA complet. `resolution_digest` exclut l’en-tête contenant le nom de trame (`tower_export.hpp:145–150`). Le digest n’est émis qu’à la dernière passe : identité finale seulement, pas preuve d’identité de chaque passe chaude.

```sh
python check.py --sources "$T2DB_SOURCE_SNAPSHOT" --repo "$E_HGP_REPO" --check
python -O check.py --sources "$T2DB_SOURCE_SNAPSHOT" --repo "$E_HGP_REPO" --check
```

Sources/manifestes externes obligatoires et vérifiés par hash ; aucun de leurs 144 Ko n’est dupliqué ici. Ce reçu concerne l’admission (CST-0018) et les frontières annoncées ; aucune qualification nouvelle ni changement produit.
