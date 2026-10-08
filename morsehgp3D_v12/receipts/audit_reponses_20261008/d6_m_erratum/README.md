# D6 M : correction de deux interprétations documentaires

Audit Codex du 8 octobre 2026, documentaire seulement, sans nouveau constat.
Le [patch proposé](proposition.patch) vise le README de la session M au pin
`72f622a556130e25afdadbef84984619417286e2`, lignes 83–95. Il ne modifie ni
les bruts, ni leurs temps, ni les verdicts. Il n'est appliqué qu'en copie.

1. Les crochets du tableau D6 sont **le minimum et le maximum des trois
   rapports par tour**, pas un intervalle de confiance. `ratios()` au source
   exécuté `957e9784…` utilise explicitement `min(values)` et `max(values)` ;
   ce pilote n'y calcule aucun IC.
2. `dilate()` multiplie exactement chaque entier par le facteur. La présence
   du couple `(21,8)` exige `8m < 2^21`, donc le maximum initial vérifie
   `m ≤ 2^18−1`. À ×2048, le maximum est au plus **536 868 864 < 2^29**.
   Le banc n'utilise donc pas des coordonnées remplissant 32 bits. Changer
   l'unité d'encodage des entiers déjà quantifiés n'ajoute aucune information
   physique. La déduction utilise le pilote et la cohorte admise, sans lecture
   des coordonnées. C'est le même point que l'[erratum K](../../g4_fullk_20261008/ERRATUM_20261008.md)
   et sa [contrelecture D6](../d6_session_k/README.md).

La [contrelecture M](../session_m_d6/README.md) a été rejouée ici en Python
normal et `-O` : sorties identiques à ses résultats, **126 journaux / 630
passes / 504 chaudes**. Les hashes du script et du résultat observés sont
dans [capture.json](capture.json). Aucun défaut d'admission trouvé dans ce
périmètre. Les corps exportés supprimés restent des hashes déclarés ; le
digest G décrit sa dernière passe seulement.

Le faible coût de C seul ne clôt pas D6 : à coordonnées inchangées, G u32/u21
donne **1,03845 / 1,04452 / 1,13629**. C et G sont mesurés dans des processus
distincts ; leurs temps ne constituent pas une mesure FULL, et leur somme
de médianes ne démontre pas le seuil produit de 3 %. Aucune nouvelle règle
d'adoption ou qualification du domaine u32 entier n'est ajoutée.

```sh
python check.py --repo /workspaces/E-HGP
python -O check.py --repo /workspaces/E-HGP
```

Le lecteur vérifie les blobs Git, les empreintes du patch et de sa postimage,
exécute `git apply --check` puis applique le patch en dossier temporaire, et
vérifie la borne entière. Les deux modes concordent. Aucun moteur, build ou
appel GCP ; aucun fichier produit ou ancien reçu modifié par l'audit.
