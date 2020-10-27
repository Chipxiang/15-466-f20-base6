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

PlayMode::PlayMode(Client& client_) : client(client_) {
	scene = *game_scene;
	std::list<Scene::Drawable>::iterator it;
	int cube_count = 0;
	for (it = scene.drawables.begin(); it != scene.drawables.end(); it++) {
		if (it->transform->name.find("Player") == 0) {
			Player player = { it->transform, (int)it->transform->position.x / 2, (int)it->transform->position.y / 2, it };
			players.push_back(player);
		}
		else if (it->transform->name.find("Cube") == 0) {
			cubes[cube_count % 16][cube_count / 16] = it;
			cube_count++;
		}
		else if (it->transform->name == "Pointer") {
			pointer = it->transform;
		}
	}
	if (players.size() == 0) throw std::runtime_error("player not found.");
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	// std::cout << camera->transform->rotation.x << "," << camera->transform->rotation.y << "," << camera->transform->rotation.z << std::endl;
}
void PlayMode::camera_focus(int id) {
	camera->transform->position = players[id].transform->position + camera_offset;
}
void PlayMode::levelup(int id, int count) {
	for (int i = 0; i < count; i++) {
		players[id].level++;
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

void PlayMode::show_attack(int id, int range) {
	for (int i = std::max(xmin, players[id].x - range); i <= players[id].x + range && i <= xmax; i++) {
		for (int j = std::max(ymin, players[id].y - range); j <= players[id].y + range && j <= ymax; j++) {
			std::cout << i << "," << j << std::endl;
			if (cubes[i][j]->transform->position.z > -1) {
				continue;
			}
			std::cout << "Attack" + std::to_string(id) << std::endl;
			Mesh const& mesh = game_scene_meshes->lookup("Attack" + std::to_string(id));
			cubes[i][j]->pipeline.type = mesh.type;
			cubes[i][j]->pipeline.start = mesh.start;
			cubes[i][j]->pipeline.count = mesh.count;
		}
	}
}

void PlayMode::reset_attack(int id, int range) {
	for (int i = std::max(xmin, players[id].x - range); i <= players[id].x + range && i <= xmax; i++) {
		for (int j = std::max(ymin, players[id].y - range); j <= players[id].y + range && j <= ymax; j++) {
			Mesh const& mesh = game_scene_meshes->lookup("Plat");
			cubes[i][j]->pipeline.type = mesh.type;
			cubes[i][j]->pipeline.start = mesh.start;
			cubes[i][j]->pipeline.count = mesh.count;
		}
	}
}

void PlayMode::show_defend(int id) {
	glm::vec3 offset = glm::vec3(0, 0, 1);
	cubes[players[id].x][players[id].y]->transform->position += offset;
	players[id].transform->position += offset;
}

void PlayMode::reset_defend(int id) {
	glm::vec3 offset = glm::vec3(0, 0, -1);
	cubes[players[id].x][players[id].y]->transform->position += offset;
	players[id].transform->position += offset;
}
PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.repeat || !accept_input || pressed) {
			//ignore repeats
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			pressed = 1;
			mov_x = -1;
			mov_y = 0;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			pressed = 1;
			mov_x = 1;
			mov_y = 0;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			pressed = 1;
			mov_y = 1;
			mov_x = 0;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			pressed = 1;
			mov_y = -1;
			mov_x = 0;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_e) { //defend
			pressed = 1;
			action = 3;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) { //charge
			pressed = 1;
			action = 1;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_BACKSPACE) { //attack
			pressed = 1;
			action = 2;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
			backspace.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION) {
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

	// update pointer position
	pointer->position.x = players[myid].transform->position.x;
	pointer->position.y = players[myid].transform->position.y;
	pointer->position.z += pointer_sign * elapsed;

	if (pointer->position.z <= pointer_min) {
		pointer_sign = -pointer_sign;
		pointer->position.z = pointer_min;
	}
	else if (pointer->position.z >= pointer_max) {
		pointer_sign = -pointer_sign;
		pointer->position.z = pointer_max;
	}
	turn_timer -= elapsed;

	if (!accept_input && !waiting && turn != 0) {
		update_timer -= elapsed;
		// find the next one to update
		if (update_timer < 0) {
			if (updating_id != -1) {
				players[updating_id].updated = true;
				is_updating = false;
			}
			updating_id = -1;
			// Find one who did not take action.
			if (curr_action == 0) {
				for (int i = 0; i < max_player; i++) {
					if (i == myid)
						continue;
					if (players[i].updated) {
						continue;
					}
					if (players[i].action == 0) {
						updating_id = i;
						break;
					}
				}
				if (updating_id == -1) {
					curr_action = 1;
				}
			}
			// Find one who leveled up.
			if (updating_id == -1 && curr_action == 1) {
				for (int i = 0; i < max_player; i++) {
					if (i == myid)
						continue;
					if (players[i].updated) {
						continue;
					}
					if (players[i].action == 1) {
						updating_id = i;
						break;
					}
				}
				if (updating_id == -1) {
					curr_action = 3;
				}
			}
			// Find one who defended.
			if (updating_id == -1 && curr_action == 3) {
				for (int i = 0; i < max_player; i++) {
					if (i == myid)
						continue;
					if (players[i].updated) {
						continue;
					}
					if (players[i].action == 3) {
						updating_id = i;
						break;
					}
				}
				if (updating_id == -1) {
					curr_action = 2;
				}
			}
			// Find one who attacked.
			if (updating_id == -1 && curr_action == 2) {
				for (int i = 0; i < max_player; i++) {
					if (i == myid)
						continue;
					if (players[i].updated) {
						continue;
					}
					if (players[i].action == 2) {
						updating_id = i;
						break;
					}
				}
				if (updating_id == -1) {
					curr_action = 4;
				}
			}
			// All updated
			if (updating_id == -1) {
				for (int i = 0; i < max_player; i++) {
					if (players[i].action == 3) {
						reset_defend(i);
					}
				}
				update_timer = 0.0f;
				curr_action = 0;
				camera_focus(myid);
				accept_input = true;
			}
			else {
				update_timer = 2.0f;
			}
		}
		// keep updating the current updating player.
		else {
			if(!is_updating)
				camera_focus(updating_id);
			switch (curr_action)
			{
			case 1:
				if (update_timer < 1.2f && !is_updating) {
					levelup(updating_id, 1);
					is_updating = true;
				}
			case 2:
				if (update_timer < 1.2f && !is_updating) {
					int range = players[updating_id].level;
					show_attack(updating_id, range);
					for (int i=0; i<max_player; i++){
						if (i != updating_id && abs(players[i].x-players[updating_id].x) <= range &&
						abs(players[i].y - players[updating_id].y) <= range){
							if (players[i].action != 3 && !(players[i].action == 2 && players[i].level >= range)){
								players[i].is_alive = false;
								// show_death(i);
							}
						}
					}
					is_updating = true;
				}
				if (update_timer < 1.0f){
					players[updating_id].level = 0;
				}
			case 3:
				if (update_timer < 1.2f && !is_updating) {
					show_defend(updating_id);
					is_updating = true;
				}
			default:
				break;
			}
		}
	}


	if (pressed == 1) {
		//send a four-byte message of type 'b':
		std::cout << "Sent message" << std::endl;
		client.connections.back().send('b');
		client.connections.back().send(mov_x);
		client.connections.back().send(mov_y);
		client.connections.back().send(action);
		pressed = 2;
	}
	if (left.downs && players[myid].x > xmin) {
		players[myid].x--;
		players[myid].transform->position.x -= unit;
	}
	if (right.downs && players[myid].x < xmax) {
		players[myid].x++;
		players[myid].transform->position.x += unit;
	}
	if (up.downs && players[myid].y < ymax) {
		players[myid].y++;
		players[myid].transform->position.y += unit;
	}
	if (down.downs && players[myid].y > ymin) {
		players[myid].y--;
		players[myid].transform->position.y -= unit;
	}
	if (action == 3) {
		show_defend(myid);
	}
	if (action == 1) {
		levelup(myid, 1);
	}
	if (action == 2) {
		show_attack(myid, players[myid].level);

	}
	if (backspace.downs) {
		leveldown(myid, 1);
		show_attack(myid, 1);
	}
	//reset button press counters:
	mov_x = 0;
	mov_y = 0;
	action = 0;

	space.downs = 0;
	backspace.downs = 0;
	client.poll([this](Connection* c, Connection::Event event) {
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		}
		else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		}
		else {
			assert(event == Connection::OnRecv);
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
				camera->transform->position = players[myid].transform->position + camera_offset;
				pointer->position = players[myid].transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 4.0f, 0.0f) + players[myid].transform->position;

				if (type == 'w') {
					server_message = "Waiting for " + server_message + " more player(s).";
					accept_input = false;
				}

				if (type == 'm') {
					// int numPlayer = std::stoi(server_message.substr(0, server_message.find("|")));
					// server_message.erase(0, server_message.find("|")+1);
					waiting = false;
					turn_timer = 10.0f;
					turn++;
					accept_input = turn == 0;
					pressed = 0;
					max_player = std::stoi(extract_first(server_message, "|"));
					for (int i = 0; i < max_player; i++) {
						// std::string player_info = server_message.substr(0, server_message.find("|"));
						// server_message.erase(0, server_message.find("|")+1);
						std::string player_info = extract_first(server_message, "|");
						// players[i].is_alive = std::stoi(extract_first(player_info, ","));
						players[i].mov_x = std::stoi(extract_first(player_info, ","));
						players[i].mov_y = std::stoi(extract_first(player_info, ","));
						// players[i].energy = std::stoi(extract_first(player_info, ","));
						players[i].action = std::stoi(extract_first(player_info, ","));
						players[i].updated = false;
					}
					for (int i = 0; i < max_player; i++) {
						std::cout << players[i].x << " " << players[i].y << " " << players[i].level << std::endl;
					}
				}
			}
		}
		}, 0.0);
}

void PlayMode::update_level() {
	for (int i = 0; i < max_player; i++) {
		if (i == myid) continue;
		if (players[i].action == 1) {
			levelup(i, 1);
		}
	}
}

std::string PlayMode::extract_first(std::string& message, std::string delimiter) {
	std::string res = message.substr(0, message.find(delimiter));
	message.erase(0, message.find(delimiter) + delimiter.size());
	return res;
}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this

	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
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

		auto draw_text = [&](glm::vec2 const& at, std::string const& text, float H) {
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
		draw_text(glm::vec2(-aspect + 0.1f, -0.6f), "Update Timer " + std::to_string(update_timer), 0.09f);
		if (!waiting) draw_text(glm::vec2(-aspect + 0.1f, -0.9f), "Turn End in " + std::to_string((int)turn_timer), 0.09f);
	}
	GL_ERRORS();
}
