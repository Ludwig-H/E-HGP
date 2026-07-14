# Revue de porte Phase 1 — clôture de l'oracle Gamma

> [!IMPORTANT]
> Cette revue ferme uniquement la Phase 1 sur le backend exhaustif `reference_cpu`. Elle ne ferme ni la porte globale G2, ni `v1-correctness`, ni l'obligation de preuve M.1 de `full_pi0`.

## Résultat

- Phase : 1 — oracle CPU exhaustif.
- Backend : `reference_cpu`.
- Profil normatif : `hgp_reduced`.
- Mode : `certified`.
- Contrat : `hgp-reduced-v2`.
- Base de preuve publique exacte : `gamma_exhaustive_reference`.
- Verdict : porte de sortie satisfaite, Phase 1 `completed` et porte d'entrée de la Phase 2A satisfaite.
- Statut scientifique : `hgp_reduced` est la réduction exacte des composantes non triviales de Gamma exhaustif dans le domaine couvert par l'oracle. Le flot Gabriel brut est un diagnostic `partial_refinement` muni seulement de `gabriel_positive_connectivity`; il ne construit pas de morphismes verticaux exacts. `full_pi0` reste `conditional` tant que M.1 n'est pas prouvé.

## Preuves et tests

- Implémentation sémantique : commit `febcd39` (`feat: intégrer l'oracle Gamma exact de phase 1`). Le profil normatif est construit depuis toutes les cofaces Gamma, tandis que le flot Gabriel brut est séparé. La sérialisation exacte reconstruit l'oracle depuis les points et compare structurellement catalogues, filtrations, niveaux, coupes, forêts, hyperarêtes, attaches, lots, couverture, verticalité et compteurs avant de publier `public_status=exact`.
- CI de l'implémentation : workflow `Repository checks` [29372593779](https://github.com/Ludwig-H/E-HGP/actions/runs/29372593779), réussi.
- Suite locale de fermeture : `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py'` — 149 tests réussis.
- Contrats : `python tools/check_contracts.py` — 21 définitions, 21 exemples de schéma et 5 fixtures de résultat validés.
- Documentation et registre : `python tools/check_docs.py` valide 24 documents actifs; `python tools/check_implementation_status.py` valide les 20 phases et leurs portes.
- Références et indépendance : `python tools/check_references.py` valide 5 références locales; `python tools/check_oracle_independence.py` valide les 9 modules de l'oracle exhaustif sans dépendance de production.
- Migration v2 : `python tools/rekey_contract_fixtures_v2.py --check` valide les 5 points fixes sans réécriture.
- Fixture Gabriel permanente : `tests/fixtures/regressions/gabriel_point_set_counterexample.json` reste un cas positif de Gamma et un désaccord attendu du flot Gabriel brut. À l'ordre deux et au niveau carré fermé $83886/3563$, Gamma couvre les cinq points dans une composante, alors que Gabriel produit deux couvertures recouvrantes. Les cofaces non-Gabriel attachent silencieusement une facette avant la fusion visible. Cette contradiction ne peut plus être masquée ni publiée comme une réduction exacte de Gamma.
- Régression verticale permanente : `tests/fixtures/regressions/vertical_q1_growth_target.json` vérifie qu'une cible verticale peut croître après sa naissance. Le contrat rejoue le dernier état fermé après lot pour la contenance verticale, mais conserve l'état strictement antérieur au lot pour les bras et descentes d'attache.
- Anti-faux-exacts : les tests négatifs refusent notamment une forêt Gabriel renommée en Gamma, une coupe Gamma amputée, des artefacts de coupe fabriqués et des attaches `full_pi0` incomplètes.
- Fixtures de capacité : les catalogues sélectionnés `n=12` et `n=14`, la matrice de fixtures de Phase 1 et les régressions minimales restent couvertes par les tests de dépôt. La tour complète `n=14` n'est pas revendiquée par cette revue.

### Matrice `n`/`K_max`

