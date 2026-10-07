# Juges et pilotes des microbancs durcis (7 octobre 2026)

Réponse du développeur aux constats `CST-0018`, `CST-0213`, `CST-0214` et `CST-0215` de l'auditeur Codex
([contre-audit au pin `95247cf4b`](../audit_socle_microbancs_20261007/README.md)). Le travail détaillé est dans
[`CHANGEMENTS.md`](CHANGEMENTS.md) (fichier par fichier) et [`RAPPORT.md`](RAPPORT.md) (tests et codes). Les règles
d'adoption, les seuils, les statistiques et la géométrie ne changent pas : seulement l'admission des entrées, la
présence et la fraîcheur des preuves, et la réplication.

| Constat | Correction | Preuve locale |
| --- | --- | --- |
| `CST-0215` | lecteur des vidages de M2 : en-tête, profil, bornes, pavage des feuilles, sites, statuts, débuts, compteurs et populations validés avant tout noyau ; refus code 2 | porte `mhgp12_dump_admission_selftest`, 46 mutants dont les six de l'auditeur |
| `CST-0018` | juge de M2 : « adopté » seulement avec toutes les preuves fraîches et rattachées (identité, auto-test d'arène, sanitizers, prises de la session avec jeton et empreinte du vidage, isolation du GPU, rehachage final des binaires et sources) ; sinon « refusé » | quatre injections de l'auditeur rendues « refusé » ; les 45 prises réelles de la session A redonnent à l'octet les verdicts publiés |
| `CST-0213` | M3 : étape `resolution` répétée dans N processus neufs, campagnes ajoutées sans écrasement, juge `juger_m3` (moyenne géométrique des rapports par processus, IC 95 % par bootstrap, seuil de 0,60 sur ng00–02 à K10) | la prise unique de la session B rend « refusé » (une prise sur cinq), ce qui reproduit le constat sur données réelles |
| `CST-0214` | M4 : sections des verticales exigées à $k\geq 2$, toute naissance non jugée par `LEM-T6` interdit le succès | carré K1..4 : correcte, fausse, absente rendent 0, 1 et 3 |
| pilotes | provenance (binaires, vidages et sources rehachés avant et après chaque étape), toutes les phases et tous les ordres exigés, mutants tués par leur réponse géométrique | 22 mutants des gardes nouvelles (`microbancs/outils/mutants_juges.py`), tous tués |

Effet de bord relevé : sur le binaire M4 d'origine, une boule de cellule hors du catalogue provoquait une écriture
hors tableau (`SIGABRT`, AddressSanitizer à `mes_m4.cpp:779`) ; le binaire durci la refuse. Les sessions G4 du 7 octobre
n'ont pas rencontré ce cas : leurs exécutions ont rendu le code 0, toutes les naissances jugées.

**Choix du développeur sur les passes de résolution** (question laissée ouverte par le rapport) : une passe par
processus à K10, dans cinq processus neufs par trame. Une passe y dure de 10 à 30 s à un fil, et les trois bras sont
mesurés dans le même processus : l'appariement tient lieu des passes répétées de `MESURE.md` § 5, et la statistique
porte sur les cinq rapports par processus.

À jouer sur G4 (session D) : M3 avec la résolution répliquée, M4 avec ses preuves, puis M2 sous le juge durci. Tant que
ce rejeu manque, l'adoption de M3 reste provisoire. GCP non utilisé pour ce travail.
