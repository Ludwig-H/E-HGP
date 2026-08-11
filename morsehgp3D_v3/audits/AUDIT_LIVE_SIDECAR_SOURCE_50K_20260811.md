# Audit live — frontière sidecar et admission de la source 50 k

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot audité

Snapshot de code audité :
`9483b1cd5ff691bc53f51eb2776aaba77b011e43`. Le commit documentaire
`232470cbaf2449e5e68c92f2c42c532c4df20458` n'a ensuite modifié que l'index et
la note de livraison du sidecar.

Le worktree reste concurrent sur d'autres prototypes. Le présent verdict porte
exactement sur les empreintes suivantes, toutes identiques aux fichiers du commit
`9483b1c` :

- `prototype/validated_hybrid_sidecar.hpp` :
  `5a53332e238dbce9c2984d293cddd56616e6c3f856a60b41e4d08afc63e645cf` ;
- `prototype/sidecar_factory_gate.cpp` :
  `e07326d7678739e783068b61954475a30fa23812c1f51c08973664ddd71e7c4f` ;
- `prototype/saturated_pipeline.cpp` :
  `61afc51a976d20af937b9d364cf6985cc72d006a6de58d112956b2e1ee077db3` ;
- `CMakeLists.txt` :
  `adff6583da0898d83b5ae733d8f62384ec347bf4a6728a93d492c922ed8fcaf8`.

Toute empreinte différente exige un nouvel audit. Les modifications concurrentes de
la sonde `cell_prune` sont hors de ce snapshot. Aucun fichier d'implémentation n'a
été modifié par cet auditeur.

## Verdict

La factory et son raccord pipeline ne sont pas recevables comme frontière de confiance.
Ils peuvent servir de harnais borné après correction, mais ils ne prouvent actuellement
ni `CarrierClosure`, ni la complétude d'une table, ni une route 50 k. Les défauts S1 à
S4 sont bloquants pour l'exactitude; S5 bloque l'architecture produit.

La mesure G4 mass-only ne reçoit aucune lane de génération. En particulier, la phrase
« q=2 admissible après prune » doit être retirée tant qu'un producteur exact n'a pas
formé ou évité cette masse sous un coût mesuré compatible avec le contrat.

## S1 — le reçu dit opaque est forgeable

`HybridSourceReceipt::make` est public aux lignes 107--124 de
`validated_hybrid_sidecar.hpp`. Les fonctions publiques de digest permettent à tout
appelant de calculer les deux valeurs attendues sur sa propre table. La factory transforme
ensuite directement `enumeration_completed && rank_bound >= point_count` en
`CarrierClosure::kCertified` aux lignes 436--444.

Une table amputée peut donc être certifiée ainsi : construire la table amputée, calculer
ses digests publics, appeler `make(digests_amputes, n, n, true)`, puis passer ce reçu à
`build`. La fixture 6 ne couvre pas cette attaque : elle réemploie le reçu de la table
complète sur la table amputée et ne teste que la désynchronisation des digests.

Le raccord réel confirme que le type n'a pas remplacé l'ancienne assertion. Aux lignes
181--187 de `saturated_pipeline.cpp`, le pipeline fabrique lui-même le reçu depuis
`smax`, `n` et `status == kOk`. Le prédicat de complétude reste donc, sous emballage,
`smax >= n`. Aux lignes 202--208, le sidecar validé a déjà été détruit et le fold reçoit
encore `points + Catalogue` bruts. Aucune frontière typée n'est effectivement consommée.

Réception minimale : constructeur de reçu inaccessible aux appelants ordinaires,
autorité issue d'un producteur terminal dont la complétude est rejouable, fold recevant
`const ValidatedHybridSidecar&`, et fixture permanente
`fresh_receipt_on_amputated_catalogue` qui doit refuser avant le fold.

## S2 — la clé exacte déborde `i128` sur le domaine u16

