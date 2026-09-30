# Régressions du développement frontière et précision

30 septembre 2026. Reçus locaux bornés, `cpu_reference`, hors registre,
`public_status=not_claimed`. Deux périmètres distincts : géométrie native
`quantized_u18_input_only` pour frontière/tour ; primitives isolées
`full_u32_grid_primitives_only` pour distance/identité. Le moteur complet
à précision supérieure reste un chantier. GCP non utilisé.

## Qualification finale

[execution.json](qualification/execution.json) conserve onze commandes,
leurs codes, les logs complets et les hashes avant/après de 51 fichiers
du snapshot final. Tous les codes attendus sont obtenus. Ce gel porte sur
les sources du dépôt sélectionnées, pas sur les en-têtes système ou le
compilateur entier.

- Recherche de rang : 36 047 contrôles, code 0. Les deux mutants compilés
  réintroduisant séparément le milieu débordant et le produit `lo*64`
  donnent code 1 sur le même juge. La limite de visites évite une boucle
  dans le **test**, sans plafonner une recherche moteur.
- Portes structurelles frontière et quotas : normal et `-O`, codes 0.
  Quotas : 642 allocations, 51 886 contrôles et 19 refus.
- CTest final ciblé : cinq portes passent, dont couverture native existante
  et refus des multiplicités. Ce n'est pas le CTest complet du moteur.
- Nouveau candidat par bande : six exports natifs par mode, 815 contrôles,
  nuages de cinq à sept sites K3/K5. Dates et exports appariés normal/`-O`
  égaux ; oracle Gamma exhaustif indépendant pour la réunion K3.

Les [reçus normaux](qualification/native_normal/receipt.json) et
[optimisés](qualification/native_optimized/receipt.json) épinglent sources,
binaire, entrées et sorties. Les fondations copiées sont dans
[foundations](foundations/), avec l'exporteur utilisé uniquement comme
instrument de test. Le moteur provient du snapshot local de main plus le
correctif RankIndex ; l'étiquette de provenance de l'export ne remplace pas
son SHA ni la fermeture de sources.

Le snapshot de 51 fichiers précède l'ajout de la cible CMake des primitives
grille32 ; cette qualification ne contient donc pas leurs exécutions.
Le contrôle CMake ultérieur ciblé conserve son propre reçu.

## Échecs et observations initiales

[initial_observations.json](initial_observations.json) conserve le premier
échec de préparation : deux tests ne trouvaient pas les modules du banc.
Après copie du banc, les quatre régressions héritées passent. Le premier
journal complet de cet échec n'a pas été archivé : les champs correspondants
sont des observations de l'outil, pas un reçu brut intégral.

LeakSanitizer refuse le sandbox sous ptrace. Le même petit binaire
ASan/UBSan est ensuite exécuté avec autorisation hors sandbox, code 0,
36 047 contrôles. Cela couvre le helper sur son
[snapshot initial](initial_rank/), pas un moteur complet sous sanitizers.
La différence du header final est son commentaire d'en-tête uniquement.

## Revue mathématique indépendante

[La revue](math_review/README.md) donne le contre-exemple K3 strictement
intérieur et la preuve de complétude des composantes couvrantes par les
témoins forts. Son test Fraction autonome normal/`-O` vérifie notamment
696 correspondances de couvertures et 72 identités de masse/réserve.
Cette revue précède les nouveaux tests natifs et ne les remplace pas.
Son manifeste est clos séparément ; ses chemins de référence historiques
ne doivent pas être réécrits comme si ses sources avaient été gelées ici.

## Largeurs du futur moteur

[L'audit arithmétique](precision_math/AUDIT.md) distingue u24 du domaine u32
complet, fournit les majorants par opération et des supports positifs extrêmes.
Ses calculs Python exacts normal/`-O` passent ; ce n'est pas une compilation
du futur moteur. Les distances jusqu'à 66 bits et les identités Morton96
sont les premières primitives portées, pas les supports q3/q4 et niveaux.

## Primitives grille32 qualifiées séparément

[Le reçu autonome](grid32_primitives/receipt.json) conserve quatre compilations
et quatre exécutions fraîches, les sources gelées et les hashes des binaires
non versionnés. Normal et UBSan passent, avec sorties identiques :
212 684 contrôles, 10 162 distances et 10 203 aller-retours. Le mutant
Morton tronqué à 21 bits et celui tronquant la distance à u64 sont rejetés
par une erreur numérique, code 1 ; aucun crash ni diagnostic UBSan.

[Le contrôle CMake](grid32_cmake/receipt.json) ferme séparément 78 fichiers du
snapshot, configuration/build/CTest codes 0 et quatre portes ciblées
(rang, grille32, structure frontière, quotas). Il ne remplace pas un CTest
complet. Les manifestes originaux restent inchangés dans leurs sous-dossiers.

Le [lecteur](verify.py), normal et `-O`, vérifie le manifeste extérieur,
les sorties appariées et la portée de ces deux qualifications.
Il juge l'intégrité d'une archive, **pas les sources ou binaires vivants**.

## Portée et reproduction

Ces régressions n'exécutent ni condensation/EOM du nouveau bras, ni ARI,
ni passage à l'échelle, ni GPU. Le contrat FULL 100 ms, le profil large
et une amélioration statistique restent ouverts. Le helper de quotas n'est
pas encore raccordé au générateur privé du développeur.

Les scripts enregistrent les chemins des builds non versionnés. Rejouer
depuis le code courant dans un **dossier neuf**, avec sources et exporteur
explicitement fournis ; ne pas exécuter les commandes archivées dans leurs
dossiers clos. `record.py` décrit le déroulé local et ses chemins historiques,
pas un launcher portable de production. Les fichiers des autres acteurs
et les campagnes R2 restent distincts de cette qualification.
