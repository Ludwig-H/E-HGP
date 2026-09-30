# Exécution

La première version de la sonde (avant le diagnostic de vote) a terminé normalement en mode normal et −O : sorties identiques, 13 924 contrôles, stderr vides. Les fichiers de sortie de travail ont ensuite été remplacés par ceux de la version finale, avant toute clôture.

Une lecture de statut intermédiaire a été lancée trop tôt, alors que la commande −O était encore en cours. Elle a essayé de lire son stdout alors vide et a donné l'exception ci-dessous. La commande de vérification elle-même a ensuite terminé avec code 0. Ce n'est ni un échec géométrique ni un échec du moteur ; aucune source n'a été corrigée pour cet incident de lecture.

```
Traceback (most recent call last):
  File "<stdin>", line 4, in <module>
  File "/home/codespace/.python/current/lib/python3.12/json/__init__.py", line 346, in loads
    return _default_decoder.decode(s)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/home/codespace/.python/current/lib/python3.12/json/decoder.py", line 337, in decode
    obj, end = self.raw_decode(s, idx=_w(s, 0).end())
               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/home/codespace/.python/current/lib/python3.12/json/decoder.py", line 355, in raw_decode
    raise JSONDecodeError("Expecting value", s, err.value) from None
json.decoder.JSONDecodeError: Expecting value: line 1 column 1 (char 0)
```

Après l'ajout des contrôles de majorité à univers/dénominateur figés, les deux commandes finales ont terminé avec code 0, 35 060 contrôles, stdout identiques et stderr vides. Les protections de la sonde utilisent `need`, qui reste exécuté sous −O. Aucune allocation de taille réelle LiDAR, aucune compilation ni exécution native n'a été effectuée.
