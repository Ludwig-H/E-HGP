# T2-c : compatibilité avec trente prises LiDAR locales

Complément CST-0018, 7 octobre 2026. La [proposition publiée](../t2c_pilote_proposition/README.md)
admet les **30 prises réelles** de l'essai local : K5/W3/P3, deux tours,
cinq bras, trois trames (39 885/35 551/45 845 sites). Tous les résumés
concordent ; un digest final commun par trame. Les quatre leviers restent
refusés uniquement pour P3<6 et deux tours<8. Aucun gain qualifié.

[Manifest](manifest.json) : trente hashes, 156 329 octets externes,
rapport et pilotes épinglés. Aucun journal ni patch recopié. Le script
réutilise la proposition en copie temporaire et vérifie la stabilité des
entrées avant/après. Normal et `-O` rendent les mêmes [résultats](results.json).
Admission structurelle et identité finale seulement ; aucun moteur relancé.
Ce reçu dépend des journaux locaux conservés par le développeur.

```sh
python3 -S check.py --journaux <dossier-sortie-epingle> --check
python3 -S -O check.py --journaux <dossier-sortie-epingle> --check
sha256sum -c SHA256SUMS
```
