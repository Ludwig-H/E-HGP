# Audit Codex — état courant v12

10 octobre 2026. **A6c `aa6338ee8` adopté selon le banc relu** ; B3b `81b0883d1`
est en mesure sur ce socle. Cadre : `exploration_v12_hors_registre`,
`cpu_reference ; cuda_g4 pour le catalogue`, `full_pi0`, `quantized_u21_input_only`,
`not_claimed`. Autorité : [registre](CONSTATS.md).

**A6c : gain confirmé dans sa cohorte, réserves ouvertes.**
[Admission indépendante](../receipts/audit_reponses_20261010/session_a6c_admission/README.md) :
85 processus, identités FULL concordantes, règle reproduite exactement. Grandes : rapport
**0,854248**, IC95 **[0,852200 ; 0,855716]** ; gardes ng00/01/02 et A/A respectées.
Médiane des 21 grandes **152,44 → 132,33 ms** ; leurs 126 passes après restent >100 ms.
Sources exactes, codes/stderr et hashes des sondes avant/après enregistrés ; 755 CTests,
7 LiDAR passés. Les 72 mutants sont inférés de la campagne agrégée, sans rapports individuels.
Le seuil 43 900 est calibré sur ces mêmes scènes ; aucune généralisation indépendante acquise.

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

**B3b :** [composition et juge relus](../receipts/audit_reponses_20261010/b3b_prelecture/README.md).
Les neuf corps B3 sont inchangés ; clés toujours transférées en GPU complet, reconstruites en
tranches. Le juge v1 admet encore les identités via résumés ; le correctif v2 reste proposé.
L'admission de la nouvelle campagne doit relire les bruts sans inventer les codes absents.

Massifs inchangés : [record R1 GPU](../receipts/audit_reponses_20261008/session_l2t_admission/README.md)
Paris sans sol 9,111M, K5 **46,453 s froid**, sans qualification chaude transférée à A6c.
[CST-0243](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : TU Wien 5,200M
réussit puis refuse la passe1 pour mémoire ; cause inconnue.
