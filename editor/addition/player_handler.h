#ifndef __PLAYER_HANDLER_H__
#define __PLAYER_HANDLER_H__

#include "scene/main/node.h"

class PlayerHandler : public Node
{
	GDCLASS(PlayerHandler, Node);
private:

protected:
	void _notification(int p_what);

public:
	
	PlayerHandler();
	~PlayerHandler();
};

#endif // __PLAYER_HANDLER_H__