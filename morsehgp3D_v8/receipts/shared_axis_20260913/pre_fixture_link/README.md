# Capture préliminaire, hors qualification finale

Le premier CTest Release a démarré pendant la recompilation du juge axial
après l'ajout de la fixture additive à quatre sites. Ses 21 tests passent,
mais la sortie du juge axial provient encore de l'ancien binaire :
157 plans et aucun compteur `conservative_cases`. Cette capture ne
qualifie donc pas la dernière fixture. Elle est conservée sans lui
attribuer les nouvelles sources ; la suite entière est réexécutée après
la fin des builds dans le XML Release du dossier parent. Aucun benchmark
de performance n'a été lancé pendant ces compilations ou CTests.
