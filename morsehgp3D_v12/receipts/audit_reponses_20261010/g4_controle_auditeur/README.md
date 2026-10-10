# Contrôle G4 de fin d’audit — 10 octobre 2026

Contrôle réel demandé par l’utilisateur, **18:09:45 UTC** : cible de la session
B3 close, même génération ; **TERMINATED avant et après**, garde ciblée code0,
**zéro VM E-HGP active** dans l’inventaire. Aucun nouvel ordre d’arrêt nécessaire.
Le verrou commun était acquis ; aucune session ni mesure n’a été lancée.

[capture.json](capture.json) conserve les statuts et empreintes anonymisées.
Les réponses brutes restent hors dépôt ; aucune identité de compte ou de VM
n’est publiée. La garde a été comparée à son pin avant exécution. Le contrôle
constate cet instant ; il ne préjuge pas d’un redémarrage ultérieur.

Relecture des réponses capturées, sans appel cloud :

```sh
python3 -B check.py /chemin/snapshot /chemin/session_t2db3/receipt.json
```

Le lecteur est repris à l’identique du reçu du 8 octobre ; il recalcule les
hashes, recoupe cible/génération/label/statuts et vérifie l’inventaire.
