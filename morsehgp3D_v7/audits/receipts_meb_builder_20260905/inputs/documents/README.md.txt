# Helper MEB dual-budget — overlay privé de port

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=port_prive_non_compile`, `public_status=not_claimed`.

Préparation uniquement : aucun build, test C++, moteur ou port produit. GCP non utilisé ; aucun Git, source F, prototype historique ou gate géométrique modifié. Les qualifications des deux prototypes ne sont pas transférées à ce nouveau header.

## Autorités et diff

- `build/v7_meb_pivot_prototype/pivot.hpp`, SHA `d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5` : Candidate/power, point, form, choose, ordinal et expressions de finalisation.
- `build/v7_meb_dual_budget_prototype/pivot.hpp`, SHA `0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d` : identifiant, Limits, Work, NoObserver, Attempt, charged_form, small_ball et propose.
- `port_from_0645.patch` est le diff unifié exact entre le fichier 0645 entier et `meb_proposal.hpp`. Les formes ajoutées viennent de d6db ci-dessus ; les anciennes fonctions miniball et la vieille proposition sans budget ne sont pas portées.

Namespace unique : `mhgp7::meb_proposal_detail`. Les includes `src/lanes/q2.hpp`, q3/q4 et `src/tree/cloud_index.hpp` demandent la racine source F dans le chemin d'inclusion de l'overlay. Un port produit devra adapter explicitement ces chemins ou son include-root ; cette préparation ne modifie pas CMake.

## Transformations exactes

Candidate/power, point, form, choose et ordinal sont des sections littérales de d6db. Les trois algorithmes budgétés sont des sections littérales de 0645, sauf `pivot_prototype::form` devenu `form` dans le même namespace. Leurs boucles, décisions, bornes, compteurs et ordre restent inchangés.

Seule adaptation de finalisation : `materialize<OutBall>` remplace le type concret LocalBall et initialise `OutBall ball{}`. Les expressions q2/q3/q4, niveaux bruts et ordre du support restent ceux de d6db ; l'initialisation par valeur préserve les zéros des slots inutilisés pour LocalBall. OutBall doit fournir les champs compatibles `q`, `support[0..3]`, `key` et `level`. Aucun déplacement ou include de LocalBall, aucun Builder, aucune méthode miniball, aucun statut/refus legacy dans ce helper.

Les ajouts restants sont includes explicites, namespace, provenance/préconditions en commentaires et alignement de la signature templated. Deux commentaires d'état précisent que le repli et les observations terminales appartiennent à l'appelant.

## État et contrat à raccorder par le constructeur

Work garde les quatre champs de 0645 pour ne perdre aucune observation : `meb_proposal_supports`, `pivots`, `certified`, `fallback`. Le helper modifie seulement les deux premiers. L'appelant possède Work par tentative/ordre et le conserve entre toutes les MEB et les replis ; il actualise les deux autres selon les mêmes points de décision que 0645. Trouver un certificat ne signifie pas que le budget ordinal l'acceptera.

Limits conserve P=0 par défaut ; propose s'arrête alors avant la recherche de paire. L'appelant garde la priorité du cap legacy, le comptage meb_calls une seule fois, le transfert ordinal avec garde anti-overflow, le repli F inchangé et le compteur effectif A séparé. Aucun de ces raccords n'est simulé dans le helper. Une proposition échouée conserve le Candidate de sortie, mais garde les charges P/pivots déjà acquises.

NoObserver et les templates ChargeAfter restent présents sans macro TESTING ni branche désactivant le chemin nominal. L'instanciation nominale charge avant la forme ; le mutant privé charge après. Aucun nouveau mutant géométrique, contrôle flottant ou changement de certificat n'est ajouté.

Préconditions internes, pas API d'admission hostile : CloudIndex u16 valide sans positions dupliquées ; indices sites valides ; n2..11 pour la proposition ; q2..4 et slots distincts valides pour form/ordinal. Un support à finaliser doit être construit/certifié, pas un Candidate arbitraire. Gardes locales conservées : sous-ensemble2..5, support≤4, pivots≤16, coquille finale exactement q. La priorité du cap legacy ne peut être prouvée par ce helper seul puisqu'elle appartient à l'aiguillage futur.

GO produit encore conditionné à la géométrie, à une vérification sans observateur hors chrono et à un coût utile observé, puis au raccord/API/schémas/reçus décrit par le plan d'intégration. Aucun gain ni SLO revendiqué.
