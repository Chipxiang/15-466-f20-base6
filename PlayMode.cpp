#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Load.hpp"
#include "Mesh.hpp"
#include "LitColorTextureProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint game_scene_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > game_scene_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("game_map.pnct"));
	game_scene_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
	});

Load< Scene > game_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("game_map.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		Mesh const& mesh = game_scene_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable& drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game_scene_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		});
	});

PlayMode::PlayMode(Client &client_) : client(client_) {
	scene = *game_scene;
	// std::list<Scene::Drawable>::iterator it;

	for (auto& transform : scene.transforms) {
		if (transform.name.find("Player") == 0) {
			// Player player = { &transform, (int)transform.position.x / 2, (int)transform.position.y / 2 };
			Player player = { &transform, 0, 0 };
			players.push_back(player);
		}
		// else if (transform.name.find("Cube") == 0) {
		// 	cube_vec.push_back(&transform);
		// }
		// else if (transform.name == "target") {
		// 	target = &transform;
		// }
	}
	if (players.size() == 0) throw std::runtime_error("player not found.");
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	// std::cout << camera->transform->position.x << "," << camera->transform->position.y << "," << camera->transform->position.z << std::endl;
	// std::cout << camera->transform->rotation.x << "," << camera->transform->rotation.y << "," << camera->transform->rotation.z << std::endl;
}
void PlayMode::levelup(int id, int count) {
	std::cout << "level up " << count << std::endl;
	for (int i = 0; i < count; i++) {
		Mesh const& mesh = game_scene_meshes->lookup("Level");

		// create new transform
		scene.transforms.emplace_back();
		Scene::Transform* t = &scene.transforms.back();
		t->name = "Level";
		t->parent = players[id].transform;
		t->position = (float)players[id].level_drawables.size() * level_spacing + level_offset;
		t->scale = glm::vec3(0.5f, 0.5f, 0.2f);
		Scene::Drawable drawable(t);
		drawable.pipeline = lit_color_texture_program_pipeline;
		drawable.pipeline.vao = game_scene_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
		scene.drawables.emplace_back(drawable);
		std::list<Scene::Drawable>::iterator it = scene.drawables.end();
		it--;
		players[id].level_drawables.push_back(it);
	}
}

void PlayMode::leveldown(int id, int count) {
	std::cout << "level down " << count << std::endl;
	for (int i = 0; i < count; i++) {
		if (players[id].level_drawables.size() > 0) {
			scene.drawables.erase(players[id].level_drawables.back());
			players[id].level_drawables.pop_back();
		}
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (pressed) return false;
		pressed = true;
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			mov_x = -1;
			mov_y = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			mov_x = 1;
			mov_y = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			mov_y = 1;
			mov_x = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			mov_y = -1;
			mov_x = 0;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) { //defend
			action = 3;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) { //charge
			space.downs += 1;
			space.pressed = true;
			action = 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_BACKSPACE) { //attack
			backspace.downs += 1;
			backspace.pressed = true;
			action = 2;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
			backspace.pressed = false;
			return true;
		}
	}else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->position.x += motion.x * 8.0f;
			camera->transform->position.y += motion.y * 8.0f;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//queue data for sending to server:
	//TODO: send something that makes sense for your game
	if (pressed) {
		//send a four-byte message of type 'b':
		client.connections.back().send('b');
		client.connections.back().send(mov_x);
		client.connections.back().send(mov_y);
		client.connections.back().send(action);
	}
	// if (left.downs && players[myid].x > xmin) {
	// 	players[myid].x--;
	// 	players[myid].transform->position.x -= unit;
	// }
	// if (right.downs && players[myid].x < xmax) {
	// 	players[myid].x++;
	// 	players[myid].transform->position.x += unit;
	// }
	// if (up.downs && players[myid].y < ymax) {
	// 	players[myid].y++;
	// 	players[myid].transform->position.y += unit;
	// }
	// if (down.downs && players[myid].y > ymin) {
	// 	players[myid].y--;
	// 	players[myid].transform->position.y -= unit;
	// }
	// if (space.downs) {
	// 	levelup(myid, 1);
	// }
	// if (backspace.downs) {
	// 	leveldown(myid, 1);
	// }
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	mov_x = 0;
	mov_y = 0;
	action = 0;
	pressed = false;

	space.downs = 0;
	backspace.downs = 0;
	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			// std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
			//expecting message(s) like 'm' + 3-byte length + length bytes of text:
			while (c->recv_buffer.size() >= 4) {
				// std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm' && type != 'w') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
				}
				uint32_t size = (
					(uint32_t(c->recv_buffer[1]) << 16) | (uint32_t(c->recv_buffer[2]) << 8) | (uint32_t(c->recv_buffer[3]))
				);
				if (c->recv_buffer.size() < 4 + size) break; //if whole message isn't here, can't process
				//whole message *is* here, so set current server message:
				server_message = std::string(c->recv_buffer.begin() + 4, c->recv_buffer.begin() + 4 + size);

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4 + size);
				// myid = std::stoi(server_message.substr(0, server_message.find("|")));
				// server_message.erase(0, server_message.find("|")+1);
				myid = std::stoi(extract_first(server_message, "|"));
				if (type == 'w'){
					server_message = "Waiting for " + server_message + " more player(s).";
				}
				
				if (type == 'm'){
					// int numPlayer = std::stoi(server_message.substr(0, server_message.find("|")));
					// server_message.erase(0, server_message.find("|")+1);
					waiting = false;
					max_player = std::stoi(extract_first(server_message, "|"));
					for (int i=0; i<max_player; i++){
						// std::string player_info = server_message.substr(0, server_message.find("|"));
						// server_message.erase(0, server_message.find("|")+1);
						std::string player_info = extract_first(server_message, "|");
						// players[i].is_alive = std::stoi(extract_first(player_info, ","));
						players[i].mov_x = std::stoi(extract_first(player_info, ","));
						players[i].mov_y = std::stoi(extract_first(player_info, ","));
						// players[i].energy = std::stoi(extract_first(player_info, ","));
						players[i].action = std::stoi(extract_first(player_info, ","));
					}

					for (int i=0; i<max_player; i++){
						std::cout << players[i].x << " " << players[i].y << " " << players[i].energy << std::endl;
					}
				}
			}
		}
	}, 0.0);

}

std::string PlayMode::extract_first(std::string &message, std::string delimiter){
	std::string res = message.substr(0, message.find(delimiter));
	message.erase(0, message.find(delimiter)+delimiter.size());
	return res;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this

	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text(glm::vec2(-aspect + 0.1f, 0.0f), server_message, 0.09f);

		draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "(press WASD to change your total)", 0.09f);
	}
	GL_ERRORS();
}
