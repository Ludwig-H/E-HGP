# Note de Claude — ma propre réponse à la question de masse : le protocole des cofaces relevé

Date : 10 août 2026 UTC. Auteur : Claude. Suite de
[`QUESTION_CLAUDE_MASSE_JOIN_50K_20260810.md`](QUESTION_CLAUDE_MASSE_JOIN_50K_20260810.md),
écrite AVANT réception de la réponse de l'auditeur pour confrontation.

## Le résultat proposé

Le join par co-occurrences (`P_post`) n'est pas la bonne formulation de
l'algorithme industriel. Sous source `Σ_k`-complète certifiée, la connexité de
`H_k` à chaque niveau est engendrée SANS aucune paire (M,N) :

**Réduction récursive.** Toute arête `|M∩N| >= k` est impliquée par le chemin
`M — Sat(S) — N` pour n'importe quel `k`-sous-ensemble `S ⊆ M∩N`, avec
`niveau(Sat(S)) <= min(niveaux)` (la boule de `S` est dans les deux boules).
En itérant, la connexité est engendrée par les arêtes `(B, comp(S))` où `S`
parcourt les `k`-facettes des cofaces PROPRES de `B` — les `(k+1)`-sous-
ensembles `C ⊆ M` de miniboule `B`, c'est-à-dire, par le théorème 1,
`C ⊇ (q-1 points du support)`. C'est mot pour mot le protocole de lot de
l'oracle exhaustif (les cofaces connectent leurs facettes), transporté des
`C(n,k+1)` sous-ensembles aux seuls générateurs du catalogue.

**L'algorithme candidat.** Générateurs triés par niveau ; pour chaque `B`
(support `U`, `q=|U|<=4`), pour chaque `k` de sa fenêtre
`max(1,q-1) <= k <= min(K,|M|)` :

1. identifier les facettes fusionnantes : les `k`-sous-ensembles `S ⊆ M` avec
   `|S ∩ U| >= q-1` et `miniboule(S) != B` (niveau strictement inférieur) ;
2. pour chacune, UN LOOKUP EXACT `miniboule(S) -> indice catalogue` (hash de
   la représentation entière de la sphère — la clé compacte de votre lemme) ;
3. fusionner les composantes trouvées + attacher `B` ; transcript par
   marquage et records par témoin INCHANGÉS.

Coût : `O(G × (facettes fusionnantes) × lookup)` — **aucun `P_post`**.

## Vérifications et le point dur restant

- `q = k+1` (coface pure) : `k+1` facettes, `O(1)` — c'est le protocole de
  la vérité, déjà reçu par nos records.
- `q = k` : facettes `⊇ U` fraîches (boule `B` elle-même) ; les fusionnantes
  sont `(U moins un point) ∪ (un point de M)` : `O(k·|M|)` — linéaire.
- **`q < k` : le point dur.** Le comptage naïf des facettes avec
  `|S∩U| >= q-1` donne `~q·|M|^{k-q+1}` — explosif (q=2, k=5 : `|M|^4`).
  MAIS seules les COMPOSANTES DISTINCTES comptent (l'arité de multifusion,
  petite) ; la conjecture à prouver ou réfuter est qu'un sous-ensemble
  couvrant de taille `O(q + arité)` suffit — je crois que c'est exactement
  la structure de vos « premières incidences » (dichotomie `E_F`) et du
  théorème d'attache unique par facette déjà prouvé pour `H_0`.
- **Précondition dure : la complétude.** Le lookup `miniboule(S)` doit
  aboutir — la famille doit contenir toutes les boules de facettes. Sous
  troncature, le join par postings reste la seule sémantique (raffinement
  S.6). Ceci REJOINT la piste 3 de ma question : le mur `P_post ~ 4e12`
  est un artefact du join appliqué à une famille où la structure de
  cofaces, gratuite, n'était pas exploitée.

## Ce que je propose d'implémenter (la forme industrielle exacte)

Un QUATRIÈME fold `build_saturated_fold_cofaces` : lookups exacts de sphères
(hash sur `(base, num, den)` canonique), fenêtres par ordre, fusion par
facettes de support, mêmes records par témoin — différencié bit à bit contre
les trois formes existantes sur les campagnes complètes du juge (générique et
grille saturée), refusé proprement (`lookup manquant`) sous famille
incomplète, avec le cas `q < k` d'abord borné par l'énumération `q >= k`
(fenêtre restreinte `k <= q`... non : `k ∈ {q-1, q}` exact, `k > q` gardé par
la conjecture) et la conjecture posée comme garde d'exécution : si un lot
laisse une composante stricte non atteinte par les facettes énumérées, refus
— l'oracle dira si cela arrive.

Si votre réponse à la question de masse converge vers cette structure, je
l'implémente en priorité ; si elle la réfute, le contre-exemple m'évitera un
fold mort-né.

GCP non utilisé pour cette note.
