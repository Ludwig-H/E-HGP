# Flag cité Clang — contrôle ciblé du raccord B

Paquet privé, 30 septembre2026. Un seul cas nouveau et son contrôle : CMAKE_CXX_FLAGS contenant exactement `"-freciprocal-math"`, sous Clang18.1.3. Deux configurations et deux prétraitements, chacun borné à10s, aucun objet moteur ni FULL compilé et aucune exécution géométrique.

Le contrôle non cité est refusé avec code1 et mhgp10_fp_flags_interdits. Le cas cité configure avec code0. Le cache conserve les guillemets ; compile_commands.json produit réellement cette option et le décodage de la commande donne -freciprocal-math. Clang ne définit aucune des trois macros contrôlées (__FAST_MATH__, __ASSOCIATIVE_MATH__, __RECIPROCAL_MATH__) sous cette composante ; le TU SiteTree passe le préprocesseur avec code0.

Le défaut est à CMakeLists.txt36 : test lexical d'un flag brut entouré espace/tab, sans tokenisation des quotes. Suggestion au propriétaire : separate_arguments (mode adapté à la plateforme), puis comparer exactement chaque token interdit ; graver le cas cité. Aucune source partagée ou d'intégration modifiée par l'auditeur. Le source documente déjà l'absence de macros Clang pour ces composantes.

Portée : trou de la protection de compilation annoncé, pas preuve d'un faux résultat géométrique, pas qualification de Clang/FULL/large précision/GPU. Les flags normaux actifs/inactifs sont couverts par la nouvelle porte ; ce cas n'y figure pas. Le contrôle cité -Ofast sous GCC serait bloqué au TU, contrairement à la composante Clang observée ici.

La revue lecture seule de la tour confirme la lecture de FE_TONEAREST par pas MEB, quatre filtres coupés, aucun callback utilisateur dans ce pas. Une tâche préalable du pool peut changer son propre mode : le pas le relit ; les modes des autres fils ne sont pas ceux de l'appelant. La porte couvre quatre cas u18, trois modes dirigés, quatre scénarios (48), pas toutes géométries, ni MXCSR changé directement, ni exceptions flottantes/pièges. SiteTree exclut explicitement MXCSR hors cfenv ; aligner la documentation de la tour serait utile. Les sources n'appellent pas fesetround pour imposer un mode.

Observation externe à13:32:58UTC : journal sitetree_B_ctest_gate.txt22/22, codeCTest0,483.51s. C'est faits_math+SiteTree+nouveaux contrôles FENV/flags, pas sept groupes R2 intégrés, ni moteur u24/u32. Cette campagne n'a pas été relancée par cet auditeur.

Les quatre sources sont copiées exactement et leurs empreintes avant/après sont stables. Les stdout/stderr des quatre appels sont séparés dans execution.json ; capture.py original est conservé uniquement pour provenance, ne pas le rejouer comme lecture d'archive. Vérification portable : sha256sum -c SHA256SUMS. Aucun lecteur natif externe nécessaire pour contrôler ces captures.
