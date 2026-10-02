# Lecture racine du Pool — v11 WIP et critique de R2

Capture avant calcul le 2 octobre 2026 à19:59:59UTC : worktree développeur
Git7f1922, 17 fichiers présents et un chemin explicitement absent
(`src/core/memory.hpp`). La copie BEFORE et ses hashes précèdent la lecture.
Les quatre sources R2 annoncées sont ensuite copiées et vérifiées à
`865f5e64ddd08bedf6ab8f94e8bb94812e380e79` : tous les pins correspondent.
Aucun import ni exécution de code v10/v11, aucun produit modifié, aucun GCP.

La v10 R2 fournissait deux briques utiles : CAS saturant de reclamation,
nettoyage/jointure des constructions partielles. En revanche son contrat
normalisait grain0, employait un drapeau TLS avec repli série imbriqué, une
std::function empruntée, et conservait la première exception arrivée en
interrompant les prochaines tranches. La première exception et le travail
réalisé peuvent dépendre de l'ordre d'arrivée ; c'est décrit par son contrat,
pas un nouveau défaut attribué à ce pin. Ces conventions ne satisfont pas
par simple copie la réduction déterministe et les frontières d'erreur v11.

La nouvelle lecture est favorable : garde active membre non bloquante,
refus concurrent/réentrant y compris vide/W1, workers explicites, callback
sans allocation par appel ; toutes les tranches rendent une issue fusionnée
par merge, même après refus ou exception. Les deux conversions distinguent
bad_alloc/resource_exhausted de task_exception/invariant_violated. Le Pool
n'annule pas les effets du callback : le catalogue doit publier seulement
au succès global et écrire suivant les ordinaux, jamais les workers.

La barrière exige tous les W−1 acquittements, y compris sans tranche. Chaque
worker a ainsi vu l'époque avant sa prochaine bascule booléenne. Le mutex
qui publie/capture Job puis protège les acquittements synchronise les écritures
privées d'Outcome avant leur réduction. Job, callback et contexte restent
empruntés jusqu'au retour. Ne pas reprendre de la v10 le seul comptage des
utilisateurs déjà entrés tout en conservant la nouvelle époque booléenne :
un worker retardé pourrait manquer deux bascules et attendre indéfiniment.
C'est une garde de composition, pas un bug du nouveau code.

Les portes natives présentes couvrent11 groupes : domaine, couverture,
frontières UINT64_MAX avec juge u128, travaux courts, réentrance/concurrence,
issues, exceptions ; création de threads, allocation et absence d'allocation
par appel. L'injection pthread/new appartient seulement au harnais. Huit mutants
ciblent grain, couverture, réduction, arrêt prématuré, conversion et garde busy.
Leurs résultats natifs restent à qualifier ; nous ne les avons pas exécutés.

Conseil au raccord : conserver workspaces et domaine stables jusqu'au retour,
preadmettre les buffers simultanés, lancer les phases par tâches assez larges.
Éviter d'appeler ce même Pool depuis un callback : le refus busy est explicite,
il n'y a plus de repli série. Aucun gain temporel ni FULL ne découle du Pool seul.

check.py est autonome :21 copies vérifiées, six permutations d'issues donnent
un résultat de réduction totale contre trois premiers arrivés possibles,
16 domaines scalaires aux limites. Normal/−O identiques, aucun natif. Ce modèle
illustre les contrats ; il ne simule pas les atomiques C++ ni un scheduler réel.
SOURCE_AFTER.json confirme toutes les sources LIVE inchangées à cette lecture.
