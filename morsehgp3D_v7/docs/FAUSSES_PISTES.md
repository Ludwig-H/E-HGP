# Fausses pistes et décisions écartées

6 septembre 2026. `public_status=not_claimed`. Cette note garde les raisons
des abandons et corrections sans encombrer les entrées actives. Une piste
non encore qualifiée n'est pas, à elle seule, une fausse piste.

| Idée écartée ou corrigée | Pourquoi ; décision retenue |
| --- | --- |
| Publier tous les niveaux Gamma pour reconstruire FULL | Sous régularité, minima Gabriel, vraies multifusions et parents suffisent. Les portails restent nécessaires au calcul, pas à la sortie. [Preuve](AUDIT_NIVEAUX_GABRIEL_20260905.md) |
| Garder seulement les minima avec les adjacences géométriques héritées | Quatre points réguliers suffisent à perdre une multifusion, même avec toutes les intersections entre régions témoins des minima. Transférer les bons chemins/parents vers les minima, pas supprimer leurs ponts. [Preuve et fixture](SQUELETTE_MINIMA_GABRIEL.md) |
| Remplacer partout le raccourci J=1 par la descente de facettes | La nouvelle descente est correcte mais peut demander deux MEB là où J=1 en demande une. Garder la possibilité d'un hybride, sans supposer que plus petit cardinal signifie moins de travail. [Différentiel rationnel](SQUELETTE_MINIMA_GABRIEL.md) |
| Confondre q−1 liens de fusion avec deux facettes à résoudre | Une directe à support trois peut réunir trois parents distincts. Deux bras en perdent un ; q≤4 borne les bras essentiels en 3D, pas à deux. [Contre-exemple n4](SQUELETTE_MINIMA_GABRIEL.md) |
| Écarter toute directe sans fusion lorsqu'on construit la tour | Un label Gabriel de rang m est aussi une feuille de l'ordre m. Le no-op inférieur ne retire pas cette naissance supérieure. Partager la géométrie entre ordres. [Distinction](SQUELETTE_MINIMA_GABRIEL.md) |
| Supprimer aussi tous les rattachements silencieux | La contre-fixture à cinq points perd une fusion ultérieure. Garder l'effet de ces incidences par résolution certifiée. [Contre-exemple](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) |
| Identifier une composante par sa seule couverture de points | Deux identités peuvent avoir la même couverture. Conserver les feuilles et les parents, sans quotient par l'ensemble de points. [Contrat structurel](CONTRAT_CERTIFICAT_FULL.md) |
| Installer obligatoirement toutes les facettes incidentes comme alias | Le plafond historique bloque 16k/K9 et 32k/K7. Séparer minima et ancres obligatoires du cache dérivé facultatif ; le défaut eager reste un témoin, pas l'architecture massive visée. [Mesures](RESULTATS_MONO_FULL_20260905.md), [nouveau contrat](CONTRAT_CACHE_FULL_PARESSEUX.md) |
| Assimiler moins d'alias à une accélération automatique | Les trois paires 8k lazy économisent environ 28 % de pic mémoire, mais ajoutent des MEB/census J1 et n'accélèrent pas. Conserver le gain de résidence sans promettre un gain de temps. [Comparaison](RESULTATS_MONO_FULL_LAZY_20260905.md) |
| Déduire un gain de tour de la seule baisse des allocations locales | Le delta singleton diminue les allocations sur fixtures, mais les trois paires 8k closes ont des variations de temps de signes opposés. Garder l'économie locale sans la transformer en accélération générale. [Mesures](RESULTATS_MONO_FULL_SINGLETON_20260905.md) |
| Dédupliquer les demandes strictes d'un lot unitaire avant leur résolution | Des facettes distinctes peuvent aboutir à la même racine. Supprimer leurs demandes changerait les choix first-C et les frontières de refus ; dédupliquer seulement les racines rendues. [Qualification et contre-fixtures](CONTRAT_LOT_UNITAIRE_FULL.md) |
| Soustraire après coup les opérations de compression redondantes | À profondeur un, le nouveau coût est deux : charger d'abord les quatre anciennes opérations refuserait à tort au plafond deux. Charger seulement les opérations restantes, prospectivement, avec une unité versionnée. [Contrat](CONTRAT_NORMALISATION_FULL.md) |
| Confondre déplacement du premier plafond et réussite à 32k | La normalisation v2 dépasse l'ancien préfixe de travail, mais K9 refuse ensuite à quatre millions d'appels MEB. Huit ordres réussis seulement, aucun cap relevé ni refus promu. [Mesures](RESULTATS_MONO_FULL_SUCCESSOR_20260905.md) |
| Garder un quota de quatre millions d'appels pour mesurer la croissance complète | Cette valeur était un garde-fou d'essai, pas une nécessité mathématique. La sonde v5 supprime les quotas d'opérations, garde temps/mémoire/types et conserve les anciennes captures comme censurées ou refusées. Ce retrait n'est pas un gain algorithmique. [Nouveau profil](CONTRAT_SONDE_FULL_MEB.md) |
| Arrêter toute une corde q4 après un bloc trop profond | La profondeur n'est pas monotone : un bloc admissible peut suivre. Seul le bloc profond est écartable ; cette optimisation reste à implémenter séparément. [Contre-fixture indépendante](../audits/S1_COURANT.md#7-rejet-précoce-dun-bloc-q4--frontière-de-loptimisation) |
| Borner le coût physique d'un proposeur MEB par le seul ordinal legacy | Un contre-exemple compilé distingue propositions réellement tentées et ordinal de référence. Employer deux charges persistantes distinctes. [Correction](PROPOSITION_MEB_ET_BUDGETS.md) |
| Justifier l'ordre des essais par plusieurs bases positives possibles dans un pivot admissible | Motif corrigé avec l'auditeur : Q positif affinement indépendant et z strict donnent une base positive nouvelle unique, par le plan radical. L'ordre reste observable dans le nombre d'essais et l'admission sous P ; ne pas inventer une contre-fixture native à deux bases. [Preuve corrigée](../audits/MEB_DOUBLE_BUDGET_COURANT.md#réduction-démontrée-des-formes-de-pivot) |
| Activer généralement la proposition MEB parce qu'elle teste moins de candidats | Le q2 immédiat répété ralentit ; les petits lots sont sensibles à l'ordre. Pas d'activation générale ni de seuil choisi sur ces seules mesures. [Résultat négatif](RESULTATS_COUT_MEB_20260905.md) |
| Remettre P à zéro à chaque MEB ou compter un ordinal certifié comme travail F réel | Le premier contourne le plafond de l'ordre ; le second invente des formes non exécutées. Work persistant, charges prospectives et miroir A autour du seul repli F. [Contrat du raccord](CONTRAT_MEB_FULL.md) |
| Déclarer une accélération ou choisir s à partir d'une seule paire favorable | Les paires E/F mêlent gains et régressions. Conserver s=8/10/12, apparier les instruments et distinguer observation de qualification statistique. [Mesures](RESULTATS_MONO_F_20260905.md) |
| Renforcer un rectangle vivant par les minima globaux de h_a et h_b | Ces deux minima sont nuls sur une paire de distance minimale. Utiliser des sous-groupes de crédits avec les populations parentales disjointes. [Preuve](ELIMINATION_BLOCS_WSPD.md) |
| Additionner le cœur raffiné d'un enfant aux histogrammes du parent | Des témoins peuvent être comptés deux fois : A={0,1}, B={10,11} donne quatre crédits pour deux points. Garder les exclusions parentales ou reconstruire une partition disjointe. [Contre-exemple](ELIMINATION_BLOCS_WSPD.md) |
| Employer la boule-cœur centrale pour les histogrammes h_a/h_b | En q3/q4 et s≥8 elle ne contient aucun témoin interne aux facteurs, même pour une sous-boîte d'ancres. Employer un certificat H_min/Ξ_max près des extrémités. [Preuve indépendante](../audits/receipts_block_histograms_20260906/README.md) |
| Transformer hmax≤0 sur une boîte d'ancres en rejet pour chacune | Le minimum peut ne trouver qu'une ancre défavorable. Le rejet devient sûr avec l'ancre fixée, pas pour une boîte variable. [Contre-fixture s8](../audits/receipts_block_histograms_20260906/README.md) |
| Remplacer systématiquement les deux comptages terminaux par un passage avec coins | Résultats identiques sur le test 8k, mais seulement −2,79 % de visites contre +100,77 % de coins ; le front O2 mesuré une fois prend 37,767→38,287 s. Ne pas intégrer en l'état : le premier passage économique évite des coins sur les lanes déjà tuées. [Analyse](ELIMINATION_BLOCS_WSPD.md) |
| Imposer un parcours de blocs pour chaque histogramme d'extrémité | Sur le front uniforme 8k/s8, aucun facteur ne dépasse huit points. La mesure instrumentée passe de 93,819 à 186,560 ms avec les blocs forcés ; le dispatch à huit ne les active jamais. Garder le scalaire ici, sans réfuter l'intérêt du certificat sur de grands facteurs. [Analyse](ELIMINATION_BLOCS_WSPD.md) |
| Promouvoir FULL depuis le lecteur structurel, un digest égal ou les reçus F | Aucun ne certifie la complétude géométrique du producteur FULL. Garder les autorités séparées et le succès relatif explicite. [Contrat producteur](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) |
| Traiter un refus ou une capture interrompue comme une tour rapide | Sans terminal de succès, aucun temps de complétion n'est acquis. Conserver les tentatives négatives sans réparer leurs octets. [Interruption réelle](../receipts/full_gabriel_lazy_interrupted_20260905/README.md) |