Commande : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 1 2 3 --seed-start 20000 --seeds-per-dimension 100 --point-counts 4 5 --k-max-values 3 4 5 --coordinate-bound 1048576 --permutations-per-seed 1 --metamorphic-stride 10 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_campaign_matrix.json`.

- Commit : `a24008b`; CI [29372887080](https://github.com/Ludwig-H/E-HGP/actions/runs/29372887080), réussie.
- Identifiant de campagne : `572b45298944b89e12c2a302516b07971074f7700f71480e88273a57467f3bf3`.
- SHA-256 du manifeste : `6cfcd85b88d5127eb0f11e8d669f83713d5499bc003a3105d0b5a111ac67804a`.
- Résultat : 1 800 cas et 600 nuages audités, répartis également entre les dimensions affines 1, 2 et 3, zéro échec.
- Couverture : `n<K_max`, `n=K_max`, `n>=K_max+1`, plusieurs valeurs de `K_max` et 1 800 comparaisons de préfixes. `matrix_gate.satisfied=true` et `matrix_gate.evidence_ready=true`.
- Contrôles : 80 774 coupes réduites Gamma, 27 399 lots réduits Gamma, 74 306 affectations verticales réduites et 7 302 divergences Gabriel diagnostiques.

### Cas exhaustif sélectionné `n=12`

Commande : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 3 --seed-start 0 --seeds-per-dimension 1 --point-count 12 --k-max-values 10 --coordinate-bound 97 --permutations-per-seed 0 --metamorphic-stride 0 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_selected_n12_full.json`.

