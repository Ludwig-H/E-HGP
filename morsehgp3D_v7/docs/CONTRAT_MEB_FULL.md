# Proposition MEB filtrée dans le Builder FULL

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Le [helper interne](../src/forest/meb_proposal.hpp) et son raccord dans
[FULL](../src/forest/full_gabriel.hpp) visent le coût des MEB locales.
Ils ne changent ni les catalogues demandés ni le nombre d'invocations.
Le succès FULL reste relatif à des catalogues complets, exacts et réguliers
fournis ; aucune qualification de CLI, archive, verticale ou tour industrielle
n'est déduite de ce raccord. Le défaut reste **P=0**, donc F sans proposition.

La [qualification propre au raccord](RESULTATS_MEB_FULL_20260906.md)
ferme 30/30 CTests Release et 30/30 ASan/UBSan, avec les relectures
locales, budgets, mutations et injections séparément attribués.

## Domaine et port explicite

Les formes, l'ordinal et la matérialisation sont portés du helper privé
`d6dbba19…` ; la trajectoire filtrée vient de `484a89bc…`. Leurs
[preuves et captures locales](RESULTATS_MEB_FILTREE_20260906.md) restent
attribuées à ces octets. Une nouvelle qualification est nécessaire pour
le header produit et sa composition avec FULL.

Le domaine interne est celui des demandes du Builder : 2 à 11 sites
distincts d'un index u16 authentique, coordonnées et caps immuables,
candidats internes authentiques. Ce n'est pas une API d'admission de
candidats arbitraires. Le code F `silent_incidence.hpp` reste inchangé ;
le dispatcher reçoit le Builder F déjà détenu par FULL, sans en créer
un nouveau ni appeler son constructeur de cœur `run()`.

Le diamètre global fournit le support initial q2. Chaque pivot ajoute
le premier violateur strict à la base positive courante ; seuls les
supports q3 puis q4 contenant ce violateur sont essayés, dans l'ordre
stable. La contenance concerne tout le petit ensemble, et le contrôle
final de coquille concerne tous les sites de la demande. La base positive
du pivot admissible est unique ; l'ordre des essais reste observable dans
le budget. Seize pivots bornent le travail, sans garantir la convergence.
La borne native est de 146 formes, initialisation comprise, hors recherches
de diamètre et évaluations de puissances.

Une proposition n'est certifiée que si sa boule contient tous les sites et
si sa coquille est exactement son support essentiel. Elle conserve la clé,
le support entier canonique, notamment `support[0]`, et le niveau q4 brut.
Son ordinal R est celui de F **sur tous les sites** (1 à 550), pas celui
du petit pivot. Un échec de proposition retrouve F ; une coquille irrégulière
n'est jamais rendue acceptable par ce mécanisme.

## Budgets et compteurs

L'option C++ `FullGabrielLimits.max_meb_proposal_supports` est un plafond P
partagé par **toutes** les MEB d'une tentative d'ordre, eager ou lazy.
Elle n'est ni K, ni `smax`, ni s WSPD, ni une option de la CLI historique.
Le résultat nomme `reference_ordinal_plus_native_z_q3_q4_proposal_v2`
dans `meb_accounting`, distinct du calendrier des successeurs.

| Champ | Sens |
| --- | --- |
| `stats.meb_calls` | Invocations FULL admises par `max_meb_calls`, inchangé |
| `stats.geometry.meb_supports` = c | Préfixe de référence chargé contre L=`max_meb_supports` ; devient virtuel sur une certification rapide |
| `stats.meb_proposal.meb_proposal_supports` = p | Essais de formes proposés admis, payés avant l'observation et les prédicats ; une injection peut interrompre l'essai après cette charge |
| `stats.meb_proposal.reference_supports` = A | Formes réellement essayées dans F, y compris au cours d'un repli interrompu |
| `pivots` | Pivots engagés, y compris celui qui épuise P |
| `certified` | Propositions certifiées, même si L refuse ensuite leur ordinal |
| `fallback` | Décisions de repli avec marge L, pas tous les appels refusés dès l'entrée |

L'appel FULL est payé avant le dispatcher. Si c a atteint L, aucune
proposition n'est engagée. P=0 suit directement le corps F ; sinon une
marge P épuisée retrouve F, sans nouvelle raison publique de refus.
Une certification charge R prospectivement : si R dépasse L−c, c devient
L, le refus reste `silent_meb_support_budget` et la boule sentinelle
reste intacte. A n'est pas augmenté par cette charge virtuelle.

Les plafonds L et P restent deux u64 séparés. Depuis un état cohérent,
A≤c et p≤P ; la borne mathématique A+p≤L+P ne doit pas être calculée
par une addition u64 susceptible de déborder. Les autres compteurs gardent
leur marge sous le budget externe d'invocations. Les statistiques F ne
doivent plus être présentées comme un coût physique total lorsque P>0.

## Durée de vie et erreurs

Un Work appartient au Builder entier. Ni appel local ni repli ne le
réinitialise. Le miroir A entoure uniquement l'appel F ; sa destruction
ajoute le delta de c, même sur exception. Le destructeur du Builder FULL
publie les cinq champs Work et les treize statistiques géométriques avant
la capture d'une exception par le wrapper.

Les wrappers gardent leurs transactions : `bad_alloc` et `length_error`
refusent sans forêt ; les autres exceptions se propagent. Une exception
d'observateur avant une forme peut laisser l'appel FULL et p déjà payés,
mais pas encore l'appel géométrique : leur égalité ne serait pas un
invariant d'échec. Le chemin produit utilise uniquement `NoObserver`,
sans mutation ni exception d'observateur. Les injections de tests ne sont
pas des résultats de géométrie ou de latence.

## Résidence et suite

Le raccord ajoute cinq compteurs par ordre et des temporaires locaux de
taille bornée. Il n'ajoute ni catalogue Gamma, ni mosaïque de Delaunay,
ni cache global de MEB ou de cofaces silencieuses. Il ne réduit pas les
catalogues Gabriel ni les arènes de sortie ; les contraintes de
[résidence massive](RESIDENCE_MASSIVE.md) restent ouvertes.

La réutilisation d'une terminaison déjà certifiée par label immuable est
un autre delta : mesurer T−U et conserver le premier recalcul complet,
puis normaliser le token courant, sans mémoriser une racine périmée.
Cette optimisation n'est pas incluse ici. Les anciens chronométrages
mono restent attachés à leur header ; le contrat 50k et les G4 massifs
ne sont pas acquis par ce raccord. GCP non utilisé.