Aux lignes 248--255 de `validated_hybrid_sidecar.hpp`, la clé calcule
`nx*nx + ny*ny + nz*nz` et `den*den` en `mhgp::i128`. Or
`morsehgp3D_v2/include/mhgp/sphere.hpp` documente un numérateur de centre de triangle
pouvant atteindre environ 90 bits et un dénominateur d'environ 73 bits. Le même fichier
emploie précisément `BigInt<4>` pour le numérateur du niveau et `BigInt<6>` pour ses
comparaisons. Les carrés demandent donc jusqu'à environ 181 et 146 bits : l'opération
courante peut provoquer un overflow signé et ne définit pas une `ExactBallKey`.

Réception minimale : fraction canonique multiprécision du niveau, sans passage
intermédiaire par `i128`, plus une fixture u16 dont le numérateur dépasse 64 bits et une
campagne UBSan. La clé de centre et la clé de niveau doivent être comparées sur leurs
représentations canoniques prouvées.

## S3 — `q_min` et le support propre ne sont pas recalculés

La factory vérifie que les identifiants déclarés sont triés et sur la coquille, puis recopie
`sphere.n_support` dans `certificate.q_min`. Elle ne vérifie ni indépendance affine, ni
positivité, ni égalité avec le support canonique rendu par `miniball_of`; elle ne vérifie
pas non plus `sphere.sph.support == sphere.n_support`. Le commentaire « q_min
recalculé » ne correspond donc pas au code.

Les sphères synthétiques positives de la gate laissent d'ailleurs `Sphere.support` à zéro
dans `make_sphere`. Un carré cocirculaire peut être déclaré avec ses quatre points comme
support : les quatre points sont sur la coquille et la miniboule des membres est correcte,
mais cet ensemble est redondant et n'est pas un support propre affinement indépendant.
La branche `members.size() <= q` peut alors le certifier principal sans aucun témoin.

Réception minimale : reconstruire canoniquement le support propre positif, comparer
`q_min`, les identifiants et `Sphere.support` au résultat indépendant, puis graver les
fixtures `redundant_cospherical_support`, `affinely_dependent_support` et
`sphere_support_field_mismatch`.

## S4 — le digest ne peut pas porter une preuve exacte

`sidecar_catalogue_digest` hache l'image mémoire brute de `CriticalSphere`, donc son
padding, son ABI et le `double beta`, avec FNV-1a 64 bits. Le commentaire reconnaît que
ce digest n'est pas cryptographique. Une collision demeure possible et l'image brute
n'est pas une sérialisation sémantique canonique. Le digest final ne lie ensuite que les
états `principal`, pas les certificats complets, l'ordre, les lots ni les fermetures.

Un digest exact doit porter une sérialisation canonique champ par champ, sans padding ni
projection flottante, et employer le SHA-256 contractuel. Il lie une preuve; il ne crée
jamais la preuve de complétude.

## S5 — la factory actuelle est un juge borné, pas un chemin 50 k

Pour chaque générateur, la factory rescane les `n` points afin de vérifier la saturation,
puis appelle `miniball_of` sur tous ses membres. Cette primitive énumère les supports
jusqu'à la taille quatre. Le coût est donc au moins proportionnel à `n * G` pour `G`
générateurs et peut ajouter un coût combinatoire dans le rang de chaque saturé. Ce travail
duplique précisément le census que la source est censée avoir certifié. Il est incompatible
avec un `warm_e2e` 50 k sous la seconde et doit rester un juge borné, ou être remplacé
dans le chemin produit par la consommation de certificats streamés et rejouables.

L'architecture produit ne doit conserver ni une table globale de tous les saturés, ni un
rescan nuage--générateur. Elle doit posséder les activations utiles et leurs certificats au
fil de l'eau, avec identité count/fill, déduplication par clé exacte et refus atomique.

## S6 — le prune convexe est formulé trop fortement dans les documents courants

`PROPOSITION.md` affirme qu'une séparation entre la cellule et `conv(A_C)` exclut tout
support propre. `AUDIT_ETAT_COURANT.md` répète cette assertion. La contre-fixture
entière déjà donnée dans `REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`,
lignes 323--334, montre le contraire : un tétraèdre propre de centre dans la cellule et de
niveau 2700 subsiste alors que la banque donne `Q=250` et que les deux convexes sont
séparés.

