# MES-M5 : audit du juge et du format, 7 octobre 2026

Épingle : `e30000dec1027c5f0ade3093a94563ee412409d3`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Périmètre : `microbancs/mes_m5_parcours/scripts/g4_traversal_bench.py`,
`include/mhgp12/traversal/format.hpp` et `compare.hpp`. Aucun code produit ni fichier
`audits/` modifié. Aucun GPU, GCP, réseau, données réelles ou gros build.

## CST-0018 prolongé à MES-M5 — adoption sur preuves insuffisantes

Gravité majeure, outillage de qualification. Ne pas considérer le seul verdict
`adopte` de ce script comme la fermeture de MES-M5 avant les corrections.

Le témoin exécute le **vrai `main()`**. Seuls les programmes externes, les fichiers
qu'ils produiraient et la collecte d'environnement sont simulés. Il conserve les
10 000 tirages du bootstrap, le vrai manifeste de commande, le vrai chargement des
JSON et le vrai juge. Tous les fichiers sont minuscules et synthétiques ; aucun
fichier factice n'est fourni au moteur. Les six défauts sont injectés séparément :

| Injection | Résultat observé | Contrat attendu |
| --- | --- | --- |
| Une trame, un seul cas K5/24, un processus | `adopte`, aucune raison de refus | Les neuf cas et cinq processus, dont six cas décisifs, sont requis pour l'adoption globale (§ 6 du README du banc). |
| Une seule des six fixtures | `adopte` | Les six identités hôte/appareil et les cibles sanitizer complètes sont requises. |
| L'outil d'identité hôte écrit ses lignes puis sort avec code 2 | `adopte` | Un refus d'outil ne doit pas devenir une preuve conforme. |
| L'outil des fixtures appareil écrit ses lignes puis sort avec code 7 | `adopte` | Un échec d'outil doit invalider la campagne. |
| Dix passes v11 demandées, JSON déclarant une seule passe et un seul temps | `adopte` | Les passes chaudes 2 à 10 sont requises ; aucune n'existe ici. |
| Quinze temps GPU bruts de 60 ms, médiane déclarée de 6 ms | `adopte`, ratio 0,1 | Avec le témoin v11 à 60 ms, le ratio brut vaut 1, supérieur à 1/4. |

Les contrôles positifs rendent bien les trois états : campagne synthétique complète
et rapide → `adopte` ; mêmes quinze temps GPU de 60 ms et médiane honnête de 60 ms
→ `rejete` ; chemin du dump incohérent avec la commande → `refuse`.

Points de code (chemins relatifs au microbanc) :

- `scripts/g4_traversal_bench.py:108` exige seulement des listes non vides et un
  nombre positif de processus ; `:372–385` construit la grille depuis les options
  sans distinguer diagnostic partiel et adoption du protocole complet.
- `:130–145` exige une table de fixtures non vide et une fixture précise pour le
  mutant de repère. `:481` emploie `all(...)` sur le manifeste fourni, sans vérifier
  les six noms attendus. Le selftest « valide » contient lui-même une seule trame
  et une seule fixture, puis attend `adopte` (`:194–211`).
- `:504` et `:559` enregistrent les codes d'identité sans les propager au refus.
- `:595–597` contrôle K et la taille de feuille, mais pas le nombre de passes v11 ;
  le repli `times[1:] or times` réintroduit la seule passe froide.
- `:612–615` contrôle chemin/K/taille/nombre de temps GPU, puis utilise directement
  `median_ms` sans recalculer la médiane depuis `total_ms`.

Corrections attendues : figer une grille et un ensemble de fixtures requis pour
l'adoption ; marquer les exécutions réduites comme diagnostics ; vérifier les codes
et la couverture exacte des sorties ; contrôler passes/échauffement et cohérence
des séries ; recalculer les agrégats à partir des valeurs brutes validées. Ajouter
ces injections aux portes, en plus du selftest actuel qui passe ses 16 scénarios.

Limite de la preuve : nous établissons une fausse **admission de preuves par le
juge**, pas une panne de calcul natif, ni une mesure GPU réelle erronée. Le double
d'exécution est explicite ; les données des injections ne sont pas des mesures.

## CST-0223 — Références MHGP12TR sémantiquement invalides admises

