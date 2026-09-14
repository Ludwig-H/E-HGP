# Front et census q2 sur plusieurs CPU — preuves propres

14 septembre 2026. `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`.
Cette capture mesure le composant q2 entier, pas la tour HGP FULL.
GCP non utilisé. Aucun coût cloud.

## Exactitude et durée de vie

Deux builds neufs, Release GCC13.3 et Debug Clang18.1.3 ASan/UBSan :
**53 CTests passent dans chacun**, puis les portes C++ explicites et
les rejets CLI passent ; 43 commandes par qualification. Les sources
et artefacts restent identiques avant/après. Aucune preuve Pool ancienne
n'est transférée au nouveau transport des tâches.

- [Release](qualification/release_tc6c459e/RESULT.json), avec
  [JUnit](qualification/release_tc6c459e/ctest_full.xml).
- [ASan/UBSan](qualification/sanitize_wreb9c1i/RESULT.json), avec
  [JUnit](qualification/sanitize_wreb9c1i/ctest_full.xml).
- [ThreadSanitizer](thread_sanitizer/README.md) : la porte parallèle
  s'exécute réellement, sans diagnostic ; ce contrôle borné ne prouve
  pas l'absence universelle de course.
- [Ancien/nouveau mono](qualification/differential_33vk37no/RESULT.json) :
  32 configurations à128 sites, quatre familles, K5/K10, s8/12, Pool0/64,
  64 commandes. Tous les champs hors chronos concordent exactement avec
  le binaire historique ba11e3ab, épinglé par SHA256, jamais reconstruit.

La nouvelle porte front vérifie 9 241 plans et 27 723 reprises dans des
ordres différents, dont des coupes après rejet partiel des voies.
La porte q2 compare 152 références mono et 438 appels parallèles à un
oracle scalaire indépendant : 1 106 458 évaluations, 45 161 supports,
coquilles jusqu'à30 sites. Les entiers front/census/frère/ordre/conjoint/
Pool sont identiques ; seuls les chronos et compteurs d'ordonnancement
peuvent varier. Réentrance, relâchement du propriétaire, exception
pendant un Pool effectif et échec après un premier thread lancé passent.
Les lecteurs normal/−O exercent chacun 50 captures positives et rejettent
42 corruptions, huit domaines invalides et six lignes de commande.

La [contrelecture B53d5640a](../../audits/DIALOGUE_AUDITEUR_B.md)
vérifie séparément 4 336 appels parallèles, de1 à8 workers, sans
désaccord ni doublon entre slots sur son corpus. Les huit empreintes
produit/fixture nommées dans son reçu correspondent aux fichiers courants.
Ses temps ne sont pas importés dans les tableaux ci-dessous ; les sources
transitives et qualifications propres du port restent épinglées ici.

## Erreur de harnais conservée

La [première tentative différentielle](qualification/differential_o881onql/RESULT.json)
échoue avant comparaison : le script demandait `timings_ms`, alors que
la sonde historique produit `timings`. Le code C++ n'a pas changé.
Le [script initial](qualification/record_initial.py.snapshot) est celui
des qualifications Release/ASan et de cet essai en échec. Le
[script corrigé](qualification/record_corrected.py.snapshot) ne change
que cette clé ; il porte le différentiel réussi et les lectures
des campagnes. Les commandes et sorties brutes de l'échec restent présentes.
Ces scripts calculent la racine depuis leur emplacement prévu sous
`build/v8_front_workers_20260914/`, pas depuis les snapshots.
Le journal brut `thread_sanitizer/prepare_workspace_yncai04j/configure.stdout.bin`
conserve les trois espaces de fin de ligne émis par CMake. Ce seul fichier
brut est exclu du contrôle whitespace ; le nettoyer invaliderait ses hashes.

## Protocole des mesures

Premier corpus clos : 86 invocations, processus successifs,
affinité CPU0–3 commune au mono et au multi : **deux cœurs physiques,
quatre threads matériels** (SMT), pas quatre cœurs physiques. Hôte partagé, une seule
répétition, sans échauffement ; aucune mesure p95 ni qualification G4.
Les qualifications/compilations du constructeur sont terminées avant
le lancement des mesures. Les autres utilisateurs de l'hôte ne sont
pas isolés. Le nuage et l'index sont construits une fois par invocation.
La collecte complète, les copies/tri/validation/hachage des IDs et leur
destruction sont payés ; le catalogue dédupliqué et les parents FULL
ne sont pas construits. Le temps JSON/écriture du reçu reste hors chrono.

Politique fixée : Samples, SharedBlocks, frère activé, ComplementFirst,
ancres individuelles, Pool64. Le défaut de la bibliothèque reste Pool0.
`threads=0` appelle l'API mono ; `threads=1` l'API découpée à un worker.
Comparer ces deux bras contrôle le surcoût du découpage. Les compteurs
géométriques et digests doivent rester identiques à entrée et s fixés.

| Campagne | Paramètres | Invocations |
| --- | --- | ---: |
| baseline8k | quatre familles, K5/K10, s8, 1/2/4 workers | 24 |
| separations8k | quatre familles, K5/K10, s10/12, 4 workers | 16 |
| mono8k | quatre familles, K10, s8, API mono | 4 |
| growth | quatre familles, 16k/32k, K5/K10, s8, 1/4 workers | 32 |
| granularity8k | uniforme/amas/rangées, K10/s8, 4 workers, 1/64 jobs par worker | 6 |
| component50k | quatre familles, K10/s8, 4 workers | 4 |

