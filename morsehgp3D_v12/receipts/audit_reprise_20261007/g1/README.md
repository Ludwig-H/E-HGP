# Reprise ciblée de MES-G1 : résidu de CST-0018

7 octobre 2026 ; pin `f601b36ace16bcc8f7ac9bc532e45ab5079f9667`, correction d'admission `8049bcc3990fad83358d6cc08c893393ce43df57`.
Microbanc hors produit, CPU de référence, u21, `public_status=not_claimed`. Aucun nuage réel ni GCP.

**Le résidu G1 de CST-0018 est clôturable dans sa portée d'admission.** Les deux anciens contournements sont
rejoués sur les mêmes octets, avec le binaire ancien identifié par le reçu initial et le binaire corrigé :

| Témoin : quatre sites alignés x=0,1,2,3, K2, F={0,3} | Ancien binaire | Pin corrigé |
| --- | ---: | ---: |
| route 2 honnête, census saturé | 0, une partie certifiée | 0, une partie certifiée |
| même partie annoncée route 1 du catalogue | **0, bilan accepté** | **3, refus avant bilan** |
| sélection `--ordres 9` | **0, bilan vide accepté** | **2, ordre hors de 2..K** |

Deux contrôles supplémentaires : `--ordres 2,9` est aussi refusé 2 ; annoncer la route 3 de census complet pour
cette partie saturée est refusé 3. La porte native bornée passe : quatre témoins géométriques, six témoins
d'admission, zéro écart ; elle exerce notamment la partie forgée mélangée à des parties honnêtes et le bilan vide.

## Pourquoi le refus est maintenant causal

Le catalogue synthétique contient les paires adjacentes (p=0) et celles de distance deux (p=1). La plus petite
boule de F={0,3} a deux sites strictement intérieurs, {1,2}, et q_min=2 ; p+q_min=4>K+1=3 : elle est hors Cat₂.
Sa route honnête est donc un census saturé, et la cible {1,2} est la population d'une naissance du catalogue.
Le lecteur de transition exact indépendant admet le catalogue complet utilisé par ce témoin.

Avant toute proportion, `admettre()` recalcule désormais la plus petite boule de chaque partie, le support local,
sa présence dans la table et les inclusions nécessaires. La route 1 forgée échoue sur ce contrôle ; elle n'est pas
simplement déplacée hors du dénominateur des proportions. Les routes de census doivent avoir le type exact annoncé,
et la cible de la route saturée est confrontée à la trace ou à la naissance terminale. La sélection d'ordre est
contrôlée séparément avant l'admission : la sélection partiellement invalide ne passe pas davantage.

Sur le fond mathématique, exhiber k sites **strictement** intérieurs suffit au saut : leurs distances au centre
courant sont toutes strictement inférieures au rayon r, donc leur plus petite boule a un rayon <r. Les points sur
la sphère ne fournissent pas cette décroissance. Cela justifie le certificat, pas sa rentabilité : la correction
du juge ne change pas le rejet chronométrique de G-L3 publié en session E.

## Construction, preuve et limites

Une seule unité C++ a été compilée, `mes_g1.cpp`, en C++20 Release avec avertissements stricts. L'archive v11
existante de SHA-256 `050532a95c322cc2d064eef5bfcba74b3920227f98dc21d157a0d675732cf7ca` est exactement celle du
premier audit ; elle n'a pas été reconstruite. Les 29 dépendances locales compilées sont comparées au pin avant
et après compilation et après rejeu. Le binaire ancien est vérifié contre son hash historique
`a5e5dbb83e08d36cd7c78965a4fb4fc333e5d31e3acbc89a523bdf548f2bb1f8`. Sources, deux binaires et archive sont fermés
par empreintes.

Chaque mode Python exécute cinq appels directs corrigés, trois anciens appels témoins, la porte native et le
lecteur de transition. La deuxième exécution réutilise le binaire compilé et en vérifie l'identité. Seul le champ
diagnostique `secondes` est retiré ; les sorties normal/`-O` sont identiques. Aucun ELF ni octet de nuage n'est
conservé dans ce reçu.

La clôture proposée ne porte ni sur les quatre grands vidages déclarés par le développeur, ni sur le nouveau bras
chronométrique de `151d4b6ec`, ni sur les quatre mutants compilés, ni sur les autres résidus M5/M6 de CST-0018 :
aucun de ces travaux n'est rejoué ici. Le reçu prouve précisément la fermeture des contournements d'admission
signalés par l'auditeur, avec leurs contrôles positifs et la porte bornée.

```sh
python -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/g1/check.py \
  --library /chemin/libmhgp11.a --build /tmp/construction-g1-neuve --baseline /chemin/ancien-g1
python -B -S -O morsehgp3D_v12/receipts/audit_reprise_20261007/g1/check.py \
  --library /chemin/libmhgp11.a --build /tmp/construction-g1-neuve --baseline /chemin/ancien-g1
```

Le binaire ancien est facultatif pour rejouer les contrôles corrigés ; son absence est publiée et ne rejoue pas la
comparaison causale historique. `normal.json`, `verification.json` et `SHA256SUMS` ferment cette capture.
