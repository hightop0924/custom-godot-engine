
#include "player_handler.h"

void PlayerHandler::_notification(int p_what)
{
	switch (p_what)
	{
		case NOTIFICATION_READY:
		{
			print_line("Notification was reached");
			break;
		}
		default:
		break;
	};
	print_line("Animation player exists rigging dock");
	if (Engine::get_singleton()->is_editor_hint())
	{
		
		print_line("Started from the editor viewport");
		
	}
}

PlayerHandler::PlayerHandler()
{
	
}

PlayerHandler::~PlayerHandler()
{

}


