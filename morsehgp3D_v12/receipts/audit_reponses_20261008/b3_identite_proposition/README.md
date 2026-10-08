# B3 : proposition de réadmission des preuves

8 octobre 2026. Proposition sur `545ed987e0f5a06dbb518fe42ed6c5f100f32770`, sans application au produit.
Le pilote et les bras livrés sont identiques octet pour octet à la capture initiale non commise
reproduite dans le reçu voisin `b3_identite_admission`. Ce reçu initial reste inchangé.

`proposition.patch` conserve les seuils, A/A, bootstrap et décisions après admission. Il ferme les
25 processus FULL d'identité (deux passes), les 10 résolutions (une passe G, ordres, digest et exit
réellement émis) et toutes les prises de temps. Chaque preuve exige un code entier nul, `valide`
exactement vrai, stdout/stderr présents et hachés, stderr vide, chemins locaux distincts, sortie
conforme à sa place. Les résumés sont comparés au brut puis recalculés. Le schéma futur devient
`ehgp.v12.t2d_b3_pilote.v2` ; les anciens rapports sont refusés, aucun code ni stderr n'est inventé.
Le producteur conserve désormais les codes réels de résolution et écrit les stderr de ces prises.

27 cas Python synthétiques : nominal adopté ; identité FULL réellement différente entre bras avec
résumé fidèle rejetée ; preuve absente, corrompue, hors configuration ou résumé contradictoire refusé.
Les 35 journaux d'identité supprimés étaient encore adoptés avant correction. Les booléens, chaînes
pour code et indicateur de validité, hash stderr absent/faux et ancien schéma sont refusés.
La fabrique utilise des objets de test publics, sans coordonnées, IDs, jeu sous licence ou mesures
réelles. La lecture G suit les émetteurs natifs épinglés ; le lecteur D6 est une référence de schéma,
pas une qualification héritée. 21 contrôles supplémentaires indépendants sont épinglés dans la capture, sur une copie figée
du pilote final avant et après exécution ; ils supplantent une première référence de revue affectée
par une lecture concurrente du hash.

Rejeu depuis ce dossier, avec le dépôt contenant le commit source :

```sh
python3 -B -S check.py --repo /workspaces/E-HGP
python3 -B -S -O check.py --repo /workspaces/E-HGP
```

Le lecteur extrait seulement six sources publiques et les références de schéma depuis Git, vérifie
leurs empreintes, applique le patch en répertoire temporaire et compare les 27 résultats. `--live`
exige aussi les bases identiques dans le worktree avant application éventuelle ; sinon réviser le pin.
Aucun programme natif, build, contrôleur ou appel GCP n'est exécuté. Le dépôt produit n'est pas modifié.

Limites : cette porte appartient au reçu ; elle n'est branchée ni à CTest ni à l'auto-test arithmétique
existant du pilote. Ce branchement reste à faire avant sa qualification régulière. Les noms, sites,
fils et passes restent liés aux places du rapport et aux constantes du pilote ; ce patch ne remplace
pas une admission indépendante de la cohorte, des commandes et des sources de la campagne réelle.
Le mode sans relecture `verifier_journaux=False`, utilisé par les auto-tests arithmétiques,
ne constitue pas une admission des primaires.
Aucun gain B3, verdict G4 réel ou qualification du moteur n'est acquis par ce reçu.
