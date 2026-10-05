# Relecture S5 et mécanisme SIGXFSZ

Capture de 31 sources, contrat S0 à `5adf6a59f`, WIP S5 sur `f98aeed67`.
`SOURCES.json` fixe les octets avant lecture ; `CLOSURE.json` confirme leur
égalité avec les fichiers vivants à la fermeture de cette sous-revue.

Le CLI ignore SIGPIPE, pas SIGXFSZ. Le mécanisme Linux est confirmé sur
un enfant Python borné à 64 octets : signal par défaut → code −25 ; signal
ignoré → erreur EFBIG (27), traitable. **Aucun exécutable HGP ni test natif**
n'est lancé. Cela recoupe le constat C2 du contrelecteur S5, sans rejouer
sa reproduction native ni qualifier le retrait du dossier temporaire.

Le contrat `published_complete`, l'empreinte du manifeste fermé et la
signature autonome V2 sont adoptés par S0 ; la capture S5 ne les implémente
pas encore. Le champ `tree_k_sha256` du manifeste demande un juge contre
la sérialisation indépendante V2, pas seulement une comparaison entre
appels. Les protections actuelles de stdout par stat/inode et O_RDONLY
sont correctes par lecture ; leurs deux portes causales restent utiles.

`check.py` passe 83 contrôles, normal et −O identiques. Le premier essai,
conservé dans `attempts/`, cherchait un nom de variable inexistant ; seul
le vérificateur d'audit a été corrigé.

```bash
python3 -S -B check.py
python3 -O -S -B check.py
```
