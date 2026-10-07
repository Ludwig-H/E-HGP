# Proposition ciblée : propriété des allocations CUDA

7 octobre 2026. Proposition de correction issue de la [lecture statique T1-b](../../audit_t1b_tour_prepublication_20261007/cuda/README.md).
Le prototype est encore au hash audité `620c840df6a8c3c9819bf1719754ba50f69a7d1ef7e710b031ecc0f9218fd131`
pour `src/catalogue/device_cuda.cu`. [ownership.patch](ownership.patch) ne modifie que ce fichier.
Il n’est appliqué ni au prototype ni à `main` ; aucun statut de qualification n’est transféré.

Dans `grow`, le type `Array<T>` existant possède temporairement l’allocation et sa réservation.
Tout retour sur refus déclenche son destructeur : tentative `cudaFree` avant restitution de
la réservation. Après synchronisation réussie, les échanges de pointeur, capacité et
réservation sont sans exception : le temporaire reçoit l’ancien tableau et le détruit une fois.
Le pointeur nouveau n’est donc jamais abandonné par une sortie anticipée.

Dans `stage`, une réservation locale précède un `unique_ptr` à deleter `cudaFreeHost` ; les
inclusions `<memory>` et `<utility>` sont explicites. Sur refus, le pointeur éventuel est
libéré avant la réservation. Au succès seulement, le membre adopte les deux. La politique
existante est conservée : l’ancien tampon est libéré après synchronisation et avant de
réserver le nouveau ; ce patch n’ajoute donc aucun pic de coexistence. En cas de refus après
cette libération, l’état redevient vide, sans réservation fantôme.

La revue suit aussi `Impl` : ses tableaux `state` sont détruits avant `exec`, donc avant la
destruction du flux. Les temporaires, membres et pointeurs transférés ont chacun un seul
propriétaire ; les échanges n’ouvrent aucun chemin de double libération en C++.
**Pilote défaillant : nettoyage best effort.** Les erreurs de `cudaFree`/`cudaFreeHost` dans
les destructeurs restent ignorées comme auparavant. Une erreur CUDA collante peut empêcher
la libération matérielle ; ce patch ne certifie ni cette libération ni la comptabilité
physique après échec du nettoyage. Il conserve la classification et l’état `broken` du refus
initial. Le cas d’une destruction pendant une faute de pilote reste à tester sur appareil.

Vérifications réalisées : motifs de remplacement uniques, source inchangée après lecture,
`git apply --check`, puis application textuelle sur une copie temporaire identique et contrôle
du hash obtenu. Les empreintes, tailles et revue des branches sont dans [capture.json](capture.json).
**Aucune compilation C++/nvcc, injection native, exécution CUDA ni opération cloud.** Avant
adoption produit, compilation et tests de panne restent nécessaires ; la proposition ne ferme
pas un constat global de mémoire.

```sh
sha256sum -c SHA256SUMS
```
