# Export et relecture G4 — S2 par vagues uniquement

`readback.py` publie uniquement une session fermée, puis rejoue le protocole
S2 exact depuis les pièces privées originales. Il n'appelle ni GCP ni SSH.
Il peut préserver une capture échouée ; un hôte ou worker échoué ne devient
jamais un succès parce qu'un sous-test a produit un temps.

Les pièces publiées sont : journaux/JSON VM autorisés, manifeste et provenance,
reçus/intentions hôte, logs de garde expurgés, projection GCE confirmant
`TERMINATED` sur la génération arrêtée, résumé recalculé et inventaire SHA256.
Ni clé SSH, ni archive, ni nouvelle donnée KITTI, ni réponse OS Login brute
ne sont publiées. Les motifs de caviardage et la projection sont un port
explicite du helper historique cité dans la source, pas de sa logique FULL.

La relecture est **LIVE** : elle exige le snapshot privé, le paquet et
les fichiers originaux liés dans `PRIVATE_LINKS.json`. Chaque contenu publié
est régénéré et comparé octet par octet ; modifier une source, un reçu,
un fichier publié ou l'inventaire invalide la relecture. L'absence des pièces
privées est une absence de preuve LIVE, pas une autorité archive autonome.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_cuda_g4_readback_20260927/readback.py \
  --export morsehgp3D_v9/receipts/q34_cuda_g4_20260927/r1 \
  --session /workspaces/E-HGP/build/v9-g4-cuda-waves-session-20260927-r1 \
  --package /workspaces/E-HGP/build/v9-g4-cuda-waves-package-20260927-r1

python3 -B morsehgp3D_v9/audits/b_q34_cuda_g4_readback_20260927/readback.py \
  --readback morsehgp3D_v9/receipts/q34_cuda_g4_20260927/r1
```

Ajouter `-O` pour la seconde lecture. `selftest.py` vérifie hors cloud les
allers-retours temporaires succès/échec, mutations de pièces, noms interdits,
caviardage et arrêt de la génération exacte. Ses reçus sont synthétiques et
ne sont jamais une mesure GPU ; les répertoires temporaires sont supprimés.

Portée des chiffres : les compteurs `gate.runs`, `queries`, `survivors`
comptent les boucles **portables**, pas les lancements CUDA. Le device exécute
Q7/Q257 sur chaque cas, plus Q1 pour E≤4. Le total construction→sortie S2
additionne préparation CPU, copie snapshot et runner ; il ne comprend pas
la destruction tardive de Prepared/snapshot, mélangée aux autres destructions
dans `times_ms.destruction`. Les libérations internes du runner sont payées.
Le temps global comprend aussi front, référence de contrôle, comparaison,
entrée/index et destruction finale. Aucun de ces chiffres n'est un temps FULL.

Le temps d'allocation GCE n'est pas une facture : aucun prix ni montant
n'est estimé sans export de facturation ou tarif vérifié.
