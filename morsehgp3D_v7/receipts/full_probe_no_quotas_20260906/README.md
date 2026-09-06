# Sonde FULL v5 sans quotas de travail — captures R1 conservées

`public_status=not_claimed`, CPU de référence, entrée quantifiée u16. Ce paquet
conserve les octets des captures closes ; il ne rejoue aucun test et ne répare
aucun ancien reçu. Les chemins absolus capturés restent des identifiants du
poste d'origine. Les sources effectivement consommées sont dans `source_snapshot/`.

- `build_r1/` : compilation terminée, code 0 ; quatre commandes capturées,
  sans exécution scientifique dans cette étape.
- `micro_r1/` : campagne **failed**, 225 commandes et 37 tentatives scientifiques.
  Les portes des limites ont passé 52 contrôles en O2 et en ASan/UBSan ; les
  six CTests ciblés ont passé. Les autres portes locales, leurs rejets et le
  mutant privé sont également conservés avec leurs sorties.
- 36 micros Kmax=5 sont validés par le contrôleur (180 ordres horizontaux).
  La première tentative Kmax=10, `n8_s8_k10_eager_c0_p0`, a produit son brut
  et un code moteur 0, mais son verdict de capture est **failed** :
  `KeyError: 'successor_accounting'`. L'auto-test first-C v5 omettait ce champ
  de version exigé par le contrôleur. Ce résultat n'est pas promu en micro validé.

Il ne s'agit donc **ni de 77 micros/503 ordres qualifiés, ni d'une campagne
complète**, ni d'un contrat 50k ou d'une tour inter-K intégrée. Les extensions
de domaine prévues n'ont pas été exécutées. Les benchmarks directs ultérieurs
ne figurent pas dans ce paquet.

Après la clôture, une seule ligne a ajouté ce champ au lecteur first-C v5,
SHA `d51ee4e4ad8cdcb33d86f721dd7bf6e48a11cd730d1373d0845656fa69a1ee5f`.
Un petit modèle Python séparé a passé en normal et sous `-O` (codes 0, sorties
identiques, 40/25 mutants, six champs de métadonnées par lecteur). Ce contrôle
ponctuel n'a pas relancé la campagne R1 et ne change aucun de ses verdicts.
La source first-C R1 demeure dans le snapshot avec son hash `a57e54c4…`.

`publication.json` associe les deux reçus d'origine à ce paquet.
`excluded_binaries.json` énumère exactement dix exécutables/objets absents,
avec leurs hashes vérifiés lors de la copie. Les originaux privés n'ont pas
été supprimés ou modifiés. Toutes les autres captures, y compris les fichiers
CMake de provenance et les sources du mutant privé, sont copiées à l'identique.
`SHA256SUMS` couvre tous les fichiers publiés sauf lui-même ; vérification :
`sha256sum -c SHA256SUMS` depuis ce dossier. Aucun GCP utilisé pour cette copie.
