# S9 — correction source du refus pendant le tri

5 octobre 2026. Correctif local au-dessus de
`451301787c7e02b8f5e1a4c08064275b6af6de1e`, capturé à 17:10:40 UTC.
Cadre `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

**Le défaut de source d448b3d03 est corrigé dans cette capture.**
Le tri par tas propage le premier `Outcome` refusé avant toute nouvelle
comparaison ou utilisation de sa réponse. Aucun ordre de substitution,
aucune allocation supplémentaire. Le succès garde l'ordre exact des
dates et le départage par SiteIdx. Les quatre fichiers modifiés sont
conservés avec leurs SHA dans [la capsule native](native/README.md).

Le port Python vérifie 6 011 ordres et 133 456 positions du premier refus :
arrêt à la position injectée, raison inchangée, permutation et accès dans
les bornes. Normal et `-O` concordent. Il s'agit d'un modèle du correctif,
pas d'une exécution du C++ ni d'une qualification sanitizer.

La chaîne d'appel a aussi été relue au pin local 451301787 : le refus de
`PointTreeBuilder::build` remonte dans `HangBuilder::run`, puis
`points_parts`, `compute` et `execute` du CLI. Le retour précède la
construction du produit, l'écriture du rapport de succès et l'appel à
`publish`. Les objets possédant les brouillons sont détruits au retour.
Les SHA des sources de cette lecture figurent dans `source_read.json`.
Ce constat de source ne remplace pas une injection native de bout en bout.

La nouvelle porte `sort_refusal` cible le helper, tailles 17..64 et refus
aux comparaisons 4/9/30. Sa qualification G4 reste attendue. Le compteur
`calls` existe déjà : `CHECK_EQ(calls, fail_at)` fixerait directement la
propriété d'arrêt immédiat, sans étendre le scénario. Les rapports
`impl_s9` et `verif_s9` décrivent le commit antérieur au correctif : leurs
tests ne qualifient pas les quatre fichiers modifiés.

Le [delta mathématique de S9](math/REPORT.md) confirme les raccourcis
d'égalité de rangs et deux attendus nouveaux de fixtures. Les autres
algorithmes sont identiques à la capture précédente ; leur modèle n'a
pas été rejoué. Aucun autre défaut important nouveau établi.

## Rejeu et limites

```sh
python3 replay.py
python3 -O replay.py
```

Ce rejeu est autonome en bibliothèque standard : empreintes, capture du
correctif et modèle du tri. Le rejeu mathématique séparé requiert un
dépôt **contenant le commit local 451301787** :

```sh
python3 math/replay.py /chemin/du/depot
python3 -O math/replay.py /chemin/du/depot
```

Cette dépendance Git locale est explicite ; aucun arbre de sources
identiques n'est recopié. `packaged_checks.json` conserve les contrôles
effectués après intégration. Aucun build, test natif, mesure ou GCP lancé
par l'auditeur. Publication du correctif et qualification G4 restent
distinctes de cette relecture favorable.
