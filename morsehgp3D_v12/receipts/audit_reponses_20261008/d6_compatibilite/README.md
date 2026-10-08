# D6 : compatibilité livrée et admission du plan — 8 octobre 2026

CST-0018. Le lecteur livré en `d2f39fe82` est exactement la copie SHA256
`48f40fd6…` contre-rejouée ici (`git show` vérifié, pins complets dans
[capture.json](capture.json)). La proposition précédente `4d9e6a45…` refusait
la vraie sortie catalogue T1-b. Le développeur l’adapte aux champs `path=cpu`,
aux deux diagnostics de réparation et aux seize compteurs `device` nuls.
Les trois listes sont identiques au schéma commun livré en `781fbe8d1`.
Le petit [delta](adaptation_developpeur.patch) reconstruit cette adaptation
sur la proposition publiée ; aucune source ni fixture n’est dupliquée.

Rejeu Python normal et `-O` identique : la capture native publique CPU
(2 sites, K5/W1/P3) est admise ; chemin device, compteur device non nul ou
booléen et diagnostic manquant sont refusés. La forme native G actuelle
(8 sites, K5/W1/P2, digest puis exit) est admise. Les profils 24/32 sont
seulement des mutations de métadonnées valides : aucune qualification
numérique ne s’en déduit. Le futur schéma Gc reste hors de ce rejeu.

L’admission du **plan** reste distincte et non corrigée dans ce commit.
Les cinq témoins réutilisent le vrai `main`, la dilatation et les contrôles,
avec construction/prises simulées sur deux points synthétiques :

| Plan | Livré | Avec proposition d’admission déjà publiée |
|---|---|---|
| Référence u21/x1 présente | code 0, 6 prises | identique |
| Référence u21 impossible, u24 possible | code 0, ratio nul | refus 2, aucune prise |
| Aucune combinaison possible | ZeroDivisionError | refus 2, aucune prise |
| Profil répété | code 0, 3 étiquettes répétées | refus 2, aucune prise |
| Cas répété | code 0, 6 étiquettes répétées | refus 2, aucune prise |

La [proposition d’admission du plan](../../audit_d6_20261007/pilotage/proposition.patch)
s’applique toujours à la version livrée : `git apply --check`, application
sur copie temporaire puis cinq rejeux. Hash après combinaison :
`1ced91d172f977c57d5657deedb88cbc81c9621b23532607ff9b1b475e9a8dd4`.
[Résultats](results.json). La correction des sorties ne ferme donc pas
ce résidu du plan. Un cas nominal et quatre refus attendus sont distingués,
sans compter le contrôle positif comme une anomalie.

Reproduction depuis le dépôt :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d6_compatibilite/check.py --check
python3 -O -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d6_compatibilite/check.py --check
```

`--repo-root CHEMIN` permet les mêmes lectures depuis un autre checkout.
Le script vérifie les empreintes, reconstruit les versions en temporaire et
importe seulement les témoins du reçu antérieur ; aucun fichier publié
n’est modifié. Ce lot ne rejoue ni moteur natif, ni GCP, ni oracle géométrique,
ni la campagne locale ng00 K3 annoncée dans le message du commit.
Les contrôles structurels du format binaire restent ceux de la
[proposition antérieure](../../audit_reponses_20261007/d6_admission_proposition/README.md),
sans nouvelle preuve sémantique sur les corps exportés.
