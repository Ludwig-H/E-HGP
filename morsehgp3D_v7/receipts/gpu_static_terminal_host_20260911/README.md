# Terminal composé c03 — preuves hôte et correction d’identité

Ce paquet privé conserve le premier raccord du terminal géométrique et sa révision après découverte d’une collision réelle d’identité. Cadre exploration v7 hors registre, profil u16, backend `HOST_STUB`, `public_status=not_claimed`. Aucune exécution CUDA, aucune utilisation GCP et aucune mesure de vitesse/RSS ne sont contenues dans cette clôture. La préparation CUDA ultérieure est volontairement **hors** de ce paquet.

Le helper remplace seulement le terminal statique de la référence c03 : sélection MEB, clé primitive, niveau brut comparé rationnellement, lookup de BallId dans sa fenêtre K, intrus Morton strict, remplacement du premier support et descente. La recherche n’a aucun quota ; une trace bornée reste un diagnostic. K1, seeds, DSU, ancres chronologiques et verticalité restent dans le producteur FULL hôte. Le raccord de preuve compare au CPU, puis calcule une seconde fois sans stockage de trace : ce surcroît rend ces gates impropres à une mesure de performance produit.

## Historique conservé, pas réinterprété

Le premier arbre `r2/` conserve O2 r1 avec ordinal u32, O2 r2 avec ordinal u64, et SAN core/T2/guards. Ces tests ont passé leurs fixtures mais ont manqué une collision : `HostOwner` et `IndexOwner` possédaient deux compteurs distincts démarrant à 1, tout en publiant le même type d’IndexView. Un index étranger pouvait être accepté avec le catalogue d’un autre propriétaire. Les anciennes captures ne qualifient donc pas ce contrat d’identité.

La révision `ownerfix/` réutilise le token du nouvel IndexOwner qu’elle construit et possède, et supprime le compteur indépendant. La fixture `--owner-cross`, en processus séparé, construit un IndexOwner étranger puis un HostOwner d’une autre géométrie. Elle exige le refus du mélange réel avant tout appel MEB. Le même juge compilé avec l’ancien owner **byte-identique** échoue exactement sur `guard.cross_owner_must_refuse_foreign_index`, code 1 attendu ; ce résultat négatif est conservé. Les tokens ne sécurisent pas des pointeurs/token arbitrairement forgés et les vues expirent avec leur propriétaire.

## Portée des qualifications

Les six nouvelles captures O2/SAN core/T2/guards sont requises par le publieur. Le cœur confronte 523 MEB K≥2 aux 605 fixtures incluant les refus K1/coupes strictes : 20 851 contrôles unitaires, q2/q3/q4 non vacuants, 197 coquilles supplémentaires, 483 niveaux de représentations différentes mais de valeurs égales, deux ordinaux >2^32. La gate historique FULL compte 256 672 contrôles sur 30 nuages et 124 ordres, avec 88 terminaux/106 traces appariés, 14 descentes strictes et quatre à rayon égal. Static1 et static4 rendent les mêmes sorties.

T2 apporte de nouvelles exécutions du raccord, sans héritage des résultats d’un moteur antérieur : 54 tours K1..10 sur ligne12, coquille12+centre+extérieur (14 points) et spatial12, deux identités, s=8/10/12 et threads0/1/4. La ligne entièrement couverte par les seeds n’exécute pas le helper ; elle ne sert pas de preuve d’exécution du terminal. Les cas coquille et spatial l’exécutent réellement. Les métadonnées d’ordre et l’indexation verticale sont contrôlées, avec quatre mutants indépendants attendus.

Les guards ont 139 contrôles et 31 refus nominaux, plus le mode séparé croisé (5 contrôles, un refus). Les cinq mutants de coupe stricte, fenêtre K, choix du premier support, ordre signé des clés et trace transformée en quota restent causaux ; le sixième est l’ancien propriétaire réel. SAN est exécuté avec ASan/UBSan et détection des fuites activée ; les mutations compilées de guards sont qualifiées O2, tandis que SAN exécute les rejets nominaux et le nouveau mode croisé.

La représentation héritée reste bornée : index sans doublons géométriques, terminal K2..10, catalogue c03 avec coquille ≤12 et intérieur ≤9. La validation du propriétaire est structurelle ; elle ne certifie pas à elle seule la géométrie ou la complétude d’un catalogue externe. Aucune de ces preuves ne promeut la voie CPU active post-seed ni n’établit un contrat de tour 50k/multi-millions.

## Lecture portable et sources

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7_terminal_sources_neuf
```

Le lecteur utilise uniquement la bibliothèque standard et n’exécute ni géométrie ni compilateur. Il vérifie couverture physique/logique, empreintes, sources réellement compilées, codes de sortie et diagnostics causaux. L’extraction est create-only. `sources/current/` rend le code courant directement lisible ; `storage_map.json` reconstruit tous les chemins historiques et snapshots sans duplication de leur contenu. `capture_manifest.json` conserve les pins d’ELF mais aucun ELF n’est distribué, aucun vendor non plus. Les patches séparent raccord FULL, ordinal u64, correction du propriétaire et nouvelle fixture.

Les commandes historiques conservées contiennent les chemins absolus de la session et les dépendances locales. Le lecteur est portable ; relancer une compilation nécessite d’adapter ces chemins et de fournir C++20/Boost pour les juges Gram, avec les flags stricts enregistrés. Le succès du lecteur n’est pas une nouvelle exécution des gates.
