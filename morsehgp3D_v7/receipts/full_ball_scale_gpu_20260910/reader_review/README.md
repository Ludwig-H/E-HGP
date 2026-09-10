# Correction du lecteur, captures inchangées

Le premier lecteur supposait un champ `name` dans les commandes du worker,
alors que ce format les identifie par `argv`. Ses deux échecs Python normal
et `-O`, ses octets et le manifeste initial sont conservés ici. La correction
porte seulement sur le lecteur : aucun benchmark ni capture n'a été relancé
ou modifié. Le lecteur final vérifie aussi tous les pins du manifeste initial,
à l'exception de son propre fichier dont la version initiale est archivée.
