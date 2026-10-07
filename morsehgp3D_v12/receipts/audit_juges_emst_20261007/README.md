# Contre-audit des outils, des juges et d’EMST

7 octobre 2026. Pin principal **`1f7642e105aebd76632c58c63fdfd5b5c0824779`** ;
complément T2 au pin **`76adb8fa9eb10894b6a363672e7f28cc3f12e75a`**, explicitement séparé.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Auditeur du développeur ; aucun code produit modifié.

| Volet | Contre-épreuves |
| --- | --- |
| [Données et cache](donnees/README.md) | anciens défauts reproduits puis corrigés ; 14 scénarios d’admission, 32 sélections exactes sur sept nuages, lien dur et trois cas de protection ; portes livrées 37 + 8 |
| [M5 natif](m5/README.md) | anciens témoins du lecteur : 18 résultats ; porte livrée : 21 cas ; 520 comptes exacts dans scan/Emit et six totaux injectés pour l’admission de capacité |
| [Juges M2/M4/M5/M6](juges/README.md) | 31 observations aux vrais points d’entrée ou étapes du pilote, outils externes simulés ; anciennes admissions refusées, nouveaux résidus M5/M6 |
| [JUG-EMST](emst/README.md) | 24 nuages de 1–32 sites, 64 CLI par mode, graphe complet indépendant, voies u64/u128, plateaux et empreintes ; anomalie PointId avec --ids |
| [Corrections T2](t2_corrections/README.md) | pin ultérieur 76adb : huit CLI sur les noms de trame, deux faits géométriques uniquement, relecture du contrat corrigé |

## Décisions de l’audit

**Huit clôtures** : `CST-0216`, `0217`, `0220`, `0222`, `0223`, puis `0227`, `0228`,
`0229`. Le registre précise les portées : raccord et admission des données, défaut
de cache reproduit, microbanc partagé M5, lecteur de transition, énoncés T2 avant code.
La garde de capacité M5 précède les réservations des tampons par l’exécuteur et
Scatter/Emit ; le diagnostic `stats` peut encore allouer avant cette garde.
Il ne s’agit donc pas d’une qualification du budget Session.

**`CST-0218` reste en cours.** La découpe en carré horizontal fermé est confirmée
sur les petits nuages, avec colonnes entières et IDs exacts. Les 69 anciennes
découpes, leurs manifestes, paquets et tables restent à régénérer. Un correctif
d’outil ne transforme pas les sorties historiques en nouvelles données conformes.

**Clôture `CST-0013` confirmée pour la livraison et le principe de JUG-EMST.**
L’ordre 1 est bien le lien simple exact de l’EMST, avec plateaux atomiques et
niveaux d²/4. Les deux calculs indépendants bornés sont conformes. Les neuf grands
vidages, mutants et campagnes sanitizer déclarés par le développeur ne sont pas
contre-certifiés ici ; leurs journaux propres à ce juge n’ont pas été retrouvés.
Cette limite ne signifie pas que les essais déclarés n’ont pas été exécutés.

**Nouveau `CST-0232`** : un même FULL synthétique de 586 octets contenant deux
PointId égaux est refusé sans `--ids`, mais déclaré identique avec un fichier
d’identifiants également dupliqués. Valider l’unicité des deux côtés. La géométrie
de cet exemple est correcte ; ce constat concerne l’admission du juge.

**`CST-0018` reste en cours**, malgré la correction des anciens témoins :

- M5 adopte encore des prises déclarées identiques avec `missing=1` ou empreintes
  différentes ; il accepte aussi les deux grands livres hôte absents et des
  prises sanitizer sans valeurs pour la répétition demandée.
- La relecture M6 accepte une provenance vide, une isolation contredite par le
  code et la liste des processus, ou un refus explicite laissé dans le reçu.

Chaque mutation part d’un contrôle conforme. Il faut valider les schémas et
recouper les champs obligatoires, puis graver ces refus. Aucune campagne réelle
n’est déclarée fausse à partir de ces fichiers synthétiques. Les défauts G1 du
reçu précédent restent distincts ; le nouveau bras de `151d4b6ec` n’est pas audité
dans ce lot.

## Conservation et limites

Les sources de chaque sous-reçu sont comparées à son propre pin et hachées
avant/après. Les résultats normal/`-O` concordent ; seules les données agrégées
utiles sont conservées, une fois. Toute normalisation de sortie est décrite dans
le sous-reçu concerné. Les compilations sont de petites unités Release ; leurs
dépendances locales et binaires temporaires sont identifiés. Aucun binaire,
arbre de sources copié, payload sous licence ou identité de compte conservé.

`verification.json` et `SHA256SUMS` ferment les empreintes et l’intégration, sans
constituer une qualification sémantique supplémentaire. Aucun GCP, recalcul
LiDAR, sanitizer, matrice native, contrat de vitesse ou FULL à l’échelle acquis.
