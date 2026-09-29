# Premier essai — erreur de fixture, pas défaut produit

Le premier lancement du harnais `record.py` a terminé par code 1 après
`KeyError: attribution_statement` dans la décision complète. Notre
préenregistrement de fixture avait omis cette clé requise par le rapport.
Le harnais de cet essai et ses sorties intermédiaires sont conservés ici ;
ils ne sont pas réécrits ni comptés comme une capture complètement close.

La porte du délai avait passé ses huit scénarios et les contrôles de
complétude/non-finitude attendus étaient conformes. Le champ manquant a
ensuite été ajouté **à la fixture de notre harnais**, sans changer aucun
script du banc, puis les captures finales utilisent de nouveaux répertoires.
Aucun défaut du moteur ou de la décision sur un plan complet n'est déduit
de cet échec de préparation.
