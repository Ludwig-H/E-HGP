# QUESTION aux auditeurs — cible du palier CompactDelta après la sonde d'ablation

Date : 2 septembre 2026. Pièces : `NOTE_CLAUDE_SONDE_ABLATION_REDUCE_20260902.md`
(reçu `receipts/sonde_ablation_reduce_20260902/`), arbre § 5.10, § 5.17,
design pré-enregistré « palier 3 CompactDelta synchrone » et sa critique
(deux sceptiques, 2 septembre). `public_status=not_claimed`, aucune décision
ici — je demande un verrou avant d'écrire.

## Fait mesuré

Sur `uniform` 8000/16000/32000, Σ_K, médianes de trois répétitions : la
**copie profonde `scratch → r.deltas`** (deux allocations par delta + copie
des clés 44 o) pèse 54–59 % de `materialisation_tri_copie` (part croissante
avec n), les tris 44 o 25–27 %, la lecture aléatoire `keys[fid]` du
remplissage 73–75 % de `post_remplissage`. Le palier 3 tel que pré-enregistré
conserve la copie (déplacée en passe finale) : son gain est borné à ≈ 10 %
du reduce, là où le poste dominant vaut ≈ 22–24 % du reduce Σ_K.

## Question 1 — représentation CSR de `r.deltas` (forme du payload, même objet)

Proposition : par K, `ForestResult` porte `deltas_meta` (batch, level,
output, offsets `parents_off`/`born_off`) et deux arènes de `FacetKey`
(`parents_keys`, `born_keys`), remplies **dans la boucle des lots, au même
instant et dans le même ordre** qu'aujourd'hui (`sort(post_list)`, parents et
nés triés, continuation et `drop-nonmerge` identiques, `output =
keys[canon post-lot]`), sans passe finale. Le digest v4 canonique lit les
mêmes octets dans le même ordre (le `digest_forest` v4 itère les deltas et
leurs listes — l'itérateur change, pas la séquence digérée) ; `on_forest`,
le rendu § 9.1 et les portes reçoivent une vue par delta (`span` sur les
arènes). L'ablation « sans copie » est la borne haute de ce que cela
supprime (jusqu'à −59 % de la fenêtre) ; le coût restant est une
`memcpy` par liste dans une arène pré-réservée à `Σ|pre_list| + Σ|touched|`
(bornes déjà prouvées par `fold_capacity_ok`).

Est-ce recevable comme **changement de forme sans changement d'objet**,
prouvé par : digests `digest_all`/`digest_forest_K*` bit-identiques sur
`conformite_v5` (8 portes gate + échelle), `mhgp6_profil_identite`, matrice
fils {1, T} × inflight {1, 2} × join {0, 1} sur le témoin complet
(`forest_witness`), mutants `csr-shift-offset`, `csr-drop-delta`,
`csr-dup-delta`, `csr-unsorted-born`, `csr-unsorted-parents`,
`csr-stale-output` tués sur les fixtures S1/S2/S5 gravées (celles de la
critique, continuation et multi-parents incluses), comparateur
`first_divergence` à ordre gravé ? Ou exigez-vous que `ComponentDelta`
(vecteurs par delta) reste l'interface publique du fold, auquel cas le
palier 3 est confirmé avec son gain borné et sa finale placée **après**
`liberation` (pic −400 Mo) comme résultat primaire ?

## Question 2 — critère pré-enregistré

Si la CSR est recevable, je propose de graver **avant** la mesure :
`materialisation_tri_copie[csr] ≤ 0,55 × materialisation_tri_copie[classique]`
à K ∈ {8, 9, 10} sur au moins deux des trois tailles, `post_remplissage`,
`touch`, `pre`, `unite`, `partition` dans ±3 %, `rss_max_kb` (ru_maxrss,
`join=1`) non supérieur, mur `mhgp6` Release non instrumenté (`join=0`, 8
fils, trois répétitions) en baisse — sinon rien n'est conclu. Les tris u32 et
la clé chaude du remplissage (les deux autres postes) resteraient un palier
ultérieur, séparé, avec son propre reçu.

## Question 3 — lecture `keys[]` du remplissage

73–75 % de `post_remplissage` est la lecture aléatoire de `keys[fid]`. Une
variante de la CSR stocke les **fids** (u32) dans les arènes et ne convertit
en clés qu'à la lecture (digest/rendu) par `facet_keys[fid]` : elle supprime
la lecture du remplissage et divise par 11 l'empreinte des arènes, mais
déplace la conversion vers les consommateurs. Souhaitez-vous que cette
variante soit mesurée dans le même reçu (deux bras : arènes de clés, arènes
de fids) ou tenue pour un palier séparé ?

GCP non utilisé par cette question.
