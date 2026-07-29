# Archives scientifiques de MorseHGP3D

Ce dossier conserve les décisions remplacées et les expériences qui ont fermé une voie de recherche. Un document archivé reste une preuve historique ou un falsificateur reproductible; il ne décrit jamais l'architecture courante, un repli produit ou un prochain jalon.

La voie active est définie par le [catalogue exact des paires diamétrales](../math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md), puis par la [frontière des supports trois et quatre](../math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md). Les quelques replis encore maintenus sont listés séparément dans le [registre de recherche](../research/README.md).

## Organisation

- [`abandoned/`](abandoned/README.md) : pistes abandonnées comme architectures de production, avec motif précis et artefacts conservés;
- `abandoned/phase14/` et `abandoned/phase15/` : rapports G4 scellés déplacés hors du parcours actif;
- l'[historique condensé](../HISTORIQUE.md) : décisions plus anciennes dont les sources restent dans Git ou dans `HGP-old/`.

Les JSON, transcripts et checkers restent volontairement dans `docs/validation`, `tests/` et `tools/` lorsque leurs chemins font partie d'un oracle de non-régression. L'archivage retire une voie de la navigation active; il ne casse pas sa reproductibilité.