La portée correcte est : la séparation exclut la branche locale `beta < Q`. La branche
`beta >= Q` ne devient omissible que pour le quotient horizontal normalisé, après preuve
des témoins stricts, application du théorème 4.2 et présence du resolver silencieux. Elle
n'est jamais supprimée d'une source Gamma complète par la seule séparation.

## S7 — les reçus G4 n'admettent aucune énumération de tuples

Le reçu brut `receipts/g4_massonly_20260811/cell_50k_raw.txt` dit explicitement
« aucun tuple formé ». Après le prune d'axe, les masses q2 restent comprises entre
465 371 500 et 2 862 879 000 selon la famille et le pas; q3 entre 14 667 530 000 et
131 762 100 000; q4 entre 330 437 400 000 et 9 968 861 000 000. Les temps de
0,174 à 29,153 secondes mesurent la construction top-t, les dilations et les comptes,
pas un producteur exact ni un débit CUDA.

Conséquence : q3 et q4 sont rouges, q2 est elle aussi non admise pour le contrat de
latence tant que ses tuples ne sont pas évités ou produits, certifiés et consommés sous
une enveloppe mesurée. Le count-only est un refus d'architecture combinadique, pas une
qualification positive.

## S8 — le pinceau q4 conserve au moins un milliard de triples

Le remplacement des quadruplets directs par un triple canonique ne suffit pas sur les
masses reçues. Pour une cellule survivante de taille `m_C`, le schéma qui prend les trois
plus petits `PointId` d'un quadruplet doit encore visiter les
`C(m_C-1,3)` triples susceptibles d'avoir un quatrième identifiant plus grand. L'identité
exacte est `C(m_C,4) = m_C*C(m_C-1,3)/4`. En notant `R'_4` la masse q4 après prune et
`m_max` le maximum publié, on obtient donc :

