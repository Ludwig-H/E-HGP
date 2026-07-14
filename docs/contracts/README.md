# Contrats exécutables de la phase 0

Ce répertoire rend testables les exemples minimaux exigés par la phase 0. Il ne constitue ni un oracle numérique indépendant, ni une preuve de l'obligation M.1. L'[énoncé de reconstruction M.1](M1_RECONSTRUCTION.md) conserve explicitement cette obligation, tandis que la [convention de sérialisation v1](SCHEMA_CONVENTIONS_V1.md) fixe unités, indexation, canonisation et compatibilité. Les coordonnées, niveaux exacts, coupes et arbres attendus sont décrits dans les [cinq exemples contractuels](EXEMPLES_CONTRACTUELS.md). La [matrice de traçabilité](MATRICE_ENONCE_TEST_CERTIFICAT.md) relie chaque énoncé observable à une fixture et au champ de certificat qui autorise son assertion.

Les sorties JSON associées vivent dans [`tests/fixtures/contracts`](../../tests/fixtures/contracts/). Elles sont des enregistrements canoniques de `MorseHGP3DResult` conformes au [schéma de contrat v1](../../schemas/morsehgp3d-contract-v1.schema.json). Elles ne doivent pas être interprétées comme des résultats déjà produits par un backend.

La [revue de porte de phase 0](PHASE0_GATE_REVIEW.md) enregistre les artefacts, tests, limites et conditions qui autorisent le démarrage de l'oracle CPU.

## Portée scientifique

- phase : `0`;
- backend sérialisé : `reference_cpu`, pour désigner l'oracle attendu; aucun calcul de production n'est revendiqué;
- profils : `hgp_reduced` et `full_pi0`, distingués explicitement;
- mode conceptuel : `certified`;
- statut : sortie attendue de l'oracle du contrat, sans promotion de M.1.

À l'ordre un, les deux profils coïncident. À partir de l'ordre deux, `hgp_reduced` omet une facette isolée et ne crée une racine qu'à l'apparition d'une composante non triviale. `full_pi0` conserve au contraire cette naissance et son absorption. Les exemples décrivent les deux attentes, même lorsqu'une fixture sérialisée cible un seul profil.

Tant que M.1 garde le statut `proof_obligation`, une implémentation de production ne peut pas déduire `public_status=exact` pour `full_pi0` du seul accord avec ces exemples. Les trois fixtures complètes portent donc `result_kind=partial_hierarchy`, `forest_semantics=partial_refinement`, `proof_basis=m1_conditional_contract` et `public_status=conditional`. Leurs arbres documentent la sortie attendue de l'oracle, sans promotion scientifique. Les deux fixtures réduites peuvent porter `forest_semantics=exact` sur le fondement du théorème 5 du manuscrit.

## Conventions des dessins

- `B...` désigne une naissance;
- `M...` désigne une fusion ou multifusion;
- `R...` désigne une naissance réduite;
- `ij` abrège la facette contenant les identifiants `i` et `j`;
- `@ N/D` donne le niveau exact de rayon carré;
- un nœud simultané ne reçoit que les composantes présentes à un niveau strictement inférieur comme enfants; les facettes nées et absorbées dans le même lot figurent dans le delta de couverture;
- aucun nœud artificiel à l'infini n'est représenté.

Pour un événement d'indice un porté par $S=I\cup U$, les seuls bras stricts sont $F_u=S\setminus\lbrace u\rbrace$ pour $u\in U$. Toutes les facettes de $S$ sont néanmoins activées au niveau fermé. Cette distinction est observable dans l'exemple de fusion binaire.
