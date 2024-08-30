# Arena

Arena is a cooperative multi-player game where you fight monsters, level up your hero, and protect your base.

## Playing Arena

- The host must ensure that their firewall allows connections on TCP port 53000
- Every player must choose a name and a character type (Knight, Archer, ...) on the main screen
- The host can start the game once all players have joined
- Use WASD to move your character and arrow keys to move the camera
- Use the mouse to attack and cast spells
- You can also use shortcuts 0--9 to cast spells or use potions
- Killing creeps gives XP and gold
- You can buy potions and tomes using gold in the shop (area of the map with railroad tracks)
- The pool of water regenerates HP
- On a level up, your stats increase and you can upgrade one of your spells
- If a creep reaches your base, you lose a life
- If you lose all lives, you lose the game
- Kill the guards in all creep spawn points to win

## Building Arena

` git clone https://github.com/ahnonay/Arena `

` git submodule update --init --recursive `

On Linux, ensure SFML dependencies are installed as listed [here under the heading "Installing dependencies"](https://www.sfml-dev.org/tutorials/2.6/compile-with-cmake.php).

` cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./cmake-build-release `

` cmake --build ./cmake-build-release --target Arena -- -j 3 `

The binary is then found in `./cmake-build-release/Arena`.

### Docker

The binary created through this docker container should be portable to most modern Linux distributions.

` docker build -t arena-build:1.0 -f dockerfile.arena-build . `

` docker container create -i -t --name arena_builder arena-build:1.0`

` ./dockerbuild.sh `

### Windows

- Clone and setup the Arena repository as described above
- Install [MSYS2](https://www.msys2.org/) (or set up MinGW etc. some other way)
- In the MSYS MINGW64 shell, run `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`
- In the same shell, build Arena with the commands as described above

## Developing Arena

### General

- Main menu, lobby, and the actual game are implemented through different game states, see `src/GameStates`.
- The UI is implemented using an immediate-mode GUI paradigm. I.e. each UI element is created through a single function call, which simultaneously handles the rendering and the interaction. So for example, `imgui->button(...)` will draw a button on the screen and return `true` if that button is currently pressed. In each frame, before drawing UI elements, remember to call `imgui->prepare(...)` and also call `imgui->finish()` once all elements have been created.
- The game runs as a deterministic simulation with fixed time steps (the length of which is determined by `SIMULATION_TIME_STEP_MS` in `src/Constants.h`). The only information sent across the network are player actions (e.g. moving the character or casting a spell). Everything else is simulated on the players' machines. All "randomness" arises from random generators whose seeds are synchronized at the start of the game across all players.
- All values related to game logic must be exactly the same across all players' machines. So we only use integer variables or fixed point numbers (`FPMNum, FPMVector2`, see `src/FPMUtil.h`) for values in the simulation. Floating point numbers are only used when, e.g., converting simulation coordinates to screen coordinates for rendering.
- Important classes: Most game logic is found in the `Game` class. It also contains all the game objects, such as different characters, the tilemap etc. Player skills, levelups etc. are implemented in the `Player` class, whereas `Creep` describes the behavior of monsters. Both classes are subclasses of `Character`, which contains attributes common to all characters (such as HP) and logic on how to draw a character on screen. `CharacterContainer` contains all characters currently alive on the map and allows for accessing them by map coordinates (i.e., it is a kind of scene graph). `Tilemap` contains all information about static elements on the map (e.g., where are the walls, where is the respawn region...) and also handles drawing the map on screen. 

### Network

- Player actions are not immediately executed on the player's machine, but instead sent to the Server (see `GameClient::sendLocalActionsToServer` and `GameServer::receiveActionsFromClients`).
- Shortly before the next step in the game simulation is due, the server aggregates all the actions it received and converts them to events (see `GameServer::processActionsToEvents`, `GameServer::sendEventsToClients` and `GameClient::receiveEventsFromServer`)
- There are different classes for actions (`src/NetworkEvents/Action.h`) and events (`src/NetworkEvents/Event.h`). Events contain the simulation time step in which they will occur.
- When the next simulation step is due, the events for that step are executed in the `Game::simulate` procedure.
- Events are only executed if they are legal at that point in time. E.g., in Player::useSkill, we first check whether the player has enough MP etc. by calling Player::canUseSkill.

### Rendering

- There are three coordinate systems:
  - map coordinates (e.g., the positions of characters within the map; here, only fixed point numbers are used!)
  - world coordinates (the coordinates used for rendering objects; these are not(!) the same as map coordinates, because we use an isometric map. So the (0,0) map coordinate is not rendered in the upper left but the upper center)
  - UI/screen coordinates (here, (0,0) is always at the upper left of the game window, but the lower right can change if the window is resized)
- In the `Game` class, `viewWorld` is used when rendering objects in the game world, whereas `viewUI` is used for rendering the UI. To convert between map and world coordinates, use `tilemap->mapToWorld(...)` and `tilemap->worldToMap(...)`. To convert between world and UI coordinates, use `window->mapPixelToCoords(...)`.
- Although the game is 2D, we draw characters and map tiles with a z-coordinate which is then used for depth testing/z-ordering. SFML does not support this natively, so we need to write some extra classes like `Render/Sprite3.h` and rendering code (see `Game::render(...)`).

### Adding a new skill

## Credits

- Developed by [Michael Krause](http://mijael.de).
- Built using [SFML by Laurent Gomila & contributors](https://www.sfml-dev.org/), [fpm by Mike Lankamp & contributors](https://github.com/MikeLankamp/fpm), [tmxlite by Matt Marchant & contributors](https://github.com/fallahn/tmxlite)
- Character tilesets by [Reiner "Tiles" Prokein](https://www.reinerstilesets.de/)
- Dungeon tileset by [Clint Bellanger](https://opengameart.org/content/cave-tileset)
- Skill icons by [Pavel Kutejnikov](https://opengameart.org/content/22-skill-icons)
- Item icons by [Ravenmore](https://opengameart.org/content/fantasy-icon-pack-by-ravenmore-0)
- Font by [Raymond Larabie](https://typodermicfonts.com/public-domain/)
