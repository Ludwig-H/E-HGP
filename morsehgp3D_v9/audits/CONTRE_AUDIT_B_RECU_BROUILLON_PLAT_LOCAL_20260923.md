# Contre-audit B — reçu local du brouillon FULL plat

23 septembre 2026. Relecture **sans GCP** du reçu local
`tower_flat_draft_local_20260923` dans le worktree développeur, commit
`5f36d5536`. Le code mesuré est `f93dc1659`, comparé au chemin de
phase A allégée `aa29245f`. Ce reçu n'est pas encore un essai G4.
Les **11/11 SHA** du fichier `SHA256SUMS` passent : README et dix JSON.

Les dix JSON représentent cinq couples sur **une seule trame sans sol**
08/000000, grille 1 mm/u18, s8, W8 et tour statique 8 : K5 trois
couples, K10 deux. Tous annoncent `complete_relative`. À l'intérieur
de chaque couple, nombre de boules, comptes par ordre, `tower_work` et
condensé FULL sont égaux. Le catalogue clé par clé et le payload FULL
explicite ne sont **pas** sérialisés dans ce reçu ; l'égalité des
condensés 64 bits n'est pas une égalité octet par octet de la tour ni
une preuve d'absence de sorties manquantes. Quelques compteurs de
visites/cache q3/q4 divergent entre bras, malgré les mêmes résultats
agrégés ; leur travail physique n'est donc pas strictement apparié.

| Contrôle local | Allégée → plate | Lecture correcte |
| --- | ---: | --- |
| Tour K5, trois couples (ms) | 1737→1670 ; 1698→1668 ; 1701→1628 | gain **1,7–4,3 %**, bruité |
| Phase A de K5 (ms) | 502→438 ; 514→480 ; 550→455 | −12,7 %, −6,6 %, −17,3 % ; médiane 514→455 |
| Tour K10, premier couple (ms) | 14705→11327 | −23,0 %, **une** paire exploitable |
| Phase A de K10, premier couple (ms) | 2706→1464 | −45,9 % sur cette paire |

La deuxième exécution allégée K10 est un outlier sous forte contention
(27 892 ms contre 14 705 ms à la première). Les formules du README
« K5 −10 à −13 % phase A » et « K10 environ −35 % phase A » ne
décrivent pas les pourcentages **appariés** ci-dessus ; elles ne doivent
pas être reprises comme constat précis. Le RSS affiché (~1 020→975
Mio à K5, ~4 185→3 700 Mio à K10) est indicatif, non une courbe de
crête répétée sur G4.

La provenance archivée épingle les dix JSON mais **pas** les binaires :
leurs SHA ne sont donnés que par préfixe, sans binaire ni commande
exacte. L'entrée n'a pas de SHA complet dans ce reçu ; le journal
d'entrelacement est absent. `152/152 portes` est rapporté dans le
README, sans log de ces portes à relire. Le [préflight source
B](CONTRE_AUDIT_B_BROUILLON_PLAT_FULL_WIP_20260923.md) reste applicable
à `5f36d5536` : l'overload public de `FullCoverageFlatDraft` lit des
offsets CSR modifiables avant validation. Les sorties du producteur
interne et les portes existantes ne qualifient pas les entrées plates
mal formées.

**Décision :** le chemin plat est une piste de réduction des allocations
et du temps FULL, particulièrement à K10, pas encore un gain contractuel.
Fermer d'abord la frontière CSR publique, conserver une porte directe
vectoriel/plat avec lots groupés et entrées mal formées, puis rejouer
Release/ASan/UBSan/TSan et une ablation G4 sur plusieurs trames brutes
et sans sol. Mesurer allocations et RSS réels : les lots groupés
construisent encore des actions temporaires.
