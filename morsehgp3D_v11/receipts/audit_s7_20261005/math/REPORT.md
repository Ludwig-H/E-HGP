# Contrelecture mathématique S7 — 5 octobre 2026

Verdict : aucun défaut mathématique important trouvé dans les nouveautés S7 au pin `966a351be1b58089fd13bc44a8dbafc56d87999a`. Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. Lecture indépendante et preuve Python bornée ; aucun build, appel natif, GCP ou rejeu des 963 ordres.

Sources Git figées avant lecture/rejeu, inventoriées par SHA256 dans `source_manifest.json`. Rapports développeur copiés stables dans `developer_reports/` ; leurs mentions de `bd7c8e130` précèdent l'amendement `966a351be`. Le petit delta de l'amendement ajoute notamment le refus de PointId répétés au lecteur ; ce contrôle est présent dans le pin examiné. Le snapshot ne constitue pas une qualification native.

## Apport de lecture

Le format produit par `src/api/write_supports.cpp` place les colonnes selon l'en-tête MHGP11SP v1 ; les balles sont celles de l'assemblage S6b, les comptes sont dérivés. La signature commune `src/api/manifest.cpp:187` utilise la géométrie Morton, puis la forêt canonique, les parents/rangs/genres, le site de naissance K1 ou S* des naissances K≥2 et les enfants. Le lecteur `bench/mhgp11_formats.py:529` recalcule la même sérialisation V2 depuis les supports. La nouvelle porte `tests/cli/cli_supports_oracle.py:87` ramène le fichier aux conventions géométriques de S1 et compare tous les champs ; son raccord FULL/supports natif reste une preuve développeur distincte, non exécutée ici.

## Nouvelle exécution portable

`check_format.py` construit indépendamment les colonnes et les en-têtes à partir des arbres et supports S1 (étage A, MEB en Fraction), puis appelle le nouveau lecteur et sa conversion officielle vers le document exact. Les rangs d'événement sont obtenus par enumeration indépendante des supports positifs minimaux en Gram/Fraction ; ce code ne prend pas les rangs du lecteur. La signature V2 est sérialisée séparément depuis l'arbre exact de définition et comparée à celle recalculée depuis le fichier décodé. Aucun octet n'est produit par le moteur C++.

Témoins : carré K1/K3, triangle droit K2 (site orphelin de Q), cellule passagère K1, croissance ABCZ K3 (boule interne tardive), cube K1 (tétraèdres à cofaces nulles), témoin K10 aux ordres 10 et 12. Total : 8 fichiers, 40 boules, 58 supports, 31 nœuds. Les documents décodés concordent sur les champs exacts S1, et les huit signatures V2 concordent. 1 764 requêtes comparent la positivité, le centre et le niveau des formules entières du lecteur à Gram/Fraction ; 154 autres sont faites sur le cube translaté près de `2^24−1`. Cette dernière primitive ne qualifie pas le profil natif u24.

Quatre corruptions ciblées sont rejetées par `read_supports` : PointId répété, bourrage non nul, rang de naissance-site non nul, rôle fusion changé en interne. Les messages exacts sont dans les JSON de sortie.

La suppression des deux tétraèdres du cube K1, dont les cofaces sont nulles, est admise par le lecteur seul puis détectée par le différentiel exact (longueur des comptes par support 4 contre 6). Cela confirme la frontière expressément documentée : sans la coquille, le fichier et son digest ne prouvent pas la complétude Q_b ; la nouvelle porte S1 vérifie celle-ci. Ce résultat est une contre-épreuve volontaire et non un défaut du contrat du lecteur.

Les quatre exécutions du harnais sont closes avec code 0, sans essai de harnais en échec : direct normal, direct `-O`, replay Git normal, replay Git `-O`. Les sorties sont identiques octet pour octet : SHA256 `8b65f4ee4e9a0681f24766f952690f5c9e54ba2747b9b3192f75485ae02ea1c1`. `executions.json` conserve les commandes, codes, empreintes du script et sorties exactes. Le premier script exécuté est conservé dans `attempts/initial_check_format.py`, identique au script final.

## Reproduction et limites

Depuis cette capsule, avec un dépôt Git contenant le pin :

```sh
python3 replay.py /workspaces/E-HGP
python3 -O replay.py /workspaces/E-HGP
```

Le replay reconstruit et vérifie les sources par Git + SHA256 dans un répertoire temporaire puis exécute le harnais. Il ne dépend ni du snapshot livré, ni d'un worktree développeur, ni d'un binaire natif. Fichiers à reprendre : `REPORT.md`, `source_manifest.json`, `check_format.py`, `replay.py`, les quatre JSON de sortie et leurs stderr, `executions.json`, `SHA256.json`, `developer_reports/` et `attempts/initial_check_format.py`. Aucun nouveau verrou ; les qualifications natives/G4 et leurs bornes de performance restent celles des reçus dédiés.
