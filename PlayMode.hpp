#include "Mode.hpp"

#include "Connection.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	Scene scene;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;
	Scene::Camera* camera = nullptr;


	uint8_t defend;
	uint8_t attack;
	uint8_t magic_attack;
	uint8_t charge;

	bool pressed = false;

};
