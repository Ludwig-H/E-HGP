# MEB privée filtrée — préparation seulement

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucune compilation, exécution C++, qualification,
mesure, activation produit ou modification du dispatch. GCP non utilisé.

## Sources et delta

L'audit `morsehgp3D_v7/audits/MEB_DOUBLE_BUDGET_COURANT.md` a été lu entièrement,
ainsi que les deux helpers ci-dessous et `docs/PROPOSITION_MEB_ET_BUDGETS.md`.
Son SHA est `ae53446787f4f9a191182066621dcc8852bf208730486318fcf73f9841176dc9`.

`pivot.hpp` est une dérivation explicite du prototype dual-budget
`build/v7_meb_dual_budget_prototype/pivot.hpp`, SHA
`0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d`.
Il conserve l'inclusion historique `build/v7_meb_pivot_prototype/pivot.hpp`,
SHA `d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5`,
uniquement pour `Candidate`, `point`, `form`, `ordinal` et `materialize`.
Le repli reste F, header `silent_incidence.hpp` SHA
`f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76`.
Aucun de ces fichiers n'est modifié ni recertifié par cette préparation.

Seuls changent le namespace privé, le calendrier déclaré
`reference_ordinal_plus_native_z_q3_q4_proposal_v2`, le helper de pivot nommé
`native_pivot_ball` et son appel natif. Le helper énumère d'abord les couples
de Q complétés par z (q3), puis les triples de Q complétés par z (q4), dans
l'ordre relatif historique. Aucune forme supprimée ne passe par `charged_form`.
Le helper générique historique `small_ball` reste inchangé et n'est pas appelé.

## Préconditions et invariants conservés

Le filtre n'est valable que dans cette trajectoire native : diamètre global
initial avec départage strict inchangé ; support Q positif authentique ;
premier violateur strict z dans l'ordre des sites, placé en dernier dans T.
Ce n'est pas une API validant des candidats arbitraires. Domaine interne :
2–11 sites distincts u16, indices valides et observateurs passifs.

La paire initiale q2 reste formée et chargée. Le plafond 16 et l'emplacement
de l'incrément des pivots sont conservés. Chaque proposition retenue paie
prospectivement ; `Work` persiste entre MEB et replis. P=0 reste le défaut.
La contenance porte sur T entier ; aucune coquille intermédiaire n'est
rejetée en plus. Les temporaires d'échec, la coquille finale sur tous les
sites, l'ordinal legacy sur les n sites, le support entier dont `support[0]`,
et la matérialisation q4 brute restent inchangés.

Avec une marge P suffisante dans les deux bras sur toute la séquence partageant
Work, l'audit justifie la conservation de la trajectoire et du premier accepté.
Une même valeur numérique de P ne garantit pas cette marge : routes et
compteurs de proposition peuvent diverger sous cap. Les bornes théoriques
sont 1/4/10 formes par pivot selon |Q|=2/3/4, et 146 par appel natif avec
16 pivots, initialisation comprise ; ce ne sont ni observations ni latences.
Recherche du diamètre, puissances et repli restent du travail. Le cas n=2
défavorable n'est pas accéléré par ce filtre. Aucune structure globale ajoutée.

## Qualification à préparer après GO distinct

- Comparer à 0645aa00 les traces de supports acceptés et les champs littéraux
  complets à P non limitant, sur toute une séquence partageant Work.
- Tester q2 initial, q2→q3, q3→q4, q4→q3, remplacement d'un essentiel q4,
  permutations de Q et coquilles supplémentaires ; distinguer les fixtures
  locales des trajectoires réellement produites par l'initialisation native.
- Vérifier le filtrage stable des listes historiques, sans q2 de pivot ni
  charge des candidats supprimés ; conserver le départage diamètre/violateur.
- Rejeter les généralisations z intérieur, z sur la coquille et paire initiale
  non maximale. Tester qu'une mutation d'ordre changeant le premier support
  sur coquille intermédiaire est détectée, pas seulement l'égalité du rayon.
- Requalifier budgets P/L nuls, faibles, épuisés entre MEB et proches de MAX,
  charge prospective et exceptions. Le triangle équilatéral historique devrait
  proposer deux formes, contre cinq anciennement : attendu, non exécuté.
- Conserver replis F, sentinelles, refus, ordinal global 550 et 1 507 ordinaux,
  `support[0]`, niveaux q4 à trois limbs bruts ; mutants shell, ordinal et
  représentation q4 non réduite. Aucun ancien résultat n'est hérité.
- Compiler ensuite C++20 strict sans macro de test, puis sanitizers et juges
  indépendants normal/`-O`, dans des dossiers neufs avec sources/dépendances
  et captures brutes épinglées. Aucune commande de build n'est lancée ici.

Le SHA du nouveau helper est
`484a89bc2dbd472cc0571ed31d59631d5f31f9b0a425118040c916fc16e5abcf`.
Cette préparation ne prouve aucun gain FULL, contrat 50k/1 s ou 100 ms,
passage à des millions de points, ni résultat GPU.
