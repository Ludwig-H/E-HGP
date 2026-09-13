# Morse HGP 3D v8 — première implémentation P0

Ouverture demandée le 13 septembre 2026, sur `main` uniquement.

```text
phase=exploration_v8_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=implementation_v8_p0
public_status=not_claimed
```

L'audit d'ouverture est suivi, sur demande explicite du 13 septembre,
de l'implémentation P0 mono-thread. Aucun code moteur ni résultat de
performance n'est repris automatiquement. La cible reste toute la tour
HGP FULL K=1..10 à 50 000 points sous une seconde, repli sur toute la tour
1..5, puis 100 ms ; la grande échelle G4 est un contrat distinct.

La structure v7 est conservée pour organiser la refonte : `src/`, `cli/`,
`oracle/`, `tests/`, `bench/`, `cmake/`, `docs/`, `audits/`, `receipts/`.
Le premier module C++20 compare trois méthodes de crédits locaux sur un
rectangle séparé : pool directionnel, parcours conjoint de blocs et tubes
à suffixes certifiés. Le résidu est conservé en sous-produits compacts.
Ce n'est encore ni une WSPD complète, ni un census général, ni une tour
FULL. Les mesures historiques citées restent v7.

## État exécutable

Huit CTests locaux passent en Release GCC 13.3 et en Debug Clang 18.1
avec ASan/UBSan. Les juges géométriques indépendants utilisent des entiers
multiprécision ; le produit utilise des entiers 64/128 bits sur u16.
Les tests couvrent notamment les crédits, les frontières, les identités,
les résidus, la séparation et les contre-fixtures des auditeurs.
Le propriétaire copie les coordonnées dans un stockage privé avant
validation et interdit copie/déplacement/affectation ; le runner rejette
les reçus incohérents et conserve les sorties brutes des essais invalides.

729 mesures mono sont conservées, dont n=8 000/16 000/32 000, Kmax5/10,
s8/10/12, grilles équilibrées/déséquilibrées et nappes. Sur ces rectangles
fixes, s vérifie seulement la séparation ; **la vraie comparaison des WSPD
s8/10/12 reste ouverte**. La préparation des tubes est sous-quadratique,
mais les nappes conservent un résidu quadratique : P0 n'est pas close.

- [Contrat et algorithmes P0](docs/P0_CREDITS_LOCAUX.md).
- [Mesures, limites et décision suivante](receipts/p0_local_credits_20260913/README.md).
- [Sonde reproductible](bench/P0_PROBE.md).

Construction dans un répertoire **neuf**, avec GCC/Clang, CMake et les
headers Boost pour les juges seulement :

```bash
cmake -S morsehgp3D_v8 -B build/v8_new -DCMAKE_BUILD_TYPE=Release
cmake --build build/v8_new --parallel 2
ctest --test-dir build/v8_new --output-on-failure
```

Si Boost n'est pas installé globalement, fournir `-DBOOST_ROOT=/chemin/boost`.
Cette session utilise les headers tiers Boost 1.83 déjà extraits sous
`build/v7_boost_gate/extracted/usr`, en lecture seule : aucun code moteur
ni résultat v7 n'en est repris. Le build produit seul est possible avec
`-DBUILD_TESTING=OFF`, mais ne remplace jamais les gates. Les répertoires
`build/v8_p0_r3_20260913/` et `build/v8_p0_sanitize_r3_20260913/` portent
la version corrigée ; les builds r2 et sans suffixe conservent les passes
antérieures aux corrections de propriété. Tous sont épinglés ; ne pas
les écraser pour poursuivre.

## Commencer ici

- **Priorité P0 confirmée :** [supprimer les histogrammes quadratiques systématiques](docs/PLAN_DE_REFONTE.md#priorité-p0--supprimer-la-préparation-quadratique-des-témoins-locaux).
  Comparer les architectures avant de choisir ; les petits ensembles de
  témoins certifiés sont une piste parmi d'autres. Coût des candidates
  restantes et travail aval inclus ; la première brique ne clôt pas cette priorité.
- [Audit général et décisions](docs/AUDIT_V7_SYNTHESE.md) : verdict, contrats,
  causes de lenteur, ce qui doit être conservé ou refait.
- [Tout l'algorithme expliqué simplement](docs/ALGORITHME_EXPLIQUE.md).
- [Plan de refonte priorisé](docs/PLAN_DE_REFONTE.md).
- [Verrous d'architecture — consignes au futur développeur](docs/VERROUS_ARCHITECTURE.md) :
  après P0, les cinq obstacles à ne pas reproduire, leurs sources,
  changements à comparer et critères de validation.
- [Fausses pistes à ne pas réintroduire](docs/FAUSSES_PISTES.md).

Pour approfondir : [fondements et objet FULL](audits/FONDEMENTS_ET_OBJET.md),
[WSPD, q2/q3/q4 et témoins](audits/WSPD_Q2_Q3_Q4.md),
[code et parallélisation](audits/IMPLEMENTATION_PARALLELISATION.md),
[mesures et contrats](audits/CONTRATS_ET_MESURES.md),
[périmètre et preuves de l'audit](audits/PERIMETRE_ET_PREUVES.md).

Verdict : dernières tours 50k publiées, environ 419 s pour 1..10 et
34 s pour 1..5 ; aucune chaîne GPU FULL industrielle ni qualification
multi-millions. Les optimisations privées ultérieures n'ont pas leur
nouvelle mesure 50k. La v8 démarre sur ces constats, sans statut hérité.

Entrées de suivi : [passation](PASSATION.md), [état de l'audit](audits/ETAT_COURANT.md).
GCP non utilisé pour l'audit d'ouverture et cette première tranche mono.
