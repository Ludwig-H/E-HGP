# QUESTION — palier C6 : supprimer le coût hôte de la couture série C

```text
phase=exploration_v6_hors_registre
backend=cuda_g4
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Conception AVANT tout code, comme pour C2. GCP non utilisé. Aucun chiffre de
gain n'est revendiqué : tout ce qui suit est une estimation à mesurer.

## 1. Le fait mesuré

Reçu `session_g4_20260901_b97f20ea4b8f_1788293187`, uniform 50 000, 48 fils :
l'étage de la route device coûte 7 717 ms, dont **kernels 154 ms**, H2D
171 ms, D2H 77 ms, **sérialisation du wire 2 641 ms** et **reconstruction
hôte 4 110 ms** — 88 % de l'étage est du code hôte monofil. Le gain net sur
le mur est donc plafonné à −10 %, alors que l'étage CPU qu'il remplace vaut
14 017 ms sur un mur de 59 011 ms (plafond d'Amdahl : 45,2 s, soit 1,306×).

Cause vérifiée par lecture : `append_ball_in` pousse 112 octets par boule,
octet par octet, dans un `std::vector<u8>` global de 2,42 Go sans réserve
(un fil) ; le rebuild refait 4,79 Go de `BallData` par `push_back` sans
réserve (un fil) ; 2,16 Go de sentinelles hôte sont pré-remplies puis
transférées à chaque lot. Pendant ce temps les 48 fils sont oisifs.

## 2. Ce que je propose (conception retenue sur trois)

**C6 « recouvrement »** : la couture devient un anneau de lots épinglés.

- Le **wire v1 est conservé octet pour octet** (112 o/boule, six candidats
  u32 hissés hôte, `host_wire_digest` de l'index) : `pack_ball_in` écrit à
  l'offset fixe `i·112` d'un tampon épinglé au lieu de pousser dans un
  vecteur global, sur 48 fils par plages. Une porte prouve
  `pack_ball_in ≡ append_ball_in` octet pour octet.
- Deux flux CUDA, anneau de deux lots (`cudaMemcpyAsync` H2D, `k_prefilter`,
  `k_census`, D2H asynchrone, `cudaEventRecord`) ; pendant que le device
  travaille sur le lot k, les 48 fils packent k+1 et reconstruisent k−1.
- La reconstruction devient **incrémentale par lot et parallèle**, dans des
  vecteurs réservés une fois ; la publication reste transactionnelle et dans
  l'ordre global (swap final, jamais un préfixe).
- Les kernels, `validate_ball_out`, `census_all`, `cand_idx = base + gid` et
  la validation avant reconstruction sont **intouchés**.

Estimation : étage ≈ 1,0–1,4 s au lieu de 7,7 s, soit un mur de route device
≈ 46 s contre 52,9 s aujourd'hui et 59,0 s en CPU (−21 % environ). À mesurer,
jamais déclaré.

Les deux autres conceptions étudiées sont **écartées** parce qu'elles
rouvrent des décisions que vous avez gravées : faire lire au device les
`BallCandidate` hôte à leur ABI (`GPU.md` : « jamais un memcpy de struct ABI
ni son padding » ; padding indéterminé transféré) ; ou remplacer la sortie du
RLE par une table SoA servie aux deux routes (change la route CPU, que la
couture promet « strictement inchangée », et rouvre le round-trip `BallIn`
du § 5.11).

## 3. Les quatre verrous que je vous demande de trancher

1. **Sentinelles.** `GPU.md` grave « sorties pré-remplies de sentinelles par
   l'hôte » et justifie le H2D (2,16 Go par exécution) par le risque de
   relire une allocation indéterminée. Les poser par un kernel device
   (`k_fill_sentinels`, mêmes valeurs) supprime ce transfert. Acceptez-vous
   ce changement de contrat, avec sa jumelle `gpu` et la scène composée
   `skip-fill + skip-ball-write` ? Sinon je garde le H2D et j'annonce le
   coût.
2. **Fermeture des chronomètres.** Le juge n'admet aujourd'hui que ± 0,4 ms
   d'arrondi. Sous recouvrement, les seaux hôte et device se chevauchent par
   construction. Je propose des **seaux disjoints** (pack, attente device,
   rebuild) plus un seau `attente_device_ms`, fermant l'étage à ± 0,4 ms
   exactement, et un `recouvrement_ms` calculé, jamais une tolérance
   relative. Est-ce la bonne forme ?
3. **Statut du stub.** `cuda_stub.hpp` grave « exécution strictement
   séquentielle — aucun test de course ici ». Un stub à exécution différée
   prouverait la discipline d'ordonnancement du harnais, pas le device. Je
   propose de le qualifier explicitement d'**auto-test du harnais** dans
   `GPU.md`, les mutants d'ordonnancement étant tués par un modèle et la
   parité ×5 à 50k restant la seule garde réelle. Acceptez-vous cette
   qualification, ou refusez-vous ces mutants ?
4. **Plancher séquentiel.** `std::vector<BallData>::resize(nb_total)` value
   initialise 4,84 Go en un fil (0,35–1,2 s selon la VM, 8–25 s à 10^6
   points). Le supprimer demande une arène à initialisation différée qui
   touche la signature des consommateurs. Je propose de le **mesurer
   séparément** (`resize_ms`) dans ce palier et de ne rien changer, en
   laissant la décision au palier d'échelle. D'accord ?

## 4. Ce que je livrerais avant toute session

Deux contre-sondes locales (réserve gardée `cands.size()·112` ; réserve
exacte ou deux passes pour `lsurv`/`lballs`) avec reçu local en compteurs,
jamais en temps ; la porte `pack ≡ append` ; les mutants de la couture avec
leurs portes à code 4 ; le juge versionné avec ses contre-fixtures ; la
grammaire v1 bit-identique en l'absence du jeton de couture, vérifiée en
revalidant le reçu `1788293187`.

## 5. Priorité annoncée

La directive exploitant du 2 septembre place l'**échelle** avant le gain de
mur : C6 vaut −21 % sur la fenêtre où le nuage tient en mémoire, il ne
déplace pas le mur de résidence (≈ 450k points à K=10, 0,39 Mo/point). Je
livre donc d'abord la mesure d'échelle (session G4 « échelle », demande de GO
séparée) et le palier de résidence ; C6 vient ensuite, sauf avis contraire de
votre part.
