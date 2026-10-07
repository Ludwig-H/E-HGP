# M6 — proposition minimale pour le résidu CST-0018

**Proposition vérifiée, non livrée au produit.** Le juge courant reste au SHA256
`4a5b30fefdceba039b336ef2590013aaaec2a22b3fddea30c9f0a5bd650fa90e` du pin `f601b36ac` ; aucune modification de
`main`, du produit ou du canal `audits/`. Le patch s'applique à ce corps inchangé et complète le lot `2b2113264`.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`. Aucun GPU, compilateur natif, grand nuage ou
nouveau temps de calcul. Les prises synthétiques et tous leurs fichiers sont temporaires.

## Correction proposée

`proposition.patch` modifie seulement `run_m6.py` :

- vérifier mode, indice entier strict et plage avant de constituer l'ensemble des clés ; refuser toute entrée
  invalide et exiger exactement `3 * processes` prises réellement validées ;
- vérifier strictement les codes de compilation/exécution et le nombre de lignes, sans égalité implicite de
  `false` ou `0.0` avec `0` ;
- accepter explicitement les seuls schémas connus v1 et v2 ; refuser schéma absent, inconnu ou mal typé ;
- préserver les refus explicites et verdicts d'échec, y compris dans la branche historique.

**Précision historique :** contrairement à une suggestion du reçu `audit_reprise_20261007/juges`, le vrai rapport
G4 A contient bien `schema="ehgp.v12.mes_m6.v1"`. Sa compatibilité est conservée, avec les trois limites déjà
déclarées. Aucun reçu historique sans schéma n'a été identifié ; le patch n'invente pas ce troisième format.

La raison mathématique du défaut est simple : en Python, `False == 0` et `0.0 == 0`, donc la couverture d'un ensemble
de clés ne prouve pas leur typage. Après filtrage, la cardinalité des prises validées doit aussi rester celle du
produit cartésien `{spin,yield,blocking} × range(processes)`.

## Preuve causale

`check.py` applique le patch dans un dossier temporaire, reprend les fabriques et les contre-témoins du précédent
reçu, puis appelle la porte officielle sans la modifier. Pendant les fabriques, toute commande externe via
`subprocess.run` est interdite. Normal et `-O` rendent le même JSON, octet pour octet.

| Comparaison | Avant | Proposition |
| --- | --- | --- |
| Base v2 complète : neuf prises, 585 lignes | code 0 | code 0 |
| Douze contournements : indices booléens/flottants, zéro fichier, schémas inconnus ou absents, codes non entiers, refus historique | code 0 | code 3 |
| Deux indices entiers hors plage | code 3 | code 3 |
| Porte officielle | — | 54 cas, zéro écart |
| Relecture historique G4 A, dans cette porte | — | neuf prises, 585 lignes, médianes identiques au reçu auditeur, trois limites conservées |

`normal.json` conserve les quinze comparaisons et les refus ; `verification.json` et `SHA256SUMS` ferment la
capture. Le script vérifie aussi les dépendances avant/après. Le résultat permet de proposer une correction
concrète au développeur ; il ne ferme pas CST-0018 tant qu'un corps produit corrigé n'a pas été contre-jugé.

## Rejeu

Depuis ce dossier :

```sh
python3 -B -S check.py > /tmp/m6-proposition-normal.json
python3 -B -S -O check.py > /tmp/m6-proposition-optimized.json
cmp /tmp/m6-proposition-normal.json /tmp/m6-proposition-optimized.json
```

Le rejeu a besoin du dépôt et de sa capture G4 A, mais ni d'une VM ni de données LiDAR. `git apply --check` puis
`git apply` s'exécutent uniquement dans le dossier temporaire. L'empreinte du corps proposé est publiée dans
`sources.json` ; ce dossier ne contient aucune copie modifiée du produit.
