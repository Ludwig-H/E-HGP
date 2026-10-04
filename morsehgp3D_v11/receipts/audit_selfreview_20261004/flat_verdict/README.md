# Contrelecture E1 — décision LiDAR et portée de la porte plate

Sources Git c22be4e41c2f40d150c6ef06154ab420e3451022, delta depuis 1235da4ac  ; 22 empreintes sources, dont 19 blobs copiés et 3 reçus conservés uniquement par empreinte et métadonnées choisies. La relecture après 3bd4d734e confirme ces 22 blobs inchangés. Aucun moteur, fit, bootstrap réel, test natif ou GCP exécuté. Aucun brut KITTI ni clef de connexion copié.

**Écart actionnable avant P08.** Le préenregistrement LiDAR (§primary.H_L2, ligne 367) exige la borne basse IC95%>−0,02. points_flat_summary.py:237 utilise seulement p_holm<0,05 pour claimed ; main --lidar:278–282 écrit directement ce verdict. La recherche limitée à bench/tests/docs/plans ne trouve aucun second décideur appliquant la borne IC.

check_lidar_decision.py charge par AST les fonctions exactes holm et lidar_decision du blob capturé, sans importer NumPy ni le produit. Il injecte cinq distributions de bootstrap, sans rééchantillonnage :

- 300 valeurs −0,021 et 9 700 valeurs −0,019 : p_Holm=0,030096990300969902, IC=[−0,021;−0,019], claimed=True malgré le critère IC faux ;
- la borne basse exactement −0,02 reproduit aussi l’écart avec l’inégalité stricte ;
- IC admissible, Holm en échec et données absentes conservent les verdicts attendus.

Proposition minimale testée : claimed=(p_holm<0,05) et, pour H_L2 seulement, présence de ci95 et ci95[0]>−0,02. Le p ajusté reste affiché séparément ; les trois autres lignes primaires sont inchangées.89 gardes, sorties normal/−O identiques. Ce constat porte sur le décideur, pas sur des résultats LiDAR futurs ; aucune sortie P08 n’est jugée.

**Portée réellement jouée.** claudeflat1a et 1b sont closes/completed, arrêt ciblé certifié, état final TERMINATED. Les deux archives publiées sont rehachées ; chacun des 169 payloads de leur manifeste concorde, 170 membres réguliers avec le manifeste lui-même. Les quatre gate.json copiés sont identiques aux membres des archives.

| Session | Nuages sortie plate | Comparaisons | Fixtures | Mutants |
|---|---:|---:|---:|---:|
| claudeflat1a |1654|119088|32/32|9/9|
| claudeflat1b |641|46152|32/32|9/9|

Le mot native décrit l’export C++ consommé par points_flat_gate.py:93–117. Projection H^r, condensation, sélection et labels restent Python (:121–126). Les neuf mutants sont des branches du consommateur/tête Python, pas neuf mutants C++ compilés. Formulation conseillée : « raccord export C++ → projection et tête Python contre oracle indépendant, exécuté sur G4 ». La tête C++ reste absente.

Les comparaisons directes contre l’oracle portent sur k≤4, mcs 2/3/4, EOM z=1/z=3 et leaf (LINES:38). AST des appels fixtures/compare_cloud : aucun z=2 explicite. Le z=2 retenu sur dev synthétique n’est donc pas directement qualifié par ces portes. Oracle head:654–664 et scores:537 supportent déjà le paramètre entier z générique : ajouter ('eom', 2) puis jouer la porte est la validation minimale avant mesures primaires z2. La prise en charge d’une interface générique ne qualifie pas à elle seule ses nouveaux cas.

**Progrès constaté.** Le port public points_flat_metrics.py:10–18 retire la revendication d’un certificat dual Hungarian : scipy fournit l’affectation flottante et sa somme est recalculée exactement. Aucun défaut de score joué n’en est déduit. R0 est conservé comme ligne officielle ; la tête commune atomique est une ligne distincte. Les choix z1 LiDAR/z2 synthétique et le statut not_claimed sont déclarés ; les résultats dev ne sont pas une qualification du test tenu à part.

Rejouer uniquement les lecteurs autonomes : python3 check_lidar_decision.py, python3 review_scope.py, et leurs versions −O. Les archives ne sont pas recopiées : metadata_origins.json donne leur chemin Git et leurs empreintes, avec les hashes des membres sélectionnés ; leur recoupe complète effectuée ici est un fait de lecture, pas une campagne native supplémentaire. claudeflat0 n’a pas d’archive versionnée dans ce reçu : seule sa métadonnée de fermeture est recoupée ici.
