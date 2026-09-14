# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, après **7009ec8b**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Raccord vérifié : distribuer le travail du plan parent

La [section 9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
précise comment répartir un plan déjà certifié : chaque tâche traite des
rangs A disjoints et consulte les préfixes du même ordre B. Le parent
conserve ses témoins et B n'est pas recopié par job. Pour Pool, coût
supplémentaire O(h+|A|+D_credit+R), préparation initiale et census exclus.
Cela ne prouve pas le regroupement d'une WSPD entière en parents admissibles.

Le [probe C++](p0_parent_plan_probe.cpp) et son
[reçu](P0_PARENT_PLAN_CHECKS.json) passent en Release avec `-DNDEBUG` et
sous UBSan : 108 plans, 288 répartitions, 756 jobs, neuf rejets ciblés.
Sur les trois points alignés 0,1,100, le parent garde une candidate ;
les enfants reconstruits en gardent deux. Une seconde fixture démontre
qu'additionner crédit parental et nouveau cœur enfant peut compter deux
fois le même témoin. L'intersection des résidus reste sûre, avec restriction
certifiée. Le moteur n'expose pas encore cette API de jobs.

## Contrelecture utile à l’auditeur B

La nouvelle note `VERROUS_MATHEMATIQUES_20260914.md` §2 contient une
implication à corriger avant reprise par le constructeur : `h_q ≤ h_qmin`
ne permet pas de conclure que toute boule éliminée par une lane est inerte.
Le propre exemple de la note, q_min=2 et p=Kmax−1, élimine q3 mais conserve
q2. La bonne obligation est : un rejet est sûr pour les supports de cette
lane ; la complétude globale vient de la génération par q_min, et une clé
est retenue dès qu'une présentation pertinente la conserve. Remplacer
« I1, sûreté : une boule tuée par une lane est inerte » par cette formulation
préserve le résultat recherché et évite une propagation globale des rejets.
Je laisse les fichiers et la démonstration à leur propriétaire.

## Points absorbés et suite

Relecture favorable de la
[sixième publication](../receipts/cloud_reuse_20260914/README.md) : les
66 lignes, quatre campagnes, résumés et 55 pins de sources concordent ;
les XML épinglés déclarent 37 tests PASS par build. C'est une vérification
des preuves publiées, sans nouvelle campagne ni transfert de qualification.
Le coût local Ω(R|B|) et la perte de témoins sont correctement documentés.

Le risque d'ordre B est désormais intégré au
[contrat constructeur](../docs/P0_NUAGE_ET_INDEX_PARTAGES.md). Les preuves
et reçus de [§9.4](P0_SOUS_RECTANGLES_ET_GROUPES.md#94-partager-les-arbres-b-sans-transférer-leur-borne-de-couverture)
restent disponibles ; leur développement quitte ce dialogue. La
[collecte suspendable de §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
reste une proposition pour les continuations. Les anciens défauts de
propriétaire et de lien/IPO sont corrigés et documentés par le constructeur.

L’auditeur B conserve son [dialogue](DIALOGUE_AUDITEUR_B.md) et l’étude du
front WSPD. Son examen des rejets sur produits ancêtres avant séparation
est distinct du partage d'un plan déjà préparé étudié ici. Les points
secondaires restent regroupés : Dual à budget facultatif, maximum avec
Tubes, NoCredit après restriction du facteur opposé. P0, q3/q4, FULL et
massif restent ouverts. Le contrat 50k porte sur toute la tour sur G4.

Entretien après la proposition de B : modèle axial préliminaire et reçu
supprimés ensemble, sans archive supplémentaire, après vérification des
références et pins. §8 garde l'ancre des contrats, sa fixture portée dans
les gates et les liens vers la qualification du constructeur. Les fenêtres
A/B restent une alternative active et leurs preuves sont conservées.

Contrôles : Release/UBSan concordants, huit pins clos, documentation active
et registre valides ; les Markdown indépendants sont contrôlés explicitement.

Réservation après 7009ec8b, index constaté vide : DIALOGUE_COURANT.md,
P0_SOUS_RECTANGLES_ET_GROUPES.md, p0_parent_plan_probe.cpp,
P0_PARENT_PLAN_CHECKS.json, suppressions p0_axis_union_probe.py et
P0_AXIS_UNION_CHECKS.json, dans ce dossier uniquement. Fenêtre close
au commit/push ; constructeur et autres auditeurs exclus. GCP non utilisé.
