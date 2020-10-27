# (TODO: your game's title)

Author: Jianxiang Li, Zhengyang Xia

Design: A turn-based strategy battle royale game where players can either strike early or accumulate MP for heavier attacks.

Networking: The message transmitted between the client and server only contains the players' control inputs. Each client transmits 3 bytes to the server in its update() function, representing its movements in x/y direction and its action of that turn. The server saves inputs from all the players and formats them into a string which is sent to each client. The message also includes each player's id and the total number of players in the game. Each part of the server message is separated by a '|' and each field in a player's information segment is separated by a ','. The client parses the received message and updates its local status of all the players accordingly.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:
Goal: Beat all other players and survive to be the last one standing.

Basic Rules: 

Once we have all the players joining the game, the game starts.\
Each turn every player will have 20s(simultaneously) to choose a action to take.
The actions that player can take includes moving, making an attack, defending, charging MP, and do nothing.\
Making an attack:\


Sources: (TODO: list a source URL for any assets you did not create yourself. Make sure you have a license for the asset.)\

Scene Modified from https://github.com/Chipxiang/Dangerous-Midnight-Treat/blob/master/scenes/game_map.blend\
This game was built with [NEST](NEST.md).