Les quatre mesures50k portent seulement sur q2 CPU. Elles ne sont
**jamais** les contrats de tour50k/G4. Les trois séparations sont
comparées à8k ; la croissance nouvelle est mesurée à s8 seulement.

Un second corpus séparé `physical_cores/` est mesuré sur CPU0/2/4/6,
soit quatre cœurs physiques distincts. Il n'est pas amalgamé au premier
par le lecteur : le protocole exige des affinités identiques au sein
d'une même comparaison appariée. Prévu : K10/s8, quatre familles,
8k avec1/2/4 workers puis16k/32k/50k avec4, soit24 mesures. Deux répétitions
supplémentaires des douze configurations8k portent cette comparaison à
trois essais par bras : médiane et étendue, jamais sélection du meilleur.
Le premier essai amas/mono y est particulièrement lent ; le ratio
supérieur à4 qu'il donnerait avec quatre workers n'est pas retenu seul.
Ce second corpus comporte donc48 invocations closes. La
[topologie lue](TOPOLOGY.json) documente les deux affinités.

Les [lectures86](qualification/readers_9sg3divf/RESULT.json) et
[lectures48](qualification/readers_leitp26k/RESULT.json) passent en mode
normal et optimisé, avec sorties identiques. Aucun essai ni échec perdu,
aucun changement de sources/binaire à la clôture. Les builds Release,
ASan/UBSan et ThreadSanitizer sont désormais épinglés.

## Résultats : temps réel du composant, pas sommes des workers

À 8k, K10/s8/Pool64, quatre cœurs physiques autorisés, trois essais par
configuration : médianes en secondes. Les étendues restent dans les
sorties et leur synthèse ; un essai sur hôte partagé ne donne pas un p95.

| Famille | 1 worker | 2 workers | 4 workers | Gain 1/4 |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 5,387 | 2,732 | 1,379 | ×3,91 |
| Terrain | 0,971 | 0,493 | 0,286 | ×3,40 |
| Huit amas | 2,994 | 1,489 | 0,770 | ×3,89 |
| Deux rangées | 0,230 | 0,122 | 0,119 | ×1,93 |

Le mono amas va de2,940 à4,891 s ; le quatre-workers uniforme de1,365
à2,309 s. Le bruit est réel et interdit de sélectionner le plus beau ratio.
Les rangées plafonnent malgré quatre cœurs : un gros travail de census
reste concentré sur un worker. Ni la sûreté du partage ni la quantité
de jobs ne prouvent l'équilibrage de la charge.

À K10/s8 et quatre workers sur quatre cœurs physiques, un seul essai
par case aux tailles suivantes, même collecte complète payée :

| Famille | 16k, s | 32k, s | 50k, s |
| --- | ---: | ---: | ---: |
| Uniforme | 4,036 | 8,833 | 13,998 |
| Terrain | 0,563 | 1,334 | 2,037 |
| Huit amas | 2,441 | 7,176 | 10,270 |
| Deux rangées | 0,131 | 0,273 | 0,437 |

Le dernier chiffre inférieur à une seconde ne clôt aucun contrat :
ce n'est ni la tour FULL ni une G4. Les cibles industrielles restent ouvertes.

## Croissance et déséquilibre

Le changement d'ordonnancement conserve exactement le travail discret.
À s8/K10, les visites géométriques du census donnent :

| Famille | Ratio 8k→16k | Ratio 16k→32k |
| --- | ---: | ---: |
| Uniforme | 2,406 | 2,421 |
| Terrain | 2,091 | 2,223 |
| Huit amas | 2,958 | 2,701 |
| Deux rangées | 2,070 | 2,067 |

Front, descentes du proposeur et opérations structurelles sont aussi
suivis séparément à K5/K10. Leurs ratios principaux restent inférieurs
à4 sur ces doublements, pas une borne asymptotique universelle. Les
candidates compactes ne sont pas assimilées à des paires développées.
La [synthèse recalculable](analysis/SUMMARY.json), avec son
[analyseur](analysis/analyze.py), conserve tous les essais, médianes,
étendues, comptes et hashes des27 fichiers de triplets. Les maxima
observés K5/K10 sont ×2,744 pour les candidates, ×2,682 pour les produits
du front, ×2,889 pour les descentes témoins, ×2,958 pour les visites
census, ×2,736 pour les descentes structurelles et ×2,117 pour les supports.
L'algorithme complet FULL n'est pas encore construit et certaines
familles ont une sortie intrinsèquement quadratique : rester sensible
à la taille de sortie au lieu de promettre une borne impossible.

Le préfixe initial des essais50k du premier corpus coûte environ0,02 ms ;
le coût dominant est après ce préfixe. À8k/K10 sur rangées/quatreSMT,
un worker porte78,8 % des visites census. Avec1/16/64 jobs par worker,
augmenter la granularité ne supprime pas ce retardataire. La prochaine
tranche doit redistribuer les produits pendants et distinguer les gros
callbacks indivisibles. Un plafond de file se traite par poursuite locale,
jamais par perte de résultats.

Les campagnes sont isolées sous `campaigns/`. Leur lecteur s'applique
à ce sous-dossier, pas aux tentatives de qualification ou ThreadSanitizer :

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_parallel_matrix.py check morsehgp3D_v8/receipts/q2_front_workers_20260914/campaigns --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_q2_parallel_matrix.py check morsehgp3D_v8/receipts/q2_front_workers_20260914/campaigns --summary
```

Pour les choix d'objets, les limites et la suite dynamique, lire le
[contrat des workers](../../docs/P0_FRONT_WORKERS_Q2.md).
