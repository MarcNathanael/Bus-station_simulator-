```markdown
<div align="center">

# Simulateur événementiel de gestion d’une gare routière sous contraintes

**Un simulateur événementiel discret pour optimiser le trafic d’une gare routière à portail unique.**

C++17 · CMake · SFML 2 · ImGui · SQLite3

[Contexte](#contexte--problématique) · [Approche](#approche--concepts-utilisés) · [Fonctionnement](#fonctionnement-général) · [Architecture](#architecture) · [Algorithmes](#algorithmes) · [Installation](#installation--configuration) · [Résultats](#résultats-et-performances)

</div>

---

## Contexte & problématique

À Antananarivo (Madagascar), les gares routières sont régulièrement saturées car chaque coopérative décide librement de ses horaires.  
Avec un **portail unique**, les flux entrants et sortants se bloquent mutuellement, provoquant files d’attente, retards et surcoûts.

Le problème se résume à : **comment ordonnancer le passage de convois par un goulot d’étranglement, tout en respectant des contraintes économiques, de sécurité et de satisfaction client ?**

La simulation logicielle permet de :
- tester des scénarios sans risque,
- planifier intelligemment les créneaux,
- mesurer l’impact des décisions.

---

## Approche & concepts utilisés

L’algorithme cherche à trouver un compromis entre des objectifs contradictoires :  
- **rentabilité** (ne pas faire partir des bus presque vides),  
- **satisfaction client** (respect des fenêtres de patience, priorité aux urgences),  
- **fluidité physique** (aucun chevauchement au portail).

**Concepts mis en œuvre :**
- simulation événementielle discrète (tick = 1 minute),
- ordonnancement sous contraintes avec agenda temporel,
- file d’attente prioritaire (standard / urgent),
- optimisation locale guidée par une fonction de score,
- persistance write-behind (SQLite).

---

## Objectifs

| # | Objectif |
|---|---|
| 1 | Empêcher toute collision au portail – vérifié par les tests automatisés |
| 2 | Réduire le temps d’attente des passagers |
| 3 | Maximiser le taux de remplissage des bus (seuil de rentabilité) |
| 4 | Prioriser les urgences (délai critique) |
| 5 | Respecter les plages de fermeture (nuit, pointes) |
| 6 | Maintenir la stabilité sur le long terme (réinjection des demandes) |

Le système est conçu pour empêcher toute collision au portail grâce à la combinaison d’un verrou physique et d’un agenda temporel. Cette propriété est vérifiée par les tests automatisés lorsque ceux-ci sont exécutés avec succès.

---

## Fonctionnement général

### Moteur temporel
Le temps avance par **ticks de 1 minute** (1440 ticks par jour). À chaque tick, le simulateur met à jour les états, déclenche les événements et gère le portail.

### Cycle d’exécution simplifié
1. Les convois de retour partent des provinces à l’heure prévue.
2. Ils arrivent en province, débarquent et génèrent des demandes de retour.
3. À la gare, de nouveaux passagers sont générés (loi de Poisson).
4. Toutes les **30 minutes** (par défaut), le planificateur :
   - consolide les demandes,
   - forme des convois,
   - réserve des créneaux sur l’agenda du portail,
   - optimise le plan.
5. Dès qu’un convoi est prêt et que le portail est libre, il franchit le portail.

### Billetterie et urgences
La file d’attente classe les passagers selon leur fenêtre de patience.  
Un passager devient **urgent** lorsqu’il ne reste que 15 minutes (par défaut) avant sa limite. Les convois avec urgence contournent le seuil de rentabilité et sont prioritaires.

### Agenda du portail
Le portail est protégé par un agenda (ensemble des minutes réservées). Un créneau n’est accordé que si toutes les minutes nécessaires sont libres, avec une marge de sécurité. La libération est immédiate dès la fin du franchissement.

---

## Architecture

Le projet fournit une architecture modulaire permettant des extensions futures.  
Il est découpé en quatre modules indépendants :

- **gare_core** – entités métier (Voiture, Convoi, Destination, etc.)
- **gare_simulateur** – moteur temporel, planificateur, billetterie, générateur de demandes
- **gare_db** – persistance SQLite (écriture différée)
- **gare_ui** – interface graphique (SFML + ImGui)

Cette séparation permet de compiler un mode **headless** (sans UI) pour les tests automatisés.

```
Flux de données :
GenerateurDemandes → Billetterie → Planificateur → Simulateur → SQLite (Write-Behind)
```

---

## Algorithmes

### Principe général de l’ordonnancement
Le cœur du système est un **planificateur centralisé** qui décide, toutes les 30 minutes, de l’organisation des départs. Il suit une logique en plusieurs étapes, toujours identique :

1. **Collecte des demandes** – La billetterie remonte l’ensemble des passagers en attente, classés par destination. Chaque passager est soit *standard* (sa fenêtre de patience est encore confortable), soit *urgent* (il doit impérativement partir dans les 15 prochaines minutes).

2. **Regroupement par destination** – Pour chaque destination, le planificateur calcule le nombre total de voyageurs. Cela lui donne une vision claire de la pression exercée sur chaque ligne.

3. **Recherche de véhicules disponibles** – Il identifie les bus libres (présents à la gare, non affectés à un convoi). Ces bus sont triés par taux d’occupation actuel, de manière à optimiser le remplissage.

4. **Constitution des convois** – Les bus sont regroupés en convois (1 à 8 véhicules) pour mutualiser le passage du portail. Un convoi n’est créé que s’il est **justifié** :
   - Soit le taux de remplissage global atteint le seuil de rentabilité (par défaut 50 % de la capacité),
   - Soit il transporte au moins un passager urgent,
   - Soit la gare est en situation de pénurie (aucun bus libre restant) et doit rapatrier des véhicules.

5. **Réservation du portail** – Chaque convoi cherche un créneau libre dans l’agenda du portail. L’agenda est une simple liste des minutes occupées. Un créneau est accepté si toutes les minutes requises (durée du franchissement + marge de sécurité) sont disponibles et ne tombent pas dans une plage interdite. La recherche est optimisée : test en temps constant, puis balayage vers l’avant si nécessaire.  
   Si aucun créneau n’est trouvé, le planificateur tente de **décaler** un autre convoi standard (jamais un convoi urgent) pour libérer la place.

6. **Optimisation globale** – Une fois tous les convois placés, une phase d’amélioration locale est exécutée. Elle fusionne les convois proches et similaires, ajuste les horaires de quelques minutes et supprime les convois dont le remplissage est tombé sous un seuil critique (par défaut 20 %). Chaque modification n’est retenue que si elle améliore le **score global** du plan.

7. **Exécution** – À l’heure dite, le convoi franchit le portail. Le créneau est immédiatement libéré, prêt pour une nouvelle réservation.

### Boucle de rétroaction : les retours
Lorsqu’un bus arrive en province, les passagers y séjournent (quelques heures à deux jours). Ils génèrent ensuite une demande de retour, qui réintègre la billetterie. Ainsi le système boucle et régule automatiquement la flotte.

### Pourquoi cet algorithme est efficace
- **Réservation en O(1)** : l’agenda est un `unordered_set` de minutes, garantissant une vérification instantanée.
- **Priorité absolue aux urgences** : aucun compromis n’est fait sur les délais critiques, ce qui rend le système utilisable en conditions réelles.
- **Optimisation par score** : une fonction `Score = α·passagers − β·convois − γ·retard` (poids par défaut 10, 5, 1) permet d’arbitrer automatiquement entre volume, coût et ponctualité.
- **Recherche locale légère** : fusions, micro-décalages et suppressions améliorent le plan sans explosion combinatoire.

---

## Installation & configuration

### Dépendances
CMake ≥ 3.16, compilateur C++17, SQLite3, SFML 2.x  
(ImGui et ImGui-SFML sont téléchargés automatiquement).

### Compilation
```bash
cmake -S . -B build
cmake --build build -j4
./GareRoutiere
```

### Paramétrage
Tous les paramètres (flotte, destinations, seuils, plages interdites) sont chargés depuis des fichiers CSV dans `requirement/`.  
**Changer de ville ou de règles ne nécessite aucune recompilation.**

Quelques valeurs par défaut modifiables :
- nombre de véhicules, capacité, seuil de rentabilité (50 %),
- taille maximale d’un convoi (8), espacement minimum au portail (15 min),
- plages de fermeture (nuit 00h-06h, soirée 20h-21h).

---

## Résultats et performances

Les tests en conditions réelles (5 jours simulés) montrent **zéro collision au portail**, confirmant la robustesse du verrouillage et de l’agenda.

*Insérer ici une capture d’écran de l’interface en cours de simulation*  
![Capture d’écran de l’interface](placeholder.png)  
*Légende : Vue générale de l’interface avec la carte 2D, les convois en mouvement et les panneaux de contrôle.*

**Optimisations en place :**
- Réservation / libération en temps constant.
- Écriture différée (Dirty Bits) vers SQLite.
- Interpolation graphique 60 FPS sans surcoût de simulation.
- Architecture headless pour les tests.

---

## Tests

| Test | Validation |
|---|---|
| **Stress 5 jours** (`test_stress`) | Aucune collision, respect des règles, persistance OK |
| **Test fonctionnel** (`commit`) | Pause/vitesse, injections manuelles, live tuning, plages interdites |

Les tests s’exécutent en mode headless :
```bash
cmake --build build -j4
./build/tests/test_stress
./build/tests/commit
```

---

## Limites & perspectives

- Le portail unique reste le goulot physique ; le logiciel l’optimise mais ne peut pas le supprimer.
- Améliorations futures : support multi-portails, IA avancée (algorithmes génétiques), apprentissage automatique pour la prévision de demande, export de données, interface web.

---

## FAQ 

- **Peut-on éviter toutes les collisions ?** 
Oui, vérifié par les tests.
- **Pourquoi des bus attendent alors qu’ils pourraient partir ?** 
Pour respecter le seuil de rentabilité ou l’absence d’urgence.
- **Peut-on adapter le simulateur à une autre ville ?** 
Oui, il suffit de modifier les CSV et la carte.

---

## Glossaire

| Terme | Définition |
|---|---|
| **Tick** | Une minute de simulation |
| **Agenda** | Minutes réservées du portail |
| **Convoi** | Groupe de 1 à N voitures franchissant le portail ensemble |
| **Urgence** | Passager devant impérativement partir avant sa limite |
| **Write-Behind** | Écriture différée en base de données |

---

<div align="center">

*Simulateur événementiel de gestion d’une gare routière sous contraintes · C++17 / SFML / ImGui / SQLite*

</div>
```