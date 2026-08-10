# Question de Claude — la masse du join est-elle réductible sous le contrat ?

Date : 10 août 2026 UTC. Auteur : Claude (développement). Cadre :
`phase=exploration_v3_hors_registre`, suite du sweep
[`NOTE_CLAUDE_BENCHMARK_POSTINGS_FORECAST_20260810.md`](NOTE_CLAUDE_BENCHMARK_POSTINGS_FORECAST_20260810.md).

## Le constat qui motive la question

En réexaminant la réduction `Σ_k` (théorème 2 de la note `q_min`) pour
l'implémenter, je bute sur ceci : en 3D, `q <= 4`, donc pour `k >= 3` on a
`q <= k+1` TOUJOURS — `Σ_k` contient tous les générateurs de capacité
suffisante et ne retire rien. Dans le join PARTAGÉ (une seule émission, unions
fenêtrées par ordre), retirer les `q > k+1` des `DSU_1` et `DSU_2` réduit des
unions, pas l'émission : la masse `P_post = Σ_x C(d_x, 2)` reste entière dès
que `K >= 3`, parce que les poids exacts `w = |M∩N|` exigent de compter chaque
co-occurrence. La réduction en arbre à `k=1` a le même statut : elle réduit
les arêtes de connexité, pas le comptage des poids.

Le sweep dit `P_post ~ 4e12` à 50 k sous famille tronquée — et le contrat
final est 100 ms pour toute la chaîne. Le join par co-occurrences est donc
mort sous le contrat, dans toutes ses formes, sauf erreur de ma part.

## Trois pistes que je soumets à réfutation ou construction

1. **S.5 par Borůvka sans énumération des paires.** La forêt couvrante de
   poids max pondérée `|M∩N|` compresse tous les ordres (S.5) en `G-1`
   arêtes. Kruskal exige les poids triés — retour à `P_post`. Mais un pas de
   Borůvka n'exige que : pour chaque composante, UN voisin de poids maximal.
   Existe-t-il une structure exacte (sur u16, avec les saturés triés) qui
   réponde à « quel générateur hors de ma composante partage le plus de
   points avec l'un des miens ? » sans balayer les co-occurrences — par
   exemple par la géométrie des boules (deux saturés de grande intersection
   ont des boules proches au sens de l'inclusion des lentilles) ?
2. **Le poids plafonné `min(w, K)` suffit.** Les seuils n'utilisent que
   `w >= k <= K`. Une co-occurrence au-delà de la K-ième d'une paire est du
   travail perdu. Y a-t-il une émission « à saturation » exacte — chaque
   paire cesse d'émettre après K témoins — dont le coût serait
   `Σ_paires min(w, K) = O(K · #paires intersectantes)` au lieu de `Σ w` ?
   `#paires` vaut 168 M à n=200 contre 385 M d'occurrences ; le gain est
   réel mais borné (~2×), et je ne vois pas comment ARRÊTER une paire sans
   l'avoir identifiée — c'est peut-être un faux espoir à réfuter vite.
3. **La statistique de la vraie source.** `d_x ~ 4-5 k` est un artefact de
   la famille tronquée `smax=11` sur des nuages denses : chaque point tombe
   dans des milliers de petites boules de rang <= 11. Le contrat 50 k
   porte-t-il sur cette famille-là ? Si la sortie contractuelle est la forêt
   des K niveaux pour K ~ 10, la famille PERTINENTE (généateurs des
   `Γ_k`, k <= K) a-t-elle un `Σ_x C(d_x, 2)` d'un autre ordre de grandeur —
   autrement dit, le mur est-il un artefact du générateur tronqué plutôt
   qu'une propriété de la cible ?

Si les trois pistes tombent, la conclusion honnête serait : le join exact à
50 k est un calcul en heures-machine distribué (GPU + runs bornés), pas un
calcul sous la seconde, et le contrat 100 ms doit viser une autre
formulation. Je préfère cette phrase réfutée par vous que gravée par moi.

GCP non utilisé pour cette note.
