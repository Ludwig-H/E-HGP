# R1 : complément d'exécution des portes et mutants du registre

Complément au [raccord mathématique](../r1_raccord_math/README.md) et à l'[admission G4 R1](../session_r1_admission/README.md), sans nouveau test natif. **Six mutants R1 ont un verdict local `TUE/code`, sur témoin vert ; six portes du registre passent dans l'archive G4.** Ces deux preuves sont distinctes.

## Rapport local retrouvé et raccord source

Le rapport `mut_r1.json` porte 15 mutants sélectionnés, code final 0, témoin vert ; les six entrées `r1_*` ont chacune leur porte nommée, verdict `TUE`, cause `code`. Le journal concorde et annonce zéro mort par signal, délai ou construction. La copie historique récupérée reproduit exactement son arbre source **`6ff289d7…` (405 fichiers)** et son manifeste **`91f8ad3b…`**. Elle est sauvegardée hors dépôt avec ces deux primaires ; aucun binaire, résultat géométrique ou donnée LiDAR copié.

Ce prototype global n'est pas le commit `47feedc96` : plusieurs autres sources ont changé au raccord produit. En revanche sont identiques au Git livré : `registry_branches.cpp`, `registry_unit.cpp`, l'oracle `forest_support.hpp`, le lanceur des mutants et ses lecteurs de portes, les macros de test, ainsi que **les six mutations et leurs portes**. Le bloc CMake déclarant les cinq groupes et l'inventaire est également identique. Ce raccord local précis ne transfère pas une qualification de tout le prototype au produit.

| Mutant local et livré | Porte | Discrimination lue dans le code de l'oracle |
| --- | --- | --- |
| `r1_copie_de_tous_les_enfants` | `registry_fixtures` | Toutes les lignes deviennent directes : les sous-arêtes partagées doivent garder deux branches chacune, pas les trois enfants de leur classe commune. |
| `r1_critere_d_sans_un` | `registry_fixtures` | `q=d` supprime les lignes directes : les branches peuvent rester justes, mais `branch_reads` diffère du nombre `Q_g` indépendant. |
| `r1_copie_directe_du_tampon` | `registry_grand` | La source d'une ligne directe devient `branch_nodes` ; les valeurs sont comparées, dans leur ordre, aux branches de coupe ouverte sur 5 401 lignes. |
| `r1_admission_sans_q_g` | `registry_etapes` | Les `4Q_g` octets de travail manquent à l'admission : comparaison du pic de R à ses octets admis. |
| `r1_branches_directes_non_comptees` | `registry_fixtures` | Le compteur physique `branches` omet les lignes directes : comparaison au nombre indépendant `A`. |
| `r1_admission_en_q_r` | `registry_etapes` | Admission en `Q_R` : le reliquat après soustraction des tailles exactes varie entre cas, au lieu de ne compter que tâches et compteurs. |

L'oracle ne recalcule pas le critère produit `q=d+1`. `expect()` compte les cellules contributrices d'une classe dans la forêt de référence ; `check_rows()` reconstruit les coupes ouvertes, trie/déduplique les identifiants attendus et compare la CSR **mot à mot, dans l'ordre** (`forest_support.hpp:232–261`). `judge()` contrôle les branches, leurs cardinalités et les compteurs (`registry_unit.cpp:78–92`). Les sous-arêtes, cellules inertes, doublons et naissances datées figurent parmi les neuf fixtures ; la preuve générale reste celle du reçu mathématique.

## Ce que la cause « code » atteste

Au pin du lanceur, une configuration ou compilation impossible rend le mutant `INVALIDE`, pas `TUE`. Le témoin doit exécuter et passer exactement les portes citées. Une mort `TUE/code` exige une porte exécutée en échec avec `run_expect_verdict code` ; signal et délai ont des causes distinctes. La mutation porte uniquement sur le fichier produit, pas sur l'oracle ni la commande de test. Les raccords source permettent donc de distinguer ces échecs de porte d'un échec de construction ou d'un délai.

**Limite conservée :** `judge_copy()` efface la sortie détaillée lorsqu'il rend `TUE`. Les CHECK fautifs et leur code numérique individuel ne sont pas archivés ; le tableau décrit les discriminants du code, pas une ligne d'assertion observée dans chaque exécution. Aucun ELF local ni fermeture de ces builds n'est ajouté à la preuve. Les anciens logs isolés `reg_*` ne servent pas d'autorité : certains appartiennent à des versions antérieures des seuils de test.

## G4 : preuve séparée sur la source livrée

L'archive R1 **`f288289a…`**, déjà admise et liée au paquet `47feedc96`, contient six lignes `Passed` : `fixtures`, `exhaustif`, `aleatoire`, `grand`, `etapes`, **`inventaire`**. Ce sont cinq groupes natifs et une porte d'inventaire, pas cinq portes au total. Elle contient aussi le succès du lanceur `mhgp12_mutants_tower`, manifeste de 55 mutants ; elle ne conserve pas son rapport individuel permettant d'attribuer une cause à chacun des six R1 sur G4. Aucun erratum n'est nécessaire au reçu G4 publié, qui comptait déjà six portes.

Ce complément étaye spécifiquement les branches R et leurs compteurs/admissions. **FUL1 ne couvre pas leur CSR** ; ni les empreintes massives ni le succès du lanceur global ne remplacent cette preuve. Les portes de budget portent `run_registry` et ses étapes ; elles ne resserrent pas automatiquement l'admission conservatrice de la Session recouverte.

## Relecture légère

[check.py](check.py) rehache les 405 sources locales, raccorde les sources critiques et les six entrées au Git, vérifie rapport/journal, puis relit les six portes dans l'archive G4 close. Il ne lance aucun moteur, compilateur, mutant ou contrôleur. Normal et `-O` donnent les mêmes [captures](capture.json).

```sh
python -B morsehgp3D_v12/receipts/audit_reponses_20261008/r1_mutants_execution/check.py --repo /workspaces/E-HGP --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/r1_mutants_execution_20261008 --session /workspaces/.ehgp-sessions/v12.20261008.r1
python -O -B morsehgp3D_v12/receipts/audit_reponses_20261008/r1_mutants_execution/check.py --repo /workspaces/E-HGP --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/r1_mutants_execution_20261008 --session /workspaces/.ehgp-sessions/v12.20261008.r1
```