Gravité moyenne, outillage. Une TU C++ appelle le vrai écrivain puis le vrai lecteur
sur des fichiers synthétiques de **808 octets au maximum**. Tous les témoins ont
un FNV valide, les tailles physiques exactes et des références de sites bornées.

| Fichier du témoin | Violation | Lecteur |
| --- | --- | --- |
| `impossible_test_count` | Un seul candidat, mais `UINT64_MAX` tests G1, recopiés dans le grand livre. | Accepte |
| `wrapped_test_sum` | Tests des trois nœuds : `UINT64_MAX`, 2, 2 ; somme mathématique `2^64+3`, livre égal à 3. | Accepte |
| `nonterminal_leaf` | K1, taille de feuille 4, cinq sites distincts retenus, boîte de largeur 5, genre feuille au lieu d'une coupe. | Accepte |
| `wrong_root_envelope` | Nuage singleton `(0,0,0)`, boîte racine élargie à `[0,2)` en x au lieu de `[0,1)`. | Accepte |

`format.hpp:255` contrôle qu'une coupe dépasse la taille de feuille, mais ne
contrôle pas l'implication réciproque sur une feuille non unitaire. `:256–258`
contrôle profondeur, candidats et chemin de la racine, sans son enveloppe.
`:282` additionne les tests en `u64` sans borne ni contrôle de débordement ; `:312`
compare le grand livre à cette somme modulo `2^64`.

Corrections attendues : borner les tests d'un nœud par
`candidates * min(candidates, 3*K)` avec arithmétique contrôlée ; vérifier la somme
avant addition ; contrôler la condition de terminaison d'une feuille et l'enveloppe
de la racine. Ces contrôles simples ne demandent pas de rejouer tout G1.

**Aucune adoption de ces références par l'identité native n'est établie.** Le
constat porte sur leur admission par le lecteur annoncé strict. Un FNV exact
protège les octets, pas la validité des faits qu'ils représentent.

## Résultats positifs du lecteur et du comparateur

Le lecteur refuse les six corruptions sémantiques classiques : SiteIdx hors nuage
(FNV de liste mis à jour), K nul, coordonnée hors profil, `begin=UINT64_MAX`, livre
différent des sections et refus transactionnel contenant encore un préfixe.
Un fichier de 264 octets annonçant `2^40` nœuds est refusé **avant allocation des
vecteurs**. Lecture de `:208–218` : les produits et sommes de tailles restent dans
`u64` sous les bornes déclarées ; la taille physique est contrôlée avant `resize`.

Le comparateur accepte la référence singleton valide, puis refuse séparément une
liste différente, une plage de sites invalide, un chemin différent, une boîte
dupliquée et un livre différent. L'égalité s'appuie sur les listes et métadonnées
exactes, en plus de l'empreinte (`compare.hpp:110–147`). Pas de défaut nouveau
démontré dans ce comparateur par ces contrôles ciblés.

Fraîcheur, lecture du lanceur : les anciens dumps sont supprimés (`:454–455`), les
JSON d'identité et de prises sont supprimés avant leur commande (`:496–497`,
`:551–552`, `:585–586`), et les binaires sont rehachés à la fin (`:629–634`). Le
témoin de mauvais chemin confirme un refus réel du main. Ces protections ne
remplacent pas les vérifications de contenu et de couverture manquantes ci-dessus.
Ce lot ne qualifie aucune exécution réelle de MES-M5 sur G4.

## Rejeu et fermeture

```sh
python3 morsehgp3D_v12/receipts/audit_b_m5_20261007/juge_format/run.py
python3 -O morsehgp3D_v12/receipts/audit_b_m5_20261007/juge_format/run.py
```

Captures `RESULT_normal.json` et `RESULT_O.json` : mêmes neuf injections du main,
16 scénarios du selftest existant et 18 lignes exactes du témoin C++. Contrôles par
exceptions explicites, jamais `assert`. Une seule petite TU par rejeu, aucun
sanitizer ni gros build. Les JSON/logs factices sont temporaires et supprimés.

`MANIFEST.json` épingle 18 sources au commit et les trois scripts/TU de preuve.
Le rejeu exige leurs empreintes avant et après et vérifie un HEAD stable. `-MMD`
ferme les sept en-têtes locaux effectivement compilés, avec la TU ; les en-têtes
système ne sont pas épinglés, le compilateur et le binaire sont identifiés dans
les captures. Aucun artefact binaire ni vidage synthétique conservé dans ce reçu.
