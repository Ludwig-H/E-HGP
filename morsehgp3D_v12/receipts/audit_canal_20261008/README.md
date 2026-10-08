# Canal allégé — 8 octobre 2026

Le registre de `8fe509df9` est mis en une seule table, triée par identifiant.
Les **73 lignes sont conservées octet pour octet**, avec leurs dix colonnes,
états, témoins, pins et preuves. Seuls les entêtes répétés et préambules
chronologiques quittent le canal : [texte historique](preambules.md).
Le registre courant reste l’unique autorité ; cet historique ne fixe aucun état.

[Contrôle de conservation](conservation.json) : hashes avant/après et des
73 lignes triées. Les lignes se récupèrent au pin source par `git show`,
sans dupliquer le registre entier dans les reçus. Aucun résultat mathématique,
mesure ou clôture ne change ; les ajouts futurs feront évoluer le registre.
