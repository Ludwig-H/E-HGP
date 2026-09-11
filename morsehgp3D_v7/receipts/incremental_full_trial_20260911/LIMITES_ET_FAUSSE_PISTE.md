# Une couture correcte n'est pas encore une optimisation rentable

11 septembre 2026. Le raccord incrémental privé `a2c6ab39…` est conforme aux
comparaisons physiques et aux juges O2/SAN exercés. La fausse piste réfutée par
le micro n'est pas le principe du journal incrémental ; c'est l'attente qu'en
**retirant seulement la rétention des batches owning**, on retire d'emblée les
millions d'allocations par action ou que l'on gagne nécessairement en pic.

Les batches temporaires allouent toujours leurs actions, parents et références.
Le nouveau propriétaire `append_population(const&)` recopie en outre les
buffers temporaires que l'ancien Builder déplaçait dans son vector. Enfin,
les réservations amorties des arènes évitent un risque quadratique, mais leur
capacité finale est plus élevée que les réservations exactes tardives nominales.

À n800 : +366 021 appels new, +10 933 996 octets demandés cumulés,
+17 340 480 octets retenus finaux. Le pic des octets demandés vivants ne baisse
que de 955 584 octets ; à n400 il monte. Le RSS processus de la série baisse,
mais il inclut le census et les autres tailles, et ne peut pas être substitué
à une mesure du pic FULL à n800. Les temps bruts ne sont pas un speedup établi.

La prochaine couture utile peut être un lot plat réutilisable et un singleton
emprunté, ou un append de population transférant une ligne privée sans alias
mutable survivant. Ces changements doivent conserver l'ordre des premières
contributions, l'unicité du propriétaire, les gardes whole-lot et l'échec global.
Une compaction unique d'arènes en fin d'ordre est un autre compromis possible,
avec coût et pic transitoire à mesurer ; aucune réserve exacte par lot ne doit
être introduite. Ces options sont **non implémentées/non qualifiées ici**.

Le journal n'évite pas les structures nécessaires au resolver : programmes,
ancres, deux histoires adjacentes, verticales, populations et arènes finales
subsistent. Il n'introduit ni mosaïque Delaunay, ni catalogue Gamma exhaustif,
ni garantie universelle sous-quadratique. Le gain doit être mesuré sur la
sortie utile, pas déduit d'une disparition de classe C++.

Deux échecs de test sont conservés séparément : refus LSan/ptrace dans le
contexte subagent, puis interposition incomplète de new(nothrow) dans le
harnais de pannes. Le replay ROOT et le wrapper additif v2 les résolvent sans
modifier le raccord ni désactiver les contrôles sanitizer. Aucun défaut
nominal du candidat n'a été observé dans les gates closes.

GCP non utilisé, aucun contrat 50k/1 s ou 100 ms acquis.
