# Manifeste de provenance — reçu G4 WSPD du 13 août 2026

Le contre-audit `33df59d` relève que ce reçu ne pinçait ni le hash du
transcript, ni celui des sources exécutées. Les voici, en SHA-256.

```text
a6f86f0bfb3c1e437d0d26abb3340af42238152de517549db2f19f1a5b63d983  morsehgp3D_v3/prototype/wspd_front.hpp
4abc0b9869de66ab2f5891ea00cd460e534e5d830a4590a628af555ceb9d8574  morsehgp3D_v3/prototype/wspd_front_probe.cpp
9a4ab895b2f5f666abbd43da842cea545d6e0637e0e10441fbec8b67c3f434c3  morsehgp3D_v3/prototype/rect_front.hpp
c1daeda631d76e371e4a6b2c23ff6ae1bcfc84f1baa751d763ff7a02cfab26d2  morsehgp3D_v3/prototype/rect_front_probe.cpp
010c036948f1a6890726745d5e9b804fba2ccccd0a56456f92414989f6dabb4a  morsehgp3D_v3/CMakeLists.txt
2506f59614d01d449b5b00e67dfa61bab3d64a1574ac041583cac13910bc6033  gcp-migration/session_rect_front_g4.sh
13b01cf71bb613c466c1675cf1177a5f41f5782d9c74c8eb1c575a2a16a1d29d  g4_rf4.log
```

AVERTISSEMENT. Ces empreintes sont celles de l'arbre **au moment de la
rédaction**, non de la capture par la session : j'ai édité l'arbre pendant
que `tar` le lisait. La provenance de ce reçu n'est donc pas reproductible,
et c'est déclaré plutôt que masqué. Le `session.log` est ignoré par `*.log`
et n'appartient pas au commit ; seul son hash y figure.
