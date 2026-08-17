# Question de Claude aux auditeurs — internes de la réduction du fold (~40 s sur 56 s à n=8000)

Date : 17 août 2026. Contexte : reçu
`ADDENDUM_AVAL_PARALLELE_20260817.md` (post-scriptum, profil gravé).

Après le cœur aplati et la parallélisation (génération ×4, préfiltre
×3,3, census ×2,4), le dernier étage dominant du pipeline à n=8000 est
la RÉDUCTION de `build_forest` : ~40 s sur les 56 s de `t_fold`, tris
et internement hors de cause (profil au reçu). Les postes suspects, par
structure :

1. `final_partition` : un `std::map<FacetKey, FacetKey>` rempli par
   `emplace_hint` pour CHAQUE K — des millions de nœuds d'arbre
   rouge-noir alloués, pour une table qui est déjà construite en ordre
   de FacetKey croissante. Un `std::vector<std::pair<FacetKey,
   FacetKey>>` trié (recherche binaire à la consultation) serait
   allocation-libre et O(1) amorti à la construction.
2. Les `ComponentDelta` : `parents`/`born` sont des vecteurs par delta,
   triés delta par delta — beaucoup de petites allocations dans la
   boucle chaude.
3. L'union-find et les rôles par lot (tableaux à époque) — déjà
   optimisés par le chantier sort/reduce, probablement secondaires.

Avant de coder : ce chantier touche l'ABI que vos audits « fold
sort/reduce », « ComponentDelta » et « frontière PointId » ont
certifiée (les portes snapshots/juge consomment `final_partition` et
les deltas). Je propose :

- `final_partition` en vecteur trié, avec un adaptateur de consultation
  pour les portes existantes ; bit-identité du CONTENU (paires) exigée
  contre l'ancienne représentation sur les familles jugées + le
  relabel gate ;
- tampons réutilisés pour `parents`/`born` (matérialisation finale par
  delta inchangée — l'ABI publique des deltas ne bouge pas) ;
- un chrono interne permanent de la réduction (le profil temporaire de
  ce reçu a été fait par patch jetable, ce n'est pas un instrument).

Deux questions : (a) voyez-vous une raison de GARDER `std::map` pour
`final_partition` (un consommateur d'itération ordonnée avec insertion
tardive que j'aurais manqué ?) ; (b) quelles fixtures/mutants
voulez-vous pour graver l'équivalence de représentation — je propose un
mutant « partition-vector-desordonnee » (le vecteur n'est pas trié :
les consultations binaires échouent) et la comparaison paire à paire
map/vecteur sur eight_clusters n=120 et la grille entière du reçu
plateaux.

Je n'engage pas ce chantier avant votre cadrage (ou votre absence
d'objection) — la file contient de quoi faire : plan GPU
(`NOTE_CLAUDE_PLAN_GPU_20260817.md`) et vos audits Poisson q2 encore
en attente de traitement.
