# Prépublication T1-b : passage CUDA/CPU et ressources

7 octobre 2026. Lecture statique d’un prototype local encore modifié, hors `main`.
Le commit de base est `eddf181b9eb7da70668872dec53d8012deec19bf` ; **seuls les hashes complets de
[capture.json](capture.json) identifient les octets audités**. Aucun build, test natif,
appel CUDA/cloud ou accès aux données LiDAR n’a été exécuté. Aucune qualification
produit, numérique u24/u32, FULL ou performance n’en découle.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.

1. **Propriété des ressources sur erreur.** Dans `src/catalogue/device_cuda.cu:92–102`,
   après `cudaMalloc` réussi, les refus de copie/synchronisation sortent avant adoption
   de `fresh`. Ce pointeur brut n’a aucun nettoyage de sortie, alors que sa réservation
   locale est rendue. Dans `stage():116–128`, un refus `cudaMallocHost` après réservation
   conserve au contraire la réservation membre, sans mémoire épinglée obtenue.
   Une reprise directe de `stage()` fait bien `reset()` avant `reserve()` : nous ne
   concluons pas à un blocage permanent pour la même trame. Une allocation intervenant
   avant ce prochain `stage()` peut cependant subir cette charge sans allocation ; la
   croissance XYZ d’une nouvelle trame précède justement le prochain upload.
   Correction ciblée : propriétaire temporaire et rollback, puis adoption au succès.
   Raccord possible à CST-0003 ou à un futur constat après publication ; aucun état du
   registre n’est modifié ici. Effets déduits du contrôle de flux, sans panne injectée.

2. **Réécritures physiques affichées à zéro.** `leaves_rewritten` est initialisé à zéro
   et jamais rempli par `device_diagnostics()`. Pourtant `ReplayKernel` réexécute les
   feuilles GPU de plus de 64 émissions ; les reprises CPU ont aussi un compteur
   `stage.totals().rewritten` qui n’est pas agrégé. `replayed_leaves` désigne les feuilles
   reprises sur CPU, pas toutes ces réécritures. Il faut compter les deux contributions.
   Leur temps reste inclus dans le chrono global : le défaut porte sur le diagnostic.

3. **Voie hybride et frontière de chrono.** J3 GPU est limité aux feuilles de 32 sites
   et d’étendue locale de 16 bits au plus. Les autres sont compactées, rapatriées et
   rejouées exactement sur CPU, puis remontées. Les plafonds de 256 candidats et 64 sites
   de coquille subsistent. Une compilation u24/u32 ne signifie pas traitement intégral
   GPU ; aucun défaut numérique n’est déduit de cette reprise exacte. Le `wall_ns` du
   probe inclut XYZ rechargé à chaque appel, reprises, transferts, finition et adoption.
   Il exclut préparation Cloud/Pool, ouverture du contexte, digest/export et destruction
   du catalogue rendu. Aucun nouveau temps FULL n’est publié ici.

Les références de lignes, extraits minimaux, tailles et hashes sont dans `capture.json`.
La double lecture de toutes les sources y est stable ; aucune copie intégrale du
prototype ni aucun chemin personnel n’est conservé. Vérification de l’archive :

```sh
sha256sum -c SHA256SUMS
```
