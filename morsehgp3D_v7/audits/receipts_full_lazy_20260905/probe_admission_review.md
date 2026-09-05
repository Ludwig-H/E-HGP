# Admission n=8 de la sonde lazy : nouveau reçu clos

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

L'admission publiée est contre-vérifiée : **24 reçus réussis, 156 ordres et 11 refus de parsing**. Les vrais rejeux Python passent normalement et sous `-O`. Le juge publié reste `8d8a612a`, avec 19 mutations de selftest ; le contrôle exact `first_C` n'y a pas encore été ajouté. Aucune nouvelle identité ne réfute les valeurs nominales.

Le périmètre s'arrête au [paquet d'admission clos](../../receipts/full_gabriel_lazy_probe_20260905/README.md), dont le terminal de collecte est daté **17:36:55.295111 UTC**. La compilation enregistrée s'est terminée à 17:35:56.011897 UTC ; le binaire déclaré est `1d5a38cea99555fd2db474ee43aff6ba1ee708208508cfa97c540774d0bb7e78`. Cette contrelecture ne lance ni ce binaire ni un build, et ne suit pas la campagne lourde distincte. La [revue historique du digest](digest_probe_review.md) conserve ses octets et sa date d'observation antérieure à la disponibilité de cette admission.

## Cohérence du paquet et exécutions Python

Le [replayer](probe_admission/replay.py) vérifie les tailles et hashes des 467 entrées du manifeste, les 468 entrées de SHA256SUMS, ainsi que l'inventaire fermé des 469 fichiers publiés. Les 54 entrées des cartes de sources concordent avant/après compilation et tentatives ; les 40 dépendances déclarées sont reliées à ces cartes. Les hashes des reçus de compilation et d'admission sont liés à publication.json. Le binaire est explicitement omis du paquet exporté ; son hash y est conservé. Cette vérification ne transforme pas le build local en build hermétique et ne recompile pas ses dépendances.

Les 24 tentatives couvrent n=8, s=8/10/12, Kmax demandé 5/10, EAGER et lazy avec capacités 0/1/1 000 000. Pour Kmax=10, la borne effective est **K=8**, pas K10. Chaque horizon comporte douze tentatives, soit 60 puis 96 lignes d'ordre. Les codes de sortie, flux bruts et champs du reçu concordent avec les commandes capturées.

Les [reçus normal](probe_admission/normal.json) et [optimisé](probe_admission/optimized.json) conservent chacun 26 invocations Python : 24 jugements CLI et les selftests sur une vraie capture EAGER puis une vraie capture lazy C1. Chaque selftest rejette ses 19 mutations de données au motif attendu et accepte le positif réel ainsi que les deux refus synthétiques documentés. Cela donne 19 mutations distinctes exercées sur deux fixtures par mode ; ce ne sont ni 38 mutants produit distincts ni de nouvelles exécutions moteur.

Les onze refus de parsing sont relus dans leurs flux capturés : code 2, `invalid_input`, motif `probe_arguments`, digest global vide. Ils relèvent de l'admission du parseur et ne sont pas passés au juge comme reçus de calcul complets. Le `--digest-selftest` enregistré annonce 24 contrôles, zéro échec et code 0. Cette observation est lue dans son reçu fermé, sans le relancer.

## Ce que les empreintes observées ajoutent

Dans chacun des deux groupes de douze tentatives, les empreintes d'entrée, les vecteurs de digests par ordre et l'agrégat concordent entre s et politiques. Le juge recalcule la liaison de l'agrégat depuis les hashes individuels déclarés. Il ne recalcule pas les arbres absents du paquet ni la géométrie de leurs catalogues. Le wire et ses limites restent ceux de la revue historique : forêts horizontales étiquetées, égalité d'empreintes sous l'hypothèse cryptographique habituelle, aucune carte verticale ni extension d'oracle à K10.

Ces petites exécutions ferment une admission fonctionnelle et de transport. Leurs horodatages de verdict englobent les jugements après la capture ; ils ne sont pas utilisés comme chronomètres du moteur. Aucun gain de temps ou de RSS n'est déduit de cette admission.

## First-C : valeurs nominales bonnes, dent du juge toujours ouverte

Les **117 lignes lazy**, dont **81 avec des requêtes de portail non nulles**, satisfont indépendamment `cache_inserts = min(cache_entries, portal_requests)`. Les 39 autres lignes sont EAGER. La règle ne révèle donc aucun défaut du producteur observé.

La contre-fixture historique peut désormais partir d'une vraie capture : `n8_s8_k10_lazy_c1000000`, ligne K2. Elle annonce deux requêtes, deux insertions et zéro skip. La corruption remplace les insertions par 1 et les skips par 1, en modifiant ensemble le brut et le miroir du reçu. Le cœur du juge publié accepte toujours cette capture corrompue comme cohérente ; la politique `first_C` impose pourtant deux insertions, puisque la capacité vaut un million.

Le replayer conserve les anciennes et nouvelles valeurs, le hash du brut reconstruit et le verdict obtenu, normalement et sous `-O`. Ces octets corrompus ne préservent pas le scellement du paquet original : il s'agit d'une lacune du cœur du juge, pas d'un contournement du manifeste. Ajouter la relation au succès et cette mutation au selftest reste une correction Python ; aucun nouveau moteur n'est requis.

GCP non utilisé. Les résultats de la campagne lourde, même si elle termine pendant cette contrelecture, restent hors de ce reçu.
