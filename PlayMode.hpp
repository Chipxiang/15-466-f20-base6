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
	int xmax = 15;
	int xmin = 0;
	int ymax = 15;
	int ymin = 0;
	int max_player = 1;
	int myid = 0;
	int turn = -1;

	struct Player {
		bool is_alive = true;
		std::list<Scene::Drawable>::iterator drawable;
		int action; // -1 for defend, 0 for nothing, >0 for attack range
		Scene::Transform* transform;
		int x;
		int y;
		std::vector<std::list<Scene::Drawable>::iterator> level_drawables;
	};
	std::list<Scene::Drawable>::iterator cubes[16][16] = { {} };

	void levelup(int id, int count);
	void leveldown(int id, int count);
	void make_attack(int id, int range);

	std::vector<Player> players;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client& client;
	Scene::Camera* camera = nullptr;


	int defend;
	int attack;
	int charge;

	bool pressed = false;

};