## Règle d'entretien du dossier actif

README et PASSATION décrivent le présent et renvoient aux preuves ; leurs
longues chronologies redondantes ont été retirées. Les anciennes versions
de ces textes restent récupérables dans Git. Dix fichiers de cache Python
non versionnés ont été supprimés de `bench/__pycache__/` et
`tests/__pycache__/` ; ils sont régénérables depuis les sources.

Les reçus scellés, fixtures de réfutation et sources nécessaires à leur
reproduction ne sont pas des déchets. Ils restent conservés, y compris
les échecs. `audits/` appartient à l'auditeur indépendant et n'est pas
nettoyé par le constructeur. Les builds et brouillons vont dans `build/`.
Le chemin canonique reste `morsehgp3D_v7/`, sans changement de casse.

Le 6 septembre, les deux brouillons non versionnés de comparaison MEB/croissance
propres à la sonde v4 sont déplacés vers
`build/v7_meb_compare_20260906_preparation/`. Le second n'avait jamais été
exécuté ; aucun n'est réutilisé comme juge de la sonde v5. Ils restent
récupérables, avec leurs qualifications historiques éventuelles. Le runner
direct remplace la nouvelle chaîne d'admission volumineuse pour les mesures,
à la demande de l'utilisateur ; cela ne requalifie pas sa micro échouée.
