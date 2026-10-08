# A6b : première campagne refusée, répétition rejetée

Admission du 8 octobre 2026, source **f2c106d93**, référence **47feedc96**.
Lecture locale des sources, métadonnées et JSONL ; aucun moteur, compilation,
contrôleur, appel GCP ou lecture de coordonnées/IDs LiDAR. Le
[constat de 17:08](../a6b_reprise_sans_resultats/README.md) reste une observation
historique correcte : l'archive de la première campagne est maintenant
disponible à travers une autre session de récupération.

## Provenance fermée

Les deux campagnes utilisent **exactement les mêmes paquet et plans**.
Les sources avant/après, **369/372 fichiers** des scopes natifs, bancs,
tests, CMake et lecteur, sont égales aux commits annoncés ; le pilote et
ses modules sont épinglés. L'émetteur FULL est inchangé. La règle littérale
écrite à 14:47 est conservée : IC haut grandes **< 0,95**, chaque ng
**< 1,01**, veto A/A ±1,5 %, identités FUL1.

- Première campagne `a6b`, rapatriée par `a6br` : archive interne
  **569 050 octets**, SHA `9833e053…b1f4d`, **235 entrées de manifeste**.
  Son `worker.exit=1` est expliqué par les codes des quatre commandes
  **0 / 3 / 0 / 0** : le pilote refuse la comparaison ; les autres portes
  réussissent. Le worker clôture ses quatre commandes, sans interruption.
- Session de récupération `a6br` : archive externe **576 696 octets**, SHA
  `8039d217…ac6c`, **37 entrées** ; une commande `archive_a6b`, code 0.
  Le manifeste externe et le SHA du fichier interne sont tous vérifiés.
  Cette session ne constitue pas une troisième mesure.
- Répétition `a6b2` : archive **564 448 octets**, SHA `8f391098…42ec`,
  **235 entrées** ; quatre commandes de codes 0. `DONE=0`, worker 0,
  résultats vérifiés. Le code 0 du pilote signifie campagne jugée ; son
  verdict demeure **rejeté**.

Les deux sessions achevées ont leurs arrêts ciblés **RUNNING → TERMINATED**
certifiés, code 0, sans erreur. L'arrêt de la première session est celui
de sa récupération déjà admise à 17:03. Chaque campagne donne **754 CTests
du socle, 7 LiDAR et 1 lanceur de mutants réussis**, aucun échec ni saut.
Ce lecteur ne reconstruit pas séparément les 66 exécutions causales de
mutants à partir de ce seul résumé de lanceur.

Chaque campagne ferme **85 processus et 1 306 passes FULL** : 100 passes
d'identité, 756 passes des grandes trames et 450 des trois ng. Les passes
décisives chaudes sont **783**. Les deux bras FUL1 et les passages W1 sont
identiques ; les configurations Release/u21/CUDA, les ELF initiaux et
finaux, les journaux et leurs résumés sont relus. A/A réutilise le même
ELF. Les GPU sont déclarés libres aux deux extrémités observées.
Le bootstrap indépendant reproduit **exactement** les statistiques et
les verdicts, sans différence d'arrondi enregistrée.

## Verdicts distincts

La première campagne est **refusée** : A/A ng01 = **0,9569424**, hors de
[0,985 ; 1,015]. Le bras avant du tour d'indice 2 a une médiane de
**83,970296 ms**, contre 65,39–66,04 ms pour ses quatre autres processus.
On conserve toutes les prises ; ni retrait de ce processus ni jugement
rétrospectif avec une cohorte réduite. Son gain apparent sur les grandes
trames, GM 0,85217, reste informatif dans une campagne refusée.

La répétition est **rejetée** : identité correcte, A/A recevables, gain
important sur les grandes trames, mais deux gardes ng manquées.

| Cohorte | Rapport géométrique après/avant | IC 95 % | A/A |
| --- | ---: | --- | ---: |
| 21 grandes | 0,849071 | [0,847444 ; 0,850893] | 0,997350 |
| ng00 | 1,008600 | [1,006018 ; **1,011640**] | 0,995573 |
| ng01 | 1,012978 | [1,006744 ; **1,019481**] | 1,001384 |
| ng02 | 0,956491 | [0,955779 ; 0,957203] | 0,995161 |

## Temps de la répétition

FULL K5 à chaud, **catalogue GPU et tour CPU W48**, u21, cache 8 Gio.
Médiane des cinq médianes de processus, neuf passes chaudes par processus :

| Trame | Sites | R1 avant (ms) | A6b après (ms) | Maximum brut avant / après (ms) |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 39 885 | 80,073634 | 80,692875 | 85,888606 / 82,227445 |
| ng01 | 35 551 | 65,967184 | 66,671025 | 76,163275 / 77,919984 |
| ng02 | 45 845 | 83,122575 | 79,579615 | 85,197126 / 86,481105 |

Les trois trames restent sous 100 ms pour leurs **135 passes chaudes après**.
Sur les 21 grandes trames, la médiane des médianes par trame passe de
**152,754103 à 132,303583 ms** ; la pire médiane de **289,254605 à
241,637358 ms** ; le maximum brut de **291,917600 à 242,388635 ms**.
Les **21 médianes et les 126 passes chaudes après dépassent 100 ms**.
Le gain géométrique de **15,09 %** vient des rapports appariés par tour et
trame ; ce n'est pas le quotient des deux médianes globales ci-dessus.

Ces observations actualisent aussi les chronos de la référence R1, sans
adopter A6b. Aucun nouveau temps CPU seul ni K10 à chaud : les K10 présents
sont des **identités à froid seulement**. FULL exclut lecture d'entrée,
ouverture de Session, validation, empreinte et libération. Le budget actif
n'inclut pas le cache hôte inactif ; RSS et mémoire de la carte restent
distincts. Les 37 trames sont couvertes en identité ; la cohorte de temps
décisive contient 21 grandes et les trois ng, pas 37 nouvelles mesures.

Le retrait d'A6b et l'explication des fins de G/queue se qualifient
séparément. Le compteur de priorité n'atteste que l'épuisement des tranches
G **réclamables**, pas l'achèvement de toutes les tranches déjà en vol.
Ce reçu n'attribue pas causalement les pertes ng à un levier particulier.

## Rejeu

```sh
python -B -S check.py --repo /workspaces/E-HGP --first /workspaces/.ehgp-sessions/v12.20261008.a6b --second /workspaces/.ehgp-sessions/v12.20261008.a6b2 --recovery /workspaces/.ehgp-sessions/v12.20261008.a6br --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/a6b_closed_20261008
python -B -S -O check.py --repo /workspaces/E-HGP --first /workspaces/.ehgp-sessions/v12.20261008.a6b --second /workspaces/.ehgp-sessions/v12.20261008.a6b2 --recovery /workspaces/.ehgp-sessions/v12.20261008.a6br --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/a6b_closed_20261008
sha256sum -c SHA256SUMS
```

Les sources intégrales du lecteur et les journaux restent dans la capture
externe. Le reçu ne versionne que champs publics, agrégats et empreintes,
sans identité de compte, cible distante ou commandes privées.
