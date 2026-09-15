# Capture interrompue par le renouvellement de l'environnement

Constat du15 septembre2026,07h32UTC : l'ancien identifiant de session
n'existe plus ; aucun processus de la qualification n'est vivant dans
l'environnement hôte. Le conteneur et les processus VSCode ont redémarré
vers07h28. Le runner n'a donc pas pu fermer son reçu : MANIFEST seul,
aucun record, aucune COMPLETION. Ce dossier reste **incomplet**, jamais PASS.

Le journal CTest temporaire encore présent a été sauvegardé ici dans
`ctest_interrupted.log` avant toute relance (texte conservé, dernier saut
de ligne vide normalisé :58 131octets au lieu de58 132). Son dernier test fini est
`mhgp8_campaign_gate`, PASS à07h24 ; cela ne prouve pas la fin des69tests.
La sortie brute du collecteur tué n'est pas récupérable depuis la nouvelle
session. Les sources et binaires n'ont pas été modifiés ; la reprise doit
utiliser un nouveau dossier d'essai et préserver ce témoin de l'interruption.
