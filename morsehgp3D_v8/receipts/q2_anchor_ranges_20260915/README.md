# Qualification des plages d'ancres et parents Pool partagés

15 septembre2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Flux q2 complet de supports, intérieurs stricts et coquilles complètes.
**Ni moteur q3/q4, ni HGP FULL, ni qualification G4. GCP non utilisé.**

## Captures propres

| Capture | Périmètre | État |
|---|---|---|
| [qualification_3b2h3jn6](qualification_3b2h3jn6/COMPLETION.json) |72 CTests Release + gate ranges explicite | PASS |
| [qualification_obork_m5](qualification_obork_m5/COMPLETION.json) |72 CTests Clang ASan/UBSan + gate explicite | PASS |
| [tsan_hxa_vd48](tsan_hxa_vd48/COMPLETION.json) | Gate ranges Clang ThreadSanitizer | PASS |
| [smoke_mn3kw4d3](smoke_mn3kw4d3/COMPLETION.json) |4 familles n32, Coarse W4 et ranges W1/W4 |12 mesures closes |
| [scale_d81h8296](scale_d81h8296/COMPLETION.json) |4 familles n8k/16k/32k, K10/s8, Pool64 |36 mesures closes |
| [separations_mhv79yut](separations_mhv79yut/COMPLETION.json) |4 familles n8k, K5/10, s8/10/12, Pool64 |72 mesures closes |
| [rows_large_bc5rihg1](rows_large_bc5rihg1/COMPLETION.json) |Rangées n8k/16k/32k, K5/10, s8/10/12, Pool0 |54 mesures closes |

Total :174 mesures,58 références Coarse W4 et116 appels ranges W1/W4.
Grain64, file8, cible16 jobs par worker, graine3. Chaque triplet garde
les mêmes coordonnées, K, s et seuil Pool. Sorties, clés, intérieurs,
coquilles, callbacks et tous les comptes géométriques/Pool concordent ;
les comptes de distribution et les chronos ne sont pas forcés à l'égalité.

Les16 commandes de lecture/analyse normal/−O sont closes dans
[analysis_pis6l9ji](analysis_pis6l9ji/COMPLETION.json). Sa
[synthèse déterministe](analysis_pis6l9ji/SUMMARY.json) est identique dans
les deux modes ;114 sources,84 artefacts distincts des trois builds,
scripts d'analyse et entrées des captures sont contrôlés à la fermeture.
Les captures elles-mêmes épinglent40 exécutables et leur cache pour
chaque build complet ; le build TSan épingle sa gate et son cache.

Chaque gate C++ explicite confronte415 appels ranges,135 Coarse et dix
appels avec options par défaut sur dix nuages à un oracle indépendant :
10 920 paires,778 110 tests de sites,126 065 supports vérifiés, coquille30.
Les modes géométriques, Pool0/2/64, le repli Shared, W>jobs, W1/2/4/8,
grains1/8/64, deux callbacks sur de vrais threads, jointure après erreur,
reset de l'index et appel imbriqué sur contexte distinct sont exercés.
Douze cas à quatre nappes, W8/W32, exercent des bandes Pool filtrées larges.
Les14 entrées invalides et sept mutants sont rejetés.

Les dons Pool filtrés observés dans les gates explicites valent115 en
Release,101 sous ASan/UBSan et103 sous TSan. Ils prouvent l'exercice de
cette voie, pas sa fréquence en production. Le test n'impose pas un nombre
de dons dépendant du scheduling. Aucune injection d'échec d'allocation du
nouveau parent, ni preuve d'un receveur déjà endormi lors de l'exception.

Les gates Python normal/−O exécutent chacune24 sondes réelles,12 paires
W1/W4,24 CLI invalides et plus de3 700 mutants, plus12 mutations de pins
et JSON strict. Le nombre de mutants « maximum remplacé par somme » peut
varier avec le scheduling des nouvelles sondes : en Release3 710/3 712.
L'égalité normal/−O concerne la lecture des **mêmes captures**.

Builds désormais épinglés : `build/v8_anchor_ranges_20260915`,
`build/v8_anchor_ranges_sanitize_20260915`,
`build/v8_anchor_ranges_tsan_clang_20260915`. Les
[préflights](PREFLIGHT.md) restent distincts, notamment le lancement refusé
pendant remplacement du binaire et le test antérieur sans don Pool filtré.
Les sources d'audit B ne sont ni modifiées ni promues en qualification propre.

## Ce que cette structure change réellement

La file porte des plages de72 octets sur ce build, pas les6 272 octets
de pile réservée d'une continuation. Ce ne sont pas les mêmes obligations :
la plage transfère des ancres **non commencées**, pas une branche B en cours.
Un seul parent Pool est préparé par rectangle ; ses bandes gardent leur
ordre de projection. Le receveur réutilise un moteur et des buffers privés.
Sans rejet Pool, le parcours Shared reste groupé, sans expansion en paires.

Sur les174 mesures :278 dons, dont208 Shared,70 replis Pool et **zéro
bande Pool filtrée** ;52 après attribution de toutes les seeds, pas après
leur achèvement. La capacité de partage filtré est donc qualifiée par les
petites gates, pas par un gain de ces grandes mesures. Sur la croissance
Pool64, seuls les rangées donnent ; uniforme, terrain et amas n'ont même
aucune offre au grain64. Uniforme32k conserve10,181M plages et11,084M
ancres, donc la cible suivante reste le coût des millions de petites tâches.

