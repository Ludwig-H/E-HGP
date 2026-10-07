# Pistes G après publication : calculs certifiés et mémoire du census

7 octobre 2026. **Lecture statique de `9b2747eff364d56215b589c782b1a4e51d59a576`**, propositions non appliquées. Onze sources et la situation T/M/V sont épinglées dans `capture.json`. Aucun build, test natif, chrono ou GCP. Le petit lecteur ne manipule que des suites abstraites de signes ; résultats normal et `-O` identiques.

**Priorité : mémoire et travail inutile**, notamment omission des workspaces à K1, puis étude du stockage borné. Le raccourci de support est une simplification locale du certificat, pas une réponse principale à la régression. Le [contrat de tour, § 4.3](../../docs/CONTRAT_TOUR.md) donne, sur un profil local historique, 2,3 % pour le certificat ; sur G4, les sondes pèsent 38,6–42,9 % et la proposition 25,5–26,0 %. Même supprimer entièrement un poste de 2,3 % n'enlèverait que 2,3 % du temps dans ce profil (Amdahl : facteur maximal 1/0,977 ≈ 1,024). Ce profil partiel n'est pas une mesure du patch ni une borne transférable au G actuel : sondes et proposition restent les priorités temporelles établies.

## 1. Support déjà certifié : jusqu'à quatre prédicats inutiles

`supports.cpp:137` construit d'abord `CertifiedBall::certify(S)`, puis appelle `num::side` sur **tous** les sites de F, y compris ceux de S. Après succès exact de la fabrique, ces derniers sont déjà sur la sphère. Le patch textuel [support_sites_proposed.patch](support_sites_proposed.patch) ajoute un curseur dans S trié : un site de S est ajouté directement à `on`, à sa place dans l'ordre de F ; les autres sites gardent le prédicat et ses refus.

**Authentification, pour toutes les routes actuelles.** Le seul appelant est `locate`, avec deux appels. La route proposée a déjà contrôlé `sorted_subset(S,F)` ; S vient des indices de F et est trié par `propose`. La route de repli reçoit `exact_support`, qui choisit exclusivement des identifiants de F, puis `store` les trie. Les sites de F sont distincts. Les routes table de populations et `LEM-T1` réussies n'appellent pas `certify_part` et restent intactes. `internal.hpp` exige explicitement S trié inclus dans F.

L'authentification géométrique ne vient **pas** de la proposition flottante : la fabrique exacte `num/guard.cpp:37` construit la sphère passant par S et certifie les signes barycentriques (paire distincte ; triangle strictement aigu ; quatre poids stricts). Le raccourci intervient seulement après succès de cette fabrique : le type certifié et ses domaines/paliers numériques doivent être valides selon NUM-CERTIFIEE/NUM-GARDE. La proposition flottante seule n'autorise aucun côté nul, et aucun site hors de S ne bénéficie du raccourci. Le scan garde donc exactement les mêmes sites `on`, dans le même ordre, le même premier site extérieur, puis la même canonisation et la même route table/census. Aucun contrôle `S⊂F`, `F⊂P_b`, décroissance ou cohérence du census n'est retiré.

Le gain physique est **au plus q appels**, exactement q quand le scan de F se termine ; un rejet extérieur précoce peut n'en éviter aucun. Le modèle fournit q=2/3/4 et deux rejets précoces. Les compteurs actuels de G (routes, sondes, contrôles, pas, chaînes, census) ne comptent pas ces appels `side` : ils doivent rester identiques, comme les cibles et les refus sur le domaine contractuel. Les portes de la primitive exacte restent nécessaires ; ce raisonnement ne remplace pas leur qualification.

**Coût à juger.** Curseur monotone, sans recherche de q identifiants à chaque site : O(k+q), une comparaison/branche ajoutée par site, aucune allocation. À q=2 un prédicat entier peut être très bon marché : un gain de temps n'est pas garanti. Avant adoption, compiler la copie proposée et comparer les mêmes cas à un fil, q=2/3/4, succès et rejet précoce, puis G complet sur les captures déjà prévues. Comparer les sorties et **tous les compteurs logiques** ; instrumenter séparément les appels physiques évités. Mesurer ensuite le coût total et ses intervalles, sans extrapoler un compte d'appels à des millisecondes. Aucune de ces mesures n'a été lancée ici.

## 2. Census G : contrat de stockage borné à proposer, pas allocation raccourcie à l'aveugle

