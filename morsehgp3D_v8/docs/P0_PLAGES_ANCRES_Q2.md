# Partager des plages d'ancres, pas une pile par petite requête

15 septembre 2026. Tranche18 implémentée, exploration v8 hors
registre, cpu_reference, quantized_u16_input_only, implementation_v8_p0,
not_claimed. Périmètre : front et flux de supports q2, pas la tour FULL.

## Pourquoi changer la granularité

La tranche17 permet de céder des branches intérieures du census dans
une équipe persistante. Elle conserve exactement le travail géométrique,
mais ne gagne pas de temps dans les mesures closes. Sur les petits
rectangles dominants, aucune branche assez grande ne se présente.
Forcer chaque racine à devenir une continuation crée au contraire une
allocation et une pile réservée de49 cadres par appel.

L'[audit indépendant B](../audits/DIALOGUE_AUDITEUR_B.md) confirme ce
diagnostic sur beee3341 : environ38 à45% de surcoût à un fil quand
toutes les ancres deviennent des continuations, beaucoup plus avec un
quantum de1. Ce résultat motive une autre structure ; il n'est pas une
qualification du nouveau code.

Une plage indique simplement quelles ancres restent à traiter. Elle ne
contient aucun parcours Z commencé. Le receveur utilise son moteur privé
et ses tampons réutilisables. Le don d'une moitié de plage évite donc de
créer une pile de continuation par ancre. Le dernier census d'une ancre
reste atomique : ce mécanisme et le détachement intérieur résolvent deux
granularités différentes.

## Invariants de la refonte

- Une paire n'appartient qu'à une plage active ou pendante. On ne cède
  qu'un suffixe d'ancres non commencées, jamais une ancre en cours.
- Le rectangle charge sa masse candidate et son descripteur une seule
  fois ; les workers chargent les vraies racines, visites et sorties.
- Les ancres Shared sont des rangs spatiaux, pas des IDs originaux.
  Le contexte Complement garde le B original du rectangle.
- Une bande Pool parcourt des positions de classes A et un préfixe de
  l'ordre B propre au plan. Elle n'est pas un nœud de l'arbre spatial.
- Le plan Pool est préparé une fois par rectangle et possède, à travers
  son parent, le même index immuable. Aucun crédit ne seed le census.
- Sans rejet Pool, conserver le parcours Shared au lieu de développer
  le produit en paires. Les ancres de ce repli peuvent être partagées.
- File pleine ou verrou occupé : poursuite locale, sans attente de place
  ni abandon. Fin : seeds attribuées, file vide, aucune activité.
- Annulation : réveiller les receveurs, joindre tous les workers et
  restituer l'erreur ; les émissions antérieures ne sont pas annulées.

L'API nouvelle est `run_wspd_q2_census_ranges`, séparée de Coarse et de
l'entrée à continuations. Son grain par défaut vaut64 ancres. Une plage
éligible possède plus de ce grain d'ancres restantes ; elle propose un
don à son entrée, puis tous les64 traitements, tant qu'elle reste
éligible. Le suffixe cédé porte la moitié entière des ancres restantes.
Le grain ne plafonne ni l'entrée, ni la recherche, ni les sorties.

La file préalloue des valeurs de plage, non des piles de census. Un
parent Pool peut être partagé par plusieurs entrées sans copie de son
plan. Les plans sans filtrage sont libérés avant le parcours Shared.
Au plus Q+W parents distincts sont nécessaires pour Q places de file et
W workers : tout parent vivant est retenu par une activité privée ou
une entrée pendante. Cette borne ne comprend pas les tampons utilisateur,
le nuage, les index, les allocations temporaires et les piles natives.

Attention au champ historique `pool_peak_bytes_sum` : il conserve la
somme des maxima de préparation de chaque worker, pas une borne de la
mémoire simultanée lorsque ses anciens plans survivent chez les receveurs.
La nouvelle entrée rapporte séparément les maxima de parents inscrits
simultanément et leurs capacités retenues, sans compter deux fois les
références à un même parent.

Ces intervalles commencent après construction complète et finissent à
l'entrée du destructeur. Ils omettent donc les allocations temporaires,
les queues de construction/destruction et les blocs de contrôle des
`shared_ptr` : il s'agit de mémoire retenue inscrite, pas du pic physique.
Le suivi verrouillé final remplace les atomiques séparées du premier
instantané lu par B ; ses mesures restent attachées à cet instantané.