Sur amas32k, le pic inscrit vaut quatre parents et291 992 octets ; sur
rangées il vaut un parent et72 800 octets. Cela compte chaque parent une
fois, pas chaque référence. Le suivi omet les temporaires de construction,
les queues de destruction, blocs de contrôle, moteurs, nuage et RSS.
La somme historique des maxima par créateur n'est plus une borne du
stockage simultané des plans partagés. Au plus Q+W parents distincts
inscrits sont retenus par les obligations internes, hors callbacks utilisateur.

## Temps : acquis d'architecture, pas de gain stable qualifié

Observations uniques, affinité0,2,4,6 sur quatre cœurs physiques. Toute
la campagne scale (09:49:47–09:53:32 UTC) chevauche ASan/UBSan
(09:49:10–09:55:57) ; Release finit à09:52:35. Des audits tournent aussi
en parallèle. Ne pas comparer ces valeurs à un historique sur hôte libre.

Sur les54 triplets à n≥8k, le ratio Coarse W4/ranges W4 va de0,717 à2,257,
médiane1,039 ;31 observations sur54 sont plus rapides. Sur scale seul,
6 sur12 le sont, médiane1,010 : pas d'amélioration générale établie.
Les rangées sans Pool ont12 observations favorables sur18, contre six
défavorables. Exemple K10/s8, pipeline q2 avec callbacks, hors préparation
du nuage/index et sans reconstruction FULL :

| n | Coarse W4 | Ranges W1 | Ranges W4 |
|---|---:|---:|---:|
|8 000 |154,40 ms |230,85 ms |68,40 ms |
|16 000 |176,91 ms |488,36 ms |140,02 ms |
|32 000 |332,92 ms |1 028,41 ms |324,79 ms |

Ces valeurs ponctuelles ne qualifient ni100 ms de tour ni un gain G4.
Avec Pool64, les rangées donnent12/9/5 fois dans scale et la plus grosse
part de visites d'un worker vaut33,3/37,0/31,1% : le déséquilibre est réduit,
mais le temps global n'est pas systématiquement meilleur. Coarse reste le défaut.

Le temps Pool nouveau additionne préparation et intervalles actifs des
plages, sans ajouter à nouveau un intervalle parental englobant les dons.
Les durées workers incluent l'attente ; leurs sommes ne se soustraient
pas au temps mur. Toute l'équipe, les jointures et destructions sont payées
par le temps global ; une ancre entière et son payload restent atomiques.

## Croissance : progrès mesuré, pas de borne générale

Visites census du flux q2 complet, campagne Pool64/K10/s8 :

| Famille |8k |16k |32k | Ratios |
|---|---:|---:|---:|---|
| Uniforme |171 895 354 |413 553 244 |1 001 201 993 |×2,406 / ×2,421 |
| Terrain |20 472 635 |42 798 408 |95 128 515 |×2,091 / ×2,223 |
| Amas |81 112 664 |239 954 275 |648 207 562 |×2,958 / ×2,701 |
| Rangées |5 742 485 |11 886 273 |24 566 835 |×2,070 / ×2,067 |

Les six postes principaux (produits front, descentes témoins, candidates,
visites census, divisions structurelles, supports) restent sous×3 sur ces
quatre séries. Ils sont **identiques** à Coarse : le nouveau répartiteur
ne diminue pas ces volumes. Ne pas élargir ce constat à tous les champs :

- F Pool des rangées vaut8 000/32 000/80 000, soit×4 puis×2,5 ; ses
  insertions22/8 066/24 154 montrent un changement de décomposition.
- Les racines Pairwise après Pool des amas valent11 329/29 688/102 336,
  soit×2,621 puis×3,447. Certaines divisions/certifications dépassent×3,
  même quand le total des visites reste inférieur.
- Les masses cartésiennes créditées ou rejetées par blocs ne sont pas
  des boucles de paires : elles peuvent dépasser×4. L'analyse conserve
  aussi ces champs dans `census_operations`/`sibling_operations` ; ces
  noms de regroupement ne changent pas leur nature de populations.
- Quinze ratios des champs scheduling W4 dépassent×4 dans rows_large,
  dont deux maxima de taille de file et treize ratios d'offres/refus.
  Les consultations passent par exemple59/244/613 à K5/s8, soit×4,136
  puis×2,512. Leurs valeurs et les départs de zéro restent publiés.

La gestion nouvelle est bornée par les ancres effectivement traitées A
et plages initiales R : au plus A−R dons, O(A+R) consultations, sans scan
des ancres transférées. Ce n'est pas une borne de A(n) ni du travail
géométrique. F, résidu, collecte et volume de sortie restent obligatoires.
**Aucune borne générale sous-quadratique, P0 global ou tour FULL/G4 clos.**

## Rejouer et suite

Le [runner](../../bench/run_wspd_q2_ranges_checks.py) fournit `read` pour
chaque capture et `run --campaign qualification|tsan|smoke|scale|separations|rows_large`
avec un build neuf. Les commandes exactes, sorties brutes, champs décodés,
hashes et fermetures sont conservés ; ne pas écraser les builds ci-dessus.
`analyze.py` lit des captures explicitement nommées ; `record_analysis.py`
préserve ses lectures normal/−O et les éventuels échecs dans un dossier neuf.

Suite : lots bornés en mémoire d'états singleton compacts, réutilisation
des données de contexte et réduction du coût réel front/census, pas une
autre file concurrente. Garder le B original, rang d'ancre, stade et
certificat frère encore dû ; ne pas oublier la collecte des coquilles.
Mesurer le format CPU avant son port GPU. Les deux nouvelles fixtures de
tangence [q3](../../docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) précisent un
bord mathématique ; les moteurs q3/q4 et FULL restent à implémenter.
