# Contre-audit du socle et des microbancs v12

7 octobre 2026. Auditeur Codex. **Pin examiné : `95247cf4baf2ebd0856c1ac75670f643d24daa6e`**.
Périmètre : réponse `0dbf69347`, socle T0/M2 `a0091e2b7`, M3/M4 `d8407e374` et corrections documentaires
`95247cf4b`. Cadre : `exploration_v12_hors_registre`, `cpu_reference`, `full_pi0`,
`quantized_u21_input_only`, `public_status=not_claimed`.

**Bilan : trois corrections closes, trois nouveaux constats et le verrou d'adoption CST-0018 toujours actif.**
Les petits contrôles géométriques passent ; les défauts reproduits portent surtout sur l'admission des entrées,
la présence des preuves et le protocole de mesure. Ils interdisent de déduire une adoption des seuls verdicts
actuels des bancs. Aucun résultat historique incorrect sur les données réelles n'est démontré par ces injections.

| Sujet | Conclusion et preuve |
| --- | --- |
| M6, `CST-0209`, `0210` | **clos** : quantiles interpolés corrects, premier usage séparé ; fonctions hôtes réelles, compilation CUDA et refus CLI ; [rapport](session/REPORT.md) |
| Indices M4, `CST-0212` | **clos dans le microbanc** : garde de domaine avant conversion/allocation, offsets u64, refus aux limites ; produit à requalifier ; [rapport](tour/README.md) |
| Juges, `CST-0018` | M2 peut adopter malgré identité hôte en échec, anciens JSON d'un autre cas ou aucun cas décisionnel ; M3/M4 peuvent rendre conformes des sorties sans preuve ; [injections et contrôles](preuves/README.md) |
| Réplication M3, `CST-0213` | `--processus` ne répète pas le chrono de résolution utilisé pour le seuil de −40 % ; une nouvelle prise écrase l'ancienne ; [témoin](preuves/README.md) |
| Verticales M4, `CST-0214` | carré K1..4 : six images correctes donnent code 0, une fausse code 1, toutes les sections `FLOWER` absentes donnent encore code 0 ; [témoin natif](tour/README.md) |
| Vidages M2, `CST-0215` | six entrées invalides avec checksum valide passent le lecteur ; trois cas sans accès dangereux sont déclarés conformes par l'exécutable hôte ; [témoin natif](feuille/REPORT.md) |
| Cache, `CST-0007`, `0019` | défaut v11 reproduit dans le buffer v12 : 262 145 octets admis, 286 720 demandés à l'allocateur, puis conservés hors compte ; [témoin](session/REPORT.md) |
| Contrats numériques/mémoire | sept points passent en cours : corrections documentaires contre-lues, code local et portes encore attendus ; [portée](session/REPORT.md) |

Les volets détaillent leurs comparateurs et leurs limites. M2 est comparé à la référence v11 exécutée sur l'hôte,
sans simuler les courses CUDA. M3 partage des primitives exactes v11 avec son repli ; ce n'est pas un nouvel oracle
de toutes ces primitives. M4 contrôle les images de naissances, pas la naturalité complète. Les campagnes locales
publiées ont été recomptées, sans relire leurs dumps : quatre journaux cités par le manifeste M3/M4 sont absents du
dépôt au pin examiné ; les fichiers présents ont les empreintes annoncées.

Tous les nouveaux témoins sont petits et synthétiques. Ni gros dump, ni contenu de jeu sous licence, ni copie de
sources produit, ni binaire ne sont conservés ici. **GCP non utilisé ; aucun lancement GPU ni campagne lourde.**
La compilation `sm_120` de M6 est un contrôle de construction, pas une qualification appareil. Les scripts et
manifestes des sous-dossiers permettent le rejeu et identifient les sources ; `SHA256SUMS` clôt les fichiers du reçu.
Les conclusions et leurs états courants sont reportés dans `audits/CONSTATS.md`, sans réécrire les anciens reçus.

Les dépôts ultérieurs `26b53648c` (session G4 et nouveaux pilotes) et `c55c12871` (préparation des données)
sont **hors de ce pin** et restent à contre-lire. Le présent reçu ne les qualifie pas.
