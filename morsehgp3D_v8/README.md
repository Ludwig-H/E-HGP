# Morse HGP 3D v8 — crédits et census q2 partagés

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
Le module C++20 compare trois méthodes de crédits locaux sur un
rectangle séparé : pool directionnel, parcours conjoint de blocs et tubes
à suffixes certifiés. Il partage maintenant la préparation des tubes
entre q2/q3/q4 et ajoute un filtre q2 par colonnes exactes et index B.
Le mode additif cumule les témoins disjoints de ces colonnes ; une
intersection facultative conserve les rejets d'un plan local q2.
Les résidus sont conservés en sous-produits ou plages compacts. Un nouveau
census q2 interroge tous les sites, compte par paire ou par groupes, puis
émet les supports sous le seuil avec leurs IDs intérieurs et de coquille.
Ce n'est encore ni une WSPD complète, ni un census q3/q4, ni une tour FULL.
Les mesures historiques citées restent v7.

## État exécutable

La quatrième tranche implémente le [census q2 partagé](docs/P0_CENSUS_Q2_PARTAGE.md).
L'état de recherche tient dans un groupe B, un compte et un curseur de
l'index global ; aucune liste de continuation n'est recopiée ou allouée
par requête. Le premier gate géométrique passe 277 cas et 557 exécutions,
avec 8 376 paires confrontées à l'oracle indépendant et 10 contre-modèles.
Les **31 CTests passent en Release et sous Clang ASan/UBSan**. Le flux
conserve les supports et leurs clés exactes, sans dédupliquer encore les
boules entre supports.

Les [204 mesures appariées](receipts/q2_census_20260913/README.md) paient
maintenant génération, index, préfiltre, census, collecte et émission.
Elles couvrent 8k/16k/32k, Kmax5/10 et s8/10/12 sur rectangles fixes,
puis des essais de composant à 50k. Sur les grilles32k/Kmax10,
l'intersection ∩ Pool avec census individuel prend environ 131 ms,
contre 3,74–3,79 s après le seul filtre additif. Sur les nappes, l'addition
réduit le coût total malgré sa sélection plus chère. Le census partagé
gagne sur certains grands résidus mais perd après l'intersection : moins
de visites ne suffit pas si les tests de groupes sont plus coûteux.

À 50k/Kmax10, le composant individuel mesure 176–186 ms sur les grilles,
mais 4,05–4,09 s sur les nappes. **Ni le contrat de tour à 1 s ni celui à
100 ms n'est acquis.** La croissance mesurée est sous-quadratique dans
ces familles seulement ; WSPD complète, q3/q4, FULL et grande échelle
restent à qualifier. GCP non utilisé.

Les paragraphes suivants décrivent les trois tranches précédentes.

Vingt-six CTests locaux passent en Release GCC 13.3 et en Debug Clang 18.1
avec ASan/UBSan. Les juges géométriques indépendants utilisent des entiers
multiprécision ; le produit utilise des entiers 64/128 bits sur u16.
Les tests couvrent notamment les crédits, les frontières, les identités,
les résidus, la séparation et les contre-fixtures des auditeurs.
Le propriétaire copie les coordonnées dans un stockage privé avant
validation et interdit copie/déplacement/affectation ; le runner rejette
les reçus incohérents et conserve les sorties brutes des essais invalides.
Les affectations de plans/batches conservent intégralement leur cible
en cas de panne mémoire. Les résumés refusent les provenances déclarées
hétérogènes de builds/machines et séparent les deux ordres de mesure.

Les 594 mesures de la deuxième tranche comparent le partage et le filtre
indépendant à leurs références,
avec n=8 000/16 000/32 000, Kmax5/10, s8/10/12 et deux ordres d'exécution.
Pour trois voies actives, le partage divise par trois la préparation
tri/cellules Tubes, à plans
identiques. Le filtre axial q2 réduit le résidu des nappes alignées mais
pas celui de tous les nuages : il est notamment sensible aux rotations.
Les 729 mesures r3 restent historiques, épinglées à `3589a2c9`.
Sur ces rectangles fixes, s vérifie seulement la séparation ; **la vraie
comparaison des WSPD s8/10/12 reste ouverte**. P0 n'est pas close.

La troisième tranche ajoute [648 mesures additives et d'intersection](receipts/additive_q2_20260913/README.md).
À n32k/Kmax10, l'addition réduit le résidu de la nappe complète de
6,48 à 3,93 millions, mais ralentit sa sélection : environ 238 ms contre
57 ms. Sur la grille, l'intersection Pool donne 114 716 candidates en
34–35 ms ; Pool seul garde 378 840 candidates en 2,4–2,6 ms. Le coût aval
doit encore départager ces choix. La borne linéaire des grilles planes
alignées ne se généralise ni aux rotations ni à toute la tour.

- [Contrat et algorithmes P0](docs/P0_CREDITS_LOCAUX.md).
- [Partage et filtre axial expliqués](docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).
- [Addition et intersection expliquées](docs/P0_ADDITION_ET_INTERSECTION.md).
- [Deuxième tranche publiée à 8e406f9b](receipts/shared_axis_20260913/README.md).
- [Première tranche historique](receipts/p0_local_credits_20260913/README.md).
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
les écraser pour poursuivre. La deuxième tranche est épinglée dans
`build/v8_shared_axis_20260913/` et `build/v8_shared_axis_sanitize_20260913/`.
La troisième utilise `build/v8_additive_20260913/` et
`build/v8_additive_sanitize_20260913/`, également épinglés.
La quatrième utilise `build/v8_census_20260913/` et
`build/v8_census_sanitize_20260913/`, désormais épinglés.

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
GCP non utilisé pour l'audit d'ouverture et ces quatre tranches mono.
