# Porte autonome proposée : structure couvrante de MHGP11SPv2

Complément à la capture `../formats/`, lecteur WIP inchangé (`9eee2ed4bcef1e960cdf2456012b84416854dc20` comme base, SHA `4d05bf53fb272bc2101bdd8069bab86ce28be6fa2451d852d33a6403a3248896`). La décision de publier S* des supports associés au MST est conservée. Aucun fichier développeur modifié.

Le lecteur actuel accepte trois fichiers v2 construits en stdlib et leurs dossiers avec empreintes/comptes/signature recoupés :

- triangle entier équidistant `(0,0,0), (1,1,0), (1,0,1)`, K1, deux supports AB/AC : valide ;
- même triangle, trois supports AB/AC/BC au même plateau : cycle conservé ;
- carré de quatre sites, racine à quatre enfants, deux paires disjointes : les branches couvrent les enfants mais ne les connectent pas.

`_check_roles` (:808–814 et :830–831) vérifie l'inclusion et la réunion des branches, ce qui permet les deux cas incorrects. Les portes d'échelle `tests/cli/cli_supports_scale.py:96–136` relisent cette structure, puis contrôlent invariances et signature commune FULL ; ces contrôles ne sélectionnent pas une DSU indépendante. La nouvelle porte ne remplace pas le différentiel Python du constructeur, confié au juge exact.

Le patch séparé ajoute seulement pour v2 une DSU locale sur les enfants de chaque fusion, en ordre publié des boules. Chaque boule conservée doit réussir au moins une union ; tous les enfants doivent finir connexes. Une hyper-arête peut réussir une union et contenir aussi des liens redondants : aucun refus de toute redondance intra-boule, aucune équation imposée entre nombres de supports et d'arêtes. Le rejeu appelle aussi le helper sur deux hyper-arêtes reliant quatre enfants avec un lien redondant ; elles sont admises. Les fichiers v1 et la politique des versions du manifeste ne sont pas modifiés.

La source proposée est dérivée exactement du snapshot et le patch vérifié octet pour octet. Normal/−O : triangle à deux supports accepté ; cycle et composantes disjointes refusés par les nouvelles gardes, à la lecture binaire comme par `check_directory`.

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

`supports_spanning_reader_gate.py` est une porte indépendante du CLI, utilisable après intégration du contrôle dans le lecteur :

```sh
python3 -B supports_spanning_reader_gate.py --bench /chemin/morsehgp3D_v11/bench
```

Raccord proposé après copie sous `tests/cli/` :

```cmake
mhgp11_python_gate(mhgp11_cli_supports_spanning_reader 0 supports_spanning_reader_gate.py
                    --bench ${PROJECT_SOURCE_DIR}/bench
                    LINE "supports_spanning_reader_verdict conforme cas3" LABELS fast oracle TIMEOUT 30)
```

Le helper CMake existant crée aussi la jumelle `_opt`. Aucun build ni exécution native/cloud/LiDAR dans cette preuve. Le snapshot adjacent est requis. Le contrôle vérifie la structure couvrante des branches publiées ; il ne reconstruit ni les candidats omis ni le choix canonique Kruskal global. La qualification native du constructeur et son différentiel restent séparés. Les anciennes captures restent immuables.
