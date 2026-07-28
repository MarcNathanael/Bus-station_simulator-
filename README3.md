Design & UX : une représentation 2D spatiale (une carte avec des points et des lignes pour les voitures qui se déplacent de la gare aux provinces et inversement) deux ligne par route

Je souhaite une une interface figée mais qui peut etre masque comme les interfaces des chat_bot( chat gpt , Gemini , Z.ai,...)

J'ai deja une carte (png)  a a utiliser comme maquette/fond et mettre la route dessus PNG image data, 1280 x 720, 8-bit/color RGBA, non-interlaced
 (Mais je sais oas comment tracer la route dessus )

Je n'ai pas encore definie un mapping pour la capitale et les provinces
Pour les bus(convois) , station, et et portaille j'ai telecharger des images
Je veux tracer les routes et il n'y auras pas de bus dessus mais l'icons du convois conserner lorsqu'il est en route

Contrôle du temps : Play ,Boutons vitesse , Pause ,bouton "Ajouter une plage interdite" 

Contrôle des règles métier : Je veux que l'utilisateur puisse modifier en live via des sliders ImGuides parametres critique comme m_seuil_remplissage_min , duree_franchissement_voiture ,espacement_min_entre_occupation_convois ,taille_max_convoi,duree_franchissement_voiture,frequence_planification , marge_duree_trajet


Outils d'injection : Comme dans le test_stress , je veux un petit panneau "Billetterie Manuelle" permettant de cliquer pour injecter une urgence VIP ou un groupe de passagers pour une destination précise afin de les suivre en particulier (pause aux formulaire d'injection)

Statistiques globales :Les metriques absolues afficher en permanence : Heure actuelle , Nombre de bus EN_ROUTE, Nombre de passagers en attente, État du portail,

Inspection : je veux qu'on puisse cliquer sur un convois affiché à l'écran pour ouvrir une fenêtre ImGui détaillant les voiture contenue dedans :son ID, sa coopérative, son état, son nombre de passagers et sa destination , son heure de depart et d'arrivers

Resolution par defaut 1280x720 et redimensionnable

utilisation du Dark Mode avec des couleurs d'accentuation spécifiques (ex: vert pour les départs, orange pour les retours, rouge pour les urgences) 

Visualisation de l'agenda du planificateur :
Une frise chronologique horizontale