- Commit : `145194d`; CI [29373256554](https://github.com/Ludwig-H/E-HGP/actions/runs/29373256554), réussie.
- Identifiant de campagne : `e99be8833c16ac42161d3fb94bd797cbd26b6cf202c3a2a99a62c8402c2a0826`.
- SHA-256 du manifeste : `edfd0db40f4df85eacdcf15ee9772080b3dcb7c2e211276a7bd063993f0e0011`.
- Résultat : 1 cas sur 1 et 1 nuage sur 1 audités, dix ordres, vingt contrôles de profil, 4 082 cofaces, zéro échec et zéro minimisation.
- Contrôles : 1 560 coupes réduites Gamma, 622 lots réduits Gamma, 1 734 affectations verticales réduites et 1 187 divergences Gabriel diagnostiques.

### Campagne de 10 000 graines par dimension affine

Commande : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 1 2 3 --seed-start 0 --seeds-per-dimension 10000 --point-counts 4 --k-max-values 4 --coordinate-bound 1048576 --permutations-per-seed 1 --metamorphic-stride 100 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_campaign_10000.json`.

- Commit : `782f189`; CI [29375253753](https://github.com/Ludwig-H/E-HGP/actions/runs/29375253753), réussie.
- Identifiant de campagne : `c0aff3b7225baef00d77a4059609e8181d8c8e1798095fc3f0782c735f5cd4a9`.
- SHA-256 du manifeste : `3469bddc4b18513542e4306e7eaee05e889823740c105d3415e61078f275c02b`.
- Résultat : 30 000 cas et 30 000 nuages audités, exactement 10 000 dans chacune des dimensions affines 1, 2 et 3, zéro échec.
- Contrôles : 120 000 ordres, 240 000 contrôles de profil, 980 000 coupes réduites Gamma, 310 000 lots réduits Gamma, 960 000 affectations verticales réduites, 30 000 permutations d'entrée et 300 métamorphismes de chaque type.
- Gabriel brut : 980 000 contrôles d'inclusion positive et 20 000 divergences diagnostiques, sans promotion scientifique.
- Sous-porte : `ten_thousand_seeds_per_dimension=true`, `seed_scale_evidence_ready=true` et `seed_scale_scientific_audit_satisfied=true`. La couverture matricielle provient séparément du manifeste précédent, comme l'autorise `separate_matrix_manifest_may_be_aggregated=true`.

### Provenance

Les trois manifestes utilisent le schéma v2, le runner `run_oracle_campaign` version `2.0.0` de SHA-256 source `1db81da797ba28ea100b28604997ebea1e4cfcb723ce66e1e629d6102ed8598d` et le générateur `generate_affine_cloud` version `1.0.0` de SHA-256 source `a808732f79a17dba5d83ce0a39f6fd0dfeada33082015d2ff83a55a674815985`. Chaque run lie le commit Git, certifie un arbre propre et stable au début et à la fin, compte les nuages et cas, et publie leurs racines SHA-256 ordonnées. Aucun run ne revendique à lui seul la décision de sortie du dépôt : `repository_phase_exit_claimed=false` reste volontairement honnête.

- Matrice : racine des 600 nuages `26a83a636a4f321128d522a8bfb62364720c5558e8f17983fd7477e2ab27ba01`; racine des 1 800 cas `d657ac69fc86a2ebd7c8ed902d427b6e7ac2ba0bd1fd0b6cf86e749e0437eb5e`.
- Cas `n=12` : racine du nuage `507c0921db8bad214512208988ecabe25440014abaceb4a9905503ad1fd4bdae`; racine du cas `2ddc966dceb9ab3c32bf4cc8c0504bb955a368be38d3c4f44a8929d3a46e88d7`.
- Campagne 10k : racine des 30 000 nuages `d695002e72d397c8cd6d7f3ba0294286aae55ea4e918d333113191fb4bc4aa9f`; racine des 30 000 cas `3fe4d4e9b8872b73897ca2cf92e789cfa8a8ae76a863a70d788d590a308432e1`.

## Performance

- Matériel : CPU local, backend Python rationnel exact.
- Matrice : 197,170360622 s internes, soit 0,328617267 s par nuage généré en moyenne.
- Cas `n=12` : 340,186258173 s internes.
- Campagne 10k : 2 227,075588780 s internes, soit 0,074235852 s par nuage généré en moyenne.
- p50/p95 : non mesurés. Ces durées valident la faisabilité de la référence et ne constituent pas des benchmarks de production.
- Mémoire : le suivi externe du run 10k a échantillonné 40 116 KiB de RSS près de la fin. Cette mesure est diagnostique et n'est pas liée cryptographiquement au manifeste.
- Aucun résultat de performance ne participe à la promotion `exact`.

## Limites

- La porte propre de Phase 1 est satisfaite, mais G2 global reste ouvert : aucune tour exhaustive complète jusqu'à `n=14` n'est fournie ici.
- Le cas sélectionné `n=12` démontre la capacité exhaustive sur un cas complet; il ne prétend pas couvrir toutes les classes de nuages de taille douze.
- La notion de « classe » dans l'exigence « 100 graines aléatoires par classe pour `n<=12` » du plan de test n'est définie par aucun document normatif du dépôt. Les campagnes présentes ne sont pas déclarées satisfaire cette fréquence ambiguë. Une définition normative et une campagne distincte restent nécessaires.
- La campagne 10k porte sur les petits nuages `n=4`; la matrice séparée couvre `n=4,5` et les relations à `K_max`. Leur agrégation répond à la porte de sortie de la roadmap, sans les confondre.
- Le runner audite une seule implémentation exhaustive de référence. Il ne constitue ni une seconde implémentation géométrique indépendante, ni un différentiel avec un backend de production ou GPU.
- Les dégénérescences hors du premier domaine générique restent un arrêt explicite, jamais une promotion vers `exact`.
- `full_pi0` reste conditionnel; ni un accord expérimental, ni cette clôture ne prouvent M.1.
- Gabriel brut reste une sous-filtration positive partielle; ses divergences sont attendues et ne sont pas des erreurs Gamma.
- Aucun statut G3, G4, `v1-correctness`, scalable ou GPU n'est fermé par cette revue.

## Artefacts

- Oracle : `reference/morsehgp3d_oracle/`.
- Campagne et minimisation : `tools/run_oracle_campaign.py`.
- Contrôle d'indépendance : `tools/check_oracle_independence.py`.
- Référence EMST indépendante : `tests/reference_emst/` et `tests/oracle/test_independent_emst.py`.
- Manifestes : `tests/fixtures/oracle/phase1_campaign_matrix.json`, `tests/fixtures/oracle/phase1_selected_n12_full.json` et `tests/fixtures/oracle/phase1_campaign_10000.json`.
- Régressions permanentes : `tests/fixtures/regressions/gabriel_point_set_counterexample.json` et `tests/fixtures/regressions/vertical_q1_growth_target.json`.
- Contrat public exact : schéma v2, `reconstruction_contract_id=hgp-reduced-v2`, `proof_basis=gamma_exhaustive_reference`, `backend=reference_cpu`.
- Checkpoint : aucun requis pour ces campagnes CPU locales.

## GCP

GCP non utilisé. Les trois manifestes portent `gcp_used=false`; aucune VM n'a été créée, démarrée, arrêtée ou modifiée.

## Prochaine porte

La Phase 2A est ouverte sur le backend `reference_cpu`, pour la couche commune de prédicats exacts CPU, en mode `certified`. Sa porte de sortie exige zéro signe erroné, une escalade explicite de tout cas indécis, la sérialisation canonique de `ExactLevel`, le replay par bits d'entrée et les campagnes différentielles prévues. La Phase 2B reste bloquée jusqu'à la fermeture conjointe des Phases 2A et 3. G2 ne sera réévaluée qu'avec ses propres preuves jusqu'à `n=14`.
