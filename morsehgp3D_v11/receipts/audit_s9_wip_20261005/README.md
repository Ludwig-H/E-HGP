# S9 en développement — propagation du refus pendant le tri

5 octobre 2026. Brouillon non commis de `build/v11-impl-l3`, base publiée
`53c027fe848b0d890f164eb87ebf347338c58d55`. Chaque capture conserve ses
octets et SHA ; ce verdict porte sur ces captures, pas sur un futur commit.
Cadre `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

## Point important : ne pas poursuivre le tri après un refus

`src/points/point_tree.cpp`, fonction `entry_order` : un groupe d'entrées
strictes de même plancher est trié par date exacte, avec SiteIdx pour
départager les égalités. Après un refus de `compare_dates`, le comparateur
rend à la place `a < b` sur les SiteIdx, puis emploie cet ordre pour
toutes les comparaisons restantes. `failure` n'est vérifié qu'au retour
de `std::sort`.

Ce changement de relation pendant le tri invalide sa précondition
d'ordre. Le modèle borné du partitionnement GCC, conservé dans `native/`,
montre un accès à l'index 17 dans un tableau de 17 entrées : le pivot
choisi selon les dates devient le plus grand SiteIdx quand le refus
survient à la quatrième comparaison. Le scan non gardé ne trouve plus
sa sentinelle. Les témoins sans refus et avec arrêt immédiat du tri
distinguent la cause.

**Correction attendue avant livraison S9 :** le premier `Outcome` refusé
doit interrompre le tri, sans comparaison de substitution. Un tri qui
propage `Outcome` convient. Une exception locale convient aussi si elle
est interceptée dans la fonction `noexcept` contenant `std::sort`.
Conserver le départage SiteIdx uniquement pour les égalités exactes.
Ajouter sur G4 une porte injectant le refus après plusieurs comparaisons
réussies et vérifiant son retour, les réservations rendues et l'absence
de publication.

**Limite de la preuve :** le refus est injecté dans un modèle Python du
tri. Aucun crash C++, aucun nuage u21 naturel épuisant le budget de
radicaux et aucun test natif ne sont revendiqués. Cette limite ne rend
pas sûr le chemin de refus prévu par l'API.

## Autres chemins relus

Le [modèle mathématique borné](math/REPORT.md) recoupe les qualifications,
les LCA, le plancher maximal, les partitions à chaque plateau et les
entrées dans un bloc vivant. Les six petits témoins passent en normal
et `-O`. Ils n'exercent pas le refus du tri et ne le qualifient pas.

Le raccord API et le format ont été lus sur la capture `api/` : le refus
K=n≥2 précède l'index, le produit possède ensemble l'arbre et la
hiérarchie, et la publication conserve les contrôles d'identité Session
et de provenance avant écriture. L'écrivain sérialise les colonnes
MHGP11PT dans l'ordre annoncé ; les niveaux sont exacts, non réduits.
Aucun nouveau défaut important établi dans ce raccord en lecture.
Le manifeste et la porte CLI comparent les comptes et la signature
d'arbre aux autres sorties.

Le lecteur ne prouve pas seul le plancher maximal ni toute la hiérarchie
géométrique. Le moteur certifie le plancher et son successeur sur tout
le catalogue ; le différentiel `points_vs_python` compare les colonnes
complètes et l'oracle indépendant compare les partitions. Ne pas
transformer une lecture structurelle du fichier en oracle de géométrie.

Aucun build, test natif ou GCP lancé par l'auditeur. Les résultats G4
A2/B au pin b319efc84 précèdent S8 et S9. Les sources en développement
et les qualifications antérieures restent distinctes.

## Rejeu

```sh
python3 replay.py /chemin/du/depot
python3 -O replay.py /chemin/du/depot
```

Le rejeu vérifie les empreintes de la capsule et des captures, reconstruit
le modèle mathématique par Git + fichiers figés, puis compare les deux
contre-épreuves aux sorties capturées. Aucun worktree développeur ni
binaire n'est requis. Les commandes de contrôle après intégration sont
dans `packaged_checks.json` ; les preuves G4 B et les ensembles de reprise
gardent leurs capsules distinctes.
