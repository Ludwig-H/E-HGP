# Copies parallèles et préallocation des pages — pin22

Sources Git `22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4` copiées et hachées ; inventaires avant/après du même pin. La première capture partielle a cherché deux chemins inexistants ; la chronologie est consignée dans BEFORE. Aucun produit modifié, natif, CUDA ou GCP exécuté.

**Raccord favorable en source.** Le ramassage des files concatène par ordinal, même si les callbacks sont réclamés autrement : préfixes séparés jobs/sites, spans disjoints, `job.begin` décalé par le préfixe sites. La génération est jointe avant cette lecture. Les files et le contexte emprunté restent vivants jusqu'au retour synchrone du Pool.

La copie du bloc batch découpe séparément records/population en tranches de 16 384 ; elle modifie seulement le décalage population de chaque émission, puis place le repli dans le suffixe. Le compactage des tâches termine avant celui du batch. Les refus restent dans les buffers privés, avant `Assembly::finish` ; le Pool attend tous ses callbacks même après refus. Aucun croisement de plages constaté.

Les retours CUDA touchent une position distincte par intervalle de 4 096 octets, après allocation et avant `cudaMemcpy`. Le retour synchrone du Pool sépare ces écritures de la copie complète device→host. Aux plafonds admis, le calcul du nombre d'intervalles et leurs adresses ne déborde pas u64. Ce modèle ne prétend ni taille de page OS certifiée ni vitesse NUMA favorable.

**271 contrôles stdlib**, normal/−O identiques : comparaison avec concaténations indépendantes, quatre ordres de réclamation, files vides et 1 024 files, seuils 16 383/16 384/16 385, suffixe de repli, couverture unique de chaque destination et calculs de capacité sans grandes allocations. Les payloads sont synthétiques, sans boules ni coordonnées géométriques ; pas de preuve de course native ou d'accélération matérielle.

Une première version du contrôle a comparé une tranche `list` à une ligne `tuple` et a refusé malgré contenus égaux. Les deux sorties en échec et le script sont préservés sous `attempts/attempt1/`. La conversion du type dans le juge corrige uniquement ce test ; aucun changement du transport modélisé ou du produit.

```sh
python3 -B -S check.py
python3 -B -O -S check.py
```

`SHA256SUMS` inventorie tous les fichiers réguliers sauf lui-même.