$$P_{mathrm{triple}}geqrac{4R'_4}{m_{max}}.$$

Au pas 6, les valeurs arrondies conservatrices du reçu brut imposent déjà :

| famille | `R'_4` | `m_max` | borne sur les triples canoniques |
| --- | ---: | ---: | ---: |
| `terrain` | `2,432370e12` | 3 550 | `> 2,74e9` |
| `scanline_single_pass` | `2,603368e12` | 979 | `> 1,063e10` |
| `scanline_overlap_multiecho` | `3,304374e11` | 1 295 | `> 1,020e9` |

Sans la restriction aux trois plus petits identifiants, la masse
`sum_C C(m_C,3)` est encore supérieure à `4R'_4/(m_max-3)`. Ces bornes
précèdent le range-report, les prédicats exacts, le census, la déduplication et
le fold. Scanner les points par triple recrée en outre exactement
`4*R_4+3*R_3` classifications. Le pinceau reste donc un oracle local ou un
fallback borné; il ne doit pas être implémenté comme prochaine source produit
tant que son préflight ne descend pas de plusieurs ordres de grandeur.

## S9 — audit live du plan séparateur général

Le worktree concurrent ajoute un séparateur proposé par la direction du
barycentre, puis revérifié en entiers sur tous les points de `A_C` et les coins
fermés de la cellule. Le prédicat local est fail-closed : une vérification
positive prouve bien la séparation stricte, indépendamment de la qualité de la
proposition. Il ne prouve que l'absence de support de la branche `beta<Q`.

Snapshot live exercé, distinct du commit sidecar audité plus haut :

- `prototype/cell_source_mass_probe.cpp` :
  `8f37f84fda5e25df78242f8e59962c82e9a9cfb8d00cb68661b1c9e07da68bbc` ;
- `prototype/cell_prune.hpp` :
  `20ae52f5febed8232a5c522d773387ef38f119ed7379a4d44d5785e5b9f0f0e2` ;
- `prototype/cell_prune_gate.cpp` :
  `de50ff942e7ad0144c2c4a2294307e021c63e981ae56840c2b4c7d9bff837d2a`.

La porte Release passe `4/4` CTests ciblés. Une mesure locale supplémentaire,
`n=2400`, deux threads, seed `20260810`, ne forme toujours aucun tuple. Après
axes et plan, `R'_4` reste `1,061280e9` sur `terrain` au pas 4,
`1,095035e9` sur `scanline_single_pass` au pas 6 et `7,416995e9` sur
`scanline_overlap_multiecho` au pas 6. Ces mono-mesures ne sont ni G4 ni
comparables à un `warm_e2e`; elles montrent seulement que le plan du
barycentre ne change pas encore la décision d'architecture, même à 2 400
points.

La porte manque encore la contre-fixture q4 `beta>=Q` dans son propre binaire.
Cette fixture est nécessaire pour empêcher qu'un futur appelant transforme le
verdict local en `no_support`. Pour q4 seulement, une séparation non stricte
peut aussi être utile si l'autorité testée est l'intersection avec l'intérieur
tridimensionnel de l'enveloppe; le prédicat strict partagé actuel est plus
conservateur et reste sûr pour q2/q3.

## S10 — route de source conditionnelle recommandée

La route cellules--tuples est rétrogradée en sonde de masse et fallback borné.
La meilleure route exacte actuellement documentée est conditionnelle :

```text
self-join LBVH des paires non ordonnées
  -> center-cover fail-open par blocs, seuils q2/q3/q4 = 10/9/8
  -> ancres diamètre résiduelles, owner canonique après census
  -> cordes dans le disque de Jung
  -> construction directe des niveaux de profondeur au plus 7-c_e
  -> décisions terminales exactes et BallActivation streamées
```

Pour une ancre `e`, le rang q4 vaut exactement `4+c_e+profondeur_stricte` et
le nombre de sommets utiles vérifie `Z_e<=m_e*(8-c_e)<=8*m_e`. Le gain local
est donc démontré si le constructeur ne forme jamais d'abord les
`C(m_e,2)` intersections. La parcimonie globale du nombre d'ancres `a` et de
`M=sum_e m_e` reste une hypothèse à mesurer, pas un théorème.

L'ancien prototype `center-cover` a dépassé 600 secondes à 50 k sans JSON. Il
est explicitement rejeté comme implémentation produit; la recommandation ne le
réhabilite pas. Le prochain jalon est un nouveau P1a mass-only par blocs qui
ferme l'identité `pruned + microtile = C(n,2)` et publie visites de cover,
ancres `a`, `M`, constantes `c_e`, `sum Z_e`, files, octets et ambiguïtés. Il
est NO-GO si `source-cover + cordes` dépasse 400 ms chaud sur G4, si la majorité
des paires atteint les microtuiles ou si le travail contient `sum_e m_e^2`.

L'ordre `k=1` doit suivre une lane EMST exacte distincte. L'EMST quadratique du
juge actuel reste un oracle; un Borůvka/LBVH exact device est une proposition à
recevoir. Les ordres supérieurs ne peuvent jamais être remplacés par un MST de
points.

## Portes immédiates proposées à Claude

1. Fermer S1--S4 et faire recevoir la factory uniquement comme juge borné.
2. Passer réellement le sidecar typé au fold; aucun chemin autoritaire ne reçoit plus un
   `Catalogue` brut accompagné d'un booléen ou d'un reçu constructible publiquement.
3. Ajouter les cinq fixtures ciblées ci-dessus et leurs mutants, avec codes de sortie
   exacts et planchers de non-vacuité.
4. Corriger la portée du prune convexe dans l'audit courant, le README et la proposition.
5. Retirer l'admission q2 et abandonner toute route produit qui énumère les masses
   combinadiques mesurées. Garder le plan général comme sonde mass-only et ne pas
   implémenter le pinceau tant que sa masse de triples reste rouge.
6. Prototyper le P1a par blocs `center-cover -> cordes shallow`, avec identité de
   toutes les paires, transcript rejouable à petit `n` et gates chiffrées de S10,
   avant tout portage CUDA du constructeur.

GCP non utilisé pour cet audit.
