# Actualiser la référence FULL CPU après la décision R1

**Protocole proposé, aucune commande de mesure lancée.** Lecture au pin
`a6e3634fc`. Le commit d'exécution reste à fixer après la décision R1 : ne
pas mélanger ses états dans une même cohorte. `capture.json` épingle les
cinq sources lues ; `plan.json` est un descriptif à instancier, pas un plan
de contrôleur exécutable. Aucun fichier XYZ/IDs n'a été ouvert.

But : remplacer la référence CPU ancienne par des latences du code retenu,
**cache 8 Gio, u21, W48, Session recouverte, FULL K1..K avec verticales et R**.
Cette référence CPU ne constitue pas un nouveau contrat GPU ni une mesure
isolée de l'effet R1, B2, T1-d ou du cache.

## Réemploi des outils existants

`mes_full/pilote_full.py` sait mesurer des processus neufs dans un ordre
tournant, mais son CLI fixe CPU K5 à `min(3,processus) × min(5,passes)` et
ne joue aucun CPU K10. Son option `--essai` ne doit pas servir à déguiser une
campagne réelle. APPARIÉ demande au moins deux bras et 10×10 hors essai ;
ce serait un doublage inutile pour cette seule actualisation.

Le plan utilise donc **la sonde `mhgp12_full_probe` existante et le lecteur
LF partagé**, sans nouveau pilote indispensable. Le conducteur de session
peut développer les commandes de `plan.json` et en conserver les sorties,
codes, délais et commandes comme il le fait déjà. La routine existante
`campaign_frames` expose le même ordre tournant ; elle ne remplace pas la
conservation des métadonnées de chaque processus par le conducteur.

Commande exacte d'une prise ng00/K5, à instancier avec la sonde et le dossier
de données de la session autorisée :

```sh
"$PROBE" "--trame=$DATA/lidar_ng00.u32le,$DATA/lidar_ng00.ids.u32le,ng00" \
  --k=5 --threads=48 --passes=8 --cache=8589934592 --recouvert --digest
```

La voie CPU est sélectionnée par l'absence de `--device`. Pour ng01/ng02,
changer uniquement les trois occurrences du nom ; pour K10, `--k=10`.
Conserver le même binaire Release u21 que la session courante, sa configuration
de compilation et les mêmes six fichiers déclarés que M/cache2. Relever
source, CMake, pilote/LF, SHA du binaire **avant et après toutes les prises**
et hashes d'entrée par le protocole de préparation autorisé. Le cache est
explicite ; sa valeur ne dépend pas d'un défaut futur.

## Cohorte et budget de la session

- K5 principal : **10 processus neufs par trame, 8 passes chacun**, soit
  30 processus, 240 passes dont 210 chaudes.
- K10 informatif : **3 processus neufs par trame, 8 passes chacun**, soit
  9 processus, 72 passes dont 63 chaudes. Ne pas présenter ces trois
  répétitions comme la cohorte K5 à dix processus.
- Ordre : K5 puis K10 ; à la répétition `r`, rotation des trois trames de
  `r mod 3`. Un seul processus W48 à la fois. Aucun préchauffage caché.

Proposition d'enveloppe opérationnelle : **600 s pour ces prises**, après
construction/portes, avec 90 s au plus par processus et jamais au-delà du
temps restant de la session gardée. Ce plafond comprend empreinte,
validation et libération hors mur. C'est un budget, pas une prévision de
vitesse. Toute prise non jouée ou expirée reste visible ; une cohorte K5
incomplète ne devient pas une référence complète. Si l'enveloppe est trop
courte, conserver K5 prioritaire et déclarer K10 partiel/non joué, sans
réduire rétroactivement le nombre exigé pour K5.

## Admission et statistiques annoncées avant les résultats

Chaque commande attend huit passes au schéma **recouvert**, voie `cpu`,
K commandé, W48, profil21, empreinte demandée, avec les comptes déclarés
39 885 / 35 551 / 45 845 pour ng00 / ng01 / ng02. LF contrôle les lignes,
types, partitions, mémoire, séquence et capacités appareil nulles sur CPU.
La métadonnée des comptes n'est pas une nouvelle preuve de déduplication.
Exiger code0 et huit passes conformes pour admettre un processus ; garder
refus, échec et absence distincts. Comparer FUL1 entre toutes les passes et
processus d'un même `(trame,K)` ; ne pas comparer K5 à K10. Une confrontation
à une empreinte historique demande aussi son format et ses métadonnées
compatibles, pas seulement le nom ng00.

Pour chaque processus, publier la première passe séparément et calculer la
médiane des passes **p=1..7**. Pour chaque `(trame,K)`, publier la médiane
des 10 médianes K5 (ou 3 à K10), leur maximum, et le maximum de toutes les
passes chaudes. Si la médiane groupée de MES-FULL est également utilisée,
l'étiqueter séparément : elle n'est pas en général la médiane des médianes.
Ne pas sommer des médianes d'étages ; garder les durées et fenêtres de
chaque passe pour le diagnostic. Aucun seuil d'adoption CPU nouveau.

Le mur va de l'entrée résidente avant P à la tour complète, allocations et
catalogue CPU inclus. Lecture des fichiers, ouverture de Session, validation,
FUL1 et libération sont hors mur et restent publiés. « Chaud » signifie ici
répétition de **la même trame** dans une Session avec cache ; ce n'est pas la
chaîne des 37 trames. `cpu_ns` est le temps CPU de tous les fils ; `pic_octets`
est le pic du budget actif ; `rss_max_octets` est un maximum du processus.
Ils ne sont pas interchangeables. La limite du cache ne prouve pas son taux
de remplissage ; aucune valeur de mémoire inactive non émise n'est inventée.

Les futures différences avec la session M sont descriptives : code,
configuration du cache et nombre de passes changent. Pour attribuer un gain
à un seul levier, il faudrait une comparaison appariée dédiée déjà annoncée,
ce qui n'est pas demandé par ce protocole.
