# Normalisation des chemins du manifeste

Le premier contrôle de publication dans l'index Git a refusé les chemins
`./README.md`, etc. : ils sont locaux mais non canoniques pour ce contrôle.
Le manifeste initial, SHA-256
`63dd3bbebb5b9822273eec47ee4f3a66e0d556d4d5a56530811f5c4092274fa7`,
est conservé octet pour octet sous `SHA256SUMS.original`.

Le nouveau `SHA256SUMS` retire seulement le préfixe `./` des chemins et
couvre en plus ce manifeste original et la présente note. Les hashes de
tous les fichiers antérieurs sont inchangés ; aucune capture, source,
commande ni conclusion des tests n'est réécrite ou rejouée.
