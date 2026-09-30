# Collision des sorties du banc — contre-exemple isolé

Le vrai `cmd_run` du développeur (`scale_run.py`, SHA256
`14f3915daef73088360cf5d90be1a76aab81666ddb22764dc2ea003822714dea`) est appelé
avec `measure` simulé : un catalogue et une tour donnent deux enregistrements
déterministes. Aucun enfant, binaire HGP, GCP ou mesure géométrique n'est lancé.

Contrôle : destinations CSV et JSONL distinctes, code 0, les deux formats sont
valides. Contre-exemple : destinations exactement égales, même code 0 et message
`ok`, mais les deux formats sont invalides. Deux descripteurs ouverts en `w`
écrivent avec leurs offsets indépendants dans le même fichier (`cmd_run`, ligne276).

Deux interprètes Python seulement ont été exécutés : normal et `-O`, chacun avec
le contrôle et le contre-exemple. Les deux sorties complètes et stderr sont
conservés. Les trois fichiers d'export sont ceux de l'exécution `-O` finale ;
leurs empreintes correspondent aussi aux empreintes déclarées dans la capture
normale. Le snapshot source est identique à la source lue avant/après les appels.
Le pilote importe le chemin historique vivant explicitement haché : ce paquet
est une capture close, pas un lecteur autonome ni une preuve FULL/performance.

Correction proposée : refuser l'identité des destinations avant toute troncature,
puis couvrir les identités de chemins et d'inodes (liens existants) par la porte.
Cette sonde prouve seulement la collision du nom exact, pas toutes ces variantes.
