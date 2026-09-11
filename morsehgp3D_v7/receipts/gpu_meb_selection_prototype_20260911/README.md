# Prototype privé de sélection MEB destiné à une couture GPU

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce paquet conserve une première primitive privée, non intégrée au chemin actif : un thread prévu par facette, recherche q2/q3/q4 dans l'ordre de `anchor_meb`, sortie POD support/coque/statut/compteurs et matérialisation hôte. Le kernel a été compilé et lié, jamais exécuté. Aucun wrapper device, terminal statique, hiérarchie FULL ou tour n'est qualifié ici. GCP non utilisé ; aucune mesure de performance ou de RSS, aucun gain extrapolé.

## Résultats bornés

- `o2_r1` : l'oracle passe ; compilation du test de transport échouée (accolade de namespace manquante), sources et diagnostic originaux conservés.
- `o2_r2` : O2 strict passe. 605 comparaisons au CPU exact et à l'oracle Gram rationnel, 16 592 checks, 300 permutations, 197 extra-shell ; supports q1/q2/q3/q4 = 82/393/110/20. Transport : 253 checks, 33 rejets, 4 flags causaux détectés.
- `nvcc_r1` : CUDA 12.9.86 compile et lie le vrai kernel pour `sm_120` avec les diagnostics hôte stricts, sans diagnostic stderr et sans exécution du binaire. L'adaptateur de phase hôte déjà qualifié est conservé dans les sources.
- `san_r1` : compilation ASan/UBSan réussie ; exécution code 1, diagnostic fatal LeakSanitizer/ptrace et stdout vide. Le test de transport n'a pas été compilé. Aucun replay hors sandbox, aucune désactivation des sanitizers, aucun succès SAN revendiqué.

La seule modification de primitive existante est l'annotation HD de `q4_center_strictly_inside`, formule inchangée. Le CPU `anchor_meb` du snapshot `ad7ffd28b35e153a20bd8cf42534d1cd29160bcd` reste inchangé. Les originaux, sources annotées, juges et patches sont inclus ; aucun ELF, toolkit ou vendor n'est distribué.

## Limites contractuelles importantes

Le validateur runtime `materialize` vérifie le support positif sélectionné, le confinement et la cohérence du transport, puis produit la MEB canonique sur l'hôte. Il ne prouve pas que le préfixe de recherche a été exécuté : un support positif ultérieur peut définir la même boule. Le test du carré accepte cette MEB dans le validateur, mais la comparaison des positions du support et des compteurs détecte l'écart au premier support de référence. Les égalités d'ordre et de compteurs sont donc des résultats de juges différentiels sur les cas capturés, pas une preuve d'exécution délivrée par le seul POD.

Les puissances supplémentaires de validation hôte sont comptées séparément. `selection_work` n'est rempli que sur succès ; le POD brut conserve le travail de sélection déclaré en échec. Il n'existe pas encore de propriétaire de buffers device, d'ABI wire stable, d'agrégation vérifiée des compteurs, d'autorité d'index résident ou de transaction de lot FULL. Les vérifications de lot vide/null et de sur-lancement sont exécutées par le stub hôte ; aucun comportement device exécuté n'en est déduit.

Le document détaillé [PROTOTYPE.md](PROTOTYPE.md) conserve la note privée et son plan de suite avant G4. Ses chemins `build/` désignent les lieux originaux de l'expérience, pas des dépendances présentes dans ce paquet.

## Lecture portable des preuves

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/gpu_meb_selection_prototype_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/gpu_meb_selection_prototype_20260911/verify.py
```

Le lecteur contrôle le manifeste public, reconstruit dans un répertoire temporaire les 381 fichiers textuels du manifeste privé inchangé, puis lance son lecteur normal ou `-O`. Il ne compile rien, n'exécute aucun ELF et ne touche pas GCP. Les copies répétées sont dédupliquées dans `objects/` ; `storage_map.json` conserve tous leurs chemins logiques, tailles et hashes. Les quatre captures ont leurs reçus, logs bruts et fichiers de dépendances lisibles dans `captures/`. Le manifeste privé original est [capture_manifest.json](capture_manifest.json). Les fichiers sous `sources/` sont les sources exactes retenues par les captures finales ; les patches montrent séparément l'annotation q4 et l'adaptation du juge.

Une recompilation nécessite de restaurer les chemins logiques du snapshot dans un nouveau répertoire de travail et de fournir les dépendances externes listées par les commandes et fichiers `.d` : Boost pour le juge, CUDA 12.9 pour la compilation device. Le paquet est portable pour la vérification des preuves textuelles, pas un environnement de compilation hermétique. Les durées présentes dans les reçus sont celles des commandes de qualification, pas des benchmarks du composant.
