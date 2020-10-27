# Bor-Bor Zan Online

Author: Jianxiang Li, Zhengyang Xia

Design: A turn-based strategy battle royale game where players can either strike early or accumulate MP for heavier attacks.

Networking: The message transmitted between the client and server only contains the players' control inputs. Each client transmits 3 bytes to the server in its update() function, representing its movements in x/y direction and its action of that turn. The server saves inputs from all the players and formats them into a string which is sent to each client. The message also includes each player's id and the total number of players in the game. Each part of the server message is separated by a '|' and each field in a player's information segment is separated by a ','. The client parses the received message and updates its local status of all the players accordingly.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Goal: Beat all other players and survive to be the last one standing.

Basic Rules: 

Once we have all the players joining the game, the game starts.\
Each turn every player will have 15s (simultaneously) to choose a action to take.
The actions that player can take includes moving, making an attack, defending, charging MP, and do nothing.

Moving (WASD):
Move to an adjacent tile. If two players landed on the same tile in the same turn, one with the highest MP wins, and the lower MP will be deducted from the higher MP. In the case of MP being the same, player with lower id wins. The moving kills are calculated pairwise in the order of player id.

Making an attack (Q):\
The attack consumes all the MP of the player.The range of attack will be (2 * MP + 1) * (2 * MP + 1) around the player, it will knock out players that didn't choose to defend in this turn.
If two players's attack range contains each other, one with larger range wins. If they have same range, then both survives.

Defending (E):\
Defend any attack in this turn.

Charging (Space):\
Increase MP by one.

The game map dimension will be shrinking by 1 every 3 turns, so player cannot just run around an avoid attacks.

Click mouse to grab the camera, and camera will move following the mouse movement. Press Esc to ungrab mouse.

Sources:

Scene Modified from https://github.com/Chipxiang/Dangerous-Midnight-Treat/blob/master/scenes/game_map.blend

This game was built with [NEST](NEST.md).

