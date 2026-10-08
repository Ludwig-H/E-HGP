# T2-d B : corrections d'admission et résidu FULL informatif

Prototype non publié, base `902041f66`. Pilote `183c18856d…`, manifeste
des bras `3999a137…`, copie stable conservée hors Git. Ce reçu complète
[le préaudit](../t2d_b_admission/README.md), sans modifier ses preuves.
Aucun moteur, compilation, payload ou appel GCP lancé.
Dernier delta observé `6310c1e0…` : trois lignes limitent les plans
informatifs en mode `--essai`. Seule `etape_informations` change ; les
corps d’admission, de jugement et de campagne testés restent identiques.
Pas de second rejeu attribué à ce delta statique.

Les corrections suivantes sont contre-rejouées en Python normal/−O :

- Le contrôle positif complet reste adopté : trois trames, dix tours,
  huit bras et dix passes par prise, 240 journaux distincts. Les lignes
  sont dérivées de la capture native de huit points déjà publiée ; leurs
  durées/configurations synthétiques ne sont pas des mesures.
- Le mur G forgé à1ns est maintenant refusé par la partition, avec
  relecture des bruts. Faux fils, journal partagé et trame manquante sont
  refusés ; une empreinte différente est rejetée.
- Le format FULL `voie=device` est admis ; `voie=appareil` et le journal
  sans configuration/libération sont refusés. Le vrai journal G huit
  points, inchangé, passe aussi les nouvelles bornes temporelles.
- L'auto-test développeur passe : six jugements et treize lectures. Les
  six bras se reconstruisent depuis Git par les 31 substitutions déclarées,
  avec leurs hashes avant/après ; cela ne les compile ni ne les qualifie.

Le veto A/A est un **amendement déclaré à06:04 UTC avant campagne G4** :
moyenne géométrique hors ±1,5% sur une trame décisive ⇒ refus. Le témoin
A/A1,20 est maintenant refusé. L'ancienne règle était explicitement
informative : aucun verdict passé n'est réinterprété rétroactivement.
Le retrait de l'inversion est effectif dans la vraie boucle de campagne,
rejouée avec sondes remplacées par des stubs : sur dix tours, chaque bras
occupe cinq positions paires et cinq impaires, chaque position une ou deux
fois. Cela corrige la parité figée ; cela ne prouve pas l'absence de toute
dérive temporelle ou d'effet du bras précédent.

**Résidu distinct : le lecteur des informations FULL reste partiel.**
Après un contrôle positif conforme à son schéma, quatre corruptions sont
encore admises :

| Témoin | Information invalide encore acceptée |
| --- | --- |
| blocs ignorés | `c_ns=null`, `g_ns={tables:false}`, `hors_mur_ns` chaîne ; CPU/RSS/appareil de types invalides |
| usage mémoire | usage C supérieur à son pic |
| sous-étages | T supérieur à l'enveloppe TMVR |
| G | tables supérieur à G |

`lire_full` contrôle les clés extérieures, la configuration, les étages
principaux et le maximum des pics, mais ne lit pas plusieurs blocs et ne
vérifie pas ces bornes. Les sous-prises FULL sont **informatives** et
séparées du jugement principal sur G : ce résidu ne révoque pas les
corrections d'admission de G et n'annonce aucun mauvais résultat GPU
observé. Compléter les contrôles, ou porter explicitement le lecteur
commun `a2c2fccfd`, avant de qualifier ces informations FULL. Pas de
nouvelle implantation ni de patch produit fourni ici.

`check.py` réutilise le harness du préaudit épinglé ; seules l'interface
FULL et la prise volontairement invalide à1ns sont adaptées. Les sources
de 154Ko et les journaux synthétiques ne sont pas dupliqués dans le dépôt.
Relecture reproductible avec la capture extérieure et le dépôt :

```sh
python check.py --sources /chemin/capture --repo /chemin/depot --check
python -O check.py --sources /chemin/capture --repo /chemin/depot --check
```

Les deux sorties correspondent à `results.json`. Aucune qualification
native, de budget GPU ou de gain G4 n'est transférée par ces contrôles.
