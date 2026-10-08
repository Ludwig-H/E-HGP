# CST-0241 : correctif raccordé, portée de la porte Python

8 octobre 2026. Capture de travail au contexte `8dc66863f`, 09:15:08 UTC ; modifications alors
non commises, quatre fichiers épinglés dans `capture.json`. **Livraison e78904c49 depuis vérifiée :
les quatre objets Git sont identiques à cette capture.** Le nouveau `pipeline_run.cpp`, SHA
`f7e077d8…`, est **exactement la postimage** du [correctif proposé](../a_terminaison/README.md) :
valeur du dernier retrait mémorisée, commentaire de sortie individuelle corrigé.

La porte `pipeline_terminaison.py` porte le modèle borné de l'auditeur. Les AST de
`transitions`, `graph`, `components`, `fair_cycles`, `abandoned` sont identiques au modèle publié
en6a8, après seule normalisation des docstrings et du type d'exception. Les six graphes et le
cycle de seize transitions sont conservés. CMake raccorde cette porte Python au source réel ;
le mutant `terminaison_relecture_du_compte` restaure les deux anciennes lignes.

Rejeu normal/−O : code0, six cas et période16 annoncés. Réversion ciblée : code1 dans les deux
modes, sur « retrait qui garde son passage par zéro absent », **avant l'exploration du modèle**.
Ce mutant est donc tué par le contrôle textuel du source ; ce n'est pas une exécution native
de l'entrelacement. La vérification de texte n'est pas un parseur sémantique C++.

Le défaut de source est corrigé dans la capture et le port du modèle est fidèle. La preuve
pour N participants et le [protocole natif déterministe](../a_terminaison_porte/README.md) restent
séparés. Aucun test natif, compilateur, TSan, GPU ou cloud lancé par l'audit. Aucune qualification
générale de la concurrence ou du progrès matériel n'est déduite de cette porte Python.
La campagne FULL M source957 précède ce correctif ; il ne lui est pas attribué.

```sh
python3 -B check.py --repo DEPOT --snapshot CAPTURE
# Lors de la livraison, mêmes quatre objets Git :
python3 -B check.py --repo DEPOT --source-pin COMMIT
```

Le lecteur vérifie les quatre pins et la postimage, le port et les raccords, puis rejoue les
quatre commandes Python sur une extraction temporaire. `results.json` en est le résultat.
La capture externe ne contient que sources/tests et métadonnées publiques.
