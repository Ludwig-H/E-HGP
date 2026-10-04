# Rapprochement de la contrelecture géométrique avec 77db5738

Publication figée : `77db5738eb2dd5bc84ecdc4d85ade833124c58f8`. Les douze blobs sont lus par `git show <commit>:morsehgp3D_v11/<path>` et leurs SHA256/Gitblob comparés aux douze copies fermées de `../device_geometry/`. Cette annexe ne modifie pas la capsule précédente ; elle n'apporte aucune qualification native/GPU/G4.

**11/12 fichiers identiques octet pour octet** : `leaf_device.hpp`, `leaf.cpp`, `support.cpp`, `center_line_cache.hpp`, `num/{budgets.hpp,predicates.cpp,sphere.cpp,power_certificate.hpp,orientation_certificate.hpp,q4_weights.hpp,center_region.cpp}`. La contrelecture précédente conserve donc exactement ces ancrages.

Le seul delta est `leaf_device_predicates.hpp` : SHA256 snapshot `11d5ca5887d5677bfd368123a7c2557438635a66ce7ce70b1b65835e4f34ca43` → publié `dfdb31d0f6a970d6344fc1490ff7114b4cf0c785ddea001c7babe49503a7d052`. Huit lignes ajoutent seulement commentaires, deux constantes et un static_assert des compteurs. Une fois ce bloc retiré, **tout le reste du header est identique**, y compris chaque prédicat. Les deux headers et leur diff sont conservés ici.

Calcul exact : P=41448, borne locale 32·3·P=3979008<2^22. Un lot borné à 2^32 feuilles reste donc <2^54 par réduction. La justification opérationnelle du plafond doit être la garde **explicite** `view.count<=view.cloud_sites` : elle est présente dans les blobs publiés des executeurs hôte (`leaf_batch.cpp:72`) et CUDA (`leaf_batch_cuda.cu:185`). Les listes de sites associées aux régions de centres peuvent se recouvrir ; leur disjonction ne doit pas servir de preuve de ce plafond. Cela ne réfute pas la borne sous la garde présente et n'établit pas une panne du produit.

`BINDINGS.json` conserve les douze paires SHA256 et les identités Git, plus le SHA du manifeste fermé précédent. Les sorties normal/−O sont identiques ; **23 gardes** de rapprochement/arithmetic sont jouées sans C++/nvcc. Replay : `python3 -B -S check_bindings.py` et `python3 -O -B -S check_bindings.py`. Le lecteur `verify.py` vérifie l'inventaire exhaustif, les sorties et le replay sans écriture.
