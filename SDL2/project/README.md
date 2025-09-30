# ⚽ Tiny Football

A minimalist 2D football/soccer game built with SDL2, featuring FIFA-style gameplay mechanics and AI positioning.

## 🎮 Features

### Core Gameplay
- **2v2 Football Match**: Each team has 2 players
- **FIFA-style Controls**: One active player per team, switch between teammates
- **Smart AI Positioning**: Inactive players automatically position themselves based on ball location
- **Realistic Ball Physics**: Friction, collision detection, and momentum-based movement
- **Kick System**: Players can kick the ball when in range
- **Score Tracking**: Real-time scoreboard with goal detection

### Game Mechanics
- **Team-based Switching**: Each team independently switches between their players
- **Collision Physics**: Ball reflects realistically off players and screen boundaries
- **Visual Feedback**: Active players highlighted with yellow borders and kick range indicators
- **Debug Mode**: View detailed game state information

## 🎯 Controls

### Team Blue (Left Side)
- **Movement**: `WASD` keys
- **Kick**: `Q` key (when near ball)
- **Switch Player**: `Q + Tab`

### Team Orange (Right Side)
- **Movement**: `Arrow Keys` (↑↓←→)
- **Kick**: `Right Shift` (when near ball)
- **Switch Player**: `P + Tab`

### Global Controls
- **F1**: Toggle debug information
- **F2**: Toggle AI mode for Player 3
- **ESC**: Exit game
- **1-4**: Direct player selection (testing mode)

## 🚀 Installation & Setup

### Prerequisites
- SDL2 development libraries
- SDL2_ttf for text rendering
- C++17 compatible compiler

### Linux/Mac Installation
```bash
# Install SDL2 (Ubuntu/Debian)
sudo apt-get install libsdl2-dev libsdl2-ttf-dev

# Install SDL2 (macOS with Homebrew)
brew install sdl2 sdl2_ttf

# Compile the game
g++ main.cpp -o tinyfootball `sdl2-config --cflags --libs` -lSDL2_ttf -std=c++17

# Run the game
./tinyfootball
```

### Windows Installation (MinGW)
```bash
# Download SDL2 development libraries from libsdl.org
# Extract to your preferred directory

# Compile (adjust paths as needed)
g++ main.cpp -o tinyfootball.exe -I/path/to/SDL2/include -L/path/to/SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -std=c++17

# Run the game
./tinyfootball.exe
```

## 🎲 How to Play

1. **Start the Game**: Run the executable - each team starts with one active player
2. **Move & Kick**: Use your team's controls to move and kick the ball
3. **Switch Players**: Use team-specific Tab combinations to switch between your players
4. **Score Goals**: Get the ball into the opponent's goal area (left/right edges)
5. **Win**: First team to score the most goals wins!

### Gameplay Tips
- **Positioning**: Inactive players automatically position themselves - defensive when ball is in your half, offensive when it's in opponent's half
- **Kick Range**: Yellow circle appears when you can kick the ball
- **Team Strategy**: Switch between players to maintain good field coverage
- **Ball Physics**: Use angles and momentum for better ball control

## 🏗️ Technical Details

### Architecture
- **Single-file Implementation**: All game logic in `main.cpp` for simplicity
- **Object-oriented Design**: Separate classes for Ball, Player, Game, and ScoreBoard
- **Real-time Physics**: Delta-time based movement and collision detection
- **Modular AI**: Separate positioning logic for defensive and offensive play

### Key Components
- **Ball Class**: Physics simulation with friction and collision
- **Player Class**: Movement, AI, and kick mechanics
- **Game Class**: Main game loop, input handling, and rendering
- **Collision System**: Rectangle-based collision with realistic ball reflection

## 🎯 BTL2 Compliance

This game satisfies all BTL2 "Tiny Football" requirements:

### Mandatory Features (10/10 points)
- ✅ **Multiple Players**: 4 players (2 per team)
- ✅ **Dual Input Schemes**: WASD and Arrow Keys
- ✅ **Player Switching**: Tab-based team player switching
- ✅ **Ball-Wall Collision**: Realistic physics with proper reflection angles
- ✅ **Ball-Player Collision**: Interactive ball physics
- ✅ **Score System**: Goal detection and scoreboard display
- ✅ **2-Player Mode**: Independent team controls

### Bonus Features
- ✅ **Player vs AI**: Toggle AI mode for single-player experience
- ✅ **Advanced Positioning**: FIFA-style teammate AI positioning
- ✅ **Enhanced Physics**: Friction and momentum-based ball movement

## 🔧 Development

### Project Structure
```
tiny-football/
├── main.cpp              # Complete game implementation
├── README.md            # This file
├── screenshot.png       # Game screenshot
└── DejaVuSans.ttf      # Font file (optional)
```

### Building from Source
The entire game is contained in a single `main.cpp` file for easy compilation and distribution. No external assets required except for optional font files.

### Customization
- **Screen Size**: Modify `SCREEN_W` and `SCREEN_H` constants
- **Player Speed**: Adjust `Player::speed` values
- **Ball Physics**: Tune friction and kick force parameters
- **AI Behavior**: Modify `update_positioning()` logic

## 📝 License

This project is open source. Feel free to modify and distribute according to your needs.

**Enjoy playing Tiny Football! ⚽🏆**
