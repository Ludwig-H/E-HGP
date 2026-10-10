# Audit Codex — état courant v12

10 octobre 2026. **A6c adopté ; B3-K, clés seules, adopté en `2aaed1847`.**
Cadre : `exploration_v12_hors_registre`,
`cpu_reference ; cuda_g4 pour le catalogue`, `full_pi0`, `quantized_u21_input_only`,
`not_claimed`. Autorité : [registre](CONSTATS.md).

**A6c : gain confirmé dans sa cohorte, réserves ouvertes.**
[Admission indépendante](../receipts/audit_reponses_20261010/session_a6c_admission/README.md) :
85 processus, identités FULL et règle concordantes. Grandes : rapport **0,854248**,
IC95 **[0,852200 ; 0,855716]** ; gardes ng00/01/02 et A/A respectées.
Médiane des 21 grandes **152,44 → 132,33 ms**, toutes >100 ms. Seuil 43 900 calibré
sur ces mêmes scènes ; qualification native et limites de preuve dans le reçu.

**Session O : derniers temps FULL chauds, u21/W48/cache 8 Gio.**
[Admission](../receipts/audit_reponses_20261010/session_fullo_admission/README.md),
[statistiques indépendantes](../receipts/audit_reponses_20261010/fullo_temps/README.md).
GPU signifie catalogue GPU puis tour CPU. Médianes chaudes réunies, en ms :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5** | **80,36** | **66,93** | **79,34** |
| **CPU K5** | **354,73** | **298,50** | **355,14** |
| GPU K10 | 492,64 | 365,52 | 422,87 |

37 trames/six séquences, cinq secondes visites chacune : médiane des médianes **116,59 ms**,
pire médiane **239,77 ms**, maximum **241,89 ms** ; **15/37** sous 100 contre 14 dans N.
**Contrat non tenu.** Sources 496/CTests 755/archive vérifiées ; codes/stderr individuels et
hashes finaux des sondes absents dans O : relecture concordante sous ces limites.
Lecture, masque, ouverture, validation, empreinte et libération hors mur ; CPU K10/37 absents.

**Suite performance : ne plus viser seulement K5 et la queue.** O−N est descriptif entre
sessions : FULL moyen −16,60 ms, fenêtre G +3,14, queue −19,63. Ne pas sommer les médianes.
Dernier registre K5 : 170/185 → 17/185 ; K2/K3 terminent désormais 164/185 passes.
Queue annulée seule, autres termes fixés : seulement **16/37** médianes sous 100, pire 219,23 ms.
G inclut le travail forestier concurrent ; mesurer préfixes publiés/consommés et contention
par ordre avant de changer les priorités. Juger B3b sur FULL, pas sur G seul.
CPU : C dépasse 100 ms sur 36/36 chaudes ; la [piste census](../receipts/audit_reponses_20261008/cpu_census_reduction/README.md)
reste proposée. Petits : C1/C2/C3 non tenus ; W48 plus lent que W4 sur 20/20 très petits
réels, CPU/GPU, signal à confirmer par un banc avant politique automatique.

**CST-0244/0245 restent ouverts.** [Preuve de borne](../receipts/audit_reponses_20261010/a6c_math/README.md) :
le supplément N est exigé même chaîne inactive. [Patch isolé proposé](../receipts/audit_reponses_20261010/a6c_admission_proposition/README.md) :
choix figé avant la borne, bascule tardive refusée ;27 990 cas de modèle, sans qualification native. [Portes](../receipts/audit_reponses_20261010/a6c_portes/README.md) : les petites fixtures
de budget/pénurie restent OFF ; forcer ON/OFF et tester allocations/refus. La bascule actuelle
couvre le succès sans limite. Aucun défaut géométrique nouveau démontré, aucun natif exécuté ici.

**B3b clos :** [admission](../receipts/audit_reponses_20261010/session_b3b_admission/README.md),
[statistiques](../receipts/audit_reponses_20261010/b3b_stats/README.md) :300 processus/2 100 chaudes,
clés et lot passent, balayage seul rejeté. Clés seules : gainGM **1,44–2,85 %** sur cinq trames ;
ng00/01/02 **78,63/64,90/78,12 ms** (médianes de médianes, différentes de celles de O).
Aucun bénéfice ajouté du balayage établi. [Produit B3-K vérifié](../receipts/audit_reponses_20261010/b3k_produit/README.md) :138 fichiers src identiques au bras clés mesuré. Aucun nouveau jugement
des 37 trames/CPU/massifs. Le juge v1 reste incomplet ; bruts relus ici, codes G/stderr et
clôture ELF partielle restent des réserves. Sources495/CTests755 concordants.

**Aide mathématique G :** [réponse aux quatre questions](../receipts/audit_reponses_20261010/REPONSE_G_TRAVAIL.md).
Composantes locales séparables pour coquilles étendues ; transfert entre ordres limité ;
première sonde par vue triée sans copie proposée. Preuves/modèles, aucun gain natif acquis.
D7 couvre toutes tailles : les 99 099 sites restent dans le contrat.
Suite concrète : [arrêt à égalité exacte](../receipts/audit_reponses_20261010/g_population_egalite/README.md)
et [LEM-T1 sur F\S seulement](../receipts/audit_reponses_20261010/t1_support_population/README.md),
patches proposés, 53 560 et 24 300 cas de modèle ; mesurer séparément avant adoption.
[Profil B3b relu](../receipts/audit_reponses_20261010/b3b_profil/README.md) : 75–77 % de premiers hits ;
profil du lot, pas du produit clés seules. Aucun gain nouveau mesuré.

**G4 :** [contrôle réel19:47:39 UTC](../receipts/audit_reponses_20261010/g4_controle_b3b/README.md),
même génération arrêtée avant/après, garde code0, zéro VM E-HGP active à cet instant.

Massifs inchangés : [record R1 GPU](../receipts/audit_reponses_20261008/session_l2t_admission/README.md)
Paris sans sol 9,111M, K5 **46,453 s froid**, sans qualification chaude transférée à A6c.
[CST-0243](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : TU Wien 5,200M
réussit puis refuse la passe1 pour mémoire ; cause inconnue.