## Coûts à conserver dans les preuves

Comparer les supports, intérieurs, coquilles et clés complets aux oracles,
puis tous les compteurs géométriques et Pool à Coarse. Les nouveaux
compteurs portent seulement le partage des plages : créations initiales,
transferts, travail terminé, consultations de file et mémoire.

La préparation Pool reste O(KF), avec F la somme des tailles de facteurs
sélectionnés. Aucun scan ni plan n'est recréé par plage. Le parcours des
bandes ne développe que le résidu ; le repli sans rejet ne développe pas
systématiquement son produit cartésien. Cela supprime un coût de gestion,
pas une preuve générale de complexité du front et du census.

Noter A le nombre d'ancres réellement traitées après sélection et R le
nombre de plages initiales, hors ancres Pool saturées. Toute division
conserve deux plages non vides : son arbre binaire a au plus A feuilles,
donc au plus A−R dons. Chaque plage effectue au plus une consultation à
l'entrée, puis une par grain d'ancres traitées. Le coût de gestion de
ces consultations est donc O(A+R), sans scan des ancres cédées. Cette
borne ne borne pas A en fonction de n, ni les visites géométriques.
Les populations `donated_pairs`/`donated_anchors` cumulent les transferts,
qui peuvent déplacer plusieurs fois du travail futur : ce ne sont pas
de nouvelles paires testées ni des boucles parcourant ces populations.

Les temps Pool de cette nouvelle entrée doivent être la préparation plus
les intervalles actifs des plages, sommés entre workers. Un intervalle
parental englobant du travail déjà compté chez les receveurs le compterait
deux fois. La durée mur globale inclut préparation, équipe, file, travail,
jointure, réduction et destruction, hors préparation initiale du nuage.

Mesurer n8k/16k/32k, s8/10/12, K5/10 et un/quatre workers. Séparer les
familles à petits rectangles des gros replis des rangées, et publier les
croissances des coûts de gestion même lorsqu'elles dépassent×4.

## Statut

Qualification fonctionnelle propre close :72 CTests Release et Clang
ASan/UBSan, plus la gate Clang ThreadSanitizer. La nouvelle gate compare
415 appels ranges,135 Coarse et dix appels avec options par défaut sur
dix nuages. L'oracle indépendant examine10 920 paires et778 110 sites ;
126 065 supports sont comparés. La coquille de30 sites, les trois types
de plage, les callbacks concurrents, le retour sur exception et la
possession au-delà du pointeur appelant sont exercés. Une fixture à quatre
nappes exerce les transferts de bandes Pool effectivement filtrées.

La gate ne fixe pas un nombre minimal de dons dépendant du scheduling :
leurs compteurs réels figurent dans chaque reçu. Il n'y a pas d'injection
d'échec d'allocation du nouveau parent, ni de preuve que le receveur
dormait déjà à l'instant de l'exception. L'ancre entière, y compris ses
paires Pool et leurs coquilles, reste indivisible dans ce répartiteur.

Les [174 mesures propres](../receipts/q2_anchor_ranges_20260915/README.md)
et leurs lectures/analyseurs normal/−O sont closes. Elles gardent exactement
géométrie et sorties :278 dons,208 Shared et70 replis,52 après attribution
des seeds. Le partage Pool filtré reste absent de ces grands nuages à
grain64, même s'il est positif dans les gates. La tâche fait72 octets sur
ce build. Six postes principaux sous×3 aux doublements sur Pool64, mais
F rangées×4 et quinze ratios de scheduling W4>×4 ; aucune borne générale.

Les trois builds `v8_anchor_ranges_20260915`,
`v8_anchor_ranges_sanitize_20260915`, `v8_anchor_ranges_tsan_clang_20260915`
dans `build/` sont désormais épinglés. Aucun gain stable sous charge
concurrente, résultat GPU ou contrat FULL/G4 acquis. GCP non utilisé.
Les anciennes API et leur défaut restent inchangés. Les états singleton
compacts pour GPU et les lanes q3/q4 ne sont pas implémentés par ce texte.
