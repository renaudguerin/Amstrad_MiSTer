# Questions R7 et mesure S — brouillon non envoyé

Ces questions complètent les questions C0/R5 existantes. Aucun nouveau modèle
n'est retenu à partir du seul accord entre émulateurs.

## R7 en fin de ligne sans changement de C4

Sur CRTC 1,§16.4.2p171 indique qu'une écriture R7=C4 déclenche immédiatement
la VSYNC et comptabilise la ligne partielle. Le B6/019D photographié distingue
la quatrième et la cinquième µs d'un OUTI, mais son cas terminal mélange la
fin de C0 et le changement de C4.

Pouvez-vous confirmer le cas où l'écriture arrive près de C0=R0, mais sur une
ligne C9 non terminale : C4 reste égal à la nouvelle valeur de R7 après le
retour de C0 à0. Doit-on observer une VSYNC de durée normale, incluant la ligne
partielle, ou existe-t-il une restriction liée à C0 indépendamment de C4 ?

Notre nouveau petit programme démarre normalement depuis une disquette, sans
modifier PC/compteurs depuis l'hôte. Des mots VRAM portant leur propre adresse
identifient les lignes affichées voisines : VMA bas=400, bitsC9 de0à1, avec
R4=38/R9=7/R5=R8=0. AmSpirit1.17.1/CRTC1A renvoie néanmoins0 lors de la lecture
PPI suivante. Cela qualifie une observation logicielle, pas le résultat matériel.
Les images LIVE et le programme sont conservés et peuvent accompagner la
question. La phase précise du bus n'est pas déduite des coordonnées du cadre.

Un essai normal vérifié du même programme dans CPCSyntaxError 0.12, sous ROMs
UK avec nom BIN explicite, charge les 623 octets attendus et renvoie 1. Sa trace
situe l'écriture à C0=0/C4=10/C9=1, à la granularité caractère de son ordonnanceur.
Cette divergence logicielle n'établit donc pas l'ouverture physique du bus.

Un modèle fondé seulement sur une comparaison plus tardive avec C4 devrait
préserver l'égalité dans ce cas non terminal. Nous ne souhaitons pas ajouter
une restriction C0 pour reproduire AmSpirit sans confirmation matérielle.

## Visibilité de C4 pour une écriture R7 près du changement de ligne

Un nouvel essai du B6/019D original dans CPCSyntaxError0.12 donne les six
résultats photographiés13/14/26/26/1E/1F. Son ordonnanceur avance le matériel
jusqu'à l'instant logique de l'OUTI avant l'écriture : sa trace place la
cinquième µs à C0/C4/C9=0/33/0 (égalité rejetée), la quatrième à63/32/7
(égalité acceptée). Cette observation explique l'accord de cet émulateur,
mais ne mesure pas l'ouverture physique de la comparaison.

Comment C4 devient-il visible à la comparaison provoquée par une écriture R7
autour de cette retenue, séparément du stockage de R7 et de l'évolution
ordinaire des compteurs ? Une comparaison systématiquement reportée au
prochain CLKEN ne convient pas à nos autres contraintes : dans le contrôle
optique OUT(C), le premier pixel noir attendu précède cette comparaison de
35horloges maîtres. Avec le transport inchangé, ce report produirait22/6
pour OUT(C)/OUTI, contre9/5 attendus. Il faut donc distinguer les conditions
d'admission près du changement de C4 du délai général de publication, sans
interdire le cas non terminal où C4 reste égal à R7.

Le §16.3p169 protège une VSYNC en cours si l'égalité C4/R7 change. Cette
protection s'applique-t-elle aussi dès le déclenchement pendant une écriture
R7 encore maintenue, si C4 change avant la fin de cette même écriture ? Le
résultat PPI ultérieur de B6 permet-il d'exclure une brève impulsion pendant
l'écriture, ou seulement une VSYNC qui se poursuit jusqu'à la lecture ? Nous
n'avons pas mesuré la broche VSYNC pendant cette écriture et ne retenons pas
de règle d'annulation provisoire.

Les [preuves et limites](../investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md)
conservent les deux contraintes ; aucun nouveau délai numérique n'est retenu.

## CS/02D3 : contexte vérifié

Une relecture indépendante Astra/Gemini, puis une vérification des glyphes
de la photographie par le coordinateur, donnent4E40. L'ancienne lecture4E30
était erronée : nous retirons donc la question d'une divergence numérique
photo/programme. Les branches CRTC1 disponibles en2.6/2.7 attendent toutes
deux4E40 ; cela n'identifie pas le binaire exact de la photographie.

Le build c16e7ec après correction IVM avait donné4E40 sur les deux parcours,
avec C5 inactif dès l'acquisition. Des captures ultérieures sur fdf348c montrent
pourtant0840 et0440 après préfixe. Le build actuelcb3a1a6 donne0640 après
le préfixe et4E40 en entrée directe, avec les deux chaînes de caractères
vérifiées par correspondance exacte avec la police du programme. AmSpirit1.17.1
passe maintenant le préfixe complet et fournit le compte brut4E40 avant
formatage. L'ancien état caché ae9 ne doit pas être transposé au cœur actuel.
Le mécanisme natif de cette réapparition reste à qualifier ; aucune tolérance
numérique ni remise à zéro spéciale de C5 n'en est déduite.
[État et preuves](../investigations/hardware-runs/shaker-s-current-2026-11-06.md).

## Référence CE_B/02CA

L’index public CE_B/02CA ne liste que les CRTC3/4 au4novembre. Existe-t-il
une référence CRTC1 pour cette seconde page, ou un état de sortie E à vérifier
avantP/S ?

Références : ACCC 1.11FR§16.4.2p171,§17.2.1p179 et§20.2p241;
photos Logon System B6 et CS. Les rapports datés conservent les images,
identités et limites; aucune de ces questions n'a été envoyée.
