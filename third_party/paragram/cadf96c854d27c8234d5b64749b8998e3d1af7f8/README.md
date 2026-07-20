# Série candidate Paragram pour MorseHGP3D

Cette série s'applique uniquement au commit Paragram
`cadf96c854d27c8234d5b64749b8998e3d1af7f8`, dont l'arbre Git est
`ad17c2eff4c5beec43330f597ee5d6616b62d32d`. Le manifeste de provenance
complet est `docs/references/paragram_source_pin.toml`.

Paragram est distribué sous licence Apache-2.0. Cette série ne change pas cette
licence ni le gitlink cuBQL. Elle journalise séparément les modifications
MorseHGP3D : statut explicite de débordement de pile BVH, arrêt fail-closed des
parcours concernés et quarantaine bidirectionnelle de l'adjacence des cellules
en erreur, puis boîte fermée binary32 explicitement fournie par l'appelant sans
padding caché. La série reste un candidat d'évaluation; elle n'est pas intégrée au
produit et ne confère aucun statut exact à une sortie Paragram.

La série doit être consommée atomiquement dans l'ordre de `series.toml`. Sa
validation hors ligne s'exécute depuis la racine E-HGP avec :

```sh
python tools/check_paragram_patch_series.py /chemin/vers/paragram
```

Le checker vérifie le pin source, les empreintes, les chemins, les arbres Git
avant et après application et les invariants structuraux des deux correctifs.
Il ne compile pas CUDA. Les tests CPU de l'API AABB sont courts et indépendants
du binding.

Le jalon MorseHGP3D 7.4 gèle cette série à deux patchs. La correction complète
du stream et des fautes traverse le builder cuBQL, tandis que les phases 8–9
exigent un clipper H-polytope plus général que l'API site-centrique. Paragram
reste un comparateur épinglé et peut proposer des identifiants de concurrents;
leur fermeture est rejouée exactement dans MorseHGP3D. Aucun patch d'export
géométrique, différentiel CUDA ou statut produit n'est revendiqué ici.
