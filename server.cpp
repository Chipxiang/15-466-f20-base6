
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

	size_t numPlayer = 4;
	//------------ main loop ------------
	constexpr float ServerTick = 15.0f; //TODO: set a server tick that makes sense for your game

	//server state:

	//per-client state:
	struct PlayerInfo {
		PlayerInfo() {
			static uint32_t next_player_id = 0;
			// name = "Player" + std::to_string(next_player_id);
			id = std::to_string(next_player_id);
			next_player_id += 1;
			action = 0;
			mov_x = 0;
			mov_y = 0;
		}
		std::string id;

		int action;
		int mov_x;
		int mov_y;

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
					// std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();
					if (players.size() == numPlayer){
						//look up in players list:
						auto f = players.find(c);
						assert(f != players.end());
						PlayerInfo &player = f->second;

						//handle messages from client:
						//TODO: update for the sorts of messages your clients send
						while (c->recv_buffer.size() >= 4) {
							//expecting five-byte messages 'b' (left count) (right count) (down count) (up count)
							char type = c->recv_buffer[0];
							if (type != 'b') {
								std::cout << " message of non-'b' type received from client! " << type << std::endl;
								//shut down client connection:
								c->close();
								return;
							}

							player.mov_x = c->recv_buffer[1];
							player.mov_y = c->recv_buffer[2];
							player.action = c->recv_buffer[3];

							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);
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
				
				
				status_message += player.id+","+std::to_string(player.mov_x) + "," + std::to_string(player.mov_y) + "," +std::to_string(player.action);
				status_message += "|";
				player.action = 0;
				player.mov_x = 0;
				player.mov_y = 0;
			}
			//send updated game state to all clients
			//TODO: update for your game state
			for (auto &[c, player] : players) {
				(void)player; //work around "unused variable" warning on whatever g++ github actions uses
				//send an update starting with 'm', a 24-bit size, and a blob of text:
				c->send('m');
				std::string msg = player.id+"|"+std::to_string(numPlayer)+"|"+status_message;
				c->send(uint8_t(msg.size() >> 16));
				c->send(uint8_t((msg.size() >> 8) % 256));
				c->send(uint8_t(msg.size() % 256));
				c->send_buffer.insert(c->send_buffer.end(), msg.begin(), msg.end());
			}
		} else {
			for (auto &[c, player] : players) {
				(void)player; //work around "unused variable" warning on whatever g++ github actions uses
				//send an update starting with 'm', a 24-bit size, and a blob of text:
				std::string msg = player.id + "|" + std::to_string(numPlayer-players.size());
				c->send('w');
				c->send(uint8_t(msg.size() >> 16));
				c->send(uint8_t((msg.size() >> 8) % 256));
				c->send(uint8_t(msg.size() % 256));
				c->send_buffer.insert(c->send_buffer.end(), msg.begin(), msg.end());
			}
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
