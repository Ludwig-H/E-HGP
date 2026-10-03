# Reprise développeur — audit et performance v11, 3 octobre2026

Sources produit96 fichiers de `70e494777` vérifiées byte-identiques au commit.
Les sources et archives complètes ne sont pas copiées dans ce reçu :
Git et les captures closes restent leurs propriétaires.

L’[audit courant](../../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md)
et les [contrats mathématiques](../../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md)
remplacent les deux anciens suivis de cet auteur, aux mêmes chemins.

## Ce qui est conservé

- `proof.json` : sources, modèles, résultats dérivés et portée des contrôles.
- `performance.json` : recoupe autonome de l’archive reuse1/121 membres,
  30 blobs Git exécutés et provenance ; mêmes XYZ/nombres de boules et
  cardinalités de forêts entre v10 et v11, pas comparaison de grands dumps.
- Quatre petits modèles indépendants : coupe catalogue, union directe de
  racines, deux cas de partage MEB de faces. Aucun import du produit.
- Deux premiers échecs du contrôle d’audit : paquet source LIVE absent ;
  reconstruction fautive d’I/U depuis(p,q), ignorant les coquilles étendues.
- Fermeture gardée `graph4` et confirmation en lecture seule d’absence
  de sa clé OS Login. Le retour1 de suppression est conservé, pas changé en0.

`reuse1` qualifie ae817d09e, pas70e ; 3339/3339 + ASan18 299/299 et29/29
FULL K5. Les gros dumps ont été supprimés après décodage initial ; leurs
hashes enregistrés ne sont pas des rehachages actuels. Le paquet source
historique symlinké vers `/tmp` est absent ; le lecteur LIVE original n’a
pas été rejoué. Le lecteur autonome de reprise recoupe des octets publics
conservés, avec ces limites explicites.

Les trois comparaisons v10→v11 sont historiques : profils u18/u21, troisième
passe chaude contre un processus neuf. Les premières passes v10 sont aussi
conservées, sans les transformer en paires randomisées. Buffer est distinct
du RSS. Aucun chiffre de Python n’est additionné au temps FULL moteur.

## Rejouer les preuves bornées

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/reprise_performance/check.py
python3 -B -O morsehgp3D_v11/receipts/developpement_20261003/reprise_performance/check.py
```

Le lecteur vérifie les fichiers hachés et les sources Git figées, recoupe
les trois temps v11 depuis `reuse1/metrics.json`, puis exécute uniquement
les quatre modèles Python normal/−O. Aucune compilation, sonde native ou
commande cloud. Modèle catalogue :829 contrôles/36 configurations ; DSU :
900 cas et mutant first périmé rejeté. Les deux cas MEB exposent aussi des
régressions de tests de puissance. Aucun gain natif revendiqué.

Graph4 avait perdu son contrôleur local et n’avait ni DONE ni résultat
rapatrié. `--recover` du contrôleur byte-identique à celui de91890 certifie
la génération déjà arrêtée à06:57:50UTC ; clé privée supprimée, verrou
libéré. Suppression OS Login : « Cannot find requested SSH key », code1 ;
lecture du profil07:02:45UTC : zéro clé correspondant au payload public.
Aucune VM ni campagne native nouvelle lancée. Les résultats graph4 restent
non rapatriés ; la prochaine session devra traiter cette récupération
comme un travail distinct, sans requalifier une campagne par son seul arrêt.
