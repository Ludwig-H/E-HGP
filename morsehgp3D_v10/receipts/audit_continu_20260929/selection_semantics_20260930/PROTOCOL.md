# Protocole et limites

1. Snapshots complets : selection.py, participation.py, scale.py, dev_scenes.py et mémo.
   Le collecteur vérifie les sources originales avant et après ses deux petites captures.
2. Exécution AST limitée à NodeStats/hard_stats/condense/eom et leurs trois helpers EOM.
   Ni les imports des snapshots ni les appels natifs de dev_scenes ne sont exécutés.
3. Gardes AST : usage de a.phi_date, end_mass, stab ; boucle hard.items(), argument phi courant
   de hard_stats(T,atts,phi), absence de lecture de _phi ; déclaration de masse finale dans le mémo.
   C'est un raccord de lecture de code aux calculs API, pas une campagne dev_scenes complète.
4. API21 : naissances β=1,16,25,100,1600 ; parents 2,2,4,4,−1.
   Trois copies de (β,owner)=(1,0),(4,0),(9,0),(16,1),(16,1),(100,3),(100,3).
   Noms : A=0, B=1, R=2, C=3, racine=4. φ=β^(−1/2), donc tous les calculs sont Fraction.
   L'oracle indépendant intègre la masse restante par cohortes en λ et termine à la première
   cohorte qui laisse moins de mcs points. A : 9→6 à 1/3 puis6→3 à1/2 ; fin à1/2.
5. Échelles : six cas exacts sur β carrés rationnels ; quatre contrôles mêmes z, deux désaccords.
   Les dates/propriétaires restent fixes : seule leur unité change. Le contrôle converti ne change
   aucune fonction auditée. Pas d'approximation Decimal, pas de calcul flottant décisionnel.
6. Préflight historique : premier appel inline de l'auditeur, code1, attendait [3,1,0] alors
   que le parcours réel donne [3,0,1]. Les ensembles sont identiques. UTC exacte non enregistrée,
   donc laissée null ; transcription conservée, sans fabriquer une capture horodatée.
7. Captures normal/−O : dates UTC réellement lues par le collecteur ; codes, stderr, stdout et
   pins avant/après conservés. La première erreur n'est pas masquée par le rejeu corrigé.
8. Premier collecteur : code1, garde AST confondant les contextes Store/Load de la cible de boucle.
   Reçu et check.py antérieur conservés dans preflight_ast_guard.json/preflight_guard_check.py.
   La garde finale vérifie les noms et la forme de la cible, indépendamment de ce contexte.
   Ce premier échec de harnais n'est ni un refus de source ni une nouvelle erreur mathématique.
9. Manifeste externe épinglé, 15 payloads exacts + manifest.json. Aucun refus de hash n'est un
   défaut géométrique. Le lecteur ne rejoue ni le collecteur ni un moteur natif.

Question au développeur : masse instantanée (avec événements/dates de départ et seuils continus) ou
masse finale comme seul critère structural ? Les activations progressive et par marches ont la même
masse finale mais pas les mêmes dates de passage d'un seuil instantané. Leurs condensations ne sont
donc pas automatiquement identiques dans la première sémantique.
