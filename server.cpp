
#include "Connection.hpp"

#include "hex_dump.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>

int main(int argc, char **argv) {
#ifdef _WIN32
	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);

	int numPlayer = 2;
	//------------ main loop ------------
	constexpr float ServerTick = 10.0f; //TODO: set a server tick that makes sense for your game

	//server state:

	//per-client state:
	struct PlayerInfo {
		PlayerInfo() {
			static uint32_t next_player_id = 1;
			name = "Player" + std::to_string(next_player_id);
			x = next_player_id;
			y = next_player_id;
			next_player_id += 1;
			alive = true;
		}
		std::string name;

		uint32_t left_presses = 0;
		uint32_t right_presses = 0;
		uint32_t up_presses = 0;
		uint32_t down_presses = 0;
		bool defend = false;
		uint32_t attack = 0;

		int32_t x = 0;
		int32_t y = 0;
		uint32_t energy = 0;
		bool alive = true;

	};
	std::unordered_map< Connection *, PlayerInfo > players;

	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					//create some player info for them:
					players.emplace(c, PlayerInfo());


				} else if (evt == Connection::OnClose) {
					//client disconnected:

					//remove them from the players list:
					auto f = players.find(c);
					assert(f != players.end());
					players.erase(f);


				} else { assert(evt == Connection::OnRecv);
					//got data from client:
					std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();
					if (players.size() == numPlayer){
						//look up in players list:
						auto f = players.find(c);
						assert(f != players.end());
						PlayerInfo &player = f->second;

						//handle messages from client:
						//TODO: update for the sorts of messages your clients send
						while (c->recv_buffer.size() >= 8) {
							//expecting five-byte messages 'b' (left count) (right count) (down count) (up count)
							char type = c->recv_buffer[0];
							if (type != 'b') {
								std::cout << " message of non-'b' type received from client! " << type << std::endl;
								//shut down client connection:
								c->close();
								return;
							}

							player.left_presses = c->recv_buffer[1];
							player.right_presses = c->recv_buffer[2];
							player.down_presses = c->recv_buffer[3];
							player.up_presses = c->recv_buffer[4];
							player.energy += c->recv_buffer[5];
							player.defend = (c->recv_buffer[6] == 1);
							player.attack = c->recv_buffer[5];

							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 8);
						}
					}

					
				}
			}, remain);
		}
		std::string status_message = "";
		if (players.size() == numPlayer){

			//update current game state
			//TODO: replace with *your* game state update
		
			for (auto &[c, player] : players) {
				(void)c; //work around "unused variable" warning on whatever version of g++ github actions is running
				for (; player.left_presses > 0; --player.left_presses) {
					player.x -= 1;
				}
				for (; player.right_presses > 0; --player.right_presses) {
					player.x += 1;
				}
				for (; player.down_presses > 0; --player.down_presses) {
					player.y -= 1;
				}
				for (; player.up_presses > 0; --player.up_presses) {
					player.y += 1;
				}

				// if (status_message != "") status_message += " | ";
				// status_message += std::to_string(player.x) + ", " + std::to_string(player.y) + " (" + player.name + ")";
			}

			for (auto &[c1, player1] : players) {
				(void)c1; //work around "unused variable" warning on whatever version of g++ github actions is running
				if (player1.alive && player1.attack > 0){
					(void)c1;
					for (auto &[c2, player2] : players) {
						if (player1.name != player2.name && abs(player1.x-player2.x) <= player1.attack && abs(player1.y-player2.y) <= player1.attack && player1.attack > player2.attack && player2.defend == 0){
							player2.alive = false;
						}
					}
				}
				

				
			}
			//std::cout << status_message << std::endl; //DEBUG

			for (auto &[c, player] : players) {
				(void)c; //work around "unused variable" warning on whatever version of g++ github actions is running
				if (status_message != "") status_message += "|";
				status_message += player.name + "," +std::to_string(player.alive)+","+std::to_string(player.x) + "," + std::to_string(player.y) + "," +std::to_string(player.energy)+","+std::to_string(player.attack)+","+std::to_string(player.defend);
			}
			//send updated game state to all clients
			//TODO: update for your game state
		} else {
			status_message = "waiting for "+std::to_string(numPlayer-players.size())+" more player(s) to start.";
		}
		for (auto &[c, player] : players) {
			(void)player; //work around "unused variable" warning on whatever g++ github actions uses
			//send an update starting with 'm', a 24-bit size, and a blob of text:
			c->send('m');
			c->send(uint8_t(status_message.size() >> 16));
			c->send(uint8_t((status_message.size() >> 8) % 256));
			c->send(uint8_t(status_message.size() % 256));
			c->send_buffer.insert(c->send_buffer.end(), status_message.begin(), status_message.end());
		}
	}


	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