`stage.cpp:257` crée W espaces de `n SiteIdx`. Or G demande toujours un seuil k≤K≤12 et refuse une **coquille complète** de plus de 64 sites (`resolve.cpp:184`). Les seuls résultats acceptés nécessitent donc au plus K identifiants intérieurs et 64 identifiants de coquille. Le stockage utile est borné par **K+64**, indépendamment de n.

La modification doit porter un contrat explicite de workspace borné et réutiliser **le même parcours** de `census_workspace.cpp`, avec une politique de stockage différente ; aucune seconde implantation du parcours. La factory actuelle promet exactement n éléments et `belongs_to` vérifie cette taille : raccourcir son Buffer sans revoir ce contrat serait incorrect. Conserver l'identité de l'index, le nombre de sites d'origine, les règles de concurrence et les refus.

Contrat proposé : conserver jusqu'à k intérieurs dans le même ordre : `SiteIdx` croissants, donc rangs de Morton du `Cloud`, jamais ordre lexicographique des positions ni `PointId` ; conserver les 64 premiers contacts et mémoriser le dépassement ; continuer le parcours et tous ses prédicats jusqu'au k-ième intérieur ou à EOF. Si le census sature, ignorer la coquille, y compris son dépassement. Si EOF arrive avec p<k et dépassement, refuser `shell_capacity` avec le même ordre k. Sinon exposer les listes complètes usuelles. **Ne jamais publier un `BorrowedCensus::complete` tronqué**, ni faire passer une coquille tronquée au contrôle du catalogue. Ne jamais refuser dès le 65e contact.

Témoin abstrait au seuil 2 : 65 contacts, puis deux intérieurs. L'ancien parcours et la politique bornée correcte visitent 67 sites et rendent `saturated` ; un refus au 65e contact serait faux. À l'inverse, 65 contacts, un intérieur puis un extérieur conduisent à EOF et au refus `shell_capacity`. Le modèle vérifie aussi le cas exactement à 64 et une saturation avant tous les contacts. Ce témoin illustre l'ordre logique des décisions ; il ne prétend pas réaliser un nuage géométrique ou juger les bornes de l'index.

Sur toute sortie acceptée, garder le même parcours, les mêmes signes, le même arrêt et les mêmes identifiants conserve les compteurs de visites/sites/passes. En cas de dépassement, les refus et leur ordre doivent être rejugés avec des coquilles réelles avant adoption. Stockages, plafonds et marge du cache entrent dans le budget ; une économie de réserve n'est pas une économie de RSS constatée.

Exemple de **formule seulement**, K5, n=60 000, W=48 : réserve brute actuelle 11 520 000 octets, borne proposée 13 248 octets. Aucun temps ni RSS mesuré. Variante immédiate plus simple : quand `orders()==1`, la voie `FirstOrder` n'utilise aucun workspace, alors qu'ils sont tous créés. Les omettre exige d'ajuster aussi l'admission commune (`stage.cpp:239`) et `diag.workspace_bytes` (`stage.cpp:268`), pas seulement la boucle de création. Les sorties géométriques réussies restent identiques ; un ancien refus `memory_budget` peut disparaître grâce à la réserve plus faible. Les allocations, diagnostics physiques et ce domaine d'acceptation mémoire changent donc explicitement. Ne pas annoncer cette économie comme un gain K5.

## 3. Suivi T/M/V

Le rapport du chantier annonce un rebasage `repo3` sur `9c5809919`, avec G publié, suppression des copies du codage des cibles et dépendance `io` explicitement déclarée. `forest.hpp` devient `bc6fd291…`, tandis que `forest_kernel.cpp` reste `9633c2b4…` et `forest_build.cpp` `fc7a8661…` : même pré-passe parallèle, même noyau. Le rapport annonce des portes vertes ; elles n'ont pas été rejouées par ce reçu. `INTERFACE_TMV.md` est inchangé (`1107a0a7…`). Toujours aucune collecte de `ant(b)` ni report explicite de cette partie à T3 à cette capture ; la proposition du reçu `audit_t1b_tour_prepublication_20261007/branches/` reste applicable, en adaptant l'accès aux témoins de la pré-passe. Aucune nouvelle qualification T/M/V n'en est déduite.

Rejouer le seul modèle :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_g_pistes_20261007/check.py
python3 -O -B -S morsehgp3D_v12/receipts/audit_g_pistes_20261007/check.py
```

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `public_status=not_claimed`.
