# R2 : progrès vérifiés, collision de sortie et raccord encore distinct

30 septembre 2026. Copie privée `/tmp/mhgp10-r2`, après la réponse
`e9eab2754`. Audit sans modification du moteur, aucun GCP. Les preuves sont
dans [le paquet R2](../../receipts/audit_continu_20260929/r2_followup_20260930/README.md).
Le nouveau [contre-exemple frontière](AUDIT_LAMINARITE_POINTS_20260929.md)
est en section 10 ; il doit entrer dans la porte de conception annoncée.

## Ce qui est maintenant contre-vérifié

| Groupe | Résultat et portée |
| --- | --- |
| Juges | Mon rejeu normal/−O, code 0 : cinq corruptions de listes/ordre auparavant acceptées sont refusées, ainsi que l'attache morte et le plateau ternaire binarisé. Les trois objets valides et les contrôles positifs restent correctement jugés. Aucun nouveau producteur lancé. |
| Tête | Porte native R2 rejouée ici, code 0 en 0,825 s, hashes ciblés stables. Domaine numérique 26/26 fixtures et 25/25 appels invalides sans sortie. La racine zéro est protégée indépendamment de min_cluster_size ; M·λ_max est comparé exactement avant condensation. |
| Coût de tête | Journal développeur observé : témoin temporel code 0, trois retours quadratiques tués par le watchdog de 15 s. Ces mutants ne sont pas relancés ici ; le compteur interne reste un diagnostic. |
| Pool | CAS saturant et chemin série contrôlent les réclamations sans overflow, même avec grain u64 maximal. Les formes alignées new/delete sont testées. Captures terminées observées : ASan 11 commandes, TSan 10, sondes 27, toutes code 0. Cela ne ferme pas toute la campagne ni les différentiels. |

Le premier relevé CLI concluait trop vite à l'absence de build tête R2,
à cause d'une énumération de chemins trop étroite. Cette conclusion est
**corrigée** par le build et la porte maintenant observés. Le relevé brut
reste identique, avec un erratum explicite dans son README de publication.
La source du header a aussi évolué : le reçu de tête donne son SHA actuel.

La campagne R2 des juges est désormais terminale : 33/33 CTests,
`ctest_gate rc=0`, 406,69 s, journal copié sans rejeu. Les 53 changements
de témoin survivant au fuzz sont classés par le développeur comme
représentations équivalentes de même MEB et même composante Γ ; ils ne
sont pas tous des corruptions géométriques. Cette observation ne remplace
pas notre rejeu causal des sept nouveaux refus ci-dessus.

Le mutant de pool `begin<=n` a déjà neuf délais 124 : six portes de 120 s
et trois injections de 300 s, soit 1 620 s de délais programmés pour la
même réclamation vide répétée à n. C'est un défaut du mutant, pas du pool
corrigé. Pour les campagnes suivantes, arrêter cette variante après sa
première non-progression causalement reconnue ; les répétitions ajoutent
peu de preuve. Aucun processus tiers n'a été arrêté par cet audit.

## Nouveau défaut concret : deux sorties vers un même fichier

Sur cinq sites, K2, min_cluster_size=2 et W1, le binaire de la copie CLI
accepte le même chemin pour les étiquettes et `--tree`. Il rend code 0 et
`status=ok`, mais le texte de l'arbre, 126 octets, écrase les 20 octets
d'étiquettes. Le contrôle sans `--tree` écrit les 20 octets attendus.
Les deux appels réels, données, contenus et hashes stables sont conservés.

`OutputSet::open` déduplique le nom sans distinguer les rôles logiques,
puis l'écriture retronque le même fichier. Refuser les destinations
concurrentes **avant réservation**. Documenter aussi les alias de fichier
et les collisions entrée/sortie. Ce défaut d'intégrité de sortie ne prouve
aucune erreur de géométrie.

## Deux conditions de raccord à ne pas perdre

La copie CLI garde l'ancienne tête. Elle n'est donc pas un contrôle de
l'interface après les nouveaux refus numériques. La nouvelle tête rend
des tableaux vides en cas de refus ; le raccord doit conserver sa
propagation de `Outcome` **avant** tout accès aux étiquettes, ainsi que
la validation conjointe avant écriture. Ses tests et ceux de la CLI doivent
être alignés : z=0 et un singleton K1 à niveau zéro ne peuvent pas rester
des acceptations universelles.

Inversement, conserver les écritures vérifiées de la copie CLI lors du
port de cette propagation : la copie tête ne contient pas encore tous
les correctifs d'entrée/sortie. Une fusion de fichiers ne vaut pas un
rejeu sur le binaire commun. Rejouer la collision, les refus numériques,
les mutants des juges et la borne CSR de l'autre prototype après raccord.

Ces gains de fiabilité ne qualifient ni une nouvelle projection frontière,
ni un score ARI/EOM, ni FULL/GPU en 100 ms. Les campagnes encore actives et
les résultats historiques ne sont pas promus.
