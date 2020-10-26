#include "Mode.hpp"

#include "Connection.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client& client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;
	std::string extract_first(std::string &message, std::string delimiter);

	Scene scene;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space, backspace;
	const float unit = 2.0f;
	const glm::vec3 level_offset = glm::vec3(0, 0, 1.5f);
	const glm::vec3 level_spacing = glm::vec3(0, 0, 0.2f);
	const glm::vec3 camera_offset = glm::vec3(0, -15, 10);
	const float pointer_max = 4.0f;
	const float pointer_min = 3.0f;

	int pointer_sign = -1;
	int xmax = 15;
	int xmin = 0;
	int ymax = 15;
	int ymin = 0;
	int max_player = 4;
	int myid = 0;
	int turn = -1;
	float turn_timer = 0.0f;

	struct Player {
		Scene::Transform* transform;
		int x = 0;
		int y = 0;
		bool is_alive = true;
		std::list<Scene::Drawable>::iterator drawable;
		int action = 0; // 0 for nothing, 1 for charge, 2 for attack, 3 for defend
		int level = 0;
		int mov_x = 0;
		int mov_y = 0;
		std::vector<std::list<Scene::Drawable>::iterator> level_drawables;
	};
	std::list<Scene::Drawable>::iterator cubes[16][16] = { {} };

	void levelup(int id, int count);
	void leveldown(int id, int count);
	void show_attack(int id, int range);
	void reset_attack(int id, int range);
	void show_defend(int id);
	void reset_defend(int id);
	void update_level();
	void camera_focus(int id);
	std::vector<Player> players;

	//last message from server:
	std::string server_message = "Connecting to server";

	//connection to server:
	Client& client;
	Scene::Camera* camera = nullptr;
	Scene::Transform* pointer = nullptr;
	int8_t action = 0; // 0 for nothing, 1 for charge, 2 for attack, 3 for defend
	bool pressed = false;
	int8_t mov_x = 0;
	int8_t mov_y = 0;

	bool waiting = true;

};
